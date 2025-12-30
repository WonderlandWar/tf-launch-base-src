//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client's CObjectSentrygun
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_tf_player.h"
#include "vgui_bitmapbutton.h"
#include "vgui/ILocalize.h"
#include "tf_fx_muzzleflash.h"
#include "eventlist.h"
#include "hintsystem.h"
#include <vgui_controls/ProgressBar.h>
#include "igameevents.h"

#include "c_obj_sentrygun.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

static void RecvProxy_BooleanToShieldLevel( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	// convert old boolean "m_bShielded" to uint32 "m_nShieldLevel"
	*(uint32*)pOut = ( pData->m_Value.m_Int != 0 ) ? 1 : 0;
}

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_SentryRocket, DT_TFProjectile_SentryRocket )

BEGIN_NETWORK_TABLE( C_TFProjectile_SentryRocket, DT_TFProjectile_SentryRocket )
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE_NOBASE( C_ObjectSentrygun, DT_SentrygunLocalData )
	RecvPropInt( RECVINFO(m_iKills) ),
	RecvPropInt( RECVINFO(m_iAssists) ),
END_NETWORK_TABLE()

IMPLEMENT_CLIENTCLASS_DT(C_ObjectSentrygun, DT_ObjectSentrygun, CObjectSentrygun)
	RecvPropInt( RECVINFO(m_iAmmoShells) ),
	RecvPropInt( RECVINFO(m_iAmmoRockets) ),
	RecvPropInt( RECVINFO(m_iState) ),
	RecvPropEHandle( RECVINFO( m_hEnemy ) ),
	RecvPropDataTable( "SentrygunLocalData", 0, 0, &REFERENCE_RECV_TABLE( DT_SentrygunLocalData ) ),
	RecvPropInt( RECVINFO(m_iUpgradeLevel) ),
	RecvPropInt( RECVINFO(m_iUpgradeMetal) ),
	RecvPropInt( RECVINFO(m_iUpgradeMetalRequired) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_ObjectSentrygun::C_ObjectSentrygun()
{
	m_iMaxAmmoShells = SENTRYGUN_MAX_SHELLS_1;
	m_bNearMiss = false;
	m_flNextNearMissCheck = 0.f;
	m_iOldUpgradeLevel = 0;

	m_iOldModelIndex = 0;
}


void C_ObjectSentrygun::GetAmmoCount( int &iShells, int &iMaxShells, int &iRockets, int & iMaxRockets )
{
	iShells = m_iAmmoShells;
	iMaxShells = m_iMaxAmmoShells;
	iRockets = m_iAmmoRockets;
	iMaxRockets = SENTRYGUN_MAX_ROCKETS;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::UpgradeLevelChanged()
{
	switch( m_iUpgradeLevel )
	{
	case 1:	
		{ 
			VectorCopy( SENTRYGUN_EYE_OFFSET_LEVEL_1, m_vecViewOffset );
			m_iMaxAmmoShells = SENTRYGUN_MAX_SHELLS_1;
			break;
		}
	case 2:	
		{ 
			VectorCopy( SENTRYGUN_EYE_OFFSET_LEVEL_2, m_vecViewOffset );
			m_iMaxAmmoShells = SENTRYGUN_MAX_SHELLS_2;
			break;
		}
	case 3:	
		{ 
			VectorCopy( SENTRYGUN_EYE_OFFSET_LEVEL_3, m_vecViewOffset );
			m_iMaxAmmoShells = SENTRYGUN_MAX_SHELLS_3;
			break;
		}
	default: 
		{ 
			Assert( 0 ); 
			break;
		}
	}

	// Because the bounding box size changes when upgrading, force the shadow to be reprojected using the new bounds
	g_pClientShadowMgr->AddToDirtyShadowList( this, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_iOldBodygroups = GetBody();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	// intercept bodygroup sets from the server
	// we aren't clientsideanimating, but we don't want the server setting our
	// bodygroup while we are placing
	if ( m_iOldBodygroups != GetBody() )
	{
		if ( IsPlacing() )
		{
			m_nBody = m_iOldBodygroups;
		}
	}

	if ( GetModelIndex() != m_iOldModelIndex )
	{
		m_iOldModelIndex = GetModelIndex();
	}

	if ( m_iOldUpgradeLevel != m_iUpgradeLevel )
	{
		UpgradeLevelChanged();
		m_iOldUpgradeLevel = m_iUpgradeLevel;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::GetTargetIDDataString( OUT_Z_BYTECAP(iMaxLenInBytes) wchar_t *sDataString, int iMaxLenInBytes )
{
	Assert( iMaxLenInBytes >= sizeof(sDataString[0]) );
	sDataString[0] = '\0';

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	// Sentryguns have models for each level, so we don't show it in their target ID.
	bool bShowLevel = ( GetType() != OBJ_SENTRYGUN );

	wchar_t wszLevel[32];
	if ( bShowLevel )
	{
		_snwprintf( wszLevel, ARRAYSIZE(wszLevel) - 1, L"%d", m_iUpgradeLevel );
		wszLevel[ ARRAYSIZE(wszLevel)-1 ] = '\0';
	}

	if ( m_iUpgradeLevel >= 3 )
	{
		if ( bShowLevel )
		{
			g_pVGuiLocalize->ConstructString( sDataString, iMaxLenInBytes, g_pVGuiLocalize->Find("#TF_playerid_object_level"),
				1,
				wszLevel );
		}
		return;
	}

	wchar_t wszBuilderName[ MAX_PLAYER_NAME_LENGTH ];
	wchar_t wszObjectName[ 32 ];
	wchar_t wszUpgradeProgress[ 32 ];

	g_pVGuiLocalize->ConvertANSIToUnicode( GetStatusName(), wszObjectName, sizeof(wszObjectName) );

	C_BasePlayer *pBuilder = GetOwner();

	if ( pBuilder )
	{
		g_pVGuiLocalize->ConvertANSIToUnicode( pBuilder->GetPlayerName(), wszBuilderName, sizeof(wszBuilderName) );
	}
	else
	{
		wszBuilderName[0] = '\0';
	}

	_snwprintf( wszUpgradeProgress, ARRAYSIZE(wszUpgradeProgress) - 1, L"%d / %d", m_iUpgradeMetal, GetUpgradeMetalRequired() );
	wszUpgradeProgress[ ARRAYSIZE(wszUpgradeProgress)-1 ] = '\0';
	if ( bShowLevel )
	{
		g_pVGuiLocalize->ConstructString( sDataString, iMaxLenInBytes, g_pVGuiLocalize->Find("#TF_playerid_object_upgrading_level"),
			2,
			wszLevel,
			wszUpgradeProgress );
	}
	else
	{
		g_pVGuiLocalize->ConstructString( sDataString, iMaxLenInBytes, g_pVGuiLocalize->Find("#TF_playerid_object_upgrading"),
			1,
			wszUpgradeProgress );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::CheckNearMiss( Vector vecStart, Vector vecEnd )
{
	// Check against the local player. If the laser sweeps near him, play the near miss sound...
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer || !pLocalPlayer->IsAlive() )
		return;

	// Can't hear near miss sounds from friendly guns.
//	if ( pLocalPlayer->GetTeamNumber() == GetTeamNumber() )
//		return;

	Vector vecPlayerPos = pLocalPlayer->GetAbsOrigin();
	Vector vecClosestPoint;
	float dist;
	CalcClosestPointOnLineSegment( vecPlayerPos, vecStart, vecEnd, vecClosestPoint, &dist );
	dist = vecPlayerPos.DistTo( vecClosestPoint );
	if ( dist > 120 )
	{
		StopSound( "Building_Sentrygun.ShaftLaserPass" );
		return;
	}

	if ( !m_bNearMiss )
	{
		// We're good for a near miss!
		float soundlen = 0;
		EmitSound_t params;
		params.m_flSoundTime = 0;
		params.m_pSoundName = "Building_Sentrygun.ShaftLaserPass";
		params.m_pflSoundDuration = &soundlen;
		params.m_flVolume = 1.f - (dist / 120.f);
		CSingleUserRecipientFilter localFilter( pLocalPlayer );
		EmitSound( localFilter, pLocalPlayer->entindex(), params );

		m_bNearMiss = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::DisplayHintTo( C_BasePlayer *pPlayer )
{
	bool bHintPlayed = false;

	C_TFPlayer *pTFPlayer = ToTFPlayer(pPlayer);
	if ( InSameTeam( pPlayer ) )
	{
		// We're looking at a friendly object. 
		if ( pTFPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			// If the sentrygun can be upgraded, and I can afford it, let me know
			if ( GetHealth() == GetMaxHealth() && GetUpgradeLevel() < 3 )
			{
				if ( pTFPlayer->GetBuildResources() >= SENTRYGUN_UPGRADE_COST )
				{
					bHintPlayed = pTFPlayer->HintMessage( HINT_ENGINEER_UPGRADE_SENTRYGUN, false, true );
				}
				else
				{
					bHintPlayed = pTFPlayer->HintMessage( HINT_ENGINEER_METAL_TO_UPGRADE, false, true );
				}
			}
		}
	}

	if ( !bHintPlayed )
	{
		BaseClass::DisplayHintTo( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *C_ObjectSentrygun::GetHudStatusIcon( void )
{
	const char *pszResult;

	switch( m_iUpgradeLevel )
	{
	case 1:
	default:
		pszResult = "obj_status_sentrygun_1";
		break;
	case 2:
		pszResult = "obj_status_sentrygun_2";
		break;
	case 3:
		pszResult = "obj_status_sentrygun_3";
		break;
	}

	return pszResult;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
BuildingHudAlert_t C_ObjectSentrygun::GetBuildingAlertLevel( void )
{
	BuildingHudAlert_t baseAlertLevel = BaseClass::GetBuildingAlertLevel();

	// Just warn on low shells.

	float flShellPercent = (float)m_iAmmoShells / (float)m_iMaxAmmoShells;

	BuildingHudAlert_t alertLevel = BUILDING_HUD_ALERT_NONE;

	if ( !IsBuilding() && flShellPercent < 0.25 )
	{
		alertLevel = BUILDING_HUD_ALERT_VERY_LOW_AMMO;
	}
	else if ( !IsBuilding() && flShellPercent < 0.50 )
	{
		alertLevel = BUILDING_HUD_ALERT_LOW_AMMO;
	}

	return MAX( baseAlertLevel, alertLevel );
}

//-----------------------------------------------------------------------------
// Purpose: During placement, only use the smaller bbox for shadow calc, don't include the range bodygroup
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::GetShadowRenderBounds( Vector &mins, Vector &maxs, ShadowType_t shadowType )
{
	if ( IsPlacing() )
	{
		mins = CollisionProp()->OBBMins();
		maxs = CollisionProp()->OBBMaxs();

		// HACK: The collision prop bounding box doesn't quite cover the blueprint model, so we bloat it a little
		Vector bbBloat( 10.0f, 10.0f, 0.0f );
		mins -= bbBloat;
		maxs += bbBloat;
	}
	else
	{
		BaseClass::GetShadowRenderBounds( mins, maxs, shadowType );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Re-calc our damage particles when we get a new model
//-----------------------------------------------------------------------------
CStudioHdr *C_ObjectSentrygun::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	UpdateDamageEffects( m_damageLevel );

	// Reset Bodygroups
	for ( int i = GetNumBodyGroups()-1; i >= 0; i-- )
	{
		SetBodygroup( i, 0 );
	}

	m_iPlacementBodygroup = FindBodygroupByName( "sentry1_range" );

	return hdr;
}

//-----------------------------------------------------------------------------
// Purpose: Damage level has changed, update our effects
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::UpdateDamageEffects( BuildingDamageLevel_t damageLevel )
{
	if ( m_hDamageEffects )
	{
		m_hDamageEffects->StopEmission( false, false );
		m_hDamageEffects = NULL;
	}

	const char *pszEffect = "";

	switch( damageLevel )
	{
	case BUILDING_DAMAGE_LEVEL_LIGHT:
		pszEffect = "sentrydamage_1";
		break;
	case BUILDING_DAMAGE_LEVEL_MEDIUM:
		pszEffect = "sentrydamage_2";
		break;
	case BUILDING_DAMAGE_LEVEL_HEAVY:
		pszEffect = "sentrydamage_3";
		break;
	case BUILDING_DAMAGE_LEVEL_CRITICAL:
		pszEffect = "sentrydamage_4";
		break;

	default:
		break;
	}

	if ( Q_strlen(pszEffect) > 0 )
	{
		switch( m_iUpgradeLevel )
		{
		case 1:
		case 2:
			m_hDamageEffects = ParticleProp()->Create( pszEffect, PATTACH_POINT_FOLLOW, "build_point_0" );
			break;

		case 3:
			m_hDamageEffects = ParticleProp()->Create( pszEffect, PATTACH_POINT_FOLLOW, "sentrydamage" );
			break;
		}		
	}
}

//-----------------------------------------------------------------------------
// Purpose: placement state has changed, update the model
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::OnPlacementStateChanged( bool bValidPlacement )
{
	if ( bValidPlacement && ( m_iPlacementBodygroup >= 0 ) )
	{
		SetBodygroup( m_iPlacementBodygroup, 1 );
	}
	else
	{
		SetBodygroup( m_iPlacementBodygroup, 0 );
	}

	BaseClass::OnPlacementStateChanged( bValidPlacement );
}

void C_ObjectSentrygun::DebugDamageParticles( void )
{
	Msg( "Health %d\n", GetHealth() );

	BuildingDamageLevel_t damageLevel = CalculateDamageLevel();
	Msg( "Damage Level %d\n", (int)damageLevel );

	if ( m_hDamageEffects )
	{
		Msg( "m_hDamageEffects is valid\n" );
	}
	else
	{
		Msg( "m_hDamageEffects is NULL\n" );
	}

	// print all particles owned by particleprop
	ParticleProp()->DebugPrintEffects();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char* C_ObjectSentrygun::GetStatusName() const
{
	return "#TF_Object_Sentry";
}
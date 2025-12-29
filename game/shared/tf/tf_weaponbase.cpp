//========= Copyright Valve Corporation, All rights reserved. ============//
//
//	Weapons.
//
//=============================================================================
#include "cbase.h"
#include "in_buttons.h"
#include "takedamageinfo.h"
#include "tf_weaponbase.h"
#include "ammodef.h"
#include "tf_gamerules.h"
#include "eventlist.h"
#include "activitylist.h"

#include "tf_weapon_wrench.h"

// Server specific.
#if !defined( CLIENT_DLL )
#include "tf_player.h"
#include "tf_weapon_medigun.h"
#include "tf_gamestats.h"

#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_gamestats.h"
#include "ilagcompensationmanager.h"
#include "collisionutils.h"
#include "tf_team.h"
#include "tf_obj.h"
#include "tf_weapon_grenade_pipebomb.h"
#include "particle_parse.h"
#include "tf_weaponbase_grenadeproj.h"
#include "tf_gamestats.h"
#include "tf_fx.h"
#include "tf_gamestats.h"
// Client specific.
#else
#include "c_tf_player.h"
#include "c_baseviewmodel.h"
#include "tf_viewmodel.h"
#include "hud_crosshair.h"
#include "c_tf_playerresource.h"
#include "clientmode_tf.h"
#include "r_efx.h"
#include "dlight.h"
#include "effect_dispatch_data.h"
#include "c_te_effect_dispatch.h"
#include "toolframework_client.h"
#include "hud_chat.h"
#include "prediction.h"

// for spy material proxy
#include "tf_proxyentity.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"

extern CTFWeaponInfo *GetTFWeaponInfo( int iWeapon );
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar tf_useparticletracers;
ConVar tf_scout_hype_pep_mod( "tf_scout_hype_pep_mod", "1.0", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_scout_hype_pep_max( "tf_scout_hype_pep_max", "99.0", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_scout_hype_pep_min_damage( "tf_scout_hype_pep_min_damage", "5.0", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

ConVar tf_weapon_criticals_nopred( "tf_weapon_criticals_nopred", "1.0", FCVAR_REPLICATED | FCVAR_CHEAT );

#ifdef _DEBUG
ConVar tf_weapon_criticals_anticheat( "tf_weapon_criticals_anticheat", "1.0", FCVAR_REPLICATED );
ConVar tf_weapon_criticals_debug( "tf_weapon_criticals_debug", "0.0", FCVAR_REPLICATED );
extern ConVar tf_weapon_criticals_force_random;
#endif // _DEBUG
extern ConVar tf_weapon_criticals_bucket_cap;
extern ConVar tf_weapon_criticals_bucket_bottom;

#ifdef CLIENT_DLL
extern ConVar cl_crosshair_file;
#endif

//=============================================================================
//
// Global functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool IsAmmoType( int iAmmoType, const char *pAmmoName )
{
	return GetAmmoDef()->Index( pAmmoName ) == iAmmoType;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void FindHullIntersection( const Vector &vecSrc, trace_t &tr, const Vector &mins, const Vector &maxs, CBaseEntity *pEntity )
{
	int	i, j, k;
	trace_t tmpTrace;
	Vector vecEnd;
	float distance = 1e6f;
	Vector minmaxs[2] = {mins, maxs};
	Vector vecHullEnd = tr.endpos;

	vecHullEnd = vecSrc + ((vecHullEnd - vecSrc)*2);
	UTIL_TraceLine( vecSrc, vecHullEnd, MASK_SOLID, pEntity, COLLISION_GROUP_NONE, &tmpTrace );
	if ( tmpTrace.fraction < 1.0 )
	{
		tr = tmpTrace;
		return;
	}

	for ( i = 0; i < 2; i++ )
	{
		for ( j = 0; j < 2; j++ )
		{
			for ( k = 0; k < 2; k++ )
			{
				vecEnd.x = vecHullEnd.x + minmaxs[i][0];
				vecEnd.y = vecHullEnd.y + minmaxs[j][1];
				vecEnd.z = vecHullEnd.z + minmaxs[k][2];

				UTIL_TraceLine( vecSrc, vecEnd, MASK_SOLID, pEntity, COLLISION_GROUP_NONE, &tmpTrace );
				if ( tmpTrace.fraction < 1.0 )
				{
					float thisDistance = (tmpTrace.endpos - vecSrc).Length();
					if ( thisDistance < distance )
					{
						tr = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
}

//=============================================================================
//
// TFWeaponBase tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponBase, DT_TFWeaponBase )

#ifdef GAME_DLL
void* SendProxy_SendActiveLocalWeaponDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID );
void* SendProxy_SendNonLocalWeaponDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID );
#endif

//-----------------------------------------------------------------------------
// Purpose: Only sent to the player holding it.
//-----------------------------------------------------------------------------
BEGIN_NETWORK_TABLE_NOBASE( CTFWeaponBase, DT_LocalTFWeaponData )
#if defined( CLIENT_DLL )
	RecvPropTime( RECVINFO( m_flLastCritCheckTime ) ),
	RecvPropTime( RECVINFO( m_flReloadPriorNextFire ) ),
	RecvPropTime( RECVINFO( m_flLastFireTime ) ),
	RecvPropTime( RECVINFO( m_flEffectBarRegenTime ) ),
	RecvPropFloat( RECVINFO( m_flObservedCritChance ) ),
#else
	SendPropTime( SENDINFO( m_flLastCritCheckTime ) ),
	SendPropTime( SENDINFO( m_flReloadPriorNextFire ) ),
	SendPropTime( SENDINFO( m_flLastFireTime ) ),
	SendPropTime( SENDINFO( m_flEffectBarRegenTime ) ),
	SendPropFloat( SENDINFO( m_flObservedCritChance ), 16, SPROP_NOSCALE, 0.0, 100.0 ),
#endif
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Variables sent at low precision to non-holding observers.
//-----------------------------------------------------------------------------
BEGIN_NETWORK_TABLE_NOBASE( CTFWeaponBase, DT_TFWeaponDataNonLocal )
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE( CTFWeaponBase, DT_TFWeaponBase )
// Client specific.
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bLowered ) ),
	RecvPropInt( RECVINFO( m_iReloadMode ) ),
	RecvPropBool( RECVINFO( m_bResetParity ) ), 
	RecvPropBool( RECVINFO( m_bReloadedThroughAnimEvent ) ),
	RecvPropBool( RECVINFO( m_bDisguiseWeapon ) ),
	RecvPropDataTable("LocalActiveTFWeaponData", 0, 0, &REFERENCE_RECV_TABLE(DT_LocalTFWeaponData)),
	RecvPropDataTable("NonLocalTFWeaponData", 0, 0, &REFERENCE_RECV_TABLE(DT_TFWeaponDataNonLocal)),
#else
// Server specific.
	SendPropBool( SENDINFO( m_bLowered ) ),
	SendPropBool( SENDINFO( m_bResetParity ) ),
	SendPropInt( SENDINFO( m_iReloadMode ), 4, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bReloadedThroughAnimEvent ) ),
	SendPropBool( SENDINFO( m_bDisguiseWeapon ) ),
	SendPropDataTable("LocalActiveTFWeaponData", 0, &REFERENCE_SEND_TABLE(DT_LocalTFWeaponData), SendProxy_SendActiveLocalWeaponDataTable ),
	SendPropDataTable("NonLocalTFWeaponData", 0, &REFERENCE_SEND_TABLE(DT_TFWeaponDataNonLocal), SendProxy_SendNonLocalWeaponDataTable ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponBase ) 
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_bLowered, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iReloadMode, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bReloadedThroughAnimEvent, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bDisguiseWeapon, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD_TOL( m_flLastCritCheckTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),	
	DEFINE_PRED_FIELD_TOL( m_flReloadPriorNextFire, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),	
	DEFINE_PRED_FIELD_TOL( m_flLastFireTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),	
	DEFINE_PRED_FIELD( m_bCurrentAttackIsCrit, FIELD_BOOLEAN, 0 ),
	DEFINE_PRED_FIELD( m_iCurrentSeed, FIELD_INTEGER, 0 ),
	DEFINE_PRED_FIELD_TOL( m_flEffectBarRegenTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),	
#endif
END_PREDICTION_DATA()

#ifdef GAME_DLL
BEGIN_ENT_SCRIPTDESC( CTFWeaponBase, CBaseCombatWeapon, "Team Fortress 2 Weapon" )
END_SCRIPTDESC();
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_base, CTFWeaponBase );

// Server specific.
#if !defined( CLIENT_DLL )

BEGIN_DATADESC( CTFWeaponBase )
	DEFINE_THINKFUNC( FallThink ),
END_DATADESC()

// Client specific
#else

ConVar cl_crosshaircolor( "cl_crosshaircolor", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );
ConVar cl_dynamiccrosshair( "cl_dynamiccrosshair", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );
ConVar cl_scalecrosshair( "cl_scalecrosshair", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );
ConVar cl_crosshairalpha( "cl_crosshairalpha", "200", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );

int g_iScopeTextureID = 0;
int g_iScopeDustTextureID = 0;

#endif

ConVar tf_weapon_criticals( "tf_weapon_criticals", "1", FCVAR_REPLICATED | FCVAR_NOTIFY, "Whether or not random crits are enabled" );

//=============================================================================
//
// TFWeaponBase shared functions.
//

// -----------------------------------------------------------------------------
// Purpose: Constructor.
// -----------------------------------------------------------------------------
CTFWeaponBase::CTFWeaponBase()
{
	SetPredictionEligible( true );

	// Nothing collides with these, but they get touch calls.
	AddSolidFlags( FSOLID_TRIGGER );

	// Weapons can fire underwater.
	m_bFiresUnderwater = true;
	m_bAltFiresUnderwater = true;

	// Initialize the weapon modes.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	m_iReloadMode.Set( TF_RELOAD_START );

	m_iAltFireHint = 0;
	m_bInAttack = false;
	m_bInAttack2 = false;
	m_flCritTime = 0;
	m_flLastCritCheckTime = 0;
	m_flLastRapidFireCritCheckTime = 0;
	m_iLastCritCheckFrame = 0;
	m_flObservedCritChance = 0.f;
	m_flLastFireTime = 0;
	m_flEffectBarRegenTime = 0;
	m_bCurrentAttackIsCrit = false;
	m_bCurrentCritIsRandom = false;
	m_iCurrentSeed = -1;
	m_flReloadPriorNextFire = 0;
	m_flLastDeployTime = 0;

	m_bDisguiseWeapon = false;

	m_iAmmoToAdd = 0;

#ifdef CLIENT_DLL
	m_iCachedModelIndex = 0;
	m_iEjectBrassAttachpoint = -2;
#endif // CLIENT_DLL
}

CTFWeaponBase::~CTFWeaponBase()
{

}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBase::Spawn()
{
	// Base class spawn.
	BaseClass::Spawn();

	// Set this here to allow players to shoot dropped weapons.
	SetCollisionGroup( COLLISION_GROUP_WEAPON );

	// Get the weapon information.
	WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( GetClassname() );
	Assert( hWpnInfo != GetInvalidWeaponInfoHandle() );
	CTFWeaponInfo *pWeaponInfo = dynamic_cast<CTFWeaponInfo*>( GetFileWeaponInfoFromHandle( hWpnInfo ) );
	Assert( pWeaponInfo && "Failed to get CTFWeaponInfo in weapon spawn" );
	m_pWeaponInfo = pWeaponInfo;

	if ( GetPlayerOwner() )
	{
		ChangeTeam( GetPlayerOwner()->GetTeamNumber() );
	}

#ifdef GAME_DLL
	// Move it up a little bit, otherwise it'll be at the guy's feet, and its sound origin 
	// will be in the ground so its EmitSound calls won't do anything.
	Vector vecOrigin = GetAbsOrigin();
	SetAbsOrigin( Vector( vecOrigin.x, vecOrigin.y, vecOrigin.z + 5.0f ) );
#endif

	m_szTracerName[0] = '\0';
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::Activate( void )
{
	BaseClass::Activate();

	// Reset our clip, in case we've had it modified
	GiveDefaultAmmo();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBase::FallInit( void )
{

}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFWeaponBase::Precache()
{
	BaseClass::Precache();

	if ( GetMuzzleFlashModel() )
	{
		PrecacheModel( GetMuzzleFlashModel() );
	}

	const CTFWeaponInfo *pTFInfo = &GetTFWpnData();

	if ( pTFInfo->m_szExplosionSound[0] )
	{
		CBaseEntity::PrecacheScriptSound( pTFInfo->m_szExplosionSound );
	}

	if ( pTFInfo->m_szBrassModel[0] )
	{
		PrecacheModel( pTFInfo->m_szBrassModel );
	}

	if ( GetMuzzleFlashParticleEffect() )
	{
		PrecacheParticleSystem( GetMuzzleFlashParticleEffect() );
	}

	if ( pTFInfo->m_szExplosionEffect[0] )
	{
		PrecacheParticleSystem( pTFInfo->m_szExplosionEffect );
	}

	if ( pTFInfo->m_szExplosionPlayerEffect[0] )
	{
		PrecacheParticleSystem( pTFInfo->m_szExplosionPlayerEffect );
	}

	if ( pTFInfo->m_szExplosionWaterEffect[0] )
	{
		PrecacheParticleSystem( pTFInfo->m_szExplosionWaterEffect );
	}

	const char *pszTracerEffect = pTFInfo->m_szTracerEffect;

	if ( pszTracerEffect && pszTracerEffect[0] )
	{
		char pTracerEffect[128];
		char pTracerEffectCrit[128];

		Q_snprintf( pTracerEffect, sizeof(pTracerEffect), "%s_red", pszTracerEffect );
		Q_snprintf( pTracerEffectCrit, sizeof(pTracerEffectCrit), "%s_red_crit", pszTracerEffect );
		PrecacheParticleSystem( pTracerEffect );
		PrecacheParticleSystem( pTracerEffectCrit );

		Q_snprintf( pTracerEffect, sizeof(pTracerEffect), "%s_blue", pszTracerEffect );
		Q_snprintf( pTracerEffectCrit, sizeof(pTracerEffectCrit), "%s_blue_crit", pszTracerEffect );
		PrecacheParticleSystem( pTracerEffect );
		PrecacheParticleSystem( pTracerEffectCrit );
	}

	PrecacheModel( "models/weapons/c_models/stattrack.mdl" );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
const CTFWeaponInfo &CTFWeaponBase::GetTFWpnData() const
{
	const FileWeaponInfo_t *pWeaponInfo = &GetWpnData();
	const CTFWeaponInfo *pTFInfo = dynamic_cast< const CTFWeaponInfo* >( pWeaponInfo );
	Assert( pTFInfo );
	return *pTFInfo;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
int CTFWeaponBase::GetWeaponID( void ) const
{
	Assert( false ); 
	return TF_WEAPON_NONE; 
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFWeaponBase::IsWeapon( int iWeapon ) const
{ 
	return GetWeaponID() == iWeapon; 
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
int	CTFWeaponBase::GetMaxClip1( void ) const
{
	// Handle the itemdef mod first...
	float flClip = BaseClass::GetMaxClip1();

	// Now handle in-game sources, otherwise we get weird numbers on things like the FAN
	if ( flClip >= 0 )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
		if ( pPlayer )
		{
			// Blast weps (low clip counts)
			if ( GetWeaponID() == TF_WEAPON_GRENADELAUNCHER || GetWeaponID() == TF_WEAPON_ROCKETLAUNCHER ) // TF2007: IsBlastImpactWeapon() is only called here and only the grenade launcher and rocket launcher return true for that function
			{
				return ( flClip );
			}
		}
	}

	return flClip;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
int	CTFWeaponBase::GetDefaultClip1( void ) const
{
	int iDefault = GetWpnData().iDefaultClip1;
	return ( iDefault == 0 ) ? 0 : GetMaxClip1();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
const char *CTFWeaponBase::GetViewModel( int iViewModel ) const
{
	return GetTFWpnData().szViewModel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFWeaponBase::GetWorldModel( void ) const
{
	return BaseClass::GetWorldModel();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::Equip( CBaseCombatCharacter *pOwner )
{
	SetOwner( pOwner );
	SetOwnerEntity( pOwner );

	BaseClass::Equip( pOwner );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::Drop( const Vector &vecVelocity )
{
#ifndef CLIENT_DLL
	if ( m_iAltFireHint )
	{
		CBasePlayer *pPlayer = GetPlayerOwner();
		if ( pPlayer )
		{
			pPlayer->StopHintTimer( m_iAltFireHint );
		}
	}
#endif

	BaseClass::Drop( vecVelocity );

#ifndef CLIENT_DLL
	// Never allow weapons to lie around on the ground
	UTIL_Remove( this );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::StartHolsterAnim( void )
{
	Holster();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::Holster( CBaseCombatWeapon *pSwitchingTo )
{
#ifndef CLIENT_DLL
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( pPlayer && m_iAltFireHint )
	{
		pPlayer->StopHintTimer( m_iAltFireHint );
	}
#endif

	m_iReloadMode.Set( TF_RELOAD_START );

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::Deploy( void )
{
#ifndef CLIENT_DLL
	if ( m_iAltFireHint )
	{
		CBasePlayer *pPlayer = GetPlayerOwner();
		if ( pPlayer )
		{
			pPlayer->StartHintTimer( m_iAltFireHint );
		}
	}
#endif

	m_iReloadMode.Set( TF_RELOAD_START );

	float flOriginalPrimaryAttack = m_flNextPrimaryAttack;
	float flOriginalSecondaryAttack = m_flNextSecondaryAttack;

	bool bDeploy = BaseClass::Deploy();

	if ( bDeploy )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
		if ( !pPlayer )
			return false;

		// TF2007: It's hard to know what exactly this code was.
		// I'm confident that they didn't change the playback rate
		// I'm using 0.67f because that's what's used for flPlaybackRate, but it may not be accurate

		float flWeaponSwitchTime = 0.67f;

		float flDeployTime = flWeaponSwitchTime;
#if 0
		float flPlaybackRate = Clamp( ( 0.67f / flWeaponSwitchTime ), -4.f, 12.f ); // clamp between the range that's defined in send table
		if ( pPlayer->GetViewModel(0) )
		{
			pPlayer->GetViewModel(0)->SetPlaybackRate( flPlaybackRate );
		}
		if ( pPlayer->GetViewModel(1) )
		{
			pPlayer->GetViewModel(1)->SetPlaybackRate( flPlaybackRate );
		}
#endif
		// Don't override primary attacks that are already further out than this. This prevents
		// people exploiting weapon switches to allow weapons to fire faster.
		m_flNextPrimaryAttack = MAX( flOriginalPrimaryAttack, gpGlobals->curtime + flDeployTime );
		m_flNextSecondaryAttack = MAX( flOriginalSecondaryAttack, m_flNextPrimaryAttack.Get() );

		pPlayer->SetNextAttack( m_flNextPrimaryAttack );

		m_flLastDeployTime = gpGlobals->curtime;

#ifdef GAME_DLL
		// Reset our deploy-lifetime kill counter.
		pPlayer->m_Shared.m_flFirstPrimaryAttack = m_flNextPrimaryAttack;
#endif // GAME_DLL

	}

	return bDeploy;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBase::OnActiveStateChanged( int iOldState )
{
	// Check for a speed mod change.
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( pPlayer )
	{
		pPlayer->TeamFortress_SetSpeed();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFWeaponBase::VisibleInWeaponSelection( void )
{
	if ( BaseClass::VisibleInWeaponSelection() == false )
	{
		return false;
	}
	if ( TFGameRules()->IsInTraining() )
	{
		ConVarRef training_can_select_weapon_primary	( "training_can_select_weapon_primary" );
		ConVarRef training_can_select_weapon_secondary	( "training_can_select_weapon_secondary" );
		ConVarRef training_can_select_weapon_melee		( "training_can_select_weapon_melee" );
		ConVarRef training_can_select_weapon_building	( "training_can_select_weapon_building" );
		ConVarRef training_can_select_weapon_pda		( "training_can_select_weapon_pda" );
		bool bVisible = true;
		switch ( GetTFWpnData().m_iWeaponType )
		{
		case TF_WPN_TYPE_PRIMARY:	bVisible = training_can_select_weapon_primary.GetBool(); break;
		case TF_WPN_TYPE_SECONDARY:	bVisible = training_can_select_weapon_secondary.GetBool(); break;
		case TF_WPN_TYPE_MELEE:		bVisible = training_can_select_weapon_melee.GetBool(); break;
		case TF_WPN_TYPE_BUILDING:	bVisible = training_can_select_weapon_building.GetBool(); break;
		case TF_WPN_TYPE_PDA:		bVisible = training_can_select_weapon_pda.GetBool(); break;
		} // switch
		return bVisible;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
void CTFWeaponBase::PrimaryAttack( void )
{
	// Set the weapon mode.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	if ( !CanAttack() )
		return;

	BaseClass::PrimaryAttack();

	if ( m_bReloadsSingly )
	{
		m_iReloadMode.Set( TF_RELOAD_START );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CTFWeaponBase::SecondaryAttack( void )
{
	// Set the weapon mode.
	m_iWeaponMode = TF_WEAPON_SECONDARY_MODE;


	// Don't hook secondary for now.
	return;
}

//-----------------------------------------------------------------------------
// Purpose: Most calls use the prediction seed
//-----------------------------------------------------------------------------
void CTFWeaponBase::CalcIsAttackCritical( void)
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( gpGlobals->framecount == m_iLastCritCheckFrame )
		return;
	m_iLastCritCheckFrame = gpGlobals->framecount;

	m_bCurrentCritIsRandom = false;

	if ( (TFGameRules()->State_Get() == GR_STATE_TEAM_WIN) && (TFGameRules()->GetWinningTeam() == pPlayer->GetTeamNumber()) )
	{
		m_bCurrentAttackIsCrit = true;
	}
	else if ( !AreRandomCritsEnabled() )
	{
		// Support critboosted even in no crit mode
		m_bCurrentAttackIsCrit = CalcIsAttackCriticalHelperNoCrits();
	}
	else
	{
		// call the weapon-specific helper method
		m_bCurrentAttackIsCrit = CalcIsAttackCriticalHelper();
	}
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::CalcIsAttackCriticalHelperNoCrits()
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Weapon-specific helper method to calculate if attack is crit
//-----------------------------------------------------------------------------
bool CTFWeaponBase::CalcIsAttackCriticalHelper()
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return false;

	float flCritChance = 0.f;
	float flPlayerCritMult = pPlayer->GetCritMult();

	if ( !CanFireCriticalShot() )
		return false;

	// For rapid fire weapons, allow crits while period is active
	bool bRapidFire = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_bUseRapidFireCrits;
	if ( bRapidFire && m_flCritTime > gpGlobals->curtime )
		return true;

	// --- Random crits from this point on ---
	
	// Monitor and enforce short-term random crit rate - via bucket

	// Figure out how much to add/remove from token bucket
	int nProjectilesPerShot = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_nBulletsPerShot;
	if ( nProjectilesPerShot < 1 )
	{
		nProjectilesPerShot = 1;
	}

	// Damage
	float flDamage = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_nDamage;
	flDamage *= nProjectilesPerShot;
	AddToCritBucket( flDamage );

	bool bCrit = false;
	m_bCurrentCritIsRandom = true;
	int iRandom = 0;

	if ( bRapidFire )
	{
		// only perform one crit check per second for rapid fire weapons
		if ( tf_weapon_criticals_nopred.GetBool() )
		{
			if ( gpGlobals->curtime < m_flLastRapidFireCritCheckTime + 1.f )
				return false;

			m_flLastRapidFireCritCheckTime = gpGlobals->curtime;
		}
		else
		{
			if ( gpGlobals->curtime < m_flLastCritCheckTime + 1.f )
				return false;

			m_flLastCritCheckTime = gpGlobals->curtime;
		}

		// get the total crit chance (ratio of total shots fired we want to be crits)
		float flTotalCritChance = clamp( TF_DAMAGE_CRIT_CHANCE_RAPID * flPlayerCritMult, 0.01f, 0.99f );
		// get the fixed amount of time that we start firing crit shots for	
		float flCritDuration = TF_DAMAGE_CRIT_DURATION_RAPID;
		// calculate the amount of time, on average, that we want to NOT fire crit shots for in order to achieve the total crit chance we want
		float flNonCritDuration = ( flCritDuration / flTotalCritChance ) - flCritDuration;
		// calculate the chance per second of non-crit fire that we should transition into critting such that on average we achieve the total crit chance we want
		float flStartCritChance = 1 / flNonCritDuration;

		// if base entity seed has changed since last calculation, reseed with new seed
		int iMask = ( entindex() << 8 ) | ( pPlayer->entindex() );
		int iSeed = CBaseEntity::GetPredictionRandomSeed() ^ iMask;
		if ( iSeed != m_iCurrentSeed )
		{
			m_iCurrentSeed = iSeed;
			RandomSeed( m_iCurrentSeed );
		}

		// see if we should start firing crit shots
		iRandom = RandomInt( 0, WEAPON_RANDOM_RANGE-1 );
		if ( iRandom < flStartCritChance * WEAPON_RANDOM_RANGE )
		{
			bCrit = true;
			flCritChance = flStartCritChance;
		}
	}
	else
	{
		// single-shot weapon, just use random pct per shot
		flCritChance = TF_DAMAGE_CRIT_CHANCE * flPlayerCritMult;

		// mess with the crit chance seed so it's not based solely on the prediction seed
		int iMask = ( entindex() << 8 ) | ( pPlayer->entindex() );
		int iSeed = CBaseEntity::GetPredictionRandomSeed() ^ iMask;
		if ( iSeed != m_iCurrentSeed )
		{
			m_iCurrentSeed = iSeed;
			RandomSeed( m_iCurrentSeed );
		}

		iRandom = RandomInt( 0, WEAPON_RANDOM_RANGE - 1 );
		bCrit = ( iRandom < flCritChance * WEAPON_RANDOM_RANGE );
	}

#ifdef _DEBUG
	if ( tf_weapon_criticals_debug.GetBool() )
	{
#ifdef GAME_DLL
		DevMsg( "Roll (server): %i out of %f (crit: %d)\n", iRandom, ( flCritChance * WEAPON_RANDOM_RANGE ), bCrit );
#else
		if ( prediction->IsFirstTimePredicted() )
		{
			DevMsg( "\tRoll (client): %i out of %f (crit: %d)\n", iRandom, ( flCritChance * WEAPON_RANDOM_RANGE ), bCrit );
		}
#endif // GAME_DLL
	}

	// Force seed to always say yes
	if ( tf_weapon_criticals_force_random.GetInt() )
	{
		bCrit = true;
	}
#endif // _DEBUG
	
	// Track each check
#ifdef GAME_DLL
	m_nCritChecks++;
#else
	if ( prediction->IsFirstTimePredicted() )
	{
		m_nCritChecks++;
	}
#endif // GAME_DLL

	// Seed says crit.  Run it by the manager.
	if ( bCrit )
	{
		bool bAntiCheat = true;
#ifdef _DEBUG
		bAntiCheat = tf_weapon_criticals_anticheat.GetBool();
#endif // _DEBUG

		// Monitor and enforce long-term random crit rate - via stats
		if ( bAntiCheat )
		{
			if ( !CanFireRandomCriticalShot( flCritChance ) )
				return false;

			// Make sure rapid fire weapons can pay the cost of the entire period up-front
			if ( bRapidFire )
			{
				flDamage *= TF_DAMAGE_CRIT_DURATION_RAPID / m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;

				// Never try to drain more than cap
				int nBucketCap = tf_weapon_criticals_bucket_cap.GetInt();
				if ( flDamage * TF_DAMAGE_CRIT_MULTIPLIER > nBucketCap )
					flDamage = (float)nBucketCap / TF_DAMAGE_CRIT_MULTIPLIER;
			}

			bCrit = IsAllowedToWithdrawFromCritBucket( flDamage );
		}

		if ( bCrit && bRapidFire )
		{
			m_flCritTime = gpGlobals->curtime + TF_DAMAGE_CRIT_DURATION_RAPID;
		}
	}

	return bCrit;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFWeaponBase::Reload( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return false;

	// If we're not already reloading, check to see if we have ammo to reload and check to see if we are max ammo.
	if ( m_iReloadMode == TF_RELOAD_START ) 
	{
		// If I don't have any spare ammo, I can't reload
		if ( GetOwner()->GetAmmoCount(m_iPrimaryAmmoType) <= 0 )
			return false;

		if ( Clip1() >= GetMaxClip1() )
			return false;
	}

	// Reload one object at a time.
	if ( m_bReloadsSingly )
		return ReloadSingly();

	// Normal reload.
	DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::AbortReload( void )
{
	BaseClass::AbortReload();

	m_iReloadMode.Set( TF_RELOAD_START );

	// Make sure our reloading bodygroup is hidden (shells/grenades/etc)
	int indexR = FindBodygroupByName( "reload" );
	if ( indexR >= 0 )
	{
		SetBodygroup( indexR, 0 );
	}
}


//-----------------------------------------------------------------------------
// Is the weapon reloading right now?
bool CTFWeaponBase::IsReloading() const
{
	return m_iReloadMode != TF_RELOAD_START;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFWeaponBase::ReloadSingly( void )
{
	// Don't reload.
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return false;

	// Get the current player.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return false;

	// Anti Reload Cancelling Exploit (Beggers Bazooka)
	// Force attack if we try to reload when we have ammo in the clip
	if ( AutoFiresFullClip() && Clip1() > 0 && m_iReloadMode == TF_RELOAD_START )
	{
		PrimaryAttack();
		m_bFiringWholeClip = true;

#ifdef CLIENT_DLL
		pPlayer->SetFiredWeapon( true );
#endif
		return false;
	}

	// check to see if we're ready to reload
	switch ( m_iReloadMode )
	{
	case TF_RELOAD_START:
		{
			// Play weapon and player animations.
			if ( SendWeaponAnim( ACT_RELOAD_START ) )
			{
				SetReloadTimer( SequenceDuration() );
			}
			else
			{
				// Update the reload timers with script values.
				UpdateReloadTimers( true );
			}

			// Next reload the shells.
			m_iReloadMode.Set( TF_RELOADING );

			m_iReloadStartClipAmount = Clip1();

			return true;
		}
	case TF_RELOADING:
		{
			// Did we finish the reload start?  Now we can reload a rocket.
			if ( m_flTimeWeaponIdle > gpGlobals->curtime )
				return false;

			// Play weapon reload animations and sound.
			if ( Clip1() == m_iReloadStartClipAmount )
			{
				pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD );
			}
			else
			{
				pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD_LOOP );
			}

			m_bReloadedThroughAnimEvent = false;

			if ( SendWeaponAnim( ACT_VM_RELOAD ) )
			{
				if ( GetWeaponID() == TF_WEAPON_GRENADELAUNCHER )
				{
					SetReloadTimer( GetTFWpnData().m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeReload );
				}
				else
				{
					SetReloadTimer( SequenceDuration() );
				}
			}
			else
			{
				// Update the reload timers.
				UpdateReloadTimers( false );
			}

			// Play reload
#ifdef CLIENT_DLL
			if ( ShouldPlayClientReloadSound() )
				WeaponSound( RELOAD );
#else
			WeaponSound( RELOAD );
#endif

			// Next continue to reload shells?
			m_iReloadMode.Set( TF_RELOADING_CONTINUE );

			return true;
		}
	case TF_RELOADING_CONTINUE:
		{
			// Did we finish the reload start?  Now we can finish reloading the rocket.
			if ( m_flTimeWeaponIdle > gpGlobals->curtime )
				return false;

			IncrementAmmo();

			if ( ( ( Clip1() == GetMaxClip1() || pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 ) ) )
			{
				m_iReloadMode.Set( TF_RELOAD_FINISH );
			}
			else
			{
				m_iReloadMode.Set( TF_RELOADING );
			}

			return true;
		}

	case TF_RELOAD_FINISH:
	default:
		{
			if ( SendWeaponAnim( ACT_RELOAD_FINISH ) )
			{
				// We're done, allow primary attack as soon as we like unless we're an energy weapon.
//				if ( IsEnergyWeapon() )
//				{
//					SetReloadTimer( SequenceDuration() );
//				}
			}

			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD_END );

			m_iReloadMode.Set( TF_RELOAD_START );
			return true;
		}
	}
}

void CTFWeaponBase::IncrementAmmo( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	// If we have ammo, remove ammo and add it to clip
	if ( !m_bReloadedThroughAnimEvent )
	{
		if ( pPlayer && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) > 0 && ( m_iClip1 < GetMaxClip1() ) )
		{
			m_iClip1++;
			pPlayer->RemoveAmmo( 1, m_iPrimaryAmmoType );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//			*pOperator - 
//-----------------------------------------------------------------------------
void CTFWeaponBase::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	if ( (pEvent->type & AE_TYPE_NEWEVENTSYSTEM) /*&& (pEvent->type & AE_TYPE_SERVER)*/ )
	{
		if ( pEvent->event == AE_WPN_INCREMENTAMMO )
		{
			IncrementAmmo();

			m_bReloadedThroughAnimEvent = true;
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::NeedsReloadForAmmo1( int iClipSize1 ) const
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if ( pOwner )
	{
		// If you don't have clips, then don't try to reload them.
		if ( UsesClipsForAmmo1() )
		{
			// need to reload primary clip?
			int primary = MIN( iClipSize1 - m_iClip1, pOwner->GetAmmoCount( m_iPrimaryAmmoType ) );
			if ( primary != 0 )
				return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::NeedsReloadForAmmo2( int iClipSize2 ) const
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if ( pOwner )
	{
		if ( UsesClipsForAmmo2() )
		{
			// need to reload secondary clip?
			int secondary = MIN( iClipSize2 - m_iClip2, pOwner->GetAmmoCount( m_iSecondaryAmmoType ) );
			if ( secondary != 0 )
				return true;
		}
	}

	return false;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFWeaponBase::DefaultReload( int iClipSize1, int iClipSize2, int iActivity )
{
	// The the owning local player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return false;

	// Setup and check for reload.
	bool bReloadPrimary = NeedsReloadForAmmo1( iClipSize1 );
	bool bReloadSecondary = NeedsReloadForAmmo2( iClipSize2 );

	// We didn't reload.
	if ( !( bReloadPrimary || bReloadSecondary )  )
		return false;

	// Play reload
#ifdef CLIENT_DLL
	if ( ShouldPlayClientReloadSound() )
		WeaponSound( RELOAD );
#else
	WeaponSound( RELOAD );
#endif

	// Play the player's reload animation
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD );

	float flReloadTime;
	// First, see if we have a reload animation
	if ( SendWeaponAnim( iActivity ) )
	{
		// We consider the reload finished 0.2 sec before the anim is, so that players don't keep accidentally aborting their reloads
		flReloadTime = SequenceDuration() - 0.2;
	}
	else
	{
		// No reload animation. Use the script time.
		flReloadTime = GetTFWpnData().m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeReload;  
		if ( bReloadSecondary )
		{
			flReloadTime = GetTFWpnData().m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flTimeReload;  
		}
	}

	SetReloadTimer( flReloadTime );

	m_bInReload = true;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::UpdateReloadTimers( bool bStart )
{
	// Starting a reload?
	if ( bStart )
	{
		// Get the reload start time.
		SetReloadTimer( m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeReloadStart );
	}
	// In reload.
	else
	{
		SetReloadTimer( m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeReload );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::SetReloadTimer( float flReloadTime )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	flReloadTime *= GetReloadSpeedScale();
	flReloadTime = MAX( flReloadTime, 0.00001f );

	m_flReloadPriorNextFire = m_flNextPrimaryAttack;

	float flTime = gpGlobals->curtime + flReloadTime;

	// Set next player attack time (weapon independent).
	pPlayer->m_flNextAttack = flTime;

	// Set next weapon attack times (based on reloading).
	m_flNextPrimaryAttack = Max( flTime, (float)m_flReloadPriorNextFire);

	// Don't push out secondary attack, because our secondary fire
	// systems are all separate from primary fire (sniper zooming, demoman pipebomb detonating, etc)
	//m_flNextSecondaryAttack = flTime;

	// Set next idle time (based on reloading).
	SetWeaponIdleTime( flTime );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFWeaponBase::PlayEmptySound()
{
	CPASAttenuationFilter filter( this );
	filter.UsePredictionRules();

	// TFTODO: Add default empty sound here!
//	EmitSound( filter, entindex(), "Default.ClipEmpty_Rifle" );

	return false;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBase::SendReloadEvents()
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	// Make the player play his reload animation.
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::ItemBusyFrame( void )
{
	// Call into the base ItemBusyFrame.
	BaseClass::ItemBusyFrame();

	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
	{
		return;
	}

	if ( ( pOwner->m_nButtons & IN_ATTACK2 ) && /*m_bInReload == false &&*/ m_bInAttack2 == false )
	{
		pOwner->DoClassSpecialSkill();
		m_bInAttack2 = true;
	}
	else if ( !(pOwner->m_nButtons & IN_ATTACK2) && m_bInAttack2 )
	{
		m_bInAttack2 = false;
	}

	// Interrupt a reload on reload singly weapons.
	if ( ( pOwner->m_nButtons & IN_ATTACK ) && Clip1() > 0 )
	{
		bool bAbortReload = false;
		if ( m_bReloadsSingly )
		{
			if ( m_iReloadMode != TF_RELOAD_START )
			{
				m_iReloadMode.Set( TF_RELOAD_START );
				bAbortReload = true;
			}
		}

		if ( bAbortReload )
		{
			AbortReload();
			m_bInReload = false;
			pOwner->m_flNextAttack = gpGlobals->curtime;
			m_flNextPrimaryAttack = Max<float>( gpGlobals->curtime, m_flReloadPriorNextFire );
			SetWeaponIdleTime( gpGlobals->curtime + m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeIdle );
		}
	}

	CheckEffectBarRegen();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::ItemPostFrame( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
	{
		return;
	}

	// debounce InAttack flags
	if ( m_bInAttack && !( pOwner->m_nButtons & IN_ATTACK ) )
	{
		m_bInAttack = false;
	}

	if ( m_bInAttack2 && !( pOwner->m_nButtons & IN_ATTACK2 ) )
	{
		m_bInAttack2 = false;
	}

	CheckEffectBarRegen();

	// If we're lowered, we're not allowed to fire
	if ( m_bLowered )
		return;

	// Call the base item post frame.
	BaseClass::ItemPostFrame();

	// Check for reload singly interrupts.
	if ( m_bReloadsSingly )
	{
		ReloadSinglyPostFrame();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFWeaponBase::GetNextSecondaryAttackDelay( void )
{
	// This is a little gross.  The demo needs fast-cycle secondary
	// attacks while holding down +attack2 and switching weapons
	// in order to properly handle timely sticky detonation.
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( pOwner && pOwner->IsPlayerClass( TF_CLASS_DEMOMAN ) )
	{
		return 0.1f;
	}

	return BaseClass::GetNextSecondaryAttackDelay();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBase::ItemHolsterFrame( void )
{
	BaseClass::ItemHolsterFrame();

	CheckEffectBarRegen();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::ReloadSinglyPostFrame( void )
{
	if ( m_flTimeWeaponIdle > gpGlobals->curtime )
		return;

	// if the clip is empty and we have ammo remaining,
	if ( ( !AutoFiresFullClip() && Clip1() == 0 && GetOwner()->GetAmmoCount( m_iPrimaryAmmoType ) > 0 ) ||
		// or we are already in the process of reloading but not finished
		( m_iReloadMode != TF_RELOAD_START ) )
	{
		// reload/continue reloading
		Reload();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::WeaponShouldBeLowered( void )
{
	// Can't be in the middle of another animation
	if ( GetIdealActivity() != ACT_VM_IDLE_LOWERED && GetIdealActivity() != ACT_VM_IDLE &&
		GetIdealActivity() != ACT_VM_IDLE_TO_LOWERED && GetIdealActivity() != ACT_VM_LOWERED_TO_IDLE )
		return false;

	if ( m_bLowered )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::Ready( void )
{
	// If we don't have the anim, just hide for now
	if ( SelectWeightedSequence( ACT_VM_IDLE_LOWERED ) == ACTIVITY_NOT_AVAILABLE )
	{
		RemoveEffects( EF_NODRAW );
	}

	m_bLowered = false;	
	SendWeaponAnim( ACT_VM_IDLE );

	// Prevent firing until our weapon is back up
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	pPlayer->m_flNextAttack = gpGlobals->curtime + SequenceDuration();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::Lower( void )
{
	AbortReload();

	// If we don't have the anim, just hide for now
	if ( SelectWeightedSequence( ACT_VM_IDLE_LOWERED ) == ACTIVITY_NOT_AVAILABLE )
	{
		AddEffects( EF_NODRAW );
	}

	m_bLowered = true;
	SendWeaponAnim( ACT_VM_IDLE_LOWERED );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Show/hide weapon and corresponding view model if any
// Input  : visible - 
//-----------------------------------------------------------------------------
void CTFWeaponBase::SetWeaponVisible( bool visible )
{
	if ( visible )
	{
		RemoveEffects( EF_NODRAW );
	}
	else
	{
		AddEffects( EF_NODRAW );
	}
	
#ifdef CLIENT_DLL
	UpdateVisibility();

	// Force an update
	PreDataUpdate( DATA_UPDATE_DATATABLE_CHANGED );
#endif

}

//-----------------------------------------------------------------------------
// Purpose: Allows the weapon to choose proper weapon idle animation
//-----------------------------------------------------------------------------
void CTFWeaponBase::WeaponIdle( void )
{
	//See if we should idle high or low
	if ( WeaponShouldBeLowered() )
	{
		// Move to lowered position if we're not there yet
		if ( GetActivity() != ACT_VM_IDLE_LOWERED && GetActivity() != ACT_VM_IDLE_TO_LOWERED && GetActivity() != ACT_TRANSITION )
		{
			SendWeaponAnim( ACT_VM_IDLE_LOWERED );
		}
		else if ( HasWeaponIdleTimeElapsed() )
		{
			// Keep idling low
			SendWeaponAnim( ACT_VM_IDLE_LOWERED );
		}
	}
	else
	{
		// See if we need to raise immediately
		if ( GetActivity() == ACT_VM_IDLE_LOWERED ) 
		{
			SendWeaponAnim( ACT_VM_IDLE );
		}
		else if ( HasWeaponIdleTimeElapsed() ) 
		{
			if ( !( m_bReloadsSingly && m_iReloadMode != TF_RELOAD_START ) )
			{
				SendWeaponAnim( ACT_VM_IDLE );
				m_flTimeWeaponIdle = gpGlobals->curtime + SequenceDuration();
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
const char *CTFWeaponBase::GetMuzzleFlashModel( void )
{ 
	const char *pszModel = GetTFWpnData().m_szMuzzleFlashModel;

	if ( Q_strlen( pszModel ) > 0 )
	{
		return pszModel;
	}

	return NULL;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
const char *CTFWeaponBase::GetMuzzleFlashParticleEffect( void )
{ 
	const char *pszPEffect = GetTFWpnData().m_szMuzzleFlashParticleEffect;

	if ( Q_strlen( pszPEffect ) > 0 )
	{
		return pszPEffect;
	}

	return NULL;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
float CTFWeaponBase::GetMuzzleFlashModelLifetime( void )
{ 
	return GetTFWpnData().m_flMuzzleFlashModelDuration;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFWeaponBase::GetTracerType( void )
{ 
	const char* pszTracerEffect = GetTFWpnData().m_szTracerEffect;
	if ( tf_useparticletracers.GetBool() )
	{
		if ( pszTracerEffect && pszTracerEffect[0] )
		{
			if ( !m_szTracerName[0] )
			{
				Q_snprintf( m_szTracerName, MAX_TRACER_NAME, "%s_%s", pszTracerEffect, 
					(GetOwner() && GetOwner()->GetTeamNumber() == TF_TEAM_RED ) ? "red" : "blue" );
			}

			return m_szTracerName;
		}
	}

	if ( GetWeaponID() == TF_WEAPON_MINIGUN )
		return "BrightTracer";

	return BaseClass::GetTracerType();
}

//=============================================================================
//
// TFWeaponBase functions (Server specific).
//
#if !defined( CLIENT_DLL )

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBase::CheckRespawn()
{
	// Do not respawn.
	return;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBase::Respawn()
{
	// make a copy of this weapon that is invisible and inaccessible to players (no touch function). The weapon spawn/respawn code
	// will decide when to make the weapon visible and touchable.
	CBaseEntity *pNewWeapon = CBaseEntity::Create( GetClassname(), g_pGameRules->VecWeaponRespawnSpot( this ), GetAbsAngles(), GetOwner() );

	if ( pNewWeapon )
	{
		pNewWeapon->AddEffects( EF_NODRAW );// invisible for now
		pNewWeapon->SetTouch( NULL );// no touch
		pNewWeapon->SetThink( &CTFWeaponBase::AttemptToMaterialize );

		UTIL_DropToFloor( this, MASK_SOLID );

		// not a typo! We want to know when the weapon the player just picked up should respawn! This new entity we created is the replacement,
		// but when it should respawn is based on conditions belonging to the weapon that was taken.
		pNewWeapon->SetNextThink( gpGlobals->curtime + g_pGameRules->FlWeaponRespawnTime( this ) );
	}
	else
	{
		Msg( "Respawn failed to create %s!\n", GetClassname() );
	}

	return pNewWeapon;
}

// -----------------------------------------------------------------------------
// Purpose: Make a weapon visible and tangible.
// -----------------------------------------------------------------------------
void CTFWeaponBase::Materialize()
{
	if ( IsEffectActive( EF_NODRAW ) )
	{
		RemoveEffects( EF_NODRAW );
		DoMuzzleFlash();
	}

	AddSolidFlags( FSOLID_TRIGGER );

	SetThink ( &CTFWeaponBase::SUB_Remove );
	SetNextThink( gpGlobals->curtime + 1 );
}

// -----------------------------------------------------------------------------
// Purpose: The item is trying to materialize, should it do so now or wait longer?
// -----------------------------------------------------------------------------
void CTFWeaponBase::AttemptToMaterialize()
{
	float flTime = g_pGameRules->FlWeaponTryRespawn( this );

	if ( flTime == 0 )
	{
		Materialize();
		return;
	}

	SetNextThink( gpGlobals->curtime + flTime );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBase::SetDieThink( bool bDie )
{
	if( bDie )
	{
		SetContextThink( &CTFWeaponBase::Die, gpGlobals->curtime + 30.0f, "DieContext" );
	}
	else
	{
		SetContextThink( NULL, gpGlobals->curtime, "DieContext" );
	}
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBase::Die( void )
{
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
void CTFWeaponBase::WeaponReset( void )
{
	m_iReloadMode.Set( TF_RELOAD_START );

	m_bResetParity = !m_bResetParity;
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
const Vector &CTFWeaponBase::GetBulletSpread( void )
{
	static Vector cone = VECTOR_CONE_15DEGREES;
	return cone;
}

#else

void TE_DynamicLight( IRecipientFilter& filter, float delay,
					 const Vector* org, int r, int g, int b, int exponent, float radius, float time, float decay, int nLightIndex = LIGHT_INDEX_TE_DYNAMIC );

//=============================================================================
//
// TFWeaponBase functions (Client specific).
//


bool CTFWeaponBase::IsFirstPersonView()
{
	C_TFPlayer *pPlayerOwner = GetTFPlayerOwner();
	if ( pPlayerOwner == NULL )
	{
		return false;
	}
	return pPlayerOwner->InFirstPersonView();
}

bool CTFWeaponBase::UsingViewModel()
{
	C_TFPlayer *pPlayerOwner = GetTFPlayerOwner();
	bool bIsFirstPersonView = IsFirstPersonView();
	bool bUsingViewModel = bIsFirstPersonView && ( pPlayerOwner != NULL ) && !pPlayerOwner->ShouldDrawThisPlayer();
	return bUsingViewModel;
}

C_BaseAnimating *CTFWeaponBase::GetAppropriateWorldOrViewModel()
{
	C_TFPlayer *pPlayerOwner = GetTFPlayerOwner();
	if ( pPlayerOwner && UsingViewModel() )
	{
		// Nope - it's a standard viewmodel.
		C_BaseAnimating *pViewModel = pPlayerOwner->GetViewModel();
		if ( pViewModel != NULL )
		{
			return pViewModel;
		}

		// No viewmodel, so just return the normal model.
		return this;
	}
	else
	{
		return this;
	}
}


void CTFWeaponBase::CreateMuzzleFlashEffects( C_BaseEntity *pAttachEnt, int nIndex )
{
	Vector vecOrigin;
	QAngle angAngles;

	if ( !pAttachEnt )
		return;

	if ( UsingViewModel() && !g_pClientMode->ShouldDrawViewModel() )
	{
		// Prevent effects when the ViewModel is hidden with r_drawviewmodel=0
		return;
	}

	int iMuzzleFlashAttachment = pAttachEnt->LookupAttachment( "muzzle" );

	const char *pszMuzzleFlashEffect = NULL;
	const char *pszMuzzleFlashModel = GetMuzzleFlashModel();
	const char *pszMuzzleFlashParticleEffect = GetMuzzleFlashParticleEffect();

	// Pick the right muzzleflash (3rd / 1st person)
	// (this uses IsFirstPersonView() rather than UsingViewModel() because even when NOT using the viewmodel, in 1st-person mode we still want the 1st-person muzzleflash effect)
	if ( IsFirstPersonView() )
	{
		pszMuzzleFlashEffect = GetMuzzleFlashEffectName_1st();
	}
	else
	{
		pszMuzzleFlashEffect = GetMuzzleFlashEffectName_3rd();
	}

	// If we have an attachment, then stick a light on it.
	if ( iMuzzleFlashAttachment > 0 && (pszMuzzleFlashEffect || pszMuzzleFlashModel || pszMuzzleFlashParticleEffect ) )
	{
		pAttachEnt->GetAttachment( iMuzzleFlashAttachment, vecOrigin, angAngles );

		// Muzzleflash light
/*
		CLocalPlayerFilter filter;
		TE_DynamicLight( filter, 0.0f, &vecOrigin, 255, 192, 64, 5, 70.0f, 0.05f, 70.0f / 0.05f, LIGHT_INDEX_MUZZLEFLASH );
*/

		if ( pszMuzzleFlashEffect )
		{
			// Using an muzzle flash dispatch effect
			CEffectData muzzleFlashData;
			muzzleFlashData.m_vOrigin = vecOrigin;
			muzzleFlashData.m_vAngles = angAngles;
			muzzleFlashData.m_hEntity = pAttachEnt->GetRefEHandle();
			muzzleFlashData.m_nAttachmentIndex = iMuzzleFlashAttachment;
			//muzzleFlashData.m_nHitBox = GetDODWpnData().m_iMuzzleFlashType;
			//muzzleFlashData.m_flMagnitude = GetDODWpnData().m_flMuzzleFlashScale;
			muzzleFlashData.m_flMagnitude = 0.2;
			DispatchEffect( pszMuzzleFlashEffect, muzzleFlashData );
		}

		if ( pszMuzzleFlashModel )
		{
			float flEffectLifetime = GetMuzzleFlashModelLifetime();

			// Using a model as a muzzle flash.
			if ( m_hMuzzleFlashModel[nIndex] )
			{
				// Increase the lifetime of the muzzleflash
				m_hMuzzleFlashModel[nIndex]->SetLifetime( flEffectLifetime );
			}
			else
			{
				m_hMuzzleFlashModel[nIndex] = C_MuzzleFlashModel::CreateMuzzleFlashModel( pszMuzzleFlashModel, pAttachEnt, iMuzzleFlashAttachment, flEffectLifetime );

				// FIXME: This is an incredibly brutal hack to get muzzle flashes positioned correctly for recording
				m_hMuzzleFlashModel[nIndex]->SetIs3rdPersonFlash( nIndex == 1 );
			}
		}

		if ( pszMuzzleFlashParticleEffect ) 
		{
			DispatchMuzzleFlash( pszMuzzleFlashParticleEffect, pAttachEnt );
		}
	}
}

void CTFWeaponBase::DispatchMuzzleFlash( const char* effectName, C_BaseEntity* pAttachEnt )
{
	DispatchParticleEffect( effectName, PATTACH_POINT_FOLLOW, pAttachEnt, "muzzle" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::ShouldDraw( void )
{
	C_BaseCombatCharacter *pOwner = GetOwner();
	if ( !pOwner )
		return true;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return true;

	if ( pOwner->IsPlayer() )
	{
		CTFPlayer *pTFOwner = ToTFPlayer( GetOwner() );
		if ( !pTFOwner )
			return true;

		if ( pTFOwner->m_Shared.GetDisguiseWeapon() )
		{
			if ( pTFOwner->m_Shared.InCond( TF_COND_DISGUISED ) )
			{
				int iLocalPlayerTeam = pLocalPlayer->GetTeamNumber();
				// If we are disguised we may want to draw the disguise weapon.
				if ( iLocalPlayerTeam != pOwner->GetTeamNumber() && (iLocalPlayerTeam != TEAM_SPECTATOR) )
				{
					// We are a disguised enemy, so only draw the disguise weapon.
					if ( pTFOwner->m_Shared.GetDisguiseWeapon() != this )
					{
						return false;
					}
				}
				else
				{
					// We are a disguised friendly. Don't draw the disguise weapon.
					if ( m_bDisguiseWeapon )
					{
						return false;
					}
				}
			}
			else
			{
				// We are not disguised. Never draw the disguise weapon.
				if ( m_bDisguiseWeapon )
				{
					return false;
				}
			}
		}
	}

	return BaseClass::ShouldDraw();
}

void CTFWeaponBase::UpdateVisibility( void )
{
	BaseClass::UpdateVisibility();

	C_TFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( pOwner )
	{
		pOwner->SetBodygroupsDirty();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFWeaponBase::InternalDrawModel( int flags )
{
	C_TFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	bool bNotViewModel = ( pOwner->ShouldDrawThisPlayer() );
	bool bUseInvulnMaterial = ( bNotViewModel && pOwner && pOwner->m_Shared.IsInvulnerable() );

	if ( bUseInvulnMaterial )
	{
		modelrender->ForcedMaterialOverride( *pOwner->GetInvulnMaterialRef() );
	}

	int ret = BaseClass::InternalDrawModel( flags );

	if ( bUseInvulnMaterial )
	{
		modelrender->ForcedMaterialOverride( NULL );
	}

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::OnInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	// Correct the ambient lighting position to match our owner entity
	if ( GetOwner() && pInfo )
	{
		pInfo->pLightingOrigin = &( GetOwner()->WorldSpaceCenter() );
	}

	return BaseClass::OnInternalDrawModel( pInfo );
}

void CTFWeaponBase::ProcessMuzzleFlashEvent( void )
{
	C_BaseAnimating *pAttachEnt = GetAppropriateWorldOrViewModel();
	C_TFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );

	if ( pOwner == NULL )
		return;


	bool bDrawMuzzleFlashOnViewModel = ( pAttachEnt != this );
	{
		CRecordEffectOwner recordOwner( pOwner, bDrawMuzzleFlashOnViewModel );
		CreateMuzzleFlashEffects( pAttachEnt, 0 );
	}

	// Quasi-evil
	int nModelIndex = GetModelIndex();
	int nWorldModelIndex = GetWorldModelIndex();
	bool bInToolRecordingMode = ToolsEnabled() && clienttools->IsInRecordingMode();
	if ( bInToolRecordingMode && nModelIndex != nWorldModelIndex && pOwner->IsLocalPlayer() )
	{
		CRecordEffectOwner recordOwner( pOwner, false );

		SetModelIndex( nWorldModelIndex );
		CreateMuzzleFlashEffects( this, 1 );
		SetModelIndex( nModelIndex );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
bool CTFWeaponBase::ShouldPredict()
{
	if ( GetOwner() && GetOwner() == C_BasePlayer::GetLocalPlayer() )
	{
		return true;
	}

	return BaseClass::ShouldPredict();
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
void CTFWeaponBase::WeaponReset( void )
{
	UpdateVisibility();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void CTFWeaponBase::PostDataUpdate( DataUpdateType_t updateType )
{
	UpdateModelIndex();

	BaseClass::PostDataUpdate( updateType );
}

void CTFWeaponBase::UpdateModelIndex()
{
	// We need to do this before the C_BaseAnimating code starts to drive
	// clientside animation sequences on this model, which will be using bad sequences for the world model.
	int iDesiredModelIndex = 0;
	C_BasePlayer *pOwner = ToBasePlayer(GetOwner());
	if ( !pOwner->ShouldDrawThisPlayer() )
	{
		iDesiredModelIndex = m_iViewModelIndex;
	}
	else
	{
		iDesiredModelIndex = GetWorldModelIndex();

		// Our world models never animate
		SetSequence( 0 );
	}

	if ( GetModelIndex() != iDesiredModelIndex )
	{
		SetModelIndex( iDesiredModelIndex );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
void CTFWeaponBase::OnPreDataChanged( DataUpdateType_t type )
{
	BaseClass::OnPreDataChanged( type );

	m_bOldResetParity = m_bResetParity;

}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
void CTFWeaponBase::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		ListenForGameEvent( "localplayer_changeteam" );
	}

	if ( GetPredictable() && !ShouldPredict() )
	{
		ShutdownPredictable();
	}

	//If its a world (held or dropped) model then set the correct skin color here.
	if ( m_nModelIndex == GetWorldModelIndex() )
	{
		m_nSkin = GetSkin();
	}

	if ( m_bResetParity != m_bOldResetParity )
	{
		WeaponReset();
	}
#if 0 // TF2007: This doesn't seem necessary
	if ( m_iOldTeam != m_iTeamNum )
	{
		// Recompute our tracer name
		m_szTracerName[0] = '\0';
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBase::FireGameEvent( IGameEvent *event )
{
	// If we were the active weapon, we need to update our visibility 
	// because we may switch visibility due to Spy disguises.
	const char *pszEventName = event->GetName();
	if ( Q_strcmp( pszEventName, "localplayer_changeteam" ) == 0 )
	{
		UpdateVisibility();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
int CTFWeaponBase::GetWorldModelIndex( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	// Guitar Riff taunt support.
	if ( pPlayer )
	{
		bool bReplaceModel = true;
		const char* pszCustomTauntProp = NULL;

		if ( pszCustomTauntProp )
		{
			m_iWorldModelIndex = modelinfo->GetModelIndex( pszCustomTauntProp );
		}
		else
		{
			bReplaceModel = false;
		}

		if ( bReplaceModel )
		{
			return m_iWorldModelIndex;
		}
	}

	if ( m_iCachedModelIndex == 0 )
	{
		// Remember our normal world model index so we can quickly replace it later.
		m_iCachedModelIndex = modelinfo->GetModelIndex( GetWorldModel() );
	}

	// We aren't taunting, so we want to use the cached model index.
	if ( m_iWorldModelIndex != m_iCachedModelIndex )
	{
		m_iWorldModelIndex = m_iCachedModelIndex;
	}

	if ( pPlayer )
	{
		// if we're a spy and we're disguised, we also
		// want to disguise our weapon's world model

		CTFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( !pLocalPlayer )
			return 0;

		int iLocalTeam = pLocalPlayer->GetTeamNumber();

		// We only show disguise weapon to the enemy team when owner is disguised
		bool bUseDisguiseWeapon = ( pPlayer->GetTeamNumber() != iLocalTeam && iLocalTeam > LAST_SHARED_TEAM );

		if ( bUseDisguiseWeapon && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			CTFWeaponBase *pDisguiseWeapon = pPlayer->m_Shared.GetDisguiseWeapon();
			if ( !pDisguiseWeapon )
				return BaseClass::GetWorldModelIndex();
			if ( pDisguiseWeapon == this )
				return BaseClass::GetWorldModelIndex();
			else
				return pDisguiseWeapon->GetWorldModelIndex();
		}	
	}

	return BaseClass::GetWorldModelIndex();
}

bool CTFWeaponBase::ShouldDrawCrosshair( void )
{
	const char *crosshairfile = cl_crosshair_file.GetString();
	if ( !crosshairfile || !crosshairfile[0] )
	{
		// Default crosshair.
		return GetTFWpnData().m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_bDrawCrosshair;
	}
	// Custom crosshair.
	return true;
}

void CTFWeaponBase::Redraw()
{
	if ( ShouldDrawCrosshair() && g_pClientMode->ShouldDrawCrosshair() )
	{
		DrawCrosshair();
	}
}

#endif

acttable_t s_acttablePrimary[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_PRIMARY,				false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_PRIMARY,				false },
	{ ACT_MP_DEPLOYED,			ACT_MP_DEPLOYED_PRIMARY,			false },
	{ ACT_MP_CROUCH_DEPLOYED,			ACT_MP_CROUCHWALK_DEPLOYED,	false },
	{ ACT_MP_CROUCH_DEPLOYED_IDLE,		ACT_MP_CROUCH_DEPLOYED_IDLE,	false },
	{ ACT_MP_RUN,				ACT_MP_RUN_PRIMARY,					false },
	{ ACT_MP_WALK,				ACT_MP_WALK_PRIMARY,				false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_PRIMARY,				false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_PRIMARY,			false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_PRIMARY,				false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_PRIMARY,			false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_PRIMARY,			false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_PRIMARY,			false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_PRIMARY,				false },
	{ ACT_MP_SWIM_DEPLOYED,		ACT_MP_SWIM_DEPLOYED_PRIMARY,		false },
	{ ACT_MP_DOUBLEJUMP_CROUCH,	ACT_MP_DOUBLEJUMP_CROUCH_PRIMARY,	false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_PRIMARY,	false },
	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE_DEPLOYED,		ACT_MP_ATTACK_STAND_PRIMARY_DEPLOYED, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_PRIMARY,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE_DEPLOYED,	ACT_MP_ATTACK_CROUCH_PRIMARY_DEPLOYED,	false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_PRIMARY,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_PRIMARY,	false },

	{ ACT_MP_RELOAD_STAND,		ACT_MP_RELOAD_STAND_PRIMARY,		false },
	{ ACT_MP_RELOAD_STAND_LOOP,	ACT_MP_RELOAD_STAND_PRIMARY_LOOP,	false },
	{ ACT_MP_RELOAD_STAND_END,	ACT_MP_RELOAD_STAND_PRIMARY_END,	false },
	{ ACT_MP_RELOAD_CROUCH,		ACT_MP_RELOAD_CROUCH_PRIMARY,		false },
	{ ACT_MP_RELOAD_CROUCH_LOOP,ACT_MP_RELOAD_CROUCH_PRIMARY_LOOP,	false },
	{ ACT_MP_RELOAD_CROUCH_END,	ACT_MP_RELOAD_CROUCH_PRIMARY_END,	false },
	{ ACT_MP_RELOAD_SWIM,		ACT_MP_RELOAD_SWIM_PRIMARY,			false },
	{ ACT_MP_RELOAD_SWIM_LOOP,	ACT_MP_RELOAD_SWIM_PRIMARY_LOOP,	false },
	{ ACT_MP_RELOAD_SWIM_END,	ACT_MP_RELOAD_SWIM_PRIMARY_END,		false },
	{ ACT_MP_RELOAD_AIRWALK,	ACT_MP_RELOAD_AIRWALK_PRIMARY,		false },
	{ ACT_MP_RELOAD_AIRWALK_LOOP,	ACT_MP_RELOAD_AIRWALK_PRIMARY_LOOP,	false },
	{ ACT_MP_RELOAD_AIRWALK_END,ACT_MP_RELOAD_AIRWALK_PRIMARY_END,	false },

	{ ACT_MP_GESTURE_FLINCH,	ACT_MP_GESTURE_FLINCH_PRIMARY, false },

	{ ACT_MP_GRENADE1_DRAW,		ACT_MP_PRIMARY_GRENADE1_DRAW,	false },
	{ ACT_MP_GRENADE1_IDLE,		ACT_MP_PRIMARY_GRENADE1_IDLE,	false },
	{ ACT_MP_GRENADE1_ATTACK,	ACT_MP_PRIMARY_GRENADE1_ATTACK,	false },
	{ ACT_MP_GRENADE2_DRAW,		ACT_MP_PRIMARY_GRENADE2_DRAW,	false },
	{ ACT_MP_GRENADE2_IDLE,		ACT_MP_PRIMARY_GRENADE2_IDLE,	false },
	{ ACT_MP_GRENADE2_ATTACK,	ACT_MP_PRIMARY_GRENADE2_ATTACK,	false },

	{ ACT_MP_ATTACK_STAND_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE,	false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE,	false },
	{ ACT_MP_ATTACK_SWIM_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE,	false },
	{ ACT_MP_ATTACK_AIRWALK_GRENADE,	ACT_MP_ATTACK_STAND_GRENADE,	false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_PRIMARY,	false },

	{ ACT_MP_FALLING_STOMP,	ACT_MP_FALLING_STOMP_PRIMARY,	false },
};

acttable_t s_acttableSecondary[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_SECONDARY,				false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_SECONDARY,			false },
	{ ACT_MP_RUN,				ACT_MP_RUN_SECONDARY,				false },
	{ ACT_MP_WALK,				ACT_MP_WALK_SECONDARY,				false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_SECONDARY,			false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_SECONDARY,		false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_SECONDARY,				false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_SECONDARY,		false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_SECONDARY,		false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_SECONDARY,			false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_SECONDARY,				false },
	{ ACT_MP_DOUBLEJUMP_CROUCH,	ACT_MP_DOUBLEJUMP_CROUCH_SECONDARY,	false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_SECONDARY,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_SECONDARY,		false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_SECONDARY,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_SECONDARY,	false },

	{ ACT_MP_RELOAD_STAND,		ACT_MP_RELOAD_STAND_SECONDARY,		false },
	{ ACT_MP_RELOAD_STAND_LOOP,	ACT_MP_RELOAD_STAND_SECONDARY_LOOP,	false },
	{ ACT_MP_RELOAD_STAND_END,	ACT_MP_RELOAD_STAND_SECONDARY_END,	false },
	{ ACT_MP_RELOAD_CROUCH,		ACT_MP_RELOAD_CROUCH_SECONDARY,		false },
	{ ACT_MP_RELOAD_CROUCH_LOOP,ACT_MP_RELOAD_CROUCH_SECONDARY_LOOP,false },
	{ ACT_MP_RELOAD_CROUCH_END,	ACT_MP_RELOAD_CROUCH_SECONDARY_END,	false },
	{ ACT_MP_RELOAD_SWIM,		ACT_MP_RELOAD_SWIM_SECONDARY,		false },
	{ ACT_MP_RELOAD_SWIM_LOOP,	ACT_MP_RELOAD_SWIM_SECONDARY_LOOP,	false },
	{ ACT_MP_RELOAD_SWIM_END,	ACT_MP_RELOAD_SWIM_SECONDARY_END,	false },
	{ ACT_MP_RELOAD_AIRWALK,	ACT_MP_RELOAD_AIRWALK_SECONDARY,	false },
	{ ACT_MP_RELOAD_AIRWALK_LOOP,	ACT_MP_RELOAD_AIRWALK_SECONDARY_LOOP,	false },
	{ ACT_MP_RELOAD_AIRWALK_END,ACT_MP_RELOAD_AIRWALK_SECONDARY_END,false },

	{ ACT_MP_GESTURE_FLINCH,	ACT_MP_GESTURE_FLINCH_SECONDARY, false },

	{ ACT_MP_GRENADE1_DRAW,		ACT_MP_SECONDARY_GRENADE1_DRAW,	false },
	{ ACT_MP_GRENADE1_IDLE,		ACT_MP_SECONDARY_GRENADE1_IDLE,	false },
	{ ACT_MP_GRENADE1_ATTACK,	ACT_MP_SECONDARY_GRENADE1_ATTACK,	false },
	{ ACT_MP_GRENADE2_DRAW,		ACT_MP_SECONDARY_GRENADE2_DRAW,	false },
	{ ACT_MP_GRENADE2_IDLE,		ACT_MP_SECONDARY_GRENADE2_IDLE,	false },
	{ ACT_MP_GRENADE2_ATTACK,	ACT_MP_SECONDARY_GRENADE2_ATTACK,	false },

	{ ACT_MP_ATTACK_STAND_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE,	false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE,	false },
	{ ACT_MP_ATTACK_SWIM_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE,	false },
	{ ACT_MP_ATTACK_AIRWALK_GRENADE,	ACT_MP_ATTACK_STAND_GRENADE,	false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_SECONDARY,	false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_SECONDARY,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_SECONDARY,	false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_SECONDARY,	false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_SECONDARY,	false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_SECONDARY,	false },
};

acttable_t s_acttableMelee[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_MELEE,				false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_MELEE,			false },
	{ ACT_MP_RUN,				ACT_MP_RUN_MELEE,				false },
	{ ACT_MP_WALK,				ACT_MP_WALK_MELEE,				false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_MELEE,			false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_MELEE,		false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_MELEE,				false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_MELEE,		false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_MELEE,		false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_MELEE,			false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_MELEE,				false },
	{ ACT_MP_DOUBLEJUMP_CROUCH,	ACT_MP_DOUBLEJUMP_CROUCH_MELEE,	false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_MELEE,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_MELEE,		false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_MELEE,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_MELEE,	false },

	{ ACT_MP_ATTACK_STAND_SECONDARYFIRE,	ACT_MP_ATTACK_STAND_MELEE_SECONDARY, false },
	{ ACT_MP_ATTACK_CROUCH_SECONDARYFIRE,	ACT_MP_ATTACK_CROUCH_MELEE_SECONDARY,false },
	{ ACT_MP_ATTACK_SWIM_SECONDARYFIRE,		ACT_MP_ATTACK_SWIM_MELEE,		false },
	{ ACT_MP_ATTACK_AIRWALK_SECONDARYFIRE,	ACT_MP_ATTACK_AIRWALK_MELEE,	false },

	{ ACT_MP_GESTURE_FLINCH,	ACT_MP_GESTURE_FLINCH_MELEE, false },

	{ ACT_MP_GRENADE1_DRAW,		ACT_MP_MELEE_GRENADE1_DRAW,	false },
	{ ACT_MP_GRENADE1_IDLE,		ACT_MP_MELEE_GRENADE1_IDLE,	false },
	{ ACT_MP_GRENADE1_ATTACK,	ACT_MP_MELEE_GRENADE1_ATTACK,	false },
	{ ACT_MP_GRENADE2_DRAW,		ACT_MP_MELEE_GRENADE2_DRAW,	false },
	{ ACT_MP_GRENADE2_IDLE,		ACT_MP_MELEE_GRENADE2_IDLE,	false },
	{ ACT_MP_GRENADE2_ATTACK,	ACT_MP_MELEE_GRENADE2_ATTACK,	false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_MELEE,	false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_MELEE,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_MELEE,	false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_MELEE,	false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_MELEE,	false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_MELEE,	false },

	{ ACT_MP_FALLING_STOMP,	ACT_MP_FALLING_STOMP_MELEE,	false },
};

acttable_t s_acttableBuilding[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_BUILDING,			false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_BUILDING,			false },
	{ ACT_MP_RUN,				ACT_MP_RUN_BUILDING,			false },
	{ ACT_MP_WALK,				ACT_MP_WALK_BUILDING,			false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_BUILDING,		false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_BUILDING,		false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_BUILDING,			false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_BUILDING,		false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_BUILDING,		false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_BUILDING,		false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_BUILDING,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_BUILDING,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_BUILDING,		false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_BUILDING,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_BUILDING,	false },

	{ ACT_MP_ATTACK_STAND_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE_BUILDING,	false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE_BUILDING,	false },
	{ ACT_MP_ATTACK_SWIM_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE_BUILDING,	false },
	{ ACT_MP_ATTACK_AIRWALK_GRENADE,	ACT_MP_ATTACK_STAND_GRENADE_BUILDING,	false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_BUILDING,	false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_BUILDING,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_BUILDING,	false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_BUILDING,	false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_BUILDING,	false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_BUILDING,	false },
};

acttable_t s_acttablePDA[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_PDA,			false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_PDA,			false },
	{ ACT_MP_RUN,				ACT_MP_RUN_PDA,				false },
	{ ACT_MP_WALK,				ACT_MP_WALK_PDA,			false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_PDA,			false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_PDA,		false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_PDA,			false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_PDA,		false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_PDA,		false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_PDA,		false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_PDA,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MP_ATTACK_STAND_PDA, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MP_ATTACK_SWIM_PDA, false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_PDA,	false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_PDA,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_PDA,	false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_PDA,	false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_PDA,	false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_PDA,	false },
};

ConVar mp_forceactivityset( "mp_forceactivityset", "-1", FCVAR_CHEAT|FCVAR_REPLICATED|FCVAR_DEVELOPMENTONLY );

int CTFWeaponBase::GetActivityWeaponRole() const
{
	int iWeaponRole = GetTFWpnData().m_iWeaponType;

	if ( mp_forceactivityset.GetInt() >= 0 )
	{
		iWeaponRole = mp_forceactivityset.GetInt();
	}

	return iWeaponRole;
}


acttable_t *CTFWeaponBase::ActivityList( int &iActivityCount )
{
	int iWeaponRole = GetActivityWeaponRole();

#ifdef CLIENT_DLL
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && pPlayer->IsEnemyPlayer() )
	{
		CTFWeaponBase *pDisguiseWeapon = pPlayer->m_Shared.GetDisguiseWeapon();
		if ( pDisguiseWeapon && pDisguiseWeapon != this )
		{
			return pDisguiseWeapon->ActivityList( iActivityCount );
		}
	}
#endif

	acttable_t *pTable;

	switch( iWeaponRole )
	{
	case TF_WPN_TYPE_PRIMARY:
	default:
		iActivityCount = ARRAYSIZE( s_acttablePrimary );
		pTable = s_acttablePrimary;
		break;
	case TF_WPN_TYPE_SECONDARY:
		iActivityCount = ARRAYSIZE( s_acttableSecondary );
		pTable = s_acttableSecondary;
		break;
	case TF_WPN_TYPE_MELEE:
		iActivityCount = ARRAYSIZE( s_acttableMelee );
		pTable = s_acttableMelee;
		break;
	case TF_WPN_TYPE_BUILDING:
		iActivityCount = ARRAYSIZE( s_acttableBuilding );
		pTable = s_acttableBuilding;
		break;
	case TF_WPN_TYPE_PDA:
		iActivityCount = ARRAYSIZE( s_acttablePDA );
		pTable = s_acttablePDA;
		break;
	}

	return pTable;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
CBasePlayer *CTFWeaponBase::GetPlayerOwner() const
{
	return dynamic_cast<CBasePlayer*>( GetOwner() );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
CTFPlayer *CTFWeaponBase::GetTFPlayerOwner() const
{
	return dynamic_cast<CTFPlayer*>( GetOwner() );
}

#ifdef CLIENT_DLL
// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
C_BaseEntity *CTFWeaponBase::GetWeaponForEffect()
{
	return GetAppropriateWorldOrViewModel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ShadowType_t CTFWeaponBase::ShadowCastType( void ) 
{
	// Some weapons (fists) don't actually get set to NODRAW when holstered so we
	// need some extra checks
	if ( IsEffectActive( EF_NODRAW | EF_NOSHADOW ) || m_iState != WEAPON_IS_ACTIVE )
		return SHADOWS_NONE;

	return BaseClass::ShadowCastType();
}
#endif

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFWeaponBase::CanAttack()
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( pPlayer )
		return pPlayer->CanAttack( GetCanAttackFlags() );

	return false;
}


//-----------------------------------------------------------------------------
bool CTFWeaponBase::CanFireCriticalShot( bool bIsHeadshot, CBaseEntity *pTarget /*= NULL*/ )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::CanFireRandomCriticalShot( float flCritChance )
{
#ifdef GAME_DLL
	// Todo: Create a version of this in tf_weaponbase_melee

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return false;

	PlayerStats_t *pPlayerStats = CTF_GameStats.FindPlayerStats( pPlayer );
	if ( pPlayerStats )
	{
		// Compare total damage done against total crit damage done.  If this
		// ratio is out of range for the expected crit chance, deny the crit.
		int nRandomRangedCritDamage = pPlayerStats->statsCurrentRound.m_iStat[TFSTAT_DAMAGE_RANGED_CRIT_RANDOM];
		int nTotalDamage = pPlayerStats->statsCurrentRound.m_iStat[TFSTAT_DAMAGE_RANGED];

		// Early out
		if ( !nTotalDamage )
			return true;

		float flNormalizedDamage = (float)nRandomRangedCritDamage / TF_DAMAGE_CRIT_MULTIPLIER;
		m_flObservedCritChance.Set( flNormalizedDamage / ( flNormalizedDamage + ( nTotalDamage - nRandomRangedCritDamage ) ) );

		// DevMsg ( "SERVER: CritChance: %f Observed: %f\n", flCritChance, m_flObservedCritChance.Get() );
	}
#else
		// DevMsg ( "CLIENT: CritChance: %f Observed: %f\n", flCritChance, m_flObservedCritChance.Get() );
#endif // GAME_DLL
	
	if ( m_flObservedCritChance.Get() > flCritChance + 0.1f )
		return false;

	return true;
}

#if defined( CLIENT_DLL )

static ConVar	cl_bobcycle( "cl_bobcycle","0.8", FCVAR_CHEAT );
static ConVar	cl_bobup( "cl_bobup","0.5", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: Helper function to calculate head bob
//-----------------------------------------------------------------------------
float CalcViewModelBobHelper( CBasePlayer *player, BobState_t *pBobState )
{
	Assert( pBobState );
	if ( !pBobState )
		return 0;

	float	cycle;

	// Don't allow zeros, because we divide by them.
	float flBobup = cl_bobup.GetFloat();
	if ( flBobup <= 0 )
	{
		flBobup = 0.01;
	}
	float flBobCycle = cl_bobcycle.GetFloat();
	if ( flBobCycle <= 0 )
	{
		flBobCycle = 0.01;
	}

	//NOTENOTE: For now, let this cycle continue when in the air, because it snaps badly without it

	if ( ( !gpGlobals->frametime ) || ( player == NULL ) )
	{
		//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
		return 0.0f;// just use old value
	}

	//Find the speed of the player
	float speed = player->GetLocalVelocity().Length2D();
	float flmaxSpeedDelta = MAX( 0, (gpGlobals->curtime - pBobState->m_flLastBobTime ) * 320.0f );

	// don't allow too big speed changes
	speed = clamp( speed, pBobState->m_flLastSpeed-flmaxSpeedDelta, pBobState->m_flLastSpeed+flmaxSpeedDelta );
	speed = clamp( speed, -320.f, 320.f );

	pBobState->m_flLastSpeed = speed;

	//FIXME: This maximum speed value must come from the server.
	//		 MaxSpeed() is not sufficient for dealing with sprinting - jdw

	float bob_offset = RemapVal( speed, 0, 320, 0.0f, 1.0f );

	pBobState->m_flBobTime += ( gpGlobals->curtime - pBobState->m_flLastBobTime ) * bob_offset;
	pBobState->m_flLastBobTime = gpGlobals->curtime;

	//Calculate the vertical bob
	cycle = pBobState->m_flBobTime - (int)(pBobState->m_flBobTime/flBobCycle)*flBobCycle;
	cycle /= flBobCycle;

	if ( cycle < flBobup )
	{
		cycle = M_PI * cycle / flBobup;
	}
	else
	{
		cycle = M_PI + M_PI*(cycle-flBobup)/(1.0 - flBobup);
	}

	pBobState->m_flVerticalBob = speed*0.005f;
	pBobState->m_flVerticalBob = pBobState->m_flVerticalBob*0.3 + pBobState->m_flVerticalBob*0.7*sin(cycle);

	pBobState->m_flVerticalBob = clamp( pBobState->m_flVerticalBob, -7.0f, 4.0f );

	//Calculate the lateral bob
	cycle = pBobState->m_flBobTime - (int)(pBobState->m_flBobTime/flBobCycle*2)*flBobCycle*2;
	cycle /= flBobCycle*2;

	if ( cycle < flBobup )
	{
		cycle = M_PI * cycle / flBobup;
	}
	else
	{
		cycle = M_PI + M_PI*(cycle-flBobup)/(1.0 - flBobup);
	}

	pBobState->m_flLateralBob = speed*0.005f;
	pBobState->m_flLateralBob = pBobState->m_flLateralBob*0.3 + pBobState->m_flLateralBob*0.7*sin(cycle);
	pBobState->m_flLateralBob = clamp( pBobState->m_flLateralBob, -7.0f, 4.0f );

	//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Helper function to add head bob
//-----------------------------------------------------------------------------
void AddViewModelBobHelper( Vector &origin, QAngle &angles, BobState_t *pBobState )
{
	Assert( pBobState );
	if ( !pBobState )
		return;

	Vector	forward, right;
	AngleVectors( angles, &forward, &right, NULL );

	// Apply bob, but scaled down to 40%
	VectorMA( origin, pBobState->m_flVerticalBob * 0.4f, forward, origin );

	// Z bob a bit more
	origin[2] += pBobState->m_flVerticalBob * 0.1f;

	// bob the angles
	angles[ ROLL ]	+= pBobState->m_flVerticalBob * 0.5f;
	angles[ PITCH ]	-= pBobState->m_flVerticalBob * 0.4f;
	angles[ YAW ]	-= pBobState->m_flLateralBob  * 0.3f;

	VectorMA( origin, pBobState->m_flLateralBob * 0.2f, right, origin );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CTFWeaponBase::CalcViewmodelBob( void )
{
	CBasePlayer *player = ToBasePlayer( GetOwner() );
	//Assert( player );
	BobState_t *pBobState = GetBobState();
	if ( pBobState )
		return ::CalcViewModelBobHelper( player, pBobState );
	else
		return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&angles - 
//			viewmodelindex - 
//-----------------------------------------------------------------------------
void CTFWeaponBase::AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles )
{
	// call helper functions to do the calculation
	BobState_t *pBobState = GetBobState();
	if ( pBobState )
	{
		CalcViewmodelBob();
		::AddViewModelBobHelper( origin, angles, pBobState );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns the head bob state for this weapon, which is stored
//			in the view model.  Note that this this function can return
//			NULL if the player is dead or the view model is otherwise not present.
//-----------------------------------------------------------------------------
BobState_t *CTFWeaponBase::GetBobState()
{
	// get the view model for this weapon
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner == NULL )
		return NULL;
	CBaseViewModel *baseViewModel = pOwner->GetViewModel( m_nViewModelIndex );
	if ( baseViewModel == NULL )
		return NULL;
	CTFViewModel *viewModel = dynamic_cast<CTFViewModel *>(baseViewModel);
	Assert( viewModel );

	// get the bob state out of the view model
	return &( viewModel->GetBobState() );
}

#endif // defined( CLIENT_DLL )

//-----------------------------------------------------------------------------
// Purpose: Used for spy invisiblity material, skin overrides, and team colors
//-----------------------------------------------------------------------------
int CTFWeaponBase::GetSkin()
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return 0;

	int iTeamNumber = pPlayer->GetTeamNumber();

#if defined( CLIENT_DLL )
	// Run client-only "is the viewer on the same team as the wielder" logic. Assumed to
	// always be false on the server.
	CTFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return 0;

	int iLocalTeam = pLocalPlayer->GetTeamNumber();
	
	// We only show disguise weapon to the enemy team when owner is disguised
	bool bUseDisguiseWeapon = ( iTeamNumber != iLocalTeam && iLocalTeam > LAST_SHARED_TEAM );

	if ( bUseDisguiseWeapon && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		if ( pLocalPlayer != pPlayer )
		{
			iTeamNumber = pPlayer->m_Shared.GetDisguiseTeam();
		}
	}
#endif // defined( CLIENT_DLL )

	// See if the item wants to override the skin
	int nSkin = GetSkinOverride();						// give custom gameplay code a chance to set whatever

	// If it didn't, fall back to the base skins
	if ( nSkin == -1 )
	{
		switch( iTeamNumber )
		{
		case TF_TEAM_RED:
			nSkin = 0;
			break;
		case TF_TEAM_BLUE:
			nSkin = 1;
			break;
		default:
			nSkin = 0;
			break;
		}
	}

	return nSkin;
}

#if defined( CLIENT_DLL )

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options )
{
	if ( event == 6002 && ShouldEjectBrass() )
	{
		if ( UsingViewModel() && !g_pClientMode->ShouldDrawViewModel() )
		{
			// Prevent effects when the ViewModel is hidden with r_drawviewmodel=0
			return true;
		}

		CEffectData data;
		// Look for 'eject_brass' attachment first instead of using options which is a seemingly magic number
		if ( m_iEjectBrassAttachpoint == -2 )
		{
			m_iEjectBrassAttachpoint = pViewModel->LookupAttachment( "eject_brass" );
		}

		if ( m_iEjectBrassAttachpoint > 0 )
		{
			pViewModel->GetAttachment( m_iEjectBrassAttachpoint, data.m_vOrigin, data.m_vAngles );
		}
		else
		{
			pViewModel->GetAttachment( atoi(options), data.m_vOrigin, data.m_vAngles );
		}

		data.m_nHitBox = GetWeaponID();
		DispatchEffect( "TF_EjectBrass", data );
		return true;
	}
	if ( event == AE_WPN_INCREMENTAMMO )
	{
		IncrementAmmo();

		m_bReloadedThroughAnimEvent = true;

		return true;
	}

	return BaseClass::OnFireEvent( pViewModel, origin, angles, event, options );
}

//-----------------------------------------------------------------------------
// Purpose: Used for spy invisiblity material
//-----------------------------------------------------------------------------
class CWeaponInvisProxy : public CBaseInvisMaterialProxy
{
public:
	virtual void OnBind( C_BaseEntity *pBaseEntity ) OVERRIDE;
};


extern ConVar tf_teammate_max_invis;
//-----------------------------------------------------------------------------
// Purpose: 
// Input :
//-----------------------------------------------------------------------------
void CWeaponInvisProxy::OnBind( C_BaseEntity *pBaseEntity )
{
	if( !m_pPercentInvisible )
		return;

	C_BaseEntity *pMoveParent = pBaseEntity->GetMoveParent();
	if ( !pMoveParent )
	{
		m_pPercentInvisible->SetFloatValue( 0.0f );
		return;
	}

	if ( !pMoveParent->IsPlayer() )
	{
		C_TFPlayer *pOwningPlayer = ToTFPlayer( pMoveParent->GetOwnerEntity() );
		if ( pOwningPlayer )
		{
			// mimic the owner's invisibility
			float flInvis = pOwningPlayer->GetEffectiveInvisibilityLevel();
			m_pPercentInvisible->SetFloatValue( flInvis );
		}
		else
		{
			m_pPercentInvisible->SetFloatValue( 0.0f );
		}

		return;
	}

	CTFPlayer *pPlayer = ToTFPlayer( pMoveParent );
	Assert( pPlayer );

	float flInvis = pPlayer->GetEffectiveInvisibilityLevel();
	m_pPercentInvisible->SetFloatValue( flInvis );
}

EXPOSE_INTERFACE( CWeaponInvisProxy, IMaterialProxy, "weapon_invis" IMATERIAL_PROXY_INTERFACE_VERSION );

#endif // CLIENT_DLL

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// Deliberately disabled to prevent players picking up fallen weapons.
	return;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::DisguiseWeaponThink( void )
{
	// Periodically check to make sure we are valid.
	// Disguise weapons are attached to a player, but not managed through the owned weapons list.
	CTFPlayer *pTFOwner = ToTFPlayer( GetOwner() );
	if ( !pTFOwner )
	{
		// We must have an owner to be valid.
		Drop( Vector( 0,0,0 ) );
		return;
	}

	if ( pTFOwner->m_Shared.GetDisguiseWeapon() != this )
	{
		// The owner's disguise weapon must be us, otherwise we are invalid.
		Drop( Vector( 0,0,0 ) );
		return;
	}

	SetContextThink( &CTFWeaponBase::DisguiseWeaponThink, gpGlobals->curtime + 0.5, "DisguiseWeaponThink" );
}

#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::IsViewModelFlipped( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return false;

#ifdef GAME_DLL
	if ( m_bFlipViewModel != pPlayer->m_bFlipViewModels )
	{
		return true;
	}
#else
	if ( m_bFlipViewModel != TeamFortress_ShouldFlipClientViewModel() )
	{
		return true;
	}
#endif

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Return the origin & angles for a projectile fired from the player's gun
//-----------------------------------------------------------------------------
void CTFWeaponBase::GetProjectileFireSetup( CTFPlayer *pPlayer, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammates /* = true */, float flEndDist /* = 2000 */)
{
	// @todo third person code!!

	// Flip the firing offset if our view model is flipped.
	if ( IsViewModelFlipped() )
	{
		vecOffset.y *= -1;
	}

	QAngle angSpread = GetSpreadAngles();
	Vector vecForward, vecRight, vecUp;
	AngleVectors( angSpread, &vecForward, &vecRight, &vecUp );

	Vector vecShootPos = pPlayer->Weapon_ShootPosition();

	// Estimate end point
	Vector endPos = vecShootPos + vecForward * flEndDist;	

	// Trace forward and find what's in front of us, and aim at that
	trace_t tr;

	if ( bHitTeammates )
	{
		CTraceFilterSimple traceFilter( pPlayer, COLLISION_GROUP_NONE );
		ITraceFilter *pFilterChain = NULL;

		CTraceFilterChain traceFilterChain( &traceFilter, pFilterChain );
		UTIL_TraceLine( vecShootPos, endPos, MASK_SOLID, &traceFilterChain, &tr );
	}
	else
	{
		CTraceFilterIgnoreTeammates filter( pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber() );
		UTIL_TraceLine( vecShootPos, endPos, MASK_SOLID, &filter, &tr );
	}

	// Offset actual start point
	*vecSrc = vecShootPos + (vecForward * vecOffset.x) + (vecRight * vecOffset.y) + (vecUp * vecOffset.z);

	// Find angles that will get us to our desired end point
	// Only use the trace end if it wasn't too close, which results
	// in visually bizarre forward angles
	if ( tr.fraction > 0.1 )
	{
		VectorAngles( tr.endpos - *vecSrc, *angForward );
	}
	else
	{
		VectorAngles( endPos - *vecSrc, *angForward );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
QAngle CTFWeaponBase::GetSpreadAngles( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	Assert( pOwner );

	return pOwner->EyeAngles();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::AreRandomCritsEnabled( void )
{
	return tf_weapon_criticals.GetBool();
}

bool WeaponID_IsSniperRifle( int iWeaponID )
{
	if ( iWeaponID == TF_WEAPON_SNIPERRIFLE )
		return true;
	else
		return false;
}

//-----------------------------------------------------------------------------
// Purpose: Get the current bar state (will return a value from 0.0 to 1.0)
//-----------------------------------------------------------------------------
float CTFWeaponBase::GetEffectBarProgress( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && (pPlayer->GetAmmoCount( GetEffectBarAmmo() ) < pPlayer->GetMaxAmmo( GetEffectBarAmmo() )) )
	{
		float flTime = GetEffectBarRechargeTime();
		float flProgress = (flTime - (m_flEffectBarRegenTime - gpGlobals->curtime)) / flTime;
		return flProgress;
	}

	return 1.f;
}

//-----------------------------------------------------------------------------
// Purpose: Start the regeneration bar charging from this moment in time
//-----------------------------------------------------------------------------
void CTFWeaponBase::StartEffectBarRegen( void )
{
	// Only reset regen if its less then curr time or we were full
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	bool bWasFull = false;
	if ( pPlayer && (pPlayer->GetAmmoCount( GetEffectBarAmmo() ) + 1 == pPlayer->GetMaxAmmo( GetEffectBarAmmo() ) ) )
	{
		bWasFull = true;
	}

	if ( m_flEffectBarRegenTime < gpGlobals->curtime || bWasFull ) 
	{
		m_flEffectBarRegenTime = gpGlobals->curtime + GetEffectBarRechargeTime();
	}	
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBase::CheckEffectBarRegen( void ) 
{ 
	if ( !m_flEffectBarRegenTime )
		return;
	
	// If we're full stop the timer.  Fixes a bug with "double" throws after respawning or touching a supply cab
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer->GetAmmoCount( GetEffectBarAmmo() ) == pPlayer->GetMaxAmmo( GetEffectBarAmmo() ) )
	{
		m_flEffectBarRegenTime = 0;
		return;
	}
	
	if ( m_flEffectBarRegenTime < gpGlobals->curtime ) 
	{
		m_flEffectBarRegenTime = 0;
		EffectBarRegenFinished();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBase::EffectBarRegenFinished( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && (pPlayer->GetAmmoCount( GetEffectBarAmmo() ) < pPlayer->GetMaxAmmo( GetEffectBarAmmo() )) )
	{
#ifdef GAME_DLL
		pPlayer->GiveAmmo( 1, GetEffectBarAmmo(), true );
#endif

#ifdef GAME_DLL
		// If we still have more ammo space, recharge
		if ( pPlayer->GetAmmoCount( GetEffectBarAmmo() ) < pPlayer->GetMaxAmmo( GetEffectBarAmmo() ) )
#else
		// On the client, we assume we'll get 1 more ammo as soon as the server updates us, so only restart if that still won't make us full.
		if ( pPlayer->GetAmmoCount( GetEffectBarAmmo() ) + 1 < pPlayer->GetMaxAmmo( GetEffectBarAmmo() ) )
#endif
		{
			StartEffectBarRegen();
		}
	}
}
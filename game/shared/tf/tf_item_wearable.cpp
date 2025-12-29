//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_item_wearable.h"
#include "vcollide_parse.h"
#include "tf_gamerules.h"
#include "animation.h"
#include "basecombatweapon_shared.h"
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "model_types.h"
#include "props_shared.h"
#include "usermessages.h"
#else
#include "tf_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( tf_wearable, CTFWearable );
IMPLEMENT_NETWORKCLASS_ALIASED( TFWearable, DT_TFWearable )

// Network Table --
BEGIN_NETWORK_TABLE( CTFWearable, DT_TFWearable )
#if defined( GAME_DLL )
#else
#endif // GAME_DLL
END_NETWORK_TABLE()
// -- Network Table

// Data Desc --
BEGIN_DATADESC( CTFWearable )
END_DATADESC()
// -- Data Desc

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CTFWearable )
	DEFINE_PRED_FIELD(m_nSequence, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK),
	DEFINE_PRED_FIELD(m_flPlaybackRate, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK),
	DEFINE_PRED_FIELD(m_flCycle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK),
END_PREDICTION_DATA()
#endif // CLIENT_DLL

PRECACHE_REGISTER( tf_wearable );


//-----------------------------------------------------------------------------
// SHARED CODE
//-----------------------------------------------------------------------------

CTFWearable::CTFWearable()
{
	UseClientSideAnimation();
};

//-----------------------------------------------------------------------------
// SERVER ONLY CODE
//-----------------------------------------------------------------------------

#if defined( GAME_DLL )
void CTFWearable::Break( void )
{
	CPVSFilter filter( GetAbsOrigin() );
	UserMessageBegin( filter, "BreakModel" );
		WRITE_SHORT( GetModelIndex() );
		WRITE_VEC3COORD( GetAbsOrigin() );
		WRITE_ANGLES( GetAbsAngles() );
		WRITE_SHORT( GetSkin() );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFWearable::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_FULLCHECK );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFWearable::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	if ( pInfo->m_pClientEnt && GetOwnerEntity() && CBaseEntity::Instance( pInfo->m_pClientEnt ) == GetOwnerEntity() )
	{
		return FL_EDICT_ALWAYS;
	}

	// We have some entities that have no model (ie., "hatless hats") but we still want
	// to transmit them down to clients so that the clients	can do things like update body
	// groups, etc.
	return FL_EDICT_PVSCHECK;
}

#endif

//-----------------------------------------------------------------------------
// CLIENT ONLY CODE
//-----------------------------------------------------------------------------

#if defined( CLIENT_DLL )
extern ConVar tf_playergib_forceup;

//-----------------------------------------------------------------------------
// Receive the BreakModel user message
//-----------------------------------------------------------------------------
void HandleBreakModel( bf_read &msg, bool bCheap )
{
	int nModelIndex = (int)msg.ReadShort();
	CUtlVector<breakmodel_t>	aGibs;
	BuildGibList( aGibs, nModelIndex, 1.0f, COLLISION_GROUP_NONE );
	if ( !aGibs.Count() )
		return;

	// Get the origin & angles
	Vector vecOrigin;
	QAngle vecAngles;
	int nSkin = 0;
	msg.ReadBitVec3Coord( vecOrigin );
	if ( !bCheap )
	{
		msg.ReadBitAngles( vecAngles );
		nSkin = (int)msg.ReadShort();
	}
	else
	{
		vecAngles = vec3_angle;
	}

	// Launch it straight up with some random spread
	Vector vecBreakVelocity = Vector(0,0,200);
	AngularImpulse angularImpulse( RandomFloat( 0.0f, 120.0f ), RandomFloat( 0.0f, 120.0f ), 0.0 );
	breakablepropparams_t breakParams( vecOrigin, vecAngles, vecBreakVelocity, angularImpulse );
	breakParams.impactEnergyScale = 1.0f;
	breakParams.nDefaultSkin = nSkin;

	CreateGibsFromList( aGibs, nModelIndex, NULL, breakParams, NULL, -1 , false, true );
}

//-----------------------------------------------------------------------------
// Receive the BreakModel user message
//-----------------------------------------------------------------------------
USER_MESSAGE( BreakModel )
{
	HandleBreakModel( msg, false );
}

//-----------------------------------------------------------------------------
// Receive the CheapBreakModel user message
//-----------------------------------------------------------------------------
USER_MESSAGE( CheapBreakModel )
{
	HandleBreakModel( msg, true );
}

//-----------------------------------------------------------------------------
// Receive the BreakModel_Pumpkin user message
//-----------------------------------------------------------------------------
USER_MESSAGE( BreakModel_Pumpkin )
{
	int nModelIndex = (int)msg.ReadShort();
	CUtlVector<breakmodel_t>	aGibs;
	BuildGibList( aGibs, nModelIndex, 1.0f, COLLISION_GROUP_NONE );
	if ( !aGibs.Count() )
		return;

	// Get the origin & angles
	Vector vecOrigin;
	QAngle vecAngles;
	msg.ReadBitVec3Coord( vecOrigin );
	msg.ReadBitAngles( vecAngles );

	// Launch it straight up with some random spread
	Vector vecBreakVelocity = Vector(0,0,0);
	AngularImpulse angularImpulse( RandomFloat( 0.0f, 120.0f ), RandomFloat( 0.0f, 120.0f ), 0.0 );
	breakablepropparams_t breakParams( vecOrigin /*+ Vector(0,0,20)*/, vecAngles, vecBreakVelocity, angularImpulse );
	breakParams.impactEnergyScale = 1.0f;

	for ( int i=0; i<aGibs.Count(); ++i )
	{
		aGibs[i].burstScale = 1000.f;
	}

	CUtlVector<EHANDLE>	hSpawnedGibs;
	CreateGibsFromList( aGibs, nModelIndex, NULL, breakParams, NULL, -1 , false, true, &hSpawnedGibs );

	// Make the base stay low to the ground.
	for ( int i=0; i<hSpawnedGibs.Count(); ++i )
	{
		CBaseEntity *pGib = hSpawnedGibs[i];
		if ( pGib )
		{
			IPhysicsObject *pPhysObj = pGib->VPhysicsGetObject();
			if ( pPhysObj )
			{
				Vector vecVel;
				AngularImpulse angImp;
				pPhysObj->GetVelocity( &vecVel, &angImp );
				vecVel *= 3.0;
				if ( i == 3 )
				{
					vecVel.z = 300;
				}
				else
				{
					vecVel.z = 400;
				}
				pPhysObj->SetVelocity( &vecVel, &angImp );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFWearable::InternalDrawModel( int flags )
{
	C_TFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );

	bool bUseInvulnMaterial = ( pOwner && pOwner->m_Shared.IsInvulnerable() );

	if ( bUseInvulnMaterial && (flags & STUDIO_RENDER) )
	{
		modelrender->ForcedMaterialOverride( *pOwner->GetInvulnMaterialRef() );
	}

	int ret = BaseClass::InternalDrawModel( flags );

	if ( bUseInvulnMaterial && (flags & STUDIO_RENDER) )
	{
		modelrender->ForcedMaterialOverride( NULL );
	}

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWearable::ShouldDraw()
{
	C_TFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );

	if ( pOwner )
	{
		// don't draw cosmetic while sniper is zoom
		if ( pOwner == C_TFPlayer::GetLocalTFPlayer() && pOwner->m_Shared.InCond( TF_COND_ZOOMED ) )
			return false;
	}

	// Don't draw 3rd person wearables if our owner is disguised.
	if ( pOwner && pOwner->m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		return false;
	}
	else
	{		
		return BaseClass::ShouldDraw();
	}
}
#endif

int CTFWearable::GetSkin()
{
	CTFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( !pPlayer )
		return 0;

	return BaseClass::GetSkin();
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWearable::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		ListenForGameEvent( "localplayer_changeteam" );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWearable::FireGameEvent( IGameEvent *event )
{
	const char *pszEventName = event->GetName();
	if ( Q_strcmp( pszEventName, "localplayer_changeteam" ) == 0 )
	{
		UpdateVisibility();
	}
}

#endif

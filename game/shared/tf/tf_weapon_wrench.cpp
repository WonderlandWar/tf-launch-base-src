//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_wrench.h"
#include "decals.h"
#include "baseobject_shared.h"
#include "tf_viewmodel.h"

// Client specific.
#ifdef CLIENT_DLL
	#include "c_tf_player.h"
	#include "in_buttons.h"
	// NVNT haptics system interface
	#include "haptics/ihaptics.h"
// Server specific.
#else
	#include "tf_player.h"
	#include "variant_t.h"
	#include "tf_gamerules.h"
	#include "particle_parse.h"
	#include "tf_fx.h"
	#include "tf_obj_sentrygun.h"
	#include "ilagcompensationmanager.h"
#endif

// Maximum time between robo arm hits to maintain the three-hit-combo
#define ROBOARM_COMBO_TIMEOUT 1.0f

//=============================================================================
//
// Weapon Wrench tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFWrench, DT_TFWeaponWrench )

BEGIN_NETWORK_TABLE( CTFWrench, DT_TFWeaponWrench )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWrench )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_wrench, CTFWrench );
PRECACHE_WEAPON_REGISTER( tf_weapon_wrench );

//=============================================================================
//
// Weapon Wrench functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFWrench::CTFWrench()
	: m_bReloadDown( false )
{}



//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWrench::Spawn()
{
	BaseClass::Spawn();
}


#ifdef GAME_DLL
void CTFWrench::OnFriendlyBuildingHit( CBaseObject *pObject, CTFPlayer *pPlayer, Vector hitLoc )
{
	bool bHelpTeammateBuildStructure = pObject->IsBuilding() && pObject->GetOwner() != GetOwner();

	// Did this object hit do any work? repair or upgrade?
	bool bUsefulHit = pObject->InputWrenchHit( pPlayer, this, hitLoc );

	// award achievement if we helped a teammate build a structure
	if ( bUsefulHit && bHelpTeammateBuildStructure )
	{
		CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
		if ( pOwner && pOwner->IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			pOwner->AwardAchievement( ACHIEVEMENT_TF_ENGINEER_HELP_BUILD_STRUCTURE );
		}
	}

	CDisablePredictionFiltering disabler;

	if ( bUsefulHit )
	{
		// play success sound
		WeaponSound( SPECIAL1 );
	}
	else
	{
		// play failure sound
		WeaponSound( SPECIAL2 );
	}
}
#endif

void CTFWrench::Smack( void )
{
	// see if we can hit an object with a higher range

	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

	// Setup a volume for the melee weapon to be swung - approx size, so all melee behave the same.
	static Vector vecSwingMins( -18, -18, -18 );
	static Vector vecSwingMaxs( 18, 18, 18 );

	// Setup the swing range.
	Vector vecForward; 
	AngleVectors( pPlayer->EyeAngles(), &vecForward );
	Vector vecSwingStart = pPlayer->Weapon_ShootPosition();
	Vector vecSwingEnd = vecSwingStart + vecForward * 70;

	// only trace against objects

	// See if we hit anything.
	trace_t trace;	

	CTraceFilterIgnorePlayers traceFilter( NULL, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecSwingStart, vecSwingEnd, MASK_SOLID, &traceFilter, &trace );
	if ( trace.fraction >= 1.0 )
	{
		UTIL_TraceHull( vecSwingStart, vecSwingEnd, vecSwingMins, vecSwingMaxs, MASK_SOLID, &traceFilter, &trace );
	}

	// We hit, setup the smack.
	if ( trace.fraction < 1.0f &&
		 trace.m_pEnt &&
		 trace.m_pEnt->IsBaseObject() &&
		 trace.m_pEnt->GetTeamNumber() == pPlayer->GetTeamNumber() )
	{
#ifdef GAME_DLL
		OnFriendlyBuildingHit( dynamic_cast< CBaseObject * >( trace.m_pEnt ), pPlayer, trace.endpos );
#else
		// NVNT if the local player is the owner of this wrench 
		//   Notify the haptics system we just repaired something.
		if(pPlayer==C_TFPlayer::GetLocalTFPlayer() && haptics)
			haptics->ProcessHapticEvent(2,"Weapons","tf_weapon_wrench_fix");
#endif
	}
	else
	{
		// if we cannot, Smack as usual for player hits
		BaseClass::Smack();
	}
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWrench::ItemPostFrame()
{
	BaseClass::ItemPostFrame();

	if ( !CanAttack() )
	{
		return;
	}

	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
	{
		return;
	}

	// Just pressed reload?
	if ( pOwner->m_nButtons & IN_RELOAD && !m_bReloadDown )
	{
		m_bReloadDown = true;
	}
	else if ( !(pOwner->m_nButtons & IN_RELOAD) && m_bReloadDown )
	{
		m_bReloadDown = false;
	}
}
#endif

// STAGING_ENGY
ConVar tf_construction_build_rate_multiplier( "tf_construction_build_rate_multiplier", "1.5f", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
float CTFWrench::GetConstructionValue( void )
{
	return tf_construction_build_rate_multiplier.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFWrench::GetRepairAmount( void )
{
	return 100.f;
}
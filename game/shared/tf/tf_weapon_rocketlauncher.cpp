//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Rocket Launcher
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_rocketlauncher.h"
#include "tf_fx_shared.h"
#include "tf_weaponbase_rocket.h"
#include "in_buttons.h"
#include "tf_gamerules.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include "soundenvelope.h"
#include "particle_property.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_obj_sentrygun.h"

#endif

#define BOMBARDMENT_ROCKET_MODEL "models/buildables/sentry3_rockets.mdl"

//=============================================================================
//
// Weapon Rocket Launcher tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFRocketLauncher, DT_WeaponRocketLauncher )

BEGIN_NETWORK_TABLE( CTFRocketLauncher, DT_WeaponRocketLauncher )
#ifndef CLIENT_DLL
//	SendPropInt( SENDINFO( m_iSecondaryShotsFired ) ),
#else
//	RecvPropInt( RECVINFO( m_iSecondaryShotsFired ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFRocketLauncher )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_rocketlauncher, CTFRocketLauncher );
PRECACHE_WEAPON_REGISTER( tf_weapon_rocketlauncher );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFRocketLauncher )
END_DATADESC()
#endif


//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFRocketLauncher::CTFRocketLauncher()
{
	m_bReloadsSingly = true;
	m_nReloadPitchStep = 0;

#ifdef GAME_DLL
	m_bIsOverloading = false;
#endif //GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFRocketLauncher::~CTFRocketLauncher()
{
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketLauncher::Precache()
{
	BaseClass::Precache();
	PrecacheParticleSystem( "rocketbackblast" );

	//Building_Sentrygun.FireRocket
}
#endif

//-----------------------------------------------------------------------------
bool CTFRocketLauncher::ShouldBlockPrimaryFire()
{
	return !AutoFiresFullClip();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CTFRocketLauncher::FireProjectile( CTFPlayer *pPlayer )
{
	m_flShowReloadHintAt = gpGlobals->curtime + 30;
	CBaseEntity *pRocket = BaseClass::FireProjectile( pPlayer );

	m_nReloadPitchStep = MAX( 0, m_nReloadPitchStep - 1 );

#ifdef GAME_DLL
	int iProjectile = GetWeaponProjectileType();
	if ( pPlayer->IsPlayerClass( TF_CLASS_SOLDIER ) && IsCurrentAttackARandomCrit() && ( iProjectile == TF_PROJECTILE_ROCKET ) )
	{
		// Track consecutive crit shots for achievements
		m_iConsecutiveCrits++;
		if ( m_iConsecutiveCrits == 2 )
		{
			pPlayer->AwardAchievement( ACHIEVEMENT_TF_SOLDIER_SHOOT_MULT_CRITS );
		}
	}
	else
	{
		m_iConsecutiveCrits = 0;
	}
	m_bIsOverloading = false;
#endif

	return pRocket;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketLauncher::ItemPostFrame( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	BaseClass::ItemPostFrame();

#ifdef GAME_DLL

	if ( m_flShowReloadHintAt && m_flShowReloadHintAt < gpGlobals->curtime )
	{
		if ( Clip1() < GetMaxClip1() )
		{
			pOwner->HintMessage( HINT_SOLDIER_RPG_RELOAD );
		}
		m_flShowReloadHintAt = 0;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFRocketLauncher::DefaultReload( int iClipSize1, int iClipSize2, int iActivity )
{
	m_flShowReloadHintAt = 0;
	return BaseClass::DefaultReload( iClipSize1, iClipSize2, iActivity );
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketLauncher::CreateMuzzleFlashEffects( C_BaseEntity *pAttachEnt, int nIndex )
{
	BaseClass::CreateMuzzleFlashEffects( pAttachEnt, nIndex );

	// Don't do backblast effects in first person
	C_TFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( pOwner->IsLocalPlayer() )
		return;

	ParticleProp()->Init( this );
	ParticleProp()->Create( "rocketbackblast", PATTACH_POINT_FOLLOW, "backblast" );
}
#endif
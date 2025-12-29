//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_minigun.h"
#include "decals.h"
#include "in_buttons.h"
#include "tf_fx_shared.h"
#include "debugoverlay_shared.h"
#include "tf_gamerules.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "soundenvelope.h"
#include "achievementmgr.h"
#include "baseachievement.h"
#include "achievements_tf.h"
#include "prediction.h"
#include "clientmode_tf.h"
#include "bone_setup.h"
// NVNT haptics system interface
#include "haptics/ihaptics.h"
// Server specific.
#else
#include "tf_player.h"
#include "particle_parse.h"
#include "tf_gamestats.h"
#include "baseprojectile.h"
#endif

#define MAX_BARREL_SPIN_VELOCITY	20
#define TF_MINIGUN_SPINUP_TIME 1.0

//=============================================================================
//
// Weapon Minigun tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFMinigun, DT_WeaponMinigun )

BEGIN_NETWORK_TABLE( CTFMinigun, DT_WeaponMinigun )
// Client specific.
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iWeaponState ) ),
	RecvPropBool( RECVINFO( m_bCritShot ) )
// Server specific.
#else
	SendPropInt( SENDINFO( m_iWeaponState ), 4, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropBool( SENDINFO( m_bCritShot ) )
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CTFMinigun )
	DEFINE_FIELD(  m_iWeaponState, FIELD_INTEGER ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_minigun, CTFMinigun );
PRECACHE_WEAPON_REGISTER( tf_weapon_minigun );


// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFMinigun )
END_DATADESC()
#endif

//=============================================================================
//
// Weapon Minigun functions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFMinigun::CTFMinigun()
{
#ifdef CLIENT_DLL
	m_pSoundCur = NULL;

	m_hEjectBrassWeapon = NULL;
	m_pEjectBrassEffect = NULL;
	m_iEjectBrassAttachment = -1;

	m_hMuzzleEffectWeapon = NULL;
	m_pMuzzleEffect = NULL;
	m_iMuzzleAttachment = -1;

	m_nShotsFired = 0;

	ListenForGameEvent( "teamplay_round_active" );
	ListenForGameEvent( "localplayer_respawn" );
#endif

	WeaponReset();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTFMinigun::~CTFMinigun()
{
	WeaponReset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::WeaponReset( void )
{
	BaseClass::WeaponReset();

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer )
	{
		pPlayer->m_Shared.RemoveCond( TF_COND_AIMING );
		pPlayer->TeamFortress_SetSpeed();

#ifdef GAME_DLL
		pPlayer->ClearWeaponFireScene();
		m_flAegisCheckTime = 0.0f;
#endif

		m_flNextRingOfFireAttackTime = 0.0f;
		m_flLastAmmoDrainTime = gpGlobals->curtime;
		m_flAccumulatedAmmoDrain = 0.0f;
	}

	SetWeaponState( AC_STATE_IDLE );
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	m_bCritShot = false;
	m_flStartedFiringAt = -1.0f;
	m_flNextFiringSpeech = 0.0f;

	m_flBarrelAngle = 0.0f;

	m_flBarrelCurrentVelocity = 0.0f;
	m_flBarrelTargetVelocity = 0.0f;

#ifdef CLIENT_DLL
	if ( m_pSoundCur )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pSoundCur );
		m_pSoundCur = NULL;
	}

	m_iMinigunSoundCur = -1;
	m_flMinigunSoundCurrentPitch = 1.0f;

	StopMuzzleEffect();
	StopBrassEffect();
#endif
}

#ifdef GAME_DLL
int CTFMinigun::UpdateTransmitState( void )
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::Precache( void )
{
	PrecacheScriptSound( "Halloween.HeadlessBossAxeHitWorld" );

	// FIXME: Do we still need these??
	PrecacheScriptSound( "MVM.GiantHeavyGunWindUp" );
	PrecacheScriptSound( "MVM.GiantHeavyGunWindDown" );
	PrecacheScriptSound( "MVM.GiantHeavyGunFire" );
	PrecacheScriptSound( "MVM.GiantHeavyGunSpin" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::ItemPostFrame( void )
{
	// Prevent base code from ever playing empty sounds, minigun handles them manually.
	m_flNextEmptySoundTime = gpGlobals->curtime + 1.0;

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMinigun::PrimaryAttack()
{
	SharedAttack();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::SharedAttack()
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
	{
		WeaponIdle();
		return;
	}

	if ( pPlayer->m_nButtons & IN_ATTACK )
	{
		m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	}
	else if ( pPlayer->m_nButtons & IN_ATTACK2 )
	{
		m_iWeaponMode = TF_WEAPON_SECONDARY_MODE;
	}

	switch ( m_iWeaponState )
	{
	default:
	case AC_STATE_IDLE:
		{
			// Removed the need for cells to powerup the AC
			WindUp();

			float flSpinUpTime = TF_MINIGUN_SPINUP_TIME;
			float flSpinTimeMultiplier = flSpinUpTime;
			if ( pPlayer->GetViewModel( 0 ) )
			{
				pPlayer->GetViewModel( 0 )->SetPlaybackRate( TF_MINIGUN_SPINUP_TIME / flSpinTimeMultiplier );
			}
			if ( pPlayer->GetViewModel( 1 ) )
			{
				pPlayer->GetViewModel( 1 )->SetPlaybackRate( TF_MINIGUN_SPINUP_TIME / flSpinTimeMultiplier );
			}

			m_flNextPrimaryAttack = gpGlobals->curtime + flSpinUpTime;
			m_flNextSecondaryAttack = gpGlobals->curtime + flSpinUpTime;
			m_flTimeWeaponIdle = gpGlobals->curtime + flSpinUpTime;
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRE );
			break;
		}
	case AC_STATE_STARTFIRING:
		{
			// Start playing the looping fire sound
			if ( m_flNextPrimaryAttack <= gpGlobals->curtime )
			{
				if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
				{
					SetWeaponState( AC_STATE_SPINNING );
				}
				else
				{
					SetWeaponState( AC_STATE_FIRING );
				}

#ifdef GAME_DLL
				if ( m_iWeaponState == AC_STATE_SPINNING )
				{
					pPlayer->SpeakWeaponFire( MP_CONCEPT_WINDMINIGUN );
				}
				else
				{
					pPlayer->SpeakWeaponFire( MP_CONCEPT_FIREMINIGUN );
				}
#endif

				m_flNextSecondaryAttack = m_flNextPrimaryAttack = m_flTimeWeaponIdle = gpGlobals->curtime + 0.1;
			}
			break;
		}
	case AC_STATE_FIRING:
		{
			if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
			{
				SetWeaponState( AC_STATE_SPINNING );
			}

			if ( m_iWeaponState == AC_STATE_SPINNING )
			{
#ifdef GAME_DLL
				pPlayer->ClearWeaponFireScene();
				pPlayer->SpeakWeaponFire( MP_CONCEPT_WINDMINIGUN );
#endif
				m_flNextSecondaryAttack = m_flNextPrimaryAttack = m_flTimeWeaponIdle = gpGlobals->curtime + 0.1;

			}
			else if ( pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0 )
			{
				SetWeaponState( AC_STATE_DRYFIRE );
			}
			else
			{
				if ( m_flStartedFiringAt < 0 )
				{
					m_flStartedFiringAt = gpGlobals->curtime;
				}

#ifdef GAME_DLL
				if ( m_flNextFiringSpeech < gpGlobals->curtime )
				{
					m_flNextFiringSpeech = gpGlobals->curtime + 5.0;
					pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_MINIGUN_FIREWEAPON );
				}
#endif

#ifdef CLIENT_DLL
				int nAmmo = 0;
				if ( prediction->IsFirstTimePredicted() && 
					 C_BasePlayer::GetLocalPlayer() == pPlayer )
				{
					nAmmo = pPlayer->GetAmmoCount( m_iPrimaryAmmoType );
				}
#endif

				// Only fire if we're actually shooting
				BaseClass::PrimaryAttack();		// fire and do timers
				
#ifdef CLIENT_DLL
				if ( prediction->IsFirstTimePredicted() && 
					 C_BasePlayer::GetLocalPlayer() == pPlayer &&
					 nAmmo != pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) ) // did PrimaryAttack() fire a shot? (checking our ammo to find out)
				{
					m_nShotsFired++;
					if ( m_nShotsFired == 1000 ) // == and not >= so we don't keep awarding this every shot after it's achieved
					{
						g_AchievementMgrTF.OnAchievementEvent( ACHIEVEMENT_TF_HEAVY_FIRE_LOTS );
					}
					// NVNT the local player fired a shot. notify the haptics system.
					if ( haptics )
						haptics->ProcessHapticEvent(2,"Weapons","minigun_fire");
				}
#endif
				CalcIsAttackCritical();
				m_bCritShot = IsCurrentAttackACrit();
				pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

				m_flTimeWeaponIdle = gpGlobals->curtime + 0.2;
			}
			break;
		}
	case AC_STATE_DRYFIRE:
		{
			m_flStartedFiringAt = -1.f;

			if ( pPlayer->GetAmmoCount(m_iPrimaryAmmoType) > 0 )
			{
				SetWeaponState( AC_STATE_FIRING );
			}
			else if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
			{
				SetWeaponState( AC_STATE_SPINNING );
			}
			SendWeaponAnim( ACT_VM_SECONDARYATTACK );
			break;
		}
	case AC_STATE_SPINNING:
		{
			m_flStartedFiringAt = -1.f;

			if ( m_iWeaponMode == TF_WEAPON_PRIMARY_MODE )
			{
				if ( pPlayer->GetAmmoCount(m_iPrimaryAmmoType) > 0 )
				{
#ifdef GAME_DLL
					pPlayer->ClearWeaponFireScene();
					pPlayer->SpeakWeaponFire( MP_CONCEPT_FIREMINIGUN );
#endif
					SetWeaponState( AC_STATE_FIRING );
				}
				else
				{
					SetWeaponState( AC_STATE_DRYFIRE );
				}
			}

			SendWeaponAnim( ACT_VM_SECONDARYATTACK );
			break;
		}
	}
}

void CTFMinigun::SetWeaponState( MinigunState_t nState )
{
	if ( m_iWeaponState != nState )
	{
		if ( m_iWeaponState == AC_STATE_IDLE || m_iWeaponState == AC_STATE_STARTFIRING || m_iWeaponState == AC_STATE_DRYFIRE )
		{
			// Transitioning from non firing or non fully spinning states resets when our drain start point and when the ring of fire can start
			m_flLastAmmoDrainTime = gpGlobals->curtime;
			m_flNextRingOfFireAttackTime = gpGlobals->curtime + 0.5f;
		}
		
		m_iWeaponState = nState;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fall through to Primary Attack
//-----------------------------------------------------------------------------
void CTFMinigun::SecondaryAttack( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	SharedAttack();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMinigun::WindUp( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	// Play wind-up animation and sound (SPECIAL1).
	SendWeaponAnim( ACT_MP_ATTACK_STAND_PREFIRE );

	// Set the appropriate firing state.
	SetWeaponState( AC_STATE_STARTFIRING );
	pPlayer->m_Shared.AddCond( TF_COND_AIMING );

#ifndef CLIENT_DLL
	pPlayer->StopRandomExpressions();
#endif

#ifdef CLIENT_DLL 
	WeaponSoundUpdate();
#endif

	// Update player's speed
	pPlayer->TeamFortress_SetSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFMinigun::CanHolster( void ) const
{
	if ( m_iWeaponState > AC_STATE_IDLE )
		return false;

	if ( GetActivity() == ACT_MP_ATTACK_STAND_POSTFIRE || GetActivity() == ACT_PRIMARY_ATTACK_STAND_POSTFIRE )
	{
		if ( !IsViewModelSequenceFinished() )
			return false;
	}

	return BaseClass::CanHolster();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFMinigun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( m_iWeaponState > AC_STATE_IDLE )
	{
		WindDown();
	}

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFMinigun::Lower( void )
{
	if ( m_iWeaponState > AC_STATE_IDLE )
	{
		WindDown();
	}

	return BaseClass::Lower();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMinigun::WindDown( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	SendWeaponAnim( ACT_MP_ATTACK_STAND_POSTFIRE );

	// Set the appropriate firing state.
	SetWeaponState( AC_STATE_IDLE );
	pPlayer->m_Shared.RemoveCond( TF_COND_AIMING );
#ifdef CLIENT_DLL
	WeaponSoundUpdate();
#else
	pPlayer->ClearWeaponFireScene();
#endif

	// Time to weapon idle.
	m_flTimeWeaponIdle = gpGlobals->curtime + 2.0;

	// Update player's speed
	pPlayer->TeamFortress_SetSpeed();

#ifdef CLIENT_DLL
	m_flBarrelTargetVelocity = 0;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer && GetOwner() == pLocalPlayer )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "localplayer_winddown" );
		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMinigun::WeaponIdle()
{
	if ( gpGlobals->curtime < m_flTimeWeaponIdle )
		return;

	// Always wind down if we've hit here, because it only happens when the player has stopped firing/spinning
	if ( m_iWeaponState != AC_STATE_IDLE )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
		if ( pPlayer )
		{
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_POST );
		}

		WindDown();
		return;
	}

	BaseClass::WeaponIdle();

	m_flTimeWeaponIdle = gpGlobals->curtime + 12.5;// how long till we do this again.
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMinigun::FireGameEvent( IGameEvent * event )
{
#ifdef CLIENT_DLL
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pLocalPlayer && GetOwner() == pLocalPlayer )
	{
		if ( FStrEq( event->GetName(), "teamplay_round_active" ) ||
			 FStrEq( event->GetName(), "localplayer_respawn" ) )
		{
			m_nShotsFired = 0;
		}
	}

	BaseClass::FireGameEvent( event );
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFMinigun::SendWeaponAnim( int iActivity )
{
#ifdef CLIENT_DLL
	// Client procedurally animates the barrel bone
	if ( iActivity == ACT_MP_ATTACK_STAND_PRIMARYFIRE || iActivity == ACT_MP_ATTACK_STAND_PREFIRE )
	{
		m_flBarrelTargetVelocity = MAX_BARREL_SPIN_VELOCITY;
	}
	else if ( iActivity == ACT_MP_ATTACK_STAND_POSTFIRE )
	{
		m_flBarrelTargetVelocity = 0;
	}

#endif


	// When we start firing, play the startup firing anim first
	if ( iActivity == ACT_VM_PRIMARYATTACK )
	{
		// If we're already playing the fire anim, let it continue. It loops.
		if ( GetActivity() == ACT_VM_PRIMARYATTACK )
			return true;

		// Otherwise, play the start it
		return BaseClass::SendWeaponAnim( ACT_VM_PRIMARYATTACK );		
	}

	return BaseClass::SendWeaponAnim( iActivity );
}

//-----------------------------------------------------------------------------
// Purpose: This will force the minigun to turn off the firing sound and play the spinning sound
//-----------------------------------------------------------------------------
void CTFMinigun::HandleFireOnEmpty( void )
{
	if ( m_iWeaponState == AC_STATE_FIRING || m_iWeaponState == AC_STATE_SPINNING )
	{
		 SetWeaponState( AC_STATE_DRYFIRE );

		 SendWeaponAnim( ACT_VM_SECONDARYATTACK );

		 if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
		 {
			SetWeaponState ( AC_STATE_SPINNING );
		 }
	}
}


#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStudioHdr *CTFMinigun::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	m_iBarrelBone = LookupBone( "barrel" );

	// skip resetting this while recording in the tool
	// we change the weapon to the worldmodel and back to the viewmodel when recording
	// which causes the minigun to not spin while recording
	if ( !IsToolRecording() )
	{
		m_flBarrelAngle = 0;

		m_flBarrelCurrentVelocity = 0;
		m_flBarrelTargetVelocity = 0;
	}

	return hdr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask )
{
	BaseClass::StandardBlendingRules( hdr, pos, q, currentTime, boneMask );

	if (m_iBarrelBone != -1)
	{
		UpdateBarrelMovement();

		// Weapon happens to be aligned to (0,0,0)
		// If that changes, use this code block instead to
		// modify the angles

		/*
		RadianEuler a;
		QuaternionAngles( q[iBarrelBone], a );

		a.x = m_flBarrelAngle;

		AngleQuaternion( a, q[iBarrelBone] );
		*/

		AngleQuaternion( RadianEuler( 0, 0, m_flBarrelAngle ), q[m_iBarrelBone] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the velocity and position of the rotating barrel
//-----------------------------------------------------------------------------
void CTFMinigun::UpdateBarrelMovement()
{
	if ( prediction->InPrediction() && !prediction->IsFirstTimePredicted() )
	{
		return;
	}

	if ( m_flBarrelCurrentVelocity != m_flBarrelTargetVelocity )
	{
		float flBarrelAcceleration = 0.1f;

		// update barrel velocity to bring it up to speed or to rest
		m_flBarrelCurrentVelocity = Approach( m_flBarrelTargetVelocity, m_flBarrelCurrentVelocity, flBarrelAcceleration );

		if ( 0 == m_flBarrelCurrentVelocity )
		{	
			// if we've stopped rotating, turn off the wind-down sound
			WeaponSoundUpdate();
		}
	}

	// update the barrel rotation based on current velocity
	m_flBarrelAngle += m_flBarrelCurrentVelocity * gpGlobals->frametime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::OnDataChanged( DataUpdateType_t updateType )
{
	// Brass ejection and muzzle flash.
	HandleBrassEffect();
	HandleMuzzleEffect();

	BaseClass::OnDataChanged( updateType );

	WeaponSoundUpdate();

	m_iPrevMinigunState = m_iWeaponState;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::UpdateOnRemove( void )
{
	if ( m_pSoundCur )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pSoundCur );
		m_pSoundCur = NULL;
	}

	// Force the particle system off.
	StopMuzzleEffect();
	StopBrassEffect();

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::SetDormant( bool bDormant )
{
	// If I'm going from active to dormant and I'm carried by another player, stop our firing sound.
	if ( !IsCarriedByLocalPlayer() )
	{
		// Am I firing? Stop the firing sound.
		if ( !IsDormant() && bDormant && m_iWeaponState >= AC_STATE_FIRING )
		{
			WeaponSoundUpdate();
		}

		// If firing and going dormant - stop the brass effect.
		if ( !IsDormant() && bDormant && m_iWeaponState != AC_STATE_IDLE )
		{
			StopMuzzleEffect();
			StopBrassEffect();
		}
	}

	// Deliberately skip base combat weapon
	C_BaseEntity::SetDormant( bDormant );
}


//-----------------------------------------------------------------------------
// Purpose: 
// won't be called for w_ version of the model, so this isn't getting updated twice
//-----------------------------------------------------------------------------
void CTFMinigun::ItemPreFrame( void )
{
	UpdateBarrelMovement();
	BaseClass::ItemPreFrame();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::StartBrassEffect()
{
	StopBrassEffect();

	m_hEjectBrassWeapon = GetWeaponForEffect();
	if ( !m_hEjectBrassWeapon )
		return;

	if ( UsingViewModel() && !g_pClientMode->ShouldDrawViewModel() )
	{
		// Prevent effects when the ViewModel is hidden with r_drawviewmodel=0
		return;
	}

	// Try and setup the attachment point if it doesn't already exist.
	// This caching will mess up if we go third person from first - we only do this in taunts and don't fire so we should
	// be okay for now.
	if ( m_iEjectBrassAttachment == -1 )
	{
		m_iEjectBrassAttachment = m_hEjectBrassWeapon->LookupAttachment( "eject_brass" );
	}

	// Start the brass ejection, if a system hasn't already been started.
	if ( m_iEjectBrassAttachment > 0 && m_pEjectBrassEffect == NULL )
	{
		m_pEjectBrassEffect = m_hEjectBrassWeapon->ParticleProp()->Create( "eject_minigunbrass", PATTACH_POINT_FOLLOW, m_iEjectBrassAttachment );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::StartMuzzleEffect()
{
	StopMuzzleEffect();

	m_hMuzzleEffectWeapon = GetWeaponForEffect();
	if ( !m_hMuzzleEffectWeapon )
		return;

	if ( UsingViewModel() && !g_pClientMode->ShouldDrawViewModel() )
	{
		// Prevent effects when the ViewModel is hidden with r_drawviewmodel=0
		return;
	}

	// Try and setup the attachment point if it doesn't already exist.
	// This caching will mess up if we go third person from first - we only do this in taunts and don't fire so we should
	// be okay for now.
	if ( m_iMuzzleAttachment <= 0 )
	{
		m_iMuzzleAttachment = m_hMuzzleEffectWeapon->LookupAttachment( "muzzle" );
	}

	// Start the muzzle flash, if a system hasn't already been started.
	if ( m_iMuzzleAttachment > 0 && m_pMuzzleEffect == NULL )
	{
		m_pMuzzleEffect = m_hMuzzleEffectWeapon->ParticleProp()->Create( "muzzle_minigun_constant", PATTACH_POINT_FOLLOW, m_iMuzzleAttachment );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::StopBrassEffect()
{
	if ( !m_hEjectBrassWeapon )
		return;

	// Stop the brass ejection.
	if ( m_pEjectBrassEffect )
	{
		m_hEjectBrassWeapon->ParticleProp()->StopEmission( m_pEjectBrassEffect );
		m_hEjectBrassWeapon = NULL;
		m_pEjectBrassEffect = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::StopMuzzleEffect()
{
	if ( !m_hMuzzleEffectWeapon )
		return;

	// Stop the muzzle flash.
	if ( m_pMuzzleEffect )
	{
		m_hMuzzleEffectWeapon->ParticleProp()->StopEmission( m_pMuzzleEffect );
		m_hMuzzleEffectWeapon = NULL;
		m_pMuzzleEffect		  = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::HandleBrassEffect()
{
	if ( m_iWeaponState == AC_STATE_FIRING && m_pEjectBrassEffect == NULL )
	{
		StartBrassEffect();
	}
	else if ( m_iWeaponState != AC_STATE_FIRING && m_pEjectBrassEffect )
	{
		StopBrassEffect();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::HandleMuzzleEffect()
{
	if ( m_iWeaponState == AC_STATE_FIRING && m_pMuzzleEffect == NULL )
	{
		StartMuzzleEffect();
	}
	else if ( m_iWeaponState != AC_STATE_FIRING && m_pMuzzleEffect )
	{
		StopMuzzleEffect();
	}
}

//-----------------------------------------------------------------------------
// Purpose: View model barrel rotation angle. Calculated here, implemented in 
// tf_viewmodel.cpp
//-----------------------------------------------------------------------------
float CTFMinigun::GetBarrelRotation( void )
{
	return m_flBarrelAngle;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::ViewModelAttachmentBlending( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask )
{
	int iBarrelBone = Studio_BoneIndexByName( hdr, "barrel" );

	// Assert( iBarrelBone != -1 );

	if ( iBarrelBone != -1 )
	{
		if ( hdr->boneFlags( iBarrelBone ) & boneMask )
		{
			RadianEuler a;
			QuaternionAngles( q[iBarrelBone], a );

			a.z = GetBarrelRotation();

			AngleQuaternion( a, q[iBarrelBone] );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMinigun::CreateMove( float flInputSampleTime, CUserCmd *pCmd, const QAngle &vecOldViewAngles )
{
	// Prevent jumping while firing
	if ( m_iWeaponState != AC_STATE_IDLE )
	{
		pCmd->buttons &= ~IN_JUMP;
	}

	BaseClass::CreateMove( flInputSampleTime, pCmd, vecOldViewAngles );
}

//-----------------------------------------------------------------------------
// Purpose: Ensures the correct sound (including silence) is playing for 
//			current weapon state.
//-----------------------------------------------------------------------------
void CTFMinigun::WeaponSoundUpdate()
{
	// determine the desired sound for our current state
	int iSound = -1;
	switch ( m_iWeaponState )
	{
	case AC_STATE_IDLE:
		if ( m_flBarrelCurrentVelocity > 0 )
		{
			iSound = SPECIAL2;	// wind down sound

			if ( m_flBarrelTargetVelocity > 0 )
			{
				m_flBarrelTargetVelocity = 0;
			}
		}
		else
		{
			iSound = -1;
		}
		break;
	case AC_STATE_STARTFIRING:
			iSound = SPECIAL1;	// wind up sound
		break;
	case AC_STATE_FIRING:
		{
			if ( m_bCritShot == true ) 
			{
				iSound = BURST;	// Crit sound
			}
			else
			{
				iSound = WPN_DOUBLE; // firing sound
			}
		}
		break;
	case AC_STATE_SPINNING:
		iSound = SPECIAL3;	// spinning sound
		break;
	case AC_STATE_DRYFIRE:
		iSound = EMPTY;		// out of ammo, still trying to fire
		break;
	default:
		Assert( false );
		break;
	}

	// Get the pitch we should play at
	float flPitch = 1.0f;

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	// if we're already playing the desired sound, nothing to do
	if ( m_iMinigunSoundCur == iSound )
	{
		// If the pitch is different we need to modify it
		if ( m_flMinigunSoundCurrentPitch != flPitch )
		{
			m_flMinigunSoundCurrentPitch = flPitch;

			if ( m_pSoundCur )
			{
				controller.SoundChangePitch( m_pSoundCur, m_flMinigunSoundCurrentPitch, 0.3f );
			}
		}
		return;
	}

	// if we're playing some other sound, stop it
	if ( m_pSoundCur )
	{
		// Stop the previous sound immediately
		CSoundEnvelopeController::GetController().SoundDestroy( m_pSoundCur );
		m_pSoundCur = NULL;
	}
	m_iMinigunSoundCur = iSound;
	// if there's no sound to play for current state, we're done
	if ( -1 == iSound )
		return;

	m_flMinigunSoundCurrentPitch = flPitch;

	// play the appropriate sound
	const char *shootsound = GetShootSound( iSound );
	CLocalPlayerFilter filter;
	m_pSoundCur = controller.SoundCreate( filter, entindex(), shootsound );
	controller.Play( m_pSoundCur, 1.0, 100 );
	controller.SoundChangeVolume( m_pSoundCur, 1.0, 0.1 );

	if ( m_flMinigunSoundCurrentPitch != 1.0f )
	{
		controller.SoundChangePitch( m_pSoundCur, m_flMinigunSoundCurrentPitch, 0.0 );
	}
}

void CTFMinigun::PlayStopFiringSound()
{
	if ( m_pSoundCur )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pSoundCur );
		m_pSoundCur = NULL;
	}

	m_iMinigunSoundCur = SPECIAL2;

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	const char *shootsound = GetShootSound( SPECIAL2 );
	CLocalPlayerFilter filter;
	m_pSoundCur = controller.SoundCreate( filter, entindex(), shootsound );
	controller.Play( m_pSoundCur, 1.0, 100 );
	controller.SoundChangeVolume( m_pSoundCur, 1.0, 0.1 );
}

#endif

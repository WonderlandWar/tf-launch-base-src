//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tier0/vprof.h"
#include "animation.h"
#include "studio.h"
#include "apparent_velocity_helper.h"
#include "utldict.h"
#include "tf_playeranimstate.h"
#include "base_playeranimstate.h"
#include "datacache/imdlcache.h"
#include "tf_gamerules.h"
#include "in_buttons.h"
#include "debugoverlay_shared.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "c_func_capture_zone.h"

#define CCaptureZone C_CaptureZone
#else
#include "tf_player.h"
#include "func_capture_zone.h"
#endif

#define TF_RUN_SPEED			320.0f
#define TF_WALK_SPEED			75.0f
#define TF_CROUCHWALK_SPEED		110.0f

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
// Output : CMultiPlayerAnimState*
//-----------------------------------------------------------------------------
CTFPlayerAnimState* CreateTFPlayerAnimState( CTFPlayer *pPlayer )
{
	MDLCACHE_CRITICAL_SECTION();

	// Setup the movement data.
	MultiPlayerMovementData_t movementData;
	movementData.m_flBodyYawRate = 720.0f;
	movementData.m_flRunSpeed = TF_RUN_SPEED;
	movementData.m_flWalkSpeed = TF_WALK_SPEED;
	movementData.m_flSprintSpeed = -1.0f;

	// Create animation state for this player.
	CTFPlayerAnimState *pRet = new CTFPlayerAnimState( pPlayer, movementData );

	// Specific TF player initialization.
	pRet->InitTF( pPlayer );

	return pRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFPlayerAnimState::CTFPlayerAnimState()
{
	m_pTFPlayer = NULL;

	// Don't initialize TF specific variables here. Init them in InitTF()
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//			&movementData - 
//-----------------------------------------------------------------------------
CTFPlayerAnimState::CTFPlayerAnimState( CBasePlayer *pPlayer, MultiPlayerMovementData_t &movementData )
: CMultiPlayerAnimState( pPlayer, movementData )
{
	m_pTFPlayer = NULL;

	// Don't initialize TF specific variables here. Init them in InitTF()
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFPlayerAnimState::~CTFPlayerAnimState()
{
}

//-----------------------------------------------------------------------------
// Purpose: Initialize Team Fortress specific animation state.
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CTFPlayerAnimState::InitTF( CTFPlayer *pPlayer )
{
	m_pTFPlayer = pPlayer;
	m_bInAirWalk = false;
	m_flHoldDeployedPoseUntilTime = 0.0f;
	m_flTauntMoveX = 0.f;
	m_flTauntMoveY = 0.f;
	m_vecSmoothedUp = Vector( 0.f, 0.f, 1.f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerAnimState::ClearAnimationState( void )
{
	m_bInAirWalk = false;

	BaseClass::ClearAnimationState();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : actDesired - 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CTFPlayerAnimState::TranslateActivity( Activity actDesired )
{
	Activity translateActivity = BaseClass::TranslateActivity( actDesired );

	translateActivity = ActivityOverride( translateActivity, NULL );

	CBaseCombatWeapon *pWeapon = GetTFPlayer()->GetActiveWeapon();
	if ( pWeapon )
	{
		translateActivity = pWeapon->ActivityOverride( translateActivity, NULL );
	}

	return translateActivity;
}

static acttable_t s_acttableCompetitiveLoserState[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MP_COMPETITIVE_LOSERSTATE, false },
};

static acttable_t s_acttableBuildingDeployed[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_BUILDING_DEPLOYED,			false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_BUILDING_DEPLOYED,		false },
	{ ACT_MP_RUN,				ACT_MP_RUN_BUILDING_DEPLOYED,			false },
	{ ACT_MP_WALK,				ACT_MP_WALK_BUILDING_DEPLOYED,			false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_BUILDING_DEPLOYED,		false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_BUILDING_DEPLOYED,	false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_BUILDING_DEPLOYED,			false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_BUILDING_DEPLOYED,	false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_BUILDING_DEPLOYED,	false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_BUILDING_DEPLOYED,		false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_BUILDING_DEPLOYED,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_BUILDING_DEPLOYED,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_BUILDING_DEPLOYED,		false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_BUILDING_DEPLOYED,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_BUILDING_DEPLOYED,	false },

	{ ACT_MP_ATTACK_STAND_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE_BUILDING_DEPLOYED,	false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE_BUILDING_DEPLOYED,	false },
	{ ACT_MP_ATTACK_SWIM_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE_BUILDING_DEPLOYED,	false },
	{ ACT_MP_ATTACK_AIRWALK_GRENADE,	ACT_MP_ATTACK_STAND_GRENADE_BUILDING_DEPLOYED,	false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_BUILDING,		false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_BUILDING,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_BUILDING,		false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_BUILDING,		false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_BUILDING,				false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_BUILDING,				false },
};

Activity CTFPlayerAnimState::ActivityOverride( Activity baseAct, bool *pRequired )
{
	acttable_t *pTable = NULL;
	int iActivityCount = 0;

	for ( int i = 0; i < iActivityCount; i++ )
	{
		const acttable_t& act = pTable[i];
		if ( baseAct == act.baseAct )
		{
			if (pRequired)
			{
				*pRequired = act.required;
			}
			return (Activity)act.weaponAct;
		}
	}

	return baseAct;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerAnimState::ShouldUpdateAnimState( void )
{
	CTFPlayer *pTFPlayer = GetTFPlayer();
	if ( pTFPlayer )
	{
		// Stop animating if we have a custom player model that doesn't use the normal class animations
		if ( pTFPlayer->GetPlayerClass()->HasCustomModel() && !pTFPlayer->GetPlayerClass()->CustomModelUsesClassAnimations() )
			return false;
	}

	return BaseClass::ShouldUpdateAnimState();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerAnimState::Update( float eyeYaw, float eyePitch )
{
	// Profile the animation update.
	VPROF( "CMultiPlayerAnimState::Update" );

	// Get the TF player.
	CTFPlayer *pTFPlayer = GetTFPlayer();
	if ( !pTFPlayer )
		return;

	// Get the studio header for the player.
	CStudioHdr *pStudioHdr = pTFPlayer->GetModelPtr();
	if ( !pStudioHdr )
		return;

	if ( pTFPlayer->GetPlayerClass()->HasCustomModel() )
	{
		if ( !pTFPlayer->GetPlayerClass()->CustomModelUsesClassAnimations() )
		{
			if ( pTFPlayer->GetPlayerClass()->CustomModelRotates() )
			{
				if ( pTFPlayer->GetPlayerClass()->CustomModelRotationSet() )
				{
					QAngle angRot = pTFPlayer->GetPlayerClass()->GetCustomModelRotation();
					m_angRender = angRot;
				}
				else
				{
					m_angRender = vec3_angle;
					m_angRender[YAW] = AngleNormalize( eyeYaw );
				}
			}

			// Restart our animation whenever we change models
			if ( pTFPlayer->GetPlayerClass()->CustomModelHasChanged() )
			{
				RestartMainSequence();
			}

			ClearAnimationState();
			return;
		}
	}

	// Check to see if we should be updating the animation state - dead, ragdolled?
	if ( !ShouldUpdateAnimState() )
	{
		ClearAnimationState();
		return;
	}

	// Store the eye angles.
	m_flEyeYaw = AngleNormalize( eyeYaw );
	m_flEyePitch = AngleNormalize( eyePitch );

	// Compute the player sequences.
	ComputeSequences( pStudioHdr );

	Vector vPositionToFace = ( vec3_origin );
	bool bInTaunt = pTFPlayer->m_Shared.InCond( TF_COND_TAUNTING );
	bool bIsImmobilized = bInTaunt;

	if ( SetupPoseParameters( pStudioHdr ) )
	{
		// check if new item affect player pose params
		CTFWeaponBase *pWeapon = static_cast< CTFWeaponBase* >( m_pPlayer->GetActiveWeapon() );
		if ( m_hActiveWeapon != pWeapon )
		{
			m_hActiveWeapon = pWeapon;

			m_PlayerPoseParams.RemoveAll();
		}

		for ( int i=0; i<m_PlayerPoseParams.Count(); ++i )
		{
			m_pTFPlayer->SetPoseParameter( pStudioHdr, m_PlayerPoseParams[i].first, m_PlayerPoseParams[i].second );
		}

		if ( !bIsImmobilized )
		{
			// Pose parameter - what direction are the player's legs running in.
			ComputePoseParam_MoveYaw( pStudioHdr );
		}

		if ( bInTaunt )
		{
			// If you are forcing aim yaw, your code is almost definitely broken if you don't include a delay between 
			// teleporting and forcing yaw. This is due to an unfortunate interaction between the command lookback window,
			// and the fact that m_flEyeYaw is never propogated from the server to the client.
			// TODO: Fix this after Halloween 2014.
			m_bForceAimYaw = true;
			m_flEyeYaw = pTFPlayer->GetTauntYaw();

			Taunt_ComputePoseParam_MoveX( pStudioHdr );
			Taunt_ComputePoseParam_MoveY( pStudioHdr );
		}
		else if ( TFGameRules()->PlayersAreOnMatchSummaryStage() )
		{
			m_bForceAimYaw = true;
			m_flEyeYaw = pTFPlayer->GetTauntYaw();
		}
		
		if ( !bIsImmobilized || bInTaunt )
		{
			// Pose parameter - Torso aiming (up/down).
			ComputePoseParam_AimPitch( pStudioHdr );

			// Pose parameter - Torso aiming (rotation).
			ComputePoseParam_AimYaw( pStudioHdr );
		}
	}

#ifdef CLIENT_DLL 
	if ( C_BasePlayer::ShouldDrawLocalPlayer() )
	{
		GetBasePlayer()->SetPlaybackRate( 1.0f );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerAnimState::Taunt_ComputePoseParam_MoveX( CStudioHdr *pStudioHdr )
{
	CTFPlayer *pTFPlayer = GetTFPlayer();
	if ( pTFPlayer->IsTaunting() && pTFPlayer->CanMoveDuringTaunt() )
	{
		int iMove = 0;
		iMove += pTFPlayer->m_nButtons & IN_FORWARD ? 1 : 0;
		iMove += pTFPlayer->m_nButtons & IN_BACK ? -1 : 0;

		float fl_move_x = 1.f;

		// turning?
		if ( iMove != 0.f )
		{
			m_flTauntMoveX = clamp( m_flTauntMoveX + fl_move_x, -1.f, 1.f );
		}
		else if ( m_flTauntMoveX != 0.f )
		{
			// smooth the value back to 0
			if ( m_flTauntMoveX < 0.f )
			{
				m_flTauntMoveX = clamp( m_flTauntMoveX + fabs( fl_move_x ), -1.f, 0.f );
			}
			if ( m_flTauntMoveX > 0.f )
			{
				m_flTauntMoveX = clamp( m_flTauntMoveX - fabs( fl_move_x ), 0.f, 1.f );
			}
		}
	}
	else
	{
		m_flTauntMoveX = 0.f;
	}

	pTFPlayer->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveX, Sign( m_flTauntMoveX ) * SimpleSpline( fabs( m_flTauntMoveX ) ) );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerAnimState::Taunt_ComputePoseParam_MoveY( CStudioHdr *pStudioHdr )
{
	CTFPlayer *pTFPlayer = GetTFPlayer();
	if ( pTFPlayer->IsTaunting() && pTFPlayer->CanMoveDuringTaunt() )
	{
		float flTauntYawDiff = pTFPlayer->GetTauntYaw() - pTFPlayer->GetPrevTauntYaw();
		float fl_move_y = 1.f;

		// turning?
		if ( flTauntYawDiff != 0.f )
		{
			m_flTauntMoveY = clamp( m_flTauntMoveY + fl_move_y, -1.f, 1.f );
		}
		else if ( m_flTauntMoveY != 0.f )
		{
			// smooth the value back to 0
			if ( m_flTauntMoveY < 0.f )
			{
				m_flTauntMoveY = clamp( m_flTauntMoveY + fabs( fl_move_y ), -1.f, 0.f );
			}
			if ( m_flTauntMoveY > 0.f )
			{
				m_flTauntMoveY = clamp( m_flTauntMoveY - fabs( fl_move_y ), 0.f, 1.f );
			}
		}
	}
	else
	{
		m_flTauntMoveY = 0.f;
	}
	pTFPlayer->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveY, Sign( m_flTauntMoveY ) * SimpleSpline( fabs( m_flTauntMoveY ) ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerAnimState::RestartGesture( int iGestureSlot, Activity iGestureActivity, bool bAutoKill )
{
	Activity translatedActivity = TranslateActivity( iGestureActivity );
	
	BaseClass::RestartGesture( iGestureSlot, translatedActivity, bAutoKill );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : event - 
//-----------------------------------------------------------------------------
void CTFPlayerAnimState::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	bool bInDuck = ( m_pTFPlayer->GetFlags() & FL_DUCKING ) ? true : false;
	if ( bInDuck && SelectWeightedSequence( TranslateActivity( ACT_MP_CROUCHWALK ) ) < 0 )
	{
		bInDuck = false;
	}

	Activity iGestureActivity = ACT_INVALID;

	switch( event )
	{
	case PLAYERANIMEVENT_ATTACK_PRIMARY:
		{
			CTFPlayer *pPlayer = GetTFPlayer();
			if ( !pPlayer )
				return;

			CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();
			bool bIsMinigun = ( pWpn && pWpn->GetWeaponID() == TF_WEAPON_MINIGUN );
			bool bIsSniperRifle = ( pWpn && WeaponID_IsSniperRifle( pWpn->GetWeaponID() ) );

			// Heavy weapons primary fire.
			if ( bIsMinigun )
			{
				// Play standing primary fire.
				iGestureActivity = ACT_MP_ATTACK_STAND_PRIMARYFIRE;

				if ( m_bInSwim )
				{
					// Play swimming primary fire.
					iGestureActivity = ACT_MP_ATTACK_SWIM_PRIMARYFIRE;
				}
				else if ( bInDuck )
				{
					// Play crouching primary fire.
					iGestureActivity = ACT_MP_ATTACK_CROUCH_PRIMARYFIRE;
				}

				if ( !IsGestureSlotPlaying( GESTURE_SLOT_ATTACK_AND_RELOAD, TranslateActivity(iGestureActivity) ) )
				{
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, iGestureActivity );
				}
			}
			else if ( bIsSniperRifle && pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
			{
				// Weapon primary fire, zoomed in
				if ( bInDuck )
				{
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_CROUCH_PRIMARYFIRE_DEPLOYED );
				}
				else
				{
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_PRIMARYFIRE_DEPLOYED );
				}

				iGestureActivity = ACT_VM_PRIMARYATTACK;

				// Hold our deployed pose for a few seconds
				m_flHoldDeployedPoseUntilTime = gpGlobals->curtime + 2.0;
			}
			else
			{
				Activity baseActivity = bInDuck ? ACT_MP_ATTACK_CROUCH_PRIMARYFIRE : ACT_MP_ATTACK_STAND_PRIMARYFIRE;

				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, baseActivity );
//				iGestureActivity = ACT_VM_PRIMARYATTACK;
			}

			break;
		}

	case PLAYERANIMEVENT_ATTACK_PRIMARY_SUPER:
		{
			Activity baseActivity = bInDuck ? ACT_MP_ATTACK_CROUCH_PRIMARY_SUPER : ACT_MP_ATTACK_STAND_PRIMARY_SUPER;

			if ( m_bInSwim )
				baseActivity = ACT_MP_ATTACK_SWIM_PRIMARY_SUPER;

			RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, baseActivity );
//			iGestureActivity = ACT_VM_PRIMARYATTACK;
		}
		break;

	case PLAYERANIMEVENT_VOICE_COMMAND_GESTURE:
		{
			if ( !IsGestureSlotActive( GESTURE_SLOT_ATTACK_AND_RELOAD ) )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, (Activity)nData );
			}
			break;
		}
	case PLAYERANIMEVENT_ATTACK_SECONDARY:
		{
			Activity baseActivity = bInDuck ? ACT_MP_ATTACK_CROUCH_SECONDARYFIRE : ACT_MP_ATTACK_STAND_SECONDARYFIRE;
			if ( GetBasePlayer()->GetWaterLevel() >= WL_Waist )
			{
				baseActivity = ACT_MP_ATTACK_SWIM_SECONDARYFIRE;
			}
			
			RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, baseActivity );
			iGestureActivity = ACT_VM_SECONDARYATTACK;
			break;
		}
	case PLAYERANIMEVENT_ATTACK_PRE:
		{
			CTFPlayer *pPlayer = GetTFPlayer();
			if ( !pPlayer )
				return;

			CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();
			bool bIsMinigun  = ( pWpn && pWpn->GetWeaponID() == TF_WEAPON_MINIGUN );

			bool bAutoKillPreFire = false;
			if ( bIsMinigun )
			{
				bAutoKillPreFire = true;
			}

			if ( m_bInSwim && bIsMinigun )
			{
				// Weapon pre-fire. Used for minigun windup while swimming
				iGestureActivity = ACT_MP_ATTACK_SWIM_PREFIRE;
			}
			else if ( bInDuck ) 
			{
				// Weapon pre-fire. Used for minigun windup, sniper aiming start, etc in crouch.
				iGestureActivity = ACT_MP_ATTACK_CROUCH_PREFIRE;
			}
			else
			{
				// Weapon pre-fire. Used for minigun windup, sniper aiming start, etc.
				iGestureActivity = ACT_MP_ATTACK_STAND_PREFIRE;
			}

			RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, iGestureActivity, bAutoKillPreFire );

			break;
		}
	case PLAYERANIMEVENT_ATTACK_POST:
		{
			CTFPlayer *pPlayer = GetTFPlayer();
			if ( !pPlayer )
				return;

			CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();
			bool bIsMinigun  = ( pWpn && pWpn->GetWeaponID() == TF_WEAPON_MINIGUN );

			if ( m_bInSwim && bIsMinigun )
			{
				// Weapon pre-fire. Used for minigun winddown while swimming
				iGestureActivity = ACT_MP_ATTACK_SWIM_POSTFIRE;
			}
			else if ( bInDuck ) 
			{
				// Weapon post-fire. Used for minigun winddown in crouch.
				iGestureActivity = ACT_MP_ATTACK_CROUCH_POSTFIRE;
			}
			else
			{
				// Weapon post-fire. Used for minigun winddown.
				iGestureActivity = ACT_MP_ATTACK_STAND_POSTFIRE;
			}

			RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, iGestureActivity );

			break;
		}

	case PLAYERANIMEVENT_RELOAD:
		{
			// Weapon reload.
			if ( m_bInAirWalk )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_AIRWALK );
			}
			else
			{
				BaseClass::DoAnimationEvent( event, nData );
			}
			break;
		}
	case PLAYERANIMEVENT_RELOAD_LOOP:
		{
			// Weapon reload.
			if ( m_bInAirWalk )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_AIRWALK_LOOP );
			}
			else
			{
				BaseClass::DoAnimationEvent( event, nData );
			}
			break;
		}
	case PLAYERANIMEVENT_RELOAD_END:
		{
			// Weapon reload.
			if ( m_bInAirWalk )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_AIRWALK_END );
			}
			else
			{
				BaseClass::DoAnimationEvent( event, nData );
			}
			break;
		}
	case PLAYERANIMEVENT_DOUBLEJUMP:
		{
			CTFPlayer *pPlayer = GetTFPlayer();
			if ( !pPlayer )
				return;

			// Check to see if we are jumping!
			if ( !m_bJumping )
			{
				m_bJumping = true;
				m_bFirstJumpFrame = true;
				m_flJumpStartTime = gpGlobals->curtime;
				RestartMainSequence();
			}

			// Force the air walk off.
			m_bInAirWalk = false;

			// Player the air dash gesture.
			RestartGesture( GESTURE_SLOT_JUMP, ACT_MP_DOUBLEJUMP );
			break;
		}
	case PLAYERANIMEVENT_DOUBLEJUMP_CROUCH:
//		RestartGesture( GESTURE_SLOT_JUMP, ACT_MP_DOUBLEJUMP_CROUCH );
//		m_aGestureSlots[GESTURE_SLOT_JUMP].m_pAnimLayer->m_flBlendIn = 0.4f;
//		m_aGestureSlots[GESTURE_SLOT_JUMP].m_pAnimLayer->m_flBlendOut = 0.4f;
#ifdef CLIENT_DLL
//		m_aGestureSlots[GESTURE_SLOT_JUMP].m_pAnimLayer->m_bClientBlend = true;
#endif
		break;
	case PLAYERANIMEVENT_STUN_BEGIN:
		RestartGesture( GESTURE_SLOT_CUSTOM, ACT_MP_STUN_BEGIN, false );
		break;
	case PLAYERANIMEVENT_STUN_MIDDLE:
		RestartGesture( GESTURE_SLOT_CUSTOM, ACT_MP_STUN_MIDDLE, false );
		break;
	case PLAYERANIMEVENT_STUN_END:
		RestartGesture( GESTURE_SLOT_CUSTOM, ACT_MP_STUN_END );
		break;
	case PLAYERANIMEVENT_PASSTIME_THROW_BEGIN:
		RestartGesture( GESTURE_SLOT_CUSTOM, ACT_MP_PASSTIME_THROW_BEGIN, false );
		break;
	case PLAYERANIMEVENT_PASSTIME_THROW_MIDDLE:
		RestartGesture( GESTURE_SLOT_CUSTOM, ACT_MP_PASSTIME_THROW_MIDDLE, false );
		break;
	case PLAYERANIMEVENT_PASSTIME_THROW_END:
		RestartGesture( GESTURE_SLOT_CUSTOM, ACT_MP_PASSTIME_THROW_END );
		break;
	case PLAYERANIMEVENT_CYOAPDA_BEGIN:
		RestartGesture( GESTURE_SLOT_CUSTOM, ACT_MP_CYOA_PDA_INTRO, false );
		break;
	case PLAYERANIMEVENT_CYOAPDA_MIDDLE:
		RestartGesture( GESTURE_SLOT_CUSTOM, ACT_MP_CYOA_PDA_IDLE, false );
		break;
	case PLAYERANIMEVENT_CYOAPDA_END:
		RestartGesture( GESTURE_SLOT_CUSTOM, ACT_MP_CYOA_PDA_OUTRO );
		break;
	default:
		{
			BaseClass::DoAnimationEvent( event, nData );
			break;
		}
	}

#ifdef CLIENT_DLL
	// Make the weapon play the animation as well
	if ( iGestureActivity != ACT_INVALID )
	{
		CBaseCombatWeapon *pWeapon = GetTFPlayer()->GetActiveWeapon();
		if ( pWeapon )
		{
			pWeapon->SendWeaponAnim( iGestureActivity );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
//-----------------------------------------------------------------------------
bool CTFPlayerAnimState::HandleSwimming( Activity &idealActivity )
{
	bool bInWater = BaseClass::HandleSwimming( idealActivity );

	if ( bInWater )
	{
		if ( m_pTFPlayer->m_Shared.IsAiming() )
		{
			CTFWeaponBase *pWpn = m_pTFPlayer->GetActiveTFWeapon();
			if ( pWpn && pWpn->GetWeaponID() == TF_WEAPON_MINIGUN )
			{
				idealActivity = ACT_MP_SWIM_DEPLOYED;
			}
			// Check for sniper deployed underwater - should only be when standing on something
			else if ( pWpn && WeaponID_IsSniperRifle( pWpn->GetWeaponID() ) )
			{
				if ( m_pTFPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
				{
					idealActivity = ACT_MP_SWIM_DEPLOYED;
				}
			}
		}
	}

	return bInWater;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFPlayerAnimState::HandleMoving( Activity &idealActivity )
{
	float flSpeed = GetOuterXYSpeed();

	// If we move, cancel the deployed anim hold
	if ( flSpeed > MOVING_MINIMUM_SPEED )
	{
		m_flHoldDeployedPoseUntilTime = 0.0;
	}

	if ( m_pTFPlayer->m_Shared.IsAiming() ) 
	{
		if ( flSpeed > MOVING_MINIMUM_SPEED )
		{
			idealActivity = ACT_MP_DEPLOYED;
		}
		else
		{
			idealActivity = ACT_MP_DEPLOYED_IDLE;
		}
	}
	else if ( m_flHoldDeployedPoseUntilTime > gpGlobals->curtime )
	{
		// Unless we move, hold the deployed pose for a number of seconds after being deployed
		idealActivity = ACT_MP_DEPLOYED_IDLE;
	}
	else 
	{
		return BaseClass::HandleMoving( idealActivity );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFPlayerAnimState::HandleDucking( Activity &idealActivity )
{
	bool bInDuck = ( m_pTFPlayer->GetFlags() & FL_DUCKING ) ? true : false;
	if ( bInDuck && SelectWeightedSequence( TranslateActivity( ACT_MP_CROUCHWALK ) ) < 0 )
	{
		bInDuck = false;
	}

	if ( bInDuck )
	{
		if ( GetOuterXYSpeed() < MOVING_MINIMUM_SPEED )
		{
			idealActivity = ACT_MP_CROUCH_IDLE;		
			if ( m_pTFPlayer->m_Shared.IsAiming() || m_flHoldDeployedPoseUntilTime > gpGlobals->curtime )
			{
				idealActivity = ACT_MP_CROUCH_DEPLOYED_IDLE;
			}
		}
		else
		{
			if ( m_pTFPlayer->m_Shared.GetAirDash() > 0 )
			{
				idealActivity = ACT_MP_DOUBLEJUMP_CROUCH;
			}
			else
			{
				idealActivity = ACT_MP_CROUCHWALK;		
			}

			if ( m_pTFPlayer->m_Shared.IsAiming() )
			{
				// Don't do this for the heavy! we don't usually let him deployed crouch walk
				bool bIsMinigun = false;

				CTFPlayer *pPlayer = GetTFPlayer();
				if ( pPlayer && pPlayer->GetActiveTFWeapon() )
				{
					bIsMinigun = ( pPlayer->GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_MINIGUN );
				}

				if ( !bIsMinigun )
				{
					idealActivity = ACT_MP_CROUCH_DEPLOYED;
				}
			}
		}

		return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayerAnimState::GetCurrentMaxGroundSpeed()
{
	float flSpeed = BaseClass::GetCurrentMaxGroundSpeed();

	if ( m_pTFPlayer->m_Shared.GetAirDash() > 0 )
	{
		return 1.f;
	}
	else
	{
		return flSpeed;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerAnimState::HandleJumping( Activity &idealActivity )
{
	bool bInDuck = ( m_pTFPlayer->GetFlags() & FL_DUCKING ) ? true : false;
	if ( bInDuck && SelectWeightedSequence( TranslateActivity( ACT_MP_CROUCHWALK ) ) < 0 )
	{
		bInDuck = false;
	}

	Vector vecVelocity;
	GetOuterAbsVelocity( vecVelocity );

	// Don't allow a firing heavy to jump or air walk.
	if ( m_pTFPlayer->GetPlayerClass()->IsClass( TF_CLASS_HEAVYWEAPONS ) && m_pTFPlayer->m_Shared.InCond( TF_COND_AIMING ) )
		return false;
		
	// Handle air walking before handling jumping - air walking supersedes jump
	TFPlayerClassData_t *pData = m_pTFPlayer->GetPlayerClass()->GetData();
	bool bValidAirWalkClass = ( pData && pData->m_bDontDoAirwalk == false );

	if ( bValidAirWalkClass && ( vecVelocity.z > 300.0f || m_bInAirWalk ) && !bInDuck )
	{
		// Check to see if we were in an airwalk and now we are basically on the ground.
		if ( ( GetBasePlayer()->GetFlags() & FL_ONGROUND ) && m_bInAirWalk )
		{				
			m_bInAirWalk = false;
			RestartMainSequence();
			RestartGesture( GESTURE_SLOT_JUMP, ACT_MP_JUMP_LAND );	
		}
		else if ( GetBasePlayer()->GetWaterLevel() >= WL_Waist )
		{
			// Turn off air walking and reset the animation.
			m_bInAirWalk = false;
			RestartMainSequence();
		}
		else if ( ( GetBasePlayer()->GetFlags() & FL_ONGROUND ) == 0 )
		{
			// In an air walk.
			idealActivity = ACT_MP_AIRWALK;
			m_bInAirWalk = true;
		}
	}
	// Jumping.
	else
	{
		if ( m_bJumping )
		{
			// Remove me once all classes are doing the new jump
			TFPlayerClassData_t *pDataJump = m_pTFPlayer->GetPlayerClass()->GetData();
			bool bNewJump = (pDataJump && pDataJump->m_bDontDoNewJump == false );

			if ( m_bFirstJumpFrame )
			{
				m_bFirstJumpFrame = false;
				RestartMainSequence();	// Reset the animation.
			}

			// Reset if we hit water and start swimming.
			if ( GetBasePlayer()->GetWaterLevel() >= WL_Waist )
			{
				m_bJumping = false;
				RestartMainSequence();
			}
			// Don't check if he's on the ground for a sec.. sometimes the client still has the
			// on-ground flag set right when the message comes in.
			else if ( gpGlobals->curtime - m_flJumpStartTime > 0.2f )
			{
				if ( GetBasePlayer()->GetFlags() & FL_ONGROUND )
				{
					m_bJumping = false;
					RestartMainSequence();

					if ( bNewJump )
					{
						RestartGesture( GESTURE_SLOT_JUMP, ACT_MP_JUMP_LAND );					
					}
				}
			}

			// if we're still jumping
			if ( m_bJumping )
			{
				if ( bNewJump )
				{
					if ( gpGlobals->curtime - m_flJumpStartTime > 0.5 )
					{
						idealActivity = ACT_MP_JUMP_FLOAT;
					}
					else
					{
						idealActivity = ACT_MP_JUMP_START;
					}
				}
				else
				{
					idealActivity = ACT_MP_JUMP;
				}
			}
		}	
	}

	if ( m_bJumping || m_bInAirWalk )
		return true;

	return false;
}
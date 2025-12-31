//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "c_tf_player.h"
#include "c_user_message_register.h"
#include "view.h"
#include "iclientvehicle.h"
#include "ivieweffects.h"
#include "input.h"
#include "IEffects.h"
#include "fx.h"
#include "c_basetempentity.h"
#include "hud_macros.h"
#include "engine/ivdebugoverlay.h"
#include "smoke_fog_overlay.h"
#include "playerandobjectenumerator.h"
#include "bone_setup.h"
#include "in_buttons.h"
#include "r_efx.h"
#include "dlight.h"
#include "shake.h"
#include "cl_animevent.h"
#include "animation.h"
#include "choreoscene.h"
#include "tf_weaponbase.h"
#include "c_tf_playerresource.h"
#include "toolframework/itoolframework.h"
#include "tier1/KeyValues.h"
#include "tier0/vprof.h"
#include "prediction.h"
#include "effect_dispatch_data.h"
#include "c_te_effect_dispatch.h"
#include "tf_fx_muzzleflash.h"
#include "tf_gamerules.h"
#include "view_scene.h"
#include "c_baseobject.h"
#include "toolframework_client.h"
#include "materialsystem/imaterialvar.h"
#include "soundenvelope.h"
#include "voice_status.h"
#include "clienteffectprecachesystem.h"
#include "functionproxy.h"
#include "toolframework_client.h"
#include "choreoevent.h"
#include "vguicenterprint.h"
#include "eventlist.h"
#include "input.h"
#include "tf_weapon_medigun.h"
#include "tf_weapon_pipebomblauncher.h"
#include "tf_weapon_shovel.h"
#include "tf_hud_mediccallers.h"
#include "in_main.h"
#include "c_team.h"
#include "collisionutils.h"
// for spy material proxy
#include "tf_proxyentity.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/itexturecompositor.h"
#include "c_tf_team.h"
#include "model_types.h"
#include "dt_utlvector_recv.h"
#include "tf_item_wearable.h"
#include "cam_thirdperson.h"
#include "replay/vgui/replayinputpanel.h"
#include "tf_replay.h"
#include "netadr.h"
#include "input.h"

#include "networkstringtable_clientdll.h"
#include "replay/ireplaymanager.h"
#include "c_entitydissolve.h"
#include "tf_viewmodel.h"
#include "sourcevr/isourcevirtualreality.h"
#include "tempent.h"
#include "c_tf_weapon_builder.h"
#include "baseanimatedtextureproxy.h"
#include "tf_hud_notification_panel.h"
#include "eiface.h"
#include "filesystem.h"
#include "debugoverlay_shared.h"
#include "tf_hud_chat.h"
#include <vgui_controls/AnimationController.h>
#include "soundstartparams.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"

#include "steam/steamclientpublic.h"
#include "steam/isteamfriends.h"

#if defined( REPLAY_ENABLED )
#include "replay/ienginereplay.h"
#endif

#if defined( CTFPlayer )
#undef CTFPlayer
#endif

#include "materialsystem/imesh.h"		//for materials->FindMaterial
#include "iviewrender.h"				//for view->

// NVNT haptics system interface
#include "c_tf_haptics.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static_assert( TEAM_UNASSIGNED == 0, "If this assert fires, update the assert and the enum in ctexturecompositor.cpp which specifies team colors" );
static_assert( TF_TEAM_RED == 2, "If this assert fires, update the assert and the enum in ctexturecompositor.cpp which specifies team colors" );
static_assert( TF_TEAM_BLUE == 3, "If this assert fires, update the assert and the enum in ctexturecompositor.cpp which specifies team colors" );

// Forward decl
C_TFPlayer *GetOwnerFromProxyEntity( void *pEntity );

// These are all permanently STAGING_ONLY

ConVar tf_playergib_forceup( "tf_playersgib_forceup", "1.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Upward added velocity for gibs." );
ConVar tf_playergib_force( "tf_playersgib_force", "500.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Gibs force." );
ConVar tf_playergib_maxspeed( "tf_playergib_maxspeed", "400", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Max gib speed." );

ConVar tf_always_deathanim( "tf_always_deathanim", "0", FCVAR_CHEAT, "Force death anims to always play." );

ConVar tf_clientsideeye_lookats( "tf_clientsideeye_lookats", "1", FCVAR_NONE, "When on, players will turn their pupils to look at nearby players." );

extern ConVar tf_halloween_kart_boost_recharge;
extern ConVar tf_halloween_kart_boost_duration;

extern ConVar cl_thirdperson;


ConVar tf_sheen_framerate( "tf_sheen_framerate", "25", FCVAR_NONE | FCVAR_HIDDEN, "Set Sheen Frame Rate" );

extern ConVar tf_killstreak_alwayson;

ConVar tf_sheen_alpha_firstperson( "tf_sheen_alpha_firstperson", "0.1", FCVAR_NONE, "Set the Alpha Value for first person sheens" );
ConVar tf_killstreakeyes_minkills( "tf_killstreakeyes_minkills", "5", FCVAR_DEVELOPMENTONLY, "min kills to get base eyeglow" );
ConVar tf_killstreakeyes_maxkills( "tf_killstreakeyes_maxkills", "10", FCVAR_DEVELOPMENTONLY, "kills to get the max eye glow effect" );

//ConVar spectate_random_server_basetime( "spectate_random_server_basetime", "240", FCVAR_DEVELOPMENTONLY );

ConVar cl_autorezoom( "cl_autorezoom", "1", FCVAR_USERINFO | FCVAR_ARCHIVE, "When set to 1, sniper rifle will re-zoom after firing a zoomed shot." );
ConVar tf_remember_activeweapon( "tf_remember_activeweapon", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_USERINFO, "Setting this to 1 will make the active weapon persist between lives." );
ConVar tf_remember_lastswitched( "tf_remember_lastswitched", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_USERINFO, "Setting this to 1 will make the 'last weapon' persist between lives." );

ConVar tf_respawn_on_loadoutchanges( "tf_respawn_on_loadoutchanges", "1", FCVAR_ARCHIVE, "When set to 1, you will automatically respawn whenever you change loadouts inside a respawn zone." );

ConVar sb_dontshow_maxplayer_warning( "sb_dontshow_maxplayer_warning", "0", FCVAR_ARCHIVE );
ConVar sb_close_browser_on_connect( "sb_close_browser_on_connect", "1", FCVAR_ARCHIVE );

ConVar tf_taunt_first_person( "tf_taunt_first_person", "0", FCVAR_NONE, "1 = taunts remain first-person" );


#define BDAY_HAT_MODEL		"models/effects/bday_hat.mdl"

IMaterial	*g_pHeadLabelMaterial[2] = { NULL, NULL }; 
void	SetupHeadLabelMaterials( void );

extern CBaseEntity *BreakModelCreateSingle( CBaseEntity *pOwner, breakmodel_t *pModel, const Vector &position, 
										   const QAngle &angles, const Vector &velocity, const AngularImpulse &angVelocity, int nSkin, const breakablepropparams_t &params );

extern int EconWear_ToIntCategory( float flWear );

const char *g_pszHeadGibs[] =
{
	"",
	"models/player\\gibs\\scoutgib007.mdl",
	"models/player\\gibs\\snipergib005.mdl",
	"models/player\\gibs\\soldiergib007.mdl",
	"models/player\\gibs\\demogib006.mdl",
	"models/player\\gibs\\medicgib007.mdl",
	"models/player\\gibs\\heavygib007.mdl",
	"models/player\\gibs\\pyrogib008.mdl",
	"models/player\\gibs\\spygib007.mdl",
	"models/player\\gibs\\engineergib006.mdl",
};

const char *g_pszBotHeadGibs[] =
{
	"",
	"models/bots\\gibs\\scoutbot_gib_head.mdl",
	"models/bots\\gibs\\sniperbot_gib_head.mdl",
	"models/bots\\gibs\\soldierbot_gib_head.mdl",
	"models/bots\\gibs\\demobot_gib_head.mdl",
	"models/bots\\gibs\\medicbot_gib_head.mdl",
	"models/bots\\gibs\\heavybot_gib_head.mdl",
	"models/bots\\gibs\\pyrobot_gib_head.mdl",
	"models/bots\\gibs\\spybot_gib_head.mdl",
	"models/bots\\gibs\\engineerbot_gib_head.mdl",
};

const char *pszHeadLabelNames[] =
{
	"effects/speech_voice_red",
	"effects/speech_voice_blue"
};

BonusEffect_t g_BonusEffects[ kBonusEffect_Count ] = 
{
	// Sound name							Particle name			Attach type				Attach point	Particle filter				Sound filter					Sound plays in attacker's ears for them, the world for everyone else, Large combat text
	{ "TFPlayer.CritHit",					"crit_text",			PATTACH_POINT_FOLLOW,	"head",			kEffectFilter_AttackerOnly, kEffectFilter_AttackerOnly,		true, true },
};

extern SkyBoxMaterials_t s_PyroSkyboxMaterials;

#define TF_PLAYER_HEAD_LABEL_RED 0
#define TF_PLAYER_HEAD_LABEL_BLUE 1

CLIENTEFFECT_REGISTER_BEGIN( PrecacheInvuln )
CLIENTEFFECT_MATERIAL( "models/effects/invulnfx_blue.vmt" )
CLIENTEFFECT_MATERIAL( "models/effects/invulnfx_red.vmt" )
CLIENTEFFECT_REGISTER_END()

// *********************************************************************************************************
// KillStreak Effect Data
// *********************************************************************************************************
struct killstreak_params_t
{
	killstreak_params_t ( int iShaderIndex, const char *pTexture, bool bHasTeamColor,
		int sheen_r,  int sheen_g,  int sheen_b,  int sheen_a, 
		int color1_r, int color1_g, int color1_b, int color1_a, 
		int color2_r, int color2_g, int color2_b, int color2_a
		) {
			m_iShaderIndex = iShaderIndex;
			m_pTexture = pTexture;

			m_bHasTeamColor = bHasTeamColor;

			m_sheen_r = (float)sheen_r / 255.0f;
			m_sheen_g = (float)sheen_g / 255.0f;
			m_sheen_b = (float)sheen_b / 255.0f; 
			m_sheen_a = (float)sheen_a / 255.0f;

			m_color1_r = (float)color1_r / 255.0f;
			m_color1_g = (float)color1_g / 255.0f;
			m_color1_b = (float)color1_b / 255.0f; 
			m_color1_a = (float)color1_a / 255.0f;

			m_color2_r = (float)color2_r / 255.0f;
			m_color2_g = (float)color2_g / 255.0f;
			m_color2_b = (float)color2_b / 255.0f; 
			m_color2_a = (float)color2_a / 255.0f;
	}

	int			m_iShaderIndex;
	bool		m_bHasTeamColor;
	const char *m_pTexture;
	float m_sheen_r, m_sheen_g, m_sheen_b, m_sheen_a;
	float m_color1_r, m_color1_g, m_color1_b, m_color1_a;
	float m_color2_r, m_color2_g, m_color2_b, m_color2_a;
};

// *********************************************************************************************************
// Base Sheen Colors (Team Red)
static const killstreak_params_t g_KillStreakEffectsBase[] =
{
	killstreak_params_t( 0, NULL, false,	0, 0, 0, 0, 0, 0, 0, 0,0, 0, 0, 0 ),	// Empty (Index 0)
	killstreak_params_t( 0, "Effects/AnimatedSheen/animatedsheen0", true,	200, 20,  15,  255, 255, 118, 118, 255, 255, 35,  28,  255 ),
	killstreak_params_t( 0, "Effects/AnimatedSheen/animatedsheen0", false,	242, 172, 10,  255, 255, 237, 138, 255, 255, 213, 65,  255 ),
	killstreak_params_t( 0, "Effects/AnimatedSheen/animatedsheen0", false,	255, 75,  5,   255, 255, 111, 5,   255, 255, 137, 31,  255 ),
	killstreak_params_t( 0, "Effects/AnimatedSheen/animatedsheen0", false,	100, 255, 10,  255, 230, 255, 60,  255, 193, 255, 61,  255 ),
	killstreak_params_t( 0, "Effects/AnimatedSheen/animatedsheen0", false,	40,  255, 70,  255, 103, 255, 121, 255, 165, 255, 193, 255 ),
	killstreak_params_t( 0, "Effects/AnimatedSheen/animatedsheen0", false,	105, 20,  255, 255, 105, 20,  255, 255, 185, 145, 255, 255 ),
	killstreak_params_t( 0, "Effects/AnimatedSheen/animatedsheen0", false,	255, 30,  255, 255, 255, 120, 255, 255, 255, 176, 217, 255 ),
};
// *********************************************************************************************************
// Optional Team Color
static const killstreak_params_t g_KillStreakEffectsBlue[] =
{
	killstreak_params_t( 0, NULL, false,	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ),	// Empty (Index 0)
	killstreak_params_t( 0, "Effects/AnimatedSheen/animatedsheen0", true, 40, 98, 200, 255, 0, 92, 255, 255, 134, 203, 243, 255 ),
};

// thirdperson medieval
static ConVar tf_medieval_thirdperson( "tf_medieval_thirdperson", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE , "Turns on third-person camera in medieval mode." );
static ConVar tf_medieval_cam_idealdist( "tf_medieval_cam_idealdist", "125", FCVAR_CLIENTDLL | FCVAR_CHEAT );	 // thirdperson distance
static ConVar tf_medieval_cam_idealdistright( "tf_medieval_cam_idealdistright", "25", FCVAR_CLIENTDLL | FCVAR_CHEAT );	 // thirdperson distance
static ConVar tf_medieval_cam_idealdistup( "tf_medieval_cam_idealdistup", "-10", FCVAR_CLIENTDLL | FCVAR_CHEAT );	 // thirdperson distance
static ConVar tf_medieval_cam_idealpitch( "tf_medieval_cam_idealpitch", "0", FCVAR_CLIENTDLL | FCVAR_CHEAT );	 // thirdperson pitch
extern ConVar cam_idealpitch;
extern ConVar tf_allow_taunt_switch;

static void PromptAcceptReviveCallback( bool bCancel, void *pContext )
{
	if ( bCancel )
	{
		KeyValues *kv = new KeyValues( "MVM_Revive_Response" );
		kv->SetBool( "accepted", false );
		engine->ServerCmdKeyValues( kv );
	}
}

C_EntityDissolve *DissolveEffect( C_BaseEntity *pTarget, float flTime );

void SetAppropriateCamera( C_TFPlayer *pPlayer )
{
	if ( pPlayer->IsLocalPlayer() == false )
		return;

	g_ThirdPersonManager.SetForcedThirdPerson( false );
}

// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //

class C_TEPlayerAnimEvent : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEPlayerAnimEvent, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	virtual void PostDataUpdate( DataUpdateType_t updateType )
	{
		VPROF( "C_TEPlayerAnimEvent::PostDataUpdate" );

		// Create the effect.
		if ( !m_hPlayer )
			return;

		C_TFPlayer *pPlayer = dynamic_cast< C_TFPlayer* >( m_hPlayer.Get() );
		if ( pPlayer && !pPlayer->IsDormant() )
		{
			// Ignore anim events that are also played on the client.
			PlayerAnimEvent_t animEvent = (PlayerAnimEvent_t) m_iEvent.Get();
			switch ( animEvent )
			{
			case PLAYERANIMEVENT_STUN_BEGIN:
			case PLAYERANIMEVENT_STUN_MIDDLE:
			case PLAYERANIMEVENT_STUN_END:
			case PLAYERANIMEVENT_PASSTIME_THROW_BEGIN:
			case PLAYERANIMEVENT_PASSTIME_THROW_MIDDLE:
			case PLAYERANIMEVENT_PASSTIME_THROW_END:
			case PLAYERANIMEVENT_CYOAPDA_BEGIN:
			case PLAYERANIMEVENT_CYOAPDA_MIDDLE:
			case PLAYERANIMEVENT_CYOAPDA_END:
				break; // ignore these
			default:
				pPlayer->DoAnimationEvent( animEvent, m_nData );
				break;
			};
		}	
	}

public:
	CNetworkHandle( CBasePlayer, m_hPlayer );
	CNetworkVar( int, m_iEvent );
	CNetworkVar( int, m_nData );
};

IMPLEMENT_CLIENTCLASS_EVENT( C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent, CTEPlayerAnimEvent );

//-----------------------------------------------------------------------------
// Data tables and prediction tables.
//-----------------------------------------------------------------------------
BEGIN_RECV_TABLE_NOBASE( C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent )
	RecvPropEHandle( RECVINFO( m_hPlayer ) ),
	RecvPropInt( RECVINFO( m_iEvent ) ),
	RecvPropInt( RECVINFO( m_nData ) )
END_RECV_TABLE()


//=============================================================================
//
// Ragdoll
//
// ----------------------------------------------------------------------------- //
// Client ragdoll entity.
// ----------------------------------------------------------------------------- //
ConVar cl_ragdoll_physics_enable( "cl_ragdoll_physics_enable", "1", 0, "Enable/disable ragdoll physics." );
ConVar cl_ragdoll_fade_time( "cl_ragdoll_fade_time", "15", FCVAR_CLIENTDLL );
ConVar cl_ragdoll_forcefade( "cl_ragdoll_forcefade", "0", FCVAR_CLIENTDLL );
ConVar cl_ragdoll_pronecheck_distance( "cl_ragdoll_pronecheck_distance", "64", FCVAR_GAMEDLL );

IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_TFRagdoll, DT_TFRagdoll, CTFRagdoll )
	RecvPropVector( RECVINFO(m_vecRagdollOrigin) ),
	RecvPropEHandle( RECVINFO( m_hPlayer ) ),
	RecvPropVector( RECVINFO(m_vecForce) ),
	RecvPropVector( RECVINFO(m_vecRagdollVelocity) ),
	RecvPropInt( RECVINFO( m_nForceBone ) ),
	RecvPropBool( RECVINFO( m_bGib ) ),
	RecvPropBool( RECVINFO( m_bBurning ) ),
	RecvPropBool( RECVINFO( m_bWasDisguised ) ),
	RecvPropBool( RECVINFO( m_bOnGround ) ),
	RecvPropInt( RECVINFO( m_iDamageCustom ) ),
	RecvPropInt( RECVINFO( m_iTeam ) ),
	RecvPropInt( RECVINFO( m_iClass ) ),		
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
C_TFRagdoll::C_TFRagdoll()
{
	m_fDeathTime = -1;
	m_bFadingOut = false;
	m_bGib = false;
	m_bBurning = false;
	m_bWasDisguised = false;
	m_flBurnEffectStartTime = 0.0f;
	m_iDamageCustom = 0;
	m_freezeTimer.Invalidate();
	m_frozenTimer.Invalidate();
	m_iTeam = -1;
	m_iClass = -1;
	m_nForceBone = -1;
	m_bRagdollOn = false;
	m_bDeathAnim = false;
	m_bOnGround = false;
	m_bBaseTransform = false;
	m_bFixedConstraints = false;

	UseClientSideAnimation();

	m_bCreatedWhilePlaybackSkipping = engine->IsSkippingPlayback();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
C_TFRagdoll::~C_TFRagdoll()
{
	PhysCleanupFrictionSounds( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSourceEntity - 
//-----------------------------------------------------------------------------
void C_TFRagdoll::Interp_Copy( C_BaseAnimatingOverlay *pSourceEntity )
{
	if ( !pSourceEntity )
		return;
	
	VarMapping_t *pSrc = pSourceEntity->GetVarMapping();
	VarMapping_t *pDest = GetVarMapping();
    	
	// Find all the VarMapEntry_t's that represent the same variable.
	for ( int i = 0; i < pDest->m_Entries.Count(); i++ )
	{
		VarMapEntry_t *pDestEntry = &pDest->m_Entries[i];
		for ( int j=0; j < pSrc->m_Entries.Count(); j++ )
		{
			VarMapEntry_t *pSrcEntry = &pSrc->m_Entries[j];
			if ( !Q_strcmp( pSrcEntry->watcher->GetDebugName(), pDestEntry->watcher->GetDebugName() ) )
			{
				pDestEntry->watcher->Copy( pSrcEntry->watcher );
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Setup vertex weights for drawing
//-----------------------------------------------------------------------------
void C_TFRagdoll::SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights )
{
	// While we're dying, we want to mimic the facial animation of the player. Once they're dead, we just stay as we are.
	C_TFPlayer *pPlayer = GetPlayer();
	if ( ( pPlayer && pPlayer->IsAlive()) || !pPlayer )
	{
		BaseClass::SetupWeights( pBoneToWorld, nFlexWeightCount, pFlexWeights, pFlexDelayedWeights );
	}
	else if ( pPlayer )
	{
		pPlayer->SetupWeights( pBoneToWorld, nFlexWeightCount, pFlexWeights, pFlexDelayedWeights );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTrace - 
//			iDamageType - 
//			*pCustomImpactName - 
//-----------------------------------------------------------------------------
void C_TFRagdoll::ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName )
{
	VPROF( "C_TFRagdoll::ImpactTrace" );
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if( !pPhysicsObject )
		return;

	Vector vecDir;
	VectorSubtract( pTrace->endpos, pTrace->startpos, vecDir );

	if ( iDamageType == DMG_BLAST )
	{
		// Adjust the impact strength and apply the force at the center of mass.
		vecDir *= 4000;
		pPhysicsObject->ApplyForceCenter( vecDir );
	}
	else
	{
		// Find the apporx. impact point.
		Vector vecHitPos;  
		VectorMA( pTrace->startpos, pTrace->fraction, vecDir, vecHitPos );
		VectorNormalize( vecDir );

		// Adjust the impact strength and apply the force at the impact point..
		vecDir *= 4000;
		pPhysicsObject->ApplyForceOffset( vecDir, vecHitPos );	
	}

	m_pRagdoll->ResetRagdollSleepAfterTime();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void C_TFRagdoll::CreateTFRagdoll()
{
	// Get the player.
	C_TFPlayer *pPlayer = GetPlayer();

	int nModelIndex = -1;

	if ( pPlayer && pPlayer->GetPlayerClass() && !pPlayer->ShouldDrawSpyAsDisguised() )
	{
		nModelIndex = modelinfo->GetModelIndex( pPlayer->GetPlayerClass()->GetModelName() );
	}
	else
	{
		TFPlayerClassData_t *pData = GetPlayerClassData( m_iClass );
		if ( pData )
		{
			nModelIndex = modelinfo->GetModelIndex( pData->GetModelName() );
		}
	}

	if ( nModelIndex != -1 )
	{
		SetModelIndex( nModelIndex );	

		if ( m_iTeam == TF_TEAM_RED )
		{
			m_nSkin = 0;
		}
		else
		{
			m_nSkin = 1;
		}
	}

#ifdef _DEBUG
	DevMsg( 2, "CreateTFRagdoll %d %d\n", gpGlobals->framecount, pPlayer ? pPlayer->entindex() : 0 );
#endif

	if ( pPlayer && !pPlayer->IsDormant() )
	{
		// Move my current model instance to the ragdoll's so decals are preserved.
		pPlayer->SnatchModelInstance( this );

		VarMapping_t *varMap = GetVarMapping();

		// Copy all the interpolated vars from the player entity.
		// The entity uses the interpolated history to get bone velocity.		
		if ( !pPlayer->IsLocalPlayer() && pPlayer->IsInterpolationEnabled() )
		{
			Interp_Copy( pPlayer );

			SetAbsAngles( pPlayer->GetRenderAngles() );
			GetRotationInterpolator().Reset();

			m_flAnimTime = pPlayer->m_flAnimTime;
			SetSequence( pPlayer->GetSequence() );
			m_flPlaybackRate = pPlayer->GetPlaybackRate();
		}
		else
		{
			// This is the local player, so set them in a default
			// pose and slam their velocity, angles and origin
			SetAbsOrigin( /* m_vecRagdollOrigin : */ pPlayer->GetRenderOrigin() );			
			SetAbsAngles( pPlayer->GetRenderAngles() );
			SetAbsVelocity( m_vecRagdollVelocity );

			// Hack! Find a neutral standing pose or use the idle.
			int iSeq = LookupSequence( "RagdollSpawn" );
			if ( iSeq == -1 )
			{
				Assert( false );
				iSeq = 0;
			}			
			SetSequence( iSeq );
			SetCycle( 0.0 );

			Interp_Reset( varMap );
		}

		pPlayer->RecalcBodygroupsIfDirty();
		m_nBody = pPlayer->GetBody();
	}
	else
	{
		// Overwrite network origin so later interpolation will use this position.
		SetNetworkOrigin( m_vecRagdollOrigin );
		SetAbsOrigin( m_vecRagdollOrigin );
		SetAbsVelocity( m_vecRagdollVelocity );

		Interp_Reset( GetVarMapping() );
	}

	// Play a death anim depending on the custom damage type.
	bool bPlayDeathInAir = false;
	int iDeathSeq = -1;
	if ( pPlayer )
	{
		iDeathSeq = pPlayer->m_Shared.GetSequenceForDeath( this, m_bBurning, m_iDamageCustom );

		// did we find a death sequence?
		if ( iDeathSeq > -1 )
		{
			// we only want to show the death anims 25% of the time, unless this is a demoman kill taunt
			// always play backstab animations for the ice ragdoll
			if ( !tf_always_deathanim.GetBool() && (RandomFloat( 0, 1 ) > 0.25f) )
			{
				iDeathSeq = -1;
			}
		}
	}

	bool bPlayDeathAnim = cl_ragdoll_physics_enable.GetBool() && (iDeathSeq > -1) && pPlayer;

	if ( !m_bOnGround && bPlayDeathAnim && !bPlayDeathInAir )
		bPlayDeathAnim = false; // Don't play most death anims in the air (headshot, etc).

	if ( bPlayDeathAnim )
	{
		// Set our position for a death anim.
		SetAbsOrigin( pPlayer->GetRenderOrigin() );			
		SetNetworkOrigin( pPlayer->GetRenderOrigin() );
		SetAbsAngles( pPlayer->GetRenderAngles() );
		SetAbsVelocity( Vector(0,0,0) );
		m_vecForce = Vector(0,0,0);

		// Play the death anim.
		ResetSequence( iDeathSeq );
		m_bDeathAnim = true;
	}

	// Fade out the ragdoll in a while
	StartFadeOut( cl_ragdoll_fade_time.GetFloat() );
	SetNextClientThink( CLIENT_THINK_ALWAYS );

	// Copy over impact attachments.
	if ( pPlayer )
	{
		pPlayer->MoveBoneAttachments( this );
	}

	// Save ragdoll information.
	if ( cl_ragdoll_physics_enable.GetBool() && !m_bDeathAnim )
	{
		// Make us a ragdoll..
		m_nRenderFX = kRenderFxRagdoll;

		matrix3x4_t boneDelta0[MAXSTUDIOBONES];
		matrix3x4_t boneDelta1[MAXSTUDIOBONES];
		matrix3x4_t currentBones[MAXSTUDIOBONES];
		const float boneDt = 0.05f;

		// We have to make sure that we're initting this client ragdoll off of the same model.
		// GetRagdollInitBoneArrays uses the *player* Hdr, which may be a different model than
		// the ragdoll Hdr, if we try to create a ragdoll in the same frame that the player
		// changes their player model.
		CStudioHdr *pRagdollHdr = GetModelPtr();
		CStudioHdr *pPlayerHdr = pPlayer ? pPlayer->GetModelPtr() : NULL;

		bool bChangedModel = false;

		if ( pRagdollHdr && pPlayerHdr )
		{
			bChangedModel = pRagdollHdr->GetVirtualModel() != pPlayerHdr->GetVirtualModel();

			// Assert( !bChangedModel && "C_TFRagdoll::CreateTFRagdoll: Trying to create ragdoll with a different model than the player it's based on" );
		}

		bool bBoneArraysInited;
		if ( pPlayer && !pPlayer->IsDormant() && !bChangedModel )
		{
			bBoneArraysInited = pPlayer->GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
		}
		else
		{
			bBoneArraysInited = GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
		}
		
		if ( bBoneArraysInited )
		{
			InitAsClientRagdoll( boneDelta0, boneDelta1, currentBones, boneDt, m_bFixedConstraints );
		}
	}
	else
	{
		ClientLeafSystem()->SetRenderGroup( GetRenderHandle(), RENDER_GROUP_TRANSLUCENT_ENTITY );
	}

	if ( m_bBurning )
	{
		m_flBurnEffectStartTime = gpGlobals->curtime;
		ParticleProp()->Create( "burningplayer_corpse", PATTACH_ABSORIGIN_FOLLOW );
	}

	// Birthday mode.
	if ( pPlayer && TFGameRules() && TFGameRules()->IsBirthday() )
	{
		AngularImpulse angularImpulse( RandomFloat( 0.0f, 120.0f ), RandomFloat( 0.0f, 120.0f ), 0.0 );
		breakablepropparams_t breakParams( m_vecRagdollOrigin, GetRenderAngles(), m_vecRagdollVelocity, angularImpulse );
		breakParams.impactEnergyScale = 1.0f;
		pPlayer->DropPartyHat( breakParams, m_vecRagdollVelocity.GetForModify() );
	}

	const char *materialOverrideFilename = NULL;

	if ( materialOverrideFilename )
	{
		// Ice texture...we've been turned into an ice statue!
		m_MaterialOverride.Init( materialOverrideFilename, TEXTURE_GROUP_CLIENT_EFFECTS );
	}
}

float C_TFRagdoll::FrameAdvance( float flInterval )
{
	// if we're in the process of becoming an ice statue, freeze
	if ( m_freezeTimer.HasStarted() && !m_freezeTimer.IsElapsed() )
	{
		// play the backstab anim until the timer is up
		return BaseClass::FrameAdvance( flInterval );
	}

	if ( m_frozenTimer.HasStarted() )
	{
		if ( m_frozenTimer.IsElapsed() )
		{
			// holding frozen time is up - turn to a stiff ragdoll and fall over
			m_frozenTimer.Invalidate();

			m_nRenderFX = kRenderFxRagdoll;

			matrix3x4_t boneDelta0[MAXSTUDIOBONES];
			matrix3x4_t boneDelta1[MAXSTUDIOBONES];
			matrix3x4_t currentBones[MAXSTUDIOBONES];
			const float boneDt = 0.1f;
			GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
			InitAsClientRagdoll( boneDelta0, boneDelta1, currentBones, boneDt, true );

			SetAbsVelocity( Vector( 0,0,0 ) );
			m_bRagdollOn = true;
		}
		else
		{
			// don't move at all
			return 0.0f;
		}
	}

	float fRes = BaseClass::FrameAdvance( flInterval );

	if ( !m_bRagdollOn && IsSequenceFinished() && m_bDeathAnim )
	{
		m_nRenderFX = kRenderFxRagdoll;

		matrix3x4_t boneDelta0[MAXSTUDIOBONES];
		matrix3x4_t boneDelta1[MAXSTUDIOBONES];
		matrix3x4_t currentBones[MAXSTUDIOBONES];
		const float boneDt = 0.1f;
		if ( !GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt ) )
		{
			Warning( "C_TFRagdoll::FrameAdvance GetRagdollInitBoneArrays failed.\n" );
		}
		else
		{
			InitAsClientRagdoll( boneDelta0, boneDelta1, currentBones, boneDt );
		}

		SetAbsVelocity( Vector( 0,0,0 ) );
		m_bRagdollOn = true;

		// Make it fade out.
		StartFadeOut( cl_ragdoll_fade_time.GetFloat() );
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}

	return fRes;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFRagdoll::CreateTFGibs( bool bDestroyRagdoll, bool bCurrentPosition )
{
	C_TFPlayer *pPlayer = GetPlayer();

	if ( pPlayer && ((pPlayer->m_hFirstGib == NULL) ) )
	{
		Vector vecVelocity = m_vecForce + m_vecRagdollVelocity;
		VectorNormalize( vecVelocity );
		pPlayer->CreatePlayerGibs( bCurrentPosition ? pPlayer->GetRenderOrigin() : m_vecRagdollOrigin, vecVelocity, m_vecForce.Length(), m_bBurning );
	}

	if ( pPlayer )
	{
		if ( TFGameRules() && TFGameRules()->IsBirthday() )
		{
			DispatchParticleEffect( "bday_confetti", pPlayer->GetAbsOrigin() + Vector(0,0,32), vec3_angle );

			if ( TFGameRules() && TFGameRules()->IsBirthday() )
			{
				C_BaseEntity::EmitSound( "Game.HappyBirthday" );
			}
		}
	}

	if ( bDestroyRagdoll )
	{
		EndFadeOut();
	}
	else
	{
		SetRenderMode( kRenderNone );
		UpdateVisibility();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//-----------------------------------------------------------------------------
void C_TFRagdoll::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		bool bCreateRagdoll = true;

		// Get the player.
		C_TFPlayer *pPlayer = GetPlayer();
		if ( pPlayer )
		{
			// If we're getting the initial update for this player (e.g., after resetting entities after
			//  lots of packet loss, then don't create gibs, ragdolls if the player and it's gib/ragdoll
			//  both show up on same frame.
			if ( abs( pPlayer->GetCreationTick() - gpGlobals->tickcount ) < TIME_TO_TICKS( 1.0f ) )
			{
				bCreateRagdoll = false;
			}
		}
		else if ( C_BasePlayer::GetLocalPlayer() )
		{
			// Ditto for recreation of the local player
			if ( abs( C_BasePlayer::GetLocalPlayer()->GetCreationTick() - gpGlobals->tickcount ) < TIME_TO_TICKS( 1.0f ) )
			{
				bCreateRagdoll = false;
			}
		}

		// Prevent replays from creating ragdolls on the first frame of playback after skipping through playback.
		// If a player died (leaving a ragdoll) previous to the first frame of replay playback,
		// their ragdoll wasn't yet initialized because OnDataChanged events are queued but not processed
		// until the first render. 
		if ( engine->IsPlayingDemo() )
		{
			bCreateRagdoll = !m_bCreatedWhilePlaybackSkipping;
		}

		if ( bCreateRagdoll )
		{
			if ( m_bGib )
			{
				CreateTFGibs( true );
			}
			else
			{
				CreateTFRagdoll();
			}

			m_bNoModelParticles = true;	
		}
	}
	else 
	{
		if ( !cl_ragdoll_physics_enable.GetBool() )
		{
			// Don't let it set us back to a ragdoll with data from the server.
			m_nRenderFX = kRenderFxNone;
		}
	}
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int C_TFRagdoll::InternalDrawModel( int flags )
{
	if ( m_MaterialOverride.IsValid() )
	{
		modelrender->ForcedMaterialOverride( m_MaterialOverride );
	}

	int ret = BaseClass::InternalDrawModel( flags );

	if ( m_MaterialOverride.IsValid() )
	{
		modelrender->ForcedMaterialOverride( NULL );
	}

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : IRagdoll*
//-----------------------------------------------------------------------------
IRagdoll* C_TFRagdoll::GetIRagdoll() const
{
	return m_pRagdoll;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_TFRagdoll::IsRagdollVisible()
{
	Vector vMins = Vector(-1,-1,-1);	//WorldAlignMins();
	Vector vMaxs = Vector(1,1,1);	//WorldAlignMaxs();
		
	Vector origin = GetAbsOrigin();
	
	if( !engine->IsBoxInViewCluster( vMins + origin, vMaxs + origin) )
	{
		return false;
	}
	else if( engine->CullBox( vMins + origin, vMaxs + origin ) )
	{
		return false;
	}

	return true;
}
#define DISSOLVE_FADE_IN_START_TIME			0.0f
#define DISSOLVE_FADE_IN_END_TIME			1.0f
#define DISSOLVE_FADE_OUT_MODEL_START_TIME	1.9f
#define DISSOLVE_FADE_OUT_MODEL_END_TIME	2.0f
#define DISSOLVE_FADE_OUT_START_TIME		2.0f
#define DISSOLVE_FADE_OUT_END_TIME			2.0f

void C_TFRagdoll::ClientThink( void )
{
	SetNextClientThink( CLIENT_THINK_ALWAYS );

	// Fade us away...
	if ( m_bFadingOut == true )
	{
		int iAlpha = GetRenderColor().a;
		int iFadeSpeed = 600.0f;

		iAlpha = MAX( iAlpha - ( iFadeSpeed * gpGlobals->frametime ), 0 );

		SetRenderMode( kRenderTransAlpha );
		SetRenderColorA( iAlpha );

		if ( iAlpha == 0 )
		{
			// Remove clientside ragdoll.
			EndFadeOut();
		}
		return;
	}

	// If the player is looking at us, delay the fade.
	if ( IsRagdollVisible() )
	{
		if ( cl_ragdoll_forcefade.GetBool() )
		{
			m_bFadingOut = true;
			float flDelay = cl_ragdoll_fade_time.GetFloat() * 0.33f;
			m_fDeathTime = gpGlobals->curtime + flDelay;

			RemoveAllDecals();
		}

		// Fade out after the specified delay.
		StartFadeOut( cl_ragdoll_fade_time.GetFloat() * 0.33f );
		return;
	}

	// Remove us if our death time has passed.
	if ( m_fDeathTime < gpGlobals->curtime )
	{
		EndFadeOut();
		return;
	}
}

// Deal with recording
void C_TFRagdoll::GetToolRecordingState( KeyValues *msg )
{
#ifndef _XBOX
	BaseClass::GetToolRecordingState( msg );

	if ( m_MaterialOverride.IsValid() )
	{
		msg->SetString( "materialOverride", m_MaterialOverride->GetName() );
	}
#endif
}

void C_TFRagdoll::StartFadeOut( float fDelay )
{
	if ( !cl_ragdoll_forcefade.GetBool() )
	{
		m_fDeathTime = gpGlobals->curtime + fDelay;
	}
	SetNextClientThink( CLIENT_THINK_ALWAYS );
}


void C_TFRagdoll::EndFadeOut()
{
	SetNextClientThink( CLIENT_THINK_NEVER );
	ClearRagdoll();
	SetRenderMode( kRenderNone );
	UpdateVisibility();
	DestroyBoneAttachments();

	// Remove attached effect entity
	C_BaseEntity *pEffect = GetEffectEntity();
	if ( pEffect )
	{
		pEffect->SUB_Remove();
	}

	ParticleProp()->StopEmission();
}


//-----------------------------------------------------------------------------
// Purpose: Used for spy invisibility material
//-----------------------------------------------------------------------------
class CSpyInvisProxy : public CBaseInvisMaterialProxy
{
public:
	CSpyInvisProxy( void );
	virtual bool		Init( IMaterial *pMaterial, KeyValues* pKeyValues ) OVERRIDE;
	virtual void		OnBind( C_BaseEntity *pBaseEntity ) OVERRIDE;
	virtual void		OnBindNotEntity( void *pRenderable ) OVERRIDE;

private:
	IMaterialVar		*m_pCloakColorTint;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSpyInvisProxy::CSpyInvisProxy( void )
{
	m_pCloakColorTint = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Get pointer to the color value
// Input  : *pMaterial - 
//-----------------------------------------------------------------------------
bool CSpyInvisProxy::Init( IMaterial *pMaterial, KeyValues* pKeyValues )
{
	// Need to get the material var
	bool bInvis = CBaseInvisMaterialProxy::Init( pMaterial, pKeyValues );

	bool bTint;
	m_pCloakColorTint = pMaterial->FindVar( "$cloakColorTint", &bTint );

	return ( bInvis && bTint );
}

ConVar tf_teammate_max_invis( "tf_teammate_max_invis", "0.95", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
//-----------------------------------------------------------------------------
void CSpyInvisProxy::OnBind( C_BaseEntity *pBaseEntity )
{
	if( !m_pPercentInvisible || !m_pCloakColorTint )
		return;

	float fInvis = 0.0f;

	C_TFPlayer *pPlayer = ToTFPlayer( pBaseEntity );

	if ( !pPlayer )
	{
		C_TFPlayer *pOwningPlayer = ToTFPlayer( pBaseEntity->GetOwnerEntity() );

		if ( pOwningPlayer )
		{
			// mimic the owner's invisibility
			fInvis = pOwningPlayer->GetEffectiveInvisibilityLevel();
		}
	}
	else
	{
		float r = 1.0f, g = 1.0f, b = 1.0f;
		fInvis = pPlayer->GetEffectiveInvisibilityLevel();

		switch( pPlayer->GetTeamNumber() )
		{
		case TF_TEAM_RED:
			r = 1.0; g = 0.5; b = 0.4;
			break;

		case TF_TEAM_BLUE:
		default:
			r = 0.4; g = 0.5; b = 1.0;
			break;
		}

		m_pCloakColorTint->SetVecValue( r, g, b );
	}

	m_pPercentInvisible->SetFloatValue( fInvis );
}

void CSpyInvisProxy::OnBindNotEntity( void *pRenderable )
{
	CBaseInvisMaterialProxy::OnBindNotEntity( pRenderable );

	if ( m_pCloakColorTint )
	{
		m_pCloakColorTint->SetVecValue( 1.f, 1.f, 1.f );
	}
}

EXPOSE_INTERFACE( CSpyInvisProxy, IMaterialProxy, "spy_invis" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Used for invulnerability material
//			Returns 1 if the player is invulnerable, and 0 if the player is losing / doesn't have invuln.
//-----------------------------------------------------------------------------
class CProxyInvulnLevel : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		float flResult = 1.0;

		C_TFPlayer *pPlayer = NULL;
		C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
		if ( !pEntity )
		{
			m_pResult->SetFloatValue( flResult );
			return;
		}

		if ( pEntity->IsPlayer()  )
		{
			pPlayer = dynamic_cast< C_TFPlayer* >( pEntity );
		}
		else
		{
			IHasOwner *pOwnerInterface = dynamic_cast< IHasOwner* >( pEntity );
			if ( pOwnerInterface )
			{
				pPlayer = ToTFPlayer( pOwnerInterface->GetOwnerViaInterface() );
			}
		}

		if ( pPlayer && pPlayer->m_Shared.IsInvulnerable() && pPlayer->m_Shared.InCond( TF_COND_INVULNERABLE_WEARINGOFF ) )
		{
			flResult = 0.0;
		}

		m_pResult->SetFloatValue( flResult );

		if ( ToolsEnabled() )
		{
			ToolFramework_RecordMaterialParams( GetMaterial() );
		}
	}
};

EXPOSE_INTERFACE( CProxyInvulnLevel, IMaterialProxy, "InvulnLevel" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Used for burning material on player models
//			Returns 0.0->1.0 for level of burn to show on player skin
//-----------------------------------------------------------------------------
class CProxyBurnLevel : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		float flResult = 0.0;

		C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
		if ( !pEntity )
		{
			m_pResult->SetFloatValue( flResult );
			return;
		}

		// default to zero
		float flBurnStartTime = 0;
	
		if ( pEntity->IsPlayer() )
		{
			C_TFPlayer *pPlayer = assert_cast< C_TFPlayer* >( pEntity );
			// is the player burning?
			if (  pPlayer->m_Shared.InCond( TF_COND_BURNING ) )
			{
				flBurnStartTime = pPlayer->m_flBurnEffectStartTime;
			}
		}
		else
		{
			// is the ragdoll burning?
			C_TFRagdoll *pRagDoll = dynamic_cast< C_TFRagdoll* >( pEntity );
			if ( pRagDoll )
			{
				flBurnStartTime = pRagDoll->GetBurnStartTime();
			}
		}
		
		// if player/ragdoll is burning, set the burn level on the skin
		if ( flBurnStartTime > 0 )
		{
			float flBurnPeakTime = flBurnStartTime + 0.3;
			float flTempResult;
			if ( gpGlobals->curtime < flBurnPeakTime )
			{
				// fade in from 0->1 in 0.3 seconds
				flTempResult = RemapValClamped( gpGlobals->curtime, flBurnStartTime, flBurnPeakTime, 0.0, 1.0 );
			}
			else
			{
				// fade out from 1->0 in the remaining time until flame extinguished
				flTempResult = RemapValClamped( gpGlobals->curtime, flBurnPeakTime, flBurnStartTime + TF_BURNING_FLAME_LIFE, 1.0, 0.0 );
			}	

			// We have to do some more calc here instead of in materialvars.
			flResult = 1.0 - abs( flTempResult - 1.0 );
		}

		m_pResult->SetFloatValue( flResult );

		if ( ToolsEnabled() )
		{
			ToolFramework_RecordMaterialParams( GetMaterial() );
		}
	}
};

EXPOSE_INTERFACE( CProxyBurnLevel, IMaterialProxy, "BurnLevel" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
C_TFPlayer *GetOwnerFromProxyEntity( void *pEntity )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	IClientRenderable *pRend = ( IClientRenderable * ) pEntity;
	CBaseEntity *pBaseEntity = pRend ? pRend->GetIClientUnknown()->GetBaseEntity() : NULL;

	// If an entity, find out what types it is and get the econ item view
	if ( pBaseEntity )
	{
		CBaseEntity* pOwner = pBaseEntity->GetOwnerEntity();
		if ( pOwner )
			return dynamic_cast<C_TFPlayer*>( pOwner->GetOwnerEntity() );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: RecvProxy that converts the Player's object UtlVector to entindexes
//-----------------------------------------------------------------------------
void RecvProxy_PlayerObjectList( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_TFPlayer *pPlayer = (C_TFPlayer*)pStruct;
	CBaseHandle *pHandle = (CBaseHandle*)(&(pPlayer->m_aObjects[pData->m_iElement])); 
	RecvProxy_IntToEHandle( pData, pStruct, pHandle );
}

void RecvProxyArrayLength_PlayerObjects( void *pStruct, int objectID, int currentArrayLength )
{
	C_TFPlayer *pPlayer = (C_TFPlayer*)pStruct;

	if ( pPlayer->m_aObjects.Count() != currentArrayLength )
	{
		pPlayer->m_aObjects.SetSize( currentArrayLength );
	}

	pPlayer->ForceUpdateObjectHudState();
}

EXTERN_RECV_TABLE( DT_ScriptCreatedItem );

// specific to the local player
BEGIN_RECV_TABLE_NOBASE( C_TFPlayer, DT_TFLocalPlayerExclusive )
	RecvPropVectorXY( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
	RecvPropFloat( RECVINFO_NAME( m_vecNetworkOrigin[2], m_vecOrigin[2] ) ),
	RecvPropArray2( 
		RecvProxyArrayLength_PlayerObjects,
		RecvPropInt( "player_object_array_element", 0, SIZEOF_IGNORE, 0, RecvProxy_PlayerObjectList ), 
		MAX_OBJECTS_PER_PLAYER, 
		0, 
		"player_object_array"	),

	RecvPropFloat( RECVINFO( m_angEyeAngles[0] ) ),
	RecvPropFloat( RECVINFO( m_angEyeAngles[1] ) ),

	RecvPropInt( RECVINFO( m_nExperienceLevel ) ),
	RecvPropInt( RECVINFO( m_nExperienceLevelProgress ) ),
	RecvPropInt( RECVINFO( m_bMatchSafeToLeave ) ),

END_RECV_TABLE()

// all players except the local player
BEGIN_RECV_TABLE_NOBASE( C_TFPlayer, DT_TFNonLocalPlayerExclusive )
	RecvPropVectorXY( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
	RecvPropFloat( RECVINFO_NAME( m_vecNetworkOrigin[2], m_vecOrigin[2] ) ),

	RecvPropFloat( RECVINFO( m_angEyeAngles[0] ) ),
	RecvPropFloat( RECVINFO( m_angEyeAngles[1] ) ),

END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Data that gets sent to attached medics
//-----------------------------------------------------------------------------
BEGIN_RECV_TABLE_NOBASE( C_TFPlayer, DT_TFSendHealersDataTable )
	RecvPropInt( RECVINFO( m_nActiveWpnClip ) ),
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT( C_TFPlayer, DT_TFPlayer, CTFPlayer )

	RecvPropBool(RECVINFO(m_bSaveMeParity)),

	// This will create a race condition will the local player, but the data will be the same so.....
	RecvPropInt( RECVINFO( m_nWaterLevel ) ),
	RecvPropEHandle( RECVINFO( m_hRagdoll ) ),
	RecvPropDataTable( RECVINFO_DT( m_PlayerClass ), 0, &REFERENCE_RECV_TABLE( DT_TFPlayerClassShared ) ),
	RecvPropDataTable( RECVINFO_DT( m_Shared ), 0, &REFERENCE_RECV_TABLE( DT_TFPlayerShared ) ),
	RecvPropEHandle( RECVINFO(m_hItem ) ),

	RecvPropDataTable( "tflocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_TFLocalPlayerExclusive) ),
	RecvPropDataTable( "tfnonlocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_TFNonLocalPlayerExclusive) ),

	RecvPropInt( RECVINFO( m_nForceTauntCam ) ),
	RecvPropFloat( RECVINFO( m_flTauntYaw ) ),

	RecvPropInt( RECVINFO( m_iSpawnCounter ) ),
	RecvPropBool( RECVINFO( m_bFlipViewModels ) ),

	RecvPropFloat( RECVINFO( m_flHeadScale ) ),
	RecvPropFloat( RECVINFO( m_flTorsoScale ) ),
	RecvPropFloat( RECVINFO( m_flHandScale ) ),

	RecvPropBool( RECVINFO( m_bUsingVRHeadset ) ),

	RecvPropDataTable("TFSendHealersDataTable", 0, 0, &REFERENCE_RECV_TABLE( DT_TFSendHealersDataTable ) ),
	RecvPropEHandle( RECVINFO( m_hSecondaryLastWeapon ) ),
	RecvPropBool( RECVINFO( m_bUsingActionSlot ) ),
	RecvPropFloat( RECVINFO( m_flHelpmeButtonPressTime ) ),
	RecvPropBool( RECVINFO( m_bRegenerating ) ),
END_RECV_TABLE()


BEGIN_PREDICTION_DATA( C_TFPlayer )
	DEFINE_PRED_TYPEDESCRIPTION( m_Shared, CTFPlayerShared ),
	DEFINE_PRED_FIELD( m_nSkin, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE ),
	DEFINE_PRED_FIELD( m_nBody, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE ),
	DEFINE_PRED_FIELD( m_nSequence, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_flPlaybackRate, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_flCycle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_ARRAY_TOL( m_flEncodedController, FIELD_FLOAT, MAXSTUDIOBONECTRLS, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE, 0.02f ),
	DEFINE_PRED_FIELD( m_nNewSequenceParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_nResetEventsParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_nMuzzleFlashParity, FIELD_CHARACTER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE  ),
	DEFINE_PRED_FIELD( m_hOffHandWeapon, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flTauntYaw, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flHelpmeButtonPressTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()

// ------------------------------------------------------------------------------------------ //
// C_TFPlayer implementation.
// ------------------------------------------------------------------------------------------ //

C_TFPlayer::C_TFPlayer() : 
	m_iv_angEyeAngles( "C_TFPlayer::m_iv_angEyeAngles" ),
	m_mapOverheadEffects( DefLessFunc( const char * ) )
{
	m_PlayerAnimState = CreateTFPlayerAnimState( this );
	m_Shared.Init( this );

	m_iIDEntIndex = 0;

	AddVar( &m_angEyeAngles, &m_iv_angEyeAngles, LATCH_SIMULATION_VAR );
	
	m_pTeleporterEffect = NULL;
	m_pBurningSound = NULL;
	m_pBurningEffect = NULL;
	m_flBurnEffectStartTime = 0;
	m_pDisguisingEffect = NULL;
	m_pSaveMeEffect = NULL;
	m_hOldObserverTarget = NULL;
	m_iOldObserverMode = OBS_MODE_NONE;

	m_aGibs.Purge();
	m_aNormalGibs.PurgeAndDeleteElements();
	m_aSillyGibs.Purge();

	m_bCigaretteSmokeActive = false;

	m_hRagdoll.Set( NULL );

	m_iPreviousMetal = 0;
	m_bIsDisplayingNemesisIcon = false;
	m_bShouldShowBirthdayEffect = false;

	m_bWasTaunting = false;
	m_angTauntPredViewAngles.Init();
	m_angTauntEngViewAngles.Init();
	m_pTauntSoundLoop = NULL;

	m_flWaterImpactTime = 0.0f;
//	m_rtSpottedInPVSTime = 0;
//	m_rtJoinedSpectatorTeam = 0;
//	m_rtJoinedNormalTeam = 0;

	m_flWaterEntryTime = 0;
	m_nOldWaterLevel = WL_NotInWater;
	m_bWaterExitEffectActive = false;

	m_bUpdateObjectHudState = false;

	m_flSaveMeExpireTime = 0;

	m_bWasHealedByLocalPlayer = false;

	m_bDuckJumpInterp = false;
	m_flFirstDuckJumpInterp = 0.0f;
	m_flLastDuckJumpInterp = 0.0f;
	m_flDuckJumpInterp = 0.0f;

	m_nOldMaxHealth = -1;

	m_bBodygroupsDirty = false;

	m_nExperienceLevel = 0;
	m_nExperienceLevelProgress = 0;
	m_nPrevExperienceLevel = 0;

	m_bMatchSafeToLeave = true;

	for( int i=0; i<kBonusEffect_Count; ++i )
	{
		m_flNextMiniCritEffectTime[i] = 0;
	}

	m_flHeadScale = 1.f;
	m_flTorsoScale = 1.f;
	m_flHandScale = 1.f;

	m_bUsingVRHeadset = false;

	m_flChangeClassTime = 0.f;

	m_bIsDisplayingTranqMark = false;

	m_bUsingActionSlot = false;

	m_flHelpmeButtonPressTime = 0.f;
	m_bRegenerating = false;

	ListenForGameEvent( "player_hurt" );
	ListenForGameEvent( "hltv_changed_mode" );
	ListenForGameEvent( "hltv_changed_target" );
	ListenForGameEvent( "player_spawn" );
	ListenForGameEvent( "player_changeclass" );

	//AddPhonemeFile
	engine->AddPhonemeFile( "scripts/game_sounds_vo_phonemes.txt" );
	engine->AddPhonemeFile( "scripts/game_sounds_vo_phonemes_local.txt" );	// Stomp over english for phoneme data
	engine->AddPhonemeFile( NULL );			// Null indicates to engine that we are done loading phonemes if there are any present
}

C_TFPlayer::~C_TFPlayer()
{
	ShowNemesisIcon( false );
	m_PlayerAnimState->Release();

	StopTauntSoundLoop();
}

// NOTE: This is NOT called every time the player respawns!!
// only the first time we spawn a player into the world
void C_TFPlayer::Spawn( void )
{
	BaseClass::Spawn();

	/*
	// some extra stuff here because s_pLocalPlayer is not yet initialized
	int iLocalPlayerIndex = engine->GetLocalPlayer();

	if ( entindex() == iLocalPlayerIndex && !m_LeaveServerTimer.HasStarted() )
	{
		ConVarRef random_spec_server_mode( "random_spec_server_mode" );
		if ( random_spec_server_mode.IsValid() && random_spec_server_mode.GetBool() )
		{
			m_LeaveServerTimer.Start( spectate_random_server_basetime.GetFloat() );
		}
	}
	*/

	SetShowHudMenuTauntSelection( false );

	CleanUpAnimationOnSpawn();
}

C_TFPlayer* C_TFPlayer::GetLocalTFPlayer()
{
	return ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
}

const QAngle& C_TFPlayer::GetRenderAngles()
{
	if ( IsRagdoll() )
	{
		return vec3_angle;
	}
	else
	{
		return m_PlayerAnimState->GetRenderAngles();
	}
}

bool C_TFPlayer::CanDisplayAllSeeEffect( EAttackBonusEffects_t effect ) const
{ 
	if( effect >= EAttackBonusEffects_t(0) && effect < kBonusEffect_Count )
	{
		return gpGlobals->curtime > m_flNextMiniCritEffectTime[ effect ];
	}

	return true;
}

void C_TFPlayer::SetNextAllSeeEffectTime( EAttackBonusEffects_t effect, float flTime )
{ 
	if( effect >= EAttackBonusEffects_t(0) && effect < kBonusEffect_Count )
	{
		if ( gpGlobals->curtime > m_flNextMiniCritEffectTime[ effect ] )
		{
			m_flNextMiniCritEffectTime[ effect ] = flTime;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::UpdateOnRemove( void )
{
	// Stop the taunt.
	if ( m_bWasTaunting )
	{
		// Need to go ahead and call both. Otherwise, if we changelevel while we're taunting or 
		// otherwise in "game wants us in third person mode", we will stay in third person mode 
		// in the new map.
		TurnOffTauntCam();
		TurnOffTauntCam_Finish();
	}

	// HACK!!! ChrisG needs to fix this in the particle system.
	ParticleProp()->OwnerSetDormantTo( true );
	ParticleProp()->StopParticlesInvolving( this );

	m_Shared.RemoveAllCond();

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: returns max health for this player
//-----------------------------------------------------------------------------
int C_TFPlayer::GetMaxHealth( void ) const
{	
	return ( g_TF_PR ) ? g_TF_PR->GetMaxHealth( entindex() ) : 1;
}

//-----------------------------------------------------------------------------
// Purpose: returns max buffed health for this player
//-----------------------------------------------------------------------------
int C_TFPlayer::GetMaxHealthForBuffing( void ) const
{
	return ( g_TF_PR ) ? g_TF_PR->GetMaxHealthForBuffing( entindex() ) : 1;
}

//-----------------------------------------------------------------------------
// Deal with recording
//-----------------------------------------------------------------------------
void C_TFPlayer::GetToolRecordingState( KeyValues *msg )
{
#ifndef _XBOX
	BaseClass::GetToolRecordingState( msg );
	BaseEntityRecordingState_t *pBaseEntityState = (BaseEntityRecordingState_t*)msg->GetPtr( "baseentity" );

	bool bDormant = IsDormant();
	bool bDead = !IsAlive();
	bool bSpectator = ( GetTeamNumber() == TEAM_SPECTATOR );
	bool bNoRender = ( GetRenderMode() == kRenderNone );
	bool bDeathCam = (GetObserverMode() == OBS_MODE_DEATHCAM);
	bool bNoDraw = IsEffectActive(EF_NODRAW);

	bool bVisible = 
		!bDormant && 
		!bDead && 
		!bSpectator &&
		!bNoRender &&
		!bDeathCam &&
		!bNoDraw;

	bool changed = m_bToolRecordingVisibility != bVisible;
	// Remember state
	m_bToolRecordingVisibility = bVisible;

	pBaseEntityState->m_bVisible = bVisible;
	if ( changed && !bVisible )
	{
		// If the entity becomes invisible this frame, we still want to record a final animation sample so that we have data to interpolate
		//  toward just before the logs return "false" for visiblity.  Otherwise the animation will freeze on the last frame while the model
		//  is still able to render for just a bit.
		pBaseEntityState->m_bRecordFinalVisibleSample = true;
	}
#endif
}


void C_TFPlayer::UpdateClientSideAnimation()
{
	// Update the animation data. It does the local check here so this works when using
	// a third-person camera (and we don't have valid player angles).
	if ( this == C_TFPlayer::GetLocalTFPlayer() )
	{
		// m_angEyeAngles comes from the server, and updates are infrequent, so use the local values instead.
		QAngle LocalEyeAngles = EyeAngles();
		m_PlayerAnimState->Update( LocalEyeAngles[YAW], LocalEyeAngles[PITCH] );
	}
	else
	{
		m_PlayerAnimState->Update( m_angEyeAngles[YAW], m_angEyeAngles[PITCH] );
	}

	BaseClass::UpdateClientSideAnimation();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::SetDormant( bool bDormant )
{
	// If I'm burning, stop the burning sounds
	if ( !IsDormant() && bDormant )
	{
		if ( m_pBurningSound) 
		{
			StopBurningSound();
		}
		if ( m_bIsDisplayingNemesisIcon )
		{
			ShowNemesisIcon( false );
		}

		if ( m_bShouldShowBirthdayEffect )
		{
			ShowBirthdayEffect( false );
		}
	}

	if ( IsDormant() && !bDormant )
	{
		SetBodygroupsDirty();

		if ( IsTaunting() )
		{
			float flCycle = 0.f;
			if ( m_flTauntDuration > 0.f )
			{
				float dt = gpGlobals->curtime - m_flTauntStartTime;
				while ( dt >= m_flTauntDuration )
				{
					dt -= m_flTauntDuration;
				}
				flCycle = dt / m_flTauntDuration;
				flCycle = clamp( flCycle, 0.f, 1.0f );
			}

			m_PlayerAnimState->ResetGestureSlot( GESTURE_SLOT_VCD );
			m_PlayerAnimState->AddVCDSequenceToGestureSlot( GESTURE_SLOT_VCD, m_nTauntSequence, flCycle, true );
		}
	}

// 	if ( bDormant == false )
// 	{
// 		m_rtSpottedInPVSTime = steamapicontext && steamapicontext->SteamUtils() ? steamapicontext->SteamUtils()->GetServerRealTime() : CRTime::RTime32TimeCur();
// 	}

	// Deliberately skip base combat weapon
	C_BaseEntity::SetDormant( bDormant );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_iOldHealth = m_iHealth;
	m_iOldPlayerClass = m_PlayerClass.GetClassIndex();
	m_iOldSpawnCounter = m_iSpawnCounter;
	m_bOldSaveMeParity = m_bSaveMeParity;
	m_nOldWaterLevel = GetWaterLevel();

	m_iOldTeam = GetTeamNumber();
	C_TFPlayerClass *pClass = GetPlayerClass();
	m_iOldClass = pClass ? pClass->GetClassIndex() : TF_CLASS_UNDEFINED;
	m_bDisguised = m_Shared.InCond( TF_COND_DISGUISED );
	m_iOldDisguiseTeam = m_Shared.GetDisguiseTeam();
	m_iOldDisguiseClass = m_Shared.GetDisguiseClass();

	m_flPrevTauntYaw = m_flTauntYaw;

	if ( !IsReplay() )
	{
		m_iOldObserverMode = GetObserverMode();
		m_hOldObserverTarget = GetObserverTarget();
	}

	m_Shared.OnPreDataChanged();

	m_bOldCustomModelVisible = m_PlayerClass.CustomModelIsVisibleToSelf();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::OnDataChanged( DataUpdateType_t updateType )
{
	// C_BaseEntity assumes we're networking the entity's angles, so pretend that it
	// networked the same value we already have.
	SetNetworkAngles( GetLocalAngles() );
	
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );

		InitInvulnerableMaterial();
		
		// There used to be code here to switch to first person. This breaks thirdperson mode
		// and karts when we have to get a full client update--don't do it here anymore.
		// We may need to do something more clever if this breaks something.
	}
	else
	{
		if ( m_iOldTeam != GetTeamNumber() || m_iOldDisguiseTeam != m_Shared.GetDisguiseTeam() )
		{
			InitInvulnerableMaterial();
		}
		
		if ( m_iOldDisguiseClass != m_Shared.GetDisguiseClass() )
		{
			RemoveAllDecals();
		}

		if ( m_nOldMaxHealth != GetMaxHealth() )
		{
			m_nOldMaxHealth = GetMaxHealth();
		}
	}

	// Check for full health and remove decals.
	if ( ( m_iHealth > m_iOldHealth && m_iHealth >= GetMaxHealth() ) || m_Shared.IsInvulnerable() )
	{
		// If we were just fully healed, remove all decals
		RemoveAllDecals();
	}

	// Detect class changes
	if ( m_iOldPlayerClass != m_PlayerClass.GetClassIndex() )
	{
		OnPlayerClassChange();
	}

	bool bJustSpawned = false;

	if ( m_iOldSpawnCounter != m_iSpawnCounter )
	{
		ClientPlayerRespawn();

		bJustSpawned = true;
	}

	if ( m_bSaveMeParity != m_bOldSaveMeParity )
	{
		// Player has triggered a save me command
		CreateSaveMeEffect();
	}

	// To better support old demos, which have some screwed up flags, we just ignore various things if we're a SourceTV client.
	if ( !IsHLTV() )
	{
		if ( m_Shared.InCond( TF_COND_BURNING ) && !m_pBurningSound )
		{
			StartBurningSound();
		}

		bool bShouldShowBirthdayEffect = false;//TFGameRules() && ( TFGameRules()->GetBirthdayPlayer() == this ) && !IsLocalPlayer();
		if ( bShouldShowBirthdayEffect != m_bShouldShowBirthdayEffect )
		{
			ShowBirthdayEffect( bShouldShowBirthdayEffect );
		}

		// See if we should show or hide nemesis icon for this player
		bool bShouldDisplayNemesisIcon = ( ShouldShowNemesisIcon() );
		if ( bShouldDisplayNemesisIcon != m_bIsDisplayingNemesisIcon )
		{
			ShowNemesisIcon( bShouldDisplayNemesisIcon );
		}

		m_Shared.OnDataChanged();

		if ( m_bDisguised != m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			m_flDisguiseEndEffectStartTime = MAX( m_flDisguiseEndEffectStartTime, gpGlobals->curtime );

			// Update visibility of any worn items.
			SetBodygroupsDirty();

			// Remove decals.
			RemoveAllDecals();
		}
	}

	int nNewWaterLevel = GetWaterLevel();

	if ( nNewWaterLevel != m_nOldWaterLevel )
	{
		if ( ( m_nOldWaterLevel == WL_NotInWater ) && ( nNewWaterLevel > WL_NotInWater ) )
		{
			// Set when we do a transition to/from partially in water to completely out
			m_flWaterEntryTime = gpGlobals->curtime;
		}

		// If player is now up to his eyes in water and has entered the water very recently (not just bobbing eyes in and out), play a bubble effect.
		if ( ( nNewWaterLevel == WL_Eyes ) && ( gpGlobals->curtime - m_flWaterEntryTime ) < 0.5f ) 
		{
			CNewParticleEffect *pEffect = ParticleProp()->Create( "water_playerdive", PATTACH_ABSORIGIN_FOLLOW );
			ParticleProp()->AddControlPoint( pEffect, 1, NULL, PATTACH_WORLDORIGIN, NULL, WorldSpaceCenter() );
		}		
		// If player was up to his eyes in water and is now out to waist level or less, play a water drip effect
		else if ( m_nOldWaterLevel == WL_Eyes && ( nNewWaterLevel < WL_Eyes ) && !bJustSpawned )
		{
			CNewParticleEffect *pWaterExitEffect = ParticleProp()->Create( "water_playeremerge", PATTACH_ABSORIGIN_FOLLOW );
			ParticleProp()->AddControlPoint( pWaterExitEffect, 1, this, PATTACH_ABSORIGIN_FOLLOW );
			m_bWaterExitEffectActive = true;
		}
	}

	if ( IsLocalPlayer() )
	{
		if ( updateType == DATA_UPDATE_CREATED )
		{
			SetupHeadLabelMaterials();
			GetClientVoiceMgr()->SetHeadLabelOffset( 50 );
		}

		if ( m_iOldTeam != GetTeamNumber() )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "localplayer_changeteam" );
			if ( event )
			{
				gameeventmanager->FireEventClientSide( event );
			}
			if ( IsX360() )
			{
				const char *pTeam = NULL;
				switch( GetTeamNumber() )
				{
					case TF_TEAM_RED:
						pTeam = "red";
						break;

					case TF_TEAM_BLUE:
						pTeam = "blue";
						break;

					case TEAM_SPECTATOR:
						pTeam = "spectate";
						break;
				}

				if ( pTeam )
				{
					engine->ChangeTeam( pTeam );
				}
			}

			// let the server know that we're using a VR headset
			if ( UseVR() )
			{
				KeyValues *kv = new KeyValues( "UsingVRHeadset" );
				engine->ServerCmdKeyValues( kv );
			}
		}

		if ( !IsPlayerClass(m_iOldClass) )
		{
			m_flChangeClassTime = gpGlobals->curtime;

			IGameEvent *event = gameeventmanager->CreateEvent( "localplayer_changeclass" );
			if ( event )
			{
				event->SetInt( "updateType", updateType );
				gameeventmanager->FireEventClientSide( event );
			}

		}

		bool bUpdateAttachedWeapons = (GetObserverTarget() != m_hOldObserverTarget);
		if ( m_iOldObserverMode != GetObserverMode() )
		{
			if ( m_iOldObserverMode == OBS_MODE_NONE )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "localplayer_becameobserver" );
				if ( event )
				{
					gameeventmanager->FireEventClientSide( event );
				}
				// NVNT send spectator nav
				if ( haptics )
					haptics->SetNavigationClass("spectate");
			}

			if ( m_iOldObserverMode == OBS_MODE_IN_EYE || GetObserverMode() == OBS_MODE_IN_EYE )
			{
				bUpdateAttachedWeapons = true;
			}

			if ( m_iOldObserverMode == OBS_MODE_IN_EYE )
			{
				CBaseEntity* pObserveTarget = GetObserverTarget();
				if( pObserveTarget )
				{
					pObserveTarget->UpdateVisibility();
				}
			}
			// NVNT send onfoot nav if observer mode is none.
			if(GetObserverMode()==OBS_MODE_NONE &&haptics)
			{
				haptics->SetNavigationClass("on_foot");
			}

			if ( IsReplay() )
			{
				m_iOldObserverMode = GetObserverMode();
			}
		}

		if ( bUpdateAttachedWeapons )
		{
			C_TFPlayer *pTFOldObserverTarget = ToTFPlayer( m_hOldObserverTarget.Get() );
			if ( m_hOldObserverTarget != GetObserverTarget() && pTFOldObserverTarget )
			{
				// Update visibility of any worn items.
				pTFOldObserverTarget->SetBodygroupsDirty();

				if ( IsReplay() )
				{
					m_hOldObserverTarget = GetObserverTarget();
				}
			}

			C_TFPlayer *pTFObserverTarget = ToTFPlayer( GetObserverTarget() );
			if ( pTFObserverTarget )
			{
				// Update visibility of any worn items.
				pTFObserverTarget->SetBodygroupsDirty();
			}
		}

		if ( m_iOldClass == TF_CLASS_SPY && 
		   ( m_bDisguised != m_Shared.InCond( TF_COND_DISGUISED ) || m_iOldDisguiseClass != m_Shared.GetDisguiseClass() ) )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "localplayer_changedisguise" );
			if ( event )
			{
				event->SetBool( "disguised", m_Shared.InCond( TF_COND_DISGUISED ) );
				gameeventmanager->FireEventClientSide( event );
			}
		}

		// If our metal amount changed, send a game event
		int iCurrentMetal = GetAmmoCount( TF_AMMO_METAL );	

		if ( iCurrentMetal != m_iPreviousMetal )
		{
			//msg
			IGameEvent *event = gameeventmanager->CreateEvent( "player_account_changed" );
			if ( event )
			{
				event->SetInt( "old_account", m_iPreviousMetal );
				event->SetInt( "new_account", iCurrentMetal );
				gameeventmanager->FireEventClientSide( event );
			}

			m_iPreviousMetal = iCurrentMetal;
		}

		// did the local player get any health?
		if ( m_iHealth > m_iOldHealth )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "localplayer_healed" );
			if ( event )
			{
				event->SetInt( "amount", m_iHealth - m_iOldHealth );
				gameeventmanager->FireEventClientSide( event );
			}
		}

		if ( m_bOldCustomModelVisible != m_PlayerClass.CustomModelIsVisibleToSelf() )
		{
			UpdateVisibility();
		}
	}

	C_TFWeaponBase *pOldActiveWeapon = assert_cast< CTFWeaponBase* >( m_hOldActiveWeapon.Get() ); 
	C_TFWeaponBase *pActiveWeapon = GetActiveTFWeapon(); 

	if ( pOldActiveWeapon != pActiveWeapon )
	{
		// make sure weapons data are up to date before doing anything here
		UpdateClientData();
		if ( pOldActiveWeapon )
		{
			pOldActiveWeapon->UpdateVisibility();
		}

		if ( pActiveWeapon )
		{
			pActiveWeapon->UpdateVisibility();
		}

		m_hOldActiveWeapon = pActiveWeapon;
	}

	// Some time in this network transmit we changed the size of the object array.
	// recalc the whole thing and update the hud
	if ( m_bUpdateObjectHudState )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "building_info_changed" );
		if ( event )
		{
			event->SetInt( "building_type", -1 );
			gameeventmanager->FireEventClientSide( event );
		}
	
		m_bUpdateObjectHudState = false;
	}

// 	if ( m_iOldTeam != GetTeamNumber() )
// 	{
// 		if ( GetTeamNumber() == TEAM_SPECTATOR )
// 		{
// 			m_rtJoinedSpectatorTeam = steamapicontext && steamapicontext->SteamUtils() ? steamapicontext->SteamUtils()->GetServerRealTime() : CRTime::RTime32TimeCur();
// 		}
// 		else if ( m_iOldTeam != TF_TEAM_RED && m_iOldTeam != TF_TEAM_BLUE )
// 		{
// 			m_rtJoinedNormalTeam = steamapicontext && steamapicontext->SteamUtils() ? steamapicontext->SteamUtils()->GetServerRealTime() : CRTime::RTime32TimeCur();
// 		}
// 	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::InitInvulnerableMaterial( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	const char *pszMaterial = NULL;

	int iVisibleTeam = GetTeamNumber();
	// if this player is disguised and on the other team, use disguise team
	if ( m_Shared.InCond( TF_COND_DISGUISED ) && !InSameTeam( pLocalPlayer ) )
	{
		iVisibleTeam = m_Shared.GetDisguiseTeam();
	}

	switch ( iVisibleTeam )
	{
	case TF_TEAM_BLUE:	
		pszMaterial = "models/effects/invulnfx_blue.vmt";
		break;
	case TF_TEAM_RED:	
		pszMaterial = "models/effects/invulnfx_red.vmt";
		break;
	default:
		break;
	}

	if ( pszMaterial )
	{
		m_InvulnerableMaterial.Init( pszMaterial, TEXTURE_GROUP_CLIENT_EFFECTS );
	}
	else
	{
		m_InvulnerableMaterial.Shutdown();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::StartBurningSound( void )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	if ( !m_pBurningSound )
	{
		CLocalPlayerFilter filter;
		m_pBurningSound = controller.SoundCreate( filter, entindex(), "Player.OnFire" );
	}

	controller.Play( m_pBurningSound, 0.0, 100 );
	controller.SoundChangeVolume( m_pBurningSound, 1.0, 0.1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::StopBurningSound( void )
{
	if ( m_pBurningSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pBurningSound );
		m_pBurningSound = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::UpdateRecentlyTeleportedEffect( void )
{
	bool bShow = m_Shared.ShouldShowRecentlyTeleported();

	if ( bShow )
	{			
		// NVNT if this is the local player notify haptics system of this teleporter
		//   teleporting.
		if(IsLocalPlayer()&&!tfHaptics.wasBeingTeleported) {
			if ( haptics )
				haptics->ProcessHapticEvent(2,"Game","player_teleport");
			tfHaptics.wasBeingHealed = true;
		}
		if ( !m_pTeleporterEffect )
		{
			const char *pszEffectName = NULL;

			int iTeam = GetTeamNumber();
			if ( m_Shared.InCond( TF_COND_DISGUISED ) )
			{
				iTeam = m_Shared.GetDisguiseTeam();
			}

			switch( iTeam )
			{
			case TF_TEAM_BLUE:
				pszEffectName = "player_recent_teleport_blue";
				break;
			case TF_TEAM_RED:
				pszEffectName = "player_recent_teleport_red";
				break;
			default:
				break;
			}

			if ( pszEffectName )
			{
				m_pTeleporterEffect = ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );
			}
		}
	}
	else
	{
		// NVNT if this is the local player and we were being teleported
		// flag that we are no longer teleporting.
		if(IsLocalPlayer()&&tfHaptics.wasBeingTeleported) {
			tfHaptics.wasBeingHealed = false;
		}

		if ( m_pTeleporterEffect )
		{
			ParticleProp()->StopEmission( m_pTeleporterEffect );
			m_pTeleporterEffect = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::OnPlayerClassChange( void )
{
	// Init the anim movement vars
	m_PlayerAnimState->SetRunSpeed( GetPlayerClass()->GetMaxSpeed() );
	m_PlayerAnimState->SetWalkSpeed( GetPlayerClass()->GetMaxSpeed() * 0.5 );

	ShowNemesisIcon( false );

	SetAppropriateCamera( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::InitPhonemeMappings()
{
	CStudioHdr *pStudio = GetModelPtr();
	if ( pStudio )
	{
		char szBasename[MAX_PATH];
		Q_StripExtension( pStudio->pszName(), szBasename, sizeof( szBasename ) );
		char szExpressionName[MAX_PATH];
		Q_snprintf( szExpressionName, sizeof( szExpressionName ), "%s/phonemes/phonemes", szBasename );
		if ( FindSceneFile( szExpressionName ) )
		{
			SetupMappings( szExpressionName );	
		}
		else
		{
			BaseClass::InitPhonemeMappings();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::ResetFlexWeights( CStudioHdr *pStudioHdr )
{
	if ( !pStudioHdr || pStudioHdr->numflexdesc() == 0 )
		return;

	// Reset the flex weights to their starting position.
	LocalFlexController_t iController;
	for ( iController = LocalFlexController_t(0); iController < pStudioHdr->numflexcontrollers(); ++iController )
	{
		SetFlexWeight( iController, 0.0f );
	}

	// Reset the prediction interpolation values.
	m_iv_flexWeight.Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::CalcInEyeCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov )
{
	QAngle myEyeAngles;
	VectorCopy( EyeAngles(), myEyeAngles );
	BaseClass::CalcInEyeCamView( eyeOrigin, eyeAngles, fov );

	/*
	// if we are coaching, we override the eye angles with our original ones
	// @note Tom Bui: we don't try to capture the "up" button event, because that doesn't seem so reliable
	if ( m_bIsCoaching )
	{
		const float kLerpTime = 1.0f;
		if ( ( m_nButtons & IN_JUMP ) != 0 )
		{
			VectorCopy( myEyeAngles, eyeAngles );
			engine->SetViewAngles( eyeAngles );
			m_flCoachLookAroundLerpTime = kLerpTime;
			m_angCoachLookAroundEyeAngles = myEyeAngles;
		}
		else if ( m_flCoachLookAroundLerpTime > 0 )
		{
			m_flCoachLookAroundLerpTime -= gpGlobals->frametime;
			float flPercent = ( kLerpTime - m_flCoachLookAroundLerpTime / kLerpTime );
			eyeAngles = Lerp( flPercent, m_angCoachLookAroundEyeAngles, eyeAngles );
			engine->SetViewAngles( eyeAngles );
		}
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStudioHdr *C_TFPlayer::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	// Initialize the gibs.
	InitPlayerGibs();

	InitializePoseParams();

	// Init flexes, cancel any scenes we're playing
	ClearSceneEvents( NULL, false );

	// Reset the flex weights.
	ResetFlexWeights( hdr );

	// Reset the players animation states, gestures
	if ( m_PlayerAnimState )
	{
		m_PlayerAnimState->OnNewModel();
	}

	if ( hdr )
	{
		InitPhonemeMappings();
	}

	if ( IsPlayerClass( TF_CLASS_SPY ) )
	{
		m_iSpyMaskBodygroup = FindBodygroupByName( "spyMask" );
	}
	else
	{
		m_iSpyMaskBodygroup = -1;
	}

	return hdr;
}

//-----------------------------------------------------------------------------
// Purpose: Is this player an enemy to the local player
//-----------------------------------------------------------------------------
bool C_TFPlayer::IsEnemyPlayer( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return false;

	int iTeam = pLocalPlayer->GetTeamNumber();

	switch( iTeam )
	{
	case TF_TEAM_RED:
		return ( GetTeamNumber() == TF_TEAM_BLUE );
	
	case TF_TEAM_BLUE:
		return ( GetTeamNumber() == TF_TEAM_RED );

	default:
		break;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Displays a nemesis icon on this player to the local player
//-----------------------------------------------------------------------------
void C_TFPlayer::ShowNemesisIcon( bool bShow )
{
	if ( bShow )
	{
		const char *pszEffect = NULL;
		switch ( GetTeamNumber() )
		{
		case TF_TEAM_RED:
			pszEffect = "particle_nemesis_red";
			break;
		case TF_TEAM_BLUE:
			pszEffect = "particle_nemesis_blue";
			break;
		default:
			return;	// shouldn't get called if we're not on a team; bail out if it does
		}
		AddOverheadEffect( pszEffect );
	}
	else
	{
		// stop effects for both team colors (to make sure we remove effects in event of team change)
		RemoveOverheadEffect( "particle_nemesis_red", true );
		RemoveOverheadEffect( "particle_nemesis_blue", true );
	}
	m_bIsDisplayingNemesisIcon = bShow;
}

//-----------------------------------------------------------------------------
// Purpose: Displays an icon denoting this player as the Birthday Player to the local player
//-----------------------------------------------------------------------------
void C_TFPlayer::ShowBirthdayEffect( bool bShow )
{
/*
	if ( bShow )
	{
		ParticleProp()->Create( "birthday_player_circling", PATTACH_POINT_FOLLOW, "head" );
		DispatchParticleEffect( "bday_confetti", GetAbsOrigin() + Vector(0,0,32), vec3_angle );
	}
	else
	{
		ParticleProp()->StopParticlesNamed( "birthday_player_circling", true );
	}
*/
	m_bShouldShowBirthdayEffect = bShow;
}


#define	TF_TAUNT_PITCH	0
#define TF_TAUNT_YAW	1
#define TF_TAUNT_DIST	2

#define TF_TAUNT_MAXYAW		135
#define TF_TAUNT_MINYAW		-135
#define TF_TAUNT_MAXPITCH	90
#define TF_TAUNT_MINPITCH	0
#define TF_TAUNT_IDEALLAG	4.0f

static Vector TF_TAUNTCAM_HULL_MIN( -9.0f, -9.0f, -9.0f );
static Vector TF_TAUNTCAM_HULL_MAX( 9.0f, 9.0f, 9.0f );

static ConVar tf_tauntcam_yaw( "tf_tauntcam_yaw", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
static ConVar tf_tauntcam_pitch( "tf_tauntcam_pitch", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
static ConVar tf_tauntcam_dist( "tf_tauntcam_dist", "150", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
static ConVar tf_tauntcam_speed( "tf_tauntcam_speed", "300", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

ConVar tf_halloween_kart_cam_dist( "tf_halloween_kart_cam_dist", "225", FCVAR_CHEAT );
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::TurnOnTauntCam( void )
{
	if ( !IsLocalPlayer() )
		return;

	if ( TFGameRules() && TFGameRules()->ShowMatchSummary() )
		return;

	m_flTauntCamTargetDist = ( m_flTauntCamTargetDist != 0.0f ) ? m_flTauntCamTargetDist : tf_tauntcam_dist.GetFloat();
	m_flTauntCamTargetDistUp = ( m_flTauntCamTargetDistUp != 0.0f ) ? m_flTauntCamTargetDistUp : 0.f;

	m_flTauntCamCurrentDist = 0.f;
	m_flTauntCamCurrentDistUp = 0.f;

	// Save the old view angles.
	engine->GetViewAngles( m_angTauntEngViewAngles );
	prediction->GetViewAngles( m_angTauntPredViewAngles );

	m_TauntCameraData.m_flPitch = 0;
	m_TauntCameraData.m_flYaw =  0;
	m_TauntCameraData.m_flDist = m_flTauntCamTargetDist;
	m_TauntCameraData.m_flLag = 1.f;
	m_TauntCameraData.m_vecHullMin.Init( -9.0f, -9.0f, -9.0f );
	m_TauntCameraData.m_vecHullMax.Init( 9.0f, 9.0f, 9.0f );

	if ( tf_taunt_first_person.GetBool() )
	{
		// Remain in first-person.
	}
	else
	{
		g_ThirdPersonManager.SetDesiredCameraOffset( Vector( 0, 0, 0 ) );
		g_ThirdPersonManager.SetOverridingThirdPerson( true );
	
		::input->CAM_ToThirdPerson();
		ThirdPersonSwitch( true );
	}

	m_bTauntInterpolating = true;

	if ( m_hItem )
	{
		m_hItem->UpdateVisibility();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::TurnOnTauntCam_Finish( void )
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::TurnOffTauntCam( void )
{
	if ( TFGameRules() && TFGameRules()->ShowMatchSummary() )
		return;

	// We want to interpolate back into the guy's head.
	if ( g_ThirdPersonManager.GetForcedThirdPerson() == false )
	{
		m_flTauntCamTargetDist = 0.f;
		m_TauntCameraData.m_flDist = m_flTauntCamTargetDist;
	}

	g_ThirdPersonManager.SetOverridingThirdPerson( false );

	if ( g_ThirdPersonManager.GetForcedThirdPerson() )
	{
		TurnOffTauntCam_Finish();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::TurnOffTauntCam_Finish( void )
{
	if ( !IsLocalPlayer() )
		return;	

	if ( TFGameRules() && TFGameRules()->ShowMatchSummary() )
		return;

	const Vector& vecOffset = g_ThirdPersonManager.GetCameraOffsetAngles();
	tf_tauntcam_pitch.SetValue( vecOffset[PITCH] - m_angTauntPredViewAngles[PITCH] );
	tf_tauntcam_yaw.SetValue( vecOffset[YAW] - m_angTauntPredViewAngles[YAW] );
	
	QAngle angles;

	angles[PITCH] = vecOffset[PITCH];
	angles[YAW] = vecOffset[YAW];
	angles[DIST] = vecOffset[DIST];

	if( g_ThirdPersonManager.WantToUseGameThirdPerson() == false )
	{
		::input->CAM_ToFirstPerson();
		ThirdPersonSwitch( false );
		angles = vec3_angle;
	}

	::input->CAM_SetCameraThirdData( NULL, angles );

	// Reset the old view angles.
//	engine->SetViewAngles( m_angTauntEngViewAngles );
//	prediction->SetViewAngles( m_angTauntPredViewAngles );

	// Force the feet to line up with the view direction post taunt.
	// If you are forcing aim yaw, your code is almost definitely broken if you don't include a delay between 
	// teleporting and forcing yaw. This is due to an unfortunate interaction between the command lookback window,
	// and the fact that m_flEyeYaw is never propogated from the server to the client.
	// TODO: Fix this after Halloween 2014.
	m_PlayerAnimState->m_bForceAimYaw = true;

	m_bTauntInterpolating = false;

	if ( GetViewModel() )
	{
		GetViewModel()->UpdateVisibility();
	}

	if ( m_hItem )
	{
		m_hItem->UpdateVisibility();
	}

	SetAppropriateCamera( this );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::HandleTaunting( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	// This code is only for the local player.
	Assert( pLocalPlayer == NULL || pLocalPlayer == this );

	// Clear the taunt slot.
	if (	!m_bWasTaunting &&
			(	
				m_Shared.InCond( TF_COND_TAUNTING ) ||
				m_nForceTauntCam
			)
		) 
	{
		m_bWasTaunting = true;

		// Handle the camera for the local player.
		if ( pLocalPlayer )
		{

			TurnOnTauntCam();
		}
	}

	if (	( !IsAlive() && m_nForceTauntCam < 2 ) || 
			(
				m_bWasTaunting && !m_Shared.InCond( TF_COND_TAUNTING ) &&
				!m_nForceTauntCam
			)
		)
	{
		m_bWasTaunting = false;

		// Clear the vcd slot.
		m_PlayerAnimState->ResetGestureSlot( GESTURE_SLOT_VCD );

		// Handle the camera for the local player.
		if ( pLocalPlayer )
		{
			TurnOffTauntCam();
		}
	}

	TauntCamInterpolation();
}

//-----------------------------------------------------------------------------
// Purpose: Handles third person camera interpolation directly 
// so we can manage enter & exit behavior without hacking the camera.
//-----------------------------------------------------------------------------
void C_TFPlayer::TauntCamInterpolation()
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( pLocalPlayer && m_bTauntInterpolating )
	{
		if ( m_flTauntCamCurrentDist != m_flTauntCamTargetDist )
		{
			m_flTauntCamCurrentDist += Sign( m_flTauntCamTargetDist - m_flTauntCamCurrentDist ) * gpGlobals->frametime * tf_tauntcam_speed.GetFloat();
			m_flTauntCamCurrentDist = clamp( m_flTauntCamCurrentDist, m_flTauntCamCurrentDist, m_flTauntCamTargetDist );
		}
		
		if ( m_flTauntCamCurrentDistUp != m_flTauntCamTargetDistUp )
		{
			m_flTauntCamCurrentDistUp += Sign( m_flTauntCamTargetDistUp - m_flTauntCamCurrentDistUp ) * gpGlobals->frametime * tf_tauntcam_speed.GetFloat();
			m_flTauntCamCurrentDistUp = clamp( m_flTauntCamCurrentDistUp, m_flTauntCamCurrentDistUp, m_flTauntCamTargetDistUp );
		}

		const Vector& vecCamOffset = g_ThirdPersonManager.GetCameraOffsetAngles();

		Vector vecOrigin = pLocalPlayer->GetLocalOrigin();
		vecOrigin += pLocalPlayer->GetViewOffset();

		Vector vecForward, vecUp;
		AngleVectors( QAngle( vecCamOffset[PITCH], vecCamOffset[YAW], 0 ), &vecForward, NULL, &vecUp );

		trace_t trace;
		UTIL_TraceHull( vecOrigin, vecOrigin - ( vecForward * m_flTauntCamCurrentDist ) + ( vecUp * m_flTauntCamCurrentDistUp ), Vector( -9.f, -9.f, -9.f ), 
			Vector( 9.f, 9.f, 9.f ), MASK_SOLID_BRUSHONLY, pLocalPlayer, COLLISION_GROUP_DEBRIS, &trace );

		if ( trace.fraction < 1.0 )
			m_flTauntCamCurrentDist *= trace.fraction;

		QAngle angCameraOffset = QAngle( vecCamOffset[PITCH], vecCamOffset[YAW], m_flTauntCamCurrentDist );
		::input->CAM_SetCameraThirdData( &m_TauntCameraData, angCameraOffset ); // Override camera distance interpolation.

		g_ThirdPersonManager.SetDesiredCameraOffset( Vector( m_flTauntCamCurrentDist, 0, m_flTauntCamCurrentDistUp ) );

		if ( m_flTauntCamCurrentDist == m_flTauntCamTargetDist && m_flTauntCamCurrentDistUp == m_flTauntCamTargetDistUp )
		{
			if ( m_flTauntCamTargetDist == 0.f )
				TurnOffTauntCam_Finish();
			else
				TurnOnTauntCam_Finish();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::PlayTauntSoundLoop( const char *pszSoundLoopName )
{
	if ( pszSoundLoopName && *pszSoundLoopName )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		CPASAttenuationFilter filter( this );
		m_pTauntSoundLoop = controller.SoundCreate( filter, entindex(), pszSoundLoopName );
		controller.Play( m_pTauntSoundLoop, 1.0, 100 );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::StopTauntSoundLoop()
{
	if ( m_pTauntSoundLoop )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		controller.SoundDestroy( m_pTauntSoundLoop );
		m_pTauntSoundLoop = NULL;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Indicates whether the spy's cigarette should be burning or not.
//-----------------------------------------------------------------------------
bool C_TFPlayer::CanLightCigarette( void )
{
	// Used to be a massive if-conditional.
	// Expanded for readability.
	if ( !IsPlayerClass( TF_CLASS_SPY ) )
		return false;

	if ( !IsAlive() )
		return false;

	// Don't light if we are disguised and an enemy (not the spy model).
	if ( m_Shared.InCond( TF_COND_DISGUISED ) && IsEnemyPlayer() )
	{
		if ( m_Shared.GetDisguiseClass() != TF_CLASS_SPY )
		{
			return false;
		}
	}

	// Don't light if we are invis.
	if ( GetPercentInvisible() > 0 )
		return false;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	// Don't light for the local player.
	if ( ( pLocalPlayer == this ) || !pLocalPlayer )
		return false;

	// Don't light if we're spectating in first person mode.
	if ( (pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE) && (pLocalPlayer->GetObserverTarget() == this) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_TFPlayer::ShouldPlayerDrawParticles( void )
{

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Check if passed in player is on the local player's friends list
//-----------------------------------------------------------------------------
bool C_TFPlayer::IsPlayerOnSteamFriendsList( C_BasePlayer *pPlayer )
{
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pLocalPlayer )
		return false;

	if ( !pPlayer )
		return false;

	if ( !steamapicontext->SteamFriends() || !steamapicontext->SteamUtils() )
		return false;

	player_info_t pi;
	if ( engine->GetPlayerInfo( pPlayer->entindex(), &pi ) && pi.friendsID )
	{
		CSteamID steamID( pi.friendsID, 1, GetUniverse(), k_EAccountTypeIndividual );
		if ( steamapicontext->SteamFriends()->HasFriend( steamID, k_EFriendFlagImmediate ) )
			return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::ClientThink()
{
	// Pass on through to the base class.
	BaseClass::ClientThink();

	UpdateIDTarget();

	UpdateLookAt();

	UpdateOverheadEffects();

	{
		if ( !IsTaunting() && m_PlayerAnimState->IsGestureSlotActive( GESTURE_SLOT_VCD ) )
		{
			m_PlayerAnimState->ResetGestureSlot( GESTURE_SLOT_VCD );
		}
	}

	// NVNT update state based effects (prior to healer clear)
	if ( haptics &&haptics->HasDevice() && IsLocalPlayer()) 
	{
		tfHaptics.HapticsThink(this);
	}

	// Clear our healer, it'll be reset by the medigun client think if we're being healed
	m_hHealer = NULL;

	// Start smoke if we're not invisible or disguised.
	if ( CanLightCigarette() )
	{
		if ( !m_bCigaretteSmokeActive )
		{
			int iSmokeAttachment = LookupAttachment( "cig_smoke" );
			ParticleProp()->Create( "cig_smoke", PATTACH_POINT_FOLLOW, iSmokeAttachment );
			m_bCigaretteSmokeActive = true;
		}
	}
	else	// stop the smoke otherwise if its active
	{
		if ( m_bCigaretteSmokeActive )
		{
			ParticleProp()->StopParticlesNamed( "cig_smoke", false );
			m_bCigaretteSmokeActive = false;
		}
	}

	if ( m_bWaterExitEffectActive && !IsAlive() )
	{
		ParticleProp()->StopParticlesNamed( "water_playeremerge", false );
		m_bWaterExitEffectActive = false;
	}

	// Kill the effect if either
	// a) the player is dead
	// b) the enemy disguised spy is now invisible

	if ( !IsAlive() ||
		( m_Shared.InCond( TF_COND_DISGUISED ) && IsEnemyPlayer() && ( GetPercentInvisible() > 0 ) ) )
	{
		StopSaveMeEffect( true );
	}

/*
	if ( m_LeaveServerTimer.HasStarted() && m_LeaveServerTimer.IsElapsed() )
	{
		engine->ExecuteClientCmd( "disconnect" );
	}
*/

	if ( m_Shared.IsEnteringOrExitingFullyInvisible() )
	{
		UpdateSpyStateChange();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::UpdateLookAt( void )
{
	bool bFoundViewTarget = false;

	Vector vForward;
	AngleVectors( GetLocalAngles(), &vForward );

	Vector vMyOrigin =  GetAbsOrigin();

	Vector vecLookAtTarget = vec3_origin;

	if ( tf_clientsideeye_lookats.GetBool() )
	{
		for( int iClient = 1; iClient <= gpGlobals->maxClients; ++iClient )
		{
			CBaseEntity *pEnt = UTIL_PlayerByIndex( iClient );
			if ( !pEnt || !pEnt->IsPlayer() )
				continue;

			if ( !pEnt->IsAlive() )
				continue;

			if ( pEnt == this )
				continue;

			Vector vDir = pEnt->GetAbsOrigin() - vMyOrigin;

			if ( vDir.Length() > 300 ) 
				continue;

			VectorNormalize( vDir );

			if ( DotProduct( vForward, vDir ) < 0.0f )
				continue;

			vecLookAtTarget = pEnt->EyePosition();
			bFoundViewTarget = true;
			break;
		}
	}

	if ( bFoundViewTarget == false )
	{
		// no target, look forward
		vecLookAtTarget = GetAbsOrigin() + vForward * 512;
	}

	// orient eyes
	m_viewtarget = vecLookAtTarget;

	/*
	// blinking
	if (m_blinkTimer.IsElapsed())
	{
		m_blinktoggle = !m_blinktoggle;
		m_blinkTimer.Start( RandomFloat( 1.5f, 4.0f ) );
	}
	*/

	/*
	// Figure out where we want to look in world space.
	QAngle desiredAngles;
	Vector to = vecLookAtTarget - EyePosition();
	VectorAngles( to, desiredAngles );

	// Figure out where our body is facing in world space.
	QAngle bodyAngles( 0, 0, 0 );
	bodyAngles[YAW] = GetLocalAngles()[YAW];

	float flBodyYawDiff = bodyAngles[YAW] - m_flLastBodyYaw;
	m_flLastBodyYaw = bodyAngles[YAW];

	// Set the head's yaw.
	float desired = AngleNormalize( desiredAngles[YAW] - bodyAngles[YAW] );
	desired = clamp( -desired, m_headYawMin, m_headYawMax );
	m_flCurrentHeadYaw = ApproachAngle( desired, m_flCurrentHeadYaw, 130 * gpGlobals->frametime );

	// Counterrotate the head from the body rotation so it doesn't rotate past its target.
	m_flCurrentHeadYaw = AngleNormalize( m_flCurrentHeadYaw - flBodyYawDiff );

	SetPoseParameter( m_headYawPoseParam, m_flCurrentHeadYaw );

	// Set the head's yaw.
	desired = AngleNormalize( desiredAngles[PITCH] );
	desired = clamp( desired, m_headPitchMin, m_headPitchMax );

	m_flCurrentHeadPitch = ApproachAngle( -desired, m_flCurrentHeadPitch, 130 * gpGlobals->frametime );
	m_flCurrentHeadPitch = AngleNormalize( m_flCurrentHeadPitch );
	SetPoseParameter( m_headPitchPoseParam, m_flCurrentHeadPitch );
	*/
}


//-----------------------------------------------------------------------------
// Purpose: Try to steer away from any players and objects we might interpenetrate
//-----------------------------------------------------------------------------
#define TF_AVOID_MAX_RADIUS_SQR		5184.0f			// Based on player extents and max buildable extents.
#define TF_OO_AVOID_MAX_RADIUS_SQR	0.00019f

ConVar tf_max_separation_force ( "tf_max_separation_force", "256", FCVAR_DEVELOPMENTONLY );

extern ConVar cl_forwardspeed;
extern ConVar cl_backspeed;
extern ConVar cl_sidespeed;

void C_TFPlayer::AvoidPlayers( CUserCmd *pCmd )
{
	// Turn off the avoid player code.
	if ( !tf_avoidteammates.GetBool() || !tf_avoidteammates_pushaway.GetBool() )
		return;

	// Don't test if the player doesn't exist or is dead.
	if ( IsAlive() == false )
		return;

	C_TFTeam *pTeam = (C_TFTeam*)GetTeam();
	if ( !pTeam )
		return;

	// Up vector.
	static Vector vecUp( 0.0f, 0.0f, 1.0f );

	Vector vecTFPlayerCenter = GetAbsOrigin();
	Vector vecTFPlayerMin = GetPlayerMins();
	Vector vecTFPlayerMax = GetPlayerMaxs();
	float flZHeight = vecTFPlayerMax.z - vecTFPlayerMin.z;
	vecTFPlayerCenter.z += 0.5f * flZHeight;
	VectorAdd( vecTFPlayerMin, vecTFPlayerCenter, vecTFPlayerMin );
	VectorAdd( vecTFPlayerMax, vecTFPlayerCenter, vecTFPlayerMax );

	// Find an intersecting player or object.
	int nAvoidPlayerCount = 0;
	C_TFPlayer *pAvoidPlayerList[MAX_PLAYERS_ARRAY_SAFE];

	C_TFPlayer *pIntersectPlayer = NULL;
	CBaseObject *pIntersectObject = NULL;
	float flAvoidRadius = 0.0f;

	Vector vecAvoidCenter, vecAvoidMin, vecAvoidMax;
	for ( int i = 0; i < pTeam->GetNumPlayers(); ++i )
	{
		C_TFPlayer *pAvoidPlayer = static_cast< C_TFPlayer * >( pTeam->GetPlayer( i ) );
		if ( pAvoidPlayer == NULL )
			continue;
		// Is the avoid player me?
		if ( pAvoidPlayer == this )
			continue;

		if ( !IsIndexIntoPlayerArrayValid(nAvoidPlayerCount) )
			break;

		// Save as list to check against for objects.
		pAvoidPlayerList[nAvoidPlayerCount] = pAvoidPlayer;
		++nAvoidPlayerCount;

		// Check to see if the avoid player is dormant.
		if ( pAvoidPlayer->IsDormant() )
			continue;

		// Is the avoid player solid?
		if ( pAvoidPlayer->IsSolidFlagSet( FSOLID_NOT_SOLID ) )
			continue;

		Vector t1, t2;

		vecAvoidCenter = pAvoidPlayer->GetAbsOrigin();
		vecAvoidMin = pAvoidPlayer->GetPlayerMins();
		vecAvoidMax = pAvoidPlayer->GetPlayerMaxs();
		flZHeight = vecAvoidMax.z - vecAvoidMin.z;
		vecAvoidCenter.z += 0.5f * flZHeight;
		VectorAdd( vecAvoidMin, vecAvoidCenter, vecAvoidMin );
		VectorAdd( vecAvoidMax, vecAvoidCenter, vecAvoidMax );

		if ( IsBoxIntersectingBox( vecTFPlayerMin, vecTFPlayerMax, vecAvoidMin, vecAvoidMax ) )
		{
			// Need to avoid this player.
			if ( !pIntersectPlayer )
			{
				pIntersectPlayer = pAvoidPlayer;
				break;
			}
		}
	}

	// We didn't find a player - look for objects to avoid.
	if ( !pIntersectPlayer )
	{
		for ( int iPlayer = 0; iPlayer < nAvoidPlayerCount; ++iPlayer )
		{	
			// Stop when we found an intersecting object.
			if ( pIntersectObject )
				break;

			for ( int iObject = 0; iObject < pTeam->GetNumObjects(); ++iObject )
			{
				CBaseObject *pAvoidObject = pTeam->GetObject( iObject );
				if ( !pAvoidObject )
					continue;

				// Check to see if the object is dormant.
				if ( pAvoidObject->IsDormant() )
					continue;

				// Is the object solid.
				if ( pAvoidObject->IsSolidFlagSet( FSOLID_NOT_SOLID ) )
					continue;

				// If we shouldn't avoid it, see if we intersect it.
				if ( pAvoidObject->ShouldPlayersAvoid() )
				{
					vecAvoidCenter = pAvoidObject->WorldSpaceCenter();
					vecAvoidMin = pAvoidObject->WorldAlignMins();
					vecAvoidMax = pAvoidObject->WorldAlignMaxs();
					VectorAdd( vecAvoidMin, vecAvoidCenter, vecAvoidMin );
					VectorAdd( vecAvoidMax, vecAvoidCenter, vecAvoidMax );

					if ( IsBoxIntersectingBox( vecTFPlayerMin, vecTFPlayerMax, vecAvoidMin, vecAvoidMax ) )
					{
						// Need to avoid this object.
						pIntersectObject = pAvoidObject;
						break;
					}
				}
			}
		}
	}

	// Anything to avoid?
	if ( !pIntersectPlayer && !pIntersectObject )
	{
		m_Shared.SetSeparation( false );
		m_Shared.SetSeparationVelocity( vec3_origin );
		return;
	}

	// Calculate the push strength and direction.
	Vector vecDelta;

	// Avoid a player - they have precedence.
	if ( pIntersectPlayer )
	{
		VectorSubtract( pIntersectPlayer->WorldSpaceCenter(), vecTFPlayerCenter, vecDelta );

		Vector vRad = pIntersectPlayer->WorldAlignMaxs() - pIntersectPlayer->WorldAlignMins();
		vRad.z = 0;

		flAvoidRadius = vRad.Length();
	}
	// Avoid a object.
	else
	{
		VectorSubtract( pIntersectObject->WorldSpaceCenter(), vecTFPlayerCenter, vecDelta );

		Vector vRad = pIntersectObject->WorldAlignMaxs() - pIntersectObject->WorldAlignMins();
		vRad.z = 0;

		flAvoidRadius = vRad.Length();
	}

	float flPushStrength = RemapValClamped( vecDelta.Length(), flAvoidRadius, 0, 0, tf_max_separation_force.GetInt() ); //flPushScale;

	//Msg( "PushScale = %f\n", flPushStrength );

	// Check to see if we have enough push strength to make a difference.
	if ( flPushStrength < 0.01f )
		return;

	Vector vecPush;
	if ( GetAbsVelocity().Length2DSqr() > 0.1f )
	{
		Vector vecVelocity = GetAbsVelocity();
		vecVelocity.z = 0.0f;
		CrossProduct( vecUp, vecVelocity, vecPush );
		VectorNormalize( vecPush );
	}
	else
	{
		// We are not moving, but we're still intersecting.
		QAngle angView = pCmd->viewangles;
		angView.x = 0.0f;
		AngleVectors( angView, NULL, &vecPush, NULL );
	}

	// Move away from the other player/object.
	Vector vecSeparationVelocity;
	if ( vecDelta.Dot( vecPush ) < 0 )
	{
		vecSeparationVelocity = vecPush * flPushStrength;
	}
	else
	{
		vecSeparationVelocity = vecPush * -flPushStrength;
	}

	// Don't allow the max push speed to be greater than the max player speed.
	float flMaxPlayerSpeed = MaxSpeed();
	float flCropFraction = 1.33333333f;

	if ( ( GetFlags() & FL_DUCKING ) && ( GetGroundEntity() != NULL ) )
	{	
		flMaxPlayerSpeed *= flCropFraction;
	}	

	float flMaxPlayerSpeedSqr = flMaxPlayerSpeed * flMaxPlayerSpeed;

	if ( vecSeparationVelocity.LengthSqr() > flMaxPlayerSpeedSqr )
	{
		vecSeparationVelocity.NormalizeInPlace();
		VectorScale( vecSeparationVelocity, flMaxPlayerSpeed, vecSeparationVelocity );
	}

	QAngle vAngles = pCmd->viewangles;
	vAngles.x = 0;
	Vector currentdir;
	Vector rightdir;

	AngleVectors( vAngles, &currentdir, &rightdir, NULL );

	Vector vDirection = vecSeparationVelocity;

	VectorNormalize( vDirection );

	float fwd = currentdir.Dot( vDirection );
	float rt = rightdir.Dot( vDirection );

	float forward = fwd * flPushStrength;
	float side = rt * flPushStrength;

	//Msg( "fwd: %f - rt: %f - forward: %f - side: %f\n", fwd, rt, forward, side );

	m_Shared.SetSeparation( true );
	m_Shared.SetSeparationVelocity( vecSeparationVelocity );

	pCmd->forwardmove	+= forward;
	pCmd->sidemove		+= side;

	// Clamp the move to within legal limits, preserving direction. This is a little
	// complicated because we have different limits for forward, back, and side

	//Msg( "PRECLAMP: forwardmove=%f, sidemove=%f\n", pCmd->forwardmove, pCmd->sidemove );

	float flForwardScale = 1.0f;
	if ( pCmd->forwardmove > fabs( cl_forwardspeed.GetFloat() ) )
	{
		flForwardScale = fabs( cl_forwardspeed.GetFloat() ) / pCmd->forwardmove;
	}
	else if ( pCmd->forwardmove < -fabs( cl_backspeed.GetFloat() ) )
	{
		flForwardScale = fabs( cl_backspeed.GetFloat() ) / fabs( pCmd->forwardmove );
	}

	float flSideScale = 1.0f;
	if ( fabs( pCmd->sidemove ) > fabs( cl_sidespeed.GetFloat() ) )
	{
		flSideScale = fabs( cl_sidespeed.GetFloat() ) / fabs( pCmd->sidemove );
	}

	float flScale = MIN( flForwardScale, flSideScale );
	pCmd->forwardmove *= flScale;
	pCmd->sidemove *= flScale;

	//Msg( "Pforwardmove=%f, sidemove=%f\n", pCmd->forwardmove, pCmd->sidemove );
}



//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flInputSampleTime - 
//			*pCmd - 
//-----------------------------------------------------------------------------
bool C_TFPlayer::CreateMove( float flInputSampleTime, CUserCmd *pCmd )
{
	static QAngle angMoveAngle( 0.0f, 0.0f, 0.0f );
	static float flTauntTurnSpeed = 0.f;
	
	bool bNoTaunt = true;
	bool bInTaunt = m_Shared.InCond( TF_COND_TAUNTING );

	if ( bInTaunt )
	{
		if ( tf_allow_taunt_switch.GetInt() <= 1 )
		{
			pCmd->weaponselect = 0;
		}

		int nCurrentButtons = pCmd->buttons;
		pCmd->buttons = 0;

		if ( !CanMoveDuringTaunt() )
		{
			pCmd->forwardmove = 0.0f;
			pCmd->sidemove = 0.0f;
			pCmd->upmove = 0.0f;

			VectorCopy( angMoveAngle, pCmd->viewangles );
		}
		else
		{
			float flSmoothTurnSpeed = 0.f;

			// only let these button through
			if ( pCmd->sidemove < 0 )
			{
				angMoveAngle += QAngle( 0.f, flSmoothTurnSpeed * flInputSampleTime, 0.f );
			}
			else if( pCmd->sidemove > 0 )
			{
				angMoveAngle += QAngle( 0.f, -flSmoothTurnSpeed * flInputSampleTime, 0.f );
			}
			pCmd->buttons = nCurrentButtons & ( IN_MOVELEFT | IN_MOVERIGHT | IN_FORWARD | IN_BACK );
			pCmd->sidemove = 0.0f;

			VectorCopy( angMoveAngle, pCmd->viewangles );
		}

		bNoTaunt = false;
	}
	else
	{
		flTauntTurnSpeed = 0.f;
		VectorCopy( pCmd->viewangles, angMoveAngle );
	}

	BaseClass::CreateMove( flInputSampleTime, pCmd );

	// Don't avoid players if in the middle of a high five. This prevents high-fivers from becoming separated.
	if ( !bInTaunt )
	{
		AvoidPlayers( pCmd );
	}

	return bNoTaunt;
}

//-----------------------------------------------------------------------------
// Purpose: This prevents some anims from being thrown out when the client is in prediction simulation.
// Stun anims, for example, (additive gestures) are synchronized by time and can keep playing on the client
// to prevent pauses during the stun loop.
//-----------------------------------------------------------------------------
bool C_TFPlayer::PlayAnimEventInPrediction( PlayerAnimEvent_t event )
{
	if ( !cl_predict->GetBool() )
		return true;

	switch ( event )
	{
	case PLAYERANIMEVENT_STUN_BEGIN:
	case PLAYERANIMEVENT_STUN_MIDDLE:
	case PLAYERANIMEVENT_STUN_END:
	case PLAYERANIMEVENT_PASSTIME_THROW_BEGIN:
	case PLAYERANIMEVENT_PASSTIME_THROW_MIDDLE:
	case PLAYERANIMEVENT_PASSTIME_THROW_END:
	case PLAYERANIMEVENT_CYOAPDA_BEGIN:
	case PLAYERANIMEVENT_CYOAPDA_MIDDLE:
	case PLAYERANIMEVENT_CYOAPDA_END:
		return true;
	default:
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	if ( IsLocalPlayer() )
	{
		if ( !prediction->IsFirstTimePredicted() && !PlayAnimEventInPrediction( event ) )
			return;
	}

	MDLCACHE_CRITICAL_SECTION();
	m_PlayerAnimState->DoAnimationEvent( event, nData );
}

//-----------------------------------------------------------------------------
// Purpose: Similar to OnNewModel. only reset animation related data
//-----------------------------------------------------------------------------
void C_TFPlayer::CleanUpAnimationOnSpawn()
{
	CStudioHdr *hdr = GetModelPtr();
	if ( !hdr )
		return;

	InitializePoseParams();

	// Init flexes, cancel any scenes we're playing
	ClearSceneEvents( NULL, false );

	// Reset the flex weights.
	ResetFlexWeights( GetModelPtr() );

	// Reset the players animation states, gestures
	if ( m_PlayerAnimState )
	{
		m_PlayerAnimState->ClearAnimationState();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
Vector C_TFPlayer::GetObserverCamOrigin( void )
{
	if ( !IsAlive() )
	{
		if ( m_hFirstGib )
		{
			IPhysicsObject *pPhysicsObject = m_hFirstGib->VPhysicsGetObject();
			if( pPhysicsObject )
			{
				Vector vecMassCenter = pPhysicsObject->GetMassCenterLocalSpace();
				Vector vecWorld;
				m_hFirstGib->CollisionProp()->CollisionToWorldSpace( vecMassCenter, &vecWorld );
				return (vecWorld);
			}
			return m_hFirstGib->GetRenderOrigin();
		}

		return GetDeathViewPosition();
	}

	return BaseClass::GetObserverCamOrigin();	
}

//-----------------------------------------------------------------------------
// Purpose: Consider the viewer and other factors when determining resulting
// invisibility
//-----------------------------------------------------------------------------
float C_TFPlayer::GetEffectiveInvisibilityLevel( void )
{
	float flPercentInvisible = GetPercentInvisible();

	// Crude way to limit Halloween spell
	bool bLimitedInvis = !IsEnemyPlayer();


	// If this is a teammate of the local player or viewer is observer,
	// dont go above a certain max invis
	if ( bLimitedInvis )
	{
		float flMax = tf_teammate_max_invis.GetFloat();
		if ( flPercentInvisible > flMax )
		{
			flPercentInvisible = flMax;
		}
	}
	else
	{
		// If this player just killed me, show them slightly
		// less than full invis in the deathcam and freezecam

		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

		if ( pLocalPlayer )
		{
			int iObserverMode = pLocalPlayer->GetObserverMode();

			if ( ( iObserverMode == OBS_MODE_FREEZECAM || iObserverMode == OBS_MODE_DEATHCAM ) && 
				pLocalPlayer->GetObserverTarget() == this )
			{
				float flMax = tf_teammate_max_invis.GetFloat();
				if ( flPercentInvisible > flMax )
				{
					flPercentInvisible = flMax;
				}
			}
		}
	}

	return flPercentInvisible;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::SetBodygroupsDirty( void )
{
	m_bBodygroupsDirty = true;

	CTFViewModel *pVM = dynamic_cast<CTFViewModel *>( GetViewModel() );
	if ( pVM )
	{
		pVM->m_bBodygroupsDirty = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::RecalcBodygroupsIfDirty( void )
{
	if ( m_bBodygroupsDirty )
	{
		m_Shared.RecalculatePlayerBodygroups();
		m_bBodygroupsDirty = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TFPlayer::DrawModel( int flags )
{
	// If we're a dead player with a fresh ragdoll, don't draw
	if ( m_nRenderFX == kRenderFxRagdoll )
		return 0;

	RecalcBodygroupsIfDirty();

	CMatRenderContextPtr pRenderContext( materials );
	bool bDoEffect = false;

	float flAmountToChop = 0.0;
	if ( m_Shared.InCond( TF_COND_DISGUISING ) )
	{
		flAmountToChop = ( gpGlobals->curtime - m_flDisguiseEffectStartTime ) *
			( 1.0 / TF_TIME_TO_DISGUISE );
	}
	else
	{
		if ( m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			float flETime = gpGlobals->curtime - m_flDisguiseEffectStartTime;
			if ( ( flETime > 0.0 ) && ( flETime < TF_TIME_TO_SHOW_DISGUISED_FINISHED_EFFECT ) )
			{
				flAmountToChop = 1.0 - ( flETime * ( 1.0/TF_TIME_TO_SHOW_DISGUISED_FINISHED_EFFECT ) );
			}
		}
	}

	bDoEffect = ( flAmountToChop > 0.0 ) && ( ! IsLocalPlayer() );
#if ( SHOW_DISGUISE_EFFECT == 0  )
	bDoEffect = false;
#endif
	bDoEffect = false;
	if ( bDoEffect )
	{
		Vector vMyOrigin =  GetAbsOrigin();
		BoxDeformation_t mybox;
		mybox.m_ClampMins = vMyOrigin - Vector(100,100,100);
		mybox.m_ClampMaxes = vMyOrigin + Vector(500,500,72 * ( 1 - flAmountToChop ) );
		pRenderContext->PushDeformation( &mybox );
	}

	int ret = BaseClass::DrawModel( flags );

	if ( bDoEffect )
		pRenderContext->PopDeformation();
	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::ProcessMuzzleFlashEvent()
{
	CBasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();

	// Reenable when the weapons have muzzle flash attachments in the right spot.
	bool bInToolRecordingMode = ToolsEnabled() && clienttools->IsInRecordingMode();
	if ( this == pLocalPlayer && !bInToolRecordingMode )
		return; // don't show own world muzzle flash for localplayer

	if ( pLocalPlayer && pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE )
	{
		// also don't show in 1st person spec mode
		if ( pLocalPlayer->GetObserverTarget() == this )
			return;
	}

	C_TFWeaponBase *pWeapon = m_Shared.GetActiveTFWeapon();
	if ( !pWeapon )
		return;

	pWeapon->ProcessMuzzleFlashEvent();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TFPlayer::GetIDTarget() const
{
	return m_iIDEntIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::SetForcedIDTarget( int iTarget )
{
	m_iForcedIDTarget = iTarget;
}

//-----------------------------------------------------------------------------
// Purpose: Update this client's targetid entity
//-----------------------------------------------------------------------------
void C_TFPlayer::UpdateIDTarget()
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer || !IsLocalPlayer() )
		return;

	// don't show IDs if mp_fadetoblack is on
	if ( GetTeamNumber() > TEAM_SPECTATOR && mp_fadetoblack.GetBool() && !IsAlive() )
	{
		m_iIDEntIndex = 0;
		return;
	}

	if ( m_iForcedIDTarget )
	{
		m_iIDEntIndex = m_iForcedIDTarget;
		return;
	}

	// If we're in deathcam, ID our killer
	if ( (GetObserverMode() == OBS_MODE_DEATHCAM || GetObserverMode() == OBS_MODE_CHASE) && GetObserverTarget() && GetObserverTarget() != GetLocalTFPlayer() )
	{
		m_iIDEntIndex = GetObserverTarget()->entindex();
		return;
	}

	// Clear old target and find a new one
	m_iIDEntIndex = 0;

	trace_t tr;
	Vector vecStart, vecEnd;
	VectorMA( MainViewOrigin(), MAX_TRACE_LENGTH, MainViewForward(), vecEnd );
	VectorMA( MainViewOrigin(), 10,   MainViewForward(), vecStart );

	// If we're in observer mode, ignore our observer target. Otherwise, ignore ourselves.
	if ( IsObserver() )
	{
		UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, GetObserverTarget(), COLLISION_GROUP_NONE, &tr );
	}
	else
	{
		int nMask = MASK_SOLID | CONTENTS_DEBRIS;
		UTIL_TraceLine( vecStart, vecEnd, nMask, this, COLLISION_GROUP_NONE, &tr );
	}

	bool bIsEnemyPlayer = false;

	if ( tr.m_pEnt && tr.m_pEnt->IsPlayer() )
	{
		// It's okay to start solid against enemies because we sometimes press right against them
		bIsEnemyPlayer = GetTeamNumber() != tr.m_pEnt->GetTeamNumber();
	}

	if ( ( !tr.startsolid || bIsEnemyPlayer ) && tr.DidHitNonWorldEntity() )
	{
		C_BaseEntity *pEntity = tr.m_pEnt;

		if ( pEntity && ( pEntity != this ) )
		{
			m_iIDEntIndex = pEntity->entindex();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Display appropriate hints for the target we're looking at
//-----------------------------------------------------------------------------
void C_TFPlayer::DisplaysHintsForTarget( C_BaseEntity *pTarget )
{
	// If the entity provides hints, ask them if they have one for this player
	ITargetIDProvidesHint *pHintInterface = dynamic_cast<ITargetIDProvidesHint*>(pTarget);
	if ( pHintInterface )
	{
		pHintInterface->DisplayHintTo( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TFPlayer::GetRenderTeamNumber( void )
{
	return m_nSkin;
}

static Vector WALL_MIN(-WALL_OFFSET,-WALL_OFFSET,-WALL_OFFSET);
static Vector WALL_MAX(WALL_OFFSET,WALL_OFFSET,WALL_OFFSET);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector C_TFPlayer::GetDeathViewPosition()
{
	Vector origin = EyePosition();

	C_TFRagdoll *pRagdoll = static_cast<C_TFRagdoll*>( m_hRagdoll.Get() );
	if ( pRagdoll )
	{
		if ( pRagdoll->IsDeathAnim() )
		{
			origin.z += VEC_DEAD_VIEWHEIGHT_SCALED( this ).z*4;
		}
		else
		{
			IRagdoll *pIRagdoll = GetRepresentativeRagdoll();
			if ( pIRagdoll )
			{
				origin = pIRagdoll->GetRagdollOrigin();
				origin.z += VEC_DEAD_VIEWHEIGHT_SCALED( this ).z; // look over ragdoll, not through
			}
		}
	}

	return origin;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::CalcDeathCamView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov)
{
	CBaseEntity	* killer = GetObserverTarget();
	C_BaseAnimating *pKillerAnimating = killer ? killer->GetBaseAnimating() : NULL;

	// Swing to face our killer within half the death anim time
	float interpolation = ( gpGlobals->curtime - m_flDeathTime ) / (TF_DEATH_ANIMATION_TIME * 0.5);
	interpolation = clamp( interpolation, 0.0f, 1.0f );
	interpolation = SimpleSpline( interpolation );

	float flMinChaseDistance = CHASE_CAM_DISTANCE_MIN;
	float flMaxChaseDistance = CHASE_CAM_DISTANCE_MAX;

	if ( pKillerAnimating )
	{
		float flScaleSquared = pKillerAnimating->GetModelScale() * pKillerAnimating->GetModelScale();
		flMinChaseDistance *= flScaleSquared;
		flMaxChaseDistance *= flScaleSquared;
	}

	m_flObserverChaseDistance += gpGlobals->frametime * 48.0f;
	m_flObserverChaseDistance = clamp( m_flObserverChaseDistance, flMinChaseDistance, flMaxChaseDistance );

	QAngle aForward = eyeAngles = EyeAngles();
	Vector origin = GetDeathViewPosition();

	if ( m_hHeadGib )
	{
		// View from our decapitated head.
		IPhysicsObject *pPhysicsObject = m_hHeadGib->VPhysicsGetObject();
		if( pPhysicsObject )
		{
			Vector vecMassCenter = pPhysicsObject->GetMassCenterLocalSpace();
			Vector vecWorld;
			m_hHeadGib->CollisionProp()->CollisionToWorldSpace( vecMassCenter, &vecWorld );
			m_hHeadGib->AddEffects( EF_NODRAW );

			eyeOrigin = vecWorld + Vector(0,0,6);

			QAngle aHead = m_hHeadGib->GetAbsAngles();
			Vector vBody;
			if ( m_hRagdoll )
			{
				// Turn to face our ragdoll.
				vBody = m_hRagdoll->GetAbsOrigin() - eyeOrigin;
			}
			else
			{
				vBody = m_hHeadGib->GetAbsOrigin();
			}
			QAngle aBody; VectorAngles( vBody, aBody );
			InterpolateAngles( aHead, aBody, eyeAngles, interpolation );
			return;
		}
	}

	if ( killer && (killer != this) ) 
	{
		Vector vKiller = killer->EyePosition() - origin;
		QAngle aKiller;
		VectorAngles( vKiller, aKiller );
		InterpolateAngles( aForward, aKiller, eyeAngles, interpolation );
	}

	Vector vForward;
	AngleVectors( eyeAngles, &vForward );

	VectorNormalize( vForward );

	VectorMA( origin, -m_flObserverChaseDistance, vForward, eyeOrigin );

	trace_t trace; // clip against world
	C_BaseEntity::PushEnableAbsRecomputations( false ); // HACK don't recompute positions while doing RayTrace
	UTIL_TraceHull( origin, eyeOrigin, WALL_MIN, WALL_MAX, MASK_SOLID, this, COLLISION_GROUP_NONE, &trace );
	C_BaseEntity::PopEnableAbsRecomputations();

	if (trace.fraction < 1.0)
	{
		eyeOrigin = trace.endpos;
		m_flObserverChaseDistance = VectorLength(origin - eyeOrigin);
	}

	fov = GetFOV();
}

//-----------------------------------------------------------------------------
// Purpose: Do nothing multiplayer_animstate takes care of animation.
// Input  : playerAnim - 
//-----------------------------------------------------------------------------
void C_TFPlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
	return;
}

float C_TFPlayer::GetMinFOV() const
{
	// Min FOV for Sniper Rifle
	return 20;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const QAngle& C_TFPlayer::EyeAngles()
{
	if ( IsLocalPlayer() && g_nKillCamMode == OBS_MODE_NONE )
	{
		return BaseClass::EyeAngles();
	}
	else
	{
		return m_angEyeAngles;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &color - 
//-----------------------------------------------------------------------------
void C_TFPlayer::GetTeamColor( Color &color )
{
	color[3] = 255;

	if ( GetTeamNumber() == TF_TEAM_RED )
	{
		color[0] = 159;
		color[1] = 55;
		color[2] = 34;
	}
	else if ( GetTeamNumber() == TF_TEAM_BLUE )
	{
		color[0] = 76;
		color[1] = 109;
		color[2] = 129;
	}
	else
	{
		color[0] = 255;
		color[1] = 255;
		color[2] = 255;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bCopyEntity - 
// Output : C_BaseAnimating *
//-----------------------------------------------------------------------------
C_BaseAnimating *C_TFPlayer::BecomeRagdollOnClient()
{
	// Let the C_TFRagdoll take care of this.
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : IRagdoll*
//-----------------------------------------------------------------------------
IRagdoll* C_TFPlayer::GetRepresentativeRagdoll() const
{
	if ( m_hRagdoll.Get() )
	{
		C_TFRagdoll *pRagdoll = static_cast<C_TFRagdoll*>( m_hRagdoll.Get() );
		if ( !pRagdoll )
			return NULL;

		return pRagdoll->GetIRagdoll();
	}
	else
	{
		return NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::InitPlayerGibs( void )
{
	// Clear out the gib list and create a new one.
	m_aGibs.Purge();
	m_aNormalGibs.PurgeAndDeleteElements();
	m_aSillyGibs.Purge();

	int nModelIndex = GetPlayerClass()->HasCustomModel() ? modelinfo->GetModelIndex( GetPlayerClass()->GetModelName() ) : GetModelIndex();
	BuildGibList( m_aGibs, nModelIndex, 1.0f, COLLISION_GROUP_NONE );

	if ( TFGameRules() && TFGameRules()->IsBirthday() )
	{
		for ( int i = 0; i < m_aGibs.Count(); i++ )
		{
			if ( RandomFloat(0,1) < 0.75 )
			{
				V_strcpy_safe( m_aGibs[i].modelName, g_pszBDayGibs[ RandomInt(0,ARRAYSIZE(g_pszBDayGibs)-1) ] );
			}
		}
	}

	// Copy the normal gibs list to be saved for later when swapping with Pyro Vision
	FOR_EACH_VEC ( m_aGibs, i )
	{
		char *cloneStr = new char [ 512 ];
		Q_strncpy( cloneStr, m_aGibs[i].modelName, 512 );
		m_aNormalGibs.AddToTail( cloneStr );

		// Create a list of silly gibs
		int iRandIndex = RandomInt(4,ARRAYSIZE(g_pszBDayGibs)-1);
		m_aSillyGibs.AddToTail( iRandIndex );
	}
}

//-----------------------------------------------------------------------------
// Purpose : Checks vision flags and ensures the proper gib models are loaded for vision mode
//-----------------------------------------------------------------------------
void C_TFPlayer::CheckAndUpdateGibType( void )
{
	// check the first gib, if it's different copy them all over
	if ( ( TFGameRules() && TFGameRules()->UseSillyGibs() ) )
	{
		if ( Q_strcmp( m_aGibs[0].modelName, g_pszBDayGibs[ m_aSillyGibs[0] ]) != 0 )
		{
			FOR_EACH_VEC( m_aGibs, i )
			{
				V_strcpy_safe( m_aGibs[i].modelName, g_pszBDayGibs[ m_aSillyGibs[i] ] );
			}
		}
	}
	else 
	{
		if ( Q_strcmp( m_aGibs[0].modelName, m_aNormalGibs[0]) != 0 )
		{
			FOR_EACH_VEC( m_aGibs, i )
			{
				V_strcpy_safe( m_aGibs[i].modelName, m_aNormalGibs[i] );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecOrigin - 
//			&vecVelocity - 
//			&vecImpactVelocity - 
//-----------------------------------------------------------------------------
void C_TFPlayer::CreatePlayerGibs( const Vector &vecOrigin, const Vector &vecVelocity, float flImpactScale, bool bBurning, bool bWearableGibs, bool bOnlyHead, bool bDisguiseGibs )
{
	// Make sure we have Gibs to create.
	if ( m_aGibs.Count() == 0 )
		return;

	AngularImpulse angularImpulse( RandomFloat( 0.0f, 120.0f ), RandomFloat( 0.0f, 120.0f ), 0.0 );

	Vector vecBreakVelocity = vecVelocity;
	vecBreakVelocity.z += tf_playergib_forceup.GetFloat();
	VectorNormalize( vecBreakVelocity );
	vecBreakVelocity *= tf_playergib_force.GetFloat();

	// Cap the impulse.
	float flSpeed = vecBreakVelocity.Length();
	if ( flSpeed > tf_playergib_maxspeed.GetFloat() )
	{
		VectorScale( vecBreakVelocity, tf_playergib_maxspeed.GetFloat() / flSpeed, vecBreakVelocity );
	}

	breakablepropparams_t breakParams( vecOrigin, GetRenderAngles(), vecBreakVelocity, angularImpulse );
	breakParams.impactEnergyScale = 1.0f;//

	// Break up the player.
	if ( !bWearableGibs )
	{
		// Gib the player's body.
		m_hHeadGib = NULL;
		m_hSpawnedGibs.Purge();

		bool bHasCustomModel = GetPlayerClass()->HasCustomModel();
		int nModelIndex = bHasCustomModel ? modelinfo->GetModelIndex( GetPlayerClass()->GetModelName() ) : GetModelIndex();

		if ( bOnlyHead )
		{
			if ( UTIL_IsLowViolence() )
			{
				// No bloody gibs with pyro-vision goggles
				return;
			}

			// Create only a head gib.
			CUtlVector<breakmodel_t> headGib;
			int nClassIndex = GetPlayerClass()->GetClassIndex();
			if ( bHasCustomModel )
			{
				for ( int i=0; i<m_aGibs.Count(); ++i )
				{
					if ( Q_strcmp( m_aGibs[i].modelName, g_pszBotHeadGibs[nClassIndex] ) == 0 )
					{
						headGib.AddToHead( m_aGibs[i] );
					}
				}
			}
			else
			{
				for ( int i=0; i<m_aGibs.Count(); ++i )
				{
					if ( Q_strcmp( m_aGibs[i].modelName, g_pszHeadGibs[nClassIndex] ) == 0 )
					{
						headGib.AddToHead( m_aGibs[i] );
					}
				}
			}
			
			m_hFirstGib = CreateGibsFromList( headGib, nModelIndex, NULL, breakParams, this, -1 , false, true, &m_hSpawnedGibs, bBurning );
			m_hHeadGib = m_hFirstGib;
			if ( m_hFirstGib )
			{
				IPhysicsObject *pPhysicsObject = m_hFirstGib->VPhysicsGetObject();
				if( pPhysicsObject )
				{
					// Give the head some rotational damping so it doesn't roll so much (for the player's view).
					float damping, rotdamping;
					pPhysicsObject->GetDamping( &damping, &rotdamping );
					rotdamping *= 6.f;
					pPhysicsObject->SetDamping( &damping, &rotdamping );
				}
			}
		}
		else
		{
			CheckAndUpdateGibType();
			m_hFirstGib = CreateGibsFromList( m_aGibs, nModelIndex, NULL, breakParams, this, -1 , false, true, &m_hSpawnedGibs, bBurning );
		}
		DropPartyHat( breakParams, vecBreakVelocity );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::DropPartyHat( breakablepropparams_t &breakParams, Vector &vecBreakVelocity )
{
	// Turning off party hats because we've moving to real hats
	return;

/*
	if ( m_hPartyHat )
	{
		breakmodel_t breakModel;
		Q_strncpy( breakModel.modelName, BDAY_HAT_MODEL, sizeof(breakModel.modelName) );
		breakModel.health = 1;
		breakModel.fadeTime = RandomFloat(5,10);
		breakModel.fadeMinDist = 0.0f;
		breakModel.fadeMaxDist = 0.0f;
		breakModel.burstScale = breakParams.defBurstScale;
		breakModel.collisionGroup = COLLISION_GROUP_DEBRIS;
		breakModel.isRagdoll = false;
		breakModel.isMotionDisabled = false;
		breakModel.placementName[0] = 0;
		breakModel.placementIsBone = false;
		breakModel.offset = GetAbsOrigin() - m_hPartyHat->GetAbsOrigin();
		BreakModelCreateSingle( this, &breakModel, m_hPartyHat->GetAbsOrigin(), m_hPartyHat->GetAbsAngles(), vecBreakVelocity, breakParams.angularVelocity, m_hPartyHat->m_nSkin, breakParams );

		m_hPartyHat->Release();
	}
*/
}

//-----------------------------------------------------------------------------
// Purpose: How many buildables does this player own
//-----------------------------------------------------------------------------
int	C_TFPlayer::GetObjectCount( void )
{
	return m_aObjects.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Get a specific buildable that this player owns
//-----------------------------------------------------------------------------
C_BaseObject *C_TFPlayer::GetObject( int index_ )
{
	return m_aObjects[index_].Get();
}

//-----------------------------------------------------------------------------
// Purpose: Get a specific buildable that this player owns
//-----------------------------------------------------------------------------
C_BaseObject *C_TFPlayer::GetObjectOfType( int iObjectType, int iObjectMode ) const
{
	int iCount = m_aObjects.Count();

	for ( int i=0;i<iCount;i++ )
	{
		C_BaseObject *pObj = m_aObjects[i].Get();

		if ( !pObj )
			continue;

		if ( pObj->IsDormant() || pObj->IsMarkedForDeletion() )
			continue;

		if ( pObj->GetType() != iObjectType )
			continue;

		if ( pObj->GetObjectMode() != iObjectMode )
			continue;

		return pObj;
	}
	
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : collisionGroup - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_TFPlayer::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	if ( ( ( collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT ) && tf_avoidteammates.GetBool() ) ||
		collisionGroup == TFCOLLISION_GROUP_ROCKETS )
	{
		switch( GetTeamNumber() )
		{
		case TF_TEAM_RED:
			if ( !( contentsMask & CONTENTS_REDTEAM ) )
				return false;
			break;

		case TF_TEAM_BLUE:
			if ( !( contentsMask & CONTENTS_BLUETEAM ) )
				return false;
			break;
		}
	}
	return BaseClass::ShouldCollide( collisionGroup, contentsMask );
}

float C_TFPlayer::GetPercentInvisible( void )
{
	return m_Shared.GetPercentInvisible();
}

void C_TFPlayer::AdjustSkinIndexForZombie( int iClassIndex, int &iSkinIndex )
{
	// We only know how to adjust the 4 standard base skins (red/blue * normal/invuln)
	Assert( iSkinIndex >= 0 && iSkinIndex < 4 );

	if ( iClassIndex == TF_CLASS_SPY )
	{
		// Spy has a bunch of extra skins used to adjust the mask
		iSkinIndex += 22;
	}
	else
	{
		// 4: red zombie
		// 5: blue zombie
		// 6: red zombie invuln
		// 7: blue zombie invuln
		iSkinIndex += 4;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TFPlayer::GetSkin()
{
	C_TFPlayer *pLocalPlayer = GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return 0;

	int iVisibleTeam = GetTeamNumber();

	// if this player is disguised and on the other team, use disguise team
	if ( m_Shared.InCond( TF_COND_DISGUISED ) && IsEnemyPlayer() )
	{
		iVisibleTeam = m_Shared.GetDisguiseTeam();
	}

	int nSkin;

	switch( iVisibleTeam )
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

	// Assume we'll switch skins to show the spy mask
	bool bCheckSpyMask = true;

	// 3 and 4 are invulnerable
	if ( m_Shared.IsInvulnerable() )
	{
		nSkin += 2;
		bCheckSpyMask = false;
	}

	if ( bCheckSpyMask && m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		if ( !IsEnemyPlayer() )
		{
			nSkin += 4 + ( ( m_Shared.GetDisguiseClass() - TF_FIRST_NORMAL_CLASS ) * 2 );
		}
	}

	// Check for testing override

	return nSkin;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iClass - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_TFPlayer::IsPlayerClass( int iClass ) const
{
	const C_TFPlayerClass *pClass = GetPlayerClass();
	if ( !pClass )
		return false;

	return ( pClass->GetClassIndex() == iClass );
}

//-----------------------------------------------------------------------------
// Purpose: Don't take damage decals while stealthed
//-----------------------------------------------------------------------------
void C_TFPlayer::AddDecal( const Vector& rayStart, const Vector& rayEnd,
							const Vector& decalCenter, int hitbox, int decalIndex, bool doTrace, trace_t& tr, int maxLODToDecal )
{
	if ( m_Shared.IsStealthed() )
	{
		return;
	}

	if ( m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		return;
	}

	if ( m_Shared.IsInvulnerable() )
	{ 
		Vector vecDir = rayEnd - rayStart;
		VectorNormalize(vecDir);
		g_pEffects->Ricochet( rayEnd - (vecDir * 8), -vecDir );
		return;
	}

#ifdef TF_RAID_MODE
	// no decals for the BLUE team in Raid mode
	// (temp workaround for decals causing the glows to not draw correctly)
	if ( TFGameRules() && TFGameRules()->IsRaidMode() )
	{
		if ( GetTeamNumber() == TF_TEAM_BLUE )
		{
			return;
		}
	}
#endif // TF_RAID_MODE

	// don't decal from inside the player
	if ( tr.startsolid )
	{
		return;
	}

	BaseClass::AddDecal( rayStart, rayEnd, decalCenter, hitbox, decalIndex, doTrace, tr, maxLODToDecal );
}

//-----------------------------------------------------------------------------
// Called every time the player respawns
//-----------------------------------------------------------------------------
void C_TFPlayer::ClientPlayerRespawn( void )
{
	if ( IsLocalPlayer() )
	{
		// MCJOHN - For testing, dump out all of the textures whenever we respawn
		STAGING_ONLY_EXEC( engine->ClientCmd_Unrestricted( "mat_evict_all" ) );

		// Dod called these, not sure why
		//MoveToLastReceivedPosition( true );
		//ResetLatched();

		// Reset the camera.
		HandleTaunting();
		
		ResetToneMapping(1.0);

		// Release the duck toggle key
		KeyUp( &in_ducktoggle, NULL ); 

		IGameEvent *event = gameeventmanager->CreateEvent( "localplayer_respawn" );
		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}
		// NVNT revert tf haptics
		tfHaptics.Revert();

#if defined( REPLAY_ENABLED )
		if ( GetPlayerClass()->GetClassIndex() != TF_CLASS_UNDEFINED )
		{
			// Notify replay history manager that the local player has spawned.
			// NOTE: This won't do anything if replay isn't enabled, isn't recording, etc.
			g_pClientReplayContext->OnPlayerSpawn();
		}
#endif

		SetAppropriateCamera( this );
	}

	UpdateVisibility();

	DestroyBoneAttachments();

	m_hHeadGib = NULL;
	m_hFirstGib = NULL;
	m_hSpawnedGibs.Purge();

	m_fMetersRan = 0;

	SetShowHudMenuTauntSelection( false );

	CleanUpAnimationOnSpawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_TFPlayer::ShouldDraw()
{
	if ( IsLocalPlayer() )
	{
		if ( m_PlayerClass.HasCustomModel() && !m_PlayerClass.CustomModelIsVisibleToSelf() )
			return false;
	}

	if ( this == C_TFPlayer::GetLocalTFPlayer() )
	{
		if ( this->m_Shared.InCond( TF_COND_ZOOMED ) )
		{
			return false;
		}
	}

	return BaseClass::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::CreateSaveMeEffect( MedicCallerType nType /*= CALLER_TYPE_NORMAL*/ )
{
	// Don't create them for the local player in first-person view.
	if ( IsLocalPlayer() && InFirstPersonView() )
		return;

	if ( TFGameRules() && TFGameRules()->ShowMatchSummary() )
		return;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	// If I'm disguised as the enemy, play to all players
	if ( m_Shared.InCond( TF_COND_DISGUISED ) && m_Shared.GetDisguiseTeam() != GetTeamNumber() && !m_Shared.IsStealthed() )
	{
		// play to all players
	}
	else
	{
		// only play to teammates
		if ( pLocalPlayer && pLocalPlayer->GetTeamNumber() != GetTeamNumber() )
			return;
	}

	StopSaveMeEffect();

	float flHealth = float( GetHealth() ) / float( GetMaxHealth() );
	Vector vHealth;
	vHealth.x = flHealth;
	vHealth.y = flHealth;
	vHealth.z = flHealth;

	if ( nType == CALLER_TYPE_AUTO )
	{
		m_pSaveMeEffect = ParticleProp()->Create( "speech_mediccall_auto", PATTACH_POINT_FOLLOW, "head" );
		EmitSound( "Medic.AutoCallerAnnounce" );
	}
	else
	{
		m_pSaveMeEffect = ParticleProp()->Create( "speech_mediccall", PATTACH_POINT_FOLLOW, "head" );
	}

	if ( m_pSaveMeEffect )
	{
		m_pSaveMeEffect->SetControlPoint( 1, vHealth );
	}
	
	// If the local player is a medic, add this player to our list of medic callers
	if ( pLocalPlayer && pLocalPlayer->IsPlayerClass( TF_CLASS_MEDIC ) && pLocalPlayer->IsAlive() == true )
	{
		Vector vecPos;
		if ( GetAttachmentLocal( LookupAttachment( "head" ), vecPos ) )
		{
			vecPos += Vector(0,0,18);	// Particle effect is 18 units above the attachment
			CTFMedicCallerPanel::AddMedicCaller( this, 5.0, vecPos, nType );
		}
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "player_calledformedic" );
	if ( event )
	{
		event->SetInt( "userid", GetUserID() );
		gameeventmanager->FireEventClientSide( event );
	}

	m_flSaveMeExpireTime = gpGlobals->curtime + 5.0f;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::StopSaveMeEffect( bool bForceRemoveInstantly /*= false*/ )
{
	if ( m_pSaveMeEffect )
	{
		if ( bForceRemoveInstantly )
		{
			ParticleProp()->StopEmissionAndDestroyImmediately( m_pSaveMeEffect );
		}
		else
		{
			ParticleProp()->StopEmission( m_pSaveMeEffect );
		}
		
		m_pSaveMeEffect = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_TFPlayer::IsOverridingViewmodel( void )
{
	C_TFPlayer *pPlayer = this;
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer && pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE && 
		 pLocalPlayer->GetObserverTarget() && pLocalPlayer->GetObserverTarget()->IsPlayer() )
	{
		pPlayer = assert_cast<C_TFPlayer*>(pLocalPlayer->GetObserverTarget());
	}

	if ( pPlayer->m_Shared.IsInvulnerable() )
		return true;

	return BaseClass::IsOverridingViewmodel();
}

void C_TFPlayer::OverrideView( CViewSetup *pSetup )
{
	BaseClass::OverrideView( pSetup );

	TFPlayerClassData_t *pData = GetPlayerClass()->GetData();

	if ( pData  && g_ThirdPersonManager.WantToUseGameThirdPerson() )
	{
		Vector vecOffset = pData->m_vecThirdPersonOffset;

		g_ThirdPersonManager.SetDesiredCameraOffset( vecOffset );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw my viewmodel in some special way
//-----------------------------------------------------------------------------
int	C_TFPlayer::DrawOverriddenViewmodel( C_BaseViewModel *pViewmodel, int flags )
{
	int ret = 0;

	C_TFPlayer *pPlayer = this;
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer && pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE && 
		pLocalPlayer->GetObserverTarget() && pLocalPlayer->GetObserverTarget()->IsPlayer() )
	{
		pPlayer = assert_cast<C_TFPlayer*>(pLocalPlayer->GetObserverTarget());
	}

	if ( pPlayer->m_Shared.IsInvulnerable() )
	{
		if ( flags & STUDIO_RENDER )
		{
			// Force the invulnerable material
			modelrender->ForcedMaterialOverride( *pPlayer->GetInvulnMaterialRef() );
		}

		// We allow our weapon to then override this if it wants to.
		// This allows c_* weapons to draw themselves.
		C_BaseCombatWeapon *pWeapon = pViewmodel->GetOwningWeapon();
		if ( pWeapon && pWeapon->IsOverridingViewmodel() )
		{
			ret = pWeapon->DrawOverriddenViewmodel( pViewmodel, flags );
		}
		else
		{
			ret = pViewmodel->DrawOverriddenViewmodel( flags );
		}

		if ( flags & STUDIO_RENDER )
		{
			modelrender->ForcedMaterialOverride( NULL );
		}
	}

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::ApplyBoneMatrixTransform( matrix3x4_t& transform )
{
	BaseClass::ApplyBoneMatrixTransform ( transform );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void BuildBigHeadTransformations( CBaseAnimating *pObject, CStudioHdr *hdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed, float flScale )
{
	if ( !pObject || flScale == 1.f )
		return;

	// Scale the head.
	int iHeadBone = pObject->LookupBone( "bip_head" );
	if ( iHeadBone == -1 )
		return;

	matrix3x4_t  &transform = pObject->GetBoneForWrite( iHeadBone );
	Vector head_position;
	MatrixGetTranslation( transform, head_position );

	// Scale the head.
	MatrixScaleBy ( flScale, transform );

	const int cMaxNumHelms = 2;
	int iHelmIndex[cMaxNumHelms];
	iHelmIndex[0] = pObject->LookupBone( "prp_helmet" );
	iHelmIndex[1] = pObject->LookupBone( "prp_hat" );

	for ( int i = 0; i < cMaxNumHelms; i++ )
	{
		if ( iHelmIndex[i] != -1 )
		{
			matrix3x4_t &transformhelmet = pObject->GetBoneForWrite( iHelmIndex[i] );
			MatrixScaleBy ( flScale, transformhelmet );

			Vector helmet_position;
			MatrixGetTranslation( transformhelmet, helmet_position );
			Vector offset = helmet_position - head_position;
			MatrixSetTranslation ( ( offset * flScale ) + head_position, transformhelmet );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void BuildDecapitatedTransformations( CBaseAnimating *pObject, CStudioHdr *hdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed )
{
	if ( !pObject )
		return;

	// Scale the head to nothing.
	int iHeadBone = pObject->LookupBone( "bip_head" );
	if ( iHeadBone != -1 )
	{	
		matrix3x4_t  &transform = pObject->GetBoneForWrite( iHeadBone );
		MatrixScaleByZero ( transform );
	}

	int iHelm = pObject->LookupBone( "prp_helmet" );
	if ( iHelm != -1 )
	{
		// Scale the helmet.
		matrix3x4_t  &transformhelmet = pObject->GetBoneForWrite( iHelm );
		MatrixScaleByZero ( transformhelmet );
	}

	iHelm = pObject->LookupBone( "prp_hat" );
	if ( iHelm != -1 )
	{
		matrix3x4_t  &transformhelmet = pObject->GetBoneForWrite( iHelm );
		MatrixScaleByZero ( transformhelmet );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void BuildNeckScaleTransformations( CBaseAnimating *pObject, CStudioHdr *hdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed, float flScale, int iClass )
{
	if ( !pObject || flScale == 1.f )
		return;

	int iNeck = pObject->LookupBone( "bip_neck" );
	if ( iNeck == -1 )
		return;

	matrix3x4_t &neck_transform = pObject->GetBoneForWrite( iNeck );

	Vector spine_position, neck_position, head_position, position, offset(0, 0, 0);
	if ( iClass != TF_CLASS_HEAVYWEAPONS )
	{
		// Compress the neck into the spine.
		int iSpine = pObject->LookupBone( "bip_spine_3" );
		if ( iSpine != -1 )
		{
			matrix3x4_t &spine_transform = pObject->GetBoneForWrite( iSpine );
			MatrixPosition( spine_transform, spine_position );
			MatrixPosition( neck_transform, neck_position );
			position = flScale * ( neck_position - spine_position );
			MatrixSetTranslation( spine_position + position, neck_transform );
		}
	}

	if ( iClass == TF_CLASS_SPY )
	{
		int iCig = pObject->LookupBone( "prp_cig" );
		if ( iCig != -1 )
		{
			matrix3x4_t  &cig_transform = pObject->GetBoneForWrite( iCig );
			MatrixScaleByZero ( cig_transform );
		}
	}

	// Compress the head into the neck.
	int iHead = pObject->LookupBone( "bip_head" );
	if ( iHead != -1 )
	{
		matrix3x4_t  &head_transform = pObject->GetBoneForWrite( iHead );
		MatrixPosition( head_transform, head_position );
		MatrixPosition( neck_transform, neck_position );
		offset = head_position - neck_position;
		MatrixSetTranslation( neck_position, head_transform );
	}

	// Store helmet bone offset.
	int iHelm = pObject->LookupBone( "prp_helmet" );
	if ( iHelm != -1 ) 
	{
		matrix3x4_t  &helmet_transform = pObject->GetBoneForWrite( iHelm );
		MatrixPosition( helmet_transform, position );
		MatrixSetTranslation( position - offset, helmet_transform );
	}

	// Store alternate helmet bone offset.
	iHelm = pObject->LookupBone( "prp_hat" );
	if ( iHelm != -1 )
	{
		matrix3x4_t  &hat_transform = pObject->GetBoneForWrite( iHelm );
		MatrixPosition( hat_transform, position );
		MatrixSetTranslation( position - offset, hat_transform );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Get child bones from a specified bone index
//-----------------------------------------------------------------------------
void AppendChildren_R( CUtlVector< const mstudiobone_t * > *pChildBones, const studiohdr_t *pHdr, int nBone )
{
	if ( !pChildBones || !pHdr )
		return;

    // Child bones have to have a larger bone index than their parent, so start searching from nBone + 1
    for ( int i = nBone + 1; i < pHdr->numbones; ++i )
    {
        const mstudiobone_t *pBone = pHdr->pBone( i );
        if ( pBone->parent == nBone )
        {
            pChildBones->AddToTail( pBone );
            // If you just want immediate children don't recurse, this will do a depth first traversal, could do
			// breadth first by adding all children first and then looping through the added bones and recursing
            AppendChildren_R( pChildBones, pHdr, i );
        }
    }
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void BuildTorsoScaleTransformations( CBaseAnimating *pObject, CStudioHdr *hdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed, float flScale, int iClass )
{
	if ( !pObject || flScale == 1.f )
		return;

	int iPelvis = pObject->LookupBone( "bip_pelvis" );
	if ( iPelvis == -1 )
		return;

	const studiohdr_t *pHdr = modelinfo->GetStudiomodel( pObject->GetModel() );

	int iTargetBone = iPelvis;

	// must be in this order
	static const char *s_torsoBoneNames[] =
	{
		"bip_spine_0",
		"bip_spine_1",
		"bip_spine_2",
		"bip_spine_3",
		"bip_neck"
	};

	// Compress torso bones toward pelvis in order.
	for ( int i=0; i<ARRAYSIZE( s_torsoBoneNames ); ++i )
	{
		int iMoveBone = pObject->LookupBone( s_torsoBoneNames[i] );
		if ( iMoveBone == -1 )
		{
			return;
		}

		const matrix3x4_t &targetBone_transform = pObject->GetBone( iTargetBone );
		Vector vTargetBonePos;
		MatrixPosition( targetBone_transform, vTargetBonePos );

		matrix3x4_t &moveBone_transform = pObject->GetBoneForWrite( iMoveBone );
		Vector vMoveBonePos;
		MatrixPosition( moveBone_transform, vMoveBonePos );
		Vector vNewMovePos = vTargetBonePos + flScale * ( vMoveBonePos - vTargetBonePos );
		MatrixSetTranslation( vNewMovePos, moveBone_transform );

		iTargetBone = iMoveBone;

		Vector vOffset = vNewMovePos - vMoveBonePos;

		// apply to all its child bones
		CUtlVector< const mstudiobone_t * > vecChildBones;
		AppendChildren_R( &vecChildBones, pHdr, iMoveBone );
		for ( int j=0; j<vecChildBones.Count(); ++j )
		{
			int iChildBone = pObject->LookupBone( vecChildBones[j]->pszName() );
			if ( iChildBone == -1 )
				continue;

			matrix3x4_t &childBone_transform = pObject->GetBoneForWrite( iChildBone );
			Vector vChildPos;
			MatrixPosition( childBone_transform, vChildPos );
			MatrixSetTranslation( vChildPos + vOffset, childBone_transform );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void BuildHandScaleTransformations( CBaseAnimating *pObject, CStudioHdr *hdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed, float flScale )
{
	if ( !pObject || flScale == 1.f )
		return;

	const studiohdr_t *pHdr = modelinfo->GetStudiomodel( pObject->GetModel() );

	// must be in this order
	static const char *s_handBoneNames[] =
	{
		"bip_hand_L",
		"bip_hand_R"
	};

	// scale hand bones
	for ( int i=0; i<ARRAYSIZE( s_handBoneNames ); ++i )
	{
		int iHand = pObject->LookupBone( s_handBoneNames[i] );
		if ( iHand == -1 )
		{
			continue;
		}

		matrix3x4_t& transform = pObject->GetBoneForWrite( iHand );
		MatrixScaleBy( flScale, transform );

		// apply to all its child bones
		CUtlVector< const mstudiobone_t * > vecChildBones;
		AppendChildren_R( &vecChildBones, pHdr, iHand );
		for ( int j=0; j<vecChildBones.Count(); ++j )
		{
			int iChildBone = pObject->LookupBone( vecChildBones[j]->pszName() );
			if ( iChildBone == -1 )
				continue;

			matrix3x4_t &childBone_transform = pObject->GetBoneForWrite( iChildBone );
			MatrixScaleBy( flScale, childBone_transform );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed )
{
	BaseClass::BuildTransformations( hdr, pos, q, cameraTransform, boneMask, boneComputed );

	if ( GetGroundEntity() == NULL )
	{
		Vector hullSizeNormal = VEC_HULL_MAX_SCALED( this ) - VEC_HULL_MIN_SCALED( this );
		Vector hullSizeCrouch = VEC_DUCK_HULL_MAX_SCALED( this ) - VEC_DUCK_HULL_MIN_SCALED( this );
		Vector duckOffset = ( hullSizeNormal - hullSizeCrouch );

		// The player is in the air.
		if ( GetFlags() & FL_DUCKING )
		{
			if ( !m_bDuckJumpInterp )
			{
				m_flFirstDuckJumpInterp = gpGlobals->curtime;
			}
			m_bDuckJumpInterp = true;
			m_flLastDuckJumpInterp = gpGlobals->curtime;

			float flRatio = MIN( 0.15f, gpGlobals->curtime - m_flFirstDuckJumpInterp ) / 0.15f;
			m_flDuckJumpInterp = 1.f - flRatio;
		}
		else if ( m_bDuckJumpInterp )
		{
			float flRatio = MIN( 0.15f, gpGlobals->curtime - m_flLastDuckJumpInterp ) / 0.15f;
			m_flDuckJumpInterp = -(1.f - flRatio);
			if ( m_flDuckJumpInterp == 0.f )
			{
				// Turn off interpolation when we return to our normal, unducked location.
				m_bDuckJumpInterp = false;
			}
		}

		if ( m_bDuckJumpInterp && m_flDuckJumpInterp != 0.f )
		{
			duckOffset *= m_flDuckJumpInterp;
			for (int i = 0; i < hdr->numbones(); i++) 
			{
				if ( !( hdr->boneFlags( i ) & boneMask ) )
					continue;

				matrix3x4_t &transform = GetBoneForWrite( i );
				Vector bone_pos;
				MatrixGetTranslation( transform, bone_pos );
				MatrixSetTranslation( bone_pos - duckOffset, transform );
			}
		}
	}
	else if ( m_bDuckJumpInterp )
	{
		m_bDuckJumpInterp = false;
	}

	m_BoneAccessor.SetWritableBones( BONE_USED_BY_ANYTHING );
	float flHeadScale = m_flHeadScale;
	BuildBigHeadTransformations( this, hdr, pos, q, cameraTransform, boneMask, boneComputed, flHeadScale );
	BuildTorsoScaleTransformations( this, hdr, pos, q, cameraTransform, boneMask, boneComputed, m_flTorsoScale, GetPlayerClass()->GetClassIndex() );
	BuildHandScaleTransformations( this, hdr, pos, q, cameraTransform, boneMask, boneComputed, m_flHandScale );

	BuildFirstPersonMeathookTransformations( hdr, pos, q, cameraTransform, boneMask, boneComputed, "bip_head" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFRagdoll::BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed )
{
	BaseClass::BuildTransformations( hdr, pos, q, cameraTransform, boneMask, boneComputed );

	m_BoneAccessor.SetWritableBones( BONE_USED_BY_ANYTHING );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::SetHealer( C_TFPlayer *pHealer, float flChargeLevel )
{
	if ( pHealer && IsPlayerClass( TF_CLASS_SPY ) )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "player_healedbymedic" );

		if ( event )
		{
			event->SetInt( "medic", pHealer->entindex() );
			gameeventmanager->FireEventClientSide( event );
		}
	}


	// We may be getting healed by multiple healers. Show the healer
	// who's got the highest charge level.
	if ( m_hHealer )
	{
		if ( m_flHealerChargeLevel > flChargeLevel )
			return;
	}

	m_hHealer = pHealer;
	m_flHealerChargeLevel = flChargeLevel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_TFPlayer::MedicIsReleasingCharge( void )
{
	if ( IsPlayerClass(TF_CLASS_MEDIC) )
	{
		CTFWeaponBase *pWpn = GetActiveTFWeapon();

		if ( pWpn == NULL )
			return false;

		CWeaponMedigun *pMedigun = dynamic_cast <CWeaponMedigun*>( pWpn );

		if ( pMedigun )
			return pMedigun->IsReleasingCharge();
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_TFPlayer::CanShowClassMenu( void )
{
	if ( IsHLTV() )
		return false;
		
	if( TFGameRules()  )
	{
		// Dont allow the change class menu to come up when we're doing the doors and things.  There's really weird
		// sorting issues that go on even though the class menu is supposed to draw under the match status panel.
		if ( TFGameRules()->IsCompetitiveMode() )
		{
			float flRestartTime = TFGameRules()->GetRoundRestartTime() - gpGlobals->curtime;
			if ( flRestartTime > 0.f && flRestartTime < 10.f )
			{
				return false;
			}
		}
	}
	
	return ( GetTeamNumber() > LAST_SHARED_TEAM );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_TFPlayer::CanShowTeamMenu( void )
{
	if ( IsHLTV() )
		return false;

	if ( TFGameRules() && ( TFGameRules()->IsCompetitiveMode() ) )
	
		return false;

	return ( GetTeamNumber() != TEAM_UNASSIGNED );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::InitializePoseParams( void )
{
	/*
	m_headYawPoseParam = LookupPoseParameter( "head_yaw" );
	GetPoseParameterRange( m_headYawPoseParam, m_headYawMin, m_headYawMax );

	m_headPitchPoseParam = LookupPoseParameter( "head_pitch" );
	GetPoseParameterRange( m_headPitchPoseParam, m_headPitchMin, m_headPitchMax );
	*/

	CStudioHdr *hdr = GetModelPtr();
	Assert( hdr );
	if ( !hdr )
		return;

	for ( int i = 0; i < hdr->GetNumPoseParameters() ; i++ )
	{
		SetPoseParameter( hdr, i, 0.0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector C_TFPlayer::GetChaseCamViewOffset( CBaseEntity *target )
{
	if ( target->IsBaseObject() )
	{
		return Vector(0,0,64);
	}

	return BaseClass::GetChaseCamViewOffset( target );
}

//-----------------------------------------------------------------------------
// Purpose: Called from PostDataUpdate to update the model index
//-----------------------------------------------------------------------------
void C_TFPlayer::ValidateModelIndex( void )
{
	if ( m_Shared.InCond( TF_COND_DISGUISED ) && IsEnemyPlayer() )
	{
		TFPlayerClassData_t *pData = GetPlayerClassData( m_Shared.GetDisguiseClass() );
		m_nModelIndex = modelinfo->GetModelIndex( pData->GetModelName() );
	}
	else
	{
		C_TFPlayerClass *pClass = GetPlayerClass();
		if ( pClass )
		{
			m_nModelIndex = modelinfo->GetModelIndex( pClass->GetModelName() );
		}
	}

	if ( m_iSpyMaskBodygroup > -1 && GetModelPtr() != NULL && IsPlayerClass( TF_CLASS_SPY ) )
	{
		if ( m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			if ( !IsEnemyPlayer() )
			{
				SetBodygroup( m_iSpyMaskBodygroup, 1 );
			}
		}
		else
		{
			SetBodygroup( m_iSpyMaskBodygroup, 0 );
		}
	}

	BaseClass::ValidateModelIndex();
}

//-----------------------------------------------------------------------------
// Purpose: Simulate the player for this frame
//-----------------------------------------------------------------------------
void C_TFPlayer::Simulate( void )
{
	//Frame updates
	if ( this == C_BasePlayer::GetLocalPlayer() )
	{
		//Update the flashlight
		Flashlight();
	}

	// TF doesn't do step sounds based on velocity, instead using anim events
	// So we deliberately skip over the base player simulate, which calls them.
	BaseClass::BaseClass::Simulate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#define PLAYER_HALFWIDTH	 10
#define SURFACE_SNOW		 91
void C_TFPlayer::FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options )
{
	if ( event == 7001 )
	{
		// Force a footstep sound
		m_flStepSoundTime = 0;
		Vector vel;
		EstimateAbsVelocity( vel );
		surfacedata_t *t_pSurface = GetGroundSurface();
		UpdateStepSound( t_pSurface, GetAbsOrigin(), vel );
		
		if ( t_pSurface && !this->m_Shared.IsStealthed() && !this->m_Shared.InCond( TF_COND_DISGUISED ) && ( ( vel.x < -150 || vel.x > 150 ) || ( vel.y < -150 || vel.y > 150 ) ) )
		{
			// check for snow underfoot and trigger particle and decal fx
			if ( t_pSurface->game.material == SURFACE_SNOW )
			{
				ParticleProp()->Create("snow_steppuff01", PATTACH_ABSORIGIN, 0 );
				Vector right;
				AngleVectors( angles, 0, &right, 0 );

				// Figure out where the top of the stepping leg is 
				trace_t tr;
				Vector hipOrigin;
 				VectorMA( origin, m_IsFootprintOnLeft ? -PLAYER_HALFWIDTH : PLAYER_HALFWIDTH, right, hipOrigin );

				// Find where that leg hits the ground
				UTIL_TraceLine( hipOrigin, hipOrigin + Vector(0, 0, -COORD_EXTENT * 1.74), 
					MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr);

				// Create the decal
				CPVSFilter filter( tr.endpos );
				UTIL_DecalTrace( &tr, m_IsFootprintOnLeft ? "footprintL_snow" : "footprintR_snow" );

				m_IsFootprintOnLeft = !m_IsFootprintOnLeft;
			}
		}
	}
	/// XXX(JohnS): Doing this on the *client* is begging for sync errors.  What happens if this animation starts or
	///             stops outside of your PVS?  SetWeaponVisible shouldn't even be callable on the client.  It should be
	///             done on the server, the flag it sets is networked and does the right thing.  On the client it just
	///             overrides whatever it was networked --- only until the next time it is networked.  This code
	///             probably breaks if you trigger a full update, among other things.
	///
	///             I'm not going to introduce a storm of regressions touching these spots now, but if a bug crops up
	///             with this, that is probably the reason.
	else if ( event == AE_WPN_HIDE )
	{
		if ( GetActiveWeapon() )
		{
			GetActiveWeapon()->SetWeaponVisible( false );
		}
	}
	else if ( event == AE_WPN_UNHIDE )
	{
		if ( GetActiveWeapon() )
		{
			GetActiveWeapon()->SetWeaponVisible( true );
		}
	}
	else if ( event == AE_WPN_PLAYWPNSOUND )
	{
		if ( GetActiveWeapon() )
		{
			int iSnd = GetWeaponSoundFromString(options);
			if ( iSnd != -1 )
			{
				GetActiveWeapon()->WeaponSound( (WeaponSound_t)iSnd );
			}
		}
	}
	else if ( event == TF_AE_CIGARETTE_THROW || event == TF_AE_HEAD_THROW )
	{
		CEffectData data;
		int iAttach = LookupAttachment( options );
		GetAttachment( iAttach, data.m_vOrigin, data.m_vAngles );

		data.m_vAngles = GetRenderAngles();

		data.m_hEntity = ClientEntityList().EntIndexToHandle( entindex() );
		const char *pszEvent = ( event == TF_AE_CIGARETTE_THROW ) ? "TF_ThrowCigarette" : "TF_ThrowHead";
		DispatchEffect( pszEvent, data );
		return;
	}
	else if ( event == AE_CL_BODYGROUP_SET_VALUE_CMODEL_WPN )
	{
		CTFWeaponBase *pWpn = GetActiveTFWeapon();
		if ( pWpn )
		{
			pWpn->GetAppropriateWorldOrViewModel()->FireEvent( origin, angles, AE_CL_BODYGROUP_SET_VALUE, options );
		}
	}
	else if ( event == AE_CL_EXCLUDE_PLAYER_SOUND )
	{
		if ( !IsLocalPlayer() )
		{
			EmitSound( options );
		}
	}
	else
		BaseClass::FireEvent( origin, angles, event, options );
}


void C_TFPlayer::UpdateStepSound( surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity )
{
	// don't play footstep sound while taunting
	if ( IsTaunting() )
	{
		return;
	}

	BaseClass::UpdateStepSound( psurface, vecOrigin, vecVelocity );
}

// Shadows

ConVar cl_blobbyshadows( "cl_blobbyshadows", "0", FCVAR_CLIENTDLL );
ShadowType_t C_TFPlayer::ShadowCastType( void ) 
{
	// Removed the GetPercentInvisible - should be taken care off in BindProxy now.
	if ( !IsVisible() /*|| GetPercentInvisible() > 0.0f*/ )
		return SHADOWS_NONE;

	if ( IsEffectActive(EF_NODRAW | EF_NOSHADOW) )
		return SHADOWS_NONE;

	// If in ragdoll mode.
	if ( m_nRenderFX == kRenderFxRagdoll )
		return SHADOWS_NONE;

	if ( !ShouldDrawThisPlayer() )
	{
		// First-person with viewmodels.
		return SHADOWS_NONE;
	}

	if ( cl_blobbyshadows.GetBool() )
		return SHADOWS_SIMPLE;

	return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;
}

float g_flFattenAmt = 24.0;		// Roughly how far out the Heavy's minigun pokes out.
void C_TFPlayer::GetShadowRenderBounds( Vector &mins, Vector &maxs, ShadowType_t shadowType )
{
	if ( shadowType == SHADOWS_SIMPLE )
	{
		// Don't let the render bounds change when we're using blobby shadows, or else the shadow
		// will pop and stretch.
		mins = CollisionProp()->OBBMins();
		maxs = CollisionProp()->OBBMaxs();
	}
	else
	{
		GetRenderBounds( mins, maxs );

		// We do this because the normal bbox calculations don't take pose params into account, and 
		// the rotation of the guy's upper torso can place his gun a ways out of his bbox, and 
		// the shadow will get cut off as he rotates.
		//
		// Thus, we give it some padding here.
		g_flFattenAmt = 36.0f;
		mins -= Vector( g_flFattenAmt, g_flFattenAmt, 0 );
		maxs += Vector( g_flFattenAmt, g_flFattenAmt, 0 );
	}
}


void C_TFPlayer::GetRenderBounds( Vector& theMins, Vector& theMaxs )
{
	// TODO POSTSHIP - this hack/fix goes hand-in-hand with a fix in CalcSequenceBoundingBoxes in utils/studiomdl/simplify.cpp.
	// When we enable the fix in CalcSequenceBoundingBoxes, we can get rid of this.
	//
	// What we're doing right here is making sure it only uses the bbox for our lower-body sequences since,
	// with the current animations and the bug in CalcSequenceBoundingBoxes, are WAY bigger than they need to be.
	C_BaseAnimating::GetRenderBounds( theMins, theMaxs );
}


bool C_TFPlayer::GetShadowCastDirection( Vector *pDirection, ShadowType_t shadowType ) const
{ 
	if ( shadowType == SHADOWS_SIMPLE )
	{
		// Blobby shadows should sit directly underneath us.
		pDirection->Init( 0, 0, -1 );
		return true;
	}
	else
	{
		return BaseClass::GetShadowCastDirection( pDirection, shadowType );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether this player is the nemesis of the local player
//-----------------------------------------------------------------------------
bool C_TFPlayer::IsNemesisOfLocalPlayer()
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer )
	{
		// return whether this player is dominating the local player
		return m_Shared.IsPlayerDominated( pLocalPlayer->entindex() );
	}		
	return false;
}

extern ConVar tf_tournament_hide_domination_icons;

//-----------------------------------------------------------------------------
// Purpose: Returns whether we should show the nemesis icon for this player
//-----------------------------------------------------------------------------
bool C_TFPlayer::ShouldShowNemesisIcon()
{
	if ( TFGameRules() && TFGameRules()->IsInTournamentMode() && tf_tournament_hide_domination_icons.GetBool() )
		return false;

	if ( m_PlayerClass.HasCustomModel() )
		return false;

	// we should show the nemesis effect on this player if he is the nemesis of the local player,
	// and is not dead, cloaked or disguised
	if ( IsNemesisOfLocalPlayer() && g_PR && g_PR->IsConnected( entindex() ) )
	{
		bool bStealthed = m_Shared.IsStealthed();
		bool bDisguised = m_Shared.InCond( TF_COND_DISGUISED );
		if ( IsAlive() && !bStealthed && !bDisguised )
			return true;
	}

	return false;
}

bool C_TFPlayer::IsWeaponLowered( void )
{
	CTFWeaponBase *pWeapon = GetActiveTFWeapon();

	if ( !pWeapon )
		return false;

	CTFGameRules *pRules = TFGameRules();

	// Lower losing team's weapons in bonus round
	if ( ( pRules->State_Get() == GR_STATE_TEAM_WIN ) && ( pRules->GetWinningTeam() != GetTeamNumber() ) )
		return true;

	// Hide all view models after the game is over
	if ( pRules->State_Get() == GR_STATE_GAME_OVER && ( !pRules->IsInTournamentMode() ) )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_TFPlayer::StartSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget )
{
	switch ( event->GetType() )
	{
	case CChoreoEvent::SEQUENCE:
	case CChoreoEvent::GESTURE:
		return StartGestureSceneEvent( info, scene, event, actor, pTarget );
	default:
		return BaseClass::StartSceneEvent( info, scene, event, actor, pTarget );
	}
}

bool C_TFPlayer::ClearSceneEvent( CSceneEventInfo *info, bool fastKill, bool canceled )
{
	switch ( info->m_pEvent->GetType() )
	{
	case CChoreoEvent::SEQUENCE:
	case CChoreoEvent::GESTURE:
		return StopGestureSceneEvent( info, fastKill, canceled );
	default:
		return BaseClass::ClearSceneEvent( info, fastKill, canceled );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_TFPlayer::StartGestureSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget )
{
	// Get the (gesture) sequence.
	info->m_nSequence = LookupSequence( event->GetParameters() );
	if ( info->m_nSequence < 0 )
		return false;

	// Player the (gesture) sequence.
	float flCycle = 0.0f;
	bool looping = ((GetSequenceFlags( GetModelPtr(), info->m_nSequence ) & STUDIO_LOOPING) != 0);
	if (!looping)
	{
		float dt = scene->GetTime() - event->GetStartTime();
		m_flTauntDuration = SequenceDuration( info->m_nSequence );
		flCycle = dt / m_flTauntDuration;
		flCycle = clamp( flCycle, 0.f, 1.0f );
	}
	else
	{
		float flStart, flEnd;
		scene->GetSceneTimes( flStart, flEnd );
		m_flTauntDuration = flEnd - flStart;
	}

	m_PlayerAnimState->ResetGestureSlot( GESTURE_SLOT_VCD );
	m_PlayerAnimState->AddVCDSequenceToGestureSlot( GESTURE_SLOT_VCD, info->m_nSequence, flCycle, true );

	m_nTauntSequence = info->m_nSequence;
	m_flTauntStartTime = gpGlobals->curtime;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_TFPlayer::StopGestureSceneEvent( CSceneEventInfo *info, bool fastKill, bool canceled )
{
	// The ResetGestureSlot call will prevent people from doing running taunts (which they like to do),
	// so let's only reset the gesture slot if the scene contains a loop (such as the high five pose).
	bool bSceneContainsLoop = false;
	for ( int i = 0; i < info->m_pScene->GetNumEvents(); i++ )
	{
		CChoreoEvent *pEvent = info->m_pScene->GetEvent( i );
		if ( pEvent->GetType() == CChoreoEvent::LOOP )
		{
			bSceneContainsLoop = true;
			break;
		}
	}

	if( bSceneContainsLoop )
		m_PlayerAnimState->ResetGestureSlot( GESTURE_SLOT_VCD );

	return true;
}


bool C_TFPlayer::IsAllowedToSwitchWeapons( void )
{
	if ( IsWeaponLowered() == true )
		return false;

	if ( TFGameRules() )
	{
		if ( TFGameRules()->ShowMatchSummary() )
			return false;
	}

	// Can't weapon switch during a taunt.
	if( m_Shared.InCond( TF_COND_TAUNTING ) && tf_allow_taunt_switch.GetInt() <= 1 )
		return false;

	return BaseClass::IsAllowedToSwitchWeapons();
}

IMaterial *C_TFPlayer::GetHeadLabelMaterial( void )
{
	if ( g_pHeadLabelMaterial[0] == NULL )
		SetupHeadLabelMaterials();

	if ( GetTeamNumber() == TF_TEAM_RED )
	{
		return g_pHeadLabelMaterial[TF_PLAYER_HEAD_LABEL_RED];
	}
	else
	{
		return g_pHeadLabelMaterial[TF_PLAYER_HEAD_LABEL_BLUE];
	}

	return BaseClass::GetHeadLabelMaterial();
}

void SetupHeadLabelMaterials( void )
{
	for ( int i = 0; i < 2; i++ )
	{
		if ( g_pHeadLabelMaterial[i] )
		{
			g_pHeadLabelMaterial[i]->DecrementReferenceCount();
			g_pHeadLabelMaterial[i] = NULL;
		}

		g_pHeadLabelMaterial[i] = materials->FindMaterial( pszHeadLabelNames[i], TEXTURE_GROUP_VGUI );
		if ( g_pHeadLabelMaterial[i] )
		{
			g_pHeadLabelMaterial[i]->IncrementReferenceCount();
		}
	}
}

void C_TFPlayer::ComputeFxBlend( void )
{
	BaseClass::ComputeFxBlend();

	float flInvisible = GetPercentInvisible();
	if ( flInvisible != 0.0f )
	{
		// Tell our shadow
		ClientShadowHandle_t hShadow = GetShadowHandle();
		if ( hShadow != CLIENTSHADOW_INVALID_HANDLE )
		{
			g_pClientShadowMgr->SetFalloffBias( hShadow, flInvisible * 255 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::CalcView( Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov )
{
	HandleTaunting();
	BaseClass::CalcView( eyeOrigin, eyeAngles, zNear, zFar, fov );
}

void SelectDisguise( int iClass, int iTeam );

static void cc_tf_player_lastdisguise( const CCommand &args )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( pPlayer == NULL )
		return;

	// disguise as our last known disguise. desired disguise will be initted to something sensible
	if ( pPlayer->CanDisguise() )
	{
		// disguise as the previous class, if one exists
		int nClass = pPlayer->m_Shared.GetDesiredDisguiseClass();

		int iLocalTeam = pPlayer->GetTeamNumber();
		int iEnemyTeam = ( iLocalTeam == TF_TEAM_BLUE ) ? TF_TEAM_RED : TF_TEAM_BLUE;
		int nTeam = pPlayer->m_Shared.WasLastDisguiseAsOwnTeam() ? iLocalTeam : iEnemyTeam; 

		//If we pass in "random" or whatever then just make it pick a random class.
		if ( args.ArgC() > 1 )
		{
			nClass = TF_CLASS_UNDEFINED;
		}

		if ( nClass == TF_CLASS_UNDEFINED )
		{
			// they haven't disguised yet, pick a nice one for them.
			// exclude some undesirable classes
			do
			{
				nClass = random->RandomInt( TF_FIRST_NORMAL_CLASS, TF_LAST_NORMAL_CLASS );
			} while( nClass == TF_CLASS_SCOUT || nClass == TF_CLASS_SPY );

			nTeam = iEnemyTeam;
		}

		SelectDisguise( nClass, nTeam );
		
	}

}
static ConCommand lastdisguise( "lastdisguise", cc_tf_player_lastdisguise, "Disguises the spy as the last disguise." );


static void cc_tf_player_disguise( const CCommand &args )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( pPlayer == NULL )
		return;

	if ( args.ArgC() >= 3 )
	{
		if ( pPlayer->CanDisguise() )
		{
			int nClass = atoi( args[ 1 ] );
			int nTeam = atoi( args[ 2 ] );

			//Disguise as enemy team
			if ( nTeam == -1 )
			{
				if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
				{
					nTeam = TF_TEAM_RED;
				}
				else
				{
					nTeam = TF_TEAM_BLUE;
				}
			}
			else if ( nTeam == -2 ) //disguise as my own team
			{
				nTeam = pPlayer->GetTeamNumber();
			}
			else
			{
				nTeam = ( nTeam == 1 ) ? TF_TEAM_BLUE : TF_TEAM_RED;
			}
			
			// intercepting the team value and reassigning what gets passed into Disguise()
			// because the team numbers in the client menu don't match the #define values for the teams
			SelectDisguise( nClass, nTeam );
		}
	}
}

static ConCommand disguise( "disguise", cc_tf_player_disguise, "Disguises the spy." );


#ifdef _DEBUG
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
static void cc_tf_crashclient()
{
	C_TFPlayer *pPlayer = NULL;
	pPlayer->ComputeFxBlend();
}
static ConCommand tf_crashclient( "tf_crashclient", cc_tf_crashclient, "Crashes this client for testing.", FCVAR_DEVELOPMENTONLY );
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::ForceUpdateObjectHudState( void )
{
	m_bUpdateObjectHudState = true;
}

#include "c_obj_sentrygun.h"


static void cc_tf_debugsentrydmg()
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	pPlayer->UpdateIDTarget();
	int iTarget = pPlayer->GetIDTarget();
	if ( iTarget > 0 )
	{
		C_BaseEntity *pEnt = cl_entitylist->GetEnt( iTarget );

		C_ObjectSentrygun *pSentry = dynamic_cast< C_ObjectSentrygun * >( pEnt );

		if ( pSentry )
		{
			pSentry->DebugDamageParticles();
		}
	}
}
static ConCommand tf_debugsentrydamage( "tf_debugsentrydamage", cc_tf_debugsentrydmg, "", FCVAR_DEVELOPMENTONLY );

/*
CON_COMMAND_F( spectate_random_server_extend_time, "extend the timer we're spectating this server before we disconnect", FCVAR_DEVELOPMENTONLY )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		if ( pPlayer->m_LeaveServerTimer.HasStarted() )
		{
			float flTime = spectate_random_server_basetime.GetFloat();

			if ( args.ArgC() > 1 )
			{
				flTime = MAX( 0, Q_atof( args[ 1 ] ) );
			}

			pPlayer->m_LeaveServerTimer.Start( flTime );
		}
	}	
}*/


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::GetTargetIDDataString( bool bIsDisguised, OUT_Z_BYTECAP(iMaxLenInBytes) wchar_t *sDataString, int iMaxLenInBytes, bool &bIsAmmoData, bool &bIsKillStreakData )
{
	Assert( iMaxLenInBytes >= sizeof(sDataString[0]) );
	// Make sure the output string is always initialized to a null-terminated string,
	// since the conditions below are tricky.
	sDataString[0] = 0;

	if ( bIsDisguised )
	{
		if ( !IsEnemyPlayer() )
		{
			// The target is a disguised friendly spy.  They appear to the player with no disguise.  Add the disguise
			// team & class to the target ID element.
			bool bDisguisedAsEnemy = ( m_Shared.GetDisguiseTeam() != GetTeamNumber() );
			const wchar_t *wszAlignment = g_pVGuiLocalize->Find( bDisguisedAsEnemy ? "#TF_enemy" : "#TF_friendly" );

			int classindex = m_Shared.GetDisguiseClass();
			const wchar_t *wszClassName = g_pVGuiLocalize->Find( g_aPlayerClassNames[classindex] );

			// build a string with disguise information
			g_pVGuiLocalize->ConstructString( sDataString, iMaxLenInBytes, g_pVGuiLocalize->Find( "#TF_playerid_friendlyspy_disguise" ), 
				2, wszAlignment, wszClassName );
		}
		else if ( IsEnemyPlayer() && (m_Shared.GetDisguiseClass() == TF_CLASS_SPY) )
		{
			// The target is an enemy spy disguised as a friendly spy. Show a fake team & class ID element.
			int classindex = m_Shared.GetDisguiseMask();
			const wchar_t *wszClassName = g_pVGuiLocalize->Find( g_aPlayerClassNames[classindex] );
			const wchar_t *wszAlignment = g_pVGuiLocalize->Find( "#TF_enemy" );

			g_pVGuiLocalize->ConstructString( sDataString, iMaxLenInBytes, g_pVGuiLocalize->Find( "#TF_playerid_friendlyspy_disguise" ), 
				2, wszAlignment, wszClassName );
		}
	}

	if ( IsPlayerClass( TF_CLASS_MEDIC ) )
	{
		CTFWeaponBase *pMedigun = NULL;

		// Medics put their ubercharge & medigun type into the data string
		wchar_t wszChargeLevel[ 10 ];
		_snwprintf( wszChargeLevel, ARRAYSIZE(wszChargeLevel) - 1, L"%.0f", MedicGetChargeLevel( &pMedigun ) * 100 );
		wszChargeLevel[ ARRAYSIZE(wszChargeLevel)-1 ] = '\0';

		g_pVGuiLocalize->ConstructString( sDataString, iMaxLenInBytes, g_pVGuiLocalize->Find( "#TF_playerid_mediccharge" ), 1, wszChargeLevel );

	}
	else if ( bIsDisguised && (m_Shared.GetDisguiseClass() == TF_CLASS_MEDIC) && IsEnemyPlayer() )
	{
		// Show a fake charge level for a disguised enemy medic.
		g_pVGuiLocalize->ConstructString( sDataString, iMaxLenInBytes, g_pVGuiLocalize->Find( "#TF_playerid_mediccharge" ), 1, L"0" );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::ThirdPersonSwitch( bool bThirdperson )
{
	FlushAllPlayerVisibilityState();
}

void C_TFPlayer::ForceTempForceDraw( bool bThirdPerson )
{
	m_Local.m_bForceLocalPlayerDraw = bThirdPerson;

	// Update our weapon's visibility when we switch
	C_TFWeaponBase *pWeapon = GetActiveTFWeapon();
	if ( pWeapon )
	{
		if ( bThirdPerson )
		{
			m_flTempForceDrawViewModelCycle = pWeapon->GetCycle();
			m_nTempForceDrawViewModelSequence = pWeapon->GetSequence();
			m_nTempForceDrawViewModelSkin = pWeapon->m_nSkin;
		}

		pWeapon->UpdateModelIndex();

		if ( bThirdPerson )
		{
			pWeapon->m_nSkin = pWeapon->GetSkin();
		}
		else
		{
			if ( m_nTempForceDrawViewModelSequence > 0 )
			{
				pWeapon->SetSequence( m_nTempForceDrawViewModelSequence );
				m_nTempForceDrawViewModelSequence = -1;
			}

			pWeapon->SetCycle( m_flTempForceDrawViewModelCycle );
			m_nTempForceDrawViewModelSkin = 0;

			pWeapon->m_nSkin = m_nTempForceDrawViewModelSkin;
			m_flTempForceDrawViewModelCycle = 0.0f;
		}
	}
	FlushAllPlayerVisibilityState();
}

void C_TFPlayer::FlushAllPlayerVisibilityState()
{
	// We've switch from first to third, or vice versa.
	UpdateVisibility();

	// Update the visibility of anything bone attached to us.
	if ( IsLocalPlayer() )
	{
		bool bShouldDrawLocalPlayer = ShouldDrawLocalPlayer();
		for ( int i=0; i<GetNumBoneAttachments(); ++i )
		{
			C_BaseAnimating* pBoneAttachment = GetBoneAttachment( i );
			if ( pBoneAttachment )
			{
				if ( bShouldDrawLocalPlayer )
				{
					pBoneAttachment->RemoveEffects( EF_NODRAW );
				}
				else
				{
					pBoneAttachment->AddEffects( EF_NODRAW );
				}
			}
		}
	}

	// Update our viewmodel whenever we switch view modes
	C_TFPlayer *pTFObserverTarget = ToTFPlayer( GetObserverTarget() );
	if ( pTFObserverTarget )
	{
		pTFObserverTarget->SetBodygroupsDirty();
	}

	// Update our weapon's visibility when we switch
	C_TFWeaponBase *pWeapon = GetActiveTFWeapon();
	if ( pWeapon )
	{
		pWeapon->UpdateModelIndex();
		pWeapon->UpdateVisibility();
	}

	// Update visibility of any worn items.
	SetBodygroupsDirty();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::OnAchievementAchieved( int iAchievement )
{
	EmitSound( "Achievement.Earned" );

	BaseClass::OnAchievementAchieved( iAchievement );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::UpdateSpyStateChange( void )
{
	UpdateOverhealEffect();
	UpdateRecentlyTeleportedEffect();

	// Force Weapon updates
	if ( GetActiveWeapon() )
	{
		GetActiveWeapon()->PreDataUpdate( DATA_UPDATE_DATATABLE_CHANGED );
	}

	// TranqMark
	// test : Let Marked spies be seen via the debuff
	//bool bShow = ( m_Shared.InCond( TF_COND_TRANQ_MARKED )
	//	&& !m_Shared.IsStealthed();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::UpdateOverhealEffect( void )
{
	bool bShow = m_Shared.InCond( TF_COND_HEALTH_OVERHEALED );
	int iTeam = GetTeamNumber();

	if ( IsLocalPlayer() || ( m_Shared.IsStealthed() && !InSameTeam( GetLocalTFPlayer() ) ) )
	{
		bShow = false;
	}
	else if ( IsPlayerClass( TF_CLASS_SPY ) && !InSameTeam( GetLocalTFPlayer() ) && m_Shared.InCond( TF_COND_DISGUISED ))
	{
		iTeam = m_Shared.GetDisguiseTeam();
	}

	if ( bShow )
	{
		if ( !m_pOverHealedEffect )
		{
			CreateOverhealEffect( iTeam );
		}
		else if ( m_pOverHealedEffect )
		{
			ParticleProp()->StopEmission( m_pOverHealedEffect );
			m_pOverHealedEffect = NULL;
			CreateOverhealEffect( iTeam );
		}
	}
	else
	{
		if ( m_pOverHealedEffect )
		{
			ParticleProp()->StopEmission( m_pOverHealedEffect );
			m_pOverHealedEffect = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::CreateOverhealEffect( int iTeam )
{
	const char *pEffect = NULL;
	switch( iTeam )
	{
	case TF_TEAM_BLUE:
		pEffect = "overhealedplayer_blue_pluses";
		break;
	case TF_TEAM_RED:
		pEffect = "overhealedplayer_red_pluses";
		break;
	default:
		break;
	}

	if ( pEffect )
	{
		m_pOverHealedEffect = ParticleProp()->Create( pEffect, PATTACH_ABSORIGIN_FOLLOW );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::SetMetersRan( float fMeters, int iFrame )
{
	if ( iFrame != m_iLastRanFrame )
	{
		m_iLastRanFrame = iFrame;
		m_fMetersRan = fMeters;
	}
}

bool C_TFPlayer::InSameDisguisedTeam( CBaseEntity *pEnt )
{
	if ( pEnt == NULL )
		return false;

	int iMyApparentTeam = GetTeamNumber();

	if ( m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		iMyApparentTeam = m_Shared.GetDisguiseTeam();
	}

	C_TFPlayer *pPlayerEnt = ToTFPlayer( pEnt );
	int iTheirApparentTeam = pEnt->GetTeamNumber();
	if ( pPlayerEnt && pPlayerEnt->m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		iTheirApparentTeam = pPlayerEnt->m_Shared.GetDisguiseTeam();
	}

	return ( iMyApparentTeam == iTheirApparentTeam || GetTeamNumber() == pEnt->GetTeamNumber() || iTheirApparentTeam == GetTeamNumber() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_TFPlayer::CanUseFirstPersonCommand( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer )
	{
		if ( pLocalPlayer->m_Shared.InCond( TF_COND_TAUNTING ) )
		{
			return false;
		}
	}

	return BaseClass::CanUseFirstPersonCommand();
}

//-----------------------------------------------------------------------------
void GetVectorColorForParticleSystem ( int iSystem, bool bIsBlueTeam, bool bUseColor2, Vector &vecColor )
{
	if ( iSystem < 0 || iSystem >= ARRAYSIZE( g_KillStreakEffectsBase ) )
		return;

	killstreak_params_t params = g_KillStreakEffectsBase[iSystem];

	if ( bIsBlueTeam && g_KillStreakEffectsBase[iSystem].m_bHasTeamColor )
	{
		Assert( iSystem > 0 && iSystem < ARRAYSIZE( g_KillStreakEffectsBlue ) );
		params = g_KillStreakEffectsBlue[iSystem];
	}
	
	if ( bUseColor2 )
	{
		vecColor = Vector( params.m_color2_r, params.m_color2_g, params.m_color2_b );
	}
	else
	{
		vecColor = Vector( params.m_color1_r, params.m_color1_g, params.m_color1_b );
	}
	return;
}

//-----------------------------------------------------------------------------
// Purpose: Check if local player should see spy as disguised body
//-----------------------------------------------------------------------------
bool C_TFPlayer::ShouldDrawSpyAsDisguised()
{
	if ( C_BasePlayer::GetLocalPlayer() && m_Shared.InCond( TF_COND_DISGUISED ) && 
		( GetEnemyTeam( GetTeamNumber() ) == C_BasePlayer::GetLocalPlayer()->GetTeamNumber() ) )
	{
		if ( m_Shared.GetDisguiseClass() == TF_CLASS_SPY &&
			m_Shared.GetDisguiseTeam()  == C_BasePlayer::GetLocalPlayer()->GetTeamNumber() )
		{
			// This enemy is disguised as a friendly spy.
			// Show the spy's original bodygroups.
			return false;
		}
		else
		{
			// This enemy is disguised. Show the disguise body.
			return true;
		}
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TFPlayer::GetBody( void )
{
	if ( ShouldDrawSpyAsDisguised() )
	{
		// This enemy is disguised. Show the disguise body.
		return m_Shared.GetDisguiseBody();
	}
	else
	{
		return BaseClass::GetBody();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const Vector& C_TFPlayer::GetRenderOrigin( void )
{
	if ( GetPlayerClass()->HasCustomModel() )
	{
		m_vecCustomModelOrigin = BaseClass::GetRenderOrigin() + GetPlayerClass()->GetCustomModelOffset();
		return m_vecCustomModelOrigin;
	}

	return BaseClass::GetRenderOrigin();
}

bool C_TFPlayer::IsEffectRateLimited( EBonusEffectFilter_t effect, const C_TFPlayer* pAttacker ) const
{
	// Check if we're rate limited
	switch( effect )
	{
		case kEffectFilter_AttackerOnly:
		case kEffectFilter_VictimOnly:
		case kEffectFilter_AttackerAndVictimOnly:
			return false;
		case kEffectFilter_AttackerTeam:
		case kEffectFilter_VictimTeam:
		case kEffectFilter_BothTeams:
			// Dont rate limit ourselves
			if( pAttacker == this )
				return false;

			return true;
		default:
			AssertMsg1( 0, "EBonusEffectFilter_t type not handled in %s", __FUNCTION__ );
			return false;
	}
}

bool C_TFPlayer::ShouldPlayEffect( EBonusEffectFilter_t filter, const C_TFPlayer* pAttacker, const C_TFPlayer* pVictim ) const
{
	Assert( pAttacker );
	Assert( pVictim );
	if( !pAttacker || !pVictim )
		return false;

	// Check if the right player relationship
	switch( filter )
	{
		case kEffectFilter_AttackerOnly:
			return ( pAttacker == this );
		case kEffectFilter_AttackerTeam:
			return ( pAttacker->GetTeamNumber() == this->GetTeamNumber() );
		case kEffectFilter_VictimOnly:
			return ( pVictim == this );
		case kEffectFilter_VictimTeam:
			return ( pVictim->GetTeamNumber() == this->GetTeamNumber() );
		case kEffectFilter_AttackerAndVictimOnly:
			return ( pAttacker == this || pVictim == this );
		case kEffectFilter_BothTeams:
			return ( pAttacker->GetTeamNumber() == this->GetTeamNumber() || pVictim->GetTeamNumber() == this->GetTeamNumber() );

		default:
			AssertMsg1( 0, "EBonusEffectFilter_t type not handled in %s", __FUNCTION__ );
			return false;
	};
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::FireGameEvent( IGameEvent *event )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( FStrEq( event->GetName(), "player_hurt" ) )
	{
		COMPILE_TIME_ASSERT( ARRAYSIZE( g_BonusEffects ) == kBonusEffect_Count );

		// These only should affect the local player
		if ( !pLocalPlayer || pLocalPlayer != this )
			return;

		// By default we get kBonusEffect_None. We want to use whatever value we get here if it's not kBonusEffect_None.
		// If it's not, then check for crit or minicrit
		EAttackBonusEffects_t eBonusEffect = (EAttackBonusEffects_t)event->GetInt( "bonuseffect", (int)kBonusEffect_None );
		if( eBonusEffect == kBonusEffect_None )
		{
			// Keep reading for these fields to keep replays happy
			eBonusEffect = event->GetBool( "crit", false )		? kBonusEffect_Crit		: eBonusEffect;
		}

		// No effect to show?  Bail
		if( eBonusEffect == kBonusEffect_None || eBonusEffect >= kBonusEffect_Count )
			return;

		const int iAttacker = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
		C_TFPlayer *pAttacker = ToTFPlayer( UTIL_PlayerByIndex( iAttacker ) );

		const int iVictim = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
		C_TFPlayer *pVictim = ToTFPlayer( UTIL_PlayerByIndex( iVictim ) );

		// No pointers to players?  Bail
		if( !pAttacker || !pVictim )
			return;

		bool bShowDisguisedCrit = event->GetBool( "showdisguisedcrit", 0 );

		// Victim is disguised and we're not showing disguised effects?  Bail
		if ( pVictim->m_Shared.InCond( TF_COND_DISGUISED ) && !bShowDisguisedCrit )
			return;

		// Support old system.  If "allseecrit" is set that means we want this to show for our whole team.
		EBonusEffectFilter_t eParticleFilter = g_BonusEffects[ eBonusEffect ].m_eParticleFilter;
		EBonusEffectFilter_t eSoundFilter = g_BonusEffects[ eBonusEffect ].m_eSoundFilter;
		if( event->GetBool( "allseecrit", false ) )
		{
			eParticleFilter = kEffectFilter_AttackerTeam;
			eSoundFilter	= kEffectFilter_AttackerTeam;
		}

		// Check if either of our effects are rate limited
		if( IsEffectRateLimited( eParticleFilter, pAttacker ) || IsEffectRateLimited( eSoundFilter, pAttacker ) )
		{
			// Check if we're cooling down
			if( !pVictim->CanDisplayAllSeeEffect( eBonusEffect ) )
			{
				// Too often!  Return
				return;
			}

			// Set cooldown period
			pVictim->SetNextAllSeeEffectTime( eBonusEffect, gpGlobals->curtime + 0.5f );
		}
		
		// Show the effect, unless combat text blocks
		if( ShouldPlayEffect( eParticleFilter, pAttacker, pVictim ) )
		{
			if ( g_BonusEffects[ eBonusEffect ].m_eAttachment == PATTACH_POINT || g_BonusEffects[ eBonusEffect ].m_eAttachment == PATTACH_POINT_FOLLOW )
			{
				pVictim->ParticleProp()->Create( g_BonusEffects[ eBonusEffect ].m_pszParticle,
												 g_BonusEffects[ eBonusEffect ].m_eAttachment,
												 g_BonusEffects[ eBonusEffect ].m_pszAttachmentName );
			}
			else
			{
				pVictim->ParticleProp()->Create( g_BonusEffects[ eBonusEffect ].m_pszParticle,
												 g_BonusEffects[ eBonusEffect ].m_eAttachment );
			}
		}

		// Play the sound!
		if( ShouldPlayEffect( eSoundFilter, pAttacker, pVictim ) && g_BonusEffects[ eBonusEffect ].m_pszSoundName != NULL )
		{
			const bool& bPlayInAttackersEars = g_BonusEffects[ eBonusEffect ].m_bPlaySoundInAttackersEars;

			// sound effects
			EmitSound_t params;
			params.m_flSoundTime = 0;
			params.m_pflSoundDuration = 0;
			params.m_pSoundName = g_BonusEffects[ eBonusEffect ].m_pszSoundName;

			CPASFilter filter( pVictim->GetAbsOrigin() );
			if( bPlayInAttackersEars && pAttacker == this )
			{
				// Don't let the attacker hear this version if its to be played in their ears
				filter.RemoveRecipient( pAttacker );

				// Play a sound in the ears of the attacker
				CSingleUserRecipientFilter attackerFilter( pAttacker );
				EmitSound( attackerFilter, pAttacker->entindex(), params );
			}

			EmitSound( filter, pVictim->entindex(), params );
		}
	}
	else if ( FStrEq( event->GetName(), "hltv_changed_mode" ) )
	{
		int iTarget = event->GetInt( "obs_target" );
		if ( iTarget == entindex() )
		{
			int iOld = event->GetInt( "oldmode" );
			int iNew = event->GetInt( "newmode" );

			if ( iOld == OBS_MODE_IN_EYE || iNew == OBS_MODE_IN_EYE )
			{
				// Update visibility of any worn items.
				SetBodygroupsDirty();
			}
		}
	}
	else if ( FStrEq( event->GetName(), "hltv_changed_target" ) )
	{
		int iOldTarget = event->GetInt( "old_target" );
		int iTarget = event->GetInt( "obs_target" );
		if ( iTarget == entindex() || iOldTarget == entindex() )
		{
			// Update visibility of any worn items.
			SetBodygroupsDirty();
		}
	}
	else if ( FStrEq( event->GetName(), "player_changeclass" ) )
	{
		if ( TFGameRules() && TFGameRules()->IsMatchTypeCompetitive() )
		{
			if ( g_PR &&
				pLocalPlayer &&
				pLocalPlayer == this &&
				TFGameRules() &&
				TFGameRules()->IsCompetitiveMode() &&
				TFGameRules()->State_Get() == GR_STATE_RND_RUNNING )
			{
				CBaseHudChat *pHudChat = (CBaseHudChat*)GET_HUDELEMENT( CHudChat );
				if ( pHudChat )
				{
					C_BasePlayer *pEventPlayer = UTIL_PlayerByUserId( event->GetInt( "userid" ) );
					if ( pEventPlayer && pLocalPlayer->GetTeamNumber() == g_PR->GetTeam( pEventPlayer->entindex() ) )
					{
						int nClassID = event->GetInt( "class" );
						if ( nClassID >= 0 && nClassID < ARRAYSIZE( g_aPlayerClassNames ) )
						{
							wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
							g_pVGuiLocalize->ConvertANSIToUnicode( g_PR->GetPlayerName( pEventPlayer->entindex() ), wszPlayerName, sizeof( wszPlayerName ) );

							wchar_t wszLocalized[100];
							g_pVGuiLocalize->ConstructString_safe( wszLocalized, g_pVGuiLocalize->Find( "#TF_Class_Change" ), 2, wszPlayerName, g_pVGuiLocalize->Find( g_aPlayerClassNames[nClassID] ) );

							char szLocalized[100];
							g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof( szLocalized ) );

							pHudChat->ChatPrintf( pLocalPlayer->entindex(), CHAT_FILTER_NAMECHANGE, "%s", szLocalized );
						}
					}
				}
			}
		}
	}
	BaseClass::FireGameEvent( event );
}

const char* C_TFPlayer::ModifyEventParticles( const char* token )
{
	if ( GetPlayerClass()->IsClass( TF_CLASS_SCOUT ) )
	{
		if ( !Q_strcmp( token, "doublejump_puff" ) )
		{
			if ( m_Shared.GetAirDash() > 1 )
			{
				return "doublejump_puff_alt";
			}
		}
	}

	return BaseClass::ModifyEventParticles( token );
}

void C_TFPlayer::SetTauntCameraTargets( float back, float up )
{
	m_flTauntCamTargetDist = back;
	m_flTauntCamTargetDistUp = up;
	// Force this on
	m_bTauntInterpolating = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_TFPlayer::AddOverheadEffect( const char *pszEffectName )
{
	int index_ = m_mapOverheadEffects.Find( pszEffectName );

	// particle is added already
	if ( index_ != m_mapOverheadEffects.InvalidIndex() )
		return false;

	CNewParticleEffect *pEffect = ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW, 0, GetOverheadEffectPosition() );
	if ( pEffect )
	{
		if ( m_mapOverheadEffects.Count() == 0 )
		{
			m_flOverheadEffectStartTime = gpGlobals->curtime;
		}

		m_mapOverheadEffects.Insert( pszEffectName, pEffect );

		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::RemoveOverheadEffect( const char *pszEffectName, bool bRemoveInstantly )
{
	int index_ = m_mapOverheadEffects.Find( pszEffectName );

	// particle is added already
	if ( index_ != m_mapOverheadEffects.InvalidIndex() )
	{
		if ( bRemoveInstantly )
			ParticleProp()->StopEmissionAndDestroyImmediately( m_mapOverheadEffects[index_] );
		ParticleProp()->StopParticlesNamed( pszEffectName, bRemoveInstantly );
		m_mapOverheadEffects.RemoveAt( index_ );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::UpdateOverheadEffects()
{
	if ( IsLocalPlayer() )
		return;

	const int nOverheadEffectCount = m_mapOverheadEffects.Count();
	if ( nOverheadEffectCount == 0 )
		return;

	Vector vecOverheadEffectPosition = GetOverheadEffectPosition();
	C_TFPlayer *pLocalPlayer = GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	Vector vecHeadToHead = EyePosition() - pLocalPlayer->EyePosition();

	const float flEffectGap = 24.f;
	Vector vecRightOffset = CrossProduct( vecHeadToHead, Vector( 0, 0, 1 ) ).Normalized();
	float flFirstEffectOffset = -flEffectGap * 0.5f * ( nOverheadEffectCount - 1 );
	int iValidParticleIndex = 0;
	FOR_EACH_MAP_FAST( m_mapOverheadEffects, i )
	{
		HPARTICLEFFECT hEffect = m_mapOverheadEffects[i];
		if ( hEffect )
		{
			float flCurrentOffset = flFirstEffectOffset + flEffectGap * iValidParticleIndex;
			Vector vecOffset = vecOverheadEffectPosition + flCurrentOffset * vecRightOffset;
			ParticleProp()->AddControlPoint( hEffect, 0, this, PATTACH_ABSORIGIN_FOLLOW, 0, vecOffset );
			iValidParticleIndex++;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector C_TFPlayer::GetOverheadEffectPosition()
{
	return GetClassEyeHeight() + Vector( 0, 0, 20 );
}


extern ISoundEmitterSystemBase *soundemitterbase;


static void cc_helpme_pressed( const CCommand &args )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	pLocalPlayer->SetHelpmeButtonPressedTime( gpGlobals->curtime );

	KeyValues *kv = new KeyValues( "+helpme_server" );
	engine->ServerCmdKeyValues( kv );
}
static ConCommand helpme_pressed( "+helpme", cc_helpme_pressed );

static void cc_helpme_released( const CCommand &args )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	pLocalPlayer->SetHelpmeButtonPressedTime( 0.f );

	KeyValues *kv = new KeyValues( "-helpme_server" );
	engine->ServerCmdKeyValues( kv );
}
static ConCommand helpme_released( "-helpme", cc_helpme_released );

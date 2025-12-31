//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Player for HL1.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_gamestats.h"
#include "KeyValues.h"
#include "viewport_panel_names.h"
#include "client.h"
#include "team.h"
#include "tf_weaponbase.h"
#include "tf_client.h"
#include "tf_team.h"
#include "tf_viewmodel.h"
#include "tf_item.h"
#include "in_buttons.h"
#include "entity_capture_flag.h"
#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h"
#include "game.h"
#include "tf_weapon_builder.h"
#include "tf_obj.h"
#include "tf_ammo_pack.h"
#include "datacache/imdlcache.h"
#include "particle_parse.h"
#include "props_shared.h"
#include "filesystem.h"
#include "toolframework_server.h"
#include "IEffects.h"
#include "func_respawnroom.h"
#include "networkstringtable_gamedll.h"
#include "team_control_point_master.h"
#include "tf_weapon_pda.h"
#include "sceneentity.h"
#include "fmtstr.h"
#include "tf_weapon_sniperrifle.h"
#include "tf_weapon_minigun.h"
#include "tf_weapon_fists.h"
#include "tf_weapon_shotgun.h"
#include "tf_weapon_knife.h"
#include "tf_weapon_bottle.h"
#include "tf_weapon_grenade_pipebomb.h"
#include "tf_weapon_flamethrower.h"
#include "trigger_area_capture.h"
#include "triggers.h"
#include "tf_weapon_medigun.h"
#include "tf_weapon_invis.h"
#include "hl2orange.spa.h"
#include "te_tfblood.h"
#include "activitylist.h"
#include "cdll_int.h"
#include "tf_weaponbase_gun.h"
#include "team_train_watcher.h"
#include "vgui/ILocalize.h"
#include "tier3/tier3.h"
#include "serverbenchmark_base.h"
#include "trains.h"
#include "tf_fx.h"
#include "recipientfilter.h"
#include "ilagcompensationmanager.h"
#include "dt_utlvector_send.h"
#include "tf_item_wearable.h"
#include "tier0/vprof.h"
#include "tf_obj_sentrygun.h"
#include "tf_weapon_shovel.h"
#include "NextBotUtil.h"
#include "tier0/icommandline.h"
#include "entity_healthkit.h"
#include "choreoevent.h"
#include "tf_bot_temp.h"
#include "tf_objective_resource.h"
#include "tf_weapon_pipebomblauncher.h"
#include "func_achievement.h"
#include "inetchannel.h"
#include "soundenvelope.h"
#include "tf_triggers.h"
#include "collisionutils.h"
#include "eventlist.h"
#include "tf_weapon_knife.h"
#include "player_resource.h"
#include "tf_player_resource.h"
#include "vote_controller.h"

#include "tf_weapon_bonesaw.h"
#include "pointhurt.h"
#include "info_camera_link.h"

// NVNT haptic utils
#include "haptics/haptic_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#pragma warning( disable: 4355 ) // disables ' 'this' : used in base member initializer list'

ConVar sv_motd_unload_on_dismissal( "sv_motd_unload_on_dismissal", "0", 0, "If enabled, the MOTD contents will be unloaded when the player closes the MOTD." );

#define DAMAGE_FORCE_SCALE_SELF				9
#define SCOUT_ADD_BIRD_ON_GIB_CHANCE		5
#define MEDIC_RELEASE_DOVE_COUNT			10

#define JUMP_MIN_SPEED	268.3281572999747f		

extern bool IsInCommentaryMode( void );

extern ConVar	sk_player_head;
extern ConVar	sk_player_chest;
extern ConVar	sk_player_stomach;
extern ConVar	sk_player_arm;
extern ConVar	sk_player_leg;

extern ConVar	tf_spy_invis_time;
extern ConVar	tf_spy_invis_unstealth_time;
extern ConVar	tf_stalematechangeclasstime;
extern ConVar	tf_gravetalk;

extern ConVar	tf_bot_quota_mode;
extern ConVar	tf_bot_quota;
extern ConVar	halloween_starting_souls;

extern ConVar tf_powerup_mode_killcount_timer_length;

float GetCurrentGravity( void );

float			m_flNextReflectZap = 0.f;

bool CTFPlayer::m_bTFPlayerNeedsPrecache = true;

static const char g_pszIdleKickString[] = "#TF_Idle_kicked";

EHANDLE g_pLastSpawnPoints[TF_TEAM_COUNT];

EHANDLE	g_hTestSub;

ConVar tf_playerstatetransitions( "tf_playerstatetransitions", "-2", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "tf_playerstatetransitions <ent index or -1 for all>. Show player state transitions." );
ConVar tf_playergib( "tf_playergib", "1", FCVAR_NOTIFY, "Allow player gibbing. 0: never, 1: normal, 2: always", true, 0, true, 2 );

ConVar tf_damageforcescale_other( "tf_damageforcescale_other", "6.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_damageforcescale_self_soldier_rj( "tf_damageforcescale_self_soldier_rj", "10.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_damageforcescale_self_soldier_badrj( "tf_damageforcescale_self_soldier_badrj", "5.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_damageforcescale_pyro_jump( "tf_damageforcescale_pyro_jump", "8.5", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_damagescale_self_soldier( "tf_damagescale_self_soldier", "0.60", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );


ConVar tf_damage_range( "tf_damage_range", "0.5", FCVAR_DEVELOPMENTONLY );
ConVar tf_damage_multiplier_blue( "tf_damage_multiplier_blue", "1.0", FCVAR_CHEAT, "All incoming damage to a blue player is multiplied by this value" );
ConVar tf_damage_multiplier_red( "tf_damage_multiplier_red", "1.0", FCVAR_CHEAT, "All incoming damage to a red player is multiplied by this value" );


ConVar tf_max_voice_speak_delay( "tf_max_voice_speak_delay", "1.5", FCVAR_DEVELOPMENTONLY, "Max time after a voice command until player can do another one", true, 0.1f, false, 0.f );

ConVar tf_allow_player_use( "tf_allow_player_use", "0", FCVAR_NOTIFY, "Allow players to execute +use while playing." );

ConVar tf_deploying_bomb_time( "tf_deploying_bomb_time", "1.90", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Time to deploy bomb before the point of no return." );
ConVar tf_deploying_bomb_delay_time( "tf_deploying_bomb_delay_time", "0.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Time to delay before deploying bomb." );

ConVar tf_mvm_death_penalty( "tf_mvm_death_penalty", "0", FCVAR_NOTIFY | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "How much currency players lose when dying" );
extern ConVar tf_populator_damage_multiplier;
extern ConVar tf_mvm_skill;

ConVar tf_highfive_separation_forward( "tf_highfive_separation_forward", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Forward distance between high five partners" );
ConVar tf_highfive_separation_right( "tf_highfive_separation_right", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Right distance between high five partners" );

ConVar tf_highfive_max_range( "tf_highfive_max_range", "150", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "The farthest away a high five partner can be" );
ConVar tf_highfive_height_tolerance( "tf_highfive_height_tolerance", "12", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "The maximum height difference allowed for two high-fivers." );
ConVar tf_highfive_debug( "tf_highfive_debug", "0", FCVAR_NONE, "Turns on some console spew for debugging high five issues." );

ConVar tf_test_teleport_home_fx( "tf_test_teleport_home_fx", "0", FCVAR_CHEAT );

ConVar tf_halloween_giant_health_scale( "tf_halloween_giant_health_scale", "10", FCVAR_CHEAT );

ConVar tf_grapplinghook_los_force_detach_time( "tf_grapplinghook_los_force_detach_time", "1", FCVAR_CHEAT );
ConVar tf_powerup_max_charge_time( "tf_powerup_max_charge_time", "30", FCVAR_CHEAT );

extern ConVar tf_powerup_mode;

#define TF_CANNONBALL_FORCE_SCALE	80.f
#define TF_CANNONBALL_FORCE_UPWARD	300.f

ConVar tf_tauntcam_fov_override( "tf_tauntcam_fov_override", "0", FCVAR_CHEAT );

ConVar tf_nav_in_combat_range( "tf_nav_in_combat_range", "1000", FCVAR_CHEAT );

ConVar tf_maxhealth_drain_hp_min( "tf_maxhealth_drain_hp_min", "100", FCVAR_DEVELOPMENTONLY );
ConVar tf_maxhealth_drain_deploy_cost( "tf_maxhealth_drain_deploy_cost", "20", FCVAR_DEVELOPMENTONLY );

extern ConVar sv_vote_allow_spectators;
ConVar sv_vote_late_join_time( "sv_vote_late_join_time", "90", FCVAR_NONE, "Grace period after the match starts before players who join the match receive a vote-creation cooldown" );
ConVar sv_vote_late_join_cooldown( "sv_vote_late_join_cooldown", "300", FCVAR_NONE, "Length of the vote-creation cooldown when joining the server after the grace period has expired" );

extern ConVar tf_voice_command_suspension_mode;
extern ConVar tf_feign_death_duration;
extern ConVar spec_freeze_time;
extern ConVar spec_freeze_traveltime;
extern ConVar sv_maxunlag;
extern ConVar tf_allow_taunt_switch;
extern ConVar weapon_medigun_chargerelease_rate;
extern ConVar tf_scout_energydrink_consume_rate;
extern ConVar mp_spectators_restricted;
extern ConVar mp_teams_unbalance_limit;
extern ConVar tf_tournament_classchange_allowed;
extern ConVar tf_tournament_classchange_ready_allowed;
#if defined( _DEBUG ) || defined( STAGING_ONLY )
extern ConVar mp_developer;
extern ConVar bot_mimic;
#endif // _DEBUG || STAGING_ONLY 

extern CBaseEntity *FindPickerEntity( CBasePlayer *pPlayer );

// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //

class CTEPlayerAnimEvent : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEPlayerAnimEvent, CBaseTempEntity );
	DECLARE_SERVERCLASS();

	CTEPlayerAnimEvent( const char *name )
		: CBaseTempEntity( name )
	{
	}

	CNetworkHandle( CBasePlayer, m_hPlayer );
	CNetworkVar( int, m_iEvent );
	CNetworkVar( int, m_nData );
};

IMPLEMENT_SERVERCLASS_ST_NOBASE( CTEPlayerAnimEvent, DT_TEPlayerAnimEvent )
	SendPropEHandle( SENDINFO( m_hPlayer ) ),
	SendPropInt( SENDINFO( m_iEvent ), Q_log2( PLAYERANIMEVENT_COUNT ) + 1, SPROP_UNSIGNED ),
	// BUGBUG:  ywb  we assume this is either 0 or an animation sequence #, but it could also be an activity, which should fit within this limit, but we're not guaranteed.
	SendPropInt( SENDINFO( m_nData ), ANIMATION_SEQUENCE_BITS ),
END_SEND_TABLE()

static CTEPlayerAnimEvent g_TEPlayerAnimEvent( "PlayerAnimEvent" );

void TE_PlayerAnimEvent( CBasePlayer *pPlayer, PlayerAnimEvent_t event, int nData )
{
    Vector vecEyePos = pPlayer->EyePosition();
	CPVSFilter filter( vecEyePos );
	if ( !IsCustomPlayerAnimEvent( event ) && ( event != PLAYERANIMEVENT_SNAP_YAW ) && ( event != PLAYERANIMEVENT_VOICE_COMMAND_GESTURE ) )
	{
		// if prediction is off, alway send jump
		if ( !( ( event == PLAYERANIMEVENT_JUMP ) && ( FStrEq(engine->GetClientConVarValue( pPlayer->entindex(), "cl_predict" ), "0" ) ) ) )
		{
			filter.RemoveRecipient( pPlayer );
		}
	}

	Assert( pPlayer->entindex() >= 1 && pPlayer->entindex() <= MAX_PLAYERS );
	g_TEPlayerAnimEvent.m_hPlayer = pPlayer;
	g_TEPlayerAnimEvent.m_iEvent = event;
	Assert( nData < (1<<ANIMATION_SEQUENCE_BITS) );
	Assert( (1<<ANIMATION_SEQUENCE_BITS) >= ActivityList_HighestIndex() );
	g_TEPlayerAnimEvent.m_nData = nData;
	g_TEPlayerAnimEvent.Create( filter, 0 );
}

//=================================================================================
//
// Ragdoll Entity
//
class CTFRagdoll : public CBaseAnimatingOverlay
{
public:

	DECLARE_CLASS( CTFRagdoll, CBaseAnimatingOverlay );
	DECLARE_SERVERCLASS();

	CTFRagdoll()
	{
		m_bGib = false;
		m_bBurning = false;
		m_bWasDisguised = false;
		m_bOnGround = false;
		m_iDamageCustom = 0;
		m_vecRagdollOrigin.Init();
		m_vecRagdollVelocity.Init();
	}

	// Transmit ragdolls to everyone.
	virtual int UpdateTransmitState()
	{
		UseClientSideAnimation();
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

	CNetworkHandle( CBasePlayer, m_hPlayer );
	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
	CNetworkVar( bool, m_bGib );
	CNetworkVar( bool, m_bBurning );
	CNetworkVar( bool, m_bWasDisguised );
	CNetworkVar( bool, m_bOnGround );
	CNetworkVar( int, m_iDamageCustom );
	CNetworkVar( int, m_iTeam );
	CNetworkVar( int, m_iClass );
};

LINK_ENTITY_TO_CLASS( tf_ragdoll, CTFRagdoll );

IMPLEMENT_SERVERCLASS_ST_NOBASE( CTFRagdoll, DT_TFRagdoll )
	SendPropVector( SENDINFO( m_vecRagdollOrigin ), -1,  SPROP_COORD ),
	SendPropEHandle( SENDINFO ( m_hPlayer) ),
	SendPropVector	( SENDINFO(m_vecForce), -1, SPROP_NOSCALE ),
	SendPropVector( SENDINFO( m_vecRagdollVelocity ), 13, SPROP_ROUNDDOWN, -2048.0f, 2048.0f ),
	SendPropInt( SENDINFO( m_nForceBone ) ),
	SendPropBool( SENDINFO( m_bGib ) ),
	SendPropBool( SENDINFO( m_bBurning ) ),
	SendPropBool( SENDINFO( m_bWasDisguised ) ),
	SendPropBool( SENDINFO( m_bOnGround ) ),
	SendPropInt( SENDINFO( m_iDamageCustom ) ),
	SendPropInt( SENDINFO( m_iTeam ), 3, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iClass ), 4, SPROP_UNSIGNED ),
END_SEND_TABLE()

// -------------------------------------------------------------------------------- //
// Tables.
// -------------------------------------------------------------------------------- //

//-----------------------------------------------------------------------------
// Purpose: SendProxy that converts the UtlVector list of objects to entindexes, where it's reassembled on the client
//-----------------------------------------------------------------------------
void SendProxy_PlayerObjectList( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	CTFPlayer *pPlayer = (CTFPlayer*)pStruct;

	// If this fails, then SendProxyArrayLength_PlayerObjects didn't work.
	Assert( iElement < pPlayer->GetObjectCount() );

	CBaseObject *pObject = pPlayer->GetObject(iElement);

	EHANDLE hObject;
	hObject = pObject;

	SendProxy_EHandleToInt( pProp, pStruct, &hObject, pOut, iElement, objectID );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int SendProxyArrayLength_PlayerObjects( const void *pStruct, int objectID )
{
	CTFPlayer *pPlayer = (CTFPlayer*)pStruct;
	int iObjects = pPlayer->GetObjectCount();
	Assert( iObjects <= MAX_OBJECTS_PER_PLAYER );
	return iObjects;
}

//-----------------------------------------------------------------------------
// Purpose: Send to attached medics
//-----------------------------------------------------------------------------
void* SendProxy_SendHealersDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	CTFPlayer *pPlayer = (CTFPlayer*)pStruct;
	if ( pPlayer )
	{
		// Add attached medics
		for ( int i = 0; i < pPlayer->m_Shared.GetNumHealers(); i++ )
		{
			CTFPlayer *pMedic = ToTFPlayer( pPlayer->m_Shared.GetHealerByIndex( i ) );
			if ( !pMedic )
				continue;

			pRecipients->SetRecipient( pMedic->GetClientIndex() );
			return (void*)pVarData;
		}
	}

	return NULL;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendHealersDataTable );

BEGIN_DATADESC( CTFPlayer )
	DEFINE_INPUTFUNC( FIELD_VOID, "IgnitePlayer", InputIgnitePlayer ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetCustomModel", InputSetCustomModel ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetCustomModelWithClassAnimations", InputSetCustomModelWithClassAnimations ),
	DEFINE_INPUTFUNC( FIELD_VECTOR, "SetCustomModelOffset", InputSetCustomModelOffset ),
	DEFINE_INPUTFUNC( FIELD_VECTOR, "SetCustomModelRotation", InputSetCustomModelRotation ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ClearCustomModelRotation", InputClearCustomModelRotation ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetCustomModelRotates", InputSetCustomModelRotates ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetCustomModelVisibleToSelf", InputSetCustomModelVisibleToSelf ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetForcedTauntCam", InputSetForcedTauntCam ),
	DEFINE_INPUTFUNC( FIELD_STRING,	"SpeakResponseConcept",	InputSpeakResponseConcept ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundSpawn", InputRoundSpawn ),
END_DATADESC()

BEGIN_ENT_SCRIPTDESC( CTFPlayer, CBaseMultiplayerPlayer , "Team Fortress 2 Player" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetActiveWeapon, "GetActiveWeapon", "Get the player's current weapon" )

	DEFINE_SCRIPTFUNC( ForceRespawn, "Force respawns the player" )
	DEFINE_SCRIPTFUNC( ForceRegenerateAndRespawn, "Force regenerates and respawns the player" )
	DEFINE_SCRIPTFUNC( Regenerate, "Resupplies a player. If regen health/ammo is set, clears negative conds, gives back player health/ammo" )

	DEFINE_SCRIPTFUNC( HasItem, "Currently holding an item? Eg. capture flag" )
	DEFINE_SCRIPTFUNC( GetNextRegenTime, "Get next health regen time." )
	DEFINE_SCRIPTFUNC( SetNextRegenTime, "Set next health regen time." )
	DEFINE_SCRIPTFUNC( GetNextChangeClassTime, "Get next change class time." )
	DEFINE_SCRIPTFUNC( SetNextChangeClassTime, "Set next change class time." )
	DEFINE_SCRIPTFUNC( GetNextChangeTeamTime, "Get next change team time." )
	DEFINE_SCRIPTFUNC( SetNextChangeTeamTime, "Set next change team time." )
	DEFINE_SCRIPTFUNC( DropFlag, "Force player to drop the flag." )
	DEFINE_SCRIPTFUNC( ForceChangeTeam, "Force player to change their team." )
	DEFINE_SCRIPTFUNC( CanJump, "Can the player jump?" )
	DEFINE_SCRIPTFUNC( CanPlayerMove, "Can the player move?" )
	DEFINE_SCRIPTFUNC( RemoveAllObjects, "Remove all player objects. Eg. dispensers/sentries." )
	DEFINE_SCRIPTFUNC( IsPlacingSapper, "Returns true if we placed a sapper in the last few moments" )
	DEFINE_SCRIPTFUNC( IsSapping, "Returns true if we are currently sapping" )
	DEFINE_SCRIPTFUNC( RemoveInvisibility, "Un-invisible a spy." )
	DEFINE_SCRIPTFUNC( RemoveDisguise, "Undisguise a spy." )
	DEFINE_SCRIPTFUNC( IsCallingForMedic, "Is this player calling for medic?" )
	DEFINE_SCRIPTFUNC( GetTimeSinceCalledForMedic, "When did the player last call medic" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetHealTarget, "GetHealTarget", "Who is the medic healing?" )
	DEFINE_SCRIPTFUNC( GetClassEyeHeight, "Gets the eye height of the player" )
	DEFINE_SCRIPTFUNC( FiringTalk, "Makes eg. a heavy go AAAAAAAAAAaAaa like they are firing their minigun." )
	DEFINE_SCRIPTFUNC( CanAirDash, "" )
	DEFINE_SCRIPTFUNC( CanBreatheUnderwater, "" )
	DEFINE_SCRIPTFUNC( ApplyAbsVelocityImpulse, "" )
	DEFINE_SCRIPTFUNC( ApplyPunchImpulseX, "" )
	DEFINE_SCRIPTFUNC( IsAllowedToTaunt, "" )
	DEFINE_SCRIPTFUNC( IsRegenerating, "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptAddCond, "AddCond", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptAddCondEx, "AddCondEx", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptRemoveCond, "RemoveCond", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptRemoveCondEx, "RemoveCondEx", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptInCond, "InCond", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptWasInCond, "WasInCond", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptRemoveAllCond,"RemoveAllCond", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetCondDuration, "GetCondDuration", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetCondDuration, "SetCondDuration", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetDisguiseTarget, "GetDisguiseTarget", "" )

	DEFINE_SCRIPTFUNC_NAMED( ScriptIsInvulnerable, "IsInvulnerable", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsStealthed, "IsStealthed", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptCanBeDebuffed, "CanBeDebuffed", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetDisguiseAmmoCount, "GetDisguiseAmmoCount", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetDisguiseAmmoCount, "SetDisguiseAmmoCount", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetDisguiseTeam, "GetDisguiseTeam", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsFullyInvisible, "IsFullyInvisible", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetSpyCloakMeter, "GetSpyCloakMeter", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetSpyCloakMeter, "SetSpyCloakMeter", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsJumping, "IsJumping", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsAirDashing, "IsAirDashing", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetCaptures, "GetCaptures", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetDefenses, "GetDefenses", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetDominations, "GetDominations", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetRevenge, "GetRevenge", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetBuildingsDestroyed, "GetBuildingsDestroyed", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetHeadshots, "GetHeadshots", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetBackstabs, "GetBackstabs", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetHealPoints, "GetHealPoints", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetInvulns, "GetInvulns", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetTeleports, "GetTeleports", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetResupplyPoints, "GetResupplyPoints", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetKillAssists, "GetKillAssists", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetBonusPoints, "GetBonusPoints", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptResetScores, "ResetScores", "" )

	DEFINE_SCRIPTFUNC( IgnitePlayer, "" )
	DEFINE_SCRIPTFUNC( SetCustomModel, "" )
	DEFINE_SCRIPTFUNC( SetCustomModelWithClassAnimations, "" )
	DEFINE_SCRIPTFUNC( SetCustomModelOffset, "" )
	DEFINE_SCRIPTFUNC( SetCustomModelRotation, "" )
	DEFINE_SCRIPTFUNC( ClearCustomModelRotation, "" )
	DEFINE_SCRIPTFUNC( SetCustomModelRotates, "" )
	DEFINE_SCRIPTFUNC( SetCustomModelVisibleToSelf, "" )
	DEFINE_SCRIPTFUNC( SetForcedTauntCam, "" )

	DEFINE_SCRIPTFUNC_NAMED( ScriptGetPlayerClass, "GetPlayerClass", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetPlayerClass, "SetPlayerClass", "" )

	DEFINE_SCRIPTFUNC( RemoveTeleportEffect, "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptRemoveAllItems, "RemoveAllItems", "" )

	DEFINE_SCRIPTFUNC( UpdateSkin, "" )
	DEFINE_SCRIPTFUNC_WRAPPED( Weapon_ShootPosition, "" ) // Needs this slim wrapper or the world falls apart on MSVC.
	DEFINE_SCRIPTFUNC_WRAPPED( Weapon_CanUse, "" )
	DEFINE_SCRIPTFUNC_WRAPPED( Weapon_Equip, "" )
	DEFINE_SCRIPTFUNC_WRAPPED( Weapon_Drop, "" )
	DEFINE_SCRIPTFUNC_WRAPPED( Weapon_DropEx, "" )
	DEFINE_SCRIPTFUNC_WRAPPED( Weapon_Switch, "" )
	DEFINE_SCRIPTFUNC_WRAPPED( Weapon_SetLast, "" )
	DEFINE_SCRIPTFUNC_WRAPPED( GetLastWeapon, "" )

	DEFINE_SCRIPTFUNC_WRAPPED( IsFakeClient, "" )
	DEFINE_SCRIPTFUNC_WRAPPED( GetBotType, "" )
	DEFINE_SCRIPTFUNC_WRAPPED( IsBotOfType, "" )

	DEFINE_SCRIPTFUNC( AddHudHideFlags, "Hides a hud element based on Constants.FHideHUD." )
	DEFINE_SCRIPTFUNC( RemoveHudHideFlags, "Unhides a hud element based on Constants.FHideHUD." )
	DEFINE_SCRIPTFUNC( SetHudHideFlags, "Force hud hide flags to a value" )
	DEFINE_SCRIPTFUNC( GetHudHideFlags, "Gets current hidden hud elements" )

	DEFINE_SCRIPTFUNC( IsTaunting, "" )
	DEFINE_SCRIPTFUNC( CancelTaunt, "" )
	DEFINE_SCRIPTFUNC( StopTaunt, "" )
	DEFINE_SCRIPTFUNC( HandleTauntCommand, "" )
	DEFINE_SCRIPTFUNC_WRAPPED( Taunt, "" )
END_SCRIPTDESC();


EXTERN_SEND_TABLE( DT_ScriptCreatedItem );

// specific to the local player
BEGIN_SEND_TABLE_NOBASE( CTFPlayer, DT_TFLocalPlayerExclusive )
	// send a hi-res origin to the local player for use in prediction
	SendPropVectorXY(SENDINFO(m_vecOrigin),               -1, SPROP_NOSCALE|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_OriginXY ),
	SendPropFloat   (SENDINFO_VECTORELEM(m_vecOrigin, 2), -1, SPROP_NOSCALE|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_OriginZ ),
	SendPropArray2( 
		SendProxyArrayLength_PlayerObjects,
		SendPropInt("player_object_array_element", 0, SIZEOF_IGNORE, NUM_NETWORKED_EHANDLE_BITS, SPROP_UNSIGNED, SendProxy_PlayerObjectList), 
		MAX_OBJECTS_PER_PLAYER, 
		0, 
		"player_object_array"
		),

	SendPropFloat( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 1), 10, SPROP_CHANGES_OFTEN ),

	SendPropInt( SENDINFO( m_nExperienceLevel ), 7, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nExperienceLevelProgress ), 7, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bMatchSafeToLeave ) ),

END_SEND_TABLE()

// all players except the local player
BEGIN_SEND_TABLE_NOBASE( CTFPlayer, DT_TFNonLocalPlayerExclusive )
	// send a lo-res origin to other players
	SendPropVectorXY(SENDINFO(m_vecOrigin),               -1, SPROP_COORD_MP_LOWPRECISION|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_OriginXY ),
	SendPropFloat   (SENDINFO_VECTORELEM(m_vecOrigin, 2), -1, SPROP_COORD_MP_LOWPRECISION|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_OriginZ ),

	SendPropFloat( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 1), 10, SPROP_CHANGES_OFTEN ),

END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Sent to attached medics
//-----------------------------------------------------------------------------
BEGIN_SEND_TABLE_NOBASE( CTFPlayer, DT_TFSendHealersDataTable )
	SendPropInt( SENDINFO( m_nActiveWpnClip ), -1, SPROP_VARINT | SPROP_UNSIGNED ),
END_SEND_TABLE()

//============

LINK_ENTITY_TO_CLASS( player, CTFPlayer );
PRECACHE_REGISTER(player);

IMPLEMENT_SERVERCLASS_ST( CTFPlayer, DT_TFPlayer )
	SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
	SendPropExclude( "DT_BaseAnimating", "m_flPlaybackRate" ),	
	SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
	SendPropExclude( "DT_BaseAnimating", "m_nBody" ),
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),
	SendPropExclude( "DT_BaseAnimatingOverlay", "overlay_vars" ),
	SendPropExclude( "DT_BaseEntity", "m_nModelIndex" ),
	SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),

	// cs_playeranimstate and clientside animation takes care of these on the client
	SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),	
	SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),

	SendPropExclude( "DT_BaseFlex", "m_flexWeight" ),
	SendPropExclude( "DT_BaseFlex", "m_blinktoggle" ),
	SendPropExclude( "DT_BaseFlex", "m_viewtarget" ),

	SendPropBool(SENDINFO(m_bSaveMeParity)),

	// This will create a race condition will the local player, but the data will be the same so.....
	SendPropInt( SENDINFO( m_nWaterLevel ), 2, SPROP_UNSIGNED ),

	// Ragdoll.
	SendPropEHandle( SENDINFO( m_hRagdoll ) ),
	SendPropDataTable( SENDINFO_DT( m_PlayerClass ), &REFERENCE_SEND_TABLE( DT_TFPlayerClassShared ) ),
	SendPropDataTable( SENDINFO_DT( m_Shared ), &REFERENCE_SEND_TABLE( DT_TFPlayerShared ) ),
	SendPropEHandle(SENDINFO(m_hItem)),

	// Data that only gets sent to the local player
	SendPropDataTable( "tflocaldata", 0, &REFERENCE_SEND_TABLE(DT_TFLocalPlayerExclusive), SendProxy_SendLocalDataTable ),

	// Data that gets sent to all other players
	SendPropDataTable( "tfnonlocaldata", 0, &REFERENCE_SEND_TABLE(DT_TFNonLocalPlayerExclusive), SendProxy_SendNonLocalDataTable ),

	SendPropInt( SENDINFO( m_nForceTauntCam ), 2, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_flTauntYaw ), 0, SPROP_NOSCALE ),

	SendPropInt( SENDINFO( m_iSpawnCounter ) ),
	SendPropBool( SENDINFO( m_bFlipViewModels ) ),

	SendPropBool( SENDINFO( m_bUsingVRHeadset ) ),

	SendPropBool( SENDINFO( m_bForcedSkin ) ),
	SendPropInt( SENDINFO( m_nForcedSkin ), ANIMATION_SKIN_BITS ),

	SendPropDataTable( "TFSendHealersDataTable", 0, &REFERENCE_SEND_TABLE( DT_TFSendHealersDataTable ), SendProxy_SendHealersDataTable ),

	SendPropEHandle( SENDINFO( m_hSecondaryLastWeapon ) ),
	SendPropFloat( SENDINFO( m_flHelpmeButtonPressTime ) ),
	SendPropBool( SENDINFO( m_bRegenerating ) ),
END_SEND_TABLE()

// -------------------------------------------------------------------------------- //

void cc_CreatePredictionError_f()
{
	CBaseEntity *pEnt = CBaseEntity::Instance( 1 );
	pEnt->SetAbsOrigin( pEnt->GetAbsOrigin() + Vector( 63, 0, 0 ) );
}
ConCommand cc_CreatePredictionError( "CreatePredictionError", cc_CreatePredictionError_f, "Create a prediction error", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayer::CTFPlayer() 
{
	m_PlayerAnimState = CreateTFPlayerAnimState( this );

	SetArmorValue( 10 );

	m_hItem = NULL;
	m_hTauntScene = NULL;

	m_flTauntNextStartTime = -1.f;

	UseClientSideAnimation();
	m_angEyeAngles.Init();
	m_pStateInfo = NULL;
	m_lifeState = LIFE_DEAD; // Start "dead".
	m_iMaxSentryKills = 0;

	m_flNextTimeCheck = gpGlobals->curtime;
	m_flSpawnTime = 0;

	m_flWaterExitTime = 0;

	SetViewOffset( TF_PLAYER_VIEW_OFFSET );

	m_Shared.Init( this );

	m_iLastSkin = -1;

	m_bHudClassAutoKill = false;
	m_bMedigunAutoHeal = false;

	m_vecLastDeathPosition = Vector( FLT_MAX, FLT_MAX, FLT_MAX );

	SetDesiredPlayerClassIndex( TF_CLASS_UNDEFINED );

	SetContextThink( &CTFPlayer::TFPlayerThink, gpGlobals->curtime, "TFPlayerThink" );

	m_flLastAction = gpGlobals->curtime;
	m_flTimeInSpawn = 0;

	m_bInitTaunt = false;

	m_bSpeakingConceptAsDisguisedSpy = false;

	m_iPreviousteam = TEAM_UNASSIGNED;

	m_bArenaIsAFK = false;
	m_bIsAFK = false;

	m_nDeployingBombState = TF_BOMB_DEPLOYING_NONE;

	m_flNextChangeClassTime = 0.0f;
	m_flNextChangeTeamTime = 0.0f;

	m_iLastWeaponSlot = 1;
	m_iNumberofDominations = 0;
	m_bFlipViewModels = false;
	m_fMaxHealthTime = -1;
	m_iHealthBefore = 0;

	m_iTeamChanges = 0;
	m_iClassChanges = 0;

	// Bounty Mode
	m_nExperienceLevel = 1;
	m_nExperiencePoints = 0;
	m_nExperienceLevelProgress = 0;

	SetDefLessFunc( m_Cappers );		// Tracks victims for demo achievement

	m_bIsTargetDummy = false;
	
	m_bCollideWithSentry = false;

	m_nForceTauntCam = 0;

	m_flLastThinkTime = -1.f;

	m_flLastReadySoundTime = 0.f;

	m_damageRateArray = new int[ DPS_Period ];
	ResetDamagePerSecond();

	m_nActiveWpnClip.Set( 0 );
	m_nActiveWpnClipPrev = 0;
	m_flNextClipSendTime = 0;

	m_fLastBombHeadTimestamp = 0.0f;

	m_bIsSapping = false;
	m_iSappingEvent = TF_SAPEVENT_NONE;
	m_flSapStartTime = 0.00;

	m_bUsingVRHeadset = false;

	m_bForcedSkin = false;
	m_nForcedSkin = 0;

	SetRespawnOverride( -1.f, NULL_STRING );

	m_nPrevRoundTeamNum = TEAM_UNASSIGNED;
	m_flLastDamageResistSoundTime = -1.f;
	m_hLastDamageDoneEntity = NULL;

	SetDefLessFunc( m_PlayersExtinguished );

	m_flLastAutobalanceTime = 0.f;

	m_bRegenerating = false;
	m_bRespawning = false;

	m_bAlreadyUsedExtendFreezeThisDeath = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ForcePlayerViewAngles( const QAngle& qTeleportAngles )
{
	CSingleUserRecipientFilter filter( this );

	UserMessageBegin( filter, "ForcePlayerViewAngles" );
	WRITE_BYTE( 0x01 ); // Reserved space for flags.
	WRITE_BYTE( entindex() );
	WRITE_ANGLES( qTeleportAngles );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanBeForcedToLaugh( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::TFPlayerThink()
{
	if ( m_pStateInfo && m_pStateInfo->pfnThink )
	{
		(this->*m_pStateInfo->pfnThink)();
	}

	// Time to finish the current random expression? Or time to pick a new one?
	if ( IsAlive() && ( m_flNextSpeakWeaponFire < gpGlobals->curtime ) && m_flNextRandomExpressionTime >= 0 && gpGlobals->curtime > m_flNextRandomExpressionTime )
	{
		// Random expressions need to be cleared, because they don't loop. So if we
		// pick the same one again, we want to restart it.
		ClearExpression();
		m_iszExpressionScene = NULL_STRING;
		UpdateExpression();
	}

	CBaseEntity *pGroundEntity = GetGroundEntity();

	// We consider players "in air" if they have no ground entity and they're not in water.
	if ( pGroundEntity == NULL && GetWaterLevel() == WL_NotInWater )
	{
		if ( m_iLeftGroundHealth < 0 )
		{
			m_iLeftGroundHealth = GetHealth();
		}
	}
	else
	{
		m_iLeftGroundHealth = -1;
	}

	if( IsTaunting() )
	{
		bool bStopTaunt = false;
		// if I'm not supposed to move during taunt
		// stop taunting if I lost my ground entity or was moved at all
		if ( !CanMoveDuringTaunt() )
		{
			bStopTaunt |= pGroundEntity == NULL;
		}

		if ( !bStopTaunt  )
		{
			bStopTaunt |= ShouldStopTaunting();
		}

		if ( bStopTaunt )
		{
			CancelTaunt();
		}
	}

	if ( !m_bCollideWithSentry )
	{
		if ( IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			CBaseObject	*pSentry = GetObjectOfType( OBJ_SENTRYGUN );
			if ( !pSentry )
			{
				m_bCollideWithSentry = true;
			}
			else
			{
				if ( ( pSentry->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr() > 2500 )
				{
					m_bCollideWithSentry = true;
				}
			}
		}
		else
		{
			m_bCollideWithSentry = true;
		}
	}

	// Send active weapon's clip state to attached medics
	bool bSendClipInfo = gpGlobals->curtime > m_flNextClipSendTime &&
						 m_Shared.GetNumHealers() &&
						 IsAlive();
	if ( bSendClipInfo )
	{
		CTFWeaponBase *pTFWeapon = GetActiveTFWeapon();
		if ( pTFWeapon )
		{
			int nClip = 0;

			if ( m_Shared.InCond( TF_COND_DISGUISED ) )
			{
				nClip = m_Shared.GetDisguiseAmmoCount();
			}
			else
			{
				nClip = pTFWeapon->UsesClipsForAmmo1() ? pTFWeapon->Clip1() : GetAmmoCount( pTFWeapon->GetPrimaryAmmoType() );
			}

			if ( nClip >= 0 && nClip != m_nActiveWpnClipPrev )
			{
				if ( nClip > 500 )
				{
					Warning( "Heal Target: ClipSize Data Limit Exceeded: %d (max 500)\n", nClip );
					nClip = MIN( nClip, 500 );
				}
				m_nActiveWpnClip.Set( nClip );
				m_nActiveWpnClipPrev = m_nActiveWpnClip;
				m_flNextClipSendTime = gpGlobals->curtime + 0.25f;
			}
		}
	}

	SetContextThink( &CTFPlayer::TFPlayerThink, gpGlobals->curtime, "TFPlayerThink" );
	m_flLastThinkTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: Returns a portion of health every think.
//-----------------------------------------------------------------------------
void CTFPlayer::RegenThink( void )
{
	if ( !IsAlive() )
		return;

	// Queue the next think
	SetContextThink( &CTFPlayer::RegenThink, gpGlobals->curtime + TF_REGEN_TIME, "RegenThink" );

	// if we're going in to this too often, quit out.
	if ( m_flLastHealthRegenAt + TF_REGEN_TIME > gpGlobals->curtime )
		return;

	bool bShowRegen = true;

	// Medic has a base regen amount
	if ( GetPlayerClass()->GetClassIndex() == TF_CLASS_MEDIC )
	{
		// Heal faster if we haven't been in combat for a while.
		float flTimeSinceDamage = gpGlobals->curtime - GetLastDamageReceivedTime();
		float flScale = RemapValClamped( flTimeSinceDamage, 5.0f, 10.0f, 1.0f, 2.0f );
		float flRegenAmt = TF_REGEN_AMOUNT;

		// If you are healing a hurt patient, increase your base regen
		CTFPlayer *pPatient = ToTFPlayer( MedicGetHealTarget() );
		if ( pPatient && pPatient->GetHealth() < pPatient->GetMaxHealth() )
		{
			// Double regen amount
			flRegenAmt += TF_REGEN_AMOUNT;
		}

		flRegenAmt *= flScale;

		m_flAccumulatedHealthRegen += flRegenAmt;

		bShowRegen = false;
	}

	int nHealAmount = 0;
	if ( m_flAccumulatedHealthRegen >= 1.f )
	{
		nHealAmount = floor( m_flAccumulatedHealthRegen );
		if ( GetHealth() < GetMaxHealth() )
		{
			int nHealedAmount = TakeHealth( nHealAmount, DMG_GENERIC | DMG_IGNORE_DEBUFFS );
			if ( nHealedAmount > 0 )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "player_healed" );
				if ( event )
				{
					event->SetInt( "priority", 1 );	// HLTV event priority
					event->SetInt( "patient", GetUserID() );
					event->SetInt( "healer", GetUserID() );
					event->SetInt( "amount", nHealedAmount );
					gameeventmanager->FireEvent( event );
				}
			}
		}
	}
	else if ( m_flAccumulatedHealthRegen < -1.f )
	{
		nHealAmount = ceil( m_flAccumulatedHealthRegen );
		TakeDamage( CTakeDamageInfo( this, this, NULL, vec3_origin, WorldSpaceCenter(), nHealAmount * -1, DMG_GENERIC ) );
	}

	if ( GetHealth() < GetMaxHealth() && nHealAmount != 0 && bShowRegen )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "player_healonhit" );
		if ( event )
		{
			event->SetInt( "amount", nHealAmount );
			event->SetInt( "entindex", entindex() );
			gameeventmanager->FireEvent( event ); 
		}
	}

	m_flAccumulatedHealthRegen -= nHealAmount;
	m_flLastHealthRegenAt = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::RegenAmmoInternal( int iIndex, float flRegen )
{
	m_flAccumulatedAmmoRegens[iIndex] += flRegen;
	
	// As soon as we have enough accumulated to regen a single unit of ammo, do it.
	int iMaxAmmo = GetMaxAmmo(iIndex);
	int iAmmo = m_flAccumulatedAmmoRegens[iIndex] * iMaxAmmo;
	if ( iAmmo >= 1 )
	{
		GiveAmmo( iAmmo, iIndex, true );
		m_flAccumulatedAmmoRegens[iIndex] -= ((float)iAmmo / (float)iMaxAmmo);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayer::~CTFPlayer()
{
	delete [] m_damageRateArray;

	DestroyRagdoll();
	m_PlayerAnimState->Release();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayer *CTFPlayer::CreatePlayer( const char *className, edict_t *ed )
{
	CTFPlayer::s_PlayerEdict = ed;
	return (CTFPlayer*)CreateEntityByName( className );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::UpdateTimers( void )
{
	m_Shared.SharedThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::ProcessSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event )
{
	// TF Players only process scene events on the server while running taunts
	if ( !IsTaunting() )
		return false;

	// Only process sequences
	if ( event->GetType() != CChoreoEvent::SEQUENCE )
		return false;

	return BaseClass::ProcessSceneEvent( info, scene, event );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PreThink()
{
	// Update timers.
	UpdateTimers();

	// Pass through to the base class think.
	BaseClass::PreThink();

	// Reset bullet force accumulator, only lasts one frame, for ragdoll forces from multiple shots.
	m_vecTotalBulletForce = vec3_origin;

	CheckForIdle();

	ProcessSceneEvents();

	if ( TFGameRules()->IsInArenaMode() == true )
	{
		if ( TFGameRules()->State_Get() != GR_STATE_TEAM_WIN )
		{
			if ( GetTeamNumber() == TEAM_SPECTATOR )
			{
				m_Local.m_iHideHUD &= ~HIDEHUD_MISCSTATUS;
			}
		}
	}
}

ConVar mp_idledealmethod( "mp_idledealmethod", "1", FCVAR_GAMEDLL, "Deals with Idle Players. 1 = Sends them into Spectator mode then kicks them if they're still idle, 2 = Kicks them out of the game;" );
ConVar mp_idlemaxtime( "mp_idlemaxtime", "3", FCVAR_GAMEDLL, "Maximum time a player is allowed to be idle (in minutes)" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::CheckForIdle( void )
{
	if ( m_afButtonLast != m_nButtons )
		m_flLastAction = gpGlobals->curtime;

	if ( mp_idledealmethod.GetInt() )
	{
		if ( IsHLTV() || IsReplay() )
			return;

		if ( IsFakeClient() )
			return;

		if ( TFGameRules() && TFGameRules()->ShowMatchSummary() )
			return;

		if ( TFGameRules()->State_Get() == GR_STATE_BETWEEN_RNDS )
			return;

		//Don't mess with the host on a listen server (probably one of us debugging something)
		if ( engine->IsDedicatedServer() == false && entindex() == 1 )
			return;

		if ( IsAutoKickDisabled() )
			return;

		const bool cbMoving = ( m_nButtons & ( IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT ) ) != 0;

		m_bIsAFK = false;

		if ( !cbMoving && PointInRespawnRoom( this, WorldSpaceCenter() ) )
		{
			m_flTimeInSpawn += TICK_INTERVAL;
		}
		else
			m_flTimeInSpawn = 0;

		if ( TFGameRules()->IsInArenaMode() && tf_arena_use_queue.GetBool() == true )
		{
			if ( GetTeamNumber() == TEAM_SPECTATOR )
				return;

			if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN && TFGameRules()->GetWinningTeam() != GetTeamNumber() )
			{
				if ( m_bArenaIsAFK )
				{
					m_bIsAFK = true;
					m_bArenaIsAFK = false;
				}
			}
		}
		else
		{
			// Cannot possibly get out of the spawn room in 0 seconds--so if the ConVar says 0, let's assume 30 seconds.
			float flIdleTime = Max( mp_idlemaxtime.GetFloat() * 60, 30.0f );

			if ( TFGameRules()->InStalemate() )
			{
				flIdleTime = mp_stalemate_timelimit.GetInt() * 0.5f;
			}

			m_bIsAFK = ( gpGlobals->curtime - m_flLastAction ) > flIdleTime
			        || ( m_flTimeInSpawn > flIdleTime ); 
		}
		
		if ( m_bIsAFK == true )
		{
			bool bKickPlayer = false;

			ConVarRef mp_allowspectators( "mp_allowspectators" );
			if ( ( mp_allowspectators.IsValid() && mp_allowspectators.GetBool() == false ) || ( TFGameRules()->IsInArenaMode() && tf_arena_use_queue.GetBool() ) )
			{
				// just kick the player if this server doesn't allow spectators
				bKickPlayer = true;
			}
			else if ( mp_idledealmethod.GetInt() == 1 )
			{
				if ( GetTeamNumber() < FIRST_GAME_TEAM )
				{
					bKickPlayer = true;
				}
				else
				{
					//First send them into spectator mode then kick him.
					ForceChangeTeam( TEAM_SPECTATOR );
					m_flLastAction = gpGlobals->curtime;
					m_flTimeInSpawn = 0;
					return;
				}
			}
			else if ( mp_idledealmethod.GetInt() == 2 )
			{
				bKickPlayer = true;
			}

			if ( bKickPlayer == true )
			{
				UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "#game_idle_kick", GetPlayerName() );
				engine->ServerCommand( UTIL_VarArgs( "kickid %d %s\n", GetUserID(), g_pszIdleKickString ) );
				m_flLastAction = gpGlobals->curtime;
				m_flTimeInSpawn = 0;
			}
		}
	}
}

extern ConVar flashlight;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayer::FlashlightIsOn( void )
{
	return IsEffectActive( EF_DIMLIGHT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::FlashlightTurnOn( void )
{
	if( flashlight.GetInt() > 0 && IsAlive() )
	{
		AddEffects( EF_DIMLIGHT );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::FlashlightTurnOff( void )
{
	if( IsEffectActive(EF_DIMLIGHT) )
	{
		RemoveEffects( EF_DIMLIGHT );
	}	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PostThink()
{
	BaseClass::PostThink();

	QAngle angles = GetLocalAngles();
	angles[PITCH] = 0;
	SetLocalAngles( angles );
	
	// Store the eye angles pitch so the client can compute its animation state correctly.
	m_angEyeAngles = EyeAngles();

    m_PlayerAnimState->Update( m_angEyeAngles[YAW], m_angEyeAngles[PITCH] );
}

//-----------------------------------------------------------------------------
// Purpose: Precache the player models and player model gibs.
//-----------------------------------------------------------------------------
void CTFPlayer::PrecachePlayerModels( void )
{
	int i;
	for ( i = 0; i < TF_CLASS_COUNT_ALL; i++ )
	{
		const char *pszModel = GetPlayerClassData( i )->m_szModelName;
		if ( pszModel && pszModel[0] )
		{
			int iModel = PrecacheModel( pszModel );
			PrecacheGibsForModel( iModel );
		}

		pszModel = GetPlayerClassData( i )->m_szHandModelName;
		if ( pszModel && pszModel[0] )
		{
			PrecacheModel( pszModel );
		}

/*
		if ( !IsX360() )
		{
			// Precache the hardware facial morphed models as well.
			const char *pszHWMModel = GetPlayerClassData( i )->m_szHWMModelName;
			if ( pszHWMModel && pszHWMModel[0] )
			{
				PrecacheModel( pszHWMModel );
			}
		}
*/
	}
	
	// Always precache the silly gibs.
	for ( i = 4; i < ARRAYSIZE( g_pszBDayGibs ); ++i )
	{
		PrecacheModel( g_pszBDayGibs[i] );
	}

	if ( TFGameRules() && TFGameRules()->IsBirthday() )
	{
		for ( i = 0; i < 4/*ARRAYSIZE(g_pszBDayGibs)*/; i++ )
		{
			PrecacheModel( g_pszBDayGibs[i] );
		}
		PrecacheModel( "models/effects/bday_hat.mdl" );
	}

	// Precache player class sounds
	for ( i = TF_FIRST_NORMAL_CLASS; i < TF_CLASS_COUNT_ALL; ++i )
	{
		TFPlayerClassData_t *pData = GetPlayerClassData( i );

		for ( int i = 0; i < ARRAYSIZE( pData->m_szDeathSound ); ++i )
		{
			PrecacheScriptSound( pData->m_szDeathSound[ i ] ); 
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PrecacheTFPlayer()
{
	VPROF_BUDGET( "CTFPlayer::PrecacheTFPlayer", VPROF_BUDGETGROUP_PLAYER );
	if( !m_bTFPlayerNeedsPrecache )
		return;

	m_bTFPlayerNeedsPrecache = false;

	// Precache the player models and gibs.
	PrecachePlayerModels();

	// Precache the player sounds.
	PrecacheScriptSound( "Player.Spawn" );
	PrecacheScriptSound( "TFPlayer.Pain" );
	PrecacheScriptSound( "TFPlayer.CritHit" );
	PrecacheScriptSound( "TFPlayer.CritHitMini" );
	PrecacheScriptSound( "TFPlayer.DoubleDonk" );
	PrecacheScriptSound( "TFPlayer.CritPain" );
	PrecacheScriptSound( "TFPlayer.CritDeath" );
	PrecacheScriptSound( "TFPlayer.FreezeCam" );
	PrecacheScriptSound( "TFPlayer.Drown" );
	PrecacheScriptSound( "TFPlayer.AttackerPain" );
	PrecacheScriptSound( "TFPlayer.SaveMe" );
	PrecacheScriptSound( "TFPlayer.CritBoostOn" );
	PrecacheScriptSound( "TFPlayer.CritBoostOff" );
	PrecacheScriptSound( "TFPlayer.Decapitated" );
	PrecacheScriptSound( "TFPlayer.ReCharged" );
	PrecacheScriptSound( "Camera.SnapShot" );
	PrecacheScriptSound( "TFPlayer.Dissolve" );

	PrecacheScriptSound( "Saxxy.TurnGold" );

	PrecacheScriptSound( "Icicle.TurnToIce" );
	PrecacheScriptSound( "Icicle.HitWorld" );
	PrecacheScriptSound( "Icicle.Melt" );

	PrecacheScriptSound( "DemoCharge.ChargeCritOn" );
	PrecacheScriptSound( "DemoCharge.ChargeCritOff" );
	PrecacheScriptSound( "DemoCharge.Charging" );

	PrecacheScriptSound( "TFPlayer.StunImpactRange" );
	PrecacheScriptSound( "TFPlayer.StunImpact" );
	PrecacheScriptSound( "Halloween.PlayerScream" );
	PrecacheScriptSound( "Halloween.PlayerEscapedUnderworld" );

	PrecacheScriptSound( "Game.YourTeamLost" );
	PrecacheScriptSound( "Game.YourTeamWon" );
	PrecacheScriptSound( "Game.SuddenDeath" );
	PrecacheScriptSound( "Game.Stalemate" );
	PrecacheScriptSound( "TV.Tune" );

	//This will be moved out once we do the announcer pass.
	PrecacheScriptSound( "Announcer.AM_FirstBloodRandom" );
	PrecacheScriptSound( "Announcer.AM_CapEnabledRandom" );
	PrecacheScriptSound( "Announcer.AM_RoundStartRandom" );
	PrecacheScriptSound( "Announcer.AM_FirstBloodFast" );
	PrecacheScriptSound( "Announcer.AM_FirstBloodFinally" );
	PrecacheScriptSound( "Announcer.AM_FlawlessVictoryRandom" );
	PrecacheScriptSound( "Announcer.AM_FlawlessDefeatRandom" );
	PrecacheScriptSound( "Announcer.AM_FlawlessVictory01" );
	PrecacheScriptSound( "Announcer.AM_TeamScrambleRandom" );
	PrecacheScriptSound( "Taunt.MedicHeroic" );
	PrecacheScriptSound( "Taunt.GuitarRiff" );

	// Dmg absorb sound
	PrecacheScriptSound( "Powerup.ReducedDamage" );

	// Tourney UI
	PrecacheScriptSound( "Tournament.PlayerReady" );

	PrecacheScriptSound( "Medic.AutoCallerAnnounce" );

	PrecacheScriptSound( "Player.FallDamageIndicator" );

	PrecacheScriptSound( "Player.FallDamageDealt" );


	// Precache particle systems
	PrecacheParticleSystem( "crit_text" );
	PrecacheParticleSystem( "miss_text" );
	PrecacheParticleSystem( "cig_smoke" );
	PrecacheParticleSystem( "speech_mediccall" );
	PrecacheParticleSystem( "speech_mediccall_auto" );
	PrecacheParticleSystem( "speech_taunt_all" );
	PrecacheParticleSystem( "speech_taunt_red" );
	PrecacheParticleSystem( "speech_taunt_blue" );
	PrecacheParticleSystem( "player_recent_teleport_blue" );
	PrecacheParticleSystem( "player_recent_teleport_red" );
	PrecacheParticleSystem( "particle_nemesis_red" );
	PrecacheParticleSystem( "particle_nemesis_blue" );
	PrecacheParticleSystem( "spy_start_disguise_red" );
	PrecacheParticleSystem( "spy_start_disguise_blue" );
	PrecacheParticleSystem( "burningplayer_red" );
	PrecacheParticleSystem( "burningplayer_blue" );
	PrecacheParticleSystem( "burningplayer_rainbow" );
	PrecacheParticleSystem( "blood_spray_red_01" );
	PrecacheParticleSystem( "blood_spray_red_01_far" );
	PrecacheParticleSystem( "pyrovision_blood" );

	PrecacheParticleSystem( "water_blood_impact_red_01" );
	PrecacheParticleSystem( "blood_impact_red_01" );
	PrecacheParticleSystem( "water_playerdive" );
	PrecacheParticleSystem( "water_playeremerge" );
	PrecacheParticleSystem( "healthgained_red" );
	PrecacheParticleSystem( "healthgained_blu" );
	PrecacheParticleSystem( "healthgained_red_large" );
	PrecacheParticleSystem( "healthgained_blu_large" );
	PrecacheParticleSystem( "healthgained_red_giant" );
	PrecacheParticleSystem( "healthgained_blu_giant" );


	PrecacheModel( "effects/beam001_red.vmt" );
	PrecacheModel( "effects/beam001_blu.vmt" );
	PrecacheModel( "effects/beam001_white.vmt" );

	PrecacheScriptSound( "BlastJump.Whistle" );

	PrecacheScriptSound( "Spy.TeaseVictim" );


	// precache the EOTL bomb cart replacements
	PrecacheModel( "models/props_trainyard/bomb_eotl_blue.mdl" );
	PrecacheModel( "models/props_trainyard/bomb_eotl_red.mdl" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::Precache()
{
	VPROF_BUDGET( "CTFPlayer::Precache", VPROF_BUDGETGROUP_PLAYER );
	
	/*
	Note: All TFPlayer specific must go inside PrecacheTFPlayer()
		  This assumes that we're loading all resources for any TF player class
		  The reason is to safe performance because tons of string compares is very EXPENSIVE!!!
		  The most offending function is PrecacheGibsForModel which re-parsing through KeyValues every time it's called
		  If you have any question, come talk to me (Bank)
	*/
	PrecacheTFPlayer();

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Allow pre-frame adjustments on the player
//-----------------------------------------------------------------------------
ConVar sv_runcmds( "sv_runcmds", "1" );
void CTFPlayer::PlayerRunCommand( CUserCmd *ucmd, IMoveHelper *moveHelper )
{
	static bool bSeenSyncError = false;
	VPROF( "CTFPlayer::PlayerRunCommand" );

	if ( !sv_runcmds.GetInt() )
		return;

	if ( IsTaunting() )
	{
		// For some taunts, it is critical that the player not move once they start
		if ( !CanMoveDuringTaunt() )
		{
			ucmd->forwardmove = 0;
			ucmd->upmove = 0;
			ucmd->sidemove = 0;
			ucmd->viewangles = pl.v_angle;
		}

		if ( tf_allow_taunt_switch.GetInt() == 0 && ucmd->weaponselect != 0 )
		{
			ucmd->weaponselect = 0;

			// FIXME: The client will have predicted the weapon switch and have
			// called Holster/Deploy which will make the wielded weapon
			// invisible on their end.
		}
	}

	BaseClass::PlayerRunCommand( ucmd, moveHelper );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::IsReadyToPlay( void )
{
	bool bRetVal = false;

	if ( GetTeamNumber() > LAST_SHARED_TEAM )
	{
		if ( GetDesiredPlayerClassIndex() > TF_CLASS_UNDEFINED )
		{
			bRetVal = true;
		}
	}

	return bRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::IsReadyToSpawn( void )
{
	if ( IsClassMenuOpen() )
	{
		return false;
	}

	// Map-makers can force players to have custom respawn times
	if ( GetRespawnTimeOverride() != -1.f && gpGlobals->curtime < GetDeathTime() + GetRespawnTimeOverride() )
		return false;

	return ( StateGet() != TF_STATE_DYING );
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this player should be allowed to instantly spawn
//			when they next finish picking a class.
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldGainInstantSpawn( void )
{
	return ( GetPlayerClass()->GetClassIndex() == TF_CLASS_UNDEFINED || IsClassMenuOpen() ) && !TFGameRules()->IsInArenaMode();
}

//-----------------------------------------------------------------------------
// Purpose: Resets player scores
//-----------------------------------------------------------------------------
void CTFPlayer::ResetScores( void )
{
	m_Shared.ResetScores();
	CTF_GameStats.ResetPlayerStats( this );
	RemoveNemesisRelationships();

	BaseClass::ResetScores();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::InitialSpawn( void )
{
	BaseClass::InitialSpawn();
	
	SetWeaponBuilder( NULL );

	ResetScores();
	StateEnter( TF_STATE_WELCOME );

	ResetAccumulatedSentryGunDamageDealt();
	ResetAccumulatedSentryGunKillCount();
	ResetDamagePerSecond();

	IGameEvent * event = gameeventmanager->CreateEvent( "player_initial_spawn" );
	if ( event )
	{
		event->SetInt( "index", entindex() );
		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Override Base ApplyAbsVelocityImpulse (BaseEntity) to apply potential item attributes
//-----------------------------------------------------------------------------
void CTFPlayer::ApplyAbsVelocityImpulse( const Vector &vecImpulse ) 
{
	// Check for Attributes (mult_aiming_knockback_resistance)
	Vector vecForce = vecImpulse;

	CBaseMultiplayerPlayer::ApplyAbsVelocityImpulse( vecForce );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ApplyGenericPushbackImpulse( const Vector &vecImpulse, CTFPlayer *pAttacker )
{
	Vector vForce = vecImpulse;

	// if on the ground, require min force to boost you off it
	if ( ( GetFlags() & FL_ONGROUND ) && ( vForce.z < JUMP_MIN_SPEED ) )
	{
		// Minimum value of vecForce.z
		vForce.z = JUMP_MIN_SPEED;
	}

	RemoveFlag( FL_ONGROUND );

	ApplyAbsVelocityImpulse( vForce );
}

//-----------------------------------------------------------------------------
// Purpose: Go between for Setting Local Punch Impulses. Checks item attributes
// Use this instead of directly calling m_Local.m_vecPunchAngle.SetX( value );
//-----------------------------------------------------------------------------
bool CTFPlayer::ApplyPunchImpulseX ( float flImpulse ) 
{
	// Check for No Aim Flinch
	m_Local.m_vecPunchAngle.SetX( flImpulse );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFPlayer::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	// always send information to student or client
	if ( pInfo->m_pClientEnt )
	{
		CBaseEntity *pRecipientEntity = CBaseEntity::Instance( pInfo->m_pClientEnt );
		if ( pRecipientEntity && pRecipientEntity->ShouldForceTransmitsForTeam( GetTeamNumber() ) )
			return FL_EDICT_ALWAYS;
	}

	return BaseClass::ShouldTransmit( pInfo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetupVisibility( CBaseEntity *pViewEntity, unsigned char *pvs, int pvssize )
{
	BaseClass::SetupVisibility( pViewEntity, pvs, pvssize );

	int area = pViewEntity ? pViewEntity->NetworkProp()->AreaNum() : NetworkProp()->AreaNum();
	PointCameraSetupVisibility( this, area, pvs, pvssize );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::Spawn()
{
	VPROF_BUDGET( "CTFPlayer::Spawn", VPROF_BUDGETGROUP_PLAYER );
	MDLCACHE_CRITICAL_SECTION();

	m_flSpawnTime = gpGlobals->curtime;

	SetModelScale( 1.0f );
	UpdateModel();

	SetMoveType( MOVETYPE_WALK );
	BaseClass::Spawn();

	// Create our off hand viewmodel if necessary
	CreateViewModel( 1 );
	// Make sure it has no model set, in case it had one before
	GetViewModel(1)->SetModel( "" );

	// Kind of lame, but CBasePlayer::Spawn resets a lot of the state that we initially want on.
	// So if we're in the welcome state, call its enter function to reset 
	if ( m_Shared.InState( TF_STATE_WELCOME ) )
	{
		StateEnterWELCOME();
	}

	// If they were dead, then they're respawning. Put them in the active state.
	if ( m_Shared.InState( TF_STATE_DYING ) )
	{
		StateTransition( TF_STATE_ACTIVE );
	}

	// If they're spawning into the world as fresh meat, give them items and stuff.
	bool bMatchSummary = TFGameRules() && TFGameRules()->ShowMatchSummary();
	if ( m_Shared.InState( TF_STATE_ACTIVE ) || bMatchSummary )
	{
		// remove our disguise each time we spawn
		if ( m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			m_Shared.RemoveDisguise();
		}

		if ( !bMatchSummary )
		{
			EmitSound( "Player.Spawn" );
		}
		m_bRespawning = true;
		InitClass();
		m_bRespawning = false;
		m_Shared.RemoveAllCond(); // Remove conc'd, burning, rotting, hallucinating, etc.

		UpdateSkin( GetTeamNumber() );

		// Prevent firing for a second so players don't blow their faces off
		SetNextAttack( gpGlobals->curtime + 1.0 );

		DoAnimationEvent( PLAYERANIMEVENT_SPAWN );

		// Force a taunt off, if we are still taunting, the condition should have been cleared above.
		StopTaunt();

		// turn on separation so players don't get stuck in each other when spawned
		m_Shared.SetSeparation( true );
		m_Shared.SetSeparationVelocity( vec3_origin );

		RemoveTeleportEffect();
	
		//If this is true it means I respawned without dying (changing class inside the spawn room) but doesn't necessarily mean that my healers have stopped healing me
		//This means that medics can still be linked to me but my health would not be affected since this condition is not set.
		//So instead of going and forcing every healer on me to stop healing we just set this condition back on. 
		//If the game decides I shouldn't be healed by someone (LOS, Distance, etc) they will break the link themselves like usual.
		if ( m_Shared.GetNumHealers() > 0 )
		{
			m_Shared.AddCond( TF_COND_HEALTH_BUFF );
		}

		if ( !m_bSeenRoundInfo )
		{
			TFGameRules()->ShowRoundInfoPanel( this );
			m_bSeenRoundInfo = true;
		}

		if ( IsInCommentaryMode() && !IsFakeClient() )
		{
			// Player is spawning in commentary mode. Tell the commentary system.
			CBaseEntity *pEnt = NULL;
			variant_t emptyVariant;
			while ( (pEnt = gEntList.FindEntityByClassname( pEnt, "commentary_auto" )) != NULL )
			{
				pEnt->AcceptInput( "MultiplayerSpawned", this, this, emptyVariant, 0 );
			}
		}

		bool bRemoveRestriction = ( GetTeamNumber() == TEAM_SPECTATOR ) || ( ( GetTeamNumber() > TEAM_SPECTATOR ) && !IsPlayerClass( TF_CLASS_UNDEFINED ) );
		if ( !m_bFirstSpawnAndCanCallVote && bRemoveRestriction )
		{
			m_bFirstSpawnAndCanCallVote = true;

			CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[ 0 ] : NULL;
			bool bFirstMiniRound = ( !pMaster || !pMaster->PlayingMiniRounds() || ( pMaster->GetCurrentRoundIndex() == 0 ) );
			float flRoundStart = TFGameRules()->GetRoundStart();
			bool bShouldHaveCooldown = ( TFGameRules()->GetRoundsPlayed() > 0 ) || !bFirstMiniRound || ( ( TFGameRules()->State_Get() == GR_STATE_RND_RUNNING ) && ( flRoundStart > 0.f ) && ( gpGlobals->curtime - flRoundStart > sv_vote_late_join_time.GetFloat() ) );
			if ( bShouldHaveCooldown )
			{
				// add a cooldown for our first vote
				if ( g_voteControllerGlobal )
					g_voteControllerGlobal->TrackVoteCaller( this, sv_vote_late_join_cooldown.GetFloat() );

				if ( GetTeamVoteController() )
					GetTeamVoteController()->TrackVoteCaller( this, sv_vote_late_join_cooldown.GetFloat() );
			}
		}
	}

	CTF_GameStats.Event_PlayerSpawned( this );

	m_iSpawnCounter = !m_iSpawnCounter;
	m_bAllowInstantSpawn = false;

	m_Shared.SetSpyCloakMeter( 100.0f );

	m_Shared.ClearDamageEvents();
	m_AchievementData.ClearHistories();

	m_flLastDamageTime = 0.f;
	m_flLastDamageDoneTime = 0.f;
	m_iMaxSentryKills = 0;

	m_flNextVoiceCommandTime = gpGlobals->curtime;
	m_iVoiceSpamCounter = 0;

	ClearZoomOwner();
	SetFOV( this , 0 );

	SetViewOffset( GetClassEyeHeight() );

	RemoveAllScenesInvolvingActor( this );
	ClearExpression();
	m_flNextSpeakWeaponFire = gpGlobals->curtime;
	// This makes the surrounding box always the same size as the standing collision box
	// helps with parts of the hitboxes that extend out of the crouching hitbox, eg with the
	// heavyweapons guy
	Vector mins = VEC_HULL_MIN;
	Vector maxs = VEC_HULL_MAX;
	CollisionProp()->SetSurroundingBoundsType( USE_SPECIFIED_BOUNDS, &mins, &maxs );

	m_iLeftGroundHealth = -1;
	m_bGoingFeignDeath = false;

	m_bArenaIsAFK = false;

	m_flAccumulatedHealthRegen = 0;
	memset( m_flAccumulatedAmmoRegens, 0, sizeof(m_flAccumulatedAmmoRegens) );

	IGameEvent * event = gameeventmanager->CreateEvent( "player_spawn" );
	if ( event )
	{
		event->SetInt( "userid", GetUserID() );
		event->SetInt( "team", GetTeamNumber() );
		event->SetInt( "class", GetPlayerClass()->GetClassIndex() );

		gameeventmanager->FireEvent( event );
	}

	m_bIsMissionEnemy = false;
	m_bIsSupportEnemy = false;
	m_bIsLimitedSupportEnemy = false;

	m_Shared.Spawn();

	m_bCollideWithSentry = false;
	m_calledForMedicTimer.Invalidate();
	m_placedSapperTimer.Invalidate();

	m_nForceTauntCam = 0;

	m_playerMovementStuckTimer.Invalidate();

	m_flHelpmeButtonPressTime = 0.f;

	m_bAlreadyUsedExtendFreezeThisDeath = false;

	SetRespawnOverride( -1.f, NULL_STRING );

	CTFPlayerResource *pResource = dynamic_cast<CTFPlayerResource *>( g_pPlayerResource );
	if ( pResource )
	{
		pResource->SetPlayerClassWhenKilled( entindex(), TF_CLASS_UNDEFINED );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Removes all nemesis relationships between this player and others
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveNemesisRelationships()
{
	for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
	{
		CTFPlayer *pTemp = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pTemp && pTemp != this )
		{
			bool bRemove = false;

			if ( TFGameRules()->IsInArenaMode() == true )
			{
				if ( GetTeamNumber() != TEAM_SPECTATOR )
				{
					if ( InSameTeam( pTemp ) == true )
					{
						bRemove = true;
					}
				}

				if ( IsDisconnecting() == true )
				{
					bRemove = true;
				}
			}
			else
			{
				bRemove = true;
			}
			
			if ( bRemove == true )
			{
				// set this player to be not dominating anyone else
				m_Shared.SetPlayerDominated( pTemp, false );
				m_iNumberofDominations = 0;

				// set no one else to be dominating this player	
				bool bThisPlayerIsDominatingMe = m_Shared.IsPlayerDominatingMe( i );
				pTemp->m_Shared.SetPlayerDominated( this, false );
				if ( bThisPlayerIsDominatingMe )
				{
					int iDoms = pTemp->GetNumberofDominations();
					pTemp->SetNumberofDominations( iDoms - 1);
				}
			}
		}
	}	

	if ( TFGameRules()->IsInArenaMode() == false || IsDisconnecting() == true )
	{
		// reset the matrix of who has killed whom with respect to this player
		CTF_GameStats.ResetKillHistory( this );
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "remove_nemesis_relationships" );
	if ( event )
	{
		event->SetInt( "player", entindex() );
		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::Regenerate( bool bRefillHealthAndAmmo /*= true*/ )
{
	// We may have been boosted over our max health. If we have, 
	// restore it after we reset out class values.
	int nOldMaxHealth = GetMaxHealth();
	int nOldHealth = GetHealth();
	bool bBoosted = ( nOldHealth > nOldMaxHealth || !bRefillHealthAndAmmo ) && ( nOldMaxHealth > 0 );

	int nAmmo[ TF_AMMO_COUNT ];
	if ( !bRefillHealthAndAmmo )
	{
		for ( int iAmmo = 0; iAmmo < TF_AMMO_COUNT; ++iAmmo )
		{
			nAmmo[ iAmmo ] = GetAmmoCount( iAmmo );
		}
	}

	m_bRegenerating.Set( true );
	// This recomputes MaxHealth
	InitClass();
	m_bRegenerating.Set( false );

	if ( bBoosted )
	{
		SetHealth( MAX( nOldHealth, GetMaxHealth() ) );
	}

	if ( bRefillHealthAndAmmo )
	{
		if ( m_Shared.InCond( TF_COND_BURNING ) )
		{
			m_Shared.RemoveCond( TF_COND_BURNING );
		}
	}

	// Reset our first allowed fire time. This allows honorbound weapons to be switched away
	// from for a bit.
	m_Shared.m_flFirstPrimaryAttack = MAX( m_Shared.m_flFirstPrimaryAttack, gpGlobals->curtime + 1.0f );

	if ( bRefillHealthAndAmmo )
	{
		for ( int iAmmo = 0; iAmmo < TF_AMMO_COUNT; ++iAmmo )
		{
			if ( GetAmmoCount( iAmmo ) > GetMaxAmmo( iAmmo ) )
			{
				SetAmmoCount( GetMaxAmmo( iAmmo ), iAmmo );
			}
		}
	}
	else
	{
		for ( int iAmmo = 0; iAmmo < TF_AMMO_COUNT; ++iAmmo )
		{
			SetAmmoCount( nAmmo[ iAmmo ], iAmmo );
		}
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "player_regenerate" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::InitClass( void )
{
	SetArmorValue( GetPlayerClass()->GetMaxArmor() );

	// Init the anim movement vars
	m_PlayerAnimState->SetRunSpeed( GetPlayerClass()->GetMaxSpeed() );
	m_PlayerAnimState->SetWalkSpeed( GetPlayerClass()->GetMaxSpeed() * 0.5 );

	// Give default items for class.
	GiveDefaultItems();

	// Set initial health and armor based on class.
	// Do it after items have been delivered, so items can modify it
	SetMaxHealth( GetMaxHealth() );
	SetHealth( GetMaxHealth() );

	TeamFortress_SetSpeed();

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::CreateViewModel( int iViewModel )
{
	Assert( iViewModel >= 0 && iViewModel < MAX_VIEWMODELS );

	if ( GetViewModel( iViewModel ) )
		return;

	CTFViewModel *pViewModel = ( CTFViewModel * )CreateEntityByName( "tf_viewmodel" );
	if ( pViewModel )
	{
		pViewModel->SetAbsOrigin( GetAbsOrigin() );
		pViewModel->SetOwner( this );
		pViewModel->SetIndex( iViewModel );
		DispatchSpawn( pViewModel );
		pViewModel->FollowEntity( this, false );
		m_hViewModel.Set( iViewModel, pViewModel );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gets the view model for the player's off hand
//-----------------------------------------------------------------------------
CBaseViewModel *CTFPlayer::GetOffHandViewModel()
{
	// off hand model is slot 1
	return GetViewModel( 1 );
}

//-----------------------------------------------------------------------------
// Purpose: Sends the specified animation activity to the off hand view model
//-----------------------------------------------------------------------------
void CTFPlayer::SendOffHandViewModelActivity( Activity activity )
{
	CBaseViewModel *pViewModel = GetOffHandViewModel();
	if ( pViewModel )
	{
		int sequence = pViewModel->SelectWeightedSequence( activity );
		pViewModel->SendViewModelMatchingSequence( sequence );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the player up with the default weapons, ammo, etc.
//-----------------------------------------------------------------------------
void CTFPlayer::GiveDefaultItems()
{
	// Get the player class data.
	TFPlayerClassData_t *pData = m_PlayerClass.GetData();
	if ( GetTeamNumber() == TEAM_SPECTATOR )
	{
		RemoveAllWeapons();
		return;
	}
	
	// Give weapons.
	ManageRegularWeapons( pData );

	// Give a builder weapon for each object the playerclass is allowed to build
	ManageBuilderWeapons( pData );

	// Weapons that added greater ammo than base require us to now fill the player up to max ammo
	for ( int iAmmo = 0; iAmmo < TF_AMMO_COUNT; ++iAmmo )
	{
		GiveAmmo( GetMaxAmmo(iAmmo), iAmmo, true, kAmmoSource_Resupply );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ManageBuilderWeapons( TFPlayerClassData_t *pData )
{
	// Go through each object and see if we need to create or remove builders
	for ( int i = 0; i < OBJ_LAST; ++i )
	{
		// TF2007: Builders need to be destroyed before a new one is created to fix the viewmodel problem
		if ( !GetPlayerClass()->CanBuildObject( i ) )
		{
			CTFWeaponBuilder *pBuilder = CTFPlayerSharedUtils::GetBuilderForObjectType( this, i );
			if ( pBuilder )
			{
				Weapon_Detach( pBuilder );
				UTIL_Remove( pBuilder );
			}
			continue;
		}

		// TODO:  Need to add support for "n" builders, rather hard-wired for two.
		// Currently, the only class that uses more than one is the spy:
		// - BUILDER is OBJ_ATTACHMENT_SAPPER, which is invoked via weapon selection (see objects.txt).
		// - BUILDER2 is OBJ_SPY_TRAP, which is invoked via a build command from PDA3 (spy-specific).

		// Do we have a specific builder for this object?
		CTFWeaponBuilder *pBuilder = NULL;
		if ( !GetObjectInfo( i )->m_bRequiresOwnBuilder )
		{
			// Do we have a default builder, and an object that doesn't require a specific builder?
			pBuilder = CTFPlayerSharedUtils::GetBuilderForObjectType( this, -1 );
			if ( pBuilder )
			{
				// Flag it as supported by this builder (ugly, but necessary for legacy system)
				pBuilder->SetObjectTypeAsBuildable( i );
			}
		}
				
		// Is a new builder required?
		if ( !pBuilder || ( GetObjectInfo( i )->m_bRequiresOwnBuilder && !( CTFPlayerSharedUtils::GetBuilderForObjectType( this, i ) ) ) )
		{
			pBuilder = dynamic_cast< CTFWeaponBuilder* >( GiveNamedItem( "tf_weapon_builder", i ) );
			if ( pBuilder )
			{
				pBuilder->DefaultTouch( this );
			}
		}

		// Builder settings
		if ( pBuilder )
		{
			if ( m_bRegenerating == false )
			{
				pBuilder->WeaponReset();
			}

			pBuilder->GiveDefaultAmmo();
			pBuilder->ChangeTeam( GetTeamNumber() );
			pBuilder->SetObjectTypeAsBuildable( i );
			pBuilder->m_nSkin = GetTeamNumber() - 2;	// color the w_model to the team
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ManageRegularWeapons( TFPlayerClassData_t *pData )
{	
	// Reset ammo.
	RemoveAllAmmo();

	// Remove our disguise weapon.
	m_Shared.RemoveDisguiseWeapon();

	CUtlVector<const char *> precacheStrings;

	// Give ammo. Must be done before weapons, so weapons know the player has ammo for them.
	for ( int iAmmo = 0; iAmmo < TF_AMMO_COUNT; ++iAmmo )
	{
		// This screws up weapons/items that shouldn't start with grenades
		if ( iAmmo >= TF_AMMO_GRENADES1 && iAmmo <= TF_AMMO_GRENADES2 )
			continue;

		GiveAmmo( GetMaxAmmo(iAmmo), iAmmo, true, kAmmoSource_Resupply );
	}

	ManageRegularWeaponsLegacy( pData );

	// On equip, legacy source code will autoswitch to new weapons.
	// Instead of refactoring, we check here to see if we are allowed to have certain weapons switched to

	if ( m_bRegenerating == false )
	{
		bool bWepSwitched = false;
		if ( m_bRememberActiveWeapon && m_iActiveWeaponTypePriorToDeath )
		{
			CTFWeaponBase *pWeapon = Weapon_OwnsThisID( m_iActiveWeaponTypePriorToDeath );
			if ( pWeapon )
			{
				bWepSwitched = Weapon_Switch( pWeapon );
			}
		}
		
		if ( !bWepSwitched )
		{
			SetActiveWeapon( NULL );

			// Find a weapon to switch to, starting with primary.
			CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase*>( Weapon_GetSlot( TF_WPN_TYPE_PRIMARY ) );
			if ( !pWeapon || !pWeapon->CanBeSelected() || !Weapon_Switch( pWeapon ) )
			{
				pWeapon = dynamic_cast<CTFWeaponBase*>( Weapon_GetSlot( TF_WPN_TYPE_SECONDARY ) );
				if ( !pWeapon || pWeapon->CanBeSelected() || !Weapon_Switch( pWeapon ) )
				{
					pWeapon = dynamic_cast<CTFWeaponBase*>( Weapon_GetSlot( TF_WPN_TYPE_MELEE ) );
					Weapon_Switch( pWeapon );
				}
			}
		}

		if ( (m_iLastWeaponSlot == 0 || !m_bRememberLastWeapon) && !m_bRememberActiveWeapon )  
		{
			m_iLastWeaponSlot = 1;
		}

		if ( !Weapon_GetSlot( m_iLastWeaponSlot ) )
		{
			Weapon_SetLast( Weapon_GetSlot( TF_WPN_TYPE_MELEE ) );
		}
		else
		{
			Weapon_SetLast( Weapon_GetSlot( m_iLastWeaponSlot ) );
		}		
	}

	// Now make sure we don't have too much ammo. This can happen if an item has reduced our max ammo.
	for ( int iAmmo = 0; iAmmo < TF_AMMO_COUNT; ++iAmmo )
	{
		int iMax = GetMaxAmmo(iAmmo);
		if ( iMax < GetAmmoCount(iAmmo) )
		{
			RemoveAmmo( GetAmmoCount(iAmmo) - iMax, iAmmo );
		}
	}

	// If our max health dropped below current due to item changes, drop our current health.
	// If we're not being buffed, clamp it to max. Otherwise, clamp it to the max buffed health
	int iMaxHealth = m_Shared.InCond( TF_COND_HEALTH_BUFF ) ? m_Shared.GetMaxBuffedHealth() : GetMaxHealth();
	if ( m_iHealth > iMaxHealth )
	{
		// Modify health manually to prevent showing all the "you got hurt" UI. 
		m_iHealth = iMaxHealth;
	}

	if ( TFGameRules()->InStalemate() && mp_stalemate_meleeonly.GetBool() )
	{
		CBaseCombatWeapon *meleeWeapon = Weapon_GetSlot( TF_WPN_TYPE_MELEE );
		if ( meleeWeapon )
		{
			Weapon_Switch( meleeWeapon );
		}
	}

	PostInventoryApplication();
}

//-----------------------------------------------------------------------------
// Purpose: Handles helpme trace. returns true if we do something with the trace
//-----------------------------------------------------------------------------
ConVar tf_helpme_range( "tf_helpme_range", "150", FCVAR_DEVELOPMENTONLY );
bool CTFPlayer::HandleHelpmeTrace()
{

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Handles pressing the helpme key.
//-----------------------------------------------------------------------------
void CTFPlayer::HelpmeButtonPressed()
{
	m_flHelpmeButtonPressTime = gpGlobals->curtime;

	if ( !HandleHelpmeTrace() )
	{
		// default to calling for medic
		engine->ClientCommand( edict(), "voicemenu 0 0" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles releasing the helpme key.
//-----------------------------------------------------------------------------
void CTFPlayer::HelpmeButtonReleased()
{
	m_flHelpmeButtonPressTime = 0.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveAllItems()
{
	// Nuke items.
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetWeapon(i);
		if ( !pWeapon )
			continue;

		Weapon_Detach( pWeapon );
		UTIL_Remove( pWeapon );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PostInventoryApplication( void )
{
	m_Shared.RecalculatePlayerBodygroups();

	if ( m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		// Using weapons lockers destroys our disguise weapon, so we might need a new one.
		m_Shared.DetermineDisguiseWeapon( false );
	}

	// Remove our disguise if we can't disguise.
	if ( !CanDisguise() )
	{
		RemoveDisguise();
	}

	// Notify the client.
	IGameEvent *event = gameeventmanager->CreateEvent( "post_inventory_application" );
	if ( event )
	{
		event->SetInt( "userid", GetUserID() );
		gameeventmanager->FireEvent( event ); 
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ManageRegularWeaponsLegacy( TFPlayerClassData_t *pData )
{
	for ( int iWeapon = 0; iWeapon < TF_PLAYER_WEAPON_COUNT; ++iWeapon )
	{
		if ( pData->m_aWeapons[iWeapon] != TF_WEAPON_NONE )
		{
			int iWeaponID = pData->m_aWeapons[iWeapon];
			const char *pszWeaponName = WeaponIdToAlias( iWeaponID );

			CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetWeapon( iWeapon );

			// HACK: Convert engineer's second PDA to using the second pda slot
			//if ( iWeaponID == TF_WEAPON_PDA_ENGINEER_DESTROY || iWeaponID == TF_WEAPON_INVIS )
			//{
			//	iLoadoutSlot = TF_WPN_TYPE_PDA; // TF2007: Not sure what else to use, was LOADOUT_POSITION_PDA2
			//}


			// Do we have a custom weapon in this slot?
			if ( pWeapon )
			{

				if ( pWeapon->GetWeaponID() != iWeaponID )
				{
					Weapon_Detach( pWeapon );
					UTIL_Remove( pWeapon );
					pWeapon = NULL;
				}
			}

			pWeapon = dynamic_cast<CTFWeaponBase*>(Weapon_OwnsThisID( iWeaponID ));

			if ( pWeapon )
			{
				pWeapon->ChangeTeam( GetTeamNumber() );
				pWeapon->GiveDefaultAmmo();

				if ( m_bRegenerating == false )
				{
					pWeapon->WeaponReset();
				}

				//char tempstr[1024];
				//g_pVGuiLocalize->ConvertUnicodeToANSI( pWeapon->GetAttributeContainer()->GetItem()->GetItemName(), tempstr, sizeof(tempstr) );
				//Msg("Updated %s for %s\n", tempstr, GetPlayerName() );
			}
			else
			{
				pWeapon = dynamic_cast<CTFWeaponBase*> ( GiveNamedItem( pszWeaponName ) );
				if ( pWeapon )
				{

					pWeapon->GiveTo( this );
				}
			}
		}
		else
		{
			//I shouldn't have any weapons in this slot, so get rid of it
			CTFWeaponBase *pCarriedWeapon = (CTFWeaponBase *)GetWeapon( iWeapon );

			//Don't nuke builders since they will be nuked if we don't need them later.
			if ( pCarriedWeapon && pCarriedWeapon->GetWeaponID() != TF_WEAPON_BUILDER )
			{
				Weapon_Detach( pCarriedWeapon );
				UTIL_Remove( pCarriedWeapon );
			}
		}
	}

	// If we lack a primary or secondary weapon, start with our melee weapon ready.
	// This is for supporting new unlockables that take up weapon slots and leave our character with nothing to wield.
	int iMainWeaponCount = 0;
	CTFWeaponBase* pMeleeWeapon = NULL;
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		CTFWeaponBase *pWeapon = (CTFWeaponBase*) GetWeapon(i);

		if ( pWeapon == NULL )
			continue;

		if ( pWeapon->GetTFWpnData().m_iWeaponType == TF_WPN_TYPE_PRIMARY ||
			 pWeapon->GetTFWpnData().m_iWeaponType == TF_WPN_TYPE_SECONDARY )
		{
			++iMainWeaponCount;
		}
		else if ( pWeapon->GetTFWpnData().m_iWeaponType == TF_WPN_TYPE_MELEE )
		{
			pMeleeWeapon = pWeapon;
		}
	}
	if ( pMeleeWeapon && (iMainWeaponCount==0) )
	{
		Weapon_Switch( pMeleeWeapon );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create and give the named item to the player. Then return it.
//-----------------------------------------------------------------------------
CBaseEntity	*CTFPlayer::GiveNamedItem( const char *pszName, int iSubType, bool bForce )
{
	// We need to support players putting any shotgun into a shotgun slot, pistol into a pistol slot, etc.
	// For legacy reasons, different classes actually spawn different entities for their shotguns/pistols/etc.
	// To deal with this, we translate entities into the right one for the class we're playing.
	if ( !bForce )
	{
		// We don't do this if force is set, since a spy might be disguising as this character, etc.
		pszName = TranslateWeaponEntForClass( pszName, GetPlayerClass()->GetClassIndex() );
	}

	if ( !pszName )
		return NULL;

	// If I already own this type don't create one
	if ( Weapon_OwnsThisType(pszName, iSubType) && !bForce)
	{
		Assert(0);
		return NULL;
	}

	
	CBaseCombatWeapon *pWeapon = dynamic_cast<CBaseCombatWeapon*>( CreateEntityByName(pszName) );
	
	if ( pWeapon == NULL )
	{
		Msg( "NULL Ent in GiveNamedItem!\n" );
		return NULL;
	}
	
	pWeapon->SetLocalOrigin( GetLocalOrigin() );
	pWeapon->AddSpawnFlags( SF_NORESPAWN );

	if ( pWeapon )
	{
		pWeapon->SetSubType( iSubType );
	}

	DispatchSpawn( pWeapon );

	if ( pWeapon != NULL && !(pWeapon->IsMarkedForDeletion()) ) 
	{
		pWeapon->Touch( this );
	}

	return pWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: Find a spawn point for the player.
//-----------------------------------------------------------------------------
CBaseEntity* CTFPlayer::EntSelectSpawnPoint()
{
	CBaseEntity *pSpot = g_pLastSpawnPoints[ GetTeamNumber() ];
	const char *pSpawnPointName = "";

#ifdef TF_RAID_MODE
	if ( TFGameRules()->IsRaidMode() )
	{
		if ( GetTeamNumber() == TF_TEAM_BLUE )
		{
			// only spawn next to friends if the round is not restarting
			if ( TFGameRules()->State_Get() == GR_STATE_RND_RUNNING )
			{
				if ( tf_raid_use_rescue_closets.GetBool() )
				{
					// find a valid rescue closet to spawn into
					CBaseEntity *rescueSpawn = g_pRaidLogic->GetRescueRespawn();

					if ( rescueSpawn )
					{
						return rescueSpawn;
					}
				}
				else if ( tf_boss_battle_respawn_on_friends.GetBool() )
				{
					// the raiders are in the wild - respawn next to them
					float timeSinceInjured = -1.0f;
					CBaseEntity *respawnEntity = NULL;

					// if we are observing a friend, spawn into them
					CBaseEntity *watchEntity = GetObserverTarget();
					if ( watchEntity && IsValidRaidRespawnTarget( watchEntity ) )
					{
						respawnEntity = watchEntity;
					}
					else
					{
						// spawn on the least recently damaged friend
						CTeam *raidingTeam = GetGlobalTeam( TF_TEAM_BLUE );
						for( int i=0; i<raidingTeam->GetNumPlayers(); ++i )
						{
							CTFPlayer *buddy = (CTFPlayer *)raidingTeam->GetPlayer(i);

							// we can't use IsAlive(), because that has already been reset since
							// this code is mid-spawn.  Use m_Shared state instead.
							if ( buddy != this && buddy->m_Shared.InState( TF_STATE_ACTIVE ) && IsValidRaidRespawnTarget( buddy ) )
							{
								// pick the friend who has been hurt least recently
								if ( buddy->GetTimeSinceLastInjury( TF_TEAM_RED ) > timeSinceInjured )
								{
									timeSinceInjured = buddy->GetTimeSinceLastInjury( TF_TEAM_RED );
									respawnEntity = buddy;
								}
							}
						}
					}

					if ( respawnEntity )
					{
						CPVSFilter filter( respawnEntity->GetAbsOrigin() );
						TE_TFParticleEffect( filter, 0.0, "teleported_blue", respawnEntity->GetAbsOrigin(), vec3_angle );
						TE_TFParticleEffect( filter, 0.0, "player_sparkles_blue", respawnEntity->GetAbsOrigin(), vec3_angle, this, PATTACH_POINT );
						return respawnEntity;
					}
				}
			}
		}
	}
#endif // TF_RAID_MODE

	bool bMatchSummary = TFGameRules() && TFGameRules()->ShowMatchSummary();

	// See if the map is asking to force this player to spawn at a specific location
	if ( GetRespawnLocationOverride() && !bMatchSummary )
	{
		if ( SelectSpawnSpotByName( GetRespawnLocationOverride(), pSpot ) )
		{
			m_pSpawnPoint = dynamic_cast< CTFTeamSpawn* >( pSpot );		// Is this even used anymore?
			return pSpot;
		}
		
		// If the entity doesn't exist - or isn't valid - let the regular system handle it
	}

	switch( GetTeamNumber() )
	{
	case TF_TEAM_RED:
	case TF_TEAM_BLUE:
		{
			pSpawnPointName = "info_player_teamspawn";
			if ( SelectSpawnSpotByType( pSpawnPointName, pSpot ) )
			{
				g_pLastSpawnPoints[ GetTeamNumber() ] = pSpot;
			}
			else if ( pSpot )
			{
				int iClass = GetPlayerClass()->GetClassIndex();
				if ( iClass >= 0 && iClass < ARRAYSIZE( g_aPlayerClassNames ) )
				{
					Warning( "EntSelectSpawnPoint(): No valid spawns for class %s on team %i found, even though at least one spawn entity exists.\n", g_aPlayerClassNames[iClass], GetTeamNumber() );
				}
			}

			// need to save this for later so we can apply and modifiers to the armor and grenades...after the call to InitClass()
			m_pSpawnPoint = dynamic_cast<CTFTeamSpawn*>( pSpot );
			break;
		}
	case TEAM_SPECTATOR:
	case TEAM_UNASSIGNED:
	default:
		{
			pSpot = CBaseEntity::Instance( INDEXENT(0) );
			break;		
		}
	}

	if ( !pSpot )
	{
		Warning( "PutClientInServer: no %s on level\n", pSpawnPointName );
		return CBaseEntity::Instance( INDEXENT(0) );
	}

	return pSpot;
} 

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayer::SelectSpawnSpotByType( const char *pEntClassName, CBaseEntity* &pSpot )
{
	bool bMatchSummary = TFGameRules()->ShowMatchSummary();
	CBaseEntity *pMatchSummaryFallback = NULL;

	// Get an initial spawn point.
	pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
	if ( !pSpot )
	{
		// Sometimes the first spot can be NULL????
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
	}

	// First we try to find a spawn point that is fully clear. If that fails,
	// we look for a spawn point that's clear except for another players. We
	// don't collide with our team members, so we should be fine.
	bool bIgnorePlayers = false;
	// When dealing with a standard spawn ent, try to obey any class spawn flags
	bool bRestrictByClass = !V_strcmp( pEntClassName, "info_player_teamspawn" );

	CBaseEntity *pFirstSpot = pSpot;
	do 
	{
		if ( pSpot )
		{
			// Check to see if this is a valid team spawn (player is on this team, etc.).
			if ( TFGameRules()->IsSpawnPointValid( pSpot, this, bIgnorePlayers ) )
			{
				// Check for a bad spawn entity.
				if ( pSpot->GetAbsOrigin() == Vector( 0, 0, 0 ) )
				{
					goto next_spawn_point;
				}
				// SpawnFlags were only recently added to the .fgd (Feb 2016), which means older maps won't have any flags at all (they default to on).
				// So this means we only look for restrictions when we find flags, which a map compiled after this change would/should have.
				else if ( bRestrictByClass && pSpot->GetSpawnFlags() )
				{
					int nClass = GetPlayerClass()->GetClassIndex() - 1;
					if ( !pSpot->HasSpawnFlags( ( 1 << nClass ) ) )
					{
						goto next_spawn_point;
					}
				}

				// Found a valid spawn point.
				return true;
			}
		}

	next_spawn_point:;

		// Let's save off a fallback spot for competitive mode
		if ( bMatchSummary && !pMatchSummaryFallback )
		{
			CTFTeamSpawn *pCTFSpawn = dynamic_cast<CTFTeamSpawn*>( pSpot );
			if ( pCTFSpawn )
			{
				if ( ( pCTFSpawn->GetTeamNumber() == pCTFSpawn->GetTeamNumber() ) && ( pCTFSpawn->GetMatchSummaryType() == PlayerTeamSpawn_MatchSummary_None ) )
				{
					pMatchSummaryFallback = pCTFSpawn;
				}
			}
		}

		// Get the next spawning point to check.
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );

		// Exhausted the list
		if ( pSpot == pFirstSpot )
		{
			// Loop through again, ignoring class restrictions (but check against players)
			if ( bRestrictByClass )
			{
				bRestrictByClass = false;
				pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
			}
			// Loop through again, ignoring players and classes
			else if ( !bRestrictByClass && !bIgnorePlayers )
			{
				bIgnorePlayers = true;
				pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
			}
		}
	} 
	// Continue until a valid spawn point is found or we hit the start.
	while ( pSpot != pFirstSpot );

	// Return a fallback spot for competitive mode
	if ( bMatchSummary && pMatchSummaryFallback )
	{
		pSpot = pMatchSummaryFallback;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: We're being asked to use a spawn with a specific name
//-----------------------------------------------------------------------------
bool CTFPlayer::SelectSpawnSpotByName( const char *pEntName, CBaseEntity* &pSpot )
{
	if ( pEntName && pEntName[0] )
	{
		pSpot = gEntList.FindEntityByName( pSpot, pEntName );

		while ( pSpot )
		{
			if ( TFGameRules()->IsSpawnPointValid( pSpot, this, true, PlayerTeamSpawnMode_Triggered ) )
				return true;

			pSpot = gEntList.FindEntityByName( pSpot, pEntName );
		}
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	MDLCACHE_CRITICAL_SECTION();

	m_PlayerAnimState->DoAnimationEvent( event, nData );
	TE_PlayerAnimEvent( this, event, nData );	// Send to any clients who can see this guy.
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->event == AE_WPN_HIDE )
	{
		// does nothing for now.
	}
	else if ( pEvent->event == AE_SV_EXCLUDE_PLAYER_SOUND )
	{
		CBroadcastNonOwnerRecipientFilter filter( this );
		EmitSound( filter, entindex(), pEvent->options );
	}
	else if ( pEvent->event == AE_RAGDOLL && IsAlive() )
	{
		// Eat this.  Players have found an exploit where they can run this binding:
		// bind "q" "+attack; wait 10; join_class heavyweapons; -attack
		// while they are Engineer, while holding the wrench, while in a respawn room, that will cause them
		// to ragdoll as the Heavy, but be invisible and able to kill everyone.
		//
		// The event seems to be tied to the special headshot-kill anim on the Heavy, but this doesn't break that.
	}
	else
	{
		BaseClass::HandleAnimEvent( pEvent );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PhysObjectSleep()
{
	IPhysicsObject *pObj = VPhysicsGetObject();
	if ( pObj )
		pObj->Sleep();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PhysObjectWake()
{
	IPhysicsObject *pObj = VPhysicsGetObject();
	if ( pObj )
		pObj->Wake();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayer::GetAutoTeam( int nPreferedTeam /*= TF_TEAM_AUTOASSIGN*/ )
{
	int iTeam = TEAM_SPECTATOR;

	CTFTeam *pBlue = TFTeamMgr()->GetTeam( TF_TEAM_BLUE );
	CTFTeam *pRed  = TFTeamMgr()->GetTeam( TF_TEAM_RED );

	if ( pBlue && pRed )
	{
		if ( TFGameRules() )
		{
			if ( TFGameRules()->IsInHighlanderMode() )
			{
				if ( ( pBlue->GetNumPlayers() >= TF_LAST_NORMAL_CLASS - 1 ) &&
					 ( pRed->GetNumPlayers() >= TF_LAST_NORMAL_CLASS - 1 ) )
				{
					// teams are full....join team Spectator for now
					return TEAM_SPECTATOR;
				}
			}
		}

		if ( pBlue->GetNumPlayers() < pRed->GetNumPlayers() )
		{
			iTeam = TF_TEAM_BLUE;
		}
		else if ( pRed->GetNumPlayers() < pBlue->GetNumPlayers() )
		{
			iTeam = TF_TEAM_RED;
		}
		else if ( pRed->GetRole() == TEAM_ROLE_DEFENDERS )
		{
			// AutoTeam should give new players to the attackers on A/D maps if the teams are even
			iTeam = TF_TEAM_BLUE;
		}
		else
		{
			if ( nPreferedTeam == TF_TEAM_AUTOASSIGN )
			{
				iTeam = RandomInt( 0, 1 ) ? TF_TEAM_RED : TF_TEAM_BLUE;
			}
			else
			{
				Assert( nPreferedTeam >= FIRST_GAME_TEAM );
				iTeam = nPreferedTeam;
			}
		}
	}

	return iTeam;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldForceAutoTeam( void )
{
	if ( mp_forceautoteam.GetBool() )
		return true;

	if ( TFGameRules() && TFGameRules()->IsCompetitiveMode() )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::HandleCommand_JoinTeam( const char *pTeamName )
{
	if ( TFGameRules()->State_Get() == GR_STATE_GAME_OVER )
		return;

	bool bAutoTeamed = false;

	int iTeam = TF_TEAM_RED;

	if ( stricmp( pTeamName, "auto" ) == 0 )
	{	
		iTeam = GetAutoTeam();
		bAutoTeamed = true;
	}
	else if ( stricmp( pTeamName, "spectate" ) == 0 )
	{
		iTeam = TEAM_SPECTATOR;
	}
	else
	{
		for ( int i = 0; i < TF_TEAM_COUNT; ++i )
		{
			COMPILE_TIME_ASSERT( TF_TEAM_COUNT == ARRAYSIZE( g_aTeamNames ) );
			if ( stricmp( pTeamName, g_aTeamNames[i] ) == 0 )
			{
				iTeam = i;
				break;
			}
		}
	}

	// now check if we're limited in our team selection (unless we want to be on the spectator team)
	if ( !IsBot() && iTeam != TEAM_SPECTATOR )
	{		
		int iHumanTeam = TFGameRules()->GetAssignedHumanTeam();
		if ( iHumanTeam != TEAM_ANY )
		{
			iTeam = iHumanTeam;
			bAutoTeamed = true;
		}
	}
	
	// invalid team selection
	if ( iTeam < TEAM_SPECTATOR )
	{
		return;
	}

	if ( iTeam == TEAM_SPECTATOR || ( TFGameRules()->IsInArenaMode() && tf_arena_use_queue.GetBool() && GetTeamNumber() <= LAST_SHARED_TEAM ) )
	{
		// Prevent this is the cvar is set
		if ( ( mp_allowspectators.GetBool() == false ) && !IsHLTV() && !IsReplay() && TFGameRules()->IsInArenaMode() == false )
		{
			ClientPrint( this, HUD_PRINTCENTER, "#Cannot_Be_Spectator" );
			return;
		}

		// Deny spectator access if it would unbalance the teams
		if ( ( mp_spectators_restricted.GetBool() ) && TFGameRules() )
		{
			if ( GetTeamNumber() == TF_TEAM_RED || GetTeamNumber() == TF_TEAM_BLUE )
			{
				CTeam *pRedTeam = GetGlobalTeam( TF_TEAM_RED );
				CTeam *pBlueTeam = GetGlobalTeam( TF_TEAM_BLUE );
				if ( pRedTeam && pBlueTeam )
				{
					int nRedCount = pRedTeam->GetNumPlayers();
					int nBlueCount = pBlueTeam->GetNumPlayers();
					int nGap = GetTeamNumber() == TF_TEAM_RED ? ( nBlueCount - nRedCount ) : ( nRedCount - nBlueCount );
					if ( nGap >= mp_teams_unbalance_limit.GetInt() )
					{
						ClientPrint( this, HUD_PRINTCENTER, "#Cannot_Be_Spectator_Unbalance" );
						return;
					}
				}
			}
		}

		if ( GetTeamNumber() != TEAM_UNASSIGNED && !IsDead() )
		{
			CommitSuicide( false, true );
		}

		ChangeTeam( TEAM_SPECTATOR );

		// do we have fadetoblack on? (need to fade their screen back in)
		if ( mp_fadetoblack.GetBool() )
		{
			color32_s clr = { 0,0,0,255 };
			UTIL_ScreenFade( this, clr, 0, 0, FFADE_IN | FFADE_PURGE );
		}
	}
	else
	{
		if ( iTeam == GetTeamNumber() )
		{
			return;	// we wouldn't change the team
		}

		if ( TFGameRules() && TFGameRules()->IsInHighlanderMode() )
		{
			CTFTeam *pTeam = TFTeamMgr()->GetTeam( iTeam );
			if ( pTeam )
			{
				if ( pTeam->GetNumPlayers() >= TF_LAST_NORMAL_CLASS - 1 )
				{
					// if this join would put too many players on the team in Highlander mode, refuse
					// come up with a better way to tell the player they tried to join a full team!
					ShowViewPortPanel( PANEL_TEAM );
					return;
				}
			}
		}

		// if this join would unbalance the teams, refuse
		// come up with a better way to tell the player they tried to join a full team!
		if ( TFGameRules()->WouldChangeUnbalanceTeams( iTeam, GetTeamNumber() ) )
		{
			ShowViewPortPanel( PANEL_TEAM );
			return;
		}

#ifndef _DEBUG
		TFGameRules()->SetPlayerReadyState( entindex(), false );
		TFGameRules()->SetTeamReadyState( false, GetTeamNumber() );
#endif // _DEBUG

		ChangeTeam( iTeam, bAutoTeamed, false );

		ShowViewPortPanel( ( iTeam == TF_TEAM_RED ) ? PANEL_CLASS_RED : PANEL_CLASS_BLUE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Join a team without using the game menus
//-----------------------------------------------------------------------------
void CTFPlayer::HandleCommand_JoinTeam_NoMenus( const char *pTeamName )
{
	Assert( IsX360() );

	Msg( "Client command HandleCommand_JoinTeam_NoMenus: %s\n", pTeamName );

	// Only expected to be used on the 360 when players leave the lobby to start a new game
	if ( !IsInCommentaryMode() )
	{
		Assert( GetTeamNumber() == TEAM_UNASSIGNED );
		Assert( IsX360() );
	}

	int iTeam = TEAM_SPECTATOR;
	if ( Q_stricmp( pTeamName, "spectate" ) )
	{
		for ( int i = 0; i < TF_TEAM_COUNT; ++i )
		{
			COMPILE_TIME_ASSERT( TF_TEAM_COUNT == ARRAYSIZE( g_aTeamNames ) );
			if ( stricmp( pTeamName, g_aTeamNames[i] ) == 0 )
			{
				iTeam = i;
				break;
			}
		}
	}

	ForceChangeTeam( iTeam );
}

//-----------------------------------------------------------------------------
// Purpose: Player has been forcefully changed to another team
//-----------------------------------------------------------------------------
void CTFPlayer::ForceChangeTeam( int iTeamNum, bool bFullTeamSwitch )
{
	int iNewTeam = iTeamNum;

	if ( iNewTeam == TF_TEAM_AUTOASSIGN )
	{
		iNewTeam = GetAutoTeam();
	}

	if ( !GetGlobalTeam( iNewTeam ) )
	{
		Warning( "CTFPlayer::ForceChangeTeam( %d ) - invalid team index.\n", iNewTeam );
		return;
	}

	int iOldTeam = GetTeamNumber();

	// if this is our current team, just abort
	if ( iNewTeam == iOldTeam )
		return;

	RemoveAllOwnedEntitiesFromWorld( true );
	
	m_iPreviousteam = iOldTeam;
	
	BaseClass::ChangeTeam( iNewTeam, false, true );

	if ( !bFullTeamSwitch )
	{
		RemoveNemesisRelationships();

		if ( TFGameRules() && TFGameRules()->IsInHighlanderMode() )
		{
			if ( IsAlive() )
			{
				CommitSuicide( false, true );
			}

			ResetPlayerClass();
		}
	}
	
	if ( iNewTeam == TEAM_UNASSIGNED )
	{
		StateTransition( TF_STATE_OBSERVER );
	}
	else if ( iNewTeam == TEAM_SPECTATOR )
	{
		StateTransition( TF_STATE_OBSERVER );

		RemoveAllWeapons();
		DestroyViewModels();
	}

	DropFlag();

	// Don't modify living players in any way
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::HandleFadeToBlack( void )
{
	if ( mp_fadetoblack.GetBool() )
	{
		SetObserverMode( OBS_MODE_CHASE );

		color32_s clr = { 0,0,0,255 };
		UTIL_ScreenFade( this, clr, 0.75, 0, FFADE_OUT | FFADE_STAYOUT );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ChangeTeam( int iTeamNum, bool bAutoTeam, bool bSilent, bool bAutoBalance /*= false*/ )
{
	if ( !GetGlobalTeam( iTeamNum ) )
	{
		Warning( "CTFPlayer::ChangeTeam( %d ) - invalid team index.\n", iTeamNum );
		return;
	}

	// game rules don't allow to change team
	if ( TFGameRules() && !TFGameRules()->CanChangeTeam( GetTeamNumber() ) )
	{
		return;
	}

	int iOldTeam = GetTeamNumber();

	// if this is our current team, just abort
	if ( iTeamNum == iOldTeam )
		return;

	RemoveAllOwnedEntitiesFromWorld( true );

	m_iPreviousteam = iOldTeam;

	CTF_GameStats.Event_TeamChange( this, iOldTeam, iTeamNum );

	m_iTeamChanges++;

	// If joining the underdog team, make next spawn instant (autobalance, paladins)
	if ( TFGameRules() && TFGameRules()->IsDefaultGameMode() && GetTeamNumber() >= FIRST_GAME_TEAM )
	{
		int nStackedTeam, nWeakTeam;
		if ( TFGameRules()->AreTeamsUnbalanced( nStackedTeam, nWeakTeam ) )
		{
			if ( iTeamNum == nWeakTeam )
			{
				AllowInstantSpawn();
			}
		}
	}

	BaseClass::ChangeTeam( iTeamNum, bAutoTeam, bSilent, bAutoBalance );

	if ( TFGameRules() && TFGameRules()->IsInHighlanderMode() )
	{
		if ( IsAlive() )
		{
			CommitSuicide( false, true );
		}

		ResetPlayerClass();
	}

	RemoveNemesisRelationships();

	if ( iTeamNum == TEAM_UNASSIGNED )
	{
		StateTransition( TF_STATE_OBSERVER );
	}
	else if ( iTeamNum == TEAM_SPECTATOR )
	{
		StateTransition( TF_STATE_OBSERVER );

		RemoveAllWeapons();
		DestroyViewModels();
	}
	else // active player
	{
		bool bKill = true;


		if ( bKill && !IsDead() && (iOldTeam == TF_TEAM_RED || iOldTeam == TF_TEAM_BLUE) )
		{
			// Kill player if switching teams while alive
			CommitSuicide( false, true );
		}
		else if ( IsDead() && iOldTeam < FIRST_GAME_TEAM )
		{
			HandleFadeToBlack();
		}

		// let any spies disguising as me know that I've changed teams
		for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
		{
			CTFPlayer *pTemp = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if ( pTemp && pTemp != this )
			{
				if ( ( pTemp->m_Shared.GetDisguiseTarget() == this ) || // they were disguising as me and I've changed teams
 					 ( !pTemp->m_Shared.GetDisguiseTarget() && pTemp->m_Shared.GetDisguiseTeam() == iTeamNum ) ) // they don't have a disguise and I'm joining the team they're disguising as
				{
					// choose someone else...
					pTemp->m_Shared.FindDisguiseTarget();
				}
			}
		}
	}
	
	m_Shared.RemoveAllCond();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ResetPlayerClass( void )
{
	if ( GetPlayerClass() )
	{
		GetPlayerClass()->Reset();
	}

	SetDesiredPlayerClassIndex( TF_CLASS_UNDEFINED );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::HandleCommand_JoinClass( const char *pClassName, bool bAllowSpawn /* = true */ )
{
	VPROF_BUDGET( "CTFPlayer::HandleCommand_JoinClass", VPROF_BUDGETGROUP_PLAYER );
	if ( TFGameRules()->State_Get() == GR_STATE_GAME_OVER )
	{
		return;
	}

// 	if ( TFGameRules()->ArePlayersInHell() && ( m_Shared.m_iDesiredPlayerClass > TF_CLASS_UNDEFINED ) )
// 	{
// 		ClientPrint( this, HUD_PRINTCENTER, "#TF_CantChangeClassNow" );
// 		return;
// 	}

	if ( TFGameRules()->IsCompetitiveMode() )
	{
		if ( !tf_tournament_classchange_allowed.GetBool() && 
			 TFGameRules()->State_Get() == GR_STATE_RND_RUNNING )
		{
			ClientPrint( this, HUD_PRINTCENTER, "#TF_Ladder_NoClassChangeRound" );
			return;
		}

		if ( !tf_tournament_classchange_ready_allowed.GetBool() && 
			 TFGameRules()->State_Get() == GR_STATE_BETWEEN_RNDS && 
			 TFGameRules()->IsPlayerReady( entindex() ) )
		{
			ClientPrint( this, HUD_PRINTCENTER, "#TF_Ladder_NoClassChangeReady" );
			return;
		}
	}

	if ( GetTeamNumber() == TEAM_UNASSIGNED )
		return;

	// can only join a class after you join a valid team
	if ( GetTeamNumber() <= LAST_SHARED_TEAM && TFGameRules()->IsInArenaMode() == false )
		return;

	// In case we don't get the class menu message before the spawn timer
	// comes up, fake that we've closed the menu.
	SetClassMenuOpen( false );

	if ( TFGameRules()->InStalemate() && TFGameRules()->IsInArenaMode() == false )
	{
		if ( IsAlive() && !TFGameRules()->CanChangeClassInStalemate() )
		{
			char szTime[6];
			Q_snprintf( szTime, sizeof( szTime ), "%d", tf_stalematechangeclasstime.GetInt() );
	
			ClientPrint( this, HUD_PRINTTALK, "#game_stalemate_cant_change_class", szTime );
			return;
		}
	}

	if ( TFGameRules()->IsInArenaMode() == true && IsAlive() == true )
	{
		if ( GetTeamNumber() > LAST_SHARED_TEAM && TFGameRules()->InStalemate() == true ) 
		{
			ClientPrint( this, HUD_PRINTTALK, "#TF_Arena_NoClassChange" );
			return;
		}
	}

	int iClass = TF_CLASS_UNDEFINED;
	bool bShouldNotRespawn = false;

	if ( !bAllowSpawn || ( ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN ) && ( TFGameRules()->GetWinningTeam() != GetTeamNumber() ) ) )
	{
		m_bAllowInstantSpawn = false;
		bShouldNotRespawn = true;
	}

	if ( stricmp( pClassName, "random" ) != 0 && stricmp( pClassName, "auto" ) != 0 )
	{
		int i = 0;

		for ( i = TF_CLASS_SCOUT ; i < TF_CLASS_COUNT_ALL ; i++ )
		{
			if ( stricmp( pClassName, GetPlayerClassData( i )->m_szClassName ) == 0 )
			{
				iClass = i;
				break;
			}
		}
		 
		bool bCivilianOkay = false;

		if ( !bCivilianOkay && ( i >= TF_LAST_NORMAL_CLASS ) )
		{
			Warning( "HandleCommand_JoinClass( %s ) - invalid class name.\n", pClassName );
			return;
		}

		// Check class limits
		if ( !TFGameRules()->CanPlayerChooseClass(this, iClass) )
		{
			ShowViewPortPanel( ( GetTeamNumber() == TF_TEAM_RED ) ? PANEL_CLASS_RED : PANEL_CLASS_BLUE );
			return;
		}
	}
	else
	{
		int iChoices = 0;
		int iClasses[ TF_LAST_NORMAL_CLASS - 1 ] = {}; // -1 to remove the civilian from the randomness
		int iCurrentClass = GetPlayerClass()->GetClassIndex();

		for ( iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; iClass++ )
		{
			if ( iClass != iCurrentClass && TFGameRules()->CanPlayerChooseClass( this, iClass ) )
			{
				iClasses[ iChoices++ ] = iClass;
			}
		}

		if ( !iChoices )
		{
			if ( TFGameRules()->CanPlayerChooseClass( this, iCurrentClass ) )
				return;

			// We failed to find a random class. Bring up the class menu again.
			ShowViewPortPanel( ( GetTeamNumber() == TF_TEAM_RED ) ? PANEL_CLASS_RED : PANEL_CLASS_BLUE );
			return;
		}

		iClass = iClasses[ random->RandomInt( 0, iChoices - 1 ) ];
	}

#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetBool() && !IsBot() )
	{
		Vector vPos = GetAbsOrigin();
		QAngle qAngle = GetAbsAngles();
		SetDesiredPlayerClassIndex( iClass );
		ForceRespawn();
		Teleport( &vPos, &qAngle, &vec3_origin );
		return;
	}
#endif // _DEBUG || STAGING_ONLY

	// joining the same class?
	if ( iClass != TF_CLASS_RANDOM && iClass == GetDesiredPlayerClassIndex() )
	{
		// If we're dead, and we have instant spawn, respawn us immediately. Catches the case
		// where a player misses respawn wave because they're at the class menu, and then changes
		// their mind and reselects their current class.
		if ( m_bAllowInstantSpawn && !IsAlive() )
		{
			ForceRespawn();
		}
		return;
	}

	SetDesiredPlayerClassIndex( iClass );
	IGameEvent * event = gameeventmanager->CreateEvent( "player_changeclass" );
	if ( event )
	{
		event->SetInt( "userid", GetUserID() );
		event->SetInt( "class", iClass );

		gameeventmanager->FireEvent( event );
	}

	// are they TF_CLASS_RANDOM and trying to select the class they're currently playing as (so they can stay this class)?
	if ( iClass == GetPlayerClass()->GetClassIndex() )
	{
		// If we're dead, and we have instant spawn, respawn us immediately. Catches the case
		// were a player misses respawn wave because they're at the class menu, and then changes
		// their mind and reselects their current class.
		if ( m_bAllowInstantSpawn && !IsAlive() )
		{
			ForceRespawn();
		}
		return;
	}

	// We can respawn instantly if:
	//	- We're dead, and we're past the required post-death time
	//	- We're inside a respawn room of our own team
	//	- We're in the stalemate grace period
	bool bInRespawnRoom = PointInRespawnRoom( this, WorldSpaceCenter(), true );
	if ( bInRespawnRoom && !IsAlive() )
	{
		// If we're not spectating ourselves, ignore respawn rooms. Otherwise we'll get instant spawns
		// by spectating someone inside a respawn room.
		bInRespawnRoom = (GetObserverTarget() == this);
	}
	bool bDeadInstantSpawn = !IsAlive();
	if ( bDeadInstantSpawn && m_flDeathTime )
	{
		// In death mode, don't allow class changes to force respawns ahead of respawn waves
		float flWaveTime = TFGameRules()->GetNextRespawnWave( GetTeamNumber(), this );
		bDeadInstantSpawn = (gpGlobals->curtime > flWaveTime);
	}
	bool bInStalemateClassChangeTime = false;
	if ( TFGameRules()->InStalemate() && TFGameRules()->IsInWaitingForPlayers() == false )
	{
		// Stalemate overrides respawn rules. Only allow spawning if we're in the class change time.
		bInStalemateClassChangeTime = TFGameRules()->CanChangeClassInStalemate();
		bDeadInstantSpawn = false;
		bInRespawnRoom = false;
	}

	if ( TFGameRules()->IsInArenaMode() == true )
	{
		if ( TFGameRules()->IsInWaitingForPlayers() == false )
		{
			bDeadInstantSpawn = false;

			if ( TFGameRules()->InStalemate() == false && TFGameRules()->State_Get() != GR_STATE_TEAM_WIN  )
			{
				bInRespawnRoom = true;
				bShouldNotRespawn = false;
			}
			else
			{
				bShouldNotRespawn = true;
				
				 if ( tf_arena_use_queue.GetBool() == false )
					return;
			}
		}
		else if ( tf_arena_use_queue.GetBool() == false )
		{
			return;
		}
	}

	if ( bShouldNotRespawn == false && ( m_bAllowInstantSpawn || bDeadInstantSpawn || bInRespawnRoom || bInStalemateClassChangeTime ) )
	{
		ForceRespawn();


		return;
	}

	if( iClass == TF_CLASS_RANDOM )
	{
		if( IsAlive() )
		{
			ClientPrint(this, HUD_PRINTTALK, "#game_respawn_asrandom" );
		}
		else
		{
			ClientPrint(this, HUD_PRINTTALK, "#game_spawn_asrandom" );
		}
	}
	else
	{
		if( IsAlive() )
		{
			ClientPrint(this, HUD_PRINTTALK, "#game_respawn_as", GetPlayerClassData( iClass )->m_szLocalizableName );
		}
		else
		{
			ClientPrint(this, HUD_PRINTTALK, "#game_spawn_as", GetPlayerClassData( iClass )->m_szLocalizableName );
		}
	}

	if ( IsAlive() && ( GetHudClassAutoKill() == true ) && bShouldNotRespawn == false )
	{
		CommitSuicide( false, true );
	}

}

//-----------------------------------------------------------------------------
// Purpose: The GC has told us this player wants to respawn now that their loadout has changed.
//-----------------------------------------------------------------------------
void CTFPlayer::CheckInstantLoadoutRespawn( void )
{

}

bool CTFPlayer::ClientCommand( const CCommand &args )
{
	const char *pcmd = args[0];
	
	m_flLastAction = gpGlobals->curtime;

	if ( FStrEq( pcmd, "addcond" ) )
	{
		if ( sv_cheats->GetBool() && args.ArgC() >= 2 )
		{
			CTFPlayer *pTargetPlayer = this;
			if ( args.ArgC() >= 4 )
			{
				// Find the matching netname
				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex(i) );
					if ( pPlayer )
					{
						if ( Q_strstr( pPlayer->GetPlayerName(), args[3] ) )
						{
							pTargetPlayer = ToTFPlayer(pPlayer);
							break;
						}
					}
				}
			}

			int iCond = atoi( args[1] );
			if ( args[1][0] != '0' && iCond == 0 )
			{
				iCond = GetTFConditionFromName( args[1] );
			}

			ETFCond eCond = TF_COND_INVALID;
			if ( iCond >= 0 && iCond < TF_COND_LAST )
			{
				eCond = ( ETFCond )iCond;
			}
			else
			{
				Warning( "Failed to addcond %s to player %s\n", args[1], pTargetPlayer->GetPlayerName() );
				return true;
			}

			if ( args.ArgC() >= 3 )
			{
				float flDuration = atof( args[2] );
				pTargetPlayer->m_Shared.AddCond( eCond, flDuration );
			}
			else
			{
				pTargetPlayer->m_Shared.AddCond( eCond );
			}
		}
		return true;
	}
	else if ( FStrEq( pcmd, "removecond" ) )
	{
		if ( sv_cheats->GetBool() && args.ArgC() >= 2 )
		{
			CTFPlayer *pTargetPlayer = this;
			if ( args.ArgC() >= 3 )
			{
				// Find the matching netname
				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex(i) );
					if ( pPlayer )
					{
						if ( Q_strstr( pPlayer->GetPlayerName(), args[2] ) )
						{
							pTargetPlayer = ToTFPlayer(pPlayer);
							break;
						}
					}
				}
			}

			if ( FStrEq( args[1], "all" ) )
			{
				pTargetPlayer->m_Shared.RemoveAllCond();
			}
			else
			{
				int iCond = atoi( args[1] );
				if ( args[1][0] != '0' && iCond == 0 )
				{
					iCond = GetTFConditionFromName( args[1] );
				}

				if ( iCond >= 0 && iCond < TF_COND_LAST )
				{
					ETFCond eCond = (ETFCond)iCond;
					pTargetPlayer->m_Shared.RemoveCond( eCond );
				}
				else
				{
					Warning( "Failed to removecond %s from player %s\n", args[1], pTargetPlayer->GetPlayerName() );
				}
			}
		}
		return true;
	}
#ifdef _DEBUG
	else if ( FStrEq( pcmd, "burn" ) ) 
	{
		m_Shared.Burn( this, GetActiveTFWeapon() );
		return true;
	}
	else if ( FStrEq( pcmd, "dump_damagers" ) )
	{
		m_AchievementData.DumpDamagers();
		return true;
	}
	else
#endif

	if ( FStrEq( pcmd, "jointeam" ) )
	{
		// don't let them spam the server with changes
		if ( ( GetNextChangeTeamTime() > gpGlobals->curtime ) && ( GetTeamNumber() != TEAM_UNASSIGNED ) )
			return true;

		SetNextChangeTeamTime( gpGlobals->curtime + 2.0f );  // limit to one change every 2 secs

		if ( args.ArgC() >= 2 )
		{
			HandleCommand_JoinTeam( args[1] );
		}
		return true;
	}
	else if ( FStrEq( pcmd, "jointeam_nomenus" ) )
	{
		if ( IsX360() )
		{
			if ( args.ArgC() >= 2 )
			{
				HandleCommand_JoinTeam_NoMenus( args[1] );
			}
			return true;
		}
		return false;
	}
	else if ( FStrEq( pcmd, "closedwelcomemenu" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			if ( GetTeamNumber() == TEAM_UNASSIGNED )
			{
				if ( ShouldForceAutoTeam() )
				{
					ChangeTeam( GetAutoTeam(), true, false );
					ShowViewPortPanel( ( GetTeamNumber() == TF_TEAM_BLUE ) ? PANEL_CLASS_BLUE : PANEL_CLASS_RED );
				}
				else
				{
					ShowViewPortPanel( PANEL_TEAM, true );
				}
			}
			else if ( IsPlayerClass( TF_CLASS_UNDEFINED ) )
			{
				switch( GetTeamNumber() )
				{
				case TF_TEAM_RED:
					ShowViewPortPanel( PANEL_CLASS_RED, true );
					break;

				case TF_TEAM_BLUE:
					ShowViewPortPanel( PANEL_CLASS_BLUE, true );
					break;

				default:
					break;
				}
			}
		}
		return true;
	}
	else if ( FStrEq( pcmd, "joinclass" ) ) 
	{
		// don't let them spam the server with changes
		if ( GetNextChangeClassTime() > gpGlobals->curtime )
			return true;

		SetNextChangeClassTime( gpGlobals->curtime + 0.5 );  // limit to one change every 0.5 secs

		if ( args.ArgC() >= 2 )
		{
			HandleCommand_JoinClass( args[1] );
		}
		return true;
	}
	else if ( FStrEq( pcmd, "resetclass" ) )
	{
		if ( TFGameRules() && TFGameRules()->IsInHighlanderMode() && ( GetTeamNumber() > LAST_SHARED_TEAM ) )
		{
			if ( IsAlive() )
			{
				CommitSuicide( false, true );
			}

			ResetPlayerClass();
			ShowViewPortPanel( ( GetTeamNumber() == TF_TEAM_RED ) ? PANEL_CLASS_RED : PANEL_CLASS_BLUE );
		}

		return true;
	}
	else if ( FStrEq( pcmd, "mp_playgesture" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			if ( args.ArgC() == 1 )
			{
				Warning( "mp_playgesture: Gesture activity or sequence must be specified!\n" );
				return true;
			}

			if ( sv_cheats->GetBool() )
			{
				if ( !PlayGesture( args[1] ) )
				{
					Warning( "mp_playgesture: unknown sequence or activity name \"%s\"\n", args[1] );
					return true;
				}
			}
		}
		return true;
	}
	else if ( FStrEq( pcmd, "mp_playanimation" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			if ( args.ArgC() == 1 )
			{
				Warning( "mp_playanimation: Activity or sequence must be specified!\n" );
				return true;
			}

			if ( sv_cheats->GetBool() )
			{
				if ( !PlaySpecificSequence( args[1] ) )
				{
					Warning( "mp_playanimation: Unknown sequence or activity name \"%s\"\n", args[1] );
					return true;
				}
			}
		}
		return true;
	}
	else if ( FStrEq( pcmd, "menuopen" ) )
	{
		SetClassMenuOpen( true );
		return true;
	}
	else if ( FStrEq( pcmd, "menuclosed" ) )
	{
		SetClassMenuOpen( false );
		return true;
	}
	else if ( FStrEq( pcmd, "pda_click" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			// player clicked on the PDA, play attack animation
			CTFWeaponBase *pWpn = GetActiveTFWeapon();
			CTFWeaponPDA *pPDA = dynamic_cast<CTFWeaponPDA *>( pWpn );

			if ( pPDA && !m_Shared.InCond( TF_COND_DISGUISED ) )
			{
				DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
			}
		}
		return true;
	}
	else if ( FStrEq( pcmd, "weapon_taunt" ) || FStrEq( pcmd, "taunt" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			if ( m_flTauntNextStartTime < gpGlobals->curtime )
			{
				HandleTauntCommand();
			}
		}
		return true;
	}
	else if ( FStrEq( pcmd, "-taunt" ) )
	{
		// DO NOTHING
		// We changed taunt key to be press to toggle instead of press and hold to do long taunt
		return true;
	}
	else if ( FStrEq( pcmd, "build" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			if ( TFGameRules()->InStalemate() && mp_stalemate_meleeonly.GetBool() )
				return true;

			if ( IsTaunting() )
				return true;

			int iBuilding = 0;
			int iMode = 0;
			bool bArgsChecked = false;

			// Fixup old binds.
			if ( args.ArgC() == 2 )
			{
				iBuilding = atoi( args[ 1 ] );
				if ( iBuilding == 3 ) // Teleport exit is now a mode.
				{
					iBuilding = 1;
					iMode = 1;
				}
				bArgsChecked = true;
			}
			else if ( args.ArgC() == 3 )
			{
				iBuilding = atoi( args[ 1 ] );
				iMode = atoi( args[ 2 ] );
				bArgsChecked = true;
			}

			if ( bArgsChecked )
			{
				StartBuildingObjectOfType( iBuilding, iMode );
			}
			else
			{
				Warning( "Usage: build <building> <mode>\n" );
			}

#if defined( _DEBUG ) || defined( STAGING_ONLY )
			if ( bot_mimic.GetBool() )
			{
				CUtlVector< CTFPlayer* > vecPlayers;
				CollectPlayers( &vecPlayers, TEAM_ANY, COLLECT_ONLY_LIVING_PLAYERS );
				FOR_EACH_VEC( vecPlayers, i )
				{
					CTFPlayer *pTFBot = ToTFPlayer( vecPlayers[i] );
					if ( !pTFBot )
						continue;

					if ( !pTFBot->IsPlayerClass( TF_CLASS_ENGINEER ) )
						continue;

					if ( !pTFBot->IsBot() )
						continue;

					//if ( pTFBot->GetPlayerType() != CTFPlayer::TEMP_BOT )
					//	continue;

					// Mimic it
					pTFBot->ClientCommand( args );
				}
			}
#endif // STAGING_ONLY, _DEBUG
		}

		return true;
	}
	else if ( FStrEq( pcmd, "destroy" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			if ( IsPlayerClass( TF_CLASS_ENGINEER ) ) // Spies can't destroy buildings (sappers)
			{
				int iBuilding = 0;
				int iMode = 0;
				bool bArgsChecked = false;

				// Fixup old binds.
				if ( args.ArgC() == 2 )
				{
					iBuilding = atoi( args[ 1 ] );
					if ( iBuilding == 3 ) // Teleport exit is now a mode.
					{
						iBuilding = 1;
						iMode = 1;
					}
					bArgsChecked = true;
				}
				else if ( args.ArgC() == 3 )
				{
					iBuilding = atoi( args[ 1 ] );
					iMode = atoi( args[ 2 ] );
					bArgsChecked = true;
				}

				if ( bArgsChecked )
				{
					DetonateObjectOfType( iBuilding, iMode );
				}
				else
				{
					Warning( "Usage: destroy <building> <mode>\n" );
				}
			}
		}
		return true;
	}
	else if ( FStrEq( pcmd, "extendfreeze" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) && IsDead() && !m_bAlreadyUsedExtendFreezeThisDeath )
		{
			m_bAlreadyUsedExtendFreezeThisDeath = true;
			m_flDeathTime += 2.0f;
		}
		return true;
	}
	else if ( FStrEq( pcmd, "show_motd" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			if ( ShouldForceAutoTeam() )
			{
				int nPreferedTeam = TF_TEAM_AUTOASSIGN;

				int iTeam = GetAutoTeam( nPreferedTeam );
				ChangeTeam( iTeam, true, false );
				ShowViewPortPanel( ( iTeam == TF_TEAM_RED ) ? PANEL_CLASS_RED : PANEL_CLASS_BLUE );
			}
			else
			{
				ShowViewPortPanel( PANEL_TEAM, false );
			}

			char pszWelcome[128];
			Q_snprintf( pszWelcome, sizeof(pszWelcome), "#TF_Welcome" );
			if ( UTIL_GetActiveHolidayString() )
			{
				Q_snprintf( pszWelcome, sizeof(pszWelcome), "#TF_Welcome_%s", UTIL_GetActiveHolidayString() );
			}

			KeyValues *data = new KeyValues( "data" );
			data->SetString( "title", pszWelcome );		// info panel title
			data->SetString( "type", "1" );				// show userdata from stringtable entry
			data->SetString( "msg",	"motd" );			// use this stringtable entry
			data->SetString( "msg_fallback",	"motd_text" );			// use this stringtable entry if the base is HTML, and client has disabled HTML motds
			data->SetBool( "unload", sv_motd_unload_on_dismissal.GetBool() );

			ShowViewPortPanel( PANEL_INFO, true, data );

			data->deleteThis();
		}
		return true;
	}
	else if ( FStrEq( pcmd, "show_htmlpage" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			if ( args.ArgC() != 2 )
			{
				Warning( "Usage: show_htmlpage <url>\n" );
				return true;
			}

			KeyValues *data = new KeyValues( "data" );
			data->SetString( "title", "#TF_Welcome" );	// info panel title
			data->SetString( "type", "2" );				// show url
			data->SetString( "msg",	args[1] );
			data->SetString( "msg_fallback", "motd_text" );			// use this stringtable entry if the base is HTML, and client has disabled HTML motds
			data->SetInt( "cmd", TEXTWINDOW_CMD_CLOSED_HTMLPAGE );		// exec this command if panel closed
			data->SetString( "customsvr", "1" );
			data->SetBool( "unload", false );

			ShowViewPortPanel( PANEL_INFO, true, data );

			data->deleteThis();
		}
		return true;
	}
	else if ( FStrEq( pcmd, "closed_htmlpage" ) )
	{
		// Does nothing, it's for server plugins to hook.
		return true;
	}
	else if ( FStrEq( pcmd, "condump_on" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
				Msg("Console dumping on.\n");
				return true;
		}
	}
	else if ( FStrEq( pcmd, "condump_off" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
				Msg("Console dumping off.\n");
				return true;
		}
	}
	else if ( FStrEq( pcmd, "spec_next" ) ) // chase next player
	{
// 		if ( !ShouldRunRateLimitedCommand( args ) )
// 			return true;

		// intentionally falling through to the bottom so the baseclass version is called
		m_bArenaIsAFK = false;
	}
	else if ( FStrEq( pcmd, "spec_prev" ) ) // chase prev player
	{
// 		if ( !ShouldRunRateLimitedCommand( args ) )
// 			return true;

		// intentionally falling through to the bottom so the baseclass version is called
		m_bArenaIsAFK = false;
	}
	else if ( FStrEq( pcmd, "spec_mode" ) ) // set obs mode
	{
// 		if ( !ShouldRunRateLimitedCommand( args ) )
// 			return true;

		// intentionally falling through to the bottom so the baseclass version is called
		m_bArenaIsAFK = false;
	}
	else if ( FStrEq( pcmd, "showroundinfo" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			// don't let the player open the round info menu until they're a spectator or they're on a regular team and have picked a class
			if ( ( GetTeamNumber() == TEAM_SPECTATOR ) || ( ( GetTeamNumber() != TEAM_UNASSIGNED ) && ( GetPlayerClass()->GetClassIndex() != TF_CLASS_UNDEFINED ) ) )
			{
				if ( TFGameRules() )
				{
					TFGameRules()->ShowRoundInfoPanel( this );
				}
			}
		}

		return true;
	}
	else if ( FStrEq( pcmd, "autoteam" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			int iTeam = GetAutoTeam();
			ChangeTeam( iTeam, true, false );

			if ( iTeam > LAST_SHARED_TEAM )
			{
				ShowViewPortPanel( ( iTeam == TF_TEAM_RED ) ? PANEL_CLASS_RED : PANEL_CLASS_BLUE );
			}
		}

		return true;
	}
	else if ( FStrEq( pcmd, "spectate" ) )
	{
		HandleCommand_JoinTeam( "spectate" );
		return true;
	}
	else if ( FStrEq( pcmd, "team_ui_setup" ) )
	{
		bool bAutoTeam = ShouldForceAutoTeam();
#ifdef TF_RAID_MODE
		bAutoTeam |= TFGameRules()->IsBossBattleMode();
#endif
		
		// For autoteam, display the appropriate team's CLASS selection ui
		if ( bAutoTeam )
		{
			ChangeTeam( GetAutoTeam(), true, false );
			ShowViewPortPanel( ( GetTeamNumber() == TF_TEAM_BLUE ) ? PANEL_CLASS_BLUE : PANEL_CLASS_RED );
		}
		// Otherwise, show TEAM selection ui
		else
		{
			ShowViewPortPanel( PANEL_TEAM );
		}

		return true;
	}
	else if ( FStrEq( "cyoa_pda_open", pcmd ) )
	{
		bool bOpen = atoi( args[1] ) != 0;

		if ( bOpen && IsTaunting() )
		{
			ClientPrint( this, HUD_PRINTCENTER, "#TF_CYOA_PDA_Taunting" );
		}
		else
		{
			TeamFortress_SetSpeed();
		}
		return true;
	}

	return BaseClass::ClientCommand( args );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetClassMenuOpen( bool bOpen )
{
	m_bIsClassMenuOpen = bOpen;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::IsClassMenuOpen( void )
{
	return m_bIsClassMenuOpen;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::PlayGesture( const char *pGestureName )
{
	Activity nActivity = (Activity)LookupActivity( pGestureName );
	if ( nActivity != ACT_INVALID )
	{
		DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, nActivity );
		return true;
	}

	int nSequence = LookupSequence( pGestureName );
	if ( nSequence != -1 )
	{
		DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE_SEQUENCE, nSequence );
		return true;
	} 

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::PlaySpecificSequence( const char *pAnimationName )
{
	Activity nActivity = (Activity)LookupActivity( pAnimationName );
	if ( nActivity != ACT_INVALID )
	{
		DoAnimationEvent( PLAYERANIMEVENT_CUSTOM, nActivity );
		return true;
	}

	int nSequence = LookupSequence( pAnimationName );
	if ( nSequence != -1 )
	{
		DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_SEQUENCE, nSequence );
		return true;
	} 

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::DetonateObjectOfType( int iType, int iMode, bool bIgnoreSapperState )
{
	CBaseObject *pObj = GetObjectOfType( iType, iMode );
	if( !pObj )
		return;

	if( !bIgnoreSapperState && ( pObj->HasSapper() || pObj->IsPlasmaDisabled() ) )
		return;

	IGameEvent *event = gameeventmanager->CreateEvent( "object_removed" );	
	if ( event )
	{
		event->SetInt( "userid", GetUserID() ); // user ID of the object owner
		event->SetInt( "objecttype", iType ); // type of object removed
		event->SetInt( "index", pObj->entindex() ); // index of the object removed
		gameeventmanager->FireEvent( event );
	}

	SpeakConceptIfAllowed( MP_CONCEPT_DETONATED_OBJECT, pObj->GetResponseRulesModifier() );
	pObj->DetonateObject();

	const CObjectInfo *pInfo = GetObjectInfo( iType );

	if ( pInfo )
	{
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"killedobject\" (object \"%s\") (weapon \"%s\") (objectowner \"%s<%i><%s><%s>\") (attacker_position \"%d %d %d\")\n",   
			GetPlayerName(),
			GetUserID(),
			GetNetworkIDString(),
			GetTeam()->GetName(),
			pInfo->m_pObjectName,
			"pda_engineer",
			GetPlayerName(),
			GetUserID(),
			GetNetworkIDString(),
			GetTeam()->GetName(),
			(int)GetAbsOrigin().x, 
			(int)GetAbsOrigin().y,
			(int)GetAbsOrigin().z );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	if ( m_takedamage != DAMAGE_YES )
		return;

	CTFPlayer *pAttacker = ToTFPlayer( info.GetAttacker() );
	if ( pAttacker )
	{
		// Prevent team damage here so blood doesn't appear
		if ( !g_pGameRules->FPlayerCanTakeDamage( this, pAttacker, info ) )
		{
			return;
		}
	}

	// Save this bone for the ragdoll.
	m_nForceBone = ptr->physicsbone;

	SetLastHitGroup( ptr->hitgroup );

	// Ignore hitboxes for all weapons except the sniper rifle
	CTakeDamageInfo info_modified = info;
	bool bIsHeadshot = false;

	if ( info_modified.GetDamageType() & DMG_USE_HITLOCATIONS )
	{
		if ( !m_Shared.InCond( TF_COND_INVULNERABLE ) && ptr->hitgroup == HITGROUP_HEAD )
		{
			CTFWeaponBase *pWpn = pAttacker->GetActiveTFWeapon();
			bool bCritical = true;
			bIsHeadshot = true;

			if ( pWpn && !pWpn->CanFireCriticalShot( true, this ) )
			{
				bCritical = false;
			}

			if ( bCritical )
			{
				info_modified.AddDamageType( DMG_CRITICAL );
				info_modified.SetDamageCustom( TF_DMG_CUSTOM_HEADSHOT );

				// play the critical shot sound to the shooter	
				if ( pWpn )
				{
					pWpn->WeaponSound( BURST );
				}
			}
		}
	}

	if ( GetTeamNumber() == TF_TEAM_BLUE )
	{
		info_modified.SetDamage( info_modified.GetDamage() * tf_damage_multiplier_blue.GetFloat() );
	}
	else if ( GetTeamNumber() == TF_TEAM_RED )
	{
		info_modified.SetDamage( info_modified.GetDamage() * tf_damage_multiplier_red.GetFloat() );
	}

	if ( m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		// no impact effects
	}
	else if ( m_Shared.IsInvulnerable() )
	{ 
		// Make bullet impacts
		g_pEffects->Ricochet( ptr->endpos - (vecDir * 8), -vecDir );
	}
	else
	{	
		// Since this code only runs on the server, make sure it shows the tempents it creates.
		CDisablePredictionFiltering disabler;

		// This does smaller splotches on the guy and splats blood on the world.
		TraceBleed( info_modified.GetDamage(), vecDir, ptr, info_modified.GetDamageType() );
	}

	AddMultiDamage( info_modified, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayer::TakeHealth( float flHealth, int bitsDamageType )
{
	int nResult = 0;

	// Medigun healing and player/class regen use an accumulator, so they've already factored in debuffs.
// 	if ( m_Shared.InCond( TF_COND_HEALING_DEBUFF ) && !( bitsDamageType & DMG_IGNORE_DEBUFFS ) )
// 	{
// 		flHealth *= ( 1.f - PYRO_AFTERBURN_HEALING_REDUCTION );
// 	}

	// If the bit's set, add over the max health
	if ( bitsDamageType & DMG_IGNORE_MAXHEALTH )
	{
		int iTimeBasedDamage = g_pGameRules->Damage_GetTimeBased();
		m_bitsDamageType &= ~(bitsDamageType & ~iTimeBasedDamage);
		m_iHealth += flHealth;
		nResult = flHealth;
	}
	else
	{
		float flHealthToAdd = flHealth;
		float flMaxHealth = GetMaxHealth();
		
		// don't want to add more than we're allowed to have
		if ( flHealthToAdd > flMaxHealth - m_iHealth )
		{
			flHealthToAdd = flMaxHealth - m_iHealth;
		}

		if ( flHealthToAdd <= 0 )
		{
			nResult = 0;
		}
		else
		{
			nResult = BaseClass::TakeHealth( flHealthToAdd, bitsDamageType );
		}
	}

	return nResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::TFWeaponRemove( int iWeaponID )
{
	// find the weapon that matches the id and remove it
	int i;
	for (i = 0; i < WeaponCount(); i++) 
	{
		CTFWeaponBase *pWeapon = ( CTFWeaponBase *)GetWeapon( i );
		if ( !pWeapon )
			continue;

		if ( pWeapon->GetWeaponID() != iWeaponID )
			continue;

		RemovePlayerItem( pWeapon );
		UTIL_Remove( pWeapon );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::BumpWeapon( CBaseCombatWeapon *pWeapon )
{
	CBaseCombatCharacter *pOwner = pWeapon->GetOwner();

	// Can I have this weapon type?
	if ( !IsAllowedToPickupWeapons() )
		return false;

	if ( pOwner || !Weapon_CanUse( pWeapon ) || !g_pGameRules->CanHavePlayerItem( this, pWeapon ) )
	{
		UTIL_Remove( pWeapon );
		return false;
	}

	// Don't let the player fetch weapons through walls (use MASK_SOLID so that you can't pickup through windows)
	if ( !pWeapon->FVisible( this, MASK_SOLID ) )
		return false;

	// ----------------------------------------
	// If I already have it just take the ammo
	// ----------------------------------------
	if (Weapon_OwnsThisType( pWeapon->GetClassname(), pWeapon->GetSubType())) 
	{
		UTIL_Remove( pWeapon );
		return true;
	}
	else 
	{
		// -------------------------
		// Otherwise take the weapon
		// -------------------------
		pWeapon->CheckRespawn();

		pWeapon->AddSolidFlags( FSOLID_NOT_SOLID );
		pWeapon->AddEffects( EF_NODRAW );

		Weapon_Equip( pWeapon );
		return true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::DropCurrentWeapon( void )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::DropFlag( bool bSilent /* = false */ )
{
	if ( HasItem() )
	{
		CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag*>( GetItem() );
		if ( pFlag )
		{
			int nFlagTeamNumber = pFlag->GetTeamNumber();
			pFlag->Drop( this, true, true, !bSilent );
			IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
			if ( event )
			{
				event->SetInt( "player", entindex() );
				event->SetInt( "eventtype", TF_FLAGEVENT_DROPPED );
				event->SetInt( "priority", 8 );
				event->SetInt( "team", nFlagTeamNumber );

				gameeventmanager->FireEvent( event );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayer *CTFPlayer::TeamFortress_GetDisguiseTarget( int nTeam, int nClass )
{
	if ( /*nTeam == GetTeamNumber() ||*/ nTeam == TF_SPY_UNDEFINED )
	{
		// we're not disguised as the enemy team
		return NULL;
	}

	CUtlVector<int> potentialTargets;

	CBaseEntity *pLastTarget = m_Shared.GetDisguiseTarget(); // don't redisguise self as this person
	
	// Find a player on the team the spy is disguised as to pretend to be
	CTFPlayer *pPlayer = NULL;

	// Loop through players and attempt to find a player as the team/class we're disguising as
	int i;
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer && ( pPlayer != pLastTarget ) )
		{
			// First, try to find a player with the same color AND skin
			if ( ( pPlayer->GetTeamNumber() == nTeam ) && ( pPlayer->GetPlayerClass()->GetClassIndex() == nClass ) )
			{
				potentialTargets.AddToHead( i );
			}
		}
	}

	// do we have any potential targets in the list?
	if ( potentialTargets.Count() > 0 )
	{
		int iIndex = random->RandomInt( 0, potentialTargets.Count() - 1 );
		return ToTFPlayer( UTIL_PlayerByIndex( potentialTargets[iIndex] ) );
	}

	// we didn't find someone with the class, so just find someone with the same team color
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer && ( pPlayer->GetTeamNumber() == nTeam ) )
		{
			potentialTargets.AddToHead( i );
		}
	}

	if ( potentialTargets.Count() > 0 )
	{
		int iIndex = random->RandomInt( 0, potentialTargets.Count() - 1 );
		return ToTFPlayer( UTIL_PlayerByIndex( potentialTargets[iIndex] ) );
	}

	// we didn't find anyone
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float DamageForce( const Vector &size, float damage, float scale )
{ 
	float force = damage * ((48 * 48 * 82.0) / (size.x * size.y * size.z)) * scale;
	
	if ( force > 1000.0 )
	{
		force = 1000.0;
	}

	return force;
}

// we want to ship this...do not remove
ConVar tf_debug_damage( "tf_debug_damage", "0", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayer::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo info = inputInfo;

	// damage may not come from a weapon (ie: Bosses, etc)
	// The existing code below already checked for NULL pWeapon, anyways
	CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase * >( inputInfo.GetWeapon() );

	if ( GetFlags() & FL_GODMODE )
		return 0;

	if ( IsInCommentaryMode() )
		return 0;

	bool bBuddha = ( m_debugOverlays & OVERLAY_BUDDHA_MODE ) ? true : false;

#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetInt() > 1 && !IsBot() )
		bBuddha = true;
#endif // _DEBUG || STAGING_ONLY

	if ( bBuddha )
	{
		if ( ( m_iHealth - info.GetDamage() ) <= 0 )
		{
			m_iHealth = 1;
			return 0;
		}
	}

	if ( !IsAlive() )
		return 0;

	// Early out if there's no damage
	if ( !info.GetDamage() )
		return 0;

	CBaseEntity *pAttacker = info.GetAttacker();
	CTFPlayer *pTFAttacker = ToTFPlayer( pAttacker );

	bool bDebug = tf_debug_damage.GetBool();

	// Make sure the player can take damage from the attacking entity
	if ( !g_pGameRules->FPlayerCanTakeDamage( this, pAttacker, info ) )
	{
		if ( bDebug )
		{
			Warning( "    ABORTED: Player can't take damage from that attacker.\n" );
		}

		return 0;
	}

	m_iHealthBefore = GetHealth();

	if ( bDebug )
	{
		Warning( "%s taking damage from %s, via %s. Damage: %.2f\n", GetDebugName(), info.GetInflictor() ? info.GetInflictor()->GetDebugName() : "Unknown Inflictor", pAttacker ? pAttacker->GetDebugName() : "Unknown Attacker", info.GetDamage() );
	}

	// Ignore damagers on our team, to prevent capturing rocket jumping, etc.
	if ( pAttacker && pAttacker->GetTeam() != GetTeam() )
	{
		m_AchievementData.AddDamagerToHistory( pAttacker );
		if ( pAttacker->IsPlayer() )
		{
			ToTFPlayer( pAttacker )->m_AchievementData.AddTargetToHistory( this );

			// add to list of damagers via sentry so that later we can check for achievement: ACHIEVEMENT_TF_ENGINEER_SHOTGUN_KILL_PREV_SENTRY_TARGET
			CBaseEntity *pInflictor = info.GetInflictor();
			CObjectSentrygun *pSentry = dynamic_cast< CObjectSentrygun * >( pInflictor );
			if ( pSentry )
			{
				m_AchievementData.AddSentryDamager( pAttacker, pInflictor );
			}
		}
	}

	// keep track of amount of damage last sustained
	m_lastDamageAmount = info.GetDamage();
	m_LastDamageType = info.GetDamageType();

	if ( m_LastDamageType & DMG_FALL )
	{
		if ( ( m_lastDamageAmount > m_iLeftGroundHealth ) && ( m_lastDamageAmount < GetHealth() ) )
		{
			// we gained health in the air, and it saved us from death.
			// if any medics are healing us, they get an achievement
			int iNumHealers = m_Shared.GetNumHealers();
			for ( int i=0;i<iNumHealers;i++ )
			{
				CTFPlayer *pMedic = ToTFPlayer( m_Shared.GetHealerByIndex(i) );

				// if its a medic healing us
				if ( pMedic && pMedic->IsPlayerClass( TF_CLASS_MEDIC ) )
				{
					pMedic->AwardAchievement( ACHIEVEMENT_TF_MEDIC_SAVE_FALLING_TEAMMATE );
				}
			}
		}
	}

	// Check for Demo Achievement:
	// Kill a Heavy from full health with one detonation
	if ( IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
	{
		if ( pTFAttacker && pTFAttacker->IsPlayerClass( TF_CLASS_DEMOMAN ) )
		{
			if ( pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_PIPEBOMBLAUNCHER )
			{
				// We're at full health
				if ( m_iHealthBefore >= GetMaxHealth() )
				{
					// Record the time
					m_fMaxHealthTime = gpGlobals->curtime;
				}

				// If we're still being hit in the same time window
				if ( m_fMaxHealthTime == gpGlobals->curtime )
				{
					// Check if the damage is fatal
					int iDamage = info.GetDamage();
					if ( m_iHealth - iDamage <= 0 )
					{
						pTFAttacker->AwardAchievement( ACHIEVEMENT_TF_DEMOMAN_KILL_X_HEAVIES_FULLHP_ONEDET );
					}
				}
			}
		}
	}

	// Save damage force for ragdolls.
	m_vecTotalBulletForce = info.GetDamageForce();
	m_vecTotalBulletForce.x = clamp( m_vecTotalBulletForce.x, -15000.0f, 15000.0f );
	m_vecTotalBulletForce.y = clamp( m_vecTotalBulletForce.y, -15000.0f, 15000.0f );
	m_vecTotalBulletForce.z = clamp( m_vecTotalBulletForce.z, -15000.0f, 15000.0f );

	int bTookDamage = 0;
 	int bitsDamage = inputInfo.GetDamageType();

	if ( !TFGameRules()->ApplyOnDamageModifyRules( info, this ) )
	{
		return 0;
	}

	bool bFatal = ( m_iHealth - info.GetDamage() ) <= 0;
	bool bIsBot = 
#ifdef _DEBUG
	false;
#else
	( pTFAttacker && pTFAttacker->IsBot() ) || IsBot();
#endif
	bool bTrackEvent = pTFAttacker && pTFAttacker != this && !bIsBot;
	if ( bTrackEvent )
	{
		float flHealthRemoved = bFatal ? m_iHealth : info.GetDamage();
		if ( info.GetDamageBonus() && info.GetDamageBonusProvider() )
		{
			// Don't deal with raw damage numbers, only health removed.
			// Example based on a crit rocket to a player with 120 hp:
			// Actual damage is 120, but potential damage is 300, where
			// 100 is the base, and 200 is the bonus.  Apply this ratio
			// to actual (so, attacker did 40, and provider added 80).
			float flBonusMult = info.GetDamage() / abs( info.GetDamageBonus() - info.GetDamage() );
			float flBonus = flHealthRemoved - ( flHealthRemoved / flBonusMult );
			m_AchievementData.AddDamageEventToHistory( info.GetDamageBonusProvider(), flBonus );
			flHealthRemoved -= flBonus;
		}
		m_AchievementData.AddDamageEventToHistory( pAttacker, flHealthRemoved );
	}

	// This should kill us
	if ( bFatal )
	{

		// Damage could have been modified since we started
		// Try to prevent death with buddha one more time
		if ( bBuddha )
		{
			m_iHealth = 1;
			return 0;
		}
	}

	// NOTE: Deliberately skip base player OnTakeDamage, because we don't want all the stuff it does re: suit voice
	bTookDamage = CBaseCombatCharacter::OnTakeDamage( info );

	// Early out if the base class took no damage
	if ( !bTookDamage )
	{
		if ( bDebug )
		{
			Warning( "    ABORTED: Player failed to take the damage.\n" );
		}
		return 0;
	}

	if ( bTookDamage == false )
		return 0;

	if ( bDebug )
	{
		Warning( "    DEALT: Player took %.2f damage.\n", info.GetDamage() );
		Warning( "    HEALTH LEFT: %d\n", GetHealth() );
	}

	// Send the damage message to the client for the hud damage indicator
	// Try and figure out where the damage is coming from
	Vector vecDamageOrigin = info.GetReportedPosition();

	// If we didn't get an origin to use, try using the attacker's origin
	if ( vecDamageOrigin == vec3_origin && info.GetInflictor() )
	{
		vecDamageOrigin = info.GetInflictor()->GetAbsOrigin();
	}

	// Tell the player's client that he's been hurt.
	if ( m_iHealthBefore != GetHealth() )
	{
 		CSingleUserRecipientFilter user( this );
		UserMessageBegin( user, "Damage" );
			WRITE_SHORT( clamp( (int)info.GetDamage(), 0, 32000 ) );
			WRITE_LONG( info.GetDamageType() );
			// Tell the client whether they should show it in the indicator
			if ( bitsDamage != DMG_GENERIC && !(bitsDamage & (DMG_DROWN | DMG_FALL | DMG_BURN) ) )
			{
				WRITE_BOOL( true );
				WRITE_VEC3COORD( vecDamageOrigin );
			}
			else
			{
				WRITE_BOOL( false );
			}
		MessageEnd();
	}

	// add to the damage total for clients, which will be sent as a single
	// message at the end of the frame
	// todo: remove after combining shotgun blasts?
	if ( info.GetInflictor() && info.GetInflictor()->edict() )
	{
		m_DmgOrigin = info.GetInflictor()->GetAbsOrigin();
	}

	m_DmgTake += (int)info.GetDamage();

	// Reset damage time countdown for each type of time based damage player just sustained
	for (int i = 0; i < CDMG_TIMEBASED; i++)
	{
		// Make sure the damage type is really time-based.
		// This is kind of hacky but necessary until we setup DamageType as an enum.
		int iDamage = ( DMG_PARALYZE << i );
		if ( ( info.GetDamageType() & iDamage ) && g_pGameRules->Damage_IsTimeBased( iDamage ) )
		{
			m_rgbTimeBasedDamage[i] = 0;
		}
	}

	// Display any effect associate with this damage type
	DamageEffect( info.GetDamage(),bitsDamage );

	m_bitsDamageType |= bitsDamage; // Save this so we can report it to the client
	m_bitsHUDDamage = -1;  // make sure the damage bits get reset

	// Flinch
	bool bFlinch = true;
	if ( bitsDamage != DMG_GENERIC )
	{
		if ( IsPlayerClass( TF_CLASS_SNIPER ) && m_Shared.InCond( TF_COND_AIMING ) )
		{
			if ( pTFAttacker && pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_MINIGUN )
			{
				float flDistSqr = ( pTFAttacker->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr();
				if ( flDistSqr > 750 * 750 )
				{
					bFlinch = false;
				}
			}
		}

		if ( bFlinch )
		{
			if ( ApplyPunchImpulseX( -2 ) ) 
			{
				PlayFlinch( info );
			}
		}
	}

	// Do special explosion damage effect
	if ( bitsDamage & DMG_BLAST )
	{
		OnDamagedByExplosion( info );
	}

	if ( m_iHealthBefore != GetHealth() )
	{
		PainSound( info );
	}

	// Detect drops below 25% health and restart expression, so that characters look worried.
	int iHealthBoundary = (GetMaxHealth() * 0.25);
	if ( GetHealth() <= iHealthBoundary && m_iHealthBefore > iHealthBoundary )
	{
		ClearExpression();
	}

#ifdef _DEBUG
	// Report damage from the info in debug so damage against targetdummies goes
	// through the system, as m_iHealthBefore - GetHealth() will always be 0.
	CTF_GameStats.Event_PlayerDamage( this, info, info.GetDamage() );
#else
	CTF_GameStats.Event_PlayerDamage( this, info, m_iHealthBefore - GetHealth() );
#endif // _DEBUG

	// if we take damage after we leave the ground, update the health if its less
	if ( bTookDamage && m_iLeftGroundHealth > 0 )
	{
		if ( GetHealth() < m_iLeftGroundHealth )
		{
			m_iLeftGroundHealth = GetHealth();
		}
	}
	
	if ( IsPlayerClass( TF_CLASS_SPY ) )
	{
		// Trigger feign death if the player has it prepped...
		if ( !( info.GetDamageType() & DMG_FALL ) )
		{
			m_Shared.NoteLastDamageTime( m_lastDamageAmount );
		}
	}

	// Let attacker react to the damage they dealt
	if ( pTFAttacker )
	{
		pTFAttacker->OnDealtDamage( this, info );
	}

	return info.GetDamage();
}



//-----------------------------------------------------------------------------
// Purpose: Invoked when we deal damage to another victim
//-----------------------------------------------------------------------------
void CTFPlayer::OnDealtDamage( CBaseCombatCharacter *pVictim, const CTakeDamageInfo &info )
{
	float flDamage = info.GetDamage();

	if ( pVictim )
	{
		// which second of the window are we in
		int i = (int)gpGlobals->curtime;
		i %= DPS_Period;

		if ( i != m_lastDamageRateIndex )
		{
			// a second has ticked over, start a new accumulation
			m_damageRateArray[ i ] = flDamage;
			m_lastDamageRateIndex = i;

			// track peak DPS for this player
			m_peakDamagePerSecond = 0;
			for( i=0; i<DPS_Period; ++i )
			{
				if ( m_damageRateArray[i] > m_peakDamagePerSecond )
				{
					m_peakDamagePerSecond = m_damageRateArray[i];
				}
			}
		}
		else
		{
			m_damageRateArray[ i ] += flDamage;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::AddConnectedPlayers( CUtlVector<CTFPlayer*> &vecPlayers, CTFPlayer *pPlayerToConsider )
{
	if ( !pPlayerToConsider )
		return;

	if ( vecPlayers.Find( pPlayerToConsider ) != vecPlayers.InvalidIndex() )
		return; // already in the list

	vecPlayers.AddToTail( pPlayerToConsider );
  
	if ( pPlayerToConsider->MedicGetHealTarget() )
	{
		AddConnectedPlayers( vecPlayers, ToTFPlayer( pPlayerToConsider->MedicGetHealTarget() ) );
	}

	for ( int i = 0 ; i < pPlayerToConsider->m_Shared.GetNumHealers() ; i++ )
	{
		CTFPlayer *pMedic = ToTFPlayer( pPlayerToConsider->m_Shared.GetHealerByIndex( i ) );
		if ( pMedic )
		{
			AddConnectedPlayers( vecPlayers, pMedic );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::DamageEffect(float flDamage, int fDamageType)
{
	bool bDisguised = m_Shared.InCond( TF_COND_DISGUISED );

	if (fDamageType & DMG_CRUSH)
	{
		//Red damage indicator
		color32 red = {128,0,0,128};
		UTIL_ScreenFade( this, red, 1.0f, 0.1f, FFADE_IN );
	}
	else if (fDamageType & DMG_DROWN)
	{
		//Red damage indicator
		color32 blue = {0,0,128,128};
		UTIL_ScreenFade( this, blue, 1.0f, 0.1f, FFADE_IN );
	}
	else if (fDamageType & DMG_SLASH)
	{
		if ( !bDisguised )
		{
			// If slash damage shoot some blood
			SpawnBlood(EyePosition(), g_vecAttackDir, BloodColor(), flDamage);
		}
	}
	else if ( fDamageType & DMG_BULLET )
	{
		if ( !bDisguised )
		{
			EmitSound( "Flesh.BulletImpact" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : collisionGroup - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	if ( ( ( collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT ) && tf_avoidteammates.GetBool() ) ||
		collisionGroup == TFCOLLISION_GROUP_ROCKETS || collisionGroup == TFCOLLISION_GROUP_ROCKET_BUT_NOT_WITH_OTHER_ROCKETS )
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

//---------------------------------------
// Is the player the passed player class?
//---------------------------------------
bool CTFPlayer::IsPlayerClass( int iClass ) const
{
	const CTFPlayerClass *pClass = &m_PlayerClass;

	if ( !pClass )
		return false;

	return ( pClass->IsClass( iClass ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::CommitSuicide( bool bExplode /* = false */, bool bForce /*= false*/ )
{
	// Don't suicide if we haven't picked a class for the first time, or we're not in active state
	if ( IsPlayerClass( TF_CLASS_UNDEFINED ) || !m_Shared.InState( TF_STATE_ACTIVE ) )
		return;

	// Don't suicide during the "bonus time" if we're not on the winning team
	if ( !bForce && TFGameRules()->State_Get() == GR_STATE_TEAM_WIN && 
		 GetTeamNumber() != TFGameRules()->GetWinningTeam() )
	{
		return;
	}

	if ( TFGameRules()->ShowMatchSummary() )
		return;

	m_bSuicideExplode = bExplode;
	m_iSuicideCustomKillFlags = TF_DMG_CUSTOM_SUICIDE;

	BaseClass::CommitSuicide( bExplode, bForce );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : int
//-----------------------------------------------------------------------------
ConVar tf_preround_push_from_damage_enable( "tf_preround_push_from_damage_enable", "0", FCVAR_NONE, "If enabled, this will allow players using certain type of damage to move during pre-round freeze time." );
void CTFPlayer::ApplyPushFromDamage( const CTakeDamageInfo &info, Vector vecDir )
{
	// check if player can be moved
	if ( !tf_preround_push_from_damage_enable.GetBool() && !CanPlayerMove() )
		return;

	if ( m_bIsTargetDummy )
		return;

	Vector vecForce;
	vecForce.Init();
	if ( info.GetAttacker() == this )
	{
		Vector vecSize = WorldAlignSize();
		Vector hullSizeCrouch = VEC_DUCK_HULL_MAX - VEC_DUCK_HULL_MIN;

		if ( vecSize == hullSizeCrouch )
		{
			// Use the original hull for damage force calculation to ensure our RJ height doesn't change due to crouch hull increase
			// ^^ Comment above is an ancient lie, Ducking actually increases blast force, this value increases it even more 82 standing, 62 ducking, 55 modified
			vecSize.z = 55;
		}

		float flDamageForForce = info.GetDamageForForceCalc() ? info.GetDamageForForceCalc() : info.GetDamage();

		if ( IsPlayerClass( TF_CLASS_SOLDIER ) )
		{
			// Rocket Jump
			if ( (info.GetDamageType() & DMG_BLAST) )
			{
				if ( GetFlags() & FL_ONGROUND )
				{
					vecForce = vecDir * -DamageForce( vecSize, flDamageForForce, tf_damageforcescale_self_soldier_badrj.GetFloat() );
				}
				else
				{
					vecForce = vecDir * -DamageForce( vecSize, flDamageForForce, tf_damageforcescale_self_soldier_rj.GetFloat() );
				}

				// Reset duck in air on self rocket impulse.
				m_Shared.SetAirDucked( 0 );
			}
			else
			{
				// Self Damage no force
				vecForce.Zero();
			}
			
		}
		else
		{
			// Other Jumps (Stickies)
			vecForce = vecDir * -DamageForce( vecSize, flDamageForForce, DAMAGE_FORCE_SCALE_SELF );
			
			// Reset duck in air on self grenade impulse.
			m_Shared.SetAirDucked( 0 );
		}
	}
	else
	{
		// Sentryguns push a lot harder
		if ( (info.GetDamageType() & DMG_BULLET) && info.GetInflictor() && info.GetInflictor()->IsBaseObject() )
		{
			float flSentryPushMultiplier = 16.f;
			CObjectSentrygun* pSentry = dynamic_cast<CObjectSentrygun*>( info.GetInflictor() );
			if ( pSentry )
			{
				flSentryPushMultiplier = 16.f;

				// Scale the force based on Distance, Wrangled Sentries should not push so hard at distance
				// get the distance between sentry and victim and lower push force if outside of attack range (wrangled)
				float flDistSqr = (pSentry->GetAbsOrigin() - GetAbsOrigin()).LengthSqr();
				if ( flDistSqr > SENTRY_MAX_RANGE_SQRD )
				{
					flSentryPushMultiplier *= 0.5f;
				}
			}
			vecForce = vecDir * -DamageForce( WorldAlignSize(), info.GetDamage(), flSentryPushMultiplier );
		}
		else
		{
			vecForce = vecDir * -DamageForce( WorldAlignSize(), info.GetDamage(), tf_damageforcescale_other.GetFloat() );
		
			if ( IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
			{
				// Heavies take less push from non sentryguns
				vecForce *= 0.5f;
			}
		}
	}

	ApplyAbsVelocityImpulse( vecForce );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : int
//-----------------------------------------------------------------------------
int CTFPlayer::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if ( TFGameRules()->IsInItemTestingMode() && !IsFakeClient() )
		return 0;

	// Always NULL check this below
	CTFPlayer *pTFAttacker = ToTFPlayer( info.GetAttacker() );

	bool bIgniting = false;
	float realDamage = info.GetDamage();
	if ( TFGameRules() )
	{
		realDamage = TFGameRules()->ApplyOnDamageAliveModifyRules( info, this, bIgniting );

		if ( realDamage == -1 )
		{
			// Hard out requested from ApplyOnDamageModifyRules 
			return 0;
		}
	}

	// Grab the vector of the incoming attack. 
	// (Pretend that the inflictor is a little lower than it really is, so the body will tend to fly upward a bit).
	Vector vecDir = vec3_origin;
	if ( info.GetInflictor() )
	{
		vecDir = info.GetInflictor()->WorldSpaceCenter() - Vector ( 0.0f, 0.0f, 10.0f ) - WorldSpaceCenter();
		info.GetInflictor()->AdjustDamageDirection( info, vecDir, this );
		VectorNormalize( vecDir );
	}
	g_vecAttackDir = vecDir;

	// Do the damage.
	m_bitsDamageType |= info.GetDamageType();

	int iPrevHealth = m_iHealth;

	if ( m_takedamage != DAMAGE_EVENTS_ONLY )
	{
		// Take damage - round to the nearest integer.
		int iOldHealth = m_iHealth;
		m_iHealth -= ( realDamage + 0.5f );

		// track accumulated sentry gun damage dealt by players
		if ( pTFAttacker )
		{
			// track amount of damage dealt by defender's sentry guns
			CObjectSentrygun *sentry = dynamic_cast< CObjectSentrygun * >( info.GetInflictor() );
			CTFProjectile_SentryRocket *sentryRocket = dynamic_cast< CTFProjectile_SentryRocket * >( info.GetInflictor() );

			if ( ( sentry ) || sentryRocket )
			{
				int flooredHealth = clamp( m_iHealth, 0, m_iHealth );

				pTFAttacker->AccumulateSentryGunDamageDealt( iOldHealth - flooredHealth );
			}
		}
	}

	m_flLastDamageTime = gpGlobals->curtime; // not networked

	// Apply a damage force.
	CBaseEntity *pAttacker = info.GetAttacker();
	if ( !pAttacker )
		return 0;

	if ( ( info.GetDamageType() & DMG_PREVENT_PHYSICS_FORCE ) == 0 )
	{
		if ( info.GetInflictor() && ( GetMoveType() == MOVETYPE_WALK ) && 
		   ( !pAttacker->IsSolidFlagSet( FSOLID_TRIGGER ) ) && 
		   ( !m_Shared.InCond( TF_COND_DISGUISED ) ) )	
		{
			ApplyPushFromDamage( info, vecDir );
		}
	}

	if ( bIgniting && pTFAttacker )
	{
		m_Shared.Burn( pTFAttacker, dynamic_cast< CTFWeaponBase * >( info.GetWeapon() ) );
	}

	// Fire a global game event - "player_hurt"
	IGameEvent * event = gameeventmanager->CreateEvent( "player_hurt" );
	if ( event )
	{
		event->SetInt( "userid", GetUserID() );
		event->SetInt( "health", MAX( 0, m_iHealth ) );

		// HLTV event priority, not transmitted
		event->SetInt( "priority", 5 );	

		int iDamageAmount = ( iPrevHealth - m_iHealth );
		event->SetInt( "damageamount", iDamageAmount );

		// Hurt by another player.
		if ( pAttacker->IsPlayer() )
		{
			CBasePlayer *pPlayer = ToBasePlayer( pAttacker );
			event->SetInt( "attacker", pPlayer->GetUserID() );
			
			event->SetInt( "custom", info.GetDamageCustom() );
			event->SetBool( "showdisguisedcrit", m_bShowDisguisedCrit );
			event->SetBool( "crit", (info.GetDamageType() & DMG_CRITICAL) != 0 );
			event->SetBool( "allseecrit", m_bAllSeeCrit );
			Assert( (int)m_eBonusAttackEffect < 256 );
			event->SetInt( "bonuseffect", (int)m_eBonusAttackEffect );

			if ( pTFAttacker && pTFAttacker->GetActiveTFWeapon() )
			{
				event->SetInt( "weaponid", pTFAttacker->GetActiveTFWeapon()->GetWeaponID() );
			}
		}
		// Hurt by world.
		else
		{
			event->SetInt( "attacker", 0 );
		}

        gameeventmanager->FireEvent( event );
	}
	
	if ( pTFAttacker && pTFAttacker != this )
	{
		pTFAttacker->RecordDamageEvent( info, (m_iHealth <= 0), iPrevHealth );
	}
	
	//No bleeding while invul or disguised.
	bool bBleed = ( ( m_Shared.InCond( TF_COND_DISGUISED ) == false || m_Shared.GetDisguiseTeam() != pAttacker->GetTeamNumber() )
					&& !m_Shared.IsInvulnerable() );

	// No bleed effects for DMG_GENERIC
	if ( info.GetDamageType() == 0 )
	{
		bBleed = false;
	}
	
	if ( bBleed && pTFAttacker )
	{
		CTFWeaponBase *pWeapon = pTFAttacker->GetActiveTFWeapon();
		if ( pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_FLAMETHROWER )
		{
			bBleed = false;
		}
	}

	if ( bBleed && ( realDamage > 0.f ) )
	{
		Vector vDamagePos = info.GetDamagePosition();

		if ( vDamagePos == vec3_origin )
		{
			vDamagePos = WorldSpaceCenter();
		}

		CPVSFilter filter( vDamagePos );
		TE_TFBlood( filter, 0.0, vDamagePos, -vecDir, entindex() );
	}

	if ( m_bIsTargetDummy )
	{
		// In the case of a targetdummy bot, restore any damage so it can never die
		TakeHealth( ( iPrevHealth - m_iHealth ), DMG_GENERIC );
	}

	m_vecFeignDeathVelocity = GetAbsVelocity();

	if ( pTFAttacker )
	{
		// If we're invuln, give whomever provided it rewards/credit
		if ( m_Shared.IsInvulnerable() && realDamage > 0.f )
		{
			// Medigun?
			CBaseEntity *pProvider = m_Shared.GetConditionProvider( TF_COND_INVULNERABLE );
			if ( pProvider )
			{
				CTFPlayer *pTFProvider = ToTFPlayer( pProvider );
				if ( pTFProvider )
				{
					CTF_GameStats.Event_PlayerBlockedDamage( pTFProvider, realDamage );
				}
			}
		}
	}

	// Done.
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldGib( const CTakeDamageInfo &info )
{
	// Check to see if we should allow players to gib.
	if ( tf_playergib.GetInt() != 1 )
	{
		if ( tf_playergib.GetInt() < 1 )
			return false;
		else
			return true;
	}

	// Suicide explode always gibs.
	if ( m_bSuicideExplode )
	{
		m_bSuicideExplode = false;
		return true;
	}

	// Only blast & half falloff damage can gib.
	if ( ( (info.GetDamageType() & DMG_BLAST) == 0 ) &&
		( (info.GetDamageType() & DMG_HALF_FALLOFF) == 0 ) )
		return false;

	// Explosive crits always gib.
	if ( info.GetDamageType() & DMG_CRITICAL )
		return true;

	// Hard hits also gib.
	if ( GetHealth() <= -10 )
		return true;
	
	if ( m_bGoingFeignDeath )
	{
		// The player won't actually have negative health,
		// but spies often gib from explosive damage so we should make that likely here.
		float frand = (float) rand() / VALVE_RAND_MAX;
		return (frand>0.15f) ? true : false;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	BaseClass::Event_KilledOther( pVictim, info );

	if ( pVictim->IsPlayer() )
	{
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );

		// Custom death handlers
		// TODO: Need a system here!  This conditional is getting pretty big.
		const char *pszCustomDeath = "customdeath:none";
		if ( info.GetAttacker() && info.GetAttacker()->IsBaseObject() )
		{
			pszCustomDeath = "customdeath:sentrygun";
		}
		else if ( info.GetInflictor() && info.GetInflictor()->IsBaseObject() )
		{
			pszCustomDeath = "customdeath:sentrygun";
		}
		else if ( IsHeadshot( info.GetDamageCustom() ) )
		{				
			pszCustomDeath = "customdeath:headshot";
		}
		else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BACKSTAB )
		{
			pszCustomDeath = "customdeath:backstab";
		}
		else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BURNING )
		{
			pszCustomDeath = "customdeath:burning";
		}

		// Revenge handler
		const char *pszDomination = "domination:none";
		if ( pTFVictim->GetDeathFlags() & (TF_DEATH_REVENGE|TF_DEATH_ASSISTER_REVENGE) )
		{
			pszDomination = "domination:revenge";
		}
		else if ( pTFVictim->GetDeathFlags() & TF_DEATH_DOMINATION )
		{
			pszDomination = "domination:dominated";
		}

		const char *pszVictimStunned = "victimstunned:0";

		const char *pszVictimDoubleJumping = "victimdoublejumping:0";
		if ( pTFVictim->m_Shared.GetAirDash() > 0 )
		{
			pszVictimDoubleJumping = "victimdoublejumping:1";
		}

		CFmtStrN<128> modifiers( "%s,%s,%s,%s,victimclass:%s", pszCustomDeath, pszDomination, pszVictimStunned, pszVictimDoubleJumping, g_aPlayerClassNames_NonLocalized[ pTFVictim->GetPlayerClass()->GetClassIndex() ] );

		SpeakConceptIfAllowed( MP_CONCEPT_KILLED_PLAYER, modifiers );
		
		// Check for CP_Foundry achievements
		if ( FStrEq( "cp_foundry", STRING( gpGlobals->mapname ) ) )
		{
			if ( pTFVictim && ( pTFVictim->GetTeamNumber() != GetTeamNumber() ) )
			{
				if ( pTFVictim->IsCapturingPoint() )
				{
					if ( info.GetDamageType() & DMG_CRITICAL )
					{
						AwardAchievement( ACHIEVEMENT_TF_MAPS_FOUNDRY_KILL_CAPPING_ENEMY );
					}
				}

				if ( InAchievementZone( pTFVictim ) )
				{
					IGameEvent *event = gameeventmanager->CreateEvent( "player_killed_achievement_zone" );
					if ( event )
					{
						event->SetInt( "attacker", entindex() );
						event->SetInt( "victim", pTFVictim->entindex() );
						gameeventmanager->FireEvent( event );
					}
				}
			}
		}
		
		// Check for SD_Doomsday achievements		
		if ( FStrEq( "sd_doomsday", STRING( gpGlobals->mapname ) ) )
		{
			if ( pTFVictim && ( pTFVictim->GetTeamNumber() != GetTeamNumber() ) )
			{
				// find the flag in the map
				CCaptureFlag *pFlag = NULL;
				for ( int i=0; i<ICaptureFlagAutoList::AutoList().Count(); ++i )
				{
					pFlag = static_cast< CCaptureFlag* >( ICaptureFlagAutoList::AutoList()[i] );
					if ( !pFlag->IsDisabled() )
					{
						break;
					}
				}

				// was the victim in an achievement zone?
				CAchievementZone *pZone = InAchievementZone( pTFVictim );
				if ( pZone )
				{
					int iZoneID = pZone->GetZoneID();
					if ( iZoneID == 0 )
					{
						if ( pFlag && pFlag->IsHome() )
						{
							AwardAchievement( ACHIEVEMENT_TF_MAPS_DOOMSDAY_DENY_NEUTRAL_PICKUP );
						}
					}
					else
					{
						IGameEvent *event = gameeventmanager->CreateEvent( "player_killed_achievement_zone" );
						if ( event )
						{
							event->SetInt( "attacker", entindex() );
							event->SetInt( "victim", pTFVictim->entindex() );
							event->SetInt( "zone_id", iZoneID );
							gameeventmanager->FireEvent( event );
						}
					}
				}

				// check the flag carrier to see if the victim has recently damaged them
				if ( pFlag && pFlag->IsStolen() )
				{
					CTFPlayer *pFlagCarrier = ToTFPlayer( pFlag->GetOwnerEntity() );
					if ( pFlagCarrier && ( pFlagCarrier->GetTeamNumber() == GetTeamNumber() ) )
					{
						// has the victim damaged the flag carrier in the last 3 seconds?
						if ( pFlagCarrier->m_AchievementData.IsDamagerInHistory( pTFVictim, 3.0 ) )
						{
							AwardAchievement( ACHIEVEMENT_TF_MAPS_DOOMSDAY_DEFEND_CARRIER );
						}
					}
				}
			}
		}

		// Check for CP_Snakewater achievement
		if ( FStrEq( "cp_snakewater_final1", STRING( gpGlobals->mapname ) ) )
		{
			if ( pTFVictim && ( pTFVictim->GetTeamNumber() != GetTeamNumber() ) )
			{
				if ( InAchievementZone( pTFVictim ) )
				{
					IGameEvent *event = gameeventmanager->CreateEvent( "player_killed_achievement_zone" );
					if ( event )
					{
						event->SetInt( "attacker", entindex() );
						event->SetInt( "victim", pTFVictim->entindex() );
						gameeventmanager->FireEvent( event );
					}
				}
			}
		}

		if ( IsPlayerClass( TF_CLASS_DEMOMAN ) )
		{
			if ( pVictim->GetTeamNumber() != GetTeamNumber() )
			{
				// Demoman achievement:  Kill at least 3 players capping or pushing the cart with the same detonation
				CTriggerAreaCapture *pAreaTrigger = pTFVictim->GetControlPointStandingOn();
				if ( pAreaTrigger )
				{
					CTeamControlPoint *pCP = pAreaTrigger->GetControlPoint();
					if ( pCP )
					{
						if ( pCP->GetOwner() == GetTeamNumber() )
						{
							if ( GetActiveTFWeapon() && ( GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_PIPEBOMBLAUNCHER ) )
							{
								// Add victim to our list
								int iIndex = m_Cappers.Find( pTFVictim->GetUserID() );
								if ( iIndex != m_Cappers.InvalidIndex() )
								{
									// they're already in our list
									m_Cappers[iIndex] = gpGlobals->curtime;
								}
								else
								{
									// we need to add them
									m_Cappers.Insert( pTFVictim->GetUserID(), gpGlobals->curtime );
								}
								// Did we get three?
								if ( m_Cappers.Count() >= 3 )
								{
									// Traverse the list, comparing the recorded time to curtime
									int iHitCount = 0;
									FOR_EACH_MAP_FAST ( m_Cappers, cIndex )
									{
										// For each match, increment counter
										if ( gpGlobals->curtime <= m_Cappers[cIndex] + 0.1f )
										{
											iHitCount++;
										}
										else
										{
											m_Cappers.Remove( cIndex );
										}
										
										// If we hit 3, award and purge the group
										if ( iHitCount >= 3 )
										{
											AwardAchievement( ACHIEVEMENT_TF_DEMOMAN_KILL_X_CAPPING_ONEDET );
											m_Cappers.RemoveAll();
										}					
									}
								}
							}
						}
						// Kill players defending "x" times
						else
						{
							// If we're able to cap the point...
							if ( TeamplayGameRules()->TeamMayCapturePoint( GetTeamNumber(), pCP->GetPointIndex() ) && 
								TeamplayGameRules()->PlayerMayCapturePoint( this, pCP->GetPointIndex() ) )
							{
								AwardAchievement( ACHIEVEMENT_TF_DEMOMAN_KILL_X_DEFENDING );
							}
						}
					}
				}
			}
		}

		for ( int i=0; i<m_Shared.m_nNumHealers; i++ )
		{
			m_Shared.m_aHealers[i].iKillsWhileBeingHealed++;
			if ( IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
			{
				if ( m_Shared.m_aHealers[i].iKillsWhileBeingHealed >= 5 && m_Shared.m_aHealers[i].bDispenserHeal )
				{
					// We got five kills while being healed by this dispenser. Reward the engineer with an achievement!
					CTFPlayer *pHealScorer = ToTFPlayer( m_Shared.m_aHealers[i].pHealScorer );
					if ( pHealScorer && pHealScorer->IsPlayerClass( TF_CLASS_ENGINEER ) )
					{
						pHealScorer->AwardAchievement( ACHIEVEMENT_TF_ENGINEER_HEAVY_ASSIST );
					}
				}
			}
		}

		// track accumulated sentry gun kills on owning player for Sentry Busters in MvM (so they can't clear this by rebuilding their sentry)
		CObjectSentrygun *sentry = dynamic_cast< CObjectSentrygun * >( info.GetInflictor() );
		CTFProjectile_SentryRocket *sentryRocket = dynamic_cast< CTFProjectile_SentryRocket * >( info.GetInflictor() );

		if ( ( sentry ) || sentryRocket )
		{
			IncrementSentryGunKillCount();
		}

		if ( pTFVictim )
		{
			// was the victim on a control point (includes payload carts)
			CTriggerAreaCapture *pAreaTrigger = pTFVictim->GetControlPointStandingOn();
			if ( pAreaTrigger )
			{
				CTeamControlPoint *pCP = pAreaTrigger->GetControlPoint();
				if ( pCP && ( pCP->GetOwner() != pTFVictim->GetTeamNumber() ) )
				{
					if ( TeamplayGameRules()->TeamMayCapturePoint( pTFVictim->GetTeamNumber(), pCP->GetPointIndex() ) &&
						TeamplayGameRules()->PlayerMayCapturePoint( pTFVictim, pCP->GetPointIndex() ) )
					{
						CTFPlayer *pTFAssister = NULL;
						if ( TFGameRules() ) 
						{
							pTFAssister = ToTFPlayer( TFGameRules()->GetAssister( pTFVictim, this, info.GetInflictor() ) );
						}

						IGameEvent *event = gameeventmanager->CreateEvent( "killed_capping_player" );
						if ( event )
						{
							event->SetInt( "cp", pCP->GetPointIndex() );
							event->SetInt( "killer", entindex() );
							event->SetInt( "victim", pTFVictim->entindex() );
							event->SetInt( "assister", pTFAssister ? pTFAssister->entindex() : -1 );
							event->SetInt( "priority", 9 );

							gameeventmanager->FireEvent( event );
						}
					}
				}
			}
		}
	}
	else
	{
		if ( pVictim->IsBaseObject() )
		{
			CBaseObject *pObject = dynamic_cast<CBaseObject *>( pVictim );
			SpeakConceptIfAllowed( MP_CONCEPT_KILLED_OBJECT, pObject->GetResponseRulesModifier() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::Event_Killed( const CTakeDamageInfo &info )
{
	CTFPlayer *pPlayerAttacker = NULL;
	if ( info.GetAttacker() && info.GetAttacker()->IsPlayer() )
	{
		pPlayerAttacker = ToTFPlayer( info.GetAttacker() );
	}

	CTFWeaponBase *pKillerWeapon = NULL;
	if ( pPlayerAttacker )
	{
		pKillerWeapon = dynamic_cast < CTFWeaponBase * > ( info.GetWeapon() );
	}

	if ( m_Shared.InCond( TF_COND_TAUNTING ) )
	{
		StopTaunt();
	}

	SpeakConceptIfAllowed( MP_CONCEPT_DIED );

	StateTransition( TF_STATE_DYING );	// Transition into the dying state.

	bool bOnGround = GetFlags() & FL_ONGROUND;
	bool bElectrocuted = false;
	bool bDisguised = m_Shared.InCond( TF_COND_DISGUISED );
	// we want the rag doll to burn if the player was burning and was not a pyro (who only burns momentarily)
	bool bBurning = m_Shared.InCond( TF_COND_BURNING ) && ( TF_CLASS_PYRO != GetPlayerClass()->GetClassIndex() );
	CTFPlayer *pOriginalBurner = m_Shared.GetOriginalBurnAttacker();
	CTFPlayer *pLastBurner = m_Shared.GetBurnAttacker();

	if ( m_aBurnFromBackAttackers.Count() > 0 )
	{
		CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>(info.GetWeapon());
		if ( pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_FLAMETHROWER )
		{
			for ( int i = 0; i < m_aBurnFromBackAttackers.Count(); i++ )
			{
				CTFPlayer *pBurner = ToTFPlayer( m_aBurnFromBackAttackers[i].Get() );

				if ( pBurner )
				{
					pBurner->AwardAchievement( ACHIEVEMENT_TF_PYRO_KILL_FROM_BEHIND );
				}
			}
		}

		ClearBurnFromBehindAttackers();
	}

	if ( IsPlayerClass( TF_CLASS_MEDIC ) )
	{
		CWeaponMedigun* pMedigun = assert_cast<CWeaponMedigun*>( Weapon_OwnsThisID( TF_WEAPON_MEDIGUN ) );
		float flChargeLevel = pMedigun ? pMedigun->GetChargeLevel() : 0.f;
		float flMinChargeLevel = pMedigun ? pMedigun->GetMinChargeAmount() : 1.f;

		bool bCharged = flChargeLevel >= flMinChargeLevel;

		if ( bCharged )
		{
			bElectrocuted = true;
			if ( pPlayerAttacker )
			{
				if ( pPlayerAttacker->IsPlayerClass( TF_CLASS_SCOUT ) )
				{
					pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_SCOUT_KILL_CHARGED_MEDICS );
				}
				else if ( pPlayerAttacker->IsPlayerClass( TF_CLASS_SNIPER ) )
				{
					pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_SNIPER_KILL_CHARGED_MEDIC );
				}
				else if ( pPlayerAttacker->IsPlayerClass( TF_CLASS_SPY ) )
				{
					if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BACKSTAB )
					{
						pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_SPY_BACKSTAB_MEDIC_CHARGED );
					}
				}

				CTF_GameStats.Event_PlayerAwardBonusPoints( pPlayerAttacker, this, 20 );
			}
		}

		IGameEvent * event = gameeventmanager->CreateEvent( "medic_death" );
		if ( event )
		{
			int iHealing = 0;

			PlayerStats_t *pPlayerStats = CTF_GameStats.FindPlayerStats( this );
			if ( pPlayerStats )
			{
				iHealing = pPlayerStats->statsCurrentLife.m_iStat[TFSTAT_HEALING];

				// defensive fix for the moment for bug where healing value becomes bogus sometimes: if bogus, slam it to 0
				// ...copied from CTFGameRules::CalcPlayerScore()
				if ( iHealing < 0 || iHealing > 10000000 )
				{
					iHealing = 0;
				}
			}

			event->SetInt( "userid", GetUserID() );
			event->SetInt( "attacker", pPlayerAttacker ? pPlayerAttacker->GetUserID() : 0 );
			event->SetInt( "healing", iHealing );
			event->SetBool( "charged", bCharged );

			gameeventmanager->FireEvent( event );
		}
	}
	else if ( IsPlayerClass( TF_CLASS_ENGINEER ) )
	{
		if ( pPlayerAttacker && pPlayerAttacker->IsPlayerClass( TF_CLASS_SOLDIER ) )
		{
			// Has Engineer worked on his sentrygun recently?
			CBaseObject	*pSentry = GetObjectOfType( OBJ_SENTRYGUN );
			if ( pSentry && m_AchievementData.IsTargetInHistory( pSentry, 4.0 ) )
			{
				if ( pSentry->m_AchievementData.CountDamagersWithinTime( 3.0 ) > 0 )
				{
					pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_SOLDIER_KILL_ENGY );
				}
			}
		}
	}

	if ( pPlayerAttacker )
	{
		if ( pPlayerAttacker->IsPlayerClass( TF_CLASS_SNIPER ) )
		{
			if ( pKillerWeapon && WeaponID_IsSniperRifle( pKillerWeapon->GetWeaponID() ) && pPlayerAttacker->m_Shared.IsAiming() == false )
			{
				pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_SNIPER_KILL_UNSCOPED );
			}
		}
		else if ( pPlayerAttacker->IsPlayerClass( TF_CLASS_SPY ) )
		{
			CTriggerAreaCapture *pAreaTrigger = GetControlPointStandingOn();
			if ( pAreaTrigger )
			{
				CTeamControlPoint *pCP = pAreaTrigger->GetControlPoint();
				if ( pCP )
				{
					if ( pCP->GetOwner() == GetTeamNumber() )
					{
						// killed on a control point owned by my team
						pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_SPY_KILL_CP_DEFENDERS );
					}
					else
					{
						// killed on a control point NOT owned by my team, was it a backstab?
						if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BACKSTAB )
						{
							// was i able to capture the control point?
							if ( TeamplayGameRules()->TeamMayCapturePoint( GetTeamNumber(), pCP->GetPointIndex() ) && 
								 TeamplayGameRules()->PlayerMayCapturePoint( this, pCP->GetPointIndex() ) )
							{
								pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_SPY_BACKSTAB_CAPPING_ENEMIES );
							}
						}
					}
				}
			}

			if ( IsPlayerClass( TF_CLASS_ENGINEER ) )
			{
				//m_AchievementData.CountTargetsWithinTime
				int iHistory = 0;
				EntityHistory_t *pHistory = m_AchievementData.GetTargetHistory( iHistory );

				while ( pHistory )
				{
					if ( pHistory->hEntity && pHistory->hEntity->IsBaseObject() && m_AchievementData.IsTargetInHistory( pHistory->hEntity, 1.0f ) )
					{
						CBaseObject *pObject = dynamic_cast<CBaseObject *>( pHistory->hEntity.Get() );
					
						if ( pObject->ObjectType() == OBJ_SENTRYGUN )
						{
							pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_SPY_KILL_WORKING_ENGY );
							break;
						}
					}

					iHistory++;
					pHistory = m_AchievementData.GetTargetHistory( iHistory );
				}
			}
		}
		else if ( pPlayerAttacker->IsPlayerClass( TF_CLASS_DEMOMAN ) )
		{
			// Kill "x" players with a direct pipebomb hit
			if ( pPlayerAttacker->GetActiveTFWeapon() && ( pPlayerAttacker->GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_GRENADELAUNCHER ) )
			{
				CBaseEntity *pInflictor = info.GetInflictor();
		
				if ( pInflictor && pInflictor->IsPlayer() == false )
				{
					CTFGrenadePipebombProjectile *pBaseGrenade = dynamic_cast< CTFGrenadePipebombProjectile* >( pInflictor );

					if ( pBaseGrenade && pBaseGrenade->m_bTouched != true )
					{
						pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_DEMOMAN_KILL_X_WITH_DIRECTPIPE );
					}
				}
			}
		}
		else if ( pPlayerAttacker->IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			// give achievement for killing someone who was recently damaged by our sentry
			// note that we don't check to see if the sentry is still alive
			if ( pKillerWeapon &&
				( pKillerWeapon->GetWeaponID() == TF_WEAPON_SHOTGUN_PRIMARY ) )
			{
				if ( m_AchievementData.IsSentryDamagerInHistory( pPlayerAttacker, 5.0 ) )
				{
					pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_ENGINEER_SHOTGUN_KILL_PREV_SENTRY_TARGET );
				}
			}
		}
	}

	// Determine the optional assist for the kill.
	m_Shared.SetAssist( NULL ); // TF2007: Not sure what to do here
	
	// Reset our model if we were disguised
	if ( bDisguised )
	{
		UpdateModel();
	}

	RemoveTeleportEffect();

	// Drop a pack with their leftover ammo
	// Arena: Only do this if the match hasn't started yet.
	if ( ShouldDropAmmoPack() )
	{
		DropAmmoPack( info, false, false );
	}

	// This system is designed to coarsely measure a player's skill in public pvp games.
//	UpdateSkillRatingData();

	// If the player has a capture flag and was killed by another player, award that player a defense
	if ( HasItem() && pPlayerAttacker && ( pPlayerAttacker != this ) )
	{
		CCaptureFlag *pCaptureFlag = dynamic_cast<CCaptureFlag *>( GetItem() );
		if ( pCaptureFlag )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
			if ( event )
			{
				event->SetInt( "player", pPlayerAttacker->entindex() );
				event->SetInt( "eventtype", TF_FLAGEVENT_DEFEND );
				event->SetInt( "carrier", entindex() );
				event->SetInt( "priority", 8 );
				event->SetInt( "team", pCaptureFlag->GetTeamNumber() );
				gameeventmanager->FireEvent( event );
			}
			CTF_GameStats.Event_PlayerDefendedPoint( pPlayerAttacker );
		}
	}

	ClearZoomOwner();

	m_vecLastDeathPosition = GetAbsOrigin();

	CTakeDamageInfo info_modified = info;

	// Ragdoll, gib, or death animation.
	bool bRagdoll = true;
	bool bGib = false;

	// See if we should gib.
	if ( ShouldGib( info ) )
	{
		bGib = true;
		bRagdoll = false;
	}
	else
	// See if we should play a custom death animation.
	{
		// If this was a rocket/grenade kill that didn't gib, exaggerated the blast force
		if ( ( info.GetDamageType() & DMG_BLAST ) != 0 )
		{
			Vector vForceModifier = info.GetDamageForce();
			vForceModifier.x *= 2.5;
			vForceModifier.y *= 2.5;
			vForceModifier.z *= 2;
			info_modified.SetDamageForce( vForceModifier );
		}
	}

	if ( bElectrocuted && bGib )
	{
		const char *pEffectName = ( GetTeamNumber() == TF_TEAM_RED ) ? "electrocuted_gibbed_red" : "electrocuted_gibbed_blue";
		DispatchParticleEffect( pEffectName, GetAbsOrigin(), vec3_angle );
		EmitSound( "TFPlayer.MedicChargedDeath" );
	}
	
	SetGibbedOnLastDeath( bGib );

	// show killer in death cam mode
	// chopped down version of SetObserverTarget without the team check
	if( pPlayerAttacker )
	{
		// See if we were killed by a sentrygun. If so, look at that instead of the player
		if ( info.GetInflictor() && info.GetInflictor()->IsBaseObject() )
		{
			// Catches the case where we're killed directly by the sentrygun (i.e. bullets)
			// Look at the sentrygun
			m_hObserverTarget.Set( info.GetInflictor() ); 
		}
		// See if we were killed by a projectile emitted from a base object. The attacker
		// will still be the owner of that object, but we want the deathcam to point to the 
		// object itself.
		else if ( info.GetInflictor() && info.GetInflictor()->GetOwnerEntity() && 
					info.GetInflictor()->GetOwnerEntity()->IsBaseObject() )
		{
			m_hObserverTarget.Set( info.GetInflictor()->GetOwnerEntity() );
		}
		else
		{
			// Look at the player
			m_hObserverTarget.Set( info.GetAttacker() ); 
		}

		// reset fov to default
		SetFOV( this, 0 );
	}
	else if ( info.GetAttacker() && info.GetAttacker()->IsBaseObject() )
	{
		// Catches the case where we're killed by entities spawned by the sentrygun (i.e. rockets)
		// Look at the sentrygun. 
		m_hObserverTarget.Set( info.GetAttacker() ); 
	}
	else
	{
		m_hObserverTarget.Set( NULL );
	}

	bool bSuicide = false;
	if ( info_modified.GetDamageCustom() == TF_DMG_CUSTOM_SUICIDE )
	{
		bSuicide = true;
		// if this was suicide, recalculate attacker to see if we want to award the kill to a recent damager
		info_modified.SetAttacker( TFGameRules()->GetDeathScorer( info.GetAttacker(), info.GetInflictor(), this ) );
	}
	else if ( info.GetAttacker() == this )
	{
		bSuicide = true;
		// If we killed ourselves in non-suicide fashion, and we've been hurt lately, give that guy the kill.
		CTFPlayer *pRecentDamager = TFGameRules()->GetRecentDamager( this, 0, 5.0 );
		if ( pRecentDamager )
		{
			info_modified.SetDamageCustom( TF_DMG_CUSTOM_SUICIDE );
			info_modified.SetDamageType( DMG_GENERIC );
			info_modified.SetAttacker( pRecentDamager );
			info_modified.SetWeapon( NULL );
			info_modified.SetInflictor( NULL );
		}
	}
	else if	( info.GetAttacker() == info.GetInflictor() && info.GetAttacker() && info.GetAttacker()->IsBSPModel() )
	{
		bSuicide = true;
		// If we were killed by "the world", then give credit to the next damager in the list
		CTFPlayer *pRecentDamager = TFGameRules()->GetRecentDamager( this, 1, 5.0 );
		if ( pRecentDamager )
		{
			info_modified.SetDamageCustom( TF_DMG_CUSTOM_SUICIDE );
			info_modified.SetDamageType( DMG_GENERIC );
			info_modified.SetAttacker( pRecentDamager );
			info_modified.SetWeapon( NULL );
			info_modified.SetInflictor( NULL );
		}
	}

	CTFPlayerResource *pResource = dynamic_cast<CTFPlayerResource *>( g_pPlayerResource );
	if ( pResource )
	{
		pResource->SetPlayerClassWhenKilled( entindex(), GetPlayerClass()->GetClassIndex() );
	}

	BaseClass::Event_Killed( info_modified );

	if ( !m_bSwitchedClass )
	{
		SaveLastWeaponSlot();
	}

	// Remove all items...
	RemoveAllItems( true );

	for ( int iWeapon = 0; iWeapon < TF_PLAYER_WEAPON_COUNT; ++iWeapon )
	{
		CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetWeapon( iWeapon );

		if ( pWeapon )
		{
			pWeapon->WeaponReset();
		}
	}

	if ( GetActiveWeapon() )
	{
		m_iActiveWeaponTypePriorToDeath = GetActiveTFWeapon()->GetWeaponID();
		if ( m_iActiveWeaponTypePriorToDeath == TF_WEAPON_BUILDER )
			m_iActiveWeaponTypePriorToDeath = 0;
		GetActiveWeapon()->SendViewModelAnim( ACT_IDLE );
		GetActiveWeapon()->Holster();
		SetActiveWeapon( NULL );
	}
	else
	{
		m_iActiveWeaponTypePriorToDeath = 0;
	}

	CTFPlayer *pInflictor = ToTFPlayer( info.GetInflictor() );
	if ( ( IsHeadshot( info.GetDamageCustom() ) ) && pPlayerAttacker )
	{
		bool bBowShot = false;
		CTF_GameStats.Event_Headshot( pPlayerAttacker, bBowShot );
	}
	else if ( ( TF_DMG_CUSTOM_BACKSTAB == info.GetDamageCustom() ) && pInflictor )
	{
		CTF_GameStats.Event_Backstab( pInflictor );
	}

	// Create the ragdoll entity.
	if ( bGib || bRagdoll )
	{
// 		if ( !bBurning && pKillerWeapon && ( pKillerWeapon->GetWeaponID() == TF_WEAPON_SLAP ) )
// 		{
// 			CTFSlap *pSlap = dynamic_cast< CTFSlap* >( pKillerWeapon );
// 			if ( pSlap )
// 			{
// 				bBurning = ( pSlap->GetNumKills() > 1 ); // first kill doesn't burn
// 			}
// 		}

		CreateRagdollEntity( bGib, bBurning, bOnGround, info.GetDamageCustom() );
	}


	// Remove all conditions...
	m_Shared.RemoveAllCond();

	// Don't overflow the value for this.
	m_iHealth = 0;

	// If we died in sudden death and we're an engineer, explode our buildings
	if ( IsPlayerClass( TF_CLASS_ENGINEER ) && TFGameRules()->InStalemate() && TFGameRules()->IsInArenaMode() == false )
	{
		for (int i = GetObjectCount()-1; i >= 0; i--)
		{
			CBaseObject *obj = GetObject(i);
			Assert( obj );

			if ( obj )
			{
				obj->DetonateObject();
			}		
		}
	}

	// Achievement checks
	if ( pPlayerAttacker )
	{
		// ACHIEVEMENT_TF_MEDIC_KILL_HEALED_SPY - medic kills a spy he has been healing
		if ( IsPlayerClass( TF_CLASS_SPY ) && pPlayerAttacker->IsPlayerClass( TF_CLASS_MEDIC ) )
		{
			// if we were killed by a medic, see if he healed us most recently

			for ( int i=0;i<pPlayerAttacker->WeaponCount();i++ )
			{
				CTFWeaponBase *pWpn = ( CTFWeaponBase *)pPlayerAttacker->GetWeapon( i );

				if ( pWpn == NULL )
					continue;

				if ( pWpn->GetWeaponID() == TF_WEAPON_MEDIGUN )
				{
					CWeaponMedigun *pMedigun = dynamic_cast< CWeaponMedigun * >( pWpn );
					if ( pMedigun )
					{
						if ( pMedigun->GetMostRecentHealTarget() == this )
						{
							pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_MEDIC_KILL_HEALED_SPY );
						}
					}
				}
			}
		}

		if ( bBurning && pPlayerAttacker->IsPlayerClass( TF_CLASS_PYRO ) )
		{
			// ACHIEVEMENT_TF_PYRO_KILL_MULTIWEAPONS - Pyro kills previously ignited target with other weapon
			CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>(info.GetWeapon());

			if ( ( pOriginalBurner == pPlayerAttacker || pLastBurner == pPlayerAttacker ) && pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_SHOTGUN_PYRO )
			{
				pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_PYRO_KILL_MULTIWEAPONS );
			}

			// ACHIEVEMENT_TF_PYRO_KILL_TEAMWORK - Pyro kills an enemy previously ignited by another Pyro
			if ( pOriginalBurner != pPlayerAttacker )
			{
				pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_PYRO_KILL_TEAMWORK );
			}
		}
	}

	// Is the player inside a respawn time override volume?
	FOR_EACH_VEC( ITriggerPlayerRespawnOverride::AutoList(), i )
	{
		CTriggerPlayerRespawnOverride *pTriggerRespawn = static_cast< CTriggerPlayerRespawnOverride* >( ITriggerPlayerRespawnOverride::AutoList()[i] );
		if ( !pTriggerRespawn->m_bDisabled && pTriggerRespawn->IsTouching( this ) )
		{
			SetRespawnOverride( pTriggerRespawn->GetRespawnTime(), pTriggerRespawn->GetRespawnName() );
			break;
		}
		else
		{
			SetRespawnOverride( -1.f, NULL_STRING );
		}
	}

	// Is this an environmental death?
	if ( ( info.GetAttacker() == info.GetInflictor() && info.GetAttacker() && info.GetAttacker()->IsBSPModel() ) ||
		 ( info.GetDamageType() & DMG_VEHICLE ) )
	{
		CTFPlayer *pRecentDamager = TFGameRules()->GetRecentDamager( this, 1, 5.0 );
		if ( pRecentDamager )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "environmental_death" );
			if ( event )
			{
				event->SetInt( "killer", pRecentDamager->entindex() );
				event->SetInt( "victim", entindex() );
				event->SetInt( "priority", 9 );

				gameeventmanager->FireEvent( event );
			}
		}
	}
}

struct SkillRatingAttackRecord_t
{
	CHandle< CTFPlayer > hAttacker;
	float flDamagePercent;
};
	
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pWeapon - 
//			&vecOrigin - 
//			&vecAngles - 
//-----------------------------------------------------------------------------
bool CTFPlayer::CalculateAmmoPackPositionAndAngles( CTFWeaponBase *pWeapon, Vector &vecOrigin, QAngle &vecAngles )
{
	// Look up the hand and weapon bones.
	int iHandBone = LookupBone( "weapon_bone" );
	if ( iHandBone == -1 )
		return false;

	GetBonePosition( iHandBone, vecOrigin, vecAngles );

	// need to fix up the z because the weapon bone position can be under the player
	if ( IsTaunting() )
	{
		// put the pack at the middle of the dying player
		vecOrigin = WorldSpaceCenter();
	}

	// Draw the position and angles.
	Vector vecDebugForward2, vecDebugRight2, vecDebugUp2;
	AngleVectors( vecAngles, &vecDebugForward2, &vecDebugRight2, &vecDebugUp2 );

	/*
	NDebugOverlay::Line( vecOrigin, ( vecOrigin + vecDebugForward2 * 25.0f ), 255, 0, 0, false, 30.0f );
	NDebugOverlay::Line( vecOrigin, ( vecOrigin + vecDebugRight2 * 25.0f ), 0, 255, 0, false, 30.0f );
	NDebugOverlay::Line( vecOrigin, ( vecOrigin + vecDebugUp2 * 25.0f ), 0, 0, 255, false, 30.0f ); 
	*/

	VectorAngles( vecDebugUp2, vecAngles );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// NOTE: If we don't let players drop ammo boxes, we don't need this code..
//-----------------------------------------------------------------------------
void CTFPlayer::AmmoPackCleanUp( void )
{
	// If we have more than 3 ammo packs out now, destroy the oldest one.
	int iNumPacks = 0;
	CTFAmmoPack *pOldestBox = NULL;

	// Cycle through all ammobox in the world and remove them
	CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "tf_ammo_pack" );
	while ( pEnt )
	{
		CBaseEntity *pOwner = pEnt->GetOwnerEntity();
		if (pOwner == this)
		{
			CTFAmmoPack *pThisBox = dynamic_cast<CTFAmmoPack *>( pEnt );
			Assert( pThisBox );
			if ( pThisBox )
			{
				iNumPacks++;

				// Find the oldest one
				if ( pOldestBox == NULL || pOldestBox->GetCreationTime() > pThisBox->GetCreationTime() )
				{
					pOldestBox = pThisBox;
				}
			}
		}

		pEnt = gEntList.FindEntityByClassname( pEnt, "tf_ammo_pack" );
	}

	// If they have more than 3 packs active, remove the oldest one
	if ( iNumPacks > 3 && pOldestBox )
	{
		UTIL_Remove( pOldestBox );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldDropAmmoPack()
{
	if ( TFGameRules()->IsInArenaMode() && TFGameRules()->InStalemate() == false )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::DropAmmoPack( const CTakeDamageInfo &info, bool bEmpty, bool bDisguisedWeapon )
{
	// We want the ammo packs to look like the player's weapon model they were carrying.
	// except if they are melee or building weapons
	CTFWeaponBase *pWeapon = NULL;
	CTFWeaponBase *pActiveWeapon = m_Shared.GetActiveTFWeapon();

	if ( !pActiveWeapon || pActiveWeapon->GetTFWpnData().m_bDontDrop )
	{
		// Don't drop this one, find another one to drop

		int iWeight = -1;

		// find the highest weighted weapon
		for (int i = 0;i < WeaponCount(); i++) 
		{
			CTFWeaponBase *pWpn = ( CTFWeaponBase *)GetWeapon(i);
			if ( !pWpn )
				continue;

			if ( pWpn->GetTFWpnData().m_bDontDrop )
				continue;

			int iThisWeight = pWpn->GetTFWpnData().iWeight;

			if ( iThisWeight > iWeight )
			{
				iWeight = iThisWeight;
				pWeapon = pWpn;
			}
		}
	}
	else
	{
		pWeapon = pActiveWeapon;
	}

	// If we didn't find one, bail
	if ( !pWeapon )
		return;

	// Figure out which model/skin to use for the drop. We may pull from our real weapon or
	// from the weapon we're disguised as.
	CTFWeaponBase *pDropWeaponProps = (bDisguisedWeapon && m_Shared.InCond( TF_COND_DISGUISED ) && m_Shared.GetDisguiseWeapon())
									? m_Shared.GetDisguiseWeapon()
									: pWeapon;

	const char *pszWorldModel = pDropWeaponProps->GetWorldModel();
	int nSkin = pDropWeaponProps->GetSkin();

	if ( pszWorldModel == NULL )
		return;

	// Find the position and angle of the weapons so the "ammo box" matches.
	Vector vecPackOrigin;
	QAngle vecPackAngles;
	if( !CalculateAmmoPackPositionAndAngles( pWeapon, vecPackOrigin, vecPackAngles ) )
		return;

	bool bIsSuicide = info.GetAttacker() ? info.GetAttacker()->GetTeamNumber() == GetTeamNumber() : false;

	// Create the ammo pack.
	CTFAmmoPack *pAmmoPack = CTFAmmoPack::Create( vecPackOrigin, vecPackAngles, this, pszWorldModel );
	Assert( pAmmoPack );
	if ( pAmmoPack )
	{
		pAmmoPack->InitAmmoPack( this, pWeapon, nSkin, bEmpty, bIsSuicide );
	
		// Clean up old ammo packs if they exist in the world
		AmmoPackCleanUp();	
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::DropAmmoPackFromProjectile( CBaseEntity *pProjectile )
{
	QAngle qPackAngles = pProjectile->GetAbsAngles();
	Vector vecPackOrigin = pProjectile->GetAbsOrigin();
	UTIL_Remove( pProjectile );

	// Create the ammo pack.
	CTFAmmoPack *pAmmoPack = CTFAmmoPack::Create( vecPackOrigin, qPackAngles, this, "models/items/ammopack_small.mdl" );
	Assert( pAmmoPack );
	if ( pAmmoPack )
	{
		// half of ammopack_small
		float flAmmoRatio = 0.1f;
		pAmmoPack->InitAmmoPack( this, NULL, 0, false, false, flAmmoRatio );

		// Clean up old ammo packs if they exist in the world
		AmmoPackCleanUp();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::DropHealthPack( const CTakeDamageInfo &info, bool bEmpty )
{
	Vector vecSrc = this->WorldSpaceCenter();
	CHealthKitSmall *pMedKit = assert_cast<CHealthKitSmall*>( CBaseEntity::Create( "item_healthkit_small", vecSrc, vec3_angle, this ) );
	if ( pMedKit )
	{
		Vector vecImpulse = RandomVector( -1,1 );
		vecImpulse.z = 1;
		VectorNormalize( vecImpulse );

		Vector vecVelocity = vecImpulse * 250.0;
		pMedKit->DropSingleInstance( vecVelocity, this, 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PlayerDeathThink( void )
{

}

//-----------------------------------------------------------------------------
// Purpose: Remove the tf items from the player then call into the base class
//          removal of items.
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveAllItems( bool removeSuit )
{
	// If the player has a capture flag, drop it.
	if ( HasItem() )
	{
		int nFlagTeamNumber = GetItem()->GetTeamNumber();
		GetItem()->Drop( this, true );

		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
		if ( event )
		{
			event->SetInt( "player", entindex() );
			event->SetInt( "eventtype", TF_FLAGEVENT_DROPPED );
			event->SetInt( "priority", 8 );
			event->SetInt( "team", nFlagTeamNumber );
			gameeventmanager->FireEvent( event );
		}
	}

	if ( m_hOffHandWeapon.Get() )
	{ 
		HolsterOffHandWeapon();

		// hide the weapon model
		// don't normally have to do this, unless we have a holster animation
		CBaseViewModel *vm = GetViewModel( 1 );
		if ( vm )
		{
			vm->SetWeaponModel( NULL, NULL );
		}

		m_hOffHandWeapon = NULL;
	}

	Weapon_SetLast( NULL );
	UpdateClientData();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ClientHearVox( const char *pSentence )
{
	//TFTODO: implement this.
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::UpdateModel( void )
{
	SetModel( GetPlayerClass()->GetModelName() );

	// Immediately reset our collision bounds - our collision bounds will be set to the model's bounds.
	SetCollisionBounds( GetPlayerMins(), GetPlayerMaxs() );

	m_PlayerAnimState->OnNewModel();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iSkin - 
//-----------------------------------------------------------------------------
void CTFPlayer::UpdateSkin( int iTeam )
{
	// The player's skin is team - 2.
	int iSkin = iTeam - 2;

	// Check to see if the skin actually changed.
	if ( iSkin != m_iLastSkin )
	{
		m_nSkin = iSkin;
		m_iLastSkin = iSkin;
	}
}

//=========================================================================
// Displays the state of the items specified by the Goal passed in
void CTFPlayer::DisplayLocalItemStatus( CTFGoal *pGoal )
{
#if 0
	for (int i = 0; i < 4; i++)
	{
		if (pGoal->display_item_status[i] != 0)
		{
			CTFGoalItem *pItem = Finditem(pGoal->display_item_status[i]);
			if (pItem)
				DisplayItemStatus(pGoal, this, pItem);
			else
				ClientPrint( this, HUD_PRINTTALK, "#Item_missing" );
		}
	}
#endif
}

//=========================================================================
// Called when the player disconnects from the server.
void CTFPlayer::TeamFortress_ClientDisconnected( void )
{
	RemoveAllOwnedEntitiesFromWorld( true );
	RemoveNemesisRelationships();

	StopTaunt();

	RemoveAllWeapons();

	RemoveAllItems( true );

	// notify the vote controller
	if ( g_voteControllerGlobal )
		g_voteControllerGlobal->OnPlayerDisconnected( this );

	if ( GetTeamVoteController() )
		GetTeamVoteController()->OnPlayerDisconnected( this );
}

//=========================================================================
// Removes everything this player has (buildings, grenades, etc.) from the world
void CTFPlayer::RemoveAllOwnedEntitiesFromWorld( bool bExplodeBuildings /* = false */ )
{
	RemoveOwnedProjectiles();

	// Destroy any buildables - this should replace TeamFortress_RemoveBuildings
	RemoveAllObjects( bExplodeBuildings );
}

//=========================================================================
// Removes all rockets the player has fired into the world
// (this prevents a team kill cheat where players would fire rockets 
// then change teams to kill their own team)
void CTFPlayer::RemoveOwnedProjectiles( void )
{
	FOR_EACH_VEC( IBaseProjectileAutoList::AutoList(), i )
	{
		CBaseProjectile *pProjectile = static_cast< CBaseProjectile* >( IBaseProjectileAutoList::AutoList()[i] );

		// if the player owns this entity, remove it
		bool bOwner = ( pProjectile->GetOwnerEntity() == this );

		if ( !bOwner )
		{
			if ( pProjectile->GetBaseProjectileType() == TF_BASE_PROJECTILE_GRENADE )
			{

				CTFWeaponBaseGrenadeProj *pGrenade = assert_cast<CTFWeaponBaseGrenadeProj*>( pProjectile );
				if ( pGrenade )
				{
					bOwner = ( pGrenade->GetThrower() == this );
				}
			}
			else if ( pProjectile->GetProjectileType() == TF_PROJECTILE_SENTRY_ROCKET )
			{
				CTFProjectile_SentryRocket *pRocket = assert_cast<CTFProjectile_SentryRocket*>( pProjectile );
				if ( pRocket )
				{
					bOwner = ( pRocket->GetScorer() == this );
				}
			}
		}

		if ( bOwner )
		{
			pProjectile->SetTouch( NULL );
			pProjectile->AddEffects( EF_NODRAW );
			UTIL_Remove( pProjectile );
		}
	}

	FOR_EACH_VEC( ITFFlameEntityAutoList::AutoList(), i )
	{
		CTFFlameEntity *pFlameEnt = static_cast< CTFFlameEntity* >( ITFFlameEntityAutoList::AutoList()[i] );

		if ( pFlameEnt->IsEntityAttacker( this ) )
		{
			pFlameEnt->SetTouch( NULL );
			pFlameEnt->AddEffects( EF_NODRAW );
			UTIL_Remove( pFlameEnt );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::NoteWeaponFired()
{
	Assert( m_pCurrentCommand );
	if ( m_pCurrentCommand )
	{
		m_iLastWeaponFireUsercmd = m_pCurrentCommand->command_number;
	}

	// Remember the tickcount when the weapon was fired and lock viewangles here!
	if ( m_iLockViewanglesTickNumber != gpGlobals->tickcount )
	{
		m_iLockViewanglesTickNumber = gpGlobals->tickcount;
		m_qangLockViewangles = pl.v_angle;
	}
}

//=============================================================================
//
// Player state functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CPlayerStateInfo *CTFPlayer::StateLookupInfo( int nState )
{
	// This table MUST match the 
	static CPlayerStateInfo playerStateInfos[] =
	{
		{ TF_STATE_ACTIVE,				"TF_STATE_ACTIVE",				&CTFPlayer::StateEnterACTIVE,				NULL,	NULL },
		{ TF_STATE_WELCOME,				"TF_STATE_WELCOME",				&CTFPlayer::StateEnterWELCOME,				NULL,	&CTFPlayer::StateThinkWELCOME },
		{ TF_STATE_OBSERVER,			"TF_STATE_OBSERVER",			&CTFPlayer::StateEnterOBSERVER,				NULL,	&CTFPlayer::StateThinkOBSERVER },
		{ TF_STATE_DYING,				"TF_STATE_DYING",				&CTFPlayer::StateEnterDYING,				NULL,	&CTFPlayer::StateThinkDYING },
	};

	for ( int iState = 0; iState < ARRAYSIZE( playerStateInfos ); ++iState )
	{
		if ( playerStateInfos[iState].m_nPlayerState == nState )
			return &playerStateInfos[iState];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::StateEnter( int nState )
{
	m_Shared.m_nPlayerState = nState;
	m_pStateInfo = StateLookupInfo( nState );

	if ( tf_playerstatetransitions.GetInt() == -1 || tf_playerstatetransitions.GetInt() == entindex() )
	{
		if ( m_pStateInfo )
			Msg( "ShowStateTransitions: entering '%s'\n", m_pStateInfo->m_pStateName );
		else
			Msg( "ShowStateTransitions: entering #%d\n", nState );
	}

	// Initialize the new state.
	if ( m_pStateInfo && m_pStateInfo->pfnEnterState )
	{
		(this->*m_pStateInfo->pfnEnterState)();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::StateLeave( void )
{
	if ( m_pStateInfo && m_pStateInfo->pfnLeaveState )
	{
		(this->*m_pStateInfo->pfnLeaveState)();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::StateTransition( int nState )
{
	StateLeave();
	StateEnter( nState );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::StateEnterWELCOME( void )
{
	PickWelcomeObserverPoint();  
	
	StartObserverMode( OBS_MODE_FIXED );

	// Important to set MOVETYPE_NONE or our physics object will fall while we're sitting at one of the intro cameras.
	SetMoveType( MOVETYPE_NONE );
	AddSolidFlags( FSOLID_NOT_SOLID );
	AddEffects( EF_NODRAW | EF_NOSHADOW );		

	PhysObjectSleep();

	if ( g_pServerBenchmark->IsLocalBenchmarkPlayer( this ) )
	{
		m_bSeenRoundInfo = true;

		ChangeTeam( TEAM_SPECTATOR );
	}
	else if ( gpGlobals->eLoadType == MapLoad_Background )
	{
		m_bSeenRoundInfo = true;

		ChangeTeam( TEAM_SPECTATOR );
	}
	else if ( (TFGameRules() && TFGameRules()->IsLoadingBugBaitReport()) )
	{
		m_bSeenRoundInfo = true;
		
		ChangeTeam( TF_TEAM_BLUE );
		SetDesiredPlayerClassIndex( TF_CLASS_SCOUT );
		ForceRespawn();
	}
	else if ( IsInCommentaryMode()  )
	{
		m_bSeenRoundInfo = true;
	}
	else
	{
		if ( !IsX360() )
		{
			char pszWelcome[128];
			Q_snprintf( pszWelcome, sizeof(pszWelcome), "#TF_Welcome" );
			if ( UTIL_GetActiveHolidayString() )
			{
				Q_snprintf( pszWelcome, sizeof(pszWelcome), "#TF_Welcome_%s", UTIL_GetActiveHolidayString() );
			}

			KeyValues *data = new KeyValues( "data" );
			data->SetString( "title", pszWelcome );		// info panel title
			data->SetString( "type", "1" );				// show userdata from stringtable entry
			data->SetString( "msg",	"motd" );			// use this stringtable entry
			data->SetString( "msg_fallback",	"motd_text" );			// use this stringtable entry if the base is HTML, and client has disabled HTML motds
			data->SetBool( "unload", sv_motd_unload_on_dismissal.GetBool() );

			ShowViewPortPanel( PANEL_INFO, true, data );

			data->deleteThis();
		}
		else
		{
			ShowViewPortPanel( PANEL_MAPINFO, true );
		}

		m_bSeenRoundInfo = false;
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::StateThinkWELCOME( void )
{
	if ( !IsFakeClient() )
	{
		if ( IsInCommentaryMode() )
		{
			ChangeTeam( TF_TEAM_BLUE );
			SetDesiredPlayerClassIndex( TF_CLASS_SCOUT );
			ForceRespawn();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::StateEnterACTIVE()
{
	SetMoveType( MOVETYPE_WALK );
	RemoveEffects( EF_NODRAW | EF_NOSHADOW );
	RemoveSolidFlags( FSOLID_NOT_SOLID );
	m_Local.m_iHideHUD = 0;
	PhysObjectWake();

	m_flLastAction = gpGlobals->curtime;
	m_flLastHealthRegenAt = gpGlobals->curtime;
	SetContextThink( &CTFPlayer::RegenThink, gpGlobals->curtime + TF_REGEN_TIME, "RegenThink" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::SetObserverMode(int mode)
{
	if ( !TFGameRules() )
		return false;

	if ( mode < OBS_MODE_NONE || mode >= NUM_OBSERVER_MODES )
		return false;

	if ( TFGameRules()->ShowMatchSummary() )
		return false;

	// Skip over OBS_MODE_POI if we're not in Passtime mode
	if ( mode == OBS_MODE_POI )
	{
		mode = OBS_MODE_ROAMING;
	}

	// Skip over OBS_MODE_ROAMING for dead players
	if( GetTeamNumber() > TEAM_SPECTATOR )
	{
		if ( IsDead() && ( mode > OBS_MODE_FIXED ) && mp_fadetoblack.GetBool() )
		{
			mode = OBS_MODE_CHASE;
		}
		else if ( mode == OBS_MODE_ROAMING )
		{
			mode = OBS_MODE_IN_EYE;
		}
	}

	if ( m_iObserverMode > OBS_MODE_DEATHCAM )
	{
		// remember mode if we were really spectating before
		m_iObserverLastMode = m_iObserverMode;
	}

	m_iObserverMode = mode;

	if ( !m_bArenaIsAFK )
	{
		m_flLastAction = gpGlobals->curtime;
	}

	// this is the old behavior, still supported for community servers
	bool bAllowSpecModeChange = TFGameRules()->IsInTournamentMode() ? false : true;

	if ( !bAllowSpecModeChange )
	{
		if ( ( mode != OBS_MODE_DEATHCAM ) && ( mode != OBS_MODE_FREEZECAM ) && ( GetTeamNumber() > TEAM_SPECTATOR ) )
		{
			if ( IsValidObserverTarget( GetObserverTarget() ) )
			{
				m_iObserverMode.Set( OBS_MODE_IN_EYE );
			}
			else
			{
				m_iObserverMode.Set( OBS_MODE_DEATHCAM );
			}
		}
	}

	switch ( m_iObserverMode )
	{
	case OBS_MODE_NONE:
	case OBS_MODE_FIXED :
	case OBS_MODE_DEATHCAM :
		SetFOV( this, 0 );	// Reset FOV
		SetViewOffset( vec3_origin );
		SetMoveType( MOVETYPE_NONE );
		break;

	case OBS_MODE_CHASE :
	case OBS_MODE_IN_EYE :	
		// udpate FOV and viewmodels
		SetObserverTarget( m_hObserverTarget );	
		SetMoveType( MOVETYPE_OBSERVER );
		break;

	case OBS_MODE_POI : // PASSTIME
		SetObserverTarget( TFGameRules()->GetObjectiveObserverTarget() );	
		SetMoveType( MOVETYPE_OBSERVER );
		break;

	case OBS_MODE_ROAMING :
		SetFOV( this, 0 );	// Reset FOV
		SetObserverTarget( m_hObserverTarget );
		SetViewOffset( vec3_origin );
		SetMoveType( MOVETYPE_OBSERVER );
		break;
		
	case OBS_MODE_FREEZECAM:
		SetFOV( this, 0 );	// Reset FOV
		SetObserverTarget( m_hObserverTarget );
		SetViewOffset( vec3_origin );
		SetMoveType( MOVETYPE_OBSERVER );
		break;
	}

	CheckObserverSettings();

	return true;	
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::StateEnterOBSERVER( void )
{
	// Always start a spectator session in chase mode
	m_iObserverLastMode = OBS_MODE_CHASE;

	if( m_hObserverTarget == NULL )
	{
		// find a new observer target
		CheckObserverSettings();
	}

	if ( !m_bAbortFreezeCam )
	{
		FindInitialObserverTarget();
	}

	// If we haven't yet set a valid observer mode, such as when
	// the player aborts the freezecam and sets a mode "by hand"
	// force the initial mode to last mode
	if ( m_iObserverMode <= OBS_MODE_FREEZECAM )
	{
		m_iObserverMode = m_iObserverLastMode;
	}

	// If we're in fixed mode, but we found an observer target, move to non fixed.
	if ( m_hObserverTarget.Get() != NULL && m_iObserverMode == OBS_MODE_FIXED )
	{
		m_iObserverMode.Set( OBS_MODE_IN_EYE );
	}

	StartObserverMode( m_iObserverMode );

	PhysObjectSleep();

	if ( GetTeamNumber() != TEAM_SPECTATOR )
	{
		HandleFadeToBlack();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::StateThinkOBSERVER()
{
	// Make sure nobody has changed any of our state.
	Assert( m_takedamage == DAMAGE_NO );
	Assert( IsSolidFlagSet( FSOLID_NOT_SOLID ) );

	// Must be dead.
	Assert( m_lifeState == LIFE_DEAD );
	Assert( pl.deadflag );

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::StateEnterDYING( void )
{
	SetMoveType( MOVETYPE_NONE );
	AddSolidFlags( FSOLID_NOT_SOLID );

	m_bPlayedFreezeCamSound = false;
	m_bAbortFreezeCam = false;

	if ( TFGameRules() && TFGameRules()->IsInArenaMode() )
	{
		float flLastActionTime =  gpGlobals->curtime - m_flLastAction;
		float flAliveThisRoundTime = gpGlobals->curtime - TFGameRules()->GetRoundStart();

		if ( flAliveThisRoundTime - flLastActionTime < 0 )
		{
			m_bArenaIsAFK = true;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Move the player to observer mode once the dying process is over
//-----------------------------------------------------------------------------
void CTFPlayer::StateThinkDYING( void )
{
	// If we have a ragdoll, it's time to go to deathcam
	if ( !m_bAbortFreezeCam && m_hRagdoll && 
		(m_lifeState == LIFE_DYING || m_lifeState == LIFE_DEAD) && 
		GetObserverMode() != OBS_MODE_FREEZECAM )
	{
		if ( GetObserverMode() != OBS_MODE_DEATHCAM )
		{
			StartObserverMode( OBS_MODE_DEATHCAM );	// go to observer mode
		}
		RemoveEffects( EF_NODRAW | EF_NOSHADOW );	// still draw player body
	}

	float flTimeInFreeze = spec_freeze_traveltime.GetFloat() + spec_freeze_time.GetFloat();
	float flFreezeEnd = (m_flDeathTime + TF_DEATH_ANIMATION_TIME + flTimeInFreeze );
	if ( !m_bPlayedFreezeCamSound  && GetObserverTarget() && GetObserverTarget() != this )
	{
		// Start the sound so that it ends at the freezecam lock on time
		float flFreezeSoundLength = 0.3;
		float flFreezeSoundTime = (m_flDeathTime + TF_DEATH_ANIMATION_TIME ) + spec_freeze_traveltime.GetFloat() - flFreezeSoundLength;
		if ( gpGlobals->curtime >= flFreezeSoundTime )
		{
			CSingleUserRecipientFilter filter( this );
			EmitSound_t params;
			params.m_flSoundTime = 0;
			params.m_pSoundName = "TFPlayer.FreezeCam";
			EmitSound( filter, entindex(), params );

			m_bPlayedFreezeCamSound = true;
		}
	}

	if ( gpGlobals->curtime >= (m_flDeathTime + TF_DEATH_ANIMATION_TIME ) )	// allow x seconds death animation / death cam
	{
		if ( GetObserverTarget() && GetObserverTarget() != this )
		{
			if ( !m_bAbortFreezeCam && gpGlobals->curtime < flFreezeEnd )
			{
				if ( GetObserverMode() != OBS_MODE_FREEZECAM )
				{
					StartObserverMode( OBS_MODE_FREEZECAM );
					PhysObjectSleep();
				}
				return;
			}
		}

		if ( GetObserverMode() == OBS_MODE_FREEZECAM )
		{
			// If we're in freezecam, and we want out, abort.  (only if server is not using mp_fadetoblack)
			if ( m_bAbortFreezeCam && !mp_fadetoblack.GetBool() )
			{
				if ( m_hObserverTarget == NULL )
				{
					// find a new observer target
					CheckObserverSettings();
				}

				FindInitialObserverTarget();

				SetObserverMode( OBS_MODE_CHASE );
				ShowViewPortPanel( "specgui" , ModeWantsSpectatorGUI(OBS_MODE_CHASE) );
			}
		}

		// Don't allow anyone to respawn until freeze time is over, even if they're not
		// in freezecam. This prevents players skipping freezecam to spawn faster.
		if ( gpGlobals->curtime < flFreezeEnd )
			return;

		m_lifeState = LIFE_RESPAWNABLE;

		StopAnimation();

		IncrementInterpolationFrame();

		if ( GetMoveType() != MOVETYPE_NONE && (GetFlags() & FL_ONGROUND) )
			SetMoveType( MOVETYPE_NONE );

		StateTransition( TF_STATE_OBSERVER );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::AttemptToExitFreezeCam( void )
{
	float flFreezeTravelTime = (m_flDeathTime + TF_DEATH_ANIMATION_TIME ) + spec_freeze_traveltime.GetFloat() + 0.5;
	if ( gpGlobals->curtime < flFreezeTravelTime )
		return;

	m_bAbortFreezeCam = true;
}

class CIntroViewpoint : public CPointEntity
{
	DECLARE_CLASS( CIntroViewpoint, CPointEntity );
public:
	DECLARE_DATADESC();

	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

	int			m_iIntroStep;
	float		m_flStepDelay;
	string_t	m_iszMessage;
	string_t	m_iszGameEvent;
	float		m_flEventDelay;
	int			m_iGameEventData;
	float		m_flFOV;
};

BEGIN_DATADESC( CIntroViewpoint )
	DEFINE_KEYFIELD( m_iIntroStep,	FIELD_INTEGER,	"step_number" ),
	DEFINE_KEYFIELD( m_flStepDelay,	FIELD_FLOAT,	"time_delay" ),
	DEFINE_KEYFIELD( m_iszMessage,	FIELD_STRING,	"hint_message" ),
	DEFINE_KEYFIELD( m_iszGameEvent,	FIELD_STRING,	"event_to_fire" ),
	DEFINE_KEYFIELD( m_flEventDelay,	FIELD_FLOAT,	"event_delay" ),
	DEFINE_KEYFIELD( m_iGameEventData,	FIELD_INTEGER,	"event_data_int" ),
	DEFINE_KEYFIELD( m_flFOV,	FIELD_FLOAT,	"fov" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( game_intro_viewpoint, CIntroViewpoint );

//-----------------------------------------------------------------------------
// Purpose: Give the player some ammo.
// Input  : iCount - Amount of ammo to give.
//			iAmmoIndex - Index of the ammo into the AmmoInfoArray
//			iMax - Max carrying capability of the player
// Output : Amount of ammo actually given
//-----------------------------------------------------------------------------
int CTFPlayer::GiveAmmo( int iCount, int iAmmoIndex, bool bSuppressSound )
{
 	return GiveAmmo( iCount, iAmmoIndex, bSuppressSound, kAmmoSource_Pickup );
}

//-----------------------------------------------------------------------------
// Purpose: Give the player some ammo.
// Input  : iCount - Amount of ammo to give.
//			iAmmoIndex - Index of the ammo into the AmmoInfoArray
//			iMax - Max carrying capability of the player
// Output : Amount of ammo actually given
//-----------------------------------------------------------------------------
int CTFPlayer::GiveAmmo( int iCount, int iAmmoIndex, bool bSuppressSound, EAmmoSource eAmmoSource )
{
	if ( iCount <= 0 )
	{
		return 0;
	}

	if ( !g_pGameRules->CanHaveAmmo( this, iAmmoIndex ) )
	{
		// game rules say I can't have any more of this ammo type.
		return 0;
	}

	if ( iAmmoIndex < 0 || iAmmoIndex >= MAX_AMMO_SLOTS )
	{
		return 0;
	}

	int iAdd = MIN( iCount, GetMaxAmmo(iAmmoIndex) - GetAmmoCount(iAmmoIndex) );
	if ( iAdd < 1 )
	{
		return 0;
	}

	// Ammo pickup sound
	if ( !bSuppressSound )
	{
		EmitSound( "BaseCombatCharacter.AmmoPickup" );
	}

	CBaseCombatCharacter::GiveAmmo( iAdd, iAmmoIndex, bSuppressSound );
	return iAdd;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveAmmo( int iCount, int iAmmoIndex )
{

#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetInt() > 1 && !IsBot() )
		return;
#endif // _DEBUG || STAGING_ONLY

	CBaseCombatCharacter::RemoveAmmo( iCount, iAmmoIndex );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveAmmo( int iCount, const char *szName )
{
	CBaseCombatCharacter::RemoveAmmo( iCount, szName );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the amount of ammunition of a particular type owned
//			owned by the character
// Input  :	Ammo Index
// Output :	The amount of ammo
//-----------------------------------------------------------------------------
int CTFPlayer::GetAmmoCount( int iAmmoIndex ) const
{
	if ( iAmmoIndex == -1 )
		return 0;

	if ( IsFakeClient() && TFGameRules()->IsInItemTestingMode() )
		return 999;

	return BaseClass::GetAmmoCount( iAmmoIndex );
}

//-----------------------------------------------------------------------------
// Purpose: Has to be const for override, but needs to access non-const member methods.
//-----------------------------------------------------------------------------
int	CTFPlayer::GetMaxHealth() const
{
	return Max( const_cast<CTFPlayer*>( this )->GetMaxHealthForBuffing(), 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayer::GetMaxHealthForBuffing()
{
	return m_PlayerClass.GetMaxHealth();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ForceRegenerateAndRespawn( void )
{
	m_bRegenerating.Set( true );
	ForceRespawn();
	m_bRegenerating.Set( false );
}

//-----------------------------------------------------------------------------
// Purpose: Reset player's information and force him to spawn
//-----------------------------------------------------------------------------
void CTFPlayer::ForceRespawn( void )
{
	VPROF_BUDGET( "CTFPlayer::ForceRespawn", VPROF_BUDGETGROUP_PLAYER );

	CTF_GameStats.Event_PlayerForceRespawn( this );

	m_flSpawnTime = gpGlobals->curtime;

	bool bRandom = false;

	int iDesiredClass = GetDesiredPlayerClassIndex();

	if ( iDesiredClass == TF_CLASS_UNDEFINED )
	{
		return;
	}

	if ( iDesiredClass == TF_CLASS_RANDOM )
	{
		bRandom = true;

		// Don't let them be the same class twice in a row
		do{
			iDesiredClass = random->RandomInt( TF_FIRST_NORMAL_CLASS, TF_LAST_NORMAL_CLASS );
		} while( iDesiredClass == GetPlayerClass()->GetClassIndex() );
	}

	if ( HasTheFlag() )
	{
		DropFlag();
	}

	if ( GetPlayerClass()->GetClassIndex() != iDesiredClass )
	{
		// clean up any pipebombs/buildings in the world (no explosions)
		m_bSwitchedClass = true;

		RemoveAllOwnedEntitiesFromWorld();

		int iOldClass = GetPlayerClass()->GetClassIndex();

		GetPlayerClass()->Init( iDesiredClass );

		// Don't report class changes if we're random, because it's not a player choice
		if ( !bRandom )
		{
			m_iClassChanges++;
			CTF_GameStats.Event_PlayerChangedClass( this, iOldClass, iDesiredClass );
		}
	}
	else
	{
		m_bSwitchedClass = false;
	}

	m_Shared.RemoveAllCond();

	if ( m_bSwitchedClass )
	{
		m_iLastWeaponSlot = 1;
	}
	else
	{
		if ( IsAlive() )
		{
			if ( GetActiveTFWeapon() )
			{
				m_iActiveWeaponTypePriorToDeath = GetActiveTFWeapon()->GetWeaponID();
			}
			SaveLastWeaponSlot();
		}
	}

	RemoveAllItems( true );

	// Reset ground state for airwalk animations
	SetGroundEntity( NULL );

	// TODO: move this into conditions
	RemoveTeleportEffect();

	// remove invisibility very quickly	
	m_Shared.FadeInvis( 0.1f );

	// Stop any firing that was taking place before respawn.
	m_nButtons = 0;

	StateTransition( TF_STATE_ACTIVE );
	Spawn();

	m_bSwitchedClass = false;
}

//-----------------------------------------------------------------------------
// Purpose: Do nothing multiplayer_animstate takes care of animation.
// Input  : playerAnim - 
//-----------------------------------------------------------------------------
void CTFPlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
	return;
}

//-----------------------------------------------------------------------------
// Purpose: Handle cheat commands
// Input  : iImpulse - 
//-----------------------------------------------------------------------------
void CTFPlayer::CheatImpulseCommands( int iImpulse )
{
	switch( iImpulse )
	{
	case 101:
		{
			if( sv_cheats->GetBool() )
			{
				extern int gEvilImpulse101;
				gEvilImpulse101 = true;

				GiveAmmo( 1000, TF_AMMO_PRIMARY );
				GiveAmmo( 1000, TF_AMMO_SECONDARY );
				GiveAmmo( 1000, TF_AMMO_METAL );
				GiveAmmo( 1000, TF_AMMO_GRENADES1 );
				GiveAmmo( 1000, TF_AMMO_GRENADES2 );
				TakeHealth( 999, DMG_GENERIC );

				// Refills weapon clips, too
				for ( int i = 0; i < MAX_WEAPONS; i++ )
				{
					CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase* >( GetWeapon( i ) );
					if ( !pWeapon )
						continue;

					pWeapon->GiveDefaultAmmo();
				}

				gEvilImpulse101 = false;
			}
		}
		break;

	default:
		{
			BaseClass::CheatImpulseCommands( iImpulse );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetWeaponBuilder( CTFWeaponBuilder *pBuilder )
{
	m_hWeaponBuilder = pBuilder;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWeaponBuilder *CTFPlayer::GetWeaponBuilder( void )
{
	Assert( 0 );
	return m_hWeaponBuilder;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if this player is building something
//-----------------------------------------------------------------------------
bool CTFPlayer::IsBuilding( void )
{
	/*
	CTFWeaponBuilder *pBuilder = GetWeaponBuilder();
	if ( pBuilder )
		return pBuilder->IsBuilding();
		*/

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveBuildResources( int iAmount )
{
	RemoveAmmo( iAmount, TF_AMMO_METAL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::AddBuildResources( int iAmount )
{
	GiveAmmo( iAmount, TF_AMMO_METAL, false, kAmmoSource_Pickup );	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseObject	*CTFPlayer::GetObject( int index ) const
{
	return (CBaseObject *)( m_aObjects[index].Get() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseObject	*CTFPlayer::GetObjectOfType( int iObjectType, int iObjectMode ) const
{
	int iNumObjects = GetObjectCount();
	for ( int i=0; i<iNumObjects; i++ )
	{
		CBaseObject *pObj = GetObject(i);

		if ( !pObj )
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
//-----------------------------------------------------------------------------
int	CTFPlayer::GetObjectCount( void ) const
{
	return m_aObjects.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Remove all the player's objects
//			If bExplodeBuildings is not set, remove all of them immediately.
//			Otherwise, make them all explode.
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveAllObjects( bool bExplodeBuildings /* = false */ )
{
	// Remove all the player's objects
	for (int i = GetObjectCount()-1; i >= 0; i--)
	{
		CBaseObject *obj = GetObject(i);
		Assert( obj );

		if ( obj )
		{
			// this is separate from the object_destroyed event, which does
			// not get sent when we remove the objects from the world
			IGameEvent *event = gameeventmanager->CreateEvent( "object_removed" );	
			if ( event )
			{
				event->SetInt( "userid", GetUserID() ); // user ID of the object owner
				event->SetInt( "objecttype", obj->GetType() ); // type of object removed
				event->SetInt( "index", obj->entindex() ); // index of the object removed
				gameeventmanager->FireEvent( event );
			}

			if ( bExplodeBuildings )
			{
				obj->DetonateObject();
			}
			else
			{
				// This fixes a bug in Raid mode where we could spawn where our sentry was but 
				// we didn't get the weapons because they couldn't trace to us in FVisible
				obj->SetSolid( SOLID_NONE );
				UTIL_Remove( obj );
			}
		}		
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::StopPlacement( void )
{
	/*
	// Tell our builder weapon
	CTFWeaponBuilder *pBuilder = GetWeaponBuilder();
	if ( pBuilder )
	{
		pBuilder->StopPlacement();
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: Player has started building an object
//-----------------------------------------------------------------------------
int	CTFPlayer::StartedBuildingObject( int iObjectType )
{
	// Deduct the cost of the object
	int iCost = m_Shared.CalculateObjectCost( this, iObjectType );
	if ( iCost > GetBuildResources() )
	{
		// Player must have lost resources since he started placing
		return 0;
	}

	RemoveBuildResources( iCost );

	// If the object costs 0, we need to return non-0 to mean success
	if ( !iCost )
		return 1;

	return iCost;
}

//-----------------------------------------------------------------------------
// Purpose: Player has aborted building something
//-----------------------------------------------------------------------------
void CTFPlayer::StoppedBuilding( int iObjectType )
{
	/*
	int iCost = CalculateObjectCost( iObjectType );

	AddBuildResources( iCost );

	// Tell our builder weapon
	CTFWeaponBuilder *pBuilder = GetWeaponBuilder();
	if ( pBuilder )
	{
		pBuilder->StoppedBuilding( iObjectType );
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: Object has been built by this player
//-----------------------------------------------------------------------------
void CTFPlayer::FinishedObject( CBaseObject *pObject )
{
	AddObject( pObject );

	CTF_GameStats.Event_PlayerCreatedBuilding( this, pObject );

	/*
	// Tell our builder weapon
	CTFWeaponBuilder *pBuilder = GetWeaponBuilder();
	if ( pBuilder )
	{
		pBuilder->FinishedObject();
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: Add the specified object to this player's object list.
//-----------------------------------------------------------------------------
void CTFPlayer::AddObject( CBaseObject *pObject )
{
	TRACE_OBJECT( UTIL_VarArgs( "%0.2f CBaseTFPlayer::AddObject adding object %p:%s to player %s\n", gpGlobals->curtime, pObject, pObject->GetClassname(), GetPlayerName() ) );

	bool alreadyInList = PlayerOwnsObject( pObject );
	// Assert( !alreadyInList );
	if ( !alreadyInList )
	{
		m_aObjects.AddToTail( pObject );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Object built by this player has been destroyed
//-----------------------------------------------------------------------------
void CTFPlayer::OwnedObjectDestroyed( CBaseObject *pObject )
{
	TRACE_OBJECT( UTIL_VarArgs( "%0.2f CBaseTFPlayer::OwnedObjectDestroyed player %s object %p:%s\n", gpGlobals->curtime, 
		GetPlayerName(),
		pObject,
		pObject->GetClassname() ) );

	RemoveObject( pObject );

	// Tell our builder weapon so it recalculates the state of the build icons
	/*
	CTFWeaponBuilder *pBuilder = GetWeaponBuilder();
	if ( pBuilder )
	{
		pBuilder->RecalcState();
	}
	*/
}

//-----------------------------------------------------------------------------
// Removes an object from the player
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveObject( CBaseObject *pObject )
{
	TRACE_OBJECT( UTIL_VarArgs( "%0.2f CBaseTFPlayer::RemoveObject %p:%s from player %s\n", gpGlobals->curtime, 
		pObject,
		pObject->GetClassname(),
		GetPlayerName() ) );

	Assert( pObject );

	int i;
	for ( i = m_aObjects.Count(); --i >= 0; )
	{
		// Also, while we're at it, remove all other bogus ones too...
		if ( (!m_aObjects[i].Get()) || (m_aObjects[i] == pObject))
		{
			m_aObjects.FastRemove(i);
		}
	}
}

//-----------------------------------------------------------------------------
// See if the player owns this object
//-----------------------------------------------------------------------------
bool CTFPlayer::PlayerOwnsObject( CBaseObject *pObject )
{
	return ( m_aObjects.Find( pObject ) != -1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PlayFlinch( const CTakeDamageInfo &info )
{
	// Don't play flinches if we just died. 
	if ( !IsAlive() )
		return;

	// No pain flinches while disguised, our man has supreme discipline
	if ( m_Shared.InCond( TF_COND_DISGUISED ) )
		return;

	PlayerAnimEvent_t flinchEvent;

	switch ( LastHitGroup() )
	{
		// pick a region-specific flinch
	case HITGROUP_HEAD:
		flinchEvent = PLAYERANIMEVENT_FLINCH_HEAD;
		break;
	case HITGROUP_LEFTARM:
		flinchEvent = PLAYERANIMEVENT_FLINCH_LEFTARM;
		break;
	case HITGROUP_RIGHTARM:
		flinchEvent = PLAYERANIMEVENT_FLINCH_RIGHTARM;
		break;
	case HITGROUP_LEFTLEG:
		flinchEvent = PLAYERANIMEVENT_FLINCH_LEFTLEG;
		break;
	case HITGROUP_RIGHTLEG:
		flinchEvent = PLAYERANIMEVENT_FLINCH_RIGHTLEG;
		break;
	case HITGROUP_STOMACH:
	case HITGROUP_CHEST:
	case HITGROUP_GEAR:
	case HITGROUP_GENERIC:
	default:
		// just get a generic flinch.
		flinchEvent = PLAYERANIMEVENT_FLINCH_CHEST;
		break;
	}

	DoAnimationEvent( flinchEvent );
}

//-----------------------------------------------------------------------------
// Purpose: Plays the crit sound that players that get crit hear
//-----------------------------------------------------------------------------
float CTFPlayer::PlayCritReceivedSound( void )
{
	float flCritPainLength = 0;
	// Play a custom pain sound to the guy taking the damage
	CSingleUserRecipientFilter receiverfilter( this );
	EmitSound_t params;
	params.m_flSoundTime = 0;
	params.m_pSoundName = "TFPlayer.CritPain";
	params.m_pflSoundDuration = &flCritPainLength;
	EmitSound( receiverfilter, entindex(), params );

	return flCritPainLength;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PainSound( const CTakeDamageInfo &info )
{
	// Don't make sounds if we just died. DeathSound will handle that.
	if ( !IsAlive() )
		return;

	// no pain sounds while disguised, our man has supreme discipline
	if ( m_Shared.InCond( TF_COND_DISGUISED ) )
		return;

	if ( m_flNextPainSoundTime > gpGlobals->curtime )
		return;
	
	if ( info.GetDamageType() & DMG_FALL )
	{
		return;
	}

	// No sound for DMG_GENERIC
	if ( info.GetDamageType() == 0 || info.GetDamageType() == DMG_PREVENT_PHYSICS_FORCE )
		return;

	if ( info.GetDamageType() & DMG_DROWN )
	{
		EmitSound( "TFPlayer.Drown" );
		return;
	}

	if ( info.GetDamageType() & DMG_BURN )
	{
		// Looping fire pain sound is done in CTFPlayerShared::ConditionThink
		return;
	}

	float flPainLength = 0;

	bool bAttackerIsPlayer = ( info.GetAttacker() && info.GetAttacker()->IsPlayer() );

	CMultiplayer_Expresser *pExpresser = GetMultiplayerExpresser();
	Assert( pExpresser );

	pExpresser->AllowMultipleScenes();

	// speak a pain concept here, send to everyone but the attacker
	CPASFilter filter( GetAbsOrigin() );

	if ( bAttackerIsPlayer )
	{
		filter.RemoveRecipient( ToBasePlayer( info.GetAttacker() ) );
	}

	// play a crit sound to the victim ( us )
	if ( info.GetDamageType() & DMG_CRITICAL )
	{
		flPainLength = PlayCritReceivedSound();

		// remove us from hearing our own pain sound if we hear the crit sound
		filter.RemoveRecipient( this );
	}

	char szResponse[AI_Response::MAX_RESPONSE_NAME];

	if ( SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_PAIN, "damagecritical:1", szResponse, AI_Response::MAX_RESPONSE_NAME, &filter ) )
	{
		flPainLength = MAX( GetSceneDuration( szResponse ), flPainLength );
	}

	// speak a louder pain concept to just the attacker
	if ( bAttackerIsPlayer )
	{
		CSingleUserRecipientFilter attackerFilter( ToBasePlayer( info.GetAttacker() ) );
		SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_ATTACKER_PAIN, "damagecritical:1", szResponse, AI_Response::MAX_RESPONSE_NAME, &attackerFilter );
	}

	pExpresser->DisallowMultipleScenes();

	m_flNextPainSoundTime = gpGlobals->curtime + flPainLength;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::DeathSound( const CTakeDamageInfo &info )
{
	// Don't make death sounds when choosing a class
	if ( IsPlayerClass( TF_CLASS_UNDEFINED ) )
		return;

	TFPlayerClassData_t *pData = GetPlayerClass()->GetData();
	if ( !pData )
		return;

	if ( m_bGoingFeignDeath  )
	{
		bool bDisguised = m_Shared.InCond( TF_COND_DISGUISED ) && (m_Shared.GetDisguiseTeam() == GetTeamNumber());
		if ( bDisguised )
		{
			// Use our disguise class, if we have one and will drop a disguise class corpse.
			pData = g_pTFPlayerClassDataMgr->Get( m_Shared.GetDisguiseClass() );
			if ( !pData )
				return;
		}
	}

	int nDeathSoundOffset = DEATH_SOUND_FIRST;

	if ( m_LastDamageType & DMG_FALL ) // Did we die from falling?
	{
		// They died in the fall. Play a splat sound.
		EmitSound( "Player.FallGib" );
	}
	else if ( m_LastDamageType & DMG_BLAST )
	{
		EmitSound( pData->GetDeathSound( DEATH_SOUND_EXPLOSION + nDeathSoundOffset ) );
	}
	else if ( m_LastDamageType & DMG_CRITICAL )
	{
		EmitSound( pData->GetDeathSound( DEATH_SOUND_CRIT + nDeathSoundOffset ) );

		PlayCritReceivedSound();
	}
	else if ( m_LastDamageType & DMG_CLUB )
	{
		EmitSound( pData->GetDeathSound( DEATH_SOUND_MELEE + nDeathSoundOffset ) );
	}
	else
	{
		EmitSound( pData->GetDeathSound( DEATH_SOUND_GENERIC + nDeathSoundOffset ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char* CTFPlayer::GetSceneSoundToken( void )
{
	return "";
}

//-----------------------------------------------------------------------------
// Purpose: called when this player burns another player
//-----------------------------------------------------------------------------
void CTFPlayer::OnBurnOther( CTFPlayer *pTFPlayerVictim, CTFWeaponBase *pWeapon )
{
#define ACHIEVEMENT_BURN_TIME_WINDOW	30.0f
#define ACHIEVEMENT_BURN_VICTIMS	5
	// add current time we burned another player to head of vector
	m_aBurnOtherTimes.AddToHead( gpGlobals->curtime );

	// remove any burn times that are older than the burn window from the list
	float flTimeDiscard = gpGlobals->curtime - ACHIEVEMENT_BURN_TIME_WINDOW;
	for ( int i = 1; i < m_aBurnOtherTimes.Count(); i++ )
	{
		if ( m_aBurnOtherTimes[i] < flTimeDiscard )
		{
			m_aBurnOtherTimes.RemoveMultiple( i, m_aBurnOtherTimes.Count() - i );
			break;
		}
	}

	// see if we've burned enough players in time window to satisfy achievement
	if ( m_aBurnOtherTimes.Count() >= ACHIEVEMENT_BURN_VICTIMS )
	{
		AwardAchievement( ACHIEVEMENT_TF_BURN_PLAYERSINMINIMIMTIME );
	}

	// ACHIEVEMENT_TF_PYRO_KILL_SPIES - Awarded for igniting enemy spies who have active sappers on friendly building
	if ( pTFPlayerVictim->IsPlayerClass(TF_CLASS_SPY))
	{
		CBaseObject	*pSapper = pTFPlayerVictim->GetObjectOfType( OBJ_ATTACHMENT_SAPPER, 0 );
		if ( pSapper )
		{
			AwardAchievement( ACHIEVEMENT_TF_PYRO_KILL_SPIES );
		}
	}

	// ACHIEVEMENT_TF_PYRO_DEFEND_POINTS - Pyro kills targets capping control points
	CTriggerAreaCapture *pAreaTrigger = pTFPlayerVictim->GetControlPointStandingOn();
	if ( pAreaTrigger )
	{
		CTeamControlPoint *pCP = pAreaTrigger->GetControlPoint();
		if ( pCP && pCP->GetOwner() == GetTeamNumber() )
		{
			if ( TeamplayGameRules()->TeamMayCapturePoint( pTFPlayerVictim->GetTeamNumber(), pCP->GetPointIndex() ) && 
				TeamplayGameRules()->PlayerMayCapturePoint( pTFPlayerVictim, pCP->GetPointIndex() ) )
			{
				AwardAchievement( ACHIEVEMENT_TF_PYRO_DEFEND_POINTS );
			}
		}
	}

	// ACHIEVEMENT_TF_MEDIC_ASSIST_PYRO
	// if we're invuln, let the medic know that we burned someone
	if ( m_Shared.InCond( TF_COND_INVULNERABLE ) || m_Shared.InCond( TF_COND_INVULNERABLE_WEARINGOFF ) )
	{
		int i;
		int iNumHealers = m_Shared.GetNumHealers();

		for ( i=0;i<iNumHealers;i++ )
		{
			// Send a message to all medics invulning the Pyro at this time
			CTFPlayer *pMedic = ToTFPlayer( m_Shared.GetHealerByIndex( i ) );

			// TF2007: There was a check here for seeing if the medigun was the invuln type
			// Tell the clients involved in the ignition
			CSingleUserRecipientFilter medic_filter( pMedic );
			UserMessageBegin( medic_filter, "PlayerIgnitedInv" );
				WRITE_BYTE( entindex() );
				WRITE_BYTE( pTFPlayerVictim->entindex() );
				WRITE_BYTE( pMedic->entindex() );
			MessageEnd();
		}
	}

	// Tell the clients involved in the ignition
	CRecipientFilter involved_filter;
	involved_filter.AddRecipient( this );
	involved_filter.AddRecipient( pTFPlayerVictim );
	UserMessageBegin( involved_filter, "PlayerIgnited" );
		WRITE_BYTE( entindex() );
		WRITE_BYTE( pTFPlayerVictim->entindex() );
		WRITE_BYTE( pWeapon ? pWeapon->GetWeaponID() : 0 );
	MessageEnd();

	IGameEvent *event = gameeventmanager->CreateEvent( "player_ignited" );
	if ( event )
	{
		event->SetInt( "pyro_entindex", entindex() );
		event->SetInt( "victim_entindex", pTFPlayerVictim->entindex() );
		event->SetInt( "weaponid", pWeapon ? pWeapon->GetWeaponID() : 0 );
		gameeventmanager->FireEvent( event, true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the player is capturing a point.
//-----------------------------------------------------------------------------
bool CTFPlayer::IsCapturingPoint()
{
	CTriggerAreaCapture *pAreaTrigger = GetControlPointStandingOn();
	if ( pAreaTrigger )
	{
		CTeamControlPoint *pCP = pAreaTrigger->GetControlPoint();
		if ( pCP )
		{
			if ( TeamplayGameRules()->TeamMayCapturePoint( GetTeamNumber(), pCP->GetPointIndex() ) && 
				TeamplayGameRules()->PlayerMayCapturePoint( this, pCP->GetPointIndex() ) )
			{
				// if we own this point, we're no longer "capturing" it
				return pCP->GetOwner() != GetTeamNumber();
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFTeam *CTFPlayer::GetTFTeam( void )
{
	CTFTeam *pTeam = dynamic_cast<CTFTeam *>( GetTeam() );
	Assert( pTeam );
	return pTeam;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFTeam *CTFPlayer::GetOpposingTFTeam( void )
{
	if ( TFTeamMgr() )
	{
		int iTeam = GetTeamNumber();
		if ( iTeam == TF_TEAM_RED )
		{
			return TFTeamMgr()->GetTeam( TF_TEAM_BLUE );
		}
		else if ( iTeam == TF_TEAM_BLUE )
		{
			return TFTeamMgr()->GetTeam( TF_TEAM_RED );
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Give this player the "i just teleported" effect for 12 seconds
//-----------------------------------------------------------------------------
void CTFPlayer::TeleportEffect( void )
{
	m_Shared.AddCond( TF_COND_TELEPORTED );

	float flDuration = 12.f;

	// Also removed on death
	SetContextThink( &CTFPlayer::RemoveTeleportEffect, gpGlobals->curtime + flDuration, "TFPlayer_TeleportEffect" );
}

//-----------------------------------------------------------------------------
// Purpose: Remove the teleporter effect
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveTeleportEffect( void )
{
	m_Shared.RemoveCond( TF_COND_TELEPORTED );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::StopRagdollDeathAnim( void )
{
	CTFRagdoll *pRagdoll = dynamic_cast<CTFRagdoll*>( m_hRagdoll.Get() );
	if ( pRagdoll )
	{
		pRagdoll->m_iDamageCustom = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::CreateRagdollEntity( void )
{
	CreateRagdollEntity( false, false, false );
}

//-----------------------------------------------------------------------------
// Purpose: Create a ragdoll entity to pass to the client.
//-----------------------------------------------------------------------------
void CTFPlayer::CreateRagdollEntity( bool bGib, bool bBurning, bool bOnGround, int iDamageCustom )
{
	// If we already have a ragdoll destroy it.
	CTFRagdoll *pRagdoll = dynamic_cast<CTFRagdoll*>( m_hRagdoll.Get() );
	if( pRagdoll )
	{
		UTIL_Remove( pRagdoll );
		pRagdoll = NULL;
	}
	Assert( pRagdoll == NULL );

	// Create a ragdoll.
	pRagdoll = dynamic_cast<CTFRagdoll*>( CreateEntityByName( "tf_ragdoll" ) );
	if ( pRagdoll )
	{
		pRagdoll->m_vecRagdollOrigin = GetAbsOrigin();
		pRagdoll->m_vecRagdollVelocity = GetAbsVelocity();
		pRagdoll->m_vecForce = m_vecForce;
		pRagdoll->m_nForceBone = m_nForceBone;
		pRagdoll->m_hPlayer = this;
		pRagdoll->m_bGib = bGib;
		pRagdoll->m_bBurning = bBurning;
		pRagdoll->m_bOnGround = bOnGround;
		pRagdoll->m_iDamageCustom = iDamageCustom;
		pRagdoll->m_iTeam = GetTeamNumber();
		pRagdoll->m_iClass = GetPlayerClass()->GetClassIndex();
	}

	// Turn off the player.
	AddSolidFlags( FSOLID_NOT_SOLID );
	AddEffects( EF_NODRAW | EF_NOSHADOW );
	SetMoveType( MOVETYPE_NONE );

	// Add additional gib setup.
	if ( bGib )
	{
		m_nRenderFX = kRenderFxRagdoll;
	}

	// Save ragdoll handle.
	m_hRagdoll = pRagdoll;
}

//-----------------------------------------------------------------------------
// Purpose: Destroy's a ragdoll, called with a player is disconnecting.
//-----------------------------------------------------------------------------
void CTFPlayer::DestroyRagdoll( void )
{
	CTFRagdoll *pRagdoll = dynamic_cast<CTFRagdoll*>( m_hRagdoll.Get() );	
	if( pRagdoll )
	{
		UTIL_Remove( pRagdoll );
	}

	// Remove the feign death ragdoll at the same time.
	pRagdoll = dynamic_cast<CTFRagdoll*>( m_hFeignRagdoll.Get() );	
	if( pRagdoll )
	{
		UTIL_Remove( pRagdoll );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::Weapon_FrameUpdate( void )
{
	BaseClass::Weapon_FrameUpdate();

	if ( m_hOffHandWeapon.Get() && m_hOffHandWeapon->IsWeaponVisible() )
	{
		m_hOffHandWeapon->Operator_FrameUpdate( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CTFPlayer::Weapon_HandleAnimEvent( animevent_t *pEvent )
{
	BaseClass::Weapon_HandleAnimEvent( pEvent );

	if ( m_hOffHandWeapon.Get() )
	{
		m_hOffHandWeapon->Operator_HandleAnimEvent( pEvent, this );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CTFPlayer::Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget , const Vector *pVelocity ) 
{

}

//-----------------------------------------------------------------------------
// Purpose: Call this when this player fires a weapon to allow other systems to react
//-----------------------------------------------------------------------------
void CTFPlayer::OnMyWeaponFired( CBaseCombatWeapon *weapon )
{
	BaseClass::OnMyWeaponFired( weapon );

	// mark region as 'in combat'
	if ( m_inCombatThrottleTimer.IsElapsed() )
	{
		CTFWeaponBase *tfWeapon = static_cast< CTFWeaponBase * >( weapon );

		if ( !tfWeapon )
		{
			return;
		}

		switch ( tfWeapon->GetWeaponID() )
		{
		case TF_WEAPON_MEDIGUN:
		case TF_WEAPON_PDA:
		case TF_WEAPON_PDA_ENGINEER_BUILD:
		case TF_WEAPON_PDA_ENGINEER_DESTROY:
		case TF_WEAPON_PDA_SPY:
		case TF_WEAPON_BUILDER:
		case TF_WEAPON_DISPENSER:
		case TF_WEAPON_INVIS:
		case TF_WEAPON_WRENCH:			// skip this so engineer building doesn't mark 'in combat'
			// not a 'combat' weapon
			return;
		};

		// important to keep this at one second, so rate cvars make sense (units/sec)
		m_inCombatThrottleTimer.Start( 1.0f );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Remove invisibility, called when player attacks
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveInvisibility( void )
{
	if ( !m_Shared.IsStealthed() )
		return;

	m_Shared.FadeInvis( 0.5f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SaveMe( void )
{
	if ( !IsAlive() || IsPlayerClass( TF_CLASS_UNDEFINED ) || GetTeamNumber() < TF_TEAM_RED )
		return;

	m_bSaveMeParity = !m_bSaveMeParity;
}

//-----------------------------------------------------------------------------
// Purpose: drops the flag
//-----------------------------------------------------------------------------
void CC_DropItem( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_GetCommandClient() );
	if ( !pPlayer )
		return;
	
	if ( pPlayer->HasTheFlag() )
	{
		pPlayer->DropFlag();
	}
}
static ConCommand dropitem( "dropitem", CC_DropItem, "Drop the flag." );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CObserverPoint::CObserverPoint()
{
	m_bMatchSummary = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObserverPoint::Activate( void )
{
	BaseClass::Activate();

	if ( m_bMatchSummary )
	{
		// sanity check to make sure the competitive match summary target is disabled until we're ready for it
		SetDisabled( true );
	}

	if ( m_iszAssociateTeamEntityName != NULL_STRING )
	{
		m_hAssociatedTeamEntity = gEntList.FindEntityByName( NULL, m_iszAssociateTeamEntityName );
		if ( !m_hAssociatedTeamEntity )
		{
			Warning("info_observer_point (%s) couldn't find associated team entity named '%s'\n", GetDebugName(), STRING(m_iszAssociateTeamEntityName) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CObserverPoint::CanUseObserverPoint( CTFPlayer *pPlayer )
{
	if ( m_bDisabled )
		return false;

	// Only spectate observer points on control points in the current miniround
	if ( g_pObjectiveResource->PlayingMiniRounds() && m_hAssociatedTeamEntity )
	{
		CTeamControlPoint *pPoint = dynamic_cast<CTeamControlPoint*>(m_hAssociatedTeamEntity.Get());
		if ( pPoint )
		{
			bool bInRound = g_pObjectiveResource->IsInMiniRound( pPoint->GetPointIndex() );
			if ( !bInRound )
				return false;
		}
	}

	if ( m_hAssociatedTeamEntity && mp_forcecamera.GetInt() == OBS_ALLOW_TEAM )
	{
		// don't care about this check during a team win
		if ( TFGameRules() && TFGameRules()->State_Get() != GR_STATE_TEAM_WIN )
		{
			// If we don't own the associated team entity, we can't use this point
			if ( m_hAssociatedTeamEntity->GetTeamNumber() != pPlayer->GetTeamNumber() && pPlayer->GetTeamNumber() >= FIRST_GAME_TEAM )
				return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CObserverPoint::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObserverPoint::InputEnable( inputdata_t &inputdata )
{
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObserverPoint::InputDisable( inputdata_t &inputdata )
{
	m_bDisabled = true;
}

BEGIN_DATADESC( CObserverPoint )
DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),
DEFINE_KEYFIELD( m_bDefaultWelcome, FIELD_BOOLEAN, "defaultwelcome" ),
DEFINE_KEYFIELD( m_iszAssociateTeamEntityName, FIELD_STRING, "associated_team_entity" ),
DEFINE_KEYFIELD( m_flFOV, FIELD_FLOAT, "fov" ),
DEFINE_KEYFIELD( m_bMatchSummary, FIELD_BOOLEAN, "match_summary" ),

DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( info_observer_point, CObserverPoint );

//-----------------------------------------------------------------------------
// Purpose: Builds a list of entities that this player can observe.
//			Returns the index into the list of the player's current observer target.
//-----------------------------------------------------------------------------
int CTFPlayer::BuildObservableEntityList( void )
{
	m_hObservableEntities.Purge();
	int iCurrentIndex = -1;

	// Add all the map-placed observer points
	CBaseEntity *pObserverPoint = gEntList.FindEntityByClassname( NULL, "info_observer_point" );
	while ( pObserverPoint )
	{
		m_hObservableEntities.AddToTail( pObserverPoint );

		if ( m_hObserverTarget.Get() == pObserverPoint )
		{
			iCurrentIndex = (m_hObservableEntities.Count()-1);
		}

		pObserverPoint = gEntList.FindEntityByClassname( pObserverPoint, "info_observer_point" );
	}

	// Add all the players
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer )
		{
			m_hObservableEntities.AddToTail( pPlayer );

			if ( m_hObserverTarget.Get() == pPlayer )
			{
				iCurrentIndex = (m_hObservableEntities.Count()-1);
			}
		}
	}

	// Add all my objects
	int iNumObjects = GetObjectCount();
	for ( int i = 0; i < iNumObjects; i++ )
	{
		CBaseObject *pObj = GetObject( i );
		if ( pObj )
		{
			m_hObservableEntities.AddToTail( pObj );

			if ( m_hObserverTarget.Get() == pObj )
			{
				iCurrentIndex = ( m_hObservableEntities.Count() - 1 );
			}
		}
	}

	return iCurrentIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayer::GetNextObserverSearchStartPoint( bool bReverse )
{
	int iDir = bReverse ? -1 : 1; 
	int startIndex = BuildObservableEntityList();
	int iMax = m_hObservableEntities.Count()-1;

	startIndex += iDir;
	if (startIndex > iMax)
		startIndex = 0;
	else if (startIndex < 0)
		startIndex = iMax;

	return startIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CTFPlayer::FindNextObserverTarget(bool bReverse)
{
	int startIndex = GetNextObserverSearchStartPoint( bReverse );

	int	currentIndex = startIndex;
	int iDir = bReverse ? -1 : 1; 

	int iMax = m_hObservableEntities.Count()-1;

	// Make sure the current index is within the max. Can happen if we were previously
	// spectating an object which has been destroyed.
	if ( startIndex > iMax )
	{
		currentIndex = startIndex = 1;
	}

	do
	{
		CBaseEntity *nextTarget = m_hObservableEntities[currentIndex];

		if ( IsValidObserverTarget( nextTarget ) )
			return nextTarget;	
 
		currentIndex += iDir;

		// Loop through the entities
		if (currentIndex > iMax)
		{
			currentIndex = 0;
		}
		else if (currentIndex < 0)
		{
			currentIndex = iMax;
		}
	} while ( currentIndex != startIndex );

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::IsValidObserverTarget( CBaseEntity * target )
{
	if ( !target || ( target == this ) )
		return false;

	if ( !target->IsPlayer() )
	{
		if ( TFGameRules()->IsInTournamentMode() )
		{
			CObserverPoint *pObsPoint = dynamic_cast<CObserverPoint *>( target );
			if ( pObsPoint )
			{
				// just connected, initial observer point is okay
				if ( GetTeamNumber() < TEAM_SPECTATOR )
					return pObsPoint->CanUseObserverPoint( this );
			}
			
			// players only
			return false;
		}

		CObserverPoint *pObsPoint = dynamic_cast<CObserverPoint *>( target );
		if ( pObsPoint && !pObsPoint->CanUseObserverPoint( this ) )
			return false;

		CFuncTrackTrain *pTrain = dynamic_cast<CFuncTrackTrain *>( target );
		if ( pTrain )
		{
			// can only spec the trains while the round is running
			if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
				return false;
		}

		if ( GetTeamNumber() == TEAM_SPECTATOR )
			return true;
		
		switch ( mp_forcecamera.GetInt() )	
		{
		case OBS_ALLOW_ALL	:	break;
		case OBS_ALLOW_TEAM :	if ( target->GetTeamNumber() != TEAM_UNASSIGNED && GetTeamNumber() != target->GetTeamNumber() )
									return false;
								break;
		case OBS_ALLOW_NONE :	return false;
		}

		return true;
	}

	return BaseClass::IsValidObserverTarget( target );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PickWelcomeObserverPoint( void )
{
	//Don't just spawn at the world origin, find a nice spot to look from while we choose our team and class.
	CObserverPoint *pObserverPoint = (CObserverPoint *)gEntList.FindEntityByClassname( NULL, "info_observer_point" );

	while ( pObserverPoint )
	{
		if ( IsValidObserverTarget( pObserverPoint ) )
		{
			SetObserverTarget( pObserverPoint );
		}

		if ( pObserverPoint->IsDefaultWelcome() )
			break;

		pObserverPoint = (CObserverPoint *)gEntList.FindEntityByClassname( pObserverPoint, "info_observer_point" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::SetObserverTarget(CBaseEntity *target)
{
	ClearZoomOwner();
	SetFOV( this, 0 );
		
	if ( !BaseClass::SetObserverTarget(target) )
		return false;

	CObserverPoint *pObsPoint = dynamic_cast<CObserverPoint *>(target);
	if ( pObsPoint )
	{
		SetViewOffset( vec3_origin );
		JumptoPosition( target->GetAbsOrigin(), target->EyeAngles() );
		SetFOV( pObsPoint, pObsPoint->m_flFOV );
	}

	if ( !m_bArenaIsAFK )
	{
		m_flLastAction = gpGlobals->curtime;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Find the nearest team member within the distance of the origin.
//			Favor players who are the same class.
//-----------------------------------------------------------------------------
CBaseEntity *CTFPlayer::FindNearestObservableTarget( Vector vecOrigin, float flMaxDist )
{
	CTeam *pTeam = GetTeam();
	CBaseEntity *pReturnTarget = NULL;
	bool bFoundClass = false;
	float flCurDistSqr = (flMaxDist * flMaxDist);
	int iNumPlayers = pTeam->GetNumPlayers();

	if ( pTeam->GetTeamNumber() == TEAM_SPECTATOR )
	{
		iNumPlayers = gpGlobals->maxClients;
	}


	for ( int i = 0; i < iNumPlayers; i++ )
	{
		CTFPlayer *pPlayer = NULL;

		if ( pTeam->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		}
		else
		{
			pPlayer = ToTFPlayer( pTeam->GetPlayer(i) );
		}

		if ( !pPlayer )
			continue;

		if ( !IsValidObserverTarget(pPlayer) )
			continue;

		float flDistSqr = ( pPlayer->GetAbsOrigin() - vecOrigin ).LengthSqr();

		if ( flDistSqr < flCurDistSqr )
		{
			// If we've found a player matching our class already, this guy needs
			// to be a matching class and closer to boot.
			if ( !bFoundClass || pPlayer->IsPlayerClass( GetPlayerClass()->GetClassIndex() ) )
			{
				pReturnTarget = pPlayer;
				flCurDistSqr = flDistSqr;

				if ( pPlayer->IsPlayerClass( GetPlayerClass()->GetClassIndex() ) )
				{
					bFoundClass = true;
				}
			}
		}
		else if ( !bFoundClass )
		{
			if ( pPlayer->IsPlayerClass( GetPlayerClass()->GetClassIndex() ) )
			{
				pReturnTarget = pPlayer;
				flCurDistSqr = flDistSqr;
				bFoundClass = true;
			}
		}
	}

	if ( !bFoundClass && IsPlayerClass( TF_CLASS_ENGINEER ) )
	{
		// let's spectate our sentry instead, we didn't find any other engineers to spec
		int iNumObjects = GetObjectCount();
		for ( int i=0;i<iNumObjects;i++ )
		{
			CBaseObject *pObj = GetObject(i);

			if ( pObj && pObj->GetType() == OBJ_SENTRYGUN )
			{
				pReturnTarget = pObj;
			}
		}
	}		

	return pReturnTarget;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::FindInitialObserverTarget( void )
{
	// If we're on a team (i.e. not a pure observer), try and find
	// a target that'll give the player the most useful information.
	if ( GetTeamNumber() >= FIRST_GAME_TEAM )
	{
		CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
		if ( pMaster )
		{
			// Has our forward cap point been contested recently?
			int iFarthestPoint = TFGameRules()->GetFarthestOwnedControlPoint( GetTeamNumber(), false );
			if ( iFarthestPoint != -1 )
			{
				float flTime = pMaster->PointLastContestedAt( iFarthestPoint );
				if ( flTime != -1 && flTime > (gpGlobals->curtime - 30) )
				{
					// Does it have an associated viewpoint?
					CBaseEntity *pObserverPoint = gEntList.FindEntityByClassname( NULL, "info_observer_point" );
					while ( pObserverPoint )
					{
						CObserverPoint *pObsPoint = assert_cast<CObserverPoint *>(pObserverPoint);
						if ( pObsPoint && pObsPoint->m_hAssociatedTeamEntity == pMaster->GetControlPoint(iFarthestPoint) )
						{
							if ( IsValidObserverTarget( pObsPoint ) )
							{
								m_hObserverTarget.Set( pObsPoint );
								return;
							}
						}

						pObserverPoint = gEntList.FindEntityByClassname( pObserverPoint, "info_observer_point" );
					}
				}
			}

			// Has the point beyond our farthest been contested lately?
			iFarthestPoint += (ObjectiveResource()->GetBaseControlPointForTeam( GetTeamNumber() ) == 0 ? 1 : -1);
			if ( iFarthestPoint >= 0 && iFarthestPoint < MAX_CONTROL_POINTS )
			{
				float flTime = pMaster->PointLastContestedAt( iFarthestPoint );
				if ( flTime != -1 && flTime > (gpGlobals->curtime - 30) )
				{
					// Try and find a player near that cap point
					CBaseEntity *pCapPoint = pMaster->GetControlPoint(iFarthestPoint);
					if ( pCapPoint )
					{
						CBaseEntity *pTarget = FindNearestObservableTarget( pCapPoint->GetAbsOrigin(), 1500 );
						if ( pTarget )
						{
							m_hObserverTarget.Set( pTarget );
							return;
						}
					}
				}
			}
		}
	}

	// Find the nearest guy near myself
	CBaseEntity *pTarget = FindNearestObservableTarget( GetAbsOrigin(), FLT_MAX );
	if ( pTarget )
	{
		m_hObserverTarget.Set( pTarget );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ValidateCurrentObserverTarget( void )
{
	// If our current target is a dead player who's gibbed / died, re-find as if 
	// we were finding our initial target, so we end up somewhere useful.
	if ( m_hObserverTarget && m_hObserverTarget->IsPlayer() )
	{
		CBasePlayer *player = ToBasePlayer( m_hObserverTarget );

		if ( player->m_lifeState == LIFE_DEAD || player->m_lifeState == LIFE_DYING )
		{
			// Once we're past the pause after death, find a new target
			if ( (player->GetDeathTime() + DEATH_ANIMATION_TIME ) < gpGlobals->curtime )
			{
				FindInitialObserverTarget();
			}

			return;
		}
	}

	if ( m_hObserverTarget && !m_hObserverTarget->IsPlayer() )
	{
		// can only spectate players in-eye
		if ( m_iObserverMode == OBS_MODE_IN_EYE )
		{
			ForceObserverMode( OBS_MODE_CHASE );
		}
	}

	BaseClass::ValidateCurrentObserverTarget();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::CheckObserverSettings()
{
	// make sure we are always observing the student
	if ( TFGameRules() )
	{
		// is there a current entity that is the required spectator target?
		if ( TFGameRules()->GetRequiredObserverTarget() )
		{
			SetObserverTarget( TFGameRules()->GetRequiredObserverTarget() );
			return;
		}
		
		// make sure we're not trying to spec the train during a team win
		// if we are, switch to spectating the last control point instead (where the train ended)
		if ( m_hObserverTarget && m_hObserverTarget->IsBaseTrain() && TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
		{
			// find the nearest spectator point to use instead of the train
			CObserverPoint *pObserverPoint = (CObserverPoint *)gEntList.FindEntityByClassname( NULL, "info_observer_point" );
			CObserverPoint *pClosestPoint = NULL;
			float flMinDistance = -1.0f;
			Vector vecTrainOrigin = m_hObserverTarget->GetAbsOrigin();

			while ( pObserverPoint )
			{
				if ( IsValidObserverTarget( pObserverPoint ) )
				{
					float flDist = pObserverPoint->GetAbsOrigin().DistTo( vecTrainOrigin );
					if ( flMinDistance < 0 || flDist < flMinDistance )
					{
						flMinDistance = flDist;
						pClosestPoint = pObserverPoint;
					}
				}

				pObserverPoint = (CObserverPoint *)gEntList.FindEntityByClassname( pObserverPoint, "info_observer_point" );
			}

			if ( pClosestPoint )
			{
				SetObserverTarget( pClosestPoint );
			}
		}
	}

	BaseClass::CheckObserverSettings();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::Touch( CBaseEntity *pOther )
{
	CTFPlayer *pVictim = ToTFPlayer( pOther );

	if ( pVictim )
	{
		// ACHIEVEMENT_TF_SPY_BUMP_CLOAKED_SPY
		if ( !m_Shared.IsAlly( pVictim ) )
		{
			if ( IsPlayerClass( TF_CLASS_SPY ) && pVictim->IsPlayerClass( TF_CLASS_SPY ) )
			{
				if ( m_Shared.InCond( TF_COND_STEALTHED ) && pVictim->m_Shared.InCond( TF_COND_STEALTHED ) )
				{
					AwardAchievement( ACHIEVEMENT_TF_SPY_BUMP_CLOAKED_SPY );
				}
			}
		}

		CheckUncoveringSpies( pVictim );

		// ACHIEVEMENT_TF_HEAVY_BLOCK_INVULN_HEAVY
		if ( !m_Shared.IsAlly( pVictim ) )
		{
			if ( IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) && pVictim->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
			{
				CTFTeam *pTeam = GetGlobalTFTeam( GetTeamNumber() );
				if ( pTeam && pTeam->GetRole() == TEAM_ROLE_DEFENDERS )
				{
					if ( m_Shared.InCond( TF_COND_INVULNERABLE ) || m_Shared.InCond( TF_COND_INVULNERABLE_WEARINGOFF ) )
					{
						if ( pVictim->m_Shared.InCond( TF_COND_INVULNERABLE ) || pVictim->m_Shared.InCond( TF_COND_INVULNERABLE_WEARINGOFF ) )
						{
							float flMaxSpeed = 50.0f * 50.0f;
							if ( ( GetAbsVelocity().LengthSqr() < flMaxSpeed ) && ( pVictim->GetAbsVelocity().LengthSqr() < flMaxSpeed ) )
							{
								AwardAchievement( ACHIEVEMENT_TF_HEAVY_BLOCK_INVULN_HEAVY );
							}
						}
					}
				}
			}
		}
	}

	BaseClass::Touch( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::RefreshCollisionBounds( void )
{
	BaseClass::RefreshCollisionBounds();

	SetViewOffset( ( IsDucked() ) ? ( VEC_DUCK_VIEW_SCALED( this ) ) : ( GetClassEyeHeight() ) );
}

//-----------------------------------------------------------------------------------------------------
// Return true if the given threat is aiming in our direction
bool CTFPlayer::IsThreatAimingTowardMe( CBaseEntity *threat, float cosTolerance ) const
{
	CTFPlayer *player = ToTFPlayer( threat );
	Vector to = GetAbsOrigin() - threat->GetAbsOrigin();
	float threatRange = to.NormalizeInPlace();
	Vector forward;

	if ( player == NULL )
	{
		CObjectSentrygun *sentry = dynamic_cast< CObjectSentrygun * >( threat );
		if ( sentry )
		{
			// are we in range?
			if ( threatRange < SENTRY_MAX_RANGE )
			{
				// is it pointing at us?
				AngleVectors( sentry->GetTurretAngles(), &forward );

				if ( DotProduct( to, forward ) > cosTolerance )
				{
					return true;
				}
			}
		}

		// not a player, not a sentry, not a threat?
		return false;
	}

	// is the player pointing at me?
	player->EyeVectors( &forward );

	if ( DotProduct( to, forward ) > cosTolerance )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------------------------------
// Return true if the given threat is aiming in our direction and firing its weapon
bool CTFPlayer::IsThreatFiringAtMe( CBaseEntity *threat ) const
{
	if ( IsThreatAimingTowardMe( threat ) )
	{
		CTFPlayer *player = ToTFPlayer( threat );

		if ( player )
		{
			return player->IsFiringWeapon();
		}

		CObjectSentrygun *sentry = dynamic_cast< CObjectSentrygun * >( threat );
		if ( sentry )
		{
			return sentry->GetTimeSinceLastFired() < 1.0f;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Check to see if this player has seen through an enemy spy's disguise
//-----------------------------------------------------------------------------
void CTFPlayer::CheckUncoveringSpies( CTFPlayer *pTouchedPlayer )
{
	// Only uncover enemies
	if ( m_Shared.IsAlly( pTouchedPlayer ) )
	{
		return;
	}

	// Only uncover if they're stealthed
	if ( !pTouchedPlayer->m_Shared.InCond( TF_COND_STEALTHED ) )
	{
		return;
	}

	// pulse their invisibility
	pTouchedPlayer->m_Shared.OnSpyTouchedByEnemy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::OnTauntSucceeded( const char* pszSceneName, int iTauntIndex /*= 0*/, int iTauntConcept /*= 0*/ )
{
	float flDuration = GetSceneDuration( pszSceneName ) + 0.2f;

	// Set player state as taunting.
	m_Shared.m_iTauntConcept.Set( iTauntConcept );
	m_flTauntStartTime = gpGlobals->curtime;
	m_flTauntNextStartTime = m_flTauntStartTime + flDuration;

	m_Shared.AddCond( TF_COND_TAUNTING );

	m_flTauntRemoveTime = gpGlobals->curtime + flDuration;

	m_angTauntCamera = EyeAngles();

	// Slam velocity to zero.
	SetAbsVelocity( vec3_origin );

	// set initial taunt yaw to make sure that the client anim not off because of lag
	SetTauntYaw( GetAbsAngles()[YAW] );

	m_vecTauntStartPosition = GetAbsOrigin();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::Taunt( taunts_t iTauntIndex, int iTauntConcept )
{
	if ( !IsAllowedToTaunt() )
		return;

	// Allow voice commands, etc to be interrupted.
	CMultiplayer_Expresser *pExpresser = GetMultiplayerExpresser();
	Assert( pExpresser );
	pExpresser->AllowMultipleScenes();

	m_bInitTaunt = true;
	char szResponse[AI_Response::MAX_RESPONSE_NAME];
	iTauntConcept = MP_CONCEPT_PLAYER_TAUNT;

	if ( SpeakConceptIfAllowed( iTauntConcept, NULL, szResponse, AI_Response::MAX_RESPONSE_NAME ) )
	{
		OnTauntSucceeded( szResponse, iTauntIndex, iTauntConcept );
	}
	else
	{
		m_bInitTaunt = false;
	}

	pExpresser->DisallowMultipleScenes();
}

//-----------------------------------------------------------------------------
// Purpose: Aborts a taunt in progress.
//-----------------------------------------------------------------------------
void CTFPlayer::CancelTaunt( void )
{
	StopTaunt();
}

//-----------------------------------------------------------------------------
// Purpose: Stops taunting
//-----------------------------------------------------------------------------
void CTFPlayer::StopTaunt( void )
{
	if ( m_hTauntScene.Get() )
	{
		StopScriptedScene( this, m_hTauntScene );
		m_flTauntRemoveTime = 0.0f;
		m_hTauntScene = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::HandleTauntCommand( void )
{
	if ( !IsAllowedToTaunt() )
		return;

	Taunt( TAUNT_BASE_WEAPON );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void DispatchRPSEffect( const CTFPlayer *pPlayer, const char* pszParticleName )
{
	CEffectData	data;
	data.m_nHitBox = GetParticleSystemIndex( pszParticleName );
	data.m_vOrigin = pPlayer->GetAbsOrigin() + Vector( 0, 0, 87.0f );
	data.m_vAngles = vec3_angle;

	CPASFilter intiatorFilter( data.m_vOrigin );
	intiatorFilter.SetIgnorePredictionCull( true );

	te->DispatchEffect( intiatorFilter, 0.0, data.m_vOrigin, "ParticleEffect", data );
}

//-----------------------------------------------------------------------------
// Purpose: Play a one-shot scene
// Input  :
// Output :
//-----------------------------------------------------------------------------
float CTFPlayer::PlayScene( const char *pszScene, float flDelay, AI_Response *response, IRecipientFilter *filter )
{
	MDLCACHE_CRITICAL_SECTION();

	// This is a lame way to detect a taunt!
	if ( m_bInitTaunt )
	{
		m_bInitTaunt = false;
		return InstancedScriptedScene( this, pszScene, &m_hTauntScene, flDelay, false, response, true, filter );
	}
	else
	{
		return InstancedScriptedScene( this, pszScene, NULL, flDelay, false, response, true, filter );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ModifyOrAppendCriteria( AI_CriteriaSet& criteriaSet )
{
	BaseClass::ModifyOrAppendCriteria( criteriaSet );

	// If we have 'disguiseclass' criteria, pretend that we are actually our
	// disguise class. That way we just look up the scene we would play as if 
	// we were that class.
	int disguiseIndex = criteriaSet.FindCriterionIndex( "disguiseclass" );

	if ( disguiseIndex != -1 )
	{
		criteriaSet.AppendCriteria( "playerclass", criteriaSet.GetValue(disguiseIndex) );
	}
	else
	{
		if ( GetPlayerClass() )
		{
			criteriaSet.AppendCriteria( "playerclass", g_aPlayerClassNames_NonLocalized[ GetPlayerClass()->GetClassIndex() ] );
		}
	}

	bool bRedTeam = ( GetTeamNumber() == TF_TEAM_RED );
	if ( m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		bRedTeam = ( m_Shared.GetDisguiseTeam() == TF_TEAM_RED );
	}
	criteriaSet.AppendCriteria( "OnRedTeam", bRedTeam ? "1" : "0" );
	
	criteriaSet.AppendCriteria( "recentkills", UTIL_VarArgs("%d", m_Shared.GetNumKillsInTime(30.0)) );
	

	int iTotalKills = 0;
	PlayerStats_t *pStats = CTF_GameStats.FindPlayerStats( this );
	if ( pStats )
	{
		iTotalKills = pStats->statsCurrentLife.m_iStat[TFSTAT_KILLS] + pStats->statsCurrentLife.m_iStat[TFSTAT_KILLASSISTS]+ 
			pStats->statsCurrentLife.m_iStat[TFSTAT_BUILDINGSDESTROYED];
	}
	criteriaSet.AppendCriteria( "killsthislife", UTIL_VarArgs( "%d", iTotalKills ) );
	criteriaSet.AppendCriteria( "disguised", m_Shared.InCond( TF_COND_DISGUISED ) ? "1" : "0" );
	criteriaSet.AppendCriteria( "cloaked", ( m_Shared.IsStealthed() || m_Shared.InCond( TF_COND_STEALTHED_BLINK ) ) ? "1" : "0" );
	criteriaSet.AppendCriteria( "invulnerable", m_Shared.InCond( TF_COND_INVULNERABLE ) ? "1" : "0" );
	criteriaSet.AppendCriteria( "beinghealed", m_Shared.InCond( TF_COND_HEALTH_BUFF ) ? "1" : "0" );
	criteriaSet.AppendCriteria( "waitingforplayers", (TFGameRules()->IsInWaitingForPlayers() || TFGameRules()->IsInPreMatch()) ? "1" : "0" );

	criteriaSet.AppendCriteria( "stunned", "0" );
	criteriaSet.AppendCriteria( "snared",  "0" );
	criteriaSet.AppendCriteria( "dodging",  "0" );
	criteriaSet.AppendCriteria( "doublejumping", (m_Shared.GetAirDash()>0) ? "1" : "0" );

	switch ( GetTFTeam()->GetRole() )
	{
	case TEAM_ROLE_DEFENDERS:
		criteriaSet.AppendCriteria( "teamrole", "defense" );
		break;
	case TEAM_ROLE_ATTACKERS:
		criteriaSet.AppendCriteria( "teamrole", "offense" );
		break;
	}

	// Current weapon role
	CTFWeaponBase *pActiveWeapon = m_Shared.GetActiveTFWeapon();
	if ( pActiveWeapon )
	{
		int iWeaponRole = pActiveWeapon->GetTFWpnData().m_iWeaponType;
		switch( iWeaponRole )
		{
		case TF_WPN_TYPE_PRIMARY:
		default:
			criteriaSet.AppendCriteria( "weaponmode", "primary" );
			break;
		case TF_WPN_TYPE_SECONDARY:
			criteriaSet.AppendCriteria( "weaponmode", "secondary" );
			break;
		case TF_WPN_TYPE_MELEE:
			criteriaSet.AppendCriteria( "weaponmode", "melee" );
			break;
		case TF_WPN_TYPE_BUILDING:
			criteriaSet.AppendCriteria( "weaponmode", "building" );
			break;
		case TF_WPN_TYPE_PDA:
			criteriaSet.AppendCriteria( "weaponmode", "pda" );
			break;
		}

		if ( WeaponID_IsSniperRifle( pActiveWeapon->GetWeaponID() ) )
		{
			CTFSniperRifle *pRifle = dynamic_cast<CTFSniperRifle*>(pActiveWeapon);
			if ( pRifle && pRifle->IsZoomed() )
			{
				criteriaSet.AppendCriteria( "sniperzoomed", "1" );
			}
		}
		else if ( pActiveWeapon->GetWeaponID() == TF_WEAPON_MINIGUN )
		{
			CTFMinigun *pMinigun = dynamic_cast<CTFMinigun*>(pActiveWeapon);
			if ( pMinigun )
			{
				criteriaSet.AppendCriteria( "minigunfiretime", UTIL_VarArgs( "%.1f", pMinigun->GetFiringDuration() ) );
			}
		}
	}

	// Player under crosshair
	trace_t tr;
	Vector forward;
	EyeVectors( &forward );
	UTIL_TraceLine( EyePosition(), EyePosition() + (forward * MAX_TRACE_LENGTH), MASK_BLOCKLOS_AND_NPCS, this, COLLISION_GROUP_NONE, &tr );
	if ( !tr.startsolid && tr.DidHitNonWorldEntity() )
	{
		CBaseEntity *pEntity = tr.m_pEnt;
		if ( pEntity && pEntity->IsPlayer() )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer(pEntity);
			if ( pTFPlayer )
			{
				int iClass = pTFPlayer->GetPlayerClass()->GetClassIndex();
				if ( !InSameTeam(pTFPlayer) )
				{
					// Prevent spotting stealthed enemies who haven't been exposed recently
					if ( pTFPlayer->m_Shared.InCond( TF_COND_STEALTHED ) )
					{
						if ( pTFPlayer->m_Shared.GetLastStealthExposedTime() < (gpGlobals->curtime - 3.0) )
						{
							iClass = TF_CLASS_UNDEFINED;
						}
						else
						{
							iClass = TF_CLASS_SPY;
						}
					}
					else if ( pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
					{
						iClass = pTFPlayer->m_Shared.GetDisguiseClass();
					}
				}

				if ( iClass > TF_CLASS_UNDEFINED && iClass <= TF_LAST_NORMAL_CLASS )
				{
					criteriaSet.AppendCriteria( "crosshair_on", g_aPlayerClassNames_NonLocalized[iClass] );

					int iVisibleTeam = pTFPlayer->GetTeamNumber();
					if ( pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
					{
						iVisibleTeam = pTFPlayer->m_Shared.GetDisguiseTeam();
					}

					if ( iVisibleTeam != GetTeamNumber() )
					{
						criteriaSet.AppendCriteria( "crosshair_enemy", "yes" );
					}
				}
			}
		}
	}

	// Previous round win
	bool bLoser = ( TFGameRules()->GetPreviousRoundWinners() != TEAM_UNASSIGNED && TFGameRules()->GetPreviousRoundWinners() != GetPrevRoundTeamNum() );
	criteriaSet.AppendCriteria( "LostRound", bLoser ? "1" : "0" );

	bool bPrevRoundTie = ( ( TFGameRules()->GetRoundsPlayed() > 0 ) && ( TFGameRules()->GetPreviousRoundWinners() == TEAM_UNASSIGNED ) );
	criteriaSet.AppendCriteria( "PrevRoundWasTie", bPrevRoundTie ? "1" : "0" );

	// Control points
	CTriggerAreaCapture *pAreaTrigger = GetControlPointStandingOn();
	if ( pAreaTrigger )
	{
		CTeamControlPoint *pCP = pAreaTrigger->GetControlPoint();
		if ( pCP )
		{
			if ( pCP->GetOwner() == GetTeamNumber() )
			{
				criteriaSet.AppendCriteria( "OnFriendlyControlPoint", "1" );
			}
			else 
			{
				if ( TeamplayGameRules()->TeamMayCapturePoint( GetTeamNumber(), pCP->GetPointIndex() ) && 
					 TeamplayGameRules()->PlayerMayCapturePoint( this, pCP->GetPointIndex() ) )
				{
					criteriaSet.AppendCriteria( "OnCappableControlPoint", "1" );
				}
			}
		}
	}

	bool bIsBonusTime = false;
	bool bGameOver = false;

	// Current game state
	criteriaSet.AppendCriteria( "GameRound", UTIL_VarArgs( "%d", TFGameRules()->State_Get() ) ); 
	if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
	{
		criteriaSet.AppendCriteria( "OnWinningTeam", ( TFGameRules()->GetWinningTeam() == GetTeamNumber() ) ? "1" : "0" ); 

		bIsBonusTime = ( TFGameRules()->GetStateTransitionTime() > gpGlobals->curtime );
		bGameOver = TFGameRules()->IsGameOver();
	}

	// Number of rounds played
	criteriaSet.AppendCriteria( "RoundsPlayed", UTIL_VarArgs( "%d", TFGameRules()->GetRoundsPlayed() ) );

	criteriaSet.AppendCriteria( "IsComp6v6", "0" );

	criteriaSet.AppendCriteria( "IsCompWinner", "0" );

	criteriaSet.AppendCriteria( "IsInHell", "0" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTriggerAreaCapture *CTFPlayer::GetControlPointStandingOn( void )
{
	touchlink_t *root = ( touchlink_t * )GetDataObject( TOUCHLINK );
	if ( root )
	{
		for ( touchlink_t *link = root->nextLink; link != root; link = link->nextLink )
		{
			CBaseEntity *pTouch = link->entityTouched;
			if ( pTouch && pTouch->IsSolidFlagSet( FSOLID_TRIGGER ) && pTouch->IsBSPModel() )
			{
				CTriggerAreaCapture *pAreaTrigger = dynamic_cast<CTriggerAreaCapture*>(pTouch);
				if ( pAreaTrigger )
					return pAreaTrigger;
			}
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanHearAndReadChatFrom( CBasePlayer *pPlayer )
{
	// can always hear the console unless we're ignoring all chat
	if ( !pPlayer )
		return m_iIgnoreGlobalChat != CHAT_IGNORE_ALL;

	// check if we're ignoring all chat
	if ( m_iIgnoreGlobalChat == CHAT_IGNORE_ALL )
		return false;

	// check if we're ignoring all but teammates
	if ( ( m_iIgnoreGlobalChat == CHAT_IGNORE_TEAM ) && ( g_pGameRules->PlayerRelationship( this, pPlayer ) != GR_TEAMMATE ) )
		return false;

	if ( pPlayer->m_lifeState != LIFE_ALIVE && m_lifeState == LIFE_ALIVE )
	{
		// Everyone can chat like normal when the round/game ends
		if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN || TFGameRules()->State_Get() == GR_STATE_GAME_OVER )
			return true;

		if ( !tf_gravetalk.GetBool() )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanBeAutobalanced()
{
	if ( IsBot() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
IResponseSystem *CTFPlayer::GetResponseSystem()
{
	int iClass = GetPlayerClass()->GetClassIndex();

	if ( m_bSpeakingConceptAsDisguisedSpy && m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		iClass = m_Shared.GetDisguiseClass();
	}

	bool bValidClass = ( iClass >= TF_CLASS_SCOUT && iClass <= TF_LAST_NORMAL_CLASS );
	bool bValidConcept = ( m_iCurrentConcept >= 0 && m_iCurrentConcept < MP_TF_CONCEPT_COUNT );
	Assert( bValidClass );
	Assert( bValidConcept );

	if ( !bValidClass || !bValidConcept )
	{
		return BaseClass::GetResponseSystem();
	}
	else
	{
		return TFGameRules()->m_ResponseRules[iClass].m_ResponseSystems[m_iCurrentConcept];
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::SpeakConceptIfAllowed( int iConcept, const char *modifiers, char *pszOutResponseChosen, size_t bufsize, IRecipientFilter *filter )
{
	if ( !IsAlive() )
		return false;

	bool bReturn = false;

	if ( IsSpeaking() )
	{
		if ( iConcept != MP_CONCEPT_DIED )
			return false;
	}

	// Save the current concept.
	m_iCurrentConcept = iConcept;

	if ( m_Shared.InCond( TF_COND_DISGUISED ) && !filter && ( iConcept != MP_CONCEPT_KILLED_PLAYER ) )
	{
		CSingleUserRecipientFilter filter(this);

		int iEnemyTeam = ( GetTeamNumber() == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED;

		// test, enemies and myself
		CTeamRecipientFilter disguisedFilter( iEnemyTeam );
		disguisedFilter.AddRecipient( this );

		CMultiplayer_Expresser *pExpresser = GetMultiplayerExpresser();
		Assert( pExpresser );

		pExpresser->AllowMultipleScenes();

		// play disguised concept to enemies and myself
		char buf[128];
		Q_snprintf( buf, sizeof(buf), "disguiseclass:%s", g_aPlayerClassNames_NonLocalized[ m_Shared.GetDisguiseClass() ] );

		if ( modifiers )
		{
			Q_strncat( buf, ",", sizeof(buf), 1 );
			Q_strncat( buf, modifiers, sizeof(buf), COPY_ALL_CHARACTERS );
		}

		m_bSpeakingConceptAsDisguisedSpy = true;

		bool bPlayedDisguised = SpeakIfAllowed( g_pszMPConcepts[iConcept], buf, pszOutResponseChosen, bufsize, &disguisedFilter );

		m_bSpeakingConceptAsDisguisedSpy = false;

		// test, everyone except enemies and myself
		CBroadcastRecipientFilter undisguisedFilter;
		undisguisedFilter.RemoveRecipientsByTeam( GetGlobalTFTeam(iEnemyTeam) );
		undisguisedFilter.RemoveRecipient( this );

		// play normal concept to teammates
		bool bPlayedNormally = SpeakIfAllowed( g_pszMPConcepts[iConcept], modifiers, pszOutResponseChosen, bufsize, &undisguisedFilter );

		pExpresser->DisallowMultipleScenes();

		bReturn = ( bPlayedDisguised || bPlayedNormally );
	}
	else
	{
		// play normally
		bReturn = SpeakIfAllowed( g_pszMPConcepts[iConcept], modifiers, pszOutResponseChosen, bufsize, filter );
	}

	//Add bubble on top of a player calling for medic.
	if ( bReturn )
	{
		if ( iConcept == MP_CONCEPT_PLAYER_MEDIC )
		{
			SaveMe();
		}
	}

	return bReturn;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::UpdateExpression( void )
{
	char szScene[ MAX_PATH ];
	if ( !GetResponseSceneFromConcept( MP_CONCEPT_PLAYER_EXPRESSION, szScene, sizeof( szScene ) ) )
	{
		ClearExpression();
		m_flNextRandomExpressionTime = gpGlobals->curtime + RandomFloat(30,40);
		return;
	}
	
	// Ignore updates that choose the same scene
	if ( m_iszExpressionScene != NULL_STRING && stricmp( STRING(m_iszExpressionScene), szScene ) == 0 )
		return;

	if ( m_hExpressionSceneEnt )
	{
		ClearExpression();
	}

	m_iszExpressionScene = AllocPooledString( szScene );
	float flDuration = InstancedScriptedScene( this, szScene, &m_hExpressionSceneEnt, 0.0, true, NULL, true );
	m_flNextRandomExpressionTime = gpGlobals->curtime + flDuration;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ClearExpression( void )
{
	if ( m_hExpressionSceneEnt != NULL )
	{
		StopScriptedScene( this, m_hExpressionSceneEnt );
	}
	for ( int i = 0; i < MAX_FIRE_WEAPON_SCENES; i++ )
	{
		if ( m_hFireWeaponScenes[ i ] != NULL )
		{
			StopScriptedScene( this, m_hFireWeaponScenes[ i ] );
		}
	}
	m_flNextRandomExpressionTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: Only show subtitle to enemy if we're disguised as the enemy
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldShowVoiceSubtitleToEnemy( void )
{
	return ( m_Shared.InCond( TF_COND_DISGUISED ) && m_Shared.GetDisguiseTeam() != GetTeamNumber() );
}

//-----------------------------------------------------------------------------
// Purpose: Don't allow rapid-fire voice commands
//-----------------------------------------------------------------------------
bool CTFPlayer::CanSpeakVoiceCommand( void )
{
	if ( tf_voice_command_suspension_mode.GetInt() == 1 )
		return false;

	if ( gpGlobals->curtime <= m_flNextVoiceCommandTime )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Note the time we're allowed to next speak a voice command
//-----------------------------------------------------------------------------
void CTFPlayer::NoteSpokeVoiceCommand( const char *pszScenePlayed )
{
	Assert( pszScenePlayed );

	float flTimeSinceAllowedVoice = gpGlobals->curtime - m_flNextVoiceCommandTime;

	// if its longer than 5 seconds, reset the counter
	if ( flTimeSinceAllowedVoice > 5.0f )
	{
		m_iVoiceSpamCounter = 0;
	}
	// if its less than a second past the allowed time, player is spamming
	else if ( flTimeSinceAllowedVoice < 1.0f )
	{
		m_iVoiceSpamCounter++;
	}

	m_flNextVoiceCommandTime = gpGlobals->curtime + MIN( GetSceneDuration( pszScenePlayed ), tf_max_voice_speak_delay.GetFloat() );

	if ( m_iVoiceSpamCounter > 0 )
	{
		m_flNextVoiceCommandTime += m_iVoiceSpamCounter * 0.5f;
	}
}

extern ConVar friendlyfire;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::WantsLagCompensationOnEntity( const CBasePlayer *pPlayer, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const
{
	bool bIsMedic = false;
	bool bIsMeleeingTeamMate = false;

	if ( !friendlyfire.GetBool() )
	{
		//Do Lag comp on medics trying to heal team mates.
		if ( IsPlayerClass( TF_CLASS_MEDIC ) == true )
		{
			bIsMedic = true;

			if ( pPlayer->GetTeamNumber() == GetTeamNumber()  )
			{
				CWeaponMedigun *pWeapon = dynamic_cast <CWeaponMedigun*>( GetActiveWeapon() );

				if ( pWeapon && pWeapon->GetHealTarget() )
				{
					if ( pWeapon->GetHealTarget() == pPlayer )
						return true;
					else
						return false;
				}
			}
		}

		if ( pPlayer->GetTeamNumber() == GetTeamNumber() )
		{
			// Josh: Lag compensate melee attacks on teammates. Helps with weapons like the Solider's whip, etc.
			CTFWeaponBaseMelee *pWeapon = dynamic_cast< CTFWeaponBaseMelee * >( GetActiveWeapon() );
			if ( pWeapon )
			{
				bIsMeleeingTeamMate = true;
			}
			else
			{
				// Josh: Don't do any lag compensation on team-mates if we aren't a medic and not using melee.
				if ( bIsMedic == false )
					return false;
			}
		}
	}

	const Vector& vMyOrigin = GetAbsOrigin();
	const Vector& vHisOrigin = pPlayer->GetAbsOrigin();
	
	// If this entity hasn't been transmitted to us and acked, then don't bother lag compensating it.
	if ( pEntityTransmitBits && !pEntityTransmitBits->Get( pPlayer->entindex() ) )
		return false;

	// get max distance player could have moved within max lag compensation time, 
	// multiply by 1.5 to to avoid "dead zones"  (sqrt(2) would be the exact value)
	float maxDistance = 1.5 * pPlayer->MaxSpeed() * sv_maxunlag.GetFloat();

	// If the player is within this distance, lag compensate them in case they're running past us.
	if ( vHisOrigin.DistTo( vMyOrigin ) < maxDistance )
		return true;

	// Josh: Don't do cone check when melee-ing team mates, as we could be inside them.
	if ( !bIsMeleeingTeamMate )
	{
		// If their origin is not within a 45 degree cone in front of us, no need to lag compensate.
		Vector vForward;
		AngleVectors( pCmd->viewangles, &vForward );

		Vector vDiff = vHisOrigin - vMyOrigin;
		VectorNormalize( vDiff );

		float flCosAngle = 0.707107f;	// 45 degree angle
		if ( vForward.Dot( vDiff ) < flCosAngle )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SpeakWeaponFire( int iCustomConcept )
{
	if ( iCustomConcept == MP_CONCEPT_NONE )
	{
		if ( m_flNextSpeakWeaponFire > gpGlobals->curtime )
			return;

		iCustomConcept = MP_CONCEPT_FIREWEAPON;
	}

	m_flNextSpeakWeaponFire = gpGlobals->curtime + 5;

	char szScene[ MAX_PATH ];
	if ( !GetResponseSceneFromConcept( iCustomConcept, szScene, sizeof( szScene ) ) )
		return;

	if ( m_hFireWeaponScenes[ m_nNextFireWeaponScene ] != NULL )
	{
		StopScriptedScene( this, m_hFireWeaponScenes[ m_nNextFireWeaponScene ] );
	}
	
	float flDuration = InstancedScriptedScene( this, szScene, &m_hFireWeaponScenes[ m_nNextFireWeaponScene ], 0.0, true, NULL, true);
	m_flNextSpeakWeaponFire = gpGlobals->curtime + flDuration;
	m_nNextFireWeaponScene = ( m_nNextFireWeaponScene + 1 ) % MAX_FIRE_WEAPON_SCENES;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ClearWeaponFireScene( void )
{
	m_flNextSpeakWeaponFire = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayer::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];

		Q_snprintf( tempstr, sizeof( tempstr ),"Health: %d / %d ( %.1f )", GetHealth(), GetMaxHealth(), (float)GetHealth() / (float)GetMaxHealth() );
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: Get response scene corresponding to concept
//-----------------------------------------------------------------------------
bool CTFPlayer::GetResponseSceneFromConcept( int iConcept, char *chSceneBuffer, int numSceneBufferBytes )
{
	AI_Response response;
	bool result = SpeakConcept( response, iConcept );

	if ( result )
	{
		// Apply contexts
		if ( response.IsApplyContextToWorld() )
		{
			CBaseEntity *pEntity = CBaseEntity::Instance( engine->PEntityOfEntIndex( 0 ) );
			if ( pEntity )
			{
				pEntity->AddContext( response.GetContext() );
			}
		}
		else
		{
			AddContext( response.GetContext() );
		}

		const char *szResponse = response.GetResponsePtr();
		Q_strncpy( chSceneBuffer, szResponse, numSceneBufferBytes );
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:calculate a score for this player. higher is more likely to be switched
//-----------------------------------------------------------------------------
int	CTFPlayer::CalculateTeamBalanceScore( void )
{
	int iScore = BaseClass::CalculateTeamBalanceScore();

	// switch engineers less often
	if ( IsPlayerClass( TF_CLASS_ENGINEER ) )
	{
		iScore -= 120;
	}

	return iScore;
}

//-----------------------------------------------------------------------------
// Purpose: Exclude during win state
//-----------------------------------------------------------------------------
void CTFPlayer::AwardAchievement( int iAchievement, int iCount )
{
	if ( TFGameRules()->State_Get() >= GR_STATE_TEAM_WIN )
	{
		// allow the Helltower loot island achievement during the bonus time
		if ( iAchievement != ACHIEVEMENT_TF_HALLOWEEN_HELLTOWER_SKULL_ISLAND_REWARD )
		{
			// reject in endround
			return;
		}
	}

	BaseClass::AwardAchievement( iAchievement, iCount );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
// Debugging Stuff
void DebugParticles( const CCommand &args )
{
	CBaseEntity *pEntity = FindPickerEntity( UTIL_GetCommandClient() );

	if ( pEntity && pEntity->IsPlayer() )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pEntity );

		// print out their conditions
		pPlayer->m_Shared.DebugPrintConditions();	
	}
}

static ConCommand sv_debug_stuck_particles( "sv_debug_stuck_particles", DebugParticles, "Debugs particles attached to the player under your crosshair.", FCVAR_DEVELOPMENTONLY );

//-----------------------------------------------------------------------------
// Purpose: Debug concommand to set the player on fire
//-----------------------------------------------------------------------------
void IgnitePlayer()
{
	CTFPlayer *pPlayer = ToTFPlayer( ToTFPlayer( UTIL_PlayerByIndex( 1 ) ) );
	pPlayer->m_Shared.Burn( pPlayer, pPlayer->GetActiveTFWeapon() );
}
static ConCommand cc_IgnitePlayer( "tf_ignite_player", IgnitePlayer, "Sets you on fire", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TestVCD( const CCommand &args )
{
	CBaseEntity *pEntity = FindPickerEntity( UTIL_GetCommandClient() );
	if ( pEntity && pEntity->IsPlayer() )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pEntity );
		if ( pPlayer )
		{
			if ( args.ArgC() >= 2 )
			{
				InstancedScriptedScene( pPlayer, args[1], NULL, 0.0f, false, NULL, true );
			}
			else
			{
				InstancedScriptedScene( pPlayer, "scenes/heavy_test.vcd", NULL, 0.0f, false, NULL, true );
			}
		}
	}
}
static ConCommand tf_testvcd( "tf_testvcd", TestVCD, "Run a vcd on the player currently under your crosshair. Optional parameter is the .vcd name (default is 'scenes/heavy_test.vcd')", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TestRR( const CCommand &args )
{
	if ( args.ArgC() < 2 )
	{
		Msg("No concept specified. Format is tf_testrr <concept>\n");
		return;
	}

	CBaseEntity *pEntity = NULL;
	const char *pszConcept = args[1];

	if ( args.ArgC() == 3 )
	{
		pszConcept = args[2];
		pEntity = UTIL_PlayerByName( args[1] );
	}

	if ( !pEntity || !pEntity->IsPlayer() )
	{
		pEntity = FindPickerEntity( UTIL_GetCommandClient() );
		if ( !pEntity || !pEntity->IsPlayer() )
		{
			pEntity = ToTFPlayer( UTIL_GetCommandClient() ); 
		}
	}

	if ( pEntity && pEntity->IsPlayer() )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pEntity );
		if ( pPlayer )
		{
			int iConcept = GetMPConceptIndexFromString( pszConcept );
			if ( iConcept != MP_CONCEPT_NONE )
			{
				pPlayer->SpeakConceptIfAllowed( iConcept );
			}
			else
			{
				Msg( "Attempted to speak unknown multiplayer concept: %s\n", pszConcept );
			}
		}
	}
}
static ConCommand tf_testrr( "tf_testrr", TestRR, "Force the player under your crosshair to speak a response rule concept. Format is tf_testrr <concept>, or tf_testrr <player name> <concept>", FCVAR_CHEAT );

#ifdef _DEBUG
CON_COMMAND_F( tf_crashclients, "testing only, crashes about 50 percent of the connected clients.", FCVAR_DEVELOPMENTONLY )
{
	for ( int i = 1; i < gpGlobals->maxClients; ++i )
	{
		if ( RandomFloat( 0.0f, 1.0f ) < 0.5f )
		{
			CBasePlayer *pl = UTIL_PlayerByIndex( i + 1 );
			if ( pl )
			{
				engine->ClientCommand( pl->edict(), "crash\n" );
			}
		}
	}
}
#endif // _DEBUG
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldAnnounceAchievement( void )
{ 
	if ( IsPlayerClass( TF_CLASS_SPY ) )
	{
		if ( m_Shared.IsStealthed() ||
			 m_Shared.InCond( TF_COND_DISGUISED ) ||
			 m_Shared.InCond( TF_COND_DISGUISING ) )
		{
			return false;
		}
	}

	return BaseClass::ShouldAnnounceAchievement(); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::IsBeingCharged( void )
{
	if ( !IsPlayerClass(TF_CLASS_MEDIC) )
		return false;

	if ( !IsBot() )
	{
		INetChannelInfo *pNetChanInfo = engine->GetPlayerNetInfo( entindex() );
		if ( !pNetChanInfo || pNetChanInfo->IsTimingOut() )
			return false;

		float flUberDuration = weapon_medigun_chargerelease_rate.GetFloat();

		// Return invalid when the medic hasn't sent a usercommand in awhile
		if ( GetTimeSinceLastUserCommand() > flUberDuration + 1.f )
			return false;

		// Prevent an exploit where clients invalidate tickcount -
		// which causes their think functions to shut down
		if ( GetTimeSinceLastThink() > flUberDuration )
			return false;
	}

	CTFWeaponBase *pWpn = GetActiveTFWeapon();
	if ( !pWpn )
		return false;

	CWeaponMedigun *pMedigun = dynamic_cast<CWeaponMedigun*>(pWpn);
	if ( pMedigun && pMedigun->IsReleasingCharge() )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: ACHIEVEMENT_TF_MEDIC_ASSIST_HEAVY handler
//-----------------------------------------------------------------------------
void CTFPlayer::HandleAchievement_Medic_AssistHeavy( CTFPlayer *pPunchVictim )
{
	if ( !pPunchVictim )
	{
		// reset
		m_aPunchVictims.RemoveAll();
		return;
	}

	// we assisted punching this guy, while invuln

	// if this is a new unique punch victim
	if ( m_aPunchVictims.Find( pPunchVictim ) == m_aPunchVictims.InvalidIndex() )
	{
		m_aPunchVictims.AddToTail( pPunchVictim );

		if ( m_aPunchVictims.Count() >= 2 )
		{
			AwardAchievement( ACHIEVEMENT_TF_MEDIC_ASSIST_HEAVY );
		}
	}	
}

//-----------------------------------------------------------------------------
// Purpose: ACHIEVEMENT_TF_PYRO_KILL_FROM_BEHIND handler
//-----------------------------------------------------------------------------
void CTFPlayer::HandleAchievement_Pyro_BurnFromBehind( CTFPlayer *pBurner )
{
	if ( !pBurner )
	{
		// reset
		m_aBurnFromBackAttackers.RemoveAll();
		return;
	}

	if ( m_aBurnFromBackAttackers.Find( pBurner ) == m_aBurnFromBackAttackers.InvalidIndex() )
	{
		m_aBurnFromBackAttackers.AddToTail( pBurner );
	}	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ResetPerRoundStats( void )
{
	m_Shared.ResetArenaNumChanges();
	BaseClass::ResetPerRoundStats();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SaveLastWeaponSlot( void )
{
	if( !m_bRememberLastWeapon && !m_bRememberActiveWeapon )
	  return;
	
	if ( GetLastWeapon() )
	{	
		if ( !m_bSwitchedClass )
		{
			if ( !m_bRememberLastWeapon )
			{
				m_iLastWeaponSlot = 0;

				CTFWeaponBase *pWpn = m_Shared.GetActiveTFWeapon();
				if ( pWpn && m_iLastWeaponSlot == pWpn->GetSlot() )
				{
					m_iLastWeaponSlot = (m_iLastWeaponSlot == 0) ? 1 : 0;
				}
			}
			else 
			{
				m_iLastWeaponSlot = GetLastWeapon()->GetSlot();

				if ( !m_bRememberActiveWeapon )
				{
	 				if ( m_iLastWeaponSlot == 0 && m_Shared.GetActiveTFWeapon() )
	 				{
	 					m_iLastWeaponSlot = m_Shared.GetActiveTFWeapon()->GetSlot();
	 				}
				}
			}
		}
		else	
		{
			m_iLastWeaponSlot = 1;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveAllWeapons()
{
	// Base class RemoveAllWeapons() doesn't remove them properly.
	// (doesn't call unequip, or remove immediately. Results in incorrect provision
	//  state for players over round restarts, because players have 2x weapon entities)
	ClearActiveWeapon();
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		CBaseCombatWeapon *pWpn = m_hMyWeapons[i];
		if ( pWpn )
		{
			Weapon_Detach( pWpn );
			UTIL_Remove( pWpn );
		}
	}

	m_Shared.RemoveDisguiseWeapon();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::OnAchievementEarned( int iAchievement )
{
	BaseClass::OnAchievementEarned( iAchievement );

	SpeakConceptIfAllowed( MP_CONCEPT_ACHIEVEMENT_AWARD );
}

//-----------------------------------------------------------------------------
// Purpose: Handles USE keypress
//-----------------------------------------------------------------------------
void CTFPlayer::PlayerUse ( void )
{
	if ( tf_allow_player_use.GetBool() == false )
	{
		if ( !IsObserver() && !IsInCommentaryMode() )
		{
			return;
		}
	}

	BaseClass::PlayerUse();
}

void CTFPlayer::InputRoundSpawn( inputdata_t &inputdata )
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::IgnitePlayer()
{
	if ( FStrEq( "sd_doomsday", STRING( gpGlobals->mapname ) ) )
	{
		CTFPlayer *pRecentDamager = TFGameRules()->GetRecentDamager( this, 0, 5.0 );
		if ( pRecentDamager && ( pRecentDamager->GetTeamNumber() != GetTeamNumber() ) )
		{
			pRecentDamager->AwardAchievement( ACHIEVEMENT_TF_MAPS_DOOMSDAY_PUSH_INTO_EXHAUST );
		}
	}

	m_Shared.Burn( this, NULL );
}

void CTFPlayer::InputIgnitePlayer( inputdata_t &inputdata )
{
	IgnitePlayer();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetCustomModel( const char *pszModel )
{
	m_PlayerClass.SetCustomModel( pszModel );
	UpdateModel();
}

void CTFPlayer::InputSetCustomModel( inputdata_t &inputdata )
{
	SetCustomModel( inputdata.value.String() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetCustomModelWithClassAnimations( const char *pszModel )
{
	m_PlayerClass.SetCustomModel( pszModel, USE_CLASS_ANIMATIONS );
	UpdateModel();
}

void CTFPlayer::InputSetCustomModelWithClassAnimations( inputdata_t &inputdata )
{
	SetCustomModelWithClassAnimations( inputdata.value.String() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetCustomModelOffset( const Vector &offset )
{
	m_PlayerClass.SetCustomModelOffset( offset );
	InvalidatePhysicsRecursive( POSITION_CHANGED );
}

void CTFPlayer::InputSetCustomModelOffset( inputdata_t &inputdata )
{
	Vector vecTmp;
	inputdata.value.Vector3D( vecTmp );
	SetCustomModelOffset( vecTmp );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetCustomModelRotation( const QAngle &angle )
{
	m_PlayerClass.SetCustomModelRotation( angle );
	InvalidatePhysicsRecursive( ANGLES_CHANGED );
}

void CTFPlayer::InputSetCustomModelRotation( inputdata_t &inputdata )
{
	Vector vecTmp;
	inputdata.value.Vector3D( vecTmp );
	QAngle angTmp(vecTmp.x, vecTmp.y, vecTmp.z);
	m_PlayerClass.SetCustomModelRotation( angTmp );
	SetCustomModelRotation( angTmp );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ClearCustomModelRotation()
{
	m_PlayerClass.ClearCustomModelRotation();
	InvalidatePhysicsRecursive( ANGLES_CHANGED );
}

void CTFPlayer::InputClearCustomModelRotation( inputdata_t &inputdata )
{
	ClearCustomModelRotation();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetCustomModelRotates( bool bRotates )
{
	m_PlayerClass.SetCustomModelRotates( bRotates );
	InvalidatePhysicsRecursive( ANGLES_CHANGED );
}

void CTFPlayer::InputSetCustomModelRotates( inputdata_t &inputdata )
{
	SetCustomModelRotates( inputdata.value.Bool() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetCustomModelVisibleToSelf( bool bVisibleToSelf )
{
	m_PlayerClass.SetCustomModelVisibleToSelf( bVisibleToSelf );
}

void CTFPlayer::InputSetCustomModelVisibleToSelf( inputdata_t &inputdata )
{
	SetCustomModelVisibleToSelf( inputdata.value.Bool() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetForcedTauntCam( int nForceTauntCam )
{
	m_nForceTauntCam = nForceTauntCam;
}

void CTFPlayer::InputSetForcedTauntCam( inputdata_t &inputdata )
{
	SetForcedTauntCam( inputdata.value.Int() );
}

//-----------------------------------------------------------------------------
// Purpose: Adds this damager to the history list of people who damaged player
//-----------------------------------------------------------------------------
void CAchievementData::AddDamagerToHistory( EHANDLE hDamager )
{
	if ( !hDamager )
		return;

	EntityHistory_t newHist;
	newHist.hEntity = hDamager;
	newHist.flTimeDamage = gpGlobals->curtime;
	aDamagers.InsertHistory( newHist );
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether or not pDamager has damaged the player in the time specified
//-----------------------------------------------------------------------------
bool CAchievementData::IsDamagerInHistory( CBaseEntity *pDamager, float flTimeWindow )
{
	for ( int i = 0; i < aDamagers.Count(); i++ )
	{
		if ( ( gpGlobals->curtime - aDamagers[i].flTimeDamage ) > flTimeWindow )
			return false;

		if ( aDamagers[i].hEntity == pDamager )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the number of players who've damaged us in the time specified
//-----------------------------------------------------------------------------
int	CAchievementData::CountDamagersWithinTime( float flTime )
{
	int iCount = 0;
	for ( int i = 0; i < aDamagers.Count(); i++ )
	{
		if ( gpGlobals->curtime - aDamagers[i].flTimeDamage < flTime )
		{
			iCount++;
		}
	}

	return iCount;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAchievementData::AddTargetToHistory( EHANDLE hTarget )
{
	if ( !hTarget )
		return;

	EntityHistory_t newHist;
	newHist.hEntity = hTarget;
	newHist.flTimeDamage = gpGlobals->curtime;
	aTargets.InsertHistory( newHist );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CAchievementData::IsTargetInHistory( CBaseEntity *pTarget, float flTimeWindow )
{
	for ( int i = 0; i < aTargets.Count(); i++ )
	{
		if ( ( gpGlobals->curtime - aTargets[i].flTimeDamage ) > flTimeWindow )
			return false;

		if ( aTargets[i].hEntity == pTarget )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CAchievementData::CountTargetsWithinTime( float flTime )
{
	int iCount = 0;
	for ( int i = 0; i < aTargets.Count(); i++ )
	{
		if ( ( gpGlobals->curtime - aTargets[i].flTimeDamage ) < flTime )
		{
			iCount++;
		}
	}

	return iCount;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAchievementData::DumpDamagers( void )
{
	Msg("Damagers:\n");
	for ( int i = 0; i < aDamagers.Count(); i++ )
	{
		if ( aDamagers[i].hEntity )
		{
			if ( aDamagers[i].hEntity->IsPlayer() )
			{
				Msg("   %s : at %.2f (%.2f ago)\n", ToTFPlayer(aDamagers[i].hEntity)->GetPlayerName(), aDamagers[i].flTimeDamage, gpGlobals->curtime - aDamagers[i].flTimeDamage );
			}
			else
			{
				Msg("   %s : at %.2f (%.2f ago)\n", aDamagers[i].hEntity->GetDebugName(), aDamagers[i].flTimeDamage, gpGlobals->curtime - aDamagers[i].flTimeDamage );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds this attacker to the history of people who damaged this player
//-----------------------------------------------------------------------------
void CAchievementData::AddDamageEventToHistory( EHANDLE hAttacker, float flDmgAmount /*= 0.f*/ )
{
	if ( !hAttacker )
		return;

	EntityDamageHistory_t newHist;
	newHist.hEntity = hAttacker;
	newHist.flTimeDamage = gpGlobals->curtime;
	newHist.nDamageAmount = flDmgAmount;
	aDamageEvents.InsertHistory( newHist );
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether or not pEntity has damaged the player in the time specified
//-----------------------------------------------------------------------------
bool CAchievementData::IsEntityInDamageEventHistory( CBaseEntity *pEntity, float flTimeWindow )
{
	for ( int i = 0; i < aDamageEvents.Count(); i++ )
	{
		if ( aDamageEvents[i].hEntity != pEntity )
			continue;

		// Sorted
		if ( ( gpGlobals->curtime - aDamageEvents[i].flTimeDamage ) > flTimeWindow )
			break;

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: The sum of damage events from pEntity
//-----------------------------------------------------------------------------
int CAchievementData::GetAmountForDamagerInEventHistory( CBaseEntity *pEntity, float flTimeWindow )
{
	int nAmount = 0;

	for ( int i = 0; i < aDamageEvents.Count(); i++ )
	{
		if ( aDamageEvents[i].hEntity != pEntity )
			continue;
		
		// Msg( "   %s : at %.2f (%.2f ago)\n", ToTFPlayer( aDamageEvents[i].hEntity )->GetPlayerName(), aDamageEvents[i].flTimeDamage, gpGlobals->curtime - aDamageEvents[i].flTimeDamage );

		// Sorted
		if ( ( gpGlobals->curtime - aDamageEvents[i].flTimeDamage ) > flTimeWindow )
			break;

		nAmount += aDamageEvents[i].nDamageAmount;
	}

	return nAmount;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CAchievementData::GetFirstEntryTimeForDamagerInHistory( CBaseEntity *pEntity )
{
	float flEarliestTime = gpGlobals->curtime;

	for ( int i = 0; i < aDamageEvents.Count(); i++ )
	{
		if ( aDamageEvents[i].hEntity != pEntity )
			continue;

		// Sorted
		if ( aDamageEvents[i].flTimeDamage < flEarliestTime )
		{
			flEarliestTime = aDamageEvents[i].flTimeDamage;
		}
	}

	return flEarliestTime;
}

//-----------------------------------------------------------------------------
// Purpose: Adds hPlayer to the history of people who pushed this player
//-----------------------------------------------------------------------------
void CAchievementData::AddPusherToHistory( EHANDLE hPlayer )
{
	if ( !hPlayer )
		return;

	EntityHistory_t newHist;
	newHist.hEntity = hPlayer;
	newHist.flTimeDamage = gpGlobals->curtime;
	aPushers.InsertHistory( newHist );
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether or not pPlayer pushed the player in the time specified
//-----------------------------------------------------------------------------
bool CAchievementData::IsPusherInHistory( CBaseEntity *pPlayer, float flTimeWindow )
{
	for ( int i = 0; i < aPushers.Count(); i++ )
	{
		if ( ( gpGlobals->curtime - aPushers[i].flTimeDamage ) > flTimeWindow )
			return false;

		if ( aPushers[i].hEntity == pPlayer )
			return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Adds this damager to the history list of people whose sentry damaged player
//-----------------------------------------------------------------------------
void CAchievementData::AddSentryDamager( EHANDLE hDamager, EHANDLE hObject )
{
	if ( !hDamager )
		return;

	EntityHistory_t newHist;
	newHist.hEntity = hDamager;
	newHist.hObject = hObject;
	newHist.flTimeDamage = gpGlobals->curtime;
	aSentryDamagers.InsertHistory( newHist );
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether or not pDamager has damaged the player in the time specified (by way of sentry gun)
//-----------------------------------------------------------------------------
EntityHistory_t* CAchievementData::IsSentryDamagerInHistory( CBaseEntity *pDamager, float flTimeWindow )
{
	for ( int i = 0; i < aSentryDamagers.Count(); i++ )
	{
		if ( ( gpGlobals->curtime - aSentryDamagers[i].flTimeDamage ) > flTimeWindow )
			return NULL;

		if ( aSentryDamagers[i].hEntity == pDamager )
		{
			return &aSentryDamagers[i];
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
extern ConVar tf_allow_all_team_partner_taunt;
int CTFPlayer::GetAllowedTauntPartnerTeam() const
{
	return tf_allow_all_team_partner_taunt.GetBool() ? TEAM_ANY : GetTeamNumber();
}

//-----------------------------------------------------------------------------
// Purpose: Return true if any enemy sentry has LOS and is facing me and is in range to attack
//-----------------------------------------------------------------------------
bool CTFPlayer::IsAnyEnemySentryAbleToAttackMe( void ) const
{
	if ( m_Shared.InCond( TF_COND_DISGUISED ) ||
		 m_Shared.InCond( TF_COND_DISGUISING ) ||
		 m_Shared.IsStealthed() )
	{
		// I'm a disguised or cloaked Spy
		return false;
	}

	for ( int i=0; i<IBaseObjectAutoList::AutoList().Count(); ++i )
	{
		CBaseObject* pObj = static_cast< CBaseObject* >( IBaseObjectAutoList::AutoList()[i] );
		if ( pObj->ObjectType() != OBJ_SENTRYGUN )
			continue;
		
		if ( pObj->HasSapper() )
			continue;

		if ( pObj->IsPlasmaDisabled() )
			continue;

		if ( pObj->IsDisabled() )
			continue;

		if ( pObj->IsBuilding() )
			continue;

		// are we in range?
		if ( ( GetAbsOrigin() - pObj->GetAbsOrigin() ).IsLengthGreaterThan( SENTRY_MAX_RANGE ) )
			continue;

		// is the sentry aiming towards me?
		if ( !IsThreatAimingTowardMe( pObj, 0.95f ) )
			continue;

		// does the sentry have clear line of fire?
		if ( !IsLineOfSightClear( pObj, IGNORE_ACTORS ) )
			continue;

		// this sentry can attack me
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::RefundExperiencePoints( void )
{
	SetExperienceLevel( 1 );

	int nAmount = 0;
	PlayerStats_t *pPlayerStats = CTF_GameStats.FindPlayerStats( this );
	if ( pPlayerStats ) 
	{
		nAmount = pPlayerStats->statsCurrentRound.m_iStat[TFSTAT_CURRENCY_COLLECTED];
	}
	
	if ( nAmount > 0 )
	{
		SetExperiencePoints(nAmount);
	}

	CalculateExperienceLevel(false);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::CalculateExperienceLevel( bool bAnnounce /*= true*/ )
{
	int nMyLevel = GetExperienceLevel();

	int nPrevLevel = nMyLevel;
	float flLevel = ( (float)m_nExperiencePoints / 400.f ) + 1.f;
	flLevel = Min( flLevel, 20.f );

	// Ding?
	if ( bAnnounce )
	{
		if ( flLevel > 1 && nPrevLevel != (int)flLevel )
		{
			const char *pszTeamName = GetTeamNumber() == TF_TEAM_RED ? "RED" : "BLU";
			UTIL_ClientPrintAll( HUD_PRINTCENTER, "#TF_PlayerLeveled", pszTeamName, GetPlayerName(), CFmtStr( "%d", (int)flLevel ) );
			UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "#TF_PlayerLeveled", pszTeamName, GetPlayerName(), CFmtStr( "%d", (int)flLevel ) );
			DispatchParticleEffect( "mvm_levelup1", PATTACH_POINT_FOLLOW, this, "head" );
			EmitSound( "Achievement.Earned" );
		}
	}

	flLevel = floor( flLevel );
	SetExperienceLevel( Max( flLevel, 1.f ) );

	// Update level progress percentage - networked
	float flLevelPerc = ( flLevel - floor( flLevel ) ) * 100.f;
	if ( m_nExperienceLevelProgress != flLevelPerc )
	{
		m_nExperienceLevelProgress.Set( (int)flLevelPerc );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PlayReadySound( void )
{
	if ( m_flLastReadySoundTime < gpGlobals->curtime )
	{
		if ( TFGameRules() )
		{
			int iTeam = GetTeamNumber();
			const char *pszFormat = "%s.Ready";

			if ( TFGameRules()->IsCompetitiveMode() )
			{
				pszFormat = "%s.ReadyComp";
			}

			CFmtStr goYell( pszFormat, g_aPlayerClassNames_NonLocalized[ m_Shared.GetDesiredPlayerClassIndex() ] );
			TFGameRules()->BroadcastSound( iTeam, goYell, 0, this );
			TFGameRules()->BroadcastSound( TEAM_SPECTATOR, goYell, 0, this ); // spectators hear the ready sounds, too

			m_flLastReadySoundTime = gpGlobals->curtime + 4.f;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::InputSpeakResponseConcept( inputdata_t &inputdata )
{
	const char *pInputString = STRING(inputdata.value.StringID());
	// if no params, early out
	if (!pInputString || *pInputString == 0)
	{
		Warning( "empty SpeakResponse input from %s to %s\n", inputdata.pCaller->GetDebugName(), GetDebugName() );
		return;
	}

	char buf[512] = {0}; // temporary for tokenizing
	char outputmodifiers[512] = {0}; // eventual output to speak
	int outWritten = 0;
	V_strncpy(buf, pInputString, 510);
	buf[511] = 0; // just in case the last character is a comma -- enforce that the 
	// last character in the buffer is always a terminator.
	// special syntax allowing designers to submit inputs with contexts like
	// "concept,context1:value1,context2:value2,context3:value3"
	// except that entity i/o seems to eat commas these days (didn't used to be the case)
	// so instead of commas we have to use spaces in the entity IO, 
	// and turn them into commas here. AWESOME.
	char *pModifiers = const_cast<char *>(V_strnchr(buf, ' ', 510));
	if ( pModifiers )
	{
		*pModifiers = 0;
		++pModifiers;

		// tokenize on spaces
		char *token = strtok(pModifiers, " ");
		while (token)
		{
			// find the start characters for the key and value
			// (seperated by a : which we replace with null)
			char * RESTRICT key = token;
			char * RESTRICT colon = const_cast<char *>(V_strnchr(key, ':', 510)); 
			char * RESTRICT value;
			if (!colon)
			{
				Warning( "faulty context k:v pair in entity io %s\n", pInputString );
				break;
			}

			// write the key and colon to the output string
			int toWrite = colon - key + 1;
			if ( outWritten + toWrite >= 512 )
			{
				Warning( "Speak input to %s had overlong parameter %s", GetDebugName(), pInputString );
				return;
			}
			memcpy(outputmodifiers + outWritten, key, toWrite);
			outWritten += toWrite;

			*colon = 0;
			value = colon + 1;

			// determine if the value is actually a procedural name
			CBaseEntity *pProcedural = gEntList.FindEntityProcedural( value, this, inputdata.pActivator, inputdata.pCaller );

			// write the value to the output -- if it's a procedural name, replace appropriately; 
			// if not, just copy over.
			const char *valString; 
			if (pProcedural)
			{
				valString = STRING(pProcedural->GetEntityName());
			}
			else
			{
				valString = value;
			}
			toWrite = strlen(valString);
			toWrite = MIN( 511-outWritten, toWrite );
			V_strncpy( outputmodifiers + outWritten, valString, toWrite+1 );
			outWritten += toWrite;

			// get the next token
			token = strtok(NULL, " ");
			if (token)
			{
				// if there is a next token, write in a comma
				if (outWritten < 511)
				{
					outputmodifiers[outWritten++]=',';
				}
			}
		}
	}

	// null terminate just in case
	outputmodifiers[outWritten <= 511 ? outWritten : 511] = 0;

	SpeakConceptIfAllowed( GetMPConceptIndexFromString( buf ), outputmodifiers[0] ? outputmodifiers : NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::CreateDisguiseWeaponList( CTFPlayer *pDisguiseTarget )
{
	ClearDisguiseWeaponList();

	// copy disguise target weapons
	if ( pDisguiseTarget )
	{
		for ( int i=0; i<TF_PLAYER_WEAPON_COUNT; ++i )
		{
			CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( pDisguiseTarget->GetWeapon( i ) );

			if ( !pWeapon )
				continue;

			int iSubType = 0;
			CTFWeaponBase *pCopyWeapon = dynamic_cast<CTFWeaponBase*>( GiveNamedItem( pWeapon->GetClassname(), iSubType, true ) );
			if ( pCopyWeapon )
			{
				pCopyWeapon->SetSolid( SOLID_NONE );
				pCopyWeapon->SetSolidFlags( FSOLID_NOT_SOLID );
				pCopyWeapon->AddEffects( EF_NODRAW | EF_NOSHADOW );
				m_hDisguiseWeaponList.AddToTail( pCopyWeapon );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ClearDisguiseWeaponList()
{
	FOR_EACH_VEC( m_hDisguiseWeaponList, i )
	{
		if ( m_hDisguiseWeaponList[i] )
		{
			m_hDisguiseWeaponList[i]->Drop( vec3_origin );
		}
	}

	m_hDisguiseWeaponList.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldForceTransmitsForTeam( int iTeam )
{ 
	return ( ( GetTeamNumber() == TEAM_SPECTATOR ) || 
			 ( ( GetTeamNumber() == iTeam ) && ( !IsAlive() ) ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldGetBonusPointsForExtinguishEvent( int userID )
{
	int iIndex = m_PlayersExtinguished.Find( userID );
	if ( iIndex != m_PlayersExtinguished.InvalidIndex() )
	{
		if ( ( gpGlobals->curtime - m_PlayersExtinguished[iIndex] ) < 20.f )
			return false;

		m_PlayersExtinguished[iIndex] = gpGlobals->curtime;
	}
	else
	{
		m_PlayersExtinguished.Insert( userID, gpGlobals->curtime );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::BCanCallVote()
{
	if ( GetTeamNumber() == TEAM_UNASSIGNED )
		return false;

	if ( GetTeamNumber() == TEAM_SPECTATOR )
		return sv_vote_allow_spectators.GetBool();

	if ( !m_bFirstSpawnAndCanCallVote )
		return false;

	return BaseClass::BCanCallVote();
}

CVoteController *CTFPlayer::GetTeamVoteController()
{
	switch ( GetTeamNumber() )
	{
		case TF_TEAM_RED:
			return g_voteControllerRed;
		
		case TF_TEAM_BLUE:
			return g_voteControllerBlu;

		default:
			return g_voteControllerGlobal;
	}
}

void	CTFPlayer::ScriptAddCond( int nCond )
{
	ScriptAddCondEx( nCond, -1.0f, nullptr );
}

void	CTFPlayer::ScriptAddCondEx( int nCond, float flDuration, HSCRIPT hProvider )
{
	ETFCond cond = TFCondIndexToEnum( nCond );
	if ( cond == TF_COND_INVALID )
		return;

	m_Shared.AddCond( cond, flDuration, ToEnt( hProvider ) );
}

void	CTFPlayer::ScriptRemoveCond( int nCond )
{
	ScriptRemoveCondEx( nCond, false );
}

void	CTFPlayer::ScriptRemoveCondEx( int nCond, bool bIgnoreDuration )
{
	ETFCond cond = TFCondIndexToEnum( nCond );
	if ( cond == TF_COND_INVALID )
		return;

	return m_Shared.RemoveCond( cond, bIgnoreDuration );
}

bool	CTFPlayer::ScriptInCond( int nCond )
{
	ETFCond cond = TFCondIndexToEnum( nCond );
	if ( cond == TF_COND_INVALID )
		return false;

	return m_Shared.InCond( cond );
}

bool	CTFPlayer::ScriptWasInCond( int nCond )
{
	ETFCond cond = TFCondIndexToEnum( nCond );
	if ( cond == TF_COND_INVALID )
		return false;

	return m_Shared.WasInCond( cond );
}

void	CTFPlayer::ScriptRemoveAllCond()
{
	m_Shared.RemoveAllCond();
}

float	CTFPlayer::ScriptGetCondDuration( int nCond )
{
	ETFCond cond = TFCondIndexToEnum( nCond );
	if ( cond == TF_COND_INVALID )
		return -1.0f;

	return m_Shared.GetConditionDuration( cond );
}

void	CTFPlayer::ScriptSetCondDuration( int nCond, float flNewDuration )
{
	ETFCond cond = TFCondIndexToEnum( nCond );
	if ( cond == TF_COND_INVALID )
		return;

	return m_Shared.SetConditionDuration( cond, flNewDuration );
}

HSCRIPT	CTFPlayer::ScriptGetDisguiseTarget()
{
	return ToHScript( m_Shared.GetDisguiseTarget() );
}

Vector CTFPlayer::ScriptWeapon_ShootPosition()
{
	return this->Weapon_ShootPosition();
}

bool CTFPlayer::ScriptWeapon_CanUse( HSCRIPT hWeapon )
{
	CBaseCombatWeapon *pCombatWeapon = ScriptToEntClass< CBaseCombatWeapon >( hWeapon );
	if ( !pCombatWeapon )
		return false;

	return this->Weapon_CanUse( pCombatWeapon );
}

void CTFPlayer::ScriptWeapon_Equip( HSCRIPT hWeapon )
{
	CBaseCombatWeapon *pCombatWeapon = ScriptToEntClass< CBaseCombatWeapon >( hWeapon );
	if ( !pCombatWeapon )
		return;

	this->Weapon_Equip( pCombatWeapon );
}

void CTFPlayer::ScriptWeapon_Drop( HSCRIPT hWeapon )
{
	CBaseCombatWeapon *pCombatWeapon = ScriptToEntClass< CBaseCombatWeapon >( hWeapon );
	if ( !pCombatWeapon )
		return;

	this->Weapon_Drop( pCombatWeapon, NULL, NULL );
}

void CTFPlayer::ScriptWeapon_DropEx( HSCRIPT hWeapon, Vector vecTarget, Vector vecVelocity )
{
	CBaseCombatWeapon *pCombatWeapon = ScriptToEntClass< CBaseCombatWeapon >( hWeapon );
	if ( !pCombatWeapon )
		return;

	this->Weapon_Drop( pCombatWeapon, &vecTarget, &vecVelocity );
}

void CTFPlayer::ScriptWeapon_Switch( HSCRIPT hWeapon )
{
	CBaseCombatWeapon *pCombatWeapon = ScriptToEntClass< CBaseCombatWeapon >( hWeapon );
	if ( !pCombatWeapon )
		return;

	this->Weapon_Switch( pCombatWeapon );
}

void CTFPlayer::ScriptWeapon_SetLast( HSCRIPT hWeapon )
{
	CBaseCombatWeapon *pCombatWeapon = ScriptToEntClass< CBaseCombatWeapon >( hWeapon );
	if ( !pCombatWeapon )
		return;

	this->Weapon_SetLast( pCombatWeapon );
}

HSCRIPT	CTFPlayer::ScriptGetLastWeapon()
{
	return ToHScript(this->GetLastWeapon() );
}
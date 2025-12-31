//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_gamerules.h"
#include "tf_player_shared.h"
#include "takedamageinfo.h"
#include "tf_weaponbase.h"
#include "effect_dispatch_data.h"
#include "tf_item.h"
#include "entity_capture_flag.h"
#include "tf_weapon_medigun.h"
#include "tf_weapon_pipebomblauncher.h"
#include "tf_weapon_invis.h"
#include "tf_weapon_sniperrifle.h"
#include "tf_weapon_shovel.h"
#include "tf_weapon_shotgun.h"
#include "in_buttons.h"
#include "tf_weapon_wrench.h"
#include "tf_weapon_knife.h"
#include "tf_weapon_syringegun.h"
#include "tf_weapon_flamethrower.h"
#include <functional>

// Client specific.
#ifdef CLIENT_DLL
#include "c_baseviewmodel.h"
#include "c_tf_player.h"
#include "c_te_effect_dispatch.h"
#include "c_tf_fx.h"
#include "soundenvelope.h"
#include "c_tf_playerclass.h"
#include "iviewrender.h"
#include "prediction.h"
#include "achievementmgr.h"
#include "baseachievement.h"
#include "achievements_tf.h"
#include "c_tf_weapon_builder.h"
#include "dt_utlvector_recv.h"
#include "recvproxy.h"
#include "c_tf_weapon_builder.h"
#include "c_func_capture_zone.h"
#include "tf_hud_target_id.h"
#include "tempent.h"
#include "cam_thirdperson.h"
#include "vgui/IInput.h"

#define CTFPlayerClass C_TFPlayerClass
#define CCaptureZone C_CaptureZone
#define CRecipientFilter C_RecipientFilter

#include "c_tf_objective_resource.h"

// Server specific.
#else
#include "tf_player.h"
#include "te_effect_dispatch.h"
#include "tf_fx.h"
#include "util.h"
#include "tf_team.h"
#include "tf_gamestats.h"
#include "tf_playerclass.h"
#include "SpriteTrail.h"
#include "tf_weapon_builder.h"
#include "nav_pathfind.h"
#include "tf_obj_dispenser.h"
#include "dt_utlvector_send.h"
#include "tf_item_wearable.h"
#include "tf_weapon_builder.h"
#include "func_capture_zone.h"
#include "hl2orange.spa.h"
#include "tf_objective_resource.h"
#include "entity_healthkit.h"
#include "tf_obj_sentrygun.h"
#include "func_respawnroom.h"
#endif

#include "tf_weapon_bonesaw.h"

static const float YAW_CAP_SCALE_MIN = 0.2f;
static const float YAW_CAP_SCALE_MAX = 2.f;

ConVar tf_scout_air_dash_count( "tf_scout_air_dash_count", "1", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

ConVar tf_spy_invis_time( "tf_spy_invis_time", "1.0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Transition time in and out of spy invisibility", true, 0.1, true, 5.0 );
ConVar tf_spy_invis_unstealth_time( "tf_spy_invis_unstealth_time", "2.0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Transition time in and out of spy invisibility", true, 0.1, true, 5.0 );

ConVar tf_spy_max_cloaked_speed( "tf_spy_max_cloaked_speed", "999", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED );	// no cap
ConVar tf_whip_speed_increase( "tf_whip_speed_increase", "105", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED );
ConVar tf_max_health_boost( "tf_max_health_boost", "1.5", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Max health factor that players can be boosted to by healers.", true, 1.0, false, 0 );
ConVar tf_invuln_time( "tf_invuln_time", "1.0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Time it takes for invulnerability to wear off." );

extern ConVar tf_player_movement_restart_freeze;
extern ConVar mp_tournament_readymode_countdown;

//ConVar tf_scout_dodge_move_penalty_duration( "tf_scout_dodge_move_penalty_duration", "3.0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED );
//ConVar tf_scout_dodge_move_penalty( "tf_scout_dodge_move_penalty", "0.5", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED );


#ifdef GAME_DLL
ConVar tf_boost_drain_time( "tf_boost_drain_time", "15.0", FCVAR_DEVELOPMENTONLY, "Time is takes for a full health boost to drain away from a player.", true, 0.1, false, 0 );

ConVar tf_damage_events_track_for( "tf_damage_events_track_for", "30",  FCVAR_DEVELOPMENTONLY );

ConVar tf_allow_sliding_taunt( "tf_allow_sliding_taunt", "0", FCVAR_NONE, "1 - Allow player to slide for a bit after taunting" );

#endif // GAME_DLL


ConVar tf_useparticletracers( "tf_useparticletracers", "1", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Use particle tracers instead of old style ones." );
ConVar tf_spy_cloak_consume_rate( "tf_spy_cloak_consume_rate", "10.0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "cloak to use per second while cloaked, from 100 max )" );	// 10 seconds of invis
ConVar tf_spy_cloak_regen_rate( "tf_spy_cloak_regen_rate", "3.3", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "cloak to regen per second, up to 100 max" );		// 30 seconds to full charge
ConVar tf_spy_cloak_no_attack_time( "tf_spy_cloak_no_attack_time", "2.0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "time after uncloaking that the spy is prohibited from attacking" );
ConVar tf_tournament_hide_domination_icons( "tf_tournament_hide_domination_icons", "0", FCVAR_REPLICATED, "Tournament mode server convar that forces clients to not display the domination icons above players dominating them." );
ConVar tf_damage_disablespread( "tf_damage_disablespread", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Toggles the random damage spread applied to all player damage." );

// STAGING_SPY
ConVar tf_feign_death_duration( "tf_feign_death_duration", "3.0", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Time that feign death buffs last." );
ConVar tf_feign_death_speed_duration( "tf_feign_death_speed_duration", "3.0", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Time that feign death speed boost last." );

ConVar tf_allow_taunt_switch( "tf_allow_taunt_switch", "0", FCVAR_REPLICATED, "0 - players are not allowed to switch weapons while taunting, 1 - players can switch weapons at the start of a taunt (old bug behavior), 2 - players can switch weapons at any time during a taunt." );

ConVar tf_allow_all_team_partner_taunt( "tf_allow_all_team_partner_taunt", "1", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );

// AFTERBURN
#ifdef DEBUG
ConVar tf_afterburn_debug( "tf_afterburn_debug", "0", FCVAR_REPLICATED | FCVAR_CHEAT );
#endif // DEBUG


#ifdef CLIENT_DLL
ConVar tf_colorblindassist( "tf_colorblindassist", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Setting this to 1 turns on colorblind mode." );

extern ConVar cam_idealdist;
extern ConVar cam_idealdistright;

#endif // CLIENT_DLL

extern ConVar tf_flamethrower_flametime;
extern ConVar weapon_medigun_chargerelease_rate;
#if defined( _DEBUG ) || defined( STAGING_ONLY )
extern ConVar mp_developer;
#endif // _DEBUG || STAGING_ONLY

//ConVar tf_spy_stealth_blink_time( "tf_spy_stealth_blink_time", "0.3", FCVAR_DEVELOPMENTONLY, "time after being hit the spy blinks into view" );
//ConVar tf_spy_stealth_blink_scale( "tf_spy_stealth_blink_scale", "0.85", FCVAR_DEVELOPMENTONLY, "percentage visible scalar after being hit the spy blinks into view" );
#define TF_SPY_STEALTH_BLINKTIME   0.3f
#define TF_SPY_STEALTH_BLINKSCALE  0.85f

#define TF_BUILDING_PICKUP_RANGE 150
#define TF_BUILDING_RESCUE_MIN_RANGE_SQ 62500  //250 * 250
#define TF_BUILDING_RESCUE_MAX_RANGE 5500

#define TF_PLAYER_CONDITION_CONTEXT	"TFPlayerConditionContext"

#define TF_SCREEN_OVERLAY_MATERIAL_BURNING		"effects/imcookin" 
#define TF_SCREEN_OVERLAY_MATERIAL_INVULN_RED	"effects/invuln_overlay_red" 
#define TF_SCREEN_OVERLAY_MATERIAL_INVULN_BLUE	"effects/invuln_overlay_blue" 

#define TF_SCREEN_OVERLAY_MATERIAL_MILK				"effects/milk_screen" 
#define TF_SCREEN_OVERLAY_MATERIAL_URINE			"effects/jarate_overlay" 
#define TF_SCREEN_OVERLAY_MATERIAL_BLEED			"effects/bleed_overlay" 
#define TF_SCREEN_OVERLAY_MATERIAL_STEALTH			"effects/stealth_overlay"
#define TF_SCREEN_OVERLAY_MATERIAL_SWIMMING_CURSE	"effects/jarate_overlay" 
#define TF_SCREEN_OVERLAY_MATERIAL_GAS				"effects/gas_overlay" 

#define TF_SCREEN_OVERLAY_MATERIAL_PHASE	"effects/dodge_overlay"

#define MAX_DAMAGE_EVENTS		128

const char *g_pszBDayGibs[22] = 
{
	"models/effects/bday_gib01.mdl",
	"models/effects/bday_gib02.mdl",
	"models/effects/bday_gib03.mdl",
	"models/effects/bday_gib04.mdl",
	"models/player/gibs/gibs_balloon.mdl",
	"models/player/gibs/gibs_burger.mdl",
	"models/player/gibs/gibs_boot.mdl",
	"models/player/gibs/gibs_bolt.mdl",
	"models/player/gibs/gibs_can.mdl",
	"models/player/gibs/gibs_clock.mdl",
	"models/player/gibs/gibs_fish.mdl",
	"models/player/gibs/gibs_gear1.mdl",
	"models/player/gibs/gibs_gear2.mdl",
	"models/player/gibs/gibs_gear3.mdl",
	"models/player/gibs/gibs_gear4.mdl",
	"models/player/gibs/gibs_gear5.mdl",
	"models/player/gibs/gibs_hubcap.mdl",
	"models/player/gibs/gibs_licenseplate.mdl",
	"models/player/gibs/gibs_spring1.mdl",
	"models/player/gibs/gibs_spring2.mdl",
	"models/player/gibs/gibs_teeth.mdl",
	"models/player/gibs/gibs_tire.mdl"
};

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void RecvProxy_BuildablesListChanged( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	RecvProxy_Int32ToInt32( pData, pStruct, pOut );

	C_TFPlayer* pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer || pLocalPlayer->entindex() != pData->m_ObjectID )
		return;

	int index = pData->m_pRecvProp->GetOffset() / sizeof(int);
	int object = pData->m_Value.m_Int;

	IGameEvent *event = gameeventmanager->CreateEvent( "update_status_item" );
	if ( event )
	{
		event->SetInt( "index", index );
		event->SetInt( "object", object );
		gameeventmanager->FireEventClientSide( event );
	}
}
#endif

//=============================================================================
//
// Tables.
//

// Client specific.

#ifdef CLIENT_DLL

BEGIN_RECV_TABLE_NOBASE( localplayerscoring_t, DT_TFPlayerScoringDataExclusive )
	RecvPropInt( RECVINFO( m_iCaptures ) ),
	RecvPropInt( RECVINFO( m_iDefenses ) ),
	RecvPropInt( RECVINFO( m_iKills ) ),
	RecvPropInt( RECVINFO( m_iDeaths ) ),
	RecvPropInt( RECVINFO( m_iSuicides ) ),
	RecvPropInt( RECVINFO( m_iDominations ) ),
	RecvPropInt( RECVINFO( m_iRevenge ) ),
	RecvPropInt( RECVINFO( m_iBuildingsBuilt ) ),
	RecvPropInt( RECVINFO( m_iBuildingsDestroyed ) ),
	RecvPropInt( RECVINFO( m_iHeadshots ) ),
	RecvPropInt( RECVINFO( m_iBackstabs ) ),
	RecvPropInt( RECVINFO( m_iHealPoints ) ),
	RecvPropInt( RECVINFO( m_iInvulns ) ),
	RecvPropInt( RECVINFO( m_iTeleports ) ),
	RecvPropInt( RECVINFO( m_iResupplyPoints ) ),
	RecvPropInt( RECVINFO( m_iKillAssists ) ),
	RecvPropInt( RECVINFO( m_iPoints ) ),
	RecvPropInt( RECVINFO( m_iBonusPoints ) ),
	RecvPropInt( RECVINFO( m_iDamageDone ) ),
	RecvPropInt( RECVINFO( m_iCrits ) ),
END_RECV_TABLE()

EXTERN_RECV_TABLE(DT_TFPlayerConditionListExclusive);

BEGIN_RECV_TABLE_NOBASE( CTFPlayerShared, DT_TFPlayerSharedLocal )
	RecvPropInt( RECVINFO( m_nDesiredDisguiseTeam ) ),
	RecvPropInt( RECVINFO( m_nDesiredDisguiseClass ) ),
	RecvPropTime( RECVINFO( m_flStealthNoAttackExpire ) ),
	RecvPropTime( RECVINFO( m_flStealthNextChangeTime ) ),
	RecvPropBool( RECVINFO( m_bLastDisguisedAsOwnTeam ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_bPlayerDominated ), RecvPropBool( RECVINFO( m_bPlayerDominated[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_bPlayerDominatingMe ), RecvPropBool( RECVINFO( m_bPlayerDominatingMe[0] ) ) ),
	RecvPropDataTable( RECVINFO_DT(m_ScoreData),0, &REFERENCE_RECV_TABLE(DT_TFPlayerScoringDataExclusive) ),
	RecvPropDataTable( RECVINFO_DT(m_RoundScoreData),0, &REFERENCE_RECV_TABLE(DT_TFPlayerScoringDataExclusive) ),
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE( condition_source_t, DT_TFPlayerConditionSource )
	//RecvPropInt( RECVINFO( m_nPreventedDamageFromCondition ) ),
	//RecvPropFloat( RECVINFO( m_flExpireTime ) ),
	RecvPropEHandle( RECVINFO( m_pProvider ) ),
	//RecvPropBool( RECVINFO( m_bPrevActive ) ),
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE( CTFPlayerShared, DT_TFPlayerShared )
	RecvPropInt( RECVINFO( m_nPlayerCond ) ),
	RecvPropInt( RECVINFO( m_bJumping) ),
	RecvPropInt( RECVINFO( m_nNumHealers ) ),
	RecvPropInt( RECVINFO( m_iCritMult ) ),
	RecvPropInt( RECVINFO( m_iAirDash ) ),
	RecvPropInt( RECVINFO( m_nAirDucked ) ),
	RecvPropFloat( RECVINFO( m_flDuckTimer ) ),
	RecvPropInt( RECVINFO( m_nPlayerState ) ),
	RecvPropInt( RECVINFO( m_iDesiredPlayerClass ) ),
	RecvPropInt( RECVINFO( m_nArenaNumChanges ) ),
	RecvPropInt( RECVINFO( m_iWeaponKnockbackID ) ),
	RecvPropInt( RECVINFO( m_iItemFindBonus ) ),
	RecvPropInt( RECVINFO( m_iDisguiseBody ) ),
	RecvPropInt( RECVINFO( m_iSpawnRoomTouchCount ) ),
	RecvPropFloat( RECVINFO( m_flFirstPrimaryAttack ) ),

	// Spy.
	RecvPropTime( RECVINFO( m_flInvisChangeCompleteTime ) ),
	RecvPropInt( RECVINFO( m_nDisguiseTeam ) ),
	RecvPropInt( RECVINFO( m_nDisguiseClass ) ),
	RecvPropInt( RECVINFO( m_nDisguiseSkinOverride ) ),
	RecvPropInt( RECVINFO( m_nMaskClass ) ),
	RecvPropEHandle( RECVINFO ( m_hDisguiseTarget ) ),
	RecvPropInt( RECVINFO( m_iDisguiseHealth ) ),
	RecvPropEHandle( RECVINFO( m_hDisguiseWeapon ) ),
	RecvPropInt( RECVINFO( m_nTeamTeleporterUsed ) ),
	RecvPropFloat( RECVINFO( m_flCloakMeter ) ),
	RecvPropFloat( RECVINFO( m_flSpyTranqBuffDuration ) ),

	// Local Data.
	RecvPropDataTable( "tfsharedlocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_TFPlayerSharedLocal) ),
	RecvPropDataTable( RECVINFO_DT(m_ConditionList),0, &REFERENCE_RECV_TABLE(DT_TFPlayerConditionListExclusive) ),

	RecvPropInt( RECVINFO( m_iTauntConcept ) ),

	RecvPropInt( RECVINFO( m_nPlayerCondEx ) ),

	RecvPropInt( RECVINFO( m_nPlayerCondEx2 ) ),
	RecvPropInt( RECVINFO( m_nPlayerCondEx3 ) ),

	RecvPropUtlVectorDataTable( m_ConditionData, TF_COND_LAST, DT_TFPlayerConditionSource ),

	RecvPropInt( RECVINFO( m_nPlayerCondEx4 ) ),

	RecvPropEHandle( RECVINFO( m_hSwitchTo ) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA_NO_BASE( CTFPlayerShared )
	DEFINE_PRED_FIELD( m_nPlayerState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nPlayerCond, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flCloakMeter, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bJumping, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iAirDash, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nAirDucked, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flDuckTimer, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flInvisChangeCompleteTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nDisguiseTeam, FIELD_INTEGER, FTYPEDESC_INSENDTABLE  ),
	DEFINE_PRED_FIELD( m_nDisguiseClass, FIELD_INTEGER, FTYPEDESC_INSENDTABLE  ),
	DEFINE_PRED_FIELD( m_nDisguiseSkinOverride, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nMaskClass, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nDesiredDisguiseTeam, FIELD_INTEGER, FTYPEDESC_INSENDTABLE  ),
	DEFINE_PRED_FIELD( m_nDesiredDisguiseClass, FIELD_INTEGER, FTYPEDESC_INSENDTABLE  ),
	DEFINE_PRED_FIELD( m_bLastDisguisedAsOwnTeam, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE  ),
	DEFINE_PRED_FIELD( m_nPlayerCondEx, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nPlayerCondEx2, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nPlayerCondEx3, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nPlayerCondEx4, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
//	DEFINE_PRED_FIELD( m_hDisguiseWeapon, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	DEFINE_FIELD( m_flDisguiseCompleteTime, FIELD_FLOAT ),
	DEFINE_PRED_FIELD( m_bScattergunJump, FIELD_BOOLEAN, 0 ),
END_PREDICTION_DATA()

// Server specific.
#else

BEGIN_SEND_TABLE_NOBASE( localplayerscoring_t, DT_TFPlayerScoringDataExclusive )
	SendPropInt( SENDINFO( m_iCaptures ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iDefenses ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iKills ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iDeaths ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iSuicides ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iDominations ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iRevenge ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iBuildingsBuilt ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iBuildingsDestroyed ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iHeadshots ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iBackstabs ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iHealPoints ), 20, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iInvulns ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iTeleports ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iDamageDone ), 20, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iCrits ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iResupplyPoints ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iKillAssists ), 12, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iBonusPoints ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iPoints ), 10, SPROP_UNSIGNED ),
END_SEND_TABLE()

EXTERN_SEND_TABLE(DT_TFPlayerConditionListExclusive);
BEGIN_SEND_TABLE_NOBASE( CTFPlayerShared, DT_TFPlayerSharedLocal )
	SendPropInt( SENDINFO( m_nDesiredDisguiseTeam ), 3, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nDesiredDisguiseClass ), 4, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bLastDisguisedAsOwnTeam ) ),
	SendPropTime( SENDINFO( m_flStealthNoAttackExpire ) ),
	SendPropTime( SENDINFO( m_flStealthNextChangeTime ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bPlayerDominated ), SendPropBool( SENDINFO_ARRAY( m_bPlayerDominated ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bPlayerDominatingMe ), SendPropBool( SENDINFO_ARRAY( m_bPlayerDominatingMe ) ) ),
	SendPropDataTable( SENDINFO_DT(m_ScoreData), &REFERENCE_SEND_TABLE(DT_TFPlayerScoringDataExclusive) ),
	SendPropDataTable( SENDINFO_DT(m_RoundScoreData), &REFERENCE_SEND_TABLE(DT_TFPlayerScoringDataExclusive) ),
END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE( condition_source_t, DT_TFPlayerConditionSource )
	//SendPropInt( SENDINFO( m_nPreventedDamageFromCondition ) ),
	//SendPropFloat( SENDINFO( m_flExpireTime ) ),
	SendPropEHandle( SENDINFO( m_pProvider ) ),
	//SendPropBool( SENDINFO( m_bPrevActive ) ),
END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE( CTFPlayerShared, DT_TFPlayerShared )
	SendPropInt( SENDINFO( m_nPlayerCond ), -1, SPROP_VARINT | SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_bJumping ), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nNumHealers ), 5, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iCritMult ), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iAirDash ), -1, SPROP_VARINT | SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nAirDucked ), 2, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_flDuckTimer )  ),
	SendPropInt( SENDINFO( m_nPlayerState ), Q_log2( TF_STATE_COUNT )+1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iDesiredPlayerClass ), Q_log2( TF_CLASS_COUNT_ALL )+1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nArenaNumChanges ), 5, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iWeaponKnockbackID ) ),
	SendPropInt( SENDINFO( m_iItemFindBonus ) ),
	SendPropInt( SENDINFO( m_iDisguiseBody ) ),
	SendPropInt( SENDINFO( m_iSpawnRoomTouchCount ) ),
	SendPropFloat( SENDINFO( m_flFirstPrimaryAttack ) ),

	// Spy
	SendPropTime( SENDINFO( m_flInvisChangeCompleteTime ) ),
	SendPropInt( SENDINFO( m_nDisguiseTeam ), 3, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nDisguiseClass ), 4, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nDisguiseSkinOverride ), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nMaskClass ), 4, SPROP_UNSIGNED ),
	SendPropEHandle( SENDINFO( m_hDisguiseTarget ) ),
	SendPropInt( SENDINFO( m_iDisguiseHealth ), -1, SPROP_VARINT ),
	SendPropEHandle( SENDINFO( m_hDisguiseWeapon ) ),
	SendPropInt( SENDINFO( m_nTeamTeleporterUsed ), 3, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_flCloakMeter ), 16, SPROP_NOSCALE, 0.0, 100.0 ),
	SendPropFloat( SENDINFO( m_flSpyTranqBuffDuration ), 16, SPROP_NOSCALE, 0.0, 100.0 ),
	
	// Local Data.
	SendPropDataTable( "tfsharedlocaldata", 0, &REFERENCE_SEND_TABLE( DT_TFPlayerSharedLocal ), SendProxy_SendLocalDataTable ),	
	SendPropDataTable( SENDINFO_DT(m_ConditionList), &REFERENCE_SEND_TABLE(DT_TFPlayerConditionListExclusive) ),

	SendPropInt( SENDINFO( m_iTauntConcept ), 8, SPROP_UNSIGNED ),

	SendPropInt( SENDINFO( m_nPlayerCondEx ), -1, SPROP_VARINT | SPROP_UNSIGNED ),
	
	SendPropInt( SENDINFO( m_nPlayerCondEx2 ), -1, SPROP_VARINT | SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nPlayerCondEx3 ), -1, SPROP_VARINT | SPROP_UNSIGNED ),

	SendPropUtlVectorDataTable( m_ConditionData, TF_COND_LAST, DT_TFPlayerConditionSource ),

	SendPropInt( SENDINFO( m_nPlayerCondEx4 ), -1, SPROP_VARINT | SPROP_UNSIGNED ),
	
	SendPropEHandle( SENDINFO( m_hSwitchTo ) ),
END_SEND_TABLE()

#endif

// --------------------------------------------------------------------------------------------------- //
// Shared CTFPlayer implementation.
// --------------------------------------------------------------------------------------------------- //

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::IsAllowedToTaunt( void )
{
	if ( !IsAlive() )
		return false;

	// Check to see if we can taunt again!
	if ( m_Shared.InCond( TF_COND_TAUNTING ) )
		return false;

	if ( IsLerpingFOV() )
		return false;

	// Check for things that prevent taunting
	if ( ShouldStopTaunting() )
		return false;

	// Check to see if we are on the ground.
	if ( GetGroundEntity() == NULL )
		return false;

	CTFWeaponBase *pActiveWeapon = m_Shared.GetActiveTFWeapon();
	if ( pActiveWeapon )
	{
		// ignore taunt key if one of these if active weapon
		if ( pActiveWeapon->GetWeaponID() == TF_WEAPON_PDA_ENGINEER_BUILD 
			 ||	pActiveWeapon->GetWeaponID() == TF_WEAPON_PDA_ENGINEER_DESTROY )
			return false;
	}

	if ( IsPlayerClass( TF_CLASS_SPY ) )
	{
		if ( m_Shared.IsStealthed() || m_Shared.InCond( TF_COND_STEALTHED_BLINK ) || 
			 m_Shared.InCond( TF_COND_DISGUISED ) || m_Shared.InCond( TF_COND_DISGUISING ) )
		{
			return false;
		}
	}

	return true;
}


// --------------------------------------------------------------------------------------------------- //
// CTFPlayerShared implementation.
// --------------------------------------------------------------------------------------------------- //

CTFPlayerShared::CTFPlayerShared()
{
	// If you hit this assert, CONGRATULATIONS!  You've added a condition that has gone 
	// beyond the amount of bits we network for conditions.  Take a look at the pattern
	// of m_nPlayerCond, m_nPlayerCondEx, m_nPlayerCondEx2, m_nPlayerCondEx3, and m_nPlayerCondEx4 to get more bits.
	// This pattern is as such to preserve replays.
	// Don't forget to add an m_nOldCond* and m_nForceCond*
	COMPILE_TIME_ASSERT( TF_COND_LAST < (32 + 32 + 32 + 32 + 32) );

	m_nPlayerState.Set( TF_STATE_WELCOME );
	m_bJumping = false;
	m_iAirDash = 0;
	m_nAirDucked = 0;
	m_flDuckTimer = 0.0f;
	m_flStealthNoAttackExpire = 0.0f;
	m_flStealthNextChangeTime = 0.0f;
	m_iCritMult = 0;
	m_flInvisibility = 0.0f;
	m_flPrevInvisibility = 0.f;

	m_bScattergunJump = false;

	m_fCloakConsumeRate = tf_spy_cloak_consume_rate.GetFloat();
	m_fCloakRegenRate = tf_spy_cloak_regen_rate.GetFloat();

	m_hAssist = NULL;

	m_bLastDisguisedAsOwnTeam = false;
		
	m_iWeaponKnockbackID = -1;

	m_nMaskClass = 0;

	m_iItemFindBonus = 0;

	m_nTeamTeleporterUsed = TEAM_UNASSIGNED;

	m_iOldKillStreak = 0;
	m_iOldKillStreakWepSlot = 0;

	m_iSpawnRoomTouchCount = 0;

	m_flFirstPrimaryAttack = 0.0f;

#ifdef GAME_DLL
	m_flBestOverhealDecayMult = -1;

	m_flHealedPerSecondTimer = -1000;
#endif

	m_nForceConditions = 0;
	m_nForceConditionsEx = 0;
	m_nForceConditionsEx2 = 0;
	m_nForceConditionsEx3 = 0;
	m_nForceConditionsEx4 = 0;

	m_flLastNoMovementTime = -1.f;

	m_hSwitchTo = NULL;

	// make sure we have all conditions in the list
	m_ConditionData.EnsureCount( TF_COND_LAST );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::Init( CTFPlayer *pPlayer )
{
	m_pOuter = pPlayer;

	m_flNextBurningSound = 0;

	m_iWeaponKnockbackID = -1;

	m_iOldKillStreak = 0;
	m_iOldKillStreakWepSlot = 0;

	SetJumping( false );
	SetAssist( NULL );

	m_flInvulnerabilityRemoveTime = -1;

	Spawn();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::Spawn( void )
{
#ifdef GAME_DLL
	m_iSpawnRoomTouchCount = 0;
	
#else
	m_bSyncingConditions = false;
#endif

	// Reset our assist here incase something happens before we get killed
	// again that checks this (getting slapped with a fish)
	SetAssist( NULL );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template < typename tIntType >
class CConditionVars
{
public:
	CConditionVars( tIntType& nPlayerCond, tIntType& nPlayerCondEx, tIntType& nPlayerCondEx2, tIntType& nPlayerCondEx3, tIntType& nPlayerCondEx4, ETFCond eCond )
	{
		if ( eCond >= 128 )
		{
			Assert( eCond < 128 + 32 );
			m_pnCondVar = &nPlayerCondEx4;
			m_nCondBit = eCond - 128; 
		}
		else if ( eCond >= 96 )
		{
			Assert( eCond < 96 + 32 );
			m_pnCondVar = &nPlayerCondEx3;
			m_nCondBit = eCond - 96;
		}
		else if( eCond >= 64 )
		{
			Assert( eCond < (64 + 32) );
			m_pnCondVar = &nPlayerCondEx2;
			m_nCondBit = eCond - 64;
		}
		else if ( eCond >= 32 )
		{
			Assert( eCond < (32 + 32) );
			m_pnCondVar = &nPlayerCondEx;
			m_nCondBit = eCond - 32;
		}
		else
		{
			m_pnCondVar = &nPlayerCond;
			m_nCondBit = eCond;
		}
	}

	tIntType& CondVar() const
	{
		return *m_pnCondVar;
	}

	int CondBit() const
	{
		return 1 << m_nCondBit;
	}

private:
	tIntType *m_pnCondVar;
	int m_nCondBit;
};

//-----------------------------------------------------------------------------
// Purpose: Add a condition and duration
// duration of PERMANENT_CONDITION means infinite duration
//-----------------------------------------------------------------------------
void CTFPlayerShared::AddCond( ETFCond eCond, float flDuration /* = PERMANENT_CONDITION */, CBaseEntity *pProvider /*= NULL */)
{
	Assert( eCond >= 0 && eCond < TF_COND_LAST );
	Assert( eCond < m_ConditionData.Count() );

	// If we're dead, don't take on any new conditions
	if( !m_pOuter || !m_pOuter->IsAlive() )
	{
		return;
	}

#ifdef CLEINT_DLL
	if ( m_pOuter->IsDormant() )
	{
		return;
	}
#endif

	// Which bitfield are we tracking this condition variable in? Which bit within
	// that variable will we track it as?
	CConditionVars<int> cPlayerCond( m_nPlayerCond.m_Value, m_nPlayerCondEx.m_Value, m_nPlayerCondEx2.m_Value, m_nPlayerCondEx3.m_Value, m_nPlayerCondEx4.m_Value, eCond );

	// See if there is an object representation of the condition.
	bool bAddedToExternalConditionList = m_ConditionList.Add( eCond, flDuration, m_pOuter, pProvider );
	if ( !bAddedToExternalConditionList )
	{
		// Set the condition bit for this condition.
		cPlayerCond.CondVar() |= cPlayerCond.CondBit();

		// Flag for gamecode to query
		m_ConditionData[eCond].m_bPrevActive = ( m_ConditionData[eCond].m_flExpireTime != 0.f ) ? true : false;

		if ( flDuration != PERMANENT_CONDITION )
		{
			// if our current condition is permanent or we're trying to set a new
			// time that's less our current time remaining, use our current time instead
			if ( ( m_ConditionData[eCond].m_flExpireTime == PERMANENT_CONDITION ) || 
				 ( flDuration < m_ConditionData[eCond].m_flExpireTime ) )
			{
				flDuration = m_ConditionData[eCond].m_flExpireTime;
			}
		}

		m_ConditionData[eCond].m_flExpireTime = flDuration;
		m_ConditionData[eCond].m_pProvider = pProvider;
		m_ConditionData[eCond].m_nPreventedDamageFromCondition = 0;

		OnConditionAdded( eCond );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Forcibly remove a condition
//-----------------------------------------------------------------------------
void CTFPlayerShared::RemoveCond( ETFCond eCond, bool ignore_duration )
{
	Assert( eCond >= 0 && eCond < TF_COND_LAST );
	Assert( eCond < m_ConditionData.Count() );

	if ( !InCond( eCond ) )
		return;

	CConditionVars<int> cPlayerCond( m_nPlayerCond.m_Value, m_nPlayerCondEx.m_Value, m_nPlayerCondEx2.m_Value, m_nPlayerCondEx3.m_Value, m_nPlayerCondEx4.m_Value, eCond );

	// If this variable is handled by the condition list, abort before doing the
	// work for the condition flags.
	if ( m_ConditionList.Remove( eCond, ignore_duration ) )
		return;

	cPlayerCond.CondVar() &= ~cPlayerCond.CondBit();
	OnConditionRemoved( eCond );

	if ( m_ConditionData[ eCond ].m_nPreventedDamageFromCondition )
	{
		IGameEvent *pEvent = gameeventmanager->CreateEvent( "damage_prevented" );
		if ( pEvent )
		{
			pEvent->SetInt( "preventor", m_ConditionData[eCond].m_pProvider ? m_ConditionData[eCond].m_pProvider->entindex() : m_pOuter->entindex() );
			pEvent->SetInt( "victim", m_pOuter->entindex() );
			pEvent->SetInt( "amount", m_ConditionData[ eCond ].m_nPreventedDamageFromCondition );
			pEvent->SetInt( "condition", eCond );

			gameeventmanager->FireEvent( pEvent, true );
		}

		m_ConditionData[ eCond ].m_nPreventedDamageFromCondition = 0;
	}

	m_ConditionData[eCond].m_flExpireTime = 0;
	m_ConditionData[eCond].m_pProvider = NULL;
	m_ConditionData[eCond].m_bPrevActive = false;

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayerShared::InCond( ETFCond eCond ) const
{
	Assert( eCond >= 0 && eCond < TF_COND_LAST );

	// Old condition system, only used for the first 32 conditions
	if ( eCond < 32 && m_ConditionList.InCond( eCond ) )
		return true;

	CConditionVars<const int> cPlayerCond( m_nPlayerCond.m_Value, m_nPlayerCondEx.m_Value, m_nPlayerCondEx2.m_Value, m_nPlayerCondEx3.m_Value, m_nPlayerCondEx4.m_Value, eCond );
	return (cPlayerCond.CondVar() & cPlayerCond.CondBit()) != 0;
}

//-----------------------------------------------------------------------------
// Purpose: Return whether or not we were in this condition before.
//-----------------------------------------------------------------------------
bool CTFPlayerShared::WasInCond( ETFCond eCond ) const
{
	// I don't know if this actually works for conditions < 32, because we definitely cannot peak into m_ConditionList (back in time).
	// But others think that m_ConditionList is propogated into m_nOldConditions, so just check if you hit the assert. (And then remove the 
	// assert. And this comment).
	Assert( eCond >= 32 && eCond < TF_COND_LAST );

	CConditionVars<const int> cPlayerCond( m_nOldConditions, m_nOldConditionsEx, m_nOldConditionsEx2, m_nOldConditionsEx3, m_nOldConditionsEx4, eCond );
	return (cPlayerCond.CondVar() & cPlayerCond.CondBit()) != 0;
}

//-----------------------------------------------------------------------------
// Purpose: Set a bit to force this condition off and then back on next time we sync bits from the server. 
//-----------------------------------------------------------------------------
void CTFPlayerShared::ForceRecondNextSync( ETFCond eCond )
{
	// I don't know if this actually works for conditions < 32. We may need to set this bit in m_ConditionList, too.
	// Please check if you hit the assert. (And then remove the assert. And this comment).
	Assert(eCond >= 32 && eCond < TF_COND_LAST);

	CConditionVars<int> playerCond( m_nForceConditions, m_nForceConditionsEx, m_nForceConditionsEx2, m_nForceConditionsEx3, m_nForceConditionsEx4, eCond );
	playerCond.CondVar() |= playerCond.CondBit();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFPlayerShared::GetConditionDuration( ETFCond eCond ) const
{
	Assert( eCond >= 0 && eCond < TF_COND_LAST );
	Assert( eCond < m_ConditionData.Count() );

	if ( InCond( eCond ) )
	{
		return m_ConditionData[eCond].m_flExpireTime;
	}
	
	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the entity that provided the passed in condition
//-----------------------------------------------------------------------------
CBaseEntity *CTFPlayerShared::GetConditionProvider( ETFCond eCond ) const
{
	Assert( eCond >= 0 && eCond < TF_COND_LAST );
	Assert( eCond < m_ConditionData.Count() );

	CBaseEntity *pProvider = NULL;
	if ( InCond( eCond ) )
	{
		pProvider = m_ConditionData[eCond].m_pProvider;
	}

	return pProvider;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::DebugPrintConditions( void )
{
#ifndef CLIENT_DLL
	const char *szDll = "Server";
#else
	const char *szDll = "Client";
#endif

	Msg( "( %s ) Conditions for player ( %d )\n", szDll, m_pOuter->entindex() );

	int i;
	int iNumFound = 0;
	for ( i=0;i<TF_COND_LAST;i++ )
	{
		if ( InCond( (ETFCond)i ) )
		{
			if ( m_ConditionData[i].m_flExpireTime == PERMANENT_CONDITION )
			{
				Msg( "( %s ) Condition %d - ( permanent cond )\n", szDll, i );
			}
			else
			{
				Msg( "( %s ) Condition %d - ( %.1f left )\n", szDll, i, m_ConditionData[i].m_flExpireTime );
			}

			iNumFound++;
		}
	}

	if ( iNumFound == 0 )
	{
		Msg( "( %s ) No active conditions\n", szDll );
	}
}

void CTFPlayerShared::InstantlySniperUnzoom( void )
{
	// Unzoom if we are a sniper zoomed!
	if ( m_pOuter->GetPlayerClass()->GetClassIndex() == TF_CLASS_SNIPER )
	{
		CTFWeaponBase *pWpn = m_pOuter->GetActiveTFWeapon();

		if ( pWpn && WeaponID_IsSniperRifle( pWpn->GetWeaponID() ) )
		{
			CTFSniperRifle *pRifle = static_cast<CTFSniperRifle*>( pWpn );
			if ( pRifle->IsZoomed() )
			{
				// Let the rifle clean up conditions and state
				pRifle->ToggleZoom();
				// Slam the FOV right now
				m_pOuter->SetFOV( m_pOuter, 0, 0.0f );
			}
		}
	}
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnPreDataChanged( void )
{
	m_ConditionList.OnPreDataChanged();

	m_nOldConditions = m_nPlayerCond;
	m_nOldConditionsEx = m_nPlayerCondEx;
	m_nOldConditionsEx2 = m_nPlayerCondEx2;
	m_nOldConditionsEx3 = m_nPlayerCondEx3;
	m_nOldConditionsEx4 = m_nPlayerCondEx4;
	m_nOldDisguiseClass = GetDisguiseClass();
	m_nOldDisguiseTeam = GetDisguiseTeam();

	SharedThink();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnDataChanged( void )
{
	m_ConditionList.OnDataChanged( m_pOuter );

	// Update conditions from last network change
	SyncConditions( m_nOldConditions, m_nPlayerCond, m_nForceConditions, 0 );
	SyncConditions( m_nOldConditionsEx, m_nPlayerCondEx, m_nForceConditionsEx, 32 );
	SyncConditions( m_nOldConditionsEx2, m_nPlayerCondEx2, m_nForceConditionsEx2, 64 );
	SyncConditions( m_nOldConditionsEx3, m_nPlayerCondEx3, m_nForceConditionsEx3, 96 );
	SyncConditions( m_nOldConditionsEx4, m_nPlayerCondEx4, m_nForceConditionsEx4, 128 );

	// Make sure these items are present
	m_nPlayerCond		|= m_nForceConditions;
	m_nPlayerCondEx		|= m_nForceConditionsEx;
	m_nPlayerCondEx2	|= m_nForceConditionsEx2;
	m_nPlayerCondEx3	|= m_nForceConditionsEx3;
	m_nPlayerCondEx4	|= m_nForceConditionsEx4;

	// Clear our force bits now that we've used them.
	m_nForceConditions = 0;
	m_nForceConditionsEx = 0;
	m_nForceConditionsEx2 = 0;
	m_nForceConditionsEx3 = 0;
	m_nForceConditionsEx4 = 0;

	if ( m_nOldDisguiseClass != GetDisguiseClass() || m_nOldDisguiseTeam != GetDisguiseTeam() )
	{
		OnDisguiseChanged();
	}

	if ( m_hDisguiseWeapon )
	{
		m_hDisguiseWeapon->UpdateVisibility();
	}

	InvisibilityThink();
}

//-----------------------------------------------------------------------------
// Purpose: check the newly networked conditions for changes
//-----------------------------------------------------------------------------
void CTFPlayerShared::SyncConditions( int nPreviousConditions, int nNewConditions, int nForceConditions, int nBaseCondBit )
{
	if ( nPreviousConditions == nNewConditions )
		return;

	int nCondChanged = nNewConditions ^ nPreviousConditions;
	int nCondAdded = nCondChanged & nNewConditions;
	int nCondRemoved = nCondChanged & nPreviousConditions;
	m_bSyncingConditions = true;

	for ( int i=0;i<32;i++ )
	{
		const int testBit = 1<<i;
		if ( nForceConditions & testBit )
		{
			if ( nPreviousConditions & testBit )
			{
				OnConditionRemoved((ETFCond)(nBaseCondBit + i));
			}
			OnConditionAdded((ETFCond)(nBaseCondBit + i));
		}
		else
		{
			if ( nCondAdded & testBit )
			{
				OnConditionAdded( (ETFCond)(nBaseCondBit + i) );
			}
			else if ( nCondRemoved & testBit )
			{
				OnConditionRemoved( (ETFCond)(nBaseCondBit + i) );
			}
		}
	}
	m_bSyncingConditions = false;
}

#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Remove any conditions affecting players
//-----------------------------------------------------------------------------
void CTFPlayerShared::RemoveAllCond()
{
	m_ConditionList.RemoveAll();

	int i;
	for ( i=0;i<TF_COND_LAST;i++ )
	{
		if ( InCond( (ETFCond)i ) )
		{
			RemoveCond( (ETFCond)i );
		}
	}

	// Now remove all the rest
	m_nPlayerCond = 0;
	m_nPlayerCondEx = 0;
	m_nPlayerCondEx2 = 0;
	m_nPlayerCondEx3 = 0;
	m_nPlayerCondEx4 = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Called on both client and server. Server when we add the bit,
// and client when it receives the new cond bits and finds one added
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnConditionAdded( ETFCond eCond )
{
	switch( eCond )
	{
	case TF_COND_ZOOMED:
		OnAddZoomed();
		break;
	case TF_COND_HEALTH_BUFF:
#ifdef GAME_DLL
		m_flHealFraction = 0;
		m_flDisguiseHealFraction = 0;

		m_flHealedPerSecondTimer = gpGlobals->curtime + 1.0f;
#endif
		break;
	
	case TF_COND_HEALTH_OVERHEALED:
		OnAddOverhealed();
		break;

	case TF_COND_STEALTHED:
		OnAddStealthed();
		break;

	case TF_COND_INVULNERABLE:
		OnAddInvulnerable();
		break;

	case TF_COND_TELEPORTED:
		OnAddTeleported();
		break;

	case TF_COND_BURNING:
		OnAddBurning();
		break;

	case TF_COND_DISGUISING:
		OnAddDisguising();
		break;

	case TF_COND_DISGUISED:
		OnAddDisguised();
		break;

	case TF_COND_TAUNTING:
		OnAddTaunting();
		break;

	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called on both client and server. Server when we remove the bit,
// and client when it receives the new cond bits and finds one removed
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnConditionRemoved( ETFCond eCond )
{
	switch( eCond )
	{
	case TF_COND_ZOOMED:
		OnRemoveZoomed();
		break;

	case TF_COND_BURNING:
		OnRemoveBurning();
		break;

	case TF_COND_HEALTH_BUFF:
#ifdef GAME_DLL
		m_flHealFraction = 0;
		m_flDisguiseHealFraction = 0;
#endif
		break;

	case TF_COND_HEALTH_OVERHEALED:
		OnRemoveOverhealed();
		break;

	case TF_COND_STEALTHED:
		OnRemoveStealthed();
		break;

	case TF_COND_DISGUISED:
		OnRemoveDisguised();
		break;

	case TF_COND_DISGUISING:
		OnRemoveDisguising();
		break;

	case TF_COND_INVULNERABLE:
		OnRemoveInvulnerable();
		break;

	case TF_COND_TELEPORTED:
		OnRemoveTeleported();
		break;

	case TF_COND_INVULNERABLE_WEARINGOFF:
		OnRemoveInvulnerableWearingOff();
		break;

	case TF_COND_TAUNTING:
		OnRemoveTaunting();
		break;

	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: The highest possible overheal this player is able to receive
//-----------------------------------------------------------------------------
int CTFPlayerShared::GetMaxBuffedHealth( bool bIgnoreAttributes /*= false*/, bool bIgnoreHealthOverMax /*= false*/ )
{
	int nMaxHealthForBuffing = m_pOuter->GetMaxHealthForBuffing();
	float flBoostMax = nMaxHealthForBuffing * tf_max_health_boost.GetFloat();

#ifdef GAME_DLL
	// Look for any attributes that might change the answer
	if ( !bIgnoreAttributes )
	{
		// MvM can boost medics past the default max
		int nMaxMedicOverheal = nMaxHealthForBuffing * GetMaxOverhealMultiplier();
		if ( nMaxMedicOverheal > flBoostMax )
		{
			flBoostMax = nMaxMedicOverheal;
		}
	}
#endif

	int iRoundDown = floor( flBoostMax / 5 );
	iRoundDown = iRoundDown * 5;

	if ( !bIgnoreHealthOverMax )
	{
		// Don't allow overheal total to be less than the buffable + unbuffable max health or the current health
		int nBoostMin = Max( m_pOuter->GetMaxHealth(), m_pOuter->GetHealth() );
		if ( iRoundDown < nBoostMin )
		{
			iRoundDown = nBoostMin;
		}
	}

	return iRoundDown;
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: Based on active medics
//-----------------------------------------------------------------------------
float CTFPlayerShared::GetMaxOverhealMultiplier( void )
{
	float flMax = 1.f;

	// Find out if any healer is providing more than the default max overheal
	for ( int i = 0; i < m_aHealers.Count(); i++ )
	{
		CTFPlayer *pTFMedic = ToTFPlayer( m_aHealers[i].pHealer );
		bool bHoldingMedigun = ( ( pTFMedic && pTFMedic->GetActiveTFWeapon() && pTFMedic->GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_MEDIGUN ) );
		float flOverHealBonus = ( ( bHoldingMedigun ) ? tf_max_health_boost.GetFloat() : m_aHealers[i].flOverhealBonus );
		if ( flOverHealBonus > flMax )
		{
			flMax = flOverHealBonus;
		}
	}

	return flMax;
}
#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayerShared::GetDisguiseMaxBuffedHealth( bool bIgnoreAttributes /*= false*/, bool bIgnoreHealthOverMax /*= false*/ )
{
	// Find the healer we have who's providing the most overheal
	float flBoostMax = GetDisguiseMaxHealth() * tf_max_health_boost.GetFloat();
#ifdef GAME_DLL
	if ( !bIgnoreAttributes )
	{
		for ( int i = 0; i < m_aHealers.Count(); i++ )
		{
			float flOverheal = GetDisguiseMaxHealth() * m_aHealers[i].flOverhealBonus;
			if ( flOverheal > flBoostMax )
			{
				flBoostMax = flOverheal;
			}
		}
	}
#endif

	int iRoundDown = floor( flBoostMax / 5 );
	iRoundDown = iRoundDown * 5;

	if ( !bIgnoreHealthOverMax )
	{
		// Don't allow overheal total to be less than the buffable + unbuffable max health or the current health
		int nBoostMin = MAX(GetDisguiseMaxHealth(), GetDisguiseHealth() );
		if ( iRoundDown < nBoostMin )
		{
			iRoundDown = nBoostMin;
		}
	}

	return iRoundDown;
}

//-----------------------------------------------------------------------------
// Purpose: Runs SERVER SIDE only Condition Think
// If a player needs something to be updated no matter what do it here (invul, etc).
//-----------------------------------------------------------------------------
void CTFPlayerShared::ConditionGameRulesThink( void )
{
#ifdef GAME_DLL

	m_ConditionList.ServerThink();

	if ( m_flNextCritUpdate < gpGlobals->curtime )
	{
		UpdateCritMult();
		m_flNextCritUpdate = gpGlobals->curtime + 0.5;
	}

	for ( int i=0; i < TF_COND_LAST; ++i )
	{
		// if we're in this condition and it's not already being handled by the condition list
		if ( InCond( (ETFCond)i ) && ((i >= 32) || !m_ConditionList.InCond( (ETFCond)i )) )
		{
			// Ignore permanent conditions
			if ( m_ConditionData[i].m_flExpireTime != PERMANENT_CONDITION )
			{
				float flReduction = gpGlobals->frametime;

				// If we're being healed, we reduce bad conditions faster
				if ( ConditionExpiresFast( (ETFCond)i) && m_aHealers.Count() > 0 )
				{
					flReduction += (m_aHealers.Count() * flReduction * 4);
				}

				m_ConditionData[i].m_flExpireTime = MAX( m_ConditionData[i].m_flExpireTime - flReduction, 0 );

				if ( m_ConditionData[i].m_flExpireTime == 0 )
				{
					RemoveCond( (ETFCond)i );
				}
			}
		}
	}

	// Our health will only decay ( from being medic buffed ) if we are not being healed by a medic
	// Dispensers can give us the TF_COND_HEALTH_BUFF, but will not maintain or give us health above 100%s
	bool bDecayHealth = true;
	bool bDecayDisguiseHealth = true;

	// If we're being healed, heal ourselves
	if ( InCond( TF_COND_HEALTH_BUFF ) )
	{
		// Heal faster if we haven't been in combat for a while
		float flTimeSinceDamage = gpGlobals->curtime - m_pOuter->GetLastDamageReceivedTime();
		float flScale = RemapValClamped( flTimeSinceDamage, 10.f, 15.f, 1.f, 3.f );

		float flCurOverheal = (float)m_pOuter->GetHealth() / (float)m_pOuter->GetMaxHealth();
		if ( flCurOverheal > 1.0f )
		{
			// If they're over their max health the overheal calculation is relative to the max buffable amount scale
			float flMaxHealthForBuffing = m_pOuter->GetMaxHealthForBuffing();
			float flBuffableRangeHealth = m_pOuter->GetHealth() - ( m_pOuter->GetMaxHealth() - flMaxHealthForBuffing );
			flCurOverheal = flBuffableRangeHealth / flMaxHealthForBuffing;
		}

		float flCurDisguiseOverheal = ( GetDisguiseMaxHealth() != 0 ) ? ( (float)GetDisguiseHealth() / (float)GetDisguiseMaxHealth() ) : ( flCurOverheal );

		float fTotalHealAmount = 0.0f;
		for ( int i = 0; i < m_aHealers.Count(); i++ )
		{
			Assert( m_aHealers[i].pHealer );

			bool bHealDisguise = InCond( TF_COND_DISGUISED );
			bool bHealActual = true;

			// Don't heal over the healer's overheal bonus
			if ( flCurOverheal >= m_aHealers[i].flOverhealBonus )
			{
				bHealActual = false;
			}

			// Same overheal check, but for fake health
			if ( InCond( TF_COND_DISGUISED ) && flCurDisguiseOverheal >= m_aHealers[i].flOverhealBonus )
			{
				// Fake over-heal
				bHealDisguise = false;
			}

			if ( !bHealActual && !bHealDisguise )
			{
				continue;
			}

			// Being healed by a medigun, don't decay our health
			if ( bHealActual )
			{
				bDecayHealth = false;
			}

			if ( bHealDisguise )
			{
				bDecayDisguiseHealth = false;
			}

			// Dispensers heal at a constant rate
			if ( m_aHealers[i].bDispenserHeal )
			{
				// Dispensers heal at a slower rate, but ignore flScale
				if ( bHealActual )
				{
					float flDispenserFraction = gpGlobals->frametime * m_aHealers[i].flAmount;
					m_flHealFraction += flDispenserFraction;

					// track how much this healer has actually done so far
					m_aHealers[i].flHealAccum += clamp( flDispenserFraction, 0.f, (float) GetMaxBuffedHealth() - m_pOuter->GetHealth() );
				}
				if ( bHealDisguise )
				{
					m_flDisguiseHealFraction += gpGlobals->frametime * m_aHealers[i].flAmount;
				}
			}
			else	// player heals are affected by the last damage time
			{
				if ( bHealActual )
				{
					// Scale this if needed
					m_flHealFraction += gpGlobals->frametime * m_aHealers[i].flAmount * flScale;
				}
				if ( bHealDisguise )
				{
					m_flDisguiseHealFraction += gpGlobals->frametime * m_aHealers[i].flAmount * flScale;
				}
			}

			fTotalHealAmount += m_aHealers[i].flAmount;

			// Keep our decay multiplier uptodate
			if ( m_flBestOverhealDecayMult == -1 || m_aHealers[i].flOverhealDecayMult < m_flBestOverhealDecayMult )
			{
				m_flBestOverhealDecayMult = m_aHealers[i].flOverhealDecayMult;
			}
		}

		int nHealthToAdd = (int)m_flHealFraction;
		int nDisguiseHealthToAdd = (int)m_flDisguiseHealFraction;
		if ( nHealthToAdd > 0 || nDisguiseHealthToAdd > 0 )
		{
			if ( nHealthToAdd > 0 )
			{
				m_flHealFraction -= nHealthToAdd;
			}

			if ( nDisguiseHealthToAdd > 0 )
			{
				m_flDisguiseHealFraction -= nDisguiseHealthToAdd;
			}

			int iBoostMax = GetMaxBuffedHealth();

			if ( InCond( TF_COND_DISGUISED ) )
			{
				// Separate cap for disguised health
				int nFakeHealthToAdd = clamp( nDisguiseHealthToAdd, 0, GetDisguiseMaxBuffedHealth() - m_iDisguiseHealth );
				m_iDisguiseHealth += nFakeHealthToAdd;
			}

			// Track health prior to healing
			int nPrevHealth = m_pOuter->GetHealth();
			
			// Cap it to the max we'll boost a player's health
			nHealthToAdd = clamp( nHealthToAdd, 0, iBoostMax - m_pOuter->GetHealth() );
			
			m_pOuter->TakeHealth( nHealthToAdd, DMG_IGNORE_MAXHEALTH | DMG_IGNORE_DEBUFFS );
			
			m_pOuter->AdjustDrownDmg( -1.0 * nHealthToAdd ); // subtract this from the drowndmg in case they're drowning and being healed at the same time

			// split up total healing based on the amount each healer contributes
			if ( fTotalHealAmount > 0 )
			{
				for ( int i = 0; i < m_aHealers.Count(); i++ )
				{
					Assert( m_aHealers[i].pHealScorer );
					Assert( m_aHealers[i].pHealer );
					if ( m_aHealers[i].pHealScorer.IsValid() && m_aHealers[i].pHealer.IsValid() )
					{
						CBaseEntity *pHealer = m_aHealers[i].pHealer;
						float flHealAmount = nHealthToAdd * ( m_aHealers[i].flAmount / fTotalHealAmount );

						if ( pHealer && IsAlly( pHealer ) )
						{
							CTFPlayer *pHealScorer = ToTFPlayer( m_aHealers[i].pHealScorer );
							if ( pHealScorer )
							{	
								// Don't report healing when we're close to the buff cap and haven't taken damage recently.
								// This avoids sending bogus heal stats while maintaining our max overheal.  Ideally we
								// wouldn't decay in this scenario, but that would be a risky change.
								if ( iBoostMax - nPrevHealth > 1 || gpGlobals->curtime - m_pOuter->GetLastDamageReceivedTime() <= 1.f )
								{
									CTF_GameStats.Event_PlayerHealedOther( pHealScorer, flHealAmount );
								}

								// Add this to the one-second-healing counter
								m_aHealers[i].flHealedLastSecond += flHealAmount;

								// If it's been one second, or we know healing beyond this point will be overheal, generate an event
								if ( ( m_flHealedPerSecondTimer <= gpGlobals->curtime || m_pOuter->GetHealth() >= m_pOuter->GetMaxHealth() ) 
									   && m_aHealers[i].flHealedLastSecond > 1 )
								{
									// Make sure this isn't pure overheal
									if ( m_pOuter->GetHealth() - m_aHealers[i].flHealedLastSecond < m_pOuter->GetMaxHealth() )
									{
										float flOverHeal = m_pOuter->GetHealth() - m_pOuter->GetMaxHealth();
										if ( flOverHeal > 0 )
										{
											m_aHealers[i].flHealedLastSecond -= flOverHeal;
										}

										// TEST THIS
										// Give the medic some uber if it is from their (AoE heal) which has no overheal
										if ( m_aHealers[i].flOverhealBonus <= 1.0f )
										{
											// Give a litte bit of uber based on actual healing
											// Give them a little bit of Uber
											CWeaponMedigun *pMedigun = static_cast<CWeaponMedigun *>( pHealScorer->Weapon_OwnsThisID( TF_WEAPON_MEDIGUN ) );
											if ( pMedigun )
											{
												// On Mediguns, per frame, the amount of uber added is based on 
												// Default heal rate is 24per second, we scale based on that and frametime
												pMedigun->AddCharge( ( m_aHealers[i].flHealedLastSecond / 24.0f ) * gpGlobals->frametime * 0.33f );
											}
										}

										IGameEvent * event = gameeventmanager->CreateEvent( "player_healed" );
										if ( event )
										{
											// HLTV event priority, not transmitted
											event->SetInt( "priority", 1 );	

											// Healed by another player.
											event->SetInt( "patient", m_pOuter->GetUserID() );
											event->SetInt( "healer", pHealScorer->GetUserID() );
											event->SetInt( "amount", m_aHealers[i].flHealedLastSecond );
											gameeventmanager->FireEvent( event );
										}
									}

									m_aHealers[i].flHealedLastSecond = 0;
									m_flHealedPerSecondTimer = gpGlobals->curtime + 1.0f;
								}
							}
						}
						else
						{
							CTF_GameStats.Event_PlayerLeachedHealth( m_pOuter, m_aHealers[i].bDispenserHeal, flHealAmount );
						}
					}
				}
			}
		}

		if ( InCond( TF_COND_BURNING ) )
		{
			// Reduce the duration of this burn 
			float flReduction = 2.f * gpGlobals->frametime;	 // ( flReduction + 1 ) x faster reduction
			m_flAfterburnDuration -= flReduction;
		}
	}

	if ( !InCond( TF_COND_HEALTH_OVERHEALED ) && m_pOuter->GetHealth() > ( m_pOuter->GetMaxHealth() ) )
	{
		AddCond( TF_COND_HEALTH_OVERHEALED, PERMANENT_CONDITION );
	}
	else if ( InCond( TF_COND_HEALTH_OVERHEALED ) && m_pOuter->GetHealth() <= ( m_pOuter->GetMaxHealth() ) )
	{
		RemoveCond( TF_COND_HEALTH_OVERHEALED );
	}

	if ( bDecayHealth )
	{
		float flOverheal = GetMaxBuffedHealth( false, true );

		// If we're not being buffed, our health drains back to our max
		if ( m_pOuter->GetHealth() > m_pOuter->GetMaxHealth() )
		{
			// Items exist that get us over max health, without ever being healed, in which case our m_flBestOverhealDecayMult will still be -1.
			float flDrainMult = ( m_flBestOverhealDecayMult == -1.f ) ? 1.f : m_flBestOverhealDecayMult;
			float flBoostMaxAmount = flOverheal - m_pOuter->GetMaxHealth();
			float flDrain = flBoostMaxAmount / (tf_boost_drain_time.GetFloat() * flDrainMult);
			m_flHealFraction += (gpGlobals->frametime * flDrain);

			int nHealthToDrain = (int)m_flHealFraction;
			if ( nHealthToDrain > 0 )
			{
				m_flHealFraction -= nHealthToDrain;

				// Manually subtract the health so we don't generate pain sounds / etc
				m_pOuter->m_iHealth -= nHealthToDrain;
			}
		}
		else if ( m_flBestOverhealDecayMult != -1 )
		{
			m_flBestOverhealDecayMult = -1;
		}

	}

	if ( bDecayDisguiseHealth )
	{
		float flOverheal = GetDisguiseMaxBuffedHealth( false, true );

		if ( InCond( TF_COND_DISGUISED ) && (GetDisguiseHealth() > GetDisguiseMaxHealth()) )
		{
			// Items exist that get us over max health, without ever being healed, in which case our m_flBestOverhealDecayMult will still be -1.
			float flDrainMult = (m_flBestOverhealDecayMult == -1) ? 1.0 : m_flBestOverhealDecayMult;
			float flBoostMaxAmount = flOverheal - GetDisguiseMaxHealth();
			float flDrain = (flBoostMaxAmount / tf_boost_drain_time.GetFloat()) * flDrainMult;
			m_flDisguiseHealFraction += (gpGlobals->frametime * flDrain);

			int nHealthToDrain = (int)m_flDisguiseHealFraction;
			if ( nHealthToDrain > 0 )
			{
				m_flDisguiseHealFraction -= nHealthToDrain;

				// Reduce our fake disguised health by roughly the same amount
				m_iDisguiseHealth -= nHealthToDrain;
			}
		}
	}

	// Taunt
	if ( InCond( TF_COND_TAUNTING ) )
	{
		if ( gpGlobals->curtime > m_pOuter->GetTauntRemoveTime() )
		{
			RemoveCond( TF_COND_TAUNTING );
		}
	}
	if ( InCond( TF_COND_BURNING ) )
	{
		if ( m_flAfterburnDuration <= 0.f || m_pOuter->GetWaterLevel() >= WL_Waist )
		{
			// If we're underwater, put the fire out
			if ( m_pOuter->GetWaterLevel() >= WL_Waist )
			{
				// General achievement for jumping into water while you're on fire
				m_pOuter->AwardAchievement( ACHIEVEMENT_TF_FIRE_WATERJUMP );

				// Pyro achievement for forcing players into water
				if ( m_hBurnAttacker )
				{
					m_hBurnAttacker->AwardAchievement( ACHIEVEMENT_TF_PYRO_FORCE_WATERJUMP );
				}
			}

			RemoveCond( TF_COND_BURNING );

			if ( InCond( TF_COND_HEALTH_BUFF ) )
			{
				// one or more players is healing us, send a "player_extinguished" event. We
				// need to send one for each player who's healing us.
				for ( int i = 0; i < m_aHealers.Count(); i++ )
				{
					Assert( m_aHealers[i].pHealer );

					if ( m_aHealers[i].bDispenserHeal )
					{
						CObjectDispenser *pDispenser = dynamic_cast<CObjectDispenser*>( m_aHealers[i].pHealer.Get() );
						if ( pDispenser )
						{
							CTFPlayer *pTFPlayer = pDispenser->GetBuilder();
							if ( pTFPlayer )
							{
								UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"player_extinguished\" against \"%s<%i><%s><%s>\" with \"dispenser\" (attacker_position \"%d %d %d\") (victim_position \"%d %d %d\")\n",    
									pTFPlayer->GetPlayerName(),
									pTFPlayer->GetUserID(),
									pTFPlayer->GetNetworkIDString(),
									pTFPlayer->GetTeam()->GetName(),
									m_pOuter->GetPlayerName(),
									m_pOuter->GetUserID(),
									m_pOuter->GetNetworkIDString(),
									m_pOuter->GetTeam()->GetName(),
									(int)m_aHealers[i].pHealer->GetAbsOrigin().x, 
									(int)m_aHealers[i].pHealer->GetAbsOrigin().y,
									(int)m_aHealers[i].pHealer->GetAbsOrigin().z,
									(int)m_pOuter->GetAbsOrigin().x, 
									(int)m_pOuter->GetAbsOrigin().y,
									(int)m_pOuter->GetAbsOrigin().z );
							}
						}

//						continue;
					}

					EHANDLE pHealer = m_aHealers[i].pHealer;
					if ( m_aHealers[i].bDispenserHeal || !pHealer || !pHealer->IsPlayer() )
						pHealer = m_aHealers[i].pHealScorer;

					if ( !pHealer )
						continue;

					CTFPlayer *pTFPlayer = ToTFPlayer( pHealer );
					if ( pTFPlayer && !m_aHealers[i].bDispenserHeal )
					{
						UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"player_extinguished\" against \"%s<%i><%s><%s>\" with \"%s\" (attacker_position \"%d %d %d\") (victim_position \"%d %d %d\")\n",    
							pTFPlayer->GetPlayerName(),
							pTFPlayer->GetUserID(),
							pTFPlayer->GetNetworkIDString(),
							pTFPlayer->GetTeam()->GetName(),
							m_pOuter->GetPlayerName(),
							m_pOuter->GetUserID(),
							m_pOuter->GetNetworkIDString(),
							m_pOuter->GetTeam()->GetName(),
							( pTFPlayer->GetActiveTFWeapon() ) ? pTFPlayer->GetActiveTFWeapon()->GetName() : "tf_weapon_medigun",
							(int)pTFPlayer->GetAbsOrigin().x, 
							(int)pTFPlayer->GetAbsOrigin().y,
							(int)pTFPlayer->GetAbsOrigin().z,
							(int)m_pOuter->GetAbsOrigin().x, 
							(int)m_pOuter->GetAbsOrigin().y,
							(int)m_pOuter->GetAbsOrigin().z );
					}

					// Tell the clients involved 
					CRecipientFilter involved_filter;
					CBasePlayer *pBasePlayerHealer = ToBasePlayer( pHealer );
					if ( pBasePlayerHealer )
					{
						involved_filter.AddRecipient( pBasePlayerHealer );
					}
					involved_filter.AddRecipient( m_pOuter );
					UserMessageBegin( involved_filter, "PlayerExtinguished" );
						WRITE_BYTE( pHealer->entindex() );
						WRITE_BYTE( m_pOuter->entindex() );
					MessageEnd();

					IGameEvent *event = gameeventmanager->CreateEvent( "player_extinguished" );
					if ( event )
					{
						event->SetInt( "victim", m_pOuter->entindex() );
						event->SetInt( "healer", pHealer->entindex() );

						gameeventmanager->FireEvent( event, true );
					}
				}
			}
		}
		else if ( gpGlobals->curtime >= m_flFlameBurnTime )
		{
			// Burn the player (if not pyro, who does not take persistent burning damage)
			if ( ( TF_CLASS_PYRO != m_pOuter->GetPlayerClass()->GetClassIndex() ) )
			{
				float flBurnDamage = TF_BURNING_DMG;
				int nKillType = TF_DMG_CUSTOM_BURNING;
#ifdef DEBUG
				if ( tf_afterburn_debug.GetBool() )
				{
					engine->Con_NPrintf( 0, "Tick afterburn duration = %f", m_flAfterburnDuration );
				}
#endif // DEBUG
	
				CTakeDamageInfo info( m_hBurnAttacker, m_hBurnAttacker, m_hBurnWeapon, flBurnDamage, DMG_BURN | DMG_PREVENT_PHYSICS_FORCE, nKillType );
				m_pOuter->TakeDamage( info );
			}

			m_flFlameBurnTime = gpGlobals->curtime + TF_BURNING_FREQUENCY;
			m_flAfterburnDuration -= TF_BURNING_FREQUENCY;
		}

		if ( m_flNextBurningSound < gpGlobals->curtime )
		{
			m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_ONFIRE );
			m_flNextBurningSound = gpGlobals->curtime + 2.5;
		}
	}


	// Stops the drain hack.
	if ( m_pOuter->IsPlayerClass( TF_CLASS_MEDIC ) )
	{
		CWeaponMedigun *pWeapon = ( CWeaponMedigun* )m_pOuter->Weapon_OwnsThisID( TF_WEAPON_MEDIGUN );
		if ( pWeapon && pWeapon->IsReleasingCharge() )
		{
			pWeapon->DrainCharge();
		}
	}

	TestAndExpireChargeEffect();

	if ( InCond( TF_COND_STEALTHED_BLINK ) )
	{
		float flBlinkTime = TF_SPY_STEALTH_BLINKTIME;
		if ( flBlinkTime < ( gpGlobals->curtime - m_flLastStealthExposeTime ) )
		{
			RemoveCond( TF_COND_STEALTHED_BLINK );
		}
	}

	if ( !InCond( TF_COND_DISGUISED ) )
	{
		// Remove our disguise weapon if we are ever not disguised and we have one.
		RemoveDisguiseWeapon();

		// also clear the disguise weapon list
		m_pOuter->ClearDisguiseWeaponList();
	}

	{
		m_flSpyTranqBuffDuration = 0;
	}
#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose: call all the shared think funcs here
//-----------------------------------------------------------------------------
void CTFPlayerShared::SharedThink( void )
{
	ConditionThink();
	InvisibilityThink();
}

//-----------------------------------------------------------------------------
// Purpose: Do CLIENT/SERVER SHARED condition thinks.
//-----------------------------------------------------------------------------
void CTFPlayerShared::ConditionThink( void )
{
	// Client Only Updates Meters for Local Only
#ifdef CLIENT_DLL
	if ( m_pOuter->IsLocalPlayer() )
#endif
	{
		UpdateCloakMeter();
	}

	m_ConditionList.Think();

	CheckDisguiseTimer();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::CheckDisguiseTimer( void )
{
	if ( InCond( TF_COND_DISGUISING ) && GetDisguiseCompleteTime() > 0 )
	{
		if ( gpGlobals->curtime > GetDisguiseCompleteTime() )
		{
			CompleteDisguise();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddZoomed( void )
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveZoomed( void )
{
#ifdef GAME_DLL
	m_pOuter->SetFOV( m_pOuter, 0, 0.1f );
#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddDisguising( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->GetPredictable() && ( !prediction->IsFirstTimePredicted() || m_bSyncingConditions ) )
		return;

	if ( m_pOuter->m_pDisguisingEffect )
	{
//		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pDisguisingEffect );
	}

	if ( !m_pOuter->IsLocalPlayer() && ( !IsStealthed() || !m_pOuter->IsEnemyPlayer() ) )
	{
		const char *pEffectName = ( m_pOuter->GetTeamNumber() == TF_TEAM_RED ) ? "spy_start_disguise_red" : "spy_start_disguise_blue";
		m_pOuter->m_pDisguisingEffect = m_pOuter->ParticleProp()->Create( pEffectName, PATTACH_ABSORIGIN_FOLLOW );
		m_pOuter->m_flDisguiseEffectStartTime = gpGlobals->curtime;
	}

	m_pOuter->EmitSound( "Player.Spy_Disguise" );

#endif
}

//-----------------------------------------------------------------------------
// Purpose: set up effects for when player finished disguising
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddDisguised( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->m_pDisguisingEffect )
	{
		// turn off disguising particles
//		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pDisguisingEffect );
		m_pOuter->m_pDisguisingEffect = NULL;
	}
	m_pOuter->m_flDisguiseEndEffectStartTime = gpGlobals->curtime;

	m_pOuter->UpdateSpyStateChange();
#endif
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Return the team that the spy is displayed as.
// Not disguised: His own team
// Disguised: The team he is disguised as
//-----------------------------------------------------------------------------
int CTFPlayerShared::GetDisplayedTeam( void ) const
{
	int iVisibleTeam = m_pOuter->GetTeamNumber();
	// if this player is disguised and on the other team, use disguise team
	if ( InCond( TF_COND_DISGUISED ) && m_pOuter->IsEnemyPlayer() )
	{
		iVisibleTeam = GetDisguiseTeam();
	}

	return iVisibleTeam;
}

//-----------------------------------------------------------------------------
// Purpose: start, end, and changing disguise classes
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnDisguiseChanged( void )
{
	// recalc disguise model index
	//RecalcDisguiseWeapon( true );
	m_pOuter->UpdateSpyStateChange();
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddInvulnerable( void )
{
#ifdef CLIENT_DLL

	if ( m_pOuter->IsLocalPlayer() )
	{
		const char *pEffectName = NULL;

		switch( m_pOuter->GetTeamNumber() )
		{
		case TF_TEAM_BLUE:
		default:
			pEffectName = TF_SCREEN_OVERLAY_MATERIAL_INVULN_BLUE;
			break;
		case TF_TEAM_RED:
			pEffectName =  TF_SCREEN_OVERLAY_MATERIAL_INVULN_RED;
			break;
		}

		IMaterial *pMaterial = materials->FindMaterial( pEffectName, TEXTURE_GROUP_CLIENT_EFFECTS, false );
		if ( !IsErrorMaterial( pMaterial ) )
		{
			view->SetScreenOverlayMaterial( pMaterial );
		}

		if ( m_pOuter->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
		{
			g_AchievementMgrTF.OnAchievementEvent( ACHIEVEMENT_TF_HEAVY_RECEIVE_UBER_GRIND );
		}
	}
#else
	// remove any persistent damaging conditions
	if ( InCond( TF_COND_BURNING ) )
	{
		RemoveCond( TF_COND_BURNING );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveInvulnerable( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->IsLocalPlayer() )
	{
		// only remove the overlay if it is an invuln material 
		IMaterial *pMaterial = view->GetScreenOverlayMaterial();

		if ( pMaterial &&
				( FStrEq( pMaterial->GetName(), TF_SCREEN_OVERLAY_MATERIAL_INVULN_BLUE ) || 
				  FStrEq( pMaterial->GetName(), TF_SCREEN_OVERLAY_MATERIAL_INVULN_RED ) ) )
		{
			view->SetScreenOverlayMaterial( NULL );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveInvulnerableWearingOff( void )
{
#ifdef CLIENT_DLL
	m_flInvulnerabilityRemoveTime = gpGlobals->curtime;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddTaunting( void )
{
	// Unzoom if we are a sniper zoomed!
	InstantlySniperUnzoom();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveTaunting( void )
{
#ifdef GAME_DLL

	m_pOuter->StopTaunt();
#endif // GAME_DLL

	m_pOuter->m_PlayerAnimState->ResetGestureSlot( GESTURE_SLOT_VCD );

	// when we stop taunting, make sure active weapon is visible
	if ( m_pOuter->GetActiveWeapon() )
	{
		m_pOuter->GetActiveWeapon()->SetWeaponVisible( true );
	}
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void AddUberScreenEffect( const CTFPlayer* pPlayer )
{
	// Add the uber effect onto the local player's screen
	if ( pPlayer && pPlayer->IsLocalPlayer() )
	{
		const char *pEffectName = NULL;
		if ( pPlayer->GetTeamNumber() == TF_TEAM_RED )
		{
			pEffectName = TF_SCREEN_OVERLAY_MATERIAL_INVULN_RED;
		}
		else
		{
			pEffectName = TF_SCREEN_OVERLAY_MATERIAL_INVULN_BLUE;
		}

		IMaterial *pMaterial = materials->FindMaterial( pEffectName, TEXTURE_GROUP_CLIENT_EFFECTS, false );
		if ( !IsErrorMaterial( pMaterial ) )
		{
			view->SetScreenOverlayMaterial( pMaterial );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void RemoveUberScreenEffect( const CTFPlayer* pPlayer )
{
	if ( pPlayer && pPlayer->IsLocalPlayer() )
	{
		// only remove the overlay if it is an invuln material 
		IMaterial *pMaterial = view->GetScreenOverlayMaterial();

		if ( pMaterial &&
			( FStrEq( pMaterial->GetName(), TF_SCREEN_OVERLAY_MATERIAL_INVULN_BLUE ) || 
			FStrEq( pMaterial->GetName(), TF_SCREEN_OVERLAY_MATERIAL_INVULN_RED ) ) )
		{
			view->SetScreenOverlayMaterial( NULL );
		}
	}
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddTeleported( void )
{
#ifdef CLIENT_DLL
	m_pOuter->UpdateRecentlyTeleportedEffect();
	m_flGotTeleEffectAt = gpGlobals->curtime;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveTeleported( void )
{
#ifdef CLIENT_DLL
	m_pOuter->UpdateRecentlyTeleportedEffect();
#endif
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerShared::ShouldShowRecentlyTeleported( void )
{
	if ( IsStealthed() )
	{
		return false;
	}

	if ( m_pOuter->IsPlayerClass( TF_CLASS_SPY ) )
	{
		// disguised as an enemy
		if ( InCond( TF_COND_DISGUISED ) && GetDisguiseTeam() != m_pOuter->GetTeamNumber() )
		{
			// was this my own team's teleporter?
			if ( GetTeamTeleporterUsed() == m_pOuter->GetTeamNumber() )
			{
				// don't show my trail
				return false;
			}
			else
			{
				// okay to show the local player the trail, but not his team (might confuse them)
				if ( !m_pOuter->IsLocalPlayer() && m_pOuter->GetTeamNumber() == GetLocalPlayerTeam() )
				{
					return false;
				}
			}
		}
		else
		{
			if ( GetTeamTeleporterUsed() != m_pOuter->GetTeamNumber() )
			{
				return false;
			}
		}
	}

	return InCond( TF_COND_TELEPORTED );
}
#endif


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::Burn( CTFPlayer *pAttacker, CTFWeaponBase *pWeapon )
{
#ifdef GAME_DLL
	// Don't bother igniting players who have just been killed by the fire damage.
	if ( !m_pOuter->IsAlive() )
		return;

	// pyros don't burn persistently or take persistent burning damage, but we show brief burn effect so attacker can tell they hit
	bool bVictimIsImmunePyro = ( TF_CLASS_PYRO ==  m_pOuter->GetPlayerClass()->GetClassIndex() );
	if ( bVictimIsImmunePyro )
	{
		bVictimIsImmunePyro = true;
	}

#ifdef DEBUG
	static float s_flStartAfterburnTime = 0.f;
	static float s_flReachMaxAfterburnTime = 0.f;
#endif // DEBUG

	if ( !InCond( TF_COND_BURNING ) )
	{
		// Start burning
		AddCond( TF_COND_BURNING, -1.f, pAttacker );
		m_flFlameBurnTime = gpGlobals->curtime + TF_BURNING_FREQUENCY;
		m_flAfterburnDuration = 3.f;

		// let the attacker know he burned me
		if ( pAttacker && !bVictimIsImmunePyro )
		{
			pAttacker->OnBurnOther( m_pOuter, pWeapon );

			m_hOriginalBurnAttacker = pAttacker;
		}

#ifdef DEBUG
		s_flStartAfterburnTime = gpGlobals->curtime;
		s_flReachMaxAfterburnTime = 0.f;
#endif // DEBUG
	}

	int bAfterburnImmunity = bVictimIsImmunePyro;

	// check afterburn duration
	float flFlameLife;
	if ( bAfterburnImmunity )
	{
		flFlameLife = TF_BURNING_FLAME_LIFE_PYRO;
	}
	else
	{
		flFlameLife = TF_BURNING_FLAME_LIFE;
	}

	m_flAfterburnDuration = flFlameLife;

#ifdef DEBUG
	if ( tf_afterburn_debug.GetBool() )
	{
		engine->Con_NPrintf( 1, "Added afterburn duration = %f", m_flAfterburnDuration );

		if ( s_flReachMaxAfterburnTime == 0.f && m_flAfterburnDuration == TF_BURNING_FLAME_LIFE )
		{
			s_flReachMaxAfterburnTime = gpGlobals->curtime;
			DevMsg( "took %f seconds to reach max afterburn duration\n", s_flReachMaxAfterburnTime - s_flStartAfterburnTime );
		}
	}
#endif // DEBUG

	m_hBurnAttacker = pAttacker;
	m_hBurnWeapon = pWeapon;

#endif // GAME_DLL
}


//-----------------------------------------------------------------------------
// Purpose: A non-TF Player burned us
// Used for Bosses, they inflict self burn
//-----------------------------------------------------------------------------
void CTFPlayerShared::SelfBurn()
{
#ifdef GAME_DLL
	Burn( m_pOuter, NULL );
#endif // GAME_DLL
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveBurning( void )
{
#ifdef CLIENT_DLL
	m_pOuter->StopBurningSound();

	if ( m_pOuter->m_nOldWaterLevel > 0 )
	{	
		m_pOuter->ParticleProp()->Create( "water_burning_steam", PATTACH_ABSORIGIN );
	}

	if ( m_pOuter->m_pBurningEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pBurningEffect );
		m_pOuter->m_pBurningEffect = NULL;
	}

	if ( m_pOuter->IsLocalPlayer() )
	{
		// only remove the overlay if it is the burning 
		IMaterial *pMaterial = view->GetScreenOverlayMaterial();

		if ( pMaterial && FStrEq( pMaterial->GetName(), TF_SCREEN_OVERLAY_MATERIAL_BURNING ) )
		{
			view->SetScreenOverlayMaterial( NULL );
		}
	}

	m_pOuter->m_flBurnEffectStartTime = 0;
#else
	m_hBurnAttacker = NULL;
	m_hOriginalBurnAttacker = NULL;
	m_hBurnWeapon = NULL;

	m_pOuter->ClearBurnFromBehindAttackers();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveOverhealed( void )
{
#ifdef CLIENT_DLL
	if ( !m_pOuter->IsLocalPlayer() )
	{
		m_pOuter->UpdateOverhealEffect();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddStealthed( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->GetPredictable() && ( !prediction->IsFirstTimePredicted() || m_bSyncingConditions ) )
		return;

	m_pOuter->EmitSound( "Player.Spy_Cloak" );
	m_pOuter->RemoveAllDecals();
#endif

	bool bSetInvisChangeTime = true;
#ifdef CLIENT_DLL
	if ( !m_pOuter->IsLocalPlayer() )
	{
		// We only clientside predict changetime for the local player
		bSetInvisChangeTime = false;
	}
#endif

	if ( bSetInvisChangeTime )
	{
		m_flInvisChangeCompleteTime = gpGlobals->curtime + tf_spy_invis_time.GetFloat();
	}

	// set our offhand weapon to be the invis weapon, but only for the spy's stealth
	if ( InCond( TF_COND_STEALTHED ) )
	{
		for (int i = 0; i < m_pOuter->WeaponCount(); i++) 
		{
			CTFWeaponInvis *pWpn = (CTFWeaponInvis *) m_pOuter->GetWeapon(i);
			if ( !pWpn )
				continue;

			if ( pWpn->GetWeaponID() != TF_WEAPON_INVIS )
				continue;

			// try to switch to this weapon
			m_pOuter->SetOffHandWeapon( pWpn );
			break;
		}
	}

	m_pOuter->TeamFortress_SetSpeed();

#ifdef CLIENT_DLL
	// Remove water balloon effect if it on player
	m_pOuter->ParticleProp()->StopParticlesNamed( "balloontoss_drip", true );

	m_pOuter->UpdateSpyStateChange();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveStealthed( void )
{
#ifdef CLIENT_DLL
	if ( !m_bSyncingConditions )
		return;

	m_pOuter->EmitSound( "Player.Spy_UnCloak" );

	if ( m_pOuter->IsLocalPlayer() )
	{
		IMaterial *pMaterial = view->GetScreenOverlayMaterial();
		if ( pMaterial && FStrEq( pMaterial->GetName(), TF_SCREEN_OVERLAY_MATERIAL_STEALTH ) )
		{
			view->SetScreenOverlayMaterial( NULL );
		}
	}
#endif

	m_pOuter->HolsterOffHandWeapon();

	m_pOuter->TeamFortress_SetSpeed();

#ifdef CLIENT_DLL
	m_pOuter->UpdateSpyStateChange();
#endif

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveDisguising( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->m_pDisguisingEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pDisguisingEffect );
		m_pOuter->m_pDisguisingEffect = NULL;
	}
#else
	m_nDesiredDisguiseTeam = TF_SPY_UNDEFINED;

	// Do not reset this value, we use the last desired disguise class for the
	// 'lastdisguise' command

	//m_nDesiredDisguiseClass = TF_CLASS_UNDEFINED;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveDisguised( void )
{
#ifdef CLIENT_DLL

	if ( m_pOuter->GetPredictable() && ( !prediction->IsFirstTimePredicted() || m_bSyncingConditions ) )
		return;

	// if local player is on the other team, reset the model of this player
	CTFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !m_pOuter->InSameTeam( pLocalPlayer ) )
	{
		TFPlayerClassData_t *pData = GetPlayerClassData( TF_CLASS_SPY );
		int iIndex = modelinfo->GetModelIndex( pData->GetModelName() );

		m_pOuter->SetModelIndex( iIndex );
	}

	m_pOuter->EmitSound( "Player.Spy_Disguise" );

	// They may have called for medic and created a visible medic bubble
	m_pOuter->StopSaveMeEffect( true );

	m_pOuter->UpdateSpyStateChange();

#else
	m_nDisguiseTeam  = TF_SPY_UNDEFINED;
	m_nDisguiseClass.Set( TF_CLASS_UNDEFINED );
	m_nDisguiseSkinOverride = 0;
	m_hDisguiseTarget.Set( NULL );
	m_iDisguiseHealth = 0;
	SetDisguiseBody( 0 );
	m_iDisguiseAmmo = 0;

	// Update the player model and skin.
	m_pOuter->UpdateModel();

	m_pOuter->ClearExpression();

	m_pOuter->ClearDisguiseWeaponList();

	RemoveDisguiseWeapon();

#endif

	m_pOuter->TeamFortress_SetSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddBurning( void )
{
#ifdef CLIENT_DLL
	// Start the burning effect
	if ( !m_pOuter->m_pBurningEffect )
	{
		if ( !( m_pOuter->IsLocalPlayer() ) )
		{
			const char *pEffectName = ( m_pOuter->GetTeamNumber() == TF_TEAM_RED ) ? "burningplayer_red" : "burningplayer_blue";
			m_pOuter->m_pBurningEffect = m_pOuter->ParticleProp()->Create( pEffectName, PATTACH_ABSORIGIN_FOLLOW );
		}

		m_pOuter->m_flBurnEffectStartTime = gpGlobals->curtime;
	}
	// set the burning screen overlay
	if ( m_pOuter->IsLocalPlayer() )
	{
		IMaterial *pMaterial = materials->FindMaterial( TF_SCREEN_OVERLAY_MATERIAL_BURNING, TEXTURE_GROUP_CLIENT_EFFECTS, false );
		if ( !IsErrorMaterial( pMaterial ) )
		{
			view->SetScreenOverlayMaterial( pMaterial );
		}
	}
#endif

	// play a fire-starting sound
	m_pOuter->EmitSound( "Fire.Engulf" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddOverhealed( void )
{
#ifdef CLIENT_DLL
	// Start the Overheal effect
	 
	if ( !m_pOuter->IsLocalPlayer() )
	{
		m_pOuter->UpdateOverhealEffect();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayerShared::GetStealthNoAttackExpireTime( void )
{
	return m_flStealthNoAttackExpire;
}

//-----------------------------------------------------------------------------
// Purpose: Sets whether this player is dominating the specified other player
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetPlayerDominated( CTFPlayer *pPlayer, bool bDominated )
{
	int iPlayerIndex = pPlayer->entindex();
	m_bPlayerDominated.Set( iPlayerIndex, bDominated );
	pPlayer->m_Shared.SetPlayerDominatingMe( m_pOuter, bDominated );
}

//-----------------------------------------------------------------------------
// Purpose: Sets whether this player is being dominated by the other player
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetPlayerDominatingMe( CTFPlayer *pPlayer, bool bDominated )
{
	int iPlayerIndex = pPlayer->entindex();
	m_bPlayerDominatingMe.Set( iPlayerIndex, bDominated );

#ifdef GAME_DLL
	if ( bDominated )
	{
		CTFPlayer *pDominatingPlayer = ToTFPlayer( pPlayer );
		if ( pDominatingPlayer && pDominatingPlayer->IsPlayerClass( TF_CLASS_MEDIC ) )
		{
			CBaseEntity *pHealedEntity = pPlayer->MedicGetHealTarget();
			CTFPlayer *pHealedPlayer = ToTFPlayer( pHealedEntity );

			if ( pHealedPlayer && pHealedPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
			{
				pHealedPlayer->AwardAchievement( ACHIEVEMENT_TF_HEAVY_EARN_MEDIC_DOMINATION );
			}
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether this player is dominating the specified other player
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsPlayerDominated( int iPlayerIndex )
{
#ifdef CLIENT_DLL
	// On the client, we only have data for the local player.
	// As a result, it's only valid to ask for dominations related to the local player
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return false;

	Assert( m_pOuter->IsLocalPlayer() || pLocalPlayer->entindex() == iPlayerIndex );

	if ( m_pOuter->IsLocalPlayer() )
		return m_bPlayerDominated.Get( iPlayerIndex );

	return pLocalPlayer->m_Shared.IsPlayerDominatingMe( m_pOuter->entindex() );
#else
	// Server has all the data.
	return m_bPlayerDominated.Get( iPlayerIndex );
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsPlayerDominatingMe( int iPlayerIndex )
{
	return m_bPlayerDominatingMe.Get( iPlayerIndex );
}

//-----------------------------------------------------------------------------
// Purpose: True if the given player is a spy disguised as our team.
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsSpyDisguisedAsMyTeam( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
		return false;

	if ( pPlayer->IsPlayerClass( TF_CLASS_SPY ) &&
		pPlayer->GetTeamNumber() != m_pOuter->GetTeamNumber() &&
		pPlayer->m_Shared.GetDisguiseTeam() == m_pOuter->GetTeamNumber() )
	{
		return true;
	}
	else
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::NoteLastDamageTime( int nDamage )
{
	// we took damage
	if ( ( nDamage > 5 ) && InCond( TF_COND_STEALTHED ) )
	{
		m_flLastStealthExposeTime = gpGlobals->curtime;
		AddCond( TF_COND_STEALTHED_BLINK );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnSpyTouchedByEnemy( void )
{
	if ( InCond( TF_COND_STEALTHED ) )
	{
		m_flLastStealthExposeTime = gpGlobals->curtime;
		AddCond( TF_COND_STEALTHED_BLINK );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsEnteringOrExitingFullyInvisible( void )
{
	return ( ( GetPercentInvisiblePrevious() != 1.f && GetPercentInvisible() == 1.f ) || 
			 ( GetPercentInvisiblePrevious() == 1.f && GetPercentInvisible() != 1.f ) );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::FadeInvis( float fAdditionalRateScale )
{
	ETFCond nExpiringCondition = TF_COND_LAST;
	if ( InCond( TF_COND_STEALTHED ) )
	{
		nExpiringCondition = TF_COND_STEALTHED;
		RemoveCond( TF_COND_STEALTHED );
	}

	// This comes from script, so sanity check the result.
	//float flDecloakRateScale = 1.0f;
	
	float flInvisFadeTime = fAdditionalRateScale
						  * (tf_spy_invis_unstealth_time.GetFloat() );

	if ( flInvisFadeTime < 0.15 )
	{
		// this was a force respawn, they can attack whenever
	}
	else
	{
		// next attack in some time
		m_flStealthNoAttackExpire = gpGlobals->curtime + (tf_spy_cloak_no_attack_time.GetFloat() * fAdditionalRateScale);
	}

	m_flInvisChangeCompleteTime = gpGlobals->curtime + flInvisFadeTime;
}

//-----------------------------------------------------------------------------
// Purpose: Approach our desired level of invisibility
//-----------------------------------------------------------------------------
void CTFPlayerShared::InvisibilityThink( void )
{
	if ( m_pOuter->GetPlayerClass()->GetClassIndex() != TF_CLASS_SPY && InCond( TF_COND_STEALTHED ) )
	{
		// Shouldn't happen, but it's a safety net
		m_flInvisibility = 0.0f;
		if ( InCond( TF_COND_STEALTHED ) )
		{
			RemoveCond( TF_COND_STEALTHED );
		}
		return;
	}

	float flTargetInvis = 0.0f;
	float flTargetInvisScale = 1.0f;
	if ( InCond( TF_COND_STEALTHED_BLINK ) )
	{
		// We were bumped into or hit for some damage.
		flTargetInvisScale = TF_SPY_STEALTH_BLINKSCALE;
	}

	// Go invisible or appear.
	if ( m_flInvisChangeCompleteTime > gpGlobals->curtime )
	{
		if ( IsStealthed() )
		{
			flTargetInvis = 1.0f - ( ( m_flInvisChangeCompleteTime - gpGlobals->curtime ) );
		}
		else
		{
			flTargetInvis = ( ( m_flInvisChangeCompleteTime - gpGlobals->curtime ) * 0.5f );
		}
	}
	else
	{
		if ( IsStealthed() )
		{
			flTargetInvis = 1.0f;
			m_flLastNoMovementTime = -1.f;
		}
		else
		{
			flTargetInvis = 0.0f;
		}
	}

	flTargetInvis *= flTargetInvisScale;
	m_flPrevInvisibility = m_flInvisibility;
	m_flInvisibility = clamp( flTargetInvis, 0.0f, 1.0f );

}


//-----------------------------------------------------------------------------
// Purpose: How invisible is the player [0..1]
//-----------------------------------------------------------------------------
float CTFPlayerShared::GetPercentInvisible( void ) const
{

	return m_flInvisibility;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsInvulnerable( void ) const
{
	return InCond( TF_COND_INVULNERABLE );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsStealthed( void ) const
{
	return InCond( TF_COND_STEALTHED );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayerShared::CanBeDebuffed( void ) const
{
	if ( IsInvulnerable() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Start the process of disguising
//-----------------------------------------------------------------------------
void CTFPlayerShared::Disguise( int nTeam, int nClass, CTFPlayer* pDesiredTarget, bool bOnKill )
{
	int nRealTeam = m_pOuter->GetTeamNumber();
	int nRealClass = m_pOuter->GetPlayerClass()->GetClassIndex();

	Assert ( ( nClass >= TF_CLASS_SCOUT ) && ( nClass <= TF_CLASS_ENGINEER ) );

	// we're not a spy
	if ( nRealClass != TF_CLASS_SPY )
	{
		return;
	}

	if ( InCond( TF_COND_TAUNTING ) )
	{
		// not allowed to disguise while taunting
		return;
	}

	// we're not disguising as anything but ourselves (so reset everything)
	if ( nRealTeam == nTeam && nRealClass == nClass )
	{
		RemoveDisguise();
		return;
	}

	// Ignore disguise of the same type, unless we're using 'Your Eternal Reward'
	if ( nTeam == m_nDisguiseTeam && nClass == m_nDisguiseClass && !bOnKill )
	{
#ifdef GAME_DLL
		DetermineDisguiseWeapon( false );
#endif
		return;
	}

	// invalid team
	if ( nTeam <= TEAM_SPECTATOR || nTeam >= TF_TEAM_COUNT )
	{
		return;
	}

	// invalid class
	if ( nClass <= TF_CLASS_UNDEFINED || nClass >= TF_CLASS_COUNT )
	{
		return;
	}

	// are we already in the middle of disguising as this class? 
	// (the lastdisguise key might get pushed multiple times before the disguise is complete)
	if ( InCond( TF_COND_DISGUISING ) )
	{
		if ( nTeam == m_nDesiredDisguiseTeam && nClass == m_nDesiredDisguiseClass )
		{
			return;
		}
	}

	m_hDesiredDisguiseTarget.Set( pDesiredTarget );
	m_nDesiredDisguiseClass = nClass;
	m_nDesiredDisguiseTeam = nTeam;

	m_bLastDisguisedAsOwnTeam = ( m_nDesiredDisguiseTeam == m_pOuter->GetTeamNumber() );

	AddCond( TF_COND_DISGUISING );

	// Start the think to complete our disguise
	float flTimeToDisguise = TF_TIME_TO_DISGUISE;

	// STAGING_SPY
	// Quick disguise if you already disguised
	if ( InCond( TF_COND_DISGUISED ) )
	{
		flTimeToDisguise = TF_TIME_TO_QUICK_DISGUISE;
	}

	if ( pDesiredTarget )
	{
		flTimeToDisguise = 0;
	}
	m_flDisguiseCompleteTime = gpGlobals->curtime + flTimeToDisguise;
}

//-----------------------------------------------------------------------------
// Purpose: Set our target with a player we've found to emulate
//-----------------------------------------------------------------------------
#ifndef CLIENT_DLL
void CTFPlayerShared::FindDisguiseTarget( void )
{
	if ( m_hDesiredDisguiseTarget )
	{
		m_hDisguiseTarget = m_hDesiredDisguiseTarget;
		m_hDesiredDisguiseTarget = NULL;
	}
	else
	{
		m_hDisguiseTarget = m_pOuter->TeamFortress_GetDisguiseTarget( m_nDisguiseTeam, m_nDisguiseClass );
	}

	m_pOuter->CreateDisguiseWeaponList( m_hDisguiseTarget );
}

#endif

int CTFPlayerShared::GetDisguiseTeam( void ) const
{
	return m_nDisguiseTeam;
}

//-----------------------------------------------------------------------------
// Purpose: Complete our disguise
//-----------------------------------------------------------------------------
void CTFPlayerShared::CompleteDisguise( void )
{
	AddCond( TF_COND_DISGUISED );

	m_nDisguiseClass = m_nDesiredDisguiseClass;
	m_nDisguiseTeam = m_nDesiredDisguiseTeam;

	if ( m_nDisguiseClass == TF_CLASS_SPY )
	{
		m_nMaskClass = rand()%9+1;
	}

	RemoveCond( TF_COND_DISGUISING );

#ifdef GAME_DLL
	// Update the player model and skin.
	m_pOuter->UpdateModel();
	m_pOuter->ClearExpression();

	FindDisguiseTarget();

	if ( GetDisguiseTarget() )
	{
		m_iDisguiseHealth = GetDisguiseTarget()->GetHealth();
		if ( m_iDisguiseHealth <= 0 || !GetDisguiseTarget()->IsAlive() )
		{
			// If we disguised as an enemy who is currently dead, just set us to full health.
			m_iDisguiseHealth = GetDisguiseMaxHealth();
		}
	}
	else
	{
		int iMaxHealth = m_pOuter->GetMaxHealth();
		m_iDisguiseHealth = (int)random->RandomInt( iMaxHealth / 2, iMaxHealth );
	}

	// In Medieval mode, don't force primary weapon because most classes just have melee weapons
	DetermineDisguiseWeapon( true );
	DetermineDisguiseWearables();
#endif

	m_pOuter->TeamFortress_SetSpeed();

	m_flDisguiseCompleteTime = 0.0f;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetDisguiseHealth( int iDisguiseHealth )
{
	m_iDisguiseHealth = iDisguiseHealth;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayerShared::GetDisguiseMaxHealth( void )
{
	TFPlayerClassData_t *pClass = g_pTFPlayerClassDataMgr->Get( GetDisguiseClass() );
	if ( pClass )
	{
		return pClass->m_nMaxHealth;
	}
	else
	{
		return m_pOuter->GetMaxHealth();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::RemoveDisguise( void )
{
	if ( GetDisguiseTeam() != m_pOuter->GetTeamNumber() )
	{
		if ( InCond( TF_COND_TELEPORTED ) )
		{
			RemoveCond( TF_COND_TELEPORTED );
		}
	}

	RemoveCond( TF_COND_DISGUISED );
	RemoveCond( TF_COND_DISGUISING );
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::DetermineDisguiseWeapon( bool bForcePrimary )
{
	Assert( m_pOuter->GetPlayerClass()->GetClassIndex() == TF_CLASS_SPY );

	const char* strDisguiseWeapon = NULL;

	CTFPlayer *pDisguiseTarget = ToTFPlayer( m_hDisguiseTarget.Get() );
	TFPlayerClassData_t *pData = GetPlayerClassData( m_nDisguiseClass );
	if ( pDisguiseTarget && (pDisguiseTarget->GetPlayerClass()->GetClassIndex() != m_nDisguiseClass) )
	{
		pDisguiseTarget = NULL;
	}

	// Determine which slot we have active.
	int iCurrentSlot = 0;
	if ( m_pOuter->GetActiveTFWeapon() && !bForcePrimary )
	{
		iCurrentSlot = m_pOuter->GetActiveTFWeapon()->GetSlot();
		if ( (iCurrentSlot == 3) && // Cig Case, so they are using the menu not a key bind to disguise.
			m_pOuter->GetLastWeapon() )
		{
			iCurrentSlot = m_pOuter->GetLastWeapon()->GetSlot();
		}
	}

	CTFWeaponBase *pItemWeapon = NULL;
	if ( pDisguiseTarget )
	{
		CTFWeaponBase *pLastDisguiseWeapon = m_hDisguiseWeapon;
		CTFWeaponBase *pFirstValidWeapon = NULL;
		// Cycle through the target's weapons and see if we have a match.
		// Note that it's possible the disguise target doesn't have a weapon in the slot we want,
		// for example if they have replaced it with an unlockable that isn't a weapon (wearable).
		for ( int i=0; i<m_pOuter->m_hDisguiseWeaponList.Count(); ++i )
		{
			CTFWeaponBase *pWeapon = m_pOuter->m_hDisguiseWeaponList[i];

			if ( !pWeapon )
				continue;

			if ( !pFirstValidWeapon )
			{
				pFirstValidWeapon = pWeapon;
			}

			if ( pWeapon->GetSlot() == iCurrentSlot )
			{
				pItemWeapon = pWeapon;
				break;
			}
		}

		if ( !pItemWeapon )
		{
			if ( pLastDisguiseWeapon )
			{
				pItemWeapon = pLastDisguiseWeapon;
			}
			else if ( pFirstValidWeapon )
			{
				pItemWeapon = pFirstValidWeapon;
			}
		}

		if ( pItemWeapon )
		{
			strDisguiseWeapon = pItemWeapon->GetClassname();
		}
	}

	if ( !pItemWeapon && pData )
	{
		// We have not found our item yet, so cycle through the class's default weapons
		// to find a match.
		for ( int i=0; i<TF_PLAYER_WEAPON_COUNT; ++i )
		{
			if ( pData->m_aWeapons[i] == TF_WEAPON_NONE )
				continue;
			const char *pWpnName = WeaponIdToAlias( pData->m_aWeapons[i] );
			pWpnName = TranslateWeaponEntForClass( pWpnName, m_nDisguiseClass );
			WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( pWpnName );
			Assert( hWpnInfo != GetInvalidWeaponInfoHandle() );
			CTFWeaponInfo *pWeaponInfo = dynamic_cast<CTFWeaponInfo*>( GetFileWeaponInfoFromHandle( hWpnInfo ) );
			if ( pWeaponInfo->iSlot == iCurrentSlot )
			{
				strDisguiseWeapon = pWeaponInfo->szClassName;
			}
		}
	}

	if ( strDisguiseWeapon )
	{
		// Remove the old disguise weapon, if any.
		RemoveDisguiseWeapon();

		// We may need a sub-type if we're a builder. Otherwise we'll always appear as a engineer's workbox.
		int iSubType = 0;
		if ( Q_strcmp( strDisguiseWeapon, "tf_weapon_builder" ) == 0 )
		{
			return; // Temporary.
		}

		m_hDisguiseWeapon.Set( dynamic_cast<CTFWeaponBase*>(m_pOuter->GiveNamedItem( strDisguiseWeapon, iSubType, true )) );
		if ( m_hDisguiseWeapon )
		{
			m_hDisguiseWeapon->SetSolid( SOLID_NONE );
			m_hDisguiseWeapon->SetSolidFlags( FSOLID_NOT_SOLID );
			m_hDisguiseWeapon->SetTouch( NULL );// no touch
			m_hDisguiseWeapon->SetOwner( dynamic_cast<CBaseCombatCharacter*>(m_pOuter) );
			m_hDisguiseWeapon->SetOwnerEntity( m_pOuter );
			m_hDisguiseWeapon->SetParent( m_pOuter );
			m_hDisguiseWeapon->FollowEntity( m_pOuter, true );
			m_hDisguiseWeapon->m_iState = WEAPON_IS_ACTIVE;
			m_hDisguiseWeapon->m_bDisguiseWeapon = true;
			m_hDisguiseWeapon->SetContextThink( &CTFWeaponBase::DisguiseWeaponThink, gpGlobals->curtime + 0.5, "DisguiseWeaponThink" );


			// Ammo/clip state is displayed to attached medics
			m_iDisguiseAmmo = 0;
			if ( !m_hDisguiseWeapon->IsMeleeWeapon() )
			{
				// Use the player we're disguised as if possible
				if ( pDisguiseTarget )
				{
					CTFWeaponBase *pWeapon = pDisguiseTarget->GetActiveTFWeapon();
					if ( pWeapon && pWeapon->GetWeaponID() == m_hDisguiseWeapon->GetWeaponID() )
					{
						m_iDisguiseAmmo = pWeapon->UsesClipsForAmmo1() ? 
										  pWeapon->Clip1() : 
										  pDisguiseTarget->GetAmmoCount( pWeapon->GetPrimaryAmmoType() );
					}
				}

				// Otherwise display a faked ammo count
				if ( !m_iDisguiseAmmo )
				{
					int nMaxCount = m_hDisguiseWeapon->UsesClipsForAmmo1() ? 
									m_hDisguiseWeapon->GetMaxClip1() : 
									m_pOuter->GetMaxAmmo( m_hDisguiseWeapon->GetPrimaryAmmoType(), m_nDisguiseClass );
				
					m_iDisguiseAmmo = (int)random->RandomInt( 1, nMaxCount );
				}
			}
		}
	}
}

void CTFPlayerShared::DetermineDisguiseWearables()
{
	CTFPlayer *pDisguiseTarget = ToTFPlayer( m_hDisguiseTarget.Get() );
	if ( !pDisguiseTarget )
		return;

	if ( GetDisguiseClass() != pDisguiseTarget->GetPlayerClass()->GetClassIndex() )
		return;

	m_nDisguiseSkinOverride = 0;
}

#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::ProcessDisguiseImpulse( CTFPlayer *pPlayer )
{
	// Get the player owning the weapon.
	if ( !pPlayer )
		return;

	if ( pPlayer->GetImpulse() > 200 )
	{ 
		char szImpulse[6];
		Q_snprintf( szImpulse, sizeof( szImpulse ), "%d", pPlayer->GetImpulse() );

		char szTeam[3];
		Q_snprintf( szTeam, sizeof( szTeam ), "%c", szImpulse[1] );

		char szClass[3];
		Q_snprintf( szClass, sizeof( szClass ), "%c", szImpulse[2] );

		if ( pPlayer->CanDisguise() )
		{
			// intercepting the team value and reassigning what gets passed into Disguise()
			// because the team numbers in the client menu don't match the #define values for the teams
			pPlayer->m_Shared.Disguise( Q_atoi( szTeam ), Q_atoi( szClass ) );

			// Switch from the PDA to our previous weapon
			if ( GetActiveTFWeapon() && GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_PDA_SPY )
			{
				pPlayer->SelectLastItem();
			}
		}
	}
}

bool CTFPlayerShared::CanRecieveMedigunChargeEffect( void ) const
{
	bool bCanRecieve = true;

	const CTFItem *pItem = m_pOuter->GetItem();
	if ( pItem && pItem->GetItemID() == TF_ITEM_CAPTURE_FLAG )
	{
		bCanRecieve = false;
	}

	return bCanRecieve;
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: Heal players.
// pPlayer is person who healed us
//-----------------------------------------------------------------------------
void CTFPlayerShared::Heal( CBaseEntity *pHealer, float flAmount, float flOverhealBonus, float flOverhealDecayMult, bool bDispenserHeal /* = false */, CTFPlayer *pHealScorer /* = NULL */ )
{
	// If already healing, stop healing
	float flHealAccum = 0;
	if ( FindHealerIndex(pHealer) != m_aHealers.InvalidIndex() )
	{
		flHealAccum = StopHealing( pHealer );
	}

	healers_t newHealer;
	newHealer.pHealer = pHealer;
	newHealer.flAmount = flAmount;
	newHealer.flHealAccum = flHealAccum;
	newHealer.iKillsWhileBeingHealed = 0;
	newHealer.flOverhealBonus = flOverhealBonus;
	newHealer.flOverhealDecayMult = flOverhealDecayMult;
	newHealer.bDispenserHeal = bDispenserHeal;
	newHealer.flHealedLastSecond = 0;

	if ( pHealScorer )
	{
		newHealer.pHealScorer = pHealScorer;
	}
	else
	{
		//Assert( pHealer->IsPlayer() );
		newHealer.pHealScorer = pHealer;
	}

	m_aHealers.AddToTail( newHealer );

	AddCond( TF_COND_HEALTH_BUFF, PERMANENT_CONDITION, pHealer );

	RecalculateChargeEffects();

	m_nNumHealers = m_aHealers.Count();

	if ( pHealer && pHealer->IsPlayer() )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pHealer );
		Assert(pPlayer);
		pPlayer->m_AchievementData.AddTargetToHistory( m_pOuter );
		pPlayer->TeamFortress_SetSpeed();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Heal players.
// pPlayer is person who healed us
//-----------------------------------------------------------------------------
float CTFPlayerShared::StopHealing( CBaseEntity *pHealer )
{
	int iIndex = FindHealerIndex(pHealer);
	if ( iIndex == m_aHealers.InvalidIndex() )
		return 0;

	float flHealingDone = 0.f;

	if ( iIndex != m_aHealers.InvalidIndex() )
	{
		flHealingDone = m_aHealers[iIndex].flHealAccum;
		m_aHealers.Remove( iIndex );
	}

	if ( !m_aHealers.Count() )
	{
		RemoveCond( TF_COND_HEALTH_BUFF );
	}

	RecalculateChargeEffects();

	m_nNumHealers = m_aHealers.Count();

	return flHealingDone;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::RecalculateChargeEffects( bool bInstantRemove )
{
	bool bActive = false;
	CTFPlayer* pProvider = NULL;

	if ( m_pOuter->IsBeingCharged() )
	{
		bActive = true;
		pProvider = m_pOuter;
	}

	// Loop through our medics and get all their charges
	for ( int i = 0; i < m_aHealers.Count(); i++ )
	{
		if ( !m_aHealers[i].pHealer )
			continue;

		CTFPlayer *pPlayer = ToTFPlayer( m_aHealers[i].pHealer );
		if ( !pPlayer )
			continue;
		
		if ( pPlayer->IsBeingCharged() )
		{
			bActive = true;
			pProvider = pPlayer;
		}
	}

	if ( !CanRecieveMedigunChargeEffect() )
	{
		bActive = false;
	}

	SetChargeEffect( bActive, bInstantRemove, tf_invuln_time.GetFloat(), pProvider );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::TestAndExpireChargeEffect( void )
{
	if ( InCond( TF_COND_INVULNERABLE )  )
	{
		bool bRemoveEffect = false;
		bool bGameInWinState = TFGameRules()->State_Get() == GR_STATE_TEAM_WIN;
		bool bPlayerOnWinningTeam = TFGameRules()->GetWinningTeam() == m_pOuter->GetTeamNumber();

		// Lose all charge effects in post-win state if we're the losing team
		if ( bGameInWinState && !bPlayerOnWinningTeam )
		{
			bRemoveEffect = true;
		}

		if ( m_flChargeEffectOffTime )
		{
			if ( gpGlobals->curtime > m_flChargeEffectOffTime )
			{
				bRemoveEffect = true;
			}
		}

		// Check healers for possible usercommand invuln exploit
		FOR_EACH_VEC( m_aHealers, i )
		{
			CTFPlayer *pTFHealer = ToTFPlayer( m_aHealers[i].pHealer );
			if ( !pTFHealer )
				continue;

			CTFPlayer *pTFProvider = ToTFPlayer( GetConditionProvider( TF_COND_INVULNERABLE ) );
			if ( !pTFProvider )
				continue;

			if ( pTFProvider == pTFHealer && pTFHealer->GetTimeSinceLastUserCommand() > weapon_medigun_chargerelease_rate.GetFloat() + 1.f )
			{
				// Force remove uber and detach the medigun
				bRemoveEffect = true;
				pTFHealer->Weapon_Switch( pTFHealer->Weapon_GetSlot( TF_WPN_TYPE_MELEE ) );
			}
		}

		if ( bRemoveEffect )
		{
			m_flChargeEffectOffTime = 0;
			RemoveCond( TF_COND_INVULNERABLE );
				RemoveCond( TF_COND_INVULNERABLE_WEARINGOFF );
		}
	}
	else if ( m_bChargeSoundEffectsOn )
	{
		m_pOuter->StopSound( "TFPlayer.InvulnerableOn" );
		m_bChargeSoundEffectsOn = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: We've started a new charge effect
//-----------------------------------------------------------------------------
void CTFPlayerShared::SendNewInvulnGameEvent( void )
{
	// for each medic healing me
	for ( int i=0;i<m_aHealers.Count();i++ )
	{
		CTFPlayer *pMedic = ToTFPlayer( GetHealerByIndex(i) );
		if ( !pMedic )
			continue;

		// ACHIEVEMENT_TF_MEDIC_CHARGE_FRIENDS
		IGameEvent *event = gameeventmanager->CreateEvent( "player_invulned" );

		if ( event )
		{
			event->SetInt( "userid", m_pOuter->GetUserID() );
			event->SetInt( "medic_userid", pMedic->GetUserID() );
			gameeventmanager->FireEvent( event );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetChargeEffect( bool bState, bool bInstant, float flWearOffTime, CTFPlayer *pProvider /*= NULL*/ )
{
	bool bCurrentState = InCond( TF_COND_INVULNERABLE );
	if ( bCurrentState == bState )
	{
		if ( bState && m_flChargeEffectOffTime )
		{
			m_flChargeEffectOffTime = 0;
			RemoveCond( TF_COND_INVULNERABLE_WEARINGOFF );

			SendNewInvulnGameEvent();
		}
		return;
	}

	float flNonBotMaxDuration = weapon_medigun_chargerelease_rate.GetFloat();
	flNonBotMaxDuration += 1.0f;

	// Avoid infinite duration, because... the internet.
	float flMaxDuration = ( pProvider && pProvider->IsBot() ) ? PERMANENT_CONDITION : flNonBotMaxDuration;

	if ( bState )
	{
		if ( m_flChargeEffectOffTime )
		{
			m_pOuter->StopSound( "TFPlayer.InvulnerableOff" );

			m_flChargeEffectOffTime = 0;
			RemoveCond( TF_COND_INVULNERABLE_WEARINGOFF );
		}

		// Invulnerable turning on
		AddCond( TF_COND_INVULNERABLE, flMaxDuration, pProvider );

		SendNewInvulnGameEvent();

		CSingleUserRecipientFilter filter( m_pOuter );
		m_pOuter->EmitSound( filter, m_pOuter->entindex(), "TFPlayer.InvulnerableOn" );
		m_bChargeSoundEffectsOn = true;
	}
	else
	{
		if ( m_bChargeSoundEffectsOn )
		{
			m_pOuter->StopSound( "TFPlayer.InvulnerableOn" );
			m_bChargeSoundEffectsOn = false;
		}

		if ( !m_flChargeEffectOffTime )
		{
			// Make sure we don't have duplicate Off sounds playing
			m_pOuter->StopSound( "TFPlayer.InvulnerableOff" );

			CSingleUserRecipientFilter filter( m_pOuter );
			m_pOuter->EmitSound( filter, m_pOuter->entindex(), "TFPlayer.InvulnerableOff" );
		}

		if ( bInstant )
		{
			m_flChargeEffectOffTime = 0;
			RemoveCond( TF_COND_INVULNERABLE );
			RemoveCond( TF_COND_INVULNERABLE_WEARINGOFF );
		}
		else
		{
			// We're already in the process of turning it off
			if ( m_flChargeEffectOffTime )
				return;
			
			AddCond( TF_COND_INVULNERABLE_WEARINGOFF, PERMANENT_CONDITION, pProvider );
			m_flChargeEffectOffTime = gpGlobals->curtime + flWearOffTime;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFPlayerShared::FindHealerIndex( CBaseEntity *pHealer )
{
	for ( int i = 0; i < m_aHealers.Count(); i++ )
	{
		if ( m_aHealers[i].pHealer == pHealer )
			return i;
	}

	return m_aHealers.InvalidIndex();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CTFPlayerShared::GetHealerByIndex( int index )
{
	int iNumHealers = m_aHealers.Count();

	if ( index < 0 || index >= iNumHealers )
		return NULL;

	return m_aHealers[index].pHealer;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerShared::HealerIsDispenser( int index )
{
	int iNumHealers = m_aHealers.Count();

	if ( index < 0 || index >= iNumHealers )
		return false;

	return m_aHealers[index].bDispenserHeal;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the first healer in the healer array.  Note that this
//		is an arbitrary healer.
//-----------------------------------------------------------------------------
EHANDLE CTFPlayerShared::GetFirstHealer()
{
	if ( m_aHealers.Count() > 0 )
		return m_aHealers.Head().pHealer;

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: External code has decided that the trigger event for an achievement
//			has occurred. Go through our data and give it to the right people.
//-----------------------------------------------------------------------------
void CTFPlayerShared::CheckForAchievement( int iAchievement )
{
	if ( iAchievement == ACHIEVEMENT_TF_MEDIC_SAVE_TEAMMATE || 
		(iAchievement == ACHIEVEMENT_TF_MEDIC_CHARGE_BLOCKER && InCond( TF_COND_INVULNERABLE ) ) )
	{
		// ACHIEVEMENT_TF_MEDIC_SAVE_TEAMMATE : We were just saved from death by invuln. See if any medics deployed
		// their charge on us recently, and if so, give them the achievement.

		// ACHIEVEMENT_TF_MEDIC_CHARGE_BLOCKER: We just blocked a capture, and we're invuln. Whoever's invulning us gets the achievement.

		for ( int i = 0; i < m_aHealers.Count(); i++ )
		{
			CTFPlayer *pPlayer = ToTFPlayer( m_aHealers[i].pHealer );
			if ( !pPlayer )
				continue;

			if ( !pPlayer->IsPlayerClass(TF_CLASS_MEDIC) )
				continue;

			CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();
			if ( !pWpn )
				continue;

			CWeaponMedigun *pMedigun = dynamic_cast<CWeaponMedigun*>(pWpn);
			if ( pMedigun && pMedigun->IsReleasingCharge() )
			{
				// Save teammate requires us to have deployed the charge within the last second
				if ( iAchievement != ACHIEVEMENT_TF_MEDIC_SAVE_TEAMMATE || (gpGlobals->curtime - pMedigun->GetReleaseStartedAt()) < 1.0 )
				{
					pPlayer->AwardAchievement( iAchievement );
				}
			}
		}
	}
}

#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: Get all of our conditions in a nice CBitVec
//-----------------------------------------------------------------------------
void CTFPlayerShared::GetConditionsBits( CBitVec< TF_COND_LAST >& vbConditions ) const
{
	vbConditions.Set( 0u, (uint32)m_nPlayerCond );
	vbConditions.Set( 1u, (uint32)m_nPlayerCondEx );
	vbConditions.Set( 2u, (uint32)m_nPlayerCondEx2 );
	vbConditions.Set( 3u, (uint32)m_nPlayerCondEx3 );
	vbConditions.Set( 4u, (uint32)m_nPlayerCondEx4 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWeaponBase *CTFPlayerShared::GetActiveTFWeapon() const
{
	return m_pOuter->GetActiveTFWeapon();
}

//-----------------------------------------------------------------------------
// Purpose: Team check.
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsAlly( CBaseEntity *pEntity )
{
	return ( pEntity->GetTeamNumber() == m_pOuter->GetTeamNumber() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayerShared::GetDesiredPlayerClassIndex( void )
{
	return m_iDesiredPlayerClass;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetJumping( bool bJumping )
{
	m_bJumping = bJumping;
}

void CTFPlayerShared::SetAirDash( int iAirDash )
{
	m_iAirDash = iAirDash;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayerShared::GetCritMult( void )
{
	float flRemapCritMul = RemapValClamped( m_iCritMult, 0, 255, 1.0, 4.0 );
/*#ifdef CLIENT_DLL
	Msg("CLIENT: Crit mult %.2f - %d\n",flRemapCritMul, m_iCritMult);
#else
	Msg("SERVER: Crit mult %.2f - %d\n", flRemapCritMul, m_iCritMult );
#endif*/

	return flRemapCritMul;
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateCritMult( void )
{
	const float flMinMult = 1.0;
	const float flMaxMult = TF_DAMAGE_CRITMOD_MAXMULT;

	if ( m_DamageEvents.Count() == 0 )
	{
		m_iCritMult = RemapValClamped( flMinMult, 1.0, 4.0, 0, 255 );
		return;
	}

	//Msg( "Crit mult update for %s\n", m_pOuter->GetPlayerName() );
	//Msg( "   Entries: %d\n", m_DamageEvents.Count() );

	// Go through the damage multipliers and remove expired ones, while summing damage of the others
	float flTotalDamage = 0;
	for ( int i = m_DamageEvents.Count() - 1; i >= 0; i-- )
	{
		float flDelta = gpGlobals->curtime - m_DamageEvents[i].flTime;
		if ( flDelta > tf_damage_events_track_for.GetFloat() )
		{
			//Msg( "      Discarded (%d: time %.2f, now %.2f)\n", i, m_DamageEvents[i].flTime, gpGlobals->curtime );
			m_DamageEvents.Remove(i);
			continue;
		}

		// Ignore damage we've just done. We do this so that we have time to get those damage events
		// to the client in time for using them in prediction in this code.
		if ( flDelta < TF_DAMAGE_CRITMOD_MINTIME )
		{
			//Msg( "      Ignored (%d: time %.2f, now %.2f)\n", i, m_DamageEvents[i].flTime, gpGlobals->curtime );
			continue;
		}

		if ( flDelta > TF_DAMAGE_CRITMOD_MAXTIME )
			continue;

		//Msg( "      Added %.2f (%d: time %.2f, now %.2f)\n", m_DamageEvents[i].flDamage, i, m_DamageEvents[i].flTime, gpGlobals->curtime );

		flTotalDamage += m_DamageEvents[i].flDamage * m_DamageEvents[i].flDamageCritScaleMultiplier;
	}

	float flMult = RemapValClamped( flTotalDamage, 0, TF_DAMAGE_CRITMOD_DAMAGE, flMinMult, flMaxMult );

//	Msg( "   TotalDamage: %.2f   -> Mult %.2f\n", flTotalDamage, flMult );

	m_iCritMult = (int)RemapValClamped( flMult, flMinMult, flMaxMult, 0, 255 );
}

#define CRIT_DAMAGE_TIME		0.1f

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::RecordDamageEvent( const CTakeDamageInfo &info, bool bKill, int nVictimPrevHealth  )
{
	if ( m_DamageEvents.Count() >= MAX_DAMAGE_EVENTS )
	{
		// Remove the oldest event
		m_DamageEvents.Remove( m_DamageEvents.Count()-1 );
	}

	// Don't count critical damage toward the critical multiplier.
	float flDamage = info.GetDamage() - info.GetDamageBonus();

	float flDamageCriticalScale = info.GetDamageType() & DMG_DONT_COUNT_DAMAGE_TOWARDS_CRIT_RATE
								? 0.0f
								: 1.0f;

	// cap the damage at our current health amount since it's going to kill us
	if ( bKill && flDamage > nVictimPrevHealth )
	{
		flDamage = nVictimPrevHealth;
	}

	// Don't allow explosions to stack up damage toward the critical modifier.
	bool bOverride = false;
	if ( info.GetDamageType() & DMG_BLAST )
	{
		int nDamageCount = m_DamageEvents.Count();
		for ( int iDamage = 0; iDamage < nDamageCount; ++iDamage )
		{
			// Was the older event I am checking against an explosion as well?
			if ( m_DamageEvents[iDamage].nDamageType & DMG_BLAST )
			{
				// Did it happen very recently?
				if ( ( gpGlobals->curtime - m_DamageEvents[iDamage].flTime ) < CRIT_DAMAGE_TIME )
				{
					if ( bKill )
					{
						m_DamageEvents[iDamage].nKills++;

						if ( m_pOuter->IsPlayerClass( TF_CLASS_DEMOMAN ) )
						{
							// Make sure the previous & the current are stickybombs, and go with it.
							if ( m_DamageEvents[iDamage].nDamageType == info.GetDamageType() &&
								m_DamageEvents[iDamage].nDamageType == g_aWeaponDamageTypes[TF_WEAPON_PIPEBOMBLAUNCHER] )
							{
								if ( m_DamageEvents[iDamage].nKills >= 3 )
								{
									m_pOuter->AwardAchievement( ACHIEVEMENT_TF_DEMOMAN_KILL3_WITH_DETONATION );
								}
							}
						}
					}

					// Take the max damage done in the time frame.
					if ( flDamage > m_DamageEvents[iDamage].flDamage )
					{
						m_DamageEvents[iDamage].flDamage = flDamage;
						m_DamageEvents[iDamage].flDamageCritScaleMultiplier = flDamageCriticalScale;
						m_DamageEvents[iDamage].flTime = gpGlobals->curtime;
						m_DamageEvents[iDamage].nDamageType = info.GetDamageType();

//						Msg( "Update Damage Event: D:%f, T:%f\n", m_DamageEvents[iDamage].flDamage, m_DamageEvents[iDamage].flTime );
					}

					bOverride = true;
				}
			}
		}
	}

	// We overrode a value, don't add this to the list.
	if ( bOverride )
		return;

	int iIndex = m_DamageEvents.AddToTail();
	m_DamageEvents[iIndex].flDamage = flDamage;
	m_DamageEvents[iIndex].flDamageCritScaleMultiplier = flDamageCriticalScale;
	m_DamageEvents[iIndex].nDamageType = info.GetDamageType();
	m_DamageEvents[iIndex].flTime = gpGlobals->curtime;
	m_DamageEvents[iIndex].nKills = bKill;

//	Msg( "Damage Event: D:%f, T:%f\n", m_DamageEvents[iIndex].flDamage, m_DamageEvents[iIndex].flTime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::AddTempCritBonus( float flAmount )
{
	if ( m_DamageEvents.Count() >= MAX_DAMAGE_EVENTS )
	{
		// Remove the oldest event
		m_DamageEvents.Remove( m_DamageEvents.Count()-1 );
	}

	int iIndex = m_DamageEvents.AddToTail();
	m_DamageEvents[iIndex].flDamage = RemapValClamped( flAmount, 0, 1, 0, TF_DAMAGE_CRITMOD_DAMAGE ) / (TF_DAMAGE_CRITMOD_MAXMULT - 1.0);
	m_DamageEvents[iIndex].flDamageCritScaleMultiplier = 1.0f;
	m_DamageEvents[iIndex].nDamageType = DMG_GENERIC;
	m_DamageEvents[iIndex].flTime = gpGlobals->curtime;
	m_DamageEvents[iIndex].nKills = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFPlayerShared::GetNumKillsInTime( float flTime )
{
	if ( tf_damage_events_track_for.GetFloat() < flTime )
	{
		Warning("Player asking for damage events for time %.0f, but tf_damage_events_track_for is only tracking events for %.0f\n", flTime, tf_damage_events_track_for.GetFloat() );
	}

	int iKills = 0;
	for ( int i = m_DamageEvents.Count() - 1; i >= 0; i-- )
	{
		float flDelta = gpGlobals->curtime - m_DamageEvents[i].flTime;
		if ( flDelta < flTime )
		{
			iKills += m_DamageEvents[i].nKills;
		}
	}

	return iKills;
}

#endif

//=============================================================================
//
// Shared player code that isn't CTFPlayerShared
//
//-----------------------------------------------------------------------------
struct penetrated_target_list
{
	CBaseEntity *pTarget;
	float flDistanceFraction;
};

//-----------------------------------------------------------------------------
class CBulletPenetrateEnum : public IEntityEnumerator
{
public:
	CBulletPenetrateEnum( Vector vecStart, Vector vecEnd, CBaseEntity *pShooter, int nCustomDamageType, bool bIgnoreTeammates = true )
	{
		m_vecStart = vecStart;
		m_vecEnd = vecEnd;
		m_pShooter = pShooter;
		m_nCustomDamageType = nCustomDamageType;
		m_bIgnoreTeammates = bIgnoreTeammates;
	}

	// We need to sort the penetrated targets into order, with the closest target first
	class PenetratedTargetLess
	{
	public:
		bool Less( const penetrated_target_list &src1, const penetrated_target_list &src2, void *pCtx )
		{
			return src1.flDistanceFraction < src2.flDistanceFraction;
		}
	};

	virtual bool EnumEntity( IHandleEntity *pHandleEntity )
	{
		trace_t tr;

		CBaseEntity *pEnt = static_cast<CBaseEntity*>(pHandleEntity);

		// Ignore collisions with the shooter
		if ( pEnt == m_pShooter )
			return true;

		if ( pEnt->IsCombatCharacter() || pEnt->IsBaseObject() )
		{
			if ( m_bIgnoreTeammates && pEnt->GetTeam() == m_pShooter->GetTeam() )
				return true;

			Ray_t ray;
			ray.Init( m_vecStart, m_vecEnd );
			enginetrace->ClipRayToEntity( ray, MASK_SOLID | CONTENTS_HITBOX, pHandleEntity, &tr );

			if (tr.fraction < 1.0f)
			{
				penetrated_target_list newEntry;
				newEntry.pTarget = pEnt;
				newEntry.flDistanceFraction = tr.fraction;
				m_Targets.Insert( newEntry );
				return true;
			}
		}

		return true;
	}

public:
	Vector		 m_vecStart;
	Vector		 m_vecEnd;
	int			 m_nCustomDamageType;
	CBaseEntity	*m_pShooter;
	bool		 m_bIgnoreTeammates;
	CUtlSortVector<penetrated_target_list, PenetratedTargetLess> m_Targets;
};


CTargetOnlyFilter::CTargetOnlyFilter( CBaseEntity *pShooter, CBaseEntity *pTarget ) 
	: CTraceFilterSimple( pShooter, COLLISION_GROUP_NONE )
{
	m_pShooter = pShooter;
	m_pTarget = pTarget;
}

bool CTargetOnlyFilter::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	CBaseEntity *pEnt = static_cast<CBaseEntity*>(pHandleEntity);

	if ( pEnt && pEnt == m_pTarget ) 
		return true;
	else if ( !pEnt || pEnt != m_pTarget )
	{
		// If we hit a solid piece of the world, we're done.
		if ( pEnt->IsBSPModel() && pEnt->IsSolid() )
			return CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask );
		return false;
	}
	else
		return CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask );
}

static bool OnOpposingTFTeams( int iTeam0, int iTeam1 )
{
	// This logic is weird because we want to make sure that we're actually shooting someone on the
	// other team, not just someone on a different team. This prevents weirdness where we count shooting
	// the BSP as an enemy because they aren't on our team.

	if ( iTeam0 == TF_TEAM_BLUE	)				// if we're on the blue team...
		return iTeam1 == TF_TEAM_RED;			// ...and we shot someone on the red team, then we're opposing.

	if ( iTeam0 == TF_TEAM_RED	)				// if we're on the blue team...
		return iTeam1 == TF_TEAM_BLUE;			// ...and we shot someone on the red team, then we're opposing.

	return iTeam0 != iTeam1;					// if we're neither red nor blue, then anyone different from us is opposing
}

// Josh: Go on, pick a better name!
class CTraceFilterIgnoreAllPlayersExceptThisOne : public CTraceFilterSimple
{
public:
	DECLARE_CLASS( CTraceFilterIgnoreAllPlayersExceptThisOne, CTraceFilterSimple );

	CTraceFilterIgnoreAllPlayersExceptThisOne( const IHandleEntity* passentity, int collisionGroup )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity* pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );
		const CBaseEntity *pPassEntity = EntityFromEntityHandle( GetPassEntity() );

		if ( pEntity->IsPlayer() )
			return pEntity == pPassEntity;

		return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
	}
};

// Josh:
// This function exists to handle hitting hitboxes outside of the player's
// bbox in a way that works for TF2.
// 
// One of the major problems it that if a player gets up against a thin wall or
// door, you can shoot them through it if their hitbox is outside of their bbox
//
// This fixes that problem in a simple way by tracing from the hitbox hit point
// to the player's origin at the hitbox hit height and checking if it hit the
// player or not.
static void UTIL_PlayerBulletTrace( const Vector& vecStart, const Vector& vecEnd, const Vector &vecDir, unsigned int mask, ITraceFilter* pFilter, trace_t* trace )
{
	UTIL_TraceLine( vecStart, vecEnd, mask | CONTENTS_HITBOX, pFilter, trace );

	if ( !trace->startsolid && !trace->DidHitNonWorldEntity() )
	{
		// Josh: Extend the ray length by ~ size of the bbox if we are clipping
		// to the player to keep the ray length consistent with the normal bbox path.
		const float rayExtension = 40.0f;

		trace_t playerClipTrace;
		memcpy( &playerClipTrace, trace, sizeof( trace_t ) );

		UTIL_ClipTraceToPlayers( vecStart, vecEnd + vecDir * rayExtension, mask | CONTENTS_HITBOX, pFilter, &playerClipTrace );
		if ( playerClipTrace.m_pEnt )
		{
			Vector entOrigin = playerClipTrace.m_pEnt->GetAbsOrigin();

			CTraceFilterIgnoreAllPlayersExceptThisOne playerClipValidationFilter( playerClipTrace.m_pEnt, COLLISION_GROUP_NONE );
			// Josh:
			// Trace from the hitbox to the player origin at hitbox hit height to see if they are obstructed by the world.
			// Don't use hitboxes on this trace.
			//
			// It's unfair if a player is poking through a thin wall and gets shot! :(
			trace_t playerClipValidationTrace;
			UTIL_TraceLine( playerClipTrace.endpos, Vector( entOrigin.x, entOrigin.y, playerClipTrace.endpos.z ), mask, &playerClipValidationFilter, &playerClipValidationTrace );

			if ( playerClipValidationTrace.m_pEnt == playerClipTrace.m_pEnt )
				memcpy( trace, &playerClipTrace, sizeof( trace_t ) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::FireBullet( CTFWeaponBase *pWpn, const FireBulletsInfo_t &info, bool bDoEffects, int nDamageType, int nCustomDamageType /*= TF_DMG_CUSTOM_NONE*/ )
{
	// Fire a bullet (ignoring the shooter).
	Vector vecStart = info.m_vecSrc;
	Vector vecEnd = vecStart + info.m_vecDirShooting * info.m_flDistance;
	trace_t trace;

	// Ignore teammates and their (physical) upgrade items when shooting in MvM
	CTraceFilterSimple traceFilter( this, COLLISION_GROUP_NONE );
	UTIL_PlayerBulletTrace( vecStart, vecEnd, info.m_vecDirShooting, MASK_SOLID, &traceFilter, &trace );
	
#ifndef CLIENT_DLL
	CTakeDamageInfo dmgInfo( this, info.m_pAttacker, info.m_flDamage, nDamageType );
	dmgInfo.SetWeapon( GetActiveWeapon() );
	dmgInfo.SetDamageCustom( nCustomDamageType );

	int iEnemyPlayersHit = 0;
	// Damage only the first entity encountered on the bullet's path.
	if ( trace.m_pEnt )
	{
		CalculateBulletDamageForce( &dmgInfo, info.m_iAmmoType, info.m_vecDirShooting, trace.endpos, 1.0 );
		trace.m_pEnt->DispatchTraceAttack( dmgInfo, info.m_vecDirShooting, &trace );
		if ( trace.m_pEnt->IsPlayer() && OnOpposingTFTeams( GetTeamNumber(), trace.m_pEnt->GetTeamNumber() ) )
		{
			iEnemyPlayersHit++;
		}
	}

	if ( pWpn )
	{
		if ( iEnemyPlayersHit )
		{	// Guarantee that the bullet that hit an enemy trumps the player viewangles
			// that are locked in for the duration of the server simulation ticks
			m_iLockViewanglesTickNumber = gpGlobals->tickcount;
			m_qangLockViewangles = pl.v_angle;
		}
	}
#endif

	if ( trace.fraction < 1.0 )
	{
		// Verify we have an entity at the point of impact.
		Assert( trace.m_pEnt );

		if ( bDoEffects )
		{
			// If shot starts out of water and ends in water
			if ( !( enginetrace->GetPointContents( trace.startpos ) & ( CONTENTS_WATER | CONTENTS_SLIME ) ) &&
				( enginetrace->GetPointContents( trace.endpos ) & ( CONTENTS_WATER | CONTENTS_SLIME ) ) )
			{	
				// Water impact effects.
				ImpactWaterTrace( trace, vecStart );
			}
			else
			{
				// Regular impact effects.

				// don't decal your teammates or objects on your team
				if ( trace.m_pEnt && trace.m_pEnt->GetTeamNumber() != GetTeamNumber() )
				{
					UTIL_ImpactTrace( &trace, nDamageType );
				}
			}
			
#ifdef CLIENT_DLL
			static int	tracerCount;
			if ( ( ( info.m_iTracerFreq != 0 ) && ( tracerCount++ % info.m_iTracerFreq ) == 0 ) )
			{
				// if this is a local player, start at attachment on view model
				// else start on attachment on weapon model
				int iUseAttachment = TRACER_DONT_USE_ATTACHMENT;
				int iAttachment = 1;

				{
					C_BaseCombatWeapon *pWeapon = GetActiveWeapon();

					if ( pWeapon )
					{
						iAttachment = pWeapon->LookupAttachment( "muzzle" );
					}
				}


				C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

				bool bInToolRecordingMode = clienttools->IsInRecordingMode();

				// If we're using a viewmodel, override vecStart with the muzzle of that - just for the visual effect, not gameplay.
				if ( ( pLocalPlayer != NULL ) && !pLocalPlayer->ShouldDrawThisPlayer() && !bInToolRecordingMode && pWpn )
				{
					C_BaseAnimating *pAttachEnt = pWpn->GetAppropriateWorldOrViewModel();
					if ( pAttachEnt != NULL )
					{
						pAttachEnt->GetAttachment( iAttachment, vecStart );
					}
				}
				else if ( !IsDormant() )
				{
					// fill in with third person weapon model index
					C_BaseCombatWeapon *pWeapon = GetActiveWeapon();

					if( pWeapon )
					{
						int nModelIndex = pWeapon->GetModelIndex();
						int nWorldModelIndex = pWeapon->GetWorldModelIndex();
						if ( bInToolRecordingMode && nModelIndex != nWorldModelIndex )
						{
							pWeapon->SetModelIndex( nWorldModelIndex );
						}

						pWeapon->GetAttachment( iAttachment, vecStart );

						if ( bInToolRecordingMode && nModelIndex != nWorldModelIndex )
						{
							pWeapon->SetModelIndex( nModelIndex );
						}
					}
				}

				if ( tf_useparticletracers.GetBool() )
				{
					const char *pszTracerEffect = GetTracerType();
					if ( pszTracerEffect && pszTracerEffect[0] )
					{
						char szTracerEffect[128];
						if ( nDamageType & DMG_CRITICAL )
						{
							Q_snprintf( szTracerEffect, sizeof(szTracerEffect), "%s_crit", pszTracerEffect );
							pszTracerEffect = szTracerEffect;
						}

						UTIL_ParticleTracer( pszTracerEffect, vecStart, trace.endpos, entindex(), iUseAttachment, true );
					}
				}
				else
				{
					UTIL_Tracer( vecStart, trace.endpos, entindex(), iUseAttachment, 5000, true, GetTracerType() );
				}
			}
#endif
		}
	}
}

#ifdef CLIENT_DLL
static ConVar tf_impactwatertimeenable( "tf_impactwatertimeenable", "0", FCVAR_CHEAT, "Draw impact debris effects." );
static ConVar tf_impactwatertime( "tf_impactwatertime", "1.0f", FCVAR_CHEAT, "Draw impact debris effects." );
#endif

//-----------------------------------------------------------------------------
// Purpose: Trace from the shooter to the point of impact (another player,
//          world, etc.), but this time take into account water/slime surfaces.
//   Input: trace - initial trace from player to point of impact
//          vecStart - starting point of the trace 
//-----------------------------------------------------------------------------
void CTFPlayer::ImpactWaterTrace( trace_t &trace, const Vector &vecStart )
{
#ifdef CLIENT_DLL
	if ( tf_impactwatertimeenable.GetBool() )
	{
		if ( m_flWaterImpactTime > gpGlobals->curtime )
			return;
	}
#endif 

	trace_t traceWater;
	UTIL_TraceLine( vecStart, trace.endpos, ( MASK_SHOT | CONTENTS_WATER | CONTENTS_SLIME ), 
		this, COLLISION_GROUP_NONE, &traceWater );
	if( traceWater.fraction < 1.0f )
	{
		CEffectData	data;
		data.m_vOrigin = traceWater.endpos;
		data.m_vNormal = traceWater.plane.normal;
		data.m_flScale = random->RandomFloat( 8, 12 );
		if ( traceWater.contents & CONTENTS_SLIME )
		{
			data.m_fFlags |= FX_WATER_IN_SLIME;
		}

		const char *pszEffectName = "tf_gunshotsplash";
		CTFWeaponBase *pWeapon = GetActiveTFWeapon();
		if ( pWeapon && ( TF_WEAPON_MINIGUN == pWeapon->GetWeaponID() ) )
		{
			// for the minigun, use a different, cheaper splash effect because it can create so many of them
			pszEffectName = "tf_gunshotsplash_minigun";
		}		
		DispatchEffect( pszEffectName, data );

#ifdef CLIENT_DLL
		if ( tf_impactwatertimeenable.GetBool() )
		{
			m_flWaterImpactTime = gpGlobals->curtime + tf_impactwatertime.GetFloat();
		}
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWeaponBase *CTFPlayer::GetActiveTFWeapon( void ) const
{
	CBaseCombatWeapon *pRet = GetActiveWeapon();
	if ( pRet )
	{
		Assert( dynamic_cast< CTFWeaponBase* >( pRet ) != NULL );
		return static_cast< CTFWeaponBase * >( pRet );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: How much build resource ( metal ) does this player have
//-----------------------------------------------------------------------------
int CTFPlayer::GetBuildResources( void )
{
	return GetAmmoCount( TF_AMMO_METAL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanPlayerMove() const
{
	if ( GetMoveType() == MOVETYPE_OBSERVER )
		return true;

	if ( GetMoveType() == MOVETYPE_NOCLIP )
		return true;

	// No one can move when in a final countdown transition.
	if ( TFGameRules() && TFGameRules()->BInMatchStartCountdown() )
		return false;

	bool bFreezeOnRestart = tf_player_movement_restart_freeze.GetBool();
	if ( bFreezeOnRestart )
	{
#if defined( _DEBUG ) || defined( STAGING_ONLY )
		if ( mp_developer.GetBool() )
			bFreezeOnRestart = false;
#endif // _DEBUG || STAGING_ONLY

	if ( TFGameRules() && TFGameRules()->UsePlayerReadyStatusMode() && ( TFGameRules()->State_Get() == GR_STATE_BETWEEN_RNDS ) )
		bFreezeOnRestart = false;
	}

	bool bInRoundRestart = TFGameRules() && TFGameRules()->InRoundRestart();
	if ( bInRoundRestart && TFGameRules()->IsCompetitiveMode() )
	{
		if ( TFGameRules()->GetRoundsPlayed() > 0 )
		{
			if ( gpGlobals->curtime < TFGameRules()->GetPreroundCountdownTime() )
			{
				bFreezeOnRestart = true;
			}
		}
		else
		{
			bFreezeOnRestart = false;
		}
	}

	bool bNoMovement = bInRoundRestart && bFreezeOnRestart;

	return !bNoMovement;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayer::TeamFortress_CalculateMaxSpeed( bool bIgnoreSpecialAbility /*= false*/ ) const
{
	if ( !GameRules() )
		return 0.0f;

	int playerclass = GetPlayerClass()->GetClassIndex();

	// Spectators can move while in Classic Observer mode
	if ( IsObserver() )
	{
		if ( GetObserverMode() == OBS_MODE_ROAMING )
			return GetPlayerClassData( TF_CLASS_SCOUT )->m_flMaxSpeed;

		return 0.0f;
	}

	// Check for any reason why they can't move at all
	if ( playerclass == TF_CLASS_UNDEFINED || !CanPlayerMove() )
		return 1.0f;					// this can't return 0 because other parts of the code interpret that as "use default speed" during setup

	// First, get their max class speed
	float default_speed = GetPlayerClassData( playerclass )->m_flMaxSpeed;

	// Slow us down if we're disguised as a slower class
	// unless we're cloaked..
	float maxfbspeed = default_speed;

	if ( m_Shared.InCond( TF_COND_DISGUISED ) && !m_Shared.IsStealthed() )
	{
		float flMaxDisguiseSpeed = GetPlayerClassData( m_Shared.GetDisguiseClass() )->m_flMaxSpeed;
		maxfbspeed = MIN( flMaxDisguiseSpeed, maxfbspeed );
	}

	// if they're a sniper, and they're aiming, their speed must be 80 or less
	if ( m_Shared.InCond( TF_COND_AIMING ) )
	{
		float flAimMax = 80;
		maxfbspeed = MIN( maxfbspeed, flAimMax );
	}

	if ( m_Shared.InCond( TF_COND_STEALTHED ) )
	{
		if (maxfbspeed > tf_spy_max_cloaked_speed.GetFloat() )
		{
			maxfbspeed = tf_spy_max_cloaked_speed.GetFloat();
		}
	}

	// if we're in bonus time because a team has won, give the winners 110% speed and the losers 90% speed
	if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
	{
		int iWinner = TFGameRules()->GetWinningTeam();
		
		if ( iWinner != TEAM_UNASSIGNED )
		{
			if ( iWinner == GetTeamNumber() )
			{
				maxfbspeed *= 1.1f;
			}
			else
			{
				maxfbspeed *= 0.9f;
			}
		}
	}
	
	return maxfbspeed;
}

void CTFPlayer::TeamFortress_SetSpeed()
{
	const float fMaxSpeed = TeamFortress_CalculateMaxSpeed();

	// Set the speed
	SetMaxSpeed( fMaxSpeed );

	if ( fMaxSpeed <= 0.0f )
	{
		SetAbsVelocity( vec3_origin );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayer::HasItem( void ) const
{
	return ( m_hItem != NULL );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::SetItem( CTFItem *pItem )
{
	m_hItem = pItem;

#ifndef CLIENT_DLL
	if ( pItem && pItem->GetItemID() == TF_ITEM_CAPTURE_FLAG )
	{
		RemoveInvisibility();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFItem	*CTFPlayer::GetItem( void ) const
{
	return m_hItem;
}

//-----------------------------------------------------------------------------
// Purpose: Is the player carrying the flag?
//-----------------------------------------------------------------------------
bool CTFPlayer::HasTheFlag( ETFFlagType exceptionTypes[], int nNumExceptions ) const
{
	if ( HasItem() && GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG )
	{
		CCaptureFlag* pFlag = static_cast< CCaptureFlag* >( GetItem() );

		for( int i=0; i < nNumExceptions; ++i )
		{
			if ( exceptionTypes[ i ] == pFlag->GetType() )
				return false;
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCaptureZone *CTFPlayer::GetCaptureZoneStandingOn( void )
{
	touchlink_t *root = ( touchlink_t * )GetDataObject( TOUCHLINK );
	if ( root )
	{
		for ( touchlink_t *link = root->nextLink; link != root; link = link->nextLink )
		{
			CBaseEntity *pTouch = link->entityTouched;
			if ( pTouch && pTouch->IsSolidFlagSet( FSOLID_TRIGGER ) && pTouch->IsBSPModel() )
			{
				CCaptureZone *pAreaTrigger = dynamic_cast< CCaptureZone* >(pTouch);
				if ( pAreaTrigger )
				{
					return pAreaTrigger;
				}
			}
		}
	}

	return NULL;
}

CCaptureZone *CTFPlayer::GetClosestCaptureZone( void )
{
	CCaptureZone *pCaptureZone = NULL;
	float flClosestDistance = FLT_MAX;

	for ( int i=0; i<ICaptureZoneAutoList::AutoList().Count(); ++i )
	{
		CCaptureZone *pTempCaptureZone = static_cast< CCaptureZone* >( ICaptureZoneAutoList::AutoList()[i] );
		if ( !pTempCaptureZone->IsDisabled() && pTempCaptureZone->GetTeamNumber() == GetTeamNumber() )
		{
			float fCurrentDistance = GetAbsOrigin().DistTo( pTempCaptureZone->WorldSpaceCenter() );
			if ( flClosestDistance > fCurrentDistance )
			{
				pCaptureZone = pTempCaptureZone;
				flClosestDistance = fCurrentDistance;
			}
		}
	}

	return pCaptureZone;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this player's allowed to build another one of the specified object
//-----------------------------------------------------------------------------
int CTFPlayer::CanBuild( int iObjectType, int iObjectMode )
{
	if ( iObjectType < 0 || iObjectType >= OBJ_LAST )
		return CB_UNKNOWN_OBJECT;

	const CObjectInfo *pInfo = GetObjectInfo( iObjectType );
	if ( pInfo && ((iObjectMode > pInfo->m_iNumAltModes) || (iObjectMode < 0)) )
		return CB_CANNOT_BUILD;

	// Does this type require a specific builder?
	bool bHasSubType = false;
	CTFWeaponBuilder *pBuilder = CTFPlayerSharedUtils::GetBuilderForObjectType( this, iObjectType );
	if ( pBuilder )
	{
		bHasSubType = true;
	}

#ifndef CLIENT_DLL
	CTFPlayerClass *pCls = GetPlayerClass();

	if ( !bHasSubType && pCls && pCls->CanBuildObject( iObjectType ) == false )
	{
		return CB_CANNOT_BUILD;
	}
#endif

	// Make sure we haven't hit maximum number
	int iObjectCount = GetNumObjects( iObjectType, iObjectMode );
	if ( iObjectCount >= GetObjectInfo( iObjectType )->m_nMaxObjects && GetObjectInfo( iObjectType )->m_nMaxObjects != -1)
	{
		return CB_LIMIT_REACHED;
	}

	// Find out how much the object should cost
	int iCost = m_Shared.CalculateObjectCost( this, iObjectType );

	// Make sure we have enough resources
	if ( GetBuildResources() < iCost )
	{
		return CB_NEED_RESOURCES;
	}

	return CB_CAN_BUILD;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsAiming( void )
{
	if ( !m_pOuter )
		return false;

	bool bAiming = InCond( TF_COND_AIMING ) && !m_pOuter->IsPlayerClass( TF_CLASS_SOLDIER );

	return bAiming;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayerShared::CalculateObjectCost( CTFPlayer* pBuilder, int iObjectType )
{
	return InternalCalculateObjectCost( iObjectType );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::HealthKitPickupEffects( int iHealthGiven /*= 0*/ )
{
	// Healthkits also contain a fire blanket.
	if ( InCond( TF_COND_BURNING ) )
	{
		RemoveCond( TF_COND_BURNING );		
	}

	// Spawns a number on the player's health bar in the HUD, and also
	// spawns a "+" particle over their head for enemies to see
	if ( iHealthGiven && !IsStealthed() && m_pOuter )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "player_healonhit" );
		if ( event )
		{
			event->SetInt( "amount", iHealthGiven );
			event->SetInt( "entindex", m_pOuter->entindex() );
			gameeventmanager->FireEvent( event ); 
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get the number of objects of the specified type that this player has
//-----------------------------------------------------------------------------
int CTFPlayer::GetNumObjects( int iObjectType, int iObjectMode /*= 0*/ )
{
	int iCount = 0;
	for (int i = 0; i < GetObjectCount(); i++)
	{
		if ( !GetObject(i) )
			continue;

		if ( GetObject(i)->GetType() == iObjectType && 
			( GetObject(i)->GetObjectMode() == iObjectMode || iObjectMode == BUILDING_MODE_ANY ) )
		{
			iCount++;
		}
	}

	return iCount;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ItemPostFrame()
{
	// cache buttons because some weapons' ItemPostFrame could change m_nButtons against other weapons
	int nButtons = m_nButtons;

	if ( m_hOffHandWeapon.Get() && m_hOffHandWeapon->IsWeaponVisible() )
	{
		if ( gpGlobals->curtime < m_flNextAttack )
		{
			m_hOffHandWeapon->ItemBusyFrame();
		}
		else
		{
#if defined( CLIENT_DLL )
			// Not predicting this weapon
			if ( m_hOffHandWeapon->IsPredicted() )
#endif
			{
				m_hOffHandWeapon->ItemPostFrame( );
			}
		}
	}

	BaseClass::ItemPostFrame();

	// restore m_nButtons so all the afButtons are in correct state
	m_nButtons = nButtons;
}

void CTFPlayer::SetOffHandWeapon( CTFWeaponBase *pWeapon )
{
	m_hOffHandWeapon = pWeapon;
	if ( m_hOffHandWeapon.Get() )
	{
		m_hOffHandWeapon->Deploy();
	}
}

// Set to NULL at the end of the holster?
void CTFPlayer::HolsterOffHandWeapon( void )
{
	if ( m_hOffHandWeapon.Get() )
	{
		m_hOffHandWeapon->Holster();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return true if we should record our last weapon when switching between the two specified weapons
//-----------------------------------------------------------------------------
bool CTFPlayer::Weapon_ShouldSetLast( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon )
{
	// if the weapon doesn't want to be auto-switched to, don't!
	CTFWeaponBase *pTFOldWeapon = dynamic_cast< CTFWeaponBase * >( pOldWeapon );
	if ( pTFOldWeapon )
	{
		if ( pTFOldWeapon->AllowsAutoSwitchTo() == false )
		{
			return false;
		}
	}

	return BaseClass::Weapon_ShouldSetLast( pOldWeapon, pNewWeapon );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::SelectItem( const char *pstr, int iSubType /*= 0*/ )
{
	// This is basically a copy from the base class with addition of Weapon_CanSwitchTo
	// We're not calling BaseClass::SelectItem on purpose to prevent breaking other games
	// that might rely on not calling Weapon_CanSwitchTo

	if (!pstr)
		return;

	CBaseCombatWeapon *pItem = Weapon_OwnsThisType( pstr, iSubType );

	if (!pItem)
		return;

	if( GetObserverMode() != OBS_MODE_NONE )
		return;// Observers can't select things.

	if ( !Weapon_ShouldSelectItem( pItem ) )
		return;

	// FIX, this needs to queue them up and delay
	// Make sure the current weapon can be holstered
	if ( !Weapon_CanSwitchTo( pItem ) )
		return;

	ResetAutoaim();

	Weapon_Switch( pItem );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayer::Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex )
{
	// set last weapon before we switch to a new weapon to make sure that we can get the correct last weapon in Deploy/Holster
	// This should be done in CBasePlayer::Weapon_Switch, but we don't want to break other games
	CBaseCombatWeapon *pPreviousLastWeapon = GetLastWeapon();
	CBaseCombatWeapon *pPreviousActiveWeapon = GetActiveWeapon();

	// always set last for Weapon_Switch code to get attribute from the correct last item
	Weapon_SetLast( GetActiveWeapon() );

	bool bSwitched = BaseClass::Weapon_Switch( pWeapon, viewmodelindex );
	if ( bSwitched )
	{
		m_PlayerAnimState->ResetGestureSlot( GESTURE_SLOT_ATTACK_AND_RELOAD );

		// valid last weapon
		if ( Weapon_ShouldSetLast( pPreviousActiveWeapon, pWeapon ) )
		{
			Weapon_SetLast( pPreviousActiveWeapon );
			SetSecondaryLastWeapon( pPreviousLastWeapon );
		}
		// previous active weapon is not valid to be last weapon, but the new active weapon is
		else if ( Weapon_ShouldSetLast( pWeapon, pPreviousLastWeapon ) )
		{
			if ( pWeapon != GetSecondaryLastWeapon() )
			{
				Weapon_SetLast( GetSecondaryLastWeapon() );
				SetSecondaryLastWeapon( pPreviousLastWeapon );
			}
			else
			{
				// new active weapon is the same as the secondary last weapon, leave the last weapon alone
				Weapon_SetLast( pPreviousLastWeapon );
			}
		}
		// both previous and new active weapons are not not valid for last weapon
		else
		{
			Weapon_SetLast( pPreviousLastWeapon );
		}
	}
	else
	{
		// restore to the previous last weapon if we failed to switch to a new weapon
		Weapon_SetLast( pPreviousLastWeapon );
	}

	return bSwitched;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::StopViewModelParticles( C_BaseEntity *pParticleEnt )
{
	pParticleEnt->ParticleProp()->StopParticlesInvolving( pParticleEnt );
}
#endif


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::GetStepSoundVelocities( float *velwalk, float *velrun )
{
	float flMaxSpeed = MaxSpeed();

	if ( ( GetFlags() & FL_DUCKING ) || ( GetMoveType() == MOVETYPE_LADDER ) )
	{
		*velwalk = flMaxSpeed * 0.25;
		*velrun = flMaxSpeed * 0.3;		
	}
	else
	{
		*velwalk = flMaxSpeed * 0.3;
		*velrun = flMaxSpeed * 0.8;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetStepSoundTime( stepsoundtimes_t iStepSoundTime, bool bWalking )
{
	float flMaxSpeed = MaxSpeed();

	switch ( iStepSoundTime )
	{
	case STEPSOUNDTIME_NORMAL:
	case STEPSOUNDTIME_WATER_FOOT:
		m_flStepSoundTime = RemapValClamped( flMaxSpeed, 200, 450, 400, 200 );
		if ( bWalking )
		{
			m_flStepSoundTime += 100;
		}
		break;

	case STEPSOUNDTIME_ON_LADDER:
		m_flStepSoundTime = 350;
		break;

	case STEPSOUNDTIME_WATER_KNEE:
		m_flStepSoundTime = RemapValClamped( flMaxSpeed, 200, 450, 600, 400 );
		break;

	default:
		Assert(0);
		break;
	}

	if ( ( GetFlags() & FL_DUCKING) || ( GetMoveType() == MOVETYPE_LADDER ) )
	{
		m_flStepSoundTime += 100;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanAttack( int iCanAttackFlags )
{
	CTFGameRules *pRules = TFGameRules();

	Assert( pRules );

	if ( ( m_Shared.GetStealthNoAttackExpireTime() > gpGlobals->curtime ) || m_Shared.InCond( TF_COND_STEALTHED ) )
	{
		if ( !( iCanAttackFlags & TF_CAN_ATTACK_FLAG_GRAPPLINGHOOK ) )
		{
#ifdef CLIENT_DLL
			HintMessage( HINT_CANNOT_ATTACK_WHILE_CLOAKED, true, true );
#endif
			return false;
		}
	}

	if ( IsTaunting() )
		return false;

	if ( ( pRules->State_Get() == GR_STATE_TEAM_WIN ) && ( pRules->GetWinningTeam() != GetTeamNumber() ) )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanJump() const
{
	// Cannot jump while taunting
	if ( m_Shared.InCond( TF_COND_TAUNTING ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Weapons can call this on secondary attack and it will link to the class
// ability
//-----------------------------------------------------------------------------
bool CTFPlayer::DoClassSpecialSkill( void )
{
	if ( !IsAlive() )
		return false;

	bool bDoSkill = false;

	switch( GetPlayerClass()->GetClassIndex() )
	{
	case TF_CLASS_SPY:
		{
			if ( !m_Shared.InCond( TF_COND_TAUNTING ) )
			{
				if ( m_Shared.m_flStealthNextChangeTime <= gpGlobals->curtime )
				{
					// Feign death if we have the right equipment mod.
					CTFWeaponInvis* pInvisWatch = static_cast<CTFWeaponInvis*>( Weapon_OwnsThisID( TF_WEAPON_INVIS ) );
					if ( pInvisWatch )
					{
						pInvisWatch->ActivateInvisibilityWatch();
					}
				}
			}
		}
		break;

	case TF_CLASS_DEMOMAN:
		{
			CTFPipebombLauncher *pPipebombLauncher = static_cast<CTFPipebombLauncher*>( Weapon_OwnsThisID( TF_WEAPON_PIPEBOMBLAUNCHER ) );
			if ( pPipebombLauncher )
			{
				pPipebombLauncher->SecondaryAttack();
			}
		}
		bDoSkill = true;
		break;
	default:
		break;
	}

	return bDoSkill;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::EndClassSpecialSkill( void )
{
	if ( !IsAlive() )
		return false;

	switch( GetPlayerClass()->GetClassIndex() )
	{
	case TF_CLASS_DEMOMAN:
		break;

	default:
		break;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanGoInvisible( bool bAllowWhileCarryingFlag )
{
	// The "flag" in Player Destruction doesn't block cloak
	if ( !bAllowWhileCarryingFlag && ( HasTheFlag() ) )
	{
		HintMessage( HINT_CANNOT_CLOAK_WITH_FLAG );
		return false;
	}

	CTFGameRules *pRules = TFGameRules();

	Assert( pRules );

	if ( ( pRules->State_Get() == GR_STATE_TEAM_WIN ) && ( pRules->GetWinningTeam() != GetTeamNumber() ) )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanStartPhase( void )
{
	if ( HasTheFlag() )
	{
		HintMessage( HINT_CANNOT_PHASE_WITH_FLAG );
		return false;
	}

	CTFGameRules *pRules = TFGameRules();

	Assert( pRules );

	if ( ( pRules->State_Get() == GR_STATE_TEAM_WIN ) && ( pRules->GetWinningTeam() != GetTeamNumber() ) )
	{
		return false;
	}

	return true;
}

//ConVar testclassviewheight( "testclassviewheight", "0", FCVAR_DEVELOPMENTONLY );
//Vector vecTestViewHeight(0,0,0);

//-----------------------------------------------------------------------------
// Purpose: Return class-specific standing eye height
//-----------------------------------------------------------------------------
Vector CTFPlayer::GetClassEyeHeight( void )
{
	CTFPlayerClass *pClass = GetPlayerClass();

	if ( !pClass )
		return VEC_VIEW_SCALED( this );

	//if ( testclassviewheight.GetFloat() > 0 )
	//{
	//	vecTestViewHeight.z = test.GetFloat();
	//	return vecTestViewHeight;
	//}

	int iClassIndex = pClass->GetClassIndex();

	if ( iClassIndex < TF_FIRST_NORMAL_CLASS || iClassIndex > TF_LAST_NORMAL_CLASS )
		return VEC_VIEW_SCALED( this );

	return g_TFClassViewVectors[pClass->GetClassIndex()] * GetModelScale();
}


CTFWeaponBase *CTFPlayer::Weapon_OwnsThisID( int iWeaponID ) const
{
	for (int i = 0;i < WeaponCount(); i++) 
	{
		CTFWeaponBase *pWpn = ( CTFWeaponBase *)GetWeapon( i );

		if ( pWpn == NULL )
			continue;

		if ( pWpn->GetWeaponID() == iWeaponID )
		{
			return pWpn;
		}
	}

	return NULL;
}

CTFWeaponBase *CTFPlayer::Weapon_GetWeaponByType( int iType )
{
	for (int i = 0;i < WeaponCount(); i++) 
	{
		CTFWeaponBase *pWpn = ( CTFWeaponBase *)GetWeapon( i );

		if ( pWpn == NULL )
			continue;

		int iWeaponRole = pWpn->GetTFWpnData().m_iWeaponType;

		if ( iWeaponRole == iType )
		{
			return pWpn;
		}
	}

	return NULL;
}

bool CTFPlayer::Weapon_CanSwitchTo( CBaseCombatWeapon *pWeapon )
{
	bool bCanSwitch = BaseClass::Weapon_CanSwitchTo( pWeapon );

	if ( bCanSwitch )
	{
		if ( GetActiveTFWeapon() )
		{
			// There's ammo in the clip while auto firing... no switching away!
			if ( GetActiveTFWeapon()->AutoFiresFullClip() && GetActiveTFWeapon()->Clip1() > 0 )
				return false;
		}

		// prevents script exploits, like switching to the minigun while eating a sandvich
		if ( IsTaunting() && tf_allow_taunt_switch.GetInt() == 0 )
		{
			return false;
		}
	}

	return bCanSwitch;
}

void CTFPlayer::PlayStepSound( Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force )
{
#ifdef CLIENT_DLL
	// Don't make predicted footstep sounds in third person, animevents will take care of that.
	if ( prediction->InPrediction() && C_BasePlayer::ShouldDrawLocalPlayer() )
		return;
#endif

	BaseClass::PlayStepSound( vecOrigin, psurface, fvol, force );
}

//-----------------------------------------------------------------------------
// Purpose: Gives the player an opportunity to abort a double jump.
//-----------------------------------------------------------------------------
bool CTFPlayer::CanAirDash( void ) const
{	
	bool bScout = GetPlayerClass()->IsClass( TF_CLASS_SCOUT );
	if ( !bScout )
		return false;

	CTFWeaponBase *pTFActiveWeapon = GetActiveTFWeapon();
	int iDashCount = tf_scout_air_dash_count.GetInt();

	if ( m_Shared.GetAirDash() >= iDashCount )
		return false;

	if ( pTFActiveWeapon )
	{
		// TODO(driller): Hack fix to restrict this to The Atomzier (currently the only item that uses this attribute) on what would be the third jump
		float flTimeSinceDeploy = gpGlobals->curtime - pTFActiveWeapon->GetLastDeployTime();
		if ( iDashCount >= 2 && m_Shared.GetAirDash() == 1 && flTimeSinceDeploy < 0.7f )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Remove disguise
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveDisguise( void )
{
	// remove quickly
	if ( m_Shared.InCond( TF_COND_DISGUISED ) || m_Shared.InCond( TF_COND_DISGUISING ) )
	{
		m_Shared.RemoveDisguise();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::RemoveDisguiseWeapon( void )
{
#ifdef GAME_DLL
	if ( m_hDisguiseWeapon )
	{
		m_hDisguiseWeapon->Drop( Vector(0,0,0) );
		m_hDisguiseWeapon = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanDisguise( void )
{
	if ( !IsAlive() )
		return false;

	if ( GetPlayerClass()->GetClassIndex() != TF_CLASS_SPY )
		return false;

	bool bHasFlag = false;

	if ( HasTheFlag() )
	{
		bHasFlag = true;
	}

	if ( bHasFlag )
	{
		HintMessage( HINT_CANNOT_DISGUISE_WITH_FLAG );
		return false;
	}

	if ( !Weapon_GetWeaponByType( TF_WPN_TYPE_PDA ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFPlayer::GetMaxAmmo( int iAmmoIndex, int iClassIndex /*= -1*/ )
{
	return ( iClassIndex == -1 ) ? m_PlayerClass.GetData()->m_aAmmoMax[iAmmoIndex] : GetPlayerClassData( iClassIndex )->m_aAmmoMax[iAmmoIndex];

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CTFPlayer::MedicGetHealTarget( void )
{
	if ( IsPlayerClass(TF_CLASS_MEDIC) )
	{
		CWeaponMedigun *pWeapon = dynamic_cast <CWeaponMedigun*>( GetActiveWeapon() );

		if ( pWeapon )
			return pWeapon->GetHealTarget();
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayer::MedicGetChargeLevel( CTFWeaponBase **pRetMedigun )
{
	if ( IsPlayerClass(TF_CLASS_MEDIC) )
	{
		CTFWeaponBase *pWpn = ( CTFWeaponBase *)Weapon_OwnsThisID( TF_WEAPON_MEDIGUN );

		if ( pWpn == NULL )
			return 0;

		CWeaponMedigun *pMedigun = dynamic_cast <CWeaponMedigun*>( pWpn );

		if ( pRetMedigun )
		{
			*pRetMedigun = pMedigun;
		}

		if ( pMedigun )
			return pMedigun->GetChargeLevel();
	}

	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayer::GetNumActivePipebombs( void )
{
	if ( IsPlayerClass( TF_CLASS_DEMOMAN ) )
	{
		CTFPipebombLauncher *pWeapon = dynamic_cast < CTFPipebombLauncher*>( Weapon_OwnsThisID( TF_WEAPON_PIPEBOMBLAUNCHER ) );

		if ( pWeapon )
		{
			return pWeapon->GetPipeBombCount();
		}
	}

	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanMoveDuringTaunt()
{

	if ( TFGameRules() && TFGameRules()->IsCompetitiveMode() )
	{
		if ( ( TFGameRules()->GetRoundRestartTime() > -1.f ) && ( (int)( TFGameRules()->GetRoundRestartTime() - gpGlobals->curtime ) <= mp_tournament_readymode_countdown.GetInt() ) )
			return false;

		if ( TFGameRules()->PlayersAreOnMatchSummaryStage() )
			return false;
	}

	if ( m_Shared.InCond( TF_COND_TAUNTING ) )
	{
#ifdef GAME_DLL
		if ( tf_allow_sliding_taunt.GetBool() )
		{
			return true;
		}
#endif // GAME_DLL
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldStopTaunting()
{
	// stop taunt if we're under water
	if ( GetWaterLevel() > WL_Waist )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetTauntYaw( float flTauntYaw )
{
	m_flPrevTauntYaw = m_flTauntYaw;
	m_flTauntYaw = flTauntYaw;

	QAngle angle = GetLocalAngles();
	angle.y = flTauntYaw;
	SetLocalAngles( angle );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::StartBuildingObjectOfType( int iType, int iMode )
{
	// early out if we can't build this type of object
	if ( CanBuild( iType, iMode ) != CB_CAN_BUILD )
		return;

	// Does this type require a specific builder?
	CTFWeaponBuilder *pBuilder = CTFPlayerSharedUtils::GetBuilderForObjectType( this, iType );
	if ( pBuilder )
	{
#ifdef GAME_DLL
		pBuilder->SetSubType( iType );
		pBuilder->SetObjectMode( iMode );


		if ( GetActiveTFWeapon() == pBuilder )
		{
			SetActiveWeapon( NULL );
		}	
#endif

		// try to switch to this weapon
		Weapon_Switch( pBuilder );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayerShared::GetSequenceForDeath( CBaseAnimating* pRagdoll, bool bBurning, int nCustomDeath )
{
	if ( !pRagdoll )
		return -1;

	int iDeathSeq = -1;
// 	if ( bBurning )
// 	{
// 		iDeathSeq = pRagdoll->LookupSequence( "primary_death_burning" );
// 	}

	switch ( nCustomDeath )
	{
	case TF_DMG_CUSTOM_HEADSHOT:
		iDeathSeq = pRagdoll->LookupSequence( "primary_death_headshot" );
		break;
	case TF_DMG_CUSTOM_BACKSTAB:
		iDeathSeq = pRagdoll->LookupSequence( "primary_death_backstab" );
		break;
	}

	return iDeathSeq;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::RecalculatePlayerBodygroups( void )
{
	// We have to clear the m_nBody bitfield.
	// Leaving bits on from previous player classes can have weird effects
	// like if we switch to a class that uses those bits for other things.
	m_pOuter->m_nBody = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateCloakMeter( void )
{
	if ( !m_pOuter->IsPlayerClass( TF_CLASS_SPY ) )
		return;

	if ( InCond( TF_COND_STEALTHED ) )
	{
		// Classic cloak: drain at a fixed rate.
		m_flCloakMeter -= gpGlobals->frametime * m_fCloakConsumeRate;
		
		if ( m_flCloakMeter <= 0.0f )	
		{
			FadeInvis( 1.0f );
		}

		// Update Debuffs
		// Decrease duration if cloaked
#ifdef GAME_DLL
		// staging_spy
		float flReduction = gpGlobals->frametime * 0.75f;
		for ( int i = 0; g_aDebuffConditions[i] != TF_COND_LAST; i++ )
		{
			if ( InCond( g_aDebuffConditions[i] ) )
			{
				if ( m_ConditionData[g_aDebuffConditions[i]].m_flExpireTime != PERMANENT_CONDITION )
				{			
					m_ConditionData[g_aDebuffConditions[i]].m_flExpireTime = MAX( m_ConditionData[g_aDebuffConditions[i]].m_flExpireTime - flReduction, 0 );
				}
				// Burning and Bleeding and extra timers
				if ( g_aDebuffConditions[i] == TF_COND_BURNING )
				{
					// Reduce the duration of this burn
					m_flAfterburnDuration -= flReduction;
				}
			}
		}
#endif
	} 
	else
	{
		m_flCloakMeter += gpGlobals->frametime * m_fCloakRegenRate;

		if ( m_flCloakMeter >= 100.0f )
		{
			m_flCloakMeter = 100.0f;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTraceFilterIgnoreTeammatesAndTeamObjects::ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
{
	CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );

	if ( pEntity->GetTeamNumber() == m_iIgnoreTeam )
	{
		return false;
	}

	CTFPlayer *pPlayer = dynamic_cast<CTFPlayer*>( pEntity );
	if ( pPlayer )
	{
		if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && pPlayer->m_Shared.GetDisguiseTeam() == m_iIgnoreTeam )
			return false;

		if ( pPlayer->m_Shared.IsStealthed() )
			return false;
	}

	return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
}

void localplayerscoring_t::UpdateStats( RoundStats_t& roundStats, CTFPlayer *pPlayer, bool bIsRoundData )
{
	m_iCaptures = roundStats.m_iStat[TFSTAT_CAPTURES];
	m_iDefenses = roundStats.m_iStat[TFSTAT_DEFENSES];

	m_iKills = roundStats.m_iStat[TFSTAT_KILLS];
	m_iDeaths = roundStats.m_iStat[TFSTAT_DEATHS];
	m_iSuicides = roundStats.m_iStat[TFSTAT_SUICIDES];
	m_iKillAssists = roundStats.m_iStat[TFSTAT_KILLASSISTS];

	m_iBuildingsBuilt = roundStats.m_iStat[TFSTAT_BUILDINGSBUILT];
	m_iBuildingsDestroyed = roundStats.m_iStat[TFSTAT_BUILDINGSDESTROYED];

	m_iHeadshots = roundStats.m_iStat[TFSTAT_HEADSHOTS];
	m_iDominations = roundStats.m_iStat[TFSTAT_DOMINATIONS];
	m_iRevenge = roundStats.m_iStat[TFSTAT_REVENGE];
	m_iInvulns = roundStats.m_iStat[TFSTAT_INVULNS];
	m_iTeleports = roundStats.m_iStat[TFSTAT_TELEPORTS];

	m_iDamageDone = roundStats.m_iStat[TFSTAT_DAMAGE];
	m_iCrits = roundStats.m_iStat[TFSTAT_CRITS];

	m_iBackstabs = roundStats.m_iStat[TFSTAT_BACKSTABS];

	int iHealthPointsHealed = (int) roundStats.m_iStat[TFSTAT_HEALING];
	// send updated healing data every 10 health points, and round off what we send to nearest 10 points
	int iHealPointsDelta = abs( iHealthPointsHealed - m_iHealPoints );
	if ( iHealPointsDelta > 10 )
	{
		m_iHealPoints = ( iHealthPointsHealed / 10 ) * 10;
	}
	m_iBonusPoints = roundStats.m_iStat[TFSTAT_BONUS_POINTS] / TF_SCORE_BONUS_POINT_DIVISOR;
	const int nPoints = TFGameRules()->CalcPlayerScore( &roundStats, pPlayer );
	const int nDelta = nPoints - m_iPoints;
	m_iPoints = nPoints;
	
	if ( nDelta > 0 && !bIsRoundData )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "player_score_changed" );
		if ( event )
		{
			event->SetInt( "player", pPlayer->entindex() );
			event->SetInt( "delta", nDelta );
			gameeventmanager->FireEvent( event );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWeaponBuilder *CTFPlayerSharedUtils::GetBuilderForObjectType( CTFPlayer *pTFPlayer, int iObjectType )
{
	const int OBJ_ANY = -1;

	if ( !pTFPlayer )
		return NULL;

	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		CTFWeaponBuilder *pBuilder = dynamic_cast< CTFWeaponBuilder* >( pTFPlayer->GetWeapon( i ) );
		if ( !pBuilder )
			continue;

		// Any builder will do - return first
		if ( iObjectType == OBJ_ANY )
			return pBuilder;

		// Requires a specific builder for this type
		if ( pBuilder->CanBuildObjectType( iObjectType ) )
			return pBuilder;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if helpme button is pressed
//-----------------------------------------------------------------------------
bool CTFPlayer::IsHelpmeButtonPressed() const
{
	return m_flHelpmeButtonPressTime != 0.f;
}


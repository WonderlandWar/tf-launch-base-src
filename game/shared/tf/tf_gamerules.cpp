//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: The TF Game rules 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_gamerules.h"
#include "tf_classdata.h"
#include "ammodef.h"
#include "KeyValues.h"
#include "tf_weaponbase.h"
#include "tf_weaponbase_gun.h"
#include "tier0/icommandline.h"
#include "convar_serverbounded.h"
#include "tf_weapon_grenadelauncher.h"

#ifdef CLIENT_DLL
	#include <game/client/iviewport.h>
	#include "c_tf_player.h"
	#include "c_tf_objective_resource.h"
	#include <filesystem.h>
	#include "c_tf_team.h"
	#include "dt_utlvector_recv.h"
	#include "video/ivideoservices.h"
	#include "c_tf_playerresource.h"
#else
	#include "basemultiplayerplayer.h"
	#include "voice_gamemgr.h"
	#include "items.h"
	#include "team.h"
	#include "game.h"
	#include "tf_bot_temp.h"
	#include "tf_player.h"
	#include "tf_team.h"
	#include "player_resource.h"
	#include "entity_tfstart.h"
	#include "filesystem.h"
	#include "tf_obj.h"
	#include "tf_objective_resource.h"
	#include "tf_player_resource.h"
	#include "team_control_point_master.h"
	#include "team_train_watcher.h"
	#include "playerclass_info_parse.h"
	#include "team_control_point_master.h"
	#include "coordsize.h"
	#include "entity_healthkit.h"
	#include "tf_gamestats.h"
	#include "entity_capture_flag.h"
	#include "tf_player_resource.h"
	#include "tf_obj_sentrygun.h"
	#include "activitylist.h"
	#include "AI_ResponseSystem.h"
	#include "hl2orange.spa.h"
	#include "hltvdirector.h"
	#include "func_suggested_build.h"
	#include "tf_weaponbase_grenadeproj.h"
	#include "engine/IEngineSound.h"
	#include "soundenvelope.h"
	#include "dt_utlvector_send.h"
	#include "vgui/ILocalize.h"
	#include "tier3/tier3.h"
	#include "tf_ammo_pack.h"
	#include "vote_controller.h"
	#include "tf_extra_map_entity.h"
	#include "tf_weapon_grenade_pipebomb.h"
	#include "tf_weapon_sniperrifle.h"
	#include "tf_weapon_knife.h"
	
	#include "util_shared.h"

	#include "teamplay_round_timer.h"
	#include "effect_dispatch_data.h"
	#include "tf_fx.h"
	#include "tf_obj_sentrygun.h"
	#include "cdll_int.h"
	#include "tf_weapon_invis.h"
	#include "tf_autobalance.h"
	#include "player_voice_listener.h"
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weaponbase_melee.h"
#include "tf_weapon_flamethrower.h"
#include "tf_weapon_medigun.h"

#include "tier3/tier3.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define ITEM_RESPAWN_TIME	10.0f
#define MASK_RADIUS_DAMAGE  ( MASK_SHOT & ~( CONTENTS_HITBOX ) )

static int g_TauntCamRagdollAchievements[] = 
{
	0,		// TF_CLASS_UNDEFINED

	0,		// TF_CLASS_SCOUT,	
	0,		// TF_CLASS_SNIPER,
	0,		// TF_CLASS_SOLDIER,
	0,		// TF_CLASS_DEMOMAN,
	ACHIEVEMENT_TF_MEDIC_FREEZECAM_RAGDOLL,		// TF_CLASS_MEDIC,
	0,		// TF_CLASS_HEAVYWEAPONS,
	0,		// TF_CLASS_PYRO,
	ACHIEVEMENT_TF_SPY_FREEZECAM_FLICK,		// TF_CLASS_SPY,
	0,		// TF_CLASS_ENGINEER,

	0,		// TF_CLASS_CIVILIAN,
	0,		// TF_CLASS_COUNT_ALL,
};

static int g_TauntCamAchievements[] = 
{
	0,		// TF_CLASS_UNDEFINED

	0,		// TF_CLASS_SCOUT,	
	ACHIEVEMENT_TF_SNIPER_FREEZECAM_HAT,		// TF_CLASS_SNIPER,
	ACHIEVEMENT_TF_SOLDIER_FREEZECAM_GIBS,		// TF_CLASS_SOLDIER,		(extra check to count the number of gibs onscreen)
	ACHIEVEMENT_TF_DEMOMAN_FREEZECAM_SMILE,		// TF_CLASS_DEMOMAN,
	0,		// TF_CLASS_MEDIC,
	ACHIEVEMENT_TF_HEAVY_FREEZECAM_TAUNT,		// TF_CLASS_HEAVYWEAPONS,  (there's an extra check on this one to see if we're also invuln)
	ACHIEVEMENT_TF_PYRO_FREEZECAM_TAUNTS,		// TF_CLASS_PYRO,
	0,		// TF_CLASS_SPY,
	ACHIEVEMENT_TF_ENGINEER_FREEZECAM_TAUNT,	// TF_CLASS_ENGINEER,
	0,		// TF_CLASS_CIVILIAN,
	0,		// TF_CLASS_COUNT_ALL,
};

// used for classes that have more than one freeze cam achievement (example: Sniper)
static int g_TauntCamAchievements2[] = 
{
	0,		// TF_CLASS_UNDEFINED

	0,		// TF_CLASS_SCOUT,	
	ACHIEVEMENT_TF_SNIPER_FREEZECAM_WAVE,		// TF_CLASS_SNIPER,
	ACHIEVEMENT_TF_SOLDIER_FREEZECAM_TAUNT,		// TF_CLASS_SOLDIER,
	ACHIEVEMENT_TF_DEMOMAN_FREEZECAM_RUMP,		// TF_CLASS_DEMOMAN,
	0,		// TF_CLASS_MEDIC,
	0,		// TF_CLASS_HEAVYWEAPONS,
	0,		// TF_CLASS_PYRO,
	0,		// TF_CLASS_SPY,
	0,		// TF_CLASS_ENGINEER,

	0,		// TF_CLASS_CIVILIAN,
	0,		// TF_CLASS_COUNT_ALL,
};

struct StatueInfo_t
{
	const char	*pDiskName;
	Vector		vec_origin;
	QAngle		vec_angle;
};

static StatueInfo_t s_StatueMaps[] = {
	{ "ctf_2fort",				Vector( 483, 613, 0 ),			QAngle( 0, 180, 0 ) },
	{ "cp_dustbowl",			Vector( -596, 2650, -256 ),		QAngle( 0, 180, 0 ) },
	{ "cp_granary",				Vector( -544, -510, -416 ),		QAngle( 0, 180, 0 ) },
	{ "cp_well",				Vector( 1255, 515, -512 ),		QAngle( 0, 180, 0 ) },
	{ "cp_foundry",				Vector( -85, 912, 0 ),			QAngle( 0, -90, 0 ) },
	{ "cp_gravelpit",			Vector( -4624, 660, -512 ),		QAngle( 0, 0, 0 ) },
	{ "ctf_well",				Vector( 1000, -240, -512 ),		QAngle( 0, 180, 0 ) },
	{ "cp_badlands",			Vector( 808, -1079, 64 ),		QAngle( 0, 135, 0 ) },
	{ "pl_goldrush",			Vector( -2780, -650, 0 ),		QAngle( 0, 90, 0 ) },
	{ "pl_badwater",			Vector( 2690, -416, 131 ),		QAngle( 0, -90, 0 ) },
	{ "plr_pipeline",			Vector( 220, -2527, 128 ),		QAngle( 0, 90, 0 ) },
	{ "cp_gorge",				Vector( -6970, 5920, -42 ),		QAngle( 0, 0, 0 ) },
	{ "ctf_doublecross",		Vector( 1304, -206, 8 ),		QAngle( 0, 180, 0 ) },
	{ "pl_thundermountain",		Vector( -720, -1058, 128 ),		QAngle( 0, -90, 0 ) },
	{ "cp_mountainlab",			Vector( -2930, 1606, -1069 ),	QAngle( 0, 90, 0 ) },
	{ "cp_degrootkeep",			Vector( -1000, 4580, -255 ),	QAngle( 0, -25, 0 ) },
	{ "pl_barnblitz",			Vector( 3415, -2144, -54 ),		QAngle( 0, 90, 0 ) },
	{ "pl_upward",				Vector( -736, -2275, 63 ),		QAngle( 0, 0, 0 ) },
	{ "plr_hightower",			Vector( 5632, 7747, 8 ),		QAngle( 0, 0, 0 ) },
	{ "koth_viaduct",			Vector( -979, 0, 240 ),			QAngle( 0, 180, 0 ) },
	{ "koth_king",				Vector( 715, -395, -224 ),		QAngle( 0, 135, 0 ) },
	{ "sd_doomsday",			Vector( -1025, 675, 128 ),		QAngle( 0, 90, 0 ) },
	{ "cp_mercenarypark",		Vector( -2800, -775, -40 ),		QAngle( 0, 0, 0 ) },
	{ "ctf_turbine",			Vector( 718, 0, -256 ),			QAngle( 0, 180, 0 ) },
	{ "koth_harvest_final",		Vector( -1428, 220, -15 ),		QAngle( 0, 0, 0 ) },
	{ "pl_swiftwater_final1",	Vector( 706, -2785, -934 ),		QAngle( 0, 0, 0 ) },
	{ "pl_frontier_final",		Vector( 3070, -3013, -193 ),	QAngle( 0, -90, 0 ) },
	{ "cp_process_final",		Vector( 650, -980, 535 ),		QAngle( 0, 90, 0 ) },
	{ "cp_gullywash_final1",	Vector( 200, 83, 47 ),			QAngle( 0, -102, 0 ) },
	{ "cp_sunshine",			Vector( -4725, 5860, 65 ),		QAngle( 0, 180, 0 ) },
};

struct MapInfo_t
{
	const char	*pDiskName;
	const char	*pDisplayName;
	const char	*pGameType;
};

static MapInfo_t s_ValveMaps[] = {
	{ "ctf_2fort",	"2Fort",		"#Gametype_CTF" },
	{ "cp_dustbowl",	"Dustbowl",		"#TF_AttackDefend" },
	{ "cp_granary",	"Granary",		"#Gametype_CP" },
	{ "cp_well",		"Well",			"#Gametype_CP" },
	{ "cp_foundry",	"Foundry",		"#Gametype_CP" },
	{ "cp_gravelpit", "Gravel Pit",	"#TF_AttackDefend" },
	{ "tc_hydro",		"Hydro",		"#TF_TerritoryControl" },
	{ "ctf_well",		"Well",			"#Gametype_CTF" },
	{ "cp_badlands",	"Badlands",		"#Gametype_CP" },
};

bool IsValveMap( const char *pMapName )
{
	for ( int i = 0; i < ARRAYSIZE( s_ValveMaps ); ++i )
	{
		if ( !Q_stricmp( s_ValveMaps[i].pDiskName, pMapName ) )
		{
			return true;
		}
	}
	return false;
}

extern ConVar mp_capstyle;
extern ConVar sv_turbophysics;

extern ConVar tf_vaccinator_small_resist;
extern ConVar tf_vaccinator_uber_resist;

extern ConVar tf_teleporter_fov_time;
extern ConVar tf_teleporter_fov_start;

#ifdef GAME_DLL
extern ConVar tf_debug_damage;
extern ConVar tf_damage_range;
extern ConVar tf_damage_disablespread;
extern ConVar tf_populator_damage_multiplier;
extern ConVar tf_weapon_criticals;
extern ConVar tf_weapon_criticals_melee;
extern ConVar mp_idledealmethod;
extern ConVar mp_idlemaxtime;

extern ConVar mp_autoteambalance;

// STAGING_SPY
ConVar tf_feign_death_activate_damage_scale( "tf_feign_death_activate_damage_scale", "0.25", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_feign_death_damage_scale( "tf_feign_death_damage_scale", "0.35", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_stealth_damage_reduction( "tf_stealth_damage_reduction", "0.8", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

ConVar tf_birthday_ball_chance( "tf_birthday_ball_chance", "100", FCVAR_REPLICATED, "Percent chance of a birthday beach ball spawning at each round start" );

ConVar tf_allow_player_name_change( "tf_allow_player_name_change", "1", FCVAR_NOTIFY, "Allow player name changes." );

ConVar tf_weapon_criticals_distance_falloff( "tf_weapon_criticals_distance_falloff", "0", FCVAR_CHEAT, "Critical weapon damage will take distance into account." );

ConVar mp_spectators_restricted( "mp_spectators_restricted", "0", FCVAR_NONE, "Prevent players on game teams from joining team spectator if it would unbalance the teams." );

ConVar tf_test_special_ducks( "tf_test_special_ducks", "1", FCVAR_DEVELOPMENTONLY );

ConVar tf_mm_abandoned_players_per_team_max( "tf_mm_abandoned_players_per_team_max", "1", FCVAR_DEVELOPMENTONLY );
#endif // GAME_DLL
ConVar tf_mm_next_map_vote_time( "tf_mm_next_map_vote_time", "15", FCVAR_REPLICATED );


static float g_fEternaweenAutodisableTime = 0.0f;

ConVar tf_spec_xray( "tf_spec_xray", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Allows spectators to see player glows. 1 = same team, 2 = both teams" );

#ifdef GAME_DLL
void cc_tf_forced_holiday_changed( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	// Tell the listeners to recalculate the holiday
	IGameEvent *event = gameeventmanager->CreateEvent( "recalculate_holidays" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}
}
#endif // GAME_DLL

ConVar tf_forced_holiday( "tf_forced_holiday", "0", FCVAR_REPLICATED, "Forced holiday, \n   Birthday = 1\n   Halloween = 2\n" //  Christmas = 3\n   Valentines = 4\n   MeetThePyro = 5\n   FullMoon=6
#ifdef GAME_DLL
, cc_tf_forced_holiday_changed
#endif // GAME_DLL
);
ConVar tf_item_based_forced_holiday( "tf_item_based_forced_holiday", "0", FCVAR_REPLICATED, "" 	// like a clone of tf_forced_holiday, but controlled by client consumable item use
#ifdef GAME_DLL
	, cc_tf_forced_holiday_changed
#endif // GAME_DLL
);
ConVar tf_force_holidays_off( "tf_force_holidays_off", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, ""
#ifdef GAME_DLL
, cc_tf_forced_holiday_changed
#endif // GAME_DLL
);
ConVar tf_birthday( "tf_birthday", "0", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tf_spells_enabled( "tf_spells_enabled", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Enable to Allow Halloween Spells to be dropped and used by players" );

ConVar tf_caplinear( "tf_caplinear", "1", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "If set to 1, teams must capture control points linearly." );
ConVar tf_stalematechangeclasstime( "tf_stalematechangeclasstime", "20", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Amount of time that players are allowed to change class in stalemates." );
ConVar mp_tournament_redteamname( "mp_tournament_redteamname", "RED", FCVAR_REPLICATED | FCVAR_HIDDEN );
ConVar mp_tournament_blueteamname( "mp_tournament_blueteamname", "BLU", FCVAR_REPLICATED | FCVAR_HIDDEN );

ConVar tf_attack_defend_map( "tf_attack_defend_map", "0", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );

#ifdef GAME_DLL
void cc_tf_stopwatch_changed( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	IGameEvent *event = gameeventmanager->CreateEvent( "stop_watch_changed" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}
}
#endif // GAME_DLL
ConVar mp_tournament_stopwatch( "mp_tournament_stopwatch", "1", FCVAR_REPLICATED | FCVAR_NOTIFY, "Use Stopwatch mode while using Tournament mode (mp_tournament)"
#ifdef GAME_DLL
	, cc_tf_stopwatch_changed 
#endif
);
ConVar mp_tournament_readymode( "mp_tournament_readymode", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Enable per-player ready status for tournament mode." );
ConVar mp_tournament_readymode_min( "mp_tournament_readymode_min", "2", FCVAR_REPLICATED | FCVAR_NOTIFY, "Minimum number of players required on the server before players can toggle ready status." );
ConVar mp_tournament_readymode_team_size( "mp_tournament_readymode_team_size", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Minimum number of players required to be ready per-team before the game can begin." );
ConVar mp_tournament_readymode_countdown( "mp_tournament_readymode_countdown", "10", FCVAR_REPLICATED | FCVAR_NOTIFY, "The number of seconds before a match begins when both teams are ready." );
#ifdef GAME_DLL
ConVar mp_tournament_prevent_team_switch_on_readyup( "mp_tournament_prevent_team_switch_on_readyup", "1", FCVAR_NONE, "Prevent switching teams on ready-up for subsequent rounds in tournament mode." );
#endif

ConVar mp_windifference( "mp_windifference", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Score difference between teams before server changes maps", true, 0, false, 0 );
ConVar mp_windifference_min( "mp_windifference_min", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Minimum score needed for mp_windifference to be applied", true, 0, false, 0 );

ConVar tf_tournament_classlimit_scout( "tf_tournament_classlimit_scout", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Scouts.\n" );
ConVar tf_tournament_classlimit_sniper( "tf_tournament_classlimit_sniper", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Snipers.\n" );
ConVar tf_tournament_classlimit_soldier( "tf_tournament_classlimit_soldier", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Soldiers.\n" );
ConVar tf_tournament_classlimit_demoman( "tf_tournament_classlimit_demoman", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Demomenz.\n" );
ConVar tf_tournament_classlimit_medic( "tf_tournament_classlimit_medic", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Medics.\n" );
ConVar tf_tournament_classlimit_heavy( "tf_tournament_classlimit_heavy", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Heavies.\n" );
ConVar tf_tournament_classlimit_pyro( "tf_tournament_classlimit_pyro", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Pyros.\n" );
ConVar tf_tournament_classlimit_spy( "tf_tournament_classlimit_spy", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Spies.\n" );
ConVar tf_tournament_classlimit_engineer( "tf_tournament_classlimit_engineer", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Engineers.\n" );
ConVar tf_tournament_classchange_allowed( "tf_tournament_classchange_allowed", "1", FCVAR_REPLICATED, "Allow players to change class while the game is active?.\n" );
ConVar tf_tournament_classchange_ready_allowed( "tf_tournament_classchange_ready_allowed", "1", FCVAR_REPLICATED, "Allow players to change class after they are READY?.\n" );

ConVar tf_classlimit( "tf_classlimit", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Limit on how many players can be any class (i.e. tf_class_limit 2 would limit 2 players per class).\n", true, 0.f, false, 0.f );
ConVar tf_player_movement_restart_freeze( "tf_player_movement_restart_freeze", "1", FCVAR_REPLICATED, "When set, prevent player movement during round restart" );

ConVar tf_autobalance_ask_candidates_maxtime( "tf_autobalance_ask_candidates_maxtime", "10", FCVAR_REPLICATED );
ConVar tf_autobalance_dead_candidates_maxtime( "tf_autobalance_dead_candidates_maxtime", "15", FCVAR_REPLICATED );
ConVar tf_autobalance_force_candidates_maxtime( "tf_autobalance_force_candidates_maxtime", "5", FCVAR_REPLICATED );


#ifdef GAME_DLL

static const float g_flStrangeEventBatchProcessInterval = 30.0f;

ConVar mp_humans_must_join_team("mp_humans_must_join_team", "any", FCVAR_REPLICATED, "Restricts human players to a single team {any, blue, red, spectator}" );

void cc_tf_medieval_changed( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	ConVarRef var( pConVar );
	bool bOldValue = flOldValue > 0;
	if ( var.IsValid() && ( bOldValue != var.GetBool() ) )
	{
		Msg( "Medieval mode changes take effect after the next map change.\n" );
	}
}

#endif
ConVar tf_medieval( "tf_medieval", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Enable Medieval Mode.\n", true, 0, true, 1
#ifdef GAME_DLL
				   , cc_tf_medieval_changed
#endif 
				    );

ConVar tf_sticky_radius_ramp_time( "tf_sticky_radius_ramp_time", "2.0", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT | FCVAR_REPLICATED, "Amount of time to get full radius after arming" );
ConVar tf_sticky_airdet_radius( "tf_sticky_airdet_radius", "0.85", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT | FCVAR_REPLICATED, "Radius Scale if detonated in the air" );


#ifndef GAME_DLL
extern ConVar cl_burninggibs;
extern ConVar english;
ConVar tf_particles_disable_weather( "tf_particles_disable_weather", "0", FCVAR_ARCHIVE, "Disable particles related to weather effects." );
#endif

// arena mode cvars
#if defined( _DEBUG ) || defined( STAGING_ONLY )
extern ConVar mp_developer;
#endif // _DEBUG || STAGING_ONLY

extern ConVar mp_tournament;
extern ConVar mp_tournament_post_match_period;

extern ConVar tf_flag_return_on_touch;
extern ConVar tf_flag_return_time_credit_factor;

static bool BIsCvarIndicatingHolidayIsActive( int iCvarValue, /*EHoliday*/ int eHoliday )
{
	return false;
}

#ifdef TF_CREEP_MODE
ConVar tf_gamemode_creep_wave( "tf_gamemode_creep_wave", "0", FCVAR_REPLICATED | FCVAR_NOTIFY );
ConVar tf_creep_wave_player_respawn_time( "tf_creep_wave_player_respawn_time", "10", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_CHEAT, "How long it takes for a player to respawn with his team after death." );
#endif

#ifdef GAME_DLL
// TF overrides the default value of this convar

#ifdef _DEBUG
#define WAITING_FOR_PLAYERS_FLAGS	0
#else
#define WAITING_FOR_PLAYERS_FLAGS	FCVAR_DEVELOPMENTONLY
#endif

ConVar hide_server( "hide_server", "0", FCVAR_GAMEDLL, "Whether the server should be hidden from the master server" );

ConVar mp_waitingforplayers_time( "mp_waitingforplayers_time", (IsX360()?"15":"30"), FCVAR_GAMEDLL | WAITING_FOR_PLAYERS_FLAGS, "WaitingForPlayers time length in seconds" );

ConVar tf_gamemode_arena ( "tf_gamemode_arena", "0", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_cp ( "tf_gamemode_cp", "0", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_ctf ( "tf_gamemode_ctf", "0", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_tc ( "tf_gamemode_tc", "0", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar tf_beta_content ( "tf_beta_content", "0", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_misc ( "tf_gamemode_misc", "0", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );

ConVar tf_bot_count( "tf_bot_count", "0", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );

#ifdef _DEBUG
ConVar tf_debug_ammo_and_health( "tf_debug_ammo_and_health", "0", FCVAR_CHEAT );
#endif // _DEBUG

static Vector s_BotSpawnPosition;

ConVar tf_gravetalk( "tf_gravetalk", "1", FCVAR_NOTIFY, "Allows living players to hear dead players using text/voice chat.", true, 0, true, 1 );

ConVar tf_ctf_bonus_time ( "tf_ctf_bonus_time", "10", FCVAR_NOTIFY, "Length of team crit time for CTF capture." );

#ifdef _DEBUG
ConVar mp_scrambleteams_debug( "mp_scrambleteams_debug", "0", FCVAR_NONE, "Debug spew." );
#endif // _DEBUG


extern ConVar tf_mm_servermode;
extern ConVar tf_flag_caps_per_round;

void cc_competitive_mode( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	IGameEvent *event = gameeventmanager->CreateEvent( "competitive_state_changed" );
	if ( event )
	{
		// Server-side here.  Client-side down below in the RecvProxy
		gameeventmanager->FireEvent( event, true );
	}
}
ConVar tf_competitive_preround_duration( "tf_competitive_preround_duration", "3", FCVAR_REPLICATED, "How long we stay in pre-round when in competitive games." );
ConVar tf_competitive_preround_countdown_duration( "tf_competitive_preround_countdown_duration", "10.5", FCVAR_HIDDEN, "How long we stay in countdown when in competitive games." );
ConVar tf_competitive_abandon_method( "tf_competitive_abandon_method", "0", FCVAR_HIDDEN );
ConVar tf_competitive_required_late_join_timeout( "tf_competitive_required_late_join_timeout", "120", FCVAR_DEVELOPMENTONLY,
                                                  "How long to wait for late joiners in matches requiring full player counts before canceling the match" );
ConVar tf_competitive_required_late_join_confirm_timeout( "tf_competitive_required_late_join_confirm_timeout", "30", FCVAR_DEVELOPMENTONLY,
                                                          "How long to wait for the GC to confirm we're in the late join pool before canceling the match" );
#endif // GAME_DLL

ConVar tf_gamemode_community ( "tf_gamemode_community", "0", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );

ConVar tf_voice_command_suspension_mode( "tf_voice_command_suspension_mode", "2", FCVAR_REPLICATED, "0 = None | 1 = No Voice Commands | 2 = Rate Limited" );

#ifdef GAME_DLL

ConVar tf_voice_command_suspension_rate_limit_bucket_count( "tf_voice_command_suspension_rate_limit_bucket_count", "5" ); // Bucket size of 5.
ConVar tf_voice_command_suspension_rate_limit_bucket_refill_rate( "tf_voice_command_suspension_rate_limit_bucket_refill_rate", "6" ); // 6s

ConVar tf_skillrating_update_interval( "tf_skillrating_update_interval", "180", FCVAR_ARCHIVE, "How often to update the GC and OGS." );

extern ConVar mp_teams_unbalance_limit;

static bool g_bRandomMap = false;

void cc_RandomMap( const CCommand& args )
{
	CTFGameRules *pRules = TFGameRules();
	if ( pRules )
	{
		if ( !UTIL_IsCommandIssuedByServerAdmin() )
			return;

		g_bRandomMap = true;
	}
	else
	{
		// There's no game rules object yet, so let's load the map cycle and pick a map.
		char mapcfile[MAX_PATH];
		CMultiplayRules::DetermineMapCycleFilename( mapcfile, sizeof(mapcfile), true );
		if ( !mapcfile[0] )
		{
			Msg( "No mapcyclefile specified. Cannot pick a random map.\n" );
			return;
		}

		CUtlVector<char*> mapList;
		// No gamerules entity yet, since we don't need the fixups to find a map just use the base version
		CMultiplayRules::RawLoadMapCycleFileIntoVector ( mapcfile, mapList );
		if ( !mapList.Count() )
		{
			Msg( "Map cycle file \"%s\" contains no valid maps or cannot be read. Cannot pick a random map.\n", mapcfile );
			return;
		}

		int iMapIndex = RandomInt( 0, mapList.Count() - 1 );
		Msg ( "randommap: selecting map %i out of %i\n", iMapIndex + 1, mapList.Count() );
		engine->ServerCommand( UTIL_VarArgs( "map %s\n", mapList[iMapIndex] ) );

		CMultiplayRules::FreeMapCycleFileVector( mapList );
	}
}

static ConCommand randommap( "randommap", cc_RandomMap, "Changelevel to a random map in the mapcycle file" );
#endif	// GAME_DLL

#ifdef GAME_DLL
void ValidateCapturesPerRound( IConVar *pConVar, const char *oldValue, float flOldValue )
{
	ConVarRef var( pConVar );

	if ( var.GetInt() <= 0 )
	{
		// reset the flag captures being played in the current round
		int nTeamCount = TFTeamMgr()->GetTeamCount();
		for ( int iTeam = FIRST_GAME_TEAM; iTeam < nTeamCount; ++iTeam )
		{
			CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
			if ( !pTeam )
				continue;

			pTeam->SetFlagCaptures( 0 );
		}
	}
}

#endif	

ConVar tf_flag_caps_per_round( "tf_flag_caps_per_round", "3", FCVAR_REPLICATED, "Number of captures per round on CTF and PASS Time maps. Set to 0 to disable.", true, 0, false, 0
#ifdef GAME_DLL
							  , ValidateCapturesPerRound
#endif
							  );


/**
 * Player hull & eye position for standing, ducking, etc.  This version has a taller
 * player height, but goldsrc-compatible collision bounds.
 */
static CViewVectors g_TFViewVectors(
	Vector( 0, 0, 72 ),		//VEC_VIEW (m_vView) eye position
							
	Vector(-24, -24, 0 ),	//VEC_HULL_MIN (m_vHullMin) hull min
	Vector( 24,  24, 82 ),	//VEC_HULL_MAX (m_vHullMax) hull max
												
	Vector(-24, -24, 0 ),	//VEC_DUCK_HULL_MIN (m_vDuckHullMin) duck hull min
	Vector( 24,  24, 62 ),	//VEC_DUCK_HULL_MAX	(m_vDuckHullMax) duck hull max
	Vector( 0, 0, 45 ),		//VEC_DUCK_VIEW		(m_vDuckView) duck view
												
	Vector( -10, -10, -10 ),	//VEC_OBS_HULL_MIN	(m_vObsHullMin) observer hull min
	Vector(  10,  10,  10 ),	//VEC_OBS_HULL_MAX	(m_vObsHullMax) observer hull max
												
	Vector( 0, 0, 14 )		//VEC_DEAD_VIEWHEIGHT (m_vDeadViewHeight) dead view height
);							

Vector g_TFClassViewVectors[11] =
{
	Vector( 0, 0, 72 ),		// TF_CLASS_UNDEFINED

	Vector( 0, 0, 65 ),		// TF_CLASS_SCOUT,			// TF_FIRST_NORMAL_CLASS
	Vector( 0, 0, 75 ),		// TF_CLASS_SNIPER,
	Vector( 0, 0, 68 ),		// TF_CLASS_SOLDIER,
	Vector( 0, 0, 68 ),		// TF_CLASS_DEMOMAN,
	Vector( 0, 0, 75 ),		// TF_CLASS_MEDIC,
	Vector( 0, 0, 75 ),		// TF_CLASS_HEAVYWEAPONS,
	Vector( 0, 0, 68 ),		// TF_CLASS_PYRO,
	Vector( 0, 0, 75 ),		// TF_CLASS_SPY,
	Vector( 0, 0, 68 ),		// TF_CLASS_ENGINEER,

	Vector( 0, 0, 65 ),		// TF_CLASS_CIVILIAN,		// TF_LAST_NORMAL_CLASS
};

const CViewVectors *CTFGameRules::GetViewVectors() const
{
	return &g_TFViewVectors;
}

REGISTER_GAMERULES_CLASS( CTFGameRules );

#ifdef CLIENT_DLL
void RecvProxy_MatchSummary( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	bool bMatchSummary = ( pData->m_Value.m_Int > 0 );
	if ( bMatchSummary && !(*(bool*)(pOut)))
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer )
		{
			pLocalPlayer->TurnOffTauntCam();
			pLocalPlayer->TurnOffTauntCam_Finish();
		}
	}

	*(bool*)(pOut) = bMatchSummary;
}

void RecvProxy_CompetitiveMode( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	*(bool*)(pOut) = ( pData->m_Value.m_Int > 0 );

	IGameEvent *event = gameeventmanager->CreateEvent( "competitive_state_changed" );
	if ( event )
	{
		// Client-side once it's actually happened
		gameeventmanager->FireEventClientSide( event );
	}
}

void RecvProxy_PlayerVotedForMap( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	if ( *(int*)(pOut) != pData->m_Value.m_Int )
	{
		*(int*)(pOut) = pData->m_Value.m_Int;

		IGameEvent *event = gameeventmanager->CreateEvent( "player_next_map_vote_change" );
		if ( event )
		{
			event->SetInt( "map_index", pData->m_Value.m_Int );
			event->SetInt( "vote", pData->m_Value.m_Int );
			// Client-side once it's actually happened
			gameeventmanager->FireEventClientSide( event );
		}
	}
}

void RecvProxy_NewMapVoteStateChanged( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	bool bChange = *(int*)(pOut) != pData->m_Value.m_Int;
	*(int*)(pOut) = pData->m_Value.m_Int;

	if ( bChange )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "vote_maps_changed" );
		if ( event )
		{
			// Client-side once it's actually happened
			gameeventmanager->FireEventClientSide( event );
		}
	}
}
#endif

BEGIN_NETWORK_TABLE_NOBASE( CTFGameRules, DT_TFGameRules )
#ifdef CLIENT_DLL

	RecvPropInt( RECVINFO( m_nGameType ) ),
	RecvPropInt( RECVINFO( m_nStopWatchState ) ),
	RecvPropString( RECVINFO( m_pszTeamGoalStringRed ) ),
	RecvPropString( RECVINFO( m_pszTeamGoalStringBlue ) ),
	RecvPropTime( RECVINFO( m_flCapturePointEnableTime ) ),

	RecvPropEHandle( RECVINFO( m_hBirthdayPlayer ) ),

	RecvPropInt( RECVINFO( m_nBossHealth ) ),
	RecvPropInt( RECVINFO( m_nMaxBossHealth ) ),
	RecvPropInt( RECVINFO( m_fBossNormalizedTravelDistance ) ),
	RecvPropBool( RECVINFO( m_bHaveMinPlayersToEnableReady ) ),

	RecvPropBool( RECVINFO( m_bBountyModeEnabled ) ),

	RecvPropBool( RECVINFO( m_bMatchEnded ) ),
	RecvPropBool( RECVINFO( m_bShowMatchSummary ), 0, RecvProxy_MatchSummary ),
	RecvPropBool( RECVINFO_NAME( m_bShowMatchSummary, "m_bShowCompetitiveMatchSummary" ), 0, RecvProxy_MatchSummary ),     // Renamed
	RecvPropBool( RECVINFO( m_bTeamsSwitched ) ),
	RecvPropBool( RECVINFO( m_bMapHasMatchSummaryStage ) ),
	RecvPropBool( RECVINFO( m_bPlayersAreOnMatchSummaryStage ) ),
	RecvPropBool( RECVINFO( m_bStopWatchWinner ) ),
#else

	SendPropInt( SENDINFO( m_nGameType ), 4, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nStopWatchState ), 3, SPROP_UNSIGNED ),
	SendPropString( SENDINFO( m_pszTeamGoalStringRed ) ),
	SendPropString( SENDINFO( m_pszTeamGoalStringBlue ) ),
	SendPropTime( SENDINFO( m_flCapturePointEnableTime ) ),

	SendPropEHandle( SENDINFO( m_hBirthdayPlayer ) ),

	SendPropInt( SENDINFO( m_nBossHealth ) ),
	SendPropInt( SENDINFO( m_nMaxBossHealth ) ),
	SendPropInt( SENDINFO( m_fBossNormalizedTravelDistance ) ),
	SendPropBool( SENDINFO( m_bHaveMinPlayersToEnableReady ) ),

	SendPropBool( SENDINFO( m_bBountyModeEnabled ) ),

	SendPropBool( SENDINFO( m_bMatchEnded ) ),
	SendPropBool( SENDINFO( m_bShowMatchSummary ) ),
	SendPropBool( SENDINFO( m_bTeamsSwitched ) ),
	SendPropBool( SENDINFO( m_bMapHasMatchSummaryStage ) ),
	SendPropBool( SENDINFO( m_bPlayersAreOnMatchSummaryStage ) ),
	SendPropBool( SENDINFO( m_bStopWatchWinner ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_gamerules, CTFGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( TFGameRulesProxy, DT_TFGameRulesProxy )

#ifdef CLIENT_DLL
	void RecvProxy_TFGameRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
	{
		CTFGameRules *pRules = TFGameRules();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CTFGameRulesProxy, DT_TFGameRulesProxy )
		RecvPropDataTable( "tf_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_TFGameRules ), RecvProxy_TFGameRules )
	END_RECV_TABLE()
#else
	void *SendProxy_TFGameRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
	{
		CTFGameRules *pRules = TFGameRules();
		Assert( pRules );
		pRecipients->SetAllRecipients();
		return pRules;
	}

	BEGIN_SEND_TABLE( CTFGameRulesProxy, DT_TFGameRulesProxy )
		SendPropDataTable( "tf_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_TFGameRules ), SendProxy_TFGameRules )
	END_SEND_TABLE()
#endif

#ifdef GAME_DLL
BEGIN_DATADESC( CTFGameRulesProxy )
	DEFINE_KEYFIELD( m_bOvertimeAllowedForCTF, FIELD_BOOLEAN, "ctf_overtime" ),

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetRedTeamRespawnWaveTime", InputSetRedTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetBlueTeamRespawnWaveTime", InputSetBlueTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "AddRedTeamRespawnWaveTime", InputAddRedTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "AddBlueTeamRespawnWaveTime", InputAddBlueTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetRedTeamGoalString", InputSetRedTeamGoalString ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetBlueTeamGoalString", InputSetBlueTeamGoalString ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetRedTeamRole", InputSetRedTeamRole ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetBlueTeamRole", InputSetBlueTeamRole ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetRequiredObserverTarget", InputSetRequiredObserverTarget ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddRedTeamScore", InputAddRedTeamScore ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddBlueTeamScore", InputAddBlueTeamScore ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetCTFCaptureBonusTime", InputSetCTFCaptureBonusTime ),
	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVORed", InputPlayVORed ),
	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVOBlue", InputPlayVOBlue ),
	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVO", InputPlayVO ),
	DEFINE_INPUTFUNC( FIELD_STRING, "HandleMapEvent", InputHandleMapEvent ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetRoundRespawnFreezeEnabled", InputSetRoundRespawnFreezeEnabled ),

	DEFINE_OUTPUT( m_OnWonByTeam1,	"OnWonByTeam1" ),
	DEFINE_OUTPUT( m_OnWonByTeam2,	"OnWonByTeam2" ),
	DEFINE_OUTPUT( m_Team1PlayersChanged,	"Team1PlayersChanged" ),
	DEFINE_OUTPUT( m_Team2PlayersChanged,	"Team2PlayersChanged" ),
	DEFINE_OUTPUT( m_OnStateEnterRoundRunning, "OnStateEnterRoundRunning" ),
	DEFINE_OUTPUT( m_OnStateEnterBetweenRounds, "OnStateEnterBetweenRounds" ),
	DEFINE_OUTPUT( m_OnStateEnterPreRound, "OnStateEnterPreRound" ),
	DEFINE_OUTPUT( m_OnStateExitPreRound, "OnStateExitPreRound" ),
	DEFINE_OUTPUT( m_OnMatchSummaryStart, "OnMatchSummaryStart" ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFGameRulesProxy::CTFGameRulesProxy()
{
	m_bOvertimeAllowedForCTF = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputAddRedTeamScore( inputdata_t &inputdata )
{
	TFTeamMgr()->AddTeamScore( TF_TEAM_RED, inputdata.value.Int() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputAddBlueTeamScore( inputdata_t &inputdata )
{
	TFTeamMgr()->AddTeamScore( TF_TEAM_BLUE, inputdata.value.Int() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetCTFCaptureBonusTime( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		TFGameRules()->SetCTFCaptureBonusTime( inputdata.value.Float() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetRequiredObserverTarget( inputdata_t &inputdata )
{
	const char *pszEntName = inputdata.value.String();
	CBaseEntity *pEnt = NULL;

	if ( pszEntName && pszEntName[0] )
	{
		pEnt = gEntList.FindEntityByName( NULL, pszEntName );
	}

	TFGameRules()->SetRequiredObserverTarget( pEnt );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetRedTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamRespawnWaveTime( TF_TEAM_RED, inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetBlueTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamRespawnWaveTime( TF_TEAM_BLUE, inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputAddRedTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->AddTeamRespawnWaveTime( TF_TEAM_RED, inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputAddBlueTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->AddTeamRespawnWaveTime( TF_TEAM_BLUE, inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetRedTeamGoalString( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamGoalString( TF_TEAM_RED, inputdata.value.String() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetBlueTeamGoalString( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamGoalString( TF_TEAM_BLUE, inputdata.value.String() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetRedTeamRole( inputdata_t &inputdata )
{
	CTFTeam *pTeam = TFTeamMgr()->GetTeam( TF_TEAM_RED );
	if ( pTeam )
	{
		pTeam->SetRole( inputdata.value.Int() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetBlueTeamRole( inputdata_t &inputdata )
{
	CTFTeam *pTeam = TFTeamMgr()->GetTeam( TF_TEAM_BLUE );
	if ( pTeam )
	{
		pTeam->SetRole( inputdata.value.Int() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Pass in a VO sound entry to play for RED
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputPlayVORed( inputdata_t &inputdata )
{
	const char *szSoundName = inputdata.value.String();
	if ( szSoundName )
	{
		TFGameRules()->BroadcastSound( TF_TEAM_RED, szSoundName );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Pass in a VO sound entry to play for BLUE
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputPlayVOBlue( inputdata_t &inputdata )
{
	const char *szSoundName = inputdata.value.String();
	if ( szSoundName )
	{
		TFGameRules()->BroadcastSound( TF_TEAM_BLUE, szSoundName );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Pass in a VO sound entry to play
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputPlayVO( inputdata_t &inputdata )
{
	const char *szSoundName = inputdata.value.String();
	if ( szSoundName )
	{
		TFGameRules()->BroadcastSound( 255, szSoundName );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputHandleMapEvent( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		TFGameRules()->HandleMapEvent( inputdata );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetRoundRespawnFreezeEnabled( inputdata_t &inputdata )
{
	tf_player_movement_restart_freeze.SetValue( inputdata.value.Bool() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::Activate()
{
	TFGameRules()->Activate();

	TFGameRules()->SetOvertimeAllowedForCTF( m_bOvertimeAllowedForCTF );

	ListenForGameEvent( "teamplay_round_win" );

	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::TeamPlayerCountChanged( CTFTeam *pTeam )
{
	if ( pTeam == TFTeamMgr()->GetTeam( TF_TEAM_RED ) )
	{
		m_Team1PlayersChanged.Set( pTeam->GetNumPlayers(), this, this );
	}
	else if ( pTeam == TFTeamMgr()->GetTeam( TF_TEAM_BLUE ) )
	{
		m_Team2PlayersChanged.Set( pTeam->GetNumPlayers(), this, this );
	}

	// Tell the clients
	IGameEvent *event = gameeventmanager->CreateEvent( "teams_changed" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::StateEnterRoundRunning( void )
{
	m_OnStateEnterRoundRunning.FireOutput( this, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::StateEnterBetweenRounds( void )
{
	m_OnStateEnterBetweenRounds.FireOutput( this, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::StateEnterPreRound( void )
{
	m_OnStateEnterPreRound.FireOutput( this, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::StateExitPreRound( void )
{
	m_OnStateExitPreRound.FireOutput( this, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::MatchSummaryStart( void )
{
	m_OnMatchSummaryStart.FireOutput( this, this );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::FireGameEvent( IGameEvent *event )
{
#ifdef GAME_DLL
	const char *pszEventName = event->GetName();

	if ( FStrEq( pszEventName, "teamplay_round_win" ) )
	{
		int iWinningTeam = event->GetInt( "team" );

		switch( iWinningTeam )
		{
		case TF_TEAM_RED:
			m_OnWonByTeam1.FireOutput( this, this );
			break;
		case TF_TEAM_BLUE:
			m_OnWonByTeam2.FireOutput( this, this );
			break;
		default:
			break;
		}
	}
#endif
}

// (We clamp ammo ourselves elsewhere).
ConVar ammo_max( "ammo_max", "5000", FCVAR_REPLICATED );

#ifndef CLIENT_DLL
ConVar sk_plr_dmg_grenade( "sk_plr_dmg_grenade","0");		// Very lame that the base code needs this defined
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFGameRules::Damage_IsTimeBased( int iDmgType )
{
	// Damage types that are time-based.
	return ( ( iDmgType & ( DMG_PARALYZE | DMG_NERVEGAS | DMG_DROWNRECOVER ) ) != 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFGameRules::Damage_ShowOnHUD( int iDmgType )
{
	// Damage types that have client HUD art.
	return ( ( iDmgType & ( DMG_DROWN | DMG_BURN | DMG_NERVEGAS | DMG_SHOCK ) ) != 0 );
}
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFGameRules::Damage_ShouldNotBleed( int iDmgType )
{
	// Should always bleed currently.
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::Damage_GetTimeBased( void )
{
	int iDamage = ( DMG_PARALYZE | DMG_NERVEGAS | DMG_DROWNRECOVER );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::Damage_GetShowOnHud( void )
{
	int iDamage = ( DMG_DROWN | DMG_BURN | DMG_NERVEGAS | DMG_SHOCK );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFGameRules::Damage_GetShouldNotBleed( void )
{
	return 0;
}

bool CTFGameRules::IsCommunityGameMode( void ) const
{
	return tf_gamemode_community.GetBool();
}

bool CTFGameRules::IsCompetitiveMode( void ) const
{
	return false;
}

bool CTFGameRules::IsMatchTypeCasual( void ) const
{
	return false;
}

bool CTFGameRules::IsMatchTypeCompetitive( void ) const
{
	return false;
}

bool CTFGameRules::BInMatchStartCountdown() const
{
	return false;
}

bool CTFGameRules::IsManagedMatchEnded() const
{
	return false;
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
void CTFGameRules::SyncMatchSettings()
{
	m_bMatchEnded.Set( IsManagedMatchEnded() );
}

//-----------------------------------------------------------------------------
bool CTFGameRules::StartManagedMatch()
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Fully completes the match
//-----------------------------------------------------------------------------
void CTFGameRules::EndCompetitiveMatch( void )
{
	MatchSummaryEnd();

	Log( "Competitive match ended.  Kicking all players.\n" );
	engine->ServerCommand( "kickall #TF_Competitive_Disconnect\n" );

	// Prepare for next match
	g_fGameOver = false;
	if ( !IsCommunityGameMode() )
		m_bAllowBetweenRounds = true;
	State_Transition( GR_STATE_RESTART );
	SetInWaitingForPlayers( true );
}

//-----------------------------------------------------------------------------
// Purpose: Called during CTFGameRules::Think()
//-----------------------------------------------------------------------------
void CTFGameRules::ManageCompetitiveMode( void )
{

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFGameRules::MatchmakingShouldUseStopwatchMode( void )
{
	return IsAttackDefenseMode();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFGameRules::IsAttackDefenseMode( void )
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	bool bRetVal = !HasMultipleTrains() && ( ( pMaster && ( pMaster->PlayingMiniRounds() || pMaster->ShouldSwitchTeamsOnRoundWin() ) ) );

	tf_attack_defend_map.SetValue( bRetVal );
	return bRetVal;
}

#endif // STAGING_ONLY

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFGameRules::UsePlayerReadyStatusMode( void )
{
	if ( mp_tournament.GetBool() && mp_tournament_readymode.GetBool() )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFGameRules::PlayerReadyStatus_HaveMinPlayersToEnable( void )
{
#ifdef GAME_DLL
	// Count connected players
	int nNumPlayers = 0;
	CUtlVector< CTFPlayer* > playerVector;
	CollectPlayers( &playerVector );
	FOR_EACH_VEC( playerVector, i )
	{
		if ( !playerVector[i] )
			continue;

		if ( playerVector[i]->IsFakeClient() )
			continue;

		if ( playerVector[i]->IsBot() )
			continue;

		if ( playerVector[i]->IsHLTV() )
			continue;

		if ( playerVector[i]->IsReplay() )
			continue;

		nNumPlayers++;
	}

	// Default
	int nMinPlayers = 1;
	
	if ( UsePlayerReadyStatusMode() && engine->IsDedicatedServer() )
	{
		nMinPlayers = mp_tournament_readymode_min.GetInt();
	}

	// Should be renamed to m_bEnableReady, not sure why we encoded our criteria in the names of all associated functions and variables...
	m_bHaveMinPlayersToEnableReady.Set( nNumPlayers >= nMinPlayers );

#endif

	return m_bHaveMinPlayersToEnableReady;
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
bool CTFGameRules::PlayerReadyStatus_ArePlayersOnTeamReady( int iTeam )
{
	// Non-match
	bool bAtLeastOneReady = false;
	for ( int i = 1; i <= MAX_PLAYERS; ++i )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( !pPlayer || ToTFPlayer( pPlayer )->GetTeamNumber() != iTeam )
			continue;

		if ( !m_bPlayerReady[i] )
		{
			return false;
		}
		else
		{
			bAtLeastOneReady = true;
		}
	}

	// Team isn't ready if there was nobody on it.
	return bAtLeastOneReady;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFGameRules::PlayerReadyStatus_ShouldStartCountdown( void )
{
	if ( IsTeamReady( TF_TEAM_RED ) && IsTeamReady( TF_TEAM_BLUE ) )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::PlayerReadyStatus_ResetState( void )
{
	// Reset the players
	ResetPlayerAndTeamReadyState();

	// Reset the team
	SetTeamReadyState( false, TF_TEAM_RED );
	SetTeamReadyState( false, TF_TEAM_BLUE );

	m_flRestartRoundTime.Set( -1.f );
	mp_restartgame.SetValue( 0 );
	m_bAwaitingReadyRestart = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::PlayerReadyStatus_UpdatePlayerState( CTFPlayer *pTFPlayer, bool bState )
{
	if ( !UsePlayerReadyStatusMode() )
		return;

	if ( !pTFPlayer )
		return;

	if ( pTFPlayer->GetTeamNumber() < FIRST_GAME_TEAM )
		return;

	if ( State_Get() != GR_STATE_BETWEEN_RNDS )
		return;

	// Don't allow toggling state in the final countdown
	if ( GetRoundRestartTime() > 0.f && GetRoundRestartTime() <= gpGlobals->curtime + TOURNAMENT_NOCANCEL_TIME )
		return;

	// Make sure we have enough to allow ready mode commands
	if ( !PlayerReadyStatus_HaveMinPlayersToEnable() )
		return;

	int nEntIndex = pTFPlayer->entindex();
	if ( !IsIndexIntoPlayerArrayValid(nEntIndex) )
		return;

	// Already this state
	if ( bState == IsPlayerReady( nEntIndex ) )
		return;

	SetPlayerReadyState( nEntIndex, bState );

	if ( !bState )
	{
		// Slam team status to Not Ready for any player that sets Not Ready
		m_bTeamReady.Set( pTFPlayer->GetTeamNumber(), false );

		// If everyone cancels ready state, stop the clock
		bool bAnyoneReady = false;
		for ( int i = 1; i <= MAX_PLAYERS; ++i )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
			if ( !pPlayer )
				continue;

			if ( m_bPlayerReady[i] )
			{
				bAnyoneReady = true;
				break;
			}
		}

		if ( !bAnyoneReady )
		{
			m_flRestartRoundTime.Set( -1.f );
			mp_restartgame.SetValue( 0 );
			ResetPlayerAndTeamReadyState();
		}
	}
	else
	{
		// Unofficial modes set team ready state here
		int nRed = 0;
		int nRedCount = 0;
		int nBlue = 0;
		int nBlueCount = 0;

		for ( int iTeam = FIRST_GAME_TEAM; iTeam < TFTeamMgr()->GetTeamCount(); iTeam++ )
		{
			CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
			if ( pTeam )
			{
				Assert( pTeam->GetTeamNumber() == TF_TEAM_RED || pTeam->GetTeamNumber() == TF_TEAM_BLUE );

				for ( int i = 0; i < pTeam->GetNumPlayers(); ++i )
				{
					if ( !pTeam->GetPlayer(i) )
						continue;

					if ( pTeam->GetTeamNumber() == TF_TEAM_RED && IsPlayerReady( pTeam->GetPlayer(i)->entindex() ) )
					{
						if ( !nRedCount )
						{
							nRedCount = pTeam->GetNumPlayers();
						}

						nRed++;
					}
					else if ( pTeam->GetTeamNumber() == TF_TEAM_BLUE && IsPlayerReady( pTeam->GetPlayer(i)->entindex() ) )
					{
						if ( !nBlueCount )
						{
							nBlueCount = pTeam->GetNumPlayers();
						}

						nBlue++;
					}
				}
			}
		}

		// Check for the convar that requires min team size, or just go with whatever each team has
		int nRedMin = ( mp_tournament_readymode_team_size.GetInt() > 0 ) ? mp_tournament_readymode_team_size.GetInt() : Max( nRedCount, 1 );
		int nBlueMin = ( mp_tournament_readymode_team_size.GetInt() > 0 ) ? mp_tournament_readymode_team_size.GetInt() : Max( nBlueCount, 1 );

		SetTeamReadyState( ( nRed == nRedMin ), TF_TEAM_RED );
		SetTeamReadyState( ( nBlue == nBlueMin ), TF_TEAM_BLUE );

		m_bPlayerReadyBefore[nEntIndex] = true;
	}
}
#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFGameRules::IsDefaultGameMode( void )
{
	if ( IsInArenaMode() )
		return false;

	if ( IsInTournamentMode() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFGameRules::CTFGameRules()
#ifdef GAME_DLL
	: m_flNextStrangeEventProcessTime( g_flStrangeEventBatchProcessInterval )
	, m_mapTeleportLocations( DefLessFunc(string_t) )
	, m_bMapCycleNeedsUpdate( false )
	, m_flSafeToLeaveTimer( -1.f )
	, m_flCompModeRespawnPlayersAtMatchStart( -1.f )
#else
	: m_bRecievedBaseline( false )
#endif
{
#ifdef GAME_DLL

	// Create teams.
	TFTeamMgr()->Init();

	ResetMapTime();

	// Create the team managers
//	for ( int i = 0; i < ARRAYSIZE( teamnames ); i++ )
//	{
//		CTeam *pTeam = static_cast<CTeam*>(CreateEntityByName( "tf_team" ));
//		pTeam->Init( sTeamNames[i], i );
//
//		g_Teams.AddToTail( pTeam );
//	}

	m_flIntermissionEndTime = 0.0f;
	m_flNextPeriodicThink = 0.0f;

	ListenForGameEvent( "teamplay_point_captured" );
	ListenForGameEvent( "teamplay_capture_blocked" );
	ListenForGameEvent( "teamplay_round_win" );
	ListenForGameEvent( "teamplay_flag_event" );
	ListenForGameEvent( "teamplay_round_start" );
	ListenForGameEvent( "player_disconnect" );

	Q_memset( m_vecPlayerPositions,0, sizeof(m_vecPlayerPositions) );

	m_iPrevRoundState = -1;
	m_iCurrentRoundState = -1;
	m_iCurrentMiniRoundMask = 0;

	// Lets execute a map specific cfg file
	// ** execute this after server.cfg!
	char szCommand[MAX_PATH] = { 0 };
	// Map names cannot contain quotes or control characters so this is safe but silly that we have to do it.
	Q_snprintf( szCommand, sizeof( szCommand ), "exec \"%s.cfg\"\n", STRING( gpGlobals->mapname ) );
	engine->ServerCommand( szCommand );

	m_flSendNotificationTime = 0.0f;

	m_bOvertimeAllowedForCTF = true;

	SetCTFCaptureBonusTime( -1 );

	m_flCapInProgressBuffer = 0.f;

	m_flNextFlagAlarm = 0.0f;
	m_flNextFlagAlert = 0.0f;

	m_hRequiredObserverTarget = NULL;
	m_bStopWatchWinner.Set( false );

#else // GAME_DLL
	m_bSillyGibs = CommandLine()->FindParm( "-sillygibs" ) ? true : false;
	if ( m_bSillyGibs )
	{
		cl_burninggibs.SetValue( 0 );
	}
	// @todo Tom Bui: game_newmap doesn't seem to be used...
	ListenForGameEvent( "game_newmap" );
	ListenForGameEvent( "overtime_nag" );
	ListenForGameEvent( "recalculate_holidays" );
	
#endif

	// Initialize the game type
	m_nGameType.Set( TF_GAMETYPE_UNDEFINED );

	m_bBountyModeEnabled.Set( false );

	m_bTeamsSwitched.Set( false );

	m_iGlobalAttributeCacheVersion = 0;

	// Set turbo physics on.  Do it here for now.
	sv_turbophysics.SetValue( 1 );

	// Initialize the team manager here, etc...

	// If you hit these asserts its because you added or removed a weapon type 
	// and didn't also add or remove the weapon name or damage type from the
	// arrays defined in tf_shareddefs.cpp
	COMPILE_TIME_ASSERT( TF_WEAPON_COUNT == ARRAYSIZE( g_aWeaponDamageTypes ) );

	m_iPreviousRoundWinners = TEAM_UNASSIGNED;

	m_pszTeamGoalStringRed.GetForModify()[0] = '\0';
	m_pszTeamGoalStringBlue.GetForModify()[0] = '\0';

	m_nStopWatchState.Set( STOPWATCH_CAPTURE_TIME_NOT_SET );

	mp_tournament_redteamname.Revert();
	mp_tournament_blueteamname.Revert();

	m_flCapturePointEnableTime = 0.0f;

	m_hBirthdayPlayer = NULL;

	m_nBossHealth = 0;
	m_nMaxBossHealth = 0;
	m_fBossNormalizedTravelDistance = 0.0f;

	m_flGravityMultiplier.Set( 1.0 );

	m_bShowMatchSummary.Set( false );
	m_bMapHasMatchSummaryStage.Set( false );
	m_bPlayersAreOnMatchSummaryStage.Set( false );

	m_bUseMatchHUD = false;
	m_bUsePreRoundDoors = false;

	m_bMatchEnded.Set( true );

#ifdef GAME_DLL
	m_flCheckPlayersConnectingTime = 0;
#endif
}

#ifdef GAME_DLL
void CTFGameRules::Precache( void )
{
	BaseClass::Precache();

	CTFPlayer::m_bTFPlayerNeedsPrecache = true;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
//#ifdef GAME_DLL
//extern void AddHalloweenGiftPositionsForMap( const char *pszMapName, CUtlVector<Vector> &vLocations );
//#endif

void CTFGameRules::LevelInitPostEntity( void )
{
	BaseClass::LevelInitPostEntity();

#ifdef GAME_DLL
	// Refind our proxy, because we might have had it deleted due to a mapmaker placed one
	m_hGamerulesProxy = dynamic_cast<CTFGameRulesProxy*>( gEntList.FindEntityByClassname( NULL, "tf_gamerules" ) );

	m_flMatchSummaryTeleportTime = -1.f;
#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFGameRules::GetRespawnTimeScalar( int iTeam )
{
	return BaseClass::GetRespawnTimeScalar( iTeam );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFGameRules::GetRespawnWaveMaxLength( int iTeam, bool bScaleWithNumPlayers )
{
	bool bScale = bScaleWithNumPlayers;

	float flTime = BaseClass::GetRespawnWaveMaxLength( iTeam, bScale );

	return flTime;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::FlagsMayBeCapped( void )
{
	if ( State_Get() != GR_STATE_TEAM_WIN && State_Get() != GR_STATE_PREROUND )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldDrawHeadLabels()
{
	if ( IsInTournamentMode() )
	{
		return false;
	}

	return BaseClass::ShouldDrawHeadLabels();
}

//-----------------------------------------------------------------------------
bool CTFGameRules::CanInitiateDuels( void )
{
	if ( IsInWaitingForPlayers() )
		return false;

	gamerules_roundstate_t roundState = State_Get();
	if ( ( roundState != GR_STATE_RND_RUNNING ) && ( roundState != GR_STATE_PREROUND ) )
		return false;

	return true;
}

#ifdef GAME_DLL
void CTFGameRules::KickPlayersNewMatchIDRequestFailed()
{
	// The GC failed to get a new MatchID for us.  Let's clear out and reset.
	engine->ServerCommand( "kickall #TF_Competitive_Disconnect\n" );

	// Tell the GC System to end the managed match mode -- we skipped this in StopCompetitiveMatch so we could roll the
	// managed match into a new one.
	Assert( !IsManagedMatchEnded() );
	if ( !IsManagedMatchEnded() )
	{
		Assert( IsManagedMatchEnded() );
		m_bMatchEnded.Set( true );
	}

	// Prepare for next match
	g_fGameOver = false;
	if ( !IsCommunityGameMode() )
		m_bAllowBetweenRounds = true;
	State_Transition( GR_STATE_RESTART );
	SetInWaitingForPlayers( true );
}

//-----------------------------------------------------------------------------
// Purpose: Sets current birthday player
//-----------------------------------------------------------------------------
void CTFGameRules::SetBirthdayPlayer( CBaseEntity *pEntity )
{
/*
	if ( IsBirthday() )
	{
		if ( pEntity && pEntity->IsPlayer() && pEntity != m_hBirthdayPlayer.Get() )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( pEntity );
			if ( pTFPlayer )
			{
				// new IT victim - warn them
//				ClientPrint( pTFPlayer, HUD_PRINTTALK, "#TF_HALLOWEEN_BOSS_WARN_VICTIM", player->GetPlayerName() );
//				ClientPrint( pTFPlayer, HUD_PRINTCENTER, "#TF_HALLOWEEN_BOSS_WARN_VICTIM", player->GetPlayerName() );

				CSingleUserReliableRecipientFilter filter( pTFPlayer );
				pTFPlayer->EmitSound( filter, pTFPlayer->entindex(), "Game.HappyBirthday" );

				// force them to scream when they become it
//				pTFPlayer->EmitSound( "Halloween.PlayerScream" );
			}
		}

// 		CTFPlayer *oldIT = ToTFPlayer( m_itHandle );
// 
// 		if ( oldIT && oldIT != who && oldIT->IsAlive() )
// 		{
// 			// tell old IT player they are safe
// 			CSingleUserReliableRecipientFilter filter( oldIT );
// 			oldIT->EmitSound( filter, oldIT->entindex(), "Player.TaggedOtherIT" );
// 
// 			ClientPrint( oldIT, HUD_PRINTTALK, "#TF_HALLOWEEN_BOSS_LOST_AGGRO" );
// 			ClientPrint( oldIT, HUD_PRINTCENTER, "#TF_HALLOWEEN_BOSS_LOST_AGGRO" );
// 		}

		m_hBirthdayPlayer = pEntity;
	}
	else
	{
		m_hBirthdayPlayer = NULL;
	}
*/
}


#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: remove all projectiles in the world
//-----------------------------------------------------------------------------
void CTFGameRules::RemoveAllProjectiles()
{
	for ( int i=0; i<IBaseProjectileAutoList::AutoList().Count(); ++i )
	{
		UTIL_Remove( static_cast< CBaseProjectile* >( IBaseProjectileAutoList::AutoList()[i] ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: remove all buildings in the world
//-----------------------------------------------------------------------------
void CTFGameRules::RemoveAllBuildings( bool bExplodeBuildings /*= false*/ )
{
	for ( int i=0; i<IBaseObjectAutoList::AutoList().Count(); ++i )
	{
		CBaseObject *pObj = static_cast< CBaseObject* >( IBaseObjectAutoList::AutoList()[i] );
		if ( !pObj->IsMapPlaced() )
		{
			// this is separate from the object_destroyed event, which does
			// not get sent when we remove the objects from the world
			IGameEvent *event = gameeventmanager->CreateEvent( "object_removed" );	
			if ( event )
			{
				CTFPlayer *pOwner = pObj->GetOwner();
				event->SetInt( "userid", pOwner ? pOwner->GetUserID() : -1 ); // user ID of the object owner
				event->SetInt( "objecttype", pObj->GetType() ); // type of object removed
				event->SetInt( "index", pObj->entindex() ); // index of the object removed
				gameeventmanager->FireEvent( event );
			}

			if ( bExplodeBuildings )
			{
				pObj->DetonateObject();
			}
			else
			{
				// This fixes a bug in Raid mode where we could spawn where our sentry was but 
				// we didn't get the weapons because they couldn't trace to us in FVisible
				pObj->SetSolid( SOLID_NONE );
				UTIL_Remove( pObj );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: remove all sentries ammo
//-----------------------------------------------------------------------------
void CTFGameRules::RemoveAllSentriesAmmo()
{
	for ( int i=0; i<IBaseObjectAutoList::AutoList().Count(); ++i )
	{
		CBaseObject *pObj = static_cast< CBaseObject* >( IBaseObjectAutoList::AutoList()[i] );
		if ( pObj->GetType() == OBJ_SENTRYGUN )
		{
			CObjectSentrygun *pSentry = assert_cast< CObjectSentrygun* >( pObj );
			pSentry->RemoveAllAmmo();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Removes all projectiles and buildings from world
//-----------------------------------------------------------------------------
void CTFGameRules::RemoveAllProjectilesAndBuildings( bool bExplodeBuildings /*= false*/ )
{
	RemoveAllProjectiles();
	RemoveAllBuildings( bExplodeBuildings );
}
#endif // GAME_DLL


//-----------------------------------------------------------------------------
// Purpose: Determines whether we should allow mp_timelimit to trigger a map change
//-----------------------------------------------------------------------------
bool CTFGameRules::CanChangelevelBecauseOfTimeLimit( void )
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;

	// we only want to deny a map change triggered by mp_timelimit if we're not forcing a map reset,
	// we're playing mini-rounds, and the master says we need to play all of them before changing (for maps like Dustbowl)
	if ( !m_bForceMapReset && pMaster && pMaster->PlayingMiniRounds() && pMaster->ShouldPlayAllControlPointRounds() )
	{
		if ( pMaster->NumPlayableControlPointRounds() > 0 )
		{
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::CanGoToStalemate( void )
{
	// In CTF, don't go to stalemate if one of the flags isn't at home
	if ( m_nGameType == TF_GAMETYPE_CTF )
	{
		for ( int i=0; i<ICaptureFlagAutoList::AutoList().Count(); ++i )
		{
			CCaptureFlag *pFlag = static_cast< CCaptureFlag* >( ICaptureFlagAutoList::AutoList()[i] );
			if ( pFlag->IsDropped() || pFlag->IsStolen() )
				return false;
		}

		// check that one team hasn't won by capping
		if ( CheckCapsPerRound() )
			return false;
	}

	return BaseClass::CanGoToStalemate();
}

// Classnames of entities that are preserved across round restarts
static const char *s_PreserveEnts[] =
{
	"tf_gamerules",
	"tf_team_manager",
	"tf_player_manager",
	"tf_team",
	"tf_objective_resource",
	"keyframe_rope",
	"move_rope",
	"tf_viewmodel",
	"tf_logic_bonusround",
	"vote_controller",
	"monster_resource",
	"tf_logic_cp_timer",
	"entity_rocket",
	"entity_carrier",
	"entity_sign",
	"", // END Marker
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Activate()
{
	m_nGameType.Set( TF_GAMETYPE_UNDEFINED );

	tf_gamemode_arena.SetValue( 0 );
	tf_gamemode_community.SetValue( 0 );
	tf_gamemode_cp.SetValue( 0 );
	tf_gamemode_ctf.SetValue( 0 );
	tf_gamemode_tc.SetValue( 0 );
	tf_beta_content.SetValue( 0 );
	tf_gamemode_misc.SetValue( 0 );

	tf_bot_count.SetValue( 0 );

	m_bBountyModeEnabled.Set( false );

	m_CPTimerEnts.RemoveAll();

	if ( !Q_strncmp( STRING( gpGlobals->mapname ), "tc_", 3 )  )
	{
		tf_gamemode_tc.SetValue( 1 );
	}
	
	bool bFlag = ICaptureFlagAutoList::AutoList().Count() > 0;
	if ( bFlag )
	{
		m_nGameType.Set( TF_GAMETYPE_CTF );
		tf_gamemode_ctf.SetValue( 1 );
	}
	else if ( g_hControlPointMasters.Count() ) // We have cap points in arena but we're not CP
	{
		m_nGameType.Set( TF_GAMETYPE_CP );
		tf_gamemode_cp.SetValue( 1 );
	}

	// bot roster
	CHandle<CCPTimerLogic> hCPTimer = dynamic_cast< CCPTimerLogic* >( gEntList.FindEntityByClassname( NULL, "tf_logic_cp_timer" ) );
	while ( hCPTimer != NULL )
	{
		m_CPTimerEnts.AddToTail( hCPTimer );
		hCPTimer = dynamic_cast< CCPTimerLogic* >( gEntList.FindEntityByClassname( hCPTimer, "tf_logic_cp_timer" ) );
	}

	m_bVoteCalled = false;
	m_bServerVoteOnReset = false;
	m_flVoteCheckThrottle = 0;

// 	if ( !IsInTournamentMode() )
// 	{
// 		CExtraMapEntity::SpawnExtraModel();
// 	}

	// If leaving MvM for any other game mode, clean up any sticky UI/state
	if ( IsInTournamentMode() && false )
	{
		mp_tournament.SetValue( false );
	}

	if ( tf_gamemode_tc.GetBool() )
	{
		tf_gamemode_misc.SetValue( 1 );
	}

	CBaseEntity *pStageLogic = gEntList.FindEntityByName( NULL, "competitive_stage_logic_case" );
	if ( pStageLogic )
	{
		m_bMapHasMatchSummaryStage.Set( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	bool bRetVal = true;

	if ( ( State_Get() == GR_STATE_TEAM_WIN ) && pVictim )
	{
		if ( pVictim->GetTeamNumber() == GetWinningTeam() )
		{
			CBaseTrigger *pTrigger = dynamic_cast< CBaseTrigger *>( info.GetInflictor() );

			// we don't want players on the winning team to be
			// hurt by team-specific trigger_hurt entities during the bonus time
			if ( pTrigger && pTrigger->UsesFilter() )
			{
				bRetVal = false;
			}
		}
	}

	return bRetVal;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetTeamGoalString( int iTeam, const char *pszGoal )
{
	if ( iTeam == TF_TEAM_RED )
	{
		if ( !pszGoal || !pszGoal[0] )
		{
			m_pszTeamGoalStringRed.GetForModify()[0] = '\0';
		}
		else
		{
			if ( Q_stricmp( m_pszTeamGoalStringRed.Get(), pszGoal ) )
			{
				Q_strncpy( m_pszTeamGoalStringRed.GetForModify(), pszGoal, MAX_TEAMGOAL_STRING );
			}
		}
	}
	else if ( iTeam == TF_TEAM_BLUE )
	{
		if ( !pszGoal || !pszGoal[0] )
		{
			m_pszTeamGoalStringBlue.GetForModify()[0] = '\0';
		}
		else
		{
			if ( Q_stricmp( m_pszTeamGoalStringBlue.Get(), pszGoal ) )
			{
				Q_strncpy( m_pszTeamGoalStringBlue.GetForModify(), pszGoal, MAX_TEAMGOAL_STRING );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::RoundCleanupShouldIgnore( CBaseEntity *pEnt )
{
	if ( FindInList( s_PreserveEnts, pEnt->GetClassname() ) )
		return true;

	//There has got to be a better way of doing this.
	if ( Q_strstr( pEnt->GetClassname(), "tf_weapon_" ) )
		return true;

	return BaseClass::RoundCleanupShouldIgnore( pEnt );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldCreateEntity( const char *pszClassName )
{
	if ( FindInList( s_PreserveEnts, pszClassName ) )
		return false;

	return BaseClass::ShouldCreateEntity( pszClassName );
}

const char* CTFGameRules::GetStalemateSong( int nTeam )
{
	return "Game.Stalemate";
}

const char* CTFGameRules::WinSongName( int nTeam )
{
	return "Game.YourTeamWon"; 
}


const char* CTFGameRules::LoseSongName( int nTeam )
{
	return BaseClass::LoseSongName( nTeam );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::CleanUpMap( void )
{
#ifdef GAME_DLL
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pTFPlayer )
			continue;

		// Remove all player conditions to prevent some dependency bugs
		pTFPlayer->m_Shared.RemoveAllCond();
	}
#endif // GAME_DLL

	BaseClass::CleanUpMap();

	if ( HLTVDirector() )
	{
		HLTVDirector()->BuildCameraList();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::RecalculateControlPointState( void )
{
	Assert( ObjectiveResource() );

	if ( !g_hControlPointMasters.Count() )
		return;

	if ( g_pObjectiveResource && g_pObjectiveResource->PlayingMiniRounds() )
		return;

	for ( int iTeam = LAST_SHARED_TEAM+1; iTeam < GetNumberOfTeams(); iTeam++ )
	{
		int iFarthestPoint = GetFarthestOwnedControlPoint( iTeam, true );
		if ( iFarthestPoint == -1 )
			continue;

		// Now enable all spawn points for that spawn point
		for ( int i=0; i<ITFTeamSpawnAutoList::AutoList().Count(); ++i )
		{
			CTFTeamSpawn *pTFSpawn = static_cast< CTFTeamSpawn* >( ITFTeamSpawnAutoList::AutoList()[i] );
			if ( pTFSpawn->GetControlPoint() )
			{
				if ( pTFSpawn->GetTeamNumber() == iTeam )
				{
					if ( pTFSpawn->GetControlPoint()->GetPointIndex() == iFarthestPoint )
					{
						pTFSpawn->SetDisabled( false );
					}
					else
					{
						pTFSpawn->SetDisabled( true );
					}
				}
			}
		}
	}
}

DECLARE_AUTO_LIST( ITFTeleportLocationAutoList )
class CTFTeleportLocation : public CPointEntity, public ITFTeleportLocationAutoList
{
public:
	DECLARE_CLASS( CTFTeleportLocation, CPointEntity );
};

IMPLEMENT_AUTO_LIST( ITFTeleportLocationAutoList );

LINK_ENTITY_TO_CLASS( tf_teleport_location, CTFTeleportLocation );

//-----------------------------------------------------------------------------
// Purpose: Called when a new round is being initialized
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnRoundStart( void )
{
	for ( int i = 0; i < MAX_TEAMS; i++ )
	{
		ObjectiveResource()->SetBaseCP( -1, i );
	}

	for ( int i = 0; i < TF_TEAM_COUNT; i++ )
	{
		m_iNumCaps[i] = 0;
	}

	SetOvertime( false );

	// Let all entities know that a new round is starting
	CBaseEntity *pEnt = gEntList.FirstEnt();
	while( pEnt )
	{
		variant_t emptyVariant;
		pEnt->AcceptInput( "RoundSpawn", NULL, NULL, emptyVariant, 0 );

		pEnt = gEntList.NextEnt( pEnt );
	}

	pEnt = gEntList.FirstEnt();
	while( pEnt )
	{
		variant_t emptyVariant;
		pEnt->AcceptInput( "RoundActivate", NULL, NULL, emptyVariant, 0 );

		pEnt = gEntList.NextEnt( pEnt );
	}

	if ( g_pObjectiveResource && !g_pObjectiveResource->PlayingMiniRounds() )
	{
		// Find all the control points with associated spawnpoints
		memset( m_bControlSpawnsPerTeam, 0, sizeof(bool) * MAX_TEAMS * MAX_CONTROL_POINTS );
		for ( int i=0; i<ITFTeamSpawnAutoList::AutoList().Count(); ++i )
		{
			CTFTeamSpawn *pTFSpawn = static_cast< CTFTeamSpawn* >( ITFTeamSpawnAutoList::AutoList()[i] );
			if ( pTFSpawn->GetControlPoint() )
			{
				m_bControlSpawnsPerTeam[ pTFSpawn->GetTeamNumber() ][ pTFSpawn->GetControlPoint()->GetPointIndex() ] = true;
				pTFSpawn->SetDisabled( true );
			}
		}

		RecalculateControlPointState();

		SetRoundOverlayDetails();
	}

	m_szMostRecentCappers[0] = 0;

	SetBirthdayPlayer( NULL );

	// Tell the clients to recalculate the holiday
	IGameEvent *event = gameeventmanager->CreateEvent( "recalculate_holidays" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}

	// cache off teleport locations and remove entities to save edicts
	m_mapTeleportLocations.PurgeAndDeleteElements();
	for ( int i=0; i<ITFTeleportLocationAutoList::AutoList().Count(); ++i )
	{
		CTFTeleportLocation *pLocation = static_cast< CTFTeleportLocation* >( ITFTeleportLocationAutoList::AutoList()[i] );

		int iMap = m_mapTeleportLocations.Find( pLocation->GetEntityName() );
		if ( !m_mapTeleportLocations.IsValidIndex( iMap ) )
		{
			CUtlVector< TeleportLocation_t > *pNew = new CUtlVector< TeleportLocation_t >;
			iMap = m_mapTeleportLocations.Insert( pLocation->GetEntityName(), pNew );
		}

		CUtlVector< TeleportLocation_t > *pLocations = m_mapTeleportLocations[iMap];
		int iLocation = pLocations->AddToTail();
		pLocations->Element( iLocation ).m_vecPosition = pLocation->GetAbsOrigin();
		pLocations->Element( iLocation ).m_qAngles = pLocation->GetAbsAngles();

		UTIL_Remove( pLocation );
	}

	m_flMatchSummaryTeleportTime = -1.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::RestartTournament( void )
{
	BaseClass::RestartTournament();

	if ( GetStopWatchTimer() )
	{
		UTIL_Remove( GetStopWatchTimer() );
	}

	ResetPlayerAndTeamReadyState();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::HandleTeamScoreModify( int iTeam, int iScore )
{
	BaseClass::HandleTeamScoreModify( iTeam, iScore );

	if ( IsInStopWatch() == true )
	{
		if ( GetStopWatchTimer() )
		{
			if ( GetStopWatchTimer()->IsWatchingTimeStamps() == true )
			{
				GetStopWatchTimer()->SetStopWatchTimeStamp();
			}
	
			StopWatchModeThink();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::StopWatchShouldBeTimedWin_Calculate( void )
{
	m_bStopWatchShouldBeTimedWin = false;

	if ( IsInTournamentMode() && IsInStopWatch() && ObjectiveResource() )
	{
		int iStopWatchTimer = ObjectiveResource()->GetStopWatchTimer();
		CTeamRoundTimer *pStopWatch = dynamic_cast< CTeamRoundTimer* >( UTIL_EntityByIndex( iStopWatchTimer ) );
		if ( pStopWatch && !pStopWatch->IsWatchingTimeStamps() )
		{
			CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;

			if ( pMaster == NULL )
				return;

			int iNumPoints = pMaster->GetNumPoints();

			CTFTeam *pAttacker = NULL;
			CTFTeam *pDefender = NULL;

			for ( int i = LAST_SHARED_TEAM+1; i < GetNumberOfTeams(); i++ )
			{
				CTFTeam *pTeam = GetGlobalTFTeam( i );

				if ( pTeam )
				{
					if ( pTeam->GetRole() == TEAM_ROLE_DEFENDERS )
					{
						pDefender = pTeam;
					}

					if ( pTeam->GetRole() == TEAM_ROLE_ATTACKERS )
					{
						pAttacker = pTeam;
					}
				}
			}

			if ( pAttacker == NULL || pDefender == NULL )
				return;

			m_bStopWatchShouldBeTimedWin = ( pDefender->GetScore() == iNumPoints );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::StopWatchShouldBeTimedWin( void )
{
	StopWatchShouldBeTimedWin_Calculate();
	return m_bStopWatchShouldBeTimedWin;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::StopWatchModeThink( void )
{
	if ( IsInTournamentMode() == false || IsInStopWatch() == false )
		return;

	if ( GetStopWatchTimer() == NULL )
		return;

	CTeamRoundTimer *pTimer = GetStopWatchTimer();

	bool bWatchingCaps = pTimer->IsWatchingTimeStamps();
	
	CTFTeam *pAttacker = NULL;
	CTFTeam *pDefender = NULL;

	for ( int i = LAST_SHARED_TEAM+1; i < GetNumberOfTeams(); i++ )
	{
		CTFTeam *pTeam = GetGlobalTFTeam( i );

		if ( pTeam )
		{
			if ( pTeam->GetRole() == TEAM_ROLE_DEFENDERS )
			{
				pDefender = pTeam;
			}

			if ( pTeam->GetRole() == TEAM_ROLE_ATTACKERS )
			{
				pAttacker = pTeam;
			}
		}
	}

	if ( pAttacker == NULL || pDefender == NULL )
		return;

	m_bStopWatchWinner.Set( false );

	if ( bWatchingCaps == false )
	{
		if ( pTimer->GetTimeRemaining() <= 0.0f )
		{
			if ( StopWatchShouldBeTimedWin() )
			{
				if ( pAttacker->GetScore() < pDefender->GetScore() )
				{
					m_bStopWatchWinner.Set( true );
					SetWinningTeam( pDefender->GetTeamNumber(), WINREASON_DEFEND_UNTIL_TIME_LIMIT, true, true );
				}
			}
			else
			{
				if ( pAttacker->GetScore() > pDefender->GetScore() )
				{
					m_bStopWatchWinner.Set( true );
					SetWinningTeam( pAttacker->GetTeamNumber(), WINREASON_ALL_POINTS_CAPTURED, true, true );	
				}
			}

			if ( pTimer->IsTimerPaused() == false )
			{
				variant_t sVariant;
				pTimer->AcceptInput( "Pause", NULL, NULL, sVariant, 0 );
			}

			m_nStopWatchState.Set( STOPWATCH_OVERTIME );
		}
		else
		{
			if ( pAttacker->GetScore() >= pDefender->GetScore() )
			{
				m_bStopWatchWinner.Set( true );
				SetWinningTeam( pAttacker->GetTeamNumber(), WINREASON_ALL_POINTS_CAPTURED, true, true );
			}
		}
	}
	else
	{
		if ( pTimer->GetTimeRemaining() <= 0.0f )
		{
			m_nStopWatchState.Set( STOPWATCH_CAPTURE_TIME_NOT_SET );
		}
		else
		{
			m_nStopWatchState.Set( STOPWATCH_RUNNING );
		}
	}

	if ( m_bStopWatchWinner == true )
	{
		UTIL_Remove( pTimer	);
		m_hStopWatchTimer = NULL;
		m_flStopWatchTotalTime = -1.0f;
		m_bStopWatch = false;
		m_nStopWatchState.Set( STOPWATCH_CAPTURE_TIME_NOT_SET );

		ShouldResetRoundsPlayed( false );
		ShouldResetScores( true, false );
		ResetScores();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::ManageStopwatchTimer( bool bInSetup ) 
{
	if ( IsInTournamentMode() == false )
		return;

	if ( mp_tournament_stopwatch.GetBool() == false )
		return;

	bool bAttacking = false;
	bool bDefending = false;

	for ( int i = LAST_SHARED_TEAM+1; i < GetNumberOfTeams(); i++ )
	{
		CTFTeam *pTeam = GetGlobalTFTeam( i );

		if ( pTeam )
		{
			if ( pTeam->GetRole() == TEAM_ROLE_DEFENDERS )
			{
				bDefending = true;
			}

			if ( pTeam->GetRole() == TEAM_ROLE_ATTACKERS )
			{
				bAttacking = true;
			}
		}
	}

	if ( bDefending == true && bAttacking == true )
	{
		SetInStopWatch( true );
	}
	else
	{
		SetInStopWatch( false );
	}

	if ( IsInStopWatch() == true )
	{
		if ( m_hStopWatchTimer == NULL )
		{
			variant_t sVariant;
			CTeamRoundTimer *pStopWatch = (CTeamRoundTimer*)CBaseEntity::CreateNoSpawn( "team_round_timer", vec3_origin, vec3_angle );
			m_hStopWatchTimer = pStopWatch;

			pStopWatch->SetName( MAKE_STRING("zz_stopwatch_timer") );
			pStopWatch->SetShowInHud( false );
			pStopWatch->SetStopWatch( true );

			if ( m_flStopWatchTotalTime < 0.0f )
			{
				pStopWatch->SetCaptureWatchState( true );
				DispatchSpawn( pStopWatch );
			
				pStopWatch->AcceptInput( "Enable", NULL, NULL, sVariant, 0 );
			}
			else
			{
				DispatchSpawn( pStopWatch );
				pStopWatch->SetCaptureWatchState( false );
				

				sVariant.SetInt( m_flStopWatchTotalTime );
				pStopWatch->AcceptInput( "Enable", NULL, NULL, sVariant, 0 );
				pStopWatch->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
				pStopWatch->SetAutoCountdown( true );
			}

			if ( bInSetup == true )
			{
				pStopWatch->AcceptInput( "Pause", NULL, NULL, sVariant, 0 );
			}

			ObjectiveResource()->SetStopWatchTimer( pStopWatch );
		}
		else
		{
			if ( bInSetup == false )
			{
				variant_t sVariant;
				m_hStopWatchTimer->AcceptInput( "Resume", NULL, NULL, sVariant, 0 );
			}
			else
			{
				variant_t sVariant;
				m_hStopWatchTimer->AcceptInput( "Pause", NULL, NULL, sVariant, 0 );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetSetup( bool bSetup ) 
{ 
	if ( m_bInSetup == bSetup )
		return;

	BaseClass::SetSetup( bSetup );

	ManageStopwatchTimer( bSetup );
}

//-----------------------------------------------------------------------------
// Purpose: Called when a new round is off and running
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnRoundRunning( void )
{
	// Let out control point masters know that the round has started
	for ( int i = 0; i < g_hControlPointMasters.Count(); i++ )
	{
		variant_t emptyVariant;
		if ( g_hControlPointMasters[i] )
		{
			g_hControlPointMasters[i]->AcceptInput( "RoundStart", NULL, NULL, emptyVariant, 0 );
		}
	}

	// Reset player speeds after preround lock
	CTFPlayer *pPlayer;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( !pPlayer )
			continue;

		pPlayer->TeamFortress_SetSpeed();

		if ( FNullEnt( pPlayer->edict() ) )
			continue;
		
		pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_ROUND_START );

		pPlayer->m_Shared.ResetRoundScores();
	}

	if ( m_hGamerulesProxy )
	{
		m_hGamerulesProxy->StateEnterRoundRunning();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called before a new round is started (so the previous round can end)
//-----------------------------------------------------------------------------
void CTFGameRules::PreviousRoundEnd( void )
{
	// before we enter a new round, fire the "end output" for the previous round
	if ( g_hControlPointMasters.Count() && g_hControlPointMasters[0] )
	{
		g_hControlPointMasters[0]->FireRoundEndOutput();
	}

	m_iPreviousRoundWinners = GetWinningTeam();
}

//-----------------------------------------------------------------------------
// Purpose: Called when a round has entered stalemate mode (timer has run out)
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnStalemateStart( void )
{
	// Remove everyone's objects
	RemoveAllProjectilesAndBuildings();
	
	// Respawn all the players
	RespawnPlayers( true );

	// Disable all the active health packs in the world
	m_hDisabledHealthKits.Purge();
	CHealthKit *pHealthPack = gEntList.NextEntByClass( (CHealthKit *)NULL );
	while ( pHealthPack )
	{
		if ( !pHealthPack->IsDisabled() )
		{
			pHealthPack->SetDisabled( true );
			m_hDisabledHealthKits.AddToTail( pHealthPack );
		}
		pHealthPack = gEntList.NextEntByClass( pHealthPack );
	}

	CTFPlayer *pPlayer;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( !pPlayer )
			continue;

		pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_SUDDENDEATH_START );
	}

	m_flStalemateStartTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnStalemateEnd( void )
{
	// Reenable all the health packs we disabled
	for ( int i = 0; i < m_hDisabledHealthKits.Count(); i++ )
	{
		if ( m_hDisabledHealthKits[i] )
		{
			m_hDisabledHealthKits[i]->SetDisabled( false );
		}
	}

	m_hDisabledHealthKits.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::InitTeams( void )
{
	BaseClass::InitTeams();

	// clear the player class data
	ResetFilePlayerClassInfoDatabase();
}


class CTraceFilterIgnoreTeammatesWithException : public CTraceFilterSimple
{
	DECLARE_CLASS( CTraceFilterIgnoreTeammatesWithException, CTraceFilterSimple );
public:
	CTraceFilterIgnoreTeammatesWithException( const IHandleEntity *passentity, int collisionGroup, int iIgnoreTeam, const IHandleEntity *pExceptionEntity )
		: CTraceFilterSimple( passentity, collisionGroup ), m_iIgnoreTeam( iIgnoreTeam )
	{
		m_pExceptionEntity = pExceptionEntity;
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );

		if ( pServerEntity == m_pExceptionEntity )
			return true;

		if ( pEntity->IsPlayer() && pEntity->GetTeamNumber() == m_iIgnoreTeam )
		{
			return false;
		}

		return true;
	}

	int m_iIgnoreTeam;
	const IHandleEntity *m_pExceptionEntity;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::RadiusDamage( CTFRadiusDamageInfo &info )
{
	float flRadSqr = (info.flRadius * info.flRadius);

	int iDamageEnemies = 0;
	int nDamageDealt = 0;
	// Some weapons pass a radius of 0, since their only goal is to give blast jumping ability
	if ( info.flRadius > 0 )
	{
		// Find all the entities in the radius, and attempt to damage them.
		CBaseEntity *pEntity = NULL;
		for ( CEntitySphereQuery sphere( info.vecSrc, info.flRadius ); (pEntity = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
		{
			// Skip the attacker, if we have a RJ radius set. We'll do it post.
			if ( info.flRJRadius && pEntity == info.dmgInfo->GetAttacker() )
				continue;

			// CEntitySphereQuery actually does a box test. So we need to make sure the distance is less than the radius first.
			Vector vecPos;
			pEntity->CollisionProp()->CalcNearestPoint( info.vecSrc, &vecPos );
			if ( (info.vecSrc - vecPos).LengthSqr() > flRadSqr )
				continue;

			int iDamageToEntity = info.ApplyToEntity( pEntity );
			if ( iDamageToEntity )
			{
				// Keep track of any enemies we damaged
				if ( pEntity->IsPlayer() && !pEntity->InSameTeam( info.dmgInfo->GetAttacker() ) )
				{
					nDamageDealt+= iDamageToEntity;
					iDamageEnemies++;
				}
			}
		}
	}

	// If we have a set RJ radius, use it to affect the inflictor. This way Rocket Launchers 
	// with modified damage/radius perform like the base rocket launcher when it comes to RJing.
	if ( info.flRJRadius && info.dmgInfo->GetAttacker() )
	{
		// Set our radius & damage to the base RL
		info.flRadius = info.flRJRadius;
		CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>(info.dmgInfo->GetWeapon());
		if ( pWeapon )
		{
			float flBaseDamage = pWeapon->GetTFWpnData().GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_nDamage;
			info.dmgInfo->SetDamage( flBaseDamage );
			info.dmgInfo->CopyDamageToBaseDamage();
			info.dmgInfo->SetDamagedOtherPlayers( iDamageEnemies );
		}

		// Apply ourselves to our attacker, if we're within range
		Vector vecPos;
		info.dmgInfo->GetAttacker()->CollisionProp()->CalcNearestPoint( info.vecSrc, &vecPos );
		if ( (info.vecSrc - vecPos).LengthSqr() <= (info.flRadius * info.flRadius) )
		{
			info.ApplyToEntity( info.dmgInfo->GetAttacker() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Calculate the damage falloff over distance
//-----------------------------------------------------------------------------
void CTFRadiusDamageInfo::CalculateFalloff( void )
{
	if ( dmgInfo->GetDamageType() & DMG_RADIUS_MAX )
		flFalloff = 0.f;
	else if ( dmgInfo->GetDamageType() & DMG_HALF_FALLOFF )
		flFalloff = 0.5f;
	else if ( flRadius )
		flFalloff = dmgInfo->GetDamage() / flRadius;
	else
		flFalloff = 1.f;
}

//-----------------------------------------------------------------------------
// Purpose: Attempt to apply the radius damage to the specified entity
//-----------------------------------------------------------------------------
int CTFRadiusDamageInfo::ApplyToEntity( CBaseEntity *pEntity )
{
	if ( pEntity == pEntityIgnore || pEntity->m_takedamage == DAMAGE_NO )
		return 0;

	trace_t	tr;
	CBaseEntity *pInflictor = dmgInfo->GetInflictor();

	// Check that the explosion can 'see' this entity.
	Vector vecSpot = pEntity->BodyTarget( vecSrc, false );
	CTraceFilterIgnorePlayers filterPlayers( pInflictor, COLLISION_GROUP_PROJECTILE );
	CTraceFilterIgnoreProjectiles filterProjectiles( pInflictor, COLLISION_GROUP_PROJECTILE );
	CTraceFilterIgnoreFriendlyCombatItems filterCombatItems( pInflictor, COLLISION_GROUP_PROJECTILE, pInflictor->GetTeamNumber() );
	CTraceFilterChain filterPlayersAndProjectiles( &filterPlayers, &filterProjectiles );
	CTraceFilterChain filter( &filterPlayersAndProjectiles, &filterCombatItems );

	UTIL_TraceLine( vecSrc, vecSpot, MASK_RADIUS_DAMAGE, &filter, &tr );
	if ( tr.startsolid && tr.m_pEnt )
	{
		// Return when inside an enemy combat shield and tracing against a player of that team ("absorbed")
		if ( tr.m_pEnt->IsCombatItem() && pEntity->InSameTeam( tr.m_pEnt ) && ( pEntity != tr.m_pEnt ) )
			return 0;

		filterPlayers.SetPassEntity( tr.m_pEnt );
		CTraceFilterChain filterSelf( &filterPlayers, &filterCombatItems );
		UTIL_TraceLine( vecSrc, vecSpot, MASK_RADIUS_DAMAGE, &filterSelf, &tr );
	}

	// If we don't trace the whole way to the target, and we didn't hit the target entity, we're blocked
	if ( tr.fraction != 1.f && tr.m_pEnt != pEntity )
	{
		// Don't let projectiles block damage
		return 0;
	}

	// Adjust the damage - apply falloff.
	float flAdjustedDamage = 0.0f;
	float flDistanceToEntity;

	// Rockets store the ent they hit as the enemy and have already dealt full damage to them by this time
	if ( pInflictor && ( pEntity == pInflictor->GetEnemy() ) )
	{
		// Full damage, we hit this entity directly
		flDistanceToEntity = 0;
	}
	else if ( pEntity->IsPlayer() )
	{
		// Use whichever is closer, absorigin or worldspacecenter
		float flToWorldSpaceCenter = ( vecSrc - pEntity->WorldSpaceCenter() ).Length();
		float flToOrigin = ( vecSrc - pEntity->GetAbsOrigin() ).Length();

		flDistanceToEntity = MIN( flToWorldSpaceCenter, flToOrigin );
	}
	else
	{
		flDistanceToEntity = ( vecSrc - tr.endpos ).Length();
	}

	flAdjustedDamage = RemapValClamped( flDistanceToEntity, 0, flRadius, dmgInfo->GetDamage(), dmgInfo->GetDamage() * flFalloff );

	CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>(dmgInfo->GetWeapon());
	
	// Grenades & Pipebombs do less damage to ourselves.
	if ( pEntity == dmgInfo->GetAttacker() && pWeapon )
	{
		switch( pWeapon->GetWeaponID() )
		{
			case TF_WEAPON_PIPEBOMBLAUNCHER :
			case TF_WEAPON_GRENADELAUNCHER :
				flAdjustedDamage *= 0.75f;
				break;
		}
	}

	// If we end up doing 0 damage, exit now.
	if ( flAdjustedDamage <= 0.f )
		return 0;

	// the explosion can 'see' this entity, so hurt them!
	if (tr.startsolid)
	{
		// if we're stuck inside them, fixup the position and distance
		tr.endpos = vecSrc;
		tr.fraction = 0.f;
	}

	CTakeDamageInfo adjustedInfo = *dmgInfo;
	adjustedInfo.SetDamage( flAdjustedDamage );

	Vector dir = vecSpot - vecSrc;
	VectorNormalize( dir );

	// If we don't have a damage force, manufacture one
	if ( adjustedInfo.GetDamagePosition() == vec3_origin || adjustedInfo.GetDamageForce() == vec3_origin )
	{
		CalculateExplosiveDamageForce( &adjustedInfo, dir, vecSrc );
	}
	else
	{
		// Assume the force passed in is the maximum force. Decay it based on falloff.
		float flForce = adjustedInfo.GetDamageForce().Length() * flFalloff;
		adjustedInfo.SetDamageForce( dir * flForce );
		adjustedInfo.SetDamagePosition( vecSrc );
	}

	adjustedInfo.ScaleDamageForce( m_flForceScale );

	int nDamageTaken = 0;
	if ( tr.fraction != 1.0 && pEntity == tr.m_pEnt )
	{
		ClearMultiDamage( );
		pEntity->DispatchTraceAttack( adjustedInfo, dir, &tr );
		ApplyMultiDamage();
	}
	else
	{
		nDamageTaken = pEntity->TakeDamage( adjustedInfo );
	}

	// Now hit all triggers along the way that respond to damage.
	pEntity->TraceAttackToTriggers( adjustedInfo, vecSrc, tr.endpos, dir );

	return nDamageTaken;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//			&vecSrcIn - 
//			flRadius - 
//			iClassIgnore - 
//			*pEntityIgnore - 
//-----------------------------------------------------------------------------
void CTFGameRules::RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore )
{
	// This shouldn't be used. Call the version above that takes a CTFRadiusDamageInfo pointer.
	Assert(0);

	CTakeDamageInfo dmgInfo = info;
	Vector vecSrc = vecSrcIn;
	CTFRadiusDamageInfo radiusinfo( &dmgInfo, vecSrc, flRadius, pEntityIgnore );
	RadiusDamage(radiusinfo);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ApplyOnDamageModifyRules( CTakeDamageInfo &info, CBaseEntity *pVictimBaseEntity )
{
	info.SetDamageForForceCalc( info.GetDamage() );
	bool bDebug = tf_debug_damage.GetBool();

	CTFPlayer *pVictim = ToTFPlayer( pVictimBaseEntity );
	CBaseEntity *pAttacker = info.GetAttacker();
	CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( info.GetWeapon() );

	// damage may not come from a weapon (ie: Bosses, etc)
	// The existing code below already checked for NULL pWeapon, anyways
	float flDamage = info.GetDamage();

	bool bShowDisguisedCrit = false;
	bool bAllSeeCrit = false;
	EAttackBonusEffects_t eBonusEffect = kBonusEffect_None;

	if ( pVictim )
	{
		pVictim->SetSeeCrit( false, false );
		pVictim->SetAttackBonusEffect( kBonusEffect_None );
	}

	int bitsDamage = info.GetDamageType();

	// Capture this before anybody mucks with it
	if ( !info.BaseDamageIsValid() )
	{
		info.CopyDamageToBaseDamage();
	}
	
	// For awarding assist damage stat later
	if ( pVictim )
	{
		pVictim->SetSeeCrit( bAllSeeCrit, bShowDisguisedCrit );
		pVictim->SetAttackBonusEffect( eBonusEffect );
	}

	// If we're invulnerable, force ourselves to only take damage events only, so we still get pushed
	if ( pVictim && pVictim->m_Shared.IsInvulnerable() )
	{
		int iOldTakeDamage = pVictim->m_takedamage;
		pVictim->m_takedamage = DAMAGE_EVENTS_ONLY;
		// NOTE: Deliberately skip base player OnTakeDamage, because we don't want all the stuff it does re: suit voice
		pVictim->CBaseCombatCharacter::OnTakeDamage( info );
		pVictim->m_takedamage = iOldTakeDamage;

		// Burn sounds are handled in ConditionThink()
		if ( !(bitsDamage & DMG_BURN ) )
		{
			pVictim->SpeakConceptIfAllowed( MP_CONCEPT_HURT );
		}

		// If this is critical explosive damage, and the Medic giving us invuln triggered 
		// it in the last second, he's earned himself an achievement. 
		if ( (bitsDamage & DMG_CRITICAL) && (bitsDamage & DMG_BLAST) )
		{
			pVictim->m_Shared.CheckForAchievement( ACHIEVEMENT_TF_MEDIC_SAVE_TEAMMATE );
		}

		return false;
	}

	// A note about why crits now go through the randomness/variance code:
	// Normally critical damage is not affected by variance.  However, we always want to measure what that variance 
	// would have been so that we can lump it into the DamageBonus value inside the info.  This means crits actually
	// boost more than 3X when you factor the reduction we avoided.  Example: a rocket that normally would do 50
	// damage due to range now does the original 100, which is then multiplied by 3, resulting in a 6x increase.
	bool bCrit = ( bitsDamage & DMG_CRITICAL ) ?  true : false;

	// If we're not damaging ourselves, apply randomness
	if ( pAttacker != pVictimBaseEntity && !(bitsDamage & (DMG_DROWN | DMG_FALL)) ) 
	{
		float flDmgVariance = 0.f;

		// Minicrits still get short range damage bonus
		bool bForceCritFalloff = ( bitsDamage & DMG_USEDISTANCEMOD ) && 
								 ( ( bCrit && tf_weapon_criticals_distance_falloff.GetBool() ) );
		bool bDoShortRangeDistanceIncrease = !bCrit;
		bool bDoLongRangeDistanceDecrease = ( bForceCritFalloff || ( !bCrit ) );

		// If we're doing any distance modification, we need to do that first
		float flRandomDamage = info.GetDamage() * tf_damage_range.GetFloat();

		float flRandomDamageSpread = 0.10f;
		float flMin = 0.5 - flRandomDamageSpread;
		float flMax = 0.5 + flRandomDamageSpread;

		if ( bitsDamage & DMG_USEDISTANCEMOD )
		{
			Vector vAttackerPos = pAttacker->WorldSpaceCenter();
			float flOptimalDistance = 512.0;

			// Use Sentry position for distance mod
			CObjectSentrygun *pSentry = dynamic_cast<CObjectSentrygun*>( info.GetInflictor() );
			if ( pSentry )
			{
				vAttackerPos = pSentry->WorldSpaceCenter();
				// Sentries have a much further optimal distance
				flOptimalDistance = SENTRY_MAX_RANGE;
			}
			// The base sniper rifle doesn't have DMG_USEDISTANCEMOD, so this isn't used. Unlockable rifle had it for a bit.
			else if ( pWeapon && WeaponID_IsSniperRifle( pWeapon->GetWeaponID() ) )
			{
				flOptimalDistance *= 2.5f;
			}

			float flDistance = MAX( 1.0, ( pVictimBaseEntity->WorldSpaceCenter() - vAttackerPos).Length() );
				
			float flCenter = RemapValClamped( flDistance / flOptimalDistance, 0.0, 2.0, 1.0, 0.0 );
			if ( ( flCenter > 0.5 && bDoShortRangeDistanceIncrease ) || flCenter <= 0.5 )
			{
				if ( bitsDamage & DMG_NOCLOSEDISTANCEMOD )
				{
					if ( flCenter > 0.5 )
					{
						// Reduce the damage bonus at close range
						flCenter = RemapVal( flCenter, 0.5, 1.0, 0.5, 0.65 );
					}
				}
				flMin = MAX( 0.0, flCenter - flRandomDamageSpread );
				flMax = MIN( 1.0, flCenter + flRandomDamageSpread );

				if ( bDebug )
				{
					Warning("    RANDOM: Dist %.2f, Ctr: %.2f, Min: %.2f, Max: %.2f\n", flDistance, flCenter, flMin, flMax );
				}
			}
			else
			{
				if ( bDebug )
				{
					Warning("    NO DISTANCE MOD: Dist %.2f, Ctr: %.2f, Min: %.2f, Max: %.2f\n", flDistance, flCenter, flMin, flMax );
				}
			}
		}
		//Msg("Range: %.2f - %.2f\n", flMin, flMax );
		float flRandomRangeVal;
		if ( tf_damage_disablespread.GetBool() )
		{
			flRandomRangeVal = flMin + flRandomDamageSpread;
		}
		else
		{
			flRandomRangeVal = RandomFloat( flMin, flMax );
		}

		//if ( bDebug )
		//{
		//	Warning( "            Val: %.2f\n", flRandomRangeVal );
		//}

		// Weapon Based Damage Mod
		if ( pWeapon && pAttacker && pAttacker->IsPlayer() )
		{
			switch ( pWeapon->GetWeaponID() )
			{
			// Rocket launcher only has half the bonus of the other weapons at short range
			// Rocket Launchers
			case TF_WEAPON_ROCKETLAUNCHER :
				if ( flRandomRangeVal > 0.5 )
				{
					flRandomDamage *= 0.5f;
				}
				break;
			case TF_WEAPON_SCATTERGUN :
				// Scattergun gets 50% bonus at short range
				if ( flRandomRangeVal > 0.5 )
				{
					flRandomDamage *= 1.5f;
				}
				break;
			}
		}

		// Random damage variance.
		flDmgVariance = SimpleSplineRemapValClamped( flRandomRangeVal, 0, 1, -flRandomDamage, flRandomDamage );
		if ( ( bDoShortRangeDistanceIncrease && flDmgVariance > 0.f ) || bDoLongRangeDistanceDecrease )
		{
			flDamage += flDmgVariance;
		}

		if ( bDebug )
		{
			Warning("            Out: %.2f -> Final %.2f\n", flDmgVariance, flDamage );
		}

		/*
		for ( float flVal = flMin; flVal <= flMax; flVal += 0.05 )
		{
			float flOut = SimpleSplineRemapValClamped( flVal, 0, 1, -flRandomDamage, flRandomDamage );
			Msg("Val: %.2f, Out: %.2f, Dmg: %.2f\n", flVal, flOut, info.GetDamage() + flOut );
		}
		*/

		// Burn sounds are handled in ConditionThink()
		if ( !(bitsDamage & DMG_BURN ) && pVictim )
		{
			pVictim->SpeakConceptIfAllowed( MP_CONCEPT_HURT );
		}


		// Save any bonus damage as a separate value
		float flCritDamage = 0.f;
		// Yes, it's weird that we sometimes fabs flDmgVariance.  Here's why: In the case of a crit rocket, we
		// know that number will generally be negative due to dist or randomness.  In this case, we want to track
		// that effect - even if we don't apply it.  In the case of our crit rocket that normally would lose 50 
		// damage, we fabs'd so that we can account for it as a bonus - since it's present in a crit.
		float flBonusDamage = bForceCritFalloff ? 0.f : fabs( flDmgVariance );
		CTFPlayer *pProvider = NULL;

		auto lambdaDoFullCrit = [&]()
		{
			COMPILE_TIME_ASSERT( TF_DAMAGE_CRIT_MULTIPLIER > 1.f );
			flCritDamage = ( TF_DAMAGE_CRIT_MULTIPLIER - 1.f ) * flDamage;
		
			if ( bDebug )
			{
				Warning( "    CRITICAL! Damage: %.2f\n", flDamage );
			}

			// Burn sounds are handled in ConditionThink()
			if ( !(bitsDamage & DMG_BURN ) && pVictim )
			{
				pVictim->SpeakConceptIfAllowed( MP_CONCEPT_HURT, "damagecritical:1" );
			}
		};

		if ( bCrit )
		{
			lambdaDoFullCrit();
		}

		// Store the extra damage and update actual damage
		if ( bCrit )
		{
			info.SetDamageBonus( flCritDamage + flBonusDamage, pProvider );	// Order-of-operations sensitive, but fine as long as TF_COND_CRITBOOSTED is last
		}

		flDamage += flCritDamage;
	}

	info.SetDamage( flDamage );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFGameRules::ApplyOnDamageAliveModifyRules( const CTakeDamageInfo &info, CBaseEntity *pVictimBaseEntity, bool &bIgniting )
{
	CTFPlayer *pVictim = ToTFPlayer( pVictimBaseEntity );
	CBaseEntity *pAttacker = info.GetAttacker();

	float flRealDamage = info.GetDamage();

	if ( pVictimBaseEntity && pVictimBaseEntity->m_takedamage != DAMAGE_EVENTS_ONLY && pVictim )
	{
		int iDamageTypeBits = info.GetDamageType() & DMG_IGNITE;

		// Start burning if we took ignition damage
		bIgniting = ( ( iDamageTypeBits & DMG_IGNITE ) && ( !pVictim || pVictim->GetWaterLevel() < WL_Waist ) );

		// Resists and Boosts
		float flDamageBonus = info.GetDamageBonus();
		float flDamageBase = flRealDamage - flDamageBonus;

		if ( sv_cheats && !sv_cheats->GetBool() )
		{
			Assert( flDamageBase >= 0.f );
		}

		// Reduce only the crit portion of the damage with crit resist
		//bool bCrit = ( info.GetDamageType() & DMG_CRITICAL ) > 0;

		if ( (info.GetDamageType() & (DMG_BLAST) ) )
		{
			bool bReduceBlast = false;

			// If someone else shot us or we're in MvM
			if( pAttacker != pVictimBaseEntity )
			{
				bReduceBlast = true;
			}
		}

		// Stomp flRealDamage with resist adjusted values
		flRealDamage = flDamageBase + flDamageBonus;

		if ( pVictim && pVictim->IsPlayerClass( TF_CLASS_SPY ) && !pVictim->IsTaunting() )
		{
			// Reduce damage taken if we have recently feigned death.
			if ( pVictim->m_Shared.InCond( TF_COND_STEALTHED ) )
			{
				flRealDamage *= tf_stealth_damage_reduction.GetFloat();
			}
		}

		if ( sv_cheats && !sv_cheats->GetBool() )
		{
			if ( flRealDamage <= 0.0f )
			{
				// Do a hard out in the caller
				return -1;
			}
		}
		else
		{
			// allow negative health values for things like the hurtme command
			if ( flRealDamage == 0.0f )
			{
				// Do a hard out in the caller
				return -1;
			}
		}
	}

	return flRealDamage;
}

// --------------------------------------------------------------------------------------------------- //
// Voice helper
// --------------------------------------------------------------------------------------------------- //

class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	virtual bool		CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker, bool &bProximity )
	{
		return TFGameRules()->TFVoiceManager( pListener, pTalker );
	}
};
CVoiceGameMgrHelper g_VoiceGameMgrHelper;
IVoiceGameMgrHelper *g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;

// Load the objects.txt file.
class CObjectsFileLoad : public CAutoGameSystem
{
public:
	virtual bool Init()
	{
		LoadObjectInfos( filesystem );
		return true;
	}
} g_ObjectsFileLoad;

// --------------------------------------------------------------------------------------------------- //
// Globals.
// --------------------------------------------------------------------------------------------------- //
/*
// NOTE: the indices here must match TEAM_UNASSIGNED, TEAM_SPECTATOR, TF_TEAM_RED, TF_TEAM_BLUE, etc.
char *sTeamNames[] =
{
	"Unassigned",
	"Spectator",
	"Red",
	"Blue"
};
*/
// --------------------------------------------------------------------------------------------------- //
// Global helper functions.
// --------------------------------------------------------------------------------------------------- //
	
// World.cpp calls this but we don't use it in TF.
void InitBodyQue()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFGameRules::~CTFGameRules()
{
	// Note, don't delete each team since they are in the gEntList and will 
	// automatically be deleted from there, instead.
	TFTeamMgr()->Shutdown();
	ShutdownCustomResponseRulesDicts();

	// clean up cached teleport locations
	m_mapTeleportLocations.PurgeAndDeleteElements();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::CheckTauntAchievement( CTFPlayer *pAchiever, int nGibs, int *pTauntCamAchievements )
{
	if ( !pAchiever || !pAchiever->GetPlayerClass() )
		return;

	int iClass = pAchiever->GetPlayerClass()->GetClassIndex();
	if ( pTauntCamAchievements[ iClass ] )
	{
		bool bAwardAchievement = true;

		// for the Heavy achievement, the player needs to also be invuln
		if ( iClass == TF_CLASS_HEAVYWEAPONS && pTauntCamAchievements[ iClass ] == ACHIEVEMENT_TF_HEAVY_FREEZECAM_TAUNT )
		{
			if ( !pAchiever->m_Shared.InCond( TF_COND_INVULNERABLE_WEARINGOFF ) && !pAchiever->m_Shared.InCond( TF_COND_INVULNERABLE ) )
			{
				bAwardAchievement = false;
			}
		}

		// for the Spy achievement, we must be in the cig lighter taunt
		if ( iClass == TF_CLASS_SPY && pTauntCamAchievements[ iClass ] == ACHIEVEMENT_TF_SPY_FREEZECAM_FLICK )
		{
			if ( pAchiever->GetActiveTFWeapon() && pAchiever->GetActiveTFWeapon()->GetWeaponID() != TF_WEAPON_PDA_SPY )
			{
				bAwardAchievement = false;
			}
		}

		// for the two Sniper achievements, we need to check for specific taunts
		if ( iClass == TF_CLASS_SNIPER )
		{
			if ( pTauntCamAchievements[ iClass ] == ACHIEVEMENT_TF_SNIPER_FREEZECAM_HAT )
			{
				if ( pAchiever->GetActiveTFWeapon() && pAchiever->GetActiveTFWeapon()->GetWeaponID() != TF_WEAPON_CLUB )
				{
					bAwardAchievement = false;
				}
			}
			else if ( pTauntCamAchievements[ iClass ] == ACHIEVEMENT_TF_SNIPER_FREEZECAM_WAVE  )
			{
				if ( pAchiever->GetActiveTFWeapon() && WeaponID_IsSniperRifle( pAchiever->GetActiveTFWeapon()->GetWeaponID() ) )
				{
					bAwardAchievement = false;
				}
			}
		}

		// For the Soldier achievements, we need to be doing a specific taunt, or have enough gibs onscreen
		if ( iClass == TF_CLASS_SOLDIER )
		{
			if ( pTauntCamAchievements[ iClass ] == ACHIEVEMENT_TF_SOLDIER_FREEZECAM_TAUNT )
			{
				if ( pAchiever->GetActiveTFWeapon() && pAchiever->GetActiveTFWeapon()->GetWeaponID() != TF_WEAPON_SHOTGUN_SOLDIER )
				{
					bAwardAchievement = false;
				}
			}
			else if ( pTauntCamAchievements[ iClass ] == ACHIEVEMENT_TF_SOLDIER_FREEZECAM_GIBS )
			{
				// Need at least 3 gibs on-screen
				if ( nGibs < 3 )
				{
					bAwardAchievement = false;
				}
			}
		}

		// for the two Demoman achievements, we need to check for specific taunts
		if ( iClass == TF_CLASS_DEMOMAN )
		{
			if ( pTauntCamAchievements[ iClass ] == ACHIEVEMENT_TF_DEMOMAN_FREEZECAM_SMILE )
			{
				if ( pAchiever->GetActiveTFWeapon() && pAchiever->GetActiveTFWeapon()->GetWeaponID() != TF_WEAPON_GRENADELAUNCHER )
				{
					bAwardAchievement = false;
				}
			}
		}

		// for the Engineer achievement, we must be in the guitar taunt
		if ( iClass == TF_CLASS_ENGINEER && pTauntCamAchievements[ iClass ] == ACHIEVEMENT_TF_ENGINEER_FREEZECAM_TAUNT )
		{
			if ( pAchiever->GetActiveTFWeapon() )
			{
				bAwardAchievement = false;
			}
		}

		if ( bAwardAchievement )
		{
			pAchiever->AwardAchievement( pTauntCamAchievements[ iClass ] );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::TFVoiceManager( CBasePlayer *pListener, CBasePlayer *pTalker )
{
	if ( pTalker && pTalker->BHaveChatSuspensionInCurrentMatch() )
		return false;

	// check coaching--we only want coaches and students to talk and listen to each other!

	if ( !tf_gravetalk.GetBool() )
	{
		// Dead players can only be heard by other dead team mates but only if a match is in progress
		if ( State_Get() != GR_STATE_TEAM_WIN && State_Get() != GR_STATE_GAME_OVER ) 
		{
			if ( pTalker->IsAlive() == false )
			{
				if ( pListener->IsAlive() == false )
					return ( pListener->InSameTeam( pTalker ) );

				return false;
			}
		}
	}

	return ( pListener->InSameTeam( pTalker ) );

}

//-----------------------------------------------------------------------------
// Purpose: TF2 Specific Client Commands
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CTFGameRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
{
	CTFPlayer *pPlayer = ToTFPlayer( pEdict );

	const char *pcmd = args[0];

	if ( IsInTournamentMode() == true && IsInPreMatch() == true )
	{
		if ( FStrEq( pcmd, "tournament_readystate" ) )
		{
			if ( UsePlayerReadyStatusMode() )
				return true;

			if ( args.ArgC() < 2 )
				return true;

			if ( pPlayer->GetTeamNumber() <= LAST_SHARED_TEAM )
				return true;

			int iReadyState = atoi( args[1] );

			//Already this state
			if ( iReadyState == (int)IsTeamReady( pPlayer->GetTeamNumber() ) )
				return true;

			SetTeamReadyState( iReadyState == 1, pPlayer->GetTeamNumber() );

			IGameEvent * event = gameeventmanager->CreateEvent( "tournament_stateupdate" );

			if ( event )
			{
				event->SetInt( "userid", pPlayer->entindex() );
				event->SetInt( "readystate", iReadyState );
				event->SetBool( "namechange", 0 );
				event->SetString( "oldname", " " );
				event->SetString( "newname", " " );

				gameeventmanager->FireEvent( event );
			}

			if ( iReadyState == 0 )
			{
				m_flRestartRoundTime.Set( -1.f ); 
				m_bAwaitingReadyRestart = true;
			}

			return true;
		}

		if ( FStrEq( pcmd, "tournament_teamname" ) )
		{
			if ( args.ArgC() < 2 )
				return true;

			if ( pPlayer->GetTeamNumber() <= LAST_SHARED_TEAM )
				return true;

			const char *commandline = args.GetCommandString();

			// find the rest of the command line past the bot index
			commandline = strstr( commandline, args[1] );
			Assert( commandline );

			char szName[MAX_TEAMNAME_STRING + 1] = { 0 };
			Q_strncpy( szName, commandline, sizeof( szName ));

			if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
			{
				if ( FStrEq( szName, mp_tournament_blueteamname.GetString() ) == true )
					return true;

				mp_tournament_blueteamname.SetValue( szName );
			}
			else if ( pPlayer->GetTeamNumber() == TF_TEAM_RED )
			{
				if ( FStrEq( szName, mp_tournament_redteamname.GetString() ) == true )
					return true;

				mp_tournament_redteamname.SetValue( szName );
			}

			IGameEvent * event = gameeventmanager->CreateEvent( "tournament_stateupdate" );

			if ( event )
			{
				event->SetInt( "userid", pPlayer->entindex() );
				event->SetBool( "readystate", 0 );
				event->SetBool( "namechange", 1 );
				event->SetString( "newname", szName );

				gameeventmanager->FireEvent( event );
			}

			return true;
		}

		if ( FStrEq( pcmd, "tournament_player_readystate" ) )
		{
			if ( State_Get() != GR_STATE_BETWEEN_RNDS )
				return true;

			// Make sure we have enough to allow ready mode commands
			if ( !PlayerReadyStatus_HaveMinPlayersToEnable() )
				return true;

			if ( args.ArgC() < 2 )
				return true;

			bool bReady = ( atoi( args[1] ) == 1 );
			PlayerReadyStatus_UpdatePlayerState( pPlayer, bReady );
			if ( bReady )
			{
				pPlayer->PlayReadySound();
			}

			return true;
		}
	}

	if ( FStrEq( pcmd, "objcmd" ) )
	{
		if ( args.ArgC() < 3 )
			return true;

		int entindex = atoi( args[1] );
		edict_t* pEdict = INDEXENT(entindex);
		if ( pEdict )
		{
			CBaseEntity* pBaseEntity = GetContainingEntity(pEdict);
			CBaseObject* pObject = dynamic_cast<CBaseObject*>(pBaseEntity);

			if ( pObject )
			{
				// We have to be relatively close to the object too...

				// BUG! Some commands need to get sent without the player being near the object, 
				// eg delayed dismantle commands. Come up with a better way to ensure players aren't
				// entering these commands in the console.

				//float flDistSq = pObject->GetAbsOrigin().DistToSqr( pPlayer->GetAbsOrigin() );
				//if (flDistSq <= (MAX_OBJECT_SCREEN_INPUT_DISTANCE * MAX_OBJECT_SCREEN_INPUT_DISTANCE))
				{
					// Strip off the 1st two arguments and make a new argument string
					CCommand objectArgs( args.ArgC() - 2, &args.ArgV()[2]);
					pObject->ClientCommand( pPlayer, objectArgs );
				}
			}
		}

		return true;
	}

	// Handle some player commands here as they relate more directly to gamerules state
	if ( FStrEq( pcmd, "nextmap" ) )
	{
		if ( pPlayer->m_flNextTimeCheck < gpGlobals->curtime )
		{
			char szNextMap[MAX_MAP_NAME];

			if ( nextlevel.GetString() && *nextlevel.GetString() )
			{
				Q_strncpy( szNextMap, nextlevel.GetString(), sizeof( szNextMap ) );
			}
			else
			{
				GetNextLevelName( szNextMap, sizeof( szNextMap ) );
			}

			ClientPrint( pPlayer, HUD_PRINTTALK, "#TF_nextmap", szNextMap);

			pPlayer->m_flNextTimeCheck = gpGlobals->curtime + 1;
		}

		return true;
	}
	else if ( FStrEq( pcmd, "timeleft" ) )
	{	
		if ( pPlayer->m_flNextTimeCheck < gpGlobals->curtime )
		{
			if ( mp_timelimit.GetInt() > 0 )
			{
				int iTimeLeft = GetTimeLeft();

				char szMinutes[5];
				char szSeconds[3];

				if ( iTimeLeft <= 0 )
				{
					Q_snprintf( szMinutes, sizeof(szMinutes), "0" );
					Q_snprintf( szSeconds, sizeof(szSeconds), "00" );
				}
				else
				{
					Q_snprintf( szMinutes, sizeof(szMinutes), "%d", iTimeLeft / 60 );
					Q_snprintf( szSeconds, sizeof(szSeconds), "%02d", iTimeLeft % 60 );
				}

				ClientPrint( pPlayer, HUD_PRINTTALK, "#TF_timeleft", szMinutes, szSeconds );
			}
			else
			{
				ClientPrint( pPlayer, HUD_PRINTTALK, "#TF_timeleft_nolimit" );
			}

			pPlayer->m_flNextTimeCheck = gpGlobals->curtime + 1;
		}
		return true;
	}
	else if( pPlayer->ClientCommand( args ) )
	{
        return true;
	}

	return BaseClass::ClientCommand( pEdict, args );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::LevelShutdown()
{
	hide_server.Revert();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Think()
{
	if ( m_bMapCycleNeedsUpdate )
	{
		m_bMapCycleNeedsUpdate = false;
		LoadMapCycleFile();
	}

	if ( g_fGameOver )
	{
		if ( ( m_flMatchSummaryTeleportTime > 0 ) && ( gpGlobals->curtime > m_flMatchSummaryTeleportTime ) )
		{
			m_flMatchSummaryTeleportTime = -1.f;
		}
	}
	else
	{
		if ( gpGlobals->curtime > m_flNextPeriodicThink )
		{
			if ( State_Get() != GR_STATE_BONUS && State_Get() != GR_STATE_TEAM_WIN && State_Get() != GR_STATE_GAME_OVER && IsInWaitingForPlayers() == false )
			{
				if ( CheckCapsPerRound() )
					return;
			}
		}

		// These network variables mirror the MM system's match state for client's sake. Gamerules should still
		// be aware of when these change, either because we caused it or via a callback.  This warning will
		// detect desync. (Ideally we'd have the ability to just share between the client GC system and server
		// GC system directly without passing things through gamerules)
		if ( m_bMatchEnded != IsManagedMatchEnded() )
		{
			Assert( false );
			Warning( "Mirrored Match parameters on gamerules don't match MatchInfo\n" );
			m_bMatchEnded.Set( IsManagedMatchEnded() );
		}
	}

	if ( g_bRandomMap == true )
	{
		g_bRandomMap = false;

		char szNextMap[MAX_MAP_NAME];
		GetNextLevelName( szNextMap, sizeof(szNextMap), true );
		IncrementMapCycleIndex();

		ChangeLevelToMap( szNextMap );

		return;
	}

	// periodically count up the fake clients and set the bot_count cvar to update server tags
	if ( m_botCountTimer.IsElapsed() )
	{
		m_botCountTimer.Start( 5.0f );

		int botCount = 0;
		for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
		{
			CTFPlayer *player = ToTFPlayer( UTIL_PlayerByIndex( i ) );

			if ( player && player->IsFakeClient() )
			{
				++botCount;
			}
		}

		tf_bot_count.SetValue( botCount );
	}

#ifdef GAME_DLL
	// Josh:
	// This is global because it handles maps and stuff.
	if ( g_voteControllerGlobal )
	{
		ManageServerSideVoteCreation();
	}

	if ( m_flNextFlagAlarm > 0.0f )
	{
		m_flNextFlagAlarm = 0.0f;
		m_flNextFlagAlert = gpGlobals->curtime + 5.0f;
	}

	// Batched strange event message processing?
	if ( engine->Time() > m_flNextStrangeEventProcessTime )
	{
		m_flNextStrangeEventProcessTime = engine->Time() + g_flStrangeEventBatchProcessInterval;
	}

	ManageCompetitiveMode();

#endif // GAME_DLL

	BaseClass::Think();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::SwitchToNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon )
{
	if ( pPlayer )
	{
		CBaseCombatWeapon *lastWeapon = ToTFPlayer( pPlayer )->GetLastWeapon();	

		if ( lastWeapon != NULL && lastWeapon->HasAnyAmmo() )
		{
			return pPlayer->Weapon_Switch( lastWeapon );
		}
	}

	return BaseClass::SwitchToNextBestWeapon( pPlayer, pCurrentWeapon );
}

//Runs think for all player's conditions
//Need to do this here instead of the player so players that crash still run their important thinks
void CTFGameRules::RunPlayerConditionThink ( void )
{
	for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer )
		{
			pPlayer->m_Shared.ConditionGameRulesThink();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::FrameUpdatePostEntityThink()
{
	BaseClass::FrameUpdatePostEntityThink();

	RunPlayerConditionThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::CheckCapsPerRound()
{
	return SetCtfWinningTeam();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::SetCtfWinningTeam()
{
	if ( tf_flag_caps_per_round.GetInt() > 0 )
	{
		int iMaxCaps = -1;
		CTFTeam *pMaxTeam = NULL;

		// check to see if any team has won a "round"
		int nTeamCount = TFTeamMgr()->GetTeamCount();
		for ( int iTeam = FIRST_GAME_TEAM; iTeam < nTeamCount; ++iTeam )
		{
			CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
			if ( !pTeam )
				continue;

			// we might have more than one team over the caps limit (if the server op lowered the limit)
			// so loop through to see who has the most among teams over the limit
			if ( pTeam->GetFlagCaptures() >= tf_flag_caps_per_round.GetInt() )
			{
				if ( pTeam->GetFlagCaptures() > iMaxCaps )
				{
					iMaxCaps = pTeam->GetFlagCaptures();
					pMaxTeam = pTeam;
				}
			}
		}

		if ( iMaxCaps != -1 && pMaxTeam != NULL )
		{
			SetWinningTeam( pMaxTeam->GetTeamNumber(), WINREASON_FLAG_CAPTURE_LIMIT );
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::CheckWinLimit( bool bAllowEnd /*= true*/, int nAddValueWhenChecking /*= 0*/ )
{
	if ( IsInPreMatch() )
		return false;

	bool bWinner = false;
	int iTeam = TEAM_UNASSIGNED;
	int iReason = WINREASON_NONE;
	const char *pszReason = "";

	if ( mp_winlimit.GetInt() != 0 )
	{
		if ( ( TFTeamMgr()->GetTeam( TF_TEAM_BLUE )->GetScore() + nAddValueWhenChecking ) >= mp_winlimit.GetInt() )
		{
			pszReason = "Team \"BLUE\" triggered \"Intermission_Win_Limit\"\n";
			bWinner = true;
			iTeam = TF_TEAM_BLUE;
			iReason = WINREASON_WINLIMIT;
		}
		else if ( ( TFTeamMgr()->GetTeam( TF_TEAM_RED )->GetScore() + nAddValueWhenChecking ) >= mp_winlimit.GetInt() )
		{
			pszReason = "Team \"RED\" triggered \"Intermission_Win_Limit\"\n";
			bWinner = true;
			iTeam = TF_TEAM_RED;
			iReason = WINREASON_WINLIMIT;
		}
	}

	// has one team go far enough ahead of the other team to trigger the win difference?
	if ( !bWinner )
	{
		int iWinLimit = mp_windifference.GetInt();
		if ( iWinLimit > 0 )
		{
			int iBlueScore = TFTeamMgr()->GetTeam( TF_TEAM_BLUE )->GetScore();
			int iRedScore = TFTeamMgr()->GetTeam( TF_TEAM_RED )->GetScore();

			if ( (iBlueScore - iRedScore) >= iWinLimit )
			{
				if ( (mp_windifference_min.GetInt() == 0) || (iBlueScore >= mp_windifference_min.GetInt()) )
				{
					pszReason = "Team \"BLUE\" triggered \"Intermission_Win_Limit\" due to mp_windifference\n";
					bWinner = true;
					iTeam = TF_TEAM_BLUE;
					iReason = WINREASON_WINDIFFLIMIT;
				}
			}
			else if ( (iRedScore - iBlueScore) >= iWinLimit )
			{
				if ( (mp_windifference_min.GetInt() == 0) || (iRedScore >= mp_windifference_min.GetInt()) )
				{
					pszReason = "Team \"RED\" triggered \"Intermission_Win_Limit\" due to mp_windifference\n";
					bWinner = true;
					iTeam = TF_TEAM_RED;
					iReason = WINREASON_WINDIFFLIMIT;
				}
			}
		}
	}

	if ( bWinner )
	{
		if ( bAllowEnd )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "tf_game_over" );
			if ( event )
			{
				if ( iReason == WINREASON_WINDIFFLIMIT )
				{
					event->SetString( "reason", "Reached Win Difference Limit" );
				}
				else
				{
					event->SetString( "reason", "Reached Win Limit" );
				}
				gameeventmanager->FireEvent( event );
			}

			if ( IsInTournamentMode() == true )
			{
				SetWinningTeam( iTeam, iReason, true, false, true );
			}
			else
			{
				GoToIntermission();
			}

			Assert( V_strlen( pszReason ) );
			UTIL_LogPrintf( "%s", pszReason );
		}
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::CheckRespawnWaves()
{
	BaseClass::CheckRespawnWaves();

	// Look for overrides
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pTFPlayer )
			continue;

		if ( pTFPlayer->IsAlive() )
			continue; 

		if ( m_iRoundState == GR_STATE_PREROUND )
			continue;

		// Triggers can force a player to spawn at a specific time
		if ( pTFPlayer->GetRespawnTimeOverride() != -1.f && 
				gpGlobals->curtime > pTFPlayer->GetDeathTime() + pTFPlayer->GetRespawnTimeOverride() )
		{
			pTFPlayer->ForceRespawn();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::PlayWinSong( int team )
{
	if ( IsInTournamentMode() && IsInStopWatch() && ObjectiveResource() )
	{
		int iStopWatchTimer = ObjectiveResource()->GetStopWatchTimer();
		CTeamRoundTimer *pStopWatch = dynamic_cast< CTeamRoundTimer* >( UTIL_EntityByIndex( iStopWatchTimer ) );
		if ( ( pStopWatch && pStopWatch->IsWatchingTimeStamps() ) || ( !m_bForceMapReset ) )
		{
			BroadcastSound( 255, "MatchMaking.RoundEndStalemateMusic" );
			return;
		}
	}

	CTeamplayRoundBasedRules::PlayWinSong( team );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetWinningTeam( int team, int iWinReason, bool bForceMapReset /* = true */, bool bSwitchTeams /* = false*/, bool bDontAddScore /* = false*/, bool bFinal /*= false*/ )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pTFPlayer )
			continue;

		// store our team for the response rules at the next round start
		// (teams might be switched for attack/defend maps)
		pTFPlayer->SetPrevRoundTeamNum( pTFPlayer->GetTeamNumber() );

		if ( team != TEAM_UNASSIGNED )
		{
			if ( pTFPlayer->GetTeamNumber() == team )
			{

			}
			else
			{
				pTFPlayer->ClearExpression();
			}
		}
	}

#ifdef TF_RAID_MODE
	if ( !IsBossBattleMode() )
	{
		// Don't do a full reset in Raid mode if the defending team didn't win
		if ( IsRaidMode() && team != TF_TEAM_PVE_DEFENDERS )
		{
			bForceMapReset = false;
		}
	}
#endif // TF_RAID_MODE

	SetBirthdayPlayer( NULL );

#ifdef GAME_DLL
	if ( HasMultipleTrains() )
	{
		for ( int i = 0 ; i < ITFTeamTrainWatcher::AutoList().Count() ; ++i )
		{
			CTeamTrainWatcher *pTrainWatcher = static_cast< CTeamTrainWatcher* >( ITFTeamTrainWatcher::AutoList()[i] );
			if ( !pTrainWatcher->IsDisabled() )
			{
				if ( pTrainWatcher->GetTeamNumber() == TF_TEAM_RED )
				{
					GetGlobalTFTeam( TF_TEAM_RED )->AddPLRTrack( pTrainWatcher->GetTrainProgress() );
				}
				else
				{
					GetGlobalTFTeam( TF_TEAM_BLUE )->AddPLRTrack( pTrainWatcher->GetTrainProgress() );
				}
			}
		}
	}
#endif

	CTeamplayRoundBasedRules::SetWinningTeam( team, iWinReason, bForceMapReset, bSwitchTeams, bDontAddScore, bFinal );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetStalemate( int iReason, bool bForceMapReset /* = true */, bool bSwitchTeams /* = false */ )
{
	BaseClass::SetStalemate( iReason, bForceMapReset, bSwitchTeams );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFGameRules::GetPreMatchEndTime() const
{
	//TFTODO: implement this.
	return gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::GoToIntermission( void )
{
	// Tell the clients to recalculate the holiday
	IGameEvent *event = gameeventmanager->CreateEvent( "recalculate_holidays" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}

	BaseClass::GoToIntermission();
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo &info )
{
	// guard against NULL pointers if players disconnect
	if ( !pPlayer || !pAttacker )
		return false;

	// if pAttacker is an object, we can only do damage if pPlayer is our builder
	if ( pAttacker->IsBaseObject() )
	{
		CBaseObject *pObj = ( CBaseObject *)pAttacker;

		if ( pObj->GetBuilder() == pPlayer || pPlayer->GetTeamNumber() != pObj->GetTeamNumber() )
		{
			// Builder and enemies
			return true;
		}
		else
		{
			// Teammates of the builder
			return false;
		}
	}

	return BaseClass::FPlayerCanTakeDamage( pPlayer, pAttacker, info );
}

Vector DropToGround( 
	CBaseEntity *pMainEnt,
	const Vector &vPos, 
	const Vector &vMins, 
	const Vector &vMaxs )
{
	trace_t trace;
	UTIL_TraceHull( vPos, vPos + Vector( 0, 0, -500 ), vMins, vMaxs, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &trace );
	return trace.endpos;
}


void TestSpawnPointType( const char *pEntClassName )
{
	// Find the next spawn spot.
	CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, pEntClassName );

	while( pSpot )
	{
		// trace a box here
		Vector vTestMins = pSpot->GetAbsOrigin() + VEC_HULL_MIN;
		Vector vTestMaxs = pSpot->GetAbsOrigin() + VEC_HULL_MAX;

		if ( UTIL_IsSpaceEmpty( pSpot, vTestMins, vTestMaxs ) )
		{
			// the successful spawn point's location
			NDebugOverlay::Box( pSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX, 0, 255, 0, 100, 60 );

			// drop down to ground
			Vector GroundPos = DropToGround( NULL, pSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX );

			// the location the player will spawn at
			NDebugOverlay::Box( GroundPos, VEC_HULL_MIN, VEC_HULL_MAX, 0, 0, 255, 100, 60 );

			// draw the spawn angles
			QAngle spotAngles = pSpot->GetLocalAngles();
			Vector vecForward;
			AngleVectors( spotAngles, &vecForward );
			NDebugOverlay::HorzArrow( pSpot->GetAbsOrigin(), pSpot->GetAbsOrigin() + vecForward * 32, 10, 255, 0, 0, 255, true, 60 );
		}
		else
		{
			// failed spawn point location
			NDebugOverlay::Box( pSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX, 255, 0, 0, 100, 60 );
		}

		// increment pSpot
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
	}
}

// -------------------------------------------------------------------------------- //

void TestSpawns()
{
	TestSpawnPointType( "info_player_teamspawn" );
}
ConCommand cc_TestSpawns( "map_showspawnpoints", TestSpawns, "Dev - test the spawn points, draws for 60 seconds", FCVAR_CHEAT );


// -------------------------------------------------------------------------------- //

void cc_ShowRespawnTimes()
{
	CTFGameRules *pRules = TFGameRules();
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );

	if ( pRules && pPlayer )
	{
		float flRedMin = ( pRules->m_TeamRespawnWaveTimes[TF_TEAM_RED] >= 0 ? pRules->m_TeamRespawnWaveTimes[TF_TEAM_RED] : mp_respawnwavetime.GetFloat() );
		float flRedScalar = pRules->GetRespawnTimeScalar( TF_TEAM_RED );
		float flNextRedRespawn = pRules->GetNextRespawnWave( TF_TEAM_RED, NULL ) - gpGlobals->curtime;

		float flBlueMin = ( pRules->m_TeamRespawnWaveTimes[TF_TEAM_BLUE] >= 0 ? pRules->m_TeamRespawnWaveTimes[TF_TEAM_BLUE] : mp_respawnwavetime.GetFloat() );
		float flBlueScalar = pRules->GetRespawnTimeScalar( TF_TEAM_BLUE );
		float flNextBlueRespawn = pRules->GetNextRespawnWave( TF_TEAM_BLUE, NULL ) - gpGlobals->curtime;

		char tempRed[128];
		Q_snprintf( tempRed, sizeof( tempRed ),   "Red:  Min Spawn %2.2f, Scalar %2.2f, Next Spawn In: %.2f\n", flRedMin, flRedScalar, flNextRedRespawn );

		char tempBlue[128];
		Q_snprintf( tempBlue, sizeof( tempBlue ), "Blue: Min Spawn %2.2f, Scalar %2.2f, Next Spawn In: %.2f\n", flBlueMin, flBlueScalar, flNextBlueRespawn );

		ClientPrint( pPlayer, HUD_PRINTTALK, tempRed );
		ClientPrint( pPlayer, HUD_PRINTTALK, tempBlue );
	}
}

ConCommand mp_showrespawntimes( "mp_showrespawntimes", cc_ShowRespawnTimes, "Show the min respawn times for the teams", FCVAR_CHEAT );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CTFGameRules::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

	// get valid spawn point
	CBaseEntity *pSpawnSpot = pPlayer->EntSelectSpawnPoint();

	// drop down to ground
	Vector GroundPos = DropToGround( pPlayer, pSpawnSpot->GetAbsOrigin(), VEC_HULL_MIN_SCALED( pTFPlayer ), VEC_HULL_MAX_SCALED( pTFPlayer ) );

	// Move the player to the place it said.
	pPlayer->SetLocalOrigin( GroundPos + Vector(0,0,1) );
	pPlayer->SetAbsVelocity( vec3_origin );
	pPlayer->SetLocalAngles( pSpawnSpot->GetLocalAngles() );
	pPlayer->m_Local.m_vecPunchAngle = vec3_angle;
	pPlayer->m_Local.m_vecPunchAngleVel = vec3_angle;
	pPlayer->SnapEyeAngles( pSpawnSpot->GetLocalAngles() );

	return pSpawnSpot;
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if the player is on the correct team and whether or
//          not the spawn point is available.
//-----------------------------------------------------------------------------
bool CTFGameRules::IsSpawnPointValid( CBaseEntity *pSpot, CBasePlayer *pPlayer, bool bIgnorePlayers, PlayerTeamSpawnMode_t nSpawnMode /* = 0*/ )
{
	bool bMatchSummary = ShowMatchSummary();

	// Check the team.
	// In Item Testing mode, bots all use the Red team spawns, and the player uses Blue
	if ( !bMatchSummary )
	{
		if ( pSpot->GetTeamNumber() != pPlayer->GetTeamNumber() )
			return false;
	}

	if ( !pSpot->IsTriggered( pPlayer ) )
		return false;

	CTFTeamSpawn *pCTFSpawn = dynamic_cast<CTFTeamSpawn*>( pSpot );
	if ( pCTFSpawn )
	{
		if ( pCTFSpawn->IsDisabled() )
			return false;

		if ( pCTFSpawn->GetTeamSpawnMode() && pCTFSpawn->GetTeamSpawnMode() != nSpawnMode )
			return false;

		if ( bMatchSummary )
		{
			if ( pCTFSpawn->AlreadyUsedForMatchSummary() )
				return false; 

			if ( pCTFSpawn->GetMatchSummaryType() == PlayerTeamSpawn_MatchSummary_Winner )
			{
				if ( pPlayer->GetTeamNumber() != GetWinningTeam() )
					return false;
			}
			else if ( pCTFSpawn->GetMatchSummaryType() == PlayerTeamSpawn_MatchSummary_Loser )
			{
				if ( pPlayer->GetTeamNumber() == GetWinningTeam() )
					return false;
			}
			else
			{
				return false;
			}
		}
		else
		{
			if ( pCTFSpawn->GetMatchSummaryType() != PlayerTeamSpawn_MatchSummary_None )
				return false;
		}
	}

	Vector mins = VEC_HULL_MIN_SCALED( pPlayer );
	Vector maxs = VEC_HULL_MAX_SCALED( pPlayer );

	if ( !bIgnorePlayers && !bMatchSummary )
	{
		Vector vTestMins = pSpot->GetAbsOrigin() + mins;
		Vector vTestMaxs = pSpot->GetAbsOrigin() + maxs;
		return UTIL_IsSpaceEmpty( pPlayer, vTestMins, vTestMaxs );
	}

	trace_t trace;
	UTIL_TraceHull( pSpot->GetAbsOrigin(), pSpot->GetAbsOrigin(), mins, maxs, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
	if ( trace.fraction == 1 && trace.allsolid != 1 && (trace.startsolid != 1) )
	{
		if ( bMatchSummary )
		{
			if ( pCTFSpawn )
			{
				pCTFSpawn->SetAlreadyUsedForMatchSummary();
			}
		}

		return true;
	}

	return false;
}

Vector CTFGameRules::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->GetOriginalSpawnOrigin();
}

QAngle CTFGameRules::VecItemRespawnAngles( CItem *pItem )
{
	return pItem->GetOriginalSpawnAngles();
}

int CTFGameRules::ItemShouldRespawn( CItem *pItem )
{
	return BaseClass::ItemShouldRespawn( pItem );
}

float CTFGameRules::FlItemRespawnTime( CItem *pItem )
{
	return ITEM_RESPAWN_TIME;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFGameRules::GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer )
{
	if ( !pPlayer )  // dedicated server output
	{
		return NULL;
	}

	const char *pszFormat = NULL;

	if ( bTeamOnly == true )
	{
		if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pszFormat = "TF_Chat_Spec";
		}
		else
		{
			if ( pPlayer->IsAlive() == false && State_Get() != GR_STATE_TEAM_WIN )
			{
				pszFormat = "TF_Chat_Team_Dead";
			}
			else
			{
				const char *chatLocation = GetChatLocation( bTeamOnly, pPlayer );
				if ( chatLocation && *chatLocation )
				{
					pszFormat = "TF_Chat_Team_Loc";
				}
				else
				{
					pszFormat = "TF_Chat_Team";
				}
			}
		}
	}
	// everyone
	else
	{	
		if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pszFormat = "TF_Chat_AllSpec";	
		}
		else
		{
			if ( pPlayer->IsAlive() == false && State_Get() != GR_STATE_TEAM_WIN )
			{
				pszFormat = "TF_Chat_AllDead";
			}
			else
			{
				pszFormat = "TF_Chat_All";	
			}
		}
	}

	return pszFormat;
}

VoiceCommandMenuItem_t *CTFGameRules::VoiceCommand( CBaseMultiplayerPlayer *pPlayer, int iMenu, int iItem )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( pTFPlayer )
	{
		if ( pTFPlayer->BHaveChatSuspensionInCurrentMatch() )
		{
			if ( tf_voice_command_suspension_mode.GetInt() == 1 )
			{
				return NULL;
			}
		}
	}

	VoiceCommandMenuItem_t *pItem = BaseClass::VoiceCommand( pPlayer, iMenu, iItem );

	if ( pItem )
	{
		int iActivity = ActivityList_IndexForName( pItem->m_szGestureActivity );

		if ( iActivity != ACT_INVALID )
		{
			if ( pTFPlayer )
			{
				pTFPlayer->DoAnimationEvent( PLAYERANIMEVENT_VOICE_COMMAND_GESTURE, iActivity );

				// Note when we call for a Medic.
				// Hardcoding this for menu 0, item 0 is an ugly hack, but we don't have a good way to 
				// translate this at this level without plumbing a through bunch of stuff (MSB)
				if ( iMenu == 0 && iItem == 0 )
				{
					pTFPlayer->NoteMedicCall();
				}
			}
		}
	}

	return pItem;
}

//-----------------------------------------------------------------------------
// Purpose: Actually change a player's name.  
//-----------------------------------------------------------------------------
void CTFGameRules::ChangePlayerName( CTFPlayer *pPlayer, const char *pszNewName )
{
	if ( !tf_allow_player_name_change.GetBool() )
		return;

	const char *pszOldName = pPlayer->GetPlayerName();

	// Check if they can change their name
	if ( State_Get() != GR_STATE_STALEMATE )
	{
		CReliableBroadcastRecipientFilter filter;
		UTIL_SayText2Filter( filter, pPlayer, false, "#TF_Name_Change", pszOldName, pszNewName );

		IGameEvent * event = gameeventmanager->CreateEvent( "player_changename" );
		if ( event )
		{
			event->SetInt( "userid", pPlayer->GetUserID() );
			event->SetString( "oldname", pszOldName );
			event->SetString( "newname", pszNewName );
			gameeventmanager->FireEvent( event );
		}
	}

	pPlayer->SetPlayerName( pszNewName );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::ClientSettingsChanged( CBasePlayer *pPlayer )
{
	const char *pszName = engine->GetClientConVarValue( pPlayer->entindex(), "name" );

	const char *pszOldName = pPlayer->GetPlayerName();

	CTFPlayer *pTFPlayer = (CTFPlayer*)pPlayer;

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	// Note, not using FStrEq so that this is case sensitive
	if ( pszOldName[0] != 0 && Q_strncmp( pszOldName, pszName, MAX_PLAYER_NAME_LENGTH-1 ) )		
	{
		ChangePlayerName( pTFPlayer, pszName );
	}

	// keep track of their hud_classautokill value
	int nClassAutoKill = Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "hud_classautokill" ) );
	pTFPlayer->SetHudClassAutoKill( nClassAutoKill > 0 ? true : false );

	// keep track of their tf_medigun_autoheal value
	pTFPlayer->SetMedigunAutoHeal( Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "tf_medigun_autoheal" ) ) > 0 );

	// keep track of their cl_autorezoom value
	pTFPlayer->SetAutoRezoom( Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "cl_autorezoom" ) ) > 0 );

	// keep track of their tf_remember_lastswitched value
	pTFPlayer->SetRememberActiveWeapon( Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "tf_remember_activeweapon" ) ) > 0 );
	pTFPlayer->SetRememberLastWeapon( Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "tf_remember_lastswitched" ) ) > 0 );

	const char *pszFov = engine->GetClientConVarValue( pPlayer->entindex(), "fov_desired" );
	int iFov = atoi(pszFov);
	iFov = clamp( iFov, 75, MAX_FOV );

	pTFPlayer->SetDefaultFOV( iFov );

	pTFPlayer->m_bFlipViewModels = Q_strcmp( engine->GetClientConVarValue( pPlayer->entindex(), "cl_flipviewmodels" ), "1" ) == 0;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the specified player can carry any more of the ammo type
//-----------------------------------------------------------------------------
bool CTFGameRules::CanHaveAmmo( CBaseCombatCharacter *pPlayer, int iAmmoIndex )
{
	if ( iAmmoIndex > -1 )
	{
		CTFPlayer *pTFPlayer = (CTFPlayer*)pPlayer;

		if ( pTFPlayer )
		{
			// Get the max carrying capacity for this ammo
			int iMaxCarry = pTFPlayer->GetMaxAmmo(iAmmoIndex);

			// Does the player have room for more of this type of ammo?
			if ( pTFPlayer->GetAmmoCount( iAmmoIndex ) < iMaxCarry )
				return true;
		}
	}

	return false;
}

void CTFGameRules::PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
	// Find the killer & the scorer
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBaseMultiplayerPlayer *pScorer = ToBaseMultiplayerPlayer( GetDeathScorer( pKiller, pInflictor, pVictim ) );
	CTFPlayer *pAssister = NULL;
	CBaseObject *pObject = NULL;

	// if inflictor or killer is a base object, tell them that they got a kill
	// ( depends if a sentry rocket got the kill, sentry may be inflictor or killer )
	if ( pInflictor )
	{
		if ( pInflictor->IsBaseObject() )
		{
			pObject = dynamic_cast<CBaseObject *>( pInflictor );
		}
		else 
		{
			CBaseEntity *pInflictorOwner = pInflictor->GetOwnerEntity();
			if ( pInflictorOwner && pInflictorOwner->IsBaseObject() )
			{
				pObject = dynamic_cast<CBaseObject *>( pInflictorOwner );
			}
		}
		
	}
	else if( pKiller && pKiller->IsBaseObject() )
	{
		pObject = dynamic_cast<CBaseObject *>( pKiller );
	}

	if ( pObject )
	{
		if ( pObject->GetBuilder() != pVictim )
		{
			pObject->IncrementKills();
		}
		pInflictor = pObject;

		if ( pObject->ObjectType() == OBJ_SENTRYGUN )
		{
			CTFPlayer *pOwner = pObject->GetOwner();
			if ( pOwner )
			{
				int iKills = pObject->GetKills();

				// keep track of max kills per a single sentry gun in the player object
				if ( pOwner->GetMaxSentryKills() < iKills )
				{
					pOwner->SetMaxSentryKills( iKills );
				}

				// if we just got 10 kills with one sentry, tell the owner's client, which will award achievement if it doesn't have it already
				if ( iKills == 10 )
				{
					pOwner->AwardAchievement( ACHIEVEMENT_TF_GET_TURRETKILLS );
				}
			}
		}
	}

	// if not killed by suicide or killed by world, see if the scorer had an assister, and if so give the assister credit
	if ( ( pVictim != pScorer ) && pKiller )
	{
		pAssister = ToTFPlayer( GetAssister( pVictim, pScorer, pInflictor ) );

		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( pAssister && pTFVictim )
		{
			EntityHistory_t* entHist = pTFVictim->m_AchievementData.IsSentryDamagerInHistory( pAssister, 5.0 );
			if ( entHist )
			{
				CBaseObject *pObj = dynamic_cast<CBaseObject*>( entHist->hObject.Get() );
				if ( pObj )
				{
					pObj->IncrementAssists();
				}
			}
		}
	}

	//find the area the player is in and see if his death causes a block
	CBaseMultiplayerPlayer *pMultiplayerPlayer = ToBaseMultiplayerPlayer(pVictim);
	for ( int i=0; i<ITriggerAreaCaptureAutoList::AutoList().Count(); ++i )
	{
		CTriggerAreaCapture *pArea = static_cast< CTriggerAreaCapture * >( ITriggerAreaCaptureAutoList::AutoList()[i] );
		if ( pArea->IsTouching( pMultiplayerPlayer ) )
		{
			// ACHIEVEMENT_TF_MEDIC_ASSIST_CAPTURER
			// kill 3 players 
			if ( pAssister && pAssister->IsPlayerClass( TF_CLASS_MEDIC ) )
			{
				// the victim is touching a cap point, see if they own it
				if ( pMultiplayerPlayer->GetTeamNumber() == pArea->GetOwningTeam() )
				{
					int iDefenderKills = pAssister->GetPerLifeCounterKV( "medic_defender_kills" );

					if ( ++iDefenderKills == 3 )
					{
						pAssister->AwardAchievement( ACHIEVEMENT_TF_MEDIC_ASSIST_CAPTURER );
					}

					pAssister->SetPerLifeCounterKV( "medic_defender_kills", iDefenderKills );
				}
			}

			if ( pArea->CheckIfDeathCausesBlock( pMultiplayerPlayer, pScorer ) )
				break;
		}
	}

	// determine if this kill affected a nemesis relationship
	int iDeathFlags = 0;
	CTFPlayer *pTFPlayerVictim = ToTFPlayer( pVictim );
	CTFPlayer *pTFPlayerScorer = ToTFPlayer( pScorer );
	if ( pScorer )
	{	
		CalcDominationAndRevenge( pTFPlayerScorer, info.GetWeapon(), pTFPlayerVictim, false, &iDeathFlags );
		if ( pAssister )
		{
			CalcDominationAndRevenge( pAssister, info.GetWeapon(), pTFPlayerVictim, true, &iDeathFlags );
		}
	}
	pTFPlayerVictim->SetDeathFlags( iDeathFlags );	

	CTFPlayer *pTFPlayerAssister = ToTFPlayer( pAssister );
	if ( pTFPlayerAssister )
	{
		CTF_GameStats.Event_AssistKill( pTFPlayerAssister, pVictim );
	}

	// check for achievements
	PlayerKilledCheckAchievements( pTFPlayerScorer, pTFPlayerVictim );

	BaseClass::PlayerKilled( pVictim, info );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::PlayerKilledCheckAchievements( CTFPlayer *pAttacker, CTFPlayer *pVictim )
{
	if ( !pAttacker || !pVictim )
		return;

	// HEAVY WEAPONS GUY
	if ( pAttacker->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
	{
		if ( GetGameType() == TF_GAMETYPE_CP )
		{
			// ACHIEVEMENT_TF_HEAVY_DEFEND_CONTROL_POINT
			CTriggerAreaCapture *pAreaTrigger = pAttacker->GetControlPointStandingOn();
			if ( pAreaTrigger )
			{
				CTeamControlPoint *pCP = pAreaTrigger->GetControlPoint();
				if ( pCP )
				{
					if ( pCP->GetOwner() == pAttacker->GetTeamNumber() )
					{
						// no suicides!
						if ( pAttacker != pVictim )
						{
							pAttacker->AwardAchievement( ACHIEVEMENT_TF_HEAVY_DEFEND_CONTROL_POINT );
						}
					}
				}
			}

			// ACHIEVEMENT_TF_HEAVY_KILL_CAPPING_ENEMIES
			pAreaTrigger = pVictim->GetControlPointStandingOn();
			if ( pAreaTrigger )
			{
				CTeamControlPoint *pCP = pAreaTrigger->GetControlPoint();
				if ( pCP )
				{
					if ( pCP->GetOwner() == pAttacker->GetTeamNumber() && 
						 TeamMayCapturePoint( pVictim->GetTeamNumber(), pCP->GetPointIndex() ) &&
						 PlayerMayCapturePoint( pVictim, pCP->GetPointIndex() ) )
					{
						pAttacker->AwardAchievement( ACHIEVEMENT_TF_HEAVY_KILL_CAPPING_ENEMIES );
					}
				}
			}
		}

		// ACHIEVEMENT_TF_HEAVY_CLEAR_STICKYBOMBS
		if ( pVictim->IsPlayerClass( TF_CLASS_DEMOMAN ) )
		{
			int iPipes = pVictim->GetNumActivePipebombs();

			for (int i = 0; i < iPipes; i++)
			{
				pAttacker->AwardAchievement( ACHIEVEMENT_TF_HEAVY_CLEAR_STICKYBOMBS );
			}
		}

		// ACHIEVEMENT_TF_HEAVY_DEFEND_MEDIC
		int i;
		int iNumHealers = pAttacker->m_Shared.GetNumHealers();
		for ( i = 0 ; i < iNumHealers ; i++ )
		{
			CTFPlayer *pMedic = ToTFPlayer( pAttacker->m_Shared.GetHealerByIndex( i ) );
			if ( pMedic && pMedic->m_AchievementData.IsDamagerInHistory( pVictim, 3.0 ) )
			{
				pAttacker->AwardAchievement( ACHIEVEMENT_TF_HEAVY_DEFEND_MEDIC );
				break; // just award it once for each kill...even if the victim attacked more than one medic attached to the killer
			}
		}

		// ACHIEVEMENT_TF_HEAVY_STAND_NEAR_DISPENSER
		for ( i = 0 ; i < iNumHealers ; i++ )
		{
			if ( pAttacker->m_Shared.HealerIsDispenser( i ) )
			{
				pAttacker->AwardAchievement( ACHIEVEMENT_TF_HEAVY_STAND_NEAR_DISPENSER );
				break; // just award it once for each kill...even if the attacker is being healed by more than one dispenser
			}
		}
	}

	int i;
	int iNumHealers = pAttacker->m_Shared.GetNumHealers();
	for ( i = 0 ; i < iNumHealers ; i++ )
	{
		CTFPlayer *pMedic = ToTFPlayer( pAttacker->m_Shared.GetHealerByIndex( i ) );
		if ( pMedic && pMedic->m_AchievementData.IsDamagerInHistory( pVictim, 10.0 ) )
		{
			IGameEvent * event = gameeventmanager->CreateEvent( "medic_defended" );
			if ( event )
			{
				event->SetInt( "userid", pAttacker->GetUserID() );
				event->SetInt( "medic", pMedic->GetUserID() );
				gameeventmanager->FireEvent( event );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Determines if attacker and victim have gotten domination or revenge
//-----------------------------------------------------------------------------
void CTFGameRules::CalcDominationAndRevenge( CTFPlayer *pAttacker, CBaseEntity *pWeapon, CTFPlayer *pVictim, bool bIsAssist, int *piDeathFlags )
{
	PlayerStats_t *pStatsVictim = CTF_GameStats.FindPlayerStats( pVictim );

	int nAttackerEntIdx = pAttacker->entindex();
	if ( !IsIndexIntoPlayerArrayValid(nAttackerEntIdx) )
		return;

	// calculate # of unanswered kills between killer & victim - add 1 to include current kill
	int iKillsUnanswered = pStatsVictim->statsKills.iNumKilledByUnanswered[nAttackerEntIdx] + 1;		
	if ( TF_KILLS_DOMINATION == iKillsUnanswered )
	{			
		// this is the Nth unanswered kill between killer and victim, killer is now dominating victim
		*piDeathFlags |= ( bIsAssist ? TF_DEATH_ASSISTER_DOMINATION : TF_DEATH_DOMINATION );
		// set victim to be dominated by killer
		pAttacker->m_Shared.SetPlayerDominated( pVictim, true );

		int iCurrentlyDominated = pAttacker->GetNumberofDominations() + 1;
		pAttacker->SetNumberofDominations( iCurrentlyDominated );

		// record stats
		CTF_GameStats.Event_PlayerDominatedOther( pAttacker );

		IGameEvent *pDominationEvent = gameeventmanager->CreateEvent( "player_domination" );
		if ( pDominationEvent )
		{
			pDominationEvent->SetInt( "dominator", pAttacker->GetUserID() );
			pDominationEvent->SetInt( "dominated", pVictim->GetUserID() );
			pDominationEvent->SetInt( "dominations", iCurrentlyDominated );

			// Send the event
			gameeventmanager->FireEvent( pDominationEvent );
		}
	}
	else if ( pVictim->m_Shared.IsPlayerDominated( pAttacker->entindex() ) )
	{
		// the killer killed someone who was dominating him, gains revenge
		*piDeathFlags |= ( bIsAssist ? TF_DEATH_ASSISTER_REVENGE : TF_DEATH_REVENGE );
		// set victim to no longer be dominating the killer
		pVictim->m_Shared.SetPlayerDominated( pAttacker, false );

		int iCurrentlyDominated = pVictim->GetNumberofDominations() - 1;
		if (iCurrentlyDominated < 0)
		{
			iCurrentlyDominated = 0;
		}
		pVictim->SetNumberofDominations( iCurrentlyDominated );

		// record stats
		CTF_GameStats.Event_PlayerRevenge( pAttacker );
	}
}

template< typename TIssue >
void NewTeamIssue()
{
	new TIssue( g_voteControllerRed );
	new TIssue( g_voteControllerBlu );
}

template< typename TIssue >
void NewGlobalIssue()
{
	new TIssue( g_voteControllerGlobal );
}

//-----------------------------------------------------------------------------
// Purpose: create some proxy entities that we use for transmitting data */
//-----------------------------------------------------------------------------
void CTFGameRules::CreateStandardEntities()
{
	// Create the player resource
	g_pPlayerResource = (CPlayerResource*)CBaseEntity::Create( "tf_player_manager", vec3_origin, vec3_angle );

	// Create the objective resource
	g_pObjectiveResource = (CTFObjectiveResource *)CBaseEntity::Create( "tf_objective_resource", vec3_origin, vec3_angle );

	Assert( g_pObjectiveResource );

	// Create the entity that will send our data to the client.
	m_hGamerulesProxy = assert_cast<CTFGameRulesProxy*>(CBaseEntity::Create( "tf_gamerules", vec3_origin, vec3_angle ));
	Assert( m_hGamerulesProxy.Get() );
	m_hGamerulesProxy->SetName( AllocPooledString("tf_gamerules" ) );
	
	g_voteControllerGlobal	=	static_cast< CVoteController *>( CBaseEntity::Create( "vote_controller", vec3_origin, vec3_angle ) );
	g_voteControllerRed		=	static_cast< CVoteController *>( CBaseEntity::Create( "vote_controller", vec3_origin, vec3_angle ) );
	g_voteControllerBlu		=	static_cast< CVoteController *>( CBaseEntity::Create( "vote_controller", vec3_origin, vec3_angle ) );
}

//-----------------------------------------------------------------------------
// Purpose: determine the class name of the weapon that got a kill
//-----------------------------------------------------------------------------
const char *CTFGameRules::GetKillingWeaponName( const CTakeDamageInfo &info, CTFPlayer *pVictim, int *iWeaponID )
{
 	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBasePlayer *pScorer = GetDeathScorer( pKiller, pInflictor, pVictim );

	const char *killer_weapon_name = "world";
	*iWeaponID = TF_WEAPON_NONE;

	if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BURNING )
	{
		// special-case burning damage, since persistent burning damage may happen after attacker has switched weapons
		killer_weapon_name = "tf_weapon_flamethrower";
		*iWeaponID = TF_WEAPON_FLAMETHROWER;
	}
	else if ( info.GetPlayerPenetrationCount() > 0 )
	{
		// This may be stomped later if the kill was a headshot.
		killer_weapon_name = "player_penetration";
	}
	else if ( pScorer && pInflictor && ( pInflictor == pScorer ) )
	{
		// If this is not a suicide
		if ( pVictim != pScorer )
		{
			// If the inflictor is the killer, then it must be their current weapon doing the damage
			if ( pScorer->GetActiveWeapon() )
			{
				killer_weapon_name = pScorer->GetActiveWeapon()->GetClassname(); 
				if ( pScorer->IsPlayer() )
				{
					*iWeaponID = ToTFPlayer(pScorer)->GetActiveTFWeapon()->GetWeaponID();
				}
			}
		}
	}
	else if ( pInflictor )
	{
		killer_weapon_name = STRING( pInflictor->m_iClassname );

		CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase * >( pInflictor );
		if ( pWeapon )
		{
			*iWeaponID = pWeapon->GetWeaponID();
		}
		else
		{
			CTFBaseRocket *pBaseRocket = dynamic_cast<CTFBaseRocket*>( pInflictor );
			if ( pBaseRocket )
			{
				*iWeaponID = pBaseRocket->GetWeaponID();
			}
			else
			{
				CTFWeaponBaseGrenadeProj *pBaseGrenade = dynamic_cast<CTFWeaponBaseGrenadeProj*>( pInflictor );
				if ( pBaseGrenade )
				{
					*iWeaponID = pBaseGrenade->GetWeaponID();
				}
			}
		}
	}

	// strip certain prefixes from inflictor's classname
	const char *prefix[] = { "tf_weapon_grenade_", "tf_weapon_", "NPC_", "func_" };
	for ( int i = 0; i< ARRAYSIZE( prefix ); i++ )
	{
		// if prefix matches, advance the string pointer past the prefix
		int len = Q_strlen( prefix[i] );
		if ( strncmp( killer_weapon_name, prefix[i], len ) == 0 )
		{
			killer_weapon_name += len;
			break;
		}
	}

	// look out for sentry rocket as weapon and map it to sentry gun, so we get the sentry death icon based off level

	if ( 0 == Q_strcmp( killer_weapon_name, "tf_projectile_sentryrocket" ) )
	{
		killer_weapon_name = "obj_sentrygun3";
	}
	else if ( 0 == Q_strcmp( killer_weapon_name, "obj_sentrygun" ) )
	{
		CObjectSentrygun *pSentrygun = assert_cast<CObjectSentrygun*>( pInflictor );
		if ( pSentrygun )
		{
			int iSentryLevel = pSentrygun->GetUpgradeLevel();
			switch( iSentryLevel)
			{
			case 1:
				killer_weapon_name = "obj_sentrygun";
				break;
			case 2:
				killer_weapon_name = "obj_sentrygun2";
				break;
			case 3:
				killer_weapon_name = "obj_sentrygun3";
				break;
			default:
				killer_weapon_name = "obj_sentrygun";
				break;
			}
		}
	}
#if 0 // TF2007: May not be necessary
	else if ( 0 == Q_strcmp( killer_weapon_name, "tf_projectile_pipe" ) )
	{
		*iWeaponID = TF_WEAPON_GRENADE_PIPEBOMB;
	}
	else if ( 0 == Q_strcmp( killer_weapon_name, "obj_attachment_sapper" ) )
	{
		// let's look-up the sapper weapon to see what type it is
		if ( pScorer )
		{
			CTFPlayer *pTFScorer = ToTFPlayer( pScorer );
			if ( pTFScorer )
			{
				CTFWeaponBuilder *pWeapon = dynamic_cast<CTFWeaponBuilder *>( pTFScorer->Weapon_OwnsThisID( TF_WEAPON_BUILDER ) );
				if ( pWeapon )
				{
					const CObjectInfo *info = GetObjectInfo( pWeapon->GetType() );
					if ( info )
					{
						killer_weapon_name = info->m_pClassName;
						*iWeaponID = TF_WEAPON_NONE;
					}
				}
			}
		}
	}
#endif
	return killer_weapon_name;
}

//-----------------------------------------------------------------------------
// Purpose: returns the player who assisted in the kill, or NULL if no assister
//-----------------------------------------------------------------------------
CBasePlayer *CTFGameRules::GetAssister( CBasePlayer *pVictim, CBasePlayer *pScorer, CBaseEntity *pInflictor )
{
	CTFPlayer *pTFScorer = ToTFPlayer( pScorer );
	CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
	if ( pTFScorer && pTFVictim )
	{
		// if victim killed himself, don't award an assist to anyone else, even if there was a recent damager
		if ( pTFScorer == pTFVictim )
			return NULL;

		// If an assist has been specified already, use it.
		if ( pTFVictim->m_Shared.GetAssist() )
		{
			return pTFVictim->m_Shared.GetAssist();
		}

		// If a player is healing the scorer, give that player credit for the assist
		CTFPlayer *pHealer = ToTFPlayer( pTFScorer->m_Shared.GetFirstHealer() );
		// Must be a medic to receive a healing assist, otherwise engineers get credit for assists from dispensers doing healing.
		// Also don't give an assist for healing if the inflictor was a sentry gun, otherwise medics healing engineers get assists for the engineer's sentry kills.
		if ( pHealer && ( TF_CLASS_MEDIC == pHealer->GetPlayerClass()->GetClassIndex() ) && ( NULL == dynamic_cast<CObjectSentrygun *>( pInflictor ) ) )
		{
			return pHealer;
		}

		// See who has damaged the victim 2nd most recently (most recent is the killer), and if that is within a certain time window.
		// If so, give that player an assist.  (Only 1 assist granted, to single other most recent damager.)
		CTFPlayer *pRecentDamager = GetRecentDamager( pTFVictim, 1, TF_TIME_ASSIST_KILL );
		if ( pRecentDamager && ( pRecentDamager != pScorer ) )
			return pRecentDamager;

		// if a teammate has recently helped this sentry (ie: wrench hit), they assisted in the kill
		CObjectSentrygun *sentry = dynamic_cast< CObjectSentrygun * >( pInflictor );
		if ( sentry )
		{
			CTFPlayer *pAssister = sentry->GetAssistingTeammate( TF_TIME_ASSIST_KILL );
			if ( pAssister )
				return pAssister;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns specified recent damager, if there is one who has done damage
//			within the specified time period.  iDamager=0 returns the most recent
//			damager, iDamager=1 returns the next most recent damager.
//-----------------------------------------------------------------------------
CTFPlayer *CTFGameRules::GetRecentDamager( CTFPlayer *pVictim, int iDamager, float flMaxElapsed )
{
	EntityHistory_t *damagerHistory = pVictim->m_AchievementData.GetDamagerHistory( iDamager );
	if ( !damagerHistory )
		return NULL;

	if ( damagerHistory->hEntity && ( gpGlobals->curtime - damagerHistory->flTimeDamage <= flMaxElapsed ) )
	{
		CTFPlayer *pRecentDamager = ToTFPlayer( damagerHistory->hEntity );
		if ( pRecentDamager )
			return pRecentDamager;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns who should be awarded the kill
//-----------------------------------------------------------------------------
CBasePlayer *CTFGameRules::GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor, CBaseEntity *pVictim )
{
	if ( ( pKiller == pVictim ) && ( pKiller == pInflictor ) )
	{
		// If this was an explicit suicide, see if there was a damager within a certain time window.  If so, award this as a kill to the damager.
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( pTFVictim )
		{
			CTFPlayer *pRecentDamager = GetRecentDamager( pTFVictim, 0, TF_TIME_SUICIDE_KILL_CREDIT );
			if ( pRecentDamager )
				return pRecentDamager;
		}
	}

	return BaseClass::GetDeathScorer( pKiller, pInflictor, pVictim );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVictim - 
//			*pKiller - 
//			*pInflictor - 
//-----------------------------------------------------------------------------
void CTFGameRules::DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
	DeathNotice( pVictim, info, "player_death" );
}

void CTFGameRules::DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info, const char* eventName )
{
	int killer_ID = 0;

	// Find the killer & the scorer
	CTFPlayer *pTFPlayerVictim = ToTFPlayer( pVictim );
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CTFPlayer *pScorer = ToTFPlayer( GetDeathScorer( pKiller, pInflictor, pVictim ) );
	CTFPlayer *pAssister = ToTFPlayer( GetAssister( pVictim, pScorer, pInflictor ) );
	
	// You can't assist yourself!
	Assert( pScorer != pAssister || !pScorer );
	if ( pScorer == pAssister && pScorer )
	{
		pAssister = NULL;
	}

	// Determine whether it's a feign death fake death notice
	bool bFeignDeath = pTFPlayerVictim->IsGoingFeignDeath();
	if ( bFeignDeath )
	{
		CTFPlayer *pDisguiseTarget = pTFPlayerVictim->m_Shared.GetDisguiseTarget();
		if ( pDisguiseTarget && (pTFPlayerVictim->GetTeamNumber() == pDisguiseTarget->GetTeamNumber()) )
		{
			// We're disguised as a team mate. Pretend to die as that player instead of us.
			pVictim = pTFPlayerVictim = pDisguiseTarget;
		}
	}

	// Work out what killed the player, and send a message to all clients about it
	int iWeaponID;
	const char *killer_weapon_name = GetKillingWeaponName( info, pTFPlayerVictim, &iWeaponID );
	const char *killer_weapon_log_name = killer_weapon_name;

	if ( pScorer )	// Is the killer a client?
	{
		killer_ID = pScorer->GetUserID();
	}

	IGameEvent * event = gameeventmanager->CreateEvent( eventName /* "player_death" */ );
	if ( event )
	{
		event->SetInt( "userid", pVictim->GetUserID() );
		event->SetInt( "victim_entindex", pVictim->entindex() );
		event->SetInt( "attacker", killer_ID );
		event->SetInt( "assister", pAssister ? pAssister->GetUserID() : -1 );
		event->SetString( "weapon", killer_weapon_name );
		event->SetString( "weapon_logclassname", killer_weapon_log_name );
		event->SetInt( "weaponid", iWeaponID );
		event->SetInt( "damagebits", info.GetDamageType() );
		event->SetInt( "customkill", info.GetDamageCustom() );
		event->SetInt( "inflictor_entindex", pInflictor ? pInflictor->entindex() : -1 );
		event->SetInt( "priority", 7 );	// HLTV event priority, not transmitted

		if ( info.GetPlayerPenetrationCount() > 0 )
		{
			event->SetInt( "playerpenetratecount", info.GetPlayerPenetrationCount() );
		}

		event->SetBool( "silent_kill", false );

		int iDeathFlags = pTFPlayerVictim->GetDeathFlags();

		if ( bFeignDeath )
		{
			iDeathFlags |= TF_DEATH_FEIGN_DEATH;
		}

		if ( pTFPlayerVictim->WasGibbedOnLastDeath() )
		{
			iDeathFlags |= TF_DEATH_GIBBED;
		}

		// We call this directly since we need more information than provided in the event alone.
		if ( FStrEq( eventName, "player_death" ) )
		{
			event->SetBool( "rocket_jump", ( pTFPlayerVictim->RocketJumped() == 1 ) );
		}

		event->SetInt( "death_flags", iDeathFlags );

		gameeventmanager->FireEvent( event );
	}	
}

void CTFGameRules::ClientDisconnected( edict_t *pClient )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetContainingEntity( pClient ) );
	if ( pPlayer )
	{
		// ACHIEVEMENT_TF_PYRO_DOMINATE_LEAVESVR - Pyro causes a dominated player to leave the server
		for ( int i = 1; i <= gpGlobals->maxClients ; i++ )
		{
			if ( pPlayer->m_Shared.IsPlayerDominatingMe(i) )
			{
				CTFPlayer *pDominatingPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
				if ( pDominatingPlayer && pDominatingPlayer != pPlayer )
				{
					if ( pDominatingPlayer->IsPlayerClass(TF_CLASS_PYRO) )
					{
						pDominatingPlayer->AwardAchievement( ACHIEVEMENT_TF_PYRO_DOMINATE_LEAVESVR );
					}
				}
			}
		}

		CTFPlayerResource *pTFResource = dynamic_cast< CTFPlayerResource* >( g_pPlayerResource );		
		if ( pTFResource )
		{
			if ( pPlayer->entindex() == pTFResource->GetPartyLeaderIndex( pPlayer->GetTeamNumber() ) )
			{
				// the leader is leaving so reset the player resource index
				pTFResource->SetPartyLeaderIndex( pPlayer->GetTeamNumber(), 0 ); 
			}
		}

		// Notify gamestats that the player left.
		CTF_GameStats.Event_PlayerDisconnectedTF( pPlayer );

		// Check Ready status for the player
		if ( UsePlayerReadyStatusMode() )
		{
			if ( !pPlayer->IsBot() && State_Get() != GR_STATE_RND_RUNNING )
			{
				// Always reset when a player leaves this type of match
				PlayerReadyStatus_ResetState();
			}
		}

		// clean up anything they left behind
		pPlayer->TeamFortress_ClientDisconnected();
	}

	BaseClass::ClientDisconnected( pClient );

	// are any of the spies disguising as this player?
	for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
	{
		CTFPlayer *pTemp = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pTemp && pTemp != pPlayer )
		{
			if ( pTemp->m_Shared.GetDisguiseTarget() == pPlayer )
			{
				// choose someone else...
				pTemp->m_Shared.FindDisguiseTarget();
			}
		}
	}
}

// Falling damage stuff.
#define TF_PLAYER_MAX_SAFE_FALL_SPEED	650		

ConVar tf_fall_damage_disablespread( "tf_fall_damage_disablespread", "0", FCVAR_NONE );

float CTFGameRules::FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	if ( pPlayer->m_Local.m_flFallVelocity > TF_PLAYER_MAX_SAFE_FALL_SPEED )
	{
		// Old TFC damage formula
		float flFallDamage = 5 * (pPlayer->m_Local.m_flFallVelocity / 300);

		// Fall damage needs to scale according to the player's max health, or
		// it's always going to be much more dangerous to weaker classes than larger.
		float flRatio = (float)pPlayer->GetMaxHealth() / 100.0;
		flFallDamage *= flRatio;

		if ( !tf_fall_damage_disablespread.GetBool() )
			flFallDamage *= random->RandomFloat( 0.8, 1.2 );

		return flFallDamage;
	}

	// Fall caused no damage
	return 0;
}

void CTFGameRules::SendWinPanelInfo( bool bGameOver )
{
	IGameEvent *winEvent = gameeventmanager->CreateEvent( "teamplay_win_panel" );

	if ( winEvent )
	{
		int iBlueScore = GetGlobalTeam( TF_TEAM_BLUE )->GetScore();
		int iRedScore = GetGlobalTeam( TF_TEAM_RED )->GetScore();
		int iBlueScorePrev = iBlueScore;
		int iRedScorePrev = iRedScore;

		bool bRoundComplete = m_bForceMapReset || ( IsGameUnderTimeLimit() && ( GetTimeLeft() <= 0 ) );

		CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
		bool bScoringPerCapture = ( pMaster ) ? ( pMaster->ShouldScorePerCapture() ) : false;

		if ( m_nGameType == TF_GAMETYPE_CTF )
		{
			if ( tf_flag_caps_per_round.GetInt() == 0 )
			{
				bScoringPerCapture = true;
			}
		}

		if ( bRoundComplete && !bScoringPerCapture && m_bUseAddScoreAnim )
		{
			// if this is a complete round, calc team scores prior to this win
			switch ( m_iWinningTeam )
			{
			case TF_TEAM_BLUE:
				iBlueScorePrev = ( iBlueScore - TEAMPLAY_ROUND_WIN_SCORE >= 0 ) ? ( iBlueScore - TEAMPLAY_ROUND_WIN_SCORE ) : 0;
				break;
			case TF_TEAM_RED:
				iRedScorePrev = ( iRedScore - TEAMPLAY_ROUND_WIN_SCORE >= 0 ) ? ( iRedScore - TEAMPLAY_ROUND_WIN_SCORE ) : 0;
				break;
			case TEAM_UNASSIGNED:
				break;	// stalemate; nothing to do
			}
		}
				
		winEvent->SetInt( "panel_style", WINPANEL_BASIC );
		winEvent->SetInt( "winning_team", m_iWinningTeam );
		winEvent->SetInt( "winreason", m_iWinReason );
		winEvent->SetString( "cappers",  ( m_iWinReason == WINREASON_ALL_POINTS_CAPTURED || m_iWinReason == WINREASON_FLAG_CAPTURE_LIMIT ) ?
							 m_szMostRecentCappers : "" );
		winEvent->SetInt( "flagcaplimit", tf_flag_caps_per_round.GetInt() );
		winEvent->SetInt( "blue_score", iBlueScore );
		winEvent->SetInt( "red_score", iRedScore );
		winEvent->SetInt( "blue_score_prev", iBlueScorePrev );
		winEvent->SetInt( "red_score_prev", iRedScorePrev );
		winEvent->SetInt( "round_complete", bRoundComplete );

		CTFPlayerResource *pResource = dynamic_cast< CTFPlayerResource * >( g_pPlayerResource );
		if ( !pResource )
			return;

		// Highest killstreak
		int nMaxStreakPlayerIndex = 0;
		int nMaxStreakCount = 0;
	 
		// determine the 3 players on winning team who scored the most points this round

		// build a vector of players & round scores
		CUtlVector<PlayerRoundScore_t> vecPlayerScore;
		int iPlayerIndex;
		for( iPlayerIndex = 1 ; iPlayerIndex <= MAX_PLAYERS; iPlayerIndex++ )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
			if ( !pTFPlayer || !pTFPlayer->IsConnected() )
				continue;
			// filter out spectators and, if not stalemate, all players not on winning team
			int iPlayerTeam = pTFPlayer->GetTeamNumber();
			if ( ( iPlayerTeam < FIRST_GAME_TEAM ) || ( m_iWinningTeam != TEAM_UNASSIGNED && ( m_iWinningTeam != iPlayerTeam ) ) )
				continue;

			int iRoundScore = 0, iTotalScore = 0;
			PlayerStats_t *pStats = CTF_GameStats.FindPlayerStats( pTFPlayer );
			if ( pStats )
			{
				iRoundScore = CalcPlayerScore( &pStats->statsCurrentRound, pTFPlayer );
				iTotalScore = CalcPlayerScore( &pStats->statsAccumulated, pTFPlayer );
			}

			PlayerRoundScore_t &playerRoundScore = vecPlayerScore[vecPlayerScore.AddToTail()];

			playerRoundScore.iRoundScore = iRoundScore;
			playerRoundScore.iPlayerIndex = iPlayerIndex;
			playerRoundScore.iTotalScore = iTotalScore;

			// Highest killstreak?
			PlayerStats_t *pPlayerStats = CTF_GameStats.FindPlayerStats( pTFPlayer );
			if ( pPlayerStats ) 
			{
				int nMax = pPlayerStats->statsCurrentRound.m_iStat[TFSTAT_KILLSTREAK_MAX];
				if ( nMax > nMaxStreakCount )
				{
					nMaxStreakCount = nMax;
					nMaxStreakPlayerIndex = iPlayerIndex;
				}
			}
		}

		// sort the players by round score
		vecPlayerScore.Sort( PlayerRoundScoreSortFunc );

		// set the top (up to) 6 players by round score in the event data
		int numPlayers = MIN( 6, vecPlayerScore.Count() );
		for ( int i = 0; i < numPlayers; i++ )
		{
			// only include players who have non-zero points this round; if we get to a player with 0 round points, stop
			if ( 0 == vecPlayerScore[i].iRoundScore )
				break;

			// set the player index and their round score in the event
			char szPlayerIndexVal[64]="", szPlayerScoreVal[64]="";
			Q_snprintf( szPlayerIndexVal, ARRAYSIZE( szPlayerIndexVal ), "player_%d", i+ 1 );
			Q_snprintf( szPlayerScoreVal, ARRAYSIZE( szPlayerScoreVal ), "player_%d_points", i+ 1 );
			winEvent->SetInt( szPlayerIndexVal, vecPlayerScore[i].iPlayerIndex );
			winEvent->SetInt( szPlayerScoreVal, vecPlayerScore[i].iRoundScore );				
		}

		winEvent->SetInt( "killstreak_player_1", nMaxStreakPlayerIndex );
		winEvent->SetInt( "killstreak_player_1_count", nMaxStreakCount );

		if ( !bRoundComplete && ( TEAM_UNASSIGNED != m_iWinningTeam ) )
		{
			// is this our new payload race game mode?
			// if this was not a full round ending, include how many mini-rounds remain for winning team to win
			if ( g_hControlPointMasters.Count() && g_hControlPointMasters[0] )
			{
				winEvent->SetInt( "rounds_remaining", g_hControlPointMasters[0]->CalcNumRoundsRemaining( m_iWinningTeam ) );
			}
		}

		winEvent->SetBool( "game_over", bGameOver );

		// Send the event
		gameeventmanager->FireEvent( winEvent );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sorts players by round score
//-----------------------------------------------------------------------------
int CTFGameRules::PlayerRoundScoreSortFunc( const PlayerRoundScore_t *pRoundScore1, const PlayerRoundScore_t *pRoundScore2 )
{
	// sort first by round score	
	if ( pRoundScore1->iRoundScore != pRoundScore2->iRoundScore )
		return pRoundScore2->iRoundScore - pRoundScore1->iRoundScore;

	// if round scores are the same, sort next by total score
	if ( pRoundScore1->iTotalScore != pRoundScore2->iTotalScore )
		return pRoundScore2->iTotalScore - pRoundScore1->iTotalScore;

	// if scores are the same, sort next by player index so we get deterministic sorting
	return ( pRoundScore2->iPlayerIndex - pRoundScore1->iPlayerIndex );
}

//-----------------------------------------------------------------------------
// Purpose: Called when the teamplay_round_win event is about to be sent, gives
//			this method a chance to add more data to it
//-----------------------------------------------------------------------------
void CTFGameRules::FillOutTeamplayRoundWinEvent( IGameEvent *event )
{
	event->SetInt( "flagcaplimit", tf_flag_caps_per_round.GetInt() );

	// determine the losing team
	int iLosingTeam;

	switch( event->GetInt( "team" ) )
	{
	case TF_TEAM_RED:
		iLosingTeam = TF_TEAM_BLUE;
		break;
	case TF_TEAM_BLUE:
		iLosingTeam = TF_TEAM_RED;
		break;
	case TEAM_UNASSIGNED:
	default:
		iLosingTeam = TEAM_UNASSIGNED;
		break;
	}

	// set the number of caps that team got any time during the round
	event->SetInt( "losing_team_num_caps", m_iNumCaps[iLosingTeam] );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetupSpawnPointsForRound( void )
{
	if ( !g_hControlPointMasters.Count() || !g_hControlPointMasters[0] || !g_hControlPointMasters[0]->PlayingMiniRounds() )
		return;

	CTeamControlPointRound *pCurrentRound = g_hControlPointMasters[0]->GetCurrentRound();
	if ( !pCurrentRound )
	{
		return;
	}

	// loop through the spawn points in the map and find which ones are associated with this round or the control points in this round
	for ( int i=0; i<ITFTeamSpawnAutoList::AutoList().Count(); ++i )
	{
		CTFTeamSpawn *pTFSpawn = static_cast< CTFTeamSpawn* >( ITFTeamSpawnAutoList::AutoList()[i] );

		CHandle<CTeamControlPoint> hControlPoint = pTFSpawn->GetControlPoint();
		CHandle<CTeamControlPointRound> hRoundBlue = pTFSpawn->GetRoundBlueSpawn();
		CHandle<CTeamControlPointRound> hRoundRed = pTFSpawn->GetRoundRedSpawn();

		if ( hControlPoint && pCurrentRound->IsControlPointInRound( hControlPoint ) )
		{
			// this spawn is associated with one of our control points
			pTFSpawn->SetDisabled( false );
			pTFSpawn->ChangeTeam( hControlPoint->GetOwner() );
		}
		else if ( hRoundBlue && ( hRoundBlue == pCurrentRound ) )
		{
			pTFSpawn->SetDisabled( false );
			pTFSpawn->ChangeTeam( TF_TEAM_BLUE );
		}
		else if ( hRoundRed && ( hRoundRed == pCurrentRound ) )
		{
			pTFSpawn->SetDisabled( false );
			pTFSpawn->ChangeTeam( TF_TEAM_RED );
		}
		else
		{
			// this spawn isn't associated with this round or the control points in this round
			pTFSpawn->SetDisabled( true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::SetCurrentRoundStateBitString( void )
{
	m_iPrevRoundState = m_iCurrentRoundState;

	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;

	if ( !pMaster )
	{
		return 0;
	}

	int iState = 0;

	for ( int i=0; i<pMaster->GetNumPoints(); i++ )
	{
		CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );

		if ( pPoint->GetOwner() == TF_TEAM_BLUE )
		{
			// Set index to 1 for the point being owned by blue
			iState |= ( 1<<i );
		}
	}

	m_iCurrentRoundState = iState;

	return iState;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetMiniRoundBitMask( int iMask )
{
	m_iCurrentMiniRoundMask = iMask;
}

//-----------------------------------------------------------------------------
// Purpose: NULL pPlayer means show the panel to everyone
//-----------------------------------------------------------------------------
void CTFGameRules::ShowRoundInfoPanel( CTFPlayer *pPlayer /* = NULL */ )
{
	KeyValues *data = new KeyValues( "data" );

	if ( m_iCurrentRoundState < 0 )
	{
		// Haven't set up the round state yet
		return;
	}

	// if prev and cur are equal, we are starting from a fresh round
	if ( m_iPrevRoundState >= 0 && pPlayer == NULL )	// we have data about a previous state
	{
		data->SetInt( "prev", m_iPrevRoundState );
	}
	else
	{
		// don't send a delta if this is just to one player, they are joining mid-round
		data->SetInt( "prev", m_iCurrentRoundState );	
	}

	data->SetInt( "cur", m_iCurrentRoundState );

	// get bitmask representing the current miniround
	data->SetInt( "round", m_iCurrentMiniRoundMask );

	if ( pPlayer )
	{
		pPlayer->ShowViewPortPanel( PANEL_ROUNDINFO, true, data );
	}
	else
	{
		for ( int i = 1;  i <= MAX_PLAYERS; i++ )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

			if ( pTFPlayer && pTFPlayer->IsReadyToPlay() )
			{
				pTFPlayer->ShowViewPortPanel( PANEL_ROUNDINFO, true, data );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::TimerMayExpire( void )
{
	// Prevent timers expiring while control points are contested
	int iNumControlPoints = ObjectiveResource()->GetNumControlPoints();
	for ( int iPoint = 0; iPoint < iNumControlPoints; iPoint ++ )
	{
		if ( ObjectiveResource()->GetCappingTeam(iPoint) )
		{
			m_flCapInProgressBuffer = gpGlobals->curtime + 0.1f;
			return false;
		}
	}

	// This delay prevents an order-of-operations issue with caps that
	// fire an AddTime output, and round timers' OnFinished output
	if ( m_flCapInProgressBuffer >= gpGlobals->curtime )
		return false;

	if ( GetOvertimeAllowedForCTF() )
	{
		// Prevent timers expiring while flags are stolen/dropped
		for ( int i=0; i<ICaptureFlagAutoList::AutoList().Count(); ++i )
		{
			CCaptureFlag *pFlag = static_cast< CCaptureFlag* >( ICaptureFlagAutoList::AutoList()[i] );
			if ( !pFlag->IsDisabled() && !pFlag->IsHome() )
				return false;
		}
	}

	for ( int i = 0 ; i < m_CPTimerEnts.Count() ; i++ )
	{
		CCPTimerLogic *pTimer = m_CPTimerEnts[i];
		if ( pTimer )
		{
			if ( pTimer->TimerMayExpire() == false )
				return false;
		}
	}

#ifdef TF_RAID_MODE
	if ( IsRaidMode() && IsBossBattleMode() && tf_raid_allow_overtime.GetBool() )
	{
		CUtlVector< CTFPlayer * > alivePlayerVector;
		CollectPlayers( &alivePlayerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS );

		// if anyone is alive, go into overtime
		if ( alivePlayerVector.Count() > 0 )
		{
			return false;
		}
	}
#endif

	return BaseClass::TimerMayExpire();
}

int SortPlayerSpectatorQueue( CTFPlayer* const *p1, CTFPlayer* const *p2 )
{
	float flTime1 = (*p1)->GetTeamJoinTime();
	float flTime2 = (*p2)->GetTeamJoinTime();

	if ( flTime1 == flTime2 )
	{
		flTime1 = (*p1)->GetConnectionTime();
		flTime2 = (*p2)->GetConnectionTime();

		if ( flTime1 < flTime2 )
			return -1;
	}
	else
	{
		if ( flTime1 > flTime2 )
			return -1;
	}
		
	if ( flTime1 == flTime2 )
		return 0;

	return 1;
}

int SortPlayersScoreBased( CTFPlayer* const *p1, CTFPlayer* const *p2 )
{
	CTFPlayerResource *pResource = dynamic_cast<CTFPlayerResource *>( g_pPlayerResource );

	if ( pResource )
	{
		int nScore2 = pResource->GetTotalScore( ( *p2 )->entindex() );
		int nScore1 = pResource->GetTotalScore( ( *p1 )->entindex() );

		// check the priority
		if ( nScore2 > nScore1 )
		{
			return 1;
		}
	}

	return -1;
}

// sort function for the list of players that we're going to use to scramble the teams
int ScramblePlayersSort( CTFPlayer* const *p1, CTFPlayer* const *p2 )
{
	CTFPlayerResource *pResource = dynamic_cast< CTFPlayerResource * >( g_pPlayerResource );

	if ( pResource )
	{
		int nScore2 = pResource->GetTotalScore( (*p2)->entindex() );
		int nScore1 = pResource->GetTotalScore( (*p1)->entindex() );

		// check the priority
		if ( nScore2 > nScore1 )
		{
			return 1;
		}
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose:  Server-side vote creation
//-----------------------------------------------------------------------------
void CTFGameRules::ManageServerSideVoteCreation( void )
{
	if ( gpGlobals->curtime < m_flVoteCheckThrottle )
		return;

	if ( IsInTournamentMode() )
		return;

	if ( IsInArenaMode() )
		return;

	if ( IsInWaitingForPlayers() )
		return;

	if ( m_bInSetup )
		return;

	if ( m_MapList.Count() < 2 )
		return;
	
	// Ask players which map they would prefer to play next, based
	// on "n" lowest playtime from server stats

	ConVarRef sv_vote_issue_nextlevel_allowed( "sv_vote_issue_nextlevel_allowed" );
	ConVarRef sv_vote_issue_nextlevel_choicesmode( "sv_vote_issue_nextlevel_choicesmode" );

	if ( sv_vote_issue_nextlevel_allowed.GetBool() && sv_vote_issue_nextlevel_choicesmode.GetBool() )
	{
		// Don't do this if we already have a nextlevel set
		if ( nextlevel.GetString() && *nextlevel.GetString() )
			return;

		if ( !m_bServerVoteOnReset && !m_bVoteCalled )
		{
			// If we have any round or win limit, ignore time
			if ( mp_winlimit.GetInt() || mp_maxrounds.GetInt() )
			{
				int nBlueScore = TFTeamMgr()->GetTeam( TF_TEAM_BLUE )->GetScore();
				int nRedScore = TFTeamMgr()->GetTeam( TF_TEAM_RED)->GetScore();
				int nWinLimit = mp_winlimit.GetInt();
				if ( ( nWinLimit - nBlueScore ) == 1 || ( nWinLimit - nRedScore ) == 1 )
				{
					m_bServerVoteOnReset = true;
				}

				int nRoundsPlayed = GetRoundsPlayed();
				if ( ( mp_maxrounds.GetInt() - nRoundsPlayed ) == 1 )
				{
					m_bServerVoteOnReset = true;
				}
			}
			else if ( mp_timelimit.GetInt() > 0 )
			{
				int nTimeLeft = GetTimeLeft();
				if ( nTimeLeft <= 120 && !m_bServerVoteOnReset )
				{
					if ( g_voteControllerGlobal )
					{
						g_voteControllerGlobal->CreateVote( DEDICATED_SERVER, "nextlevel", "" );
					}
					m_bVoteCalled = true;
				}
			}
		}
	}

	m_flVoteCheckThrottle = gpGlobals->curtime + 0.5f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::RoundRespawn( void )
{
	// remove any buildings, grenades, rockets, etc. the player put into the world
	RemoveAllProjectilesAndBuildings();

	// re-enable any sentry guns the losing team has built (and not hidden!)
	for ( int i=0; i<IBaseObjectAutoList::AutoList().Count(); ++i )
	{
		CBaseObject *pObj = static_cast< CBaseObject* >( IBaseObjectAutoList::AutoList()[i] );
		if ( pObj->ObjectType() == OBJ_SENTRYGUN && pObj->IsEffectActive( EF_NODRAW ) == false && pObj->GetTeamNumber() != m_iWinningTeam )
		{
			pObj->SetDisabled( false );
		}
	}

	// reset the flag captures
	int nTeamCount = TFTeamMgr()->GetTeamCount();
	for ( int iTeam = FIRST_GAME_TEAM; iTeam < nTeamCount; ++iTeam )
	{
		CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
		if ( !pTeam )
			continue;

		pTeam->SetFlagCaptures( 0 );
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "scorestats_accumulated_update" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}

	// reset player per-round stats
	CTF_GameStats.ResetRoundStats();

	BaseClass::RoundRespawn();

	// ** AFTER WE'VE BEEN THROUGH THE ROUND RESPAWN, SHOW THE ROUNDINFO PANEL
	if ( !IsInWaitingForPlayers() )
	{
		ShowRoundInfoPanel();
	}

	// We've hit some condition where a server-side vote should be called on respawn
	if ( m_bServerVoteOnReset )
	{
		if ( g_voteControllerGlobal )
		{
			g_voteControllerGlobal->CreateVote( DEDICATED_SERVER, "nextlevel", "" );
		}
		m_bVoteCalled = true;
		m_bServerVoteOnReset = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::ClientCommandKeyValues( edict_t *pEntity, KeyValues *pKeyValues )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( CBaseEntity::Instance( pEntity ) );

	if ( !pTFPlayer )
		return;

	char const *pszCommand = pKeyValues->GetName();
	if ( pszCommand && pszCommand[0] )
	{
		if ( FStrEq( pszCommand, "FreezeCamTaunt" ) )
		{
			CTFPlayer *pAchiever = ToTFPlayer( UTIL_PlayerByUserId( pKeyValues->GetInt( "achiever" ) ) );
			if ( pAchiever )
			{
				const char *pszCommand = pKeyValues->GetString( "command" );
				if ( pszCommand && pszCommand[0] )
				{
					int nGibs = pKeyValues->GetInt( "gibs" );

					if ( FStrEq( pszCommand, "freezecam_taunt" ) )
					{	
						CheckTauntAchievement( pAchiever, nGibs, g_TauntCamAchievements );
						CheckTauntAchievement( pAchiever, nGibs, g_TauntCamAchievements2 );
					}
					else if ( FStrEq( pszCommand, "freezecam_tauntrag" ) )
					{	
						CheckTauntAchievement( pAchiever, nGibs, g_TauntCamRagdollAchievements );
					}
					else if ( FStrEq( pszCommand, "freezecam_tauntgibs" ) )
					{	
						CheckTauntAchievement( pAchiever, nGibs, g_TauntCamAchievements );
					}
					else if ( FStrEq( pszCommand, "freezecam_tauntsentry" ) )
					{
						// Maybe should also require a taunt? Currently too easy to get?
						pAchiever->AwardAchievement( ACHIEVEMENT_TF_ENGINEER_FREEZECAM_SENTRY );
					}
				}
			}
		}
		else if ( FStrEq( pszCommand, "+helpme_server" ) )
		{
			pTFPlayer->HelpmeButtonPressed();
		}
		else if ( FStrEq( pszCommand, "-helpme_server" ) )
		{
			pTFPlayer->HelpmeButtonReleased();
		}
		else if ( FStrEq( pszCommand, "cl_drawline" ) )
		{
			BroadcastDrawLine( pTFPlayer, pKeyValues );
		}
		else
		{
			BaseClass::ClientCommandKeyValues( pEntity, pKeyValues );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::BroadcastDrawLine( CTFPlayer *pTFPlayer, KeyValues *pKeyValues )
{
	if ( true || !m_bPlayersAreOnMatchSummaryStage || pTFPlayer->BHaveChatSuspensionInCurrentMatch() )
		return;

	int paneltype = clamp( pKeyValues->GetInt( "panel", DRAWING_PANEL_TYPE_NONE ), DRAWING_PANEL_TYPE_NONE, DRAWING_PANEL_TYPE_MAX - 1 );

	if ( paneltype >= DRAWING_PANEL_TYPE_MATCH_SUMMARY )
	{
		int linetype = clamp( pKeyValues->GetInt( "line", 0 ), 0, 1 );
		float x = pKeyValues->GetFloat( "x", 0.f );
		float y = pKeyValues->GetFloat( "y", 0.f );

		IGameEvent *event = gameeventmanager->CreateEvent( "cl_drawline" );
		if ( event )
		{
			event->SetInt( "player", pTFPlayer->entindex() );
			event->SetInt( "panel",paneltype );
			event->SetInt( "line", linetype );
			event->SetFloat( "x", x );
			event->SetFloat( "y", y );

			gameeventmanager->FireEvent( event );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::HandleMapEvent( inputdata_t &inputdata )
{
	if ( FStrEq( "sd_doomsday", STRING( gpGlobals->mapname ) ) )
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

		// make sure it's being carried by one of the teams
		if ( pFlag && pFlag->IsStolen() )
		{
			CTFPlayer *pFlagCarrier = ToTFPlayer( pFlag->GetOwnerEntity() );
			if ( pFlagCarrier )
			{
				// let everyone know which team has opened the rocket
				IGameEvent *event = gameeventmanager->CreateEvent( "doomsday_rocket_open" );
				if ( event )
				{
					event->SetInt( "team", pFlagCarrier->GetTeamNumber() );
					gameeventmanager->FireEvent( event );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldWaitToStartRecording( void )
{
	return BaseClass::ShouldWaitToStartRecording(); 
}

//-----------------------------------------------------------------------------
// Purpose: return true if this flag is currently allowed to be captured
//-----------------------------------------------------------------------------
bool CTFGameRules::CanFlagBeCaptured( CBaseEntity *pOther )
{
	if ( pOther && ( tf_flag_return_on_touch.GetBool() ) )
	{
		for ( int i = 0; i < ICaptureFlagAutoList::AutoList().Count(); ++i )
		{
			CCaptureFlag *pListFlag = static_cast<CCaptureFlag*>( ICaptureFlagAutoList::AutoList()[i] );
			if ( ( pListFlag->GetType() == TF_FLAGTYPE_CTF ) && !pListFlag->IsDisabled() && ( pOther->GetTeamNumber() == pListFlag->GetTeamNumber() ) && !pListFlag->IsHome() )
				return false;
		}
	}
	return true;
}

void CTFGameRules::TeleportPlayersToTargetEntities( int iTeam, const char *pszEntTargetName, CUtlVector< CTFPlayer * > *pTeleportedPlayers )
{
	CUtlVector< CTFPlayer * > vecPlayers;
	CUtlVector< CTFPlayer * > vecRandomOrderPlayers;

	CollectPlayers( &vecPlayers, iTeam );

	FOR_EACH_VEC( vecPlayers, i )
	{
		CTFPlayer *pPlayer = vecPlayers[i];

		// Skip players not on a team or who have not chosen a class
		if ( ( pPlayer->GetTeamNumber() != TF_TEAM_RED && pPlayer->GetTeamNumber() != TF_TEAM_BLUE )
			|| pPlayer->IsPlayerClass( TF_CLASS_UNDEFINED ) )
			continue;

		vecRandomOrderPlayers.AddToTail( pPlayer );
	}

	// Randomize the order so players dont go to the same spot every time
	vecRandomOrderPlayers.Shuffle();

	string_t sName = MAKE_STRING( pszEntTargetName );
	int iCachedLocationIndex = m_mapTeleportLocations.Find( sName );
	CUtlVector< TeleportLocation_t > *pCachedLocations = NULL;
	// is there any cached entities to teleport to
	if ( m_mapTeleportLocations.IsValidIndex( iCachedLocationIndex ) )
	{
		pCachedLocations = m_mapTeleportLocations[iCachedLocationIndex];
	}

	
	int iCurrentTeleportLocation = 0;
	CBaseEntity *pSpawnPoint = NULL;
	FOR_EACH_VEC_BACK( vecRandomOrderPlayers, i )
	{
		// don't do anything if we run out of spawn point
		Vector vTeleportPosition;
		QAngle qTeleportAngles;

		// if we have cached locations, use them
		if ( pCachedLocations )
		{
			Assert( iCurrentTeleportLocation < pCachedLocations->Count() );
			if ( iCurrentTeleportLocation < pCachedLocations->Count() )
			{
				const TeleportLocation_t& location = pCachedLocations->Element( iCurrentTeleportLocation );
				vTeleportPosition = location.m_vecPosition;
				qTeleportAngles = location.m_qAngles;
				iCurrentTeleportLocation++;
			}
			else
			{
				// we need to add more teleport location in the map for players to teleport to
				continue;
			}
		}
		else // use old search for entities by name
		{
			pSpawnPoint = gEntList.FindEntityByName( pSpawnPoint, pszEntTargetName );

			Assert( pSpawnPoint );
			if ( pSpawnPoint )
			{
				vTeleportPosition = pSpawnPoint->GetAbsOrigin();
				qTeleportAngles = pSpawnPoint->GetAbsAngles();
			}
			else
			{
				// we need to add more teleport location in the map for players to teleport to
				continue;
			}
		}

		CTFPlayer *pPlayer = vecRandomOrderPlayers[ i ];
		pPlayer->m_Shared.RemoveAllCond();
		
		// Respawn dead players
		if ( !pPlayer->IsAlive() )
		{
			pPlayer->ForceRespawn();
		}

		// Unzoom if we are a sniper zoomed!
		pPlayer->m_Shared.InstantlySniperUnzoom();

		// Teleport
		pPlayer->Teleport( &vTeleportPosition, &qTeleportAngles, &vec3_origin );
		pPlayer->SnapEyeAngles( qTeleportAngles );

		// Force client to update all view angles (including kart and taunt yaw)
		pPlayer->ForcePlayerViewAngles( qTeleportAngles );

		// fill in the teleported player vector
		if ( pTeleportedPlayers )
		{
			pTeleportedPlayers->AddToTail( pPlayer );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::InternalHandleTeamWin( int iWinningTeam )
{
	// remove any spies' disguises and make them visible (for the losing team only)
	// and set the speed for both teams (winners get a boost and losers have reduced speed)
	for ( int i = 1;  i <= MAX_PLAYERS; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer )
		{
			if ( pPlayer->GetTeamNumber() > LAST_SHARED_TEAM )
			{
				if ( pPlayer->GetTeamNumber() != iWinningTeam )
				{
					pPlayer->RemoveInvisibility();
//					pPlayer->RemoveDisguise();

					if ( pPlayer->HasTheFlag() )
					{
						pPlayer->DropFlag();
					}
				}

				pPlayer->TeamFortress_SetSpeed();
			}
		}
	}

	for ( int i=0; i<IBaseObjectAutoList::AutoList().Count(); ++i )
	{
		CBaseObject *pObj = static_cast< CBaseObject* >( IBaseObjectAutoList::AutoList()[i] );
		if ( pObj->GetTeamNumber() != iWinningTeam )
		{
			// Stop placing any carried objects or else they will float around
			// at our feet at the end of the round
			if( pObj->IsPlacing() )
			{
				pObj->StopPlacement();
			}

			// Disable sentry guns that the losing team has built
			if( pObj->GetType() == OBJ_SENTRYGUN )
			{
				pObj->SetDisabled( true );
			}
		}
	}

	if ( m_bForceMapReset )
	{
		m_iPrevRoundState = -1;
		m_iCurrentRoundState = -1;
		m_iCurrentMiniRoundMask = 0;
	}

	if ( IsInStopWatch() == true && GetStopWatchTimer() )
	{
		variant_t sVariant;
		GetStopWatchTimer()->AcceptInput( "Pause", NULL, NULL, sVariant, 0 );

		if ( m_bForceMapReset )
		{
			if ( GetStopWatchTimer()->IsWatchingTimeStamps() == true )
			{
				m_flStopWatchTotalTime = GetStopWatchTimer()->GetStopWatchTotalTime();
			}
			else
			{
				ShouldResetScores( true, false );
				UTIL_Remove( m_hStopWatchTimer	);
				m_hStopWatchTimer = NULL;
				m_flStopWatchTotalTime = -1.0f;
				m_bStopWatch = false;
				m_nStopWatchState.Set( STOPWATCH_CAPTURE_TIME_NOT_SET );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Helper function for scramble teams
//-----------------------------------------------------------------------------
int FindScoreDifferenceBetweenTeams( CUtlVector< CTFPlayer* > &vecSource, CTFPlayerResource *pPR, int &nRedScore, int &nBlueScore )
{
	if ( !pPR )
		return false;

	nRedScore = 0;
	nBlueScore = 0;

	FOR_EACH_VEC( vecSource, i )
	{
		if ( !vecSource[i] )
			continue;

		if ( vecSource[i]->GetTeamNumber() == TF_TEAM_RED )
		{
			nRedScore += pPR->GetTotalScore( vecSource[i]->entindex() );
		}
		else
		{
			nBlueScore += pPR->GetTotalScore( vecSource[i]->entindex() );
		}
	}

	return abs( nRedScore - nBlueScore );
}

//-----------------------------------------------------------------------------
// Purpose: Helper function for scramble teams
//-----------------------------------------------------------------------------
bool FindAndSwapPlayersToBalanceTeams( CUtlVector< CTFPlayer* > &vecSource, int &nDelta, CTFPlayerResource *pPR )
{
	if ( !pPR )
		return false;

	int nTeamScoreRed = 0;
	int nTeamScoreBlue = 0;
	FindScoreDifferenceBetweenTeams( vecSource, pPR, nTeamScoreRed, nTeamScoreBlue );

	FOR_EACH_VEC( vecSource, i )
	{
		if ( !vecSource[i] )
			continue;

		if ( vecSource[i]->GetTeamNumber() != TF_TEAM_RED )
			continue;

		// Check against players on the other team
		FOR_EACH_VEC( vecSource, j )
		{
			if ( !vecSource[j] )
				continue;

			if ( vecSource[j]->GetTeamNumber() != TF_TEAM_BLUE )
				continue;

			if ( vecSource[i] == vecSource[j] )
				continue;

			int nRedPlayerScore = pPR->GetTotalScore( vecSource[i]->entindex() );
			int nBluePlayerScore = pPR->GetTotalScore( vecSource[j]->entindex() );

			int nPlayerDiff = abs( nRedPlayerScore - nBluePlayerScore );
			if ( nPlayerDiff )
			{
				int nNewRedScore = nTeamScoreRed;
				int nNewBlueScore = nTeamScoreBlue;

				if ( nRedPlayerScore > nBluePlayerScore )
				{
					nNewRedScore -= nPlayerDiff;
					nNewBlueScore += nPlayerDiff;
				}
				else
				{
					nNewRedScore += nPlayerDiff;
					nNewBlueScore -= nPlayerDiff;
				}

				int nNewDelta = abs( nNewRedScore - nNewBlueScore );
				if ( nNewDelta < nDelta )
				{
					// Swap and recheck
					vecSource[i]->ForceChangeTeam( TF_TEAM_BLUE );
					vecSource[j]->ForceChangeTeam( TF_TEAM_RED );

					nDelta = FindScoreDifferenceBetweenTeams( vecSource, pPR, nTeamScoreRed, nTeamScoreBlue );
					return true;
				}
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::HandleScrambleTeams( void )
{
	static CUtlVector< CTFPlayer* > playerVector;
	playerVector.RemoveAll();

	CollectPlayers( &playerVector, TF_TEAM_RED );
	CollectPlayers( &playerVector, TF_TEAM_BLUE, false, APPEND_PLAYERS );

	// Sort by player score.
	playerVector.Sort( ScramblePlayersSort );

	// Put everyone on Spectator to clear the teams (or the autoteam step won't work correctly)
	FOR_EACH_VEC_BACK( playerVector, i )
	{
		if ( !playerVector[i] )
		{
			playerVector.Remove( i );
			continue;
		}

		playerVector[i]->ForceChangeTeam( TEAM_SPECTATOR );
	}

	// Assign players using the original, quick method.
	FOR_EACH_VEC( playerVector, i )
	{
		if ( !playerVector[i] )
			continue;

		if ( playerVector[i]->GetTeamNumber() >= FIRST_GAME_TEAM ) // are they already on a game team?
			continue;

		playerVector[i]->ForceChangeTeam( TF_TEAM_AUTOASSIGN );
	}

	// New method
	if ( playerVector.Count() > 2 )
	{
		CTFPlayerResource *pPR = dynamic_cast< CTFPlayerResource* >( g_pPlayerResource );
		if ( pPR )
		{
			int nTeamScoreRed = 0;
			int nTeamScoreBlue = 0;
			int nDelta = FindScoreDifferenceBetweenTeams( playerVector, pPR, nTeamScoreRed, nTeamScoreBlue );

#ifdef _DEBUG
			if ( mp_scrambleteams_debug.GetBool() )
			{
				DevMsg( "FIRST PASS -- Team1: %i || Team2: %i || Diff: %i\n", 
						nTeamScoreRed, 
						nTeamScoreBlue, 
						nDelta );
			}
#endif // _DEBUG

			// Try swapping players to bring scores closer
			if ( nDelta > 1 )
			{
				int nOrigValue = mp_teams_unbalance_limit.GetInt();
				mp_teams_unbalance_limit.SetValue( 0 );

				static const int nPassLimit = 8;
				for ( int i = 0; i < nPassLimit && FindAndSwapPlayersToBalanceTeams( playerVector, nDelta, pPR ); ++i )
				{
#ifdef _DEBUG
					if ( mp_scrambleteams_debug.GetBool() )
					{
						nTeamScoreRed = 0;
						nTeamScoreBlue = 0;
						DevMsg( "EXTRA PASS -- Team1: %i || Team2: %i || Diff: %i\n", 
								nTeamScoreRed, 
								nTeamScoreBlue, 
								FindScoreDifferenceBetweenTeams( playerVector, pPR, nTeamScoreRed, nTeamScoreBlue ) );
					}
#endif // _DEBUG
				}
			
				mp_teams_unbalance_limit.SetValue( nOrigValue );
			}
		}
	}

	// scrambleteams_auto tracking
	ResetTeamsRoundWinTracking();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::TeamPlayerCountChanged( CTFTeam *pTeam )
{
	if ( m_hGamerulesProxy )
	{
		m_hGamerulesProxy->TeamPlayerCountChanged( pTeam );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::BIsManagedMatchEndImminent( void )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Restrict team human players can join
//-----------------------------------------------------------------------------
int CTFGameRules::GetAssignedHumanTeam( void )
{
	if ( FStrEq( "blue", mp_humans_must_join_team.GetString() ) )
	{
		return TF_TEAM_BLUE;
	}
	else if ( FStrEq( "red", mp_humans_must_join_team.GetString() ) )
	{
		return TF_TEAM_RED;
	}
	else if ( FStrEq( "spectator", mp_humans_must_join_team.GetString() ) )
	{
		return TEAM_SPECTATOR;
	}
	else
	{
		return TEAM_ANY;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::HandleSwitchTeams( void )
{
	m_bTeamsSwitched.Set( !m_bTeamsSwitched );

	// switch this as well
	if ( FStrEq( mp_humans_must_join_team.GetString(), "blue" ) )
	{
		mp_humans_must_join_team.SetValue( "red" );
	}
	else if ( FStrEq( mp_humans_must_join_team.GetString(), "red" ) )
	{
		mp_humans_must_join_team.SetValue( "blue" );
	}

	int i = 0;

	// remove everyone's projectiles and objects
	RemoveAllProjectilesAndBuildings();

	// respawn the players
	for ( i = 1 ; i <= gpGlobals->maxClients ; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer )
		{
			if ( pPlayer->GetTeamNumber() == TF_TEAM_RED )
			{
				pPlayer->ForceChangeTeam( TF_TEAM_BLUE, true );
			}
			else if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
			{
				pPlayer->ForceChangeTeam( TF_TEAM_RED, true );
			}
		}
	}

	// switch the team scores
	CTFTeam *pRedTeam = GetGlobalTFTeam( TF_TEAM_RED );
	CTFTeam *pBlueTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
	if ( pRedTeam && pBlueTeam )
	{
		int nRed = pRedTeam->GetScore();
		int nBlue = pBlueTeam->GetScore();

		pRedTeam->SetScore( nBlue );
		pBlueTeam->SetScore( nRed );

		if ( IsInTournamentMode() == true )
		{
			char szBlueName[16];
			char szRedName[16];

			Q_strncpy( szBlueName, mp_tournament_blueteamname.GetString(), sizeof ( szBlueName ) );
			Q_strncpy( szRedName, mp_tournament_redteamname.GetString(), sizeof ( szRedName ) );

			mp_tournament_redteamname.SetValue( szBlueName );
			mp_tournament_blueteamname.SetValue( szRedName );
		}
	}

	UTIL_ClientPrintAll( HUD_PRINTTALK, "#TF_TeamsSwitched" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::CanChangeClassInStalemate( void ) 
{ 
	return (gpGlobals->curtime < (m_flStalemateStartTime + tf_stalematechangeclasstime.GetFloat())); 
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::CanChangeTeam( int iCurrentTeam ) const
{
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetRoundOverlayDetails( void )
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;

	if ( pMaster && pMaster->PlayingMiniRounds() )
	{
		CTeamControlPointRound *pRound = pMaster->GetCurrentRound();

		if ( pRound )
		{
			CHandle<CTeamControlPoint> pRedPoint = pRound->GetPointOwnedBy( TF_TEAM_RED );
			CHandle<CTeamControlPoint> pBluePoint = pRound->GetPointOwnedBy( TF_TEAM_BLUE );

			// do we have opposing points in this round?
			if ( pRedPoint && pBluePoint )
			{
				int iMiniRoundMask = ( 1<<pBluePoint->GetPointIndex() ) | ( 1<<pRedPoint->GetPointIndex() );
				SetMiniRoundBitMask( iMiniRoundMask );
			}
			else
			{
				SetMiniRoundBitMask( 0 );
			}

			SetCurrentRoundStateBitString();
		}
	}

	BaseClass::SetRoundOverlayDetails();
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether a team should score for each captured point
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldScorePerRound( void )
{ 
	bool bRetVal = true;

	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( pMaster && pMaster->ShouldScorePerCapture() )
	{
		bRetVal = false;
	}

	return bRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::IsValveMap( void )
{ 
	char szCurrentMap[MAX_MAP_NAME];
	Q_strncpy( szCurrentMap, STRING( gpGlobals->mapname ), sizeof( szCurrentMap ) );

	if ( ::IsValveMap( szCurrentMap ) )
	{
		return true;
	}

	return BaseClass::IsValveMap();
}

bool CTFGameRules::IsOfficialMap( void )
{ 
	char szCurrentMap[MAX_MAP_NAME];
	Q_strncpy( szCurrentMap, STRING( gpGlobals->mapname ), sizeof( szCurrentMap ) );

	if ( ::IsValveMap( szCurrentMap ) )
	{
		return true;
	}

	return BaseClass::IsOfficialMap();
}
#endif  // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::GetFarthestOwnedControlPoint( int iTeam, bool bWithSpawnpoints )
{
	int iOwnedEnd = ObjectiveResource()->GetBaseControlPointForTeam( iTeam );
	if ( iOwnedEnd == -1 )
		return -1;

	int iNumControlPoints = ObjectiveResource()->GetNumControlPoints();
	int iWalk = 1;
	int iEnemyEnd = iNumControlPoints-1;
	if ( iOwnedEnd != 0 )
	{
		iWalk = -1;
		iEnemyEnd = 0;
	}

	// Walk towards the other side, and find the farthest owned point that has spawn points
	int iFarthestPoint = iOwnedEnd;
	for ( int iPoint = iOwnedEnd; iPoint != iEnemyEnd; iPoint += iWalk )
	{
		// If we've hit a point we don't own, we're done
		if ( ObjectiveResource()->GetOwningTeam( iPoint ) != iTeam )
			break;

		if ( bWithSpawnpoints && !m_bControlSpawnsPerTeam[iTeam][iPoint] )
			continue;

		iFarthestPoint = iPoint;
	}

	return iFarthestPoint;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::TeamMayCapturePoint( int iTeam, int iPointIndex ) 
{ 
	if ( !tf_caplinear.GetBool() )
		return true; 

	// Any previous points necessary?
	int iPointNeeded = ObjectiveResource()->GetPreviousPointForPoint( iPointIndex, iTeam, 0 );

	// Points set to require themselves are always cappable 
	if ( iPointNeeded == iPointIndex )
		return true;

	if ( IsInKothMode() && IsInWaitingForPlayers() )
		return false;

	// Is the point locked?
	if ( ObjectiveResource()->GetCPLocked( iPointIndex ) )
		return false;

	// No required points specified? Require all previous points.
	if ( iPointNeeded == -1 )
	{
		if ( IsInArenaMode() == true )
		{

#ifdef CLIENT_DLL

			if ( m_flCapturePointEnableTime - 5.0f <= gpGlobals->curtime && State_Get() == GR_STATE_STALEMATE )
				return true;
#endif

			if ( m_flCapturePointEnableTime <= gpGlobals->curtime && State_Get() == GR_STATE_STALEMATE )
				return true;

			return false;
		}

		if ( !ObjectiveResource()->PlayingMiniRounds() )
		{
			// No custom previous point, team must own all previous points
			int iFarthestPoint = GetFarthestOwnedControlPoint( iTeam, false );
			return (abs(iFarthestPoint - iPointIndex) <= 1);
		}
		else
		{
			// No custom previous point, team must own all previous points in the current mini-round
			return true;
		}
	}

	// Loop through each previous point and see if the team owns it
	for ( int iPrevPoint = 0; iPrevPoint < MAX_PREVIOUS_POINTS; iPrevPoint++ )
	{
		iPointNeeded = ObjectiveResource()->GetPreviousPointForPoint( iPointIndex, iTeam, iPrevPoint );
		if ( iPointNeeded != -1 )
		{
			if ( ObjectiveResource()->GetOwningTeam( iPointNeeded ) != iTeam )
				return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::PlayerMayCapturePoint( CBasePlayer *pPlayer, int iPointIndex, char *pszReason /* = NULL */, int iMaxReasonLength /* = 0 */ )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

	if ( !pTFPlayer )
	{
		return false;
	}

	// Disguised and invisible spies cannot capture points
	if ( pTFPlayer->m_Shared.IsStealthed() )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_stealthed" );
		}
		return false;
	}
	if ( ( pTFPlayer->m_Shared.IsInvulnerable() ) )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_invuln" );
		}
		return false;
	}

	// spies disguised as the enemy team cannot capture points
 	if ( pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && pTFPlayer->m_Shared.GetDisguiseTeam() != pTFPlayer->GetTeamNumber() )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_disguised" );
		}
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::PlayerMayBlockPoint( CBasePlayer *pPlayer, int iPointIndex, char *pszReason, int iMaxReasonLength )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( !pTFPlayer )
		return false;

	// Invuln players can block points
	if ( pTFPlayer->m_Shared.IsInvulnerable() )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_invuln" );
		}
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Calculates score for player
//-----------------------------------------------------------------------------
int CTFGameRules::CalcPlayerScore( RoundStats_t *pRoundStats, CTFPlayer *pPlayer )
{
	Assert( pRoundStats );
	if ( !pRoundStats )
		return 0;

	// defensive fix for the moment for bug where healing value becomes bogus sometimes: if bogus, slam it to 0
	int iHealing = pRoundStats->m_iStat[TFSTAT_HEALING];
	Assert( iHealing >= 0 );
	Assert( iHealing <= 10000000 );
	if ( iHealing < 0 || iHealing > 10000000 )
	{
		iHealing = 0;
	}

	int iScore =	( pRoundStats->m_iStat[TFSTAT_KILLS] * TF_SCORE_KILL ) + 
					( pRoundStats->m_iStat[TFSTAT_CAPTURES] * ( TF_SCORE_CAPTURE ) ) +
					( pRoundStats->m_iStat[TFSTAT_FLAGRETURNS] * TF_SCORE_FLAG_RETURN ) +
					( pRoundStats->m_iStat[TFSTAT_DEFENSES] * TF_SCORE_DEFEND ) + 
					( pRoundStats->m_iStat[TFSTAT_BUILDINGSDESTROYED] * TF_SCORE_DESTROY_BUILDING ) + 
					( pRoundStats->m_iStat[TFSTAT_HEADSHOTS] / TF_SCORE_HEADSHOT_DIVISOR ) + 
					( pRoundStats->m_iStat[TFSTAT_BACKSTABS] * TF_SCORE_BACKSTAB ) + 
					( iHealing / ( TF_SCORE_HEAL_HEALTHUNITS_PER_POINT ) ) +
					( pRoundStats->m_iStat[TFSTAT_KILLASSISTS] / TF_SCORE_KILL_ASSISTS_PER_POINT ) + 
					( pRoundStats->m_iStat[TFSTAT_TELEPORTS] / TF_SCORE_TELEPORTS_PER_POINT ) +
					( pRoundStats->m_iStat[TFSTAT_INVULNS] / TF_SCORE_INVULN ) +
					( pRoundStats->m_iStat[TFSTAT_REVENGE] / TF_SCORE_REVENGE ) +
					( pRoundStats->m_iStat[TFSTAT_BONUS_POINTS] / TF_SCORE_BONUS_POINT_DIVISOR ) +
					( pRoundStats->m_iStat[TFSTAT_CURRENCY_COLLECTED] / TF_SCORE_CURRENCY_COLLECTED );

	const int nDivisor = TF_SCORE_HEAL_HEALTHUNITS_PER_POINT;
	iScore += ( pRoundStats->m_iStat[TFSTAT_DAMAGE] / nDivisor );
	iScore += ( pRoundStats->m_iStat[TFSTAT_DAMAGE_ASSIST] / nDivisor );
	iScore += ( pRoundStats->m_iStat[TFSTAT_DAMAGE_BOSS] / nDivisor );
	iScore += ( pRoundStats->m_iStat[TFSTAT_HEALING_ASSIST] / nDivisor );
	iScore += ( pRoundStats->m_iStat[TFSTAT_DAMAGE_BLOCKED] / nDivisor );

	return Max( iScore, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Calculates score for player
//-----------------------------------------------------------------------------
int	CTFGameRules::CalcPlayerSupportScore( RoundStats_t *pRoundStats, int iPlayerIdx )
{
#ifdef GAME_DLL
	Assert( pRoundStats );
	if ( !pRoundStats )
		return 0;

	return ( pRoundStats->m_iStat[TFSTAT_DAMAGE_ASSIST] +
		   pRoundStats->m_iStat[TFSTAT_HEALING_ASSIST] +
		   pRoundStats->m_iStat[TFSTAT_DAMAGE_BLOCKED] +
		   ( pRoundStats->m_iStat[TFSTAT_BONUS_POINTS] * 25 ) );
#else
	Assert( g_TF_PR );
	if ( !g_TF_PR )
		return 0;

	return	g_TF_PR->GetDamageAssist( iPlayerIdx ) +
			g_TF_PR->GetHealingAssist( iPlayerIdx ) +
			g_TF_PR->GetDamageBlocked( iPlayerIdx ) +
			( g_TF_PR->GetBonusPoints( iPlayerIdx ) * 25 );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::IsBirthday( void ) const
{
	if ( IsX360() )
		return false;

	return tf_birthday.GetBool();
}

#ifndef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::UseSillyGibs( void )
{
	if ( UTIL_IsLowViolence() )
		return true;

	return m_bSillyGibs;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::AllowWeatherParticles( void )
{
	return ( tf_particles_disable_weather.GetBool() == false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::ModifySentChat( char *pBuf, int iBufSize )
{
	// replace all " with ' to prevent exploits related to chat text
	// example:   ";exit
	for ( char *ch = pBuf; *ch != 0; ch++ )
	{
		if ( *ch == '"' )
		{
			*ch = '\'';
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::AllowMapParticleEffect( const char *pszParticleEffect )
{
	static const char *s_WeatherEffects[] =
	{
		"tf_gamerules",
		"env_rain_001",
		"env_rain_002_256",
		"env_rain_ripples",
		"env_snow_light_001",
		"env_rain_gutterdrip",
		"env_rain_guttersplash",
		"", // END Marker
	};

	if ( !AllowWeatherParticles() )
	{
		if ( FindInList( s_WeatherEffects, pszParticleEffect ) )
			return false;
	}

	return true;
}
#endif


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::GetClassLimit( int iClass )
{
	if ( IsInTournamentMode() )
	{
		switch ( iClass )
		{
		case TF_CLASS_SCOUT: return tf_tournament_classlimit_scout.GetInt(); break;
		case TF_CLASS_SNIPER: return tf_tournament_classlimit_sniper.GetInt(); break;
		case TF_CLASS_SOLDIER: return tf_tournament_classlimit_soldier.GetInt(); break;
		case TF_CLASS_DEMOMAN: return tf_tournament_classlimit_demoman.GetInt(); break;
		case TF_CLASS_MEDIC: return tf_tournament_classlimit_medic.GetInt(); break;
		case TF_CLASS_HEAVYWEAPONS: return tf_tournament_classlimit_heavy.GetInt(); break;
		case TF_CLASS_PYRO: return tf_tournament_classlimit_pyro.GetInt(); break;
		case TF_CLASS_SPY: return tf_tournament_classlimit_spy.GetInt(); break;
		case TF_CLASS_ENGINEER: return tf_tournament_classlimit_engineer.GetInt(); break;
		default:
			break;
		}
	}
	else if ( IsInHighlanderMode() )
	{
		return 1;
	}
	else if ( tf_classlimit.GetInt() )
	{
		return tf_classlimit.GetInt();
	}

	return NO_CLASS_LIMIT;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::CanPlayerChooseClass( CBasePlayer *pPlayer, int iClass )
{
	int iClassLimit = GetClassLimit( iClass );

#ifdef TF_RAID_MODE
	if ( IsRaidMode() && !pPlayer->IsBot() )
	{
		// bots are exempt from class limits, to allow for additional support bot "friends"
		if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
		{
			if ( tf_raid_allow_all_classes.GetBool() == false )
			{
				if ( iClass == TF_CLASS_SCOUT )
					return false;

				if ( iClass == TF_CLASS_SPY )
					return false;
			}

			if ( tf_raid_enforce_unique_classes.GetBool() )
			{
				// only one of each class on the raiding team
				iClassLimit = 1;
			}
		}
	}
	else if ( IsBossBattleMode() )
	{
		return true;
	}
	else
#endif // TF_RAID_MODE

	if ( iClassLimit == NO_CLASS_LIMIT )
		return true;

	if ( pPlayer->GetTeamNumber() != TF_TEAM_BLUE && pPlayer->GetTeamNumber() != TF_TEAM_RED )
		return true;
#ifdef GAME_DLL
	CTFTeam *pTeam = assert_cast<CTFTeam*>(pPlayer->GetTeam());
#else
	C_TFTeam *pTeam = assert_cast<C_TFTeam*>(pPlayer->GetTeam());
#endif
	if ( !pTeam )
		return true;

	int iTeamClassCount = 0;
	for ( int iPlayer = 0; iPlayer < pTeam->GetNumPlayers(); iPlayer++ )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pTeam->GetPlayer( iPlayer ) );
		if ( pTFPlayer && pTFPlayer != pPlayer && pTFPlayer->GetPlayerClass()->GetClassIndex() == iClass )
		{
			iTeamClassCount++;
		}
	}

	return ( iTeamClassCount < iClassLimit );
}


#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Populate vector with set of control points the player needs to capture
void CTFGameRules::CollectCapturePoints( CBasePlayer *player, CUtlVector< CTeamControlPoint * > *captureVector ) const
{
	if ( !captureVector )
		return;

	captureVector->RemoveAll();

	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( pMaster )
	{
		// special case hack for KotH mode to use control points that are locked at the start of the round
		if ( IsInKothMode() && pMaster->GetNumPoints() == 1 )
		{
			captureVector->AddToTail( pMaster->GetControlPoint( 0 ) );
			return;
		}

		for( int i=0; i<pMaster->GetNumPoints(); ++i )
		{
			CTeamControlPoint *point = pMaster->GetControlPoint( i );
			if ( point && pMaster->IsInRound( point ) )
			{
				if ( ObjectiveResource()->GetOwningTeam( point->GetPointIndex() ) == player->GetTeamNumber() )
					continue;

				if ( player && player->IsBot() && point->ShouldBotsIgnore() )
					continue;

				if ( ObjectiveResource()->TeamCanCapPoint( point->GetPointIndex(), player->GetTeamNumber() ) )
				{
					if ( TeamplayGameRules()->TeamMayCapturePoint( player->GetTeamNumber(), point->GetPointIndex() ) )
					{
						// unlocked point not on our team available to capture
						captureVector->AddToTail( point );
					}
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Populate vector with set of control points the player needs to defend from capture
void CTFGameRules::CollectDefendPoints( CBasePlayer *player, CUtlVector< CTeamControlPoint * > *defendVector ) const
{
	if ( !defendVector )
		return;

	defendVector->RemoveAll();

	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( pMaster )
	{
		for( int i=0; i<pMaster->GetNumPoints(); ++i )
		{
			CTeamControlPoint *point = pMaster->GetControlPoint( i );
			if ( point && pMaster->IsInRound( point ) )
			{
				if ( ObjectiveResource()->GetOwningTeam( point->GetPointIndex() ) != player->GetTeamNumber() )
					continue;
					
				if ( player && player->IsBot() && point->ShouldBotsIgnore() )
					continue;

				if ( ObjectiveResource()->TeamCanCapPoint( point->GetPointIndex(), GetEnemyTeam( player->GetTeamNumber() ) ) )
				{
					if ( TeamplayGameRules()->TeamMayCapturePoint( GetEnemyTeam( player->GetTeamNumber() ), point->GetPointIndex() ) )
					{
						// unlocked point on our team vulnerable to capture
						defendVector->AddToTail( point );
					}
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
CObjectSentrygun *CTFGameRules::FindSentryGunWithMostKills( int team ) const
{
	CObjectSentrygun *dangerousSentry = NULL;
	int dangerousSentryKills = -1;

	for ( int i=0; i<IBaseObjectAutoList::AutoList().Count(); ++i )
	{
		CBaseObject *pObj = static_cast< CBaseObject* >( IBaseObjectAutoList::AutoList()[i] );
		if ( pObj->ObjectType() == OBJ_SENTRYGUN && pObj->GetTeamNumber() == team && pObj->GetKills() >= dangerousSentryKills )
		{
			dangerousSentryKills = pObj->GetKills();
			dangerousSentry = static_cast<CObjectSentrygun *>( pObj );
		}
	}

	return dangerousSentry;
}

#endif // GAME_DLL

#ifndef CLIENT_DLL

void CTFGameRules::Status( void (*print) (const char *fmt, ...) )
{
#if defined( _DEBUG )
	print( " == GameStats ==\n" );
	print( "Total Time: %d seconds\n", CTF_GameStats.m_currentMap.m_Header.m_iTotalTime );
	print( "Blue Team Wins: %d\n", CTF_GameStats.m_currentMap.m_Header.m_iBlueWins );
	print( "Red Team Wins: %d\n", CTF_GameStats.m_currentMap.m_Header.m_iRedWins );
	print( "Stalemates: %d\n", CTF_GameStats.m_currentMap.m_Header.m_iStalemates );

	print( "         Spawns Points Kills Deaths Assists\n" );
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; iClass++ )
	{
		TF_Gamestats_ClassStats_t &Stats = CTF_GameStats.m_currentMap.m_aClassStats[ iClass ];

		print( "%-8s %6d %6d %5d %6d %7d\n",
			g_aPlayerClassNames_NonLocalized[ iClass ],
			Stats.iSpawns, Stats.iScore, Stats.iKills, Stats.iDeaths, Stats.iAssists );
	}
	print( "\n" );
#endif // defined( _DEBUG )
}

#endif // !CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		::V_swap( collisionGroup0, collisionGroup1 );
	}
	
	//Don't stand on COLLISION_GROUP_WEAPONs
	if( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT &&
		collisionGroup1 == COLLISION_GROUP_WEAPON )
	{
		return false;
	}

	// Don't stand on projectiles
	if( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT &&
		collisionGroup1 == COLLISION_GROUP_PROJECTILE )
	{
		return false;
	}

	// Rockets need to collide with players when they hit, but
	// be ignored by player movement checks
	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER ) && 
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS || collisionGroup1 == TFCOLLISION_GROUP_ROCKET_BUT_NOT_WITH_OTHER_ROCKETS ) )
		return true;

	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT ) && 
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS || collisionGroup1 == TFCOLLISION_GROUP_ROCKET_BUT_NOT_WITH_OTHER_ROCKETS ) )
		return false;

	if ( ( collisionGroup0 == COLLISION_GROUP_WEAPON ) && 
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS || collisionGroup1 == TFCOLLISION_GROUP_ROCKET_BUT_NOT_WITH_OTHER_ROCKETS ) )
		return false;

	if ( ( collisionGroup0 == TF_COLLISIONGROUP_GRENADES ) && 
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS || collisionGroup1 == TFCOLLISION_GROUP_ROCKET_BUT_NOT_WITH_OTHER_ROCKETS ) )
		return false;

	if ( ( collisionGroup0 == COLLISION_GROUP_PROJECTILE ) && 
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS || collisionGroup1 == TFCOLLISION_GROUP_ROCKET_BUT_NOT_WITH_OTHER_ROCKETS ) )
		return false;

	if ( ( collisionGroup0 == TFCOLLISION_GROUP_ROCKETS ) &&
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKET_BUT_NOT_WITH_OTHER_ROCKETS ) )
		return false;

	if ( ( collisionGroup0 == TFCOLLISION_GROUP_ROCKET_BUT_NOT_WITH_OTHER_ROCKETS ) &&
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKET_BUT_NOT_WITH_OTHER_ROCKETS ) )
		return false;

	// Grenades don't collide with players. They handle collision while flying around manually.
	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER ) && 
		( collisionGroup1 == TF_COLLISIONGROUP_GRENADES ) )
		return false;

	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT ) && 
		( collisionGroup1 == TF_COLLISIONGROUP_GRENADES ) )
		return false;

	// Respawn rooms only collide with players
	if ( collisionGroup1 == TFCOLLISION_GROUP_RESPAWNROOMS )
		return ( collisionGroup0 == COLLISION_GROUP_PLAYER ) || ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT );
	
/*	if ( collisionGroup0 == COLLISION_GROUP_PLAYER )
	{
		// Players don't collide with objects or other players
		if ( collisionGroup1 == COLLISION_GROUP_PLAYER  )
			 return false;
 	}

	if ( collisionGroup1 == COLLISION_GROUP_PLAYER_MOVEMENT )
	{
		// This is only for probing, so it better not be on both sides!!!
		Assert( collisionGroup0 != COLLISION_GROUP_PLAYER_MOVEMENT );

		// No collide with players any more
		// Nor with objects or grenades
		switch ( collisionGroup0 )
		{
		default:
			break;
		case COLLISION_GROUP_PLAYER:
			return false;
		}
	}
*/
	// don't want caltrops and other grenades colliding with each other
	// caltops getting stuck on other caltrops, etc.)
	if ( ( collisionGroup0 == TF_COLLISIONGROUP_GRENADES ) && 
		 ( collisionGroup1 == TF_COLLISIONGROUP_GRENADES ) )
	{
		return false;
	}


	if ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT &&
		collisionGroup1 == TFCOLLISION_GROUP_COMBATOBJECT )
	{
		return false;
	}

	if ( collisionGroup0 == COLLISION_GROUP_PLAYER &&
		collisionGroup1 == TFCOLLISION_GROUP_COMBATOBJECT )
	{
		return false;
	}

	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER || collisionGroup0 ==  COLLISION_GROUP_PLAYER_MOVEMENT ) &&
		 collisionGroup1 == TFCOLLISION_GROUP_TANK )
	{
		return false;
	}

	return BaseClass::ShouldCollide( collisionGroup0, collisionGroup1 ); 
}

//-----------------------------------------------------------------------------
// Purpose: Return the value of this player towards capturing a point
//-----------------------------------------------------------------------------
int	CTFGameRules::GetCaptureValueForPlayer( CBasePlayer *pPlayer )
{
	int nValue = BaseClass::GetCaptureValueForPlayer( pPlayer );


	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( pTFPlayer->IsPlayerClass( TF_CLASS_SCOUT ) )
	{
		if ( mp_capstyle.GetInt() == 1 )
		{
			// Scouts count for 2 people in timebased capping.
			nValue = 2;
		}
		else
		{
			// Scouts can cap all points on their own.
			nValue = 10;
		}
	}

	return nValue;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::GetTimeLeft( void )
{
	float flTimeLimit = mp_timelimit.GetInt() * 60;

	Assert( flTimeLimit > 0 && "Should not call this function when !IsGameUnderTimeLimit" );

	float flMapChangeTime = m_flMapResetTime + flTimeLimit;

	int iTime = (int)(flMapChangeTime - gpGlobals->curtime);
	if ( iTime < 0 )
	{
		iTime = 0;
	}

	return ( iTime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::FireGameEvent( IGameEvent *event )
{
	const char *eventName = event->GetName();

#ifdef GAME_DLL
	if ( !Q_strcmp( eventName, "teamplay_point_captured" ) )
	{
		// keep track of how many times each team caps
		int iTeam = event->GetInt( "team" );
		Assert( iTeam >= FIRST_GAME_TEAM && iTeam < TF_TEAM_COUNT );
		m_iNumCaps[iTeam]++;

		// award a capture to all capping players
		const char *cappers = event->GetString( "cappers" );

		Q_strncpy( m_szMostRecentCappers, cappers, ARRAYSIZE( m_szMostRecentCappers ) );	
		for ( int i =0; i < Q_strlen( cappers ); i++ )
		{
			int iPlayerIndex = (int) cappers[i];
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
			if ( pPlayer )
			{
				CTF_GameStats.Event_PlayerCapturedPoint( pPlayer );
			}
		}
	}
	else if ( !Q_strcmp( eventName, "teamplay_capture_blocked" ) )
	{
		int iPlayerIndex = event->GetInt( "blocker" );
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
		CTF_GameStats.Event_PlayerDefendedPoint( pPlayer );

		pPlayer->m_Shared.CheckForAchievement( ACHIEVEMENT_TF_MEDIC_CHARGE_BLOCKER );
	}	
	else if ( !Q_strcmp( eventName, "teamplay_round_win" ) )
	{
		int iWinningTeam = event->GetInt( "team" );
		bool bFullRound = event->GetBool( "full_round" );
		float flRoundTime = event->GetFloat( "round_time" );
		bool bWasSuddenDeath = event->GetBool( "was_sudden_death" );
		CTF_GameStats.Event_RoundEnd( iWinningTeam, bFullRound, flRoundTime, bWasSuddenDeath );
	}
	else if ( !Q_strcmp( eventName, "teamplay_flag_event" ) )
	{
		// if this is a capture event, remember the player who made the capture		
		int iEventType = event->GetInt( "eventtype" );
		if ( TF_FLAGEVENT_CAPTURE == iEventType )
		{
			int iPlayerIndex = event->GetInt( "player" );
			m_szMostRecentCappers[0] = iPlayerIndex;
			m_szMostRecentCappers[1] = 0;
		}
	}
#else	// CLIENT_DLL
	if ( !Q_strcmp( eventName, "overtime_nag" ) )
	{
		HandleOvertimeBegin();
	}
#endif

	BaseClass::FireGameEvent( event );
}


const char *CTFGameRules::GetGameTypeName( void )
{
	return ::GetGameTypeName( m_nGameType.Get() );
}


void CTFGameRules::ClientSpawned( edict_t * pPlayer )
{
}

void CTFGameRules::OnFileReceived( const char * fileName, unsigned int transferID )
{
}

//-----------------------------------------------------------------------------
// Purpose: Init ammo definitions
//-----------------------------------------------------------------------------

// shared ammo definition
// JAY: Trying to make a more physical bullet response
#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.002285*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION			1	

// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)


CAmmoDef* GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;

	if ( !bInitted )
	{
		bInitted = true;
		
		// Start at 1 here and skip the dummy ammo type to make CAmmoDef use the same indices
		// as our #defines.
		for ( int i=1; i < TF_AMMO_COUNT; i++ )
		{
			const char *pszAmmoName = GetAmmoName( i );
			def.AddAmmoType( pszAmmoName, DMG_BULLET | DMG_USEDISTANCEMOD | DMG_NOCLOSEDISTANCEMOD, TRACER_LINE, 0, 0, "ammo_max", 2400, 10, 14 );
			Assert( def.Index( pszAmmoName ) == i );
		}
	}

	return &def;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFGameRules::GetTeamGoalString( int iTeam )
{
	if ( iTeam == TF_TEAM_RED )
		return m_pszTeamGoalStringRed.Get();
	if ( iTeam == TF_TEAM_BLUE )
		return m_pszTeamGoalStringBlue.Get();
	return NULL;
}

#ifdef GAME_DLL

	Vector MaybeDropToGround( 
							CBaseEntity *pMainEnt, 
							bool bDropToGround, 
							const Vector &vPos, 
							const Vector &vMins, 
							const Vector &vMaxs )
	{
		if ( bDropToGround )
		{
			trace_t trace;
			UTIL_TraceHull( vPos, vPos + Vector( 0, 0, -500 ), vMins, vMaxs, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &trace );
			return trace.endpos;
		}
		else
		{
			return vPos;
		}
	}

	//-----------------------------------------------------------------------------
	// Purpose: This function can be used to find a valid placement location for an entity.
	//			Given an origin to start looking from and a minimum radius to place the entity at,
	//			it will sweep out a circle around vOrigin and try to find a valid spot (on the ground)
	//			where mins and maxs will fit.
	// Input  : *pMainEnt - Entity to place
	//			&vOrigin - Point to search around
	//			fRadius - Radius to search within
	//			nTries - Number of tries to attempt
	//			&mins - mins of the Entity
	//			&maxs - maxs of the Entity
	//			&outPos - Return point
	// Output : Returns true and fills in outPos if it found a spot.
	//-----------------------------------------------------------------------------
	bool EntityPlacementTest( CBaseEntity *pMainEnt, const Vector &vOrigin, Vector &outPos, bool bDropToGround )
	{
		// This function moves the box out in each dimension in each step trying to find empty space like this:
		//
		//											  X  
		//							   X			  X  
		// Step 1:   X     Step 2:    XXX   Step 3: XXXXX
		//							   X 			  X  
		//											  X  
		//

		Vector mins, maxs;
		pMainEnt->CollisionProp()->WorldSpaceAABB( &mins, &maxs );
		mins -= pMainEnt->GetAbsOrigin();
		maxs -= pMainEnt->GetAbsOrigin();

		// Put some padding on their bbox.
		float flPadSize = 5;
		Vector vTestMins = mins - Vector( flPadSize, flPadSize, flPadSize );
		Vector vTestMaxs = maxs + Vector( flPadSize, flPadSize, flPadSize );

		// First test the starting origin.
		if ( UTIL_IsSpaceEmpty( pMainEnt, vOrigin + vTestMins, vOrigin + vTestMaxs ) )
		{
			outPos = MaybeDropToGround( pMainEnt, bDropToGround, vOrigin, vTestMins, vTestMaxs );
			return true;
		}

		Vector vDims = vTestMaxs - vTestMins;


		// Keep branching out until we get too far.
		int iCurIteration = 0;
		int nMaxIterations = 15;

		int offset = 0;
		do
		{
			for ( int iDim=0; iDim < 3; iDim++ )
			{
				float flCurOffset = offset * vDims[iDim];

				for ( int iSign=0; iSign < 2; iSign++ )
				{
					Vector vBase = vOrigin;
					vBase[iDim] += (iSign*2-1) * flCurOffset;

					if ( UTIL_IsSpaceEmpty( pMainEnt, vBase + vTestMins, vBase + vTestMaxs ) )
					{
						// Ensure that there is a clear line of sight from the spawnpoint entity to the actual spawn point.
						// (Useful for keeping things from spawning behind walls near a spawn point)
						trace_t tr;
						UTIL_TraceLine( vOrigin, vBase, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &tr );

						if ( tr.fraction != 1.0 )
						{
							continue;
						}

						outPos = MaybeDropToGround( pMainEnt, bDropToGround, vBase, vTestMins, vTestMaxs );
						return true;
					}
				}
			}

			++offset;
		} while ( iCurIteration++ < nMaxIterations );

		//	Warning( "EntityPlacementTest for ent %d:%s failed!\n", pMainEnt->entindex(), pMainEnt->GetClassname() );
		return false;
	}

#else // GAME_DLL

CTFGameRules::~CTFGameRules()
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	m_bRecievedBaseline |= updateType == DATA_UPDATE_CREATED;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::HandleOvertimeBegin()
{
	C_TFPlayer *pTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pTFPlayer )
	{
		pTFPlayer->EmitSound( "Game.Overtime" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldShowTeamGoal( void )
{
	if ( State_Get() == GR_STATE_PREROUND || State_Get() == GR_STATE_RND_RUNNING || InSetup() )
		return true;

	return false;
}

#endif

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::ShutdownCustomResponseRulesDicts()
{
	DestroyCustomResponseSystems();

	if ( m_ResponseRules.Count() != 0 )
	{
		int nRuleCount = m_ResponseRules.Count();
		for ( int iRule = 0; iRule < nRuleCount; ++iRule )
		{
			m_ResponseRules[iRule].m_ResponseSystems.Purge();
		}
		m_ResponseRules.Purge();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::InitCustomResponseRulesDicts()
{
	MEM_ALLOC_CREDIT();

	// Clear if necessary.
	ShutdownCustomResponseRulesDicts();

	// Initialize the response rules for TF.
	m_ResponseRules.AddMultipleToTail( TF_CLASS_COUNT_ALL );

	char szName[512];
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_CLASS_COUNT_ALL; ++iClass )
	{
		m_ResponseRules[iClass].m_ResponseSystems.AddMultipleToTail( MP_TF_CONCEPT_COUNT );

		for ( int iConcept = 0; iConcept < MP_TF_CONCEPT_COUNT; ++iConcept )
		{
			AI_CriteriaSet criteriaSet;
			criteriaSet.AppendCriteria( "playerclass", g_aPlayerClassNames_NonLocalized[iClass] );
			criteriaSet.AppendCriteria( "Concept", g_pszMPConcepts[iConcept] );

			// 1 point for player class and 1 point for concept.
			float flCriteriaScore = 2.0f;

			// Name.
			V_sprintf_safe( szName, "%s_%s\n", g_aPlayerClassNames_NonLocalized[iClass], g_pszMPConcepts[iConcept] );
			m_ResponseRules[iClass].m_ResponseSystems[iConcept] = BuildCustomResponseSystemGivenCriteria( "scripts/talker/response_rules.txt", szName, criteriaSet, flCriteriaScore );
		}		
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifdef _DEBUG
CON_COMMAND( hud_notify, "Show a hud notification." )
{
	if ( args.ArgC() < 2 )
	{
		Warning( "Requires one argument, HudNotification_t, between 0 and %i\n", NUM_STOCK_NOTIFICATIONS );
		return;
	}

	if ( !TFGameRules() )
	{
		Warning( "Can't do that right now\n" );
		return;
	}

	CRecipientFilter filter;
	filter.AddAllPlayers();	
	TFGameRules()->SendHudNotification( filter, (HudNotification_t) V_atoi(args.Arg(1)) );
}
#endif

void CTFGameRules::SendHudNotification( IRecipientFilter &filter, HudNotification_t iType, bool bForceShow /*= false*/ )
{
	if ( !bForceShow && IsInWaitingForPlayers() )
		return;

	UserMessageBegin( filter, "HudNotify" );
		WRITE_BYTE( iType );
		WRITE_BOOL( bForceShow );	// Display in cl_hud_minmode
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SendHudNotification( IRecipientFilter &filter, const char *pszText, const char *pszIcon, int iTeam /*= TEAM_UNASSIGNED*/ )
{
	if ( IsInWaitingForPlayers() )
		return;

	UserMessageBegin( filter, "HudNotifyCustom" );
		WRITE_STRING( pszText );
		WRITE_STRING( pszIcon );
		WRITE_BYTE( iTeam );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: Is the player past the required delays for spawning
//-----------------------------------------------------------------------------
bool CTFGameRules::HasPassedMinRespawnTime( CBasePlayer *pPlayer )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

	if ( pTFPlayer && pTFPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_UNDEFINED )
		return true;

	float flMinSpawnTime = GetMinTimeWhenPlayerMaySpawn( pPlayer ); 

	return ( gpGlobals->curtime > flMinSpawnTime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldRespawnQuickly( CBasePlayer *pPlayer )
{
#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetBool() )
		return true;
#endif // _DEBUG || STAGING_ONLY

	return BaseClass::ShouldRespawnQuickly( pPlayer );
}

typedef bool (*BIgnoreConvarChangeFunc)(void);

struct convar_tags_t
{
	const char *pszConVar;
	const char *pszTag;
	BIgnoreConvarChangeFunc ignoreConvarFunc;
};

// The list of convars that automatically turn on tags when they're changed.
// Convars in this list need to have the FCVAR_NOTIFY flag set on them, so the
// tags are recalculated and uploaded to the master server when the convar is changed.
convar_tags_t convars_to_check_for_tags[] =
{
	{ "mp_friendlyfire", "friendlyfire", NULL },
	{ "tf_birthday", "birthday", NULL },
	{ "mp_respawnwavetime", "respawntimes", NULL },
	{ "mp_fadetoblack", "fadetoblack", NULL },
	{ "tf_weapon_criticals", "nocrits", NULL },
	{ "mp_disable_respawn_times", "norespawntime", NULL },
	{ "tf_gamemode_arena", "arena", NULL },
	{ "tf_gamemode_cp", "cp", NULL },
	{ "tf_gamemode_ctf", "ctf", NULL },
	{ "tf_gamemode_tc",	"tc", NULL },
	{ "tf_beta_content", "beta", NULL },
	{ "tf_damage_disablespread", "dmgspread", NULL },
	{ "mp_highlander", "highlander", NULL },
	{ "sv_registration_successful", "_registered", NULL },
	{ "tf_server_identity_disable_quickplay", "noquickplay", NULL },
	{ "tf_gamemode_misc", "misc", NULL }, // catch-all for matchmaking to identify sd, tc, and pd servers via sv_tags
};

//-----------------------------------------------------------------------------
// Purpose: Engine asks for the list of convars that should tag the server
//-----------------------------------------------------------------------------
void CTFGameRules::GetTaggedConVarList( KeyValues *pCvarTagList )
{
	BaseClass::GetTaggedConVarList( pCvarTagList );

	for ( int i = 0; i < ARRAYSIZE(convars_to_check_for_tags); i++ )
	{
		if ( convars_to_check_for_tags[i].ignoreConvarFunc && convars_to_check_for_tags[i].ignoreConvarFunc() )
			continue;

		KeyValues *pKV = new KeyValues( "tag" );
		pKV->SetString( "convar", convars_to_check_for_tags[i].pszConVar );
		pKV->SetString( "tag", convars_to_check_for_tags[i].pszTag );

		pCvarTagList->AddSubKey( pKV );
	}
}
#endif


//-----------------------------------------------------------------------------
// Purpose: Get the video file name for the current map.
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
const char *CTFGameRules::GetVideoFileForMap( bool bWithExtension /*= true*/ )
{
	char mapname[MAX_MAP_NAME];
	mapname[0] = 0;

	Q_FileBase( engine->GetLevelName(), mapname, sizeof( mapname ) );
	if ( mapname[0] == 0 )
	{
		return NULL;
	}
	
	Q_strlower( mapname );
	return FormatVideoName( (const char *)mapname, bWithExtension );
}

//=============================================================================
// HPE_BEGIN
// [msmith] Used for the client to tell the server that we're watching a movie or not.
//			Also contains the name of a movie if it's an in game video.
//=============================================================================
//-----------------------------------------------------------------------------
// Purpose: Format a video file name from the name passed in.
//-----------------------------------------------------------------------------
const char *CTFGameRules::FormatVideoName( const char *videoName, bool bWithExtension /*= true*/ )
{

	static char strFullpath[MAX_PATH];		// this buffer is returned to the caller
	
#ifdef _X360
	// Should we be modifying a const buffer?
	// need to remove the .360 extension on the end of the map name
	char *pExt = Q_stristr( videoName, ".360" );
	if ( pExt )
	{
		*pExt = '\0';
	}
#endif

	Q_strncpy( strFullpath, "media/", MAX_PATH );	// Assume we must play out of the media directory

	if ( Q_strstr( videoName, "arena_" ) )
	{
		char strTempPath[MAX_PATH];
		Q_strncpy( strTempPath, "media/", MAX_PATH );
		Q_strncat( strTempPath, videoName, MAX_PATH );
		Q_strncat( strTempPath, FILE_EXTENSION_ANY_MATCHING_VIDEO, MAX_PATH );	

		VideoSystem_t vSystem = VideoSystem::NONE;

		// default to arena_intro video if we can't find the specified video
		if ( !g_pVideo || g_pVideo->LocatePlayableVideoFile( strTempPath, "GAME", &vSystem, strFullpath, sizeof(strFullpath), VideoSystemFeature::PLAY_VIDEO_FILE_IN_MATERIAL ) != VideoResult::SUCCESS )
		{
			V_strncpy( strFullpath, "media/" "arena_intro", MAX_PATH );
		}
	}
	else if ( Q_strstr( videoName, "mvm_" ) )
	{
		char strTempPath[MAX_PATH];
		Q_strncpy( strTempPath, "media/", MAX_PATH );
		Q_strncat( strTempPath, videoName, MAX_PATH );
		Q_strncat( strTempPath, FILE_EXTENSION_ANY_MATCHING_VIDEO, MAX_PATH );	

		VideoSystem_t vSystem = VideoSystem::NONE;

		// default to mvm_intro video if we can't find the specified video
		if ( !g_pVideo || g_pVideo->LocatePlayableVideoFile( strTempPath, "GAME", &vSystem, strFullpath, sizeof(strFullpath), VideoSystemFeature::PLAY_VIDEO_FILE_IN_MATERIAL ) != VideoResult::SUCCESS )
		{
			V_strncpy( strFullpath, "media/" "mvm_intro", MAX_PATH );
		}
	}
	else if ( Q_strstr( videoName, "zi_" ) )
	{
		char strTempPath[ MAX_PATH ];
		Q_strncpy( strTempPath, "media/", MAX_PATH );
		Q_strncat( strTempPath, videoName, MAX_PATH );
		Q_strncat( strTempPath, FILE_EXTENSION_ANY_MATCHING_VIDEO, MAX_PATH );

		VideoSystem_t vSystem = VideoSystem::NONE;

		// default to zi_intro video if we can't find the specified video
		if ( !g_pVideo || g_pVideo->LocatePlayableVideoFile( strTempPath, "GAME", &vSystem, strFullpath, sizeof( strFullpath ), VideoSystemFeature::PLAY_VIDEO_FILE_IN_MATERIAL ) != VideoResult::SUCCESS )
		{
			V_strncpy( strFullpath, "media/" "zi_intro", MAX_PATH );
		}
	}
	else if ( Q_strstr( videoName, "vsh_" ) )
	{
		char strTempPath[ MAX_PATH ];
		Q_strncpy( strTempPath, "media/", MAX_PATH );
		Q_strncat( strTempPath, videoName, MAX_PATH );
		Q_strncat( strTempPath, FILE_EXTENSION_ANY_MATCHING_VIDEO, MAX_PATH );

		VideoSystem_t vSystem = VideoSystem::NONE;

		// default to vsh_intro video if we can't find the specified video
		if ( !g_pVideo || g_pVideo->LocatePlayableVideoFile( strTempPath, "GAME", &vSystem, strFullpath, sizeof( strFullpath ), VideoSystemFeature::PLAY_VIDEO_FILE_IN_MATERIAL ) != VideoResult::SUCCESS )
		{
			V_strncpy( strFullpath, "media/" "vsh_intro", MAX_PATH );
		}
	}
	else
	{
		Q_strncat( strFullpath, videoName, MAX_PATH );
	}

	if ( bWithExtension )
	{
		Q_strncat( strFullpath, FILE_EXTENSION_ANY_MATCHING_VIDEO, MAX_PATH );		// Don't assume any specific video type, let the video services find it
	}

	return strFullpath;
}
#endif
//=============================================================================
// HPE_END
//=============================================================================

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *GetMapDisplayName( const char *mapName, bool bTitleCase /* = false */ )
{
	static char szDisplayName[256];
	char szTempName[256];
	const char *pszSrc = NULL;

	szDisplayName[0] = '\0';

	if ( !mapName )
		return szDisplayName;

	// check our lookup table
	Q_strncpy( szTempName, mapName, sizeof( szTempName ) );
	Q_strlower( szTempName );
	pszSrc = szTempName;

	for ( int i = 0; i < ARRAYSIZE( s_ValveMaps ); ++i )
	{
		if ( !Q_stricmp( s_ValveMaps[i].pDiskName, pszSrc ) )
		{
			return s_ValveMaps[i].pDisplayName;
		}
	}

	char *pszFinal = Q_strstr( pszSrc, "_final" );
	if ( pszFinal )
	{
		// truncate the _final (or _final1) part of the filename if it's at the end of the name
		char *pszNextChar = pszFinal + Q_strlen( "_final" );
		if ( pszNextChar )
		{
			if ( ( *pszNextChar == '\0' ) ||
				 ( ( *pszNextChar == '1' ) && ( *(pszNextChar+1) == '\0' ) ) )
			{
				*pszFinal = '\0';
			}
		}
	}

	// Our workshop maps will be of the format workshop/cp_somemap.ugc12345
	const char szWorkshop[] = "workshop/";
	if ( V_strncmp( pszSrc, szWorkshop, sizeof( szWorkshop ) - 1 ) == 0 )
	{
		pszSrc += sizeof( szWorkshop ) - 1;
		char *pszUGC = V_strstr( pszSrc, ".ugc" );
		int nUGCLen = pszUGC ? strlen( pszUGC ) : 0;
		if ( pszUGC && nUGCLen > 4 )
		{
			int i;
			for ( i = 4; i < nUGCLen; i ++ )
			{
				if ( pszUGC[i] < '0' || pszUGC[i] > '9' )
				{
					break;
				}
			}

			if ( i == nUGCLen )
			{
				*pszUGC = '\0';
			}
		}
	}

	// we haven't found a "friendly" map name, so let's just clean up what we have
	if ( !Q_strncmp( pszSrc, "cp_", 3 ) ||
		 !Q_strncmp( pszSrc, "tc_", 3 ) )
	{
		pszSrc +=  3;
	}
	else if ( !Q_strncmp( pszSrc, "ctf_", 4 ) )
	{
		pszSrc +=  4;
	}

	Q_strncpy( szDisplayName, pszSrc, sizeof( szDisplayName ) );

	// replace underscores with spaces
	for ( char *pszUnderscore = szDisplayName ; pszUnderscore != NULL && *pszUnderscore != 0 ; pszUnderscore++ )
	{
		// Replace it with a space
		if ( *pszUnderscore == '_' )
		{
			*pszUnderscore = ' ';
		}
	}

	if ( bTitleCase )
	{
		V_strtitlecase( szDisplayName );
	}
	else
	{
		// Default behavior - tf maps are LOUD
		Q_strupr( szDisplayName );
	}

	return szDisplayName;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *GetMapType( const char *mapName )
{
	int i;

	if ( mapName )
	{
		for ( i = 0; i < ARRAYSIZE( s_ValveMaps ); ++i )
		{
			if ( !Q_stricmp( s_ValveMaps[i].pDiskName, mapName ) )
			{
				return s_ValveMaps[i].pGameType;
			}
		}
	}

	if ( !IsX360() )
	{
		// we haven't found a "friendly" map name, so let's just clean up what we have
		if ( !Q_strnicmp( mapName, "cp_", 3 ) )
		{
			return "#Gametype_CP";
		}
		else if ( !Q_strnicmp( mapName, "tc_", 3 ) )
		{
			return "#TF_TerritoryControl";
		}
		else if ( !Q_strnicmp( mapName, "ctf_", 4 ) )
		{
			return "#Gametype_CTF";
		}
		else
		{
			if ( TFGameRules() )
			{
				return TFGameRules()->GetGameTypeName();
			}
		}
	}

	return "";
}
#endif

#ifdef GAME_DLL

BEGIN_DATADESC(CCPTimerLogic)
	DEFINE_KEYFIELD( m_iszControlPointName, FIELD_STRING, "controlpoint" ),
	DEFINE_KEYFIELD( m_nTimerLength, FIELD_INTEGER,	"timer_length" ),
	DEFINE_KEYFIELD( m_nTimerTeam, FIELD_INTEGER, "team_number" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "RoundSpawn", InputRoundSpawn ),

	DEFINE_OUTPUT( m_onCountdownStart, "OnCountdownStart" ),
	DEFINE_OUTPUT( m_onCountdown15SecRemain, "OnCountdown15SecRemain" ),
	DEFINE_OUTPUT( m_onCountdown10SecRemain, "OnCountdown10SecRemain" ),
	DEFINE_OUTPUT( m_onCountdown5SecRemain, "OnCountdown5SecRemain" ),
	DEFINE_OUTPUT( m_onCountdownEnd, "OnCountdownEnd" ),
END_DATADESC();

LINK_ENTITY_TO_CLASS( tf_logic_cp_timer, CCPTimerLogic );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCPTimerLogic::InputRoundSpawn( inputdata_t &input )
{
	if ( m_iszControlPointName != NULL_STRING )
	{
		// We need to re-find our control point, because they're recreated over round restarts
		m_hControlPoint = dynamic_cast<CTeamControlPoint*>( gEntList.FindEntityByName( NULL, m_iszControlPointName ) );
		if ( !m_hControlPoint )
		{
			Warning( "%s failed to find control point named '%s'\n", GetClassname(), STRING(m_iszControlPointName) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CCPTimerLogic::TimerMayExpire( void )
{
	if ( m_hControlPoint )
	{
		if ( TeamplayGameRules()->TeamMayCapturePoint( m_nTimerTeam, m_hControlPoint->GetPointIndex() ) )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCPTimerLogic::Think( void )
{
	if ( !TFGameRules() || !ObjectiveResource() )
		return;
	
	if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
	{
		// game has already been won, our job is done
		m_pointTimer.Invalidate();
		SetContextThink( &CCPTimerLogic::Think, gpGlobals->curtime + 0.15, CP_TIMER_THINK );
	}

	if ( m_hControlPoint )
	{
		if ( TeamplayGameRules()->TeamMayCapturePoint( m_nTimerTeam, m_hControlPoint->GetPointIndex() ) )
		{
			if ( !m_pointTimer.HasStarted() )
			{
				m_pointTimer.Start( m_nTimerLength );
				m_onCountdownStart.FireOutput( this, this );

				ObjectiveResource()->SetCPTimerTime( m_hControlPoint->GetPointIndex(), gpGlobals->curtime + m_nTimerLength );
			}
			else
			{
				if ( m_pointTimer.IsElapsed() )
				{
					// the point must be fully owned by the owner before we reset
					if ( ObjectiveResource()->GetCappingTeam( m_hControlPoint->GetPointIndex() ) == TEAM_UNASSIGNED )
					{
						m_pointTimer.Invalidate();
						m_onCountdownEnd.FireOutput( this, this );
						m_bFire15SecRemain = m_bFire10SecRemain = m_bFire5SecRemain = true;

						ObjectiveResource()->SetCPTimerTime( m_hControlPoint->GetPointIndex(), -1.0f );
					}
				}
				else 
				{
					float flRemainingTime = m_pointTimer.GetRemainingTime();

					if ( flRemainingTime <= 15.0f && m_bFire15SecRemain )
					{
						m_bFire15SecRemain = false;
					}
					else if ( flRemainingTime <= 10.0f && m_bFire10SecRemain )
					{
						m_bFire10SecRemain = false;
					}
					else if ( flRemainingTime <= 5.0f && m_bFire5SecRemain )
					{
						m_bFire5SecRemain = false;
					}
				}
			}
		}
		else
		{
			m_pointTimer.Invalidate();
		}
	}

	SetContextThink( &CCPTimerLogic::Think, gpGlobals->curtime + 0.15, CP_TIMER_THINK );
}
#ifdef _DEBUG
#endif
#endif

int CTFGameRules::GetStatsMinimumPlayers( void )
{
	if ( IsInArenaMode() == true )
	{
		return 1;
	}

	return 3;
}

int CTFGameRules::GetStatsMinimumPlayedTime( void )
{
	return 4 * 60; //Default of 4 minutes
}

bool CTFGameRules::IsConnectedUserInfoChangeAllowed( CBasePlayer *pPlayer )
{
	CTFPlayer* pTFPlayer = NULL;
#ifdef GAME_DLL
	pTFPlayer = ToTFPlayer( pPlayer );
#else
	pTFPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
#endif

	if( pTFPlayer )
	{
		// We can change if we're not alive
		if( pTFPlayer->m_lifeState != LIFE_ALIVE )
			return true;
		
		// We can change if we're not on team red or blue
		int iPlayerTeam = pTFPlayer->GetTeamNumber();
		if( ( iPlayerTeam != TF_TEAM_RED ) && ( iPlayerTeam != TF_TEAM_BLUE ) )
			return true;
		
		// We can change if we've respawned/changed classes within the last 2 seconds.
		// This allows for <classname>.cfg files to change these types of convars
		float flRespawnTime = 0.f;
#ifdef GAME_DLL
		flRespawnTime = pTFPlayer->GetSpawnTime();	// Called everytime the player respawns
#else
		flRespawnTime = pTFPlayer->GetClassChangeTime(); // Called when the player changes class and respawns
#endif
		if( ( gpGlobals->curtime - flRespawnTime ) < 2.f )
			return true;

		// CTFPlayerShared has an option to suppress prediction. If the client is trying to change itself to match that
		// requested state, it needs to be allowed to avoid prediction desync.
		bool bShouldBePredicting = true;

#ifdef GAME_DLL
		// server - if client's new userinfo is attempting to change their prediction state to the value CTFPlayerShared
		// wants, allow it regardless.
		bool bIsPredicting = pTFPlayer->m_bRequestPredict;
		bool bRequestingPredict = Q_atoi( engine->GetClientConVarValue( pTFPlayer->entindex(), "cl_predict" ) ) != 0;
		if ( bShouldBePredicting != bIsPredicting && bRequestingPredict == bShouldBePredicting )
			{ return true; }
#else
		// client - If we're trying to change our cl_predict to what CTFPlayerShared wants, it's allowed
		static ConVarRef cl_predict( "cl_predict" );
		bool bIsPredicting = cl_predict.GetBool();
		if ( bShouldBePredicting != bIsPredicting )
		{
			return true;
		}
#endif
	}

	return false;
}

//==================================================================================================================
// BONUS ROUND LOGIC
#ifndef CLIENT_DLL
EXTERN_SEND_TABLE( DT_ScriptCreatedItem );
#else
EXTERN_RECV_TABLE( DT_ScriptCreatedItem );
#endif

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::ProcessVerboseLogOutput( void )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		if ( pPlayer && ( pPlayer->GetTeamNumber() > TEAM_UNASSIGNED ) )
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" position_report (position \"%d %d %d\")\n",   
				pPlayer->GetPlayerName(),
				pPlayer->GetUserID(),
				pPlayer->GetNetworkIDString(),
				pPlayer->GetTeam()->GetName(),
				(int)pPlayer->GetAbsOrigin().x, 
				(int)pPlayer->GetAbsOrigin().y,
				(int)pPlayer->GetAbsOrigin().z );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::MatchSummaryStart( void )
{
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer )
		{
			pPlayer->AddFlag( FL_FROZEN );
		}
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "show_match_summary" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}

	m_bShowMatchSummary.Set( true );
	m_flMatchSummaryTeleportTime = gpGlobals->curtime + 2.f;

	if ( m_hGamerulesProxy )
	{
		m_hGamerulesProxy->MatchSummaryStart();
	}

	CBaseEntity *pLogicCase = NULL;
	while ( ( pLogicCase = gEntList.FindEntityByName( pLogicCase, "competitive_stage_logic_case" ) ) != NULL )
	{
		if ( pLogicCase )
		{
			variant_t sVariant;
			sVariant.SetInt( GetWinningTeam() );
			pLogicCase->AcceptInput( "InValue", NULL, NULL, sVariant, 0 );
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::MatchSummaryEnd( void )
{
	m_bShowMatchSummary.Set( false );
	m_bPlayersAreOnMatchSummaryStage.Set( false );

	SetRequiredObserverTarget( NULL );

	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( !pPlayer )
			continue;

		pPlayer->RemoveFlag( FL_FROZEN );
		pPlayer->SetViewEntity( NULL );
		pPlayer->SetFOV( pPlayer, 0 );
	}

	// reset bot convars here
	static ConVarRef tf_bot_quota( "tf_bot_quota" );
	tf_bot_quota.SetValue( tf_bot_quota.GetDefault() );
	static ConVarRef tf_bot_quota_mode( "tf_bot_quota_mode" );
	tf_bot_quota_mode.SetValue( tf_bot_quota_mode.GetDefault() );
}

void CTFGameRules::PushAllPlayersAway( const Vector& vFromThisPoint, float flRange, float flForce, int nTeam, CUtlVector< CTFPlayer* > *pPushedPlayers /*= NULL*/ )
{
	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, nTeam, COLLECT_ONLY_LIVING_PLAYERS );

	for( int i=0; i<playerVector.Count(); ++i )
	{
		CTFPlayer *pPlayer = playerVector[i];

		Vector toPlayer = pPlayer->EyePosition() - vFromThisPoint;

		if ( toPlayer.LengthSqr() < flRange * flRange )
		{
			// send the player flying
			// make sure we push players up and away
			toPlayer.z = 0.0f;
			toPlayer.NormalizeInPlace();
			toPlayer.z = 1.0f;

			Vector vPush = flForce * toPlayer;

			pPlayer->ApplyAbsVelocityImpulse( vPush );

			if ( pPushedPlayers )
			{
				pPushedPlayers->AddToTail( pPlayer );
			}
		}
	}
}

#endif // GAME_DLL

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::BalanceTeams( bool bRequireSwitcheesToBeDead )
{
	if ( mp_autoteambalance.GetInt() == 2 )
		return;

	BaseClass::BalanceTeams( bRequireSwitcheesToBeDead );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::PointsMayBeCaptured( void )
{
	if ( GetActiveRoundTimer() && InSetup() )
		return false;

	return BaseClass::PointsMayBeCaptured();
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::BroadcastSound( int iTeam, const char *sound, int iAdditionalSoundFlags /* = 0 */, CBasePlayer *pPlayer /* = NULL */ )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

	//send it to everyone
	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_broadcast_audio" );
	if ( event )
	{
		event->SetInt( "team", iTeam );
		event->SetString( "sound", sound );
		event->SetInt( "additional_flags", iAdditionalSoundFlags );
		event->SetInt( "player", pTFPlayer ? pTFPlayer->entindex() : -1 );
		gameeventmanager->FireEvent( event );
	}
}

#define TF_GAMERULES_SCRIPT_FUNC( function, desc ) \
	ScriptRegisterFunctionNamed( g_pScriptVM, Script##function, #function, desc )

int		ScriptGetRoundState()										{ return TFGameRules()->GetRoundState(); }
bool	ScriptIsInWaitingForPlayers()								{ return TFGameRules()->IsInWaitingForPlayers(); }
int		ScriptGetWinningTeam()										{ return TFGameRules()->GetWinningTeam(); }
bool	ScriptInOvertime()											{ return TFGameRules()->InOvertime(); }
bool	ScriptIsBirthday()											{ return TFGameRules()->IsBirthday(); }
bool	ScriptIsHolidayActive( int eHoliday )						{ return TFGameRules()->IsHolidayActive( eHoliday ); }
bool	ScriptPointsMayBeCaptured()									{ return TFGameRules()->PointsMayBeCaptured(); }
int		ScriptGetClassLimit( int iClass )							{ return TFGameRules()->GetClassLimit( iClass ); }
bool	ScriptFlagsMayBeCapped()									{ return TFGameRules()->FlagsMayBeCapped(); }
int		ScriptGetStopWatchState()									{ return TFGameRules()->GetStopWatchState(); }
bool	ScriptIsInArenaMode()										{ return TFGameRules()->IsInArenaMode(); }
bool	ScriptIsInKothMode()										{ return TFGameRules()->IsInKothMode(); }
bool	ScriptIsMatchTypeCasual()									{ return TFGameRules()->IsMatchTypeCasual(); }
bool	ScriptIsMatchTypeCompetitive()								{ return TFGameRules()->IsMatchTypeCompetitive(); }
bool	ScriptInMatchStartCountdown()								{ return TFGameRules()->InMatchStartCountdown(); }
bool	ScriptMatchmakingShouldUseStopwatchMode()					{ return TFGameRules()->MatchmakingShouldUseStopwatchMode(); }
bool	ScriptIsAttackDefenseMode()									{ return TFGameRules()->IsAttackDefenseMode(); }
bool	ScriptUsePlayerReadyStatusMode()							{ return TFGameRules()->UsePlayerReadyStatusMode(); }
bool	ScriptPlayerReadyStatus_HaveMinPlayersToEnable()			{ return TFGameRules()->PlayerReadyStatus_HaveMinPlayersToEnable(); }
bool	ScriptPlayerReadyStatus_ArePlayersOnTeamReady(int iTeam)	{ return TFGameRules()->PlayerReadyStatus_ArePlayersOnTeamReady( iTeam ); }
void	ScriptPlayerReadyStatus_ResetState()						{ TFGameRules()->PlayerReadyStatus_ResetState(); }
bool	ScriptIsDefaultGameMode()									{ return TFGameRules()->IsDefaultGameMode(); }
bool	ScriptAllowThirdPersonCamera()								{ return TFGameRules()->AllowThirdPersonCamera(); }
void	ScriptSetGravityMultiplier( float flMultiplier )			{ return TFGameRules()->SetGravityMultiplier( flMultiplier ); }
float	ScriptGetGravityMultiplier()								{ return TFGameRules()->GetGravityMultiplier(); }
bool	ScriptMapHasMatchSummaryStage()								{ return TFGameRules()->MapHasMatchSummaryStage(); }
bool	ScriptPlayersAreOnMatchSummaryStage()						{ return TFGameRules()->PlayersAreOnMatchSummaryStage(); }
bool	ScriptHaveStopWatchWinner()									{ return TFGameRules()->HaveStopWatchWinner(); }
void	ScriptSetOvertimeAllowedForCTF( bool bAllowed )				{ TFGameRules()->SetOvertimeAllowedForCTF( bAllowed ); }
bool	ScriptGetOvertimeAllowedForCTF()							{ return TFGameRules()->GetOvertimeAllowedForCTF(); }

void CTFGameRules::RegisterScriptFunctions()
{
	TF_GAMERULES_SCRIPT_FUNC( GetRoundState,							"Get current round state. See Constants.ERoundState" );
	TF_GAMERULES_SCRIPT_FUNC( IsInWaitingForPlayers,					"Are we waiting for some stragglers?" );
	TF_GAMERULES_SCRIPT_FUNC( GetWinningTeam,							"Who won!" );
	TF_GAMERULES_SCRIPT_FUNC( InOvertime,								"Currently in overtime?" );

	TF_GAMERULES_SCRIPT_FUNC( IsBirthday,								"Are we in birthday mode?" );
	TF_GAMERULES_SCRIPT_FUNC( IsHolidayActive,							"Is the given holiday active? See Constants.EHoliday" );
	TF_GAMERULES_SCRIPT_FUNC( PointsMayBeCaptured,						"Are points able to be captured?" );
	TF_GAMERULES_SCRIPT_FUNC( GetClassLimit,							"Get class limit for class. See Constants.ETFClass" );
	TF_GAMERULES_SCRIPT_FUNC( FlagsMayBeCapped,							"May a flag be captured?" );
	TF_GAMERULES_SCRIPT_FUNC( GetStopWatchState,						"Get the current stopwatch state. See Constants.EStopwatchState" );
	TF_GAMERULES_SCRIPT_FUNC( IsMatchTypeCasual,						"Playing casual?" );
	TF_GAMERULES_SCRIPT_FUNC( IsMatchTypeCompetitive,					"Playing competitive?" );
	TF_GAMERULES_SCRIPT_FUNC( InMatchStartCountdown,					"Are we in the pre-match state?" );
	TF_GAMERULES_SCRIPT_FUNC( MatchmakingShouldUseStopwatchMode,		"" );
	TF_GAMERULES_SCRIPT_FUNC( IsAttackDefenseMode,						"" );
	TF_GAMERULES_SCRIPT_FUNC( UsePlayerReadyStatusMode,					"" );
	TF_GAMERULES_SCRIPT_FUNC( PlayerReadyStatus_HaveMinPlayersToEnable,	"" );
	TF_GAMERULES_SCRIPT_FUNC( PlayerReadyStatus_ArePlayersOnTeamReady,	"" );
	TF_GAMERULES_SCRIPT_FUNC( PlayerReadyStatus_ResetState,				"" );
	TF_GAMERULES_SCRIPT_FUNC( IsDefaultGameMode,						"The absence of arena, mvm, tournament mode, etc" );
	TF_GAMERULES_SCRIPT_FUNC( AllowThirdPersonCamera,					"" );
	TF_GAMERULES_SCRIPT_FUNC( SetGravityMultiplier,						"" );
	TF_GAMERULES_SCRIPT_FUNC( GetGravityMultiplier,						"" );
	TF_GAMERULES_SCRIPT_FUNC( MapHasMatchSummaryStage,					"" );
	TF_GAMERULES_SCRIPT_FUNC( PlayersAreOnMatchSummaryStage,			"" );
	TF_GAMERULES_SCRIPT_FUNC( HaveStopWatchWinner,						"" );
	TF_GAMERULES_SCRIPT_FUNC( GetOvertimeAllowedForCTF,					"" );
	TF_GAMERULES_SCRIPT_FUNC( SetOvertimeAllowedForCTF,					"" );

	g_pScriptVM->RegisterInstance( &PlayerVoiceListener(), "PlayerVoiceListener" );
}

#endif // GAME_DLL

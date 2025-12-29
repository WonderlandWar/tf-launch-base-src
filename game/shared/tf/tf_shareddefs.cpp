//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_shareddefs.h"
#include "KeyValues.h"
#include "takedamageinfo.h"
#include "tf_gamerules.h"
#include "filesystem.h"

//-----------------------------------------------------------------------------
// Teams.
//-----------------------------------------------------------------------------
const char *g_aTeamNames[TF_TEAM_COUNT] =
{
	"Unassigned",
	"Spectator",
	"Red",
	"Blue"
};

color32 g_aTeamColors[TF_TEAM_COUNT] = 
{
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 255, 0, 0, 0 },
	{ 0, 0, 255, 0 }
};

//-----------------------------------------------------------------------------
// Classes.
//-----------------------------------------------------------------------------

const char *g_aPlayerClassNames[TF_CLASS_MENU_BUTTONS] =
{
	"#TF_Class_Name_Undefined",
	"#TF_Class_Name_Scout",
	"#TF_Class_Name_Sniper",
	"#TF_Class_Name_Soldier",
	"#TF_Class_Name_Demoman",
	"#TF_Class_Name_Medic",
	"#TF_Class_Name_HWGuy",
	"#TF_Class_Name_Pyro",
	"#TF_Class_Name_Spy",
	"#TF_Class_Name_Engineer",
	"#TF_Class_Name_Civilian",
	"",
	"#TF_Random"
};

const char *g_aPlayerClassNames_NonLocalized[TF_CLASS_MENU_BUTTONS] =
{
	"Undefined",
	"Scout",
	"Sniper",
	"Soldier",
	"Demoman",
	"Medic",
	"Heavy",
	"Pyro",
	"Spy",
	"Engineer",
	"Civilian",
	"",
	"Random"
};

const char *g_aRawPlayerClassNamesShort[TF_CLASS_MENU_BUTTONS] =
{
	"undefined",
	"scout",
	"sniper",
	"soldier",
	"demo",	// short
	"medic",
	"heavy",// short
	"pyro",
	"spy",
	"engineer",
	"civilian",
	"",
	"random"
};

const char *g_aRawPlayerClassNames[TF_CLASS_MENU_BUTTONS] =
{
	"undefined",
	"scout",
	"sniper",
	"soldier",
	"demoman",
	"medic",
	"heavyweapons",
	"pyro",
	"spy",
	"engineer",
	"civilian",
	"",
	"random"
};
int GetClassIndexFromString( const char *pClassName, int nLastClassIndex/*=TF_LAST_NORMAL_CLASS*/ )
{
	for ( int i = TF_FIRST_NORMAL_CLASS; i <= nLastClassIndex; ++i )
	{
		// compare first N characters to allow matching both "heavy" and "heavyweapons"
		int classnameLength = V_strlen( g_aPlayerClassNames_NonLocalized[i] );

		if ( V_strlen( pClassName ) < classnameLength )
			continue;

		if ( !V_strnicmp( g_aPlayerClassNames_NonLocalized[i], pClassName, classnameLength ) )
		{
			return i;
		}
	}

	return TF_CLASS_UNDEFINED;
}

int iRemapIndexToClass[TF_CLASS_MENU_BUTTONS] =
{
	0,
		TF_CLASS_SCOUT,
		TF_CLASS_SOLDIER,
		TF_CLASS_PYRO,
		TF_CLASS_DEMOMAN,
		TF_CLASS_HEAVYWEAPONS,
		TF_CLASS_ENGINEER,
		TF_CLASS_MEDIC,
		TF_CLASS_SNIPER,
		TF_CLASS_SPY,
		0,
		0,
		TF_CLASS_RANDOM
};

int GetRemappedMenuIndexForClass( int iClass )
{
	int iIndex = 0;

	for ( int i = 0 ; i < TF_CLASS_MENU_BUTTONS ; i++ )
	{
		if ( iRemapIndexToClass[i] == iClass )
		{
			iIndex = i;
			break;
		}
	}

	return iIndex;
}

ETFCond condition_to_attribute_translation[]  =
{
	TF_COND_BURNING,					// 1 (1<<0)
	TF_COND_AIMING,						// 2 (1<<1)
	TF_COND_ZOOMED,						// 4 (1<<2)
	TF_COND_DISGUISING,					// 8 (...)
	TF_COND_DISGUISED,					// 16
	TF_COND_STEALTHED,					// 32
	TF_COND_INVULNERABLE,				// 64
	TF_COND_TELEPORTED,					// 128
	TF_COND_TAUNTING,					// 256
	TF_COND_INVULNERABLE_WEARINGOFF,	// 512
	TF_COND_STEALTHED_BLINK,			// 1024
	TF_COND_SELECTED_TO_TELEPORT,		// 2048
	TF_COND_HEALTH_BUFF,				// 131072
	TF_COND_HEALTH_OVERHEALED,			// 262144

	TF_COND_LAST				// sentinel value checked against when iterating
};

ETFCond g_aDebuffConditions[] =
{
	TF_COND_BURNING,
	TF_COND_LAST
};

bool ConditionExpiresFast( ETFCond eCond )
{
	return eCond == TF_COND_BURNING;
}

static const char *g_aConditionNames[] =
{
	"TF_COND_AIMING",                           // = 0 - Sniper aiming, Heavy minigun.
	"TF_COND_ZOOMED",                           // = 1
	"TF_COND_DISGUISING",                       // = 2
	"TF_COND_DISGUISED",                        // = 3
	"TF_COND_STEALTHED",                        // = 4 - Spy specific
	"TF_COND_INVULNERABLE",                     // = 5
	"TF_COND_TELEPORTED",                       // = 6
	"TF_COND_TAUNTING",                         // = 7
	"TF_COND_INVULNERABLE_WEARINGOFF",          // = 8
	"TF_COND_STEALTHED_BLINK",                  // = 9
	"TF_COND_SELECTED_TO_TELEPORT",             // = 10
	"TF_COND_HEALTH_BUFF",						// = 11
	"TF_COND_BURNING",                          // = 12
	"TF_COND_HEALTH_OVERHEALED",                // = 13

	//
	// ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//

	// ******** Keep this block last! ********
	// Keep experimental conditions below and graduate out of it before shipping
};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_aConditionNames ) == TF_COND_LAST );

const char *GetTFConditionName( ETFCond eCond )
{
	if ( ( eCond >= ARRAYSIZE( g_aConditionNames ) ) || ( eCond < 0 ) )
		return NULL;

	return g_aConditionNames[eCond];
}


ETFCond GetTFConditionFromName( const char *pszCondName )
{
	for( uint i=0; i<TF_COND_LAST; i++ )
	{ 
		ETFCond eCond = (ETFCond)i;
		if ( !V_stricmp( GetTFConditionName( eCond ), pszCondName ) ) 
			return eCond;
	} 

	Assert( !!"Invalid Condition Name" );
	return TF_COND_INVALID;
}


//-----------------------------------------------------------------------------
// Gametypes.
//-----------------------------------------------------------------------------
static const char *s_aGameTypeNames[] =
{
	"Undefined",
	"#Gametype_CTF",
	"#Gametype_CP",
};
COMPILE_TIME_ASSERT( TF_GAMETYPE_COUNT == ARRAYSIZE( s_aGameTypeNames ) );

const char *GetGameTypeName( ETFGameType gameType )
{
	return s_aGameTypeNames[ gameType ];
}

static const char *s_aEnumGameTypeName[] =
{
	"TF_GAMETYPE_UNDEFINED",
	"TF_GAMETYPE_CTF",
	"TF_GAMETYPE_CP",
};
COMPILE_TIME_ASSERT( TF_GAMETYPE_COUNT == ARRAYSIZE( s_aEnumGameTypeName ) );

const char *GetEnumGameTypeName( ETFGameType gameType )
{
	return s_aEnumGameTypeName[ gameType ];
}

ETFGameType GetGameTypeFromName( const char *pszGameType )
{
	for ( int i=0; i<TF_GAMETYPE_COUNT; ++i )
	{
		if ( FStrEq( pszGameType, s_aEnumGameTypeName[i] ) )
			return ETFGameType(i);
	}

	return TF_GAMETYPE_UNDEFINED;
}

//-----------------------------------------------------------------------------
// Ammo.
//-----------------------------------------------------------------------------
const char *g_aAmmoNames[] =
{
	"DUMMY AMMO",
	"TF_AMMO_PRIMARY",
	"TF_AMMO_SECONDARY",
	"TF_AMMO_METAL",
	"TF_AMMO_GRENADES1",
	"TF_AMMO_GRENADES2"
};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_aAmmoNames ) == TF_AMMO_COUNT );

const char *GetAmmoName( int iAmmoType )
{
	ETFAmmoType eAmmoType = (ETFAmmoType)iAmmoType;
	return g_aAmmoNames[ eAmmoType ];
}

const char *g_aCTFEventNames[] =
{
	"",
	"TF_FLAGEVENT_PICKUP",
	"TF_FLAGEVENT_CAPTURE",
	"TF_FLAGEVENT_DEFEND",
	"TF_FLAGEVENT_DROPPED",
	"TF_FLAGEVENT_RETURNED",
};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_aCTFEventNames ) == TF_NUM_FLAG_EVENTS );

const char *GetCTFEventName( ETFFlagEventTypes iEventType )
{
	return g_aCTFEventNames[ iEventType ];
}

ETFFlagEventTypes GetCTFEventTypeFromName( const char *pszName )
{
	for( int i=TF_FLAGEVENT_PICKUP; i < TF_NUM_FLAG_EVENTS; ++i )
	{
		if ( FStrEq( pszName, GetCTFEventName( (ETFFlagEventTypes)i ) ) )
		{
			return (ETFFlagEventTypes)i;
		}
	}
	
	Assert( false );
	return TF_NUM_FLAG_EVENTS;
}

//-----------------------------------------------------------------------------
// Weapons.
//-----------------------------------------------------------------------------
const char *g_aWeaponNames[] =
{
	"TF_WEAPON_NONE",
	"TF_WEAPON_BAT",
	"TF_WEAPON_BOTTLE", 
	"TF_WEAPON_FIREAXE",
	"TF_WEAPON_CLUB",
	"TF_WEAPON_CROWBAR",
	"TF_WEAPON_KNIFE",
	"TF_WEAPON_FISTS",
	"TF_WEAPON_SHOVEL",
	"TF_WEAPON_WRENCH",
	"TF_WEAPON_BONESAW",
	"TF_WEAPON_SHOTGUN_PRIMARY",
	"TF_WEAPON_SHOTGUN_SOLDIER",
	"TF_WEAPON_SHOTGUN_HWG",
	"TF_WEAPON_SHOTGUN_PYRO",
	"TF_WEAPON_SCATTERGUN",
	"TF_WEAPON_SNIPERRIFLE",
	"TF_WEAPON_MINIGUN",
	"TF_WEAPON_SMG",
	"TF_WEAPON_SYRINGEGUN_MEDIC",
	"TF_WEAPON_TRANQ",
	"TF_WEAPON_ROCKETLAUNCHER",
	"TF_WEAPON_GRENADELAUNCHER",
	"TF_WEAPON_PIPEBOMBLAUNCHER",
	"TF_WEAPON_FLAMETHROWER",
	"TF_WEAPON_GRENADE_NORMAL",
	"TF_WEAPON_GRENADE_CONCUSSION",
	"TF_WEAPON_GRENADE_NAIL",
	"TF_WEAPON_GRENADE_MIRV",
	"TF_WEAPON_GRENADE_MIRV_DEMOMAN",
	"TF_WEAPON_GRENADE_NAPALM",
	"TF_WEAPON_GRENADE_GAS",
	"TF_WEAPON_GRENADE_EMP",
	"TF_WEAPON_GRENADE_CALTROP",
	"TF_WEAPON_GRENADE_PIPEBOMB",
	"TF_WEAPON_GRENADE_SMOKE_BOMB",
	"TF_WEAPON_GRENADE_HEAL",
	"TF_WEAPON_PISTOL",
	"TF_WEAPON_PISTOL_SCOUT",
	"TF_WEAPON_REVOLVER",
	"TF_WEAPON_NAILGUN",
	"TF_WEAPON_PDA",
	"TF_WEAPON_PDA_ENGINEER_BUILD",
	"TF_WEAPON_PDA_ENGINEER_DESTROY",
	"TF_WEAPON_PDA_SPY",
	"TF_WEAPON_BUILDER",
	"TF_WEAPON_MEDIGUN",
	"TF_WEAPON_GRENADE_MIRVBOMB",
	"TF_WEAPON_FLAMETHROWER_ROCKET",
	"TF_WEAPON_GRENADE_DEMOMAN",
	"TF_WEAPON_SENTRY_BULLET",
	"TF_WEAPON_SENTRY_ROCKET",
	"TF_WEAPON_DISPENSER",
	"TF_WEAPON_INVIS",

};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_aWeaponNames ) == TF_WEAPON_COUNT );

int g_aWeaponDamageTypes[] =
{
	DMG_GENERIC,	// TF_WEAPON_NONE
	DMG_CLUB,		// TF_WEAPON_BAT,
	DMG_CLUB,		// TF_WEAPON_BOTTLE, 
	DMG_CLUB,		// TF_WEAPON_FIREAXE,
	DMG_CLUB,		// TF_WEAPON_CLUB,
	DMG_CLUB,		// TF_WEAPON_CROWBAR,
	DMG_SLASH,		// TF_WEAPON_KNIFE,
	DMG_CLUB,		// TF_WEAPON_FISTS,
	DMG_CLUB,		// TF_WEAPON_SHOVEL,
	DMG_CLUB,		// TF_WEAPON_WRENCH,
	DMG_SLASH,		// TF_WEAPON_BONESAW,
	DMG_BUCKSHOT | DMG_USEDISTANCEMOD,	// TF_WEAPON_SHOTGUN_PRIMARY,
	DMG_BUCKSHOT | DMG_USEDISTANCEMOD,	// TF_WEAPON_SHOTGUN_SOLDIER,
	DMG_BUCKSHOT | DMG_USEDISTANCEMOD,	// TF_WEAPON_SHOTGUN_HWG,
	DMG_BUCKSHOT | DMG_USEDISTANCEMOD,	// TF_WEAPON_SHOTGUN_PYRO,
	DMG_BUCKSHOT | DMG_USEDISTANCEMOD,  // TF_WEAPON_SCATTERGUN,
	DMG_BULLET | DMG_USE_HITLOCATIONS,	// TF_WEAPON_SNIPERRIFLE,
	DMG_BULLET | DMG_USEDISTANCEMOD,		// TF_WEAPON_MINIGUN,
	DMG_BULLET | DMG_USEDISTANCEMOD,		// TF_WEAPON_SMG,
	DMG_BULLET | DMG_USEDISTANCEMOD | DMG_NOCLOSEDISTANCEMOD | DMG_PREVENT_PHYSICS_FORCE,		// TF_WEAPON_SYRINGEGUN_MEDIC,
	DMG_BULLET | DMG_USEDISTANCEMOD | DMG_PREVENT_PHYSICS_FORCE | DMG_PARALYZE,		// TF_WEAPON_TRANQ,
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_USEDISTANCEMOD,		// TF_WEAPON_ROCKETLAUNCHER,
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_USEDISTANCEMOD,		// TF_WEAPON_GRENADELAUNCHER,
	DMG_BLAST | DMG_HALF_FALLOFF,		// TF_WEAPON_PIPEBOMBLAUNCHER,
	DMG_IGNITE | DMG_PREVENT_PHYSICS_FORCE | DMG_PREVENT_PHYSICS_FORCE,		// TF_WEAPON_FLAMETHROWER,
	DMG_BLAST | DMG_HALF_FALLOFF,		// TF_WEAPON_GRENADE_NORMAL,
	DMG_SONIC | DMG_HALF_FALLOFF,		// TF_WEAPON_GRENADE_CONCUSSION,
	DMG_BULLET | DMG_HALF_FALLOFF,		// TF_WEAPON_GRENADE_NAIL,
	DMG_BLAST | DMG_HALF_FALLOFF,		// TF_WEAPON_GRENADE_MIRV,
	DMG_BLAST | DMG_HALF_FALLOFF,		// TF_WEAPON_GRENADE_MIRV_DEMOMAN,
	DMG_BURN | DMG_RADIUS_MAX,		// TF_WEAPON_GRENADE_NAPALM,
	DMG_POISON | DMG_HALF_FALLOFF,		// TF_WEAPON_GRENADE_GAS,
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_PREVENT_PHYSICS_FORCE,		// TF_WEAPON_GRENADE_EMP,
	DMG_GENERIC,	// TF_WEAPON_GRENADE_CALTROP,
	DMG_BLAST | DMG_HALF_FALLOFF,		// TF_WEAPON_GRENADE_PIPEBOMB,
	DMG_GENERIC,	// TF_WEAPON_GRENADE_SMOKE_BOMB,
	DMG_GENERIC,	// TF_WEAPON_GRENADE_HEAL
	DMG_BULLET | DMG_USEDISTANCEMOD,		// TF_WEAPON_PISTOL,
	DMG_BULLET | DMG_USEDISTANCEMOD,		// TF_WEAPON_PISTOL_SCOUT,
	DMG_BULLET | DMG_USEDISTANCEMOD,		// TF_WEAPON_REVOLVER,
	DMG_BULLET | DMG_USEDISTANCEMOD | DMG_NOCLOSEDISTANCEMOD | DMG_PREVENT_PHYSICS_FORCE,		// TF_WEAPON_NAILGUN,
	DMG_BULLET,		// TF_WEAPON_PDA,
	DMG_BULLET,		// TF_WEAPON_PDA_ENGINEER_BUILD,
	DMG_BULLET,		// TF_WEAPON_PDA_ENGINEER_DESTROY,
	DMG_BULLET,		// TF_WEAPON_PDA_SPY,
	DMG_BULLET,		// TF_WEAPON_BUILDER
	DMG_BULLET,		// TF_WEAPON_MEDIGUN
	DMG_BLAST,		// TF_WEAPON_GRENADE_MIRVBOMB
	DMG_BLAST | DMG_IGNITE | DMG_RADIUS_MAX,		// TF_WEAPON_FLAMETHROWER_ROCKET
	DMG_BLAST | DMG_HALF_FALLOFF,					// TF_WEAPON_GRENADE_DEMOMAN
	DMG_BULLET,	// TF_WEAPON_SENTRY_BULLET
	DMG_BLAST,	// TF_WEAPON_SENTRY_ROCKET
	DMG_GENERIC,	// TF_WEAPON_DISPENSER
	DMG_GENERIC,	// TF_WEAPON_INVIS

};

const char *g_szSpecialDamageNames[] =
{
	"",
	"TF_DMG_CUSTOM_HEADSHOT",
	"TF_DMG_CUSTOM_BACKSTAB",
	"TF_DMG_CUSTOM_BURNING",
	"TF_DMG_WRENCH_FIX",
	"TF_DMG_CUSTOM_MINIGUN",
	"TF_DMG_CUSTOM_SUICIDE",
};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_szSpecialDamageNames ) == TF_DMG_CUSTOM_END );

const char *GetCustomDamageName( ETFDmgCustom eDmgCustom )
{
	if ( ( eDmgCustom >= ARRAYSIZE( g_szSpecialDamageNames ) ) || ( eDmgCustom < 0 ) )
		return NULL;

	return g_szSpecialDamageNames[eDmgCustom];
}

ETFDmgCustom GetCustomDamageFromName( const char *pszCustomDmgName )
{
	for( uint i=0; i<TF_DMG_CUSTOM_END; i++ )
	{ 
		ETFDmgCustom eDmgCustom = (ETFDmgCustom)i;
		if ( !V_stricmp( GetCustomDamageName( eDmgCustom ), pszCustomDmgName ) ) 
			return eDmgCustom;
	} 

	Assert( !!"Invalid Custom Damage Name" );
	return TF_DMG_CUSTOM_NONE;
}

const char *g_szProjectileNames[] =
{
	"",
	"projectile_bullet",
	"projectile_rocket",
	"projectile_pipe",
	"projectile_pipe_remote",
	"projectile_syringe",
	"projectile_sentry_rocket",

};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_szProjectileNames ) == TF_NUM_PROJECTILES );

// these map to the projectiles named in g_szProjectileNames
int g_iProjectileWeapons[] = 
{
	TF_WEAPON_NONE,
	TF_WEAPON_PISTOL,
	TF_WEAPON_ROCKETLAUNCHER,
	TF_WEAPON_PIPEBOMBLAUNCHER,
	TF_WEAPON_GRENADELAUNCHER,
	TF_WEAPON_SYRINGEGUN_MEDIC,
	TF_WEAPON_SENTRY_ROCKET,
};

COMPILE_TIME_ASSERT( ARRAYSIZE( g_szProjectileNames ) == ARRAYSIZE( g_iProjectileWeapons ) );

const char *g_pszHintMessages[] =
{
	"#Hint_spotted_a_friend",
	"#Hint_spotted_an_enemy",
	"#Hint_killing_enemies_is_good",
	"#Hint_out_of_ammo",
	"#Hint_turn_off_hints",
	"#Hint_pickup_ammo",
	"#Hint_Cannot_Teleport_With_Flag",
	"#Hint_Cannot_Cloak_With_Flag",
	"#Hint_Cannot_Disguise_With_Flag",
	"#Hint_Cannot_Attack_While_Feign_Armed",
	"#Hint_ClassMenu",

// Grenades
	"#Hint_gren_caltrops",
	"#Hint_gren_concussion",
	"#Hint_gren_emp",
	"#Hint_gren_gas",
	"#Hint_gren_mirv",
	"#Hint_gren_nail",
	"#Hint_gren_napalm",
	"#Hint_gren_normal",

// Altfires
	"#Hint_altfire_sniperrifle",
	"#Hint_altfire_flamethrower",
	"#Hint_altfire_grenadelauncher",
	"#Hint_altfire_pipebomblauncher",
	"#Hint_altfire_rotate_building",

// Soldier
	"#Hint_Soldier_rpg_reload",

// Engineer
	"#Hint_Engineer_use_wrench_onown",
	"#Hint_Engineer_use_wrench_onother",
	"#Hint_Engineer_use_wrench_onfriend",
	"#Hint_Engineer_build_sentrygun",
	"#Hint_Engineer_build_dispenser",
	"#Hint_Engineer_build_teleporters",
	"#Hint_Engineer_pickup_metal",
	"#Hint_Engineer_repair_object",
	"#Hint_Engineer_metal_to_upgrade",
	"#Hint_Engineer_upgrade_sentrygun",

	"#Hint_object_has_sapper",

	"#Hint_object_your_object_sapped",
	"#Hint_enemy_using_dispenser",
	"#Hint_enemy_using_tp_entrance",
	"#Hint_enemy_using_tp_exit",

	"#Hint_Cannot_Phase_With_Flag",

	"#Hint_Cannot_Attack_While_Cloaked",

	"#Hint_Cannot_Arm_Feign_Now",
};

const char *GetWeaponIDName( int iWeaponID )
{
	return ClampedArrayElement( g_aWeaponNames, iWeaponID );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int GetWeaponId( const char *pszWeaponName )
{
	// if this doesn't match, you need to add missing weapons to the array
	COMPILE_TIME_ASSERT( TF_WEAPON_COUNT == ARRAYSIZE( g_aWeaponNames ) );

	for ( int iWeapon = 0; iWeapon < ARRAYSIZE( g_aWeaponNames ); ++iWeapon )
	{
		if ( !Q_stricmp( pszWeaponName, g_aWeaponNames[iWeapon] ) )
			return iWeapon;
	}

	return TF_WEAPON_NONE;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *WeaponIdToAlias( int iWeapon )
{
	// if this doesn't match, you need to add missing weapons to the array
	COMPILE_TIME_ASSERT( TF_WEAPON_COUNT == ARRAYSIZE( g_aWeaponNames ) );

	if ( ( iWeapon >= ARRAYSIZE( g_aWeaponNames ) ) || ( iWeapon < 0 ) )
		return NULL;

	return g_aWeaponNames[iWeapon];
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int GetWeaponFromDamage( const CTakeDamageInfo &info )
{
	int iWeapon = TF_WEAPON_NONE;

	const char *killer_weapon_name = "";

	// Find the killer & the scorer
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBasePlayer *pScorer = TFGameRules()->GetDeathScorer( pKiller, pInflictor, NULL );

	// find the weapon the killer used

	if ( pScorer )	// Is the killer a client?
	{
		if ( pInflictor )
		{
			if ( pInflictor == pScorer )
			{
				// If the inflictor is the killer,  then it must be their current weapon doing the damage
				if ( pScorer->GetActiveWeapon() )
				{
					killer_weapon_name = pScorer->GetActiveWeapon()->GetClassname();
				}
			}
			else
			{
				killer_weapon_name = STRING( pInflictor->m_iClassname );  // it's just that easy
			}
		}
	}
	else if ( pInflictor )
	{
		killer_weapon_name = STRING( pInflictor->m_iClassname );
	}

	if ( !Q_strnicmp( killer_weapon_name, "tf_projectile", 13 ) )
	{
		for( int i = 0; i < ARRAYSIZE( g_szProjectileNames ); i++ )
		{
			if ( !Q_stricmp( &killer_weapon_name[ 3 ], g_szProjectileNames[ i ] ) )
			{
				iWeapon = g_iProjectileWeapons[ i ];
				break;
			}
		}
	}
	else
	{
		int iLen = Q_strlen( killer_weapon_name );

		// strip off _projectile from projectiles shot from other projectiles
		if ( ( iLen < 256 ) && ( iLen > 11 ) && !Q_stricmp( &killer_weapon_name[ iLen - 11 ], "_projectile" ) )
		{
			char temp[ 256 ];
			V_strcpy_safe( temp, killer_weapon_name );
			temp[ iLen - 11 ] = 0;

			// set the weapon used
			iWeapon = GetWeaponId( temp );
		}
		else
		{
			// set the weapon used
			iWeapon = GetWeaponId( killer_weapon_name );
		}
	}

	AssertMsg( iWeapon >= 0 && iWeapon < TF_WEAPON_COUNT, "Referencing a weapon not in tf_shareddefs.h.  Check to make it's defined and it's mapped correctly in g_szProjectileNames and g_iProjectileWeapons." );
	return iWeapon;
}

#endif

// ------------------------------------------------------------------------------------------------ //
// CObjectInfo tables.
// ------------------------------------------------------------------------------------------------ //

CObjectInfo::CObjectInfo( const char *pObjectName )
{
	m_pObjectName = pObjectName;
	m_pClassName = NULL;
	m_flBuildTime = -9999;
	m_nMaxObjects = -9999;
	m_Cost = -9999;
	m_CostMultiplierPerInstance = -999;
	m_flUpgradeDuration = -999;
	m_UpgradeCost = -9999;
	m_MaxUpgradeLevel = -9999;
	m_pBuilderWeaponName = NULL;
	m_pBuilderPlacementString = NULL;
	m_SelectionSlot = -9999;
	m_SelectionPosition = -9999;
	m_bSolidToPlayerMovement = false;
	m_pIconActive = NULL;
	m_pIconInactive = NULL;
	m_pIconMenu = NULL;
	m_pViewModel = NULL;
	m_pPlayerModel = NULL;
	m_iDisplayPriority = 0;
	m_bVisibleInWeaponSelection = true;
	m_pExplodeSound = NULL;
	m_pUpgradeSound = NULL;
	m_pExplosionParticleEffect = NULL;
	m_bAutoSwitchTo = false;
	m_iBuildCount = 0;
	m_iNumAltModes = 0;
	m_bRequiresOwnBuilder = false;
}


CObjectInfo::~CObjectInfo()
{
	delete [] m_pClassName;
	delete [] m_pStatusName;
	delete [] m_pBuilderWeaponName;
	delete [] m_pBuilderPlacementString;
	delete [] m_pIconActive;
	delete [] m_pIconInactive;
	delete [] m_pIconMenu;
	delete [] m_pViewModel;
	delete [] m_pPlayerModel;
	delete [] m_pExplodeSound;
	delete [] m_pUpgradeSound;
	delete [] m_pExplosionParticleEffect;
}

CObjectInfo g_ObjectInfos[OBJ_LAST] =
{
	CObjectInfo( "OBJ_DISPENSER" ),
	CObjectInfo( "OBJ_TELEPORTER" ),
	CObjectInfo( "OBJ_SENTRYGUN" ),
	CObjectInfo( "OBJ_ATTACHMENT_SAPPER" ),
};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_ObjectInfos ) == OBJ_LAST );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int GetBuildableId( const char *pszBuildableName )
{
	for ( int iBuildable = 0; iBuildable < OBJ_LAST; ++iBuildable )
	{
		if ( !Q_stricmp( pszBuildableName, g_ObjectInfos[iBuildable].m_pObjectName ) )
			return iBuildable;
	}

	return OBJ_LAST;
}

bool AreObjectInfosLoaded()
{
	return g_ObjectInfos[0].m_pClassName != NULL;
}

static void SpewFileInfo( IBaseFileSystem *pFileSystem, const char *resourceName, const char *pathID, KeyValues *pValues )
{
	bool bFileExists = pFileSystem->FileExists( resourceName, pathID );
	bool bFileWritable = pFileSystem->IsFileWritable( resourceName, pathID );
	unsigned int nSize = pFileSystem->Size( resourceName, pathID );

	Msg( "resourceName:%s pathID:%s bFileExists:%d size:%u writeable:%d\n", resourceName, pathID, bFileExists, nSize, bFileWritable );

	unsigned int filesize = ( unsigned int )-1;
	FileHandle_t f = filesystem->Open( resourceName, "rb", pathID );
	if ( f )
	{
		filesize = filesystem->Size( f );
		filesystem->Close( f );
	}

	Msg( " FileHandle_t:%p size:%u\n", f, filesize );

	IFileSystem *pFS = 	(IFileSystem *)filesystem;

	char cwd[ MAX_PATH ];
	cwd[ 0 ] = 0;
	pFS->GetCurrentDirectory( cwd, ARRAYSIZE( cwd ) );

	bool bAvailable = pFS->IsFileImmediatelyAvailable( resourceName );

	Msg( " IsFileImmediatelyAvailable:%d cwd:%s\n", bAvailable, cwd );

	pFS->PrintSearchPaths();

	if ( pValues )
	{
		Msg( "Keys:" );
		KeyValuesDumpAsDevMsg( pValues, 2, 0 );
	}
}

void LoadObjectInfos( IBaseFileSystem *pFileSystem )
{
	const char *pFilename = "scripts/objects.txt";

	// Make sure this stuff hasn't already been loaded.
	Assert( !AreObjectInfosLoaded() );

	KeyValues *pValues = new KeyValues( "Object descriptions" );
	if ( !pValues->LoadFromFile( pFileSystem, pFilename, "GAME" ) )
	{
		// Getting "Can't open scripts/objects.txt for object info." errors. Spew file information
		//  before the Error() call which should show up in the minidumps.
		SpewFileInfo( pFileSystem, pFilename, "GAME", NULL );

		Error( "Can't open %s for object info.", pFilename );
		pValues->deleteThis();
		return;
	}

	// Now read each class's information in.
	for ( int iObj=0; iObj < ARRAYSIZE( g_ObjectInfos ); iObj++ )
	{
		CObjectInfo *pInfo = &g_ObjectInfos[iObj];
		KeyValues *pSub = pValues->FindKey( pInfo->m_pObjectName );
		if ( !pSub )
		{
			// Getting "Missing section 'OBJ_DISPENSER' from scripts/objects.txt" errors.
			SpewFileInfo( pFileSystem, pFilename, "GAME", pValues );

			// It seems that folks have corrupt files when these errors are seen in http://minidump.
			// Does it make sense to call the below Steam API so it'll force a validation next startup time?
			// Need to verify it's real corruption and not someone dorking around with their objects.txt file...
			//
			// From Martin Otten: If you have a file on disc and you’re 100% sure it’s
			//  corrupt, call ISteamApps::MarkContentCorrupt( false ), before you shutdown
			//  the game. This will cause a content validation in Steam.

			Error( "Missing section '%s' from %s.", pInfo->m_pObjectName, pFilename );
			pValues->deleteThis();
			return;
		}

		// Read all the info in.
		if ( (pInfo->m_flBuildTime = pSub->GetFloat( "BuildTime", -999 )) == -999 ||
			(pInfo->m_nMaxObjects = pSub->GetInt( "MaxObjects", -999 )) == -999 ||
			(pInfo->m_Cost = pSub->GetInt( "Cost", -999 )) == -999 ||
			(pInfo->m_CostMultiplierPerInstance = pSub->GetFloat( "CostMultiplier", -999 )) == -999 ||
			(pInfo->m_flUpgradeDuration = pSub->GetFloat( "UpgradeDuration", -999 )) == -999 ||
			(pInfo->m_UpgradeCost = pSub->GetInt( "UpgradeCost", -999 )) == -999 ||
			(pInfo->m_MaxUpgradeLevel = pSub->GetInt( "MaxUpgradeLevel", -999 )) == -999 ||
			(pInfo->m_SelectionSlot = pSub->GetInt( "SelectionSlot", -999 )) == -999 ||
			(pInfo->m_iBuildCount = pSub->GetInt( "BuildCount", -999 )) == -999 ||
			(pInfo->m_SelectionPosition = pSub->GetInt( "SelectionPosition", -999 )) == -999 )
		{
			SpewFileInfo( pFileSystem, pFilename, "GAME", pValues );

			Error( "Missing data for object '%s' in %s.", pInfo->m_pObjectName, pFilename );
			pValues->deleteThis();
			return;
		}

		pInfo->m_pClassName = ReadAndAllocStringValue( pSub, "ClassName", pFilename );
		pInfo->m_pStatusName = ReadAndAllocStringValue( pSub, "StatusName", pFilename );
		pInfo->m_pBuilderWeaponName = ReadAndAllocStringValue( pSub, "BuilderWeaponName", pFilename );
		pInfo->m_pBuilderPlacementString = ReadAndAllocStringValue( pSub, "BuilderPlacementString", pFilename );
		pInfo->m_bSolidToPlayerMovement = pSub->GetInt( "SolidToPlayerMovement", 0 ) ? true : false;
		pInfo->m_pIconActive = ReadAndAllocStringValue( pSub, "IconActive", pFilename );
		pInfo->m_pIconInactive = ReadAndAllocStringValue( pSub, "IconInactive", pFilename );
		pInfo->m_pIconMenu = ReadAndAllocStringValue( pSub, "IconMenu", pFilename );
		pInfo->m_pViewModel = ReadAndAllocStringValue( pSub, "Viewmodel", pFilename );
		pInfo->m_pPlayerModel = ReadAndAllocStringValue( pSub, "Playermodel", pFilename );
		pInfo->m_iDisplayPriority = pSub->GetInt( "DisplayPriority", 0 );
		pInfo->m_pHudStatusIcon = ReadAndAllocStringValue( pSub, "HudStatusIcon", pFilename );
		pInfo->m_bVisibleInWeaponSelection = ( pSub->GetInt( "VisibleInWeaponSelection", 1 ) > 0 );
		pInfo->m_pExplodeSound = ReadAndAllocStringValue( pSub, "ExplodeSound", pFilename );
		pInfo->m_pUpgradeSound = ReadAndAllocStringValue( pSub, "UpgradeSound", pFilename );
		pInfo->m_pExplosionParticleEffect = ReadAndAllocStringValue( pSub, "ExplodeEffect", pFilename );
		pInfo->m_bAutoSwitchTo = ( pSub->GetInt( "autoswitchto", 0 ) > 0 );
		pInfo->m_iMetalToDropInGibs = pSub->GetInt( "MetalToDropInGibs", 0 );
		pInfo->m_bRequiresOwnBuilder = pSub->GetBool( "RequiresOwnBuilder", 0 );

		// Read the other alternate object modes.
		KeyValues *pAltModesKey = pSub->FindKey( "AltModes" );
		if ( pAltModesKey )
		{
			int iIndex = 0;
			while ( iIndex<OBJECT_MAX_MODES )
			{
				char buf[256];
				Q_snprintf( buf, sizeof(buf), "AltMode%d", iIndex );
				KeyValues *pCurrentModeKey = pAltModesKey->FindKey( buf );
				if ( !pCurrentModeKey )
					break;

				pInfo->m_AltModes[iIndex].pszStatusName = ReadAndAllocStringValue( pCurrentModeKey, "StatusName", pFilename );
				pInfo->m_AltModes[iIndex].pszModeName   = ReadAndAllocStringValue( pCurrentModeKey, "ModeName",   pFilename );
				pInfo->m_AltModes[iIndex].pszIconMenu   = ReadAndAllocStringValue( pCurrentModeKey, "IconMenu",   pFilename );

				iIndex++;
			}
			pInfo->m_iNumAltModes = iIndex-1;
		}

		// Alternate mode 0 always matches the defaults.
		pInfo->m_AltModes[0].pszStatusName = pInfo->m_pStatusName;
		pInfo->m_AltModes[0].pszIconMenu   = pInfo->m_pIconMenu;
	}

	pValues->deleteThis();
}


const CObjectInfo* GetObjectInfo( int iObject )
{
	Assert( iObject >= 0 && iObject < OBJ_LAST );
	Assert( AreObjectInfosLoaded() );
	return &g_ObjectInfos[iObject];
}

ConVar tf_cheapobjects( "tf_cheapobjects","0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Set to 1 and all objects will cost 0" );

//-----------------------------------------------------------------------------
// Purpose: Return the cost of another object of the specified type
//			If bLast is set, return the cost of the last built object of the specified type
// 
// Note: Used to contain logic from tf2 that multiple instances of the same object
//       cost different amounts. See tf2/game_shared/tf_shareddefs.cpp for details
//-----------------------------------------------------------------------------
int InternalCalculateObjectCost( int iObjectType )
{
	if ( tf_cheapobjects.GetInt() )
	{
		return 0;
	}

	int iCost = GetObjectInfo( iObjectType )->m_Cost;

	return iCost;
}

//-----------------------------------------------------------------------------
// Purpose: Calculate the cost to upgrade an object of a specific type
//-----------------------------------------------------------------------------
int	CalculateObjectUpgrade( int iObjectType, int iObjectLevel )
{
	// Max level?
	if ( iObjectLevel >= GetObjectInfo( iObjectType )->m_MaxUpgradeLevel )
		return 0;

	int iCost = GetObjectInfo( iObjectType )->m_UpgradeCost;
	for ( int i = 0; i < (iObjectLevel - 1); i++ )
	{
		iCost *= OBJECT_UPGRADE_COST_MULTIPLIER_PER_LEVEL;
	}

	return iCost;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the specified class is allowed to build the specified object type
//-----------------------------------------------------------------------------
bool ClassCanBuild( int iClass, int iObjectType )
{
	/*
	for ( int i = 0; i < OBJ_LAST; i++ )
	{
		// Hit the end?
		if ( g_TFClassInfos[iClass].m_pClassObjects[i] == OBJ_LAST )
			return false;

		// Found it?
		if ( g_TFClassInfos[iClass].m_pClassObjects[i] == iObjectType )
			return true;
	}

	return false;
	*/

	return ( iClass == TF_CLASS_ENGINEER );
}

const unsigned char *GetTFEncryptionKey( void )
{ 
	return (unsigned char *)"E2NcUkG2"; 
}

//-----------------------------------------------------------------------------
// Per-class weapon entity translations
//-----------------------------------------------------------------------------
struct wpntranslation_class_weapons_t
{
	const char *pszWpnString;
	const char *pszClassWpn[TF_LAST_NORMAL_CLASS];
};

wpntranslation_class_weapons_t pszWpnEntTranslationList[] = 
{
	{
		"tf_weapon_shotgun",
		{
			"",							// TF_CLASS_UNDEFINED = 0,
			"",							// TF_CLASS_SCOUT,
			"",							// TF_CLASS_SNIPER,
			"tf_weapon_shotgun_soldier",// TF_CLASS_SOLDIER,
			"",							// TF_CLASS_DEMOMAN,
			"",							// TF_CLASS_MEDIC,
			"tf_weapon_shotgun_hwg",	// TF_CLASS_HEAVYWEAPONS,
			"tf_weapon_shotgun_pyro",	// TF_CLASS_PYRO,
			"",							// TF_CLASS_SPY,
			"tf_weapon_shotgun_primary",// TF_CLASS_ENGINEER,		
		}
	},

	{
		"tf_weapon_pistol",
		{
			"",							// TF_CLASS_UNDEFINED = 0,
			"tf_weapon_pistol_scout",	// TF_CLASS_SCOUT,
			"",							// TF_CLASS_SNIPER,
			"",							// TF_CLASS_SOLDIER,
			"",							// TF_CLASS_DEMOMAN,
			"",							// TF_CLASS_MEDIC,
			"",							// TF_CLASS_HEAVYWEAPONS,
			"",							// TF_CLASS_PYRO,
			"",							// TF_CLASS_SPY,
			"tf_weapon_pistol",			// TF_CLASS_ENGINEER,		
		}
	},

	{
		"tf_weapon_shovel",
		{
			"",							// TF_CLASS_UNDEFINED = 0,
			"",							// TF_CLASS_SCOUT,
			"",							// TF_CLASS_SNIPER,
			"tf_weapon_shovel",			// TF_CLASS_SOLDIER,
			"tf_weapon_bottle",			// TF_CLASS_DEMOMAN,
			"",							// TF_CLASS_MEDIC,
			"",							// TF_CLASS_HEAVYWEAPONS,
			"",							// TF_CLASS_PYRO,
			"",							// TF_CLASS_SPY,
			"",							// TF_CLASS_ENGINEER,		
		}
	},
	{
		"tf_weapon_bottle",
		{
			"",							// TF_CLASS_UNDEFINED = 0,
			"",							// TF_CLASS_SCOUT,
			"",							// TF_CLASS_SNIPER,
			"tf_weapon_shovel",			// TF_CLASS_SOLDIER,
			"tf_weapon_bottle",			// TF_CLASS_DEMOMAN,
			"",							// TF_CLASS_MEDIC,
			"",							// TF_CLASS_HEAVYWEAPONS,
			"",							// TF_CLASS_PYRO,
			"",							// TF_CLASS_SPY,
			"",							// TF_CLASS_ENGINEER,		
		}
	},
	{
		"saxxy",
		{
			"",							// TF_CLASS_UNDEFINED = 0,
			"tf_weapon_bat",			// TF_CLASS_SCOUT,
			"tf_weapon_club",			// TF_CLASS_SNIPER,
			"tf_weapon_shovel",			// TF_CLASS_SOLDIER,
			"tf_weapon_bottle",			// TF_CLASS_DEMOMAN,
			"tf_weapon_bonesaw",		// TF_CLASS_MEDIC,
			"tf_weapon_fireaxe",		// TF_CLASS_HEAVYWEAPONS,		HWG uses a fireaxe because he doesn't have a default melee weapon of his own; also I am a terrible person
			"tf_weapon_fireaxe",		// TF_CLASS_PYRO,
			"tf_weapon_knife",			// TF_CLASS_SPY,
			"tf_weapon_wrench",			// TF_CLASS_ENGINEER,		
		}
	},
	{
		"tf_weapon_throwable",
		{
			"",											// TF_CLASS_UNDEFINED = 0,
			"tf_weapon_throwable_secondary",			// TF_CLASS_SCOUT,
			"tf_weapon_throwable_secondary",			// TF_CLASS_SNIPER,
			"tf_weapon_throwable_secondary",			// TF_CLASS_SOLDIER,
			"tf_weapon_throwable_secondary",			// TF_CLASS_DEMOMAN,
			"tf_weapon_throwable_primary",				// TF_CLASS_MEDIC,
			"tf_weapon_throwable_secondary",			// TF_CLASS_HEAVYWEAPONS
			"tf_weapon_throwable_secondary",			// TF_CLASS_PYRO,
			"tf_weapon_throwable_secondary",			// TF_CLASS_SPY,
			"tf_weapon_throwable_secondary",			// TF_CLASS_ENGINEER,		
		}
	},
	{
		"tf_weapon_parachute",
		{
			"",											// TF_CLASS_UNDEFINED = 0,
			"",			// TF_CLASS_SCOUT,
			"",			// TF_CLASS_SNIPER,
			"tf_weapon_parachute_secondary",			// TF_CLASS_SOLDIER,
			"tf_weapon_parachute_primary",				// TF_CLASS_DEMOMAN,
			"",			// TF_CLASS_MEDIC,
			"",			// TF_CLASS_HEAVYWEAPONS
			"",			// TF_CLASS_PYRO,
			""			// TF_CLASS_SPY,
			"",			// TF_CLASS_ENGINEER,		
		}
	},
	{
		"tf_weapon_revolver",
		{
			"",											// TF_CLASS_UNDEFINED = 0,
			"",			// TF_CLASS_SCOUT,
			"",			// TF_CLASS_SNIPER,
			"",			// TF_CLASS_SOLDIER,
			"",				// TF_CLASS_DEMOMAN,
			"",			// TF_CLASS_MEDIC,
			"",			// TF_CLASS_HEAVYWEAPONS
			"",			// TF_CLASS_PYRO,
			"tf_weapon_revolver",				// TF_CLASS_SPY,
			"tf_weapon_revolver_secondary",		// TF_CLASS_ENGINEER,		
		}
	},
};

//-----------------------------------------------------------------------------
// Purpose: We need to support players putting any shotgun into a shotgun slot, pistol into a pistol slot, etc.
//			For legacy reasons, different classes actually spawn different entities for their shotguns/pistols/etc.
//			To deal with this, we translate entities into the right one for the class we're playing.
//-----------------------------------------------------------------------------
const char *TranslateWeaponEntForClass( const char *pszName, int iClass )
{
	if ( pszName )
	{
		for ( int i = 0; i < ARRAYSIZE(pszWpnEntTranslationList); i++ )
		{
			if ( !Q_stricmp( pszName, pszWpnEntTranslationList[i].pszWpnString ) )
			{
				const char *pTransName = pszWpnEntTranslationList[i].pszClassWpn[ iClass ];
				Assert( pTransName && pTransName[0] );
				return pTransName;
			}
		}
	}

	return pszName;
}

#ifdef TF_CLIENT_DLL
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
const char *g_pszClassIcons[SCOREBOARD_CLASS_ICONS] =
{
	"",
	"../hud/leaderboard_class_scout",
	"../hud/leaderboard_class_sniper",
	"../hud/leaderboard_class_soldier",
	"../hud/leaderboard_class_demo",
	"../hud/leaderboard_class_medic",
	"../hud/leaderboard_class_heavy",
	"../hud/leaderboard_class_pyro",
	"../hud/leaderboard_class_spy",
	"../hud/leaderboard_class_engineer",
	"../hud/leaderboard_class_scout_d",
	"../hud/leaderboard_class_sniper_d",
	"../hud/leaderboard_class_soldier_d",
	"../hud/leaderboard_class_demo_d",
	"../hud/leaderboard_class_medic_d",
	"../hud/leaderboard_class_heavy_d",
	"../hud/leaderboard_class_pyro_d",
	"../hud/leaderboard_class_spy_d",
	"../hud/leaderboard_class_engineer_d",
};

const char *g_pszItemClassImagesRed[] =
{
	"class_portraits/all_class",	// TF_CLASS_UNDEFINED = 0,
	"class_portraits/scout",		// TF_CLASS_SCOUT,			
	"class_portraits/sniper",		// TF_CLASS_SNIPER,
	"class_portraits/soldier",		// TF_CLASS_SOLDIER,
	"class_portraits/demoman",		// TF_CLASS_DEMOMAN,
	"class_portraits/medic",		// TF_CLASS_MEDIC,
	"class_portraits/heavy",		// TF_CLASS_HEAVYWEAPONS,
	"class_portraits/pyro",			// TF_CLASS_PYRO,
	"class_portraits/spy",			// TF_CLASS_SPY,
	"class_portraits/engineer",		// TF_CLASS_ENGINEER,
	"class_portraits/scout_grey",		// TF_CLASS_SCOUT,			
	"class_portraits/sniper_grey",		// TF_CLASS_SNIPER,
	"class_portraits/soldier_grey",		// TF_CLASS_SOLDIER,
	"class_portraits/demoman_grey",		// TF_CLASS_DEMOMAN,
	"class_portraits/medic_grey",		// TF_CLASS_MEDIC,
	"class_portraits/heavy_grey",		// TF_CLASS_HEAVYWEAPONS,
	"class_portraits/pyro_grey",		// TF_CLASS_PYRO,
	"class_portraits/spy_grey",			// TF_CLASS_SPY,
	"class_portraits/engineer_grey",	// TF_CLASS_ENGINEER,
};

const char *g_pszItemClassImagesBlue[] =
{
	"class_portraits/all_class",		// TF_CLASS_UNDEFINED = 0,
	"class_portraits/scout_blue",		// TF_CLASS_SCOUT,			
	"class_portraits/sniper_blue",		// TF_CLASS_SNIPER,
	"class_portraits/soldier_blue",		// TF_CLASS_SOLDIER,
	"class_portraits/demoman_blue",		// TF_CLASS_DEMOMAN,
	"class_portraits/medic_blue",		// TF_CLASS_MEDIC,
	"class_portraits/heavy_blue",		// TF_CLASS_HEAVYWEAPONS,
	"class_portraits/pyro_blue",		// TF_CLASS_PYRO,
	"class_portraits/spy_blue",			// TF_CLASS_SPY,
	"class_portraits/engineer_blue",	// TF_CLASS_ENGINEER,
	"class_portraits/scout_blue_grey",		// TF_CLASS_SCOUT,			
	"class_portraits/sniper_blue_grey",		// TF_CLASS_SNIPER,
	"class_portraits/soldier_blue_grey",	// TF_CLASS_SOLDIER,
	"class_portraits/demoman_blue_grey",	// TF_CLASS_DEMOMAN,
	"class_portraits/medic_blue_grey",		// TF_CLASS_MEDIC,
	"class_portraits/heavy_blue_grey",		// TF_CLASS_HEAVYWEAPONS,
	"class_portraits/pyro_blue_grey",		// TF_CLASS_PYRO,
	"class_portraits/spy_blue_grey",		// TF_CLASS_SPY,
	"class_portraits/engineer_blue_grey",	// TF_CLASS_ENGINEER,
};

#endif // TF_CLIENT_DLL
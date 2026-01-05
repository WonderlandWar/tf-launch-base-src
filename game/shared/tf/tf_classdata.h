//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#ifndef TF_CLASSDATA_H
#define TF_CLASSDATA_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Cache structure for the TF player class data (includes citizen). 
//-----------------------------------------------------------------------------
#define MAX_PLAYERCLASS_SOUND_LENGTH	128
#define TF_NAME_LENGTH		128


#define DEATH_SOUND_FIRST			( DEATH_SOUND_GENERIC )
#define DEATH_SOUND_LAST			( DEATH_SOUND_EXPLOSION )

enum DeathSoundType_t
{
	DEATH_SOUND_GENERIC = 0,
	DEATH_SOUND_CRIT,
	DEATH_SOUND_MELEE,
	DEATH_SOUND_EXPLOSION,

	DEATH_SOUND_TOTAL
};

struct TFPlayerClassData_t
{
	char		m_szClassName[TF_NAME_LENGTH];
	char		m_szModelName[TF_NAME_LENGTH];
	char		m_szHWMModelName[TF_NAME_LENGTH];
	char		m_szHandModelName[TF_NAME_LENGTH];
	char		m_szLocalizableName[TF_NAME_LENGTH];
	float		m_flMaxSpeed;
	int			m_nMaxHealth;
	int			m_nMaxArmor;
	int			m_aWeapons[TF_PLAYER_WEAPON_COUNT];
	int			m_aGrenades[TF_PLAYER_GRENADE_COUNT];
	int			m_aAmmoMax[TF_AMMO_COUNT];
	int			m_aBuildable[TF_PLAYER_BLUEPRINT_COUNT];

	bool		m_bDontDoAirwalk;
	bool		m_bDontDoNewJump;

	bool		m_bParsed;
	Vector		m_vecThirdPersonOffset;

#ifdef GAME_DLL
	// sounds
	char		m_szDeathSound[ DEATH_SOUND_TOTAL ][MAX_PLAYERCLASS_SOUND_LENGTH];
#endif

	TFPlayerClassData_t();
	const char *GetModelName() const;

#ifdef GAME_DLL
	const char *GetDeathSound( int nType );
#endif

	void Parse( const char *pszClassName );
	void ParseData( KeyValues *pKeyValuesData );
};


class CTFPlayerClassDataMgr : public CAutoGameSystem
{
public:

	CTFPlayerClassDataMgr();
	virtual bool Init( void );	
	TFPlayerClassData_t *Get( unsigned int iClass );
private:

	TFPlayerClassData_t m_aTFPlayerClassData[TF_CLASS_COUNT_ALL];
};

extern CTFPlayerClassDataMgr *g_pTFPlayerClassDataMgr;

// Legacy.
TFPlayerClassData_t *GetPlayerClassData( unsigned int iClass );

#endif // TF_CLASSDATA_H

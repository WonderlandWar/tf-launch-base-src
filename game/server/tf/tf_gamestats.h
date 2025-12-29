//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#ifndef TF_GAMESTATS_H
#define TF_GAMESTATS_H
#ifdef _WIN32
#pragma once
#endif

#include "gamestats.h"
#include "tf_obj.h"
#include "tf_gamestats_shared.h"
#include "GameEventListener.h"

class CTFPlayer;

//=============================================================================
//
// TF Game Stats Class
//
class CTFGameStats : public CBaseGameStats, public CGameEventListener, public CAutoGameSystem
{
public:

	// Constructor/Destructor.
	CTFGameStats( void );
	~CTFGameStats( void );

	virtual void Clear( void );

	virtual bool UseOldFormat() { return false; }
	virtual bool AddDataForSend( KeyValues *pKV, StatSendType_t sendType );

	void AddMapData( KeyValues *pKV, TF_Gamestats_LevelStats_t *pCurrentMap );

	virtual bool Init();
	virtual void LevelInitPreEntity();
	virtual void LevelShutdownPreClearSteamAPIContext();

	// Events.
	virtual void Event_LevelInit( void );
	virtual void Event_LevelShutdown( float flElapsed );
	virtual void Event_PlayerKilled( CBasePlayer *pPlayer, const CTakeDamageInfo &info );
	void Event_RoundStart();
	void Event_RoundEnd( int iWinningTeam, bool bFullRound, float flRoundTime, bool bWasSuddenDeathWin );
	void Event_GameEnd();
	void Event_PlayerConnected( CBasePlayer *pPlayer );
	void Event_PlayerDisconnected( CTFPlayer *pPlayer ) {}
	void Event_PlayerDisconnectedTF( CTFPlayer *pPlayer );
	void Event_PlayerChangedClass( CTFPlayer *pPlayer, int iOldClass, int iNewClass );
	void Event_PlayerSpawned( CTFPlayer *pPlayer );
	void Event_PlayerForceRespawn( CTFPlayer *pPlayer );
	void Event_PlayerLeachedHealth( CTFPlayer *pPlayer, bool bDispenserHeal, float amount );
	void Event_PlayerHealedOther( CTFPlayer *pPlayer, float amount );
	void Event_PlayerHealedOtherAssist( CTFPlayer *pPlayer, float amount );
	void Event_PlayerBlockedDamage( CTFPlayer *pPlayer, int nAmount );
	void Event_AssistKill( CTFPlayer *pPlayer, CBaseEntity *pVictim );
	void Event_PlayerInvulnerable( CTFPlayer *pPlayer );
	void Event_PlayerCreatedBuilding( CTFPlayer *pPlayer, CBaseObject *pBuilding );
	void Event_PlayerDestroyedBuilding( CTFPlayer *pPlayer, CBaseObject *pBuilding );
	void Event_AssistDestroyBuilding( CTFPlayer *pPlayer, CBaseObject *pBuilding );
	void Event_Headshot( CTFPlayer *pKiller, bool bBowShot=false );
	void Event_Backstab( CTFPlayer *pKiller );
	void Event_PlayerUsedTeleport( CTFPlayer *pTeleportOwner, CTFPlayer *pTeleportingPlayer );
	void Event_PlayerFiredWeapon( CTFPlayer *pPlayer, bool bCritical );
	void Event_PlayerDamage( CBasePlayer *pPlayer, const CTakeDamageInfo &info, int iDamageTaken );
	void Event_PlayerDamageAssist( CBasePlayer *pProvider, int iBonusDamage );
	void Event_PlayerKilledOther( CBasePlayer *pAttacker, CBaseEntity *pVictim, const CTakeDamageInfo &info );
	void Event_PlayerCapturedPoint( CTFPlayer *pPlayer );
	void Event_PlayerReturnedFlag( CTFPlayer *pPlayer );
	void Event_PlayerDefendedPoint( CTFPlayer *pPlayer );
	void Event_PlayerDominatedOther( CTFPlayer *pAttacker );
	void Event_PlayerRevenge( CTFPlayer *pAttacker );
	void Event_PlayerAwardBonusPoints( CTFPlayer *pPlayer, CBaseEntity *pSource, int nCount );
	void Event_PlayerHealthkitPickup( CTFPlayer *pTFPlayer );
	void Event_PlayerAmmokitPickup( CTFPlayer *pTFPlayer );
	void Event_PlayerSuicide( CBasePlayer *pPlayer );
	void Event_TeamChange( CTFPlayer* pPlayer, int oldTeam, int newTeam );

	void Event_HalloweenBossEvent( uint8 unBossType, uint16 unBossLevel, uint8 unEventType, uint8 unPlayersInvolved, float fElapsedTime );

	virtual void FireGameEvent( IGameEvent * event );

	bool IsRealGameplay( TF_Gamestats_LevelStats_t *game );

	void AccumulateGameData();
	void AccumulateVoteData( void );
	bool GetVoteData( const char *szIssueName, int nNumOptions, CUtlVector <const char*> &vecNames );
	
	void ClearCurrentGameData();

	// Utilities.
	TF_Gamestats_LevelStats_t	*GetCurrentMap( void )			{ return m_reportedStats.m_pCurrentGame; }
	TF_Gamestats_RoundStats_t*	GetRoundStatsForTeam( int iTeamNumber );
	void StoreGameEndReason( const char* reason );

	struct PlayerStats_t *		FindPlayerStats( CBasePlayer *pPlayer );
	void						ResetPlayerStats( CTFPlayer *pPlayer );
	void						ResetKillHistory( CTFPlayer *pPlayer );
	void						ResetRoundStats();
protected:	
	void						IncrementStat( CTFPlayer *pPlayer, TFStatType_t statType, int iValue );
	void						SendStatsToPlayer( CTFPlayer *pPlayer, bool bIsAlive );
	void						AccumulateAndResetPerLifeStats( CTFPlayer *pPlayer );
	void						TrackKillStats( CBasePlayer *pAttacker, CBasePlayer *pVictim );

public:
	TFReportedStats_t			m_reportedStats;		// Stats which are uploaded from TF server to Steam
	PlayerStats_t				m_aPlayerStats[MAX_PLAYERS_ARRAY_SAFE];	// List of stats for each player for current life - reset after each death or class change

	// Stats structs used for the new steamworks reporting.
	TF_Gamestats_RoundStats_t	m_currentRoundRed;
	TF_Gamestats_RoundStats_t	m_currentRoundBlue;
	TF_Gamestats_KillStats_t	m_currentKill;
	TF_Gamestats_LevelStats_t	m_currentMap;

	bool						m_bRoundActive;
	int							m_iRoundsPlayed;
	bool						m_bServerShutdown;
	int							m_iGameEndReason;

	// Unique Column Keys
	int							m_iEvents;
	int							m_iKillCount;
	int							m_iPlayerUpdates;
	int							m_iLoadoutChangesCount;

	// Robot Destruction Struct
	RobotDestructionStats_t		m_rdStats;

	// Passtime
	PasstimeStats_t				m_passtimeStats;

private:
	CUtlMap< CUtlConstString, int > m_MapsPlaytime;
	char						m_szNextMap[32];
};


extern CTFGameStats CTF_GameStats;

#endif // TF_GAMESTATS_H

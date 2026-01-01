//========= Copyright Valve Corporation, All rights reserved. ============//
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: The TF Game rules object
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#ifndef TF_GAMERULES_H
#define TF_GAMERULES_H

#ifdef _WIN32
#pragma once
#endif


#include "teamplayroundbased_gamerules.h"
#include "convar.h"
#include "gamevars_shared.h"
#include "GameEventListener.h"
#include "tf_gamestats_shared.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#else
#include "tf_player.h"
#endif

#ifdef CLIENT_DLL
	
	#define CTFGameRules C_TFGameRules
	#define CTFGameRulesProxy C_TFGameRulesProxy
#else
	extern BOOL no_cease_fire_text;
	extern BOOL cease_fire;

	class CHealthKit;
	class CCPTimerLogic;
	class CPopulationManager;
#endif

class CPhysicsProp;
#ifdef GAME_DLL
class CObjectSentrygun;
#else
class C_ObjectSentrygun;
#endif
extern ConVar	tf_spec_xray;
extern ConVar	tf_avoidteammates;
extern ConVar	tf_avoidteammates_pushaway;
extern ConVar	mp_tournament_blueteamname;
extern ConVar	mp_tournament_redteamname;
extern ConVar	tf_ctf_bonus_time;

#ifdef GAME_DLL
extern ConVar mp_tournament_prevent_team_switch_on_readyup;
#endif

#ifdef TF_RAID_MODE

class CRaidLogic;
class CBossBattleLogic;

extern ConVar tf_gamemode_raid;
extern ConVar tf_gamemode_creep_wave;
extern ConVar tf_gamemode_boss_battle;

#endif // TF_RAID_MODE

//extern ConVar tf_populator_health_multiplier;
//extern ConVar tf_populator_damage_multiplier;

extern ConVar tf_mvm_defenders_team_size;
extern ConVar tf_mvm_max_invaders;

const int kLadder_TeamSize_6v6 = 6;
const int kLadder_TeamSize_9v9 = 9;
const int kLadder_TeamSize_12v12 = 12;

//#define TF_MVM_FCVAR_CHEAT 0 /* Cheats enabled */
#define TF_MVM_FCVAR_CHEAT FCVAR_CHEAT /* Cheats disabled */

// How many achievements we show in the summary screen.
#define MAX_SHOWN_ACHIEVEMENTS 6
//=============================================================================
// HPE_END
//=============================================================================


extern Vector g_TFClassViewVectors[];

#define NO_CLASS_LIMIT -1

enum {
	STOPWATCH_CAPTURE_TIME_NOT_SET = 0,
	STOPWATCH_RUNNING,
	STOPWATCH_OVERTIME,
};

class CTFGameRulesProxy : public CTeamplayRoundBasedRulesProxy, public CGameEventListener
{
public:
	DECLARE_CLASS( CTFGameRulesProxy, CTeamplayRoundBasedRulesProxy );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	DECLARE_DATADESC();

	CTFGameRulesProxy();

	void	InputSetRedTeamRespawnWaveTime( inputdata_t &inputdata );
	void	InputSetBlueTeamRespawnWaveTime( inputdata_t &inputdata );
	void	InputAddRedTeamRespawnWaveTime( inputdata_t &inputdata );
	void	InputAddBlueTeamRespawnWaveTime( inputdata_t &inputdata );
	void	InputSetRedTeamGoalString( inputdata_t &inputdata );
	void	InputSetBlueTeamGoalString( inputdata_t &inputdata );
	void	InputSetRedTeamRole( inputdata_t &inputdata );
	void	InputSetBlueTeamRole( inputdata_t &inputdata );
	void	InputSetRequiredObserverTarget( inputdata_t &inputdata );
	void	InputAddRedTeamScore( inputdata_t &inputdata );
	void	InputAddBlueTeamScore( inputdata_t &inputdata );
	void	InputSetCTFCaptureBonusTime( inputdata_t &inputdata );
	void	InputPlayVORed( inputdata_t &inputdata );
	void	InputPlayVOBlue( inputdata_t &inputdata );
	void	InputPlayVO( inputdata_t &inputdata );
	void	InputHandleMapEvent( inputdata_t &inputdata );
	void	InputSetRoundRespawnFreezeEnabled( inputdata_t &inputdata );

	void	TeamPlayerCountChanged( CTFTeam *pTeam );
	void	StateEnterRoundRunning( void );
	void	StateEnterBetweenRounds( void );
	void	StateEnterPreRound( void );
	void	StateExitPreRound( void );
	void	MatchSummaryStart( void );

	COutputEvent m_OnWonByTeam1;
	COutputEvent m_OnWonByTeam2;
	COutputInt m_Team1PlayersChanged;
	COutputInt m_Team2PlayersChanged;
	COutputEvent m_OnStateEnterRoundRunning;
	COutputEvent m_OnStateEnterBetweenRounds;
	COutputEvent m_OnStateEnterPreRound;
	COutputEvent m_OnStateExitPreRound;
	COutputEvent m_OnMatchSummaryStart;

	virtual void Activate();

private:

	bool	m_bOvertimeAllowedForCTF;
#endif

public: // IGameEventListener Interface
	virtual void FireGameEvent( IGameEvent * event );
};

class CTFRadiusDamageInfo
{
	DECLARE_CLASS_NOBASE( CTFRadiusDamageInfo );
public:
	CTFRadiusDamageInfo( CTakeDamageInfo *pInfo, const Vector &vecSrcIn, float flRadiusIn, CBaseEntity *pIgnore = NULL, float flRJRadiusIn = 0, float flForceScaleIn = 1.0f )
	{
		dmgInfo = pInfo;
		vecSrc = vecSrcIn;
		flRadius = flRadiusIn;
		pEntityIgnore = pIgnore;
		flRJRadius = flRJRadiusIn;
		flFalloff = 0;
		m_flForceScale = flForceScaleIn;
		m_pEntityTarget = NULL;

		CalculateFalloff();
	}

	void CalculateFalloff( void );
	int ApplyToEntity( CBaseEntity *pEntity );

public:
	// Fill these in & call RadiusDamage()
	CTakeDamageInfo	*dmgInfo;
	Vector			vecSrc;
	float			flRadius;
	CBaseEntity		*pEntityIgnore;
	float			flRJRadius;	// Radius to use to calculate RJ, to maintain RJs when damage/radius changes on a RL
	float			m_flForceScale;
	CBaseEntity		*m_pEntityTarget;		// Target being direct hit if any
private:
	// These are used during the application of the RadiusDamage 
	float			flFalloff;
};

struct PlayerRoundScore_t
{
	int iPlayerIndex;	// player index
	int iRoundScore;	// how many points scored this round
	int	iTotalScore;	// total points scored across all rounds
};

#ifdef CLIENT_DLL
const char *GetMapType( const char *mapName );
const char *GetMapDisplayName( const char *mapName, bool bTitleCase = false );
#else

#endif

#define MAX_TEAMGOAL_STRING		256
#define MAX_TEAMNAME_STRING		6

class CTFGameRules : public CTeamplayRoundBasedRules
{
public:
	DECLARE_CLASS( CTFGameRules, CTeamplayRoundBasedRules );

	CTFGameRules();

	virtual void	LevelInitPostEntity( void );
	virtual float	GetRespawnTimeScalar( int iTeam );
	virtual float	GetRespawnWaveMaxLength( int iTeam, bool bScaleWithNumPlayers = true );

	// Damage Queries.
	virtual bool	Damage_IsTimeBased( int iDmgType );			// Damage types that are time-based.
	virtual bool	Damage_ShowOnHUD( int iDmgType );				// Damage types that have client HUD art.
	virtual bool	Damage_ShouldNotBleed( int iDmgType );			// Damage types that don't make the player bleed.
	// TEMP:
	virtual int		Damage_GetTimeBased( void );		
	virtual int		Damage_GetShowOnHud( void );
	virtual int		Damage_GetShouldNotBleed( void );

	int				GetFarthestOwnedControlPoint( int iTeam, bool bWithSpawnpoints );
	virtual bool	TeamMayCapturePoint( int iTeam, int iPointIndex );
	virtual bool	PlayerMayCapturePoint( CBasePlayer *pPlayer, int iPointIndex, char *pszReason = NULL, int iMaxReasonLength = 0 );
	virtual bool	PlayerMayBlockPoint( CBasePlayer *pPlayer, int iPointIndex, char *pszReason = NULL, int iMaxReasonLength = 0 );

	static int		CalcPlayerScore( RoundStats_t *pRoundStats, CTFPlayer *pPlayer );
	static int		CalcPlayerSupportScore( RoundStats_t *pRoundStats, int iPlayerIdx );

	bool			IsBirthday( void ) const;

	virtual const unsigned char *GetEncryptionKey( void ) { return GetTFEncryptionKey(); }

	int				GetClassLimit( int iClass );
	bool			CanPlayerChooseClass( CBasePlayer *pPlayer, int iClass );

	virtual bool	PointsMayBeCaptured( void ) OVERRIDE;

#ifdef GAME_DLL
public:
	virtual void	Precache( void );

	// Override this to prevent removal of game specific entities that need to persist
	virtual bool	RoundCleanupShouldIgnore( CBaseEntity *pEnt );
	virtual bool	ShouldCreateEntity( const char *pszClassName );
	virtual void	CleanUpMap( void );

	virtual void	FrameUpdatePostEntityThink();

	// Called when a new round is being initialized
	virtual void	SetupOnRoundStart( void );

	// Called when a new round is off and running
	virtual void	SetupOnRoundRunning( void );

	// Called before a new round is started (so the previous round can end)
	virtual void	PreviousRoundEnd( void );

	// Send the team scores down to the client
	virtual void	SendTeamScoresEvent( void ) { return; }

	// Send the end of round info displayed in the win panel
	virtual void	SendWinPanelInfo( bool bGameOver ) OVERRIDE;

	// Setup spawn points for the current round before it starts
	virtual void	SetupSpawnPointsForRound( void );

	// Called when a round has entered stalemate mode (timer has run out)
	virtual void	SetupOnStalemateStart( void );
	virtual void	SetupOnStalemateEnd( void );

	virtual void	RecalculateControlPointState( void );

	void			TeamPlayerCountChanged( CTFTeam *pTeam );
	int				GetAssignedHumanTeam( void );
	virtual void	HandleSwitchTeams( void );
	virtual void	HandleScrambleTeams( void );
	bool			CanChangeClassInStalemate( void );
	bool			CanChangeTeam( int iCurrentTeam ) const;

	virtual void	SetRoundOverlayDetails( void );	
	virtual void	ShowRoundInfoPanel( CTFPlayer *pPlayer = NULL ); // NULL pPlayer means show the panel to everyone

	virtual bool	TimerMayExpire( void );

	virtual void	Activate();

	virtual bool	AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info );

	void			SetTeamGoalString( int iTeam, const char *pszGoal );

	// Speaking, vcds, voice commands.
	virtual void	InitCustomResponseRulesDicts();
	virtual void	ShutdownCustomResponseRulesDicts();

	virtual bool	HasPassedMinRespawnTime( CBasePlayer *pPlayer );
	virtual bool	ShouldRespawnQuickly( CBasePlayer *pPlayer );

	bool			ShouldScorePerRound( void );

	virtual bool	IsValveMap( void );
	virtual bool 	IsOfficialMap();

	void			SetRequiredObserverTarget( CBaseEntity *pEnt ){ m_hRequiredObserverTarget = pEnt; }
	void			SetObjectiveObserverTarget( CBaseEntity *pEnt ) { m_hObjectiveObserverTarget = pEnt; }
	EHANDLE			GetRequiredObserverTarget( void ){ return m_hRequiredObserverTarget.Get(); }
	EHANDLE			GetObjectiveObserverTarget( void ){ return m_hObjectiveObserverTarget.Get(); }

	virtual void	GetTaggedConVarList( KeyValues *pCvarTagList );

	virtual void	ClientCommandKeyValues( edict_t *pEntity, KeyValues *pKeyValues );

	void SetCTFCaptureBonusTime( float flTime ){ m_flCTFCaptureBonusTime = flTime; }
	float GetCTFCaptureBonusTime( void )
	{ 
		float flRetVal = tf_ctf_bonus_time.GetFloat();
		if ( m_flCTFCaptureBonusTime >= 0.0f )
		{
			flRetVal = m_flCTFCaptureBonusTime;
		}

		return flRetVal; 
	}

	// populate vector with set of control points the player needs to capture
	virtual void CollectCapturePoints( CBasePlayer *player, CUtlVector< CTeamControlPoint * > *captureVector ) const;

	// populate vector with set of control points the player needs to defend from capture
	virtual void CollectDefendPoints( CBasePlayer *player, CUtlVector< CTeamControlPoint * > *defendVector ) const;

	CObjectSentrygun *FindSentryGunWithMostKills( int team = TEAM_ANY ) const;

	virtual bool ShouldSkipAutoScramble( void ) OVERRIDE
	{
		return false;
	}

	void			HandleMapEvent( inputdata_t &inputdata );

	virtual bool	ShouldWaitToStartRecording( void );

	void			SetGravityMultiplier( float flValue ){ m_flGravityMultiplier.Set( flValue ); }

	bool			CanFlagBeCaptured( CBaseEntity *pOther );

	void			TeleportPlayersToTargetEntities( int iTeam, const char *pszEntTargetName, CUtlVector< CTFPlayer * > *pTeleportedPlayers );

	virtual void	BroadcastSound( int iTeam, const char *sound, int iAdditionalSoundFlags = 0, CBasePlayer *pPlayer = NULL ) override;

	void			RegisterScriptFunctions() override;

	int				GetRoundState() { return (int)State_Get(); }

	bool			InMatchStartCountdown() { return BInMatchStartCountdown(); }

protected:

	virtual const char* GetStalemateSong( int nTeam ) OVERRIDE;
	virtual const char* WinSongName( int nTeam ) OVERRIDE;
	virtual const char* LoseSongName( int nTeam ) OVERRIDE;

	virtual void	InitTeams( void );

	virtual void	RoundRespawn( void );

	virtual void	InternalHandleTeamWin( int iWinningTeam );
	
	static int		PlayerRoundScoreSortFunc( const PlayerRoundScore_t *pRoundScore1, const PlayerRoundScore_t *pRoundScore2 );

	virtual void FillOutTeamplayRoundWinEvent( IGameEvent *event );

	virtual bool CanChangelevelBecauseOfTimeLimit( void );
	virtual bool CanGoToStalemate( void );

	void BroadcastDrawLine( CTFPlayer *pTFPlayer, KeyValues *pKeyValues );

#endif // GAME_DLL

public:
	// Return the value of this player towards capturing a point
	virtual int		GetCaptureValueForPlayer( CBasePlayer *pPlayer );

	// Collision and Damage rules.
	virtual bool	ShouldCollide( int collisionGroup0, int collisionGroup1 );
	
	int GetTimeLeft( void );

	// Get the view vectors for this mod.
	virtual const CViewVectors *GetViewVectors() const;

	virtual void FireGameEvent( IGameEvent *event );

	virtual const char *GetGameTypeName( void );
	virtual int GetGameType( void ){ return m_nGameType; }

	virtual void ClientSpawned( edict_t * pPlayer );

	virtual void OnFileReceived( const char * fileName, unsigned int transferID );

	virtual bool FlagsMayBeCapped( void );

	void	RunPlayerConditionThink ( void );

	const char *GetTeamGoalString( int iTeam );

	int		GetStopWatchState( void ) { return m_nStopWatchState; }

	// TF2007: This should all be removed eventually
	 
	// Competitive games
	bool IsCommunityGameMode( void ) const;
	bool IsCompetitiveMode( void ) const;			// means we're using competitive/casual matchmaking
	bool IsMatchTypeCasual( void ) const;
	bool IsMatchTypeCompetitive( void ) const;
	// Are we showing the match-start-countdown doors right now
	bool BInMatchStartCountdown() const;
#ifdef GAME_DLL
	void SyncMatchSettings();
	// ! Check return
	bool StartManagedMatch();
	void SetCompetitiveMode( bool bValue );
#endif
	void EndCompetitiveMatch( void );
	void ManageCompetitiveMode( void );
	bool MatchmakingShouldUseStopwatchMode( void );
	bool IsAttackDefenseMode( void );

	bool IsManagedMatchEnded() const;

	bool UsePlayerReadyStatusMode( void );
	bool PlayerReadyStatus_HaveMinPlayersToEnable( void );
#ifdef GAME_DLL
	bool PlayerReadyStatus_ArePlayersOnTeamReady( int iTeam );
	bool PlayerReadyStatus_ShouldStartCountdown( void );
	void PlayerReadyStatus_ResetState( void );
	void PlayerReadyStatus_UpdatePlayerState( CTFPlayer *pTFPlayer, bool bState );
#endif // GAME_DLL

	bool IsDefaultGameMode( void );		// The absence of arena, mvm, tournament mode, etc

	virtual bool AllowThirdPersonCamera( void ) { return ( ShowMatchSummary() ); }

	int		GetStatsMinimumPlayers( void );
	int		GetStatsMinimumPlayedTime( void );

	// BountyMode
	bool IsBountyMode( void ) { return false; }

	float GetGravityMultiplier(  void ){ return m_flGravityMultiplier; }

	virtual bool IsConnectedUserInfoChangeAllowed( CBasePlayer *pPlayer );

	bool MapHasMatchSummaryStage( void ){ return m_bMapHasMatchSummaryStage; }
	bool PlayersAreOnMatchSummaryStage( void ){ return m_bPlayersAreOnMatchSummaryStage; }

	bool ShowMatchSummary( void ){ return m_bShowMatchSummary; }

	bool HaveStopWatchWinner( void ) { return m_bStopWatchWinner; }
#ifdef GAME_DLL
	void KickPlayersNewMatchIDRequestFailed();
#endif // GAME_DLL

#ifdef CLIENT_DLL

	DECLARE_CLIENTCLASS_NOBASE(); // This makes data tables able to access our private vars.

	virtual ~CTFGameRules();

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	HandleOvertimeBegin();

	bool			ShouldShowTeamGoal( void );

	const char *GetVideoFileForMap( bool bWithExtension = true );

	const char *FormatVideoName( const char *videoName, bool bWithExtension = true );

	bool			UseSillyGibs( void );

	virtual bool	AllowMapParticleEffect( const char *pszParticleEffect );

	virtual bool	AllowWeatherParticles( void );

	virtual void	ModifySentChat( char *pBuf, int iBufSize );

	bool RecievedBaseline() const { return m_bRecievedBaseline; }

#else

	DECLARE_SERVERCLASS_NOBASE(); // This makes data tables able to access our private vars.
	
	virtual ~CTFGameRules();

	virtual void LevelShutdown();
	virtual bool ClientCommand( CBaseEntity *pEdict, const CCommand &args );
	virtual void Think();

	virtual bool SwitchToNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon );

	bool CheckWinLimit( bool bAllowEnd = true, int nAddValueWhenChecking = 0 ) OVERRIDE;
	bool SetCtfWinningTeam();
	bool CheckCapsPerRound();
	virtual void CheckRespawnWaves();
	virtual void PlayWinSong( int team ) OVERRIDE;

	virtual void SetWinningTeam( int team, int iWinReason, bool bForceMapReset = true, bool bSwitchTeams = false, bool bDontAddScore = false, bool bFinal = false ) OVERRIDE;
	virtual void SetStalemate( int iReason, bool bForceMapReset = true, bool bSwitchTeams = false );

	void CheckTauntAchievement( CTFPlayer *pAchiever, int nGibs, int *pTauntCamAchievements );

	virtual bool FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo &info );

	// Spawing rules.
	CBaseEntity *GetPlayerSpawnSpot( CBasePlayer *pPlayer );
	bool IsSpawnPointValid( CBaseEntity *pSpot, CBasePlayer *pPlayer, bool bIgnorePlayers, PlayerTeamSpawnMode_t nSpawndMode = PlayerTeamSpawnMode_Normal );

	virtual int ItemShouldRespawn( CItem *pItem );
	virtual float FlItemRespawnTime( CItem *pItem );
	virtual Vector VecItemRespawnSpot( CItem *pItem );
	virtual QAngle VecItemRespawnAngles( CItem *pItem );

	virtual const char *GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer );
	void ClientSettingsChanged( CBasePlayer *pPlayer );
	void ChangePlayerName( CTFPlayer *pPlayer, const char *pszNewName );

	virtual VoiceCommandMenuItem_t *VoiceCommand( CBaseMultiplayerPlayer *pPlayer, int iMenu, int iItem ); 

	float GetPreMatchEndTime() const;	// Returns the time at which the prematch will be over.
	void GoToIntermission( void );

	virtual int GetAutoAimMode()	{ return AUTOAIM_NONE; }
	void SetSetup( bool bSetup );
	void ManageStopwatchTimer( bool bInSetup );
	virtual void HandleTeamScoreModify( int iTeam, int iScore);

	bool CanHaveAmmo( CBaseCombatCharacter *pPlayer, int iAmmoIndex );

	virtual const char *GetGameDescription( void ){ return "Team Fortress"; }

	virtual void Status( void (*print) (PRINTF_FORMAT_STRING const char *fmt, ...) );

	// Sets up g_pPlayerResource.
	virtual void CreateStandardEntities();

	virtual void PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info );
	virtual void PlayerKilledCheckAchievements( CTFPlayer *pAttacker, CTFPlayer *pVictim );
	virtual void DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info );
	virtual void DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info, const char* eventName );
	virtual CBasePlayer *GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor, CBaseEntity *pVictim );

	void CalcDominationAndRevenge( CTFPlayer *pAttacker, CBaseEntity *pWeapon, CTFPlayer *pVictim, bool bIsAssist, int *piDeathFlags );

	const char *GetKillingWeaponName( const CTakeDamageInfo &info, CTFPlayer *pVictim, int *iWeaponID );
	CBasePlayer *GetAssister( CBasePlayer *pVictim, CBasePlayer *pScorer, CBaseEntity *pInflictor );
	CTFPlayer *GetRecentDamager( CTFPlayer *pVictim, int iDamager, float flMaxElapsed );

	virtual void ClientDisconnected( edict_t *pClient );

	virtual void  RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore );
	void	RadiusDamage( CTFRadiusDamageInfo &info );

	bool ApplyOnDamageModifyRules( CTakeDamageInfo &info, CBaseEntity *pVictimBaseEntity );

	float ApplyOnDamageAliveModifyRules( const CTakeDamageInfo &info, CBaseEntity *pVictimBaseEntity, bool &bIgniting );

	virtual float FlPlayerFallDamage( CBasePlayer *pPlayer );

	virtual bool  FlPlayerFallDeathDoesScreenFade( CBasePlayer *pl ) { return false; }

	virtual bool UseSuicidePenalty() { return false; }

	int		GetPreviousRoundWinners( void ) { return m_iPreviousRoundWinners; }

	void	SendHudNotification( IRecipientFilter &filter, HudNotification_t iType, bool bForceShow = false  );
	void	SendHudNotification( IRecipientFilter &filter, const char *pszText, const char *pszIcon, int iTeam = TEAM_UNASSIGNED );
	void	StopWatchModeThink( void );

	virtual		void RestartTournament( void );

	bool	TFVoiceManager( CBasePlayer *pListener, CBasePlayer *pTalker );

	float		GetRoundStart( void ) { return m_flRoundStartTime; }

	// Voting
	void		ManageServerSideVoteCreation( void );

	virtual bool StopWatchShouldBeTimedWin( void ) OVERRIDE;

public:
	bool	TournamentModeCanEndWithTimelimit( void ){ return ( GetStopWatchTimer() == NULL ); }

	void SetOvertimeAllowedForCTF( bool bAllowed ){ m_bOvertimeAllowedForCTF = bAllowed; }
	bool GetOvertimeAllowedForCTF( void ){ return m_bOvertimeAllowedForCTF; }

	virtual void ProcessVerboseLogOutput( void );

	void PushAllPlayersAway( const Vector& vFromThisPoint, float flRange, float flForce, int nTeam, CUtlVector< CTFPlayer* > *pPushedPlayers = NULL );
	
	void MatchSummaryStart( void );
	void MatchSummaryEnd( void );

	int GetTeamAssignmentOverride( CTFPlayer *pTFPlayer, int iDesiredTeam, bool bAutoBalance = false );
private:

	int DefaultFOV( void ) { return 75; }

	void StopWatchShouldBeTimedWin_Calculate( void );
	
#endif // GAME_DLL

private:


#ifdef GAME_DLL

	Vector2D	m_vecPlayerPositions[MAX_PLAYERS_ARRAY_SAFE];

	CUtlVector<CHandle<CHealthKit> > m_hDisabledHealthKits;	


	char	m_szMostRecentCappers[MAX_PLAYERS_ARRAY_SAFE];	// list of players who made most recent capture.  Stored as string so it can be passed in events.
	int		m_iNumCaps[TF_TEAM_COUNT];				// # of captures ever by each team during a round

	int SetCurrentRoundStateBitString();
	void SetMiniRoundBitMask( int iMask );
	int m_iPrevRoundState;	// bit string representing the state of the points at the start of the previous miniround
	int m_iCurrentRoundState;
	int m_iCurrentMiniRoundMask;

	CHandle<CTeamRoundTimer>	m_hStopWatchTimer;
	

	CTeamRoundTimer* GetStopWatchTimer( void ) { return (CTeamRoundTimer*)m_hStopWatchTimer.Get(); }

	EHANDLE m_hRequiredObserverTarget;
	EHANDLE m_hObjectiveObserverTarget;

	CHandle<CTFGameRulesProxy> m_hGamerulesProxy;

	float	m_flSendNotificationTime;
	
	bool	m_bOvertimeAllowedForCTF;

	// Automatic vote called near the end of a map
	bool	m_bVoteCalled;
	bool	m_bServerVoteOnReset;
	float	m_flVoteCheckThrottle;

	CUtlVector< CHandle< CCPTimerLogic > > m_CPTimerEnts;
	float	m_flCapInProgressBuffer;

#ifdef TF_RAID_MODE
	CHandle< CRaidLogic >		m_hRaidLogic;
	CHandle< CBossBattleLogic > m_hBossBattleLogic;
#endif // TF_RAID_MODE

	float	m_flCheckPlayersConnectingTime;

	float m_flNextStrangeEventProcessTime;
#else

	bool	m_bSillyGibs;

#endif

	CNetworkVar( ETFGameType, m_nGameType ); // Type of game this map is (CTF, CP)
	CNetworkVar( int, m_nStopWatchState );
	CNetworkString( m_pszTeamGoalStringRed, MAX_TEAMGOAL_STRING );
	CNetworkString( m_pszTeamGoalStringBlue, MAX_TEAMGOAL_STRING );
	CNetworkVar( float, m_flCapturePointEnableTime );
	CNetworkVar( int, m_iGlobalAttributeCacheVersion );

	CNetworkVar( bool, m_bHaveMinPlayersToEnableReady );

	CNetworkVar( bool, m_bBountyModeEnabled );
	CNetworkVar( float, m_flGravityMultiplier );
	CNetworkVar( bool, m_bMatchEnded );

	CNetworkVar( bool, m_bTeamsSwitched );

#ifdef GAME_DLL
	float	m_flNextFlagAlarm;
	float	m_flNextFlagAlert;

	float	m_flSafeToLeaveTimer;
#endif

	CNetworkVar( bool, m_bShowMatchSummary );
	CNetworkVar( bool, m_bMapHasMatchSummaryStage );
	CNetworkVar( bool, m_bPlayersAreOnMatchSummaryStage );
	CNetworkVar( bool, m_bStopWatchWinner );

	float		m_flCTFCaptureBonusTime;
public:

	bool m_bControlSpawnsPerTeam[ MAX_TEAMS ][ MAX_CONTROL_POINTS ];
	int	 m_iPreviousRoundWinners;

	float	GetCapturePointTime( void ) { return m_flCapturePointEnableTime; }

	virtual bool ShouldDrawHeadLabels() override;

	bool CanInitiateDuels( void );

#ifdef GAME_DLL
	void SetBirthdayPlayer( CBaseEntity *pEntity );

	// remove all projectiles in the world
	void RemoveAllProjectiles();

	// remove all buildings in the world
	void RemoveAllBuildings( bool bExplodeBuildings = false );

	// remove all sentry's ammo
	void RemoveAllSentriesAmmo();

	// remove all projectiles and objects
	void RemoveAllProjectilesAndBuildings( bool bExplodeBuildings = false );

#endif // GAME_DLL
	CBaseEntity *GetBirthdayPlayer( void ) const
	{
		return m_hBirthdayPlayer.Get();
	}

	int GetGlobalAttributeCacheVersion( void ) const
	{
		return m_iGlobalAttributeCacheVersion;
	}

	void FlushAllAttributeCaches( void )
	{
		m_iGlobalAttributeCacheVersion++;
	}

private:	
#ifdef CLIENT_DLL
	bool m_bRecievedBaseline;
#endif


	CountdownTimer m_botCountTimer;

	bool m_bUseMatchHUD;
	bool m_bUsePreRoundDoors;
#ifdef GAME_DLL
public:

	bool BIsManagedMatchEndImminent( void );

private:
	struct TeleportLocation_t
	{
		Vector m_vecPosition;
		QAngle m_qAngles;
	};
	CUtlMap< string_t, CUtlVector< TeleportLocation_t >* > m_mapTeleportLocations;

	bool	m_bMapCycleNeedsUpdate;

	float	m_flCompModeRespawnPlayersAtMatchStart;

#endif // GAME_DLL

	// LEGACY BOSS CODE. Keeping this to not break demo
	CNetworkVar( int, m_nBossHealth );
	CNetworkVar( int, m_nMaxBossHealth );
	CNetworkVar( float, m_fBossNormalizedTravelDistance );

	CNetworkHandle( CBaseEntity, m_hBirthdayPlayer );	// entindex of current birthday player (0 = none)

	bool m_bIsBirthday;

// MvM Helpers
#ifdef GAME_DLL
public:

	virtual void BalanceTeams( bool bRequireSwitcheesToBeDead );
#endif
};

//-----------------------------------------------------------------------------
// Gets us at the team fortress game rules
//-----------------------------------------------------------------------------

inline CTFGameRules* TFGameRules()
{
	return static_cast<CTFGameRules*>(g_pGameRules);
}

#ifdef GAME_DLL
bool EntityPlacementTest( CBaseEntity *pMainEnt, const Vector &vOrigin, Vector &outPos, bool bDropToGround );

#define CP_TIMER_THINK "CCPTimerLogicThink"
class CCPTimerLogic : public CPointEntity
{
	DECLARE_CLASS( CCPTimerLogic, CPointEntity );
public:
	DECLARE_DATADESC();

	CCPTimerLogic()
	{ 
		m_nTimerLength = 60; // seconds
		m_iszControlPointName = NULL_STRING;
		m_hControlPoint = NULL;
		m_bFire15SecRemain = m_bFire10SecRemain = m_bFire5SecRemain = true;
	
		SetContextThink( &CCPTimerLogic::Think, gpGlobals->curtime + 0.15, CP_TIMER_THINK );
	}
	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

	void InputRoundSpawn( inputdata_t &inputdata );
	void Think( void );
	bool TimerMayExpire( void );

private:
	int m_nTimerLength;
	string_t m_iszControlPointName;
	CHandle<CTeamControlPoint> m_hControlPoint;
	CountdownTimer m_pointTimer;

	bool m_bFire15SecRemain;
	bool m_bFire10SecRemain;
	bool m_bFire5SecRemain;

	COutputEvent m_onCountdownStart;
	COutputEvent m_onCountdown15SecRemain;
	COutputEvent m_onCountdown10SecRemain;
	COutputEvent m_onCountdown5SecRemain;
	COutputEvent m_onCountdownEnd;

	int m_nTimerTeam = TF_TEAM_BLUE;
};
#endif

#ifdef GAME_DLL
class CSingleUserReliableRecipientFilter : public CRecipientFilter
{
public:
	CSingleUserReliableRecipientFilter( CBasePlayer *player )
	{
		AddRecipient( player );
		MakeReliable();
	}
};
#endif

#endif // TF_GAMERULES_H

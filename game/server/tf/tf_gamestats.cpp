//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "tf_gamerules.h"
#include "tf_gamestats.h"
#include "tf_obj_sentrygun.h"
#include "tf_obj_dispenser.h"
#include "tf_obj_sapper.h"
#include "tf_team.h"
#include "usermessages.h"
#include "player_resource.h"
#include "team.h"
#include "achievementmgr.h"
#include "hl2orange.spa.h"
#include "tf_weapon_medigun.h"
#include "team_control_point_master.h"
#include "steamworks_gamestats.h"
#include "vote_controller.h"

#include "filesystem.h" // for temp passtime local stats logging

// Must run with -gamestats to be able to turn on/off stats with ConVar below.
static ConVar tf_stats_nogameplaycheck( "tf_stats_nogameplaycheck", "0", FCVAR_NONE , "Disable normal check for valid gameplay, send stats regardless." );
//static ConVar tf_stats_track( "tf_stats_track", "1", FCVAR_NONE, "Turn on//off tf stats tracking." );
//static ConVar tf_stats_verbose( "tf_stats_verbose", "0", FCVAR_NONE, "Turn on//off verbose logging of stats." );

CTFGameStats CTF_GameStats;

const char *g_aClassNames[] =
{
	"TF_CLASS_UNDEFINED",
	"TF_CLASS_SCOUT",
	"TF_CLASS_SNIPER",
	"TF_CLASS_SOLDIER",
	"TF_CLASS_DEMOMAN",
	"TF_CLASS_MEDIC",
	"TF_CLASS_HEAVYWEAPONS",
	"TF_CLASS_PYRO",
	"TF_CLASS_SPY",
	"TF_CLASS_ENGINEER",
	"TF_CLASS_CIVILIAN",
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGameStats::CTFGameStats()
{
	gamestats = this;
	Clear();

	SetDefLessFunc( m_MapsPlaytime );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGameStats::~CTFGameStats()
{
	Clear();
}

//-----------------------------------------------------------------------------
// Purpose: Clear out game stats
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFGameStats::Clear( void )
{
	m_bRoundActive = false;
	m_iRoundsPlayed = 0;
	m_iEvents = 0;
	m_iKillCount = 0;
	m_iPlayerUpdates = 0;
	m_reportedStats.Clear();
	Q_memset( m_aPlayerStats, 0, sizeof( m_aPlayerStats ) );
	m_rdStats.Clear();
	m_passtimeStats.Clear();
	m_iLoadoutChangesCount = 1;
}

//-----------------------------------------------------------------------------
// Purpose: Adds our data to the gamestats data that gets uploaded.
//			Returns true if we added data, false if we didn't
//-----------------------------------------------------------------------------
bool CTFGameStats::AddDataForSend( KeyValues *pKV, StatSendType_t sendType )
{
	// we only have data to send at level shutdown
	if ( sendType != STATSEND_LEVELSHUTDOWN )
		return false;

	// do we have anything to report?
	if ( !m_reportedStats.m_bValidData )
		return false;

	// Save data per map.

	CUtlDict<TF_Gamestats_LevelStats_t, unsigned short> &dictMapStats = m_reportedStats.m_dictMapStats;
	Assert( dictMapStats.Count() <= 1 );

	for ( int iMap = dictMapStats.First(); iMap != dictMapStats.InvalidIndex(); iMap = dictMapStats.Next( iMap ) )
	{
		// Get the current map.
		TF_Gamestats_LevelStats_t *pCurrentMap = &dictMapStats[iMap];
		Assert( pCurrentMap );

		KeyValues *pKVData = new KeyValues( "tf_mapdata" );
		pKVData->SetInt( "RoundsPlayed", pCurrentMap->m_Header.m_iRoundsPlayed );
		pKVData->SetInt( "TotalTime", pCurrentMap->m_Header.m_iTotalTime );
		pKVData->SetInt( "BlueWins", pCurrentMap->m_Header.m_iBlueWins );
		pKVData->SetInt( "RedWins", pCurrentMap->m_Header.m_iRedWins );
		pKVData->SetInt( "Stalemates", pCurrentMap->m_Header.m_iStalemates );
		pKVData->SetInt( "BlueSuddenDeathWins", pCurrentMap->m_Header.m_iBlueSuddenDeathWins );
		pKVData->SetInt( "RedSuddenDeathWins", pCurrentMap->m_Header.m_iRedSuddenDeathWins );
		for ( int i = 0; i <= MAX_CONTROL_POINTS; i++ )
		{
			if ( pCurrentMap->m_Header.m_iLastCapChangedInRound[i] )
			{
				pKVData->SetInt( UTIL_VarArgs("RoundsEndingOnCP%d",i), pCurrentMap->m_Header.m_iLastCapChangedInRound[i] );
			}
		}
		pKV->AddSubKey( pKVData );

		// save class stats
		for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass <= TF_LAST_NORMAL_CLASS; iClass ++ )
		{
			TF_Gamestats_ClassStats_t &classStats = pCurrentMap->m_aClassStats[iClass];
			if ( classStats.iTotalTime > 0 )
			{
				pKVData = new KeyValues( "tf_classdata" );
				pKVData->SetInt( "Class", iClass );
				pKVData->SetInt( "Spawns", classStats.iSpawns );
				pKVData->SetInt( "TotalTime", classStats.iTotalTime );
				pKVData->SetInt( "Score", classStats.iScore );
				pKVData->SetInt( "Kills", classStats.iKills );
				pKVData->SetInt( "Deaths", classStats.iDeaths );
				pKVData->SetInt( "Assists", classStats.iAssists );
				pKVData->SetInt( "Captures", classStats.iCaptures );
				pKVData->SetInt( "ClassChanges", classStats.iClassChanges );
				pKV->AddSubKey( pKVData );
			}	
		}

		// save weapon stats
		for ( int iWeapon = TF_WEAPON_NONE+1; iWeapon < TF_WEAPON_COUNT; iWeapon++ )
		{
			TF_Gamestats_WeaponStats_t &weaponStats = pCurrentMap->m_aWeaponStats[iWeapon];
			if ( weaponStats.iShotsFired > 0 )
			{
				pKVData = new KeyValues( "tf_weapondata" );
				pKVData->SetInt( "WeaponID", iWeapon );
				pKVData->SetInt( "ShotsFired", weaponStats.iShotsFired );
				pKVData->SetInt( "ShotsFiredCrit", weaponStats.iCritShotsFired );
				pKVData->SetInt( "ShotsHit", weaponStats.iHits );
				pKVData->SetInt( "DamageTotal", weaponStats.iTotalDamage );
				pKVData->SetInt( "HitsWithKnownDistance", weaponStats.iHitsWithKnownDistance );
				pKVData->SetInt( "DistanceTotal", weaponStats.iTotalDistance );
				pKV->AddSubKey( pKVData );
			}	
		}

		//// save deaths
		//for ( int i = 0; i < pCurrentMap->m_aPlayerDeaths.Count(); i++ )
		//{
		//	TF_Gamestats_LevelStats_t::PlayerDeathsLump_t &playerDeath = pCurrentMap->m_aPlayerDeaths[i];

		//	pKVData = new KeyValues( "tf_deaths" );
		//	pKVData->SetInt( "DeathIndex", i );
		//	pKVData->SetInt( "X", playerDeath.nPosition[0] );
		//	pKVData->SetInt( "Y", playerDeath.nPosition[1] );
		//	pKVData->SetInt( "Z", playerDeath.nPosition[2] );
		//	pKV->AddSubKey( pKVData );
		//}
	}

	// clear stats since we've now reported these
	m_reportedStats.Clear();

	return true;
}

extern CBaseGameStats_Driver CBGSDriver;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameStats::Init( void )
{
	ListenForGameEvent( "teamplay_round_start" );
	ListenForGameEvent( "tf_game_over" );
	ListenForGameEvent( "teamplay_game_over" );	

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void StripNewlineFromString( char *string )
{
	int nStrlength = strlen( string ) - 1;
	if ( nStrlength >= 0 )
	{
		if ( string[nStrlength] == '\n' || string[nStrlength] == '\r' )
			string[nStrlength] = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_LevelInit( void )
{
	ClearCurrentGameData();

	// Get the host ip and port.
	int nIPAddr = 0;
	short nPort = 0;
	ConVar *hostip = cvar->FindVar( "hostip" );
	if ( hostip )
	{
		nIPAddr = hostip->GetInt();
	}			

	ConVar *hostport = cvar->FindVar( "hostip" );
	if ( hostport )
	{
		nPort = hostport->GetInt();
	}			

	m_reportedStats.m_pCurrentGame->Init( STRING( gpGlobals->mapname ), gpGlobals->mapversion, nIPAddr, nPort, gpGlobals->curtime );
	m_reportedStats.m_bValidData = false;

	TF_Gamestats_LevelStats_t *map = m_reportedStats.FindOrAddMapStats( STRING( gpGlobals->mapname ) );
	map->Init( STRING( gpGlobals->mapname ), gpGlobals->mapversion, nIPAddr, nPort, gpGlobals->curtime );

	m_currentRoundRed.m_iNumRounds = 0;

	m_currentMap.Init( STRING( gpGlobals->mapname ), gpGlobals->mapversion, nIPAddr, nPort, gpGlobals->curtime );

	if ( !g_pStringTableServerMapCycle )
		return;

	// Parse the server's mapcycle for playtime tracking (used in voting)
	if ( !m_MapsPlaytime.Count() )
	{
		int index = g_pStringTableServerMapCycle->FindStringIndex( "ServerMapCycle" );
		if ( index != ::INVALID_STRING_INDEX )
		{
			int nLength = 0;
			const char *pszMapCycle = (const char *)g_pStringTableServerMapCycle->GetStringUserData( index, &nLength );
			if ( pszMapCycle && pszMapCycle[0] )
			{
				if ( pszMapCycle && nLength )
				{
					CUtlVector< char * > vecMapCycle;
					V_SplitString( pszMapCycle, "\n", vecMapCycle );

					if ( vecMapCycle.Count() )
					{
						for ( int index = 0; index < vecMapCycle.Count(); index++ )
						{
							// Initialize the list with random playtimes - to vary the first vote options
							int nSeed = RandomInt( 1, 300 );
							StripNewlineFromString( vecMapCycle[index] );

							// Canonicalize map name and ensure we know of it
							char szMapName[ 64 ] = { 0 };
							V_strncpy( szMapName, vecMapCycle[index], sizeof( szMapName ) );
							IVEngineServer::eFindMapResult eResult = engine->FindMap( szMapName, sizeof( szMapName ) );
							switch ( eResult )
							{
							case IVEngineServer::eFindMap_Found:
							case IVEngineServer::eFindMap_NonCanonical:
							case IVEngineServer::eFindMap_FuzzyMatch:
								m_MapsPlaytime.Insert( CUtlConstString( szMapName ), nSeed );
								break;
							case IVEngineServer::eFindMap_NotFound:
								// Don't know the canonical map name for stats here :-/
							case IVEngineServer::eFindMap_PossiblyAvailable:
							default:
								break;
							}
						}
					}

					vecMapCycle.PurgeAndDeleteElements();
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Opens a new server session when a level is started.
//-----------------------------------------------------------------------------
void CTFGameStats::LevelInitPreEntity()
{
	// Start the server session.
	GetSteamWorksSGameStatsUploader().StartSession();
}

//-----------------------------------------------------------------------------
// Purpose: Closes the server session when the level is shutdown.
//-----------------------------------------------------------------------------
void CTFGameStats::LevelShutdownPreClearSteamAPIContext()
{
	// End the server session.
	GetSteamWorksSGameStatsUploader().EndSession();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_LevelShutdown( float flElapsed )
{
	if ( m_reportedStats.m_pCurrentGame )
	{
		flElapsed = gpGlobals->curtime - m_reportedStats.m_pCurrentGame->m_flRoundStartTime;
		m_reportedStats.m_pCurrentGame->m_Header.m_iTotalTime += (int) flElapsed;
	}

	// Store data for the vote system (for issues that present choices based on stats)
	AccumulateVoteData();

	// add current game data in to data for this level
	AccumulateGameData();

	if ( m_bRoundActive )
	{
		m_bRoundActive = false;
		m_iRoundsPlayed++;
	}

	if ( m_bServerShutdown )
	{
		StopListeningForAllEvents();
	}

	m_bServerShutdown = false;
	m_bRoundActive = false;
	m_iRoundsPlayed = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Resets all stats for this player
//-----------------------------------------------------------------------------
void CTFGameStats::ResetPlayerStats( CTFPlayer *pPlayer )
{
	int iPlayerIndex = pPlayer->entindex();
	
	if ( !IsIndexIntoPlayerArrayValid(iPlayerIndex) )
		return;

	PlayerStats_t &stats = m_aPlayerStats[iPlayerIndex];
	// reset the stats on this player
	stats.Reset();
	// reset the matrix of who killed whom with respect to this player
	ResetKillHistory( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: Resets the kill history for this player
//-----------------------------------------------------------------------------
void CTFGameStats::ResetKillHistory( CTFPlayer *pPlayer )
{
	int iPlayerIndex = pPlayer->entindex();
	
	if ( !IsIndexIntoPlayerArrayValid(iPlayerIndex) )
		return;

	// for every other player, set all all the kills with respect to this player to 0
	for ( int i = 0; i < ARRAYSIZE( m_aPlayerStats ); i++ )
	{
		PlayerStats_t &statsOther = m_aPlayerStats[i];
		statsOther.statsKills.iNumKilled[iPlayerIndex] = 0;
		statsOther.statsKills.iNumKilledBy[iPlayerIndex] = 0;
		statsOther.statsKills.iNumKilledByUnanswered[iPlayerIndex] = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Resets per-round stats for all players
//-----------------------------------------------------------------------------
void CTFGameStats::ResetRoundStats()
{
	for ( int i = 0; i < ARRAYSIZE( m_aPlayerStats ); i++ )
	{		
		m_aPlayerStats[i].statsCurrentRound.Reset();
	}
	m_currentRoundRed.Reset();
	m_currentRoundBlue.Reset();

	IGameEvent *event = gameeventmanager->CreateEvent( "stats_resetround" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Increments specified stat for specified player by specified amount
//-----------------------------------------------------------------------------
void CTFGameStats::IncrementStat( CTFPlayer *pPlayer, TFStatType_t statType, int iValue )
{
	PlayerStats_t &stats = m_aPlayerStats[pPlayer->entindex()];
	stats.statsCurrentLife.m_iStat[statType] += iValue;
	stats.statsCurrentRound.m_iStat[statType] += iValue;
	stats.mapStatsCurrentLife.m_iStat[statType] += iValue;
	stats.mapStatsCurrentRound.m_iStat[statType] += iValue;
	stats.statsAccumulated.m_iStat[statType] += iValue;
	stats.mapStatsAccumulated.m_iStat[statType] += iValue;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::SendStatsToPlayer( CTFPlayer *pPlayer, bool bIsAlive )
{
	PlayerStats_t &stats = m_aPlayerStats[pPlayer->entindex()];

	// set the play time for the round
	stats.statsCurrentLife.m_iStat[TFSTAT_PLAYTIME] = (int) gpGlobals->curtime - pPlayer->GetSpawnTime();
	stats.statsCurrentLife.m_iStat[TFSTAT_POINTSSCORED] = TFGameRules()->CalcPlayerScore( &stats.statsCurrentLife, pPlayer );
	stats.statsCurrentLife.m_iStat[TFSTAT_MAXSENTRYKILLS] = pPlayer->GetMaxSentryKills();
	stats.mapStatsCurrentLife.m_iStat[TFMAPSTAT_PLAYTIME] = (int) gpGlobals->curtime - pPlayer->GetSpawnTime();

	// make a bit field of all the stats we want to send (all with non-zero values)
	int iStat;
	int iSendBits = 0;
	for ( iStat = TFSTAT_FIRST; iStat <= TFSTAT_LAST; iStat++ )
	{
		if ( stats.statsCurrentLife.m_iStat[iStat] > 0 )
		{
			iSendBits |= ( 1 << ( iStat - TFSTAT_FIRST ) );
		}
	}

	iStat = TFSTAT_FIRST;
	CSingleUserRecipientFilter filter( pPlayer );
	filter.MakeReliable();
	UserMessageBegin( filter, "PlayerStatsUpdate" );
	WRITE_BYTE( pPlayer->GetPlayerClass()->GetClassIndex() );		// write the class
	WRITE_BYTE( bIsAlive );											// write if the player is currently alive
	WRITE_LONG( iSendBits );										// write the bit mask of which stats follow in the message

	// write all the non-zero stats according to the bit mask
	while ( iSendBits > 0 )
	{
		if ( iSendBits & 1 )
		{
			WRITE_LONG( stats.statsCurrentLife.m_iStat[iStat] );
		}
		iSendBits >>= 1;
		iStat ++;
	}
	MessageEnd();

	AccumulateAndResetPerLifeStats( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::AccumulateAndResetPerLifeStats( CTFPlayer *pPlayer )
{
	int iClass = pPlayer->GetPlayerClass()->GetClassIndex();

	PlayerStats_t &stats = m_aPlayerStats[pPlayer->entindex()];

	// add score from previous life and reset current life stats
	int iScore = TFGameRules()->CalcPlayerScore( &stats.statsCurrentLife, pPlayer );
	if ( m_reportedStats.m_pCurrentGame != NULL )
	{
		m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iScore += iScore;
	}
	stats.statsCurrentRound.m_iStat[TFSTAT_POINTSSCORED] += iScore;
	stats.statsAccumulated.m_iStat[TFSTAT_POINTSSCORED] += iScore;
	stats.statsCurrentLife.Reset();	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerConnected( CBasePlayer *pPlayer )
{
	ResetPlayerStats( ToTFPlayer( pPlayer ) );

	PlayerStats_t &stats = CTF_GameStats.m_aPlayerStats[pPlayer->entindex()];
	stats.iConnectTime = GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerDisconnectedTF( CTFPlayer *pTFPlayer )
{
	if ( !pTFPlayer )
		return;

	ResetPlayerStats( pTFPlayer );

	if ( pTFPlayer->IsAlive() )
	{
		int iClass = pTFPlayer->GetPlayerClass()->GetClassIndex();
		if ( m_reportedStats.m_pCurrentGame != NULL )
		{
			m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iTotalTime += (int) ( gpGlobals->curtime - pTFPlayer->GetSpawnTime() );
		}
	}

	TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pTFPlayer->GetTeamNumber() );
	if ( round )
	{
		round->m_Summary.iTeamQuit++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerChangedClass( CTFPlayer *pPlayer, int iOldClass, int iNewClass )
{
	if ( iNewClass >= TF_FIRST_NORMAL_CLASS && iNewClass <= TF_LAST_NORMAL_CLASS )
	{
		if ( m_reportedStats.m_pCurrentGame )
		{
			m_reportedStats.m_pCurrentGame->m_aClassStats[iNewClass].iClassChanges += 1;
		}
		IncrementStat( pPlayer, TFSTAT_CLASSCHANGES, 1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerSpawned( CTFPlayer *pPlayer )
{	
	// if player is spawning as a member of valid team, increase the spawn count for his class
	int iTeam = pPlayer->GetTeamNumber();
	int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
	if ( TEAM_UNASSIGNED != iTeam && TEAM_SPECTATOR != iTeam )
	{
		if ( m_reportedStats.m_pCurrentGame != NULL )
		{
			m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iSpawns++;
		}
	}

	TF_Gamestats_LevelStats_t *map = m_reportedStats.m_pCurrentGame;
	if ( !map )
		return;

	// calculate peak player count on each team
	for ( iTeam = FIRST_GAME_TEAM; iTeam < TF_TEAM_COUNT; iTeam++ )
	{
		int iPlayerCount = GetGlobalTeam( iTeam )->GetNumPlayers();
		if ( iPlayerCount > map->m_iPeakPlayerCount[iTeam] )
		{
			map->m_iPeakPlayerCount[iTeam] = iPlayerCount;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------	
void CTFGameStats::Event_PlayerForceRespawn( CTFPlayer *pPlayer )
{
	if ( pPlayer->IsAlive() && !TFGameRules()->PrevRoundWasWaitingForPlayers() )
	{		
		// send stats to player
		SendStatsToPlayer( pPlayer, true );

		// if player is alive before respawn, add time from this life to class stat
		int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
		if ( m_reportedStats.m_pCurrentGame != NULL )
		{
			m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iTotalTime += (int) ( gpGlobals->curtime - pPlayer->GetSpawnTime() );
		}
	}

	AccumulateAndResetPerLifeStats( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerLeachedHealth( CTFPlayer *pPlayer, bool bDispenserHeal, float amount ) 
{
	// make sure value is sane
	Assert( amount >= 0 );
	Assert( amount < 1000 );

	if ( !bDispenserHeal )
	{
		// If this was a heal by enemy medic and the first such heal that the server is aware of for this player,
		// send an achievement event to client.  On the client, it will award achievement if player doesn't have it yet
		PlayerStats_t &stats = m_aPlayerStats[pPlayer->entindex()];
		if ( 0 == stats.statsAccumulated.m_iStat[TFSTAT_HEALTHLEACHED] )
		{
			pPlayer->AwardAchievement( ACHIEVEMENT_TF_GET_HEALED_BYENEMY );
		}
	}

	IncrementStat( pPlayer, TFSTAT_HEALTHLEACHED, (int) amount );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
TF_Gamestats_RoundStats_t* CTFGameStats::GetRoundStatsForTeam( int iTeamNumber )
{
	if ( iTeamNumber == TF_TEAM_BLUE )
		return &m_currentRoundBlue;
	else if ( iTeamNumber == TF_TEAM_RED )
		return &m_currentRoundRed;
	else
		return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerHealedOther( CTFPlayer *pPlayer, float amount ) 
{
	// make sure value is sane
	int iAmount = (int) amount;
	Assert( iAmount >= 0 );
	Assert( iAmount <= 1000 );
	if ( iAmount < 0 || iAmount > 1000 )
	{
		DevMsg( "CTFGameStats: bogus healing value of %d reported, ignoring\n", iAmount );
		return;
	}
	IncrementStat( pPlayer, TFSTAT_HEALING, (int) amount );

	TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pPlayer->GetTeamNumber() );
	if ( round )
	{
		round->m_Summary.iHealingDone += amount;
	}
}

//-----------------------------------------------------------------------------
// Purpose:  How much health effects like mad milk generate - awarded to the provider
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerHealedOtherAssist( CTFPlayer *pPlayer, float amount ) 
{
	Assert ( pPlayer );

	// make sure value is sane
	int iAmount = (int) amount;
	Assert( iAmount >= 0 );
	Assert( iAmount <= 1000 );
	if ( iAmount < 0 || iAmount > 1000 )
	{
		DevMsg( "CTFGameStats: bogus healing value of %d reported, ignoring\n", iAmount );
		return;
	}
	IncrementStat( pPlayer, TFSTAT_HEALING_ASSIST, (int) amount );
}

//-----------------------------------------------------------------------------
// Purpose:  Raw damage blocked due to effects like invuln, projectile shields, etc
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerBlockedDamage( CTFPlayer *pPlayer, int nAmount ) 
{
	Assert( pPlayer && nAmount > 0 && nAmount < 3000 );
	if ( nAmount < 0 || nAmount > 3000 )
	{
		DevMsg( "CTFGameStats: bogus blocked damage value of %d reported, ignoring\n", nAmount );
		return;
	}
	IncrementStat( pPlayer, TFSTAT_DAMAGE_BLOCKED, nAmount );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_AssistKill( CTFPlayer *pAttacker, CBaseEntity *pVictim )
{
	// increment player's stat
	IncrementStat( pAttacker, TFSTAT_KILLASSISTS, 1 );
	// increment reported class stats
	int iClass = pAttacker->GetPlayerClass()->GetClassIndex();
	if ( m_reportedStats.m_pCurrentGame != NULL )
	{
		m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iAssists++;

		TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pAttacker->GetTeamNumber() );
		if ( round )
		{
			round->m_Summary.iAssists++;
		}
	}

	if ( pVictim->IsPlayer() )
	{
		// keep track of how many times every player kills every other player
		CTFPlayer *pPlayerVictim = ToTFPlayer( pVictim );
		TrackKillStats( pAttacker, pPlayerVictim );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerInvulnerable( CTFPlayer *pPlayer ) 
{
	IncrementStat( pPlayer, TFSTAT_INVULNS, 1 );

	TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pPlayer->GetTeamNumber() );
	if ( round )
	{
		round->m_Summary.iInvulns++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerCreatedBuilding( CTFPlayer *pPlayer, CBaseObject *pBuilding )
{
	// sappers are buildings from the code's point of view but not from the player's, don't count them
	CObjectSapper *pSapper = dynamic_cast<CObjectSapper *>( pBuilding );
	if ( pSapper )
		return;

	IncrementStat( pPlayer, TFSTAT_BUILDINGSBUILT, 1 );

	TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pPlayer->GetTeamNumber() );
	if ( round )
	{
		round->m_Summary.iBuildingsBuilt++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerDestroyedBuilding( CTFPlayer *pPlayer, CBaseObject *pBuilding )
{
	// sappers are buildings from the code's point of view but not from the player's, don't count them
	CObjectSapper *pSapper = dynamic_cast<CObjectSapper *>( pBuilding );
	if ( pSapper )
		return;

	IncrementStat( pPlayer, TFSTAT_BUILDINGSDESTROYED, 1 );

	TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pPlayer->GetTeamNumber() );
	if ( round )
	{
		round->m_Summary.iBuildingsDestroyed++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_AssistDestroyBuilding( CTFPlayer *pPlayer, CBaseObject *pBuilding )
{
	// sappers are buildings from the code's point of view but not from the player's, don't count them
	CObjectSapper *pSapper = dynamic_cast<CObjectSapper *>( pBuilding );
	if ( pSapper )
		return;

	IncrementStat( pPlayer, TFSTAT_KILLASSISTS, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_Headshot( CTFPlayer *pKiller, bool bBowShot )
{
	IncrementStat( pKiller, TFSTAT_HEADSHOTS, 1 );
	if ( bBowShot ) // Extra points for a bow shot.
	{
		IncrementStat( pKiller, TFSTAT_BONUS_POINTS, 5 ); // Extra point.
	}

	TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pKiller->GetTeamNumber() );
	if ( round )
	{
		round->m_Summary.iHeadshots++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_Backstab( CTFPlayer *pKiller )
{
	IncrementStat( pKiller, TFSTAT_BACKSTABS, 1 );

	TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pKiller->GetTeamNumber() );
	if ( round )
	{
		round->m_Summary.iBackstabs++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerAwardBonusPoints( CTFPlayer *pPlayer, CBaseEntity *pSource, int nCount )
{
	IncrementStat( pPlayer, TFSTAT_BONUS_POINTS, nCount );

	// This event ends up drawing a combattext number
	if ( pPlayer && pSource )
	{
		if ( nCount >= 10 )
		{
			CSingleUserRecipientFilter filter( pPlayer );
			filter.MakeReliable();
			UserMessageBegin( filter, "PlayerBonusPoints" );
			WRITE_BYTE( nCount );
			WRITE_BYTE( pPlayer->entindex() );
			WRITE_BYTE( pSource->entindex() );
			MessageEnd();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerHealthkitPickup( CTFPlayer *pTFPlayer )
{
	IncrementStat( pTFPlayer, TFSTAT_HEALTHKITS, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerAmmokitPickup( CTFPlayer *pTFPlayer )
{
	IncrementStat( pTFPlayer, TFSTAT_AMMOKITS, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerUsedTeleport( CTFPlayer *pTeleportOwner, CTFPlayer *pTeleportingPlayer )
{
	// We don't count the builder's teleports
	if ( pTeleportOwner && pTeleportOwner != pTeleportingPlayer )
	{
		IncrementStat( pTeleportOwner, TFSTAT_TELEPORTS, 1 );

		TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pTeleportOwner->GetTeamNumber() );
		if ( round )
		{
			round->m_Summary.iTeleports++;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerFiredWeapon( CTFPlayer *pPlayer, bool bCritical ) 
{
	// If normal gameplay state, track weapon stats. 
	if ( TFGameRules()->State_Get() == GR_STATE_RND_RUNNING )
	{
		CTFWeaponBase *pTFWeapon = pPlayer->GetActiveTFWeapon();
		if ( pTFWeapon )
		{
			// record shots fired in reported per-weapon stats
			int iWeaponID = pTFWeapon->GetWeaponID();

			if ( m_reportedStats.m_pCurrentGame != NULL )
			{
				TF_Gamestats_WeaponStats_t *pWeaponStats = &m_reportedStats.m_pCurrentGame->m_aWeaponStats[iWeaponID];
				pWeaponStats->iShotsFired++;
				if ( bCritical )
				{
					pWeaponStats->iCritShotsFired++;
					TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pPlayer->GetTeamNumber() );
					if ( round )
					{
						round->m_Summary.iCrits++;
					}
					IncrementStat( pPlayer, TFSTAT_CRITS, 1 );
				}
			}

			// need a better place to do this
			pPlayer->OnMyWeaponFired( pTFWeapon );
		}
	}

	IncrementStat( pPlayer, TFSTAT_SHOTS_FIRED, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerDamage( CBasePlayer *pBasePlayer, const CTakeDamageInfo &info, int iDamageTaken )
{
	// defensive guard against insanely huge damage values that apparently get into the stats system once in a while -- ignore insane values
	const int INSANE_PLAYER_DAMAGE = 1500;

	if ( sv_cheats && !sv_cheats->GetBool() )
	{
		Assert( iDamageTaken >= 0 );
	}
	if ( ( iDamageTaken < 0 ) || ( iDamageTaken > INSANE_PLAYER_DAMAGE ) )
		return;

	CObjectSentrygun *pSentry = NULL;
	CTFPlayer *pTarget = ToTFPlayer( pBasePlayer );
	CTFPlayer *pAttacker = ToTFPlayer( info.GetAttacker() );
	if ( !pAttacker )
	{
		pSentry = dynamic_cast< CObjectSentrygun * >( info.GetAttacker() );
		if ( !pSentry )
			return;

		pAttacker = pSentry->GetOwner();
	}
	// don't count damage to yourself
	if ( pTarget == pAttacker )
		return;

	if ( pAttacker )
	{
		IncrementStat( pAttacker, TFSTAT_DAMAGE, iDamageTaken );

		if ( info.GetDamageType() & (DMG_BURN | DMG_IGNITE) )
		{
			IncrementStat( pAttacker, TFSTAT_FIREDAMAGE, iDamageTaken );
		}
		if ( info.GetDamageType() & DMG_BLAST )
		{
			IncrementStat( pAttacker, TFSTAT_BLASTDAMAGE, iDamageTaken );
		}
		// Ranged stats
		if ( !( info.GetDamageType() & DMG_MELEE ) )
		{
			IncrementStat( pAttacker, TFSTAT_DAMAGE_RANGED, iDamageTaken );

			if ( info.GetDamageType() & DMG_CRITICAL )
			{
				IncrementStat( pAttacker, TFSTAT_DAMAGE_RANGED_CRIT_RANDOM, iDamageTaken );
			}
		}

		TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pAttacker->GetTeamNumber() );
		if ( round )
		{
			round->m_Summary.iDamageDone += iDamageTaken;
		}
	}

	if ( pTarget )
	{
		IncrementStat( pTarget, TFSTAT_DAMAGETAKEN, iDamageTaken );
	}

	TF_Gamestats_LevelStats_t::PlayerDamageLump_t damage;
	Vector killerOrg(0, 0, 0);

	// set the location where the target was hit
	const Vector &org = pTarget ? pTarget->GetAbsOrigin() : vec3_origin;
	damage.nTargetPosition[ 0 ] = static_cast<int>( org.x );
	damage.nTargetPosition[ 1 ] = static_cast<int>( org.y );
	damage.nTargetPosition[ 2 ] = static_cast<int>( org.z );

	// set the class of the attacker
	CBaseEntity *pInflictor = info.GetInflictor();
	CBasePlayer *pScorer = TFGameRules()->GetDeathScorer( pAttacker, pInflictor, pTarget );

	if ( !pSentry )
	{
		pSentry = dynamic_cast< CObjectSentrygun * >( pInflictor );
	}

	if ( pSentry != NULL )
	{
		killerOrg = pSentry->GetAbsOrigin();
		damage.iAttackClass = TF_CLASS_ENGINEER;
		damage.iWeapon = ( info.GetDamageType() & DMG_BLAST ) ? TF_WEAPON_SENTRY_ROCKET : TF_WEAPON_SENTRY_BULLET;
	} 
	else if ( dynamic_cast<CObjectDispenser *>( pInflictor ) )
	{
		damage.iAttackClass = TF_CLASS_ENGINEER;
		damage.iWeapon = TF_WEAPON_DISPENSER;
	}
	else
	{
		CTFPlayer *pTFAttacker = ToTFPlayer( pScorer );
		if ( pTFAttacker )
		{
			CTFPlayerClass *pAttackerClass = pTFAttacker->GetPlayerClass();
			damage.iAttackClass = ( !pAttackerClass ) ? TF_CLASS_UNDEFINED : pAttackerClass->GetClassIndex();
			killerOrg = pTFAttacker->GetAbsOrigin();
		}
		else
		{
			damage.iAttackClass = TF_CLASS_UNDEFINED;
			killerOrg = org;
		}

		// find the weapon the killer used
		damage.iWeapon = GetWeaponFromDamage( info );
	}

	// If normal gameplay state, track weapon stats. 
	if ( ( TFGameRules()->State_Get() == GR_STATE_RND_RUNNING ) && ( damage.iWeapon != TF_WEAPON_NONE  ) )
	{
		// record hits & damage in reported per-weapon stats
		if ( m_reportedStats.m_pCurrentGame != NULL )
		{
			TF_Gamestats_WeaponStats_t *pWeaponStats = &m_reportedStats.m_pCurrentGame->m_aWeaponStats[damage.iWeapon];
			pWeaponStats->iHits++;
			pWeaponStats->iTotalDamage += iDamageTaken;

			// Try and figure out where the damage is coming from
			Vector vecDamageOrigin = info.GetReportedPosition();
			// If we didn't get an origin to use, try using the attacker's origin
			if ( vecDamageOrigin == vec3_origin )
			{
				if ( pSentry )
				{
					vecDamageOrigin = pSentry->GetAbsOrigin();
				}
				else
				{
					vecDamageOrigin = killerOrg;
				}					
			}
			if ( vecDamageOrigin != vec3_origin )
			{
				pWeaponStats->iHitsWithKnownDistance++;
				int iDistance = (int) vecDamageOrigin.DistTo( pBasePlayer->GetAbsOrigin() );
				//				Msg( "Damage distance: %d\n", iDistance );
				pWeaponStats->iTotalDistance += iDistance;
			}
		}
	}

	Assert( damage.iAttackClass != TF_CLASS_UNDEFINED );

	// record the time the damage occurred
	damage.fTime = gpGlobals->curtime;

	// store the attacker's position
	damage.nAttackerPosition[ 0 ] = static_cast<int>( killerOrg.x );
	damage.nAttackerPosition[ 1 ] = static_cast<int>( killerOrg.y );
	damage.nAttackerPosition[ 2 ] = static_cast<int>( killerOrg.z );

	// set the class of the target
	CTFPlayer *pTFPlayer = ToTFPlayer( pTarget );
	CTFPlayerClass *pTargetClass = ( pTFPlayer ) ? pTFPlayer->GetPlayerClass() : NULL;
	damage.iTargetClass = ( !pTargetClass ) ? TF_CLASS_UNDEFINED : pTargetClass->GetClassIndex();

	Assert( damage.iTargetClass != TF_CLASS_UNDEFINED );

	// record the damage done
	damage.iDamage = info.GetDamage();

	// record if it was a crit
	damage.iCrit = ( ( info.GetDamageType() & DMG_CRITICAL ) != 0 );

	// record if it was a kill
	damage.iKill = ( pTarget->GetHealth() <= 0 );

	// add it to the list of damages
	if ( m_reportedStats.m_pCurrentGame != NULL )
	{
		//m_reportedStats.m_pCurrentGame->m_aPlayerDamage.AddToTail( damage );
		m_reportedStats.m_pCurrentGame->m_bIsRealServer = true;
	}	
}

//-----------------------------------------------------------------------------
// Purpose:  How much damage effects like jarate add - awarded to the provider
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerDamageAssist( CBasePlayer *pProvider, int iBonusDamage )
{
	Assert( pProvider );

	const int INSANE_PLAYER_DAMAGE = 5000;
	Assert( iBonusDamage >= 0 );
	Assert( iBonusDamage <= INSANE_PLAYER_DAMAGE );
	if ( iBonusDamage < 0 || iBonusDamage > INSANE_PLAYER_DAMAGE )
		return;

	CTFPlayer *pTFProvider = ToTFPlayer( pProvider );
	if ( pTFProvider )
	{
		IncrementStat( pTFProvider, TFSTAT_DAMAGE_ASSIST, iBonusDamage );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerSuicide( CBasePlayer *pPlayer )
{
	CTFPlayer *pTFPlayer = dynamic_cast<CTFPlayer*>( pPlayer );
	if ( pTFPlayer )
	{
		IncrementStat( pTFPlayer, TFSTAT_SUICIDES, 1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerKilledOther( CBasePlayer *pAttacker, CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	// This also gets called when the victim is a building.  That gets tracked separately as building destruction, don't count it here
	if ( !pVictim->IsPlayer() )
		return;

	CTFPlayer *pPlayerAttacker = static_cast< CTFPlayer * >( pAttacker );

	IncrementStat( pPlayerAttacker, TFSTAT_KILLS, 1 );

	// keep track of how many times every player kills every other player
	CTFPlayer *pPlayerVictim = ToTFPlayer( pVictim );
	TrackKillStats( pAttacker, pPlayerVictim );

	int iClass = pPlayerAttacker->GetPlayerClass()->GetClassIndex();
	if ( m_reportedStats.m_pCurrentGame != NULL )
	{
		m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iKills++;

		TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pAttacker->GetTeamNumber() );
		if ( round )
		{
			round->m_Summary.iKills++;
		}
	}

	// Scouts get additional points for killing medics that were actively healing.
	if ( (iClass == TF_CLASS_SCOUT) && pPlayerVictim && (pPlayerVictim->GetPlayerClass()->GetClassIndex() == TF_CLASS_MEDIC) )
	{
		// Determine if the medic was using their (now holstered) heal gun.
		CWeaponMedigun *pMedigun = (CWeaponMedigun *) pPlayerVictim->Weapon_OwnsThisID( TF_WEAPON_MEDIGUN );
		if ( pMedigun && pMedigun->m_bWasHealingBeforeDeath )
		{
			IncrementStat( pPlayerAttacker, TFSTAT_BONUS_POINTS, 10 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_RoundStart()
{
	m_iGameEndReason = 0;
	m_currentRoundRed.Reset();
	m_currentRoundBlue.Reset();
	m_bRoundActive = true;
	m_iKillCount = 0;
	m_iPlayerUpdates = 0;
	m_currentRoundRed.m_iRoundStartTime = GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch();

	m_rdStats.Clear();
	m_passtimeStats.Clear();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_RoundEnd( int iWinningTeam, bool bFullRound, float flRoundTime, bool bWasSuddenDeathWin )
{
	TF_Gamestats_LevelStats_t *map = m_reportedStats.m_pCurrentGame;
	Assert( map );
	if ( !map )
		return;

	m_reportedStats.m_pCurrentGame->m_Header.m_iTotalTime += (int) flRoundTime;
	m_reportedStats.m_pCurrentGame->m_flRoundStartTime = gpGlobals->curtime;

//	if ( !bFullRound )
//		return;

	map->m_Header.m_iRoundsPlayed++;
	switch ( iWinningTeam )
	{
	case TF_TEAM_RED:
		map->m_Header.m_iRedWins++;
		if ( bWasSuddenDeathWin )
		{
			map->m_Header.m_iRedSuddenDeathWins++;
		}
		break;
	case TF_TEAM_BLUE:
		map->m_Header.m_iBlueWins++;
		if ( bWasSuddenDeathWin )
		{
			map->m_Header.m_iBlueSuddenDeathWins++;
		}
		break;
	case TEAM_UNASSIGNED:
		map->m_Header.m_iStalemates++;
		break;
	default:
		Assert( false );
		break;
	}

	// Determine which control point was last captured
	if ( TFGameRules() && ( TFGameRules()->GetGameType() == TF_GAMETYPE_CP ) )
	{
		int iLastCP = TFGameRules()->GetLastCapPointChanged();
		if ( iLastCP >= 0 && iLastCP <= MAX_CONTROL_POINTS )
		{
			map->m_Header.m_iLastCapChangedInRound[iLastCP]++;
		}
	}

	// add current game data in to data for this level
	AccumulateGameData();

	m_bRoundActive = false;
	m_iRoundsPlayed++;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_GameEnd()
{
	// Send a stats update to all players who are still alive.  (Dead one have already
	// received theirs when they died.)
	for( int iPlayerIndex = 1 ; iPlayerIndex <= MAX_PLAYERS; iPlayerIndex++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
		if ( pPlayer && pPlayer->IsConnected() && pPlayer->IsAlive() )
		{
			SendStatsToPlayer( pPlayer, true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerCapturedPoint( CTFPlayer *pPlayer )
{
	// increment player stats
	IncrementStat( pPlayer, TFSTAT_CAPTURES, 1 );
	// increment reported stats
	int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
	if ( m_reportedStats.m_pCurrentGame != NULL )
	{
		m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iCaptures++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerReturnedFlag( CTFPlayer *pPlayer )
{
	// increment player stats
	IncrementStat( pPlayer, TFSTAT_FLAGRETURNS, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerDefendedPoint( CTFPlayer *pPlayer )
{
	IncrementStat( pPlayer, TFSTAT_DEFENSES, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerDominatedOther( CTFPlayer *pAttacker )
{
	IncrementStat( pAttacker, TFSTAT_DOMINATIONS, 1 );

	TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pAttacker->GetTeamNumber() );
	if ( round )
	{
		round->m_Summary.iDominations++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerRevenge( CTFPlayer *pAttacker )
{
	IncrementStat( pAttacker, TFSTAT_REVENGE, 1 );

	TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pAttacker->GetTeamNumber() );
	if ( round )
	{
		round->m_Summary.iRevenges++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerKilled( CBasePlayer *pPlayer, const CTakeDamageInfo &info )
{
	Assert( pPlayer );
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

	IncrementStat( pTFPlayer, TFSTAT_DEATHS, 1 );
	SendStatsToPlayer( pTFPlayer, false );

	// TF_Gamestats_LevelStats_t::PlayerDeathsLump_t death;
	Vector killerOrg;

	// set the location where the target died
	const Vector &org = pPlayer->GetAbsOrigin();
	// death.nPosition[ 0 ] = static_cast<int>( org.x );
	// death.nPosition[ 1 ] = static_cast<int>( org.y );
	// death.nPosition[ 2 ] = static_cast<int>( org.z );

	// set the class of the attacker
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CTFPlayer *pScorer = ToTFPlayer( TFGameRules()->GetDeathScorer( pKiller, pInflictor, pPlayer ) );

	if ( pInflictor && pInflictor->IsBaseObject() && dynamic_cast< CObjectSentrygun * >( pInflictor ) != NULL )
	{
		killerOrg = pInflictor->GetAbsOrigin();
	}
	else
	{		
		if ( pScorer )
		{
			// CTFPlayerClass *pAttackerClass = pScorer->GetPlayerClass();
			// death.iAttackClass = ( !pAttackerClass ) ? TF_CLASS_UNDEFINED : pAttackerClass->GetClassIndex();
			killerOrg = pScorer->GetAbsOrigin();
		}
		else
		{
			// death.iAttackClass = TF_CLASS_UNDEFINED;
			killerOrg = org;
		}
	}

	// set the class of the target
	// CTFPlayerClass *pTargetClass = ( pTFPlayer ) ? pTFPlayer->GetPlayerClass() : NULL;
	// death.iTargetClass = ( !pTargetClass ) ? TF_CLASS_UNDEFINED : pTargetClass->GetClassIndex();

	// find the weapon the killer used
	// death.iWeapon = GetWeaponFromDamage( info );

	// calculate the distance to the killer
	// death.iDistance = static_cast<unsigned short>( ( killerOrg - org ).Length() );

	// add it to the list of deaths
	TF_Gamestats_LevelStats_t *map = m_reportedStats.m_pCurrentGame;
	if ( map )
	{
		//const int MAX_REPORTED_DEATH_COORDS = 2000;	// limit list of death coords reported so it doesn't grow unbounded.
		//if ( map->m_aPlayerDeaths.Count() < MAX_REPORTED_DEATH_COORDS )
		//{
		//	map->m_aPlayerDeaths.AddToTail( death );
		//}

		int iClass = ToTFPlayer( pPlayer )->GetPlayerClass()->GetClassIndex();

		if ( m_reportedStats.m_pCurrentGame != NULL )
		{
			m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iDeaths++;
			m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iTotalTime += (int) ( gpGlobals->curtime - pTFPlayer->GetSpawnTime() );

			TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pPlayer->GetTeamNumber() );
			if ( round )
			{
				round->m_Summary.iDeaths++;
			}
			if ( pKiller != NULL && pKiller == pPlayer )
			{
				TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pKiller->GetTeamNumber() );
				if ( round )
				{
					round->m_Summary.iSuicides++;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Event handler
//-----------------------------------------------------------------------------
void CTFGameStats::FireGameEvent( IGameEvent *event )
{
	const char *pEventName = event->GetName();

	if ( Q_strcmp( "tf_game_over", pEventName ) == 0 )
	{
		StoreGameEndReason( event->GetString( "reason" ) );

		Event_GameEnd();
	}
	else if ( Q_strcmp( "teamplay_game_over", pEventName ) == 0 )
	{
		StoreGameEndReason( event->GetString( "reason" ) );

		Event_GameEnd();
	}
	else if ( Q_strcmp( "teamplay_round_start", pEventName ) == 0 )
	{
		Event_RoundStart();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds data from current game into accumulated data for this level.
//-----------------------------------------------------------------------------
void CTFGameStats::StoreGameEndReason( const char* reason )
{
	if ( Q_strcmp( reason, "Reached Time Limit" ) == 0 )
	{
		m_iGameEndReason = RE_TIME_LIMIT;
	}
	else if ( Q_strcmp( reason, "Reached Win Limit" ) == 0 )
	{
		m_iGameEndReason = RE_WIN_LIMIT;
	}
	else if ( Q_strcmp( reason, "Reached Win Difference Limit" ) == 0 )
	{
		m_iGameEndReason = RE_WIN_DIFF_LIMIT;
	}
	else if ( Q_strcmp( reason, "Reached Round Limit" ) == 0 )
	{
		m_iGameEndReason = RE_ROUND_LIMIT;
	}
	else if ( Q_strcmp( reason, "NextLevel CVAR" ) == 0 )
	{
		m_iGameEndReason = RE_NEXT_LEVEL_CVAR;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds data from current game into accumulated data for this level.
//-----------------------------------------------------------------------------
void CTFGameStats::AccumulateGameData()
{
	// find or add a bucket for this level
	TF_Gamestats_LevelStats_t *map = m_reportedStats.FindOrAddMapStats( STRING( gpGlobals->mapname ) );
	// get current game data
	TF_Gamestats_LevelStats_t *game = m_reportedStats.m_pCurrentGame;
	if ( !map || !game )
		return;

	if ( IsRealGameplay( game ) )
	{
		// if this looks like real game play, add it to stats
		map->Accumulate( game );
		m_reportedStats.m_bValidData = true;
	}
	m_currentMap.Accumulate( game ); // Steamworks stats always accumulate.

	ClearCurrentGameData();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGameStats::AccumulateVoteData( void )
{
	if ( !g_voteControllerGlobal || !g_voteControllerRed || !g_voteControllerBlu )
		return;

	if ( !g_pStringTableServerMapCycle )
		return;

	// Find the current map and update playtime
	int iIndex = m_MapsPlaytime.Find( CUtlConstString( STRING( gpGlobals->mapname ) ) );
	if ( iIndex != m_MapsPlaytime.InvalidIndex() )
	{
		TF_Gamestats_LevelStats_t *CurrentGame = m_reportedStats.m_pCurrentGame;
		//Msg( "%s -- old: %i  ", STRING( gpGlobals->mapname ), m_MapsPlaytime[iIndex] );
		m_MapsPlaytime[iIndex] += CurrentGame->m_Header.m_iTotalTime;
		//Msg( "new: %i\n", STRING( gpGlobals->mapname ), m_MapsPlaytime[iIndex] );
	}
}

struct MapNameAndPlaytime_t
{
	const char* szName;
	int nTime;
};

// Returns negative if elem2 > elem1, positive if elem2 < elem1, and zero if elem 1 == elem2
static int __cdecl SortMapPlaytime( const void *elem1, const void *elem2 )
{
	int time1 = static_cast< const MapNameAndPlaytime_t * >( elem1 )->nTime;
	int time2 = static_cast< const MapNameAndPlaytime_t * >( elem2 )->nTime;
	
	if ( time2 < time1 )
		return -1;

	if ( time2 > time1 )
		return 1;
	
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose:  Method used by the vote system to retrieve various data, like map playtime
//-----------------------------------------------------------------------------
bool CTFGameStats::GetVoteData( const char *szIssueName, int nNumOptions, CUtlVector <const char*> &vecNames )
{
	// Feeds lowest playtime maps to the vote system to present as options
	if ( Q_strcmp( szIssueName, "NextLevel" ) == 0 )
	{
		// This can only happen if we don't get any maps from the mapcycle file
		if ( !m_MapsPlaytime.Count() )
			return false;

		vecNames.EnsureCapacity( MIN( nNumOptions, (int) m_MapsPlaytime.Count() ) );

		// What's the next map in the mapcycle? Place that first in the output
		m_szNextMap[0] = '\0';
		CMultiplayRules *pRules = dynamic_cast< CMultiplayRules * >( GameRules() );
		if ( pRules )
		{
			pRules->GetNextLevelName( m_szNextMap, sizeof( m_szNextMap ) );
			if ( m_szNextMap[0] )
			{
				vecNames.AddToTail( m_szNextMap );
			}
		}

		CUtlVector< MapNameAndPlaytime_t > vecMapsAndPlaytime;
		vecMapsAndPlaytime.EnsureCapacity( m_MapsPlaytime.Count() );

		// Feed the maps into a vector for sorting
		FOR_EACH_MAP_FAST( m_MapsPlaytime, iIndex )
		{
			const char *szItemName = m_MapsPlaytime.Key( iIndex ).Get();
			int nItemTime = m_MapsPlaytime.Element( iIndex );
			// Exclude the next map (already added) and the current map (omitted)
			if ( Q_strcmp( szItemName, m_szNextMap ) != 0 &&
				 Q_strcmp( szItemName, STRING( gpGlobals->mapname ) ) != 0 )
			{
				int iVec = vecMapsAndPlaytime.AddToTail();
				vecMapsAndPlaytime[ iVec ].szName = szItemName;
				vecMapsAndPlaytime[ iVec ].nTime = nItemTime;
			}
		}
		qsort( vecMapsAndPlaytime.Base(), vecMapsAndPlaytime.Count(), sizeof( MapNameAndPlaytime_t ), SortMapPlaytime );

		// Copy sorted maps to output until we have got enough options
		FOR_EACH_VEC( vecMapsAndPlaytime, iVec )
		{
			if ( vecNames.Count() >= nNumOptions )
				break;
			vecNames.AddToTail( vecMapsAndPlaytime[ iVec ].szName );
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Clears data for current game
//-----------------------------------------------------------------------------
void CTFGameStats::ClearCurrentGameData()
{
	if ( m_reportedStats.m_pCurrentGame )
	{
		delete m_reportedStats.m_pCurrentGame;
	}
	m_reportedStats.m_pCurrentGame = new TF_Gamestats_LevelStats_t;
}

//-----------------------------------------------------------------------------
// Purpose: Updates the stats of who has killed whom
//-----------------------------------------------------------------------------
void CTFGameStats::TrackKillStats( CBasePlayer *pAttacker, CBasePlayer *pVictim )
{
	int iPlayerIndexAttacker = pAttacker->entindex();
	int iPlayerIndexVictim = pVictim->entindex();
	
	if ( !IsIndexIntoPlayerArrayValid(iPlayerIndexAttacker) || !IsIndexIntoPlayerArrayValid(iPlayerIndexVictim) )
		return;

	PlayerStats_t &statsAttacker = m_aPlayerStats[iPlayerIndexAttacker];
	PlayerStats_t &statsVictim = m_aPlayerStats[iPlayerIndexVictim];

	statsVictim.statsKills.iNumKilledBy[iPlayerIndexAttacker]++;
	statsVictim.statsKills.iNumKilledByUnanswered[iPlayerIndexAttacker]++;
	statsAttacker.statsKills.iNumKilled[iPlayerIndexVictim]++;
	statsAttacker.statsKills.iNumKilledByUnanswered[iPlayerIndexVictim] = 0;
}

struct PlayerStats_t *CTFGameStats::FindPlayerStats( CBasePlayer *pPlayer )
{
	if ( !pPlayer )
		return NULL;

	return &m_aPlayerStats[pPlayer->entindex()];
}

bool CTFGameStats::IsRealGameplay( TF_Gamestats_LevelStats_t *game )
{
	// Sanity-check that this looks like real game play -- must have minimum # of players on both teams,
	// minimum time and some damage to players must have occurred
	if ( tf_stats_nogameplaycheck.GetInt() )
		return true;

	bool bIsRealGameplay = ( 
		( game->m_iPeakPlayerCount[TF_TEAM_RED] >= TFGameRules()->GetStatsMinimumPlayers()  ) &&
		( game->m_iPeakPlayerCount[TF_TEAM_BLUE] >= TFGameRules()->GetStatsMinimumPlayers() ) &&
		( game->m_Header.m_iTotalTime >= TFGameRules()->GetStatsMinimumPlayedTime() ) && ( game->m_bIsRealServer ) 
		);

	return bIsRealGameplay;
}

//-----------------------------------------------------------------------------
// Purpose: //Deprecated
//-----------------------------------------------------------------------------
//static void CC_ListDeaths( const CCommand &args )
//{
//	if ( !UTIL_IsCommandIssuedByServerAdmin() )
//		return;
//
//	Msg( "Command Deprecated");
//
//	//TF_Gamestats_LevelStats_t *map = CTF_GameStats.m_reportedStats.m_pCurrentGame;
//	//if ( !map )
//	//	return;
//
//	//for( int i = 0; i < map->m_aPlayerDeaths.Count(); i++ )
//	//{
//	//	Msg( "%s killed %s with %s at (%d,%d,%d), distance %d\n",
//	//		g_aClassNames[ map->m_aPlayerDeaths[ i ].iAttackClass ],
//	//		g_aClassNames[ map->m_aPlayerDeaths[ i ].iTargetClass ],
//	//		WeaponIdToAlias( map->m_aPlayerDeaths[ i ].iWeapon ), 
//	//		map->m_aPlayerDeaths[ i ].nPosition[ 0 ],
//	//		map->m_aPlayerDeaths[ i ].nPosition[ 1 ],
//	//		map->m_aPlayerDeaths[ i ].nPosition[ 2 ],
//	//		map->m_aPlayerDeaths[ i ].iDistance );
//	//}
//
//	//Msg( "\n---------------------------------\n\n" );
//
//	//for( int i = 0; i < map->m_aPlayerDamage.Count(); i++ )
//	//{
//	//	Msg( "%.2f : %s at (%d,%d,%d) caused %d damage to %s with %s at (%d,%d,%d)%s%s\n",
//	//		map->m_aPlayerDamage[ i ].fTime,
//	//		g_aClassNames[ map->m_aPlayerDamage[ i ].iAttackClass ],
//	//		map->m_aPlayerDamage[ i ].nAttackerPosition[ 0 ],
//	//		map->m_aPlayerDamage[ i ].nAttackerPosition[ 1 ],
//	//		map->m_aPlayerDamage[ i ].nAttackerPosition[ 2 ],
//	//		map->m_aPlayerDamage[ i ].iDamage,
//	//		g_aClassNames[ map->m_aPlayerDamage[ i ].iTargetClass ],
//	//		WeaponIdToAlias( map->m_aPlayerDamage[ i ].iWeapon ), 
//	//		map->m_aPlayerDamage[ i ].nTargetPosition[ 0 ],
//	//		map->m_aPlayerDamage[ i ].nTargetPosition[ 1 ],
//	//		map->m_aPlayerDamage[ i ].nTargetPosition[ 2 ],
//	//		map->m_aPlayerDamage[ i ].iCrit ? ", CRIT!" : "",
//	//		map->m_aPlayerDamage[ i ].iKill ? ", KILL" : ""	);
//	//}
//
//	//Msg( "\n---------------------------------\n\n" );
//	//Msg( "listed %d deaths\n", map->m_aPlayerDeaths.Count() );
//	//Msg( "listed %d damages\n\n", map->m_aPlayerDamage.Count() );
//}
//
//static ConCommand listDeaths("listdeaths", CC_ListDeaths, "lists player deaths", FCVAR_DEVELOPMENTONLY );

CON_COMMAND_F( tf_dumpplayerstats, "Dumps current player stats", FCVAR_DEVELOPMENTONLY )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer && pPlayer->IsConnected() )
		{
			PlayerStats_t &stats = CTF_GameStats.m_aPlayerStats[pPlayer->entindex()];
			Msg( "%s:\n", pPlayer->GetPlayerName() );
			for ( int iStat = TFSTAT_FIRST; iStat <= TFSTAT_LAST; iStat++ )
			{
				Msg( "   Stat %d = %d (round), %d (map)\n", iStat, stats.statsCurrentRound.m_iStat[iStat], stats.statsAccumulated.m_iStat[iStat] );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Records player team activity during a round.
//-----------------------------------------------------------------------------
void CTFGameStats::Event_TeamChange( CTFPlayer* pPlayer, int oldTeam, int newTeam )
{
	if ( pPlayer->IsBot() )
		return;

	CSteamID steamIDForPlayer;
	if ( !pPlayer->GetSteamID( &steamIDForPlayer ) )
		return;

	if ( oldTeam == newTeam )
		return;

//	if ( oldTeam == 0 || newTeam == 0 )
//		return;

#if !defined(NO_STEAM)
	KeyValues* pKVData = new KeyValues( "TF2ServerTeamChanges" );
	pKVData->SetInt( "MapIndex", 0 );
	pKVData->SetInt( "RoundIndex", m_iRoundsPlayed+1 );
//	pKVData->SetInt( "TimeSubmitted", GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch() );

	pKVData->SetString( "OldTeam", ClampedArrayElement( g_aTeamNames, oldTeam ) );
	pKVData->SetString( "NewTeam", ClampedArrayElement( g_aTeamNames, newTeam ) );

	pKVData->SetInt( "ChangeCount", pPlayer->GetTeamChangeCount() );
	pKVData->SetUint64( "AccountIDPlayer", steamIDForPlayer.ConvertToUint64() );
	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Halloween!
//-----------------------------------------------------------------------------
void CTFGameStats::Event_HalloweenBossEvent( uint8 unBossType, uint16 unBossLevel, uint8 unEventType, uint8 unPlayersInvolved, float fElapsedTime )
{
	//if ( !GCClientSystem() )
	//	return;

	//static uint8 unEventCounter = 0;

	//GCSDK::CProtoBufMsg<CMsgHalloween_ServerBossEvent> msg( k_EMsgGC_Halloween_ServerBossEvent );

	//msg.Body().set_event_counter( unEventCounter++ );
	//msg.Body().set_timestamp( CRTime::RTime32TimeCur() );
	//msg.Body().set_boss_type( unBossType );
	//msg.Body().set_boss_level( unBossLevel );
	//msg.Body().set_event_type( unEventType );
	//msg.Body().set_players_involved( unPlayersInvolved );
	//msg.Body().set_elapsed_time( fElapsedTime );

	//GCClientSystem()->BSendMessage( msg );
}

//-----------------------------------------------------------------------------
#undef min
#undef max
struct PasstimeHistogramStats
{
	double min, max, mean, median, mode, stdev;
	PasstimeHistogramStats() : min(0), max(0), mean(0), median(0), mode(0), stdev(0) {}
};

static int qsort_ascending_uint16( const void *a, const void *b )
{
	return *((uint16*)b) - *((uint16*)a);
}

template<int TMaxSamples>
static PasstimeHistogramStats Passtime_SampleStats( const uint16 (&samples)[TMaxSamples], uint16 iSampleCount )
{
	PasstimeHistogramStats result;
	if ( (iSampleCount <= 1) || (iSampleCount > TMaxSamples) )
	{
		return result;
	}

	// mode is useless, so don't bother

	// sort for median
	qsort( (void*) &samples[0], iSampleCount, sizeof(samples[0]), &qsort_ascending_uint16 );

	//
	// Sum, Min, Max
	//
	double sum = 0;
	result.min = DBL_MAX;
	result.max = DBL_MIN;
	for ( uint32 i = 0; i < iSampleCount; ++i )
	{
		float s = samples[i];
		sum += s;
		result.min = MIN( s, result.min );
		result.max = MAX( s, result.max );
	}

	//
	// Mean
	//
	result.mean = (double)sum / (double)iSampleCount;

	//
	// Median
	//
	result.median = samples[ iSampleCount / 2 ]; // close enough

	//
	// Stdev
	//
	for ( uint32 i = 0; i < iSampleCount; ++i )
	{
		double s = samples[i];
		result.stdev += (s - result.mean) * (s - result.mean);
	}
	result.stdev = sqrt( result.stdev / ((double)(iSampleCount - 1)) );

	return result;
}

template<int TBinCount>
static PasstimeHistogramStats Passtime_HistogramStats( const uint32 (&hist)[TBinCount], uint32 iHistSum, uint32 iSampleCount )
{
	PasstimeHistogramStats result;
	if ( iSampleCount <= 1 )
	{
		return result;
	}

	//
	// Mean
	//
	result.mean = (float)iHistSum / (float)iSampleCount;

	//
	// Min
	//
	for ( uint32 i = 0; i < 256; ++i )
	{
		if ( hist[i] != 0 )
		{
			result.min = i;
			break;
		}
	}

	//
	// Max
	//
	for ( int32 i = 255; i >= 0; --i )
	{
		if ( hist[i] != 0 )
		{
			result.max = i;
			break;
		}
	}

	//
	// Median
	//
	int iMedSample = iSampleCount / 2;
	int iMedian;
	for ( iMedian = 0; iMedian < 256; ++iMedian )
	{
		if ( hist[iMedian] != 0 )
			break;
	}
	while( (iMedSample > 0) && (iMedian < 256) )
	{
		iMedSample -= hist[iMedian];
		++iMedian;
	}
	result.median = iMedian - 1;

	//
	// Mode, stdev
	//
	uint32 iLargestCount = 0;
	result.mode = -1; // wat
	for ( uint32 i = 0; i < 256; ++i )
	{
		uint32 iSampleCount = hist[i];
		for ( uint32 j = 0; j < iSampleCount; ++j )
		{
			// this feels dumb
			result.stdev += (i - result.mean) * (i - result.mean);
		}

		if ( iSampleCount > iLargestCount )
		{
			iLargestCount = iSampleCount;
			result.mode = i;
		}
	}
	result.stdev = sqrt( result.stdev / ((double)iSampleCount - 1) );

	return result;
}
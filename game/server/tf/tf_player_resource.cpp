//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF's custom CPlayerResource
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tf_player.h"
#include "player_resource.h"
#include "tf_player_resource.h"
#include "tf_gamestats.h"
#include "tf_gamerules.h"
#include <coordsize.h>

#define STATS_SEND_FREQUENCY 1.f

// Datatable
IMPLEMENT_SERVERCLASS_ST( CTFPlayerResource, DT_TFPlayerResource )
	SendPropArray3( SENDINFO_ARRAY3( m_iTotalScore ), SendPropInt( SENDINFO_ARRAY( m_iTotalScore ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iMaxHealth ), SendPropInt( SENDINFO_ARRAY( m_iMaxHealth ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iMaxBuffedHealth ), SendPropInt( SENDINFO_ARRAY( m_iMaxBuffedHealth ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iPlayerClass ), SendPropInt( SENDINFO_ARRAY( m_iPlayerClass ), 5, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iActiveDominations ), SendPropInt( SENDINFO_ARRAY( m_iActiveDominations ), 6, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_flNextRespawnTime ), SendPropTime( SENDINFO_ARRAY( m_flNextRespawnTime ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iChargeLevel ), SendPropInt( SENDINFO_ARRAY( m_iChargeLevel ), 8, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iDamage ), SendPropInt( SENDINFO_ARRAY( m_iDamage ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iDamageAssist ), SendPropInt( SENDINFO_ARRAY( m_iDamageAssist ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iDamageBoss ), SendPropInt( SENDINFO_ARRAY( m_iDamageBoss ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iHealing ), SendPropInt( SENDINFO_ARRAY( m_iHealing ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iHealingAssist ), SendPropInt( SENDINFO_ARRAY( m_iHealingAssist ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iDamageBlocked ), SendPropInt( SENDINFO_ARRAY( m_iDamageBlocked ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iCurrencyCollected ), SendPropInt( SENDINFO_ARRAY( m_iCurrencyCollected ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iBonusPoints ), SendPropInt( SENDINFO_ARRAY( m_iBonusPoints ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropInt( SENDINFO( m_iPartyLeaderRedTeamIndex ), -1, SPROP_UNSIGNED | SPROP_VARINT ),
	SendPropInt( SENDINFO( m_iPartyLeaderBlueTeamIndex ), -1, SPROP_UNSIGNED | SPROP_VARINT ),
	SendPropInt( SENDINFO( m_iEventTeamStatus ), -1, SPROP_UNSIGNED | SPROP_VARINT ),
	SendPropArray3( SENDINFO_ARRAY3( m_iPlayerClassWhenKilled ), SendPropInt( SENDINFO_ARRAY( m_iPlayerClassWhenKilled ), 5, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iConnectionState ), SendPropInt( SENDINFO_ARRAY( m_iConnectionState ), 3, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_flConnectTime ), SendPropTime( SENDINFO_ARRAY( m_flConnectTime ) ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( tf_player_manager, CTFPlayerResource );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayerResource::CTFPlayerResource( void )
{
	ListenForGameEvent( "mvm_wave_complete" );

	m_flNextDamageAndHealingSend = 0.f;

	m_iPartyLeaderRedTeamIndex = 0;
	m_iPartyLeaderBlueTeamIndex = 0;
	m_iEventTeamStatus = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerResource::FireGameEvent( IGameEvent * event )
{
	const char *pszEvent = event->GetName();

	if ( !Q_strcmp( pszEvent, "mvm_wave_complete" ) )
	{
		// Force a re-send on wave complete
		m_flNextDamageAndHealingSend = 0.f;
		UpdatePlayerData();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerResource::SetPartyLeaderIndex( int iTeam, int iIndex )
{
	Assert( iIndex >= 0 && iIndex <= MAX_PLAYERS );

	switch( iTeam )
	{
	case TF_TEAM_RED:
		m_iPartyLeaderRedTeamIndex = iIndex;
		break;
	case TF_TEAM_BLUE:
		m_iPartyLeaderBlueTeamIndex = iIndex;
		break;
	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayerResource::GetPartyLeaderIndex( int iTeam )
{
	if ( iTeam == TF_TEAM_RED )
		return m_iPartyLeaderRedTeamIndex;
	else if ( iTeam == TF_TEAM_BLUE )
		return m_iPartyLeaderBlueTeamIndex;

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerResource::UpdatePlayerData( void )
{
	m_vecRedPlayers.RemoveAll();
	m_vecBluePlayers.RemoveAll();
	m_vecFreeSlots.RemoveAll();

	BaseClass::UpdatePlayerData();

	if ( gpGlobals->curtime > m_flNextDamageAndHealingSend )
	{
		m_flNextDamageAndHealingSend = gpGlobals->curtime + STATS_SEND_FREQUENCY;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerResource::UpdateConnectedPlayer( int iIndex, CBasePlayer *pPlayer )
{
	BaseClass::UpdateConnectedPlayer( iIndex, pPlayer );

	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

	PlayerStats_t *pTFPlayerStats = CTF_GameStats.FindPlayerStats( pTFPlayer );
	if ( pTFPlayerStats ) 
	{
		// Update accumulated and per-round stats
		localplayerscoring_t *pData = pTFPlayer->m_Shared.GetScoringData();
		pData->UpdateStats( pTFPlayerStats->statsAccumulated, pTFPlayer, false );

		pData = pTFPlayer->m_Shared.GetRoundScoringData();
		pData->UpdateStats( pTFPlayerStats->statsCurrentRound, pTFPlayer, true );

		// Send every STATS_SEND_FREQUENCY (1.f)
		if ( gpGlobals->curtime > m_flNextDamageAndHealingSend )
		{
			m_iDamage.Set( iIndex, pTFPlayerStats->statsCurrentRound.m_iStat[TFSTAT_DAMAGE] );
			m_iDamageAssist.Set( iIndex, pTFPlayerStats->statsCurrentRound.m_iStat[TFSTAT_DAMAGE_ASSIST] );
			m_iDamageBoss.Set( iIndex, pTFPlayerStats->statsCurrentRound.m_iStat[TFSTAT_DAMAGE_BOSS] );
			m_iHealing.Set( iIndex, pTFPlayerStats->statsCurrentRound.m_iStat[TFSTAT_HEALING] );
			m_iHealingAssist.Set( iIndex, pTFPlayerStats->statsCurrentRound.m_iStat[TFSTAT_HEALING_ASSIST] );
			m_iDamageBlocked.Set( iIndex, pTFPlayerStats->statsCurrentRound.m_iStat[TFSTAT_DAMAGE_BLOCKED] );
			m_iCurrencyCollected.Set( iIndex, pTFPlayerStats->statsCurrentRound.m_iStat[TFSTAT_CURRENCY_COLLECTED] );
			m_iBonusPoints.Set( iIndex, pTFPlayerStats->statsCurrentRound.m_iStat[TFSTAT_BONUS_POINTS] );
		}
	}

	m_iMaxHealth.Set( iIndex, pTFPlayer->GetMaxHealth() );

	// m_iMaxBuffedHealth is misnamed -- it should be m_iMaxHealthForBuffing, but we don't want to change it now due to demos.
	m_iMaxBuffedHealth.Set( iIndex, pTFPlayer->GetMaxHealthForBuffing() );
	m_iPlayerClass.Set( iIndex, pTFPlayer->GetPlayerClass()->GetClassIndex() );

	m_iActiveDominations.Set( iIndex, pTFPlayer->GetNumberofDominations() );

	int iTotalScore = CTFGameRules::CalcPlayerScore( &pTFPlayerStats->statsAccumulated, pTFPlayer );
		
	m_iTotalScore.Set( iIndex, iTotalScore );

	if ( TFGameRules()->IsInTournamentMode() )
	{
		float flCharge = pTFPlayer->MedicGetChargeLevel();
		m_iChargeLevel.Set( iIndex, (int)(flCharge * 100) );
	}
	else
	{
		m_iChargeLevel.Set( iIndex, 0 );
	}

	float flRespawnTime = pTFPlayer->IsAlive() ? 0 : TFGameRules()->GetNextRespawnWave( pTFPlayer->GetTeamNumber(), pTFPlayer );
	if ( pTFPlayer->GetRespawnTimeOverride() != -1.f )
	{
		flRespawnTime = pTFPlayer->GetDeathTime() + pTFPlayer->GetRespawnTimeOverride();
	}
	m_flNextRespawnTime.Set( iIndex, flRespawnTime );

	m_flConnectTime.Set( iIndex, pTFPlayer->GetConnectionTime() );

	CSteamID steamID;
	pTFPlayer->GetSteamID( &steamID );

	int iTeam = pPlayer->GetTeamNumber();

	CUtlVector< uint32 >* pVecPlayers = ( iTeam == TF_TEAM_RED ) ? &m_vecRedPlayers : ( ( iTeam == TF_TEAM_BLUE ) ? &m_vecBluePlayers : NULL );
	if ( pVecPlayers )
	{
		if ( pVecPlayers->Find( steamID.GetAccountID() ) == pVecPlayers->InvalidIndex()	)
		{
			pVecPlayers->AddToTail( steamID.GetAccountID() );
		}
	}

	m_iConnectionState.Set( iIndex, MM_CONNECTED );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerResource::UpdateDisconnectedPlayer( int iIndex )
{
	BaseClass::UpdateDisconnectedPlayer( iIndex );
	
	// free up the slot if we're not preserving it
	m_iConnectionState.Set( iIndex, MM_DISCONNECTED );
	m_vecFreeSlots.AddToTail( iIndex );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerResource::Spawn( void )
{
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerResource::Init( int iIndex )
{
	BaseClass::Init( iIndex );

	m_iTotalScore.Set( iIndex, 0 );
	m_iMaxHealth.Set( iIndex, TF_HEALTH_UNDEFINED );
	m_iMaxBuffedHealth.Set( iIndex, TF_HEALTH_UNDEFINED );
	m_iPlayerClass.Set( iIndex, TF_CLASS_UNDEFINED );
	m_iActiveDominations.Set( iIndex, 0 );
	m_iPlayerClassWhenKilled.Set( iIndex, TF_CLASS_UNDEFINED );
	m_iConnectionState.Set( iIndex, MM_DISCONNECTED );
	m_bValid.Set( iIndex, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Gets a value from an array member
//-----------------------------------------------------------------------------
int CTFPlayerResource::GetTotalScore( int iIndex )
{
	Assert( iIndex >= 0 && iIndex <= MAX_PLAYERS );

	CTFPlayer *pPlayer = (CTFPlayer*)UTIL_PlayerByIndex( iIndex );

	if ( pPlayer && pPlayer->IsConnected() )
	{	
		return m_iTotalScore[iIndex];
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerResource::SetPlayerClassWhenKilled( int iIndex, int iClass )
{
	Assert( iIndex >= 0 && iIndex <= MAX_PLAYERS );

	m_iPlayerClassWhenKilled.Set( iIndex, iClass );
}



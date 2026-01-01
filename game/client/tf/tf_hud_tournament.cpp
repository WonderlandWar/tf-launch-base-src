//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/EditablePanel.h>
#include "tf_imagepanel.h"
#include "tf_gamerules.h"
#include "c_tf_team.h"
#include "vgui/tf_controls.h"
#include <vgui_controls/TextEntry.h>
#include "vgui_controls/AnimationController.h"
#include "hud_basechat.h"
#include "clientmode_shared.h"
#include "c_playerresource.h"
#include "c_tf_playerresource.h"
#include "tf_hud_objectivestatus.h"
#include "c_team_objectiveresource.h"
#include "c_tf_team.h"
#include "tf_clientscoreboard.h"
#include "tf_playerpanel.h"
#include "tf_hud_tournament.h"
#include "c_tf_objective_resource.h"
#include "tf_time_panel.h"

#include "inputsystem/iinputsystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void AddSubKeyNamed( KeyValues *pKeys, const char *pszName );

using namespace vgui;

#define TOURNAMENT_PANEL_UPDATE_INTERVAL 0.25f

extern ConVar mp_timelimit;
extern ConVar mp_winlimit;
extern ConVar mp_maxrounds;
extern ConVar mp_tournament;

class CHudChat;

DECLARE_HUDELEMENT( CHudTournament );

static const wchar_t* GetSCGlyph( const char* action )
{
	auto origin = g_pInputSystem->GetSteamControllerActionOrigin( action, GAME_ACTION_SET_FPSCONTROLS );
	return g_pInputSystem->GetSteamControllerFontCharacterForActionOrigin( origin );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTournament::CHudTournament( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudTournament" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 50 );

	for ( int i = 0; i < MAX_TEAMS; i++ )
	{
		m_bTeamReady[i] = false;
	}

	m_bReadyStatusMode = false;
	m_bCompetitiveMode = false;
	m_bReadyTextBlinking = false;
	m_bCountDownVisible = false;

	m_pPlayerPanelKVs = NULL;
	m_bReapplyPlayerPanelKVs = false;

	m_pScoreboard = NULL;

	m_pCountdownBG = new vgui::ScalableImagePanel( this, "CountdownBG" );
	m_pCountdownLabel = new CTFLabel( this, "CountdownLabel", "" );
	m_pCountdownLabelShadow = new CTFLabel( this, "CountdownLabelShadow", "" );
	m_pModeImage = new vgui::ImagePanel( this, "ModeImage" );
	m_pHudTournamentBG = new vgui::ScalableImagePanel( this, "HudTournamentBG" );
	m_pTournamentConditionLabel = new CTFLabel( this, "TournamentConditionLabel", "" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTournament::~CHudTournament()
{
	if ( m_pPlayerPanelKVs )
	{
		m_pPlayerPanelKVs->deleteThis();
		m_pPlayerPanelKVs = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTournament::Init( void )
{
	// listen for events
	ListenForGameEvent( "tournament_stateupdate" );
	ListenForGameEvent( "teams_changed" );
	ListenForGameEvent( "localplayer_respawn" );
	ListenForGameEvent( "restart_timer_time" );
	ListenForGameEvent( "competitive_victory" );

	m_bShouldBeVisible = false;
	SetVisible( false );
	CHudElement::Init();
	m_flNextUpdate = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTournament::PlaySounds( int nTime )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return;

	bool bCompetitiveMode = TFGameRules() && TFGameRules()->IsCompetitiveMode();

	switch( nTime )
	{
		case 60:
		{
			if ( bCompetitiveMode )
			{
				pLocalPlayer->EmitSound( "Announcer.CompGame1Begins60Seconds" );
			}
			break;
		}
		case 30:
		{
			if ( bCompetitiveMode )
			{
				pLocalPlayer->EmitSound( "Announcer.CompGame1Begins30Seconds" );
			}
			break;
		}
		case 10:
		{
			{
				pLocalPlayer->EmitSound( bCompetitiveMode ? "Announcer.CompGame1Begins10Seconds" : "Announcer.RoundBegins10Seconds" );
			}
			break;
		}
		case 9:
		{
			break;
		}
		case 5:
		{
			pLocalPlayer->EmitSound( bCompetitiveMode ? "Announcer.CompGameBegins05Seconds" : "Announcer.RoundBegins5Seconds" );
			break;
		}
		case 4:
		{
			pLocalPlayer->EmitSound( bCompetitiveMode ? "Announcer.CompGameBegins04Seconds" : "Announcer.RoundBegins4Seconds" );
			break;
		}
		case 3:
		{
			pLocalPlayer->EmitSound( bCompetitiveMode ? "Announcer.CompGameBegins03Seconds" : "Announcer.RoundBegins3Seconds" );
			break;
		}
		case 2:
		{
			pLocalPlayer->EmitSound( bCompetitiveMode ? "Announcer.CompGameBegins02Seconds" : "Announcer.RoundBegins2Seconds" );
			break;
		}
		case 1:
		{
			pLocalPlayer->EmitSound( bCompetitiveMode ? "Announcer.CompGameBegins01Seconds" : "Announcer.RoundBegins1Seconds" );
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTournament::PreparePanel( void )
{
	if ( m_flNextUpdate > gpGlobals->curtime )
		return;

	if ( !TFGameRules() )
		return;

	bool bSteamController = ::input->IsSteamControllerActive();
	bool bShowReadyHintIcon = false;

	if ( TFGameRules()->IsInPreMatch() )
	{
		bool bCountdownVisible = false;

		if ( ( TFGameRules()->IsWaitingForTeams() || TFGameRules()->GetRoundRestartTime() < 0 ) )
		{
			if ( m_bReadyStatusMode )
			{
				const char *pszLabelText;
				if ( TFGameRules() && TFGameRules()->PlayerReadyStatus_HaveMinPlayersToEnable() )
				{
					if ( bSteamController )
					{
						pszLabelText = "Tournament_Instructions_Ready_NoKeyHintText";
						bShowReadyHintIcon = true;
					}
					else
					{
						pszLabelText = "Tournament_Instructions_Ready";
					}
				}
				else
				{
					pszLabelText = "Tournament_Instructions_Waiting";
				}

				SetDialogVariable( "readylabel", g_pVGuiLocalize->Find( pszLabelText ) );
				SetDialogVariable( "tournamentstatelabel", g_pVGuiLocalize->Find( "Tournament_WaitingForTeam" ) );
				SetPlayerPanelsVisible( true );
				m_pModeImage->SetVisible( m_bCompetitiveMode );
			}
			else
			{
				SetDialogVariable( "readylabel", g_pVGuiLocalize->Find( "Tournament_Instructions" ) );
				SetDialogVariable( "tournamentstatelabel", g_pVGuiLocalize->Find( "Tournament_WaitingForTeams" ) );
				SetPlayerPanelsVisible( false );
				m_pModeImage->SetVisible( false );
			}
		}
		else
		{
			float flTime = TFGameRules()->GetRoundRestartTime() - gpGlobals->curtime;
			int nTime = (int)( ceil( flTime ) );
			
			wchar szCountdown[64];
			wchar_t wzVal[16];

			const char *szCountDown = m_bReadyStatusMode ? "Tournament_CountDownTime" : "Tournament_Countdown";
			const char *szCountDownSec = m_bReadyStatusMode ? "Tournament_CountDownTime" : "Tournament_Countdown_Sec";

			swprintf( wzVal, ARRAYSIZE( wzVal ), L"%d", nTime );
			wchar_t *pFormatString = g_pVGuiLocalize->Find( szCountDown );
			if ( nTime == 1 )
			{
				pFormatString = g_pVGuiLocalize->Find( szCountDownSec );
			}

			if ( pFormatString )
			{
				g_pVGuiLocalize->ConstructString_safe( szCountdown, pFormatString, 1, wzVal );
				SetDialogVariable( "tournamentstatelabel", szCountdown );
			}

			if ( nTime <= TOURNAMENT_NOCANCEL_TIME )
			{
				SetDialogVariable( "readylabel", g_pVGuiLocalize->Find( "" ) );
			}
			else
			{
				if ( m_bReadyStatusMode )
				{
					if ( bSteamController )
					{
						SetDialogVariable( "readylabel", g_pVGuiLocalize->Find( "Tournament_Instructions_Ready_NoKeyHintText" ) );
						bShowReadyHintIcon = true;
					}
					else
					{
						SetDialogVariable( "readylabel", g_pVGuiLocalize->Find( "Tournament_Instructions_Ready" ) );
					}
				}
				else
				{
					SetDialogVariable( "readylabel", g_pVGuiLocalize->Find( "" ) );
				}
			}

			if ( m_bReadyStatusMode && nTime >= 0 )
			{
				bCountdownVisible = true;
			}
		}

		if ( m_bCountDownVisible != bCountdownVisible )
		{
			m_bCountDownVisible = bCountdownVisible;

			if ( m_bCountDownVisible )
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(this, m_bCompetitiveMode ? "HudTournament_ShowTimerCompetitive" : "HudTournament_ShowTimerDefault", false);
			}
			else
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(this, "HudTournament_HideTimer", false);
			}
		}	
	}
	
#ifdef WIN32
#define STRING_FMT L"%s"
#else
#define STRING_FMT L"%S"	
#endif
	
	C_TFTeam *pBlueTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
	SetDialogVariable( "bluenamelabel", pBlueTeam ? pBlueTeam->Get_Localized_Name() : L"BLU" );

	C_TFTeam *pRedTeam = GetGlobalTFTeam( TF_TEAM_RED );
	SetDialogVariable( "rednamelabel", pRedTeam ? pRedTeam->Get_Localized_Name() : L"RED" );

	SetDialogVariable( "bluestate", TFGameRules()->IsTeamReady( TF_TEAM_BLUE ) ? g_pVGuiLocalize->Find( "Tournament_TeamReady" ) : g_pVGuiLocalize->Find( "Tournament_TeamNotReady" ) );
	SetDialogVariable( "redstate", TFGameRules()->IsTeamReady( TF_TEAM_RED ) ? g_pVGuiLocalize->Find( "Tournament_TeamReady" ) : g_pVGuiLocalize->Find( "Tournament_TeamNotReady" ) );
	
	if ( m_bTeamReady[TF_TEAM_BLUE] != TFGameRules()->IsTeamReady( TF_TEAM_BLUE ) || m_bTeamReady[TF_TEAM_RED] != TFGameRules()->IsTeamReady( TF_TEAM_RED ) )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

		if ( pLocalPlayer )
		{
			pLocalPlayer->EmitSound( "Hud.Hint" );
		}
	}

	m_bTeamReady[TF_TEAM_BLUE] = TFGameRules()->IsTeamReady( TF_TEAM_BLUE );
	m_bTeamReady[TF_TEAM_RED] = TFGameRules()->IsTeamReady( TF_TEAM_RED );

	wchar_t szWindConditions[1024];
	_snwprintf( szWindConditions, ARRAYSIZE( szWindConditions ), STRING_FMT, g_pVGuiLocalize->Find( "Tournament_WinConditions" ) );
	
	if ( mp_timelimit.GetInt() > 0 || mp_winlimit.GetInt() > 0 || mp_maxrounds.GetInt() )
	{
		bool bPrev = false;

		if ( mp_timelimit.GetInt() > 0 )
		{
			wchar_t szWindConditionsTmp[1024];
			V_wcscpy_safe( szWindConditionsTmp, szWindConditions );

			_snwprintf( szWindConditions, ARRAYSIZE( szWindConditions ), STRING_FMT L"%d " STRING_FMT, szWindConditionsTmp, mp_timelimit.GetInt(), mp_timelimit.GetInt() == 1 ? g_pVGuiLocalize->Find( "Tournament_WinConditionsMinute" ) : g_pVGuiLocalize->Find( "Tournament_WinConditionsMinutes" ) );
			bPrev = true;
		}

		if ( mp_winlimit.GetInt() > 0 )
		{
			if ( bPrev )
			{
				wchar_t szWindConditionsTmp[1024];
				V_wcscpy_safe( szWindConditionsTmp, szWindConditions );
				_snwprintf( szWindConditions, ARRAYSIZE( szWindConditions ), STRING_FMT L", ", szWindConditionsTmp );
			}

			wchar_t szWindConditionsTmp2[1024];
			V_wcscpy_safe( szWindConditionsTmp2, szWindConditions );
			_snwprintf( szWindConditions, ARRAYSIZE( szWindConditions ), STRING_FMT L"%d " STRING_FMT, szWindConditionsTmp2, mp_winlimit.GetInt(), mp_winlimit.GetInt() == 1 ? g_pVGuiLocalize->Find( "Tournament_WinConditionsWin" ) : g_pVGuiLocalize->Find( "Tournament_WinConditionsWins" ) );
			bPrev = true;
		}

		if ( mp_maxrounds.GetInt() > 0 )
		{
			if ( bPrev )
			{
				wchar_t szWindConditionsTmp[1024];
				V_wcscpy_safe( szWindConditionsTmp, szWindConditions );
				_snwprintf( szWindConditions, ARRAYSIZE( szWindConditions ), STRING_FMT L", ", szWindConditionsTmp );
			}

			wchar_t szWindConditionsTmp2[1024];
			V_wcscpy_safe( szWindConditionsTmp2, szWindConditions );
			_snwprintf( szWindConditions, ARRAYSIZE( szWindConditions ), STRING_FMT L"%d " STRING_FMT, szWindConditionsTmp2, mp_maxrounds.GetInt(), mp_maxrounds.GetInt() == 1 ? g_pVGuiLocalize->Find( "Tournament_WinConditionsRound" ) : g_pVGuiLocalize->Find( "Tournament_WinConditionsRounds" ) );
		}
	}
	else
	{
		wchar_t szWindConditionsTmp[1024];
		V_wcscpy_safe( szWindConditionsTmp, szWindConditions );
		_snwprintf( szWindConditions, ARRAYSIZE( szWindConditions ), STRING_FMT STRING_FMT, szWindConditionsTmp, g_pVGuiLocalize->Find( "Tournament_WinConditionsNone" ) );
	}

	SetDialogVariable( "winconditions", szWindConditions );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTournament::FireGameEvent( IGameEvent * event )
{
	const char *pEventName = event->GetName();
	CBaseHudChat *pHUDChat = (CBaseHudChat *)GET_HUDELEMENT( CHudChat );

	if ( Q_strcmp( "tournament_stateupdate", pEventName ) == 0 )
	{
		if ( !g_TF_PR )
			return;

		if ( !pHUDChat )
			return;

		wchar_t wszLocalized[100];

		bool bNameChange = event->GetBool( "namechange" );
		int iTeamState = event->GetInt( "readystate", -1 );
		int iIndex = event->GetInt("userid");

		const char *pszName = g_TF_PR->GetPlayerName( iIndex );
	
		wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
		g_pVGuiLocalize->ConvertANSIToUnicode( pszName, wszPlayerName, sizeof(wszPlayerName) );

		if ( bNameChange )
		{
			wchar_t wszTeam[16];
			g_pVGuiLocalize->ConvertANSIToUnicode( event->GetString( "newname" ), wszTeam, sizeof(wszTeam) );
		
			g_pVGuiLocalize->ConstructString_safe( wszLocalized, g_pVGuiLocalize->Find( "#Tournament_TeamName_Change" ), 2, wszPlayerName, wszTeam );
		}
		else
		{
			g_pVGuiLocalize->ConstructString_safe( wszLocalized, g_pVGuiLocalize->Find( "#Tournament_ReadyState_Change" ), 2, wszPlayerName, iTeamState == 1 ? g_pVGuiLocalize->Find( "#Tournament_TeamReady" ) : g_pVGuiLocalize->Find( "#Tournament_TeamNotReady" ) );
		}

		char szLocalized[100];
		g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof(szLocalized) );

		pHUDChat->ChatPrintf( iIndex, CHAT_FILTER_NONE, "%s ", szLocalized );
	}
	else if ( Q_strcmp( "localplayer_respawn", pEventName ) == 0 )
	{
		if ( m_bReadyStatusMode )
		{
			if ( m_bReadyTextBlinking )
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HudReadyPulse" );
			}
		}
	}
	else if ( FStrEq( "restart_timer_time", pEventName ) )
	{
		PlaySounds( event->GetInt( "time" ) );

		if ( TFGameRules()->GetRoundsPlayed() == 0 && m_bCompetitiveMode )
		{
			if ( event->GetInt( "time" ) == 10 )
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudTournament_MoveTimerDown", false );
				//g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HudTournament_MoveChatWindow", false );
			}
		}
	}
	else if ( FStrEq( "competitive_victory", pEventName ) )
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudTournament_DoorsCloseEndRound", false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CHudTournament::OnTick( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return;

	if ( TFGameRules() )
	{
		if ( TFGameRules()->IsInTournamentMode() )
		{
			if ( TFGameRules()->IsInWaitingForPlayers() && TFGameRules()->State_Get() != GR_STATE_GAME_OVER )
			{
				m_bShouldBeVisible = true;
				PreparePanel();

				if ( !TFGameRules()->IsInArenaMode() )
				{
					if ( !pLocalPlayer->IsAlive() )
					{
						m_bShouldBeVisible = false;
					}
				}
			}
			else
			{
				m_bShouldBeVisible = false;
			}

			if ( TFGameRules()->UsePlayerReadyStatusMode() )
			{
				if ( !m_bReadyStatusMode )
				{
					m_bReadyStatusMode = true;
					InvalidateLayout( false, true );
				}
			}
			else if ( m_bReadyStatusMode )
			{
				m_bReadyStatusMode = false;
				InvalidateLayout( false, true );
			}

			if ( TFGameRules()->IsCompetitiveMode() )
			{
				if ( !m_bCompetitiveMode )
				{
					m_bCompetitiveMode = true;
					InvalidateLayout( false, true );
				}
			}
			else if ( m_bCompetitiveMode )
			{
				m_bCompetitiveMode = false;
				InvalidateLayout( false, true );
			}
		}
		else
		{
			m_bShouldBeVisible = false;
		}

		if ( m_bReadyStatusMode )
		{
			RecalculatePlayerPanels();

			// Ready text animation
			if ( !TFGameRules()->IsPlayerReady( GetLocalPlayerIndex() ) && !m_bReadyTextBlinking )
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HudReadyPulse" );
				m_bReadyTextBlinking = true;
			}
			else if ( TFGameRules()->IsPlayerReady( GetLocalPlayerIndex() ) && m_bReadyTextBlinking )
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HudReadyPulseEnd" );
				m_bReadyTextBlinking = false;
			}

			if ( !m_pScoreboard.Get() && gViewPortInterface )
			{
				m_pScoreboard = (CTFClientScoreBoardDialog *)( gViewPortInterface->FindPanelByName( PANEL_SCOREBOARD ) );
			}

			if ( m_pScoreboard.Get() && m_pScoreboard->IsVisible() )
			{
				m_bShouldBeVisible = false;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTournament::LevelInit( void )
{
	m_bShouldBeVisible = false;
	SetVisible( false );
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTournament::SetVisible( bool state )
{
	// we can only turn on tournament mode if we're in prematch
	if ( state )
	{
		if ( m_bReadyStatusMode )
		{
			if ( m_bReadyTextBlinking )
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HudReadyPulse" );
			}
		}

		if ( TeamplayRoundBasedRules() && !TeamplayRoundBasedRules()->IsInPreMatch() )
			return;
	}

	BaseClass::SetVisible( state );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTournament::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_bReapplyPlayerPanelKVs = true;

	KeyValues *pConditions = NULL;
	if ( m_bCompetitiveMode )
	{
		pConditions = new KeyValues( "conditions" );
		AddSubKeyNamed( pConditions, "if_competitive" );
	}
	else if ( m_bReadyStatusMode )
	{
		pConditions = new KeyValues( "conditions" );
		AddSubKeyNamed( pConditions, "if_readymode" );
	}

	// load control settings...
	LoadControlSettings( "resource/UI/HudTournament.res", NULL, NULL, pConditions );

	if ( pConditions )
	{
		pConditions->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTournament::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	KeyValues *pItemKV = inResourceData->FindKey( "playerpanels_kv" );
	if ( pItemKV )
	{
		if ( m_pPlayerPanelKVs )
		{
			m_pPlayerPanelKVs->deleteThis();
		}
		m_pPlayerPanelKVs = new KeyValues("playerpanels_kv");
		pItemKV->CopySubkeys( m_pPlayerPanelKVs );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTournament::PerformLayout( void )
{
	BaseClass::PerformLayout();

	if ( m_bReapplyPlayerPanelKVs )
	{
		m_bReapplyPlayerPanelKVs = false;

		if ( m_pPlayerPanelKVs )
		{
			for ( int i = 0; i < m_PlayerPanels.Count(); i++ )
			{
				m_PlayerPanels[i]->ApplySettings( m_pPlayerPanelKVs );
				m_PlayerPanels[i]->InvalidateLayout( false, true );
			}
		}
	}

	bool bShowTournamentConditions = !m_bCompetitiveMode;

	// Hide some elements when in competitive mode
	if ( m_pTournamentConditionLabel )
	{
		m_pTournamentConditionLabel->SetVisible( bShowTournamentConditions );
	}

	Panel* pTournamentBG = FindChildByName( "HudTournamentBG" );
	if ( pTournamentBG )
	{
		pTournamentBG->SetVisible( bShowTournamentConditions );
	}

	UpdatePlayerPanels();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTournament::SetPlayerPanelsVisible( bool bVisible )
{
	for ( int i = 0; i < m_PlayerPanels.Count(); i++ )
	{
		if ( m_PlayerPanels[i] )
		{
			if ( m_PlayerPanels[i]->IsVisible() != bVisible )
			{
				m_PlayerPanels[i]->SetVisible( bVisible );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create a new panel if necessary
//-----------------------------------------------------------------------------
CTFPlayerPanel *CHudTournament::GetOrAddPanel( int iPanelIndex )
{
	if ( iPanelIndex < m_PlayerPanels.Count() )
	{
		return m_PlayerPanels[ iPanelIndex ];
	}
	Assert( iPanelIndex == m_PlayerPanels.Count() );
	CTFPlayerPanel *pPanel = new CTFPlayerPanel( this, VarArgs("playerpanel%d", iPanelIndex) );
	if ( m_pPlayerPanelKVs )
	{
		pPanel->ApplySettings( m_pPlayerPanelKVs );
		pPanel->InvalidateLayout( false, true );
	}
	m_PlayerPanels.AddToTail( pPanel );
	return pPanel;
}

//-----------------------------------------------------------------------------
// Purpose: Decide which players we will show
//-----------------------------------------------------------------------------
void CHudTournament::RecalculatePlayerPanels( void )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer || !g_TF_PR )
		return;

	int iPanel = 0;
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		if ( !g_TF_PR->IsConnected( i ) )
			continue;

		int iTeam = g_TF_PR->GetTeam( i );
		if ( iTeam == TEAM_UNASSIGNED && !m_bReadyStatusMode )
			continue;

		if ( iTeam != TF_TEAM_RED && iTeam != TF_TEAM_BLUE )
			continue;

		// Add an entry for him
		CTFPlayerPanel *pPanel = GetOrAddPanel( iPanel );
		pPanel->SetPlayerIndex( i );
		++iPanel;
	}

	// Check if we have a lobby, then add in players that have a reservation in the lobby,
	// but aren't in the game yet

	// Clear out any extra panels
	for ( int i = iPanel; i < m_PlayerPanels.Count(); i++  )
	{
		m_PlayerPanels[i]->SetPlayerIndex( 0 );
	}

	UpdatePlayerPanels();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTournament::UpdatePlayerPanels( void )
{
	if ( !g_TF_PR )
		return;

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	if ( !TFGameRules() )
		return;

	// Hide panels for players when they're no longer able to stop the countdown
	if ( TFGameRules()->GetRoundRestartTime() >= 0.f && TFGameRules()->GetRoundRestartTime() - gpGlobals->curtime <= TOURNAMENT_NOCANCEL_TIME )
	{
		SetPlayerPanelsVisible( false );
		m_pModeImage->SetVisible( false );

		return;
	}

	bool bNeedsPlayerLayout = false;
	for ( int i = 0; i < m_PlayerPanels.Count(); i++ )
	{
		if ( m_PlayerPanels[i]->Update() )
		{
			bNeedsPlayerLayout = true;
		}
	}

	if ( !bNeedsPlayerLayout || !TFGameRules() )
		return;

	// Try and always put the local player's team on team1, if he's in a team
	int iTeam1 = TF_TEAM_BLUE;
	int iTeam2 = TF_TEAM_RED;
	int iLocalTeam = g_TF_PR->GetTeam( pPlayer->entindex() );
	if ( ( iLocalTeam == TF_TEAM_RED || iLocalTeam == TF_TEAM_BLUE ) && !TFGameRules()->IsCompetitiveMode() )	// Blue always on left in comp
	{
		iTeam1 = iLocalTeam;
		iTeam2 = ( iTeam1 == TF_TEAM_BLUE ) ? TF_TEAM_RED : TF_TEAM_BLUE;
	}

	int iTeamSize = g_TF_PR->GetNumPlayersForTeam( iTeam1, false );
	int iTeam1Processed = 0;
	int iTeam2Processed = 0;
	int iCenter = GetWide() * 0.5;

	for ( int i = 0; i < m_PlayerPanels.Count(); i++ )
	{
		int iTeam = m_PlayerPanels[i]->GetTeam();

		if ( !m_PlayerPanels[i]->GetPlayerIndex() && iTeam == TEAM_INVALID )
			continue;

		int iXPos = ( m_bCompetitiveMode ) ? -XRES( 30 ) : 0;	// Hack to make space for the season image
		int iYPos = ( m_iTeam1PlayerBaseY + m_iTeam1PlayerDeltaY );
		int nOffset = ( m_bCompetitiveMode ) ? m_iTeamsPlayerDeltaXComp : m_iTeam2PlayerDeltaX;

		if ( iTeam == iTeam1 )
		{
			// Two teams.  First team left of center.
			if ( m_bReadyStatusMode )
			{
				int iTeam1LeftCorner = iCenter - ( iTeamSize * nOffset );
				iXPos += ( iTeam1LeftCorner + ( iTeam1Processed * nOffset ) );
			}
			// One team.  Centered.
			else
			{
				int iTeam1LeftCorner = ( iCenter - ( iTeamSize * nOffset ) * 0.5 );
				iXPos += ( iTeam1LeftCorner + ( iTeam1Processed * nOffset ) );
			}
			m_PlayerPanels[i]->SetSpecIndex( 6 - iTeam1Processed );
			++iTeam1Processed;
		}
		else if ( iTeam == iTeam2 )
		{
			// Two teams.  Second team right of center.
			iXPos = ( iCenter + ( iTeam2Processed * nOffset ) );
			iXPos += ( m_bCompetitiveMode ) ? XRES( 30 ) : 0;	// Hack to make space for the season image
			m_PlayerPanels[i]->SetSpecIndex( 7 + iTeam2Processed );
			++iTeam2Processed;
		}

		m_PlayerPanels[i]->SetPos( iXPos, iYPos );
	}
}

DECLARE_HUDELEMENT( CHudTournamentSetup );

bool TournamentHudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	CHudTournamentSetup *pTournamentPanel = ( CHudTournamentSetup * )GET_HUDELEMENT( CHudTournamentSetup );

	if ( pTournamentPanel && down == 1 )
	{
		return pTournamentPanel->ToggleState( keynum );
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTournamentSetup::CHudTournamentSetup( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudTournamentSetup" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );

	m_pNameEntry = new TextEntry(this, "TournamentNameEdit" );
	m_pEntryBG = new CTFImagePanel(this, "HudTournamentNameBG" );
	m_pReadyButton = new CTFButton(this, "TournamentReadyButton", "" );
	m_pNotReadyButton = new CTFButton(this, "TournamentNotReadyButton", "" );

	m_pTeamNameLabel = NULL;
	m_flNextThink = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTournamentSetup::Init( void )
{
	SetVisible( false );
	CHudElement::Init();

	DisableInput();
	
	m_pNameEntry->SetText( g_pVGuiLocalize->Find( "Tournament_TeamNameNotSet" ) );

	m_flNextThink = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTournamentSetup::OnCommand( const char *command )
{
	if ( !Q_strcmp( command, "teamready" ) )
	{
		char szText[64];
		m_pNameEntry->GetText( szText, sizeof( szText ) );

		char szTeamName[64];
		Q_snprintf ( szTeamName, sizeof( szTeamName ), "tournament_teamname %s", szText );
		engine->ClientCmd_Unrestricted( szTeamName );

		engine->ClientCmd_Unrestricted( "tournament_readystate 1" );
		DisableInput();
	}
	else if ( !Q_strcmp( command, "teamnotready" ) )
	{
		engine->ClientCmd_Unrestricted( "tournament_readystate 0" );
		DisableInput();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudTournamentSetup::ToggleState( ButtonCode_t code )
{
	if ( !IsVisible() )
		return false;

	if ( !g_TF_PR )
		return false;

	if ( code == KEY_F4 || code == STEAMCONTROLLER_F4 )
	{
		if ( TFGameRules() && TFGameRules()->UsePlayerReadyStatusMode() )
		{
			int nReady = ( TFGameRules()->IsPlayerReady( GetLocalPlayerIndex() ) ) ? 0 : 1;
			char szCommand[64];
			Q_snprintf( szCommand, sizeof( szCommand ), "tournament_player_readystate %d", nReady );
			engine->ClientCmd_Unrestricted( szCommand );
		}
		else
		{
			if ( IsMouseInputEnabled() )
			{
				DisableInput();
				return true;
			}
			else
			{
				EnableInput();
				return true;
			}
		}
	}

	if ( IsMouseInputEnabled() )
	{
		if ( code == KEY_ESCAPE || code == KEY_ENTER )
		{
			if ( code == KEY_ENTER )
			{
				char szText[64];
				m_pNameEntry->GetText( szText, sizeof( szText ) );

				char szTeamName[64];
				Q_snprintf ( szTeamName, sizeof( szTeamName ), "tournament_teamname %s", szText );
				engine->ClientCmd_Unrestricted( szTeamName );

				m_flNextThink = gpGlobals->curtime + TOURNAMENT_PANEL_UPDATE_INTERVAL;
			}

			DisableInput();
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTournamentSetup::FireGameEvent( IGameEvent * event )
{
	const char *pEventName = event->GetName();

	if ( Q_strcmp( "tournament_stateupdate", pEventName ) == 0 )
	{
		if ( TFGameRules()->IsInTournamentMode() && TFGameRules()->IsInWaitingForPlayers() )
		{
			m_flNextThink = gpGlobals->curtime;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CHudTournamentSetup::OnTick( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	if ( !g_TF_PR )
		return;

	int iLocalTeam = g_TF_PR->GetTeam( pLocalPlayer->entindex() );
	if ( iLocalTeam <= LAST_SHARED_TEAM || TFGameRules()->State_Get() == GR_STATE_GAME_OVER )
	{
		SetVisible( false );
		return;
	}

	if ( TFGameRules() )
	{
		if ( TFGameRules()->UsePlayerReadyStatusMode() )
		{
			if ( TFGameRules()->IsInTournamentMode() && TFGameRules()->IsInWaitingForPlayers() )
			{
				if ( !IsVisible() )
				{
					SetVisible( true );
				}
				SetPos( 0, YRES( -100 ) ); // make sure the panel is WAY off the screen for MvM mode
			}
			else
			{
				if ( IsVisible() )
				{
					SetVisible( false );
				}
			}
		}
		else if ( TFGameRules()->IsInTournamentMode() && TFGameRules()->IsInWaitingForPlayers() )
		{
			if ( !IsVisible() )
			{
				SetVisible( true );
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HudTournamentSetupPanelClose" );
			}
		}
		else
		{
			if ( IsVisible() )
			{
				SetVisible( false );
			}
		}
	}

	if ( m_flNextThink <= gpGlobals->curtime )
	{
		if ( !IsMouseInputEnabled() )
		{
			m_pNameEntry->SetText( ( iLocalTeam == TF_TEAM_BLUE ) ? mp_tournament_blueteamname.GetString() : mp_tournament_redteamname.GetString() );
		}

		SetDialogVariable( "tournamentstatelabel", TFGameRules()->IsTeamReady( iLocalTeam ) ? g_pVGuiLocalize->Find( "Tournament_TeamSetupReady" ) : g_pVGuiLocalize->Find( "Tournament_TeamSetupNotReady" ) );

		m_flNextThink = gpGlobals->curtime + TOURNAMENT_PANEL_UPDATE_INTERVAL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTournamentSetup::EnableInput( void )
{
	SetVisible( true );
	vgui::SETUP_PANEL( this );
	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );
	m_pNameEntry->SetVisible( true );
	vgui::surface()->CalculateMouseVisible();
	m_pNameEntry->RequestFocus();
	m_pNameEntry->SetPaintBorderEnabled( true );
	m_pNameEntry->SetMouseInputEnabled( true );
	m_pNameEntry->SetKeyBoardInputEnabled( true );
	MakePopup();

	m_pEntryBG->SetVisible( true );
	m_pReadyButton->SetVisible( true );
	m_pReadyButton->SetMouseInputEnabled( true );
	m_pNotReadyButton->SetVisible( true );
	m_pNotReadyButton->SetMouseInputEnabled( true );

	m_pTeamNameLabel->SetVisible( true );

	engine->ClientCmd_Unrestricted( "gameui_preventescapetoshow\n" );

	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HudTournamentSetupPanelOpen" );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTournamentSetup::DisableInput( void )
{
	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( false );

	m_pNameEntry->SetMouseInputEnabled( false );
	m_pNameEntry->SetKeyBoardInputEnabled( false );
	
	engine->ClientCmd_Unrestricted( "gameui_allowescapetoshow\n" );

	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HudTournamentSetupPanelClose" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTournamentSetup::LevelInit( void )
{
	SetVisible( false );
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudTournamentSetup::ShouldDraw( void )
{
	return ( IsVisible() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTournamentSetup::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudTournamentSetup.res" );

	BaseClass::ApplySchemeSettings( pScheme );

	m_pNameEntry->SetFont( pScheme->GetFont( "HudFontSmallest", IsProportional() ) );
	m_pTeamNameLabel = dynamic_cast<Label *>( FindChildByName( "TournamenTeamNameLabel" ) );
}


DECLARE_HUDELEMENT( CHudStopWatch );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudStopWatch::CHudStopWatch( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudStopWatch" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	m_bShouldBeVisible = false;
	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );

	m_pTimePanel = new CTFHudTimeStatus( this, "ObjectiveStatusTimePanel" );
	m_pStopWatchLabel = new CTFLabel( this, "StopWatchLabel", "" );
	m_pStopWatchScore = new CTFLabel( this, "StopWatchScoreToBeat", "" );
	m_pStopWatchPointsLabel = new CTFLabel( this, "StopWatchPointsLabel", "" );
	m_pStopWatchImage = new ImagePanel( this, "StopWatchImageCaptureTime" );
	m_pStopWatchDescriptionLabel = new CTFLabel( this, "StopWatchDescriptionLabel", "" );

	ListenForGameEvent( "competitive_state_changed" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudStopWatch::ShouldDraw( void )
{ 
	bool bRetVal = m_bShouldBeVisible;
	if ( bRetVal )
	{
		// give the HUD a chance to turn us off when we should be hidden
		bRetVal = CHudElement::ShouldDraw();
	}
	
	return bRetVal; 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudStopWatch::LevelInit( void )
{
	m_bShouldBeVisible = true;
	
	if ( m_pTimePanel )
	{
		m_pTimePanel->SetVisible( true );
		m_pTimePanel->Reset();
	}

	m_pStopWatchLabel->SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudStopWatch::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudStopWatch.res" );

	BaseClass::ApplySchemeSettings( pScheme );

	m_pStopWatchDescriptionBG = FindChildByName( "HudStopWatchDescriptionBG" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudStopWatch::OnTick( void )
{
	BaseClass::OnTick();

	if ( !TFGameRules() )
		return;

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer )
		return;

	bool bInFreezeCam = ( pPlayer && pPlayer->GetObserverMode() == OBS_MODE_FREEZECAM );

	bool bProperMatch = TFGameRules()->IsInTournamentMode() || TFGameRules()->IsCompetitiveMode();
	if ( !bProperMatch || TFGameRules()->IsInPreMatch() || !TFGameRules()->IsInStopWatch() || bInFreezeCam || TFGameRules()->State_Get() == GR_STATE_GAME_OVER  )
	{
		m_bShouldBeVisible = false;
		return;
	}
	else
	{
		m_bShouldBeVisible = true;
	}
}

void CHudStopWatch::FireGameEvent( IGameEvent * event )
{
	if ( FStrEq( event->GetName(), "competitive_state_changed" ) )
	{
		InvalidateLayout( false, true );
		return;
	}

	CHudElement::FireGameEvent( event );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CON_COMMAND( player_ready_toggle, "Toggle player ready state" )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		CHudTournamentSetup *pTournamentPanel = dynamic_cast< CHudTournamentSetup* >( GET_HUDELEMENT( CHudTournamentSetup ) );
		if ( pTournamentPanel )
		{
			pTournamentPanel->ToggleState( KEY_F4 );
		}
	}
}

//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include "iclientmode.h"
#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/ISurface.h>
#include <vgui/IImage.h>
#include <vgui_controls/Label.h>

#include "c_playerresource.h"
#include "teamplay_round_timer.h"
#include "utlvector.h"
#include "entity_capture_flag.h"
#include "c_tf_player.h"
#include "c_team.h"
#include "c_tf_team.h"
#include "c_team_objectiveresource.h"
#include "tf_hud_flagstatus.h"
#include "tf_hud_objectivestatus.h"
#include "tf_spectatorgui.h"
#include "teamplayroundbased_gamerules.h"
#include "tf_gamerules.h"
#include "c_tf_playerresource.h"

void AddSubKeyNamed( KeyValues *pKeys, const char *pszName );

ConVar tf_hud_show_servertimelimit( "tf_hud_show_servertimelimit", "0", FCVAR_ARCHIVE, "Display time left before the current map ends." );

extern ConVar tf_arena_round_time;

using namespace vgui;

DECLARE_HUDELEMENT( CTFHudObjectiveStatus );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudObjectiveStatus::CTFHudObjectiveStatus( const char *pElementName ) 
	: CHudElement( pElementName )
	, BaseClass( NULL, "HudObjectiveStatus" ) 
	, m_pTimePanel( NULL )
	, m_pFlagPanel( NULL )
	, m_pControlPointIconsPanel( NULL )
	, m_pControlPointProgressBar( NULL )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	
	m_pTimePanel = new CTFHudTimeStatus( this, "ObjectiveStatusTimePanel" );
	m_pFlagPanel = new CTFHudFlagObjectives( this, "ObjectiveStatusFlagPanel" );
	m_pControlPointIconsPanel = NULL;
	m_pControlPointProgressBar = new CControlPointProgressBar( this );

	SetHiddenBits( 0 );

	RegisterForRenderGroup( "mid" );
	RegisterForRenderGroup( "commentary" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudObjectiveStatus::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	//LoadControlSettings( "resource/UI/HudObjectiveStatus.res" );

	if ( !m_pControlPointIconsPanel )
	{
		m_pControlPointIconsPanel = GET_HUDELEMENT( CHudControlPointIcons );
		m_pControlPointIconsPanel->SetParent( this );
	}

	if ( m_pControlPointProgressBar )
	{
		m_pControlPointProgressBar->InvalidateLayout( true, true );
	}

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudObjectiveStatus::Reset()
{
	if ( m_pTimePanel )
	{
		m_pTimePanel->Reset();
	}

	if ( m_pFlagPanel )
	{
		m_pFlagPanel->Reset();
	}

	if ( m_pControlPointProgressBar )
	{
		m_pControlPointProgressBar->Reset();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CControlPointProgressBar *CTFHudObjectiveStatus::GetControlPointProgressBar( void )
{
	return m_pControlPointProgressBar;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudObjectiveStatus::SetVisiblePanels( void )
{
	if ( !TFGameRules() )
		return;

	int iGameType = TFGameRules()->GetGameType();

	bool bCTFVisible = bCTFVisible = ( iGameType == TF_GAMETYPE_CTF );

	if ( m_pFlagPanel && m_pFlagPanel->IsVisible() != bCTFVisible )
	{
		m_pFlagPanel->SetVisible( bCTFVisible );
	}

	bool bCPVisible = iGameType == TF_GAMETYPE_CP;

	if ( m_pControlPointIconsPanel && m_pControlPointIconsPanel->IsVisible() != bCPVisible )
	{
		m_pControlPointIconsPanel->SetVisible( bCPVisible );
	}
	
	// check for an active timer and turn the time panel on or off if we need to
	if ( m_pTimePanel )
	{
		// Don't draw in freezecam, or when the game's not running
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		bool bDisplayTimer = !( pPlayer && pPlayer->GetObserverMode() == OBS_MODE_FREEZECAM );

		if ( TeamplayRoundBasedRules()->IsInTournamentMode() && TeamplayRoundBasedRules()->IsInWaitingForPlayers() )
		{
			bDisplayTimer = false;
		}

		if ( bDisplayTimer )
		{
			// is the time panel still pointing at an active timer?
			int iCurrentTimer = m_pTimePanel->GetTimerIndex();
			CTeamRoundTimer *pTimer = dynamic_cast< CTeamRoundTimer* >( ClientEntityList().GetEnt( iCurrentTimer ) );

			if ( pTimer && !pTimer->IsDormant() && !pTimer->IsDisabled() && pTimer->ShowInHud() )
			{
				// the current timer is fine, make sure the panel is visible
				bDisplayTimer = true;
			}
			else if ( ObjectiveResource() )
			{
				// check for a different timer
				int iActiveTimer = ObjectiveResource()->GetTimerToShowInHUD();

				pTimer = dynamic_cast< CTeamRoundTimer* >( ClientEntityList().GetEnt( iActiveTimer ) );
				bDisplayTimer = ( iActiveTimer != 0 && pTimer && !pTimer->IsDormant() );
				m_pTimePanel->SetTimerIndex( iActiveTimer );
			}
		}

		if ( bDisplayTimer && !TFGameRules()->ShowMatchSummary() )
		{
			if ( !m_pTimePanel->IsVisible() )
			{
				m_pTimePanel->SetVisible( true );

				// If our spectator GUI is visible, invalidate its layout so that it moves the reinforcement label
				if ( g_pSpectatorGUI )
				{
					g_pSpectatorGUI->InvalidateLayout();
				}
			}
		}
		else 
		{
			if ( m_pTimePanel->IsVisible() )
			{
				m_pTimePanel->SetVisible( false );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudObjectiveStatus::Think()
{
	if ( !TeamplayRoundBasedRules() || !TFGameRules() )
		return;

	SetVisiblePanels();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFHudObjectiveStatus::ShouldDraw()
{
	if ( TFGameRules() && TFGameRules()->ShowMatchSummary() )
		return false;

	return CHudElement::ShouldDraw();
}

//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: HUD Target ID element
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_tf_player.h"
#include "c_playerresource.h"
#include "c_tf_playerresource.h"
#include "tf_classdata.h"
#include "hud.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "tf_imagepanel.h"
#include "tf_hud_playerstatus.h"
#include "tf_spectatorgui.h"
#include "vgui/ILocalize.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include <vgui/IVGui.h>
#include <vgui/IScheme.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/Frame.h>
#include "vgui_controls/TextImage.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/Button.h"
#include "ienginevgui.h"
#include "hud_chat.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CDisguiseStatus : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CDisguiseStatus, EditablePanel );

public:
	CDisguiseStatus( const char *pElementName );

	void			Init( void );
	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual void	Paint( void );

	virtual bool	ShouldDraw( void );

private:
	CPanelAnimationVar( HFont, m_hFont, "TextFont", "TargetID" );
};

DECLARE_HUDELEMENT( CDisguiseStatus );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CDisguiseStatus::CDisguiseStatus( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "DisguiseStatus" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );
}

//-----------------------------------------------------------------------------
// Purpose: Setup
//-----------------------------------------------------------------------------
void CDisguiseStatus::Init( void )
{

}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDisguiseStatus::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	SetPaintBackgroundEnabled( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CDisguiseStatus::ShouldDraw( void )
{
	bool bShow = false;
	CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return false;

	if ( !pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && !pPlayer->m_Shared.InCond( TF_COND_DISGUISING ) )
	{
		bShow = false;
	}
	else
	{
		bShow = true;
	}

	return bShow;
}

//-----------------------------------------------------------------------------
// Purpose: Draw function for the element
//-----------------------------------------------------------------------------
void CDisguiseStatus::Paint()
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	bool bDisguised = pPlayer->m_Shared.InCond( TF_COND_DISGUISED );
	if ( !bDisguised && !pPlayer->m_Shared.InCond( TF_COND_DISGUISING ) )
		return;
	
	// Avoid crashes
	int iDisguiseClass = bDisguised ? pPlayer->m_Shared.GetDisguiseClass() : pPlayer->m_Shared.GetDesiredDisguiseClass();
	if ( (iDisguiseClass > TF_LAST_NORMAL_CLASS || iDisguiseClass < TF_FIRST_NORMAL_CLASS) )
		return;

	// TF2007: This is all modified HL2MP target id code
	//

	wchar_t sIDString[ MAX_ID_STRING ];
	sIDString[0] = 0;

	int iDisguiseTeam = bDisguised ? pPlayer->m_Shared.GetDisguiseTeam() : pPlayer->m_Shared.GetDesiredDisguiseTeam();
	bool bBlue = iDisguiseTeam == TF_TEAM_BLUE;
	wchar_t *pwszTeamName = g_pVGuiLocalize->Find( bBlue ? "#TF_Spy_Disguise_Team_Blue" : "#TF_Spy_Disguise_Team_Red" );
	wchar_t *pwszClassName = g_pVGuiLocalize->Find( g_aPlayerClassNames[iDisguiseClass] );
	
	if ( bDisguised )
	{
		g_pVGuiLocalize->ConstructString( sIDString, sizeof(sIDString), g_pVGuiLocalize->Find( "#TF_Spy_Disguised_as" ), 2, pwszTeamName, pwszClassName );
	}
	else
	{
		g_pVGuiLocalize->ConstructString( sIDString, sizeof(sIDString), g_pVGuiLocalize->Find( "#TF_Spy_Disguising" ), 2, pwszTeamName, pwszClassName );
	}
	
	if ( sIDString[0] )
	{
		Color c = GameResources()->GetTeamColor( bDisguised ? pPlayer->m_Shared.GetDisguiseTeam() : pPlayer->GetTeamNumber() );

		// The black text
		vgui::surface()->DrawSetTextFont( m_hFont );
		vgui::surface()->DrawSetTextPos( 1, 1 );
		vgui::surface()->DrawSetTextColor( Color( 0, 0, 0, 255 ) );
		vgui::surface()->DrawPrintText( sIDString, wcslen(sIDString) );

		// The normal colored text
		vgui::surface()->DrawSetTextPos( 0, 0 );
		vgui::surface()->DrawSetTextColor( c );
		vgui::surface()->DrawPrintText( sIDString, wcslen(sIDString) );
		
	}
}
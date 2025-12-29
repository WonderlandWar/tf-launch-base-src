//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_SCOREBOARD_H
#define TF_SCOREBOARD_H
#ifdef _WIN32
#pragma once
#endif

#include "hud.h"
#include "hudelement.h"
#include "tf_hud_playerstatus.h"
#include "clientscoreboarddialog.h"
#include "ehandle.h"
#include "vgui_controls/ImagePanel.h"

class CAvatarImagePanel;
//class CTFStatsGraph;

//-----------------------------------------------------------------------------
// Purpose: displays the scoreboard
//-----------------------------------------------------------------------------

class CTFClientScoreBoardDialog : public CClientScoreBoardDialog
{
private:
	DECLARE_CLASS_SIMPLE( CTFClientScoreBoardDialog, CClientScoreBoardDialog );

public:
	CTFClientScoreBoardDialog( IViewPort *pViewPort );
	virtual ~CTFClientScoreBoardDialog();

	virtual void Reset() OVERRIDE;
	virtual void Update() OVERRIDE;
	virtual void ShowPanel( bool bShow ) OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;

	int	HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );
	
	MESSAGE_FUNC_PTR( OnItemSelected, "ItemSelected", panel );
	MESSAGE_FUNC_PTR( OnItemContextMenu, "ItemContextMenu", panel );
	void OnScoreBoardMouseRightRelease( void );


	MESSAGE_FUNC_PARAMS( OnVoteKickPlayer, "VoteKickPlayer", pData );

protected:
	virtual void PerformLayout();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	virtual void PostApplySchemeSettings( vgui::IScheme *pScheme ) {};

	vgui::SectionedListPanel *GetPlayerListRed( void ){ return m_pPlayerListRed; }
	vgui::SectionedListPanel *GetPlayerListBlue( void ){ return m_pPlayerListBlue; }

private:
	void InitPlayerList( vgui::SectionedListPanel *pPlayerList );
	void SetPlayerListImages( vgui::SectionedListPanel *pPlayerList );
	void UpdateTeamInfo();
	void UpdatePlayerList();
	void UpdateSpectatorList();
	void UpdatePlayerDetails();
	void ClearPlayerDetails();
	bool ShouldShowAsSpectator( int iPlayerIndex );
	void GetCameraUnderlayBounds( int *pX, int *pY, int *pWide, int *pTall );
	bool UseMouseMode( void );
	void InitializeInputScheme( void );

	void AdjustForVisibleScrollbar( void );

	virtual void FireGameEvent( IGameEvent *event );

	static bool TFPlayerSortFunc( vgui::SectionedListPanel *list, int itemID1, int itemID2 );

	vgui::SectionedListPanel *GetSelectedPlayerList( void );

	vgui::SectionedListPanel	*m_pPlayerListBlue;
	vgui::SectionedListPanel	*m_pPlayerListRed;
	CTFLabel					*m_pLabelPlayerName;
	CTFLabel					*m_pLabelDuelOpponentPlayerName;
	vgui::ImagePanel			*m_pImagePanelHorizLine;
	vgui::ImagePanel			*m_pClassImage;
	vgui::Menu					*m_pRightClickMenu;

	CTFLabel					*m_pKillsLabel;
	CTFLabel					*m_pDeathsLabel;
	CTFLabel					*m_pAssistLabel;
	CTFLabel					*m_pDestructionLabel;
	CTFLabel					*m_pCapturesLabel;
	CTFLabel					*m_pDefensesLabel;
	CTFLabel					*m_pDominationsLabel;
	CTFLabel					*m_pRevengeLabel;
	CTFLabel					*m_pHealingLabel;
	CTFLabel					*m_pInvulnsLabel;
	CTFLabel					*m_pTeleportsLabel;
	CTFLabel					*m_pHeadshotsLabel;
	CTFLabel					*m_pBackstabsLabel;
	CTFLabel					*m_pBonusLabel;
	CTFLabel					*m_pSupportLabel;
	CTFLabel					*m_pDamageLabel;

	vgui::HFont					m_pFontTimeLeftNumbers;
	vgui::HFont					m_pFontTimeLeftString;
	
	int							m_iImageDominated;
	int							m_iImageDominatedDead;
	int							m_iImageNemesis;
	int							m_iImageNemesisDead;

	int							m_iImageDom[SCOREBOARD_DOMINATION_ICONS];
	int							m_iImageDomDead[SCOREBOARD_DOMINATION_ICONS];
	int							m_iImageClass[SCOREBOARD_CLASS_ICONS];

//	bool						m_bDisplayLevel;
	bool						m_bMouseActivated;
	vgui::HFont					m_hScoreFontDefault;
	vgui::HFont					m_hScoreFontSmallest;

	CPanelAnimationVarAliasType( int, m_iNemesisWidth, "nemesis_width", "20", "proportional_int" );

	bool						m_bRedScrollBarVisible;
	bool						m_bBlueScrollBarVisible;
	int							m_nExtraSpace;

	CAvatarImagePanel			*m_pRedLeaderAvatarImage;
	EditablePanel				*m_pRedLeaderAvatarBG;
	vgui::ImagePanel			*m_pRedTeamImage;
	CAvatarImagePanel			*m_pBlueLeaderAvatarImage;
	EditablePanel				*m_pBlueLeaderAvatarBG;
	vgui::ImagePanel			*m_pBlueTeamImage;

	CHandle< C_TFPlayer >		m_hSelectedPlayer;
};

const wchar_t *GetPointsString( int iPoints );

#endif // TF_SCOREBOARD_H

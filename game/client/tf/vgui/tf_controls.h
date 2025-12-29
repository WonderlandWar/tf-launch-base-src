//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_CONTROLS_H
#define TF_CONTROLS_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/IScheme.h>
#include <vgui/KeyCode.h>
#include <KeyValues.h>
#include <vgui/IVGui.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/ScrollBar.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/RichText.h>
#include "utlvector.h"
#include "vgui_controls/PHandle.h"
#include <vgui_controls/Tooltip.h>
#if defined( TF_CLIENT_DLL )
#include "tf_shareddefs.h"
#include "tf_imagepanel.h"
#endif
#include <vgui_controls/Frame.h>
#include <../common/GameUI/scriptobject.h>
#include <vgui/KeyCode.h>
#include <vgui_controls/Tooltip.h>
#include <vgui_controls/CheckButton.h>
#include "steam/steamclientpublic.h"

wchar_t* LocalizeNumberWithToken( const char* pszLocToken, int nValue );
wchar_t* LocalizeNumberWithToken( const char* pszLocToken, int nValue1, int nValue2 );
void GetPlayerNameForSteamID( wchar_t *wCharPlayerName, int nBufSizeBytes, const CSteamID &steamID );
bool BGeneralPaintSetup( const Color& color );
void DrawFilledColoredCircle( float flXPos, float flYPos, float flRadius, const Color& color );
void DrawFilledColoredCircleSegment( float flXPos, float flYPos, float flRadiusOuter, float flRadiusInner, const Color& color, float flStartAngleOuter, float flEndAngleOuter, bool bCW = true );
void DrawFilledColoredCircleSegment( float flXPos, float flYPos, float flRadiusOuter, float flRadiusInner, const Color& color, float flStartAngleOuter, float flEndAngleOuter, float flStartAngleInner, float flEndAngleInner, bool bCW = true );
void DrawColoredCircle( float flXPos, float flYPos, float flRadius, const Color& color );
void BrigthenColor( Color& color, int nBrigthenAmount );
void CreateSwoop( int nX, int nY, int nWide, int nTall, float flDelay, bool bDown );

enum tooltippos_t
{
	TTP_ABOVE = 0,
	TTP_RIGHT_CENTERED,
	TTP_RIGHT,
	TTP_BELOW,
	TTP_LEFT,
	TTP_LEFT_CENTERED,

	MAX_POSITIONS
};

void PositionTooltip( const tooltippos_t ePreferredTooltipPosition, 
					  vgui::Panel* pMouseOverPanel,
					  vgui::Panel *pToolTipPanel );

//-----------------------------------------------------------------------------
// Purpose: A recreation of CTFLabel
//-----------------------------------------------------------------------------
class CTFLabel : public vgui::Label
{
public:
	DECLARE_CLASS_SIMPLE( CTFLabel, vgui::Label );

	CTFLabel( Panel *parent, const char *panelName, const char *text );
	
	virtual void	ApplySettings( KeyValues *pResourceData );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

private:
	char		m_szColor[64];
};

//-----------------------------------------------------------------------------
// Purpose: A recreation of CTFButton
//-----------------------------------------------------------------------------
class CTFButton : public vgui::Button
{
public:
	DECLARE_CLASS_SIMPLE( CTFButton, vgui::Button );

	CTFButton( vgui::Panel *parent, const char *name, const char *text, vgui::Panel *pActionSignalTarget = NULL, const char *cmd = NULL );
	CTFButton( vgui::Panel *parent, const char *name, const wchar_t *wszText, vgui::Panel *pActionSignalTarget = NULL, const char *cmd = NULL );
};

// TF2007: FIXME: Rename CExImageButton to CTFImageButton
//-----------------------------------------------------------------------------
// Purpose: Expanded image button, that handles images per button state, and color control in the .res file
//-----------------------------------------------------------------------------
class CExImageButton : public CTFButton
{
public:
	DECLARE_CLASS_SIMPLE( CExImageButton, CTFButton );

	CExImageButton( vgui::Panel *parent, const char *name, const char *text = "", vgui::Panel *pActionSignalTarget = NULL, const char *cmd = NULL );
	CExImageButton( vgui::Panel *parent, const char *name, const wchar_t *wszText = L"", vgui::Panel *pActionSignalTarget = NULL, const char *cmd = NULL );
	~CExImageButton( void );

	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	virtual void SetArmed(bool state);
	virtual void SetEnabled(bool state);
	virtual void SetSelected(bool state);
	void		 SetSubImage( const char *pszImage );

	void		 SetImageDefault( const char *pszImageDefault );
	void		 SetImageArmed( const char *pszImageArmed );
	void		 SetImageSelected( const char *pszImageSelected );

	Color		 GetImageColor( void );

	vgui::ImagePanel	*GetImage( void ) { return m_pEmbeddedImagePanel; }

private:
	// Embedded image panels
	vgui::ImagePanel	*m_pEmbeddedImagePanel;
	Color				m_ImageDrawColor;
	Color				m_ImageArmedColor;
	Color				m_ImageDisabledColor;
	Color				m_ImageSelectedColor;
	Color				m_ImageDepressedColor;
	char				m_szImageDefault[MAX_PATH];
	char				m_szImageArmed[MAX_PATH];
	char				m_szImageSelected[MAX_PATH];
};

//-----------------------------------------------------------------------------
// Purpose: Expanded Richtext control that allows customization of scrollbar display, font, and color .res controls.
//-----------------------------------------------------------------------------
class CTFRichText : public vgui::RichText
{
public:
	DECLARE_CLASS_SIMPLE( CTFRichText, vgui::RichText );

	CTFRichText( vgui::Panel *parent, const char *panelName );

	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout();
	virtual void SetText( const char *text );
	virtual void SetText( const wchar_t *text );

	virtual void OnTick( void );
	void SetScrollBarImagesVisible( bool visible );

	void SetFontStr( const char *pFont );
	void SetColorStr( const char *pColor );
	void SetCustomImage( vgui::Panel *pImage, const char *pszImage, char *pszStorage );

	void CreateImagePanels( void );

protected:
	char		m_szFont[64];
	char		m_szColor[64];
	char		m_szImageUpArrow[MAX_PATH];
	char		m_szImageDownArrow[MAX_PATH];
	char		m_szImageLine[MAX_PATH];
	char		m_szImageBox[MAX_PATH];
	bool		m_bUseImageBorders;

	CExImageButton		*m_pUpArrow;
	vgui::Panel			*m_pLine;
	CExImageButton		*m_pDownArrow;
	vgui::Panel			*m_pBox;
};

//-----------------------------------------------------------------------------
// Purpose: Xbox-specific panel that displays button icons text labels
//-----------------------------------------------------------------------------
class CTFFooter : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFFooter, vgui::EditablePanel );

public:
	CTFFooter( Panel *parent, const char *panelName );
	virtual ~CTFFooter();

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void	ApplySettings( KeyValues *pResourceData );
	virtual void	Paint( void );
	virtual void	PaintBackground( void );

	void			ShowButtonLabel( const char *name, bool show = true );
	void			AddNewButtonLabel( const char *name, const char *text, const char *icon );
	void			ClearButtons();

private:
	struct FooterButton_t
	{
		bool	bVisible;
		char	name[MAX_PATH];
		wchar_t	text[MAX_PATH];
		wchar_t	icon[3];			// icon can be one or two characters
	};

	CUtlVector< FooterButton_t* > m_Buttons;

	bool			m_bPaintBackground;		// fill the background?
	int				m_nButtonGap;			// space between buttons
	int				m_FooterTall;			// height of the footer
	int				m_ButtonOffsetFromTop;	// how far below the top the buttons should be drawn
	int				m_ButtonSeparator;		// space between the button icon and text
	int				m_TextAdjust;			// extra adjustment for the text (vertically)...text is centered on the button icon and then this value is applied
	bool			m_bCenterHorizontal;	// center buttons horizontally?
	int				m_ButtonPinRight;		// if not centered, this is the distance from the right margin that we use to start drawing buttons (right to left)

	char			m_szTextFont[64];		// font for the button text
	char			m_szButtonFont[64];		// font for the button icon
	char			m_szFGColor[64];		// foreground color (text)
	char			m_szBGColor[64];		// background color (fill color)

	vgui::HFont		m_hButtonFont;
	vgui::HFont		m_hTextFont;
};

//-----------------------------------------------------------------------------
// Purpose: A panel that can dragged and zoomed
//-----------------------------------------------------------------------------
class CDraggableScrollingPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CDraggableScrollingPanel, vgui::EditablePanel );
public:

	enum EPinPosition
	{
		PIN_TOP_LEFT = 0,
		PIN_TOP_RIGHT,
		PIN_BOTTOM_LEFT,
		PIN_BOTTOM_RIGHT,
		PIN_CENTER
	};

	// Child panel positions as a function of our width and height so that
	// when we scale the background, we can reposition these panels
	struct ChildPositionInfo_t
	{
		Panel* m_pChild;
		CUtlString m_strName;
		float m_flX;
		float m_flY;
		float m_flWide;
		float m_flTall;
		bool m_bScaleWithZoom;
		bool m_bMoveWithDrag;
		bool m_bWaitingForSettings;
		EPinPosition m_ePinPosition;
	};

	CDraggableScrollingPanel( Panel *pParent, const char *pszPanelname );

	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual void OnChildRemoved( Panel* pChild ) OVERRIDE;
	virtual void OnTick() OVERRIDE;

	virtual void OnMousePressed( vgui::MouseCode code ) OVERRIDE;
	virtual void OnMouseReleased( vgui::MouseCode code ) OVERRIDE;
	virtual void OnMouseWheeled( int delta ) OVERRIDE;

	MESSAGE_FUNC_INT_INT( InternalCursorMoved, "CursorMoved", xpos, ypos );
	MESSAGE_FUNC_PARAMS( OnSliderMoved, "SliderMoved", pParams );

	void AddOrUpdateChild( Panel* pChild, bool bScaleWithZoom, bool bMoveWithDrag, EPinPosition ePinPosition );
	void SetZoomAmount( float flZoomAmount, int nXZoomFocus, int nYZoomFocus );
	float GetZoomAmount() const { return m_flZoom; }

	const ChildPositionInfo_t* GetChildPositionInfo( const Panel* pChildPanel ) const;

private:

	bool BCheckForPendingChildren();
	virtual void OnChildSettingsApplied( KeyValues *pInResourceData, Panel *pChild ) OVERRIDE;
	void UpdateChildren();
	void CaptureChildSettings( Panel* pChild );

	CUtlVector< ChildPositionInfo_t > m_vecChildOriginalData;
	CUtlVector< ChildPositionInfo_t > m_vecPendingChildren;

	float m_flMinZoom;
	float m_flMaxZoom;
	float m_flZoom;
	float m_flMouseWheelZoomRate;

	int m_iOriginalWide;
	int m_iOriginalTall;
	int m_iDragStartX;
	int m_iDragStartY;
	int m_iDragTotalDistance;
	bool m_bDragging;
};

class CTFLogoPanel : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CTFLogoPanel, vgui::Panel );
public:
	CTFLogoPanel( Panel *pParent, const char *pszPanelname );

	virtual void Paint() OVERRIDE;

protected:
	CPanelAnimationVarAliasType( float, m_flRadius, "radius", "5", "float" );
	CPanelAnimationVarAliasType( float, m_flVelocity, "velocity", "0", "float" );

private:
	void PaintTFLogo( float flAngle, const Color& color ) const;

	float m_flOffsetAngle = 0.f;
};

#endif // TF_CONTROLS_H

//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"

#include "ienginevgui.h"
#include <vgui_controls/ScrollBarSlider.h>
#include "vgui/ILocalize.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include "tf_controls.h"
#include "vgui_controls/TextImage.h"
#include "vgui_controls/PropertyPage.h"
#include "iachievementmgr.h"
#include "clientmode_tf.h"
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/ListPanel.h>
#include <vgui_controls/PanelListPanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/MessageBox.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/TextEntry.h>
#include <../common/GameUI/cvarslider.h>
#include "filesystem.h"
#include "hud_controlpointicons.h"

using namespace vgui;

wchar_t* LocalizeNumberWithToken( const char* pszLocToken, int nValue )
{
	static wchar_t wszOutString[ 256 ];
	wchar_t wszCount[ 16 ];
	_snwprintf( wszCount, ARRAYSIZE( wszCount ), L"%d", nValue );
	const wchar_t *wpszFormat = g_pVGuiLocalize->Find( pszLocToken );
	g_pVGuiLocalize->ConstructString_safe( wszOutString, wpszFormat, 1, wszCount );
	
	return wszOutString;
}

wchar_t* LocalizeNumberWithToken( const char* pszLocToken, int nValue1, int nValue2 )
{
	static wchar_t wszOutString[ 256 ];
	wchar_t wszCount1[ 16 ];
	wchar_t wszCount2[ 16 ];
	_snwprintf( wszCount1, ARRAYSIZE( wszCount1 ), L"%d", nValue1 );
	_snwprintf( wszCount2, ARRAYSIZE( wszCount2 ), L"%d", nValue2 );
	const wchar_t *wpszFormat = g_pVGuiLocalize->Find( pszLocToken );
	g_pVGuiLocalize->ConstructString_safe( wszOutString, wpszFormat, 2, wszCount1, wszCount2 );

	return wszOutString;
}

void GetPlayerNameForSteamID( wchar_t *wCharPlayerName, int nBufSizeBytes, const CSteamID &steamID )
{
	const char *pszName = steamapicontext->SteamFriends()->GetFriendPersonaName( steamID );
	V_UTF8ToUnicode( pszName, wCharPlayerName, nBufSizeBytes );
}

bool BGeneralPaintSetup( const Color& color )
{
	static int snWhiteTextureID = -1;
	if ( snWhiteTextureID == -1 )
	{
		snWhiteTextureID = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile( snWhiteTextureID, "vgui/white" , true, false);
		if (snWhiteTextureID == -1)
			return false;

	}

	vgui::surface()->DrawSetTexture( snWhiteTextureID );
	vgui::surface()->DrawSetColor( color );

	return true;
}

void DrawColoredCircle( float flXPos, float flYPos, float flRadius, const Color& color )
{
	if ( !BGeneralPaintSetup( color ) )
		return;

	// Create a circle by creating verts
	const int nNumSmallSegments = 20;
	const int nNumLargeSements = 50;
	int nNumSegments = RemapValClamped( flRadius, 50.f, 300.f, (float)nNumSmallSegments, (float)nNumLargeSements );

	surface()->DrawOutlinedCircle( flXPos, flYPos, flRadius, nNumSegments );
}

void DrawFilledColoredCircle( float flXPos, float flYPos, float flRadius, const Color& color )
{
	if ( !BGeneralPaintSetup( color ) )
		return;

	// Create a circle by creating verts
	const int nNumSmallSegments = 20;
	const int nNumLargeSements = 50;
	int nNumSegments = RemapValClamped( flRadius, 50.f, 300.f, (float)nNumSmallSegments, (float)nNumLargeSements );

	vgui::Vertex_t verts[ nNumLargeSements ];

	float invDelta = 2.0f * M_PI / nNumSegments;
	for ( int i = 0; i < nNumSegments; ++i )
	{
		float flRadians = i * invDelta;
		float ca = cos( flRadians );
		float sa = sin( flRadians );

		// Rotate it around the circle
		verts[i].m_Position.x = flXPos + (flRadius * ca);
		verts[i].m_Position.y = flYPos + (flRadius * sa);
		verts[i].m_TexCoord.x = 0.5f * (ca + 1.0f);
		verts[i].m_TexCoord.y = 0.5f * (sa + 1.0f);
	}

	surface()->DrawTexturedPolygon( nNumSegments, verts, false );
}

void DrawFilledColoredCircleSegment( float flXPos, float flYPos, float flRadiusOuter, float flRadiusInner, const Color& color, float flStartAngle, float flEndAngle, bool bCW /* = true */ )
{
	DrawFilledColoredCircleSegment( flXPos, flYPos, flRadiusOuter, flRadiusInner, color, flStartAngle, flEndAngle, flStartAngle, flEndAngle, bCW );
}

void DrawFilledColoredCircleSegment( float flXPos, float flYPos, float flRadiusOuter, float flRadiusInner, const Color& color, float flStartAngleOuter, float flEndAngleOuter, float flStartAngleInner, float flEndAngleInner, bool bCW /*= true*/ )
{
	if ( !BGeneralPaintSetup( color ) )
		return;

	if ( flEndAngleOuter < flStartAngleOuter )
	{
		flEndAngleOuter += 360.f;
	}

	if ( flEndAngleInner < flStartAngleInner )
	{
		flEndAngleInner += 360.f;
	}

	// Create a circle by creating verts
	const int nNumSmallSegments = 20;
	const int nNumLargeSements = 50;
	float flAngleDelta[2];
	flAngleDelta[ 0 ] = flEndAngleOuter - flStartAngleOuter;
	flAngleDelta[ 1 ] = flEndAngleInner - flStartAngleInner;

	//if ( flAngleDelta > 180.f )
	//{
	//	DrawFilledColoredCircleSegment( flXPos, flYPos, flRadiusOuter, flRadiusInner, color, flStartAngle, flStartAngle + 180.f, bCW );
	//	flStartAngle += 180.f;
	//	flAngleDelta -= 180.f;
	//}

	if ( !bCW )
	{
		flStartAngleOuter = flStartAngleOuter - flAngleDelta[ 0 ];
		flEndAngleOuter = flStartAngleOuter + flAngleDelta[ 0 ];

		flStartAngleInner = flStartAngleInner - flAngleDelta[ 1 ];
		flEndAngleInner = flStartAngleInner + flAngleDelta[ 1 ];
	}

	int nNumSegments = RemapValClamped( flRadiusOuter, 50.f, 300.f, (float)nNumSmallSegments, (float)nNumLargeSements );
	nNumSegments = RemapValClamped( Max( flAngleDelta[ 0 ], flAngleDelta[ 1 ] ), 1, 360, 1, nNumSegments );

	float flStartRadAngle[ 2 ];
	float flDeltaRadAngle[ 2 ];

	flStartRadAngle[ 0 ] = DEG2RAD(flStartAngleOuter);
	flDeltaRadAngle[ 0 ] = DEG2RAD(flAngleDelta[ 0 ]);
	flStartRadAngle[ 1 ] = DEG2RAD(flStartAngleInner);
	flDeltaRadAngle[ 1 ] = DEG2RAD(flAngleDelta[ 1 ]);

	vgui::Vertex_t verts[ 4 ];

	float invDelta[ 2 ] = { flDeltaRadAngle[ 0 ] / nNumSegments, flDeltaRadAngle[ 1 ] / nNumSegments };
	float fl90 = DEG2RAD( 90 );

	auto lambdaCompVerts = [&]( float flAngOuter, float flAngInner, vgui::Vertex_t& vert0, vgui::Vertex_t& vert1 )
	{
		float ca = cos( flAngOuter - fl90 );
		float sa = sin( flAngOuter - fl90 );

		// Rotate it around the circle
		vert0.m_Position.x = flXPos + (flRadiusOuter * ca);
		vert0.m_Position.y = flYPos + (flRadiusOuter * sa);
		vert0.m_TexCoord.x = 0.5f * (ca + 1.0f);
		vert0.m_TexCoord.y = 0.5f * (sa + 1.0f);

		ca = cos( flAngInner - fl90 );
		sa = sin( flAngInner - fl90 );

		vert1.m_Position.x = flXPos + (flRadiusInner * ca);
		vert1.m_Position.y = flYPos + (flRadiusInner * sa);
		vert1.m_TexCoord.x = 0.5f * (ca + 1.0f);
		vert1.m_TexCoord.y = 0.5f * (sa + 1.0f);
	};

	// Seed 2 and 3 so the loop below works
	lambdaCompVerts( flStartRadAngle[ 0 ], flStartRadAngle[ 1 ], verts[ 2 ], verts[ 3 ] ); 

	for ( int i = 0; i <= nNumSegments; ++i )
	{
		// Use the previous leading-edge verts as the start-edge verts for
		// the next section.
		verts[ 0 ] = verts[ 3 ];
		verts[ 1 ] = verts[ 2 ];
		// Compute the leading-edge verts
		lambdaCompVerts( flStartRadAngle[ 0 ] + i * invDelta[ 0 ],
						 flStartRadAngle[ 1 ] + i * invDelta[ 1 ],
						 verts[ 2 ], verts[ 3 ] );

		surface()->DrawTexturedPolygon( 4, verts, true );
	}
}

void BrigthenColor( Color& color, int nBrigthenAmount )
{
	color.SetColor( Min( 255, color.r() + nBrigthenAmount ),
					Min( 255, color.g() + nBrigthenAmount ),
					Min( 255, color.b() + nBrigthenAmount ),
					color.a() );
};

//-----------------------------------------------------------------------------
// Purpose: Tests a position for moving a tooltip panel and returns a score.
//			Returns how many pixels of the mouseover panel are covered by the 
//			tooltip panel.  A score of 0 is perfect.
//-----------------------------------------------------------------------------
int AttemptPositionTooltip( const tooltippos_t eTooltipPosition, 
							 Panel* pMouseOverPanel,
							 Panel *pToolTipPanel,
							 int &iXPos, 
							 int &iYPos )
{
	int iPanelX = pMouseOverPanel->GetXPos();
	int iPanelY = pMouseOverPanel->GetYPos();
	pMouseOverPanel->ParentLocalToScreen( iPanelX, iPanelY );

	switch ( eTooltipPosition )
	{
		case TTP_LEFT:
			iXPos = (iPanelX - pToolTipPanel->GetWide() + XRES(18));
			iYPos = iPanelY - YRES(7);
			break;
		case TTP_RIGHT: 
			iXPos = (iPanelX + pMouseOverPanel->GetWide() - XRES(20));
			iYPos = iPanelY - YRES(7);
			break;
		case TTP_LEFT_CENTERED:
			iXPos = (iPanelX - pToolTipPanel->GetWide()) - XRES(4);
			iYPos = (iPanelY - (pToolTipPanel->GetTall() * 0.5));
			break;
		case TTP_RIGHT_CENTERED:
			iXPos = (iPanelX + pMouseOverPanel->GetWide()) + XRES(4);
			iYPos = (iPanelY - (pToolTipPanel->GetTall() * 0.5));
			break;
		case TTP_ABOVE:
			iXPos = (iPanelX + (pMouseOverPanel->GetWide() * 0.5)) - (pToolTipPanel->GetWide() * 0.5);
			iYPos = (iPanelY - pToolTipPanel->GetTall() - YRES(4));
			break;
		case TTP_BELOW:
			iXPos = (iPanelX + (pMouseOverPanel->GetWide() * 0.5)) - (pToolTipPanel->GetWide() * 0.5);
			iYPos = (iPanelY + pMouseOverPanel->GetTall() + YRES(4));
			break;
	}

	int iScreenX, iScreenY;
	surface()->GetScreenSize( iScreenX, iScreenY );

	// Make sure the panel stays on screen
	iXPos = clamp( iXPos, 0, iScreenX - pToolTipPanel->GetWide() );
	iYPos = clamp( iYPos, 0, iScreenY - pToolTipPanel->GetTall() );

	// Detect how much overlap we have in X
	int nXMin = Max( iXPos, pMouseOverPanel->GetXPos() );
	int nXMax = Min( iXPos + pToolTipPanel->GetWide(), pMouseOverPanel->GetXPos() + pMouseOverPanel->GetWide() );
	int nXScore = Max( 0, nXMax - nXMin );

	// Detect overlap in Y
	int nYMin = Max( iYPos, pMouseOverPanel->GetYPos() );
	int nYMax = Min( iYPos + pToolTipPanel->GetTall(), pMouseOverPanel->GetYPos() + pMouseOverPanel->GetTall() );
	int nYScore = Max( 0, nYMax - nYMin );

	return nXScore + nYScore;
}

//-----------------------------------------------------------------------------
// Purpose: Takes a mouse over panel, a tooltip panel and a preferred position
//			and tries to move the tooltip into a nice position next to the
//			mouse over panel.
//-----------------------------------------------------------------------------
void PositionTooltip( const tooltippos_t ePreferredTooltipPosition, 
					  vgui::Panel* pMouseOverPanel,
					  vgui::Panel *pToolTipPanel )
{
	struct PosResult_t
	{
		int nScore;
		int nX;
		int nY;
	};
	PosResult_t arResults[ MAX_POSITIONS ];

	int nBestScore = INT_MAX;
	tooltippos_t eBestType = MAX_POSITIONS;

	float flAverageScore = 0;
	// Try all of the positions and score each by how much they overlap.
	for( int i=0; i < MAX_POSITIONS; ++i )
	{
		arResults[ i ].nScore = AttemptPositionTooltip( (tooltippos_t)i,
														pMouseOverPanel,
														pToolTipPanel,
														arResults[ i ].nX,
														arResults[ i ].nY );
		
		flAverageScore += arResults[ i ].nScore;
		// 0 is a perfect score.
		if ( arResults[ i ].nScore < nBestScore )
		{
			eBestType = (tooltippos_t)i;
			nBestScore = arResults[ i ].nScore;
		}
	}

	flAverageScore /= MAX_POSITIONS;

	Assert( eBestType != MAX_POSITIONS );
	// Go ahead and use their preferred if it's decent
	if( arResults[ ePreferredTooltipPosition ].nScore < ( flAverageScore / 2.f ) )
	{
		pToolTipPanel->SetPos( arResults[ ePreferredTooltipPosition ].nX,
							   arResults[ ePreferredTooltipPosition ].nY );
		return;
	}
	
	// Go with the best we've got
	pToolTipPanel->SetPos( arResults[ eBestType ].nX, arResults[ eBestType ].nY );
}

DECLARE_BUILD_FACTORY( CTFFooter );
DECLARE_BUILD_FACTORY( CTFRichText );
DECLARE_BUILD_FACTORY_DEFAULT_TEXT( CTFLabel, CTFLabel );
DECLARE_BUILD_FACTORY_DEFAULT_TEXT( CTFButton, CTFButton );

CTFLabel::CTFLabel( Panel *parent, const char *panelName, const char *text ) : BaseClass( parent, panelName, text ) 
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLabel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
	
	const char *fgcolor = inResourceData->GetString( "fgcolor", "Label.TextColor" );

	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
	Color cColor = pScheme->GetColor( fgcolor, Color( 0, 255, 0, 255 ) );
	
	Q_snprintf( m_szColor, ARRAYSIZE(m_szColor), "%d %d %d %d", cColor.r(), cColor.g(), cColor.b(), cColor.a() );
	SetFgColor( cColor );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLabel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// Reapply our custom color, so we stomp the base scheme's
	Color cColor = pScheme->GetColor( m_szColor, Color( 0, 255, 0, 255 ) );
	
	Q_snprintf( m_szColor, ARRAYSIZE(m_szColor), "%d %d %d %d", cColor.r(), cColor.g(), cColor.b(), cColor.a() );
	SetFgColor( cColor );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFButton::CTFButton( Panel *parent, const char *name, const char *text, vgui::Panel *pActionSignalTarget, const char *cmd ) : Button( parent, name, text, pActionSignalTarget, cmd )
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFButton::CTFButton( vgui::Panel *parent, const char *name, const wchar_t *wszText, vgui::Panel *pActionSignalTarget, const char *cmd ) : Button( parent, name, wszText, pActionSignalTarget, cmd )
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFRichText::CTFRichText( Panel *parent, const char *name ) : RichText( parent, name )
{
	m_szFont[0] = '\0';
	m_szColor[0] = '\0';
	m_szImageUpArrow[0] = '\0';
	m_szImageDownArrow[0] = '\0';
	m_szImageLine[0] = '\0';
	m_szImageBox[0] = '\0';
	m_bUseImageBorders = false;
	m_pBox = NULL;
	m_pLine = NULL;

	SetCursor(dc_arrow);

	m_pUpArrow = new CExImageButton( this, "UpArrow", "" );
	if ( m_pUpArrow )
	{
		m_pUpArrow->AddActionSignalTarget( _vertScrollBar );
		m_pUpArrow->SetCommand(new KeyValues("ScrollButtonPressed", "index", 0));
		m_pUpArrow->GetImage()->SetShouldScaleImage( true );
		m_pUpArrow->SetFgColor( Color( 255, 255, 255, 255 ) );
		m_pUpArrow->SetAlpha( 255 );
		m_pUpArrow->SetPaintBackgroundEnabled( false );
		m_pUpArrow->SetVisible( false );
	}

	m_pDownArrow = new CExImageButton( this, "DownArrow", "" );
	if ( m_pDownArrow )
	{
		m_pDownArrow->AddActionSignalTarget( _vertScrollBar );
		m_pDownArrow->SetCommand(new KeyValues("ScrollButtonPressed", "index", 1));
		m_pDownArrow->GetImage()->SetShouldScaleImage( true );
		m_pDownArrow->SetFgColor( Color( 255, 255, 255, 255 ) );
		m_pDownArrow->SetAlpha( 255 );
		m_pDownArrow->SetPaintBackgroundEnabled( false );
		m_pDownArrow->SetVisible( false );
	}

	_vertScrollBar->SetOverriddenButtons( m_pUpArrow, m_pDownArrow );
	// TF2007: Might need to implement this
	//m_pUpArrow->PassMouseTicksTo( _vertScrollBar );
	//m_pDownArrow->PassMouseTicksTo( _vertScrollBar );

	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRichText::CreateImagePanels( void )
{
	if ( m_pBox || m_pLine )
		return;

	if ( m_bUseImageBorders )
	{
		m_pLine = new vgui::Panel( this, "Line" );
		m_pBox = new vgui::Panel( this, "Box" );
	}
	else
	{
		m_pLine = new vgui::ImagePanel( this, "Line" );
		m_pBox = new vgui::ImagePanel( this, "Box" );

		dynamic_cast<vgui::ImagePanel *>(m_pBox)->SetShouldScaleImage( true );
		dynamic_cast<vgui::ImagePanel *>(m_pLine)->SetShouldScaleImage( true );
	}
	m_pBox->SetVisible( false );
	m_pLine->SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRichText::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	SetFontStr( inResourceData->GetString( "font", "Default" ) );
	SetColorStr( inResourceData->GetString( "fgcolor", "RichText.TextColor" ) );

	SetCustomImage( m_pUpArrow->GetImage(), inResourceData->GetString( "image_up_arrow", "chalkboard_scroll_up" ), m_szImageUpArrow );
	SetCustomImage( m_pDownArrow->GetImage(), inResourceData->GetString( "image_down_arrow", "chalkboard_scroll_down" ), m_szImageDownArrow );
	SetCustomImage( m_pLine, inResourceData->GetString( "image_line", "chalkboard_scroll_line" ), m_szImageLine );
	SetCustomImage( m_pBox, inResourceData->GetString( "image_box", "chalkboard_scroll_box" ), m_szImageBox );

	const char *pszMouseover = inResourceData->GetString( "image_up_arrow_mouseover", NULL );
	if ( pszMouseover )
	{
		m_pUpArrow->SetImageArmed( pszMouseover );
		m_pUpArrow->SetImageDefault( m_szImageUpArrow );
	}
	pszMouseover = inResourceData->GetString( "image_down_arrow_mouseover", NULL );
	if ( pszMouseover )
	{
		m_pDownArrow->SetImageArmed( pszMouseover );
		m_pDownArrow->SetImageDefault( m_szImageDownArrow );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRichText::SetFontStr( const char *pFont )
{
	if ( pFont != m_szFont )
	{
		V_strcpy_safe( m_szFont, pFont );
	}

	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
	SetFont( pScheme->GetFont( m_szFont, true ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRichText::SetColorStr( const char *pColor )
{
	if ( pColor != m_szColor )
	{
		V_strcpy_safe( m_szColor, pColor );
	}

	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
	SetFgColor( pScheme->GetColor( m_szColor, Color( 255, 255, 255, 255 ) ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRichText::SetCustomImage( vgui::Panel *pImage, const char *pszImage, char *pszStorage )
{
	if ( pszStorage )
	{
		V_strcpy( pszStorage, pszImage );
	}
	if ( !pImage )
		return;

	if ( m_bUseImageBorders )
	{
		vgui::IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
		IBorder *pBorder = pScheme->GetBorder( pszImage );

		if ( pBorder )
		{
			pImage->SetBorder( pBorder );
			return;
		}
	}

	vgui::ImagePanel *pImagePanel = dynamic_cast<vgui::ImagePanel *>(pImage);
	if ( pImagePanel )
	{
		pImagePanel->SetImage( pszImage );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRichText::ApplySchemeSettings( IScheme *pScheme )
{
	CreateImagePanels();

	BaseClass::ApplySchemeSettings( pScheme );

	// Reapply any custom font/color, so we stomp the base scheme's
	SetFontStr( m_szFont );
	SetColorStr( m_szColor );
	SetCustomImage( m_pUpArrow->GetImage(), m_szImageUpArrow, NULL );
	SetCustomImage( m_pDownArrow->GetImage(), m_szImageDownArrow, NULL );
	SetCustomImage( m_pLine, m_szImageLine, NULL );
	SetCustomImage( m_pBox, m_szImageBox, NULL );

	SetBorder( pScheme->GetBorder( "NoBorder" ) );
	SetBgColor( pScheme->GetColor( "Blank", Color( 0,0,0,0 ) ) );
	SetPanelInteractive( false );
	SetUnusedScrollbarInvisible( true );

	if ( m_pDownArrow  )
	{
		m_pDownArrow->SetFgColor( Color( 255, 255, 255, 255 ) );
	}

	if ( m_pUpArrow  )
	{
		m_pUpArrow->SetFgColor( Color( 255, 255, 255, 255 ) );
	}

	SetScrollBarImagesVisible( false );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRichText::PerformLayout()
{
	BaseClass::PerformLayout();

	if ( _vertScrollBar )
	{
		_vertScrollBar->SetZPos( 500 );
		m_pUpArrow->SetZPos( 501 );
		m_pDownArrow->SetZPos( 501 );
		
		// turn off painting the vertical scrollbar
		_vertScrollBar->SetPaintBackgroundEnabled( false );
		_vertScrollBar->SetPaintBorderEnabled( false );
		_vertScrollBar->SetPaintEnabled( false );
		_vertScrollBar->SetScrollbarButtonsVisible( false );
		_vertScrollBar->GetButton(0)->SetMouseInputEnabled( false );
		_vertScrollBar->GetButton(1)->SetMouseInputEnabled( false );
		
		if (  _vertScrollBar->IsVisible() )
		{
			int nMin, nMax;
			_vertScrollBar->GetRange( nMin, nMax );
			_vertScrollBar->SetValue( nMin );

			int nScrollbarWide = _vertScrollBar->GetWide();

			int wide, tall;
			GetSize( wide, tall );

			if ( m_pUpArrow )
			{
				m_pUpArrow->SetBounds( wide - nScrollbarWide, 0, nScrollbarWide, nScrollbarWide );
				m_pUpArrow->GetImage()->SetSize( nScrollbarWide, nScrollbarWide );
			}

			if ( m_pLine )
			{
				m_pLine->SetBounds( wide - nScrollbarWide, nScrollbarWide, nScrollbarWide, tall - ( 2 * nScrollbarWide ) );
			}

			if ( m_pBox )
			{
				m_pBox->SetBounds( wide - nScrollbarWide, nScrollbarWide, nScrollbarWide, nScrollbarWide );
			}

			if ( m_pDownArrow )
			{
				m_pDownArrow->SetBounds( wide - nScrollbarWide, tall - nScrollbarWide, nScrollbarWide, nScrollbarWide );
				m_pDownArrow->GetImage()->SetSize( nScrollbarWide, nScrollbarWide );
			}

			SetScrollBarImagesVisible( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRichText::SetText( const wchar_t *text )
{
	wchar_t buffer[2048];
	Q_wcsncpy( buffer, text, sizeof( buffer ) );

	// transform '\r' to ' ' to eliminate double-spacing on line returns
	for ( wchar_t *ch = buffer; *ch != 0; ch++ )
	{
		if ( *ch == '\r' )
		{
			*ch = ' ';
		}
	}

	BaseClass::SetText( buffer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRichText::SetText( const char *text )
{
	char buffer[2048];
	Q_strncpy( buffer, text, sizeof( buffer ) );

	// transform '\r' to ' ' to eliminate double-spacing on line returns
	for ( char *ch = buffer; *ch != 0; ch++ )
	{
		if ( *ch == '\r' )
		{
			*ch = ' ';
		}
	}

	BaseClass::SetText( buffer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRichText::SetScrollBarImagesVisible( bool visible )
{
	if ( m_pDownArrow && m_pDownArrow->IsVisible() != visible )
	{
		m_pDownArrow->SetVisible( visible );
		m_pDownArrow->SetEnabled( visible );
	}

	if ( m_pUpArrow && m_pUpArrow->IsVisible() != visible )
	{
		m_pUpArrow->SetVisible( visible );
		m_pUpArrow->SetEnabled( visible );
	}

	if ( m_pLine && m_pLine->IsVisible() != visible )
	{
		m_pLine->SetVisible( visible );
	}

	if ( m_pBox && m_pBox->IsVisible() != visible )
	{
		m_pBox->SetVisible( visible );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRichText::OnTick()
{
	if ( !IsVisible() )
		return;

	if ( m_pDownArrow && m_pUpArrow && m_pLine && m_pBox )
	{
		if ( _vertScrollBar && _vertScrollBar->IsVisible() )
		{
			// turn on our own images
			SetScrollBarImagesVisible ( true );

			// set the alpha on the up arrow
			int nMin, nMax;
			_vertScrollBar->GetRange( nMin, nMax );
			int nScrollPos = _vertScrollBar->GetValue();
			int nRangeWindow = _vertScrollBar->GetRangeWindow();
			int nBottom = nMax - nRangeWindow;
			if ( nBottom < 0 )
			{
				nBottom = 0;
			}

			// set the alpha on the up arrow
			int nAlpha = ( nScrollPos - nMin <= 0 ) ? 90 : 255;
			m_pUpArrow->SetAlpha( nAlpha );

			// set the alpha on the down arrow
			nAlpha = ( nScrollPos >= nBottom ) ? 90 : 255;
			m_pDownArrow->SetAlpha( nAlpha );

			ScrollBarSlider *pSlider = _vertScrollBar->GetSlider();
			if ( pSlider && pSlider->GetRangeWindow() > 0 )
			{
				int x, y, w, t, min, max;
				m_pLine->GetBounds( x, y, w, t );
				pSlider->GetNobPos( min, max );

				m_pBox->SetBounds( x, y + min, w, ( max - min ) );
			}
		}
		else
		{
			// turn off our images
			SetScrollBarImagesVisible ( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CExImageButton::CExImageButton( Panel *parent, const char *name, const char *text, vgui::Panel *pActionSignalTarget, const char *cmd ) : CTFButton( parent, name, text, pActionSignalTarget, cmd )
{
	m_ImageDrawColor = Color(255,255,255,255);
	m_ImageArmedColor = Color(255,255,255,255);
	m_ImageDepressedColor = Color(255,255,255,255);
	m_ImageDisabledColor = Color(255,255,255,255);
	m_ImageSelectedColor = Color(255,255,255,255);
	m_pEmbeddedImagePanel = new vgui::ImagePanel( this, "SubImage" );
	m_szImageDefault[0] = '\0';
	m_szImageArmed[0] = '\0';
	m_szImageSelected[0] = '\0';
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CExImageButton::CExImageButton( Panel *parent, const char *name, const wchar_t *wszText, vgui::Panel *pActionSignalTarget, const char *cmd ) : CTFButton( parent, name, wszText, pActionSignalTarget, cmd )
{
	m_ImageDrawColor = Color(255,255,255,255);
	m_ImageArmedColor = Color(255,255,255,255);
	m_ImageDepressedColor = Color(255,255,255,255);
	m_ImageDisabledColor = Color(255,255,255,255);
	m_ImageSelectedColor = Color(255,255,255,255);
	m_pEmbeddedImagePanel = new vgui::ImagePanel( this, "SubImage" );
	m_szImageDefault[0] = '\0';
	m_szImageArmed[0] = '\0';
	m_szImageSelected[0] = '\0';
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CExImageButton::~CExImageButton( void )
{
	m_pEmbeddedImagePanel->MarkForDeletion();
	m_pEmbeddedImagePanel = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExImageButton::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	int r,g,b,a;

	const char *pszDrawColor = inResourceData->GetString("image_drawcolor", "");
	if (*pszDrawColor)
	{
		if (sscanf(pszDrawColor, "%d %d %d %d", &r, &g, &b, &a) >= 3)
		{
			m_ImageDrawColor = Color(r, g, b, a);
		}
	}

	pszDrawColor = inResourceData->GetString("image_armedcolor", "");
	if (*pszDrawColor)
	{
		if (sscanf(pszDrawColor, "%d %d %d %d", &r, &g, &b, &a) >= 3)
		{
			m_ImageArmedColor = Color(r, g, b, a);
		}
	}

	pszDrawColor = inResourceData->GetString("image_depressedcolor", "");
	if (*pszDrawColor)
	{
		if (sscanf(pszDrawColor, "%d %d %d %d", &r, &g, &b, &a) >= 3)
		{
			m_ImageDepressedColor = Color(r, g, b, a);
		}
	}

	pszDrawColor = inResourceData->GetString("image_disabledcolor", "");
	if (*pszDrawColor)
	{
		if (sscanf(pszDrawColor, "%d %d %d %d", &r, &g, &b, &a) >= 3)
		{
			m_ImageDisabledColor = Color(r, g, b, a);
		}
	}

	pszDrawColor = inResourceData->GetString( "image_selectedcolor", "" );
	if (*pszDrawColor)
	{
		if (sscanf(pszDrawColor, "%d %d %d %d", &r, &g, &b, &a) >= 3)
		{
			m_ImageSelectedColor = Color(r, g, b, a);
		}
	}

	KeyValues *pButtonKV = inResourceData->FindKey( "SubImage" );
	if ( pButtonKV )
	{
		m_pEmbeddedImagePanel->ApplySettings( pButtonKV );
	}

	const char *pszImageDefault = inResourceData->GetString("image_default", "");
	if (*pszImageDefault)
	{
		SetImageDefault( pszImageDefault );
	}

	const char *pszImageArmed = inResourceData->GetString("image_armed", "");
	if (*pszImageArmed)
	{
		SetImageArmed( pszImageArmed );
	}

	const char *pszImageSelected = inResourceData->GetString("image_selected", "");
	if (*pszImageSelected)
	{
		SetImageSelected( pszImageSelected );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Color CExImageButton::GetImageColor( void )
{
	if ( !IsEnabled() )
		return m_ImageDisabledColor;
	if ( IsSelected() )
		return m_ImageSelectedColor;
	if ( IsDepressed() )
		return m_ImageDepressedColor;
	if ( IsArmed() )
		return m_ImageArmedColor;
	return m_ImageDrawColor;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExImageButton::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	m_pEmbeddedImagePanel->SetMouseInputEnabled( false );
	m_pEmbeddedImagePanel->SetDrawColor( GetImageColor() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExImageButton::SetArmed(bool state)
{
	BaseClass::SetArmed( state );

	if ( m_pEmbeddedImagePanel )
	{
		m_pEmbeddedImagePanel->SetDrawColor( GetImageColor() );

		const char *pszImage = state ? m_szImageArmed : m_szImageDefault;
		if ( *pszImage )
		{
			SetSubImage( pszImage );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExImageButton::SetEnabled(bool state)
{
	BaseClass::SetEnabled( state );

	if ( m_pEmbeddedImagePanel )
	{
		m_pEmbeddedImagePanel->SetDrawColor( GetImageColor() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExImageButton::SetSelected(bool state)
{
	BaseClass::SetSelected( state );

	if ( m_pEmbeddedImagePanel )
	{
		m_pEmbeddedImagePanel->SetDrawColor( GetImageColor() );

		const char *pszImage = state ? m_szImageSelected : m_szImageDefault;
		if ( *pszImage )
		{
			SetSubImage( pszImage );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExImageButton::SetSubImage( const char *pszImage )
{
	m_pEmbeddedImagePanel->SetImage( pszImage );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExImageButton::SetImageDefault( const char *pszImageDefault )
{
	V_strcpy_safe( m_szImageDefault, pszImageDefault );	
	if ( !IsArmed() )
	{
		SetSubImage( pszImageDefault );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExImageButton::SetImageArmed( const char *pszImageArmed )
{
	V_strcpy_safe( m_szImageArmed, pszImageArmed );
	if ( IsArmed() )
	{
		SetSubImage( m_szImageArmed );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExImageButton::SetImageSelected( const char *pszImageSelected )
{
	V_strcpy_safe( m_szImageSelected, pszImageSelected );
	if ( IsSelected() )
	{
		SetSubImage( m_szImageSelected );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Xbox-specific panel that displays button icons text labels
//-----------------------------------------------------------------------------
CTFFooter::CTFFooter( Panel *parent, const char *panelName ) : BaseClass( parent, panelName ) 
{
	SetVisible( true );
	SetAlpha( 0 );

	m_nButtonGap = 32;
	m_ButtonPinRight = 100;
	m_FooterTall = 80;

	m_ButtonOffsetFromTop = 0;
	m_ButtonSeparator = 4;
	m_TextAdjust = 0;

	m_bPaintBackground = false;
	m_bCenterHorizontal = true;

	m_szButtonFont[0] = '\0';
	m_szTextFont[0] = '\0';
	m_szFGColor[0] = '\0';
	m_szBGColor[0] = '\0';
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFooter::~CTFFooter()
{
	ClearButtons();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFooter::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_hButtonFont = pScheme->GetFont( ( m_szButtonFont[0] != '\0' ) ? m_szButtonFont : "GameUIButtons" );
	m_hTextFont = pScheme->GetFont( ( m_szTextFont[0] != '\0' ) ? m_szTextFont : "MenuLarge" );

	SetFgColor( pScheme->GetColor( m_szFGColor, Color( 255, 255, 255, 255 ) ) );
	SetBgColor( pScheme->GetColor( m_szBGColor, Color( 0, 0, 0, 255 ) ) );

	int x, y, w, h;
	GetParent()->GetBounds( x, y, w, h );
	SetBounds( x, h - m_FooterTall, w, m_FooterTall );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFooter::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	// gap between hints
	m_nButtonGap = inResourceData->GetInt( "buttongap", 32 );
	m_ButtonPinRight = inResourceData->GetInt( "button_pin_right", 100 );
	m_FooterTall = inResourceData->GetInt( "tall", 80 );
	m_ButtonOffsetFromTop = inResourceData->GetInt( "buttonoffsety", 0 );
	m_ButtonSeparator = inResourceData->GetInt( "button_separator", 4 );
	m_TextAdjust = inResourceData->GetInt( "textadjust", 0 );

	m_bCenterHorizontal = ( inResourceData->GetInt( "center", 1 ) == 1 );
	m_bPaintBackground = ( inResourceData->GetInt( "paintbackground", 0 ) == 1 );

	// fonts for text and button
	Q_strncpy( m_szTextFont, inResourceData->GetString( "fonttext", "MenuLarge" ), sizeof( m_szTextFont ) );
	Q_strncpy( m_szButtonFont, inResourceData->GetString( "fontbutton", "GameUIButtons" ), sizeof( m_szButtonFont ) );

	// fg and bg colors
	Q_strncpy( m_szFGColor, inResourceData->GetString( "fgcolor", "White" ), sizeof( m_szFGColor ) );
	Q_strncpy( m_szBGColor, inResourceData->GetString( "bgcolor", "Black" ), sizeof( m_szBGColor ) );

	// clear the buttons because we're going to re-add them here
	ClearButtons();

	for ( KeyValues *pButton = inResourceData->GetFirstSubKey(); pButton != NULL; pButton = pButton->GetNextKey() )
	{
		const char *pNameButton = pButton->GetName();

		if ( !Q_stricmp( pNameButton, "button" ) )
		{
			// Add a button to the footer
			const char *pName = pButton->GetString( "name", "NULL" );
			const char *pText = pButton->GetString( "text", "NULL" );
			const char *pIcon = pButton->GetString( "icon", "NULL" );
			AddNewButtonLabel( pName, pText, pIcon );
		}
	}

	InvalidateLayout( false, true ); // force ApplySchemeSettings to run
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFooter::AddNewButtonLabel( const char *name, const char *text, const char *icon )
{
	FooterButton_t *button = new FooterButton_t;

	button->bVisible = true;
	Q_strncpy( button->name, name, sizeof( button->name ) );

	// Button icons are a single character
	wchar_t *pIcon = g_pVGuiLocalize->Find( icon );
	if ( pIcon )
	{
		button->icon[0] = pIcon[0];
		button->icon[1] = '\0';
	}
	else
	{
		button->icon[0] = '\0';
	}

	// Set the help text
	wchar_t *pText = g_pVGuiLocalize->Find( text );
	if ( pText )
	{
		wcsncpy( button->text, pText, wcslen( pText ) + 1 );
	}
	else
	{
		button->text[0] = '\0';
	}

	m_Buttons.AddToTail( button );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFooter::ShowButtonLabel( const char *name, bool show )
{
	for ( int i = 0; i < m_Buttons.Count(); ++i )
	{
		if ( !Q_stricmp( m_Buttons[ i ]->name, name ) )
		{
			m_Buttons[ i ]->bVisible = show;
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFooter::PaintBackground( void )
{
	if ( !m_bPaintBackground )
		return;

	BaseClass::PaintBackground();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFooter::Paint( void )
{
	// inset from right edge
	int wide = GetWide();

	// center the text within the button
	int buttonHeight = vgui::surface()->GetFontTall( m_hButtonFont );
	int fontHeight = vgui::surface()->GetFontTall( m_hTextFont );
	int textY = ( buttonHeight - fontHeight )/2 + m_TextAdjust;

	if ( textY < 0 )
	{
		textY = 0;
	}

	int y = m_ButtonOffsetFromTop;

	if ( !m_bCenterHorizontal )
	{
		// draw the buttons, right to left
		int x = wide - m_ButtonPinRight;

		vgui::Label label( this, "temp", L"" );
		for ( int i = m_Buttons.Count() - 1 ; i >= 0 ; --i )
		{
			FooterButton_t *pButton = m_Buttons[i];
			if ( !pButton->bVisible )
				continue;

			// Get the string length
			label.SetFont( m_hTextFont );
			label.SetText( pButton->text );
			label.SizeToContents();

			int iTextWidth = label.GetWide();

			if ( iTextWidth == 0 )
				x += m_nButtonGap;	// There's no text, so remove the gap between buttons
			else
				x -= iTextWidth;

			// Draw the string
			vgui::surface()->DrawSetTextFont( m_hTextFont );
			vgui::surface()->DrawSetTextColor( GetFgColor() );
			vgui::surface()->DrawSetTextPos( x, y + textY );
			vgui::surface()->DrawPrintText( pButton->text, wcslen( pButton->text ) );

			// Draw the button
			// back up button width and a little extra to leave a gap between button and text
			x -= ( vgui::surface()->GetCharacterWidth( m_hButtonFont, pButton->icon[0] ) + m_ButtonSeparator );
			vgui::surface()->DrawSetTextFont( m_hButtonFont );
			vgui::surface()->DrawSetTextColor( 255, 255, 255, 255 );
			vgui::surface()->DrawSetTextPos( x, y );
			vgui::surface()->DrawPrintText( pButton->icon, 1 );

			// back up to next string
			x -= m_nButtonGap;
		}
	}
	else
	{
		// center the buttons (as a group)
		int x = wide / 2;
		int totalWidth = 0;
		int i = 0;
		int nButtonCount = 0;

		vgui::Label label( this, "temp", L"" );

		// need to loop through and figure out how wide our buttons and text are (with gaps between) so we can offset from the center
		for ( i = 0; i < m_Buttons.Count(); ++i )
		{
			FooterButton_t *pButton = m_Buttons[i];

			if ( !pButton->bVisible )
				continue;

			// Get the string length
			label.SetFont( m_hTextFont );
			label.SetText( pButton->text );
			label.SizeToContents();

			totalWidth += vgui::surface()->GetCharacterWidth( m_hButtonFont, pButton->icon[0] );
			totalWidth += m_ButtonSeparator;
			totalWidth += label.GetWide();

			nButtonCount++; // keep track of how many active buttons we'll be drawing
		}

		totalWidth += ( nButtonCount - 1 ) * m_nButtonGap; // add in the gaps between the buttons
		x -= ( totalWidth / 2 );

		for ( i = 0; i < m_Buttons.Count(); ++i )
		{
			FooterButton_t *pButton = m_Buttons[i];

			if ( !pButton->bVisible )
				continue;

			// Get the string length
			label.SetFont( m_hTextFont );
			label.SetText( pButton->text );
			label.SizeToContents();

			int iTextWidth = label.GetWide();

			// Draw the icon
			vgui::surface()->DrawSetTextFont( m_hButtonFont );
			vgui::surface()->DrawSetTextColor( 255, 255, 255, 255 );
			vgui::surface()->DrawSetTextPos( x, y );
			vgui::surface()->DrawPrintText( pButton->icon, 1 );
			x += vgui::surface()->GetCharacterWidth( m_hButtonFont, pButton->icon[0] ) + m_ButtonSeparator;

			// Draw the string
			vgui::surface()->DrawSetTextFont( m_hTextFont );
			vgui::surface()->DrawSetTextColor( GetFgColor() );
			vgui::surface()->DrawSetTextPos( x, y + textY );
			vgui::surface()->DrawPrintText( pButton->text, wcslen( pButton->text ) );

			x += iTextWidth + m_nButtonGap;
		}
	}
}	

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFooter::ClearButtons( void )
{
	m_Buttons.PurgeAndDeleteElements();
}

DECLARE_BUILD_FACTORY( CDraggableScrollingPanel );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CDraggableScrollingPanel::CDraggableScrollingPanel( Panel *pParent, const char *pszPanelname )
	: EditablePanel( pParent, pszPanelname )
	, m_iDragStartX( 0 )
	, m_iDragStartY( 0 )
	, m_iDragTotalDistance( 0 )
	, m_bDragging( false )
	, m_flZoom( 1.f )
{}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDraggableScrollingPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	FOR_EACH_VEC_BACK( m_vecChildOriginalData, i )
	{
		bool bFound = false;
		for( int j=0; j < GetChildCount(); ++j )
		{
			if ( m_vecChildOriginalData[ i ].m_pChild == GetChild( j ) )
			{
				bFound = true;
				break;
			}
		}

		if ( !bFound )
		{
			m_vecChildOriginalData.Remove( i );
		}
	}

	m_flMinZoom				= inResourceData->GetFloat( "min_zoom", 1.f );
	m_flMaxZoom				= inResourceData->GetFloat( "max_zoom", 2.f );
	m_flZoom				= inResourceData->GetFloat( "zoom", 1.f );
	m_flMouseWheelZoomRate	= inResourceData->GetFloat( "mouse_wheel_zoom_rate", 0.05f );

	m_iOriginalWide = GetWide();
	m_iOriginalTall = GetTall();

	SetZoomAmount( m_flZoom, GetWide() / 2, GetTall() /  2 );

	KeyValues* pKVPendingChildren = inResourceData->FindKey( "pending_children" );
	if ( pKVPendingChildren )
	{
		FOR_EACH_TRUE_SUBKEY( pKVPendingChildren, pKVChild )
		{
			auto& child = m_vecPendingChildren[ m_vecPendingChildren.AddToTail() ];
			child.m_strName			= pKVChild->GetString( "child_name" );
			child.m_ePinPosition	= (EPinPosition)pKVChild->GetInt( "pin", PIN_CENTER );
			child.m_bScaleWithZoom	= pKVChild->GetBool( "scale", true );
			child.m_bMoveWithDrag	= pKVChild->GetBool( "move", true );
		}

		if ( !m_vecPendingChildren.IsEmpty() )
		{
			vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: When a child is removed, remove the original data for that child
//-----------------------------------------------------------------------------
void CDraggableScrollingPanel::OnChildRemoved( Panel* pChild )
{
	BaseClass::OnChildRemoved( pChild );

	auto idx = m_vecChildOriginalData.FindPredicate( [ & ]( const ChildPositionInfo_t& other )
	{
		return other.m_pChild == pChild;
	} );

	m_vecChildOriginalData.Remove( idx );
}

//-----------------------------------------------------------------------------
// Purpose: Check if our pending children are loaded yer
//-----------------------------------------------------------------------------
void CDraggableScrollingPanel::OnTick()
{
	BaseClass::OnTick();

	if ( BCheckForPendingChildren() )
	{
		vgui::ivgui()->RemoveTickSignal( GetVPanel() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Try to setup our pending children
//-----------------------------------------------------------------------------
bool CDraggableScrollingPanel::BCheckForPendingChildren()
{
	FOR_EACH_VEC_BACK( m_vecPendingChildren, i )
	{
		auto& child = m_vecPendingChildren[ i ];
		Panel* pChildPanel = FindChildByName( child.m_strName );
		if ( pChildPanel )
		{
			AddOrUpdateChild( pChildPanel, child.m_bScaleWithZoom, child.m_bMoveWithDrag, child.m_ePinPosition );
			m_vecPendingChildren.Remove( i );
		}
	}

	return m_vecPendingChildren.IsEmpty();
}

//-----------------------------------------------------------------------------
// Purpose: Remember where we started pressing
//-----------------------------------------------------------------------------
void CDraggableScrollingPanel::OnMousePressed( MouseCode code )
{
	input()->SetMouseCapture(GetVPanel());

	m_bDragging = true;
	m_iDragTotalDistance = 0;
	input()->GetCursorPosition( m_iDragStartX, m_iDragStartY );

	PostActionSignal( new KeyValues("DragStart") );
}

//-----------------------------------------------------------------------------
// Purpose: Done dragging
//-----------------------------------------------------------------------------
void CDraggableScrollingPanel::OnMouseReleased( MouseCode code )
{
	input()->SetMouseCapture( NULL );
	m_bDragging = false;
	PostActionSignal( new KeyValues( "DragStop", "dist", m_iDragTotalDistance ) );
}

//-----------------------------------------------------------------------------
// Purpose: Move the panel corresponding to mouse deltas
//-----------------------------------------------------------------------------
void CDraggableScrollingPanel::InternalCursorMoved( int x, int y )
{
	if ( !m_bDragging )
		return;

	// How far to go
	int nDeltaX = x - m_iDragStartX;
	int nDeltaY = y - m_iDragStartY;

	m_iDragTotalDistance += abs( nDeltaX );
	m_iDragTotalDistance += abs( nDeltaY );

	// Store where the mouse is now, so we can get the next delta
	m_iDragStartX = x;
	m_iDragStartY = y;

	int nNewX, nNewY;
	GetPos( nNewX, nNewY );

	// Move us, but keep us within parents bounds
	nNewX = clamp( nDeltaX + nNewX, -( GetWide() - GetParent()->GetWide() ), 0 );
	nNewY = clamp( nDeltaY + nNewY, -( GetTall() - GetParent()->GetTall() ), 0 );

	SetPos( nNewX, nNewY );

	// We moved.  Move children
	UpdateChildren();
}

//-----------------------------------------------------------------------------
// Purpose: Zoom based on how much the wheel was wheeled
//-----------------------------------------------------------------------------
void CDraggableScrollingPanel::OnMouseWheeled( int delta )
{
	int nXP, pYP;
	ipanel()->GetAbsPos(GetVParent(), nXP, pYP);

	int nXM, nYM;
	input()->GetCursorPosition( nXM, nYM );

	SetZoomAmount( m_flZoom + delta * m_flMouseWheelZoomRate, nXM - nXP, nYM - pYP );

	BaseClass::OnMouseWheeled(delta);
}

//-----------------------------------------------------------------------------
// Purpose: External slider movement
//-----------------------------------------------------------------------------
void CDraggableScrollingPanel::OnSliderMoved( KeyValues *pParams )
{
	Slider *pSlider = reinterpret_cast<Slider*>( const_cast<KeyValues*>(pParams)->GetPtr("panel") );
	if ( pSlider )
	{
		int nMin, nMax;
		pSlider->GetRange( nMin, nMax );
		SetZoomAmount( RemapVal( (float)pSlider->GetValue(), (float)nMin, (float)nMax, m_flMinZoom, m_flMaxZoom )
					 , GetParent()->GetWide() / 2.f
					 , GetParent()->GetTall() / 2.f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Zoom!  Make our children scale up and move into position.  We use
//			a zoom focus point that we zoom into so if the user uses the mouse
//			wheel to zoom in, the point under the cursor will be maintained
//			as they zoom.
//-----------------------------------------------------------------------------
void CDraggableScrollingPanel::SetZoomAmount( float flZoomAmount, int nXZoomFocus, int nYZoomFocus )
{
	float flNewZoom = clamp( flZoomAmount, m_flMinZoom, m_flMaxZoom );
	m_flZoom = flNewZoom;

	// Tell everyone we zoomed
	KeyValues *pParams = new KeyValues( "ZoomChanged" );
	pParams->SetFloat( "zoom", RemapVal( m_flZoom, m_flMinZoom, m_flMaxZoom, 0.f, 1.f ) );
	PostActionSignal( pParams );

	int nXPos, nYPos;
	GetPos( nXPos, nYPos );

	// Maintain focal point as we zoom in/out
	float flXZoomFocalPoint = float( nXZoomFocus - nXPos ) / GetWide();
	float flYZoomFocalPoint = float( nYZoomFocus - nYPos ) / GetTall();

	// Resize ourselves
	SetWide( m_flZoom * m_iOriginalWide );
	SetTall( m_flZoom * m_iOriginalTall );

	nXPos = ( flXZoomFocalPoint * GetWide() ) - nXZoomFocus;
	nYPos = ( flYZoomFocalPoint * GetTall() ) - nYZoomFocus;

	// Make sure we stay within bounds
	nXPos = clamp( -nXPos, -( GetWide() - GetParent()->GetWide() ), 0 );
	nYPos = clamp( -nYPos, -( GetTall() - GetParent()->GetTall() ), 0 );

	SetPos( nXPos, nYPos );

	// Update children since we scaled and possibly moved
	UpdateChildren();
}

const CDraggableScrollingPanel::ChildPositionInfo_t* CDraggableScrollingPanel::GetChildPositionInfo( const Panel* pChildPanel ) const
{
	auto idx =m_vecChildOriginalData.FindPredicate( [ & ]( const ChildPositionInfo_t& other )
	{
		return other.m_pChild == pChildPanel;
	} );

	if ( idx == m_vecChildOriginalData.InvalidIndex() )
		return NULL;

	return &m_vecChildOriginalData[ idx ];
}

//-----------------------------------------------------------------------------
// Purpose: Store the original size and position of children so we know how to
//			scale them up and down
//-----------------------------------------------------------------------------
void CDraggableScrollingPanel::AddOrUpdateChild( Panel* pChild, bool bScaleWithZoom, bool bMoveWithDrag, EPinPosition ePinPosition )
{
	// Check if we already have this panel and dont do anything if os
	auto idx = m_vecChildOriginalData.InvalidIndex();
	FOR_EACH_VEC( m_vecChildOriginalData, i )
	{
		if ( m_vecChildOriginalData[ i ].m_pChild == pChild )
		{
			idx = i;
			break;
		}
	}

	if ( idx == m_vecChildOriginalData.InvalidIndex() )
	{
		idx = m_vecChildOriginalData.AddToTail();
	}

	// Setup initial data
	ChildPositionInfo_t& info = m_vecChildOriginalData[ idx ];
	info.m_pChild = pChild;
	info.m_bScaleWithZoom = bScaleWithZoom;
	info.m_bMoveWithDrag = bMoveWithDrag;
	info.m_ePinPosition = ePinPosition;

	// Capture their settings now or do we have to wait?
	if ( pChild->IsLayoutInvalid() )
	{
		info.m_bWaitingForSettings = true;
	}
	else
	{
		CaptureChildSettings( pChild );
		UpdateChildren();
	}
}

//-----------------------------------------------------------------------------
// Purpose: See if we were waiting for a child to get their settings applied
//			before we added them as a managed child
//-----------------------------------------------------------------------------
void CDraggableScrollingPanel::OnChildSettingsApplied( KeyValues *pInResourceData, Panel *pChild )
{
	FOR_EACH_VEC( m_vecChildOriginalData, i )
	{
		ChildPositionInfo_t& info = m_vecChildOriginalData[ i ];
		if ( info.m_pChild == pChild && info.m_bWaitingForSettings )
		{
			info.m_bWaitingForSettings = false;
			CaptureChildSettings( pChild );
			return;
		}
	}
}

void CDraggableScrollingPanel::CaptureChildSettings( Panel* pChild )
{
	float flStartWide = GetWide() / m_flZoom;
	float flStartTall = GetTall() / m_flZoom;

	FOR_EACH_VEC( m_vecChildOriginalData, i )
	{
		if ( m_vecChildOriginalData[ i ].m_pChild == pChild )
		{
			ChildPositionInfo_t& info = m_vecChildOriginalData[ i ];
			switch ( info.m_ePinPosition )
			{
				case PIN_CENTER:
				{
					info.m_flX = ( pChild->GetXPos() + ( pChild->GetWide() / 2.f ) ) / flStartWide;
					info.m_flY = ( pChild->GetYPos() + ( pChild->GetTall() / 2.f ) ) / flStartTall;
				}
				break;

				case PIN_TOP_LEFT:
				{
					info.m_flX = pChild->GetXPos() / flStartWide;
					info.m_flY = pChild->GetYPos() / flStartTall;
				}
				break;

				case PIN_TOP_RIGHT:
				{
					info.m_flX = ( pChild->GetXPos() + pChild->GetWide() ) / flStartWide;
					info.m_flY =   pChild->GetYPos() / flStartTall;
				}
				break;

				case PIN_BOTTOM_LEFT:
				{
					info.m_flX =   pChild->GetXPos() / flStartWide;
					info.m_flY = ( pChild->GetYPos() + pChild->GetTall() ) / flStartTall;
				}
				break;

				case PIN_BOTTOM_RIGHT:
				{
					info.m_flX = ( pChild->GetXPos() + pChild->GetWide() ) / flStartWide;
					info.m_flY = ( pChild->GetYPos() + pChild->GetTall() ) / flStartTall;
				}
				break;
			}
			
			info.m_flWide = pChild->GetWide() / flStartWide;
			info.m_flTall = pChild->GetTall() / flStartTall;
			return;
		}
	}

	// Should've been captured above
	Assert( false );
}

//-----------------------------------------------------------------------------
// Purpose: Resize and reposition children based on zoom and drag offset
//-----------------------------------------------------------------------------
void CDraggableScrollingPanel::UpdateChildren()
{
	int nXPos, nYPos;
	GetPos( nXPos, nYPos );

	FOR_EACH_VEC( m_vecChildOriginalData, i )
	{
		ChildPositionInfo_t& child = m_vecChildOriginalData[ i ];

		if ( child.m_bWaitingForSettings )
		{
			CaptureChildSettings( child.m_pChild );
			child.m_bWaitingForSettings = false;
		}

		if ( child.m_bScaleWithZoom )
		{
			child.m_pChild->SetWide( child.m_flWide * GetWide() );
			child.m_pChild->SetTall( child.m_flTall * GetTall() );
		}

		if ( child.m_bMoveWithDrag )
		{
			switch ( child.m_ePinPosition )
			{
				case PIN_CENTER:
				{
					child.m_pChild->SetPos( ( child.m_flX * GetWide() ) - ( child.m_pChild->GetWide() / 2.f )
										  , ( child.m_flY * GetTall() ) - ( child.m_pChild->GetTall() / 2.f ) );
				}
				break;

				case PIN_TOP_LEFT:
				{
					child.m_pChild->SetPos( child.m_flX * GetWide()
										  , child.m_flY * GetTall() );
				}
				break;

				case PIN_TOP_RIGHT:
				{
					child.m_pChild->SetPos( ( child.m_flX * GetWide() ) - child.m_pChild->GetWide()
										  , ( child.m_flY * GetTall() ) );
				}
				break;

				case PIN_BOTTOM_LEFT:
				{
					child.m_pChild->SetPos( ( child.m_flX * GetWide() )
										  , ( child.m_flY * GetTall() ) - child.m_pChild->GetTall() );
				}
				break;

				case PIN_BOTTOM_RIGHT:
				{
					child.m_pChild->SetPos( ( child.m_flX * GetWide() ) - ( child.m_pChild->GetWide() )
										  , ( child.m_flY * GetTall() ) - ( child.m_pChild->GetTall() ) );
				}
				break;
			}

			
		}
	}
}

DECLARE_BUILD_FACTORY( CTFLogoPanel );
CTFLogoPanel::CTFLogoPanel( Panel *pParent, const char *pszPanelname )
	: BaseClass( pParent, pszPanelname )
{}

void CTFLogoPanel::PaintTFLogo( float flAngle, const Color& color ) const
{
	const float flTotalRadius = YRES( m_flRadius );
	// I did the math
	const float flOuterToInnerRatio = 0.35f;
	const float flInnerRadius = flTotalRadius * flOuterToInnerRatio;
	constexpr const float flNaturalTiltAngle = 6.7f;
	constexpr const float fl90 = DEG2RAD( 90 );

	// Vgui....
	const float flCenterX = const_cast< CTFLogoPanel* >( this )->GetWide() / 2.f;
	const float flCenterY = const_cast< CTFLogoPanel* >( this )->GetTall() / 2.f;


	auto lambdaDrawSegment = [&]( float flStartAngle, float flEndAngle )
	{
		// Rotate it around the circle
		float flX = flCenterX ;
		float flY = flCenterY ;

		float flMagicOuter = 6.f;
		float flMagicInner = flMagicOuter * ( 1.f / flOuterToInnerRatio );

		DrawFilledColoredCircleSegment( flX,
										flY,
										flTotalRadius,
										flInnerRadius,
										color,
										flStartAngle	+ flMagicOuter,
										flEndAngle		- flMagicOuter,
										flStartAngle	+ flMagicInner,
										flEndAngle		- flMagicInner,
										true );
	};

	

	lambdaDrawSegment( flNaturalTiltAngle + flAngle,
					   flNaturalTiltAngle + flAngle + 90 );

	lambdaDrawSegment( flNaturalTiltAngle + flAngle + 90,
					   flNaturalTiltAngle + flAngle + 180  );

	lambdaDrawSegment( flNaturalTiltAngle + flAngle + 180,
					   flNaturalTiltAngle + flAngle + 270 );

	lambdaDrawSegment( flNaturalTiltAngle + flAngle + 270,
					   flNaturalTiltAngle + flAngle + 360 );

}

void CTFLogoPanel::Paint()
{
	m_flOffsetAngle += gpGlobals->frametime * m_flVelocity;
	m_flOffsetAngle = fmodf( m_flOffsetAngle, 360.f );
	PaintTFLogo( m_flOffsetAngle, GetFgColor() );
	BaseClass::Paint();
}

class CScrollingIndicatorPanel : public EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CScrollingIndicatorPanel , EditablePanel );
	CScrollingIndicatorPanel( const wchar* pwszText,
							  const char* pszSoundName,
							  float flDelay,
							  int nXTravel,
							  int nYTravel,
							  bool bPositive )
		: BaseClass( NULL, "Indicator" )
		, m_strSound( pszSoundName )
		, m_nXTravel( nXTravel )
		, m_nYTravel( nYTravel )
		, m_bPositive( bPositive )
	{
		vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
		SetProportional( true );
		SetScheme(scheme);

		LoadControlSettings( "resource/ui/XPSourcePanel.res" );
		SetMouseInputEnabled( false );

		PostMessage( GetVPanel(), new KeyValues( "Start" ), flDelay );
		PostMessage( GetVPanel(), new KeyValues( "End" ), flDelay + 3.5f );

		memset( m_wszBuff, 0, sizeof( m_wszBuff ) );

		if( pwszText )
		{
			SetText( pwszText );
		}

		SetAutoDelete( false );
	}

	virtual ~CScrollingIndicatorPanel()
	{

	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE
	{
		BaseClass::ApplySchemeSettings( pScheme );

		
	}

	virtual void PerformLayout() OVERRIDE
	{
		BaseClass::PerformLayout();

		SetDialogVariable( "source", m_wszBuff );
	}

	MESSAGE_FUNC( Start, "Start" )
	{
		SetVisible( true );

		// Do starting stuff
		if ( g_pClientMode && g_pClientMode->GetViewport() && g_pClientMode->GetViewportAnimationController() )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, m_bPositive ? "XPSourceShow_Positive" : "XPSourceShow_Negative", false );
			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( this, "xpos", GetXPos() + m_nXTravel, 0.f, 3.f, AnimationController::INTERPOLATOR_DEACCEL, 0, true, false );
			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( this, "ypos", GetYPos() + m_nYTravel, 0.f, 3.f, AnimationController::INTERPOLATOR_DEACCEL, 0, true, false );
		}

		if ( !m_strSound.IsEmpty() )
		{
			PlaySoundEntry( m_strSound );
		}
	}

	MESSAGE_FUNC( End, "End" )
	{
		// We're done!  Delete ourselves
		MarkForDeletion();
	}

	void SetText( const wchar* pwszText )
	{
		_snwprintf( m_wszBuff, ARRAYSIZE( m_wszBuff ), L"%ls", pwszText );
		InvalidateLayout();
	}

private:

	wchar m_wszBuff[ 256 ];
	CUtlString m_strSound;
	bool m_bPositive;
	int m_nXTravel = 0;
	int m_nYTravel = 0;
};

//-----------------------------------------------------------------------------
// Purpose: A label that can have multiple fonts specified and will try to use
//			them in order specified, using the first one that fits.
//-----------------------------------------------------------------------------
class CAutoFittingLabel : public Label
{
	DECLARE_CLASS_SIMPLE( CAutoFittingLabel, Label );
public:

	CAutoFittingLabel( Panel *parent, const char *name )
		: Label( parent, name, (const char*)NULL )
		, m_mapColors( DefLessFunc( int ) )
	{}

	virtual void ApplySettings( KeyValues *inResourceData )
	{
		BaseClass::ApplySettings( inResourceData );
		vgui::IScheme *pScheme = scheme()->GetIScheme( GetScheme() );

		m_vecFonts.Purge();
		KeyValues *pFonts = inResourceData->FindKey( "fonts" );
		if ( pFonts )
		{
			//
			// Get all the fonts
			//
			
			// Old style
			FOR_EACH_TRUE_SUBKEY( pFonts, pFont )
			{
				const HFont& font = pScheme->GetFont( pFont->GetString( "font" ), true );
				m_vecFonts.AddToTail( font );			
			}

			// New style
			FOR_EACH_VALUE( pFonts, pValue )
			{
				const HFont& font = pScheme->GetFont( pValue->GetString(), true );
				m_vecFonts.AddToTail( font );			
			}
		}
		else
		{
			m_vecFonts.AddToTail( GetFont() );
		}

		m_mapColors.Purge();
		KeyValues* pKVColors = inResourceData->FindKey( "Colors" );
		if ( pKVColors )
		{
			FOR_EACH_VALUE( pKVColors, pKVColor )
			{
				m_mapColors.Insert( atoi( pKVColor->GetName() ) ,GetColor( pKVColor->GetString() ) );
			}
		}
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();

		SetFont( m_vecFonts.Head() );

		// Go through all the fonts and try to find one that fits
		int nIndex = 0;
		GetTextImage()->ResizeImageToContentMaxWidth( GetWide() );
		while ( ( GetTextImage()->IsWrapping() || GetTextImage()->GetEllipsesPosition() ) && nIndex < m_vecFonts.Count() )
		{
			SetFont( m_vecFonts[ nIndex ] );
			GetTextImage()->ResizeImageToContentMaxWidth( GetWide() );

			++nIndex;
		}

		// Go through each character in the buffer and look for color change codes.
		// When a code is found, add a color change into the text image, where we 
		// use the color code as an index into the map.
		const wchar_t* pwszText = GetTextImage()->GetUText();
		int nTextIndex = 0;
		GetTextImage()->ClearColorChangeStream();
		while( pwszText && pwszText[0] )
		{
			auto idx = m_mapColors.Find( pwszText[0] );
			if ( idx != m_mapColors.InvalidIndex() )
			{
				GetTextImage()->AddColorChange( m_mapColors[ idx ], nTextIndex );
			}

			++nTextIndex;
			++pwszText;
		}
	}

private:

	CUtlVector< HFont > m_vecFonts;
	CUtlMap< int, Color > m_mapColors;
};

DECLARE_BUILD_FACTORY( CAutoFittingLabel );

class CGenericSwoop : public CControlPointIconSwoop
{
	DECLARE_CLASS_SIMPLE( CGenericSwoop, CControlPointIconSwoop );
	CGenericSwoop( float flSwoopTime, bool bDown )
		: CControlPointIconSwoop( NULL, "swoop", bDown )
	{
		vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
		SetScheme(scheme);
		SetProportional( true );

		SetZPos( 50000 );
		SetRotation( bDown ? ROTATED_UNROTATED : ROTATED_FLIPPED );
		PostMessage( this, new KeyValues( "StartSwoop" ), flSwoopTime );
	}

	virtual ~CGenericSwoop()
	{

	}

	MESSAGE_FUNC( MsgStartSwoop, "StartSwoop" )
	{
		StartSwoop();
		PostMessage( this, new KeyValues( "Delete" ), STARTCAPANIM_SWOOP_LENGTH );
	}
};

void CreateSwoop( int nX, int nY, int nWide, int nTall, float flDelay, bool bDown )
{
	CGenericSwoop* pSwoop = new CGenericSwoop( flDelay, bDown );
	pSwoop->MakeReadyForUse();
	pSwoop->SetBounds( nX, nY, nWide, nTall );
}

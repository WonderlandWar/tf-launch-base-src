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
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ProgressBar.h>
#include "view_scene.h"
#include "view.h"
#include "tf_gamerules.h"
#include "tf_weapon_invis.h"
#include <vgui_controls/AnimationController.h>

#include "c_tf_objective_resource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

extern ISoundEmitterSystemBase *soundemitterbase;

// Floating delta text items, float off the top of the frame to 
// show changes to the metal account value
typedef struct 
{
	enum eAccountDeltaType_t
	{
		ACCOUNT_DELTA_INVALID,
		ACCOUNT_DELTA_HEALING,
		ACCOUNT_DELTA_DAMAGE,
		ACCOUNT_DELTA_BONUS_POINTS,
		ACCOUNT_DELTA_ROBOT_DESTRUCTION_POINT_RED,
		ACCOUNT_DELTA_ROBOT_DESTRUCTION_POINT_BLUE,
	};



	// amount of delta
	int m_iAmount;
	
	bool m_bLargeFont;		// display larger font
	eAccountDeltaType_t m_eDataType;

	// die time
	float m_flDieTime;

	// position
	int m_nX;				// X Pos in screen space & world space
	int m_nXEnd;			// Ending X Pos in screen space and world space
	int m_nHStart;			// Starting Y Pos in screen space, Z pos in world space
	int m_nHEnd;			// Ending Y Pos in screen space, Z pos in world space
	int m_nY;				// Y Coord in world space, not used in screen space
	bool m_bWorldSpace;
	float m_flBatchWindow;
	int m_nSourceID;		// Can be entindex, etc
	Color m_color;
	bool m_bShadows;

	// append a bit of extra text to the end
	wchar_t m_wzText[8];

} account_delta_t;

#define NUM_ACCOUNT_DELTA_ITEMS 10

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CAccountPanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CAccountPanel, EditablePanel );

public:
	CAccountPanel( Panel *parent, const char *name )
		: EditablePanel( parent, name )
	{
		m_nBGTexture = -1;
		m_bNegativeFlipDir = false;
		SetDialogVariable( "metal", 0 );
	}

	virtual void	ApplySchemeSettings( IScheme *scheme ) OVERRIDE;
	virtual void	ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual void	Paint( void ) OVERRIDE;

	virtual account_delta_t *OnAccountValueChanged( int iOldValue, int iNewValue, account_delta_t::eAccountDeltaType_t type );

	virtual const char *GetResFileName( void ) { return "resource/UI/HudAccountPanel.res"; }

protected:
	virtual Color GetColor( const account_delta_t::eAccountDeltaType_t &type, const int iDeltaValue = 0 );

	CUtlVector <account_delta_t> m_AccountDeltaItems;

	int m_nBGTexture;
	bool m_bNegativeFlipDir;

	CPanelAnimationVarAliasType( float, m_flDeltaItemStartPos, "delta_item_start_y", "100", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flDeltaItemEndPos, "delta_item_end_y", "0", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flDeltaItemX, "delta_item_x", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flDeltaItemXEndPos, "delta_item_end_x", "0", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flBGImageX, "bg_image_x", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBGImageY, "bg_image_y", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBGImageWide, "bg_image_wide", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBGImageTall, "bg_image_tall", "0", "proportional_float" );

	CPanelAnimationVar( Color, m_DeltaPositiveColor, "PositiveColor", "0 255 0 255" );
	CPanelAnimationVar( Color, m_DeltaNegativeColor, "NegativeColor", "255 0 0 255" );
	CPanelAnimationVar( Color, m_DeltaEventColor, "EventColor", "255 0 255 255" );
	CPanelAnimationVar( Color, m_DeltaRedRobotScoreColor, "RedRobotScoreColor", "255 0 0 255" );
	CPanelAnimationVar( Color, m_DeltaBlueRobotScoreColor, "BlueRobotScoreColor", "0 166 255 255" );

	CPanelAnimationVar( float, m_flDeltaLifetime, "delta_lifetime", "2.0" );

	CPanelAnimationVar( vgui::HFont, m_hDeltaItemFont, "delta_item_font", "Default" );
	CPanelAnimationVar( vgui::HFont, m_hDeltaItemFontBig, "delta_item_font_big", "Default" );
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudAccountPanel : public CHudElement, public CAccountPanel
{
	DECLARE_CLASS_SIMPLE( CHudAccountPanel, CAccountPanel );

public:
	CHudAccountPanel( const char *pElementName ) 
		: CHudElement( pElementName )
		, CAccountPanel( NULL, pElementName )
	{
		Panel *pParent = g_pClientMode->GetViewport();
		SetParent( pParent );
		SetHiddenBits( HIDEHUD_MISCSTATUS );
		ListenForGameEvent( "player_account_changed" );
	}

	virtual void LevelInit( void ) OVERRIDE
	{
		CHudElement::LevelInit();
	}

	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE
	{
		const char * type = event->GetName();

		if ( Q_strcmp(type, "player_account_changed") == 0 )
		{
			int iOldValue = event->GetInt( "old_account" );
			int iNewValue = event->GetInt( "new_account" );
			account_delta_t::eAccountDeltaType_t deltaType = ( iNewValue - iOldValue >= 0 ) ? account_delta_t::ACCOUNT_DELTA_HEALING
																							: account_delta_t::ACCOUNT_DELTA_DAMAGE;

			OnAccountValueChanged( iOldValue, iNewValue, deltaType );
		}
		else
		{
			CHudElement::FireGameEvent( event );
		}
	}

	virtual bool ShouldDraw( void ) OVERRIDE
	{
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( !pPlayer || !pPlayer->IsAlive() || !pPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			m_AccountDeltaItems.RemoveAll();
			return false;
		}
		
		if ( TFGameRules() && TFGameRules()->ShowMatchSummary() )
			return false;

		return CHudElement::ShouldDraw();
	}
};

DECLARE_HUDELEMENT( CHudAccountPanel );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAccountPanel::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( GetResFileName() );

	BaseClass::ApplySchemeSettings( pScheme );
}

void CAccountPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	// Backwards compatibility.  If we DONT find "delta_item_end_x" specified in the keyvalues,
	// then just take the starting x-pos as the ending x-pos.
	if ( inResourceData->FindKey( "delta_item_end_x" ) == NULL )
	{
		m_flDeltaItemXEndPos = m_flDeltaItemX;
	}

	m_bNegativeFlipDir = inResourceData->FindKey( "negative_flip_dir", false );

	const char *pszBGTextureName = inResourceData->GetString( "bg_texture", NULL );
	if ( m_nBGTexture == -1 && pszBGTextureName && pszBGTextureName[0] )
	{
		m_nBGTexture = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile( m_nBGTexture , pszBGTextureName, true, false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
account_delta_t *CAccountPanel::OnAccountValueChanged( int iOldValue, int iNewValue, account_delta_t::eAccountDeltaType_t type )
{
	// update the account value
	SetDialogVariable( "metal", iNewValue ); 

	int iDelta = iNewValue - iOldValue;

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( iDelta != 0 && pPlayer && pPlayer->IsAlive() )
	{
		int index = m_AccountDeltaItems.AddToTail();
		account_delta_t *pNewDeltaItem = &m_AccountDeltaItems[index];

		pNewDeltaItem->m_flDieTime = gpGlobals->curtime + m_flDeltaLifetime;
		pNewDeltaItem->m_iAmount = iDelta;
		pNewDeltaItem->m_nX = m_flDeltaItemX;
		pNewDeltaItem->m_nXEnd = m_flDeltaItemXEndPos;
		pNewDeltaItem->m_nHStart = m_flDeltaItemStartPos;
		pNewDeltaItem->m_nHEnd = m_flDeltaItemEndPos;
		pNewDeltaItem->m_bWorldSpace = false;
		pNewDeltaItem->m_nSourceID = -1;
		pNewDeltaItem->m_flBatchWindow = 0.f;
		pNewDeltaItem->m_bLargeFont = false;
		pNewDeltaItem->m_eDataType = type;
		pNewDeltaItem->m_wzText[0] = NULL;
		pNewDeltaItem->m_color = GetColor( type, iDelta );
		pNewDeltaItem->m_bShadows = false;
		return &m_AccountDeltaItems[index];
	}

	return NULL;
}

Color CAccountPanel::GetColor( const account_delta_t::eAccountDeltaType_t &type, const int iDeltaValue )
{
	if ( type == account_delta_t::ACCOUNT_DELTA_BONUS_POINTS )
	{
		return m_DeltaEventColor;
	}
	else if ( type == account_delta_t::ACCOUNT_DELTA_HEALING )
	{
		return ( iDeltaValue < 0 ) ? m_DeltaNegativeColor : m_DeltaPositiveColor;
	}
	else if ( type == account_delta_t::ACCOUNT_DELTA_DAMAGE )
	{
		return Color( 255, 0, 0 );
	}
	else if ( type == account_delta_t::ACCOUNT_DELTA_ROBOT_DESTRUCTION_POINT_BLUE )
	{
		return m_DeltaBlueRobotScoreColor;
	}
	else if ( type == account_delta_t::ACCOUNT_DELTA_ROBOT_DESTRUCTION_POINT_RED )
	{
		return m_DeltaRedRobotScoreColor;
	}
	
	return Color( 255, 255, 255, 255 );
}

//-----------------------------------------------------------------------------
// Purpose: Paint the deltas
//-----------------------------------------------------------------------------
void CAccountPanel::Paint( void )
{
	BaseClass::Paint();

	FOR_EACH_VEC_BACK( m_AccountDeltaItems, i )
	{
		// Reduce lifetime when count grows too high
		float flTimeMod = m_AccountDeltaItems.Count() > NUM_ACCOUNT_DELTA_ITEMS ? RemapValClamped( m_AccountDeltaItems.Count(), 10.f, 15.f, 0.5f, 1.5f ) : 0.f;

		// update all the valid delta items
		if ( ( m_AccountDeltaItems[i].m_flDieTime - flTimeMod ) > gpGlobals->curtime )
		{
			// position and alpha are determined from the lifetime
			Color c = m_AccountDeltaItems[i].m_color;

			float flLifetimePercent = ( m_flDeltaLifetime - ( m_AccountDeltaItems[i].m_flDieTime - gpGlobals->curtime ) ) / m_flDeltaLifetime;
			// fade out after half our lifetime
			int nAlpha = flLifetimePercent > 0.5 ? (int)( 255.0f * ( ( 0.5f - flLifetimePercent ) / 0.5f ) ) : 255;
			c[3] = nAlpha;
			


			// Some items want to be batched together as they're super frequent (i.e. damage events from a flamethrower, or minigun)
			if ( m_AccountDeltaItems[i].m_flBatchWindow > 0.f && m_AccountDeltaItems[i].m_nSourceID != -1 && m_AccountDeltaItems.IsValidIndex( i - 1 ) )
			{
				// If next item is from the same source and too close, merge
				float flDelay = m_AccountDeltaItems[i].m_flBatchWindow;
				if ( m_AccountDeltaItems[i].m_flDieTime - m_AccountDeltaItems[i-1].m_flDieTime <= flDelay &&
					 m_AccountDeltaItems[i-1].m_nSourceID == m_AccountDeltaItems[i].m_nSourceID )
				{
					m_AccountDeltaItems[i].m_iAmount += m_AccountDeltaItems[i-1].m_iAmount;
					m_AccountDeltaItems.Remove( i - 1 );
					continue;
				}
			}

			float flHeight = m_AccountDeltaItems[i].m_nHEnd - m_AccountDeltaItems[i].m_nHStart;
			float flWidth = m_AccountDeltaItems[i].m_nXEnd - m_AccountDeltaItems[i].m_nX;

			// We can be told to go the opposite direction if we're negative
			if ( m_bNegativeFlipDir && m_AccountDeltaItems[i].m_iAmount < 0 )
			{
				flHeight = -flHeight;
				flWidth = -flWidth;
			}

			float flYPos = m_AccountDeltaItems[i].m_nHStart + ( flLifetimePercent * flHeight );
			float flXPos = m_AccountDeltaItems[i].m_nX + ( flLifetimePercent * flWidth );
			if ( m_AccountDeltaItems[i].m_bWorldSpace )
			{
				Vector vecWorld( m_AccountDeltaItems[i].m_nX, m_AccountDeltaItems[i].m_nY, flYPos );
				int iX,iY;
				if ( !GetVectorInHudSpace( vecWorld, iX, iY ) )				// Tested - NOT GetVectorInScreenSpace
					continue;

				flXPos = iX;
				flYPos = iY;
			}

			// If we have a background texture, then draw it!
			if ( m_nBGTexture != -1 )
			{
				vgui::surface()->DrawSetColor(255,255,255,nAlpha);
				vgui::surface()->DrawSetTexture(m_nBGTexture);
				vgui::surface()->DrawTexturedRect( flXPos + m_flBGImageX, flYPos + m_flBGImageY, flXPos + m_flBGImageX + m_flBGImageWide, flYPos + m_flBGImageY + m_flBGImageTall );
			}

			wchar_t wBuf[20];

			if ( m_AccountDeltaItems[i].m_iAmount > 0 )
			{
				V_swprintf_safe( wBuf, L"+%d", m_AccountDeltaItems[i].m_iAmount );
			}
			else
			{
				V_swprintf_safe( wBuf, L"%d", m_AccountDeltaItems[i].m_iAmount );
			}

			// Append?
			if ( m_AccountDeltaItems[i].m_wzText[0] )
			{
				wchar_t wAppend[8] = { 0 };
				V_swprintf_safe( wAppend, L"%ls", m_AccountDeltaItems[i].m_wzText );
				V_wcscat_safe( wBuf, wAppend );
			}


			if ( m_AccountDeltaItems[i].m_bLargeFont )
			{
				vgui::surface()->DrawSetTextFont( m_hDeltaItemFontBig );
			}
			else
			{
				vgui::surface()->DrawSetTextFont( m_hDeltaItemFont );
			}

			// If we're supposed to have shadows, then draw the text as black and offset a bit first.
			// Things get ugly as we approach 0 alpha, so stop drawing the shadow a bit early.
			if ( m_AccountDeltaItems[i].m_bShadows && c[3] > 10 )
			{
				vgui::surface()->DrawSetTextPos( (int)flXPos + XRES(1), (int)flYPos + YRES(1) );
				vgui::surface()->DrawSetTextColor( COLOR_BLACK );
				vgui::surface()->DrawPrintText( wBuf, wcslen(wBuf), FONT_DRAW_NONADDITIVE );
			}

			vgui::surface()->DrawSetTextPos( (int)flXPos, (int)flYPos );
			vgui::surface()->DrawSetTextColor( c );
			vgui::surface()->DrawPrintText( wBuf, wcslen(wBuf), FONT_DRAW_NONADDITIVE );
		}
		else
		{
			m_AccountDeltaItems.Remove( i );
		}
	}
}


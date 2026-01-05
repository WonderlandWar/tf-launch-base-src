//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_hud_mediccallers.h"
#include "iclientmode.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include "view.h"
#include "ivieweffects.h"
#include "viewrender.h"
#include "prediction.h"
#include "GameEventListener.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MEDICCALLER_WIDE		(XRES(56))
#define MEDICCALLER_TALL		(YRES(48))
#define MEDICCALLER_ARROW_WIDE	(XRES(16))
#define MEDICCALLER_ARROW_TALL	(YRES(24))

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFMedicCallerPanel::CTFMedicCallerPanel( Panel *parent, const char *name ) : EditablePanel(parent,name)
{
	m_pArrowMaterial = NULL;
	m_iDrawArrow = DRAW_ARROW_UP;
	m_bOnscreen = false;
	m_flPanelScale = 1.0f;
	m_bBurning = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFMedicCallerPanel::~CTFMedicCallerPanel( void )
{
	if ( m_pArrowMaterial )
	{
		m_pArrowMaterial->DecrementReferenceCount();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CTFMedicCallerPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( GetControlSettingFile() );

	if ( m_pArrowMaterial )
	{
		m_pArrowMaterial->DecrementReferenceCount();
	}
	m_pArrowMaterial = materials->FindMaterial( "HUD/medic_arrow", TEXTURE_GROUP_VGUI );
	m_pArrowMaterial->IncrementReferenceCount();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMedicCallerPanel::PerformLayout( void )
{
	BaseClass::PerformLayout();

	int nWide = XRES(100), nTall = YRES(100);

	// Adjust scale of the panel based on distance to the caller
	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalTFPlayer && m_hEntity )
	{
		Vector vecDistance = m_hEntity->GetAbsOrigin() - pLocalTFPlayer->GetAbsOrigin();
		m_flPanelScale = RemapValClamped( vecDistance.LengthSqr(), 0.0f, (2000.0f * 2000.0f), 1.0f, 0.5f );
	}

	vgui::Panel *pPanel = FindChildByName( "CallerBG" );
	if ( pPanel )
	{
		pPanel->SetVisible( true );
		
		pPanel->GetSize( nWide, nTall );
		pPanel->SetSize ( nWide * m_flPanelScale, nTall * m_flPanelScale );
		pPanel->SetPos( (GetWide() - pPanel->GetWide()) * 0.5, (GetTall() - pPanel->GetTall()) * 0.5 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMedicCallerPanel::GetCallerPosition( const Vector &vecDelta, float flRadius, float *xpos, float *ypos, float *flRotation )
{
	// Player Data
	Vector playerPosition = MainViewOrigin();
	QAngle playerAngles = MainViewAngles();

	Vector forward, right, up(0,0,1);
	AngleVectors (playerAngles, &forward, NULL, NULL );
	forward.z = 0;
	VectorNormalize(forward);
	CrossProduct( up, forward, right );
	float front = DotProduct(vecDelta, forward);
	float side = DotProduct(vecDelta, right);
	*xpos = flRadius * -side;
	*ypos = flRadius * -front;

	// Get the rotation (yaw)
	*flRotation = atan2(*xpos,*ypos) + M_PI;
	*flRotation *= 180 / M_PI;

	float yawRadians = -(*flRotation) * M_PI / 180.0f;
	float ca = cos( yawRadians );
	float sa = sin( yawRadians );

	// Rotate it around the circle
	*xpos = (int)((ScreenWidth() / 2) + (flRadius * sa));
	*ypos = (int)((ScreenHeight() / 2) - (flRadius * ca));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMedicCallerPanel::OnTick( void )
{
	if ( !m_hEntity || ( m_hEntity->IsPlayer() && !m_hEntity->IsAlive() ) || gpGlobals->curtime > m_flRemoveAt )
	{
		MarkForDeletion();
		return;
	}

	// If the local player has started healing this guy, remove it too.
	// Also don't draw it if we're dead.
	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalTFPlayer )
	{
		CBaseEntity *pHealTarget = pLocalTFPlayer->MedicGetHealTarget();
		if ( ( pHealTarget && pHealTarget == m_hEntity ) || ( m_hEntity->IsPlayer() && !pLocalTFPlayer->IsAlive() ) )
		{
			MarkForDeletion();
			return;
		}

		if ( m_hEntity->IsPlayer() )
		{
			C_TFPlayer *pTFPlayer = ToTFPlayer( m_hEntity );
			if ( pTFPlayer )
			{
				// If we're pointing to an enemy spy and they are no longer disguised, remove ourselves
				if ( pTFPlayer->IsPlayerClass( TF_CLASS_SPY ) && 
					 pTFPlayer->GetTeamNumber() != pLocalTFPlayer->GetTeamNumber() &&
					!( pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && pTFPlayer->m_Shared.GetDisguiseTeam() == pLocalTFPlayer->GetTeamNumber() ) )
				{
					MarkForDeletion();
					return;
				}

				m_bBurning = pTFPlayer->m_Shared.InCond( TF_COND_BURNING );
				vgui::Panel *pBurningPanel = FindChildByName( "CallerBurning" );
				if ( pBurningPanel && pBurningPanel->IsVisible() != m_bBurning )
				{
					pBurningPanel->SetVisible( m_bBurning );
				}
			}
		}
	}

	// Tints caller panel based on health remaining.
	vgui::Panel *pPanelHealth = FindChildByName( "CallerHealth" );
	if ( pPanelHealth )
	{
		float flHealth = ( float(m_hEntity->GetHealth()) / float(m_hEntity->GetMaxHealth()) );
		int iCallerHurtAlpha = 255 * ( 1 - flHealth ) + 75;
		pPanelHealth->SetAlpha( clamp( iCallerHurtAlpha, 0, 255 ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMedicCallerPanel::PaintBackground( void )
{
	// If the local player has started healing this guy, remove it too.
	//Also don't draw it if we're dead.
	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalTFPlayer )
		return;

	if ( !m_hEntity || m_hEntity->IsDormant() )
	{
		SetAlpha(0);
		return;
	}

	// Reposition the callout based on our target's position
	int iX, iY;
	Vector vecTarget = (m_hEntity->GetAbsOrigin() + m_vecOffset);
	Vector vecDelta = vecTarget - MainViewOrigin();

	bool bOnscreen = GetVectorInHudSpace( vecTarget, iX, iY );				// Tested and correct - should NOT be GetVectorInScreenSpace.

	int halfWidth = GetWide() / 2;
	if( !bOnscreen || iX < halfWidth || iX > ScreenWidth()-halfWidth )
	{
		// It's off the screen. Position the callout.
		VectorNormalize(vecDelta);
		float xpos, ypos;
		float flRotation;
		float flRadius = YRES(100);
		GetCallerPosition( vecDelta, flRadius, &xpos, &ypos, &flRotation );

		iX = xpos;
		iY = ypos;

		Vector vCenter = m_hEntity->WorldSpaceCenter( );
		if( MainViewRight().Dot( vCenter - MainViewOrigin() ) > 0 )
		{
			m_iDrawArrow = DRAW_ARROW_RIGHT;
		}
		else
		{
			m_iDrawArrow = DRAW_ARROW_LEFT;
		}

		// Move the icon there
		SetPos( iX - halfWidth, iY - (GetTall() / 2) );
		SetAlpha( 255 );
	}
	else
	{
		// On screen
		// If our target isn't visible, we draw transparently
		trace_t	tr;
		UTIL_TraceLine( vecTarget, MainViewOrigin(), MASK_OPAQUE, NULL, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction >= 1.0f )
		{
			m_bOnscreen = true;
			SetAlpha( 0 );
			return;
		}

		m_iDrawArrow = DRAW_ARROW_UP;
		SetAlpha( 92 );
		SetPos( iX - halfWidth, iY - (GetTall() / 2) );
	}

	m_bOnscreen = false;
	BaseClass::PaintBackground();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMedicCallerPanel::Paint( void )
{
	// Don't draw if our target is visible. The particle effect will be doing it for us.
	if ( m_bOnscreen )
		return;

	BaseClass::Paint();

	if ( m_iDrawArrow == DRAW_ARROW_UP )
		return;

	float uA,uB,yA,yB;
	int x,y;
	GetPos( x,y );
	if ( m_iDrawArrow == DRAW_ARROW_LEFT )
	{
		uA = 1.0;
		uB = 0.0;
		yA = 0.0;
		yB = 1.0;
	}
	else
	{
		uA = 0.0;
		uB = 1.0;
		yA = 0.0;
		yB = 1.0;
		x += GetWide() - MEDICCALLER_ARROW_WIDE;
	}

	int iyindent = (GetTall() - MEDICCALLER_ARROW_TALL) * 0.5;
	y += iyindent;

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( m_pArrowMaterial );
	IMesh* pMesh = pRenderContext->GetDynamicMesh( true );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Position3f( x, y, 0.0f );
	meshBuilder.TexCoord2f( 0, uA, yA );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( x + MEDICCALLER_ARROW_WIDE, y, 0.0f );
	meshBuilder.TexCoord2f( 0, uB, yA );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( x + MEDICCALLER_ARROW_WIDE, y + MEDICCALLER_ARROW_TALL, 0.0f );
	meshBuilder.TexCoord2f( 0, uB, yB );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( x, y + MEDICCALLER_ARROW_TALL, 0.0f );
	meshBuilder.TexCoord2f( 0, uA, yB );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMedicCallerPanel::SetEntity( C_BaseEntity *pEntity, float flDuration, Vector &vecOffset )
{
	m_hEntity = pEntity;
	m_flRemoveAt = gpGlobals->curtime + flDuration;
	m_vecOffset = vecOffset;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMedicCallerPanel::AddMedicCaller( C_BaseEntity *pEntity, float flDuration, Vector &vecOffset )
{
	CTFMedicCallerPanel *pCaller = new CTFMedicCallerPanel( g_pClientMode->GetViewport(), "MedicCallerPanel" );
	vgui::SETUP_PANEL(pCaller);
	pCaller->SetBounds( 0,0, MEDICCALLER_WIDE, MEDICCALLER_TALL );
	pCaller->SetEntity( pEntity, flDuration, vecOffset );
	pCaller->SetVisible( true );
	vgui::ivgui()->AddTickSignal( pCaller->GetVPanel() );
	pCaller->OnTick();
}

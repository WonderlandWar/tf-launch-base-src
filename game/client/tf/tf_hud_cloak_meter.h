//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A recreation of HudCloakMeter based on HudDemomanChargeMeter
//
// $NoKeywords: $
//=============================================================================

#ifndef C_TF_ITEMEFFECTMETER_H
#define C_TF_ITEMEFFECTMETER_H

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
#include "tf_weaponbase.h"
#include "tf_gamerules.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudCloakMeter : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudCloakMeter, EditablePanel );

public:
	CHudCloakMeter( const char *pElementName );

	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );
	virtual void	OnTick( void );

private:
	vgui::ContinuousProgressBar *m_pCloakMeter;
};

#endif

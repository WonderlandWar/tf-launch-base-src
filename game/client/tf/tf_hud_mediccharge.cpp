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
#include "tf_weapon_medigun.h"
#include <vgui_controls/AnimationController.h>
#include "tf_imagepanel.h"
#include "vgui_controls/Label.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudMedicChargeMeter : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudMedicChargeMeter, EditablePanel );

public:
	CHudMedicChargeMeter( const char *pElementName );

	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );
	virtual void	OnTick( void );

private:

	void UpdateKnownChargeType( bool bForce );
	void UpdateControlVisibility();

	vgui::ContinuousProgressBar*	m_pChargeMeter;
	Label*							m_pUberchargeLabel;

	bool m_bCharged;
	float m_flLastChargeValue;

	int m_nLastActiveResist;
};

DECLARE_HUDELEMENT( CHudMedicChargeMeter );

struct ResistIcons_t
{
	const char* m_pzsRed;
	const char* m_pzsBlue;
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudMedicChargeMeter::CHudMedicChargeMeter( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudMedicCharge" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pChargeMeter = new ContinuousProgressBar( this, "ChargeMeter" );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	m_bCharged = false;
	m_flLastChargeValue = -1;

	//SetDialogVariable( "charge", 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMedicChargeMeter::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudMedicCharge.res" );

	m_pUberchargeLabel = dynamic_cast<Label*>( FindChildByName("ChargeLabel") );
	Assert( m_pUberchargeLabel );
	
	// Figure out which controls to show
	UpdateKnownChargeType( true );

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudMedicChargeMeter::ShouldDraw( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer || !pPlayer->IsPlayerClass( TF_CLASS_MEDIC ) || !pPlayer->IsAlive() )
	{
		return false;
	}

	CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();

	if ( !pWpn )
	{
		return false;
	}

	if ( pWpn->GetWeaponID() != TF_WEAPON_MEDIGUN )
	{
		return false;
	}

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMedicChargeMeter::OnTick( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
		return;

	CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();

	// Might have switched medigun types
	UpdateKnownChargeType( false);

	if ( !pWpn || ( pWpn->GetWeaponID() != TF_WEAPON_MEDIGUN ) )
		return;

	CWeaponMedigun *pMedigun = assert_cast< CWeaponMedigun *>( pWpn );

	float flCharge = pMedigun->GetChargeLevel();


	if ( flCharge != m_flLastChargeValue )
	{
		if ( m_pChargeMeter )	// Regular uber
		{
			m_pChargeMeter->SetProgress( flCharge );
			//SetDialogVariable( "charge", (int)( flCharge * 100 ) );
		}

		if ( !m_bCharged )
		{
			if ( flCharge >= 1.0 )
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudMedicCharged" );
				m_bCharged = true;
			}
		}
		else
		{
			// we've got invuln charge or we're using our invuln
			if ( !pMedigun->IsReleasingCharge() )
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudMedicChargedStop" );
				m_bCharged = false;
			}
		}

	}	

	m_flLastChargeValue = flCharge;
}

void CHudMedicChargeMeter::UpdateKnownChargeType( bool bForce )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
		return;

	CTFWeaponBase *pWpn = pPlayer->Weapon_OwnsThisID( TF_WEAPON_MEDIGUN );

	if( !pWpn )
		return;

	if( bForce )
	{
		UpdateControlVisibility();
	}
}

void CHudMedicChargeMeter::UpdateControlVisibility()
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
		return;

	CTFWeaponBase *pWpn = pPlayer->Weapon_OwnsThisID( TF_WEAPON_MEDIGUN );

	if ( !pWpn || ( pWpn->GetWeaponID() != TF_WEAPON_MEDIGUN ) )
		return;

	CWeaponMedigun *pMedigun = static_cast< CWeaponMedigun *>( pWpn );

	if ( !pMedigun )
		return;

	m_pChargeMeter->SetVisible( true );
	if( m_pUberchargeLabel )
	{
		m_pUberchargeLabel->SetVisible( true );
	}
}
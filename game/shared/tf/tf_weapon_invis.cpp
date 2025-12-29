//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_invis.h"
#include "in_buttons.h"

#if !defined( CLIENT_DLL )
	#include "vguiscreen.h"
	#include "tf_player.h"
#else
	#include "c_tf_player.h"
#endif

extern ConVar tf_spy_invis_unstealth_time;
extern ConVar tf_spy_cloak_consume_rate;
extern ConVar tf_spy_cloak_regen_rate;

//=============================================================================
//
// TFWeaponBase Melee tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponInvis, DT_TFWeaponInvis )

BEGIN_NETWORK_TABLE( CTFWeaponInvis, DT_TFWeaponInvis )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponInvis )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_invis, CTFWeaponInvis );
PRECACHE_WEAPON_REGISTER( tf_weapon_invis );

// Server specific.
#if !defined( CLIENT_DLL ) 
	BEGIN_DATADESC( CTFWeaponInvis )
	END_DATADESC()
#endif

//-----------------------------------------------------------------------------
// Purpose: Use the offhand view model
//-----------------------------------------------------------------------------
void CTFWeaponInvis::Spawn( void )
{
	BaseClass::Spawn();

	SetViewModelIndex( 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponInvis::OnActiveStateChanged( int iOldState )
{
	BaseClass::OnActiveStateChanged( iOldState );

	// If we are being removed, we need to remove all stealth effects from our owner
	if ( m_iState == WEAPON_NOT_CARRIED && iOldState != WEAPON_NOT_CARRIED )
	{
		CleanupInvisibilityWatch();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Clear out the view model when we hide
//-----------------------------------------------------------------------------
void CTFWeaponInvis::HideThink( void )
{ 
	SetWeaponVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: Show/hide weapon and corresponding view model if any
// Input  : visible - 
//-----------------------------------------------------------------------------
void CTFWeaponInvis::SetWeaponVisible( bool visible )
{
	CBaseViewModel *vm = NULL;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner )
	{
		vm = pOwner->GetViewModel( m_nViewModelIndex );
	}

	if ( visible )
	{
		RemoveEffects( EF_NODRAW );
		if ( vm )
		{
			vm->RemoveEffects( EF_NODRAW );
		}
	}
	else
	{
		AddEffects( EF_NODRAW );
		if ( vm )
		{
			vm->AddEffects( EF_NODRAW );
		}
	}
}

//-----------------------------------------------------------------------------
bool CTFWeaponInvis::Deploy( void )
{
	bool b = BaseClass::Deploy();

	SetWeaponIdleTime( gpGlobals->curtime + 1.5 );

	return b;
}

//-----------------------------------------------------------------------------
bool CTFWeaponInvis::Holster( CBaseCombatWeapon *pSwitchingTo )
{ 
	bool bHolster = BaseClass::Holster( pSwitchingTo );

	// far in the future
	SetWeaponIdleTime( gpGlobals->curtime + 10 );

	return bHolster;
}

//-----------------------------------------------------------------------------
void CTFWeaponInvis::PrimaryAttack( void )
{
	// do nothing
}

//-----------------------------------------------------------------------------
void CTFWeaponInvis::SecondaryAttack( void )
{
	// do nothing
}

//-----------------------------------------------------------------------------
void CTFWeaponInvis::ItemBusyFrame( void )
{
	// do nothing
}

//-----------------------------------------------------------------------------
// Purpose: the player alt-fired or otherwise activated the functionality of
//			the invisibility watch
//-----------------------------------------------------------------------------
bool CTFWeaponInvis::ActivateInvisibilityWatch( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return false;

	SetCloakRates();

	bool bDoSkill = false;
	// If we're in TF_COND_STEALTHED - which means we gave it ourselves - always remove it
	// If we're in TF_COND_STEALTHED_USER_BUFF and we have Dead Ringer, allow it to be toggled
	// since we're allowed to fire from stealth
	if ( pOwner->m_Shared.InCond( TF_COND_STEALTHED ) )
	{
		pOwner->m_Shared.FadeInvis( 1.0f );
	}
	else
	{
		if ( pOwner->CanGoInvisible() && ( pOwner->m_Shared.GetSpyCloakMeter() > 8.0f ) )	// must have over 10% cloak to start
		{
			// Do standard cloak.
			pOwner->m_Shared.AddCond( TF_COND_STEALTHED, -1.f, pOwner );


			bDoSkill = true;
		}
	}

	if ( bDoSkill )
	{
		pOwner->m_Shared.SetNextStealthTime( gpGlobals->curtime + 0.5 );
	}
	else
	{
		pOwner->m_Shared.SetNextStealthTime( gpGlobals->curtime + 0.1 );
	}

	return bDoSkill;
}

//-----------------------------------------------------------------------------
// Purpose: the player has changed loadouts or done something else that causes
//			us to clean up any side effects of our watch
//-----------------------------------------------------------------------------
void CTFWeaponInvis::CleanupInvisibilityWatch( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	if ( pOwner->m_Shared.IsStealthed() )
	{
		// De-cloak.
		pOwner->m_Shared.FadeInvis( 1.0f );
	}

	pOwner->HolsterOffHandWeapon();
}

//-----------------------------------------------------------------------------
// Purpose: Set the correct cloak consume & regen rates for this item.
//-----------------------------------------------------------------------------
void CTFWeaponInvis::SetCloakRates( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	
	pOwner->m_Shared.SetCloakConsumeRate( tf_spy_cloak_consume_rate.GetFloat() );

	float fCloakRegenRate = tf_spy_cloak_regen_rate.GetFloat();
	pOwner->m_Shared.SetCloakRegenRate( fCloakRegenRate );
}

#ifndef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Return the right pda panel for the watch model we're using
//-----------------------------------------------------------------------------
void CTFWeaponInvis::GetControlPanelInfo( int nPanelIndex, const char *&pPanelName )
{
	const char *pszViewModel = GetViewModel(0);
	if ( Q_stristr( pszViewModel, "pocket" ) )
	{
		pPanelName = "pda_panel_spy_invis_pocket";
	}
	else if ( Q_stristr( pszViewModel, "ttg_watch_spy" ) )
	{
		pPanelName = "pda_panel_spy_invis_pocket_ttg";
	}
	else if ( Q_stristr( pszViewModel, "hm_watch" ) )
	{
		pPanelName = "pda_panel_spy_invis_pocket_hm";
	}
	else
	{
		pPanelName = "pda_panel_spy_invis";
	}
}

#endif

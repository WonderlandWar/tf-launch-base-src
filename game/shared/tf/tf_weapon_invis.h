//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: PDA Weapon
//
//=============================================================================

#ifndef TF_WEAPON_INVIS_H
#define TF_WEAPON_INVIS_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "tf_weaponbase.h"
#include "tf_gamerules.h"

// Client specific.
#if defined( CLIENT_DLL ) 
#define CTFWeaponInvis C_TFWeaponInvis
#endif

class CTFWeaponInvis : public CTFWeaponBase
{
public:
	DECLARE_CLASS( CTFWeaponInvis, CTFWeaponBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

#if !defined( CLIENT_DLL ) 
	DECLARE_DATADESC();
#endif

	CTFWeaponInvis() {}

	virtual void	Spawn();
	virtual void	OnActiveStateChanged( int iOldState );
	virtual void	PrimaryAttack( void );
	virtual void	SecondaryAttack();
	virtual bool	Deploy( void );

	virtual void	HideThink( void );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );

	virtual int		GetWeaponID( void ) const						{ return TF_WEAPON_INVIS; }
	virtual bool	ShouldDrawCrosshair( void )						{ return false; }
	virtual bool	HasPrimaryAmmo()								{ return true; }
	virtual bool	CanBeSelected()									{ return true; }

	virtual bool	VisibleInWeaponSelection( void )				{ return false; }

	virtual bool	ShouldShowControlPanels( void )					{ return true; }

	virtual void	SetWeaponVisible( bool visible );

	virtual void	ItemBusyFrame( void );

	virtual void	SetCloakRates( void );

	virtual bool	ActivateInvisibilityWatch( void );
	virtual void	CleanupInvisibilityWatch( void );

	virtual	bool	AllowsAutoSwitchTo( void )						{ return false; }
	virtual bool	CanDeploy( void )								{ return false; }

#ifndef CLIENT_DLL
	virtual void	GetControlPanelInfo( int nPanelIndex, const char *&pPanelName );
#endif

private:

	CTFWeaponInvis( const CTFWeaponInvis & ) {}
};

#endif // TF_WEAPON_INVIS_H

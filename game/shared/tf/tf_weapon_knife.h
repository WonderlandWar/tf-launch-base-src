//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Weapon Knife Class
//
//=============================================================================
#ifndef TF_WEAPON_KNIFE_H
#define TF_WEAPON_KNIFE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFKnife C_TFKnife
#endif

//=============================================================================
//
// Knife class.
//
class CTFKnife : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFKnife, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFKnife();
	virtual void		PrimaryAttack( void ) OVERRIDE;
	virtual int			GetWeaponID( void ) const OVERRIDE			{ return TF_WEAPON_KNIFE; }

	virtual float		GetMeleeDamage( CBaseEntity *pTarget, int* piDamageType, int* piCustomDamage ) OVERRIDE;

	virtual void		SendPlayerAnimEvent( CTFPlayer *pPlayer ) OVERRIDE;

	bool				CanPerformBackstabAgainstTarget( CTFPlayer *pTarget );		// "backstab" sometimes means "frontstab"
	bool				IsBehindAndFacingTarget( CTFPlayer *pTarget );
	bool				IsBackstab( void ) { return (m_hBackstabVictim.Get() != NULL); }

	virtual void		ItemPostFrame( void ) OVERRIDE;

	virtual bool		CalcIsAttackCriticalHelper( void ) OVERRIDE;
	virtual bool		CalcIsAttackCriticalHelperNoCrits( void ) OVERRIDE;
	virtual void		WeaponReset( void ) OVERRIDE; 
	
	virtual void		SecondaryAttack( void ) OVERRIDE;

private:
	CHandle<CTFPlayer>	m_hBackstabVictim;

	bool m_bWasTaunting;

	CTFKnife( const CTFKnife & ) {}
};

#endif // TF_WEAPON_KNIFE_H

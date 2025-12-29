//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_WRENCH_H
#define TF_WEAPON_WRENCH_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"
#include "tf_item_wearable.h"

#ifdef CLIENT_DLL
#define CTFWrench C_TFWrench
#define CTFRobotArm C_TFRobotArm
#define CTFWearableRobotArm C_TFWearableRobotArm
#endif

//=============================================================================
//
// Wrench class.
//
class CTFWrench : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFWrench, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFWrench();

	virtual void		Spawn();
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_WRENCH; }
	virtual void		Smack( void );

	// TF2007: It might be more accurate to remove these 2
	float				GetConstructionValue( void );
	float				GetRepairAmount( void );

#ifdef GAME_DLL
	void				OnFriendlyBuildingHit( CBaseObject *pObject, CTFPlayer *pPlayer, Vector hitLoc );
#else
	virtual void		ItemPostFrame();
#endif


private:
	bool				m_bReloadDown;
	CTFWrench( const CTFWrench & ) {}
};

#endif // TF_WEAPON_WRENCH_H

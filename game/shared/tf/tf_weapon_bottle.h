//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef TF_WEAPON_BOTTLE_H
#define TF_WEAPON_BOTTLE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFBreakableMelee C_TFBreakableMelee
#define CTFBottle C_TFBottle
#define CTFBreakableSign C_TFBreakableSign
#define CTFStickBomb C_TFStickBomb
#endif

class CTFBreakableMelee : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFBreakableMelee, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS_OVERRIDE();
	DECLARE_PREDICTABLE_OVERRIDE();

	CTFBreakableMelee();

	virtual void		Smack( void ) OVERRIDE;
	virtual void		WeaponReset( void ) OVERRIDE;
	virtual bool		DefaultDeploy( char *szViewModel, char *szWeaponModel, int iActivity, char *szAnimExt ) OVERRIDE;

	virtual void		SwitchBodyGroups( void );

	virtual bool		IsBroken( void ) const { return m_bBroken; }
	virtual void		SetBroken( bool bBroken );

#ifdef CLIENT_DLL
	static void RecvProxy_Broken( const CRecvProxyData *pData, void *pStruct, void *pOut );
#endif

protected:

	CNetworkVar( bool,	m_bBroken  );
};

//=============================================================================
//
// Bottle class.
//
class CTFBottle : public CTFBreakableMelee
{
public:

	DECLARE_CLASS( CTFBottle, CTFBreakableMelee );
	DECLARE_NETWORKCLASS_OVERRIDE();
	DECLARE_PREDICTABLE_OVERRIDE();

	virtual int			GetWeaponID( void ) const OVERRIDE { return TF_WEAPON_BOTTLE; }
};

#endif // TF_WEAPON_BOTTLE_H

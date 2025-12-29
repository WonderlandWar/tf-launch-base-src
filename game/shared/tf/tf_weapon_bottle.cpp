//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_bottle.h"
#include "decals.h"

// Client specific.
#ifdef CLIENT_DLL
#include "prediction.h"
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_fx.h"
#include "tf_gamerules.h"
#endif

//=============================================================================
//
// Weapon Breakable Melee tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFBreakableMelee, DT_TFWeaponBreakableMelee )

BEGIN_NETWORK_TABLE( CTFBreakableMelee, DT_TFWeaponBreakableMelee )
#if defined( CLIENT_DLL )
	RecvPropBool( RECVINFO( m_bBroken ), 0, CTFBreakableMelee::RecvProxy_Broken )
#else
	SendPropBool( SENDINFO( m_bBroken ) )
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFBreakableMelee )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_bBroken, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nBody, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_INSENDTABLE )
#endif // CLIENT_DLL
END_PREDICTION_DATA()

//=============================================================================
//
// Weapon Bottle tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFBottle, DT_TFWeaponBottle )

BEGIN_NETWORK_TABLE( CTFBottle, DT_TFWeaponBottle )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFBottle )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_bottle, CTFBottle );
PRECACHE_WEAPON_REGISTER( tf_weapon_bottle );

//=============================================================================

#define TF_BREAKABLE_MELEE_BREAK_BODYGROUP 1
// Absolute body number of broken/not-broken since the server can't figure them out from the studiohdr.  Would only
// matter if we had other body groups going on anyway
#define TF_BREAKABLE_MELEE_BODY_NOTBROKEN 0
#define TF_BREAKABLE_MELEE_BODY_BROKEN 1

//=============================================================================
//
// Weapon Breakable Melee functions.
//

CTFBreakableMelee::CTFBreakableMelee()
{
	m_bBroken = false;
}

void CTFBreakableMelee::WeaponReset( void )
{
	BaseClass::WeaponReset();

	if ( !GetOwner() || !GetOwner()->IsAlive() )
	{
		m_bBroken = false;
	}
}

bool CTFBreakableMelee::DefaultDeploy( char *szViewModel, char *szWeaponModel, int iActivity, char *szAnimExt )
{
	bool bRet = BaseClass::DefaultDeploy( szViewModel, szWeaponModel, iActivity, szAnimExt );

	if ( bRet )
	{
		SwitchBodyGroups();
	}

	return bRet;
}

void CTFBreakableMelee::SwitchBodyGroups( void )
{
	int iState = 0;

	if ( m_bBroken == true )
	{
		iState = 1;
	}

#ifdef CLIENT_DLL
	// We'll successfully predict m_nBody along with m_bBroken, but this can be called outside prediction, in which case
	// we want to use the networked m_nBody value -- but still fixup our viewmodel which is clientside only.
	if ( prediction->InPrediction() )
		{ SetBodygroup( TF_BREAKABLE_MELEE_BREAK_BODYGROUP, iState ); }

	CTFPlayer *pTFPlayer = ToTFPlayer( GetOwner() );
	if ( pTFPlayer && pTFPlayer->GetActiveWeapon() == this )
	{
		C_BaseAnimating *pViewWpn = GetAppropriateWorldOrViewModel();
		if ( pViewWpn != this )
		{
			pViewWpn->SetBodygroup( TF_BREAKABLE_MELEE_BREAK_BODYGROUP, iState );
		}
	}
#else // CLIENT_DLL
	m_nBody = iState ? TF_BREAKABLE_MELEE_BODY_BROKEN : TF_BREAKABLE_MELEE_BODY_NOTBROKEN;
#endif // CLIENT_DLL
}

void CTFBreakableMelee::Smack( void )
{
	BaseClass::Smack();

	if ( ConnectedHit() && IsCurrentAttackACrit() )
	{
		SetBroken( true );
	}
}

void CTFBreakableMelee::SetBroken( bool bBroken )
{ 
	m_bBroken = bBroken;
	SwitchBodyGroups();
}

#ifdef CLIENT_DLL
/* static */ void CTFBreakableMelee::RecvProxy_Broken( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_TFBreakableMelee* pWeapon = ( C_TFBreakableMelee*) pStruct;

	if ( !!pData->m_Value.m_Int != pWeapon->m_bBroken )
	{
		pWeapon->m_bBroken = !!pData->m_Value.m_Int;
		pWeapon->SwitchBodyGroups();
	}
}
#endif // CLIENT_DLL

//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This is being kept for tf_2007, party hats have been a thing since launch
//
//=============================================================================

#ifndef TF_WEARABLE_H
#define TF_WEARABLE_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#include "c_baseanimating.h"
#else
#include "baseanimating.h"
#endif
#include "props_shared.h"
#include "GameEventListener.h"

#if defined( CLIENT_DLL )
#define CTFWearable C_TFWearable
#endif


#if defined( CLIENT_DLL )
class CTFWearable : public CBaseAnimating, public CGameEventListener
#else
class CTFWearable : public CBaseAnimating
#endif
{
	DECLARE_CLASS( CTFWearable, CBaseAnimating );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();
	DECLARE_PREDICTABLE();

	CTFWearable();

#if defined( GAME_DLL )
	void				Break( void );
	virtual int			UpdateTransmitState();
	virtual	int			ShouldTransmit( const CCheckTransmitInfo *pInfo );
#endif

#if defined( CLIENT_DLL )
	virtual int			InternalDrawModel( int flags );
	virtual bool		ShouldDraw();

	virtual void		OnDataChanged( DataUpdateType_t updateType );
	virtual void		FireGameEvent( IGameEvent *event );
#endif

	virtual int			GetSkin( void );
};

#endif // TF_WEARABLE_H

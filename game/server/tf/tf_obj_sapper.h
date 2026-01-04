//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Upgrade that damages the object over time
//
//=============================================================================//

#ifndef TF_OBJ_SAPPER_H
#define TF_OBJ_SAPPER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_obj_baseupgrade_shared.h"

#define SAPPER_MAX_HEALTH	100


enum SapperModel_t
{
	SAPPER_MODEL_PLACED,
	SAPPER_MODEL_PLACEMENT,
	SAPPER_MODEL_TOTAL
};


// ------------------------------------------------------------------------ //
// Sapper upgrade
// ------------------------------------------------------------------------ //
class CObjectSapper : public CBaseObjectUpgrade
{
	DECLARE_CLASS( CObjectSapper, CBaseObjectUpgrade );

public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CObjectSapper();

	virtual void	Spawn();
	virtual void	Precache();
	virtual bool	IsHostileUpgrade( void ) { return true; }
	virtual void	FinishedBuilding( void );
	virtual void	SetupAttachedVersion( void );
	virtual void	DetachObjectFromObject( void );
	virtual void	UpdateOnRemove( void );
	virtual void	OnGoActive( void );
	bool			IsParentValid( void );

	const char*		GetSapperModelName( SapperModel_t nModel );
	
	virtual void	SapperThink( void );

	virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	virtual void	Killed( const CTakeDamageInfo &info );

	virtual int		GetBaseHealth( void );

private:
	float m_flSapperDamageAccumulator;
	float m_flLastThinkTime;
};

#endif // TF_OBJ_SAPPER_H

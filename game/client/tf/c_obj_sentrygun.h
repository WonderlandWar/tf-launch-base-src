//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_OBJ_SENTRYGUN_H
#define C_OBJ_SENTRYGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseobject.h"
#include "ObjectControlPanel.h"
#include "c_tf_projectile_rocket.h"
#include "tempent.h"
#include "c_te_legacytempents.h"
#include "c_tf_player.h"

class C_MuzzleFlashModel;

//-----------------------------------------------------------------------------
// Purpose: Sentry object
//-----------------------------------------------------------------------------
class C_ObjectSentrygun : public C_BaseObject
{
	DECLARE_CLASS( C_ObjectSentrygun, C_BaseObject );
public:
	DECLARE_CLIENTCLASS();

	C_ObjectSentrygun();

	void GetAmmoCount( int &iShells, int &iMaxShells, int &iRockets, int & iMaxRockets );

	virtual BuildingHudAlert_t GetBuildingAlertLevel( void );

	virtual const char *GetHudStatusIcon( void );

	int GetKills( void ) { return m_iKills; }
	int GetAssists( void ) { return m_iAssists; }

	virtual void GetShadowRenderBounds( Vector &mins, Vector &maxs, ShadowType_t shadowType );

	virtual CStudioHdr *OnNewModel( void );
	virtual void UpdateDamageEffects( BuildingDamageLevel_t damageLevel );

	virtual void OnPlacementStateChanged( bool bValidPlacement );

	void DebugDamageParticles();

	virtual const char* GetStatusName() const;

	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	
	virtual void	GetTargetIDDataString( OUT_Z_BYTECAP(iMaxLenInBytes) wchar_t *sDataString, int iMaxLenInBytes );
	
	// Upgrading
	virtual int		GetUpgradeLevel( void ) { return m_iUpgradeLevel; }
	int				GetUpgradeMetal( void ) { return m_iUpgradeMetal; }
	virtual int		GetUpgradeMetalRequired( void ) { return m_iUpgradeMetalRequired; }
	virtual bool	IsUpgrading( void ) const { return ( m_iState == SENTRY_STATE_UPGRADING ); }

	void			CheckNearMiss( Vector vecStart, Vector vecEnd );

	// ITargetIDProvidesHint
public:
	virtual void	DisplayHintTo( C_BasePlayer *pPlayer );

private:

	virtual void UpgradeLevelChanged();

private:
	int m_iState;

	int m_iAmmoShells;
	int m_iMaxAmmoShells;
	int m_iAmmoRockets;

	// Upgrading
	int				m_iUpgradeLevel;
	int				m_iOldUpgradeLevel;
	int				m_iUpgradeMetal;
	int				m_iUpgradeMetalRequired;

	int m_iKills;
	int m_iAssists;

	int m_iPlacementBodygroup;

	int m_iOldBodygroups;

	int m_iOldModelIndex;

	bool m_bNearMiss;
	float m_flNextNearMissCheck;

	CNetworkHandle( CBaseEntity, m_hEnemy );

	Vector	m_vecLaserBeamPos;

private:
	C_ObjectSentrygun( const C_ObjectSentrygun & ); // not defined, not accessible
};

class C_TFProjectile_SentryRocket : public C_TFProjectile_Rocket
{
	DECLARE_CLASS( C_TFProjectile_SentryRocket, C_TFProjectile_Rocket );
public:
	DECLARE_CLIENTCLASS();

	virtual void CreateRocketTrails( void ) {}
};

#endif	//C_OBJ_SENTRYGUN_H

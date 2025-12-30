//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Engineer's Sentrygun
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_OBJ_SENTRYGUN_H
#define TF_OBJ_SENTRYGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_obj.h"
#include "tf_projectile_rocket.h"

class CTFPlayer;

enum
{
	SENTRY_LEVEL_1 = 0,
	// SENTRY_LEVEL_1_UPGRADING,
	SENTRY_LEVEL_2,
	// SENTRY_LEVEL_2_UPGRADING,
	SENTRY_LEVEL_3,
};

#define SF_SENTRY_UPGRADEABLE	(LAST_SF_BASEOBJ<<1)
#define SF_SENTRY_INFINITE_AMMO (LAST_SF_BASEOBJ<<2)

#define SENTRYGUN_MAX_HEALTH	150

#define SENTRYGUN_MINI_MAX_HEALTH	100

#define SENTRY_MAX_RANGE 1100.0f		// magic numbers are evil, people. adding this #define to demystify the value. (MSB 5/14/09)
#define SENTRY_MAX_RANGE_SQRD 1210000.0f


// ------------------------------------------------------------------------ //
// Sentrygun object that's built by the player
// ------------------------------------------------------------------------ //
class CObjectSentrygun : public CBaseObject
{
	DECLARE_CLASS( CObjectSentrygun, CBaseObject );

public:
	DECLARE_SERVERCLASS();

	CObjectSentrygun();

	static CObjectSentrygun* Create(const Vector &vOrigin, const QAngle &vAngles);

	virtual void	Spawn();
	virtual void	FirstSpawn();
	virtual void	Precache();
	virtual void	OnGoActive( void );
	virtual int		DrawDebugTextOverlays(void);
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	virtual void	Killed( const CTakeDamageInfo &info );
	virtual void	SetModel( const char *pModel );

	virtual bool	StartBuilding( CBaseEntity *pBuilder );
	virtual void	SetStartBuildingModel( void );
	virtual void	StartPlacement( CTFPlayer *pPlayer );
	
	// Upgrading
	void			UpgradeThink( void );
	virtual int		GetUpgradeLevel( void ) const { return m_iUpgradeLevel; }
	int				GetUpgradeMetal( void ) const { return m_iUpgradeMetal; }
	void			DoQuickBuild( bool bForceMax = false );
	float			GetUpgradeDuration( void );
	virtual int		GetMaxUpgradeLevel( void ) { return OBJ_MAX_UPGRADE_LEVEL; }
	int				GetUpgradeAmountPerHit( void );
	
	virtual int  GetUpgradeMetalRequired();

	// Engineer hit me with a wrench
	virtual bool	OnWrenchHit( CTFPlayer *pPlayer, CTFWrench *pWrench, Vector hitLoc );
	virtual bool	CheckUpgradeOnHit( CTFPlayer *pPlayer );
	virtual void	OnStartDisabled( void );
	virtual void	OnEndDisabled( void );

	virtual int		GetTracerAttachment( void );

	virtual bool	IsUpgrading( void ) const { return m_iState == SENTRY_STATE_UPGRADING; }

	virtual float	GetTimeSinceLastFired( void ) const		{ return m_timeSinceLastFired.GetElapsedTime();	}

	virtual const QAngle &GetTurretAngles( void ) const		{ return m_vecCurAngles; }

	virtual void	SetBuildingSize( void );

	void			ClearTarget( void ) { m_hEnemy = NULL; }
	CBaseEntity		*GetTarget( void ) { return m_hEnemy.Get(); }

	int				GetFireAttachment( void );

	void			EmitSentrySound( const char* soundname );
	void			EmitSentrySound( IRecipientFilter& filter, int iEntIndex, const char *soundname );

	CTFPlayer *		GetAssistingTeammate( float maxAssistDuration = -1.0f ) const;

	virtual int		GetBaseHealth( void ) { return SENTRYGUN_MAX_HEALTH; }

	void RemoveAllAmmo();

	virtual int		GetMaxHealthForCurrentLevel( void );
	
public:
	// Upgrade Level ( 1, 2, 3 )
	CNetworkVar( int, m_iUpgradeLevel );
	CNetworkVar( int, m_iUpgradeMetal );
	CNetworkVar( int, m_iUpgradeMetalRequired );
	int m_nDefaultUpgradeLevel;
	float m_flUpgradeCompleteTime;			// Time when the upgrade animation will complete

private:
	Vector GetEnemyAimPosition( CBaseEntity* pEnemy ) const;

	// Main think
	void SentryThink( void );

	// If the players hit us with a wrench, should we upgrade
	bool CanBeUpgraded( CTFPlayer *pPlayer );
	void StartUpgrading( void );
	void FinishUpgrading( void );

	// Target acquisition
	bool FindTarget( void );
	bool ValidTargetPlayer( CTFPlayer *pPlayer, const Vector &vecStart, const Vector &vecEnd );
	bool ValidTargetObject( CBaseObject *pObject, const Vector &vecStart, const Vector &vecEnd );
	bool ValidTargetBot( CBaseCombatCharacter *pBot, const Vector &vecStart, const Vector &vecEnd );

	void FoundTarget( CBaseEntity *pTarget, const Vector &vecSoundCenter, bool bNoSound=false );
	bool FInViewCone ( CBaseEntity *pEntity );
	int Range( CBaseEntity *pTarget );

	// Rotations
	void SentryRotate( void );
	bool MoveTurret( void );

	// Attack
	void Attack( void );
	void MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );
	bool Fire( void );
	bool FireRocket( void );

	int GetBaseTurnRate( void );
	
	CNetworkVar( int, m_iState );

	float m_flNextAttack;
	float m_flFireRate;
	IntervalTimer m_timeSinceLastFired;

	// Rotation
	int m_iRightBound;
	int m_iLeftBound;
	int	m_iBaseTurnRate;
	bool m_bTurningRight;

	QAngle m_vecCurAngles;
	QAngle m_vecGoalAngles;

	float m_flTurnRate;

	// Ammo
	CNetworkVar( int, m_iAmmoShells );
	CNetworkVar( int, m_iMaxAmmoShells );
	CNetworkVar( int, m_iAmmoRockets );
	CNetworkVar( int, m_iMaxAmmoRockets );

	int	m_iAmmoType;

	float m_flNextRocketAttack;

	// Target player / object
	CNetworkHandle( CBaseEntity, m_hEnemy );

	//cached attachment indeces
	int m_iAttachments[4];

	int m_iPitchPoseParameter;
	int m_iYawPoseParameter;

	float m_flLastAttackedTime;

	float m_flHeavyBulletResist;

	DECLARE_DATADESC();

	CountdownTimer m_inCombatThrottleTimer;
	void UpdateNavMeshCombatStatus( void );

	CHandle< CTFPlayer > m_lastTeammateWrenchHit;	// which teammate last hit us with a wrench
	IntervalTimer m_lastTeammateWrenchHitTimer;		// time since last wrench hit

	int m_iLastMuzzleAttachmentFired;
};

// sentry rocket class just to give it a unique class name, so we can distinguish it from other rockets
class CTFProjectile_SentryRocket : public CTFProjectile_Rocket
{
public:
	DECLARE_CLASS( CTFProjectile_SentryRocket, CTFProjectile_Rocket );
	DECLARE_NETWORKCLASS();

	CTFProjectile_SentryRocket();

	virtual int GetProjectileType() const OVERRIDE { return TF_PROJECTILE_SENTRY_ROCKET; }

	// Creation.
	static CTFProjectile_SentryRocket *Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL );	

	virtual void Spawn();
};

#endif // TF_OBJ_SENTRYGUN_H

//========= Copyright Valve Corporation, All rights reserved. ============//
//
//	Weapons.
//
//	CTFWeaponBase
//	|
//	|--> CTFWeaponBaseMelee
//	|		|
//	|		|--> CTFWeaponCrowbar
//	|		|--> CTFWeaponKnife
//	|		|--> CTFWeaponMedikit
//	|		|--> CTFWeaponWrench
//	|
//	|--> CTFWeaponBaseGrenade
//	|		|
//	|		|--> CTFWeapon
//	|		|--> CTFWeapon
//	|
//	|--> CTFWeaponBaseGun
//
//=============================================================================
#ifndef TF_WEAPONBASE_H
#define TF_WEAPONBASE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_playeranimstate.h"
#include "tf_weapon_parse.h"
#include "npcevent.h"
#include "tf_item_wearable.h"

// Client specific.
#if defined( CLIENT_DLL )
#define CTFWeaponBase C_TFWeaponBase
#define CTFWeaponAttachmentModel C_TFWeaponAttachmentModel
#define CTFWeaponBaseGrenadeProj C_TFWeaponBaseGrenadeProj
#include "tf_fx_muzzleflash.h"
#include "GameEventListener.h"
#endif // CLIENT_DLL

#define MAX_TRACER_NAME		128

class CTFPlayer;
class CBaseObject;
class CTFWeaponBaseGrenadeProj;
class CTFWeaponAttachmentModel;

// Given an ammo type (like from a weapon's GetPrimaryAmmoType()), this compares it
// against the ammo name you specify.
// TFTODO: this should use indexing instead of searching and strcmp()'ing all the time.
bool IsAmmoType( int iAmmoType, const char *pAmmoName );
void FindHullIntersection( const Vector &vecSrc, trace_t &tr, const Vector &mins, const Vector &maxs, CBaseEntity *pEntity );

// Reloading singly.
enum
{
	TF_RELOAD_START = 0,
	TF_RELOADING,
	TF_RELOADING_CONTINUE,
	TF_RELOAD_FINISH
};

// structure to encapsulate state of head bob
struct BobState_t
{
	BobState_t() 
	{ 
		m_flBobTime = 0; 
		m_flLastBobTime = 0;
		m_flLastSpeed = 0;
		m_flVerticalBob = 0;
		m_flLateralBob = 0;
	}

	float m_flBobTime;
	float m_flLastBobTime;
	float m_flLastSpeed;
	float m_flVerticalBob;
	float m_flLateralBob;
};

#ifdef CLIENT_DLL
float CalcViewModelBobHelper( CBasePlayer *player, BobState_t *pBobState );
void AddViewModelBobHelper( Vector &origin, QAngle &angles, BobState_t *pBobState );
#endif

// Interface for weapons that have a charge time
class ITFChargeUpWeapon 
{
public:
	virtual bool CanCharge( void ) = 0;

	virtual float GetChargeBeginTime( void ) = 0;

	virtual float GetChargeMaxTime( void ) = 0;

	virtual float GetCurrentCharge( void )
	{ 
		return ( gpGlobals->curtime - GetChargeBeginTime() ) / GetChargeMaxTime();
	}
};

class CTraceFilterIgnoreTeammates : public CTraceFilterSimple
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS( CTraceFilterIgnoreTeammates, CTraceFilterSimple );

	CTraceFilterIgnoreTeammates( const IHandleEntity *passentity, int collisionGroup, int iIgnoreTeam )
		: CTraceFilterSimple( passentity, collisionGroup ), m_iIgnoreTeam( iIgnoreTeam )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );

		if ( ( pEntity->IsPlayer() || pEntity->IsCombatItem() ) && ( pEntity->GetTeamNumber() == m_iIgnoreTeam || m_iIgnoreTeam == TEAM_ANY ) )
		{
			return false;
		}

		return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
	}

	int m_iIgnoreTeam;
};

class CTraceFilterIgnorePlayers : public CTraceFilterSimple
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS( CTraceFilterIgnorePlayers, CTraceFilterSimple );

	CTraceFilterIgnorePlayers( const IHandleEntity *passentity, int collisionGroup )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );
		if ( pEntity && pEntity->IsPlayer() )
			return false;

		return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
	}
};

class CTraceFilterIgnoreFriendlyCombatItems : public CTraceFilterSimple
{
public:
	DECLARE_CLASS( CTraceFilterIgnoreFriendlyCombatItems, CTraceFilterSimple );

	CTraceFilterIgnoreFriendlyCombatItems( const IHandleEntity *passentity, int collisionGroup, int iIgnoreTeam, bool bIsProjectile = false )
		: CTraceFilterSimple( passentity, collisionGroup ), m_iIgnoreTeam( iIgnoreTeam )
	{
		m_bCallerIsProjectile = bIsProjectile;
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );

// 		if ( ( pEntity->MyCombatCharacterPointer() || pEntity->MyCombatWeaponPointer() ) && pEntity->GetTeamNumber() == m_iIgnoreTeam )
// 			return false;
// 
// 		if ( pEntity->IsPlayer() && pEntity->GetTeamNumber() == m_iIgnoreTeam )
// 			return false;

		if ( pEntity->IsCombatItem() )
		{
			if ( pEntity->GetTeamNumber() == m_iIgnoreTeam )
				return false;

			// If source is a enemy projectile, be explicit, otherwise we fail a "IsTransparent" test downstream
			if ( m_bCallerIsProjectile )
				return true;
		}

		return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
	}

	int m_iIgnoreTeam;
	bool m_bCallerIsProjectile;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTraceFilterCollisionArrows : public CTraceFilterEntitiesOnly
{
public:
	DECLARE_CLASS_NOBASE( CTraceFilterCollisionArrows );

	CTraceFilterCollisionArrows( const IHandleEntity *passentity, const IHandleEntity *passentity2 )
		: m_pPassEnt( passentity ), m_pPassEnt2( passentity2 )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt ) )
			return false;
		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
		if ( pEntity )
		{
			if ( pEntity == m_pPassEnt2 )
				return false;
			if ( pEntity->GetCollisionGroup() == TF_COLLISIONGROUP_GRENADES )
				return false;
			if ( pEntity->GetCollisionGroup() == TFCOLLISION_GROUP_ROCKETS )
				return false;
			if ( pEntity->GetCollisionGroup() == TFCOLLISION_GROUP_ROCKET_BUT_NOT_WITH_OTHER_ROCKETS )
				return false;
			if ( pEntity->GetCollisionGroup() == COLLISION_GROUP_DEBRIS )
				return false;
			if ( pEntity->GetCollisionGroup() == TFCOLLISION_GROUP_RESPAWNROOMS )
				return false;
			if ( pEntity->GetCollisionGroup() == COLLISION_GROUP_NONE )
				return false;

			return true;
		}

		return true;
	}

protected:
	const IHandleEntity *m_pPassEnt;
	const IHandleEntity *m_pPassEnt2;
};

//=============================================================================
//
// Base TF Weapon Class
//
#if defined( CLIENT_DLL )
class CTFWeaponBase : public CBaseCombatWeapon, public CGameEventListener
#else
class CTFWeaponBase : public CBaseCombatWeapon
#endif
{
	DECLARE_CLASS( CTFWeaponBase, CBaseCombatWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
#if !defined ( CLIENT_DLL )
	DECLARE_DATADESC();
	DECLARE_ENT_SCRIPTDESC();
#endif

	// Setup.
	CTFWeaponBase();
	~CTFWeaponBase();

	virtual void Spawn();
	virtual void Activate( void );
	virtual void Precache();
	virtual bool IsPredicted() const			{ return true; }
	virtual void FallInit( void );

	// Weapon Data.
	CTFWeaponInfo const	&GetTFWpnData() const;
	virtual int GetWeaponID( void ) const;
	bool IsWeapon( int iWeapon ) const;
	virtual int	GetDamageType() const { return g_aWeaponDamageTypes[ GetWeaponID() ]; }
	virtual int GetCustomDamageType() const { return TF_DMG_CUSTOM_NONE; }
	virtual int	GetMaxClip1( void ) const;
	virtual int GetDefaultClip1( void ) const;

	virtual int	Clip1() { return m_iClip1; }
	virtual int	Clip2() { return m_iClip2; }

	// View model.
	virtual const char *GetViewModel( int iViewModel = 0 ) const;
	virtual const char *GetWorldModel( void ) const;

	virtual void Equip( CBaseCombatCharacter *pOwner );
	virtual void Drop( const Vector &vecVelocity );
	virtual void StartHolsterAnim( void );
	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	virtual bool Deploy( void );
	virtual void OnActiveStateChanged( int iOldState ); // TF2007: This function should probably be removed
	virtual bool VisibleInWeaponSelection( void );

	// Attacks.
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	void CalcIsAttackCritical( void );
	virtual bool CalcIsAttackCriticalHelper();
	virtual bool CalcIsAttackCriticalHelperNoCrits();
	bool IsCurrentAttackACrit() const { return m_bCurrentAttackIsCrit; }
	bool IsCurrentAttackARandomCrit() const { return m_bCurrentAttackIsCrit && m_bCurrentCritIsRandom; }
	virtual void GetProjectileFireSetup( CTFPlayer *pPlayer, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammates = true, float flEndDist = 2000.f );
	virtual QAngle GetSpreadAngles( void );
	virtual bool AreRandomCritsEnabled( void );

	// Reloads.
	virtual bool Reload( void );
	virtual void AbortReload( void );
	virtual bool DefaultReload( int iClipSize1, int iClipSize2, int iActivity );
	void SendReloadEvents();
	virtual bool IsReloading() const;			// is the weapon reloading right now?
	virtual float GetReloadSpeedScale() const { return 1.f; }

	// Sound.
	bool PlayEmptySound();

	// Activities.
	virtual void ItemBusyFrame( void );
	virtual void ItemPostFrame( void );
	virtual void ItemHolsterFrame( void );

	virtual void SetWeaponVisible( bool visible );

	virtual int GetActivityWeaponRole() const;
	virtual acttable_t *ActivityList( int &iActivityCount ) OVERRIDE;

	virtual int			GetViewModelWeaponRole() { return GetTFWpnData().m_iWeaponType; }

#ifdef GAME_DLL
	virtual void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
#endif

	// Utility.
	CBasePlayer *GetPlayerOwner() const;
	CTFPlayer *GetTFPlayerOwner() const;

#ifdef CLIENT_DLL
	virtual bool	ShouldPlayClientReloadSound() { return false; }

	C_BaseEntity *GetWeaponForEffect();

	virtual const char* ModifyEventParticles( const char* token ) { return token; }

	// Shadows
	virtual ShadowType_t ShadowCastType( void ) OVERRIDE;
#endif

	virtual bool	CanAttack();
	virtual int		GetCanAttackFlags() const { return TF_CAN_ATTACK_FLAG_NONE; }

	// Raising & Lowering for grenade throws
	bool			WeaponShouldBeLowered( void );
	virtual bool	Ready( void );
	virtual bool	Lower( void );

	virtual void	WeaponIdle( void );

	virtual void	WeaponReset( void );

	// Muzzleflashes
	virtual const char *GetMuzzleFlashEffectName_3rd( void ) { return NULL; }
	virtual const char *GetMuzzleFlashEffectName_1st( void ) { return NULL; }
	virtual const char *GetMuzzleFlashModel( void );
	virtual float	GetMuzzleFlashModelLifetime( void );
	virtual const char *GetMuzzleFlashParticleEffect( void );

	virtual const char	*GetTracerType( void );

	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	virtual bool		CanFireCriticalShot( bool bIsHeadshot = false, CBaseEntity *pTarget = NULL );
	virtual bool		CanFireRandomCriticalShot( float flCritChance );

	virtual bool IsViewModelFlipped( void );

	virtual float		GetLastDeployTime( void ) { return m_flLastDeployTime; }

	virtual bool		UseServerRandomSeed( void ) const { return true; }

// Server specific.
#if !defined( CLIENT_DLL )

	// Spawning.
	virtual void CheckRespawn();
	virtual CBaseEntity* Respawn();
	void Materialize();
	void AttemptToMaterialize();

	// Death.
	void Die( void );
	void SetDieThink( bool bDie );

	// Disguise weapon.
	void DisguiseWeaponThink( void );

	// Ammo.
	virtual const Vector& GetBulletSpread();

// Client specific.
#else

	bool			IsFirstPersonView();
	bool			UsingViewModel();
	C_BaseAnimating *GetAppropriateWorldOrViewModel();

	virtual bool	ShouldDraw( void ) OVERRIDE;
	virtual void	UpdateVisibility( void ) OVERRIDE;

	virtual void	ProcessMuzzleFlashEvent( void );
	virtual void	DispatchMuzzleFlash( const char* effectName, C_BaseEntity* pAttachEnt );
	virtual int		InternalDrawModel( int flags );
	virtual bool	OnInternalDrawModel( ClientModelRenderInfo_t *pInfo ) OVERRIDE;

	virtual bool	ShouldPredict();
	virtual void	PostDataUpdate( DataUpdateType_t updateType );
	void			UpdateModelIndex();
	virtual void	OnDataChanged( DataUpdateType_t type );
	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual int		GetWorldModelIndex( void );
	virtual bool	ShouldDrawCrosshair( void );
	virtual void	Redraw( void );
	virtual void	FireGameEvent( IGameEvent *event );

	virtual void	AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles );
	virtual	float	CalcViewmodelBob( void );
	BobState_t		*GetBobState();
#ifndef TF_DISABLE_ECON_ITEMS
	virtual bool	AttachmentModelsShouldBeVisible( void ) OVERRIDE { return (m_iState == WEAPON_IS_ACTIVE); }
#endif
	virtual bool ShouldEjectBrass() { return true; }

	bool OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options );

	// Model muzzleflashes
	CHandle<C_MuzzleFlashModel>		m_hMuzzleFlashModel[2];

	bool			IsUsingOverrideModel() const { return m_iWorldModelIndex != m_iCachedModelIndex; }

#endif

	virtual int		GetSkin();

	// Effect / Regeneration bar handling
	virtual float	GetEffectBarProgress( void );			// Get the current bar state (will return a value from 0.0 to 1.0)
	bool			HasEffectBarRegeneration( void ) { return InternalGetEffectBarRechargeTime() > 0; }	// Check the base, not modified by attribute, because attrib may have reduced it to 0.
	float			GetEffectBarRechargeTime( void ) { return InternalGetEffectBarRechargeTime(); }
	void			DecrementBarRegenTime( float flTime ) { m_flEffectBarRegenTime -= flTime; }

	virtual bool	CanPickupOtherWeapon() const { return true; }

	virtual bool ShouldRemoveInvisibilityOnPrimaryAttack() const { return true; }

protected:
	virtual int		GetEffectBarAmmo( void ) { return m_iPrimaryAmmoType; }
	virtual float	InternalGetEffectBarRechargeTime( void ) { return 0; }	// Time it takes for this regeneration bar to fully recharge from 0 to full.

	void			StartEffectBarRegen( void );						// Call this when you want your bar to start recharging (usually when you've deployed your action)
	void			EffectBarRegenFinished( void );
	void			CheckEffectBarRegen( void );

private:
	CNetworkVar(	float, m_flEffectBarRegenTime );	// The time Regen is scheduled to complete

protected:
#ifdef CLIENT_DLL
	virtual void CreateMuzzleFlashEffects( C_BaseEntity *pAttachEnt, int nIndex );
#endif // CLIENT_DLL

	// Reloads.
	void UpdateReloadTimers( bool bStart );
	void SetReloadTimer( float flReloadTime );
	bool ReloadSingly( void );
	void ReloadSinglyPostFrame( void );
	void IncrementAmmo( void );

	bool NeedsReloadForAmmo1( int iClipSize1 ) const;
	bool NeedsReloadForAmmo2( int iClipSize2 ) const;

protected:

	int				m_iWeaponMode;
	CNetworkVar(	int,	m_iReloadMode );
	CNetworkVar( float, m_flReloadPriorNextFire );
	CTFWeaponInfo	*m_pWeaponInfo;
	bool			m_bInAttack;
	bool			m_bInAttack2;
	bool			m_bCurrentAttackIsCrit;
	bool			m_bCurrentCritIsRandom;

	CNetworkVar(	bool,	m_bLowered );

	int				m_iAltFireHint;

	int				m_iReloadStartClipAmount;

	float			m_flCritTime;
	CNetworkVar( float, m_flLastCritCheckTime );	// Deprecated
	int				m_iLastCritCheckFrame;
	int				m_iCurrentSeed;
	float			m_flLastRapidFireCritCheckTime;

	float			m_flLastDeployTime;

	char			m_szTracerName[MAX_TRACER_NAME];

	CNetworkVar(	bool, m_bResetParity );

	int				m_iAmmoToAdd;

#ifdef CLIENT_DLL
	bool m_bOldResetParity;
	int m_iCachedModelIndex;
	int m_iEjectBrassAttachpoint;

#endif

	CNetworkVar(	bool,	m_bReloadedThroughAnimEvent );

public:
	CNetworkVar(	bool, m_bDisguiseWeapon );

	CNetworkVar(	float, m_flLastFireTime );

	CNetworkVar( float, m_flObservedCritChance );

	virtual float GetNextSecondaryAttackDelay( void ) OVERRIDE;

private:
	CTFWeaponBase( const CTFWeaponBase & );
};

bool WeaponID_IsSniperRifle( int iWeaponID );

#define WEAPON_RANDOM_RANGE 10000

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// CTFWeaponAttachmentModel 
//-----------------------------------------------------------------------------
class CTFWeaponAttachmentModel : public CBaseAnimating, public IHasOwner
{
	DECLARE_CLASS( CTFWeaponAttachmentModel, CBaseAnimating );
public:
	CTFWeaponAttachmentModel() { m_bIsViewModelAttachment = false; m_hWeaponAssociatedWith = NULL; }

	virtual bool ShouldDraw( void );
	
	void				Init( CBaseEntity *pParent, CTFWeaponBase *pAssociatedWeapon, bool bIsViewModel );
	void				SetWeaponAssociatedWith( CTFWeaponBase *pWeapon ) { m_hWeaponAssociatedWith = pWeapon; }
	CBaseEntity*		GetWeaponAssociatedWith( void ) const { return m_hWeaponAssociatedWith.Get(); }

	bool BIsViewModelAttachment() { return m_bIsViewModelAttachment; }
	
	virtual CBaseEntity	*GetOwnerViaInterface( void ) OVERRIDE { return m_hWeaponAssociatedWith.Get() ? m_hWeaponAssociatedWith.Get()->GetOwner() : NULL; }
private:

	bool m_bIsViewModelAttachment;
	CHandle< CTFWeaponBase > m_hWeaponAssociatedWith;
};
#endif // CLIENT_DLL

#endif // TF_WEAPONBASE_H

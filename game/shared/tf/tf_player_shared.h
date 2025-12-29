//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Shared player code.
//
//=============================================================================
#ifndef TF_PLAYER_SHARED_H
#define TF_PLAYER_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "networkvar.h"
#include "tf_shareddefs.h"
#include "tf_weaponbase.h"
#include "basegrenade_shared.h"
#include "SpriteTrail.h"
#include "tf_condition.h"

// Client specific.
#ifdef CLIENT_DLL
#include "baseobject_shared.h"
#include "c_tf_weapon_builder.h"
class C_TFPlayer;
// Server specific.
#else
#include "tf_weapon_builder.h"
class CTFPlayer;
#endif


//=============================================================================
//
// Tables.
//

// Client specific.
#ifdef CLIENT_DLL

	EXTERN_RECV_TABLE( DT_TFPlayerShared );

// Server specific.
#else

	EXTERN_SEND_TABLE( DT_TFPlayerShared );

#endif

//=============================================================================

#define PERMANENT_CONDITION		-1

// Damage storage for crit multiplier calculation
class CTFDamageEvent
{
#ifdef CLIENT_DLL
	// This redundantly declares friendship which leads to gcc warnings.
	//DECLARE_CLIENTCLASS_NOBASE();
#else
public:
	// This redundantly declares friendship which leads to gcc warnings.
	//DECLARE_SERVERCLASS_NOBASE();
#endif
	DECLARE_EMBEDDED_NETWORKVAR();

public:
	float	flDamage;
	float	flDamageCritScaleMultiplier;		// scale the damage by this amount when taking it into consideration for "should I crit?" calculations
	float	flTime;
	int		nDamageType;
	byte	nKills;
};

#ifdef CLIENT_DLL
struct WheelEffect_t
{
	WheelEffect_t( float flTriggerSpeed, const char *pszRedParticleName, const char *pszBlueParticleName )
		: m_flMinTriggerSpeed( flTriggerSpeed )
	{
		memset( m_pszParticleName, NULL, sizeof( m_pszParticleName ) );
		m_pszParticleName[ TF_TEAM_RED ] = pszRedParticleName;
		m_pszParticleName[ TF_TEAM_BLUE ] = pszBlueParticleName;
	}

	float m_flMinTriggerSpeed;
	const char *m_pszParticleName[ TF_TEAM_COUNT ];
};
#endif

//=============================================================================
// Scoring data for the local player
struct RoundStats_t;
struct localplayerscoring_t
{
	DECLARE_EMBEDDED_NETWORKVAR();
	DECLARE_CLASS_NOBASE( localplayerscoring_t );

	localplayerscoring_t()
	{
		Reset();
	}

	void Reset( void )
	{
		m_iCaptures = 0;
		m_iDefenses = 0;
		m_iKills = 0;
		m_iDeaths = 0;
		m_iSuicides = 0;
		m_iKillAssists = 0;
		m_iBuildingsBuilt = 0;
		m_iBuildingsDestroyed = 0;
		m_iHeadshots = 0;
		m_iDominations = 0;
		m_iRevenge = 0;
		m_iInvulns = 0;
		m_iTeleports = 0;
		m_iDamageDone = 0;
		m_iCrits = 0;
		m_iBackstabs = 0;
		m_iHealPoints = 0;
		m_iResupplyPoints = 0;
		m_iBonusPoints = 0;
		m_iPoints = 0;
	}

	void UpdateStats( RoundStats_t& roundStats, CTFPlayer *pPlayer, bool bIsRoundData );

	CNetworkVar( int, m_iCaptures );
	CNetworkVar( int, m_iDefenses );
	CNetworkVar( int, m_iKills );
	CNetworkVar( int, m_iDeaths );
	CNetworkVar( int, m_iSuicides );
	CNetworkVar( int, m_iDominations );
	CNetworkVar( int, m_iRevenge );
	CNetworkVar( int, m_iBuildingsBuilt );
	CNetworkVar( int, m_iBuildingsDestroyed );
	CNetworkVar( int, m_iHeadshots );
	CNetworkVar( int, m_iBackstabs );
	CNetworkVar( int, m_iHealPoints );
	CNetworkVar( int, m_iInvulns );
	CNetworkVar( int, m_iTeleports );
	CNetworkVar( int, m_iDamageDone );
	CNetworkVar( int, m_iCrits );
	CNetworkVar( int, m_iResupplyPoints );
	CNetworkVar( int, m_iKillAssists );
	CNetworkVar( int, m_iBonusPoints );
	CNetworkVar( int, m_iPoints );
};


// Condition Provider
struct condition_source_t
{
	DECLARE_EMBEDDED_NETWORKVAR();
	DECLARE_CLASS_NOBASE( condition_source_t );

	condition_source_t()
	{
		m_nPreventedDamageFromCondition = 0;
		m_flExpireTime = 0.f;
		m_pProvider = NULL;
		m_bPrevActive = false;
	}

	int	m_nPreventedDamageFromCondition;
	float	m_flExpireTime;
	CNetworkHandle( CBaseEntity, m_pProvider );
	bool	m_bPrevActive;
};

//=============================================================================
//
// Shared player class.
//
class CTFPlayerShared
{
public:

// Client specific.
#ifdef CLIENT_DLL

	friend class C_TFPlayer;
	typedef C_TFPlayer OuterClass;
	DECLARE_PREDICTABLE();

// Server specific.
#else

	friend class CTFPlayer;
	typedef CTFPlayer OuterClass;

#endif
	
	DECLARE_EMBEDDED_NETWORKVAR()
	DECLARE_CLASS_NOBASE( CTFPlayerShared );

private:

	// Condition Provider tracking
	CUtlVector< condition_source_t > m_ConditionData;

public:

	// Initialization.
	CTFPlayerShared();
	void Init( OuterClass *pOuter );
	void Spawn( void );

	// State (TF_STATE_*).
	int		GetState() const					{ return m_nPlayerState; }
	void	SetState( int nState )				{ m_nPlayerState = nState; }
	bool	InState( int nState )				{ return ( m_nPlayerState == nState ); }

	void	SharedThink( void );

	// Condition (TF_COND_*).
	void	AddCond( ETFCond eCond, float flDuration = PERMANENT_CONDITION, CBaseEntity *pProvider = NULL );
	void	RemoveCond( ETFCond eCond, bool ignore_duration=false );
	bool	InCond( ETFCond eCond ) const;
	bool	WasInCond( ETFCond eCond ) const;
	void	ForceRecondNextSync( ETFCond eCond );
	void	RemoveAllCond();
	void	OnConditionAdded( ETFCond eCond );
	void	OnConditionRemoved( ETFCond eCond );
	void	ConditionThink( void );
	float	GetConditionDuration( ETFCond eCond ) const;
	void	SetConditionDuration( ETFCond eCond, float flNewDur )
	{
		Assert( eCond < m_ConditionData.Count() );
		m_ConditionData[eCond].m_flExpireTime = flNewDur;
	}

	CBaseEntity *GetConditionProvider( ETFCond eCond ) const;

	void	ConditionGameRulesThink( void );

	void	InvisibilityThink( void );

	void	CheckDisguiseTimer( void );

	int		GetMaxBuffedHealth( bool bIgnoreAttributes = false, bool bIgnoreHealthOverMax = false );
#ifdef GAME_DLL
	float	GetMaxOverhealMultiplier( void );
#endif 

	bool	IsAiming( void );

#ifdef CLIENT_DLL
	// This class only receives calls for these from C_TFPlayer, not
	// natively from the networking system
	virtual void OnPreDataChanged( void );
	virtual void OnDataChanged( void );

	// check the newly networked conditions for changes
	void	SyncConditions( int nPreviousConditions, int nNewConditions, int nForceConditionBits, int nBaseCondBit );

	bool	ShouldShowRecentlyTeleported( void );
#endif

	bool	IsInvulnerable( void ) const;
	bool	IsStealthed( void ) const;
	bool	CanBeDebuffed( void ) const;

	void	Disguise( int nTeam, int nClass, CTFPlayer* pDesiredTarget=NULL, bool bOnKill = false );
	void	CompleteDisguise( void );
	void	RemoveDisguise( void );
	void	RemoveDisguiseWeapon( void );
	void	FindDisguiseTarget( void );
	int		GetDisguiseTeam( void ) const;
	int		GetDisguiseClass( void ) const			{ return m_nDisguiseClass; }
	int		GetDisguisedSkinOverride( void ) const	{ return m_nDisguiseSkinOverride; }
	int		GetDisguiseMask( void )	const			{ return m_nMaskClass; }
	int		GetDesiredDisguiseClass( void )	const	{ return m_nDesiredDisguiseClass; }
	int		GetDesiredDisguiseTeam( void ) const	{ return m_nDesiredDisguiseTeam; }
	bool	WasLastDisguiseAsOwnTeam( void ) const	{ return m_bLastDisguisedAsOwnTeam; }
	// Josh: Hack for not including c_tf_player.h in replay code
	// as it causes a bunch of issues with redefines.
#ifndef REPLAY_SOURCE_FILE
	CTFPlayer *GetDisguiseTarget( void ) const			{ return m_hDisguiseTarget; }
#endif
	CTFWeaponBase *GetDisguiseWeapon( void ) const		{ return m_hDisguiseWeapon; }
	int		GetDisguiseHealth( void )			{ return m_iDisguiseHealth; }
	void	SetDisguiseHealth( int iDisguiseHealth );
	int		GetDisguiseMaxHealth( void );
	int		GetDisguiseMaxBuffedHealth( bool bIgnoreAttributes = false, bool bIgnoreHealthOverMax = false );
	void	ProcessDisguiseImpulse( CTFPlayer *pPlayer );
	int		GetDisguiseAmmoCount( void ) { return m_iDisguiseAmmo; }
	void	SetDisguiseAmmoCount( int nValue ) { m_iDisguiseAmmo = nValue; }

	bool	CanRecieveMedigunChargeEffect() const;
#ifdef CLIENT_DLL
	int		GetDisplayedTeam( void ) const;
	void	OnDisguiseChanged( void );

	float	GetTimeTeleEffectAdded( void ) { return m_flGotTeleEffectAt; }
	int		GetTeamTeleporterUsed( void ) { return m_nTeamTeleporterUsed; }
#endif

	int		CalculateObjectCost( CTFPlayer* pBuilder, int iObjectType );

	// Pickup effects, including putting out fires, updating HUD, etc.
	void	HealthKitPickupEffects( int iHealthGiven = 0 );

#ifdef GAME_DLL
	void	DetermineDisguiseWeapon( bool bForcePrimary = false );
	void	DetermineDisguiseWearables();
	void	Heal( CBaseEntity *pHealer, float flAmount, float flOverhealBonus, float flOverhealDecayMult, bool bDispenserHeal = false, CTFPlayer *pHealScorer = NULL );
	float	StopHealing( CBaseEntity *pHealer );
	void	TestAndExpireChargeEffect( void );
	void	RecalculateChargeEffects( bool bInstantRemove = false );
	int		FindHealerIndex( CBaseEntity *pPlayer );
	EHANDLE	GetFirstHealer();
	void	CheckForAchievement( int iAchievement );

	void	IncrementArenaNumChanges( void ) { m_nArenaNumChanges++; }
	void	ResetArenaNumChanges( void ) { m_nArenaNumChanges = 0; }

	void	SetTeamTeleporterUsed( int nTeam ){ m_nTeamTeleporterUsed.Set( nTeam ); }

#endif	// GAME_DLL

	int		GetArenaNumChanges( void ) { return m_nArenaNumChanges; }

	CBaseEntity *GetHealerByIndex( int index );
	bool HealerIsDispenser( int index );
	int		GetNumHealers( void ) { return m_nNumHealers; }

	void	Burn( CTFPlayer *pPlayer, CTFWeaponBase *pWeapon );
	void	SelfBurn( void );		// Boss Burn

	// Weapons.
	CTFWeaponBase *GetActiveTFWeapon() const;

	// Utility.
	bool	IsAlly( CBaseEntity *pEntity );

	// Separation force
	bool	IsSeparationEnabled( void ) const	{ return m_bEnableSeparation; }
	void	SetSeparation( bool bEnable )		{ m_bEnableSeparation = bEnable; }
	const Vector &GetSeparationVelocity( void ) const { return m_vSeparationVelocity; }
	void	SetSeparationVelocity( const Vector &vSeparationVelocity ) { m_vSeparationVelocity = vSeparationVelocity; }

	void	FadeInvis( float fAdditionalRateScale );
	float	GetPercentInvisible( void ) const;
	float	GetPercentInvisiblePrevious( void ) { return m_flPrevInvisibility; }
	void	NoteLastDamageTime( int nDamage );
	void	OnSpyTouchedByEnemy( void );
	float	GetLastStealthExposedTime( void ) { return m_flLastStealthExposeTime; }
	void	SetNextStealthTime( float flTime ) { m_flStealthNextChangeTime = flTime; }
	bool	IsFullyInvisible( void ) { return ( GetPercentInvisible() == 1.f ); }

	bool	IsEnteringOrExitingFullyInvisible( void );

	int		GetDesiredPlayerClassIndex( void );

	// Cloak, rage, phase, team juice...this should really all be done with composition?
	void	UpdateCloakMeter( void );
	float	GetSpyCloakMeter() const		{ return m_flCloakMeter; }
	void	SetSpyCloakMeter( float val ) { m_flCloakMeter = val; }

	bool	IsJumping( void ) const			{ return m_bJumping; }
	void	SetJumping( bool bJumping );
	bool    IsAirDashing( void ) const		{ return (m_iAirDash > 0); }
	int		GetAirDash( void ) const		{ return m_iAirDash; }
	void    SetAirDash( int iAirDash );
	void	SetAirDucked( int nAirDucked )	{ m_nAirDucked = nAirDucked; }
	int		AirDuckedCount( void )			{ return m_nAirDucked; }
	void	SetDuckTimer( float flTime )	{ m_flDuckTimer = flTime; }
	float	GetDuckTimer( void ) const		{ return m_flDuckTimer; }

	void	DebugPrintConditions( void );
	void	InstantlySniperUnzoom( void );

	float	GetStealthNoAttackExpireTime( void );

	void	SetPlayerDominated( CTFPlayer *pPlayer, bool bDominated );
	bool	IsPlayerDominated( int iPlayerIndex );
	bool	IsPlayerDominatingMe( int iPlayerIndex );
	void	SetPlayerDominatingMe( CTFPlayer *pPlayer, bool bDominated );

	float	GetDisguiseCompleteTime( void ) { return m_flDisguiseCompleteTime; }
	bool	IsSpyDisguisedAsMyTeam( CTFPlayer *pPlayer );

	CTFPlayer *GetAssist( void ) const			{ return m_hAssist; }
	void	SetAssist( CTFPlayer* newAssist )	{ m_hAssist = newAssist; }

#ifdef GAME_DLL
	CTFPlayer *GetBurnAttacker( void ) const			{ return m_hBurnAttacker; }
	CTFPlayer *GetOriginalBurnAttacker( void ) const	{ return m_hOriginalBurnAttacker; }
	CTFWeaponBase *GetBurnWeapon( void ) const			{ return m_hBurnWeapon; }
#endif // GAME_DLL

	void SetCloakConsumeRate( float newCloakConsumeRate ) { m_fCloakConsumeRate = newCloakConsumeRate; }
	void SetCloakRegenRate( float newCloakRegenRate ) { m_fCloakRegenRate = newCloakRegenRate; }

	int		GetWeaponKnockbackID( void ) const	{ return m_iWeaponKnockbackID; }
	void	SetWeaponKnockbackID( int iID )		{ m_iWeaponKnockbackID = iID; }

	float GetInvulOffTime( void ) { return m_flInvulnerabilityRemoveTime; }

	int		GetDisguiseBody( void ) const	{ return m_iDisguiseBody; }
	void	SetDisguiseBody( int iVal )		{ m_iDisguiseBody = iVal; }

	void	StartBuildingObjectOfType( int iType, int iObjectMode=0 );

public:
	// Scoring
	int	GetCaptures( int iIndex ) const				{ return m_ScoreData.m_iCaptures; }
	int	GetDefenses( int iIndex ) const				{ return m_ScoreData.m_iDefenses; }
	int	GetDominations( int iIndex ) const			{ return m_ScoreData.m_iDominations; }
	int	GetRevenge( int iIndex ) const				{ return m_ScoreData.m_iRevenge; }
	int	GetBuildingsDestroyed( int iIndex ) const	{ return m_ScoreData.m_iBuildingsDestroyed; }
	int	GetHeadshots( int iIndex ) const			{ return m_ScoreData.m_iHeadshots; }
	int	GetBackstabs( int iIndex ) const			{ return m_ScoreData.m_iBackstabs; }
	int	GetHealPoints( int iIndex ) const			{ return m_ScoreData.m_iHealPoints; }
	int	GetInvulns( int iIndex ) const				{ return m_ScoreData.m_iInvulns; }
	int	GetTeleports( int iIndex ) const			{ return m_ScoreData.m_iTeleports; }
	int	GetResupplyPoints( int iIndex ) const		{ return m_ScoreData.m_iResupplyPoints; }
	int	GetKillAssists( int iIndex ) const			{ return m_ScoreData.m_iKillAssists; }
	int GetBonusPoints( int iIndex ) const			{ return m_ScoreData.m_iBonusPoints; }

	void ResetScores( void ) { m_ScoreData.Reset(); }
	localplayerscoring_t *GetScoringData( void ) { return &m_ScoreData; }

	// Per-round scoring data utilized by the steamworks stats system.
	void ResetRoundScores( void ) { m_RoundScoreData.Reset(); }
	localplayerscoring_t *GetRoundScoringData( void ) { return &m_RoundScoreData; }

	int GetLastDuckStreakIncrement( void ) const	{ return m_nLastDuckStreakIncrement; }

	int GetSequenceForDeath( CBaseAnimating* pRagdoll, bool bBurning, int nCustomDeath );

	void IncrementRespawnTouchCount() { ++m_iSpawnRoomTouchCount; }
	void DecrementRespawnTouchCount() { m_iSpawnRoomTouchCount = Max( m_iSpawnRoomTouchCount - 1, 0 ); }
	int GetRespawnTouchCount() const { return m_iSpawnRoomTouchCount; }

#ifdef GAME_DLL
	void SetBestOverhealDecayMult( float fValue )	{ m_flBestOverhealDecayMult = fValue; }
	float GetBestOverhealDecayMult() const			{ return m_flBestOverhealDecayMult; }

	void  SetChargeEffectOffTime( float flTime ) { m_flChargeEffectOffTime = flTime; }
#endif

	int GetTauntConcept( void ) const { return m_iTauntConcept; }

	int	GetItemFindBonus( void ) //{ return m_iItemFindBonus; }
	{
#ifdef GAME_DLL
		if ( !m_iItemFindBonus )
		{
			m_iItemFindBonus = RandomInt( 1, 300 );
		}
#endif
		return m_iItemFindBonus;
	}
	
	void	RecalculatePlayerBodygroups( void );
	
#ifdef GAME_DLL
	float	GetFlameBurnTime( void ) const { return m_flFlameBurnTime; }
#endif // GAME_DLL

	void GetConditionsBits( CBitVec< TF_COND_LAST >& vbConditions ) const;

private:
	CNetworkVarEmbedded( localplayerscoring_t,	m_ScoreData );
	CNetworkVarEmbedded( localplayerscoring_t,	m_RoundScoreData );

private:

	void ImpactWaterTrace( trace_t &trace, const Vector &vecStart );

	void OnAddZoomed( void );
	void OnAddStealthed( void );
	void OnAddInvulnerable( void );
	void OnAddTeleported( void );
	void OnAddBurning( void );
	void OnAddDisguising( void );
	void OnAddDisguised( void );
	void OnAddOverhealed( void );
	void OnAddTaunting( void );


	void OnRemoveZoomed( void );
	void OnRemoveBurning( void );
	void OnRemoveStealthed( void );
	void OnRemoveDisguised( void );
	void OnRemoveDisguising( void );
	void OnRemoveInvulnerable( void );
	void OnRemoveTeleported( void );
	void OnRemoveOverhealed( void );
	void OnRemoveInvulnerableWearingOff( void );
	void OnRemoveTaunting( void );

	float GetCritMult( void );

#ifdef GAME_DLL
	void  UpdateCritMult( void );
	void  RecordDamageEvent( const CTakeDamageInfo &info, bool bKill, int nVictimPrevHealth );
	void  AddTempCritBonus( float flAmount );
	void  ClearDamageEvents( void ) { m_DamageEvents.Purge(); }
	int	  GetNumKillsInTime( float flTime );

	// Invulnerable.
	void  SendNewInvulnGameEvent( void );
	void  SetChargeEffect( bool bState, bool bInstant, float flWearOffTime, CTFPlayer *pProvider = NULL );
	void  SetCritBoosted( bool bState );

public:
	void SetAfterburnDuration( float flDuration ) { m_flAfterburnDuration = flDuration; }
	float GetAfterburnDuration( void ) { return m_flAfterburnDuration; }
#endif // GAME_DLL

private:
	// Vars that are networked.
	CNetworkVar( int, m_nPlayerState );			// Player state.
	CNetworkVar( int, m_nPlayerCond );			// Player condition flags.
	CNetworkVar( int, m_nPlayerCondEx );		// Player condition flags (extended -- we overflowed 32 bits).
	CNetworkVar( int, m_nPlayerCondEx2 );		// Player condition flags (extended -- we overflowed 64 bits).
	CNetworkVar( int, m_nPlayerCondEx3 );		// Player condition flags (extended -- we overflowed 96 bits).
	CNetworkVar( int, m_nPlayerCondEx4 );		// Player condition flags (extended -- we overflowed 128 bits).

	CNetworkVarEmbedded( CTFConditionList, m_ConditionList );

//TFTODO: What if the player we're disguised as leaves the server?
//...maybe store the name instead of the index?
	CNetworkVar( int, m_nDisguiseTeam );		// Team spy is disguised as.
	CNetworkVar( int, m_nDisguiseClass );		// Class spy is disguised as.
	CNetworkVar( int, m_nDisguiseSkinOverride ); // skin override value of the player spy disguised as.
	CNetworkVar( int, m_nMaskClass );
	CNetworkHandle( CTFPlayer, m_hDisguiseTarget ); // Player the spy is using for name disguise.
	CNetworkVar( int, m_iDisguiseHealth );		// Health to show our enemies in player id
	CNetworkVar( int, m_nDesiredDisguiseClass );
	CNetworkVar( int, m_nDesiredDisguiseTeam );
	CNetworkHandle( CTFWeaponBase, m_hDisguiseWeapon );
	CNetworkVar( int, m_nTeamTeleporterUsed ); // for disguised spies using enemy teleporters
	CHandle<CTFPlayer>	m_hDesiredDisguiseTarget;
	int m_iDisguiseAmmo;

	bool m_bEnableSeparation;		// Keeps separation forces on when player stops moving, but still penetrating
	Vector m_vSeparationVelocity;	// Velocity used to keep player separate from teammates

	float m_flInvisibility;
	float m_flPrevInvisibility;
	CNetworkVar( float, m_flInvisChangeCompleteTime );		// when uncloaking, must be done by this time
	float m_flLastStealthExposeTime;
	float m_fCloakConsumeRate;
	float m_fCloakRegenRate;

	CNetworkVar( int, m_nNumHealers );

	// Vars that are not networked.
	OuterClass			*m_pOuter;					// C_TFPlayer or CTFPlayer (client/server).

#ifdef GAME_DLL
	// Healer handling
	struct healers_t
	{
		EHANDLE	pHealer;
		float	flAmount;
		float   flHealAccum;
		float	flOverhealBonus;
		float	flOverhealDecayMult;
		bool	bDispenserHeal;
		EHANDLE pHealScorer;
		int		iKillsWhileBeingHealed; // for engineer achievement ACHIEVEMENT_TF_ENGINEER_TANK_DAMAGE
		float	flHealedLastSecond;
	};
	CUtlVector< healers_t >	m_aHealers;	
	float					m_flHealFraction;	// Store fractional health amounts
	float					m_flDisguiseHealFraction;	// Same for disguised healing
	float					m_flBestOverhealDecayMult;
	float					m_flHealedPerSecondTimer;

	float m_flChargeEffectOffTime;
	bool  m_bChargeSoundEffectsOn;

#endif

	CNetworkVar( bool, m_bLastDisguisedAsOwnTeam );

#ifdef GAME_DLL
	// Burn handling
	CHandle<CTFPlayer>		m_hBurnAttacker;
	CHandle<CTFPlayer>		m_hOriginalBurnAttacker;		// Player who originally ignited this target
	CHandle<CTFWeaponBase>	m_hBurnWeapon;
	float					m_flFlameBurnTime;
	float					m_flAfterburnDuration;
#endif // GAME_DLL

	CNetworkVar( int, m_iTauntConcept );

	float					m_flDisguiseCompleteTime;

#ifdef CLIENT_DLL
	bool m_bSyncingConditions;
#endif

	// conditions
	int	m_nOldConditions;
	int m_nOldConditionsEx;
	int m_nOldConditionsEx2;
	int m_nOldConditionsEx3;
	int m_nOldConditionsEx4;

	int	m_nForceConditions;
	int m_nForceConditionsEx;
	int m_nForceConditionsEx2;
	int m_nForceConditionsEx3;
	int m_nForceConditionsEx4;

	int	m_nOldDisguiseClass;
	int	m_nOldDisguiseTeam;

	CNetworkVar( int, m_iDesiredPlayerClass );

	float m_flNextBurningSound;

	CNetworkVar( float, m_flCloakMeter );	// [0,100]

	// Movement.
	CNetworkVar( bool, m_bJumping );
	CNetworkVar( int,  m_iAirDash );
	CNetworkVar( int, m_nAirDucked );
	CNetworkVar( float, m_flDuckTimer );

	CNetworkVar( float, m_flStealthNoAttackExpire );
	CNetworkVar( float, m_flStealthNextChangeTime );

	CNetworkVar( int, m_iCritMult );

	CNetworkArray( bool, m_bPlayerDominated, MAX_PLAYERS_ARRAY_SAFE );		// array of state per other player whether player is dominating other players
	CNetworkArray( bool, m_bPlayerDominatingMe, MAX_PLAYERS_ARRAY_SAFE );	// array of state per other player whether other players are dominating this player

	CHandle<CTFPlayer>	m_hAssist;

	CNetworkVar( int, m_nArenaNumChanges );			// number of times a player has re-rolled their class

	CNetworkVar( int, m_iWeaponKnockbackID );

	CNetworkVar( int,	m_iItemFindBonus );

	int m_nLastDuckStreakIncrement;

	int m_iOldKillStreak;
	int m_iOldKillStreakWepSlot;

	CNetworkVar( int,  m_iDisguiseBody );

	CNetworkVar( int,  m_iSpawnRoomTouchCount );

public:
	CNetworkVar( float, m_flFirstPrimaryAttack );

	CNetworkVar( float, m_flSpyTranqBuffDuration );

private:

#ifdef GAME_DLL
	float	m_flNextCritUpdate;
	CUtlVector<CTFDamageEvent> m_DamageEvents;

#else
	float	m_flGotTeleEffectAt;
#endif

public:
	bool m_bScattergunJump;

	float	m_flInvulnerabilityRemoveTime;

	float	m_flLastNoMovementTime;

private:
	
	CNetworkHandle( CBaseCombatWeapon, m_hSwitchTo );
};

extern const char *g_pszBDayGibs[22];

class CTraceFilterIgnoreTeammatesAndTeamObjects : public CTraceFilterSimple
{
public:
	DECLARE_CLASS( CTraceFilterIgnoreTeammatesAndTeamObjects, CTraceFilterSimple );

	CTraceFilterIgnoreTeammatesAndTeamObjects( const IHandleEntity *passentity, int collisionGroup, int iIgnoreTeam )
		: CTraceFilterSimple( passentity, collisionGroup ), m_iIgnoreTeam( iIgnoreTeam ) {}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask );

private:
	int m_iIgnoreTeam;
};

class CTFPlayerSharedUtils
{
public:
	static CTFWeaponBuilder *GetBuilderForObjectType( CTFPlayer *pTFPlayer, int iObjectType );
};

class CTargetOnlyFilter : public CTraceFilterSimple
{
public:
	CTargetOnlyFilter( CBaseEntity *pShooter, CBaseEntity *pTarget );
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );
	CBaseEntity	*m_pShooter;
	CBaseEntity	*m_pTarget;
};

#endif // TF_PLAYER_SHARED_H

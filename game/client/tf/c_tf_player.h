//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef C_TF_PLAYER_H
#define C_TF_PLAYER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_playeranimstate.h"
#include "c_baseplayer.h"
#include "tf_shareddefs.h"
#include "baseparticleentity.h"
#include "tf_player_shared.h"
#include "c_tf_playerclass.h"
#include "tf_item.h"
#include "props_shared.h"
#include "hintsystem.h"
#include "c_playerattachedmodel.h"
#include "c_playerrelativemodel.h"
#include "iinput.h"
#include "GameEventListener.h"
#include "c_te_legacytempents.h"


class C_MuzzleFlashModel;
class C_BaseObject;
class C_TFRagdoll;
class C_CaptureZone;
class C_MerasmusBombEffect;

extern ConVar tf_medigun_autoheal;
extern ConVar cl_autorezoom;

enum EBonusEffectFilter_t
{
	kEffectFilter_AttackerOnly,
	kEffectFilter_AttackerTeam,
	kEffectFilter_VictimOnly,
	kEffectFilter_VictimTeam,
	kEffectFilter_AttackerAndVictimOnly,
	kEffectFilter_BothTeams,
};

struct BonusEffect_t
{
	const char* m_pszSoundName;
	const char* m_pszParticle;
	ParticleAttachment_t m_eAttachment;
	const char* m_pszAttachmentName;
	EBonusEffectFilter_t m_eParticleFilter;
	EBonusEffectFilter_t m_eSoundFilter;
	bool m_bPlaySoundInAttackersEars;
	bool m_bLargeCombatText;
};

extern BonusEffect_t g_BonusEffects[ kBonusEffect_Count ];

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_TFPlayer : public C_BasePlayer
{
public:

	DECLARE_CLASS( C_TFPlayer, C_BasePlayer );
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();

	C_TFPlayer();
	~C_TFPlayer();

	virtual void Spawn();

	static C_TFPlayer* GetLocalTFPlayer();

	virtual void UpdateOnRemove( void );

	virtual const QAngle& GetRenderAngles();
	virtual void UpdateClientSideAnimation();
	virtual void SetDormant( bool bDormant );
	virtual void OnPreDataChanged( DataUpdateType_t updateType );
	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual void ProcessMuzzleFlashEvent();
	virtual void ValidateModelIndex( void );

	virtual Vector GetObserverCamOrigin( void );
	virtual int DrawModel( int flags );

	virtual void ApplyBoneMatrixTransform( matrix3x4_t& transform );
	virtual void BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed );

	virtual bool CreateMove( float flInputSampleTime, CUserCmd *pCmd ) OVERRIDE;
	void CreateVehicleMove( float flInputSampleTime, CUserCmd *pCmd );

	virtual bool		IsAllowedToSwitchWeapons( void );

	void    StopViewModelParticles( C_BaseEntity *pParticleEnt );

	virtual void ClientThink();

	// Deal with recording
	virtual void GetToolRecordingState( KeyValues *msg );

	CTFWeaponBase *GetActiveTFWeapon( void ) const;
	int GetPassiveWeapons( CUtlVector<CTFWeaponBase*>& vecOut );

	virtual void Simulate( void );
	virtual void FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options ) OVERRIDE;
	virtual void UpdateStepSound( surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity ) OVERRIDE;
	virtual void PlayStepSound( Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force );

	void FireBullet( CTFWeaponBase *pWpn, const FireBulletsInfo_t &info, bool bDoEffects, int nDamageType, int nCustomDamageType = TF_DMG_CUSTOM_NONE );

	void ImpactWaterTrace( trace_t &trace, const Vector &vecStart );

	bool CanAttack( int iCanAttackFlags = 0 );
	bool CanJump() const;
	bool CanDuck() const;

	const C_TFPlayerClass *GetPlayerClass( void ) const	{ return &m_PlayerClass; }
	C_TFPlayerClass *GetPlayerClass( void )				{ return &m_PlayerClass; }
	bool IsPlayerClass( int iClass ) const;
	virtual int GetMaxHealth( void ) const;
	int			GetMaxHealthForBuffing()  const;

	virtual int GetRenderTeamNumber( void );

	bool IsWeaponLowered( void );

	void	AvoidPlayers( CUserCmd *pCmd );

	// Get the ID target entity index. The ID target is the player that is behind our crosshairs, used to
	// display the player's name.
	void UpdateIDTarget();
	int GetIDTarget() const;
	void SetForcedIDTarget( int iTarget );

	void SetAnimation( PLAYER_ANIM playerAnim );

	virtual float GetMinFOV() const;

	virtual const QAngle& EyeAngles();

	bool	ShouldDrawSpyAsDisguised();
	virtual int GetBody( void );

	int GetBuildResources( void );

	// MATTTODO: object selection if necessary
	void SetSelectedObject( C_BaseObject *pObject ) {}

	void GetTeamColor( Color &color );
	bool InSameDisguisedTeam( CBaseEntity *pEnt );

	void ForceTempForceDraw( bool bThirdPerson );

	void FlushAllPlayerVisibilityState();

	virtual void ComputeFxBlend( void );

	// Taunts/VCDs
	virtual bool	StartSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, C_BaseEntity *pTarget );
	virtual	bool	ClearSceneEvent( CSceneEventInfo *info, bool fastKill, bool canceled );
	virtual void	CalcView( Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov );
	bool			StartGestureSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget );
	bool			StopGestureSceneEvent( CSceneEventInfo *info, bool fastKill, bool canceled );
	void			TurnOnTauntCam( void );
	void			TurnOnTauntCam_Finish( void );
	void			TurnOffTauntCam( void );
	void			TurnOffTauntCam_Finish( void );
	bool			IsTaunting( void ) const { return m_Shared.InCond( TF_COND_TAUNTING ); }

	bool			IsRegenerating( void ) const { return m_bRegenerating; }

	virtual void	InitPhonemeMappings();

	// Gibs.
	void InitPlayerGibs( void );
	void CheckAndUpdateGibType( void );
	void CreatePlayerGibs( const Vector &vecOrigin, const Vector &vecVelocity, float flImpactScale, bool bBurning, bool bWearableGibs=false, bool bOnlyHead=false, bool bDisguiseGibs=false );
	void DropPartyHat( breakablepropparams_t &breakParams, Vector &vecBreakVelocity );

	int	GetObjectCount( void );
	C_BaseObject *GetObject( int index );
	C_BaseObject *GetObjectOfType( int iObjectType, int iObjectMode=0 ) const;
	int GetNumObjects( int iObjectType, int iObjectMode=0 );

	virtual bool ShouldCollide( int collisionGroup, int contentsMask ) const;

	float GetPercentInvisible( void );
	float GetEffectiveInvisibilityLevel( void );	// takes viewer into account
	virtual bool IsTransparent( void ) OVERRIDE { return GetPercentInvisible() > 0.f; }

	virtual void AddDecal( const Vector& rayStart, const Vector& rayEnd,
		const Vector& decalCenter, int hitbox, int decalIndex, bool doTrace, trace_t& tr, int maxLODToDecal = ADDDECAL_TO_ALL_LODS );

	virtual void CalcDeathCamView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov);
	virtual Vector GetChaseCamViewOffset( CBaseEntity *target );
	virtual Vector GetDeathViewPosition();

	void ClientPlayerRespawn( void );

	virtual bool	ShouldDraw();

	void CreateSaveMeEffect( MedicCallerType nType = CALLER_TYPE_NORMAL );
	void StopSaveMeEffect( bool bForceRemoveInstantly = false );

	bool IsAllowedToTaunt( void );
	
	virtual bool	IsOverridingViewmodel( void );
	virtual int		DrawOverriddenViewmodel( C_BaseViewModel *pViewmodel, int flags );

	void			SetHealer( C_TFPlayer *pHealer, float flChargeLevel );
	void			SetWasHealedByLocalPlayer( bool bState )	{ m_bWasHealedByLocalPlayer = bState; }
	void			GetHealer( C_TFPlayer **pHealer, float *flChargeLevel ) { *pHealer = m_hHealer; *flChargeLevel = m_flHealerChargeLevel; }
	bool			GetWasHealedByLocalPlayer() { return m_bWasHealedByLocalPlayer; }
	float			MedicGetChargeLevel( CTFWeaponBase **pRetMedigun = NULL );
	bool			MedicIsReleasingCharge( void );
	CBaseEntity		*MedicGetHealTarget( void );

	void			StartBurningSound( void );
	void			StopBurningSound( void );

	void			UpdateSpyStateChange( void );

	void			UpdateRecentlyTeleportedEffect( void );
	void			UpdateOverhealEffect( void );
	void			CreateOverhealEffect( int iTeam );

	bool			CanShowClassMenu( void );
	bool			CanShowTeamMenu( void );

	void			InitializePoseParams( void );
	void			UpdateLookAt( void );

	bool			IsEnemyPlayer( void );
	void			ShowNemesisIcon( bool bShow );

	void			ShowBirthdayEffect( bool bShow );

	CUtlVector<EHANDLE>		*GetSpawnedGibs( void ) { return &m_hSpawnedGibs; }

	Vector			GetClassEyeHeight( void );

	void			ForceUpdateObjectHudState( void );

	bool			GetMedigunAutoHeal( void ){ return tf_medigun_autoheal.GetBool(); }
	bool			ShouldAutoRezoom( void ){ return cl_autorezoom.GetBool(); }

	void			GetTargetIDDataString( bool bIsDisguised, OUT_Z_BYTECAP(iMaxLenInBytes) wchar_t *sDataString, int iMaxLenInBytes, bool &bIsAmmoData, bool &bIsKillStreakData );

	void			RemoveDisguise( void );
	bool			CanDisguise( void );

	virtual void OnAchievementAchieved( int iAchievement );

	virtual void OverrideView( CViewSetup *pSetup );

	bool			CanAirDash( void ) const;

	bool			CanUseFirstPersonCommand( void );

	bool			IsEffectRateLimited( EBonusEffectFilter_t effect, const C_TFPlayer* pAttacker ) const;
	bool			ShouldPlayEffect( EBonusEffectFilter_t filter, const C_TFPlayer* pAttacker, const C_TFPlayer* pVictim ) const;
	virtual void	FireGameEvent( IGameEvent *event );

	virtual const char* ModifyEventParticles( const char* token );

	// Set the distances the camera should use. 
	void			SetTauntCameraTargets( float back, float up );

	virtual const Vector&	GetRenderOrigin( void );

public:
	// Shared functions
	bool			CanPlayerMove() const;
	float			TeamFortress_CalculateMaxSpeed( bool bIgnoreSpecialAbility = false ) const;
	void			TeamFortress_SetSpeed();
	bool			HasItem( void ) const;				// Currently can have only one item at a time.
	void			SetItem( C_TFItem *pItem );
	C_TFItem		*GetItem( void ) const;
	bool			HasTheFlag( ETFFlagType exceptionTypes[] = NULL, int nNumExceptions = 0 ) const;
	float			GetCritMult( void ) { return m_Shared.GetCritMult(); }

	virtual void	ItemPostFrame( void );

	void			SetOffHandWeapon( CTFWeaponBase *pWeapon );
	void			HolsterOffHandWeapon( void );
	CTFWeaponBase*	GetOffHandWeapon( void ) { return m_hOffHandWeapon; }

	virtual int GetSkin();

	virtual bool		Weapon_CanSwitchTo( CBaseCombatWeapon *pWeapon );

	virtual bool		Weapon_ShouldSetLast( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon ) OVERRIDE;
	virtual	bool		Weapon_Switch( C_BaseCombatWeapon *pWeapon, int viewmodelindex = 0 ) OVERRIDE;
	virtual void 		SelectItem( const char *pstr, int iSubType = 0 ) OVERRIDE;

	CTFWeaponBase		*Weapon_OwnsThisID( int iWeaponID ) const;
	CTFWeaponBase		*Weapon_GetWeaponByType( int iType );

	virtual void		GetStepSoundVelocities( float *velwalk, float *velrun );
	virtual void		SetStepSoundTime( stepsoundtimes_t iStepSoundTime, bool bWalking );

	virtual void		ThirdPersonSwitch( bool bThirdperson );

	bool	DoClassSpecialSkill( void );
	bool	EndClassSpecialSkill( void );
	bool	CanGoInvisible( bool bAllowWhileCarryingFlag = false );
	int		GetMaxAmmo( int iAmmoIndex, int iClassIndex = -1 );

	bool	CanStartPhase( void );

	void	StartBuildingObjectOfType( int iType, int iObjectMode=0 );

	C_CaptureZone *GetCaptureZoneStandingOn( void );
	C_CaptureZone *GetClosestCaptureZone( void );

	float			GetMetersRan( void )	{ return m_fMetersRan; }
	void			SetMetersRan( float fMeters, int iFrame );

	void			SetBodygroupsDirty( void );
	void			RecalcBodygroupsIfDirty( void );

	bool			CanMoveDuringTaunt();
	bool			ShouldStopTaunting();
	float			GetTauntYaw( void )				{ return m_flTauntYaw; }
	float			GetPrevTauntYaw( void )		{ return m_flPrevTauntYaw; }
	void			SetTauntYaw( float flTauntYaw );
	void			PlayTauntSoundLoop( const char *pszSoundLoopName );
	void			StopTauntSoundLoop();

	float			GetHeadScale() const { return m_flHeadScale; }
	float			GetTorsoScale() const { return m_flTorsoScale; }
	float			GetHandScale() const { return m_flHandScale; }
	static void AdjustSkinIndexForZombie( int iClassIndex, int &iSkinIndex );

	// Ragdolls.
	virtual C_BaseAnimating *BecomeRagdollOnClient();
	virtual IRagdoll		*GetRepresentativeRagdoll() const;
	EHANDLE	m_hRagdoll;
	Vector m_vecRagdollVelocity;

	// Objects
	int CanBuild( int iObjectType, int iObjectMode=0 );
	CUtlVector< CHandle<C_BaseObject> > m_aObjects;

	virtual CStudioHdr *OnNewModel( void );

	void				DisplaysHintsForTarget( C_BaseEntity *pTarget );

	// Shadows
	virtual ShadowType_t ShadowCastType( void ) ;
	virtual void GetShadowRenderBounds( Vector &mins, Vector &maxs, ShadowType_t shadowType );
	virtual void GetRenderBounds( Vector& theMins, Vector& theMaxs );
	virtual bool GetShadowCastDirection( Vector *pDirection, ShadowType_t shadowType ) const;

	CMaterialReference *GetInvulnMaterialRef( void ) { return &m_InvulnerableMaterial; }
	bool IsNemesisOfLocalPlayer();
	bool ShouldShowNemesisIcon();

	virtual	IMaterial *GetHeadLabelMaterial( void );

	// Spy Cigarette
	bool CanLightCigarette( void );

	// Bounty Mode
	int	 GetExperienceLevel( void ) { return m_nExperienceLevel; }

	// Matchmaking
	bool	GetMatchSafeToLeave() { return m_bMatchSafeToLeave; }


	bool	IsUsingVRHeadset( void ){ return m_bUsingVRHeadset; }

	bool	ShouldPlayerDrawParticles( void );

	bool	IsPlayerOnSteamFriendsList( C_BasePlayer *pPlayer );

protected:

	void ResetFlexWeights( CStudioHdr *pStudioHdr );

	virtual void CalcInEyeCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );

private:

	void HandleTaunting( void );
	void TauntCamInterpolation( void );

	void OnPlayerClassChange( void );
	void UpdatePartyHat( void );

	void InitInvulnerableMaterial( void );

	bool				m_bWasTaunting;
	bool				m_bTauntInterpolating;
	CameraThirdData_t	m_TauntCameraData;
	float				m_flTauntCamCurrentDist;
	float				m_flTauntCamTargetDist;
	float				m_flTauntCamCurrentDistUp;
	float				m_flTauntCamTargetDistUp;

	QAngle				m_angTauntPredViewAngles;
	QAngle				m_angTauntEngViewAngles;

	CSoundPatch			*m_pTauntSoundLoop;

	C_TFPlayerClass		m_PlayerClass;

	// ID Target
	int					m_iIDEntIndex;
	int					m_iForcedIDTarget;

	CNewParticleEffect	*m_pTeleporterEffect;
	bool				m_bToolRecordingVisibility;

	int					m_iOldSpawnCounter;

	// Healer
	CHandle<C_TFPlayer>	m_hHealer;
	bool				m_bWasHealedByLocalPlayer;
	float				m_flHealerChargeLevel;
	int					m_iOldHealth;
	int					m_nOldMaxHealth;

	float				m_fMetersRan;
	int					m_iLastRanFrame;

	HPARTICLEFFECT		m_pEyeEffect;

	bool				m_bOldCustomModelVisible;

	CHandle< C_BaseCombatWeapon > m_hOldActiveWeapon;

	// Look At
	/*
	int m_headYawPoseParam;
	int m_headPitchPoseParam;
	float m_headYawMin;
	float m_headYawMax;
	float m_headPitchMin;
	float m_headPitchMax;
	float m_flLastBodyYaw;
	float m_flCurrentHeadYaw;
	float m_flCurrentHeadPitch;
	*/

	// Spy cigarette smoke
	bool m_bCigaretteSmokeActive;

	// Medic callout particle effect
	CNewParticleEffect	*m_pSaveMeEffect;

	bool m_bUpdateObjectHudState;
	bool	m_bBodygroupsDirty;

public:

	CTFPlayerShared m_Shared;
	friend class CTFPlayerShared;

// Called by shared code.
public:
	float GetClassChangeTime() const { return m_flChangeClassTime; }

	void DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0 );
	bool PlayAnimEventInPrediction( PlayerAnimEvent_t event );

	const QAngle& GetNetworkEyeAngles() const { return m_angEyeAngles; }

	void CleanUpAnimationOnSpawn();
	CTFPlayerAnimState *m_PlayerAnimState;

	CInterpolatedVar< QAngle >	m_iv_angEyeAngles;

	CNetworkHandle( C_TFItem, m_hItem );

	CNetworkHandle( C_TFWeaponBase, m_hOffHandWeapon );

	int				m_iOldPlayerClass;	// Used to detect player class changes
	bool			m_bIsDisplayingNemesisIcon;
	bool			m_bIsDisplayingTranqMark;
	bool			m_bShouldShowBirthdayEffect;

	int				m_iSpawnCounter;

	bool			m_bFlipViewModels;

	bool			m_bSaveMeParity;
	bool			m_bOldSaveMeParity;

private:
	QAngle			m_angEyeAngles;

	int				m_nForceTauntCam;
	float			m_flTauntYaw;
	float			m_flPrevTauntYaw;

	int				m_nTauntSequence;
	float			m_flTauntStartTime;
	float			m_flTauntDuration;

public:

	int				m_nOldWaterLevel;
	float			m_flWaterEntryTime;
	bool			m_bWaterExitEffectActive;

	bool			m_bDuckJumpInterp;
	float			m_flFirstDuckJumpInterp;
	float			m_flLastDuckJumpInterp;
	float			m_flDuckJumpInterp;

	CMaterialReference	m_InvulnerableMaterial;


	// Burning
	CSoundPatch			*m_pBurningSound;
	HPARTICLEFFECT      m_pBurningEffect;
	float				m_flBurnEffectStartTime;

	// Urine
	HPARTICLEFFECT		m_pUrineEffect;

	// Milk
	HPARTICLEFFECT		m_pMilkEffect;

	// Gas
	HPARTICLEFFECT		m_pGasEffect;

	// Soldier Buff
	HPARTICLEFFECT		m_pSoldierOffensiveBuffEffect;
	HPARTICLEFFECT		m_pSoldierDefensiveBuffEffect;
	HPARTICLEFFECT		m_pSoldierOffensiveHealthRegenBuffEffect;
	HPARTICLEFFECT		m_pSoldierNoHealingDamageBuffEffect;

	// Speed boost
	HPARTICLEFFECT		m_pSpeedBoostEffect;

	// Taunt effects
	HPARTICLEFFECT		m_pTauntEffect;

	// Temp HACK for crit boost
	HPARTICLEFFECT m_pCritBoostEffect;

	HPARTICLEFFECT m_pOverHealedEffect;
	HPARTICLEFFECT m_pPhaseStandingEffect;

	CNewParticleEffect	*m_pDisguisingEffect;
	float m_flDisguiseEffectStartTime;
	float m_flDisguiseEndEffectStartTime;

	EHANDLE					m_hFirstGib;
	EHANDLE					m_hHeadGib;
	CUtlVector<EHANDLE>		m_hSpawnedGibs;

	int				m_iOldTeam;
	int				m_iOldClass;
	int				m_iOldDisguiseTeam;
	int				m_iOldDisguiseClass;
	int				m_iOldObserverMode;
	EHANDLE			m_hOldObserverTarget;

	bool			m_bDisguised;
	int				m_iPreviousMetal;

	int GetNumActivePipebombs( void );

	int				m_iSpyMaskBodygroup;
	Vector			m_vecCustomModelOrigin;

	// Achievements
	float m_flSaveMeExpireTime;

	//CountdownTimer m_LeaveServerTimer;

public:

	bool		CanDisplayAllSeeEffect( EAttackBonusEffects_t effect ) const;
	void		SetNextAllSeeEffectTime( EAttackBonusEffects_t effect, float flTime );

private:
	float			m_flChangeClassTime;

	float m_flWaterImpactTime;
//	RTime32 m_rtSpottedInPVSTime;
//	RTime32 m_rtJoinedSpectatorTeam;
//	RTime32 m_rtJoinedNormalTeam;

	// Gibs.
	CUtlVector< int > m_aSillyGibs;
	CUtlVector< char* > m_aNormalGibs;
	CUtlVector<breakmodel_t>	m_aGibs;

	C_TFPlayer( const C_TFPlayer & );

	// Bounty Mode
	int m_nExperienceLevel;
	int m_nExperienceLevelProgress;
	int m_nPrevExperienceLevel;

	// Matchmaking
	// is this player bound to the match on penalty of abandon. Sync'd via local-player-only DT
	bool m_bMatchSafeToLeave;

	// Medic healtarget active weapon ammo/clip count
	uint16	m_nActiveWpnClip;

	CNetworkVar( float, m_flHeadScale );
	CNetworkVar( float, m_flTorsoScale );
	CNetworkVar( float, m_flHandScale );

	// Allseecrit throttle - other clients ask us if we can be the source of another particle+sound
	float	m_flNextMiniCritEffectTime[ kBonusEffect_Count ];

	CNetworkVar( bool, m_bUsingVRHeadset );

	CNetworkVar( bool, m_bForcedSkin );
	CNetworkVar( int, m_nForcedSkin );

public:
	void SetShowHudMenuTauntSelection( bool bShow ) { m_bShowHudMenuTauntSelection = bShow; }
	bool ShouldShowHudMenuTauntSelection() const { return m_bShowHudMenuTauntSelection; }

private:
	bool m_bShowHudMenuTauntSelection;

public:

	bool IsUsingActionSlot() const { return m_bUsingActionSlot; }
	void SetUsingActionSlot( bool bUsingActionSlot ) { m_bUsingActionSlot = bUsingActionSlot; }
	
	void SetSecondaryLastWeapon( CBaseCombatWeapon *pSecondaryLastWeapon ) { m_hSecondaryLastWeapon = pSecondaryLastWeapon; }
	CBaseCombatWeapon* GetSecondaryLastWeapon() const { return m_hSecondaryLastWeapon; }

	void SetHelpmeButtonPressedTime( float flPressTime ) { m_flHelpmeButtonPressTime = flPressTime; }
	bool IsHelpmeButtonPressed() const;

	bool AddOverheadEffect( const char *pszEffectName );
	void RemoveOverheadEffect( const char *pszEffectName, bool bRemoveInstantly );
	void UpdateOverheadEffects();
	Vector GetOverheadEffectPosition();

private:
	CNetworkHandle( CBaseCombatWeapon, m_hSecondaryLastWeapon );
	CNetworkVar( bool, m_bUsingActionSlot );
	CNetworkVar( float, m_flHelpmeButtonPressTime );
	CNetworkVar( bool, m_bRegenerating );

	CUtlMap< const char *, HPARTICLEFFECT > m_mapOverheadEffects;
	float m_flOverheadEffectStartTime;

	int m_nTempForceDrawViewModelSequence = -1;
	int m_nTempForceDrawViewModelSkin = 0;
	float m_flTempForceDrawViewModelCycle  = 0.0f;
};

inline C_TFPlayer* ToTFPlayer( C_BaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	Assert( dynamic_cast<C_TFPlayer*>( pEntity ) != 0 );
	return static_cast< C_TFPlayer* >( pEntity );
}

void SetAppropriateCamera( C_TFPlayer *pPlayer );

class C_TFRagdoll : public C_BaseFlex
{
public:

	DECLARE_CLASS( C_TFRagdoll, C_BaseFlex );
	DECLARE_CLIENTCLASS();

	C_TFRagdoll();
	~C_TFRagdoll();

	virtual void OnDataChanged( DataUpdateType_t type );

	IRagdoll* GetIRagdoll() const;

	void ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName );

	void ClientThink( void );

	// Deal with recording
	virtual void GetToolRecordingState( KeyValues *msg );

	void StartFadeOut( float fDelay );
	void EndFadeOut();

	C_TFPlayer *GetPlayer( void ) const { return m_hPlayer; }

	bool IsRagdollVisible();
	float GetBurnStartTime() { return m_flBurnEffectStartTime; }

	virtual void BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed );

	virtual void SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights );

	bool IsDeathAnim() { return m_bDeathAnim; }

	int GetDamageCustom() { return m_iDamageCustom; }

	int GetClass() { return m_iClass; }

	float GetPercentInvisible( void ) { return m_flPercentInvisible; }

	int GetRagdollTeam( void ) { return m_iTeam; }

private:

	C_TFRagdoll( const C_TFRagdoll & ) {}

	void Interp_Copy( C_BaseAnimatingOverlay *pSourceEntity );

	void CreateTFRagdoll();
	void CreateTFGibs( bool bDestroyRagdoll = true, bool bCurrentPosition = false );

	virtual float FrameAdvance( float flInterval );

	virtual int	InternalDrawModel( int flags );

private:

	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
	CNetworkHandle( CTFPlayer, m_hPlayer );
	float m_fDeathTime;
	bool  m_bFadingOut;
	bool  m_bGib;
	bool  m_bBurning;
	bool  m_bWasDisguised;
	int	  m_iDamageCustom;
	CountdownTimer m_freezeTimer;
	CountdownTimer m_frozenTimer;
	int	  m_iTeam;
	int	  m_iClass;
	float m_flBurnEffectStartTime;	// start time of burning, or 0 if not burning
	bool  m_bRagdollOn;
	bool  m_bDeathAnim;
	bool  m_bOnGround;
	bool  m_bFixedConstraints;
	bool  m_bBaseTransform;
	float m_flPercentInvisible;

	CMaterialReference		m_MaterialOverride;

	bool  m_bCreatedWhilePlaybackSkipping;
};

#endif // C_TF_PLAYER_H

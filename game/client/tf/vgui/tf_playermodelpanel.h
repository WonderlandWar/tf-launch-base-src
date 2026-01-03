//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_PLAYERMODELPANEL_H
#define TF_PLAYERMODELPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "basemodel_panel.h"
#include "ichoreoeventcallback.h"

class CChoreoScene;

extern CMouthInfo g_ClientUIMouth;

class CTFPlayerAttachmentModel : public CDefaultClientRenderable
{
public:
	DECLARE_CLASS_SIMPLE( CTFPlayerAttachmentModel, CDefaultClientRenderable );
	
	CTFPlayerAttachmentModel();

	// IClientRenderable
	virtual const Vector&	GetRenderOrigin( void ) { return vec3_origin; }
	virtual const QAngle&	GetRenderAngles( void ) { return vec3_angle; }
	virtual bool			ShouldDraw( void ) { return false; }
	virtual bool			IsTransparent( void ) { return false;}
	virtual const matrix3x4_t &RenderableToWorldTransform() { static matrix3x4_t mat; SetIdentityMatrix( mat ); return mat; }
	virtual void			GetRenderBounds( Vector& mins, Vector& maxs );

	char m_szModelName[MAX_PATH];
};


// A model panel that knows how to imitate a TF2 player, including wielding/wearing unlockable items.
class CTFPlayerModelPanel : public CBaseModelPanel, public IChoreoEventCallback, public IHasLocalToGlobalFlexSettings, public IModelLoadCallback
{
	DECLARE_CLASS_SIMPLE( CTFPlayerModelPanel, CBaseModelPanel );
public:
	CTFPlayerModelPanel( vgui::Panel *pParent, const char *pName );
	~CTFPlayerModelPanel( void );

	void	ApplySettings( KeyValues *inResourceData );

	void	SetToPlayerClass( int iClass, bool bForceRefresh = false, const char *pszPlayerModelOverride = NULL );
	bool	HoldItemInSlot( int iSlot );
	bool	HoldItem( int iItemNumber );
	void	SwitchHeldItemTo( CTFPlayerAttachmentModel *pItem );
	CTFPlayerAttachmentModel	*GetHeldItem() { return m_pHeldItem; }
	CTFPlayerAttachmentModel	*GetOrCreateHeldItem();
	void ClearHeldItem() { ClearMergeMDLs(); delete m_pHeldItem; m_pHeldItem = NULL; }

	void	PlayVCD( const char *pszVCD );

	// Handle animation events
	virtual void FireEvent( const char *pszEventName, const char *pszEventOptions );

	int		GetPlayerClass() const	{ return m_iCurrentClassIndex; }

	void	SetTeam( int iTeam );
	int		GetTeam( void ) { return m_iTeam; }

	void	UpdatePreviewVisuals( void );

	// From IChoreoEventCallback
	virtual void	StartEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual void	EndEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual void	ProcessEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual bool	CheckEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual void	SetupFlexWeights( void );

	// IHasLocalToGlobalFlexSettings
	virtual void	EnsureTranslations( const flexsettinghdr_t *pSettinghdr );
	int				FlexControllerLocalToGlobal( const flexsettinghdr_t *pSettinghdr, int key );

	// IModelLoadCallback
	virtual void	OnModelLoadComplete( const model_t *pModel );

private:

	// Choreo Scene handling
	void	ClearScene( void );
	void	ProcessSequence( CChoreoScene *scene, CChoreoEvent *event );
	void	ProcessExpression( CChoreoScene *scene, CChoreoEvent *event );
	void	ProcessFlexSettingSceneEvent( CChoreoScene *scene, CChoreoEvent *event );
	void	ProcessLoop( CChoreoScene *scene, CChoreoEvent *event );
	void	AddFlexSetting( const char *expr, float scale, const flexsettinghdr_t *pSettinghdr );
	void	ProcessFlexAnimation( CChoreoScene *scene, CChoreoEvent *event );
	void	SetFlexWeight( LocalFlexController_t index, float value );
	float	GetFlexWeight( LocalFlexController_t index );

	LocalFlexController_t GetNumFlexControllers( void );
	const char *GetFlexDescFacs( int iFlexDesc );
	const char *GetFlexControllerName( LocalFlexController_t iFlexController );
	const char *GetFlexControllerType( LocalFlexController_t iFlexController );
	LocalFlexController_t		FindFlexController( const char *szName );

	// Mouth processing
	CMouthInfo&	MouthInfo() { return g_ClientUIMouth; }
	void	ProcessVisemes( Emphasized_Phoneme *classes );
	void	AddVisemesForSentence( Emphasized_Phoneme *classes, float emphasis_intensity, CSentence *sentence, float t, float dt, bool juststarted );
	void	AddViseme( Emphasized_Phoneme *classes, float emphasis_intensity, int phoneme, float scale, bool newexpression );
	bool	SetupEmphasisBlend( Emphasized_Phoneme *classes, int phoneme );
	void	ComputeBlendedSetting( Emphasized_Phoneme *classes, float emphasis_intensity );
	void	InitPhonemeMappings( void );
	void	SetupMappings( char const *pchFileRoot );

	void	HoldFirstValidItem( void );
	void	EquipItem( CTFPlayerAttachmentModel *pItem );
	bool	UpdateHeldItem( int iDesiredSlot );

	// Use this instead of SetMergeModel() - handles dynamic asset allocation
	void	LoadAndAttachAdditionalModel( const char *pMDLName, CTFPlayerAttachmentModel *pItem );
	bool	FinishAttachAdditionalModel( const model_t *pModel );
	void	RemoveAdditionalModels( void );

	int				m_iCurrentClassIndex;
	int				m_iCurrentSlotIndex;
	QAngle			m_angPlayerOrg;

	int				m_nBody;
	int				m_iTeam;

	MDLHandle_t		m_MergeMDL;

	CTFPlayerAttachmentModel	*m_pHeldItem;

	const char		*m_pszVCD;

	CChoreoScene	*m_pScene;
	float			m_flSceneTime;
	float			m_flSceneEndTime;
	float			m_flLastTickTime;

	// Choreo scenes
	bool				m_bShouldRunFlexEvents;
	float				m_flexWeight[ MAXSTUDIOFLEXCTRL ];
	Emphasized_Phoneme	m_PhonemeClasses[ NUM_PHONEME_CLASSES ];
	CUtlRBTree< FS_LocalToGlobal_t, unsigned short > m_LocalToGlobal;

	CUtlString m_strPlayerModelOverride;

	CPanelAnimationVar( bool, m_bDisableSpeakEvent, "disable_speak_event", "0" );
};

#endif // TF_PLAYERMODELPANEL_H

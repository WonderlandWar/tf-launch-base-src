//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_shareddefs.h"
#include "tf_playermodelpanel.h"
#include "tf_classdata.h"
#include "vgui/IVGui.h"
#include "animation.h"
#include "choreoscene.h"
#include "choreoevent.h"
#include "choreoactor.h"
#include "choreochannel.h"
#include "scenefilecache/ISceneFileCache.h"
#include "c_sceneentity.h"
#include "c_baseflex.h"
#include "sentence.h"
#include "engine/IEngineSound.h"
#include "c_tf_player.h"
#include "tier2/renderutils.h"
#include "bone_setup.h"
#include "matsys_controls/matsyscontrols.h"
#include "vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

DECLARE_BUILD_FACTORY( CTFPlayerModelPanel );

#define SCENE_LERP_TIME 0.1f

char g_szSceneTmpName[256];

CTFPlayerAttachmentModel::CTFPlayerAttachmentModel()
{
	memset( m_szModelName, 0, sizeof( m_szModelName ) );
}

//-----------------------------------------------------------------------------
void CTFPlayerAttachmentModel::GetRenderBounds( Vector& mins, Vector& maxs ) 
{
	if ( !m_szModelName[0] )
		return;

	int iIndex = modelinfo->GetModelIndex( m_szModelName );

	if ( iIndex == -1 )
	{
		// hard load the model to get its bounds
		MDLHandle_t hMDL = g_pMDLCache->FindMDL( m_szModelName );
		if ( g_pMDLCache->IsErrorModel( hMDL ) )
			return;

		const studiohdr_t * pStudioHdr = g_pMDLCache->GetStudioHdr( hMDL );
		VectorCopy( pStudioHdr->hull_min, mins );
		VectorCopy( pStudioHdr->hull_max, maxs );

		g_pMDLCache->Release( hMDL );
	}
	else
	{
		const model_t *pModel = modelinfo->GetModel( iIndex );
		modelinfo->GetModelRenderBounds( pModel, mins, maxs );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayerModelPanel::CTFPlayerModelPanel( vgui::Panel *pParent, const char *pName ) : BaseClass( pParent, pName ),
	m_LocalToGlobal( 0, 0, FlexSettingLessFunc )
{
	m_iCurrentClassIndex = TF_CLASS_UNDEFINED;
	m_iCurrentSlotIndex = -1;

	m_nBody = 0;
	m_pHeldItem = NULL;
	m_iTeam = TF_TEAM_RED;
	m_pszVCD = NULL;

	InitPhonemeMappings();

	m_pScene = NULL;
	ClearScene();
	memset( m_flexWeight, 0, sizeof( m_flexWeight ) );

	SetIgnoreDoubleClick( true );

	m_strPlayerModelOverride = "";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayerModelPanel::~CTFPlayerModelPanel( void )
{
	if ( m_pHeldItem )
	{
		delete m_pHeldItem;
		m_pHeldItem = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	m_angPlayerOrg = m_angPlayer;

	static ConVarRef cl_hud_minmode( "cl_hud_minmode", true );
	if ( cl_hud_minmode.IsValid() && cl_hud_minmode.GetBool() )
	{
		inResourceData->ProcessResolutionKeys( "_minmode" );
	}

	// always allow particle for this panel
	m_bUseParticle = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::SetToPlayerClass( int iClass, bool bForceRefresh /*= false*/, const char *pszPlayerModelOverride /*= NULL*/ )
{
	if ( !m_strPlayerModelOverride.IsEqual_CaseInsensitive( pszPlayerModelOverride ) )
	{
		bForceRefresh = true;
		m_strPlayerModelOverride = pszPlayerModelOverride ? pszPlayerModelOverride : "";
	}

	if ( m_iCurrentClassIndex == iClass && !bForceRefresh )
		return;

	m_iCurrentClassIndex = iClass;
	ClearScene();

	if ( IsValidTFPlayerClass( m_iCurrentClassIndex ) )
	{
		if ( !m_strPlayerModelOverride.IsEmpty() )
		{
			SetMDL( m_strPlayerModelOverride.Get() );
		}
		else
		{
			TFPlayerClassData_t *pData = GetPlayerClassData( m_iCurrentClassIndex );
			SetMDL( pData->GetModelName() );
			HoldFirstValidItem();
		}

		m_angPlayer = m_angPlayerOrg;
	}
	else
	{
		SetMDL( MDLHANDLE_INVALID );
		RemoveAdditionalModels();
	}

	InitPhonemeMappings();

	SetTeam( TF_TEAM_RED );

	m_nBody = 0;
}
extern int g_iLegacyClassSelectWeaponSlots[TF_LAST_NORMAL_CLASS];
extern const char *g_pszLegacyClassSelectVCDWeapons[TF_LAST_NORMAL_CLASS];
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::HoldFirstValidItem( void )
{
	RemoveAdditionalModels();

	if ( m_iCurrentClassIndex == TF_CLASS_UNDEFINED )
		return;
	
	//UpdateHeldItem( iDesiredSlot );

	// If we didn't find a weapon to wield, we wield the class's base primary weapon
	CTFPlayerAttachmentModel *pItem = GetOrCreateHeldItem();

	if ( pItem )
	{
		SwitchHeldItemTo( pItem );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerModelPanel::HoldItemInSlot( int iSlot )
{
	if ( m_iCurrentClassIndex == TF_CLASS_UNDEFINED )
		return false;

	return UpdateHeldItem( iSlot );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerModelPanel::HoldItem( int iItemNumber )
{
	if ( m_iCurrentClassIndex == TF_CLASS_UNDEFINED )
		return false;
	
	//SwitchHeldItemTo( pItem );
	
	//UpdateHeldItem( m_iCurrentSlotIndex );

	// We were trying to stay on the current weapon, and it's not valid. Find anything.
	HoldFirstValidItem();

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerModelPanel::UpdateHeldItem( int iDesiredSlot )
{
	m_pHeldItem = NULL;

	// If we were trying to switch to a new item, and it's not valid, stick to our current
	if ( iDesiredSlot != m_iCurrentSlotIndex )
	{
		UpdateHeldItem( m_iCurrentSlotIndex );
		return false;
	}

	// We were trying to stay on the current weapon, and it's not valid. Find anything.
	HoldFirstValidItem();
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::ClearScene( void )
{
	if ( m_pScene )
	{
		delete m_pScene;
	}

	m_pScene = NULL;
	m_flSceneTime = 0;
	m_flSceneEndTime = 0;
	m_flLastTickTime = 0;
	//memset( m_flexWeight, 0, sizeof( m_flexWeight ) );
}

extern CChoreoStringPool g_ChoreoStringPool;
CChoreoScene *LoadSceneForModel( const char *filename, IChoreoEventCallback *pCallback, float *flSceneEndTime )
{
	char loadfile[ 512 ];
	V_strcpy_safe( loadfile, filename );
	V_SetExtension( loadfile, ".vcd", sizeof( loadfile ) );
	V_FixSlashes( loadfile );

	char *pBuffer = NULL;
	size_t bufsize = scenefilecache->GetSceneBufferSize( loadfile );
	if ( bufsize <= 0 )
		return NULL;

	pBuffer = new char[ bufsize ];
	if ( !scenefilecache->GetSceneData( filename, (byte *)pBuffer, bufsize ) )
	{
		delete[] pBuffer;
		return NULL;
	}

	CChoreoScene *pScene;
	if ( IsBufferBinaryVCD( pBuffer, bufsize ) )
	{
		pScene = new CChoreoScene( pCallback );
		CUtlBuffer buf( pBuffer, bufsize, CUtlBuffer::READ_ONLY );
		if ( !pScene->RestoreFromBinaryBuffer( buf, loadfile, &g_ChoreoStringPool ) )
		{
			Warning( "Unable to restore binary scene '%s'\n", loadfile );
			delete pScene;
			pScene = NULL;
		}
		else
		{
			pScene->SetPrintFunc( Scene_Printf );
			pScene->SetEventCallbackInterface( pCallback );
		}
	}
	else
	{
		g_TokenProcessor.SetBuffer( pBuffer );
		pScene = ChoreoLoadScene( loadfile, pCallback, &g_TokenProcessor, Scene_Printf );
	}

	delete[] pBuffer;

	if ( flSceneEndTime != NULL )
	{
		// find the scene length
		// The scene is as long as the end point for the last event unless one of the events is a loop
		*flSceneEndTime = 0.0f;
		bool bSetEndTime = false;
		for ( int i = 0; i < pScene->GetNumEvents(); i++ )
		{
			CChoreoEvent *pEvent = pScene->GetEvent( i );
			if ( pEvent->GetType() == CChoreoEvent::LOOP )
			{
				*flSceneEndTime = -1.0f;
				bSetEndTime = false;
				break;
			}

			if ( pEvent->GetEndTime() > *flSceneEndTime )
			{
				*flSceneEndTime = pEvent->GetEndTime();
				bSetEndTime = true;
			}
		}

		if ( bSetEndTime )
		{
			*flSceneEndTime += SCENE_LERP_TIME; // give time for lerp to idle pose
		}
	}

	return pScene;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::PlayVCD( const char *pszVCD )
{
	m_pszVCD = pszVCD;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::FireEvent( const char *pszEventName, const char *pszEventOptions )
{
	//Plat_DebugString( CFmtStr( "********* ANIM EVENT: %s\n", pszEventName ) );

	if ( V_strcmp( pszEventName, "AE_WPN_HIDE" ) == 0 )
	{
		int nWeaponIndex = GetMergeMDLIndex( static_cast<IClientRenderable*>(m_pHeldItem) );
		if ( nWeaponIndex >= 0 )
		{
			m_aMergeMDLs[nWeaponIndex].m_bDisabled = true;
		}
	}
	else if ( V_strcmp( pszEventName, "AE_WPN_UNHIDE" ) == 0 )
	{
		int nWeaponIndex = GetMergeMDLIndex( static_cast<IClientRenderable*>(m_pHeldItem) );
		if ( nWeaponIndex >= 0 )
		{
			m_aMergeMDLs[nWeaponIndex].m_bDisabled = false;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::SwitchHeldItemTo( CTFPlayerAttachmentModel *pItem )
{
	m_nBody = 0;

	ClearScene();

	m_pHeldItem = pItem;
	// force yeti model for yeti taunt item
	if ( m_pHeldItem )
	{
		SetToPlayerClass( m_iCurrentClassIndex );
	}

	// Clear out visible items, and re-equip out wearables
	RemoveAdditionalModels();

	// Then equip the held item
	EquipItem( pItem );
	m_iCurrentSlotIndex = g_iLegacyClassSelectWeaponSlots[ m_iCurrentClassIndex ];

	SetSequenceLayers( NULL, 0 );

	// See if our VCD is overridden
	if ( m_pszVCD )
	{
		// Make sure we're holding the weapon, if it's required
		bool bCanRunScene = true;

		if ( bCanRunScene )
		{
			if ( true )
			{
				// auto complete relative path for the vcd file
				V_sprintf_safe( g_szSceneTmpName, "scenes/player/%s/low/%s", g_aPlayerClassNames_NonLocalized[m_iCurrentClassIndex], m_pszVCD );
			}
			else
			{
				// m_pszVCD should be a valid relative path
				V_strcpy_safe( g_szSceneTmpName, m_pszVCD );
			}
			
			m_pScene = LoadSceneForModel( g_szSceneTmpName, this, &m_flSceneEndTime );

			return;
		}
	}

	// update poseparam
	{
		SetPoseParameterByName( "r_hand_grip", 0.f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::EquipRequiredLoadoutSlot( int iRequiredLoadoutSlot )
{
	if ( iRequiredLoadoutSlot != TF_WPN_TYPE_COUNT )
	{
		// If we didn't find a weapon in the appropriate slot, get the base item
		CTFPlayerAttachmentModel *pWeapon = GetOrCreateHeldItem();
		if ( pWeapon )
		{
			EquipItem( pWeapon );
		}
	}
}

CTFPlayerAttachmentModel *CTFPlayerModelPanel::GetOrCreateHeldItem()
{
	if ( m_pHeldItem )
		return m_pHeldItem;

	CTFWeaponInfo *pInfo = static_cast<CTFWeaponInfo*>( GetFileWeaponInfoFromHandle( LookupWeaponInfoSlot( g_pszLegacyClassSelectVCDWeapons[m_iCurrentClassIndex] ) ) );
	CTFPlayerAttachmentModel *pModel = new CTFPlayerAttachmentModel;
	V_strcpy( pModel->m_szModelName, pInfo->szWorldModel );
	return pModel;
}

static const char *s_pszDefaultAnimForWpnSlot[] =
{
	"ACT_MP_STAND_PRIMARY",			// TF_WPN_TYPE_PRIMARY
	"ACT_MP_STAND_SECONDARY",		// TF_WPN_TYPE_SECONDARY
	"ACT_MP_STAND_MELEE",			// TF_WPN_TYPE_MELEE
	NULL,							// TF_WPN_TYPE_GRENADE
	"ACT_MP_STAND_BUILDING",		// TF_WPN_TYPE_BUILDING
	"ACT_MP_STAND_PDA",				// TF_WPN_TYPE_PDA
};
COMPILE_TIME_ASSERT( ARRAYSIZE( s_pszDefaultAnimForWpnSlot ) == TF_WPN_TYPE_COUNT );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::EquipItem( CTFPlayerAttachmentModel *pItem )
{
	if ( m_iCurrentClassIndex == TF_CLASS_UNDEFINED )
		return;

	// Non wearables can modify the animation
	int iAnimSlot = g_iLegacyClassSelectWeaponSlots[m_iCurrentClassIndex];

	// Ignore items that don't want to control player animation
	if ( iAnimSlot == -2 )
		return;

	{
		MDLCACHE_CRITICAL_SECTION();

		// Get the studio header of the root model.
		if ( !m_RootMDL.m_pStudioHdr )
			return;

		CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;
		int iSequence = FindSequenceFromActivity( &studioHdr, s_pszDefaultAnimForWpnSlot[ iAnimSlot ] );
		if ( iSequence != ACT_INVALID )
		{
			SetSequence( iSequence, true );
		}
	}

	// Attach the models for the item
	const char *pszAttached = pItem->m_szModelName;

	if ( pszAttached && pszAttached[0] )
	{
		LoadAndAttachAdditionalModel( pszAttached, pItem );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::RemoveAdditionalModels( void )
{
	ClearMergeMDLs();

	// Unregister for all callbacks
	modelinfo->UnregisterModelLoadCallback( -1, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::LoadAndAttachAdditionalModel( const char *pMDLName, CTFPlayerAttachmentModel *pItem )
{
	int nModelIndex = modelinfo->GetModelIndex( pMDLName );
	
	if ( nModelIndex == -1 )
	{
		// Get the client-only dynamic model index. The auto-addref
		// of vecDynamicAssetsLoaded will actually trigger the load.
		nModelIndex = modelinfo->RegisterDynamicModel( pMDLName, true );
		// Dynamic models never fail to register in this engine.
		Assert( nModelIndex != -1 );
	}

	if ( nModelIndex == -1 )
	{
		MDLHandle_t hMDL = vgui::MDLCache()->FindMDL( pMDLName );
		Assert( hMDL != MDLHANDLE_INVALID );
		if ( hMDL != MDLHANDLE_INVALID )
		{
			// Model not loaded, not dynamic. Hard load and exit out.
			SetMergeMDL( hMDL, static_cast<IClientRenderable*>(pItem), m_iTeam == TF_TEAM_RED ? 0 : 1 );
		}
		m_MergeMDL = hMDL;
		return;
	}

	// callback triggers immediately if not dynamic
	modelinfo->RegisterModelLoadCallback( nModelIndex, this, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void SetMDLSkinForTeam( CMDL *pMDL, int iTeam )
{
	if ( !pMDL )
		return;

	pMDL->m_nSkin = iTeam == TF_TEAM_RED ? 0 : 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::OnModelLoadComplete( const model_t *pModel )
{
	CTFPlayerAttachmentModel *pItem = m_pHeldItem;

	Assert( pItem );
	if ( pItem )
	{
		MDLHandle_t hMDL = modelinfo->GetCacheHandle( pModel );
		Assert( hMDL != MDLHANDLE_INVALID );
		if ( hMDL != MDLHANDLE_INVALID )
		{
			SetMergeMDL( hMDL, static_cast<IClientRenderable*>(pItem) );

			// Set the custom skin.
			SetMDLSkinForTeam( GetMergeMDL( hMDL ), m_iTeam );
		}
	}
}

void CTFPlayerModelPanel::SetTeam( int iTeam )
{
	m_iTeam = iTeam;

	UpdatePreviewVisuals();
}

void CTFPlayerModelPanel::UpdatePreviewVisuals()
{
	// Assume skin will be chosen based only on the preview team
	int iSkin = m_iTeam == TF_TEAM_RED ? 0 : 1;

	// Set the player model skin.
	SetSkin( iSkin );

	// Set the weapon's skin.
	if ( m_MergeMDL && m_pHeldItem )
	{
		SetMDLSkinForTeam( GetMergeMDL( m_MergeMDL ), m_iTeam );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::StartEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	if ( !event || !event->GetActive() )
		return;

	CChoreoActor *actor = event->GetActor();
	if ( actor && !actor->GetActive() )
		return;

	CChoreoChannel *channel = event->GetChannel();
	if ( channel && !channel->GetActive() )
		return;

	//Msg( "Got STARTEVENT at %.2f\n", currenttime );
	//Msg( "%8.4f:  start %s\n", currenttime, event->GetDescription() );

	switch ( event->GetType() )
	{
	case CChoreoEvent::SEQUENCE:
		ProcessSequence( scene, event );
		break;

	case CChoreoEvent::SPEAK:
		{
			if ( m_bDisableSpeakEvent )
				return;

			// FIXME: dB hack.  soundlevel needs to be moved into inside of wav?
			soundlevel_t iSoundlevel = SNDLVL_TALKING;
			if ( event->GetParameters2() )
			{
				iSoundlevel = (soundlevel_t)atoi( event->GetParameters2() );
				if ( iSoundlevel == SNDLVL_NONE )
				{
					iSoundlevel = SNDLVL_TALKING;
				}
			}

			float time_in_past = currenttime - event->GetStartTime() ;
			float soundtime = gpGlobals->curtime - time_in_past;

			EmitSound_t es;
			es.m_nChannel = CHAN_VOICE;
			es.m_flVolume = 1;
			es.m_SoundLevel = iSoundlevel;
			es.m_flSoundTime = soundtime;
			es.m_bEmitCloseCaption = false;
			es.m_pSoundName = event->GetParameters();

			C_RecipientFilter filter;
			C_BaseEntity::EmitSound( filter, SOUND_FROM_UI_PANEL, es );
		}
		break;

	case CChoreoEvent::STOPPOINT:
		{
			// Nothing, this is a symbolic event for keeping the vcd alive for ramping out after the last true event
			//ClearScene();
		}
		break;

	case CChoreoEvent::LOOP:
		ProcessLoop( scene, event );
		break;

	// Not supported in TF2's model previews
	case CChoreoEvent::SUBSCENE:
	case CChoreoEvent::SECTION:
		{
			Assert(0);
		}
		break;

	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::EndEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	if ( !event || !event->GetActive() )
		return;

	CChoreoActor *actor = event->GetActor();
	if ( actor && !actor->GetActive() )
		return;

	CChoreoChannel *channel = event->GetChannel();
	if ( channel && !channel->GetActive() )
		return;

	//Msg( "Got ENDEVENT at %.2f\n", currenttime );
	//Msg( "%8.4f:  end %s %i\n", currenttime, event->GetDescription(), event->GetType() );

	switch ( event->GetType() )
	{
	case CChoreoEvent::SUBSCENE:
		{
			// Not supported in TF2's model previews
			Assert(0);
		}
		break;
	case CChoreoEvent::SPEAK:
		{
		}
		break;
	case CChoreoEvent::STOPPOINT:
		{
			//SetSequenceLayers( NULL, 0 );
			//ClearScene();
		}
		break;
	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::ProcessEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	if ( !event || !event->GetActive() )
		return;

	CChoreoActor *actor = event->GetActor();
	if ( actor && !actor->GetActive() )
		return;

	CChoreoChannel *channel = event->GetChannel();
	if ( channel && !channel->GetActive() )
		return;

	//Msg("PROCESSEVENT at %.2f\n", currenttime );

	switch( event->GetType() )
	{
	case CChoreoEvent::EXPRESSION:
		if ( !m_bShouldRunFlexEvents )
		{
			ProcessFlexSettingSceneEvent( scene, event );
		}
		break;

	case CChoreoEvent::FLEXANIMATION:
		if ( m_bShouldRunFlexEvents )
		{
			ProcessFlexAnimation( scene, event );
		}
		break;

	case CChoreoEvent::SEQUENCE:
	case CChoreoEvent::SPEAK:
	case CChoreoEvent::STOPPOINT:
		// Nothing
		break;

	// Not supported in TF2's model previews
	case CChoreoEvent::LOOKAT:
	case CChoreoEvent::FACE:
	case CChoreoEvent::SUBSCENE:
	case CChoreoEvent::MOVETO:
	case CChoreoEvent::INTERRUPT:
	case CChoreoEvent::PERMIT_RESPONSES:
	case CChoreoEvent::GESTURE:
		Assert(0);
		break;

	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerModelPanel::CheckEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Apply a sequence
// Input  : *event - 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::ProcessSequence( CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event->GetType() == CChoreoEvent::SEQUENCE );

	CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;

	if ( !event->GetActor() )
		return;

	int iSequence = LookupSequence( &studioHdr, event->GetParameters() );
	if (iSequence < 0)
		return;

	// making sure the mdl has correct playback rate
	mstudioseqdesc_t &seqdesc = studioHdr.pSeqdesc( iSequence );
	mstudioanimdesc_t &animdesc = studioHdr.pAnimdesc( studioHdr.iRelativeAnim( iSequence, seqdesc.anim(0,0) ) );
	m_RootMDL.m_MDL.m_flPlaybackRate = animdesc.fps;

	MDLSquenceLayer_t	tmpSequenceLayers[1];
	tmpSequenceLayers[0].m_nSequenceIndex = iSequence;
	tmpSequenceLayers[0].m_flWeight = 1.0;
	tmpSequenceLayers[0].m_bNoLoop = true;
	tmpSequenceLayers[0].m_flCycleBeganAt = m_RootMDL.m_MDL.m_flTime;
	SetSequenceLayers( tmpSequenceLayers, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scene - 
//			*event - 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::ProcessLoop( CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event->GetType() == CChoreoEvent::LOOP );

	float backtime = (float)atof( event->GetParameters() );

	bool process = true;
	int counter = event->GetLoopCount();
	if ( counter != -1 )
	{
		int remaining = event->GetNumLoopsRemaining();
		if ( remaining <= 0 )
		{
			process = false;
		}
		else
		{
			event->SetNumLoopsRemaining( --remaining );
		}
	}

	if ( !process )
		return;

	//Msg("LOOP: %.2f (%.2f)\n", m_flSceneTime, scene->GetTime() );

	float flPrevTime = m_flSceneTime;
	scene->LoopToTime( backtime );
	m_flSceneTime = backtime;

	//Msg("   -> %.2f (%.2f)\n", m_flSceneTime, scene->GetTime() );

	float flDelta = flPrevTime - backtime;

	//Msg("	-> Delta %.2f\n", flDelta );

	// If we're running noloop sequences, we need to push out their begin time, so they keep playing
	for ( int i = 0; i < m_nNumSequenceLayers; i++ )
	{
		if ( m_SequenceLayers[i].m_bNoLoop )
		{
			m_SequenceLayers[i].m_flCycleBeganAt += flDelta;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
LocalFlexController_t CTFPlayerModelPanel::GetNumFlexControllers( void )
{
	CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;
	return studioHdr.numflexcontrollers();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFPlayerModelPanel::GetFlexDescFacs( int iFlexDesc )
{
	CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;

	mstudioflexdesc_t *pflexdesc = studioHdr.pFlexdesc( iFlexDesc );

	return pflexdesc->pszFACS( );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFPlayerModelPanel::GetFlexControllerName( LocalFlexController_t iFlexController )
{
	CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;

	mstudioflexcontroller_t *pflexcontroller = studioHdr.pFlexcontroller( iFlexController );

	return pflexcontroller->pszName( );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFPlayerModelPanel::GetFlexControllerType( LocalFlexController_t iFlexController )
{
	CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;

	mstudioflexcontroller_t *pflexcontroller = studioHdr.pFlexcontroller( iFlexController );

	return pflexcontroller->pszType( );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
LocalFlexController_t CTFPlayerModelPanel::FindFlexController( const char *szName )
{
	for (LocalFlexController_t i = LocalFlexController_t(0); i < GetNumFlexControllers(); i++)
	{
		if (stricmp( GetFlexControllerName( i ), szName ) == 0)
		{
			return i;
		}
	}

	// AssertMsg( 0, UTIL_VarArgs( "flexcontroller %s couldn't be mapped!!!\n", szName ) );
	return LocalFlexController_t(-1);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::SetFlexWeight( LocalFlexController_t index, float value )
{
	if (index >= 0 && index < GetNumFlexControllers())
	{
		CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;

		mstudioflexcontroller_t *pflexcontroller = studioHdr.pFlexcontroller( index );

		if (pflexcontroller->max != pflexcontroller->min)
		{
			value = (value - pflexcontroller->min) / (pflexcontroller->max - pflexcontroller->min);
			value = clamp( value, 0.0f, 1.0f );
		}

		m_flexWeight[ index ] = value;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayerModelPanel::GetFlexWeight( LocalFlexController_t index )
{
	if (index >= 0 && index < GetNumFlexControllers())
	{
		CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;

		mstudioflexcontroller_t *pflexcontroller = studioHdr.pFlexcontroller( index );

		if (pflexcontroller->max != pflexcontroller->min)
		{
			return m_flexWeight[index] * (pflexcontroller->max - pflexcontroller->min) + pflexcontroller->min;
		}

		return m_flexWeight[index];
	}
	return 0.0;
}

//-----------------------------------------------------------------------------
// Purpose: During paint, apply the flex weights to the model
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::SetupFlexWeights( void )
{
	if ( m_RootMDL.m_MDL.GetMDL() == MDLHANDLE_INVALID )
		return;

	// initialize the models local to global flex controller mappings
	CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;
	if (studioHdr.pFlexcontroller( LocalFlexController_t(0) )->localToGlobal == -1)
	{
		for ( LocalFlexController_t i = LocalFlexController_t(0); i < studioHdr.numflexcontrollers(); i++)
		{
			int j = C_BaseFlex::AddGlobalFlexController( studioHdr.pFlexcontroller( i )->pszName() );
			studioHdr.pFlexcontroller( i )->localToGlobal = j;
		}
	}

	int iControllers = GetNumFlexControllers();
	for ( int j = 0; j < iControllers; j++ )
	{
		m_RootMDL.m_MDL.m_pFlexControls[j] = 0;
	}

	LocalFlexController_t i;

	// Decay to neutral
	for ( i = LocalFlexController_t(0); i < GetNumFlexControllers(); i++)
	{
		SetFlexWeight( i, GetFlexWeight( i ) * 0.95 );
	}

	// Run scene
	if ( m_pScene )
	{
		m_bShouldRunFlexEvents = true;
		m_pScene->Think( m_flSceneTime );
	}

	// get the networked flexweights and convert them from 0..1 to real dynamic range
	for (i = LocalFlexController_t(0); i < studioHdr.numflexcontrollers(); i++)
	{
		mstudioflexcontroller_t *pflex = studioHdr.pFlexcontroller( i );

		m_RootMDL.m_MDL.m_pFlexControls[pflex->localToGlobal] = m_flexWeight[i];
		// rescale
		m_RootMDL.m_MDL.m_pFlexControls[pflex->localToGlobal] = m_RootMDL.m_MDL.m_pFlexControls[pflex->localToGlobal] * (pflex->max - pflex->min) + pflex->min;
	}

	if ( m_pScene )
	{
		m_bShouldRunFlexEvents = false;
		m_pScene->Think( m_flSceneTime );
	}

	ProcessVisemes( m_PhonemeClasses );

	if ( m_pScene )
	{
		// Advance time
		if ( m_flLastTickTime < FLT_EPSILON )
		{
			m_flLastTickTime = m_RootMDL.m_MDL.m_flTime - SCENE_LERP_TIME;
		}

		m_flSceneTime += (m_RootMDL.m_MDL.m_flTime - m_flLastTickTime);
		m_flSceneTime = Max( m_flSceneTime, -SCENE_LERP_TIME );
		m_flLastTickTime = m_RootMDL.m_MDL.m_flTime;

		if ( m_flSceneEndTime > FLT_EPSILON && m_flSceneTime > m_flSceneEndTime )
		{
			char filename[MAX_PATH];
			V_strcpy_safe( filename, m_pScene->GetFilename() );

			SetSequenceLayers( NULL, 0 );
			ClearScene();

			m_pScene = LoadSceneForModel( filename, this, &m_flSceneEndTime );
		}
	}
}

extern CFlexSceneFileManager g_FlexSceneFileManager;
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::ProcessExpression( CChoreoScene *scene, CChoreoEvent *event )
{
	// Flexanimations have to have an end time!!!
	if ( !event->HasEndTime() )
		return;

	// Look up the actual strings
	const char *scenefile	= event->GetParameters();
	const char *name		= event->GetParameters2();

	// Have to find both strings
	if ( scenefile && name )
	{
		// Find the scene file
		const flexsettinghdr_t *pExpHdr = ( const flexsettinghdr_t * )g_FlexSceneFileManager.FindSceneFile( this, scenefile, true );
		if ( pExpHdr )
		{
			float scenetime = scene->GetTime();

			float flIntensity = event->GetIntensity( scenetime );

			int i;
			const flexsetting_t *pSetting = NULL;

			// Find the named setting in the base
			for ( i = 0; i < pExpHdr->numflexsettings; i++ )
			{
				pSetting = pExpHdr->pSetting( i );
				if ( !pSetting )
					continue;

				if ( !V_stricmp( pSetting->pszName(), name ) )
					break;
			}

			if ( i>=pExpHdr->numflexsettings )
				return;

			flexweight_t *pWeights = NULL;
			int truecount = pSetting->psetting( (byte *)pExpHdr, 0, &pWeights );
			if ( !pWeights )
				return;

			for (i = 0; i < truecount; i++, pWeights++)
			{
				int j = FlexControllerLocalToGlobal( pExpHdr, pWeights->key );

				float s = clamp( pWeights->influence * flIntensity, 0.0f, 1.0f );
				m_RootMDL.m_MDL.m_pFlexControls[ j ] = m_RootMDL.m_MDL.m_pFlexControls[j] * (1.0f - s) + pWeights->weight * s;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::ProcessFlexSettingSceneEvent( CChoreoScene *scene, CChoreoEvent *event )
{
	// Flexanimations have to have an end time!!!
	if ( !event->HasEndTime() )
		return;

	VPROF( "C_BaseFlex::ProcessFlexSettingSceneEvent" );

	// Look up the actual strings
	const char *scenefile	= event->GetParameters();
	const char *name		= event->GetParameters2();

	// Have to find both strings
	if ( scenefile && name )
	{
		// Find the scene file
		const flexsettinghdr_t *pExpHdr = ( const flexsettinghdr_t * )g_FlexSceneFileManager.FindSceneFile( this, scenefile, true );
		if ( pExpHdr )
		{
			float scenetime = scene->GetTime();

			float scale = event->GetIntensity( scenetime );

			// Add the named expression
			AddFlexSetting( name, scale, pExpHdr );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *expr - 
//			scale - 
//			*pSettinghdr - 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::AddFlexSetting( const char *expr, float scale, const flexsettinghdr_t *pSettinghdr )
{
	int i;
	const flexsetting_t *pSetting = NULL;

	// Find the named setting in the base
	for ( i = 0; i < pSettinghdr->numflexsettings; i++ )
	{
		pSetting = pSettinghdr->pSetting( i );
		if ( !pSetting )
			continue;

		const char *name = pSetting->pszName();

		if ( !V_stricmp( name, expr ) )
			break;
	}

	if ( i>=pSettinghdr->numflexsettings )
	{
		return;
	}

	flexweight_t *pWeights = NULL;
	int truecount = pSetting->psetting( (byte *)pSettinghdr, 0, &pWeights );
	if ( !pWeights )
		return;

	for (i = 0; i < truecount; i++, pWeights++)
	{
		// Translate to local flex controller
		// this is translating from the settings's local index to the models local index
		int index = FlexControllerLocalToGlobal( pSettinghdr, pWeights->key );

		// blend scaled weighting in to total (post networking g_flexweight!!!!)
		float s = clamp( scale * pWeights->influence, 0.0f, 1.0f );
		m_RootMDL.m_MDL.m_pFlexControls[index] = m_RootMDL.m_MDL.m_pFlexControls[index] * (1.0f - s) + pWeights->weight * s;

		for ( int iMergeMDL=0; iMergeMDL<m_aMergeMDLs.Count(); ++iMergeMDL )
		{
			m_aMergeMDLs[iMergeMDL].m_MDL.m_pFlexControls[index] = m_aMergeMDLs[iMergeMDL].m_MDL.m_pFlexControls[index] * (1.0f - s) + pWeights->weight * s;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Apply flexanimation to actor's face
// Input  : *event - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::ProcessFlexAnimation( CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event->GetType() == CChoreoEvent::FLEXANIMATION );

	CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;
	CStudioHdr *hdr = &studioHdr;
	if ( !hdr )
		return;

	if ( !event->GetTrackLookupSet() )
	{
		// Create lookup data
		for ( int i = 0; i < event->GetNumFlexAnimationTracks(); i++ )
		{
			CFlexAnimationTrack *track = event->GetFlexAnimationTrack( i );
			if ( !track )
				continue;

			if ( track->IsComboType() )
			{
				char name[ 512 ];
				Q_strncpy( name, "right_" ,sizeof(name));
				Q_strncat( name, track->GetFlexControllerName(),sizeof(name), COPY_ALL_CHARACTERS );

				track->SetFlexControllerIndex( MAX( FindFlexController( name ), LocalFlexController_t(0) ), 0, 0 );

				Q_strncpy( name, "left_" ,sizeof(name));
				Q_strncat( name, track->GetFlexControllerName(),sizeof(name), COPY_ALL_CHARACTERS );

				track->SetFlexControllerIndex( MAX( FindFlexController( name ), LocalFlexController_t(0) ), 0, 1 );
			}
			else
			{
				track->SetFlexControllerIndex( MAX( FindFlexController( (char *)track->GetFlexControllerName() ), LocalFlexController_t(0)), 0 );
			}
		}

		event->SetTrackLookupSet( true );
	}

	float scenetime = scene->GetTime();

	float weight = event->GetIntensity( scenetime );

	// Iterate animation tracks
	for ( int i = 0; i < event->GetNumFlexAnimationTracks(); i++ )
	{
		CFlexAnimationTrack *track = event->GetFlexAnimationTrack( i );
		if ( !track )
			continue;

		// Disabled
		if ( !track->IsTrackActive() )
			continue;

		// Map track flex controller to global name
		if ( track->IsComboType() )
		{
			for ( int side = 0; side < 2; side++ )
			{
				LocalFlexController_t controller = track->GetRawFlexControllerIndex( side );

				// Get spline intensity for controller
				float flIntensity = track->GetIntensity( scenetime, side );
				if ( controller >= LocalFlexController_t(0) )
				{
					float orig = GetFlexWeight( controller );
					float value = orig * (1 - weight) + flIntensity * weight;
					SetFlexWeight( controller, value );
				}
			}
		}
		else
		{
			LocalFlexController_t controller = track->GetRawFlexControllerIndex( 0 );

			// Get spline intensity for controller
			float flIntensity = track->GetIntensity( scenetime, 0 );
			if ( controller >= LocalFlexController_t(0) )
			{
				float orig = GetFlexWeight( controller );
				float value = orig * (1 - weight) + flIntensity * weight;
				SetFlexWeight( controller, value );
			}
		}
	}
}

extern ConVar g_CV_PhonemeDelay;
extern ConVar g_CV_PhonemeFilter;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::ProcessVisemes( Emphasized_Phoneme *classes )
{
	// Any sounds being played?
	if ( !MouthInfo().IsActive() )
		return;

	// Multiple phoneme tracks can overlap, look across all such tracks.
	for ( int source = 0 ; source < MouthInfo().GetNumVoiceSources(); source++ )
	{
		CVoiceData *vd = MouthInfo().GetVoiceSource( source );
		if ( !vd || vd->ShouldIgnorePhonemes() )
			continue;

		CSentence *sentence = engine->GetSentence( vd->GetSource() );
		if ( !sentence )
			continue;

		float	sentence_length = engine->GetSentenceLength( vd->GetSource() );
		float	timesincestart = vd->GetElapsedTime();

		// This sound should be done...why hasn't it been removed yet???
		if ( timesincestart >= ( sentence_length + 2.0f ) )
			continue;

		// Adjust actual time
		float t = timesincestart - g_CV_PhonemeDelay.GetFloat();

		// Get box filter duration
		float dt = g_CV_PhonemeFilter.GetFloat();

		// Streaming sounds get an additional delay...
		/*
		// Tracker 20534:  Probably not needed any more with the async sound stuff that
		//  we now have (we don't have a disk i/o hitch on startup which might have been
		//  messing up the startup timing a bit )
		bool streaming = engine->IsStreaming( vd->m_pAudioSource );
		if ( streaming )
		{
			t -= g_CV_PhonemeDelayStreaming.GetFloat();
		}
		*/

		// Assume sound has been playing for a while...
		bool juststarted = false;

		// Get intensity setting for this time (from spline)
		float emphasis_intensity = sentence->GetIntensity( t, sentence_length );

		// Blend and add visemes together
		AddVisemesForSentence( classes, emphasis_intensity, sentence, t, dt, juststarted );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::AddVisemesForSentence( Emphasized_Phoneme *classes, float emphasis_intensity, CSentence *sentence, float t, float dt, bool juststarted )
{
	int pcount = sentence->GetRuntimePhonemeCount();
	for ( int k = 0; k < pcount; k++ )
	{
		const CBasePhonemeTag *phoneme = sentence->GetRuntimePhoneme( k );

		if (t > phoneme->GetStartTime() && t < phoneme->GetEndTime())
		{
			bool bCrossfade = true;
			if (bCrossfade)
			{
				if (k < pcount-1)
				{
					const CBasePhonemeTag *next = sentence->GetRuntimePhoneme( k + 1 );
					// if I have a neighbor
					if ( next )
					{
						//  and they're touching
						if (next->GetStartTime() == phoneme->GetEndTime() )
						{
							// no gap, so increase the blend length to the end of the next phoneme, as long as it's not longer than the current phoneme
							dt = MAX( dt, MIN( next->GetEndTime() - t, phoneme->GetEndTime() - phoneme->GetStartTime() ) );
						}
						else
						{
							// dead space, so increase the blend length to the start of the next phoneme, as long as it's not longer than the current phoneme
							dt = MAX( dt, MIN( next->GetStartTime() - t, phoneme->GetEndTime() - phoneme->GetStartTime() ) );
						}
					}
					else
					{
						// last phoneme in list, increase the blend length to the length of the current phoneme
						dt = MAX( dt, phoneme->GetEndTime() - phoneme->GetStartTime() );
					}
				}
			}
		}

		float t1 = ( phoneme->GetStartTime() - t) / dt;
		float t2 = ( phoneme->GetEndTime() - t) / dt;

		if (t1 < 1.0 && t2 > 0)
		{
			float scale;

			// clamp
			if (t2 > 1)
				t2 = 1;
			if (t1 < 0)
				t1 = 0;

			// FIXME: simple box filter.  Should use something fancier
			scale = (t2 - t1);

			AddViseme( classes, emphasis_intensity, phoneme->GetPhonemeCode(), scale, juststarted );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *classes - 
//			phoneme - 
//			scale - 
//			newexpression - 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::AddViseme( Emphasized_Phoneme *classes, float emphasis_intensity, int phoneme, float scale, bool newexpression )
{
	CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;
	CStudioHdr *hdr = &studioHdr;
	if ( !hdr )
		return;

	int type;

	// Setup weights for any emphasis blends
	bool skip = SetupEmphasisBlend( classes, phoneme );

	phoneme = 230;
	scale = 1.0;

	// Uh-oh, missing or unknown phoneme???
	if ( skip )
	{
		return;
	}

	// Compute blend weights
	ComputeBlendedSetting( classes, emphasis_intensity );

	for ( type = 0; type < NUM_PHONEME_CLASSES; type++ )
	{
		Emphasized_Phoneme *info = &classes[ type ];
		if ( !info->valid || info->amount == 0.0f )
			continue;

		const flexsettinghdr_t *actual_flexsetting_header = info->base;
		const flexsetting_t *pSetting = actual_flexsetting_header->pIndexedSetting( phoneme );
		if (!pSetting)
		{
			continue;
		}

		flexweight_t *pWeights = NULL;

		int truecount = pSetting->psetting( (byte *)actual_flexsetting_header, 0, &pWeights );
		if ( pWeights )
		{
			for ( int i = 0; i < truecount; i++)
			{
				// Translate to global controller number
				int j = FlexControllerLocalToGlobal( actual_flexsetting_header, pWeights->key );
				// Add scaled weighting in
				if ( pWeights->weight > 0 )
				{
					m_RootMDL.m_MDL.m_pFlexControls[j] += info->amount * scale * pWeights->weight;
				}
				// Go to next setting
				pWeights++;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: A lot of the one time setup and also resets amount to 0.0f default
//  for strong/weak/normal tracks
// Returning true == skip this phoneme
// Input  : *classes - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFPlayerModelPanel::SetupEmphasisBlend( Emphasized_Phoneme *classes, int phoneme )
{
	int i;

	bool skip = false;

	for ( i = 0; i < NUM_PHONEME_CLASSES; i++ )
	{
		Emphasized_Phoneme *info = &classes[ i ];

		// Assume it's bogus
		info->valid = false;
		info->amount = 0.0f;

		// One time setup
		if ( !info->basechecked )
		{
			info->basechecked = true;
			info->base = (flexsettinghdr_t *)g_FlexSceneFileManager.FindSceneFile( this, info->classname, false );
		}
		info->exp = NULL;
		if ( info->base )
		{
			Assert( info->base->id == ('V' << 16) + ('F' << 8) + ('E') );
			info->exp = info->base->pIndexedSetting( phoneme );
		}

		if ( info->required && ( !info->base || !info->exp ) )
		{
			skip = true;
			break;
		}

		if ( info->exp )
		{
			info->valid = true;
		}
	}

	return skip;
}

#define STRONG_CROSSFADE_START		0.60f
#define WEAK_CROSSFADE_START		0.40f

//-----------------------------------------------------------------------------
// Purpose: 
// Here's the formula
// 0.5 is neutral 100 % of the default setting
// Crossfade starts at STRONG_CROSSFADE_START and is full at STRONG_CROSSFADE_END
// If there isn't a strong then the intensity of the underlying phoneme is fixed at 2 x STRONG_CROSSFADE_START
//  so we don't get huge numbers
// Input  : *classes - 
//			emphasis_intensity - 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::ComputeBlendedSetting( Emphasized_Phoneme *classes, float emphasis_intensity )
{
	// See which blends are available for the current phoneme
	bool has_weak	= classes[ PHONEME_CLASS_WEAK ].valid;
	bool has_strong = classes[ PHONEME_CLASS_STRONG ].valid;

	// Better have phonemes in general
	Assert( classes[ PHONEME_CLASS_NORMAL ].valid );

	if ( emphasis_intensity > STRONG_CROSSFADE_START )
	{
		if ( has_strong )
		{
			// Blend in some of strong
			float dist_remaining = 1.0f - emphasis_intensity;
			float frac = dist_remaining / ( 1.0f - STRONG_CROSSFADE_START );

			classes[ PHONEME_CLASS_NORMAL ].amount = (frac) * 2.0f * STRONG_CROSSFADE_START;
			classes[ PHONEME_CLASS_STRONG ].amount = 1.0f - frac; 
		}
		else
		{
			emphasis_intensity = MIN( emphasis_intensity, STRONG_CROSSFADE_START );
			classes[ PHONEME_CLASS_NORMAL ].amount = 2.0f * emphasis_intensity;
		}
	}
	else if ( emphasis_intensity < WEAK_CROSSFADE_START )
	{
		if ( has_weak )
		{
			// Blend in some weak
			float dist_remaining = WEAK_CROSSFADE_START - emphasis_intensity;
			float frac = dist_remaining / ( WEAK_CROSSFADE_START );

			classes[ PHONEME_CLASS_NORMAL ].amount = (1.0f - frac) * 2.0f * WEAK_CROSSFADE_START;
			classes[ PHONEME_CLASS_WEAK ].amount = frac; 
		}
		else
		{
			emphasis_intensity = MAX( emphasis_intensity, WEAK_CROSSFADE_START );
			classes[ PHONEME_CLASS_NORMAL ].amount = 2.0f * emphasis_intensity;
		}
	}
	else
	{
		// Assume 0.5 (neutral) becomes a scaling of 1.0f
		classes[ PHONEME_CLASS_NORMAL ].amount = 2.0f * emphasis_intensity;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::InitPhonemeMappings( void )
{
	CStudioHdr *pStudioHdr = m_RootMDL.m_pStudioHdr;
	if ( pStudioHdr && pStudioHdr->IsValid() )
	{
		char szBasename[MAX_PATH];
		Q_StripExtension( pStudioHdr->pszName(), szBasename, sizeof( szBasename ) );

		char szExpressionName[MAX_PATH];
		Q_snprintf( szExpressionName, sizeof( szExpressionName ), "%s/phonemes/phonemes", szBasename );
		if ( g_FlexSceneFileManager.FindSceneFile( this, szExpressionName, false ) )
		{
			SetupMappings( szExpressionName );	
			return;
		}
	}

	SetupMappings( "phonemes" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::SetupMappings( char const *pchFileRoot )
{
	// Fill in phoneme class lookup
	memset( m_PhonemeClasses, 0, sizeof( m_PhonemeClasses ) );

	Emphasized_Phoneme *normal = &m_PhonemeClasses[ PHONEME_CLASS_NORMAL ];
	Q_snprintf( normal->classname, sizeof( normal->classname ), "%s", pchFileRoot );
	normal->required = true;

	Emphasized_Phoneme *weak = &m_PhonemeClasses[ PHONEME_CLASS_WEAK ];
	Q_snprintf( weak->classname, sizeof( weak->classname ), "%s_weak", pchFileRoot );
	Emphasized_Phoneme *strong = &m_PhonemeClasses[ PHONEME_CLASS_STRONG ];
	Q_snprintf( strong->classname, sizeof( strong->classname ), "%s_strong", pchFileRoot );
}

//-----------------------------------------------------------------------------
// Purpose: Since everyone shared a pSettinghdr now, we need to set up the localtoglobal mapping per entity, but 
//  we just do this in memory with an array of integers (could be shorts, I suppose)
// Input  : *pSettinghdr - 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::EnsureTranslations( const flexsettinghdr_t *pSettinghdr )
{
	Assert( pSettinghdr );

	FS_LocalToGlobal_t entry( pSettinghdr );

	unsigned short idx = m_LocalToGlobal.Find( entry );
	if ( idx != m_LocalToGlobal.InvalidIndex() )
		return;

	entry.SetCount( pSettinghdr->numkeys );

	for ( int i = 0; i < pSettinghdr->numkeys; ++i )
	{
		entry.m_Mapping[ i ] = C_BaseFlex::AddGlobalFlexController( pSettinghdr->pLocalName( i ) );
	}

	m_LocalToGlobal.Insert( entry );
}

//-----------------------------------------------------------------------------
// Purpose: Look up instance specific mapping
// Input  : *pSettinghdr - 
//			key - 
// Output : int
//-----------------------------------------------------------------------------
int CTFPlayerModelPanel::FlexControllerLocalToGlobal( const flexsettinghdr_t *pSettinghdr, int key )
{
	FS_LocalToGlobal_t entry( pSettinghdr );

	int idx = m_LocalToGlobal.Find( entry );
	if ( idx == m_LocalToGlobal.InvalidIndex() )
	{
		// This should never happen!!!
		Assert( 0 );
		Warning( "Unable to find mapping for flexcontroller %i, settings %p on CTFPlayerModelPanel\n", key, pSettinghdr );
		EnsureTranslations( pSettinghdr );
		idx = m_LocalToGlobal.Find( entry );
		if ( idx == m_LocalToGlobal.InvalidIndex() )
		{
			Error( "CTFPlayerModelPanel::FlexControllerLocalToGlobal failed!\n" );
		}
	}

	FS_LocalToGlobal_t& result = m_LocalToGlobal[ idx ];
	// Validate lookup
	Assert( result.m_nCount != 0 && key < result.m_nCount );
	int index = result.m_Mapping[ key ];
	return index;
}


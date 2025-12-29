//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: L4D mod render targets are specified by and accessable through this singleton
//
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "tf_rendertargets.h"
#include "materialsystem/imaterialsystem.h"
#include "rendertexture.h"
#if defined( REPLAY_ENABLED )
#include "replay/replay_screenshot.h"
#endif

ConVar tf_water_resolution( "tf_water_resolution", "1024", FCVAR_NONE, "Needs to be set at game launch time to override." );
ConVar tf_monitor_resolution( "tf_monitor_resolution", "1024", FCVAR_NONE, "Needs to be set at game launch time to override." );

//-----------------------------------------------------------------------------
// Purpose: InitClientRenderTargets, interface called by the engine at material system init in the engine
// Input  : pMaterialSystem - the interface to the material system from the engine (our singleton hasn't been set up yet)
//			pHardwareConfig - the user's hardware config, useful for conditional render targets setup
//-----------------------------------------------------------------------------
void CTFRenderTargets::InitClientRenderTargets( IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig )
{
	BaseClass::InitClientRenderTargets( pMaterialSystem, pHardwareConfig, tf_water_resolution.GetInt(), tf_monitor_resolution.GetInt() );
}


static CTFRenderTargets g_TFRenderTargets;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CTFRenderTargets, IClientRenderTargets, 
	CLIENTRENDERTARGETS_INTERFACE_VERSION, g_TFRenderTargets );
CTFRenderTargets* g_pTFRenderTargets = &g_TFRenderTargets;
//========= Copyright Valve Corporation, All rights reserved. ============//
//=======================================================================================//


#ifndef TF_REPLAY_H
#define TF_REPLAY_H
#ifdef _WIN32
#pragma once
#endif
// TF2007: There has to be a replay interface of some kind
#include "replay/iclientreplay.h"
#include "replay/replayperformanceplaybackhandler.h"
#include "replay/vgui/replayperformanceeditor.h"
#include "replay/ireplayscreenshotsystem.h"
#include "replay/ireplaycamera.h"
#include "tf_viewrender.h"

class CReplayPerformancePlaybackHandler : public IReplayPerformancePlaybackHandler
{
public:
	CReplayPerformancePlaybackHandler() {}

private:
	//
	// IReplayPerformancePlaybackController
	//
	virtual void OnEvent_Camera_Change_FirstPerson( float flTime, int nEntityIndex ) {}

	virtual void OnEvent_Camera_Change_ThirdPerson( float flTime, int nEntityIndex ) {}

	virtual void OnEvent_Camera_Change_Free( float flTime ) {}

	virtual void OnEvent_Camera_ChangePlayer( float flTime, int nEntIndex ) {}

	virtual void OnEvent_Camera_SetView( const SetViewParams_t &params ) {}

	virtual void OnEvent_TimeScale( float flTime, float flScale ) {}
};

class CReplayCamera : public IReplayCamera
{
public:
	virtual void		ClearOverrideView() {}
};

extern IViewRender* view;

class CClientReplayImp : public IClientReplay
{
public:
	virtual uint64 GetServerSessionId() { return 0; }
	
	virtual IReplayScreenshotSystem *GetReplayScreenshotSystem() { return (CTFViewRender*)view; }

	virtual IReplayPerformancePlaybackHandler *GetPerformancePlaybackHandler() { return &m_ReplayPerformancePlaybackHandler; }

	virtual bool CacheReplayRagdolls( const char* pFilename, int nStartTick ) { return false; }

	virtual void OnSaveReplay( ReplayHandle_t hNewReplay, bool bShowInputDlg ) {}

	virtual void OnDeleteReplay( ReplayHandle_t hReplay ) {}

	virtual void DisplayReplayMessage( const char *pLocalizeStr, bool bUrgent, bool bDlg, const char *pSound ) {}

	virtual void DisplayReplayMessage( const wchar_t *pText, bool bUrgent, bool bDlg, const char *pSound ) {}

	virtual bool OnConfirmQuit() { return false; }

	virtual void OnRenderStart() {}

	virtual void OnRenderComplete( const RenderMovieParams_t &RenderParams, bool bCancelled, bool bSuccess, bool bShowBrowser ) {}

	virtual void InitPerformanceEditor( ReplayHandle_t hReplay ) {}

	virtual void HidePerformanceEditor() {}

	virtual bool ShouldRender() { return true; }

	virtual void PlaySound( const char *pSound ) {}

	virtual void UploadOgsData( KeyValues *pData, bool bIncludeTimeField ) {}

	virtual bool ShouldCompletePendingReplay( IGameEvent *pEvent ) { return false; }

	virtual void OnPlaybackComplete( ReplayHandle_t hReplay, int iPerformance ) {}

	virtual IReplayCamera *GetReplayCamera() { return &m_ReplayCamera; }

	virtual bool OnEndOfReplayReached() { return false; }

private:
	CReplayPerformancePlaybackHandler m_ReplayPerformancePlaybackHandler;
	CReplayCamera m_ReplayCamera;
};

#endif	// TF_REPLAY_H


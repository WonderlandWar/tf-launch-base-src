//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#include "cbase.h"
#include "tf_replay.h"
#include "tf/tf_shareddefs.h"
#include "tf/c_tf_player.h"
#include "tf/c_tf_playerresource.h"
#include "tf/c_tf_gamestats.h"
#include "tf/tf_gamestats_shared.h"
#include "tf/tf_hud_statpanel.h"
#include "tf/c_obj_sentrygun.h"
#include "clientmode_shared.h"
#include "replay/ireplaymoviemanager.h"
#include "replay/ireplayfactory.h"
#include "replay/ireplayscreenshotmanager.h"
#include "replay/screenshot.h"
#include <time.h>

static CClientReplayImp s_ClientReplayImp;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CClientReplayImp, IClientReplay, CLIENT_REPLAY_INTERFACE_VERSION, s_ClientReplayImp );

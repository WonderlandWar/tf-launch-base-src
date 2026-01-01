//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "usermessages.h"
#include "shake.h"
#include "voice_gamemgr.h"

// NVNT include to register in haptic user messages
#include "haptics/haptic_msgs.h"

void RegisterUserMessages()
{
	usermessages->Register( "Geiger", 1 );		// geiger info data
	usermessages->Register( "Train", 1 );		// train control data
	usermessages->Register( "HudText", -1 );	
	usermessages->Register( "SayText", -1 );	
	usermessages->Register( "SayText2", -1 );
	usermessages->Register( "TextMsg", -1 );
	usermessages->Register( "ResetHUD", 1 );	// called every respawn
	usermessages->Register( "GameTitle", 0 );	// show game title
	usermessages->Register( "ItemPickup", -1 );	// for item history on screen
	usermessages->Register( "ShowMenu", -1 );	// show hud menu
	usermessages->Register( "Shake", 13 );		// shake view
	usermessages->Register( "Fade", 10 );		// fade HUD in/out
	usermessages->Register( "VGUIMenu", -1 );	// Show VGUI menu
	usermessages->Register( "Rumble", 3 );	// Send a rumble to a controller
	usermessages->Register( "CloseCaption", -1 ); // Show a caption (by string id number)(duration in 10th of a second)

	usermessages->Register( "SendAudio", -1 );	// play radion command

	usermessages->Register( "VoiceMask", VOICE_MAX_PLAYERS_DW*4 * 2 + 1 );
	usermessages->Register( "RequestState", 0 );

	usermessages->Register( "Damage", -1 );		// for HUD damage indicators
	usermessages->Register( "HintText", -1 );	// Displays hint text display
	usermessages->Register( "KeyHintText", -1 );	// Displays hint text display
	
	usermessages->Register( "HudMsg", -1 );
	usermessages->Register( "AmmoDenied", 2 );
	usermessages->Register( "AchievementEvent", -1 );

	usermessages->Register( "UpdateRadar", -1 );

	usermessages->Register( "VoiceSubtitle", 3 );

	usermessages->Register( "HudNotify", 2 );	// Type, bForceDisplay
	usermessages->Register( "HudNotifyCustom", -1 );

	usermessages->Register( "PlayerStatsUpdate", -1 );
	usermessages->Register( "MapStatsUpdate", -1 );

	usermessages->Register( "PlayerIgnited", 3 );
	usermessages->Register( "PlayerIgnitedInv", 3 );

	usermessages->Register( "HudArenaNotify", 2 );

	usermessages->Register( "UpdateAchievement", -1 );


	usermessages->Register( "DamageDodged", -1 );

	usermessages->Register( "PlayerJarated", 2 );
	usermessages->Register( "PlayerExtinguished", 2 );
	usermessages->Register( "PlayerJaratedFade", 2 );
	usermessages->Register( "PlayerShieldBlocked", 2 );

	usermessages->Register( "BreakModel", -1 );
	usermessages->Register( "CheapBreakModel", -1 );
	usermessages->Register( "BreakModelRocketDud", -1 );

	// Voting
	usermessages->Register( "CallVoteFailed", -1 );
	usermessages->Register( "VoteStart", -1 );
	usermessages->Register( "VotePass", -1 );
	usermessages->Register( "VoteFailed", 6 );
	usermessages->Register( "VoteSetup", -1 );  // Initiates client-side voting UI

	usermessages->Register( "PlayerBonusPoints", 3 );
	usermessages->Register( "RDTeamPointsChanged", 4 );

	usermessages->Register( "SpawnFlyingBird", -1 );
	usermessages->Register( "PlayerGodRayEffect", -1 );
	usermessages->Register( "PlayerTeleportHomeEffect", -1 );

	usermessages->Register( "PlayerTauntSoundLoopStart", -1 );
	usermessages->Register( "PlayerTauntSoundLoopEnd", -1 );

	usermessages->Register( "ForcePlayerViewAngles", -1 );
	usermessages->Register( "BonusDucks", 2 );	// ent index, ignoretimer

	usermessages->Register( "SdkRequestEquipment", -1 );

	usermessages->Register( "BuiltObject", 3 ); // object type, object mode (entrance vs. exit), index

	// NVNT register haptic user messages
	RegisterHapticMessages();
	RegisterScriptMessages();
}


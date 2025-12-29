//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Draws CSPort's death notices
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "c_playerresource.h"
#include "iclientmode.h"
#include <vgui_controls/Controls.h>
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include "vgui_controls/TextImage.h"
#include <KeyValues.h>
#include "c_baseplayer.h"
#include "c_team.h"

#include "hud_basedeathnotice.h"

#include "tf_shareddefs.h"
#include "clientmode_tf.h"
#include "c_tf_player.h"
#include "c_tf_playerresource.h"
#include "tf_hud_freezepanel.h"
#include "engine/IEngineSound.h"
#include "tf_controls.h"
#include "tf_gamerules.h"
//#include "econ/econ_controls.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;

// Must match resource/tf_objects.txt!!!
const char *szLocalizedObjectNames[OBJ_LAST] =
{
	"#TF_Object_Dispenser",
	"#TF_Object_Tele",
	"#TF_Object_Sentry",
	"#TF_object_Sapper"
};

//-----------------------------------------------------------------------------
// TFDeathNotice
//-----------------------------------------------------------------------------
class CTFHudDeathNotice : public CHudBaseDeathNotice
{
	DECLARE_CLASS_SIMPLE( CTFHudDeathNotice, CHudBaseDeathNotice );
public:
	CTFHudDeathNotice( const char *pElementName ) : CHudBaseDeathNotice( pElementName ) {};
	virtual void Init( void );
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );
	virtual bool IsVisible( void );
	virtual bool ShouldDraw( void );

	virtual void FireGameEvent( IGameEvent *event );
	void PlayRivalrySounds( int iKillerIndex, int iVictimIndex, int iType  );
	virtual bool ShouldShowDeathNotice( IGameEvent *event );

protected:	
	virtual void OnGameEvent( IGameEvent *event, int iDeathNoticeMsg );
	virtual Color GetTeamColor( int iTeamNumber, bool bLocalPlayerInvolved = false );
	virtual Color GetInfoTextColor( int iDeathNoticeMsg );
	virtual Color GetBackgroundColor ( int iDeathNoticeMsg );

	virtual int UseExistingNotice( IGameEvent *event );

private:
	void AddAdditionalMsg( int iKillerID, int iVictimID, const char *pMsgKey );

	CHudTexture		*m_iconDomination;
	CHudTexture		*m_iconKillStreak;
	CHudTexture		*m_iconDuckStreak;
	CHudTexture		*m_iconDuckStreakDNeg;
	CHudTexture		*m_iconKillStreakDNeg;

	CPanelAnimationVar( Color, m_clrBlueText, "TeamBlue", "153 204 255 255" );
	CPanelAnimationVar( Color, m_clrRedText, "TeamRed", "255 64 64 255" );
	CPanelAnimationVar( Color, m_clrGreenText, "GreenText", "112 176 74 255" );
	CPanelAnimationVar( Color, m_clrLocalPlayerText, "LocalPlayerColor", "65 65 65 255" );

	bool m_bShowItemOnKill;
};

DECLARE_HUDELEMENT( CTFHudDeathNotice );

void CTFHudDeathNotice::Init()
{
	BaseClass::Init();

	m_bShowItemOnKill = true;
}

void CTFHudDeathNotice::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	m_iconDomination = gHUD.GetIcon( "leaderboard_dominated" );
	
	m_iconKillStreak = gHUD.GetIcon( "leaderboard_streak" );
	m_iconKillStreakDNeg = gHUD.GetIcon( "leaderboard_streak_dneg" );
	m_iconDuckStreak = gHUD.GetIcon( "eotl_duck" );
	m_iconDuckStreakDNeg = gHUD.GetIcon( "eotl_duck_dneg" );
}

bool CTFHudDeathNotice::IsVisible( void )
{
	if ( IsTakingAFreezecamScreenshot() )
		return false;

	return BaseClass::IsVisible();
}

bool CTFHudDeathNotice::ShouldDraw( void )
{
	return true;
}

bool CTFHudDeathNotice::ShouldShowDeathNotice( IGameEvent *event )
{ 
	if ( event->GetBool( "silent_kill" ) )
	{
		// Don't show a kill event for the team of the silent kill victim.
		int iVictimID = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
		C_TFPlayer* pVictim = ToTFPlayer( UTIL_PlayerByIndex( iVictimID ) );
		if ( pVictim && pVictim->GetTeamNumber() == GetLocalPlayerTeam() && iVictimID != GetLocalPlayerIndex() )
		{
			return false;
		}
	}

	return true;
}

void CTFHudDeathNotice::PlayRivalrySounds( int iKillerIndex, int iVictimIndex, int iType )
{
	int iLocalPlayerIndex = GetLocalPlayerIndex();

	//We're not involved in this kill
	if ( iKillerIndex != iLocalPlayerIndex && iVictimIndex != iLocalPlayerIndex )
		return;

	const char *pszSoundName = NULL;

	if ( iType == TF_DEATH_DOMINATION )
	{
		if ( iKillerIndex == iLocalPlayerIndex )
		{
			pszSoundName = "Game.Domination";
		}
		else if ( iVictimIndex == iLocalPlayerIndex )
		{
			pszSoundName = "Game.Nemesis";
		}
	}
	else if ( iType == TF_DEATH_REVENGE )
	{
		pszSoundName = "Game.Revenge";
	}

	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, pszSoundName );
}


//-----------------------------------------------------------------------------
// Purpose: Server's told us that someone's died
//-----------------------------------------------------------------------------
void CTFHudDeathNotice::FireGameEvent( IGameEvent *event )
{
	BaseClass::FireGameEvent( event );
}

//-----------------------------------------------------------------------------
// Purpose: Called when a game event happens and a death notice is about to be 
//			displayed.  This method can examine the event and death notice and
//			make game-specific tweaks to it before it is displayed
//-----------------------------------------------------------------------------
void CTFHudDeathNotice::OnGameEvent( IGameEvent *event, int iDeathNoticeMsg )
{
	const char *pszEventName = event->GetName();

	if ( FStrEq( pszEventName, "player_death" ) || FStrEq( pszEventName, "object_destroyed" ) )
	{
		bool bIsObjectDestroyed = FStrEq( pszEventName, "object_destroyed" );
		int iCustomDamage = event->GetInt( "customkill" );
		int iLocalPlayerIndex = GetLocalPlayerIndex();
		
		const int iKillerID = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
		const int iVictimID = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
		// if there was an assister, put both the killer's and assister's names in the death message
		int iAssisterID = engine->GetPlayerForUserID( event->GetInt( "assister" ) );

		EHorriblePyroVisionHack ePyroVisionHack = kHorriblePyroVisionHack_KillAssisterType_Default;
		CUtlConstString sAssisterNameScratch;
		const char *assister_name = ( iAssisterID > 0 ? g_PR->GetPlayerName( iAssisterID ) : NULL );

		bool bMultipleKillers = false;

		if ( assister_name )
		{
			DeathNoticeItem &msg = m_DeathNotices[ iDeathNoticeMsg ];
			const char *pszKillerName = msg.Killer.szName;
			const char *pszAssisterName = assister_name;

			// Check to see if we're swapping the killer and the assister. We use this so the brain slug can get the kill
			// credit for the HUD death notices, with the player being the assister.
			if ( pszAssisterName && (ePyroVisionHack == kHorriblePyroVisionHack_KillAssisterType_CustomName_First ||
									 ePyroVisionHack == kHorriblePyroVisionHack_KillAssisterType_LocalizationString_First) )
			{
				std::swap( pszKillerName, pszAssisterName );
			}

			char szKillerBuf[MAX_PLAYER_NAME_LENGTH*2];
			Q_snprintf( szKillerBuf, ARRAYSIZE(szKillerBuf), "%s + %s", pszKillerName, pszAssisterName );
			Q_strncpy( msg.Killer.szName, szKillerBuf, ARRAYSIZE( msg.Killer.szName ) );
			if ( iLocalPlayerIndex == iAssisterID )
			{
				msg.bLocalPlayerInvolved = true;
			}

			bMultipleKillers = true;
		}

		// play an exciting sound if a sniper pulls off any sort of penetration kill
		const int iPlayerPenetrationCount = !event->IsEmpty( "playerpenetratecount" ) ? event->GetInt( "playerpenetratecount" ) : 0;

		bool bPenetrateSound = iPlayerPenetrationCount > 0;

		if ( bPenetrateSound )
		{
			CLocalPlayerFilter filter;
			C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "Game.PenetrationKill" );
		}


		int deathFlags = event->GetInt( "death_flags" );

		if ( !bIsObjectDestroyed )
		{
			// if this death involved a player dominating another player or getting revenge on another player, add an additional message
			// mentioning that
			
			// WARNING: AddAdditionalMsg will grow and potentially realloc the m_DeathNotices array. So be careful
			//	using pointers to m_DeathNotices elements...

			if ( deathFlags & TF_DEATH_DOMINATION )
			{
				AddAdditionalMsg( iKillerID, iVictimID, "#Msg_Dominating" );
				PlayRivalrySounds( iKillerID, iVictimID, TF_DEATH_DOMINATION );
			}
			if ( deathFlags & TF_DEATH_ASSISTER_DOMINATION && ( iAssisterID > 0 ) )
			{
				AddAdditionalMsg( iAssisterID, iVictimID, "#Msg_Dominating" );
				PlayRivalrySounds( iAssisterID, iVictimID, TF_DEATH_DOMINATION );
			}
			if ( deathFlags & TF_DEATH_REVENGE ) 
			{
				AddAdditionalMsg( iKillerID, iVictimID, "#Msg_Revenge" );
				PlayRivalrySounds( iKillerID, iVictimID, TF_DEATH_REVENGE );
			}
			if ( deathFlags & TF_DEATH_ASSISTER_REVENGE && ( iAssisterID > 0 ) ) 
			{
				AddAdditionalMsg( iAssisterID, iVictimID, "#Msg_Revenge" );
				PlayRivalrySounds( iAssisterID, iVictimID, TF_DEATH_REVENGE );
			}
		}
		else
		{
			// if this is an object destroyed message, set the victim name to "<object type> (<owner>)"
			int iObjectType = event->GetInt( "objecttype" );
			if ( iObjectType >= 0 && iObjectType < OBJ_LAST )
			{
				// get the localized name for the object
				char szLocalizedObjectName[MAX_PLAYER_NAME_LENGTH];
				szLocalizedObjectName[ 0 ] = 0;
				const wchar_t *wszLocalizedObjectName = g_pVGuiLocalize->Find( szLocalizedObjectNames[iObjectType] );
				if ( wszLocalizedObjectName )
				{
					g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalizedObjectName, szLocalizedObjectName, ARRAYSIZE( szLocalizedObjectName ) );
				}
				else
				{
					Warning( "Couldn't find localized object name for '%s'\n", szLocalizedObjectNames[iObjectType] );
					Q_strncpy( szLocalizedObjectName, szLocalizedObjectNames[iObjectType], sizeof( szLocalizedObjectName ) );
				}

				// compose the string
				DeathNoticeItem &msg = m_DeathNotices[ iDeathNoticeMsg ];
				if ( msg.Victim.szName[0] )
				{
					char szVictimBuf[MAX_PLAYER_NAME_LENGTH*2];
					Q_snprintf( szVictimBuf, ARRAYSIZE(szVictimBuf), "%s (%s)", szLocalizedObjectName, msg.Victim.szName );
					Q_strncpy( msg.Victim.szName, szVictimBuf, ARRAYSIZE( msg.Victim.szName ) );
				}
				else
				{
					Q_strncpy( msg.Victim.szName, szLocalizedObjectName, ARRAYSIZE( msg.Victim.szName ) );
				}
				
			}
			else
			{
				Assert( false ); // invalid object type
			}
		}

		const wchar_t *pMsg = NULL;
		DeathNoticeItem &msg = m_DeathNotices[ iDeathNoticeMsg ];

		switch ( iCustomDamage )
		{
		case TF_DMG_CUSTOM_BACKSTAB:
			if ( FStrEq( msg.szIcon, "d_sharp_dresser" ) )
			{
				Q_strncpy( msg.szIcon, "d_sharp_dresser_backstab", ARRAYSIZE( msg.szIcon ) );
			}
			else
			{
				Q_strncpy( msg.szIcon, "d_backstab", ARRAYSIZE( msg.szIcon ) );
			}
			break;
		case TF_DMG_CUSTOM_HEADSHOT:
			{
				if ( FStrEq( event->GetString( "weapon" ), "ambassador" ) )
				{
					Q_strncpy( msg.szIcon, "d_ambassador_headshot", ARRAYSIZE( msg.szIcon ) );
				}
				else if ( FStrEq( event->GetString( "weapon" ), "huntsman" ) )
				{
					Q_strncpy( msg.szIcon, "d_huntsman_headshot", ARRAYSIZE( msg.szIcon ) );
				}
				else
				{
					// Did this headshot penetrate something before the kill? If so, show a fancy icon
					// so the player feels proud.
					if ( iPlayerPenetrationCount > 0 )
					{
						Q_strncpy( msg.szIcon, "d_headshot_player_penetration", ARRAYSIZE( msg.szIcon ) );
					}
					else
					{
						Q_strncpy( msg.szIcon, "d_headshot", ARRAYSIZE( msg.szIcon ) );
					}
				}
				
				break;
			}
		case TF_DMG_CUSTOM_BURNING:
			if ( event->GetInt( "attacker" ) == event->GetInt( "userid" ) )
			{
				// suicide by fire
				Q_strncpy( msg.szIcon, "d_firedeath", ARRAYSIZE( msg.szIcon ) );
				msg.wzInfoText[0] = 0;
			}
			break;
			
		case TF_DMG_CUSTOM_SUICIDE:
			{
				// display a different message if this was suicide, or assisted suicide (suicide w/recent damage, kill awarded to damager)
				bool bAssistedSuicide = event->GetInt( "userid" ) != event->GetInt( "attacker" );
				pMsg = g_pVGuiLocalize->Find( ( bAssistedSuicide ) ? ( bMultipleKillers ? "#DeathMsg_AssistedSuicide_Multiple" : "#DeathMsg_AssistedSuicide" ) : ( "#DeathMsg_Suicide" ) );
				if ( pMsg )
				{
					V_wcsncpy( msg.wzInfoText, pMsg, sizeof( msg.wzInfoText ) );
				}			
				break;
			}
		default:
			break;
		}

		if ( ( event->GetInt( "damagebits" ) & DMG_NERVEGAS )  )
		{
			// special case icon for hit-by-vehicle death
			Q_strncpy( msg.szIcon, "d_saw_kill", ARRAYSIZE( msg.szIcon ) );
		}

		int iKillStreakWep = event->GetInt( "kill_streak_wep" );
		int iDuckStreakTotal = event->GetInt( "duck_streak_total" );
		int iDucksThisKill = event->GetInt( "ducks_streaked" );

		if ( iKillStreakWep > 0 )
		{
			// append kill streak count to this notification
			wchar_t wzCount[10];
			_snwprintf( wzCount, ARRAYSIZE( wzCount ), L"%d", iKillStreakWep );
			g_pVGuiLocalize->ConstructString_safe( msg.wzPreKillerText, g_pVGuiLocalize->Find("#Kill_Streak"), 1, wzCount );
			if ( msg.bLocalPlayerInvolved )
			{
				msg.iconPostKillerName = m_iconKillStreakDNeg;
			}
			else
			{
				msg.iconPostKillerName = m_iconKillStreak;
			}
		}
		else if ( iDuckStreakTotal > 0 && iDucksThisKill )
		{
			// Duckstreak icon (always lower priority)
			wchar_t wzCount[10];
			_snwprintf( wzCount, ARRAYSIZE( wzCount ), L"%d", iDuckStreakTotal );
			g_pVGuiLocalize->ConstructString_safe( msg.wzPreKillerText, g_pVGuiLocalize->Find("#Duck_Streak"), 1, wzCount );
			msg.iconPostKillerName = msg.bLocalPlayerInvolved ? m_iconDuckStreakDNeg : m_iconDuckStreak;
		}
	} 
	else if ( FStrEq( "teamplay_point_captured", pszEventName ) ||
			  FStrEq( "teamplay_capture_blocked", pszEventName ) || 
			  FStrEq( "teamplay_flag_event", pszEventName ) )
	{
		bool bDefense = ( FStrEq( "teamplay_capture_blocked", pszEventName ) || ( FStrEq( "teamplay_flag_event", pszEventName ) &&
			TF_FLAGEVENT_DEFEND == event->GetInt( "eventtype" ) ) );

		DeathNoticeItem &msg = m_DeathNotices[ iDeathNoticeMsg ];
		const char *szCaptureIcons[] = { "d_redcapture", "d_bluecapture" };
		const char *szDefenseIcons[] = { "d_reddefend", "d_bluedefend" };
		
		int iTeam = msg.Killer.iTeam;
		Assert( iTeam >= FIRST_GAME_TEAM );
		Assert( iTeam < FIRST_GAME_TEAM + TF_TEAM_COUNT );
		if ( iTeam < FIRST_GAME_TEAM || iTeam >= FIRST_GAME_TEAM + TF_TEAM_COUNT )
			return;

		int iIndex = msg.Killer.iTeam - FIRST_GAME_TEAM;
		Assert( iIndex < ARRAYSIZE( szCaptureIcons ) );

		Q_strncpy( msg.szIcon, bDefense ? szDefenseIcons[iIndex] : szCaptureIcons[iIndex], ARRAYSIZE( msg.szIcon ) );
	}
	else if ( FStrEq( "fish_notice", pszEventName ) || FStrEq( "fish_notice__arm", pszEventName ) || FStrEq( "slap_notice", pszEventName ) )
	{
		DeathNoticeItem &msg = m_DeathNotices[ iDeathNoticeMsg ];
		int deathFlags = event->GetInt( "death_flags" );

		if ( ( deathFlags & TF_DEATH_FEIGN_DEATH ) )
		{
			const wchar_t *wpszFormat = g_pVGuiLocalize->Find( "#Humiliation_Kill" );
			if ( FStrEq( "fish_notice__arm", pszEventName ) )
			{
				wpszFormat = g_pVGuiLocalize->Find( "#Humiliation_Kill_Arm" );
			}
			else if ( FStrEq( "slap_notice", pszEventName ) )
			{
				wpszFormat = g_pVGuiLocalize->Find( "#Humiliation_Kill_Slap" );
			}
			g_pVGuiLocalize->ConstructString_safe( msg.wzInfoText, wpszFormat, 0 );
		}
		else
		{
			wchar_t wzCount[10];
			_snwprintf( wzCount, ARRAYSIZE( wzCount ), L"%d", ++msg.iCount );
			g_pVGuiLocalize->ConstructString_safe( msg.wzInfoText, g_pVGuiLocalize->Find("#Humiliation_Count"), 1, wzCount );
		}

		// if there was an assister, put both the killer's and assister's names in the death message
		int iAssisterID = engine->GetPlayerForUserID( event->GetInt( "assister" ) );
		const char *assister_name = ( iAssisterID > 0 ? g_PR->GetPlayerName( iAssisterID ) : NULL );
		if ( assister_name )
		{
			char szKillerBuf[MAX_PLAYER_NAME_LENGTH*2];
			Q_snprintf( szKillerBuf, ARRAYSIZE(szKillerBuf), "%s + %s", msg.Killer.szName, assister_name );
			Q_strncpy( msg.Killer.szName, szKillerBuf, ARRAYSIZE( msg.Killer.szName ) );
		}
	}
	//else if ( FStrEq( "throwable_hit", pszEventName ) )
	//{
	//	DeathNoticeItem &msg = m_DeathNotices[ iDeathNoticeMsg ];
	//	int deathFlags = event->GetInt( "death_flags" );
	//	int iCustomDamage = event->GetInt( "customkill" );

	//	// Make sure the icon is up to date
	//	m_DeathNotices[iDeathNoticeMsg].iconDeath = GetIcon( m_DeathNotices[ iDeathNoticeMsg ].szIcon, m_DeathNotices[iDeathNoticeMsg].bLocalPlayerInvolved ? kDeathNoticeIcon_Inverted : kDeathNoticeIcon_Standard );

	//	if ( ( iCustomDamage == TF_DMG_CUSTOM_THROWABLE_KILL ) || ( deathFlags & TF_DEATH_FEIGN_DEATH ) )
	//	{
	//		g_pVGuiLocalize->ConstructString_safe( msg.wzInfoText, g_pVGuiLocalize->Find("#Throwable_Kill"), 0 );
	//	}
	//	else
	//	{
	//		wchar_t wzCount[10];
	//		_snwprintf( wzCount, ARRAYSIZE( wzCount ), L"%d", event->GetInt( "totalhits" ) );
	//		g_pVGuiLocalize->ConstructString_safe( msg.wzInfoText, g_pVGuiLocalize->Find("#Humiliation_Count"), 1, wzCount );
	//	}

	//	// if there was an assister, put both the killer's and assister's names in the death message
	//	int iAssisterID = engine->GetPlayerForUserID( event->GetInt( "assister" ) );
	//	const char *assister_name = ( iAssisterID > 0 ? g_PR->GetPlayerName( iAssisterID ) : NULL );
	//	if ( assister_name )
	//	{
	//		char szKillerBuf[MAX_PLAYER_NAME_LENGTH*2];
	//		Q_snprintf( szKillerBuf, ARRAYSIZE(szKillerBuf), "%s + %s", msg.Killer.szName, assister_name );
	//		Q_strncpy( msg.Killer.szName, szKillerBuf, ARRAYSIZE( msg.Killer.szName ) );
	//	}
	//}
	else if ( FStrEq( "rd_robot_killed", pszEventName ) )
	{
		DeathNoticeItem &msg = m_DeathNotices[ iDeathNoticeMsg ];

		int killer = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
		const char *killedwith = event->GetString( "weapon" );

		// flags
		if ( GetLocalPlayerIndex() == killer )
			msg.bLocalPlayerInvolved = true;

		msg.Killer.iTeam = g_PR->GetTeam( killer );
		Q_strncpy( msg.Killer.szName, g_PR->GetPlayerName( killer ), ARRAYSIZE( msg.Killer.szName ) );

		Q_strncpy( msg.Victim.szName, g_PR->GetTeam( killer ) == TF_TEAM_RED ? "BLUE ROBOT" : "RED ROBOT", ARRAYSIZE( msg.Victim.szName ) );
		msg.Victim.iTeam = g_PR->GetTeam( killer ) == TF_TEAM_RED ? TF_TEAM_BLUE : TF_TEAM_RED;

		Q_snprintf( msg.szIcon, sizeof(msg.szIcon), "d_%s", killedwith );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds an additional death message
//-----------------------------------------------------------------------------
void CTFHudDeathNotice::AddAdditionalMsg( int iKillerID, int iVictimID, const char *pMsgKey )
{
	DeathNoticeItem &msg2 = m_DeathNotices[AddDeathNoticeItem()];
	Q_strncpy( msg2.Killer.szName, g_PR->GetPlayerName( iKillerID ), ARRAYSIZE( msg2.Killer.szName ) );
	msg2.Killer.iTeam = g_PR->GetTeam( iKillerID );
	Q_strncpy( msg2.Victim.szName, g_PR->GetPlayerName( iVictimID ), ARRAYSIZE( msg2.Victim.szName ) );
	msg2.Victim.iTeam = g_PR->GetTeam( iVictimID );
	const wchar_t *wzMsg =  g_pVGuiLocalize->Find( pMsgKey );
	if ( wzMsg )
	{
		V_wcsncpy( msg2.wzInfoText, wzMsg, sizeof( msg2.wzInfoText ) );
	}
	msg2.iconDeath = m_iconDomination;
	int iLocalPlayerIndex = GetLocalPlayerIndex();
	if ( iLocalPlayerIndex == iVictimID || iLocalPlayerIndex == iKillerID )
	{
		msg2.bLocalPlayerInvolved = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns the color to draw text in for this team.  
//-----------------------------------------------------------------------------
Color CTFHudDeathNotice::GetTeamColor( int iTeamNumber, bool bLocalPlayerInvolved /* = false */ )
{
	switch ( iTeamNumber )
	{
	case TF_TEAM_BLUE:
		return m_clrBlueText;
		break;
	case TF_TEAM_RED:
		return m_clrRedText;
		break;
	case TEAM_UNASSIGNED:
		if ( bLocalPlayerInvolved )
			return m_clrLocalPlayerText;
		else
			return Color( 255, 255, 255, 255 );
		break;
	default:
		AssertOnce( false );	// invalid team
		return Color( 255, 255, 255, 255 );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Color CTFHudDeathNotice::GetInfoTextColor( int iDeathNoticeMsg )
{ 
	DeathNoticeItem &msg = m_DeathNotices[ iDeathNoticeMsg ];

	if ( msg.bLocalPlayerInvolved )
		return m_clrLocalPlayerText;

	return Color( 255, 255, 255, 255 );
}

//-----------------------------------------------------------------------------
Color CTFHudDeathNotice::GetBackgroundColor ( int iDeathNoticeMsg )
{ 
	DeathNoticeItem &msg = m_DeathNotices[ iDeathNoticeMsg ];

	return msg.bLocalPlayerInvolved ? m_clrLocalBGColor : m_clrBaseBGColor; 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFHudDeathNotice::UseExistingNotice( IGameEvent *event )
{
	return BaseClass::UseExistingNotice( event );
}
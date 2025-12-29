//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: HUD Target ID element
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tf_hud_target_id.h"
#include "c_tf_playerresource.h"
#include "iclientmode.h"
#include "vgui/ILocalize.h"
#include "c_baseobject.h"
#include "c_team.h"
#include "tf_gamerules.h"
#include "tf_hud_statpanel.h"
#if defined( REPLAY_ENABLED )
#include "replay/iclientreplaycontext.h"
#include "replay/ireplaymoviemanager.h"
#include "replay/ienginereplay.h"
#endif // REPLAY_ENABLED
#include "tf_weapon_bonesaw.h"
#include "sourcevr/isourcevirtualreality.h"
#include "entity_capture_flag.h"
#include "vgui_avatarimage.h"

#include "VGuiMatSurface/IMatSystemSurface.h"
#include "renderparm.h"
#include "fmtstr.h"
#include "inputsystem/iinputsystem.h"
#include "c_obj_sentrygun.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar cl_hud_minmode;

DECLARE_HUDELEMENT( CMainTargetID );
DECLARE_HUDELEMENT( CSpectatorTargetID );
DECLARE_HUDELEMENT( CSecondaryTargetID );

using namespace vgui;

enum
{
	SPECTATOR_TARGET_ID_NORMAL = 0,
	SPECTATOR_TARGET_ID_BOTTOM_LEFT,
	SPECTATOR_TARGET_ID_BOTTOM_CENTER,
	SPECTATOR_TARGET_ID_BOTTOM_RIGHT,
};

void SpectatorTargetLocationCallback( IConVar *var, const char *oldString, float oldFloat )
{
	CSpectatorTargetID *pSpecTargetID = (CSpectatorTargetID *)GET_HUDELEMENT( CSpectatorTargetID );
	if ( pSpecTargetID )
	{
		pSpecTargetID->InvalidateLayout();
	}
}
ConVar tf_spectator_target_location( "tf_spectator_target_location", "0", FCVAR_ARCHIVE, "Determines the location of the spectator targetID panel.", true, 0, true, 3, SpectatorTargetLocationCallback );

void DisableFloatingHealthCallback( IConVar *var, const char *oldString, float oldFloat )
{
	CMainTargetID *pTargetID = (CMainTargetID *)GET_HUDELEMENT( CMainTargetID );
	if ( pTargetID )
	{
		pTargetID->InvalidateLayout();
	}
}
ConVar tf_hud_target_id_alpha( "tf_hud_target_id_alpha", "100", FCVAR_ARCHIVE, "Alpha value of target id background, default 100" );
ConVar tf_hud_target_id_offset( "tf_hud_target_id_offset", "0", FCVAR_ARCHIVE, "RES file Y offset for target id" );

bool ShouldHealthBarBeVisible( CBaseEntity *pTarget, CTFPlayer *pLocalPlayer )
{
	if ( !pTarget || !pLocalPlayer )
		return false;

	if ( true )
		return false;

	if ( !pTarget->IsPlayer() )
		return false;

	if ( pLocalPlayer->IsPlayerClass( TF_CLASS_SPY ) )
		return true;

	if ( pLocalPlayer->InSameTeam( pTarget ) )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTargetID::CTargetID( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, pElementName )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_hFont = g_hFontTrebuchet24;
	m_flLastChangeTime = 0;
	m_iLastEntIndex = 0;
	m_nOriginalY = 0;

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	m_pTargetNameLabel = NULL;
	m_pTargetDataLabel = NULL;
	m_pBGPanel = NULL;
	m_pTargetHealth = new CTFSpectatorGUIHealth( this, "SpectatorGUIHealth" );
	m_bLayoutOnUpdate = false;

	m_iLastScannedEntIndex = 0;

	RegisterForRenderGroup( "mid" );
	RegisterForRenderGroup( "commentary" );

	m_iRenderPriority = 5;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTargetID::LevelShutdown( void )
{

}

//-----------------------------------------------------------------------------
// Purpose: Setup
//-----------------------------------------------------------------------------
void CTargetID::Reset( void )
{
	m_pTargetHealth->Reset();

	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
	if ( pScheme )
	{
		m_LabelColorDefault = pScheme->GetColor( "Label.TextColor", Color( 255, 255, 255, 255 ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTargetID::ApplySchemeSettings( vgui::IScheme *scheme )
{
	LoadControlSettings( "resource/UI/TargetID.res" );

	BaseClass::ApplySchemeSettings( scheme );

	m_pTargetNameLabel = dynamic_cast<Label *>(FindChildByName("TargetNameLabel"));
	m_pTargetDataLabel = dynamic_cast<Label *>(FindChildByName("TargetDataLabel"));
	m_pBGPanel =  dynamic_cast<CTFImagePanel *> ( FindChildByName("TargetIDBG") );
	m_hFont = scheme->GetFont( "TargetID", true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTargetID::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	m_iRenderPriority = inResourceData->GetInt( "priority" );

	int x;
	GetPos( x, m_nOriginalY );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTargetID::GetRenderGroupPriority( void )
{
	return m_iRenderPriority;
}

//-----------------------------------------------------------------------------
// Purpose: clear out string etc between levels
//-----------------------------------------------------------------------------
void CTargetID::VidInit()
{
	CHudElement::VidInit();

	m_flLastChangeTime = 0;
	m_iLastEntIndex = 0;
}

bool CTargetID::IsValidIDTarget( int nEntIndex, float flOldTargetRetainFOV, float &flNewTargetRetainFOV )
{
	bool bReturn = false;
	flNewTargetRetainFOV = 0.0f;

	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalTFPlayer )
		return false;


	if ( nEntIndex )
	{
		C_BaseEntity *pEnt = cl_entitylist->GetEnt( nEntIndex );
		if ( pEnt )
		{
			Vector vDiff = pEnt->EyePosition() - pLocalTFPlayer->EyePosition();
			float flDist;
			flDist = VectorNormalize( vDiff );

			if ( flOldTargetRetainFOV != 0.0f )
			{
				// It has a FOV that maintains previous targets
				Vector vForward;
				pLocalTFPlayer->EyeVectors( &vForward );

				float fAngle = 1.0f - vDiff.Dot( vForward );
				fAngle = RemapVal( fAngle, 0.0f, 1.0f, 0.0f, 90.0f );

				if ( fAngle > flOldTargetRetainFOV )
				{
					return false;
				}
			}

			C_TFPlayer *pPlayer = ToTFPlayer( pEnt );

			bool bSpectator = pLocalTFPlayer->GetTeamNumber() == TEAM_SPECTATOR;
			bool bHealthBarVisible = ShouldHealthBarBeVisible( pEnt, pLocalTFPlayer );
			bool bShow = bHealthBarVisible;
			bool bInSameTeam = pLocalTFPlayer->GetTeamNumber() == pEnt->GetTeamNumber();	

			if ( pPlayer )
			{
				if ( pPlayer->m_Shared.IsStealthed() )
				{
					bHealthBarVisible = false;
					bShow = false;
				}

				bool bMaintainInFOV = !pLocalTFPlayer->InSameTeam( pEnt );

				if ( bHealthBarVisible )
				{
					if ( bShow )
					{
						bMaintainInFOV = false;
					}
				}

				if ( bMaintainInFOV )
				{
					const float flMaxDist = 800.0f;
					float fInterp = RemapVal( flMaxDist - MIN( flDist, flMaxDist ), 0.0f, flMaxDist, 0.0f, 1.0f );
					fInterp *= fInterp;
					flNewTargetRetainFOV = fInterp * 13.0f + 0.75f;
				}

				bReturn = ( bSpectator || pLocalTFPlayer->InSameTeam( pEnt ) );
			}

			if ( pEnt->IsBaseObject() && ( bInSameTeam ) )
			{
				bReturn = true;
			}
			else if ( pEnt->IsVisibleToTargetID() )
			{
				bReturn = true;
			}
		}
	}

	return bReturn;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTargetID::ShouldDraw( void )
{
	if ( !CHudElement::ShouldDraw() )
	{
		return false;
	}

	if ( TFGameRules() && TFGameRules()->ShowMatchSummary() )
	{
		return false;
	}

	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalTFPlayer )
	{
		return false;
	}

	if ( pLocalTFPlayer->IsTaunting() )
	{
		return false;
	}

	// Get our target's ent index
	m_iTargetEntIndex = CalculateTargetIndex(pLocalTFPlayer);
	if ( !m_iTargetEntIndex )
	{
		if ( m_flTargetRetainFOV == 0.0f )
		{
			// Check to see if we should clear our ID
			if ( m_flLastChangeTime && ( gpGlobals->curtime > m_flLastChangeTime ) )
			{
				m_flLastChangeTime = 0;
				m_iLastEntIndex = 0;
			}
			else
			{
				// Keep re-using the old one
				m_iTargetEntIndex = m_iLastEntIndex;
			}
		}
	}
	else
	{
		m_flLastChangeTime = gpGlobals->curtime;

		if ( m_iTargetEntIndex != m_iLastScannedEntIndex )
		{
			m_iLastScannedEntIndex = m_iTargetEntIndex;
		}
	}

	float flTargetRetainFOV = 0.0f;
	bool bReturn = IsValidIDTarget( m_iTargetEntIndex, 0.0f, flTargetRetainFOV );

	if ( !bReturn )
	{
		m_iLastEntIndex = 0;
	}
	else
	{
		if ( !IsVisible() || (m_iTargetEntIndex != m_iLastEntIndex) )
		{
			m_iLastEntIndex = m_iTargetEntIndex;
			m_bLayoutOnUpdate = true;
			m_flTargetRetainFOV = flTargetRetainFOV;
		}

		UpdateID();
	}

	return bReturn;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTargetID::PerformLayout( void )
{
	int iXIndent = XRES(5);
	int iXPostdent = XRES(10);
	int iWidth = iXIndent + iXPostdent;
	iWidth += m_pTargetHealth->GetWide();

	int iTextW, iTextH;
	int iDataW, iDataH;

	if ( m_pTargetNameLabel && m_pTargetDataLabel )
	{
		m_pTargetNameLabel->GetContentSize( iTextW, iTextH );
		m_pTargetDataLabel->GetContentSize( iDataW, iDataH );
		iWidth += MAX(iTextW,iDataW);

		if ( m_pBGPanel )
		{
			m_pBGPanel->SetSize( iWidth, GetTall() );
		}

		int x1 = 0, y1 = 0;
		int x2 = 0, y2 = 0;
		m_pTargetNameLabel->GetPos( x1, y1 );
		m_pTargetDataLabel->GetPos( x2, y2 );
		
		int iWideExtra = m_pTargetHealth->GetWide();

		int nBuffer = 8;
		m_pTargetNameLabel->SetPos( XRES( nBuffer ) + iWideExtra, y1 );
		m_pTargetDataLabel->SetPos( XRES( nBuffer ) + iWideExtra, y2 );
	}

	SetSize( iWidth, GetTall() );

	if( UseVR() )
	{
		SetPos( ScreenWidth() - iWidth - m_iXOffset,  m_nOriginalY + YRES( tf_hud_target_id_offset.GetInt() ) );
	}
	else
	{
		SetPos( (ScreenWidth() - iWidth) * 0.5,  m_nOriginalY + YRES( tf_hud_target_id_offset.GetInt() ) );
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTargetID::CalculateTargetIndex( C_TFPlayer *pLocalTFPlayer ) 
{ 
	int iIndex = pLocalTFPlayer->GetIDTarget(); 

	// If our target entity is already in our secondary ID, don't show it in primary.
	CSecondaryTargetID *pSecondaryID = GET_HUDELEMENT( CSecondaryTargetID );
	if ( pSecondaryID && pSecondaryID != this && pSecondaryID->GetTargetIndex() == iIndex )
	{
		iIndex = 0;
	}

	return iIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTargetID::UpdateID( void )
{
	wchar_t sIDString[ MAX_ID_STRING ] = L"";
	wchar_t sDataString[ MAX_ID_STRING ] = L"";

	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalTFPlayer )
		return;

	// Default the labels' colors
	Color colorName = m_LabelColorDefault;
	Color colorData = m_LabelColorDefault;

	// Get our target's ent index
	// Is this an entindex sent by the server?
	if ( m_iTargetEntIndex )
	{
		C_BaseEntity *pEnt = cl_entitylist->GetEnt( m_iTargetEntIndex );
		if ( !pEnt )
			return;

		bool bShowHealth = false;
		float flHealth = 0;
		float flMaxHealth = 1;
		int iMaxBuffedHealth = 0;
		int iTargetTeam = pEnt->GetTeamNumber();
		const char *pszActionIcon = NULL;

		m_pTargetHealth->SetBuilding( false );

		// Some entities we always want to check, cause the text may change
		// even while we're looking at it
		// Is it a player?
		if ( IsPlayerIndex( m_iTargetEntIndex ) )
		{
			const char *printFormatString = NULL;
			wchar_t wszPlayerName[ MAX_PLAYER_NAME_LENGTH ];
			bool bDisguisedTarget = false;
			bool bDisguisedEnemy = false;

			C_TFPlayer *pPlayer = static_cast<C_TFPlayer*>( pEnt );
			if ( !pPlayer )
				return;

			C_TFPlayer *pDisguiseTarget = NULL;
			g_pVGuiLocalize->ConvertANSIToUnicode( pPlayer->GetPlayerName(), wszPlayerName, sizeof(wszPlayerName) );

			// determine if the target is a disguised spy (either friendly or enemy)
			if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && // they're disguised
				//!pPlayer->m_Shared.InCond( TF_COND_DISGUISING ) && // they're not in the process of disguising
				!pPlayer->m_Shared.IsStealthed() ) // they're not cloaked
			{
				bDisguisedTarget = true;
				pDisguiseTarget = pPlayer->m_Shared.GetDisguiseTarget();

				if ( pLocalTFPlayer->InSameTeam( pEnt ) == false )
				{
					iTargetTeam = pPlayer->m_Shared.GetDisguiseTeam();
				}
			}

			if ( bDisguisedTarget )
			{
				// is the target a disguised enemy spy?
				if ( pPlayer->IsEnemyPlayer() )
				{
					if ( pDisguiseTarget )
					{
						bDisguisedEnemy = true;
						// change the player name
						g_pVGuiLocalize->ConvertANSIToUnicode( pDisguiseTarget->GetPlayerName(), wszPlayerName, sizeof(wszPlayerName) );
						// change the team  / team color
					}
				}
			}

			bool bInSameTeam = pLocalTFPlayer->InSameDisguisedTeam( pEnt );
			bool bSpy = pLocalTFPlayer->IsPlayerClass( TF_CLASS_SPY );
			bool bMedic = pLocalTFPlayer->IsPlayerClass( TF_CLASS_MEDIC );
			bool bHeavy = pLocalTFPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS );

			// See if the player wants to fill in the data string
			bool bIsAmmoData = false;
			bool bIsKillStreakData = false;
			pPlayer->GetTargetIDDataString( bDisguisedTarget, sDataString, sizeof(sDataString), bIsAmmoData, bIsKillStreakData );
			if ( pLocalTFPlayer->GetTeamNumber() == TEAM_SPECTATOR || bInSameTeam || bSpy || bDisguisedEnemy || bMedic || bHeavy )
			{
				printFormatString = "#TF_playerid_sameteam";
				bShowHealth = true;
			}
			else if ( pLocalTFPlayer->m_Shared.GetState() == TF_STATE_DYING )
			{
				// We're looking at an enemy who killed us.
				printFormatString = "#TF_playerid_diffteam";
				bShowHealth = true;
			}			

			if ( bShowHealth )
			{
				if ( g_TF_PR )
				{
					if ( bDisguisedEnemy )
					{
						flHealth = (float)pPlayer->m_Shared.GetDisguiseHealth();
						flMaxHealth = (float)pPlayer->m_Shared.GetDisguiseMaxHealth();
						iMaxBuffedHealth = pPlayer->m_Shared.GetDisguiseMaxBuffedHealth();
					}
					else
					{
						flHealth = (float)pPlayer->GetHealth();
						flMaxHealth = g_TF_PR->GetMaxHealth( m_iTargetEntIndex );
						iMaxBuffedHealth = pPlayer->m_Shared.GetMaxBuffedHealth();
					}
				}
				else
				{
					bShowHealth = false;
				}
			}

			if ( printFormatString )
			{
				const wchar_t *pszPrepend = GetPrepend();
				if ( !pszPrepend || !pszPrepend[0] )
				{
					pszPrepend = L"";
				}
				g_pVGuiLocalize->ConstructString_safe( sIDString, g_pVGuiLocalize->Find(printFormatString), 2, pszPrepend, wszPlayerName );
			}
		}
		else	
		{
			// see if it is an object
			if ( pEnt->IsBaseObject() )
			{
				C_BaseObject *pObj = assert_cast<C_BaseObject *>( pEnt );

				pObj->GetTargetIDString( sIDString, sizeof(sIDString), false );
				pObj->GetTargetIDDataString( sDataString, sizeof(sDataString) );
				bShowHealth = true;
				flHealth = pObj->GetHealth();
				flMaxHealth = pObj->GetMaxHealth();
				m_pTargetHealth->SetBuilding( true );

				// Switch the icon to the right object
				if ( pObj->GetBuilder() == pLocalTFPlayer )
				{
					int iObj = pObj->GetType();

					if ( iObj >= OBJ_DISPENSER && iObj <= OBJ_SENTRYGUN )
					{						
						switch ( iObj )
						{
						default:
						case OBJ_DISPENSER:
							pszActionIcon = "obj_status_dispenser";
							break;
						case OBJ_TELEPORTER:
							{
								pszActionIcon = (pObj->GetObjectMode() == MODE_TELEPORTER_ENTRANCE) ? "obj_status_tele_entrance" : "obj_status_tele_exit";
							}
							break;
						case OBJ_SENTRYGUN:
							{
								int iLevel = ((C_ObjectSentrygun*)pObj)->GetUpgradeLevel();
								if ( iLevel == 3 )
								{
									pszActionIcon =  "obj_status_sentrygun_3";
								}
								else
								{
									pszActionIcon = (iLevel == 2) ? "obj_status_sentrygun_2" : "obj_status_sentrygun_1";
								}
							}
							break;
						}
					}
				}
			}
			// Generic
			else if ( pEnt->IsVisibleToTargetID() )
			{
				CCaptureFlag *pFlag = dynamic_cast< CCaptureFlag * >( pEnt );
				if ( pFlag && pFlag->GetPointValue() > 0 )
				{
					bShowHealth = false;
					g_pVGuiLocalize->ConvertANSIToUnicode( CFmtStr("%d Points", pFlag->GetPointValue() ), sIDString, sizeof(sIDString) );
				}
				else
				{
					if ( pLocalTFPlayer->InSameTeam( pEnt ) )
					{
						bShowHealth = true;
						flHealth = pEnt->GetHealth();
						flMaxHealth = pEnt->GetMaxHealth();
						iMaxBuffedHealth = pEnt->GetMaxHealth();
					}
				}
			}
		}

		// Setup health icon
		if ( !pEnt->IsAlive() && ( pEnt->IsPlayer() || pEnt->IsBaseObject() ) )
		{
			flHealth = 0;	// fixup for health being 1 when dead
		}

		m_pTargetHealth->SetHealth( flHealth, flMaxHealth, iMaxBuffedHealth );
		m_pTargetHealth->SetVisible( true );

		if ( m_pTargetNameLabel && m_pTargetDataLabel )
		{
			int iNameW, iDataW, iIgnored;
			m_pTargetNameLabel->GetContentSize( iNameW, iIgnored );
			m_pTargetDataLabel->GetContentSize( iDataW, iIgnored );

			// Target name
			if ( sIDString[0] )
			{
				sIDString[ ARRAYSIZE(sIDString)-1 ] = '\0';
				m_pTargetNameLabel->SetVisible(true);
				m_pTargetNameLabel->SetFgColor( colorName );

				// TODO: Support	if( hud_centerid.GetInt() == 0 )
				SetDialogVariable( "targetname", sIDString );
			}
			else
			{
				m_pTargetNameLabel->SetVisible(false);
				m_pTargetNameLabel->SetText("");
			}

			// Extra target data
			if ( sDataString[0] )
			{
				sDataString[ ARRAYSIZE(sDataString)-1 ] = '\0';
				m_pTargetDataLabel->SetVisible(true);
				m_pTargetDataLabel->SetFgColor( colorData );

				SetDialogVariable( "targetdata", sDataString );
			}
			else
			{
				m_pTargetDataLabel->SetVisible(false);
				m_pTargetDataLabel->SetText("");
			}

			int iPostNameW, iPostDataW;
			m_pTargetNameLabel->GetContentSize( iPostNameW, iIgnored );
			m_pTargetDataLabel->GetContentSize( iPostDataW, iIgnored );

			if ( m_pBGPanel )
			{
				m_pBGPanel->SetBGTeam( iTargetTeam );
				m_pBGPanel->UpdateBGImage();
				m_pBGPanel->SetAlpha( tf_hud_target_id_alpha.GetInt() );
			}

			if ( m_bLayoutOnUpdate || (iPostDataW != iDataW) || (iPostNameW != iNameW) )
			{
				InvalidateLayout( true );
				m_bLayoutOnUpdate = false;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSecondaryTargetID::CSecondaryTargetID( const char *pElementName ) : CTargetID( pElementName )
{
	m_wszPrepend[0] = '\0';

	RegisterForRenderGroup( "mid" );

	m_bWasHidingLowerElements = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSecondaryTargetID::ShouldDraw( void )
{
	bool bDraw = BaseClass::ShouldDraw();

	if ( bDraw )
	{
		if ( !m_bWasHidingLowerElements )
		{
			HideLowerPriorityHudElementsInGroup( "mid" );
			m_bWasHidingLowerElements = true;
		}
	}
	else 
	{
		if ( m_bWasHidingLowerElements )
		{
			UnhideLowerPriorityHudElementsInGroup( "mid" );
			m_bWasHidingLowerElements = false;
		}
	}

	return bDraw;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CSecondaryTargetID::CalculateTargetIndex( C_TFPlayer *pLocalTFPlayer )
{
	// If we're a medic & we're healing someone, target him.
	CBaseEntity *pHealTarget = pLocalTFPlayer->MedicGetHealTarget();
	if ( pHealTarget )
	{
		if ( pHealTarget->entindex() != m_iTargetEntIndex )
		{
			g_pVGuiLocalize->ConstructString_safe( m_wszPrepend, g_pVGuiLocalize->Find("#TF_playerid_healtarget" ), 0 );
		}
		return pHealTarget->entindex();
	}

	// If we have a healer, target him.
	C_TFPlayer *pHealer;
	float flHealerChargeLevel;
	pLocalTFPlayer->GetHealer( &pHealer, &flHealerChargeLevel );
	if ( pHealer )
	{
		if ( pHealer->entindex() != m_iTargetEntIndex )
		{
			g_pVGuiLocalize->ConstructString_safe( m_wszPrepend, g_pVGuiLocalize->Find("#TF_playerid_healer" ), 0 );
		}
		return pHealer->entindex();
	}

	if ( m_iTargetEntIndex )
	{
		m_wszPrepend[0] = '\0';
	}
	return 0;
}

// Separately declared versions of the hud element for alive and dead so they
// can have different positions

bool CMainTargetID::ShouldDraw( void )
{
	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalTFPlayer )
		return false;

	if ( pLocalTFPlayer->GetObserverMode() > OBS_MODE_NONE )
		return false;

	return BaseClass::ShouldDraw();
}

bool CSpectatorTargetID::ShouldDraw( void )
{
	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalTFPlayer )
		return false;

	if ( pLocalTFPlayer->GetObserverMode() <= OBS_MODE_NONE ||
		 pLocalTFPlayer->GetObserverMode() == OBS_MODE_FREEZECAM )
		return false;

	// Hide panel for freeze-cam screenshot?
	extern bool IsTakingAFreezecamScreenshot();
	extern ConVar hud_freezecamhide;
	
	if ( IsTakingAFreezecamScreenshot() && hud_freezecamhide.GetBool() )
		return false;

#if defined( REPLAY_ENABLED )
	if ( g_pEngineClientReplay->IsPlayingReplayDemo() )
		return false;
#endif

	return BaseClass::ShouldDraw();
}

int	CSpectatorTargetID::CalculateTargetIndex( C_TFPlayer *pLocalTFPlayer ) 
{ 
	int iIndex = BaseClass::CalculateTargetIndex( pLocalTFPlayer );

#if defined( REPLAY_ENABLED )
	// Don't execute this if we're watching a replay
	if ( ( !g_pEngineClientReplay || !g_pEngineClientReplay->IsPlayingReplayDemo() ) && pLocalTFPlayer->GetObserverMode() == OBS_MODE_IN_EYE && pLocalTFPlayer->GetObserverTarget() )
#else
	if ( pLocalTFPlayer->GetObserverMode() == OBS_MODE_IN_EYE && pLocalTFPlayer->GetObserverTarget() )
#endif
	{
		iIndex = pLocalTFPlayer->GetObserverTarget()->entindex();
	}

	return iIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSpectatorTargetID::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	if ( m_pBGPanel )
	{
		m_pBGPanel->SetVisible( false );
	}

	m_pBGPanel_Spec_Blue = FindChildByName("TargetIDBG_Spec_Blue");
	m_pBGPanel_Spec_Red = FindChildByName("TargetIDBG_Spec_Red");

	if ( m_pBGPanel_Spec_Blue )
	{
		m_pBGPanel_Spec_Blue->SetVisible( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSpectatorTargetID::PerformLayout( void )
{
	int iXIndent = XRES(5);
	int iXPostdent = XRES(10);
	int iWidth = m_pTargetHealth->GetWide() + iXIndent + iXPostdent;

	int iTextW, iTextH;
	int iDataW, iDataH;

	if ( m_pTargetNameLabel && m_pTargetDataLabel )
	{
		m_pTargetNameLabel->GetContentSize( iTextW, iTextH );
		m_pTargetDataLabel->GetContentSize( iDataW, iDataH );
		iWidth += MAX(iTextW,iDataW);

		SetSize( iWidth, GetTall() );

		int x1 = 0, y1 = 0;
		int x2 = 0, y2 = 0;
		m_pTargetNameLabel->GetPos( x1, y1 );
		m_pTargetDataLabel->GetPos( x2, y2 );

		// Shift Labels
		{
			int nBuffer = 8;
			m_pTargetNameLabel->SetPos( XRES( nBuffer ) + m_pTargetHealth->GetWide(), y1 );
			m_pTargetDataLabel->SetPos( XRES( nBuffer ) + m_pTargetHealth->GetWide(), y2 );
		}

		if ( tf_spectator_target_location.GetInt() == SPECTATOR_TARGET_ID_NORMAL )
		{
			SetPos( (ScreenWidth() - iWidth) * 0.5,  m_nOriginalY );
		}
		else
		{
			int iBottomBarHeight = 0;
			if ( g_pSpectatorGUI && g_pSpectatorGUI->IsVisible() )
			{
				iBottomBarHeight = g_pSpectatorGUI->GetBottomBarHeight();
			}

			int iYPos =	ScreenHeight() - GetTall() - iBottomBarHeight - m_iYOffset;

			if ( tf_spectator_target_location.GetInt() == SPECTATOR_TARGET_ID_BOTTOM_LEFT )
			{
				SetPos( m_iXOffset, iYPos );
			}
			else if ( tf_spectator_target_location.GetInt() == SPECTATOR_TARGET_ID_BOTTOM_CENTER )
			{
				SetPos( (ScreenWidth() - iWidth) * 0.5, iYPos );
			}
			else if ( tf_spectator_target_location.GetInt() == SPECTATOR_TARGET_ID_BOTTOM_RIGHT )
			{
				SetPos( ScreenWidth() - iWidth - m_iXOffset, iYPos );
			}
		}

		if ( m_pBGPanel_Spec_Blue )
		{
			m_pBGPanel_Spec_Blue->SetSize( iWidth, GetTall() );
		}

		if ( m_pBGPanel_Spec_Red )
		{
			m_pBGPanel_Spec_Red->SetSize( iWidth, GetTall() );
		}

		if ( m_pBGPanel_Spec_Blue && m_pBGPanel_Spec_Red )
		{
			if ( m_iTargetEntIndex )
			{
				C_BaseEntity *pEnt = cl_entitylist->GetEnt( m_iTargetEntIndex );
				if ( pEnt )
				{
					bool bRed = ( pEnt->GetTeamNumber() == TF_TEAM_RED );
					m_pBGPanel_Spec_Blue->SetVisible( !bRed );
					m_pBGPanel_Spec_Red->SetVisible( bRed );			

					m_pBGPanel_Spec_Blue->SetAlpha( tf_hud_target_id_alpha.GetInt() );
					m_pBGPanel_Spec_Red->SetAlpha( tf_hud_target_id_alpha.GetInt() );
				}
			}
		}
	}
}
//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates objective data
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "clientmode_tf.h"
#include "c_tf_objective_resource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT( C_TFObjectiveResource, DT_TFObjectiveResource, CTFObjectiveResource)
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFObjectiveResource::C_TFObjectiveResource()
{
	PrecacheMaterial( "sprites/obj_icons/icon_obj_cap_blu" );
	PrecacheMaterial( "sprites/obj_icons/icon_obj_cap_blu_up" );
	PrecacheMaterial( "sprites/obj_icons/icon_obj_cap_red" );
	PrecacheMaterial( "sprites/obj_icons/icon_obj_cap_red_up" );
	PrecacheMaterial( "VGUI/flagtime_empty" );
	PrecacheMaterial( "VGUI/flagtime_full" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFObjectiveResource::~C_TFObjectiveResource()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *C_TFObjectiveResource::GetGameSpecificCPCappingSwipe( int index_, int iCappingTeam )
{
	Assert( index_ < m_iNumControlPoints );
	Assert( iCappingTeam != TEAM_UNASSIGNED );

	if ( iCappingTeam == TF_TEAM_RED )
		return "sprites/obj_icons/icon_obj_cap_red";

	return "sprites/obj_icons/icon_obj_cap_blu";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *C_TFObjectiveResource::GetGameSpecificCPBarFG( int index_, int iOwningTeam )
{
	Assert( index_ < m_iNumControlPoints );

	if ( iOwningTeam == TF_TEAM_RED )
		return "progress_bar_red";

	if ( iOwningTeam == TF_TEAM_BLUE )
		return "progress_bar_blu";

	return "progress_bar";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *C_TFObjectiveResource::GetGameSpecificCPBarBG( int index_, int iCappingTeam )
{
	Assert( index_ < m_iNumControlPoints );
	Assert( iCappingTeam != TEAM_UNASSIGNED );

	if ( iCappingTeam == TF_TEAM_RED )
		return "progress_bar_red";

	return "progress_bar_blu";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFObjectiveResource::SetCappingTeam( int index_, int team )
{
	//Display warning that someone is capping our point.
	//Only do this at the start of a cap and if WE own the point.
	//Also don't warn on a point that will do a "Last Point cap" warning.
	if ( GetNumControlPoints() > 0 && GetCapWarningLevel( index_ ) == CP_WARN_NORMAL && GetCPCapPercentage( index_ ) == 0.0f && team != TEAM_UNASSIGNED )
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pLocalPlayer )
		{
			if ( pLocalPlayer->GetTeamNumber() != team )
			{
				CLocalPlayerFilter filter;
				if ( GetOwningTeam( index_ ) != TEAM_UNASSIGNED )
				{
					C_BaseEntity::EmitSound( filter, -1, "Announcer.ControlPointContested" );
				}
				else
				{
					C_BaseEntity::EmitSound( filter, -1, "Announcer.ControlPointContested_Neutral" );
				}
			}
		}
	}

	BaseClass::SetCappingTeam( index_, team );
}

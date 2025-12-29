//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: DOD's objective resource, transmits all objective states to players
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_OBJECTIVE_RESOURCE_H
#define TF_OBJECTIVE_RESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "team_objectiveresource.h"

class CTFObjectiveResource : public CBaseTeamObjectiveResource
{
	DECLARE_CLASS( CTFObjectiveResource, CBaseTeamObjectiveResource );
public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CTFObjectiveResource();
	~CTFObjectiveResource();

	virtual void Spawn( void );


	string_t GetTeleporterString() const { return m_teleporterString; }

private:

	string_t m_teleporterString;
};

inline CTFObjectiveResource *TFObjectiveResource()
{
	return static_cast< CTFObjectiveResource *>( g_pObjectiveResource );
}

#endif	// TF_OBJECTIVE_RESOURCE_H


//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_TRIGGERS_H
#define TF_TRIGGERS_H
#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"

//-----------------------------------------------------------------------------
// Purpose: Trigger that allows the map-maker to force the spawn time of players that die inside
//-----------------------------------------------------------------------------
DECLARE_AUTO_LIST( ITriggerPlayerRespawnOverride );
class CTriggerPlayerRespawnOverride : public CTriggerMultiple, public ITriggerPlayerRespawnOverride
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS( CTriggerPlayerRespawnOverride, CTriggerMultiple );
	DECLARE_AUTO_LIST( ITriggerPlayerRespawnOverride );

	void InputSetRespawnTime( inputdata_t &inputdata ) { m_flRespawnTime = inputdata.value.Float(); }
	void InputSetRespawnName( inputdata_t &inputdata ) { m_strRespawnEnt = inputdata.value.StringID(); }
	float GetRespawnTime( void ) { return m_flRespawnTime; }
	string_t GetRespawnName( void ) { return m_strRespawnEnt; }

private:
	float m_flRespawnTime;
	string_t m_strRespawnEnt;
};

#endif // TF_TRIGGERS_H

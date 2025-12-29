//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Team Fortress specific special triggers
//
//===========================================================================//

#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "entityapi.h"
#include "entitylist.h"
#include "saverestore_utlvector.h"
#include "tf_player.h"
#include "triggers.h"
#include "tf_triggers.h"
#include "doors.h"
#include "trigger_area_capture.h"
#include "particle_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Trigger that controls door speed by specified time
//-----------------------------------------------------------------------------
class CTriggerTimerDoor : public CTriggerAreaCapture
{
public:
	DECLARE_CLASS( CTriggerTimerDoor, CTriggerAreaCapture );
	DECLARE_DATADESC();

	virtual void Spawn( void ) OVERRIDE;
	virtual void StartTouch( CBaseEntity *pOther ) OVERRIDE;

	virtual void OnStartCapture( int iTeam ) OVERRIDE;
	virtual void OnEndCapture( int iTeam ) OVERRIDE;

protected:
	virtual bool CaptureModeScalesWithPlayers() const OVERRIDE { return false; }

private:
	CHandle<CBaseDoor>	m_hDoor;	//the door that we are linked to!

	string_t m_iszDoorName;
};

BEGIN_DATADESC( CTriggerTimerDoor )
	DEFINE_KEYFIELD( m_iszDoorName, FIELD_STRING, "door_name" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_timer_door, CTriggerTimerDoor );

#define GATE_THINK_TIME 0.1f


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerTimerDoor::Spawn( void )
{
	BaseClass::Spawn();
	InitTrigger();
}

//-----------------------------------------------------------------------------
// Purpose: Bot enters the trigger, open the door
//-----------------------------------------------------------------------------
void CTriggerTimerDoor::StartTouch( CBaseEntity *pOther )
{
	if ( m_bDisabled )
		return;

	if (!PassesTriggerFilters(pOther))
		return;

	if ( !m_hDoor )
	{
		m_hDoor = dynamic_cast< CBaseDoor* >( gEntList.FindEntityByName(NULL, m_iszDoorName ) );
		if ( !m_hDoor )
		{
			Warning( "trigger_bot_gate failed to find \"%s\" door entity", STRING( m_iszDoorName ) );
			return;
		}
		else
		{
			float flDoorTravelDistance = ( m_hDoor->m_vecPosition2 - m_hDoor->m_vecPosition1 ).Length();
			m_hDoor->m_flSpeed = flDoorTravelDistance / GetCapTime();
		}
	}

	BaseClass::StartTouch( pOther );
}


//-----------------------------------------------------------------------------
// Purpose: Bot starts opening the door
//-----------------------------------------------------------------------------
void CTriggerTimerDoor::OnStartCapture( int iTeam )
{
	BaseClass::OnStartCapture( iTeam );
}


//-----------------------------------------------------------------------------
// Purpose: Bot finishes opening the door
//-----------------------------------------------------------------------------
void CTriggerTimerDoor::OnEndCapture( int iTeam )
{
	BaseClass::OnEndCapture( iTeam );
}

//-----------------------------------------------------------------------------
// Purpose: Trigger that adds a condition to players
//-----------------------------------------------------------------------------
class CTriggerAddTFPlayerCondition : public CBaseTrigger
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS( CTriggerAddTFPlayerCondition, CBaseTrigger );

	virtual void Spawn( void );

	virtual void StartTouch( CBaseEntity *pOther );
	virtual void EndTouch( CBaseEntity *pOther );

private:
	ETFCond m_nCondition;
	
	float m_flDuration;
};

BEGIN_DATADESC( CTriggerAddTFPlayerCondition )
	DEFINE_KEYFIELD( m_nCondition, FIELD_INTEGER, "condition" ),
	DEFINE_KEYFIELD( m_flDuration, FIELD_FLOAT, "duration" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_add_tf_player_condition, CTriggerAddTFPlayerCondition );

void CTriggerAddTFPlayerCondition::Spawn()
{
	BaseClass::Spawn();
	InitTrigger();
}


void CTriggerAddTFPlayerCondition::StartTouch( CBaseEntity *pOther )
{
	if ( m_bDisabled )
	{
		return;
	}

	if ( !PassesTriggerFilters(pOther) )
	{
		return;
	}

	if ( !pOther->IsPlayer() )
	{
		return;
	}

	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( !pPlayer )
	{
		return;
	}

	if ( m_nCondition != TF_COND_INVALID )
	{
		pPlayer->m_Shared.AddCond( m_nCondition, m_flDuration );
		BaseClass::StartTouch( pOther );
	}
	else
	{
		Warning( "Invalid Condition ID [%d] in trigger %s\n", m_nCondition, GetEntityName().ToCStr() );
	}
}

void CTriggerAddTFPlayerCondition::EndTouch( CBaseEntity *pOther )
{
	BaseClass::EndTouch( pOther );

	if ( m_flDuration != PERMANENT_CONDITION )
		return;

	if ( m_bDisabled )
		return;

	if ( !PassesTriggerFilters(pOther) )
		return;

	if ( !pOther->IsPlayer() )
		return;

	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( !pPlayer )
		return;

	if ( m_nCondition != TF_COND_INVALID )
	{
		pPlayer->m_Shared.RemoveCond( m_nCondition );
	}
	else
	{
		Warning( "Invalid Condition ID [%d] in trigger %s\n", m_nCondition, GetEntityName().ToCStr() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: CTriggerPlayerRespawnOverride
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CTriggerPlayerRespawnOverride )
	DEFINE_KEYFIELD( m_flRespawnTime, FIELD_FLOAT, "RespawnTime" ),
	DEFINE_KEYFIELD( m_strRespawnEnt, FIELD_STRING, "RespawnName" ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetRespawnTime", InputSetRespawnTime ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetRespawnName", InputSetRespawnName ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_player_respawn_override, CTriggerPlayerRespawnOverride );

IMPLEMENT_AUTO_LIST( ITriggerPlayerRespawnOverride );

//-----------------------------------------------------------------------------
// Purpose: Trigger that ignites players
//-----------------------------------------------------------------------------
class CTriggerIgnite : public CBaseTrigger
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS( CTriggerIgnite, CBaseTrigger );

	CTriggerIgnite();

	virtual void Spawn( void );
	virtual void Precache( void );

	virtual void StartTouch( CBaseEntity *pOther );

	void BurnThink();

private:
	void IgniteEntity( CBaseEntity *pOther );
	int BurnEntities();

	float m_flDamagePercentPerSecond;
	string_t m_iszIgniteParticleName;
	string_t m_iszIgniteSoundName;

	float m_flLastBurnTime;
};

BEGIN_DATADESC( CTriggerIgnite )
	DEFINE_KEYFIELD( m_flDamagePercentPerSecond,	FIELD_FLOAT,	"damage_percent_per_second" ),
	DEFINE_KEYFIELD( m_iszIgniteParticleName,		FIELD_STRING,	"ignite_particle_name" ),
	DEFINE_KEYFIELD( m_iszIgniteSoundName,			FIELD_STRING,	"ignite_sound_name" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_ignite, CTriggerIgnite );

#define BURN_INTERVAL 0.1f


CTriggerIgnite::CTriggerIgnite()
{
	m_flDamagePercentPerSecond = 10.f;
	m_flLastBurnTime = 0.f;
}


void CTriggerIgnite::Spawn()
{
	BaseClass::Spawn();
	InitTrigger();

	SetNextThink( TICK_NEVER_THINK );
	SetThink( NULL );
}


void CTriggerIgnite::Precache( void )
{
	BaseClass::Precache();

	const char *pszParticleName = STRING( m_iszIgniteParticleName );
	if ( pszParticleName && *pszParticleName )
	{
		PrecacheParticleSystem( pszParticleName );
	}

	const char *pszSoundName = STRING( m_iszIgniteSoundName );
	if ( pszSoundName && *pszSoundName )
	{
		PrecacheScriptSound( pszSoundName );
	}
}


void CTriggerIgnite::BurnThink()
{
	if ( BurnEntities() > 0 )
	{
		SetNextThink( gpGlobals->curtime + BURN_INTERVAL );
	}
	else
	{
		SetThink( NULL );
	}
}


void CTriggerIgnite::StartTouch( CBaseEntity *pOther )
{
	if ( m_bDisabled )
	{
		return;
	}

	if ( !PassesTriggerFilters(pOther) )
	{
		return;
	}

	IgniteEntity( pOther );

	BaseClass::StartTouch( pOther );

	if ( m_pfnThink == NULL )
	{
		m_flLastBurnTime = gpGlobals->curtime;
		SetThink( &CTriggerIgnite::BurnThink );
		SetNextThink( gpGlobals->curtime + BURN_INTERVAL );
	}
}


void CTriggerIgnite::IgniteEntity( CBaseEntity *pOther )
{
	Vector vecEffectPos = pOther->GetAbsOrigin();
	const char *pszParticleName = STRING( m_iszIgniteParticleName );
	if ( pszParticleName && *pszParticleName )
	{
		DispatchParticleEffect( pszParticleName, vecEffectPos, vec3_angle );
	}

	const char *pszSoundName = STRING( m_iszIgniteSoundName );
	if ( pszSoundName && *pszSoundName )
	{
		CSoundParameters params;
		if ( CBaseEntity::GetParametersForSound( pszSoundName, params, NULL ) )
		{
			CPASAttenuationFilter soundFilter( vecEffectPos, params.soundlevel );
			EmitSound_t ep( params );
			ep.m_pOrigin = &vecEffectPos;
			EmitSound( soundFilter, entindex(), ep );
		}
	}

	if ( pOther->IsPlayer() )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pOther );
		if ( pTFPlayer && !pTFPlayer->m_Shared.IsInvulnerable() && !pTFPlayer->m_Shared.InCond( TF_COND_BURNING ) )
		{
			pTFPlayer->m_Shared.SelfBurn();
		}
	}
	else
	{
		UTIL_Remove( pOther );
	}
}


int CTriggerIgnite::BurnEntities()
{
	if ( m_hTouchingEntities.IsEmpty() )
		return 0;

	float flDT = gpGlobals->curtime - m_flLastBurnTime;
	m_flLastBurnTime = gpGlobals->curtime;
	int nBurn = 0;
	FOR_EACH_VEC( m_hTouchingEntities, i )
	{
		CBaseEntity *pEnt = m_hTouchingEntities[i];
		if ( pEnt && pEnt->IsPlayer() )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( pEnt );
			if ( pTFPlayer )
			{
				float flDamageScale = m_flDamagePercentPerSecond * 0.01f;
				float flDamage = flDT * flDamageScale * pTFPlayer->GetMaxHealth();
				CTakeDamageInfo info( this, this, flDamage, DMG_BURN );
				if ( !pTFPlayer->m_Shared.IsInvulnerable() && !pTFPlayer->m_Shared.InCond( TF_COND_BURNING ) ) // if player enters trigger invuln, we need to ignite them when it wears off. We also don't want to ignite an already burning player
				{
					IgniteEntity( pTFPlayer );
				}

				pTFPlayer->TakeDamage( info );
				nBurn++;
			}
		}
	}

	return nBurn;
}


//-----------------------------------------------------------------------------
// Purpose: Trigger that spawn particles on entities
//-----------------------------------------------------------------------------
class CTriggerParticle : public CBaseTrigger
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS( CTriggerParticle, CBaseTrigger );

	CTriggerParticle();

	virtual void Spawn( void );
	virtual void Precache( void );

	virtual void StartTouch( CBaseEntity *pOther );

private:
	string_t m_iszParticleName;
	string_t m_iszAttachmentName;
	ParticleAttachment_t m_nAttachType;
};

BEGIN_DATADESC( CTriggerParticle )
	DEFINE_KEYFIELD( m_iszParticleName,		FIELD_STRING,	"particle_name" ),
	DEFINE_KEYFIELD( m_iszAttachmentName,	FIELD_STRING,	"attachment_name" ),
	DEFINE_KEYFIELD( m_nAttachType,			FIELD_INTEGER,	"attachment_type" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_particle, CTriggerParticle );


CTriggerParticle::CTriggerParticle()
{
	m_nAttachType = PATTACH_ABSORIGIN;
}


void CTriggerParticle::Spawn()
{
	BaseClass::Spawn();
	InitTrigger();

	SetNextThink( TICK_NEVER_THINK );
	SetThink( NULL );
}


void CTriggerParticle::Precache( void )
{
	BaseClass::Precache();

	const char *pszParticleName = STRING( m_iszParticleName );
	if ( pszParticleName && *pszParticleName )
	{
		PrecacheParticleSystem( pszParticleName );
	}
}


void CTriggerParticle::StartTouch( CBaseEntity *pOther )
{
	if ( m_bDisabled )
	{
		return;
	}

	if ( !PassesTriggerFilters(pOther) )
	{
		return;
	}

	BaseClass::StartTouch( pOther );

	const char *pszParticleName = STRING( m_iszParticleName );
	const char *pszAttachmentName = STRING( m_iszAttachmentName );
	int iAttachment = -1;
	if ( pszAttachmentName && *pszAttachmentName )
	{
		iAttachment = pOther->GetBaseAnimating()->LookupAttachment( pszAttachmentName );
	}

	DispatchParticleEffect( pszParticleName, m_nAttachType, pOther, iAttachment );
}


class CTriggerRemoveTFPlayerCondition : public CBaseTrigger
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS( CTriggerRemoveTFPlayerCondition, CBaseTrigger );

	virtual void Spawn( void );

	virtual void StartTouch( CBaseEntity *pOther );

private:
	ETFCond m_nCondition;
};


BEGIN_DATADESC( CTriggerRemoveTFPlayerCondition )
	DEFINE_KEYFIELD( m_nCondition, FIELD_INTEGER, "condition" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_remove_tf_player_condition, CTriggerRemoveTFPlayerCondition );


void CTriggerRemoveTFPlayerCondition::Spawn()
{
	BaseClass::Spawn();
	InitTrigger();
}


void CTriggerRemoveTFPlayerCondition::StartTouch( CBaseEntity *pOther )
{
	if ( m_bDisabled )
	{
		return;
	}

	if ( !PassesTriggerFilters(pOther) )
	{
		return;
	}

	if ( !pOther->IsPlayer() )
	{
		return;
	}

	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( !pPlayer )
	{
		return;
	}

	if ( m_nCondition != TF_COND_INVALID )
	{
		pPlayer->m_Shared.RemoveCond( m_nCondition );
	}
	else
	{
		pPlayer->m_Shared.RemoveAllCond();
	}

	BaseClass::StartTouch( pOther );
}
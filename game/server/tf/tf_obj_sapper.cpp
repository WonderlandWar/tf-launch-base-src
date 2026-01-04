//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Slowly damages the object it's attached to
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tf_player.h"
#include "tf_team.h"
#include "tf_gamerules.h"
#include "tf_obj.h"
#include "tf_obj_sentrygun.h"
#include "tf_obj_sapper.h"
#include "ndebugoverlay.h"
#include "tf_gamestats.h"
#include "tf_obj_teleporter.h"
#include "tf_weapon_builder.h"
#include "tf_fx.h"

// ------------------------------------------------------------------------ //

#define SAPPER_MINS				Vector(0, 0, 0)
#define SAPPER_MAXS				Vector(1, 1, 1)

const char * g_sapperModel = "models/buildables/sapper_placed.mdl";
const char * g_sapperPlacementModel = "models/buildables/sapper_placement.mdl";

BEGIN_DATADESC( CObjectSapper )
	DEFINE_THINKFUNC( SapperThink ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CObjectSapper, DT_ObjectSapper)
END_SEND_TABLE();

LINK_ENTITY_TO_CLASS(obj_attachment_sapper, CObjectSapper);
PRECACHE_REGISTER(obj_attachment_sapper);

ConVar	obj_sapper_amount( "obj_sapper_amount", "25", FCVAR_NONE, "Amount of health inflicted by a Sapper object per second" );

#define SAPPER_THINK_CONTEXT		"SapperThink"
#define SAPPER_REMOVE_DISABLE_TIME			0.5f

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CObjectSapper::CObjectSapper()
{
	m_iHealth = GetBaseHealth();
	SetMaxHealth( m_iHealth );

	UseClientSideAnimation();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSapper::UpdateOnRemove()
{
	StopSound( "Weapon_Sapper.Timer" );

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSapper::Spawn()
{
	SetModel( GetSapperModelName( SAPPER_MODEL_PLACEMENT ) );

	m_takedamage = DAMAGE_YES;
	m_iHealth = GetBaseHealth();

	SetType( OBJ_ATTACHMENT_SAPPER );

	BaseClass::Spawn();

	Vector mins = SAPPER_MINS;
	Vector maxs = SAPPER_MAXS;
	CollisionProp()->SetSurroundingBoundsType( USE_SPECIFIED_BOUNDS, &mins, &maxs );

	int nFlags = m_fObjectFlags | OF_ALLOW_REPEAT_PLACEMENT;

	m_fObjectFlags.Set( nFlags );

	SetSolid( SOLID_NONE );

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSapper::Precache()
{
	int iModelIndex = PrecacheModel( GetSapperModelName( SAPPER_MODEL_PLACED ) );
	PrecacheGibsForModel( iModelIndex );
	PrecacheModel( GetSapperModelName( SAPPER_MODEL_PLACEMENT ) );

	PrecacheScriptSound( "Weapon_Sapper.Plant" );
	PrecacheScriptSound( "Weapon_Sapper.Timer" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSapper::FinishedBuilding( void )
{
	BaseClass::FinishedBuilding();

	CBaseEntity *pEntity =  m_hBuiltOnEntity.Get();
	if ( pEntity )
	{
		if ( GetParentObject() )
		{
			GetParentObject()->OnAddSapper();

			CBaseObject *pObject = dynamic_cast<CBaseObject *>( m_hBuiltOnEntity.Get() );
			if ( pObject )
			{
				if ( GetBuilder() && pObject->GetBuilder() )
				{
					IGameEvent * event = gameeventmanager->CreateEvent( "player_sapped_object" );
					if ( event )
					{
						event->SetInt( "userid", GetBuilder()->GetUserID() );
						event->SetInt( "ownerid", pObject->GetBuilder()->GetUserID() );
						event->SetInt( "object", pObject->ObjectType() );
						event->SetInt( "sapperid", entindex() );

						gameeventmanager->FireEvent( event );
					}
				}
			}
		}
	}

	EmitSound( "Weapon_Sapper.Plant" );
	EmitSound( "Weapon_Sapper.Timer" );	// start looping "Weapon_Sapper.Timer", killed when we die

	m_flSapperDamageAccumulator = 0;
	m_flLastThinkTime = gpGlobals->curtime;

	SetContextThink( &CObjectSapper::SapperThink, gpGlobals->curtime + 0.1, SAPPER_THINK_CONTEXT );
}

//-----------------------------------------------------------------------------
// Purpose: Change our model based on the object we are attaching to
//-----------------------------------------------------------------------------
void CObjectSapper::SetupAttachedVersion( void )
{
	if ( !IsParentValid() )
		return;

	if ( IsPlacing() )
	{
		CBaseEntity *pEntity = m_hBuiltOnEntity.Get();
		if ( pEntity )
		{
			SetModel( GetSapperModelName( SAPPER_MODEL_PLACEMENT ) );
		}
	}

	BaseClass::SetupAttachedVersion();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSapper::OnGoActive( void )
{
	if ( !IsParentValid() )
		return;

	// set new model
	CBaseEntity *pEntity = m_hBuiltOnEntity.Get();

	if ( pEntity )
	{
		SetModel( GetSapperModelName( SAPPER_MODEL_PLACED ) );
	}

	UTIL_SetSize( this, SAPPER_MINS, SAPPER_MAXS );
	SetSolid( SOLID_NONE );

	BaseClass::OnGoActive();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CObjectSapper::IsParentValid( void )
{
	bool bValid = false;

	CBaseEntity *pEntity = m_hBuiltOnEntity.Get();
	if ( pEntity )
	{
		if ( pEntity->IsPlayer() )	// sapped bot in MvM mode
		{
			bValid = true;
		}
		else
		{
			CBaseObject *pObject = dynamic_cast<CBaseObject *>( pEntity ); 
			if ( pObject )
			{
				bValid = true;
			}
		}
	}

	if ( !bValid )
	{
		DestroyObject();
	}

	return bValid;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSapper::DetachObjectFromObject( void )
{
	CBaseObject *pParent = GetParentObject();
	if ( pParent )
	{
		pParent->OnRemoveSapper();

	}

	BaseClass::DetachObjectFromObject();
}

//-----------------------------------------------------------------------------
const char* CObjectSapper::GetSapperModelName( SapperModel_t nModel )
{
	if ( nModel >= SAPPER_MODEL_PLACEMENT )
		return g_sapperPlacementModel;

	return g_sapperModel;
}

//-----------------------------------------------------------------------------
// Purpose: Slowly destroy the object I'm attached to
//-----------------------------------------------------------------------------
void CObjectSapper::SapperThink( void )
{
	if ( !GetTeam() )
		return;

	bool bThink = true;

	CBaseEntity *pEntity = m_hBuiltOnEntity.Get();
	if ( pEntity )
	{
		if ( pEntity->IsPlayer() )	// sapping bots in MvM mode
		{
			bool bDestroy = false;

			CTFPlayer *pTFOwner = ToTFPlayer( m_hBuiltOnEntity.Get() );
			CTFPlayer *pBuilder = GetBuilder();
			if ( !pBuilder || !pTFOwner || ( pTFOwner && !pTFOwner->IsAlive() ) )
			{		
				bDestroy = true;
			}

			if ( bDestroy )
			{
				DestroyObject();
				bThink = false;
				return;
			}
		}
		else
		{
			CBaseObject *pObject = GetParentObject();
			if ( !pObject )
			{
				DestroyObject();
				bThink = false;
				return;
			}

			// Don't bring objects back from the dead
			if ( !pObject->IsAlive() || pObject->IsDying() )
				return;

			// how much damage to give this think?
			float flTimeSinceLastThink = gpGlobals->curtime - m_flLastThinkTime;
			float flDamageToGive = ( flTimeSinceLastThink ) * obj_sapper_amount.GetFloat();

			// add to accumulator
			m_flSapperDamageAccumulator += flDamageToGive;

			int iDamage = (int)m_flSapperDamageAccumulator;

			m_flSapperDamageAccumulator -= iDamage;

			CTakeDamageInfo info;
			info.SetDamage( iDamage );
			info.SetAttacker( this );
			info.SetInflictor( this );
			info.SetDamageType( DMG_CRUSH );
			info.SetDamageCustom( 0 );

			pObject->TakeDamage( info );
		}
	}

	if ( bThink )
	{
		SetNextThink( gpGlobals->curtime + 0.1f, SAPPER_THINK_CONTEXT );
	}

	m_flLastThinkTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CObjectSapper::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( info.GetDamageCustom() != TF_DMG_WRENCH_FIX )
	{
		return 0;
	}

	// Is the damage from something other than another sapper? (which might be on our matching teleporter)
	if ( !( info.GetDamageType() & DMG_FROM_OTHER_SAPPER ) )
	{
		if ( GetParentObject() )
		{
			CTakeDamageInfo localDamageInfo = info;
			localDamageInfo.AddDamageType( DMG_FROM_OTHER_SAPPER );

			// If there's a matching teleporter with a sapper then have that sapper take damage, too.
			CObjectTeleporter *pParentTeleporter = dynamic_cast< CObjectTeleporter * >( GetParentObject() );
			if ( pParentTeleporter )
			{
				// GetMatchingTeleporter is set when a matching teleporter is ACTIVE
				// if we don't find the cache matching teleporter, try to find with a more expensive FindMatch func
				CObjectTeleporter *pMatchingTeleporter = pParentTeleporter->GetMatchingTeleporter() ? pParentTeleporter->GetMatchingTeleporter() : pParentTeleporter->FindMatch();
				if ( pMatchingTeleporter && pMatchingTeleporter->HasSapper() )
				{
					// Do damage to any attached buildings
					IHasBuildPoints *pBPInterface = dynamic_cast< IHasBuildPoints * >( pMatchingTeleporter );
					int iNumObjects = pBPInterface->GetNumObjectsOnMe();
					for ( int iPoint = 0 ; iPoint < iNumObjects ; iPoint++ )
					{
						CBaseObject *pObject = pMatchingTeleporter->GetBuildPointObject( iPoint );
						if ( pObject && pObject->IsHostileUpgrade() )
						{
							pObject->TakeDamage( localDamageInfo );
						}
					}
				}
			}
		}
	}

	return BaseClass::OnTakeDamage( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSapper::Killed( const CTakeDamageInfo &info )
{
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CTFPlayer *pScorer = ToTFPlayer( TFGameRules()->GetDeathScorer( pKiller, pInflictor, this ) );

	// We don't own the building we removed the sapper from
	if ( pScorer && GetParentObject() && GetParentObject()->GetOwner() != pScorer )
	{
		if ( pScorer->IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			pScorer->AwardAchievement( ACHIEVEMENT_TF_ENGINEER_DESTROY_SAPPERS, 1 );
		}
	}

	CBaseObject *pParent = GetParentObject();
	if ( pParent )
	{
		pParent->SetPlasmaDisabled( SAPPER_REMOVE_DISABLE_TIME );
	}

	BaseClass::Killed( info );
}

int CObjectSapper::GetBaseHealth( void )
{
	return SAPPER_MAX_HEALTH;
}

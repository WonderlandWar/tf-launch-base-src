//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Engineer's Dispenser
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"

#include "tf_obj_dispenser.h"
#include "engine/IEngineSound.h"
#include "tf_player.h"
#include "tf_team.h"
#include "tf_gamerules.h"
#include "vguiscreen.h"
#include "world.h"
#include "explode.h"
#include "tf_gamestats.h"
#include "tf_fx.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define DISPENSER_MINS			Vector( -20, -20,  0 )
#define DISPENSER_MAXS			Vector(  20,  20, 55 )	// tweak me

#define DISPENSER_TRIGGER_MINS			Vector( -70, -70,  0 )
#define DISPENSER_TRIGGER_MAXS			Vector(  70,  70, 50 )	// tweak me

#define REFILL_CONTEXT			"RefillContext"
#define DISPENSE_CONTEXT		"DispenseContext"

ConVar tf_cart_spell_drop_rate( "tf_cart_spell_drop_rate", "4" );
ConVar tf_cart_duck_drop_rate( "tf_cart_duck_drop_rate", "10", FCVAR_DEVELOPMENTONLY );
ConVar tf_cart_soul_drop_rate( "tf_cart_soul_drop_rate", "10", FCVAR_DEVELOPMENTONLY );

//-----------------------------------------------------------------------------
// Purpose: SendProxy that converts the Healing list UtlVector to entindices
//-----------------------------------------------------------------------------
void SendProxy_HealingList( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	CObjectDispenser *pDispenser = (CObjectDispenser*)pStruct;

	// If this assertion fails, then SendProxyArrayLength_HealingArray must have failed.
	Assert( iElement < pDispenser->m_hHealingTargets.Size() );

	CBaseEntity *pEnt = pDispenser->m_hHealingTargets[iElement].Get();
	EHANDLE hOther = pEnt;

	SendProxy_EHandleToInt( pProp, pStruct, &hOther, pOut, iElement, objectID );
}

int SendProxyArrayLength_HealingArray( const void *pStruct, int objectID )
{
	CObjectDispenser *pDispenser = (CObjectDispenser*)pStruct;
	return pDispenser->m_hHealingTargets.Count();
}

IMPLEMENT_SERVERCLASS_ST( CObjectDispenser, DT_ObjectDispenser )
	SendPropInt( SENDINFO( m_iAmmoMetal ), -1, SPROP_VARINT ),

	SendPropArray2( 
		SendProxyArrayLength_HealingArray,
		SendPropInt("healing_array_element", 0, SIZEOF_IGNORE, NUM_NETWORKED_EHANDLE_BITS, SPROP_UNSIGNED, SendProxy_HealingList), 
		MAX_PLAYERS, 
		0, 
		"healing_array"
		)

END_SEND_TABLE()

BEGIN_DATADESC( CObjectDispenser )
	DEFINE_THINKFUNC( RefillThink ),
	DEFINE_THINKFUNC( DispenseThink ),

	// key
	DEFINE_KEYFIELD( m_iszCustomTouchTrigger, FIELD_STRING, "touch_trigger" ),

END_DATADESC()

LINK_ENTITY_TO_CLASS(obj_dispenser, CObjectDispenser);
PRECACHE_REGISTER(obj_dispenser);

// How much ammo is given our per use
#define DISPENSER_DROP_METAL		40

LINK_ENTITY_TO_CLASS( dispenser_touch_trigger, CDispenserTouchTrigger );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CObjectDispenser::CObjectDispenser()
{
	int iHealth = GetBaseHealth();
	m_hTouchTrigger = NULL;
	SetMaxHealth( iHealth );
	SetHealth( iHealth );
	UseClientSideAnimation();

	m_hTouchingEntities.Purge();
	m_bUseGenerateMetalSound = true;
	m_bThrown = false;

	m_bPlayAmmoPickupSound = true;
	m_flPrevRadius = -1.f;

	SetType( OBJ_DISPENSER );
}

CObjectDispenser::~CObjectDispenser()
{
	if ( m_hTouchTrigger.Get() )
	{
		UTIL_Remove( m_hTouchTrigger );
	}

	ResetHealingTargets();

	StopSound( "Building_Dispenser.Idle" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectDispenser::DetonateObject( void )
{
	// We already dying?
	if ( m_bDying )
		return;

	BaseClass::DetonateObject();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectDispenser::Spawn()
{
	SetModel( GetPlacementModel() );

	SetTouch( &CObjectDispenser::Touch );

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectDispenser::FirstSpawn()
{
	SetSolid( SOLID_BBOX );

	UTIL_SetSize(this, DISPENSER_MINS, DISPENSER_MAXS );

	m_takedamage = DAMAGE_YES;
	m_iAmmoMetal = 0;

	int iHealth = GetBaseHealth();

	SetMaxHealth( iHealth );
	SetHealth( iHealth );

	BaseClass::FirstSpawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char* CObjectDispenser::GetBuildingModel( int iLevel )
{
	{
		switch ( iLevel )
		{
		case 1:
			return DISPENSER_MODEL_BUILDING;
			break;
		case 2:
			return DISPENSER_MODEL_BUILDING_LVL2;
			break;
		case 3:
			return DISPENSER_MODEL_BUILDING_LVL3;
			break;
		default:
			return DISPENSER_MODEL_BUILDING;
			break;
		}
	}

	Assert( 0 );
	return DISPENSER_MODEL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char* CObjectDispenser::GetFinishedModel( int iLevel )
{
	{
		switch ( iLevel )
		{
		case 1:
			return DISPENSER_MODEL;
			break;
		case 2:
			return DISPENSER_MODEL_LVL2;
			break;
		case 3:
			return DISPENSER_MODEL_LVL3;
			break;
		default:
			return DISPENSER_MODEL;
			break;
		}
	}

	Assert( 0 );
	return DISPENSER_MODEL;
}

const char* CObjectDispenser::GetPlacementModel()
{
	return /*IsMiniBuilding() ? MINI_DISPENSER_MODEL_PLACEMENT :*/ DISPENSER_MODEL_PLACEMENT;
}

void CObjectDispenser::StartPlacement( CTFPlayer *pPlayer )
{
	BaseClass::StartPlacement( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: Start building the object
//-----------------------------------------------------------------------------
bool CObjectDispenser::StartBuilding( CBaseEntity *pBuilder )
{
	SetStartBuildingModel();

	CreateBuildPoints();

	return BaseClass::StartBuilding( pBuilder );
}

void CObjectDispenser::SetStartBuildingModel( void )
{
	SetModel( GetBuildingModel( 1 ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectDispenser::SetModel( const char *pModel )
{
	BaseClass::SetModel( pModel );

		// Reset this after model change
		UTIL_SetSize( this,
			DISPENSER_MINS,
			DISPENSER_MAXS );
	ResetSequenceInfo();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectDispenser::InitializeMapPlacedObject( void )
{
	// make sure we are using an appropriate model so that the control panels are in the right place
	SetModel( GetFinishedModel( 1 ) );

	BaseClass::InitializeMapPlacedObject();
}

//-----------------------------------------------------------------------------
// Purpose: Finished building
//-----------------------------------------------------------------------------
void CObjectDispenser::OnGoActive( void )
{
	SetModel( GetFinishedModel( 1 ) );

	CreateBuildPoints();

	ReattachChildren();

	// Put some ammo in the Dispenser
	m_iAmmoMetal = 25;

	// Begin thinking
	SetContextThink( &CObjectDispenser::RefillThink, gpGlobals->curtime + 3, REFILL_CONTEXT );
	SetContextThink( &CObjectDispenser::DispenseThink, gpGlobals->curtime + 0.1, DISPENSE_CONTEXT );

	m_flNextAmmoDispense = gpGlobals->curtime + 0.5;

	float flRadius = GetDispenserRadius();
	
	if ( m_iszCustomTouchTrigger != NULL_STRING )
	{
		m_hTouchTrigger = dynamic_cast<CDispenserTouchTrigger *> ( gEntList.FindEntityByName( NULL, m_iszCustomTouchTrigger ) );

		if ( m_hTouchTrigger.Get() != NULL )
		{
			m_hTouchTrigger->SetOwnerEntity( this );	//owned
		}
	}

	if ( m_hTouchTrigger.Get() == NULL )
	{
		m_hTouchTrigger = CBaseEntity::Create( "dispenser_touch_trigger", GetAbsOrigin(), vec3_angle, this );
		UTIL_SetSize(m_hTouchTrigger.Get(), Vector(-flRadius,-flRadius,-flRadius), Vector(flRadius,flRadius,flRadius) );
		m_hTouchTrigger->SetSolid(SOLID_BBOX);
	}

	Assert( m_hTouchTrigger.Get() );

	BaseClass::OnGoActive();

	PlayActiveSound();
}

void CObjectDispenser::PlayActiveSound()
{
	EmitSound( "Building_Dispenser.Idle" );
}

//-----------------------------------------------------------------------------
// Spawn the vgui control screens on the object
//-----------------------------------------------------------------------------
void CObjectDispenser::GetControlPanelInfo( int nPanelIndex, const char *&pPanelName )
{
	// Panels 0 and 1 are both control panels for now
	if ( nPanelIndex == 0 || nPanelIndex == 1 )
	{
		if ( GetTeamNumber() == TF_TEAM_RED )
		{
			pPanelName = "screen_obj_dispenser_red";
		}
		else
		{
			pPanelName = "screen_obj_dispenser_blue";
		}
	}
	else
	{
		BaseClass::GetControlPanelInfo( nPanelIndex, pPanelName );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectDispenser::Precache()
{
	BaseClass::Precache();

	int iModelIndex;

	PrecacheModel( DISPENSER_MODEL_PLACEMENT );

	iModelIndex = PrecacheModel( DISPENSER_MODEL_BUILDING );
	PrecacheGibsForModel( iModelIndex );

	iModelIndex = PrecacheModel( DISPENSER_MODEL );
	PrecacheGibsForModel( iModelIndex );

	iModelIndex = PrecacheModel( DISPENSER_MODEL_BUILDING_LVL2 );
	PrecacheGibsForModel( iModelIndex );

	iModelIndex = PrecacheModel( DISPENSER_MODEL_LVL2 );
	PrecacheGibsForModel( iModelIndex );

	iModelIndex = PrecacheModel( DISPENSER_MODEL_BUILDING_LVL3 );
	PrecacheGibsForModel( iModelIndex );

	iModelIndex = PrecacheModel( DISPENSER_MODEL_LVL3 );
	PrecacheGibsForModel( iModelIndex );


	PrecacheVGuiScreen( "screen_obj_dispenser_blue" );
	PrecacheVGuiScreen( "screen_obj_dispenser_red" );

	PrecacheScriptSound( "Building_Dispenser.Idle" );
	PrecacheScriptSound( "Building_Dispenser.GenerateMetal" );
	PrecacheScriptSound( "Building_Dispenser.Heal" );

	PrecacheParticleSystem( "dispenser_heal_red" );
	PrecacheParticleSystem( "dispenser_heal_blue" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CObjectDispenser::DispenseAmmo( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
		return false;

	int iTotalPickedUp = 0;
	int iAmmoToAdd = 0;

	float flAmmoRate = 0.2;

	// primary
	iAmmoToAdd = (int)( pPlayer->GetMaxAmmo( TF_AMMO_PRIMARY ) * flAmmoRate );
	iTotalPickedUp += pPlayer->GiveAmmo( iAmmoToAdd, TF_AMMO_PRIMARY, !m_bPlayAmmoPickupSound, kAmmoSource_DispenserOrCart );
	
	// secondary
	iAmmoToAdd = (int)( pPlayer->GetMaxAmmo( TF_AMMO_SECONDARY ) * flAmmoRate );
	iTotalPickedUp += pPlayer->GiveAmmo( iAmmoToAdd, TF_AMMO_SECONDARY, !m_bPlayAmmoPickupSound, kAmmoSource_DispenserOrCart );

	// metal
	iTotalPickedUp += DispenseMetal( pPlayer );
	
	if ( iTotalPickedUp > 0 )
	{
		if ( m_bPlayAmmoPickupSound )
		{
			EmitSound( "BaseCombatCharacter.AmmoPickup" );
		}

		return true;
	}

	// return false if we didn't pick up anything
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CObjectDispenser::DispenseMetal( CTFPlayer *pPlayer )
{
	int iMetalToGive = DISPENSER_DROP_METAL;
	int iMetal = pPlayer->GiveAmmo( Min( m_iAmmoMetal.Get(), iMetalToGive ), TF_AMMO_METAL, false, kAmmoSource_DispenserOrCart );
	m_iAmmoMetal -= iMetal;

	return iMetal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectDispenser::RefillThink( void )
{
	SetContextThink( &CObjectDispenser::RefillThink, gpGlobals->curtime + 6, REFILL_CONTEXT );

	if ( IsDisabled() )
	{
		return;
	}

	// Auto-refill half the amount as tfc, but twice as often
	if ( m_iAmmoMetal < DISPENSER_MAX_METAL_AMMO )
	{
		int iMetal = (DISPENSER_MAX_METAL_AMMO * 0.1);

		m_iAmmoMetal = Min( m_iAmmoMetal + iMetal, DISPENSER_MAX_METAL_AMMO );

		if ( m_bUseGenerateMetalSound )
		{
			EmitSound( "Building_Dispenser.GenerateMetal" );
		}
	}
}


//-----------------------------------------------------------------------------
// Generate ammo over time
//-----------------------------------------------------------------------------
void CObjectDispenser::DispenseThink( void )
{
	if ( IsDisabled() )
	{
		// Don't heal or dispense ammo
		SetContextThink( &CObjectDispenser::DispenseThink, gpGlobals->curtime + 0.1, DISPENSE_CONTEXT );

		// stop healing everyone
		ResetHealingTargets();
		return;
	}
	
	if ( GetOwner() )
	{
		float flRadius = GetDispenserRadius();
		if ( ( flRadius != m_flPrevRadius ) && m_hTouchTrigger.Get() )
		{
			m_hTouchTrigger->SetAbsOrigin( WorldSpaceCenter() );
			UTIL_SetSize( m_hTouchTrigger.Get(), Vector( -flRadius, -flRadius, -flRadius ), Vector( flRadius, flRadius, flRadius ) );
			m_flPrevRadius = flRadius;
		}
	}	

	// time to dispense ammo?
	bool bDispenseAmmo = ( m_flNextAmmoDispense <= gpGlobals->curtime );
	bool bPlayerReceivedAmmo = false;

	// for each player in touching list
	int iSize = m_hTouchingEntities.Count();
	bool bIsAnyTeammateTouching = false;

	if ( m_hTouchTrigger )
	{
		for ( int i = iSize-1; i >= 0; i-- )
		{
			EHANDLE hOther = m_hTouchingEntities[i];

			CBaseEntity *pEnt = hOther.Get();
			if ( !pEnt )
				continue;

			// stop touching and healing a dead entity, or one that is grossly out of range (EndTouch() can be flakey)
			float flDistSqr = ( m_hTouchTrigger->WorldSpaceCenter() - pEnt->WorldSpaceCenter() ).LengthSqr();
			Vector vecMins, vecMaxs;
			m_hTouchTrigger->GetCollideable()->WorldSpaceSurroundingBounds( &vecMins, &vecMaxs );
			float flDoubleRadiusSqr = ( vecMaxs - vecMins ).LengthSqr();
			if ( !pEnt->IsAlive() || ( flDistSqr > flDoubleRadiusSqr ) )
			{
				m_hTouchingEntities.FindAndRemove( hOther );
				StopHealing( pEnt );
				continue;
			}

			bIsAnyTeammateTouching |= ( pEnt->IsPlayer() && pEnt->GetTeamNumber() == GetTeamNumber() );

			bool bHealingTarget = IsHealingTarget( pEnt );
			bool bValidHealTarget = CouldHealTarget( pEnt );

			// handle healing
			if ( bHealingTarget && !bValidHealTarget )
			{
				// if we can't see them, remove them from healing list
				// does nothing if we are not healing them already
				StopHealing( pEnt );
			}
			else if ( !bHealingTarget && bValidHealTarget )
			{
				// if we can see them, add to healing list	
				// does nothing if we are healing them already
				StartHealing( pEnt );
			}	

			// handle ammo
			if ( bDispenseAmmo && bValidHealTarget )
			{
				if ( DispenseAmmo( ToTFPlayer( pEnt ) ) )
				{
					bPlayerReceivedAmmo = true;
				}
			}
		}
	}

	if ( bDispenseAmmo )
	{
		// Try to dispense more often when no players are around so we 
		// give it as soon as possible when a new player shows up
		float flNextAmmoDelay = 1.0;
		m_flNextAmmoDispense = gpGlobals->curtime + ( bPlayerReceivedAmmo ? flNextAmmoDelay : 0.1 );
	}

	if ( bIsAnyTeammateTouching )
	{
		if ( !m_spellTimer.HasStarted() )
		{
			m_spellTimer.Start( tf_cart_spell_drop_rate.GetFloat() );
		}

		if ( !m_duckTimer.HasStarted() )
		{
			m_duckTimer.Start( tf_cart_duck_drop_rate.GetFloat() );
		}

		if ( !m_soulTimer.HasStarted() )
		{
			m_soulTimer.Start( tf_cart_soul_drop_rate.GetFloat() );
		}
	}
	else
	{
		m_spellTimer.Invalidate();
		m_duckTimer.Invalidate();
		m_soulTimer.Invalidate();
	}

	if ( m_spellTimer.HasStarted() && m_spellTimer.IsElapsed() )
	{
		m_spellTimer.Start( tf_cart_spell_drop_rate.GetFloat() );
		DropSpellPickup();
	}

	if ( m_duckTimer.HasStarted() && m_duckTimer.IsElapsed() )
	{
		m_duckTimer.Start( tf_cart_duck_drop_rate.GetFloat() );
		DropDuckPickup();
	}

	if ( m_soulTimer.HasStarted() && m_soulTimer.IsElapsed() )
	{
		m_soulTimer.Start( tf_cart_soul_drop_rate.GetFloat() );
		DispenseSouls();
	}

	SetContextThink( &CObjectDispenser::DispenseThink, gpGlobals->curtime + 0.1, DISPENSE_CONTEXT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectDispenser::StartTouch( CBaseEntity *pOther )
{
	// add to touching entities
	EHANDLE hOther = pOther;
	m_hTouchingEntities.AddToTail( hOther );

	if ( !IsBuilding() && !IsDisabled() && CouldHealTarget( pOther ) && !IsHealingTarget( pOther ) )
	{
		// try to start healing them
		StartHealing( pOther );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectDispenser::Touch( CBaseEntity *pOther )
{
	// We dont want to touch these
	if ( pOther->IsSolidFlagSet( FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS ) )
		return;

	// Handle hitting skybox (disappear).
	const trace_t *pTrace = &CBaseEntity::GetTouchTrace();
	if( pTrace->surface.flags & SURF_SKY )
	{
		UTIL_Remove( this );
		return;
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectDispenser::EndTouch( CBaseEntity *pOther )
{
	// remove from touching entities
	EHANDLE hOther = pOther;
	m_hTouchingEntities.FindAndRemove( hOther );

	// remove from healing list
	StopHealing( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectDispenser::ResetHealingTargets( void )
{
	// tell all the players we're not healing them anymore
	for ( int i = m_hHealingTargets.Count()-1 ; i >= 0 ; i-- )
	{
		EHANDLE hEnt = m_hHealingTargets[i];
		CBaseEntity *pOther = hEnt.Get();

		if ( pOther )
		{
			StopHealing( pOther );
		}
	}

}

//-----------------------------------------------------------------------------
// Purpose: Try to start healing this target
//-----------------------------------------------------------------------------
void CObjectDispenser::StartHealing( CBaseEntity *pOther )
{
	AddHealingTarget( pOther );

	CTFPlayer *pPlayer = ToTFPlayer( pOther );

	if ( pPlayer )
	{
		float flHealRate = 10.0;
		float flOverhealBonus = 1.f;
		pPlayer->m_Shared.Heal( this, flHealRate, flOverhealBonus, 1.f, true, GetBuilder() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Stop healing this target
//-----------------------------------------------------------------------------
void CObjectDispenser::StopHealing( CBaseEntity *pOther )
{
	RemoveHealingTarget( pOther );
}

// Josh: Basically everything except grating.
#define MASK_DISPENSER (MASK_BLOCKLOS | CONTENTS_WINDOW)

//-----------------------------------------------------------------------------
// Purpose: Is this a valid heal target? and not already healing them?
//-----------------------------------------------------------------------------
bool CObjectDispenser::CouldHealTarget( CBaseEntity *pTarget )
{
	if ( !HasSpawnFlags( SF_DISPENSER_IGNORE_LOS ) && !pTarget->FVisible( this, MASK_DISPENSER ) )
		return false;

	if ( pTarget->IsPlayer() && pTarget->IsAlive() )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pTarget );

		// don't heal enemies unless they are disguised as our team
		int iTeam = GetTeamNumber();
		int iPlayerTeam = pTFPlayer->GetTeamNumber();

		if ( iPlayerTeam != iTeam && pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			iPlayerTeam = pTFPlayer->m_Shared.GetDisguiseTeam();
		}

		if ( iPlayerTeam != iTeam )
		{
			return false;
		}

		if ( HasSpawnFlags( SF_DISPENSER_DONT_HEAL_DISGUISED_SPIES ) )
		{
			// if they're invis, no heals
			if ( pTFPlayer->m_Shared.IsStealthed() )
			{
				return false;
			}

			// if they're disguised as enemy
			if ( pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) &&
				 pTFPlayer->m_Shared.GetDisguiseTeam() != iTeam )
			{
				return false;
			}
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CObjectDispenser::GetDispenserRadius( void )
{
	return 64.f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CObjectDispenser::AddHealingTarget( CBaseEntity *pOther )
{
	// add to tail
	EHANDLE hOther = pOther;
	m_hHealingTargets.AddToTail( hOther );
	NetworkStateChanged();

	// check how many healing targets we now have and possibly award an achievement
	if ( m_hHealingTargets.Count() >= 3 && GetBuilder() )
	{
		GetBuilder()->AwardAchievement( ACHIEVEMENT_TF_ENGINEER_DISPENSER_HEAL_GROUP );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CObjectDispenser::RemoveHealingTarget( CBaseEntity *pOther )
{
	// remove
	EHANDLE hOther = pOther;
	bool bFound = m_hHealingTargets.FindAndRemove( hOther );
	NetworkStateChanged();

	return bFound;
}

//-----------------------------------------------------------------------------
// Purpose: Are we healing this target already
//-----------------------------------------------------------------------------
bool CObjectDispenser::IsHealingTarget( CBaseEntity *pTarget )
{
	EHANDLE hOther = pTarget;
	return m_hHealingTargets.HasElement( hOther );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CObjectDispenser::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];
		Q_snprintf( tempstr, sizeof( tempstr ),"Metal: %d", m_iAmmoMetal.Get() );
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}


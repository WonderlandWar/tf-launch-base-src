//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "c_tf_projectile_rocket.h"
#include "particles_new.h"
#include "tf_gamerules.h"

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Rocket, DT_TFProjectile_Rocket )

BEGIN_NETWORK_TABLE( C_TFProjectile_Rocket, DT_TFProjectile_Rocket )
	RecvPropBool( RECVINFO( m_bCritical ) ),
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFProjectile_Rocket::C_TFProjectile_Rocket( void )
{
	pEffect = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFProjectile_Rocket::~C_TFProjectile_Rocket( void )
{
	if ( pEffect )
	{
		ParticleProp()->StopEmission( pEffect );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFProjectile_Rocket::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFProjectile_Rocket::CreateTrails( void )
{
	if ( IsDormant() )
		return;

	bool bUsingCustom = false;

	if ( pEffect )
	{
		ParticleProp()->StopEmission( pEffect );
		pEffect = NULL;
	}

	int iAttachment = LookupAttachment( "trail" );
	if ( iAttachment == INVALID_PARTICLE_ATTACHMENT )
		return;

	if ( enginetrace->GetPointContents( GetAbsOrigin() ) & MASK_WATER )
	{
		ParticleProp()->Create( "rockettrail_underwater", PATTACH_POINT_FOLLOW, "trail" );
		bUsingCustom = true;
	}
	else if ( GetTeamNumber() == TEAM_UNASSIGNED )
	{
		ParticleProp()->Create( "rockettrail_underwater", PATTACH_POINT_FOLLOW, "trail" );
		bUsingCustom = true;
	}

	if ( !bUsingCustom )
	{
		if ( GetTrailParticleName() )
		{
			ParticleProp()->Create( GetTrailParticleName(), PATTACH_POINT_FOLLOW, iAttachment );
		}
	}

	if ( m_bCritical )
	{
		switch( GetTeamNumber() )
		{
		case TF_TEAM_BLUE:
			pEffect = ParticleProp()->Create( "critical_rocket_blue", PATTACH_ABSORIGIN_FOLLOW );
			break;
		case TF_TEAM_RED:
			pEffect = ParticleProp()->Create( "critical_rocket_red", PATTACH_ABSORIGIN_FOLLOW );
			break;
		default:
			pEffect = ParticleProp()->Create( "eyeboss_projectile", PATTACH_ABSORIGIN_FOLLOW );
			break;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *C_TFProjectile_Rocket::GetTrailParticleName( void )
{	
	return "rockettrail"; 
}

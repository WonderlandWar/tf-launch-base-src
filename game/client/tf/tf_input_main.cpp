//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF2 specific input handling
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "kbutton.h"
#include "input.h"

#include "c_tf_player.h"
#include "cam_thirdperson.h"

extern ConVar		thirdperson_platformer;
extern ConVar		cam_idealyaw;
extern ConVar		cl_yawspeed;
extern kbutton_t	in_left;
extern kbutton_t	in_right;
extern CThirdPersonManager g_ThirdPersonManager;

//-----------------------------------------------------------------------------
// Purpose: TF Input interface
//-----------------------------------------------------------------------------
class CTFInput : public CInput
{
public:
	CTFInput()
		: m_angThirdPersonOffset( 0.f, 0.f, 0.f )
	{}
	virtual		float		CAM_CapYaw( float fVal ) const OVERRIDE;
	virtual		float		CAM_CapPitch( float fVal ) const OVERRIDE;
	virtual		void		AdjustYaw( float speed, QAngle& viewangles );
private:

	QAngle m_angThirdPersonOffset;
};

static CTFInput g_Input;

// Expose this interface
IInput *input = ( IInput * )&g_Input;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFInput::CAM_CapYaw( float fVal ) const
{
	return fVal;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFInput::CAM_CapPitch( float fVal ) const
{
	CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return fVal;

	return fVal;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFInput::AdjustYaw( float speed, QAngle& viewangles )
{
	if ( !(in_strafe.state & 1) )
	{
		float yaw_right = speed*cl_yawspeed.GetFloat() * KeyState (&in_right);
		float yaw_left = speed*cl_yawspeed.GetFloat() * KeyState (&in_left);

		viewangles[YAW] -= yaw_right;
		viewangles[YAW] += yaw_left;
	}

	if ( CAM_IsThirdPerson() )
	{
		if ( thirdperson_platformer.GetInt() )
		{
			float side = KeyState(&in_moveleft) - KeyState(&in_moveright);
			float forward = KeyState(&in_forward) - KeyState(&in_back);

			if ( side || forward )
			{
				viewangles[YAW] = RAD2DEG(atan2(side, forward)) + g_ThirdPersonManager.GetCameraOffsetAngles()[ YAW ];
			}
			if ( side || forward || KeyState (&in_right) || KeyState (&in_left) )
			{
				cam_idealyaw.SetValue( g_ThirdPersonManager.GetCameraOffsetAngles()[ YAW ] - viewangles[ YAW ] );
			}
		}
	}
}
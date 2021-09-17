#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "skill.h"
#include "gamerules.h"
#include "effects.h"

// HLPS2's suit charger and associated "field" entity which is actually just for putting glass around the cylinders

class CRechargeNewField : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
};

LINK_ENTITY_TO_CLASS( item_rechargefield, CRechargeNewField );

void CRechargeNewField::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	SET_MODEL( ENT( pev ), "models/field.mdl" );
	pev->movetype = MOVETYPE_NONE;
	UTIL_SetOrigin( pev, pev->origin );
	pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 150.0;
}

void CRechargeNewField::Precache()
{
	PRECACHE_MODEL( "models/field.mdl" );
}

CRechargeNewField *SpawnRechargeField( Vector *vecOrigin, Vector *vecAngles )
{
	CRechargeNewField *pField = GetClassPtr( (CRechargeNewField *)NULL );

	pField->pev->classname = MAKE_STRING( "item_rechargefield" );
	pField->pev->origin = *vecOrigin;
	pField->pev->angles = *vecAngles;
	pField->Spawn();

	return pField;
}

class CRechargeNew : public CBaseToggle
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	EXPORT void Think1( void );
	EXPORT void Think2( void );
	void Recharge( void );
	void Off( void );
	BOOL FindPlayer( void );
	void DetermineYaw( void );
	float VecToYaw( Vector vecDir );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int ObjectCaps() { return ( CBaseToggle::ObjectCaps() | FCAP_CONTINUOUS_USE ); }
	int Save( CSave &save );
	int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	float m_flNextCharge;
	float m_flReactivate;
	int m_iJuice;
	int m_iOn;
	float m_flSoundTime;
	float m_flNextReactivate;
	EHANDLE m_hField;
	EHANDLE m_hPlayer;
	int m_fPlayerIsNear;
	int m_iIdleState;
	float m_flIdealArmYaw;
	float m_flIdealEyeYaw;
	float m_flCurrentArmYaw;
	float m_flCurrentEyeYaw;
	float m_flOffTime;
	int m_fLightsOn;
	float m_flCylinderRotation;
	CBeam *m_pBeam;

	int field_0x124;
};

TYPEDESCRIPTION CRechargeNew::m_SaveData[] = {
	DEFINE_FIELD( CRechargeNew, m_flNextCharge, FIELD_TIME ),
	DEFINE_FIELD( CRechargeNew, m_flReactivate, FIELD_FLOAT ),
	DEFINE_FIELD( CRechargeNew, m_iJuice, FIELD_INTEGER ),
	DEFINE_FIELD( CRechargeNew, m_iOn, FIELD_INTEGER),
	DEFINE_FIELD( CRechargeNew, m_flSoundTime, FIELD_TIME ),
	DEFINE_FIELD( CRechargeNew, m_flNextReactivate, FIELD_TIME ),
	DEFINE_FIELD( CRechargeNew, m_hField, FIELD_EHANDLE ),
	DEFINE_FIELD( CRechargeNew, m_fPlayerIsNear, FIELD_BOOLEAN ),
	DEFINE_FIELD( CRechargeNew, m_iIdleState, FIELD_INTEGER ),
	DEFINE_FIELD( CRechargeNew, m_flIdealArmYaw, FIELD_FLOAT ),
	DEFINE_FIELD( CRechargeNew, m_flIdealEyeYaw, FIELD_FLOAT ),
	DEFINE_FIELD( CRechargeNew, m_flCurrentArmYaw, FIELD_FLOAT ),
	DEFINE_FIELD( CRechargeNew, m_flCurrentEyeYaw, FIELD_FLOAT ),
	DEFINE_FIELD( CRechargeNew, m_flOffTime, FIELD_TIME ),
	DEFINE_FIELD( CRechargeNew, m_fLightsOn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CRechargeNew, m_flCylinderRotation, FIELD_FLOAT ),
	DEFINE_FIELD( CRechargeNew, m_pBeam, FIELD_CLASSPTR )
};

IMPLEMENT_SAVERESTORE( CRechargeNew, CBaseToggle );

LINK_ENTITY_TO_CLASS( item_recharge, CRechargeNew );

void CRechargeNew::KeyValue( KeyValueData *pkvd )
{
		if (	FStrEq(pkvd->szKeyName, "style") ||
				FStrEq(pkvd->szKeyName, "height") ||
				FStrEq(pkvd->szKeyName, "value1") ||
				FStrEq(pkvd->szKeyName, "value2") ||
				FStrEq(pkvd->szKeyName, "value3"))
	{
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "dmdelay"))
	{
		m_flReactivate = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

void CRechargeNew::Spawn()
{
	Precache();
	pev->solid = SOLID_BBOX;
	SET_MODEL( ENT( pev ), "models/hev.mdl" );

	if ( pev->angles.y < 0.0 )
	{
		pev->angles.y += 360.0;
	}

	if ( pev->angles.y == 0.0 || pev->angles.y == 180.0 )
	{
		UTIL_SetSize( pev, Vector( -16.0, -18.0, 0.0 ), Vector( 16.0, 18.0, 32.0 ) );
	}
	else
	{
		UTIL_SetSize( pev, Vector( -18.0, -16.0, 0.0 ), Vector( 18.0, 16.0, 32.0 ) );
	}

	// wtf is this
	if ( pev->angles.y > 45.0 || pev->angles.y < 0.0 )
	{
		if ( pev->angles.y > 315.0 && pev->angles.y <= 360.0 )
		{
			pev->view_ofs.x = -8.0;
			pev->view_ofs.y = 4.0;
			pev->view_ofs.z = 16.0;
		}
		else if ( pev->angles.y <= 135.0 )
		{
			if ( pev->angles.y > 45.0 )
			{
				pev->view_ofs.x = -4.0;
				pev->view_ofs.y = 8.0;
				pev->view_ofs.z = 16.0;
			}
		}
		else if ( pev->angles.y <= 255.0 )
		{
			if ( pev->angles.y > 135.0 )
			{
				pev->view_ofs.x = 8.0;
				pev->view_ofs.y = -4.0;
				pev->view_ofs.z = 16.0;
			}
		}
		else if ( pev->angles.y > 315.0 && pev->angles.y <= 255.0 )
		{
			pev->view_ofs.x = 4.0;
			pev->view_ofs.y = -8.0;
			pev->view_ofs.z = 16.0;
		}
	}
	else
	{
		pev->view_ofs.x = -8.0;
		pev->view_ofs.y = 4.0;
		pev->view_ofs.z = 16.0;
	}

	m_hField = SpawnRechargeField( &pev->origin, &pev->angles );
	m_flNextReactivate = 0.0;
	m_fPlayerIsNear = FALSE;
	m_iIdleState = 0;
	m_fLightsOn = FALSE;
	pev->movetype = MOVETYPE_PUSH;
	pev->skin = 0;
	pev->sequence = LookupSequence( "rest" );
	pev->frame = 0.0;
	ResetSequenceInfo();
	SetBoneController( 3, 0.0 );
	SetBoneController( 0, 0.0 );
	SetBoneController( 1, 0.0 );
	SetBoneController( 2, 0.0 );
	m_flCurrentArmYaw = 0.0;
	m_flIdealArmYaw = 0.0;
	m_flCurrentEyeYaw = 0.0;
	m_flIdealEyeYaw = 0.0;
	m_flOffTime = 0.0;
	UTIL_SetOrigin( pev, pev->origin );
	m_iJuice = gSkillData.suitchargerCapacity;
	SetThink( &CRechargeNew::Think1 );
	pev->nextthink = pev->ltime + 0.5;
	m_flCylinderRotation = 0;

	m_pBeam = CBeam::BeamCreate( "sprites/lgtning.spr", 6 );

	if ( m_pBeam )
	{
		m_pBeam->EntsInit( ENTINDEX( ENT( pev ) ), ENTINDEX( ENT( pev ) ) );
		m_pBeam->SetEndAttachment( 3 );
		m_pBeam->SetStartAttachment( 4 );
		m_pBeam->SetColor( 128, 192, 0 );
		m_pBeam->SetBrightness( 0 );
		m_pBeam->SetNoise( 15 );
	}
}

void CRechargeNew::Precache()
{
	PRECACHE_MODEL( "models/hev.mdl" );
	UTIL_PrecacheOther( "item_rechargefield" );
	PRECACHE_SOUND( "items/suitcharge1.wav" );
	PRECACHE_SOUND( "items/suitchargeno1.wav" );
	PRECACHE_SOUND( "items/suitchargeok1.wav" );
}

void CRechargeNew::Think1()
{
	// why
	SetThink( &CRechargeNew::Think2 );
	pev->nextthink = pev->ltime + 0.1;
}

void CRechargeNew::Think2()
{
	if ( field_0x124 != 0 )
	{
		m_pBeam->SetEndAttachment( 3 );
		m_pBeam->SetStartAttachment( 4 );

		if ( m_fLightsOn )
			m_fLightsOn = FALSE;

		field_0x124 = 0;
	}

	DispatchAnimEvents( 0.1 );
	StudioFrameAdvance( 0.0 );

	if ( m_fSequenceFinished )
	{
		if ( !m_fPlayerIsNear )
		{
			if ( m_iIdleState != 1 )
			{
				if ( !m_iOn )
				{
					pev->sequence = LookupSequence( "rest" );
					pev->frame = 0.0;
					ResetSequenceInfo();
					m_iIdleState = 1;
				}
			}
		}
		else
		{
			if ( m_iIdleState == 2 && m_iOn )
			{
				pev->sequence = LookupSequence( "charge_idle" );
				pev->frame = 0.0;
				ResetSequenceInfo();
			}
			else
			{
				if ( m_iIdleState != 2 && !m_iOn )
				{
					pev->sequence = LookupSequence( "prep_charge" );
					pev->frame = 0.0;
					ResetSequenceInfo();
					m_iIdleState = 2;
				}
			}
		}
	}

	if ( m_flNextReactivate > 0.0 && m_flNextReactivate < gpGlobals->time )
	{
		Recharge();
	}

	if ( m_flOffTime > 0.0 && m_flOffTime < gpGlobals->time )
	{
		Off();
	}

	if ( m_iJuice >= 1 )
	{
		if ( FindPlayer() )
		{
			if ( !m_fPlayerIsNear )
			{
				pev->sequence = LookupSequence( "deploy" );
				pev->frame = 0.0;
				ResetSequenceInfo();
				m_fPlayerIsNear = TRUE;
				EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/suitchargeok1.wav", 1.0, ATTN_NORM );
				m_flSoundTime = gpGlobals->time + 0.56;
				m_pBeam->SetBrightness( 96.0 );
			}

			DetermineYaw();

			if ( m_flIdealArmYaw != m_flCurrentArmYaw )
			{
				// gearbox moment: calculating the maximum distance that the yaw should change by and then just discarding it
				if ( m_flIdealArmYaw < m_flCurrentArmYaw )
				{
					m_flCurrentArmYaw -= V_min( m_flCurrentArmYaw - m_flIdealArmYaw, 5.0 );
				}
				else
				{
					m_flCurrentArmYaw += V_min( m_flIdealArmYaw - m_flCurrentArmYaw, 5.0 );
				}

				m_flCurrentArmYaw = m_flIdealArmYaw;
				SetBoneController( 3, m_flIdealArmYaw );
			}

			if ( m_flIdealEyeYaw != m_flCurrentEyeYaw )
			{
				// ditto
				if ( m_flIdealEyeYaw < m_flCurrentEyeYaw )
				{
					m_flCurrentEyeYaw -= V_min( m_flCurrentEyeYaw - m_flIdealEyeYaw, 5.0 );
				}
				else
				{
					m_flCurrentEyeYaw += V_min( m_flIdealEyeYaw - m_flCurrentEyeYaw, 5.0 );
				}

				m_flCurrentEyeYaw = m_flIdealEyeYaw;
				SetBoneController( 0, m_flIdealEyeYaw );
			}

			if ( m_iOn != 0 )
			{
				if ( m_flCylinderRotation >= 360.0 )
					m_flCylinderRotation = 0.0;
				else
					m_flCylinderRotation += 1.0; // changed from 10.0 because we think faster than HLPS2

				SetBoneController( 1, m_flCylinderRotation );
				SetBoneController( 2, 360.0 - m_flCylinderRotation );
			}

			// NOT ACCURATE
			pev->nextthink = pev->ltime + 0.01; /* + 0.1; */
			return;
		}
	}

	if ( m_fPlayerIsNear )
	{
		m_flIdealArmYaw = 0.0;
		SetBoneController( 0, 0.0 );
		pev->sequence = LookupSequence( "retract_arm" );
		pev->frame = 0.0;
		ResetSequenceInfo();
		m_fPlayerIsNear = FALSE;
		m_pBeam->SetBrightness( 0.0 );
	}

	if ( m_iOn != 0 )
	{
		if ( m_flCylinderRotation >= 360.0 )
			m_flCylinderRotation = 0.0;
		else
			m_flCylinderRotation += 10.0;

		SetBoneController( 1, m_flCylinderRotation );
		SetBoneController( 2, 360.0 - m_flCylinderRotation );
	}

	pev->nextthink = pev->ltime + 0.1;
}

void CRechargeNew::Recharge()
{
	m_flNextReactivate = 0.0;
	m_iJuice = gSkillData.suitchargerCapacity;
}

void CRechargeNew::Off()
{
	if ( m_iOn >= 2 )
	{
		STOP_SOUND( ENT( pev ), CHAN_STATIC, "items/suitcharge1.wav" );
	}

	m_iOn = 0;

	pev->sequence = LookupSequence( "retract_charge" );
	pev->frame = 0.0;
	ResetSequenceInfo();

	if ( m_iJuice == 0 )
	{
		m_flReactivate = g_pGameRules->FlHEVChargerRechargeTime();

		if ( m_flReactivate > 0.0 )
		{
			m_flNextReactivate = gpGlobals->time + m_flReactivate;
		}

		m_pBeam->SetBrightness( 0.0 );
	}
	else
	{
		m_pBeam->SetBrightness( 96.0 );
	}

	m_flOffTime = 0.0;
}

BOOL CRechargeNew::FindPlayer()
{
	if ( gpGlobals->maxClients > 0 )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

			if ( pPlayer )
			{
				if ( ( pPlayer->pev->origin - pev->origin ).Length() < 96.0 )
				{
					if ( m_hPlayer == NULL )
					{
						if ( FVisible( pPlayer->pev->origin ) )
						{
							m_hPlayer = pPlayer;
							return TRUE;
						}
					}
					else
					{
						return TRUE;
					}
				}
			}
		}
	}

	m_hPlayer = NULL;
	return FALSE;
}

void CRechargeNew::DetermineYaw()
{
	Vector vec;
	float yaw;

	// arm yaw
	if ( pev->angles.y > 45.0 || pev->angles.y < 0.0 )
	{
		if ( pev->angles.y > 315.0 && pev->angles.y <= 360.0 )
		{
			vec.x = pev->origin.x + 4.0;
			vec.y = pev->origin.y;
		}
		else if ( pev->angles.y <= 135.0 && pev->angles.y > 45.0 )
		{
			vec.x = pev->origin.x;
			vec.y = pev->origin.y + -4.0;
		}
		else if ( pev->angles.y <= 225.0 && pev->angles.y > 135.0 )
		{
			vec.x = pev->origin.x + -4.0;
			vec.y = pev->origin.y;
		}
		else if ( pev->angles.y <= 315.0 && pev->angles.y > 225.0 )
		{
			vec.x = pev->origin.x;
			vec.y = pev->origin.y + 4.0;
		}
	}
	else
	{
		vec.x = pev->origin.x + 4.0;
		vec.y = pev->origin.y;
	}

	vec.z = pev->origin.z + 16.0;

	CBaseEntity *pPlayer = m_hPlayer;
	yaw = VecToYaw( pPlayer->pev->origin - vec );
	m_flIdealArmYaw = UTIL_AngleDistance( yaw, pev->angles.y );

	// eye yaw
	if ( pev->angles.y > 45.0 || pev->angles.y < 0.0 )
	{
		if ( pev->angles.y > 315.0 && pev->angles.y <= 360.0 )
		{
			vec.x = pev->origin.x + 4.0;
			vec.y = pev->origin.y + -8.0;
		}
		else if ( pev->angles.y > 135.0 || pev->angles.y <= 45.0 )
		{
			if ( pev->angles.y > 225.0 || pev->angles.y <= 135.0 )
			{
				vec.x = pev->origin.x + -8.0;
				vec.y = pev->origin.y + 4.0;
			}
			else
			{
				vec.x = pev->origin.x + -4.0;
				vec.y = pev->origin.y + 8.0;
			}
		}
		else
		{
			vec.x = pev->origin.x + 8.0;
			vec.y = pev->origin.y + -4.0;
		}
	}
	else
	{
		vec.x = pev->origin.x + 4.0;
		vec.y = pev->origin.y + -8.0;
	}

	vec.z = pev->origin.z + 16.0;

	yaw = VecToYaw( pPlayer->pev->origin - vec );
	m_flIdealEyeYaw = UTIL_AngleDistance( yaw, pev->angles.y );
}

float CRechargeNew::VecToYaw( Vector vecDir )
{
	if ( vecDir.x == 0 && vecDir.y == 0 && vecDir.z == 0 )
		return pev->angles.y;

	return UTIL_VecToYaw( vecDir );
}

void CRechargeNew::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// alien mode doesn't do anything here
	return;

	if ( !pActivator )
		return;

	if ( !pActivator->IsPlayer() )
		return;

	if ( ( pActivator->pev->origin - pev->origin ).Length2D() > 48.0 )
		return;

	if ( m_iJuice <= 0 || ( !( pActivator->pev->weapons & ( 1 << WEAPON_SUIT ) ) ) )
	{
		if ( m_flSoundTime <= gpGlobals->time )
		{
			m_flSoundTime = gpGlobals->time + 0.62;
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/suitchargeno1.wav", 0.85, ATTN_NORM );
		}
		return;
	}

	m_flOffTime = gpGlobals->time + 0.25;

	if ( m_flNextCharge >= gpGlobals->time )
		return;

	if ( !m_iOn )
	{
		m_iOn++;
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/suitchargeok1.wav", 0.85, ATTN_NORM );
		m_flSoundTime = gpGlobals->time + 0.56;
		pev->sequence = LookupSequence( "give_charge" );
		pev->frame = 0.0;
		ResetSequenceInfo();
		m_pBeam->SetBrightness( 196.0 );
	}

	if ( m_iOn == 1 && m_flSoundTime <= gpGlobals->time )
	{
		m_iOn++;
		EMIT_SOUND( ENT( pev ), CHAN_STATIC, "items/suitcharge1.wav", 0.85, ATTN_NORM );
	}

	if ( pActivator->pev->armorvalue < 100.0 )
	{
		m_iJuice--;
		pActivator->pev->armorvalue++;

		if ( pActivator->pev->armorvalue > 100.0 )
			pActivator->pev->armorvalue = 100.0;

		if ( m_iJuice < 1 )
		{
			pev->skin = 1;
			Off();
		}
	}

	m_flNextCharge = gpGlobals->time + 0.1;
}
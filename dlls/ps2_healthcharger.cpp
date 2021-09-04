#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "skill.h"
#include "gamerules.h"

// HLPS2's health charger and associated canister

class CWallHealthNewCanister : public CBaseAnimating
{
public:
	void Spawn( void );
	void Precache( void );
	EXPORT void LiquidThink( void );
	void SetLiquid( int liquid );
	void SetSloshing( BOOL sloshing );
	int ObjectCaps( void ) { return CBaseAnimating::ObjectCaps(); }
	int Save( CSave &save );
	int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	int m_fDraining;
	int m_fSettling;
};

TYPEDESCRIPTION CWallHealthNewCanister::m_SaveData[] =
{
	DEFINE_FIELD( CWallHealthNewCanister, m_fDraining, FIELD_BOOLEAN ),
	DEFINE_FIELD( CWallHealthNewCanister, m_fSettling, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CWallHealthNewCanister, CBaseAnimating );

LINK_ENTITY_TO_CLASS( item_healthchargercan, CWallHealthNewCanister );

void CWallHealthNewCanister::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	SET_MODEL( ENT( pev ), "models/health_charger_both.mdl" );
	pev->movetype = MOVETYPE_NONE;
	UTIL_SetOrigin( pev, pev->origin );
	pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 150.0;
	pev->sequence = LookupSequence( "still" );
	pev->frame = 0.0;
	ResetSequenceInfo();
	SetBoneController( 0, 0.0 );
	SetThink( &CWallHealthNewCanister::LiquidThink );
	pev->nextthink = gpGlobals->time + 0.1;
	m_fSettling = FALSE;
	m_fDraining = FALSE;
}

void CWallHealthNewCanister::Precache()
{
	PRECACHE_MODEL( "models/health_charger_both.mdl" );
}

void CWallHealthNewCanister::LiquidThink()
{
	DispatchAnimEvents( 0.1 );
	StudioFrameAdvance( 0.0 );

	if ( m_fSequenceFinished && !m_fSequenceLoops )
	{
		if ( m_fSettling )
		{
			pev->sequence = LookupSequence( "still" );
			m_fSettling = FALSE;
		}

		pev->frame = 0.0;
		ResetSequenceInfo();
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

void CWallHealthNewCanister::SetLiquid( int liquid )
{
	SetBoneController( 0, (float)liquid * 0.1 + -11.0 );
}

void CWallHealthNewCanister::SetSloshing( BOOL sloshing )
{
	if ( sloshing )
	{
		if ( !m_fDraining )
		{
			m_fSettling = FALSE;
			m_fDraining = TRUE;
			pev->sequence = LookupSequence( "slosh" );
			pev->frame = 0.0;
			ResetSequenceInfo();
		}
	}
	else
	{
		if ( m_fDraining )
		{
			m_fDraining = FALSE;
			m_fSettling = TRUE;
			pev->sequence = LookupSequence( "to_rest" );
			pev->frame = 0.0;
			ResetSequenceInfo();
		}
	}
}

CWallHealthNewCanister *SpawnHealthCanister( Vector *vecOrigin, Vector *vecAngles )
{
	CWallHealthNewCanister *canister = GetClassPtr( (CWallHealthNewCanister *)NULL );
	
	canister->pev->classname = MAKE_STRING( "item_healthchargercan" );
	canister->pev->origin = *vecOrigin;
	canister->pev->angles = *vecAngles;
	canister->Spawn();
	
	return canister;
}

class CWallHealthNew : public CBaseToggle
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
	int m_iOn; // 0 = off, 1 = startup, 2 = going
	float m_flSoundTime;
	float m_flNextReactivate;
	EHANDLE m_hCanister;
	EHANDLE m_hPlayer;
	int m_fPlayerIsNear;
	int m_iIdleState;
	float m_flIdealArmYaw;
	float m_flIdealEyeYaw;
	float m_flCurrentArmYaw;
	float m_flCurrentEyeYaw;
	float m_flOffTime;
	int m_fLightsOn;

	int field_0x118; // ???
};

TYPEDESCRIPTION CWallHealthNew::m_SaveData[] = {
	DEFINE_FIELD( CWallHealthNew, m_flNextCharge, FIELD_TIME ),
	DEFINE_FIELD( CWallHealthNew, m_flReactivate, FIELD_FLOAT ),
	DEFINE_FIELD( CWallHealthNew, m_iJuice, FIELD_INTEGER ),
	DEFINE_FIELD( CWallHealthNew, m_iOn, FIELD_INTEGER ),
	DEFINE_FIELD( CWallHealthNew, m_flSoundTime, FIELD_TIME ),
	DEFINE_FIELD( CWallHealthNew, m_flNextReactivate, FIELD_TIME ),
	DEFINE_FIELD( CWallHealthNew, m_hCanister, FIELD_EHANDLE ),
	DEFINE_FIELD( CWallHealthNew, m_hPlayer, FIELD_EHANDLE ),
	DEFINE_FIELD( CWallHealthNew, m_fPlayerIsNear, FIELD_BOOLEAN ),
	DEFINE_FIELD( CWallHealthNew, m_iIdleState, FIELD_INTEGER ),
	DEFINE_FIELD( CWallHealthNew, m_flIdealArmYaw, FIELD_FLOAT ),
	DEFINE_FIELD( CWallHealthNew, m_flIdealEyeYaw, FIELD_FLOAT ),
	DEFINE_FIELD( CWallHealthNew, m_flCurrentArmYaw, FIELD_FLOAT ),
	DEFINE_FIELD( CWallHealthNew, m_flCurrentEyeYaw, FIELD_FLOAT ),
	DEFINE_FIELD( CWallHealthNew, m_flOffTime, FIELD_TIME ),
	DEFINE_FIELD( CWallHealthNew, m_fLightsOn, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CWallHealthNew, CBaseToggle );

LINK_ENTITY_TO_CLASS( item_healthcharger, CWallHealthNew );

void CWallHealthNew::KeyValue( KeyValueData *pkvd )
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

void CWallHealthNew::Spawn()
{
	Precache();
	pev->solid = SOLID_BBOX;
	SET_MODEL( ENT( pev ), "models/health_charger_body.mdl" );

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
			pev->view_ofs.z = 32.0;
		}
		else if ( pev->angles.y <= 135.0 )
		{
			if ( pev->angles.y > 45.0 )
			{
				pev->view_ofs.x = -4.0;
				pev->view_ofs.y = 8.0;
				pev->view_ofs.z = 32.0;
			}
		}
		else if ( pev->angles.y <= 255.0 )
		{
			if (pev->angles.y > 135.0)
			{
				pev->view_ofs.x = 8.0;
				pev->view_ofs.y = -4.0;
				pev->view_ofs.z = 32.0;
			}
		}
		else if ( pev->angles.y > 315.0 && pev->angles.y <= 255.0 )
		{
			pev->view_ofs.x = 4.0;
			pev->view_ofs.y = -8.0;
			pev->view_ofs.z = 32.0;
		}
	}
	else
	{
		pev->view_ofs.x = -8.0;
		pev->view_ofs.y = 4.0;
		pev->view_ofs.z = 32.0;
	}

	m_hCanister = SpawnHealthCanister( &pev->origin, &pev->angles );
	m_flNextReactivate = 0.0;
	m_fPlayerIsNear = FALSE;
	m_iIdleState = 0;
	m_fLightsOn = FALSE;
	pev->movetype = MOVETYPE_PUSH;
	pev->skin = 0;
	pev->sequence = LookupSequence( "still" );
	pev->frame = 0.0;
	ResetSequenceInfo();
	SetBoneController( 0, 0.0 );
	SetBoneController( 1, 0.0 );
	m_flOffTime = 0.0;
	m_flCurrentArmYaw = 0.0;
	m_flIdealArmYaw = 0.0;
	m_flCurrentEyeYaw = 0.0;
	m_flIdealEyeYaw = 0.0;
	UTIL_SetOrigin( pev, pev->origin );
	m_iJuice = gSkillData.healthchargerCapacity;
	SetThink( &CWallHealthNew::Think1 );
	pev->nextthink = pev->ltime + 0.5;
}

void CWallHealthNew::Precache()
{
	PRECACHE_MODEL( "models/health_charger_body.mdl" );
	UTIL_PrecacheOther( "item_healthchargercan" );
	PRECACHE_SOUND( "items/medshot4.wav" );
	PRECACHE_SOUND( "items/medshotno1.wav" );
	PRECACHE_SOUND( "items/medcharge4.wav" );
}

void CWallHealthNew::Think1()
{
	// why
	SetThink( &CWallHealthNew::Think2 );
	pev->nextthink = pev->ltime + 0.1;
}

void CWallHealthNew::Think2()
{
	if ( field_0x118 != 0 && m_fLightsOn )
	{
		m_fLightsOn = FALSE;
		field_0x118 = 0;
	}

	DispatchAnimEvents( 0.1 );
	StudioFrameAdvance( 0.0 );

	if ( m_fSequenceFinished )
	{
		if ( !m_fPlayerIsNear )
		{
			if ( m_iIdleState != 1 ) {
				if ( !m_iOn )
				{
					pev->sequence = LookupSequence( "still" );
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
				pev->sequence = LookupSequence( "shot_idle" );
				pev->frame = 0.0;
				ResetSequenceInfo();
			}
			else
			{
				if ( m_iIdleState != 2 && !m_iOn )
				{
					pev->sequence = LookupSequence( "prep_shot" );
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
				EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/medshot4.wav", 1.0, ATTN_NORM );
				m_flSoundTime = gpGlobals->time + 0.56;
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
				SetBoneController( 0, m_flIdealArmYaw );
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
				SetBoneController( 1, m_flIdealEyeYaw );
			}

			// NOT ACCURATE
			pev->nextthink = pev->ltime + 0.01; /* + 0.1; */
			return;
		}
	}

	if ( m_fPlayerIsNear )
	{
		m_flIdealArmYaw = 0.0;
		SetBoneController( 0, m_flIdealArmYaw );
		pev->sequence = LookupSequence( "retract_arm" );
		pev->frame = 0.0;
		ResetSequenceInfo();
		m_fPlayerIsNear = FALSE;
	}

	pev->nextthink = pev->ltime + 0.1;
}

void CWallHealthNew::Recharge()
{
	EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/medshot4.wav", 1.0, ATTN_NORM );
	m_flNextReactivate = 0.0;
	m_iJuice = gSkillData.healthchargerCapacity;
	( (CWallHealthNewCanister *)( (CBaseEntity *)m_hCanister ) )->SetLiquid( 100 );
}

void CWallHealthNew::Off()
{
	if ( m_iOn < 2 )
	{
		m_iOn = 0;
	}
	else
	{
		STOP_SOUND( ENT( pev ), CHAN_STATIC, "items/medcharge4.wav" );
		m_iOn = 0;
	}

	pev->sequence = LookupSequence( "retract_shot" );
	pev->frame = 0.0;
	ResetSequenceInfo();
	( (CWallHealthNewCanister *)( (CBaseEntity *)m_hCanister ) )->SetSloshing( FALSE );

	if ( m_iJuice == 0 )
	{
		m_flReactivate = g_pGameRules->FlHealthChargerRechargeTime();

		if ( m_flReactivate > 0.0 )
		{
			m_flNextReactivate = gpGlobals->time + m_flReactivate;
		}
		m_flOffTime = 0.0;
	}
	else
	{
		m_flOffTime = 0.0;
	}
}

BOOL CWallHealthNew::FindPlayer()
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
						}
					}

					return TRUE;
				}
			}
		}
	}

	m_hPlayer = NULL;
	return FALSE;
}

void CWallHealthNew::DetermineYaw()
{
	Vector vec;
	float yaw;

	// arm yaw
	if ( pev->angles.y > 45.0 || pev->angles.y < 0.0 )
	{
		if ( pev->angles.y > 315.0 && pev->angles.y <= 360.0 )
		{
			vec.x = pev->origin.x + 4.0;
			vec.y = pev->origin.y + 8.0;
		}
		else if ( pev->angles.y <= 135.0 && pev->angles.y > 45.0 )
		{
			vec.x = pev->origin.x + -8.0;
			vec.y = pev->origin.y + -4.0;
		}
		else if ( pev->angles.y <= 225.0 && pev->angles.y > 135.0 )
		{
			vec.x = pev->origin.x + -4.0;
			vec.y = pev->origin.y + -8.0;
		}
		else if ( pev->angles.y <= 315.0 && pev->angles.y > 225.0 )
		{
			vec.x = pev->origin.x + 8.0;
			vec.y = pev->origin.y + 4.0;
		}
	}
	else
	{
		vec.x = pev->origin.x + 4.0;
		vec.y = pev->origin.y + 8.0;
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
				if ( pev->angles.y > 315.0 || pev->angles.y <= 225.0 )
				{
					// according to ghidra, if this block is entered, it skips to VecToYaw
					// but that doesn't make sense since it didn't set any values for vec
					// so i think this might be a gearbox moment but i'm not 100% sure
					// it could be a clever optimization thing the compiler did
					// i'm just going to do this
					vec.x = pev->origin.x + 8.0;
					vec.y = pev->origin.y + 4.0;
				}
				else
				{
					vec.x = pev->origin.x + -8.0;
					vec.y = pev->origin.y + 4.0;
				}
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

	vec.z = pev->origin.z + 32.0;
	
	yaw = VecToYaw( pPlayer->pev->origin - vec );
	m_flIdealEyeYaw = UTIL_AngleDistance( yaw, pev->angles.y );
}

float CWallHealthNew::VecToYaw( Vector vecDir )
{
	if ( vecDir.x == 0 && vecDir.y == 0 && vecDir.z == 0 )
		return pev->angles.y;

	return UTIL_VecToYaw( vecDir );
}

void CWallHealthNew::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CWallHealthNewCanister *pCanister = (CWallHealthNewCanister*)(CBaseEntity*)m_hCanister;

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
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/medshotno1.wav", 1.0, ATTN_NORM );
		}
		return;
	}

	m_flOffTime = gpGlobals->time + 0.25;

	if ( m_flNextCharge >= gpGlobals->time )
		return;

	if ( !m_iOn )
	{
		m_iOn++;
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/medshot4.wav", 1.0, ATTN_NORM );
		m_flSoundTime = gpGlobals->time + 0.56;
		pCanister->SetSloshing( TRUE );
		pev->sequence = LookupSequence( "give_shot" );
		pev->frame = 0.0;
		ResetSequenceInfo();
	}

	if ( m_iOn == 1 && m_flSoundTime <= gpGlobals->time )
	{
		m_iOn++;
		EMIT_SOUND( ENT( pev ), CHAN_STATIC, "items/medcharge4.wav", 1.0, ATTN_NORM );
	}

	// gearbox moment: redundant activator null and player check
	if ( pActivator && pActivator->IsPlayer() )
	{
		pActivator->TakeDamage( pActivator->pev, pActivator->pev, 1.0, DMG_GENERIC );
		m_iJuice--;
		// gearbox moment: if alien mode is active, the amount of liquid in the canister isn't updated
		// this is probably a bug so i'm going to make it update by default
		pCanister->SetLiquid( ( m_iJuice / gSkillData.healthchargerCapacity ) * 100 );
	}

	if ( m_iJuice < 1 )
	{
		pev->skin = 1;
		Off();
	}

	m_flNextCharge = gpGlobals->time + 0.1;
}
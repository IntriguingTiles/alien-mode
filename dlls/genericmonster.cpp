/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// Generic Monster - purely for scripted sequence work.
//=========================================================
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "soundent.h"

// For holograms, make them not solid so the player can walk through them
#define	SF_GENERICMONSTER_NOTSOLID					4 
#define SF_TURNHEAD									8

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CGenericMonster : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int  Classify () override;
	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;
	int ISoundMask () override;

	bool KeyValue( KeyValueData *pkvd );
	void PlayScriptedSentence( const char *pszSentence, float duration, float volume, float attenuation, bool bConcurrent, CBaseEntity *pListener );
	void MonsterThink();
	void IdleHeadTurn( Vector &vecFriend );
	bool TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	bool Save( CSave &save );
	bool Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	float m_talkTime;
	EHANDLE m_hTalkTarget;
	float m_flIdealYaw;
	float m_flCurrentYaw;
	int m_nNumHits;
	int m_nNumNoHits;
	int m_nTriggerHits;
	string_t m_iszTargetTrigger;
	string_t m_iszNoTargetTrigger;
};

TYPEDESCRIPTION CGenericMonster::m_SaveData[] = {
	DEFINE_FIELD( CGenericMonster, m_talkTime, FIELD_FLOAT ), // gearbox moment: should be FIELD_TIME
	DEFINE_FIELD( CGenericMonster, m_hTalkTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( CGenericMonster, m_flIdealYaw, FIELD_FLOAT ),
	DEFINE_FIELD( CGenericMonster, m_flCurrentYaw, FIELD_FLOAT ),
	DEFINE_FIELD( CGenericMonster, m_nNumHits, FIELD_INTEGER ),
	DEFINE_FIELD( CGenericMonster, m_nNumNoHits, FIELD_INTEGER ),
	DEFINE_FIELD( CGenericMonster, m_nTriggerHits, FIELD_INTEGER ),
	DEFINE_FIELD( CGenericMonster, m_iszTargetTrigger, FIELD_STRING ),
	DEFINE_FIELD( CGenericMonster, m_iszNoTargetTrigger, FIELD_STRING )
};

IMPLEMENT_SAVERESTORE( CGenericMonster, CBaseMonster );

LINK_ENTITY_TO_CLASS( monster_generic, CGenericMonster );

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CGenericMonster::Classify()
{
	if ( pev->spawnflags & 0x2000 )
		return CLASS_NONE;

	return	CLASS_PLAYER_ALLY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGenericMonster::SetYawSpeed()
{
	int ys;

	switch (m_Activity)
	{
	case ACT_IDLE:
	default:
		ys = 90;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGenericMonster::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case 0:
	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// ISoundMask - generic monster can't hear.
//=========================================================
int CGenericMonster::ISoundMask()
{
	return bits_SOUND_NONE;
}

//=========================================================
// Spawn
//=========================================================
void CGenericMonster::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), STRING(pev->model));

	/*
	if ( FStrEq( STRING(pev->model), "models/player.mdl" ) )
		UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	else
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);
*/

	if (FStrEq(STRING(pev->model), "models/player.mdl") || FStrEq(STRING(pev->model), "models/holo.mdl"))
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);
	else
		UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;

	if ( !m_iszKillTarget && !m_iszNoTargetTrigger )
		m_bloodColor	= BLOOD_COLOR_RED;
	else
		m_bloodColor	= DONT_BLEED;

	if ( pev->max_health > 0.0 )
		pev->health = pev->max_health;
	else
		pev->health			= 8;

	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();

	if ((pev->spawnflags & SF_GENERICMONSTER_NOTSOLID) != 0)
	{
		pev->solid = SOLID_NOT;
		pev->takedamage = DAMAGE_NO;
	}

	if ( pev->spawnflags & SF_TURNHEAD )
	{
		m_afCapability = bits_CAP_TURN_HEAD;
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGenericMonster::Precache()
{
	PRECACHE_MODEL((char*)STRING(pev->model));
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

bool CGenericMonster::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "target_trigger" ) )
	{
		m_iszTargetTrigger = ALLOC_STRING( pkvd->szValue );
		return true;
	}
	else if ( FStrEq( pkvd->szKeyName, "no_target_trigger" ) )
	{
		m_iszNoTargetTrigger = ALLOC_STRING( pkvd->szValue );
		return true;
	}
	else if ( FStrEq( pkvd->szKeyName, "trigger_hits" ) )
	{
		m_nTriggerHits = atoi( pkvd->szValue );
		return true;
	}
	else
	{
		return CBaseMonster::KeyValue( pkvd );
	}
}

void CGenericMonster::PlayScriptedSentence( const char *pszSentence, float duration, float volume, float attenuation, bool bConcurrent, CBaseEntity *pListener )
{
	CBaseMonster::PlayScriptedSentence( pszSentence, duration, volume, attenuation, bConcurrent, pListener );

	m_talkTime = gpGlobals->time + duration;
	m_hTalkTarget = pListener;
}

void CGenericMonster::MonsterThink()
{
	if ( m_afCapability & bits_CAP_TURN_HEAD )
	{
		CBaseEntity *pTalkTarget = m_hTalkTarget;

		if ( pTalkTarget )
		{
			if ( m_talkTime >= gpGlobals->time )
			{
				IdleHeadTurn( pTalkTarget->pev->origin );
			}
			else
			{
				m_flIdealYaw = 0.0;
				m_hTalkTarget = NULL;
			}
		}

		if ( m_flIdealYaw != m_flCurrentYaw )
		{
			if ( m_flIdealYaw < m_flCurrentYaw )
				m_flCurrentYaw -= V_min( m_flCurrentYaw - m_flIdealYaw, 20.0 );
			else
				m_flCurrentYaw += V_min( m_flIdealYaw - m_flCurrentYaw, 20.0 );
		}

		SetBoneController( 0, m_flCurrentYaw );
	}

	CBaseMonster::MonsterThink();
}

void CGenericMonster::IdleHeadTurn( Vector &vecFriend )
{
	// turn head in desired direction only if ent has a turnable head
	if ( m_afCapability & bits_CAP_TURN_HEAD )
	{
		float yaw = VecToYaw( vecFriend - pev->origin ) - pev->angles.y;

		if ( yaw > 180 )
			yaw -= 360;
		if ( yaw < -180 )
			yaw += 360;

		m_flIdealYaw = yaw;
	}
}

bool CGenericMonster::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( !FNullEnt( pevAttacker ) )
	{
		CBaseEntity *pAttacker = CBaseEntity::Instance( pevAttacker );

		if ( m_iszTargetTrigger && pAttacker->IsPlayer() )
		{
			// there's a check here that determines if the player used auto-aim to hit us
			// but since that's not implemented here, let's always claim that auto-aim was used
			m_nNumNoHits = 0;
			m_nNumHits++;
		}

		if ( m_nNumHits >= m_nTriggerHits )
		{
			FireTargets( STRING( m_iszTargetTrigger ), this, this, USE_TOGGLE, 0 );
			m_nNumHits = 0;
			m_nNumNoHits = 0;
		}
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}
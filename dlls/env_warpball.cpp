#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "effects.h"
#include "weapons.h"
#include "monsters.h"

// HLPS2's env_warpball, a modified version of monstermaker

class CWarpBall : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	EXPORT void Use1( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void FireWarpBall( void );
	EXPORT void Think1( void );
	void MakeMonster( void );

	int Save( CSave &save );
	int Restore( CRestore &restore );
	
	static TYPEDESCRIPTION m_SaveData[];

	CLightning *m_pBeams;
	int m_iBeams; // unused
	float m_flLastTime;
	float m_flMaxFrame;
	float m_flBeamRadius;
	string_t m_iszWarpTarget;
	float m_flWarpStart;
	float m_flDamageDelay;
	float m_flTargetDelay;
	int m_fPlaying;
	int m_fDamageApplied;
	int m_fBeamsCleared;
	float m_flGround;
	string_t m_iszMonsterClassname;
	int m_iMonsterSpawnFlags;
	int m_fMonsterCreated;
	int m_cLiveChildren;
	int m_iMaxLiveChildren;
	int m_fFadeChildren; // unused, leftover from monstermaker
	int m_fWarpRefire;
};

TYPEDESCRIPTION CWarpBall::m_SaveData[] = {
	DEFINE_FIELD( CWarpBall, m_pBeams, FIELD_CLASSPTR ),
	DEFINE_FIELD( CWarpBall, m_iBeams, FIELD_INTEGER ),
	DEFINE_FIELD( CWarpBall, m_flLastTime, FIELD_FLOAT ),
	DEFINE_FIELD( CWarpBall, m_flMaxFrame, FIELD_FLOAT ),
	DEFINE_FIELD( CWarpBall, m_flBeamRadius, FIELD_FLOAT ),
	DEFINE_FIELD( CWarpBall, m_iszWarpTarget, FIELD_STRING ),
	DEFINE_FIELD( CWarpBall, m_flWarpStart, FIELD_FLOAT ),
	DEFINE_FIELD( CWarpBall, m_flTargetDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CWarpBall, m_fPlaying, FIELD_BOOLEAN ),
	DEFINE_FIELD( CWarpBall, m_fDamageApplied, FIELD_BOOLEAN ),
	DEFINE_FIELD( CWarpBall, m_fBeamsCleared, FIELD_BOOLEAN ),
	DEFINE_FIELD( CWarpBall, m_flGround, FIELD_FLOAT ),
	DEFINE_FIELD( CWarpBall, m_iszMonsterClassname, FIELD_STRING ),
	DEFINE_FIELD( CWarpBall, m_iMonsterSpawnFlags, FIELD_INTEGER ),
	DEFINE_FIELD( CWarpBall, m_fMonsterCreated, FIELD_INTEGER ),
	DEFINE_FIELD( CWarpBall, m_cLiveChildren, FIELD_INTEGER ),
	DEFINE_FIELD( CWarpBall, m_iMaxLiveChildren, FIELD_INTEGER ),
	DEFINE_FIELD( CWarpBall, m_fFadeChildren, FIELD_BOOLEAN ),
	DEFINE_FIELD( CWarpBall, m_fWarpRefire, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CWarpBall, CBaseEntity );

LINK_ENTITY_TO_CLASS( env_warpball, CWarpBall );

// unused
CWarpBall *CreateWarpBall( Vector *vecOrigin )
{
	CWarpBall *pWarpBall = GetClassPtr( (CWarpBall *)NULL );

	UTIL_SetOrigin( pWarpBall->pev, *vecOrigin );
	pWarpBall->pev->classname = MAKE_STRING( "env_warpball" );
	pWarpBall->Spawn();
	
	return pWarpBall;
}

void CWarpBall::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "radius" ) )
	{
		m_flBeamRadius = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "warp_target" ) )
	{
		m_iszWarpTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "damage_delay" ) )
	{
		m_flDamageDelay = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "monstertype" ) )
	{
		if ( !pkvd->szValue || pkvd->szValue[0] == '\0' )
			m_iszMonsterClassname = NULL;
		else
			m_iszMonsterClassname = ALLOC_STRING( pkvd->szValue );

		pkvd->fHandled = TRUE;		
	}
	else if ( FStrEq( pkvd->szKeyName, "monsterspawnflags" ) )
	{
		m_iMonsterSpawnFlags = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "maxlivechildren" ) )
	{
		m_iMaxLiveChildren = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseEntity::KeyValue(pkvd);
	}
}

void CWarpBall::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	pev->rendermode = kRenderGlow;
	pev->renderamt = 255.0;
	pev->renderfx = kRenderFxNoDissipation;
	pev->framerate = 10.0;
	SetUse( &CWarpBall::Use1 );

	if ( pev->spawnflags & 2 && m_flDamageDelay == 0.0 )
		m_flDamageDelay = 1.0;
}

void CWarpBall::Precache()
{
	PRECACHE_MODEL( "sprites/warpball.spr" );
	PRECACHE_MODEL( "sprites/lgtning.spr" );
	PRECACHE_SOUND( "debris/alien_teleport.wav" );

	if ( m_iszMonsterClassname )
		UTIL_PrecacheOther( STRING( m_iszMonsterClassname ) );
}

void CWarpBall::Use1( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	FireWarpBall();
}

void CWarpBall::FireWarpBall()
{
	if ( m_iMaxLiveChildren > 0 && m_cLiveChildren >= m_iMaxLiveChildren )
	{ // not allowed to make a new one yet. Too many live ones out right now.
		return;
	}

	if ( m_iszWarpTarget )
	{
		edict_t *pEnt = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( m_iszWarpTarget ) );

		if ( pEnt )
			UTIL_SetOrigin( pev, pEnt->v.origin );
	}

	if ( !( pev->spawnflags & 2 ) )
	{
		if ( !m_flGround )
		{
			// set altitude. Now that I'm activated, any breakables, etc should be out from under me.
			TraceResult tr;

			UTIL_TraceLine( pev->origin, pev->origin - Vector( 0, 0, 2048 ), ignore_monsters, ENT( pev ), &tr );
			m_flGround = tr.vecEndPos.z;
		}

		Vector mins = pev->origin - Vector( 34, 34, 0 );
		Vector maxs = pev->origin + Vector( 34, 34, 0 );
		maxs.z = pev->origin.z;
		mins.z = m_flGround;

		CBaseEntity *pList[2];
		int count = UTIL_EntitiesInBox( pList, 2, mins, maxs, FL_CLIENT | FL_MONSTER );
		if ( count )
		{
			SetThink( &CWarpBall::FireWarpBall );
			pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 2.0, 4.0 );
			return;
		}

	}

	m_fWarpRefire = FALSE;

	SET_MODEL( ENT( pev ), "sprites/warpball.spr" );
	m_flMaxFrame = MODEL_FRAMES( pev->modelindex ) - 1;
	pev->rendercolor = { 77, 210, 130 };
	pev->scale = 1.2;
	pev->frame = 0.0;

	if ( !m_pBeams )
	{
		m_pBeams = CLightning::LightningCreate( "sprites/lgtning.spr", 18 );

		if ( m_pBeams )
		{
			m_pBeams->m_iszSpriteName = MAKE_STRING( "sprites/lgtning.spr" );
			m_pBeams->pev->origin = pev->origin;
			UTIL_SetOrigin( m_pBeams->pev, pev->origin );

			// there's a check here to set restrike to -0.3 if the game is coop (presumably for perf)
			// but we're not implementing any coop stuff so i'm skipping that

			m_pBeams->m_restrike = -0.5;
			m_pBeams->m_noiseAmplitude = 65;
			m_pBeams->m_boltWidth = 18;
			m_pBeams->m_life = 0.5;
			m_pBeams->SetColor( 0, 255, 0 );
			m_pBeams->pev->spawnflags |= SF_BEAM_SPARKEND | SF_BEAM_TOGGLE;
			m_pBeams->m_radius = m_flBeamRadius;
			m_pBeams->m_iszStartEntity = pev->targetname;
			m_pBeams->BeamUpdateVars();
		}
	}

	if ( m_pBeams )
	{
		m_pBeams->pev->solid = SOLID_NOT;
		m_pBeams->Precache();
		m_pBeams->SetThink( &CLightning::StrikeThink );
		m_pBeams->pev->nextthink = gpGlobals->time + 0.1;
	}

	SetThink( &CWarpBall::Think1 );
	pev->nextthink = gpGlobals->time + 0.1;
	m_fBeamsCleared = FALSE;
	m_flLastTime = gpGlobals->time;
	m_fPlaying = TRUE;
	m_fDamageApplied = FALSE;

	if ( pev->spawnflags & 2 && m_flDamageDelay == 0.0 )
	{
		RadiusDamage( pev->origin, pev, pev, 300.0, 48.0, CLASS_NONE, DMG_SHOCK );
		m_fDamageApplied = TRUE;
	}

	SUB_UseTargets( this, USE_TOGGLE, 0 );
	UTIL_ScreenShake( pev->origin, 4.0, 100.0, 2.0, 1000.0 );
	m_flWarpStart = gpGlobals->time;
	EMIT_SOUND_DYN( pev->pContainingEntity, CHAN_WEAPON, "debris/alien_teleport.wav", 1.0, ATTN_NORM, 0, 100 );
}

void CWarpBall::Think1()
{
	pev->frame += pev->framerate * ( gpGlobals->time - m_flLastTime );

	if ( pev->frame > m_flMaxFrame )
	{
		SET_MODEL( ENT( pev ), "" );
		SetThink( NULL );

		if ( pev->spawnflags & 1 )
			UTIL_Remove( this );

		m_fPlaying = FALSE;

		if ( m_fWarpRefire )
		{
			SetThink( &CWarpBall::FireWarpBall );
			pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 2.0, 4.0 );
		}
	}
	else
	{
		if ( m_iszMonsterClassname )
		{
			if ( !m_fMonsterCreated )
			{
				if ( gpGlobals->time - m_flWarpStart >= V_max( m_flDamageDelay + 0.2, 1.0 ) )
				{
					MakeMonster();
					m_fMonsterCreated = TRUE;
				}
			}
		}

		if ( pev->frame >= m_flMaxFrame - 4.0 )
		{
			m_pBeams->SetThink( NULL );
			m_pBeams->pev->nextthink = gpGlobals->time;
		}

		pev->nextthink = gpGlobals->time + 0.1;
		m_flLastTime = gpGlobals->time;
	}
}

void CWarpBall::MakeMonster()
{
	edict_t *pent;
	entvars_t *pevCreate;

	if ( m_iMaxLiveChildren > 0 && m_cLiveChildren >= m_iMaxLiveChildren )
	{ // not allowed to make a new one yet. Too many live ones out right now.
		return;
	}

	if ( !m_flGround )
	{
		// set altitude. Now that I'm activated, any breakables, etc should be out from under me.
		TraceResult tr;

		UTIL_TraceLine( pev->origin, pev->origin - Vector( 0, 0, 2048 ), ignore_monsters, ENT( pev ), &tr );
		m_flGround = tr.vecEndPos.z;
	}

	Vector mins = pev->origin - Vector( 34, 34, 0 );
	Vector maxs = pev->origin + Vector( 34, 34, 0 );
	maxs.z = pev->origin.z;
	mins.z = m_flGround;

	CBaseEntity *pList[2];
	int count = UTIL_EntitiesInBox( pList, 2, mins, maxs, FL_CLIENT | FL_MONSTER );
	if ( count )
	{
		// don't build a stack of monsters!
		if ( pev->spawnflags & 2 )
		{
			for ( int i = 0; i < count; i++ )
			{
				pList[i]->TakeDamage( pev, pev, 300.0, DMG_SHOCK );
			}
		}
		else
		{
			m_fWarpRefire = TRUE;
			return;
		}
	}

	pent = CREATE_NAMED_ENTITY( m_iszMonsterClassname );

	if ( FNullEnt( pent ) )
	{
		ALERT( at_console, "NULL Ent in MonsterMaker!\n" );
		return;
	}

	pevCreate = VARS( pent );
	pevCreate->origin = pev->origin;
	pevCreate->angles = pev->angles;
	SetBits( pevCreate->spawnflags, m_iMonsterSpawnFlags | SF_MONSTER_FALL_TO_GROUND );

	DispatchSpawn( ENT( pevCreate ) );
	pevCreate->owner = edict();

	if ( !FStringNull( pev->netname ) )
	{
		// if I have a netname (overloaded), give the child monster that name as a targetname
		pevCreate->targetname = pev->netname;
	}

	m_cLiveChildren++; // count this monster
}
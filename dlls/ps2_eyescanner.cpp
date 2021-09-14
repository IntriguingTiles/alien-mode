#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"

// HLPS2's eye scanner

class CEyeScanner : public CBaseToggle
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	EXPORT void UseThink( void );
	int ObjectCaps( void ) { return FCAP_IMPULSE_USE; }
	int Save( CSave &save );
	int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	float m_flStopUseTime;
	CBaseEntity *m_pActivator;
	string_t m_iszLockedTarget;
	string_t m_iszUnlockedTarget;
	int m_iMoving;
	float m_flResetDelay;
	float m_flNextUseTime;
};

TYPEDESCRIPTION CEyeScanner::m_SaveData[] = {
	DEFINE_FIELD( CEyeScanner, m_flStopUseTime, FIELD_TIME ),
	DEFINE_FIELD( CEyeScanner, m_pActivator, FIELD_CLASSPTR ),
	DEFINE_FIELD( CEyeScanner, m_iszLockedTarget, FIELD_STRING ),
	DEFINE_FIELD( CEyeScanner, m_iszUnlockedTarget, FIELD_STRING ),
	DEFINE_FIELD( CEyeScanner, m_iMoving, FIELD_INTEGER ),
	DEFINE_FIELD( CEyeScanner, m_flResetDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CEyeScanner, m_flNextUseTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CEyeScanner, CBaseToggle );

LINK_ENTITY_TO_CLASS( item_eyescanner, CEyeScanner );

void CEyeScanner::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "locked_target" ) )
	{
		m_iszLockedTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "unlocked_target" ) )
	{
		m_iszUnlockedTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "reset_delay" ) )
	{
		m_flResetDelay = atof( pkvd->szValue );
	}

	CBaseToggle::KeyValue( pkvd );
}

void CEyeScanner::Spawn()
{
	// not sure why gearbox did this
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0.0;
	pev->skin = 0;
	Precache();
	SET_MODEL( ENT( pev ), "models/eye_scanner.mdl" );

	if ( pev->angles.y == 0.0 || pev->angles.y == 180.0 )
	{
		UTIL_SetSize( pev, Vector( -3.0, -5.0, 0.0 ), Vector( 3.0, 5.0, 64.0 ) );
	}
	else
	{
		UTIL_SetSize( pev, Vector( -5.0, -3.0, 0.0 ), Vector( 5.0, 3.0, 64.0 ) );
	}
}

void CEyeScanner::Precache()
{
	PRECACHE_MODEL( "models/eye_scanner.mdl" );
}

void CEyeScanner::UseThink()
{
	DispatchAnimEvents();
	StudioFrameAdvance();

	if ( m_fSequenceFinished )
	{
		switch ( m_iMoving )
		{
			case 1:
				pev->sequence = LookupSequence( "idle_OPEN" );
				pev->frame = 0.0;
				ResetSequenceInfo();
				m_iMoving = 0;
				m_flStopUseTime = gpGlobals->time + 2.5;
				break;
			case 2:
				pev->sequence = LookupSequence( "idle_CLOSED" );
				pev->frame = 0.0;
				ResetSequenceInfo();
				m_iMoving = 0;
				SetThink( NULL );
				pev->nextthink = gpGlobals->time;
				return;
		}
	}

	if ( m_iMoving == 0 )
	{
		pev->skin++;

		if ( ( pev->skin - 1 ) > 2 )
			pev->skin = 1;

		if ( m_flStopUseTime <= gpGlobals->time )
		{
			pev->skin = 0;
			m_pActivator = NULL;
			pev->nextthink = gpGlobals->time + 0.1;
			pev->sequence = LookupSequence( "DEACTIVATE" );
			pev->frame = 0.0;
			ResetSequenceInfo();
			m_iMoving = 2;
		}
		else
		{
			pev->nextthink = gpGlobals->time + 0.15;
		}
	}
	else
	{
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CEyeScanner::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( m_pActivator )
		return;

	if ( m_flNextUseTime >= gpGlobals->time )
		return;

	if ( pActivator && ( pActivator->pev->origin - pev->origin ).Length2D() > 24.0 )
		return;

	SetThink( &CEyeScanner::UseThink );
	pev->nextthink = gpGlobals->time + 0.1;
	m_pActivator = pActivator;
	m_iMoving = 1;
	pev->sequence = LookupSequence( "ACTIVATE" );
	pev->frame = 0.0;
	ResetSequenceInfo();
	m_flNextUseTime = gpGlobals->time + m_flResetDelay;

	if ( pev->spawnflags & 1 )
	{
		if ( m_pActivator && m_pActivator->IsPlayer() )
		{
			if ( !m_sMaster || !UTIL_IsMasterTriggered( m_sMaster, m_pActivator ) )
			{
				FireTargets( STRING( m_iszLockedTarget ), m_pActivator, this, USE_TOGGLE, 0 );
				return;
			}
		}
	}

	FireTargets( STRING( m_iszUnlockedTarget ), m_pActivator, this, USE_TOGGLE, 0 );
}

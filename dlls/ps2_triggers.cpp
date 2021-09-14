#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "saverestore.h"
#include "alien_util.h"

// HLPS2 has several new triggers that need to be implemented:
// trigger_playerfreeze  - toggles the FL_FROZEN flag on all players
// trigger_player_islave - fires the target if alien mode is active
// trigger_random        - stupid trigger used in a SINGLE map for a silly easter egg

class CTriggerPlayerFreeze : public CBaseDelay
{
public:
	void Spawn( void );
	void EXPORT FreezeThink( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	BOOL m_bUnFrozen = true;
};

LINK_ENTITY_TO_CLASS( trigger_playerfreeze, CTriggerPlayerFreeze );

TYPEDESCRIPTION CTriggerPlayerFreeze::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerPlayerFreeze, m_bUnFrozen, FIELD_BOOLEAN )
};

// because gearbox chose to modify Restore, we are forced to implement Save

int CTriggerPlayerFreeze::Save( CSave &save )
{
	if ( !CBaseDelay::Save(save) )
		return 0;
	return save.WriteFields( "CTriggerPlayerFreeze", this, m_SaveData, ARRAYSIZE(m_SaveData) );
}

int CTriggerPlayerFreeze::Restore( CRestore &restore )
{
	if ( !CBaseDelay::Restore(restore) )
		return 0;

	int ret = restore.ReadFields("CTriggerPlayerFreeze", this, m_SaveData, ARRAYSIZE(m_SaveData) );

	if ( ret && m_bUnFrozen == FALSE )
	{
		SetThink( &CTriggerPlayerFreeze::FreezeThink );
		// gearbox moment:
		// this would allow players to move slightly when loading a save
		// although this is impractical on PS2 since it takes ages to save and load
		pev->nextthink = gpGlobals->time + 0.5;
	}

	return ret;
}

void CTriggerPlayerFreeze::Spawn()
{
	m_bUnFrozen = TRUE;
}

void CTriggerPlayerFreeze::FreezeThink()
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity* pPlayer = UTIL_PlayerByIndex( i );

		if ( pPlayer )
			UTIL_FreezePlayer( pPlayer, m_bUnFrozen );
	}

	SetThink( NULL );
}

void CTriggerPlayerFreeze::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_bUnFrozen = !m_bUnFrozen;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity* pPlayer = UTIL_PlayerByIndex( i );

		if ( pPlayer )
			UTIL_FreezePlayer( pPlayer, m_bUnFrozen );
	}
}

class CTriggerPlayerISlave : public CBaseEntity
{
	void Spawn( void );
	void EXPORT TriggerUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT TriggerThink( void );
};

LINK_ENTITY_TO_CLASS( trigger_player_islave, CTriggerPlayerISlave );

void CTriggerPlayerISlave::Spawn()
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->effects = EF_NODRAW;
	
	SetUse( &CTriggerPlayerISlave::TriggerUse );
}

void CTriggerPlayerISlave::TriggerUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( &CTriggerPlayerISlave::TriggerThink );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CTriggerPlayerISlave::TriggerThink()
{
	CBaseEntity *pPlayer = CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );

	if ( !pPlayer || !pPlayer->IsPlayer() )
	{
		pev->nextthink = gpGlobals->time + 0.3;
	}
	else
	{
		SUB_UseTargets( this, USE_ON, 0 );
	}
}

class CTriggerRandom : public CBaseDelay
{
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	int triggerType;
	int m_iRandomRange;
	int m_dwUsedNumbers;
	int m_iNextSequence;
};

TYPEDESCRIPTION CTriggerRandom::m_SaveData[] = {
	DEFINE_FIELD( CTriggerRandom, triggerType, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerRandom, m_iRandomRange, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerRandom, m_dwUsedNumbers, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerRandom, m_iNextSequence, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CTriggerRandom, CBaseDelay );

LINK_ENTITY_TO_CLASS( trigger_random, CTriggerRandom );

void CTriggerRandom::Spawn()
{
	m_iNextSequence = 0;
	m_dwUsedNumbers = -1;
}

void CTriggerRandom::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "triggerstate" ) )
	{
		switch ( atoi( pkvd->szValue ) )
		{
			case 0:
				triggerType = 0;
				break;
			case 2:
				triggerType = 3;
				break;
			default:
				triggerType = 1;
		}

		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "randomrange" ) )
	{
		m_iRandomRange = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseDelay::KeyValue( pkvd );
	}
}

// this function might not be right, though it seems to work correctly
void CTriggerRandom::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !pev->target && !m_iszKillTarget )
		return;

	int iRand = RANDOM_LONG( 0, m_iRandomRange - 1 );

	if ( !( pev->spawnflags & 2 ) )
	{
		if ( pev->spawnflags & 4 && m_iRandomRange <= ++m_iNextSequence )
		{
			m_iNextSequence = 0;
		}
	}
	else
	{

		if ( m_dwUsedNumbers == -1 )
		{
			m_dwUsedNumbers = -1 << m_iRandomRange;
		}
		else if ( m_dwUsedNumbers & ( 1 << ( iRand & 0x1F ) ) )
		{

			for ( int i = 0; i < m_iRandomRange; i++ )
			{
				iRand = RANDOM_LONG( 0, m_iRandomRange - 1 );
				
				if ( !( m_dwUsedNumbers & ( 1 << ( iRand & 0x1F ) ) ) )
					break;
			}

			if ( m_dwUsedNumbers & ( 1 << ( iRand & 0x1F ) ) )
			{
				if ( m_iRandomRange >= 1 && m_dwUsedNumbers & 1 )
				{
					for ( iRand = 1; iRand < m_iRandomRange; iRand++ )
					{
						if ( !( m_dwUsedNumbers & ( 1 << ( iRand & 0x1F ) ) ) )
							break;
					}
				}
			}
		}

		m_dwUsedNumbers |= ( 1 << ( iRand & 0x1F ) );
	}

	char szTarget[64];
	sprintf( szTarget, "%s%ld", STRING( pev->target ), iRand);

	// slightly modified SUB_UseTargets

	//
	// check for a delay
	//
	if ( m_flDelay != 0 )
	{
		// create a temp object to fire at a later time
		CBaseDelay *pTemp = GetClassPtr( (CBaseDelay *)NULL );
		pTemp->pev->classname = MAKE_STRING( "DelayedUse" );

		pTemp->pev->nextthink = gpGlobals->time + m_flDelay;

		pTemp->SetThink( &CBaseDelay::DelayThink );

		// Save the useType
		pTemp->pev->button = (int)useType;
		pTemp->m_iszKillTarget = m_iszKillTarget;
		pTemp->m_flDelay = 0; // prevent "recursion"
		pTemp->pev->target = MAKE_STRING( szTarget );

		// HACKHACK
		// This wasn't in the release build of Half-Life.  We should have moved m_hActivator into this class
		// but changing member variable hierarchy would break save/restore without some ugly code.
		// This code is not as ugly as that code
		if ( pActivator && pActivator->IsPlayer() ) // If a player activates, then save it
		{
			pTemp->pev->owner = pActivator->edict();
		}
		else
		{
			pTemp->pev->owner = NULL;
		}

		return;
	}

	//
	// kill the killtargets
	//

	if ( m_iszKillTarget )
	{
		edict_t *pentKillTarget = NULL;

		ALERT( at_aiconsole, "KillTarget: %s\n", STRING( m_iszKillTarget ) );
		pentKillTarget = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( m_iszKillTarget ) );
		while ( !FNullEnt( pentKillTarget ) )
		{
			UTIL_Remove( CBaseEntity::Instance( pentKillTarget ) );

			ALERT( at_aiconsole, "killing %s\n", STRING( pentKillTarget->v.classname ) );
			pentKillTarget = FIND_ENTITY_BY_TARGETNAME( pentKillTarget, STRING( m_iszKillTarget ) );
		}
	}

	//
	// fire targets
	//
	if ( !FStringNull( pev->target ) )
	{
		FireTargets( szTarget, pActivator, this, useType, value );
	}

	if ( pev->spawnflags & 1 )
	{
		UTIL_Remove( this );
	}
}
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "saverestore.h"
#include "alien_util.h"

// alien mode has two new triggers that need to be implemented:
// trigger_playerfreeze  - toggles the FL_FROZEN flag on all players
// trigger_player_islave - fires the target if alien mode is active

class CTriggerPlayerFreeze : public CBaseDelay
{
public:
	void Spawn( void );
	void EXPORT FreezeThink( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
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
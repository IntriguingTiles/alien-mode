#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "alien_util.h"

// misc util functions added by HLPS2

void UTIL_FreezePlayer( CBaseEntity* pPlayer, BOOL bUnFreeze )
{
	if ( bUnFreeze ) pPlayer->pev->flags &= !FL_FROZEN;
	else pPlayer->pev->flags |= FL_FROZEN;
}
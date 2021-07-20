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

void UTIL_GaussianSpread( Vector &a, Vector &b, Vector c )
{
	Vector forward, right, up;

	UTIL_MakeVectorsPrivate( b, forward, right, up );

	float x, y, z;
	do
	{
		x = RANDOM_FLOAT( -0.5, 0.5 ) + RANDOM_FLOAT( -0.5, 0.5 );
		y = RANDOM_FLOAT( -0.5, 0.5 ) + RANDOM_FLOAT( -0.5, 0.5 );
		z = x * x + y * y;
	} while ( z > 1 );

	x *= c.x;
	y *= c.y;
	a = b + right * x + up * y;
}
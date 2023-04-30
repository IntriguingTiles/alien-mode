/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "ps2_util.h"


#define CROWBAR_BODYHIT_VOLUME 128
#define CROWBAR_WALLHIT_VOLUME 512

LINK_ENTITY_TO_CLASS( weapon_islave, CWeaponISlave );

#ifndef CLIENT_DLL
extern int gmsgISlaveHud;

	#define SEND_HUD_MSG()                                                                                             \
		MESSAGE_BEGIN( MSG_ONE, gmsgISlaveHud, NULL, m_pPlayer->pev );                                                 \
		WRITE_BYTE( m_iHudState );                                                                                     \
		MESSAGE_END();
#else
	#define SEND_HUD_MSG()
#endif

void CWeaponISlave::Spawn()
{
	Precache();
	m_iId = WEAPON_ISLAVE;
	SET_MODEL( ENT( pev ), "models/w_crowbar.mdl" );
	m_flUpdateBeamTime = 0.0;
	m_iHudState = 0;
	m_iClip = -1;

	FallInit(); // get ready to fall down.
}

void CWeaponISlave::Precache()
{
	PRECACHE_MODEL( "models/v_slave.mdl" );
	PRECACHE_MODEL( "models/w_crowbar.mdl" );
	PRECACHE_MODEL( "models/p_crowbar.mdl" );
	PRECACHE_MODEL( "sprites/lgtning.spr" );
	PRECACHE_SOUND( "debris/zap1.wav" );
	PRECACHE_SOUND( "debris/zap4.wav" );
	PRECACHE_SOUND( "zombie/claw_strike1.wav" );
	PRECACHE_SOUND( "zombie/claw_strike2.wav" );
	PRECACHE_SOUND( "zombie/claw_strike3.wav" );
	PRECACHE_SOUND( "zombie/claw_miss1.wav" );
	PRECACHE_SOUND( "zombie/claw_miss2.wav" );
	PRECACHE_SOUND( "aslave/slv_word5.wav" );
	PRECACHE_SOUND( "aslave/slv_word7.wav" );
	PRECACHE_SOUND( "aslave/slv_word3.wav" );
	PRECACHE_SOUND( "aslave/slv_word4.wav" );
	PRECACHE_SOUND( "scientist/gibberish01.wav" );
	PRECACHE_SOUND( "scientist/gibberish02.wav" );
	PRECACHE_SOUND( "scientist/gibberish03.wav" );
	PRECACHE_SOUND( "scientist/gibberish04.wav" );
	PRECACHE_SOUND( "scientist/gibberish05.wav" );
	PRECACHE_SOUND( "scientist/gibberish06.wav" );
	PRECACHE_SOUND( "scientist/gibberish07.wav" );
	PRECACHE_SOUND( "scientist/gibberish08.wav" );
	PRECACHE_SOUND( "scientist/gibberish09.wav" );
	PRECACHE_SOUND( "scientist/gibberish10.wav" );
	PRECACHE_SOUND( "scientist/gibberish11.wav" );
	PRECACHE_SOUND( "scientist/gibberish12.wav" );

	for ( int i = 0; i < 8; i++ )
	{
		if ( m_pBeam[i] )
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}

	m_iBeams = 0;

	for ( int i = 0; i < 4; i++ )
	{
		if ( m_pBeam2[i] )
		{
			UTIL_Remove( m_pBeam2[i] );
			m_pBeam2[i] = NULL;
		}
	}
}

int CWeaponISlave::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 2;
	p->iId = WEAPON_ISLAVE;
	p->iWeight = -1;
	return 1;
}

BOOL CWeaponISlave::Deploy()
{
	m_pPlayer->m_iHideHUD |= HIDEHUD_HEALTH | HIDEHUD_FLASHLIGHT | HIDEHUD_WEAPONS;

	if ( m_pPlayer->pev->weapons & ( 1 << WEAPON_CROWBAR ) )
	{
		// the only way to obtain other weapons is via impulse 101
		// therefore if the player has a crowbar, they have other weapons
		// and they should be allowed to switch between them
		m_pPlayer->m_iHideHUD &= ~HIDEHUD_WEAPONS;
	}

	m_iHudState = 0;
	SEND_HUD_MSG();
	return DefaultDeploy( "models/v_slave.mdl", "models/p_crowbar.mdl", SLAVE_IDLE1, "crowbar" );
}

void CWeaponISlave::Holster( int skiplocal /* = 0 */ )
{
	if ( m_pPlayer->pev->weapons & ( 1 << WEAPON_CROWBAR ) )
		m_pPlayer->m_iHideHUD &= ~( HIDEHUD_HEALTH | HIDEHUD_FLASHLIGHT | HIDEHUD_WEAPONS );
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	ClearBeams();
	m_flZapTime = 0.0;
	m_iHudState = 0;
	SEND_HUD_MSG();
	m_pPlayer->pev->viewmodel = 0;
	m_pPlayer->pev->weaponmodel = 0;
}

void FindHullIntersection( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity );

void CWeaponISlave::PrimaryAttack()
{
	int fDidHit = FALSE;

	TraceResult tr;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32;

	ClearBeams();

	if ( m_iHudState != 0 )
	{
		m_iHudState = 0;
		SEND_HUD_MSG();
	}

	m_flZapTime = 0.0;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

#ifndef CLIENT_DLL
	if ( tr.flFraction >= 1.0 )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
		if ( tr.flFraction < 1.0 )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if ( !pHit || pHit->IsBSPModel() )
				FindHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			vecEnd = tr.vecEndPos; // This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif

	if ( tr.flFraction >= 1.0 )
	{
		// missed
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.6;
		switch ( m_iSwing )
		{
			case 0:
				SendWeaponAnim( SLAVE_ATTACK1MISS );
				break;
			case 1:
				SendWeaponAnim( SLAVE_ATTACK2MISS );
				break;
			case 2:
				SendWeaponAnim( SLAVE_ATTACK3MISS );
				m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.75;
				break;
			default:
				SendWeaponAnim( SLAVE_ATTACK1MISS );
				m_iSwing = 0;
				break;
		}

		switch ( RANDOM_LONG( 0, 1 ) )
		{
			case 0:
				EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON, "zombie/claw_miss1.wav", 1.0, ATTN_NORM, 0, 94 + RANDOM_LONG( 0, 0xF ) );
				break;
			case 1:
				EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON, "zombie/claw_miss2.wav", 1.0, ATTN_NORM, 0, 94 + RANDOM_LONG( 0, 0xF ) );
				break;
		}
	}
	else
	{
		// hit
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.4;

		switch ( m_iSwing )
		{
			case 0:
				SendWeaponAnim( SLAVE_ATTACK1HIT );
				break;
			case 1:
				SendWeaponAnim( SLAVE_ATTACK2HIT );
				break;
			case 2:
				SendWeaponAnim( SLAVE_ATTACK3HIT );
				m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.45;
				break;
			default:
				SendWeaponAnim( SLAVE_ATTACK1HIT );
				m_iSwing = 0;
				break;
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		fDidHit = TRUE;
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		ClearMultiDamage();

		pEntity->TraceAttack( m_pPlayer->pev, gSkillData.plrDmgISlave, gpGlobals->v_forward, &tr, DMG_CLUB );

		ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

		if ( !pEntity->IsPlayer() && pEntity->pev->takedamage != DAMAGE_NO && m_pPlayer->pev->deadflag == DEAD_NO )
		{
			m_pPlayer->TakeHealth( ( 100.0 - m_pPlayer->pev->health ) * 0.1, DMG_GENERIC );
		}

		// play thwack, smack, or dong sound
		int fHitWorld = TRUE;

		if ( pEntity )
		{
			if ( pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE )
			{
				// play thwack or smack sound
				switch ( RANDOM_LONG( 0, 2 ) )
				{
					case 0:
						EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "zombie/claw_strike1.wav", 1, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) );
						break;
					case 1:
						EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "zombie/claw_strike2.wav", 1, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) );
						break;
					case 2:
						EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "zombie/claw_strike3.wav", 1, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) );
						break;
				}

				fHitWorld = FALSE;
			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if ( fHitWorld )
		{
			switch ( RANDOM_LONG( 0, 2 ) )
			{
				case 0:
					EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "zombie/claw_strike1.wav", 1, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) );
					break;
				case 1:
					EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "zombie/claw_strike2.wav", 1, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) );
					break;
				case 2:
					EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "zombie/claw_strike3.wav", 1, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) );
					break;
			}
		}

		// delay the decal a bit
		m_trHit = tr;

		// think function here is used purely for controller vibration
		// SetThink( &CWeaponISlave::Smack);
		// pev->nextthink = gpGlobals->time + 0.2;

		m_pPlayer->m_iWeaponVolume = CROWBAR_WALLHIT_VOLUME;
	}

	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_iSwing++;

	if ( 2 < m_iSwing )
		m_iSwing = 0;

		// gearbox moment: this overwrites the previously set primary attack delays
		// although this might be intentional
#ifndef CLIENT_DLL
	if ( CVAR_GET_FLOAT( "sv_alien_alt_melee" ) == 0 )
#endif
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.3;
}

void CWeaponISlave::SecondaryAttack()
{
	if ( m_flZapTime == 0.0 )
	{
		ChargeZap();
	}
	else
	{
		if ( m_flZapTime <= gpGlobals->time )
		{
			if ( m_iHudState != 1 )
			{
				FailZap();
				return;
			}
			m_iHudState = 3;
			SEND_HUD_MSG();
			m_flZapTime = gpGlobals->time + 0.5;
			SendWeaponAnim( SLAVE_CHARGE_LOOP );
		}

		m_pPlayer->pev->velocity.x = 0.0;
		m_pPlayer->pev->velocity.y = 0.0;

		if ( m_iHudState == 2 && ( m_flZapTime - gpGlobals->time <= 0.65f ) )
		{
			m_iHudState = 1;
			SEND_HUD_MSG();
		}

		if ( m_flUpdateBeamTime < gpGlobals->time )
		{
			ArmBeam( 1 );
			ArmBeam( -1 );
			BeamGlow();
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON, "debris/zap4.wav", 1.0, ATTN_NORM, 0, m_iBeams * 10 + 100 );

			m_flUpdateBeamTime = gpGlobals->time + 0.15;
		}

		// not accurate but you won't ever see it
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	}
}

void CWeaponISlave::ChargeZap()
{
	m_flZapTime = gpGlobals->time + 1.3;
	SendWeaponAnim( SLAVE_CHARGE );
	Vector vecSrc = m_pPlayer->GetGunPosition();

	// there's a check here to skip spawning a dlight if there's more than one player

#ifndef CLIENT_DLL
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
	WRITE_BYTE( TE_DLIGHT );
	WRITE_COORD( vecSrc.x ); // X
	WRITE_COORD( vecSrc.y ); // Y
	WRITE_COORD( vecSrc.z ); // Z
	WRITE_BYTE( 12 );		 // radius * 0.1
	WRITE_BYTE( 255 );		 // r
	WRITE_BYTE( 180 );		 // g
	WRITE_BYTE( 96 );		 // b
	WRITE_BYTE( 13 );		 // time
	WRITE_BYTE( 0 );		 // decay * 0.1
	MESSAGE_END();
#endif

	m_iHudState = 2;
	SEND_HUD_MSG();
	m_flUpdateBeamTime = gpGlobals->time;
}

void CWeaponISlave::FailZap()
{
	m_flZapTime = 0.0;
	m_iHudState = 0;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.5;
	SEND_HUD_MSG();
	SendWeaponAnim( SLAVE_RETURN );
	ClearBeams();
}

void CWeaponISlave::ClearBeams()
{
	for ( int i = 0; i < 8; i++ )
	{
		if ( m_pBeam[i] )
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}

	m_iBeams = 0;
	pev->skin = 0;

	for ( int i = 0; i < 4; i++ )
	{
		if ( m_pBeam2[i] )
		{
			UTIL_Remove( m_pBeam2[i] );
			m_pBeam2[i] = NULL;
		}
	}

	STOP_SOUND( ENT( pev ), CHAN_WEAPON, "debris/zap4.wav" );
}

void CWeaponISlave::BeamGlow()
{
	int b = m_iBeams * 32;
	if ( b > 255 )
		b = 255;

	for ( int i = 0; i < m_iBeams; i++ )
	{
		if ( m_pBeam[i]->GetBrightness() != 255 )
		{
			m_pBeam[i]->SetBrightness( b );
		}
	}
}

void CWeaponISlave::ArmBeam( int side )
{
	TraceResult tr;
	float flDist = 1.0;

	if ( m_iBeams >= 8 )
		return;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_up * 36 + gpGlobals->v_right * side * 16 + gpGlobals->v_forward * 32;

	for ( int i = 0; i < 3; i++ )
	{
		Vector vecAim = gpGlobals->v_right * side * RANDOM_FLOAT( 0, 1 ) + gpGlobals->v_up * RANDOM_FLOAT( -1, 1 );
		TraceResult tr1;
		UTIL_TraceLine( vecSrc, vecSrc + vecAim * 512, dont_ignore_monsters, ENT( pev ), &tr1 );
		if ( flDist > tr1.flFraction )
		{
			tr = tr1;
			flDist = tr.flFraction;
		}
	}

	// Couldn't find anything close enough
	if ( flDist == 1.0 )
		return;

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );
	if ( !m_pBeam[m_iBeams] )
		return;

	m_pBeam[m_iBeams]->PointEntInit( tr.vecEndPos, m_pPlayer->entindex() );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? 2 : 1 );
	m_pBeam[m_iBeams]->SetColor( 96, 128, 16 );
	m_pBeam[m_iBeams]->SetBrightness( 64 );
	m_pBeam[m_iBeams]->SetNoise( 80 );
	m_pBeam[m_iBeams]->pev->spawnflags |= SF_BEAM_TEMPORARY;
	m_iBeams++;
}

void CWeaponISlave::ShootBeam()
{
	SendWeaponAnim( SLAVE_ZAP );
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;
	ZapBeam( 1 );
	ZapBeam( -1 );

	for ( int i = 0; i < 8; i++ )
	{
		if ( m_pBeam[i] )
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}

	m_iBeams = 0;
	pev->skin = 0;
	m_flZapTime = 0.0;
	SetThink( &CWeaponISlave::ClearBeamsThink );
	pev->nextthink = gpGlobals->time + 0.3;
}

void CWeaponISlave::ClearBeamsThink()
{
	m_iHudState = 0;
	ClearBeams();
}

void CWeaponISlave::ZapBeam( int side )
{
#ifndef CLIENT_DLL
	Vector vecEnd;
	TraceResult tr, tr2;
	int whichSide = 0 < side;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	vecEnd = gpGlobals->v_forward;

	if ( side < 0 )
	{
		Vector vecSpread;
		UTIL_GaussianSpread( vecSpread, gpGlobals->v_forward, VECTOR_CONE_5DEGREES );
		vecEnd = vecSpread;
	}

	vecEnd = vecEnd * 8192.0 + m_pPlayer->GetGunPosition();

	UTIL_TraceLine( m_pPlayer->GetGunPosition(), vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

	m_pBeam2[whichSide] = CBeam::BeamCreate( "sprites/lgtning.spr", 50 );
	if ( !m_pBeam2[whichSide] )
		return;

	// todo: where does that number at the end come from?
	float flDamage = ( 1.3 - fabs( m_flZapTime - gpGlobals->time ) ) * 0.7692308;

	if ( flDamage > 1.0 || m_iHudState == 3 )
	{
		flDamage = 1.0;
	}

	float flActualDmg = gSkillData.plrDmgISlaveZap * flDamage;

	m_pBeam2[whichSide]->PointEntInit( tr.vecEndPos, m_pPlayer->entindex() );
	m_pBeam2[whichSide]->SetEndAttachment( side < 0 ? 2 : 1 );
	m_pBeam2[whichSide]->SetColor( 180, 255, 96 );
	m_pBeam2[whichSide]->SetBrightness( flDamage * 255 );
	m_pBeam2[whichSide]->SetNoise( 20 );
	m_pBeam2[whichSide]->pev->spawnflags |= SF_BEAM_TEMPORARY;

	if ( flDamage >= 0.95 )
	{
		Vector vecSpread;

		UTIL_GaussianSpread( vecSpread, gpGlobals->v_forward, VECTOR_CONE_15DEGREES );

		vecSpread = vecSpread * 8192 + m_pPlayer->GetGunPosition();

		UTIL_TraceLine( m_pPlayer->GetGunPosition(), vecSpread, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr2 );

		CBaseEntity *pEntity = CBaseEntity::Instance( tr2.pHit );

		if ( pEntity && pEntity->pev->takedamage != DAMAGE_NO && m_pPlayer->pev->deadflag == DEAD_NO )
		{
			ClearMultiDamage();
			pEntity->TraceAttack( m_pPlayer->pev, flActualDmg * 0.5, gpGlobals->v_forward, &tr, DMG_SHOCK );
			ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );
			if ( !pEntity->IsPlayer() && m_pPlayer->pev->deadflag == DEAD_NO )
			{
				m_pPlayer->TakeHealth( ( 100.0 - m_pPlayer->pev->health ) * 0.1, DMG_GENERIC );
			}
		}

		m_pBeam2[whichSide + 2] = CBeam::BeamCreate( "sprites/lgtning.spr", 65 );

		if ( !m_pBeam2[whichSide + 2] )
			return;

		m_pBeam2[whichSide + 2]->PointEntInit( tr2.vecEndPos, m_pPlayer->entindex() );
		m_pBeam2[whichSide + 2]->SetEndAttachment( side < 0 ? 2 : 1 );
		m_pBeam2[whichSide + 2]->SetColor( 130, 255, 70 );
		m_pBeam2[whichSide + 2]->SetBrightness( 255 );
		m_pBeam2[whichSide + 2]->SetNoise( 30 );
		m_pBeam2[whichSide + 2]->pev->spawnflags |= SF_BEAM_TEMPORARY;

		UTIL_DecalTrace( &tr2, RANDOM_LONG( 28, 32 ) );
	}

	CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

	if ( pEntity && pEntity->pev->takedamage != DAMAGE_NO && m_pPlayer->pev->deadflag == DEAD_NO )
	{
		ClearMultiDamage();
		pEntity->TraceAttack( m_pPlayer->pev, flActualDmg * 0.5, gpGlobals->v_forward, &tr, DMG_SHOCK );
		ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );
		// gearbox moment: redundant player dead check
		if ( !pEntity->IsPlayer() && m_pPlayer->pev->deadflag == DEAD_NO )
		{
			m_pPlayer->TakeHealth( ( 100.0 - m_pPlayer->pev->health ) * 0.1, DMG_GENERIC );
		}
	}

	UTIL_DecalTrace( &tr, RANDOM_LONG( 28, 32 ) );
#endif
}

void CWeaponISlave::WeaponIdle( void )
{
	// yep, gearbox really put part of the secondary attack here
	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flZapTime != 0.0 )
	{
		ShootBeam();
		m_iHudState = 0;
		SEND_HUD_MSG();
		return;
	}

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	float flVolume = RANDOM_FLOAT( 0.3, 0.8 );

	switch ( RANDOM_LONG( 0, 3 ) )
	{
		case 0:
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON, "aslave/slv_word5.wav", flVolume, ATTN_NORM, 0, 100 );
			break;
		case 1:
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON, "aslave/slv_word7.wav", flVolume, ATTN_NORM, 0, 100 );
			break;
		case 2:
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON, "aslave/slv_word3.wav", flVolume, ATTN_NORM, 0, 100 );
			break;
		case 3:
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON, "aslave/slv_word4.wav", flVolume, ATTN_NORM, 0, 100 );
			break;
	}

	int iAnim = RANDOM_LONG( 0, 1 );

	if ( iAnim == 0 )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 8.0, 15.0 );
	}
	else
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10.0, 20.0 );
	}

	SendWeaponAnim( iAnim );
}
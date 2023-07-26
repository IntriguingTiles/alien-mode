#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"

// HLPS2's item_generic

class CItemGeneric : public CBaseAnimating
{
public:
	void Spawn(void);
	void Precache(void);
	bool KeyValue(KeyValueData* pkvd);
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	EXPORT void Think1(void);
	EXPORT void Think2(void);

	float field_0x4c; // unused?
	string_t m_iszSequenceName;
};

LINK_ENTITY_TO_CLASS(item_generic, CItemGeneric);

void CItemGeneric::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;
	Precache();
	SET_MODEL(ENT(pev), STRING(pev->model));

	if (m_iszSequenceName || pev->spawnflags & 1)
	{
		SetThink(&CItemGeneric::Think1);
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CItemGeneric::Precache()
{
	PRECACHE_MODEL(STRING(pev->model));
}

bool CItemGeneric::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "sequencename"))
	{
		if (!pkvd->szValue || pkvd->szValue[0] == '\0' || pkvd->szValue[0] == '0')
		{
			m_iszSequenceName = NULL;
		}
		else
		{
			m_iszSequenceName = ALLOC_STRING(pkvd->szValue);
		}
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "skin"))
	{
		pev->skin = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "body"))
	{
		pev->body = atoi(pkvd->szValue);
		return true;
	}
	else
	{
		return CBaseAnimating::KeyValue(pkvd);
	}
}

void CItemGeneric::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	SetThink(&CBaseEntity::SUB_Remove);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CItemGeneric::Think1()
{
	if (pev->spawnflags & 1)
	{
		if (DROP_TO_FLOOR(ENT(pev)) == 0)
		{
			ALERT(at_error, "Item %s fell out of level at %f,%f,%f", STRING(pev->classname), pev->origin.x, pev->origin.y, pev->origin.z);
			UTIL_Remove(this);
			return;
		}
	}

	if (m_iszSequenceName)
	{
		pev->effects = 0;
		pev->sequence = LookupSequence(STRING(m_iszSequenceName));

		if (pev->sequence = -1)
			pev->sequence = 0;

		pev->frame = 0.0;
		ResetSequenceInfo();
		SetThink(&CItemGeneric::Think2);
		pev->nextthink = gpGlobals->time + 0.1;
		// gearbox moment: redundant frame reset
		pev->frame = 0.0;
	}
}

void CItemGeneric::Think2()
{
	DispatchAnimEvents();
	StudioFrameAdvance();

	if (m_fSequenceFinished && !m_fSequenceLoops)
	{
		pev->frame = 0.0;
		ResetSequenceInfo();
	}

	pev->nextthink = gpGlobals->time + 0.1;
	field_0x4c = gpGlobals->time;
}

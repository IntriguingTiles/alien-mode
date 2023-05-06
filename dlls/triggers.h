#pragma once

#define SF_MULTIMAN_CLONE 0x80000000
#define SF_MULTIMAN_THREAD 0x00000001

class CMultiManager : public CBaseToggle
{
public:
	bool KeyValue( KeyValueData *pkvd ) override;
	void Spawn() override;
	virtual void ManagerThink();
	void EXPORT ManagerUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT CallManagerThink()
	{
		ManagerThink();
	}

#if _DEBUG
	void EXPORT ManagerReport();
#endif

	bool HasTarget( string_t targetname ) override;

	int ObjectCaps() override
	{
		return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION;
	}

	bool Save( CSave &save ) override;
	bool Restore( CRestore &restore ) override;

	static TYPEDESCRIPTION m_SaveData[];

	int m_cTargets;							  // the total number of targets in this manager's fire list.
	int m_index;							  // Current target
	float m_startTime;						  // Time we started firing
	int m_iTargetName[MAX_MULTI_TARGETS];	  // list if indexes into global string array
	float m_flTargetDelay[MAX_MULTI_TARGETS]; // delay (in seconds) from time of manager fire to target fire

	inline bool IsClone() { return (pev->spawnflags & SF_MULTIMAN_CLONE) != 0; }
	inline bool ShouldClone()
	{
		if (IsClone())
			return false;

		return (pev->spawnflags & SF_MULTIMAN_THREAD) != 0;
	}

	virtual CMultiManager *Clone();
};
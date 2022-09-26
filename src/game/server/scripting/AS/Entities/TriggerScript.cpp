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

#include "cbase.h"

#include "scripting/AS/as_utils.h"
#include "scripting/AS/ASGenericCall.h"
#include "scripting/AS/ASScriptingSystem.h"
#include "scripting/AS/ScriptPluginList.h"

namespace scripting
{
/**
*	@brief Entity to call a script function on trigger.
*	Has save support, but likely won't work properly due to scripts not being saved.
*/
class TriggerScript final : public CBaseEntity
{
	DECLARE_CLASS(TriggerScript, CBaseEntity);
	DECLARE_DATAMAP();

public:
	int ObjectCaps() override { return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }

	bool KeyValue(KeyValueData* pkvd) override;

	void Precache() override;

	void Spawn() override;

	void Use(CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value) override;

private:
	string_t m_TriggerFunctionName;

	// Don't save, reacquire.
	as::UniquePtr<asIScriptFunction> m_TriggerFunction;
};

BEGIN_DATAMAP(TriggerScript)
DEFINE_FIELD(m_TriggerFunction, FIELD_STRING),
	END_DATAMAP();

bool TriggerScript::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "trigger_function"))
	{
		m_TriggerFunctionName = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

void TriggerScript::Precache()
{
	// Done in precache because the function needs to be reacquired on load.

	if (FStringNull(pev->targetname))
	{
		g_Scripting.GetLogger()->warn("{} with no targetname at {}, {}, {}",
			STRING(pev->classname), pev->origin.x, pev->origin.y, pev->origin.z);
	}

	const bool success = [this]()
	{
		auto logger = g_Scripting.GetLogger();

		// TODO: maybe return a span of the modules vector instead.
		auto plugin = g_Scripting.GetMapPlugin();

		if (!plugin)
		{
			logger->error("{}({}): No map scripts loaded", STRING(pev->classname), STRING(pev->targetname));
			return false;
		}

		if (FStringNull(m_TriggerFunctionName))
		{
			logger->error("{}({}): No trigger function specified", STRING(pev->classname), STRING(pev->targetname));
			return false;
		}

		// TODO: if Use functions change to use a struct change this as well.
		const auto completeFunctionName{fmt::format("void {}(CBaseEntity@ activator, CBaseEntity@ caller, USE_TYPE useType, float value)",
			STRING(m_TriggerFunctionName))};

		// Find the function. There may be multiple occurrences in different modules so keep looking to warn about that.
		bool firstWarning = true;

		for (auto& module : plugin->Modules)
		{
			auto candidate = module->GetFunctionByDecl(completeFunctionName.c_str());

			if (candidate)
			{
				if (!m_TriggerFunction)
				{
					m_TriggerFunction = as::MakeUnique(candidate);
					logger->trace("{}({}): Found function \"{}\" in module \"{}\"",
						STRING(pev->classname), STRING(pev->targetname), completeFunctionName, *module);
				}
				else
				{
					if (firstWarning)
					{
						firstWarning = false;
						logger->warn("{}({}): Encountered multiple functions with signature \"{}\"",
							STRING(pev->classname), STRING(pev->targetname), completeFunctionName);
					}

					logger->warn("{}({}): Next function encountered in module \"{}\"",
						STRING(pev->classname), STRING(pev->targetname), *module);
				}
			}
		}

		if (!m_TriggerFunction)
		{
			logger->error("{}({}): No trigger function with signature \"{}\"",
				STRING(pev->classname), STRING(pev->targetname), completeFunctionName);
			return false;
		}

		return true;
	}();

	if (!success)
	{
		UTIL_Remove(this);
	}
}

void TriggerScript::Spawn()
{
	Precache();
}

void TriggerScript::Use(CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value)
{
	// Guard against triggers occurring between UTIL_Remove calls and removal.
	if (!m_TriggerFunction)
	{
		return;
	}

	const auto context = scripting::g_Scripting.GetContext();

	call::ExecuteFunction<call::ReturnVoid>(*context, m_TriggerFunction.get(), activator, caller, useType, value);
}
}

LINK_ENTITY_TO_CLASS(trigger_script, scripting::TriggerScript);

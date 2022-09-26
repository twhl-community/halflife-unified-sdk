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

#include <cassert>

#include "cbase.h"

#include "CustomEntityDictionary.h"
#include "scripting/AS/ASManager.h"
#include "scripting/AS/ASScriptingSystem.h"

namespace scripting
{
/**
 *	@brief Special entity that handles construction of Angelscript-based custom entities.
 *	@details The first keyvalue passed to this entity should be @c "customclass" @c "<classname>".
 */
class CustomEntityFactory final : public CBaseEntity
{
public:
	bool KeyValue(KeyValueData* pkvd) override
	{
		assert(FStrEq("custom", pkvd->szClassName));
		assert(FStrEq("customclass", pkvd->szKeyName));

		// TODO: should really run this logic before calling EntvarsKeyValue.

		if ((pev->flags & FL_KILLME) != 0)
		{
			// Already marked for removal so this is probably a bugged entity.
			// Don't try to create it since it may be in an invalid state.
			return false;
		}

		if (FStrEq("customclass", pkvd->szKeyName))
		{
			g_Scripting.GetLogger()->trace("Creating custom entity \"{}\"", pkvd->szValue);

			// Do not call any entity methods or access pev after this call.
			// The new object is this entity on success so it's no longer valid.
			auto scriptObject = g_Scripting.GetCustomEntityDictionary()->TryCreateCustomEntity(pkvd->szValue, pev);

			if (scriptObject)
			{
				g_Scripting.GetLogger()->debug("Created custom entity \"{}\" with entity index {}", pkvd->szValue, scriptObject->entindex());

				// Object fully created, now managed by the engine.
				scriptObject.release();

				// Delete the factory object.
				// TODO: need a way to ensure this doesn't mess up edict members.
				OnDestroy();
				delete this;
			}
			else
			{
				m_FailedToCreate = true;
				g_Scripting.GetLogger()->error("Failed to create custom entity \"{}\"", pkvd->szValue);
				UTIL_Remove(this);
			}

			return true;
		}

		g_Scripting.GetLogger()->error("Custom entity factory received incorrect keyvalue \"{}\" \"{}\"", pkvd->szKeyName, pkvd->szValue);
		UTIL_Remove(this);

		return false;
	}

	void Precache() override
	{
		// If no custom entity by the given name exists then we will still exist so just wait until we're removed.
		if (m_FailedToCreate)
		{
			return;
		}

		// If we get here then something went wrong.
		// Possibly somebody creating an entity using "custom" as the classname.

		g_Scripting.GetLogger()->error("Custom entity factory used incorrectly: classname \"{}\", targetname \"{}\"",
			STRING(pev->classname), STRING(pev->targetname));

		UTIL_Remove(this);
	}

	void Spawn() override
	{
		Precache();
	}

private:
	bool m_FailedToCreate = false;
};
}

LINK_ENTITY_TO_CLASS(custom, scripting::CustomEntityFactory);

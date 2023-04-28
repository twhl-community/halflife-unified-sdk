/***
 *
 *	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
 *
 *	This product contains software technology licensed from Id
 *	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
 *	All Rights Reserved.
 *
 *   This source code contains proprietary and confidential information of
 *   Valve LLC and its suppliers.  Access to this code is restricted to
 *   persons who have executed a written SDK license with Valve.  Any access,
 *   use or distribution of this code by or to any unlicensed person is illegal.
 *
 ****/

#pragma once

#define TALKRANGE_MIN 500.0 // don't talk to anyone farther away than this

#define TLK_STARE_DIST 128 // anyone closer than this and looking at me is probably staring at me.

#define bit_saidDamageLight (1 << 0) // bits so we don't repeat key sentences
#define bit_saidDamageMedium (1 << 1)
#define bit_saidDamageHeavy (1 << 2)
#define bit_saidHelloPlayer (1 << 3)
#define bit_saidWoundLight (1 << 4)
#define bit_saidWoundHeavy (1 << 5)
#define bit_saidHeard (1 << 6)
#define bit_saidSmelled (1 << 7)

enum TALKGROUPNAMES
{
	TLK_ANSWER = 0,
	TLK_QUESTION,
	TLK_IDLE,
	TLK_STARE,
	TLK_USE,
	TLK_UNUSE,
	TLK_STOP,
	TLK_NOSHOOT,
	TLK_HELLO,
	TLK_PHELLO,
	TLK_PIDLE,
	TLK_PQUESTION,
	TLK_PLHURT1,
	TLK_PLHURT2,
	TLK_PLHURT3,
	TLK_SMELL,
	TLK_WOUND,
	TLK_MORTAL,

	TLK_CGROUPS, // MUST be last entry
};

enum
{
	SCHED_CANT_FOLLOW = LAST_COMMON_SCHEDULE + 1,
	SCHED_MOVE_AWAY,		// Try to get out of the player's way
	SCHED_MOVE_AWAY_FOLLOW, // same, but follow afterward
	SCHED_MOVE_AWAY_FAIL,	// Turn back toward player

	LAST_TALKMONSTER_SCHEDULE, // MUST be last
};

enum
{
	TASK_CANT_FOLLOW = LAST_COMMON_TASK + 1,
	TASK_MOVE_AWAY_PATH,
	TASK_WALK_PATH_FOR_UNITS,

	TASK_TLK_RESPOND,		 // say my response
	TASK_TLK_SPEAK,			 // question or remark
	TASK_TLK_HELLO,			 // Try to say hello to player
	TASK_TLK_HEADRESET,		 // reset head position
	TASK_TLK_STOPSHOOTING,	 // tell player to stop shooting friend
	TASK_TLK_STARE,			 // let the player know I know he's staring at me.
	TASK_TLK_LOOK_AT_CLIENT, // faces player if not moving and not talking and in idle.
	TASK_TLK_CLIENT_STARE,	 // same as look at client, but says something if the player stares.
	TASK_TLK_EYECONTACT,	 // maintain eyecontact with person who I'm talking to
	TASK_TLK_IDEALYAW,		 // set ideal yaw to face who I'm talking to
	TASK_FACE_PLAYER,		 // Face the player

	LAST_TALKMONSTER_TASK, // MUST be last
};

/**
 *	@brief Talking monster base class
 *	Used for scientists and barneys
 */
class CTalkMonster : public CBaseMonster
{
	DECLARE_CLASS(CTalkMonster, CBaseMonster);
	DECLARE_DATAMAP();
	DECLARE_CUSTOM_SCHEDULES();

public:
	/**
	 *	@brief monsters derived from ctalkmonster should call this in precache()
	 */
	virtual void TalkInit();

	/**
	 *	@brief Scan for nearest, visible friend. If fPlayer is true, look for nearest player
	 */
	CBaseEntity* FindNearestFriend(bool fPlayer);
	float TargetDistance();
	void StopTalking() { SentenceStop(); }

	// Base Monster functions
	void OnCreate() override;

	void Precache() override;
	bool TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) override;
	void Touch(CBaseEntity* pOther) override;
	void Killed(CBaseEntity* attacker, int iGib) override;
	Relationship IRelationship(CBaseEntity* pTarget) override;
	bool CanPlaySentence(bool fDisregardState) override;

protected:
	void PlaySentenceCore(const char* pszSentence, float duration, float volume, float attenuation) override;

public:
	void PlayScriptedSentence(const char* pszSentence, float duration, float volume, float attenuation, bool bConcurrent, CBaseEntity* pListener) override;
	bool KeyValue(KeyValueData* pkvd) override;

	CTalkMonster* MyTalkMonsterPointer() override { return this; }

	// AI functions
	const Schedule_t* GetScheduleOfType(int Type) override;
	void StartTask(const Task_t* pTask) override;
	void RunTask(const Task_t* pTask) override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	void PrescheduleThink() override;


	// Conversations / communication
	int GetVoicePitch();

	/**
	 *	@brief Respond to a previous question
	 */
	void IdleRespond();

	/**
	 *	@brief ask question of nearby friend, or make statement
	 */
	bool FIdleSpeak();

	bool FIdleStare();

	/**
	 *	@brief Try to greet player first time he's seen
	 */
	bool FIdleHello();

	/**
	 *	@brief turn head towards supplied origin
	 */
	void IdleHeadTurn(Vector& vecFriend);
	bool FOkToSpeak();

	/**
	 *	@brief try to smell something
	 */
	void TrySmellTalk();
	void AlertFriends();
	void ShutUpFriends();

	/**
	 *	@brief am I saying a sentence right now?
	 */
	bool IsTalking();

	/**
	 *	@brief set a timer that tells us when the monster is done talking.
	 */
	void Talk(float flDuration);

	/**
	 *	@brief Invokes @c callback on each friend
	 *	@details Return false to stop iteration
	 */
	template <typename Callback>
	void ForEachFriend(Callback callback);

	template <typename Callback>
	void EnumFriends(Callback callback, bool trace);

	// For following
	bool CanFollow();
	bool IsFollowing() { return m_hTargetEnt != nullptr && m_hTargetEnt->IsPlayer(); }
	void StopFollowing(bool clearSchedule) override;
	void StartFollowing(CBaseEntity* pLeader);
	virtual void DeclineFollowing() {}
	void LimitFollowers(CBaseEntity* pPlayer, int maxFollowers);

	void FollowerUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	/**
	 *	@brief Prepare this talking monster to answer question
	 */
	virtual void SetAnswerQuestion(CTalkMonster* pSpeaker);

	static float g_talkWaitTime;

	int m_bitsSaid;					  //!< set bits for sentences we don't want repeated
	int m_nSpeak;					  //!< number of times initiated talking
	int m_voicePitch;				  //!< pitch of voice for this head
	const char* m_szGrp[TLK_CGROUPS]; //!< sentence group names
	float m_useTime;				  //!< Don't allow +USE until this time
	string_t m_iszUse;				  //!< Custom +USE sentence group (follow)
	string_t m_iszUnUse;			  //!< Custom +USE sentence group (stop following)

	float m_flLastSaidSmelled; //!< last time we talked about something that stinks
	float m_flStopTalkTime;	   //!< when in the future that I'll be done saying this sentence.

	bool m_fStartSuspicious;

	EHANDLE m_hTalkTarget; //!< who to look at while talking
};

template <typename Callback>
void CTalkMonster::ForEachFriend(Callback callback)
{
	// First pass: check for other NPCs of the same classification.
	// This typically includes NPCs with the same entity class, but a modder may implement custom classifications.
	const auto myClass = Classify();

	for (auto friendEntity : UTIL_FindEntities())
	{
		if (friendEntity->IsPlayer())
		{
			// No players
			continue;
		}

		auto talkMonster = friendEntity->MyTalkMonsterPointer();

		if (!talkMonster)
		{
			// Not a talk monster, can't be a friend.
			continue;
		}

		if (myClass != talkMonster->Classify())
		{
			continue;
		}

		callback(talkMonster);
	}

	// Second pass: check for other NPCs that are friendly to us
	for (auto friendEntity : UTIL_FindEntities())
	{
		if (friendEntity->IsPlayer())
		{
			// No players
			continue;
		}

		auto talkMonster = friendEntity->MyTalkMonsterPointer();

		if (!talkMonster)
		{
			// Not a talk monster, can't be a friend.
			continue;
		}

		if (myClass == talkMonster->Classify())
		{
			// Already checked these. Includes other NPCs of my type
			continue;
		}

		const auto relationship = IRelationship(talkMonster);

		if (relationship != Relationship::Ally && relationship != Relationship::None)
		{
			// Not a friend
			continue;
		}

		callback(talkMonster);
	}
}

template <typename Callback>
void CTalkMonster::EnumFriends(Callback callback, bool trace)
{
	auto wrapper = [&](CTalkMonster* pFriend)
	{
		if (pFriend == this || !pFriend->IsAlive() || !(pFriend->pev->flags & FL_MONSTER))
		{
			// don't talk to self or dead people or non-monster entities
			return;
		}

		if (trace)
		{
			Vector vecCheck = pFriend->pev->origin;
			vecCheck.z = pFriend->pev->absmax.z;

			TraceResult tr;
			UTIL_TraceLine(pev->origin, vecCheck, ignore_monsters, edict(), &tr);

			if (tr.flFraction != 1.0)
			{
				return;
			}
		}

		callback(pFriend);
	};

	ForEachFriend(wrapper);
}

// Clients can push talkmonsters out of their way
#define bits_COND_CLIENT_PUSH (bits_COND_SPECIAL1)
// Don't see a client right now.
#define bits_COND_CLIENT_UNSEEN (bits_COND_SPECIAL2)

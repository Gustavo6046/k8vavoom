#include "i_defs.h"

#include "p_action.h"
#include "vc.h"

#define FUNC(name) \
	void name(mobj_t * object) \
	{ \
		fprintf(cur_file, " { %s(); ", #name); \
		if (((state_t *)object)->action_par) \
			fprintf(cur_file, "/* PAR */ "); \
	}
#define FUNC_AS(name, realname) \
	void name(mobj_t * object) \
	{ \
		fprintf(cur_file, " { %s(); ", #realname); \
		if (((state_t *)object)->action_par) \
			fprintf(cur_file, "/* PAR */ "); \
	}
#define START_FUNC(name) \
	void name(mobj_t * object) \
	{ \
		fprintf(cur_file, " { %s(", #name);
#define START_FUNC_AS(name, realname) \
	void name(mobj_t * object) \
	{ \
		fprintf(cur_file, " { %s(", #realname);
#define END_FUNC \
		fprintf(cur_file, "); "); \
	}
//				if (s->action_par)
//					fprintf(f, "/* PAR */ ");

#define ATTACK_FUNC(name) \
	void name(mobj_t *object) \
	{ \
		if (((state_t *)object)->action_par) \
		{ \
			fprintf(cur_file, "\n"); \
			fprintf(cur_file, "\t{\n"); \
			fprintf(cur_file, "\t\tAttackHolder tmp;\n"); \
			fprintf(cur_file, "\n"); \
			fprintf(cur_file, "\t\ttmp = SpawnObject(AttackHolder, none);\n"); \
			fprintf(cur_file, "\t"); \
			PrintAttack(cur_file, (attacktype_t *)((state_t *)object)->action_par, "\ttmp.Attack");	\
			fprintf(cur_file, "\t\t"#name"(&tmp.Attack);\n"); \
			fprintf(cur_file, "\t\ttmp.Destroy();\n"); \
			fprintf(cur_file, "\t"); \
		} \
		else \
		{ \
			fprintf(cur_file, " { "#name"(NULL); "); \
		} \
	}

// Weapon Action Routine pointers
FUNC(A_Light0)
FUNC(A_Light1)
FUNC(A_Light2)

FUNC(A_WeaponReady)
ATTACK_FUNC(A_WeaponShoot)
ATTACK_FUNC(A_WeaponEject)
START_FUNC(A_WeaponJump)
{
	act_jump_info_t *jump;
  
	jump = (act_jump_info_t *)((state_t *)object)->action_par;

	if (((state_t *)object)->jumpstate == S_NULL)
		fprintf(cur_file, "%1.2f, \'S_NULL\'", jump->chance);
	else
		fprintf(cur_file, "%1.2f, \'S_%d\'", jump->chance,
			((state_t *)object)->jumpstate);
}
END_FUNC
FUNC(A_Lower)
FUNC(A_Raise)
FUNC(A_ReFire)
FUNC(A_NoFire)
FUNC(A_NoFireReturn)
FUNC(A_CheckReload)
FUNC(A_SFXWeapon1)
FUNC(A_SFXWeapon2)
FUNC(A_SFXWeapon3)
START_FUNC(A_WeaponPlaySound)
	fprintf(cur_file, "\'%s\'", SFX((sfx_t *)((state_t *)object)->action_par));
END_FUNC
FUNC(A_WeaponKillSound)
FUNC(A_WeaponTransSet)
FUNC(A_WeaponTransFade)
FUNC(A_WeaponEnableRadTrig)
FUNC(A_WeaponDisableRadTrig)

FUNC(A_SetCrosshair)
FUNC(A_GotTarget)
FUNC(A_GunFlash)
FUNC(A_WeaponKick)

ATTACK_FUNC(A_WeaponShootSA)
FUNC(A_ReFireSA)
FUNC(A_NoFireSA)
FUNC(A_NoFireReturnSA)
FUNC(A_CheckReloadSA)
FUNC(A_GunFlashSA)

// Needed for the bossbrain.
FUNC(A_BrainScream)
FUNC(A_BrainDie)
FUNC(A_BrainSpit)
FUNC(A_CubeSpawn)
FUNC(A_BrainMissileExplode)

// Visibility Actions
START_FUNC(P_ActTransSet)
	if (((state_t *)object)->action_par)
	{
		int val = (int)((1 - *((percent_t *)((state_t *)object)->action_par)) * 100);
		if (val < 0)
			val = 0;
		else if (val > 100)
			val = 100;
		fprintf(cur_file, "%d", val);
	}
	else
	{
		fprintf(cur_file, "0");
	}
END_FUNC
FUNC(P_ActTransFade)
FUNC(P_ActTransMore)
FUNC(P_ActTransLess)
FUNC(P_ActTransAlternate)

// Sound Actions
START_FUNC(P_ActPlaySound)
	fprintf(cur_file, "\'%s\'", SFX((sfx_t *)((state_t *)object)->action_par));
END_FUNC
FUNC(P_ActKillSound)
FUNC(P_ActMakeAmbientSound)
FUNC(P_ActMakeAmbientSoundRandom)
FUNC(P_ActMakeCloseAttemptSound)
FUNC(P_ActMakeDyingSound)
FUNC(P_ActMakeOverKillSound)
FUNC(P_ActMakePainSound)
FUNC(P_ActMakeRangeAttemptSound)
FUNC(P_ActMakeActiveSound)

// Explosion Damage Actions
FUNC(P_ActDamageExplosion)
FUNC(P_ActThrust)

// Stand-by / Looking Actions
FUNC_AS(P_ActStandardLook, A_Look)
FUNC(P_ActPlayerSupportLook)

// Meander, aimless movement actions.
FUNC(P_ActStandardMeander)
FUNC(P_ActPlayerSupportMeander)

// Chasing Actions
FUNC(P_ActResurrectChase)
FUNC_AS(P_ActStandardChase, A_Chase)
FUNC(P_ActWalkSoundChase)

// Attacking Actions
FUNC(P_ActComboAttack)
ATTACK_FUNC(P_ActMeleeAttack)
ATTACK_FUNC(P_ActRangeAttack)
ATTACK_FUNC(P_ActSpareAttack)
FUNC(P_ActRefireCheck)

// Miscellanous
FUNC_AS(P_ActFaceTarget, A_FaceTarget)
FUNC_AS(P_ActMakeIntoCorpse, A_Fall)
FUNC(P_ActResetSpreadCount)
FUNC(P_ActExplode)
FUNC(P_ActActivateLineType)
FUNC(P_ActEnableRadTrig)
FUNC(P_ActDisableRadTrig)
FUNC(P_ActTouchyRearm)
FUNC(P_ActTouchyDisarm)
FUNC(P_ActBounceRearm)
FUNC(P_ActBounceDisarm)
FUNC(P_ActPathCheck)
FUNC(P_ActPathFollow)
FUNC(P_ActDropItem)
FUNC(P_ActDLightSet)
FUNC(P_ActDLightFade)
FUNC(P_ActDLightRandom)

// Movement actions
FUNC(P_ActFaceDir)
FUNC(P_ActTurnDir)
FUNC(P_ActTurnRandom)
FUNC(P_ActMlookFace)
FUNC(P_ActMlookTurn)
FUNC(P_ActMoveFwd)
FUNC(P_ActMoveRight)
FUNC(P_ActMoveUp)
FUNC(P_ActStopMoving)

// Projectiles
FUNC(P_ActFixedHomingProjectile)
FUNC(P_ActRandomHomingProjectile)
FUNC(P_ActLaunchOrderedSpread)
FUNC(P_ActLaunchRandomSpread)
FUNC(P_ActCreateSmokeTrail)
FUNC(P_ActHomeToSpot)
//boolean_t P_ActLookForTargets)

// Trackers
FUNC(P_ActEffectTracker)
FUNC(P_ActTrackerActive)
FUNC(P_ActTrackerFollow)
FUNC(P_ActTrackerStart)

// Blood and bullet puffs
FUNC(P_ActCheckBlood)
FUNC(P_ActCheckMoving)

START_FUNC(P_ActJump)
{
	act_jump_info_t *jump;
  
	jump = (act_jump_info_t *)((state_t *)object)->action_par;

	if (((state_t *)object)->jumpstate == S_NULL)
		fprintf(cur_file, "%1.2f, \'S_NULL\'", jump->chance);
	else
		fprintf(cur_file, "%1.2f, \'S_%d\'", jump->chance,
			((state_t *)object)->jumpstate);
}
END_FUNC
FUNC(A_PlayerScream)

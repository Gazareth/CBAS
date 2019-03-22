#include "GameObjects.h"
#include "CBAS_ConfigHandler.h"
#include "CharacterBasedAttackSpeed.h"

#define DEFAULT_ATTACK_SPEED 1.0f		//in case of emergency, default aspd

typedef std::map<UInt32,struct AttackingActorVals*> AAMap;

/*************************
	Attacking Actors and the AttackingActorMap
*************************/

static CBAS_Config::IniHandler* CBAS_IniHandler = NULL;

struct AttackingActorVals {

	//"tracked" character values. when changed- will trigger an attack speed change
	const SInt32 SPD;
	const UInt32 _FTG;		//base fatigue -- this changes when STR,END,AGI,WILL change, so we can use this as a "tracker" for those four stats

	float wWgt;
	float aSpd;

	UInt32 actorRef;
	UInt32 wepRef;
	UInt32 wepType;

	DWORD t;	//time since attack speed was last checked/updated

	//construct actor with all basic vals needed
	AttackingActorVals(Actor* a);

	// OutOfDate():-- convenient function for determining when attack speed needs updating
	//		we only want to update attack speed when either:
	//		1 - weapon has changed
	//		2 - more than one second has passed
	//		3 - base fatigue has shifted by at least 10
	bool OutOfDate(AttackingActorVals* compareWith);

	void CalcAttackSpeed(Actor* a);

};




class AttackingActors {
	AAMap AttackingActorMap;
	DWORD t_c;	//time since last cleanse

	void CleanseOldActors();

public:
	AttackingActors();

	bool GetIniValues();

	float GetDesiredAttackSpeed(Actor* a);
};

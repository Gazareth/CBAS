#include "GameObjects.h"
#include "CBAS_Config.h"
#include "CBAS_Lib.h"
#include "CBAS_ActorAnimData.h"




/*************************
	Attacking Actors and the ActorValuesMap
*************************/

static CBAS_Config::IniHandler* CBAS_IniHandler = NULL;

//Once an actor has been acquired, this struct is used to track them
struct CBAS_ActorVals {

	//"tracked" character values. when changed- will trigger an attack speed change
	const SInt32 SPD;
	const SInt32 AGI;
	const UInt32 _FTG;		//base fatigue -- this changes when STR,END,AGI,WILL change, so we can use this as a "tracker" for those four stats

	CBAS_Weapon* wep;

	float aSpd;

	UInt32 actorRef;

	DWORD t;	//time attack speed was last checked/updated

	//construct actor with all basic vals needed
	CBAS_ActorVals(Actor* a);

	// OutOfDate():-- convenient function for determining when attack speed needs updating
	//		we only want to update attack speed when either:
	//		1 - weapon has changed
	//		2 - more than one second has passed
	//		3 - base fatigue has shifted by at least 10
	bool OutOfDate(CBAS_ActorVals* compareWith);

	void UpdateAttackSpeed(Actor* a);

};


typedef std::map<uintptr_t,DWORD> CBAS_AADMap;
typedef std::map<UInt32,struct CBAS_ActorVals*> CBAS_AVMap;

class CBAS_Actors {
	CBAS_AADMap AnimDataMap;	//only stores last time each ActorAnimData was processed
	CBAS_AVMap ActorValuesMap;		//stores all cbas actor values so they can potentially be reused

	DWORD t_c;		//time since maps were last cleansed

	void CleanseAnimDataMap();
	void CleanseActorMap();

	float AttackSpeedFromActor(Actor* a);
	Actor* ResolveActorFromAnimData(void *p);
public:
	CBAS_Actors();

	void LoadIniValues();

	void SetAttackSpeed(void* p);
};

#include "AttackingActor.h"

//constants
#define MIN_WEAP_WEIGHT 1.f

#define UPDATE_INTERVAL 2000				//attack speed updated every 1s or if wep changed
#define CLEANSE_CHECK_INTERVAL 60000	//check to cleanse actors every 1 min
#define CLEANSE_INTERVAL 30000			//cleanse actors that haven't been updated in last 30 seconds

//functions
#define TIME_PASSED(t,i) ((t + i) < GetTickCount())
#define GETINI( x ) (*CBAS_IniHandler)(CBAS_Config::IniEntries::x)

#define DIFF(d) SInt32(d - compareWith->d)
#define A_DIFF(x) (abs(DIFF(x))
#define AA_DIFF(y,z) (A_DIFF(y) > z))

/*************************
	Attacking Actor Values
*************************/

AttackingActorVals::AttackingActorVals(Actor* a) :
		SPD(a->GetActorValue(kActorVal_Speed)),
		_FTG(a->GetBaseActorValue(kActorVal_Fatigue))
	{
		t = GetTickCount();

		//Weapon info
		actorRef = a->refID;
		TESObjectWEAP* wep = GetEquippedWeapon(a);
		if(wep){
			wepRef = wep->refID;
			wWgt = wep->weight.weight;
			wepType = wep->type;
		} else {
			wepRef = 0;
			wWgt = 0.f;
			wepType = TESObjectWEAP::kType_Max;		//Max: HandToHand
#if _DEBUG
		_MESSAGE("Couldnt find equipped weapon for actor %s(%d)",GetFullName(a),a->refID);
#endif
		}
		wWgt = max(MIN_WEAP_WEIGHT,wWgt);	//protect against very low weapon weights
	}



bool AttackingActorVals::OutOfDate(AttackingActorVals* compareWith) {
	return (AA_DIFF(wepRef,0) ||  ((t + UPDATE_INTERVAL) < compareWith->t) || AA_DIFF(_FTG,10));
}



void AttackingActorVals::CalcAttackSpeed(Actor* a) {
	//attack speed being updated, so update time
	t = GetTickCount();
	aSpd = DEFAULT_ATTACK_SPEED;

	const SInt32 STR = a->GetActorValue(kActorVal_Strength);

	//Attack [F]actors
	//
	float ftgF = CBAS::FatigueFactor(a,_FTG,GETINI(CBAS_FatigueThreshold));					//fatigue
	float encF = CBAS::EncumbranceFactor(a,STR,GETINI(CBAS_LocalisedEncumbrance));			//encumbrance
	float skillF = CBAS::SkillFactor(a,wepType);											//skill in equipped weapon (e.g. blade, blunt)

	float attrF = CBAS::AttrFactor(a,STR,wepType,wWgt);										//main attribute factor, depends on weapon type, e.g. melee weapons use str, bows use agi
	float spdF = SPD*.01f;																	//speed
		
	if( (*CBAS_IniHandler)() ){		//this gets whether or not we should use all components
		aSpd = ftgF*encF*skillF*attrF*spdF;
	} else {
		aSpd = 
			PRSNK_UNDERWEIGH(ftgF,GETINI(CBAS_FatigueComponent)) *
			PRSNK_UNDERWEIGH(encF,GETINI(CBAS_FatigueComponent)) *
			PRSNK_UNDERWEIGH(skillF,GETINI(CBAS_SkillComponent)) *
			PRSNK_UNDERWEIGH(attrF,GETINI(CBAS_AttributeComponent)) *
			PRSNK_UNDERWEIGH(spdF,GETINI(CBAS_SpeedComponent));
	}

#if _DEBUG
	_MESSAGE("	Calculating attack speed...");
	_MESSAGE("		Fatigue factor: %f",ftgF);
	_MESSAGE("		Encumbrance factor: %f",encF);
	_MESSAGE("		Skill factor: %f",skillF);
	_MESSAGE("		Attribute factor: %f",attrF);
	_MESSAGE("		Speed factor: %f",spdF);
	_MESSAGE("			MaxSpeed: %f",PRSNK_MAX_WEAP_SPEED(wWgt));
	_MESSAGE("	Full modification factor (PRE PRSNK %f)",aSpd);
#endif
	aSpd = PRSNK_FACTOR(aSpd);
	aSpd *= PRSNK_MAX_WEAP_SPEED(wWgt);
#if _DEBUG
	_MESSAGE("	Returning attack speed of %f.",aSpd);
#endif
}



/*************************
	Attacking Actors Tracker
*************************/

void AttackingActors::CleanseOldActors() {
	t_c = GetTickCount();
#if _DEBUG
	_MESSAGE("	Cleansing old actors!");
#endif
	AAMap::iterator aavIter;
	for( aavIter = AttackingActorMap.begin(); aavIter != AttackingActorMap.end(); aavIter++){
#if _DEBUG
		_MESSAGE("		Checking actor %s (%x)...",GetFullName(LookupFormByID(aavIter->first)),aavIter->first);
#endif
		if( TIME_PASSED(aavIter->second->t,CLEANSE_INTERVAL) ) {
#if _DEBUG
		_MESSAGE("			Cleansing actor!");
#endif
			AttackingActorMap.erase(aavIter->first);	//erasing an actor from the map, invalidating the iterator
			CleanseOldActors();	//start a new loop in case there are any more actors to erase
			break;
		}
	}
}

AttackingActors::AttackingActors() : 
		t_c(GetTickCount())		//construct with current time as last cleansed time
		{}		



bool AttackingActors::GetIniValues() {
	CBAS_IniHandler = new CBAS_Config::IniHandler();
	return (*CBAS_IniHandler)();
}

float AttackingActors::GetDesiredAttackSpeed(Actor* a){
	float AtkSpd = 1.f;
	AttackingActorVals* AAV;
	AttackingActorVals *nAAV = new AttackingActorVals(a);		//new actor vals to compare against

	bool update = false;
	bool actorExists = AttackingActorMap.count(a->refID) > 0;
	if( actorExists ){
		AAV = AttackingActorMap[a->refID];
		AAV = AttackingActorMap.at(a->refID);	//get actor from map
		update = AAV->OutOfDate(nAAV);	//is this actor out of date?
	}

	if( !actorExists || update ){
		//use the new actor as the current one in the map
		AAV = nAAV;

#if _DEBUG
	DWORD ticks = (AAV->t - t_c) / 1000;
	const DWORD seconds = ticks % 60;
	ticks /= 60;
	const DWORD minutes = ticks % 60;
	ticks /= 60;
	const DWORD hours = ticks; // may exceed 24 hours.
	const char* addOr = actorExists ? "Updating" : "Adding";
	_MESSAGE("[Time: %d:%02d:%02d (%d)]: %s actor %s (%X)!",hours, minutes, seconds, AAV->t,addOr,GetFullName(a),a->refID);
#endif

		//calculate attack speed for new actor
		AAV->CalcAttackSpeed(a);
		//put new actor in place
		AttackingActorMap[AAV->actorRef] = AAV;
	}

	//clean old actors every so often
	if( update && TIME_PASSED(t_c,CLEANSE_CHECK_INTERVAL) ) CleanseOldActors();

#if _DEBUG
	//TESObjectWEAP* eqObj = (TESObjectWEAP*)LookupFormByID(AAV->wepRef);
	//_MESSAGE("[Time: %d]: Returning attack speed for actor %s using weapon %s with weight %f to %f!",AAV->t,GetFullName(a),GetFullName(eqObj),AAV->wWgt,AAV->aSpd);
	//_MESSAGE("(Actor existed in map? %d. Actor needed updating? %d.)",actorExists, update);
#endif
	return AAV->aSpd;
}

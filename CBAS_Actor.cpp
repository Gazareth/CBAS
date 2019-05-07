#include "CBAS_Actor.h"

//constants
#define DEFAULT_ATTACK_SPEED 1.0f		//in case of emergency, default aspd

#define TOUCH_INTERVAL 200				//CBAS code is run every frame during weapon action but to improve performance we only check each animdata pointer once every 0.2s

#define UPDATE_INTERVAL 2000			//attack speed updated every 2000 or if wep changed
#define CLEANSE_CHECK_INTERVAL 60000	//check if any actors need to be removed from the map every 1 min
#define CLEANSE_INTERVAL 30000			//actors are removed from the map if they haven't been updated in last 30 seconds

//functions
#define TIME_PASSED(t,i) ((t + i) < GetTickCount())
#define GETINI( x ) (*CBAS_IniHandler)(CBAS_Config::IniEntries::x)

#define DIFF(d) SInt32(d - compareWith->d)
#define A_DIFF(x) (abs(DIFF(x))
#define AA_DIFF(y,z) (A_DIFF(y) > z))

/*************************
	Actor Values
*************************/

CBAS_ActorVals::CBAS_ActorVals(Actor* a) :
		SPD(a->GetActorValue(kActorVal_Speed)),
		_FTG(a->GetBaseActorValue(kActorVal_Fatigue))
	{
		t = GetTickCount();
		//Weapon info
		actorRef = a->refID;
		wep = GetEquippedWeapon(a);
	}



bool CBAS_ActorVals::OutOfDate(CBAS_ActorVals* compareWith) {
	return (AA_DIFF(wep->ref,0) ||  ((t + UPDATE_INTERVAL) < compareWith->t) || AA_DIFF(_FTG,10));
}



void CBAS_ActorVals::UpdateAttackSpeed(Actor* a) {
	//attack speed being updated, so update time
	t = GetTickCount();
	aSpd = DEFAULT_ATTACK_SPEED;

	const SInt32 STR = a->GetActorValue(kActorVal_Strength);

	//Attack [F]actors
	//
	float ftgF = CBAS_Lib::FatigueFactor(a,_FTG,GETINI(CBAS_FatigueThreshold));					//fatigue
	float encF = CBAS_Lib::EncumbranceFactor(a,STR,GETINI(CBAS_LocalisedEncumbrance));			//encumbrance
	float skillF = CBAS_Lib::SkillFactor(a,wep->type);											//skill in equipped weapon (e.g. blade, blunt)

	float attrF = CBAS_Lib::AttrFactor(a,STR,wep->type,wep->weight);										//main attribute factor, depends on weapon type, e.g. melee weapons use str, bows use agi
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
	_MESSAGE("			MaxSpeed [weight: %f]: %f",wep->weight,PRSNK_MAX_WEAP_SPEED(wep->weight));
	_MESSAGE("	Full modification factor (PRE PRSNK %f)",aSpd);
#endif

	aSpd = PRSNK_FACTOR_CONFIG(aSpd,CBAS_IniHandler->LowComplement,CBAS_IniHandler->LowMult);

	_DOUT("	Final calculation speed: [%f x %f]",PRSNK_MAX_WEAP_SPEED(wep->weight),aSpd);

	aSpd *= PRSNK_MAX_WEAP_SPEED(wep->weight);

	_DOUT("	Raw (POST PRSNK): %f",aSpd);

	aSpd *= GETINI(CBAS_FinalMult);

	_DOUT("	Final (Config Multiplier): %f",aSpd);
}



/*************************
	Actor Tracker
*************************/

CBAS_Actors::CBAS_Actors() {
	t_c = GetTickCount();		//construct with current time as last cleansed time(s)
}	


//cleanse actor map every so often so long play sessions don't fill up map with actor data
void CBAS_Actors::CleanseActorMap() {
	t_c = GetTickCount();
_DOUT("	Cleansing old actors!");
	CBAS_AVMap::iterator avIter;
	for( avIter = ActorValuesMap.begin(); avIter != ActorValuesMap.end(); avIter++){
_DOUT("		Checking actor %s (%x)...",GetFullName(LookupFormByID(avIter->first)),avIter->first);
	if( TIME_PASSED(avIter->second->t,CLEANSE_INTERVAL) ) {

_DOUT("			Cleansing actor!");

			ActorValuesMap.erase(avIter->first);	//erasing an actor from the map, invalidating the iterator
			CleanseActorMap();	//start a new loop in case there are any more actors to erase
			break;
		}
	}
}


void CBAS_Actors::CleanseAnimDataMap() {
	t_c = GetTickCount();

_DOUT("	Cleansing old animdata!");

	CBAS_AADMap::iterator aadIter;
	for( aadIter = AnimDataMap.begin(); aadIter != AnimDataMap.end(); aadIter++){

_DOUT("		Checking animdata %i...",GetFullName(LookupFormByID(aadIter->first)),aadIter->first);

		if( TIME_PASSED(aadIter->second,CLEANSE_INTERVAL) ) {
_DOUT("			Cleansing animdata!");
			AnimDataMap.erase(aadIter->first);	//erasing an actor from the map, invalidating the iterator
			CleanseActorMap();	//start a new loop in case there are any more actors to erase
			break;
		}
	}
}




void CBAS_Actors::LoadIniValues() {
#ifdef _DEBUG
	delete CBAS_IniHandler;
#endif
	CBAS_IniHandler = new CBAS_Config::IniHandler();
}

float CBAS_Actors::AttackSpeedFromActor(Actor* a){
	bool actorExists = ActorValuesMap.count(a->refID) > 0;		//first find out if this actor already exists in the map
	bool update = false;	//do we need to calc a new attack speed for the actor?

	CBAS_ActorVals* AV;
	CBAS_ActorVals *n_AV = new CBAS_ActorVals(a);		//new actor vals to compare against

	if( actorExists ){
		AV = ActorValuesMap[a->refID];
		AV = ActorValuesMap.at(a->refID);	//get actor from map
		update = AV->OutOfDate(n_AV);	//is this actor out of date?
	}

	if( !actorExists || update ){
		//use the new actor as the current one in the map
		AV = n_AV;

#if _DEBUG
	DWORD ticks = (AV->t - t_c) / 1000;
	const DWORD seconds = ticks % 60;
	ticks /= 60;
	const DWORD minutes = ticks % 60;
	ticks /= 60;
	const DWORD hours = ticks; // may exceed 24 hours.
	const char* addOr = actorExists ? "Updating" : "Adding";
	_MESSAGE("[Time: %d:%02d:%02d (%d)]: %s actor %s (%X)!",hours, minutes, seconds, AV->t,addOr,GetFullName(a),a->refID);
#endif

		//calculate attack speed for new actor
		AV->UpdateAttackSpeed(a);
		//put new actor in place
		ActorValuesMap[AV->actorRef] = AV;
	}

	//clean old actors every so often
	if( update && TIME_PASSED(t_c,CLEANSE_CHECK_INTERVAL) ) { CleanseActorMap(); CleanseAnimDataMap(); }

#if _DEBUG
	//TESObjectWEAP* eqObj = (TESObjectWEAP*)LookupFormByID(AV->wepRef);
	//_MESSAGE("[Time: %d]: Returning attack speed for actor %s using weapon %s with weight %f to %f!",AV->t,GetFullName(a),GetFullName(eqObj),AV->wWgt,AV->aSpd);
	//_MESSAGE("(Actor existed in map? %d. Actor needed updating? %d.)",actorExists, update);
#endif
	return AV->aSpd;
}


void CBAS_Actors::SetAttackSpeed(void* p){
//_DOUT("SetAttackSpeed: %p at %i...",p,GetTickCount());
	bool aadExist = AnimDataMap.count((uintptr_t)p) > 0;		//find out if this animdata pointer has already an entry in the map
	bool touched = false;	//has this pointer actually been processed fully or is it still not time
	
#if _DEBUG
	if(aadExist){
		//_MESSAGE("%p timings: (%i - %i = %i)",p,GetTickCount(),AnimDataMap.at((uintptr_t)p),(GetTickCount() - AnimDataMap.at((uintptr_t)p)));
	}
#endif

	if( aadExist && !TIME_PASSED(AnimDataMap.at((uintptr_t)p), TOUCH_INTERVAL) ){
//_DOUT("Not checking %p because too soon (%i - %i = %i)",p,GetTickCount(),AnimDataMap.at((uintptr_t)p),(GetTickCount() - AnimDataMap.at((uintptr_t)p)));
		return;			//skip any further processing on this pointer
	}

	touched = true;
//_DOUT("Received pointer %p, attempting to get animdata...",p);
	if ( Actor* a = ResolveActorFromAnimData(p) ){
//_DOUT("	GOT ACTOR FROM %p",p);
		(reinterpret_cast<CBAS_ActorAnimData*>(p))->t_period = AttackSpeedFromActor(a);
	} else {
		//if we can't get actor
_DOUT("	Didn't get actor from %p",p);
	}

	if( !aadExist || touched ) {
		AnimDataMap[(uintptr_t)p] = GetTickCount();		//update map
_DOUT("Updating map at %x to be %i because either %p didn't exist: %i, or touched: %i",(uintptr_t)p,GetTickCount(),p,!aadExist,touched);
	}
}


Actor* CBAS_Actors::ResolveActorFromAnimData(void* p){
	if( CBAS_ActorAnimData* c_aad = reinterpret_cast<CBAS_ActorAnimData*>(p) ){
		if( NiNode* niNode = c_aad->niNode04 ){
			if( niNode->m_extraDataList && niNode->m_extraDataListLen > 2 ){
				NiExtraData* niExD = niNode->m_extraDataList[2];
				if( std::string(niExD->GetType()->name) == "TESObjectExtraData" ){
					if( Actor* ActorFromAnimData = reinterpret_cast<Actor*>(((TESObjectExtraData*)niExD)->refr) ){
						return ActorFromAnimData;
					PRSNK_ERR("		Failed to cast actor at last hurdle! %s",niExD->GetType()->name);
					}
				PRSNK_ERR("		NiExtraData does not match TESObjectExtraData! %s",niExD->GetType()->name);
				}
			PRSNK_ERR("		NiNode's extradatalist not found or too short from node at %p",niNode);
			}
			PRSNK_ERR("		NiNode not found from animdata at %p",c_aad);
		}
	PRSNK_ERR("		Animdata not found at %p",p);
	}

	return NULL;
}


#if 0

Actor* CBAS_Actors::ResolveActorFromAnimData(void* p){
	if( CBAS_ActorAnimData* c_aad = reinterpret_cast<CBAS_ActorAnimData*>(p) ){
		if( NiNode* niNode = c_aad->niNode04 ){
			if( niNode->m_extraDataList && niNode->m_extraDataListLen > 2 ){
				NiExtraData* niExD = niNode->m_extraDataList[2];
				if( std::string(niExD->GetType()->name) == "TESObjectExtraData" ){
					if( Actor* ActorFromAnimData = reinterpret_cast<Actor*>(((TESObjectExtraData*)niExD)->refr) ){
#ifdef _DEBUG
						_MESSAGE("	Got actor %s at last hurdle from: %s",GetFullName(ActorFromAnimData),ActorFromAnimData->refID,niNode->m_extraDataList[2]->GetType()->name);
#endif
						return ActorFromAnimData;
#ifdef _DEBUG
					} else {
						_MESSAGE("		Failed to cast actor at last hurdle! %s",niExD->GetType()->name);
#endif
					}
#ifdef _DEBUG
				} else {
					_MESSAGE("		NiExtraData does not match TESObjectExtraData! %s",niExD->GetType()->name);
#endif
				}
#ifdef _DEBUG
			} else {
				_MESSAGE("		NiNode's extradatalist not found or too short from node at %p",niNode);
#endif
			}
#ifdef _DEBUG
		} else {
			_MESSAGE("		NiNode not found from animdata at %p",c_aad);
#endif
		}
#ifdef _DEBUG
	} else {
			_MESSAGE("		Animdata not found at %p",p);
#endif
	}

	return NULL;
}

#endif
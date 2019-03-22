#include "CharacterBasedAttackSpeed.h"

/***************************
* OBSE and Game code
***************************/

#if 0 //this section is just here for reference



static bool Cmd_GetEquipmentSlotType_Execute(COMMAND_ARGS)
{
	UInt32	* refResult = (UInt32 *)result;

	*refResult = 0;

	// easy out if we don't have an object
	if(!thisObj) return true;

	UInt32	slotIdx = 0;
	if(!ExtractArgs(paramInfo, arg1, opcodeOffsetPtr, thisObj, arg3, scriptObj, eventList, &slotIdx)) return true;
	feGetObject getObject;

	bool bFound = FindEquipped(thisObj, slotIdx, &getObject, result);
	if (IsConsoleMode())
	{
		if (!bFound)
			Console_Print("Nothing equipped in that slot.");
		else
		{
			TESForm* eqObj = LookupFormByID(*refResult);
			if (eqObj)
				Console_Print("GetEquippedObject %d >> %s (%08x)", slotIdx, GetFullName(eqObj), eqObj->refID);
		}
	}
	return true;
}

static enum {
	kActorVal_Strength = 0,		// 0x00
	kActorVal_Intelligence,
	kActorVal_Willpower,
	kActorVal_Agility,
	kActorVal_Speed,
	kActorVal_Endurance,		//0x05
	kActorVal_Personality,
	kActorVal_Luck,
	kActorVal_Health,
	kActorVal_Magicka,
	kActorVal_Fatigue,			// 0x0a
	kActorVal_Encumbrance,

	kActorVal_Armorer,
	kActorVal_Athletics,
	kActorVal_Blade,
	kActorVal_Block,			// 0x0f
	kActorVal_Blunt,			// 0x10
	kActorVal_HandToHand,
	kActorVal_HeavyArmor,

	kActorVal_Alchemy,			
	kActorVal_Alteration,
	kActorVal_Conjuration,		// 0x15
	kActorVal_Destruction,
	kActorVal_Illusion,
	kActorVal_Mysticism,
	kActorVal_Restoration,

	kActorVal_Acrobatics,		// 0x1a
	kActorVal_LightArmor,
	kActorVal_Marksman,
	kActorVal_Mercantile,
	kActorVal_Security,
	kActorVal_Sneak,			// 0x1f
	kActorVal_Speechcraft,		// 0x20
	kActorVal_OblivionMax
};

#endif


class FoundEquipped
{
public:
	virtual bool Found(ExtraContainerChanges::EntryData* entryData, double *result, ExtraContainerChanges::EntryExtendData* extendData) = 0;
};


class feGetObject : public FoundEquipped
{
public:
	feGetObject() {}
	~feGetObject() {}
	virtual bool Found(ExtraContainerChanges::EntryData* entryData, double *result, ExtraContainerChanges::EntryExtendData* extendData) {
		UInt32* refResult = (UInt32*) result;
		if (entryData) {
			// cool, we win
			*refResult = entryData->type->refID;
			//Console_Print("refID = %08X (%s)", *refResult, GetFullName(entry->data->type));
			return true;
		}
		return false;
	}
};


/***************************
* Optimised for CBAS
***************************/


/***************************
* GetARMOR Functions

***************************/
static bool ArmorSlotMatches(TESForm* pForm, UInt32 slot)
{
	bool bMatches = false;
	if (pForm) {
		TESBipedModelForm	* pBip = (TESBipedModelForm *)Oblivion_DynamicCast(pForm, 0, RTTI_TESForm, RTTI_TESBipedModelForm, 0);
		if (pBip) {
			// if slot is an official slot match it
			if (slot < TESBipedModelForm::kPart_Max) {
				UInt32 inMask = TESBipedModelForm::MaskForSlot(slot);
				if ((inMask & pBip->partMask) != 0) {
					bMatches = true;
				}
			}
			// if slot is an unoffical multi-slot - it must match exactly
			else if (pBip->GetSlot() == slot){
				bMatches = true;
			}
		}
	}
	return bMatches;
}



static bool FindEquippedArmor(TESObjectREFR* thisObj, UInt32 slotIdx, FoundEquipped* foundEquippedFunctor, double* result)
{
	ExtraContainerChanges	* containerChanges = static_cast <ExtraContainerChanges *>(thisObj->baseExtraList.GetByType(kExtraData_ContainerChanges));
	if(containerChanges && containerChanges->data && containerChanges->data->objList)
	{
		for(ExtraContainerChanges::Entry * entry = containerChanges->data->objList; entry; entry = entry->next)
		{
			// do the fast check first (an object must have extend data to be equipped)
			if(entry->data && entry->data->extendData && entry->data->type)
			{
				if (ArmorSlotMatches(entry->data->type, slotIdx)) {

					// ok, it's the right type, now is it equipped?
					for(ExtraContainerChanges::EntryExtendData * extend = entry->data->extendData; extend; extend = extend->next)
					{
						if (extend->data) {
							bool bFound = false;
							if (extend->data->HasType(kExtraData_Worn))
							{
								bFound = true;
							}

							if (bFound)
							{
								return foundEquippedFunctor->Found(entry->data, result, extend);
							}
						}
					}
				}
			}
		}
	}
	return false;
}

static CBAS_Armor* GetCuirass(TESObjectREFR * thisObj)
{
	//a double with a pointer to it is required by FindEquippedWeapon
	double resultVal = 0;
	double * result = &resultVal;
	UInt32	* refResult = (UInt32 *)result;
	*refResult = 0;

	UInt32	slotIdx = TESBipedModelForm::kPart_UpperBody; //the cuirass slot
	feGetObject getObject;

	//bool bFound = ;
	CBAS_Armor* arm;

	if( FindEquippedArmor(thisObj, slotIdx, &getObject, result) ) { 
		TESObjectARMO* eqObj = (TESObjectARMO*)LookupFormByID(*refResult);
		arm = new CBAS_Armor(eqObj->IsHeavyArmor(),eqObj->weight.weight);
	} else {
		arm = new CBAS_Armor();
	}

	return arm;
}

static CBAS_Armor* GetGauntlets(TESObjectREFR * thisObj)
{
	//a double with a pointer to it is required by FindEquippedWeapon
	double resultVal = 0;
	double * result = &resultVal;
	UInt32	* refResult = (UInt32 *)result;
	*refResult = 0;

	UInt32	slotIdx = TESBipedModelForm::kPart_Hand; //the gauntlets slot
	feGetObject getObject;

	//bool bFound = ;
	CBAS_Armor* arm;

	if( FindEquippedArmor(thisObj, slotIdx, &getObject, result) ) { 
		TESObjectARMO* eqObj = (TESObjectARMO*)LookupFormByID(*refResult);
		arm = new CBAS_Armor(eqObj->IsHeavyArmor(),eqObj->weight.weight);
	} else {
		arm = new CBAS_Armor();
	}

	return arm;
}


/***************************
* GetWEAPON Functions
***************************/


static bool FindEquippedWeapon(TESObjectREFR* thisObj, UInt32 slotIdx, FoundEquipped* foundEquippedFunctor, double* result)
{
	ExtraContainerChanges	* containerChanges = static_cast <ExtraContainerChanges *>(thisObj->baseExtraList.GetByType(kExtraData_ContainerChanges));
	if(containerChanges && containerChanges->data && containerChanges->data->objList)
	{
		for(ExtraContainerChanges::Entry * entry = containerChanges->data->objList; entry; entry = entry->next)
		{
			// do the fast check first (an object must have extend data to be equipped)
			if(entry->data && entry->data->extendData && entry->data->type)
			{
				if (Oblivion_DynamicCast(entry->data->type, 0, RTTI_TESForm, RTTI_TESObjectWEAP, 0) != 0) {		//check type can be cast to weapon
					// ok, it's the right type, now is it equipped?
					for(ExtraContainerChanges::EntryExtendData * extend = entry->data->extendData; extend; extend = extend->next)
					{
						if (extend->data) {
							if (extend->data->HasType(kExtraData_Worn))
							{
								return foundEquippedFunctor->Found(entry->data, result, extend);
							}
						}
					}
				}
			}
		}
	}
	return false;
}


//This is a custom version of Cmd_GetEquipmentSlotType_Execute from Commands_Inventory.cpp, reduced to only require necessary arguments
TESObjectWEAP* GetEquippedWeapon(TESObjectREFR * thisObj)
{
	//a double with a pointer to it is required by FindEquippedWeapon
	double resultVal = 0;
	double * result = &resultVal;
	UInt32	* refResult = (UInt32 *)result;
	*refResult = 0;

	UInt32	slotIdx = TESBipedModelForm::kPart_Max; //_TESBipedModelForm::kPart_Max is 16 -- the weapon slot
	feGetObject getObject;

	bool bFound = FindEquippedWeapon(thisObj, slotIdx, &getObject, result);

	TESObjectWEAP* eqObj = (TESObjectWEAP*)LookupFormByID(*refResult);

	return eqObj;
}


#define ACTOR_STRENGTH_ENCUMBRANCE_MULT_DEFAULT 5.f
#define ACTOR_STRENGTH_ENCUMBRANCE_MULT "fActorStrengthEncumbranceMult"

static float EncumbranceMultiplier(){
	float enMult = ACTOR_STRENGTH_ENCUMBRANCE_MULT_DEFAULT;
	//Get encumbrance multiplier from game
	SettingInfo* setting = NULL;
	if( GetGameSetting(ACTOR_STRENGTH_ENCUMBRANCE_MULT,&setting) ) {
		enMult = setting->f;
#ifdef _DEBUG
	} else {
		_MESSAGE("encumbrance mult GameSetting not found! ");
#endif
	}
	return enMult;
}



/***************************
* CharacterBasedAttackSpeed Functions
***************************/

namespace CBAS {


// - SkillFactor -
// Simply get the weapon type and corresponding skill for actor
// Then put through PRSNK_FACTOR() as a percentage, so that 0 skill halves the weapon speed, 50 skill has a ~25% penalty etc..
float SkillFactor(Actor* a, UInt32 wepType) {
	float skillVal = 100.f;
	UInt32 skillToGet = WepTypeSkillMap[wepType];
#if _DEBUG
			_MESSAGE("	Using 0x%x as skill for skill factor.",skillToGet);
#endif

	skillVal = skillToGet == kActorVal_OblivionMax ? 100.f : a->GetActorValue(skillToGet);
	return skillVal/100.f;
}


// - AttrFactor -
// Simply get the weapon type and corresponding attribute(s) for actor
// Then put through PRSNK_FACTOR() so that 0 attr halves the weapon speed, 50 skill has a ~25% penalty etc..
// The non-melee weapons are generally lighter, so the ratio will come out much higher, for this reason the ratios are quartered
// Let's say (early game) bow has 8 weight.. early game agi might be 30-40 for one specialised in marksmanship, 
//	which becomes 7-10, so the ratio is around 1.00 -- no penalty.
//	other character types will have say 15-20 agi 
float AttrFactor(Actor* a, SInt32 STR, UInt32 wepType, float wepWeight) {
	float attrF = 1.f;
	UInt32 attrToGet = kActorVal_OblivionMax;
	
	if(wepType < kType_Staff){			//melee
			attrF = float(STR)/wepWeight;
#if _DEBUG
			_MESSAGE("	Using STR as attribute factor because melee weapon.");
#endif
	} else if(wepType < kType_Bow){		//staff
		attrF = (a->GetActorValue(kActorVal_Intelligence) + a->GetActorValue(kActorVal_Willpower))*.125f/wepWeight; //staffs can have weight as low as 5, high as 12, so contribution is further halved to give .125
#if _DEBUG
			_MESSAGE("	Using INT+WILL as attribute factor because staff.");
#endif
	} else if(wepType < kType_Max) {	//bow
		attrF = a->GetActorValue(kActorVal_Agility)*.25f/wepWeight;		//bows can have weight as low as 8 and high as 22 so contribution from agility halved
#if _DEBUG
			_MESSAGE("	Using AGI as attribute factor because bow.");
#endif
	}

	return PRSNK_FACTOR(attrF);
}


// - EncumbranceFactor -
float EncumbranceFactor(Actor* a, SInt32 STR, float fComplex){
	//get total encumbrance ratio
	float T_Encumbrance = PRSNK_FACTOR(1 - (float(a->GetActorValue(kActorVal_Encumbrance))/float(a->GetActorValue(kActorVal_Strength)*EncumbranceMultiplier())));		//"1-x" cause less encumbered = more speed

	//fComplex is a ratio of how much 'weight' (mathematical) the upper armor and gauntlets get compared to 
	//I will probably set it to around .75f by default
	//giving a .25 contribution of total weight slowing down attacks
	if( fComplex > 0 ){
		T_Encumbrance *= (1 - fComplex);	//"total encumbrance" contribution gets diminished

		//(upperbody + arms) ranges: 6-18 (light) and 36-72 (heavy)
		//it's awkward but need to get armor type of each and calculate separately -.-'
		CBAS_Armor* chest = GetCuirass(a);
		CBAS_Armor* hands = GetGauntlets(a);

		//only retrieve armor skill if the armor type is in use
		float LArm = chest->isHeavy+hands->isHeavy < 2 ? a->GetActorValue(kActorVal_LightArmor) : 1.0f;
		float HArm = chest->isHeavy+hands->isHeavy > 0 ? a->GetActorValue(kActorVal_HeavyArmor) : 1.0f;

		//Below are the actual ratios for how each part will affect weapon speed; we will just average them so they get equal contribution (hands should have more influence, but weigh less generally)
		//
		//Chest --	varies from 5-15 (light), 30-60 (heavy)
		//Heavy: Take average of heavy arm and strength, divide by weight
		//Light: light armor divided by 5 divided by weight
		float ChestVal = PRSNK_FACTOR(chest->isHeavy ? (float(STR+HArm)*.5f/chest->weight) : (LArm*.2f/max(chest->weight,0.0001f)) );

		//Hands -- varies from 1-3 (light), 6-12 (heavy)
		//Heavy: Take average of heavy arm and str, divide by ~4.5, then weight
		//Light: light armor divided by 32, then weight
		float HandsVal = PRSNK_FACTOR(hands->isHeavy ? (float(STR+HArm)*.5f*.225f/hands->weight) : (LArm*.03125f/max(hands->weight,0.0001f)) );

		fComplex = fComplex*((ChestVal+HandsVal)*.5f);	//complex ratio contribution gets diminished, after finding average of chest/hands values
	}

#if _DEBUG
	//_MESSAGE("Returning encumbrance factor of %f -- Complex: [%f] Total: [%f]",(fComplex + T_Encumbrance),fComplex,T_Encumbrance);
#endif
	return (fComplex + T_Encumbrance);
}


//Fatigue Factor
//Using a threshold, below which weapon speed starts to be impacted
float FatigueFactor(Actor* a, UInt32 baseFatigue,float fatigueThreshold){
	float fatigue = float(a->GetActorValue(kActorVal_Fatigue))/float(baseFatigue);
	_MESSAGE("Getting fatigue... curr %i base %i ratio %f thresh: %f",a->GetActorValue(kActorVal_Fatigue),a->GetBaseActorValue(kActorVal_Fatigue),fatigue,fatigueThreshold);
	return min(max(0.f,fatigue/fatigueThreshold),1.f);
}






}
#include "obse/GameObjects.h"


class FoundEquipped;

/***************************
* Optimised for CBAS
***************************/


/*
* GetARMOR Functions
*/
struct CBAS_Armor {
	bool isHeavy;
	float weight;

	CBAS_Armor(bool bH = false, float fW = 0.f):isHeavy(bH),weight(fW){}
};

static bool ArmorSlotMatches(TESForm* pForm, UInt32 slot);

static bool FindEquippedArmor(TESObjectREFR* thisObj, UInt32 slotIdx, FoundEquipped* foundEquippedFunctor, double* result);

static CBAS_Armor* GetCuirass(TESObjectREFR * thisObj);

static CBAS_Armor* GetGauntlets(TESObjectREFR * thisObj);

/***************************
* GetWEAPON Functions
***************************/


static bool FindEquippedWeapon(TESObjectREFR* thisObj, UInt32 slotIdx, FoundEquipped* foundEquippedFunctor, double* result);

TESObjectWEAP* GetEquippedWeapon(TESObjectREFR * thisObj);


#define ACTOR_STRENGTH_ENCUMBRANCE_MULT_DEFAULT 5.f
#define ACTOR_STRENGTH_ENCUMBRANCE_MULT "fActorStrengthEncumbranceMult"

static float EncumbranceMultiplier();


/***************************
/ --  PRSNK Factor --
/	maps to: [x=0, y=0.5], [x=0.5,y=0.76], [x=1,y=0.91], [x=2,y=0.97]
/		- good for weapon speeds because we never want it to dip below 0.5
/		 and high attribute values (200+) cannot raise the value past 1 (asymptotic)
/ formula:
/		1 - (0.6/((4*(x^2.2) + 1)))		[where x is e.g. character Speed or Str out of 100]
/		old formula (not as punishing): 1 - (0.5/((5*(x^2.2) + 1)))
***************************/
#define PRSNK_POW(b,p) (pow(max(0.f,b),p))
#define PRSNK_FACTOR(x) (1.f - (.6f/((4.f*(PRSNK_POW(x,2.2f)) + 1.f))))


// - GetMaxWeaponSpeed -
// We have weight of weapon, we want to know how fast such a weapon could be swung... for this we get a new weight factor
//	Let's say A weapon of 93 weight will have ~1.06 attacks per second; a weapon of 3 weight: ~3.5 aps
//	Formula for this is:
//		8/(2*(x^0.225))	[Where x is weight]
//	m = minimum weapon weight (if people make mods with ridiculously light weapons, swing speed will go up to >6x per sec...)
#define PRSNK_MAX_WEAP_SPEED(weight) (8.f / (2.f*pow(weight,.25f)))


//This can be used to reduce the impact that a particular component of the main attack speed formula has
//e.g. if a weighting of 0 is provided, that particular component provides a 1.0f contribution as this means
// it is not contributing to any kind of diminishing of the attack speed
#define PRSNK_UNDERWEIGH(component,weight) ((1 - weight) + (component*weight))


/***************************
* CharacterBasedAttackSpeed Functions
***************************/

namespace CBAS {

static enum
{
	kType_BladeOneHand = 0,
	kType_BladeTwoHand,
	kType_BluntOneHand,
	kType_BluntTwoHand,
	kType_Staff,
	kType_Bow,

	kType_Max,
};

//Map weapon type to skill (every instruction counts!)
static UInt32 WepTypeSkillMap[kType_Max] = {
	kActorVal_Blade,		//blade1h - 0
	kActorVal_Blade,		//blade2h - 1
	kActorVal_Blunt,		//blunt1h - 2
	kActorVal_Blunt,		//bl2h - 3
	kActorVal_OblivionMax,	//staff... nothing - 4
	kActorVal_Marksman		//bow - 5
};


// - SkillFactor -
// Simply get the weapon type and corresponding skill for actor
// Then put through PRSNK_FACTOR() as a percentage, so that 0 skill halves the weapon speed, 50 skill has a ~25% penalty etc..
float SkillFactor(Actor* a, UInt32 wepType);


// - AttrFactor -
// Simply get the weapon type and corresponding attribute(s) for actor
// Then put through PRSNK_FACTOR() so that 0 attr halves the weapon speed, 50 skill has a ~25% penalty etc..
// The non-melee weapons are generally lighter, so the ratio will come out much higher, for this reason the ratios are quartered
// Let's say (early game) bow has 8 weight.. early game agi might be 30-40 for one specialised in marksmanship, 
//	which becomes 7-10, so the ratio is around 1.00 -- no penalty.
//	other character types will have say 15-20 agi 
float AttrFactor(Actor* a, SInt32 STR, UInt32 wepType, float wepWeight);


// - EncumbranceFactor -
float EncumbranceFactor(Actor* a, SInt32 STR, float fComplex);

//Fatigue Factor
//Using a threshold, below which weapon speed starts to be impacted
float FatigueFactor(Actor* a, UInt32 baseFatigue,float fatigueThreshold);

}
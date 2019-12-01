#include "obse/GameObjects.h"

//Convenient debug-only messaging
#ifdef _DEBUG
	#define _DOUT(errmsg, ...) _MESSAGE(errmsg,__VA_ARGS__)
#else
	#define _DOUT(errmsg, ...)
#endif

//ELSE-based error messaging
#if defined _DEBUG
	#define PRSNK_ERR(errmsg, ...) } else { _MESSAGE(errmsg,__VA_ARGS__)
#else
	#define PRSNK_ERR(errmsg, ...)
#endif

/***********************
/ WEAPON WEIGHT BOUNDS
**********************/

//global
#define MIN_WEAP_WEIGHT 1.f

//staff
#define MIN_STAFF_WEIGHT 5.f
#define MAX_STAFF_WEIGHT 12.f
#define RANGE_STAFF_WEIGHT (MAX_STAFF_WEIGHT - MIN_STAFF_WEIGHT)

//bow
#define MIN_BOW_WEIGHT 8.f
#define MAX_BOW_WEIGHT 22.f
#define RANGE_BOW_WEIGHT (MAX_BOW_WEIGHT - MIN_BOW_WEIGHT)

//weight to default to for fists/handtohand
#define UNARMED_WEAP_WEIGHT 90.f				//equates to around 1.005 attacks per second with MaxSpeed formula 10/(2+x)^0.5
#define ARM_BASE_WEIGHT 7.f						//this is added on to all weapons so that even the very lightest ones are slower
#define CREAT_BASE_WEIGHT ARM_BASE_WEIGHT*3.f	//this is used in creature weight formula

#define BOUND_WEIGHT_FACTOR .8f	//bound weapons are slightly lighter than they should be due to being magic
//Default weapon weights, used for bound weapons, based on damage done by bound weapon type (the average value then multiplied by 0.8 [BOUND WEIGHT FACTOR])
#define BLADE1H_WEIGHT	6.f				// 7.5f
#define BLADE2H_WEIGHT	49.6f			// 62.f - the bound claymore actually does more damage than any other claymore, picking the max claymore weight here which ends up being a little bonus for the bound version
#define BLUNT1H_WEIGHT	28.8f			// 36.f - there are two bound blunt 1h types, this is an approximate average of the two weights for their damage values (18 - axe, 22 - mace)
#define BLUNT2H_WEIGHT	74.4f			// 93.f - there isn't a bound blunt 2h in the base game, picking the heaviest to be safe
#define STAFF_WEIGHT	9.6f			// 12.f - again, no bound staff so picking heaviest
#define BOW_WEIGHT		13.6f			// 17.f - approx. based on damage of 15

//


/*********************/
/*********************/
/*********************/


class FoundEquipped;

namespace CBAS_Lib {

	static enum
	{
		kType_BladeOneHand = 0,
		kType_BladeTwoHand,
		kType_BluntOneHand,
		kType_BluntTwoHand,
		kType_Staff,
		kType_Bow,
		kType_HandToHand,
		kType_Max,
	};

	static enum
	{
		CBAS_Attr_STR,
		CBAS_Attr_INT_WILL,
		CBAS_Attr_AGI
	};

	//Map weapon type to default weapon weights	(these are used when weapon provided has no weight but it does have a type (i.e. bound weapon)
	static float DefaultWepWeights[kType_Max+1] = {
		BLADE1H_WEIGHT,			//blade1h - 0
		BLADE2H_WEIGHT,			//blade2h - 1
		BLUNT1H_WEIGHT,			//blunt1h - 2
		BLUNT2H_WEIGHT,			//bl2h - 3
		STAFF_WEIGHT,			//staff - 4
		BOW_WEIGHT,				//bow - 5
		UNARMED_WEAP_WEIGHT,	//handtohand (CBAS) - 6
		MIN_WEAP_WEIGHT			//kType_Max
	};

	//Map CBAS weapon types to skills (every instruction counts!)
	static UInt32 WepTypeSkillMap[kType_Max+1] = {
		kActorVal_Blade,		//blade1h - 0
		kActorVal_Blade,		//blade2h - 1
		kActorVal_Blunt,		//blunt1h - 2
		kActorVal_Blunt,		//bl2h - 3
		kActorVal_OblivionMax,	//staff - 4
		kActorVal_Marksman,		//bow - 5
		kActorVal_HandToHand,	//handtohand (CBAS) - 6
		kActorVal_OblivionMax	//kType_Max
	};

	//Map CBAS weapon types to attributes
	static UInt32 WepTypeAttributeMap[kType_Max+1] = {
		CBAS_Attr_STR,			//blade1h - 0
		CBAS_Attr_STR,			//blade2h - 1
		CBAS_Attr_STR,			//blunt1h - 2
		CBAS_Attr_STR,			//bl2h - 3
		CBAS_Attr_INT_WILL,		//staff... nothing - 4
		CBAS_Attr_AGI,			//bow - 5
		CBAS_Attr_STR,			//handtohand (CBAS) - 6
		kActorVal_OblivionMax	//kType_Max
	};

}




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

struct CBAS_Weapon {
	float weight;
	UInt32 ref;
	UInt32 type;

	CBAS_Weapon(float w, float r, UInt32 rID, UInt32 t) : weight(max(MIN_WEAP_WEIGHT,w)),ref(rID),type(t) {
		if( w <= 0 ) {		// it has no weight! be bound weapon, retrieve preassigned weight
			weight = CBAS_Lib::DefaultWepWeights[t];
			_DOUT("[DOUT] CBAS - Got bound weapon!? ref: %x type: %d, assigning weight of: %f",rID,t,weight);
		}
		if( r > 0.f && r < 1.f ) {
			_DOUT("Weight is: %f, reducing by reach %f to give %f",weight,r,weight/r);
			weight /= r;
		}
		//add arm weight
		_DOUT("Add arm base weight: %f to give %f",ARM_BASE_WEIGHT,weight+ARM_BASE_WEIGHT);
		weight += ARM_BASE_WEIGHT;
	}

	//no weapon found
	CBAS_Weapon() : weight(UNARMED_WEAP_WEIGHT), ref(0), type(CBAS_Lib::kType_HandToHand) {_DOUT("CBAS - UNARMED! EMPTY WEAPON!");}
};

static bool FindEquippedWeapon(TESObjectREFR* thisObj, UInt32 slotIdx, FoundEquipped* foundEquippedFunctor, double* result);

CBAS_Weapon* GetEquippedWeapon(TESObjectREFR * thisObj);

TESCreature* GetCreature(TESObjectREFR* thisObj);


/***************************
/ --  PRSNK Factor --
/	maps to: [x=0, y=0.5], [x=0.5,y=0.76], [x=1,y=0.91], [x=2,y=0.97]
/		- good for weapon speeds because we never want it to dip below 0.5
/		 and high attribute values (200+) cannot raise the value past 1 (asymptotic)
/ formula:
/		1 - (0.6/((4*(x^2.2) + 1)))		[where x is e.g. character Speed or Str out of 100]
/		old formula (not as punishing): 1 - (0.5/((5*(x^2.2) + 1)))
***************************/
#define PRSNK_POW(b,p) (pow(max(0.0001f,b),p))	//must not go <= 0.f
//#define PRSNK_FACTOR(x) (1.f - (.6f/((4.f*(PRSNK_POW(x,2.2f)) + 1.f))))	//original
#define PRSNK_FACTOR(x) (1.f - (.6f/((4.f*(PRSNK_POW(x,1.5f)) + 1.f))))	//tweaked
#define PRSNK_FACTOR_CONFIG(x,C,L) (1.f - (C/((L*(PRSNK_POW(x,1.5f)) + 1.f))))	//configurable

// - GetMaxWeaponSpeed -
// We have weight of weapon, we want to know how fast such a weapon could be swung... for this we get a new weight factor
//	Let's say A weapon of 93 weight will have ~1.06 attacks per second; a weapon of 3 weight: ~3.5 aps
//	Formula for this is:
//		8/(2*(x^0.225))	[Where x is weight]
#define PRSNK_MAX_WEAP_SPEED(weight) (10.f / pow(2+weight,.5f)) //1.75


//This can be used to reduce the impact that a particular component of the main attack speed formula has
//e.g. if a weighting of 0 is provided, that particular component provides a 1.0f contribution as this means
// it is not contributing to any kind of diminishing of the attack speed
#define PRSNK_UNDERWEIGH(component,weight) ((1 - weight) + (component*weight))


/**************************
/ - Staff/Bow Value mapping -
/
/	Map the weights of bows or staves onto a ratio 0-100 or 1-200 so that it aligns with attribute factors
/
************************/
#define PRSNK_STAFF_VALUEMAP(weight) (((weight - MIN_STAFF_WEIGHT)/RANGE_STAFF_WEIGHT)*200.f)
#define PRSNK_BOW_VALUEMAP(weight) (((weight - MIN_BOW_WEIGHT)/RANGE_BOW_WEIGHT)*100.f)




/***************************
* CharacterBasedAttackSpeed Functions
***************************/

namespace CBAS_Lib {


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
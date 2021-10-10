#include "GameObjects.h"
#include "inih/INIReader.h"

namespace CBAS_Config
{
	static enum IniValTypes {
		CBAS_iniFloat,
		CBAS_iniBool,
		CBAS_iniInt
	};

	static enum IniHeadings
	{
		CBAS_Meta,
		CBAS_Components,
		CBAS_Settings,
		CBAS_EndHeadings
	};

	enum IniEntries
	{
		CBAS_Enabled,

		CBAS_Creatures,
		CBAS_Staves,
		CBAS_Hands,
		CBAS_Bows,

		CBAS_FatigueComponent,
		CBAS_EncumbranceComponent,
		CBAS_SkillComponent,
		CBAS_AttributeComponent,
		CBAS_AlacrityComponent,

		CBAS_FinalMult,
		CBAS_LowMult,

		//CBAS_Alacrity_SpeedAgi,
		CBAS_FatigueThreshold,
		CBAS_LocalisedEncumbrance,

		CBAS_EndEntries
	};


struct IniEntry {
	const char* name;
	const char* header;
	float value;

	IniValTypes type;

	IniEntry(const char* h,const char* n, float v, IniValTypes t = CBAS_iniFloat);
};



class IniHandler {
	
	bool bUseAllComponents;

	void SetUseAllComponents();



public:
	IniHandler();

	float operator() (UInt32 iniEntryIndex);

	bool operator()();

	float LowMult;			//used in PRSNK_FACTOR, lowest value of max attack speed is this ratio
	float LowComplement;	//used in PRSNK_FACTOR, accounts for the weird relationship between the two constants in the formula (see constructor)

};

}	//end namespace
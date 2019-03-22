#include "GameObjects.h"
#include "inih/INIReader.h"

namespace CBAS_Config
{
	static enum IniHeadings
	{
		CBAS_Components,
		CBAS_Settings,
		CBAS_EndHeadings
	};

	enum IniEntries
	{
		CBAS_FatigueComponent,
		CBAS_EncumbranceComponent,
		CBAS_SkillComponent,
		CBAS_AttributeComponent,
		CBAS_SpeedComponent,

		CBAS_FatigueThreshold,
		CBAS_LocalisedEncumbrance,

		CBAS_EndEntries
	};


struct IniEntry {
	const char* name;
	const char* header;
	float value;

	IniEntry(const char* h,const char* n, float v);
};



class IniHandler {
	
	bool bUseAllComponents;

	void SetUseAllComponents();

public:
	IniHandler();

	float operator() (UInt32 iniEntryIndex);

	bool operator()();
};

}	//end namespace
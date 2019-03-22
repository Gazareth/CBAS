#include "CBAS_ConfigHandler.h"

#define INI_PATH "Data\\OBSE\\Plugins\\CharacterBasedAttackSpeed.ini"

namespace CBAS_Config
{

IniEntry::IniEntry(const char* h,const char* n, float v) : name(n), header(h),value(v) {}



//Arrays for quick reading from ini file
static char* CBAS_HeadingsStrings[CBAS_EndHeadings] = {
	"Components",
	"Settings"
};

static IniEntry* CBAS_IniEntries[CBAS_EndEntries] = {
	new IniEntry(CBAS_HeadingsStrings[CBAS_Components],"fCBAS_Fatigue",1.f),
	new IniEntry(CBAS_HeadingsStrings[CBAS_Components],"fCBAS_Encumbrance",1.f),
	new IniEntry(CBAS_HeadingsStrings[CBAS_Components],"fCBAS_Skill",1.f),
	new IniEntry(CBAS_HeadingsStrings[CBAS_Components],"fCBAS_Attribute",1.f),
	new IniEntry(CBAS_HeadingsStrings[CBAS_Components],"fCBAS_Speed",1.f),

	new IniEntry(CBAS_HeadingsStrings[CBAS_Settings],"fCBAS_FatigueThreshold",1.f),
	new IniEntry(CBAS_HeadingsStrings[CBAS_Settings],"fCBAS_LocalisedEncumbrance",.75f),
};



void IniHandler::SetUseAllComponents() {
	UInt8 componentsOn = 0;
	UInt8 components[5] = {
		CBAS_FatigueComponent,
		CBAS_EncumbranceComponent,
		CBAS_SkillComponent,
		CBAS_AttributeComponent,
		CBAS_SpeedComponent
	};

	for(UInt8 i = 0;i<5;i++){
		if(CBAS_IniEntries[i]->value != 1.f) return;
	}
	_MESSAGE("Using all components fully!");
	bUseAllComponents = true;
}


IniHandler::IniHandler() : bUseAllComponents(false)
{
	//Now actually read the entries from the ini
	INIReader reader(INI_PATH);
	if (reader.ParseError() < 0) {
		_MESSAGE("[Character-Based Attack Speed]: Couldn't read ini at: %s",INI_PATH);
	} else {
		for(UInt32 i=0; i<CBAS_EndEntries; i++){
			IniEntry* iniE = CBAS_IniEntries[i];

			CBAS_IniEntries[i]->value = reader.GetReal(iniE->header, iniE->name, iniE->value);
#if _DEBUG
			_MESSAGE("[Character-Based Attack Speed] Read: [%s]: %s = %f",iniE->header, iniE->name, iniE->value);
#endif
		}
	}
	SetUseAllComponents();
}

float IniHandler::operator() (UInt32 iniEntryIndex) {
	return CBAS_IniEntries[iniEntryIndex]->value;
}

bool IniHandler::operator()(){
	return bUseAllComponents;
}


}	//end namespace
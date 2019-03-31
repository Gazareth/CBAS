#include "CBAS_ActorAnimData.h"


bool ci_equals(char ch1, char ch2)
{
	return tolower((unsigned char)ch1) == tolower((unsigned char)ch2);
}

bool CBAS_ActorAnimData::PathsInclude(const char* subString)
{
	bool found = false;
	std::string subStr(subString);
	for (UInt32 idx = 0; idx < 5 && !found; idx++)
	{
		BSAnimGroupSequence* anim = animSequences[idx];
		if (anim && anim->filePath)
		{
			std::string pathStr(anim->filePath);
			found = (std::search(pathStr.begin(), pathStr.end(), subStr.begin(), subStr.end(), ci_equals) != pathStr.end());
		}
	}
	return found;
}

bool CBAS_ActorAnimData::FindAnimInRange(UInt32 lowBound, UInt32 highBound)
{
	bool found = false;
	if (highBound == -1)
		highBound = lowBound;

	for (UInt32 idx = 0; idx < 5; idx++)
	{
		BSAnimGroupSequence* anim = animSequences[idx];
		found = (anim && anim->animGroup->animGroup >= lowBound && anim->animGroup->animGroup <= highBound);
		if (found)
			break;
	}
	return found;
}

/*

void CBAS_ActorAnimData::ScanNiObjs(){
	if( !niNode04->m_extraDataList ){
		_MESSAGE("	Extra data list not available!");
		return;
	}
	_MESSAGE("	Extra data list is %i long!",niNode04->m_extraDataListLen);
	for( UInt8 i = 0; i<niNode04->m_extraDataListLen; i++ ){
		if( NiObject* ni_obj = niNode04->m_extraDataList[i] ) {
			std::string typeName(ni_obj->GetType()->name);
			if( typeName  == "TESObjectExtraData" ) {
				_MESSAGE("		EXTRA DATA VALID!!?? %s",ni_obj->GetType()->name);
				if( Actor* ActorFromAnimData = reinterpret_cast<Actor*>(((TESObjectExtraData*)ni_obj)->refr)){
					_MESSAGE("			Got actor! %s",GetFullName(ActorFromAnimData));
				} else {
					_MESSAGE("			Didn't get actor!");
				}
			} else {
				_MESSAGE("		Type name does not match! %s", ni_obj->GetType()->name);
			}
		} else {
			_MESSAGE("	NiObject not found from ActorAnimdata at %i from %p",i,this);
		}
	}
}




void CBAS_ActorAnimData::DumpAnimSequences()
{
	_MESSAGE("		-- DUMPING ANIMSEQUENCES FOR %p", this);
	for (UInt32 idx = 0; idx < 5; idx++)
	{
		BSAnimGroupSequence* anim = animSequences[idx];
		if (anim && anim->filePath)
		{
			_MESSAGE("		- [%i]: %s",idx,anim->filePath);
		}
		else {
			_MESSAGE("		- [%i]: --",idx);
		}
	}
}

*/
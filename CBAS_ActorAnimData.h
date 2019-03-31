#include "NiObjects.h";
#include "NiExtraData.h";

#include "obse/GameObjects.h"

#pragma once

//copied from GameProcess.h/ActorAnimData
class CBAS_ActorAnimData
{
public:
	CBAS_ActorAnimData();
	~CBAS_ActorAnimData();

	UInt32				unk00;			//00
	NiNode				* niNode04;		//04 seen BSFadeNode for 3rd Person, NiNode for 1st
	NiNode				* niNode08;		//08
	UInt32				unk0C[6];		//0C
	NiNode				* niNodes24[5];	//24
	UInt32				unk38[23];		//38	still unknown, was 24 elements long 
	float				t_alive;		//94	in-game time passed (i.e. not menu)
	NiControllerManager	* manager;		//98
	NiTPointerMap<UInt32> * map9C;		//9C NiTPointerMap<AnimSequenceBase>
	BSAnimGroupSequence	* animSequences[5]; //A0
	UInt32				unkB4;			//B4
	UInt32				unkB8;			//B8
	float				unkBC;			//BC
	float				t_period;		//C0 time period of current animation?
	float				unkC4;			//C4
	UInt32				unkC8[3];		//C8
	UInt32				unkD4;			//D4
	void				* unkD8;		//D8 looks like struct with idle anim transform data

	bool FindAnimInRange(UInt32 lowBound, UInt32 highBound = -1);
	bool PathsInclude(const char* subString);

	//CBAS	-- leave commented out as this is not actually a child of ActorAnimData
	//
	//BSFadeNode* GetBSFadeNode() { return reinterpret_cast<BSFadeNode*>(niNode04); }
	//const char* GetNiNodeType() { return niNode04->GetType()->name; }
	//
	//void ScanNiObjs();
	//void DumpAnimSequences();
};
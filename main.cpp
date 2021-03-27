#include "obse/PluginAPI.h"
#include "obse/CommandTable.h"
#include "obse_common/SafeWrite.cpp"
#if OBLIVION
#include "obse/GameAPI.h"

/*	As of 0020, ExtractArgsEx() and ExtractFormatStringArgs() are no longer directly included in plugin builds.
	They are available instead through the OBSEScriptInterface.
	To make it easier to update plugins to account for this, the following can be used.
	It requires that g_scriptInterface is assigned correctly when the plugin is first loaded.
*/
#define ENABLE_EXTRACT_ARGS_MACROS 1	// #define this as 0 if you prefer not to use this

#if ENABLE_EXTRACT_ARGS_MACROS

OBSEScriptInterface * g_scriptInterface = NULL;	// make sure you assign to this
#define ExtractArgsEx(...) g_scriptInterface->ExtractArgsEx(__VA_ARGS__)
#define ExtractFormatStringArgs(...) g_scriptInterface->ExtractFormatStringArgs(__VA_ARGS__)

#endif

#else
#include "obse_editor/EditorAPI.h"
#endif

#include "CBAS_Actor.h"

//#include "obse/ParamInfos.h"
#include "obse/Script.h"
//#include "obse/GameObjects.h"
//#include "obse/NiExtraData.h"
//#include "obse/GameProcess.h"
//#include "obse/GameActorValues.h"
//#include "Utilities.h"
//#include <string>

#define CBAS_VERSION "1.80"

IDebugLog		gLog("Data/OBSE/Plugins/logs/CharacterBasedAttackSpeed.log");

PluginHandle				g_pluginHandle = kPluginHandle_Invalid;
OBSESerializationInterface	* g_serialization = NULL;
OBSEArrayVarInterface		* g_arrayIntfc = NULL;
OBSEScriptInterface			* g_scriptIntfc = NULL;
OBSECommandTableInterface* cmdIntfc = NULL;

//CBAS
static UInt32 AttackSpeedInstruction = 0x477086;
static UInt32 nextInstruction = 0x47708C;


#if OBLIVION

/*************************
	Game code hook with variables
*************************/
CBAS_Actors CBAS_Actors_;
CBAS_Actors* g_cbasActors = &CBAS_Actors_;

void* p_aad;		//pointer to actor anim data

void* ESI_REGISTER;


_declspec(naked) void EmptyHook(void)
{
	_asm {
		fld dword ptr[esi+0xC0]				//load weapon speed onto floating point stack	-- this was the original instruction
		jmp nextInstruction					//jump back to next instruction in base game code
	}
}

_declspec(naked) void AttackSpeedHook(void)
{


	_asm {
		push eax
		push ecx
		push edx
		push esi
		push ah		//store current ah
		lahf		//put current flags into ah
		push ah		//store new ah
			//edx:
			//0 - unsheathing weapon
			//1 - sheathing weapon
			//2 - bow	
			//3 - backswing (chain from normal atk)
			//4 - normal attack
			//5 - neutral power attack
			//6	- forward power attack
			//7 - backwards power attack
			//8 - left power attack
			//9 - right power attack
			// ??? danger zone
		mov p_aad, esi
	}
	
	g_cbasActors->SetAttackSpeed(p_aad);


	_asm {
		pop ah			//restore stored flags into ah
		sahf			//restore flags
		pop ah			//restore ah from before 
		pop esi
		pop edx
		pop ecx
		pop eax
		fld dword ptr[esi+0xC0]				//load weapon speed onto floating point stack	-- this was the original instruction
		jmp nextInstruction					//jump back to next instruction in base game code	
	}
}



#endif


void DoHook() {
	if( !(*CBAS_IniHandler)(CBAS_Config::IniEntries::CBAS_Enabled) ) {
		_MESSAGE("[CharacterBasedAttackSpeed] -- has been set to disabled in the INI file.");
		_MESSAGE("[CharacterBasedAttackSpeed] disabled.");
		//hook nothing
		WriteRelJump (AttackSpeedInstruction, (UInt32)&EmptyHook);
	} else {
		//hook attack speed algorithm on to location where it is
		WriteRelJump (AttackSpeedInstruction, (UInt32)&AttackSpeedHook);
	}
}

/*************************
	Messaging API example
*************************/

#ifdef _DEBUG
OBSEMessagingInterface* g_msg;

void MessageHandler(OBSEMessagingInterface::Message* msg)
{
	switch (msg->type)
	{
	case OBSEMessagingInterface::kMessage_ExitGame:
		_MESSAGE("Plugin Example received ExitGame message");
		break;
	case OBSEMessagingInterface::kMessage_ExitToMainMenu:
		_MESSAGE("Plugin Example received ExitToMainMenu message");
		break;
	case OBSEMessagingInterface::kMessage_PostLoad:
		_MESSAGE("Plugin Example received PostLoad mesage");
		break;
	case OBSEMessagingInterface::kMessage_LoadGame:
	case OBSEMessagingInterface::kMessage_SaveGame:
		_MESSAGE("Plugin Example received save/load message with file path %s", msg->data);
		break;
	case OBSEMessagingInterface::kMessage_Precompile: 
		{
			ScriptBuffer* buffer = (ScriptBuffer*)msg->data;		
			_MESSAGE("Plugin Example received precompile message. Script Text:\n%s", buffer->scriptText);
			break;
		}
	case OBSEMessagingInterface::kMessage_PreLoadGame:
		_MESSAGE("Plugin Example received pre-loadgame message with file path %s", msg->data);
		break;
	case OBSEMessagingInterface::kMessage_ExitGame_Console:
		_MESSAGE("Plugin Example received quit game from console message");
		break;
	default:
		_MESSAGE("Plugin Example received unknown message");
		break;
	}
}

static void CBASMain_LoadCallback(void * reserved){
	g_cbasActors->LoadIniValues();

	time_t rawtime;
	struct tm * timeinfo;

	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	_MESSAGE("[%s]: CBAS Reloaded Config because of Load Game!",strtok(asctime(timeinfo),"\n"));

	DoHook();
}

#endif

extern "C" {

bool OBSEPlugin_Query(const OBSEInterface * obse, PluginInfo * info)
{
	// fill out the info structure
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "CharacterBasedAttackSpeed";
	info->version = 1;

	// version checks
	if(!obse->isEditor)
	{
		if(obse->obseVersion < OBSE_VERSION_INTEGER)
		{
			_ERROR("OBSE version older than expected (got %08X instead of %08X)", obse->obseVersion, OBSE_VERSION_INTEGER);
			//return false; //TODO: UNCOMMENT THIS! -- sort out versions
		}

#if OBLIVION
		if(obse->oblivionVersion != OBLIVION_VERSION)
		{
			_ERROR("incorrect Oblivion version (got %08X need %08X)", obse->oblivionVersion, OBLIVION_VERSION);
			return false;
		}
#endif

		g_serialization = (OBSESerializationInterface *)obse->QueryInterface(kInterface_Serialization);
		if(!g_serialization)
		{
			_ERROR("serialization interface not found");
			return false;
		}

		if(g_serialization->version < OBSESerializationInterface::kVersion)
		{
			_ERROR("incorrect serialization version found (got %08X need %08X)", g_serialization->version, OBSESerializationInterface::kVersion);
			return false;
		}

		g_arrayIntfc = (OBSEArrayVarInterface*)obse->QueryInterface(kInterface_ArrayVar);
		if (!g_arrayIntfc)
		{
			_ERROR("Array interface not found");
			return false;
		}

		g_scriptIntfc = (OBSEScriptInterface*)obse->QueryInterface(kInterface_Script);		
	}
	else
	{
		// no version checks needed for editor
	}

	// version checks pass

	return true;
}

bool OBSEPlugin_Load(const OBSEInterface * obse)
{
	_MESSAGE("Loading... [CharacterBasedAttackSpeed - v%s]",CBAS_VERSION);


	/***************************************************************************
	 *	
	 *	READ THIS!
	 *	
	 *	Before releasing your plugin, you need to request an opcode range from
	 *	the OBSE team and set it in your first SetOpcodeBase call. If you do not
	 *	do this, your plugin will create major compatibility issues with other
	 *	plugins, and may not load in future versions of OBSE. See
	 *	obse_readme.txt for more information.
	 *	
	 **************************************************************************/
	//obse->SetOpcodeBase(0x2000);

	// set up serialization callbacks when running in the runtime
	if(!obse->isEditor)
	{
		//initialises ini object
		g_cbasActors->LoadIniValues();

		DoHook();

		// register to use string var interface
		// this allows plugin commands to support '%z' format specifier in format string arguments
		OBSEStringVarInterface* g_Str = (OBSEStringVarInterface*)obse->QueryInterface(kInterface_StringVar);
		g_Str->Register(g_Str);

		// get an OBSEScriptInterface to use for argument extraction
		g_scriptInterface = (OBSEScriptInterface*)obse->QueryInterface(kInterface_Script);
	}

#ifdef _DEBUG
	obse->SetOpcodeBase(0x2000);
	g_pluginHandle = obse->GetPluginHandle();

	g_serialization->SetLoadCallback(g_pluginHandle, CBASMain_LoadCallback);

	// register to receive messages from OBSE
	OBSEMessagingInterface* msgIntfc = (OBSEMessagingInterface*)obse->QueryInterface(kInterface_Messaging);
	msgIntfc->RegisterListener(g_pluginHandle, "OBSE", MessageHandler);
	g_msg = msgIntfc;

	// get command table, if needed
	cmdIntfc = (OBSECommandTableInterface*)obse->QueryInterface(kInterface_CommandTable);
	if (cmdIntfc) {
#if 0	// enable the following for loads of log output
		for (const CommandInfo* cur = cmdIntfc->Start(); cur != cmdIntfc->End(); ++cur) {
			_MESSAGE("%s",cur->longName);
		}
#endif
	}
	else {
		_MESSAGE("Couldn't read command table");
	}
#endif

	return true;
}

};

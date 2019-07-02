
#include "../stdafx.h"

#include "conutils.hpp"
#include "../interprocess.hpp"
#include "../moddll.hpp"
#include "../SPTLib/sptlib.hpp"
#include "../SPTLib/Hooks.hpp"
#include "../modules/WitcherModule.h"
#include <chrono>

const wchar_t EVENT_NAME[] = L"witcherMod-Injector";

static FILE* logfile = nullptr;

static void Log(const char* prefix, const char* msg)
{
	if (logfile)
	{
		auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		auto ltime = localtime(&time);
		fprintf(logfile, "[%02d:%02d:%02d] [%s] %s", ltime->tm_hour, ltime->tm_min, ltime->tm_sec, prefix, msg);
		fflush(logfile);
	}
}

void PrintMessage(const char* format, ...)
{
	va_list args;
	va_start(args, format);

	char temp[1024];
	vsnprintf(temp, 1024, format, args);

	va_end(args);

	ConUtils::Log(temp);
	Log("Msg", temp);
}

void PrintDevMessage(const char* format, ...)
{
	va_list args;
	va_start(args, format);

	char temp[1024];
	vsnprintf(temp, 1024, format, args);

	va_end(args);

	ConUtils::Log(temp, FOREGROUND_RED | FOREGROUND_GREEN);
	Log("DevMsg", temp);
}

void PrintWarning(const char* format, ...)
{
	va_list args;
	va_start(args, format);

	char temp[1024];
	vsnprintf(temp,1024, format, args);

	va_end(args);

	ConUtils::Log(temp, FOREGROUND_RED | FOREGROUND_INTENSITY);
	Log("Warning", temp);
}

void PrintDevWarning(const char* format, ...)
{
	va_list args;
	va_start(args, format);

	char temp[1024];
	vsnprintf(temp,1024, format, args);

	va_end(args);

	ConUtils::Log(temp, FOREGROUND_RED);
	Log("DevWarning", temp);
}

unsigned int __stdcall MainThread(void* args)
{
	ConUtils::Init();
	ConUtils::Log("WitcherMod version " WITCHERMOD_VERSION ".\n", FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);

	auto bxtLogfile = getenv("BXT_LOGFILE");
	if (bxtLogfile)
	{
		logfile = fopen(bxtLogfile, "a");
		if (!logfile)
			PrintWarning("Could not open the log file (%s): %s.\n", bxtLogfile, strerror(errno));
	}

	_EngineMsg = PrintMessage;
	_EngineDevMsg = PrintDevMessage;
	_EngineWarning = PrintWarning;
	_EngineDevWarning = PrintDevWarning;

	Interprocess::Initialize();

    Hooks::AddToHookedModules(&WitcherModule::GetInstance());
	Hooks::Init(true);


	auto resume_event = OpenEventW(EVENT_MODIFY_STATE, FALSE, EVENT_NAME);
	if (resume_event != NULL) {
		SetEvent(resume_event);
		CloseHandle(resume_event);
		EngineDevMsg("Signaled the injector to resume the process.\n");
	}

	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		_beginthreadex(NULL, 0, MainThread, NULL, 0, NULL);
		break;

	case DLL_PROCESS_DETACH:
		Hooks::Free();
		Interprocess::Shutdown();
		ConUtils::Free();
		if (logfile)
			fclose(logfile);

		break;
	}

	return TRUE;
}

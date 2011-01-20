#include <windows.h>
#include <shlwapi.h>
#include <stdio.h>

#include <string>

#pragma comment(lib, "shlwapi.lib")

typedef int (*DLL_MAIN)(HINSTANCE, HINSTANCE, LPSTR, int);
#ifdef DEBUG
typedef int (*DLL_DEBUG_MAIN)(int, char**);
#endif

std::wstring GetExecutablePath() {
	wchar_t path[MAX_PATH];

	GetModuleFileNameW(NULL, path, MAX_PATH);
	if (!PathRemoveFileSpecW(path))
		return std::wstring();

	std::wstring exe_path(path);
	return exe_path.append(L"\\");
}

#define PATH_BUF 1024
#define SUBDIR L""

bool SetPath(std::wstring* x) {
	SetDllDirectoryW(L"");

	wchar_t path_env[PATH_BUF];
	DWORD res = GetEnvironmentVariableW(L"PATH", path_env, PATH_BUF);
	if (res == 0 || res >= PATH_BUF) {
		return false;
	}

	std::wstring new_path(GetExecutablePath());
	new_path.append(SUBDIR L"\\;");
	new_path.append(path_env);

	SetEnvironmentVariableW(L"PATH", new_path.c_str());

	x->assign(GetExecutablePath());
	x->append(SUBDIR L"\\");

	SetCurrentDirectoryW(x->c_str());
	SetEnvironmentVariableW(L"MUMBLE_PATH", x->c_str());

	x->append(L"mumble1.dll");

	return true;
}

#ifdef DEBUG
int main(int argc, char** argv) {
	std::wstring x;

	if (!SetPath(&x))
		return -3;

	HMODULE dll = LoadLibraryExW(x.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	if (!dll)
		return -1;

	DLL_DEBUG_MAIN entry_point = reinterpret_cast<DLL_DEBUG_MAIN>(GetProcAddress(dll, "main"));
	if (!entry_point)
		return -2;

	int rc = entry_point(argc, argv);

	return rc;
}
#endif  // DEBUG

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE prevInstance, wchar_t* /*cmdArg*/, int cmdShow) {
	std::wstring x;

	if (!SetPath(&x))
		return -3;

	HMODULE dll = LoadLibraryExW(x.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	if (!dll)
		return -1;

	DLL_MAIN entry_point = reinterpret_cast<DLL_MAIN>(GetProcAddress(dll, "MumbleMain"));
	if (!entry_point)
		return -2;

	int rc = entry_point(instance, prevInstance, NULL, cmdShow);

	return rc;
}

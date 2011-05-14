/* Copyright (C) 2005-2011, Thorvald Natvig <thorvald@natvig.com>

   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.
   - Neither the name of the Mumble Developers nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <windows.h>
#include <tlhelp32.h>
#include <dbghelp.h>
#include <emmintrin.h>

#include "Global.h"
#include "Version.h"

extern "C" {
	void __cpuid(int a[4], int b);
	void mumble_speex_init();
};

#define PATH_MAX 1024

static wchar_t wcCrashDumpPath[PATH_MAX];
static FILE *fConsole = NULL;

static wchar_t wcComment[PATH_MAX] = L"";
static MINIDUMP_USER_STREAM musComment;

static int cpuinfo[4];

bool bIsWin7 = false;
bool bIsVistaSP1 = false;

static void mumbleMessageOutput(QtMsgType type, const char *msg) {
	char c;
	switch (type) {
		case QtDebugMsg:
			c='D';
			break;
		case QtWarningMsg:
			c='W';
			break;
		case QtFatalMsg:
			c='F';
			break;
		default:
			c='X';
	}
	fprintf(fConsole, "<%c>%s %s\n", c, qPrintable(QDateTime::currentDateTime().toString(QLatin1String("yyyy-MM-dd hh:mm:ss.zzz"))), msg);
	fflush(fConsole);
	OutputDebugStringA(msg);
	if (type == QtFatalMsg) {
		::MessageBoxA(NULL, msg, "Mumble", MB_OK | MB_ICONERROR);
		exit(0);
	}
}

static LONG WINAPI MumbleUnhandledExceptionFilter(struct _EXCEPTION_POINTERS* ExceptionInfo) {
	MINIDUMP_EXCEPTION_INFORMATION i;
	i.ThreadId = GetCurrentThreadId();
	i.ExceptionPointers = ExceptionInfo;

	MINIDUMP_USER_STREAM_INFORMATION musi;

	musi.UserStreamCount = 1;
	musi.UserStreamArray = &musComment;

	HANDLE hMinidump = CreateFile(wcCrashDumpPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hMinidump != INVALID_HANDLE_VALUE) {
		if (MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hMinidump, static_cast<MINIDUMP_TYPE>(MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithThreadInfo), &i, &musi, NULL)) {
			FlushFileBuffers(hMinidump);
		}
		CloseHandle(hMinidump);
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

BOOL SetHeapOptions() {
	HMODULE hLib = LoadLibrary(L"kernel32.dll");
	if (hLib == NULL)
		return FALSE;

	typedef BOOL (WINAPI *HSI)(HANDLE, HEAP_INFORMATION_CLASS, PVOID, SIZE_T);
	HSI pHsi = (HSI)GetProcAddress(hLib, "HeapSetInformation");
	if (!pHsi) {
		FreeLibrary(hLib);
		return FALSE;
	}

#ifndef HeapEnableTerminationOnCorruption
#define HeapEnableTerminationOnCorruption (HEAP_INFORMATION_CLASS)1
#endif

	BOOL fRet = (pHsi)(NULL, HeapEnableTerminationOnCorruption, NULL, 0) ? TRUE : FALSE;
	if (hLib)
		FreeLibrary(hLib);

	return fRet;
}

void os_init() {
	__cpuid(cpuinfo, 1);

#define SSE2_BIT (26)
	if (!(cpuinfo[3] & (1 << SSE2_BIT))) {
		::MessageBoxA(NULL, "This version of Mumble requires a SSE2 capable processor (Pentium 4 / Athlon 64)", "Mumble", MB_OK | MB_ICONERROR);
		exit(0);
	}

	OSVERSIONINFOEXW ovi;
	memset(&ovi, 0, sizeof(ovi));

	ovi.dwOSVersionInfoSize = sizeof(ovi);
	GetVersionEx(reinterpret_cast<OSVERSIONINFOW *>(&ovi));
	bIsWin7 = (ovi.dwMajorVersion >= 7) || ((ovi.dwMajorVersion == 6) &&(ovi.dwBuildNumber >= 7100));
	bIsVistaSP1 = (ovi.dwMajorVersion >= 7) || ((ovi.dwMajorVersion == 6) &&(ovi.dwBuildNumber >= 6001));

	unsigned int currentControl = 0;
	_controlfp_s(&currentControl, _DN_FLUSH, _MCW_DN);

	SetHeapOptions();
	mumble_speex_init();

#ifdef QT_NO_DEBUG
	QString console = g.qdBasePath.filePath(QLatin1String("Console.txt"));
	fConsole = _wfsopen(console.toStdWString().c_str(), L"a+", _SH_DENYWR);

	if (fConsole)
		qInstallMsgHandler(mumbleMessageOutput);

	QString hash;
	QFile f(qApp->applicationFilePath());
	if (! f.open(QIODevice::ReadOnly)) {
		qWarning("VersionCheck: Failed to open binary");
	} else {
		QByteArray a = f.readAll();
		if (a.size() > 0) {
			QCryptographicHash qch(QCryptographicHash::Sha1);
			qch.addData(a);
			hash = QLatin1String(qch.result().toHex());
		}
	}

	QString comment = QString::fromLatin1("%1\n%2\n%3").arg(QString::fromLatin1(MUMBLE_RELEASE), QString::fromLatin1(MUMTEXT(MUMBLE_VERSION_STRING)), hash);

	wcscpy_s(wcComment, PATH_MAX, comment.toStdWString().c_str());
	musComment.Type = CommentStreamW;
	musComment.Buffer = wcComment;
	musComment.BufferSize = wcslen(wcComment) * sizeof(wchar_t);

	QString dump = g.qdBasePath.filePath(QLatin1String("mumble.dmp"));

	QFileInfo fi(dump);
	QDir::root().mkpath(fi.absolutePath());

	if (wcscpy_s(wcCrashDumpPath, PATH_MAX, dump.toStdWString().c_str()) == 0)
		SetUnhandledExceptionFilter(MumbleUnhandledExceptionFilter);

	// Increase our priority class to live alongside games.
	if (!SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS))
		qWarning("Application: Failed to set priority!");
#endif

	g.qdBasePath.mkpath(QLatin1String("Snapshots"));
	if (bIsWin7)
		SetCurrentProcessExplicitAppUserModelID(L"net.sourceforge.mumble.Mumble");
}


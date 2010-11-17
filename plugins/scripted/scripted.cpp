/* Copyright (C) 2010, Benjamin Jemlich <pcgod@users.sourceforge.net>

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

#include "../mumble_plugin_win32.h"

#include "scripted.h"

#ifdef Q_OS_UNIX
#define __cdecl
typedef WId HWND;
#define DLL_PUBLIC __attribute__((visibility("default")))
#else
#define DLL_PUBLIC __declspec(dllexport)
#endif

namespace {
	QSharedPointer<Scripted> mDlg;
	const std::multimap<std::wstring, unsigned long long>* pids;

	QScriptValue DebugPrint(QScriptContext* context, QScriptEngine* engine) {
		QString result;
		for (int i = 0; i < context->argumentCount(); ++i) {
			if (i > 0)
				result.append(" ");
			result.append(context->argument(i).toString());
		}

		QScriptValue calleeData = context->callee().data();
		QPlainTextEdit* edit = qobject_cast<QPlainTextEdit*>(calleeData.toQObject());
		edit->appendPlainText(result);

		return engine->undefinedValue();
	}

	void __cdecl ods(const char *format, ...) {
		char    buf[4096], *p = buf;
		va_list args;

		va_start(args, format);
		int len = _vsnprintf_s(p, sizeof(buf) - 1, _TRUNCATE, format, args);
		va_end(args);

		if (len <= 0)
			return;

		p += len;

		while (p > buf  &&  isspace(p[-1]))
			*--p = '\0';

		*p++ = '\r';
		*p++ = '\n';
		*p   = '\0';

		OutputDebugStringA(buf);
	}

	int trylock(const std::multimap<std::wstring, unsigned long long>& pids_) {
		QSharedPointer<Scripted> dlg(mDlg);
		if (!dlg) {
			return false;
		}

		pids = &pids_;

		bool res = false;
		if (PositionScript* ps = dlg->GetScript()) {
			res = ps->trylock();
		}

		return res;
	}

	int trylock1() {
		return trylock(std::multimap<std::wstring, unsigned long long int>());
	}

	void unlock() {
		QSharedPointer<Scripted> dlg(mDlg);
		if (!dlg) {
			return;
		}

		if (PositionScript* ps = dlg->GetScript()) {
			ps->unlock();
		}

		dlg->UnloadScript();
	}

	void config(HWND h) {
		Q_UNUSED(h)

		if (!mDlg) {
			mDlg = QSharedPointer<Scripted>(new Scripted());
		}

		mDlg->show();
	}

	int fetch(float* avatar_pos, float* avatar_front, float* avatar_top, float* camera_pos, float* camera_front, float* camera_top, std::string& context, std::wstring& identity) {
		QSharedPointer<Scripted> dlg(mDlg);
		if (!dlg) {
			return false;
		}

		bool res = false;
		if (PositionScript* ps = dlg->GetScript()) {
			res = ps->fetch(avatar_pos, avatar_front, avatar_top, camera_pos, camera_front, camera_top, context, identity);
		}

		return res;
	}

	const std::wstring longdesc() {
		return std::wstring(L"Positional plugin script loader.");
	}

	std::wstring description(L"Positional plugin script loader");
	std::wstring shortname(L"Script loader");

	void about(WId h) {
		QMessageBox::about(QWidget::find(h), QString::fromStdWString(description), QString::fromStdWString(longdesc()));
	}

	MumblePlugin scripted = {
		MUMBLE_PLUGIN_MAGIC,
		description,
		shortname,
		about,
		config,
		trylock1,
		unlock,
		longdesc,
		fetch
	};

	MumblePlugin2 scripted2 = {
		MUMBLE_PLUGIN_MAGIC_2,
		MUMBLE_PLUGIN_VERSION,
		trylock
	};

}  // namespace

extern "C" DLL_PUBLIC MumblePlugin *getMumblePlugin() {
	return &scripted;
}

extern "C" __declspec(dllexport) MumblePlugin2 *getMumblePlugin2() {
	return &scripted2;
}


PositionScript::PositionScript(QString& fileName) : bLoadSucessful(false) {
	QString title = tr("Script Loader");
	QString errorMsg = tr("%1 is not a function");

	QFile scriptFile(fileName);
	if (!scriptFile.open(QIODevice::ReadOnly)) {
		QMessageBox::warning(NULL, title, tr("Couldn't open file %1").arg(fileName), QMessageBox::Ok);
		return;
	}

	QTextStream stream(&scriptFile);
	QString contents = stream.readAll();
	scriptFile.close();

	ed.attachTo(&e);
	ed.action(QScriptEngineDebugger::InterruptAction)->trigger();

	// evalute the script
	QScriptValue r = e.evaluate(contents, fileName);
	if (e.hasUncaughtException()) {
		QMessageBox::warning(NULL, title, e.uncaughtException().toString(), QMessageBox::Ok);
		return;
	}

	// fetch function references
	qsvFetch = r.property("fetch");
	if (!qsvFetch.isFunction()) {
		QMessageBox::warning(NULL, title, errorMsg.arg("fetch"), QMessageBox::Ok);
		return;
	}
	qsvTrylock = r.property("trylock");
	if (!qsvTrylock.isFunction()) {
		QMessageBox::warning(NULL, title, errorMsg.arg("trylock"), QMessageBox::Ok);
		return;
	}
	qsvUnlock = r.property("unlock");
	if (!qsvUnlock.isFunction()) {
		QMessageBox::warning(NULL, title, errorMsg.arg("unlock"), QMessageBox::Ok);
		return;
	}

	// create the this object
	qsvThis = e.newQObject(this);

	// change the print function
	QScriptValue pf = e.newFunction(DebugPrint);
	pf.setData(e.newQObject(mDlg->qpteLog));
	e.globalObject().setProperty("print", pf);

	bLoadSucessful = true;
}

int PositionScript::trylock() {
	QScriptValue r = qsvTrylock.call(qsvThis);

	return r.toBool();
}

int PositionScript::fetch(float* avatar_pos, float* avatar_front, float* avatar_top, float* camera_pos, float* camera_front, float* camera_top, std::string& context, std::wstring& identity) {
	avatar_pos_0    = 0;
	avatar_pos_1    = 0;
	avatar_pos_2    = 0;

	avatar_front_0  = 0;
	avatar_front_1  = 0;
	avatar_front_2  = 0;

	avatar_top_0    = 0;
	avatar_top_1    = 0;
	avatar_top_2    = 0;

	camera_pos_0    = 0;
	camera_pos_1    = 0;
	camera_pos_2    = 0;

	camera_front_0  = 0;
	camera_front_1  = 0;
	camera_front_2  = 0;

	camera_top_0    = 0;
	camera_top_1    = 0;
	camera_top_2    = 0;

	qsContext.clear();
	qsIdentity.clear();

	// call the fetch function
	QScriptValue r = qsvFetch.call(qsvThis);
	bool res = r.toBool();

	avatar_pos[0]   = avatar_pos_0;
	avatar_pos[1]   = avatar_pos_1;
	avatar_pos[2]   = avatar_pos_2;

	avatar_front[0] = avatar_front_0;
	avatar_front[1] = avatar_front_1;
	avatar_front[2] = avatar_front_2;

	avatar_top[0]   = avatar_top_0;
	avatar_top[1]   = avatar_top_1;
	avatar_top[2]   = avatar_top_2;

	camera_pos[0]   = camera_pos_0;
	camera_pos[1]   = camera_pos_1;
	camera_pos[2]   = camera_pos_2;

	camera_front[0] = camera_front_0;
	camera_front[1] = camera_front_1;
	camera_front[2] = camera_front_2;

	camera_top[0]   = camera_top_0;
	camera_top[1]   = camera_top_1;
	camera_top[2]   = camera_top_2;

	context.assign(qsContext.toStdString());
	identity.assign(qsIdentity.toStdWString());

	return res;
}

void PositionScript::unlock() {
	qsvUnlock.call(qsvThis);

	if (hProcess) {
		CloseHandle(hProcess);
		hProcess = NULL;
		pModule = NULL;
		dwPid = 0;
	}
}

bool PositionScript::readBytes(qlonglong base, QString dest, quint32 len) {
	SIZE_T r;
	char* tmp = (char*)malloc(len);

	BOOL ok = ReadProcessMemory(hProcess, (void*)base, tmp, len, &r);
	qsvThis.setProperty(dest, QString(QByteArray(tmp, len)));
	free(tmp);

	return (ok && (r == len));
}

qlonglong PositionScript::readPtr(qlonglong base) {
	SIZE_T r;
	void* tmp = 0;
	ReadProcessMemory(hProcess, (void*)base, &tmp, sizeof(void*), &r);
	return (qlonglong)tmp;
}

bool PositionScript::readFloat(qlonglong base, QString dest) {
	SIZE_T r;
	float tmp;
	BOOL ok = ReadProcessMemory(hProcess, (void*)base, &tmp, sizeof(float), &r);
	qsvThis.setProperty(dest, tmp);
	return (ok && (r == sizeof(float)));
}

qlonglong PositionScript::getModuleAddr(QString modname) {
	return (qlonglong)::getModuleAddr(dwPid, modname.toStdWString().c_str());
}

bool PositionScript::initialize(const QString& procname, const QString& modname) {
	hProcess = NULL;
	pModule = NULL;

	if (! pids->empty()) {
		std::multimap<std::wstring, unsigned long long int>::const_iterator iter = pids->find(procname.toStdWString());

		if (iter != pids->end())
			dwPid = static_cast<DWORD>(iter->second);
		else
			dwPid = 0;
	} else {
		dwPid = getProcess(procname.toStdWString().c_str());
	}

	if (!dwPid)
		return false;

	pModule = ::getModuleAddr(dwPid, !modname.isEmpty() ? modname.toStdWString().c_str() : procname.toStdWString().c_str());
	if (!pModule) {
		dwPid = 0;
		return false;
	}

	hProcess = OpenProcess(PROCESS_VM_READ, false, dwPid);
	if (!hProcess) {
		dwPid = 0;
		pModule = NULL;
		return false;
	}

	return true;
}

Scripted::Scripted(QWidget* p) : QDialog(p), ps(NULL) {
	setupUi(this);
}

PositionScript* Scripted::GetScript() {
	return ps;
}

void Scripted::UnloadScript() {
	delete ps;
	ps = NULL;
}

void Scripted::changeEvent(QEvent* e) {
	QDialog::changeEvent(e);
	switch (e->type()) {
		case QEvent::LanguageChange:
			retranslateUi(this);
			break;
		default:
			break;
	}
}

void Scripted::on_qpbLoad_pressed() {
	QString fileName = QFileDialog::getOpenFileName(this, tr("Select script"));
	if (!fileName.isEmpty()) {
		UnloadScript();

		ps = new PositionScript(fileName);
		if (!ps->bLoadSucessful) {
			UnloadScript();
		}
	}
}

void Scripted::on_qpbUnload_pressed() {
	UnloadScript();
}

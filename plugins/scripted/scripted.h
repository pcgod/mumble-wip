#ifndef SCRIPTED_H
#define SCRIPTED_H

#include <windows.h>

#include <QtCore>
#include <QtGui>
#include <QtScript>
#include <QtScriptTools>

#include "ui_scripted.h"

class PositionScript : QObject {
	Q_OBJECT
	Q_DISABLE_COPY(PositionScript)
	Q_PROPERTY(float avatar_pos_0 READ avatarPos0 WRITE setAvatarPos0)
	Q_PROPERTY(float avatar_pos_1 READ avatarPos1 WRITE setAvatarPos1)
	Q_PROPERTY(float avatar_pos_2 READ avatarPos2 WRITE setAvatarPos2)

	Q_PROPERTY(float avatar_front_0 READ avatarFront0 WRITE setAvatarFront0)
	Q_PROPERTY(float avatar_front_1 READ avatarFront1 WRITE setAvatarFront1)
	Q_PROPERTY(float avatar_front_2 READ avatarFront2 WRITE setAvatarFront2)

	Q_PROPERTY(float avatar_top_0 READ avatarTop0 WRITE setAvatarTop0)
	Q_PROPERTY(float avatar_top_1 READ avatarTop1 WRITE setAvatarTop1)
	Q_PROPERTY(float avatar_top_2 READ avatarTop2 WRITE setAvatarTop2)

	Q_PROPERTY(float camera_pos_0 READ cameraPos0 WRITE setCameraPos0)
	Q_PROPERTY(float camera_pos_1 READ cameraPos1 WRITE setCameraPos1)
	Q_PROPERTY(float camera_pos_2 READ cameraPos2 WRITE setCameraPos2)

	Q_PROPERTY(float camera_front_0 READ cameraFront0 WRITE setCameraFront0)
	Q_PROPERTY(float camera_front_1 READ cameraFront1 WRITE setCameraFront1)
	Q_PROPERTY(float camera_front_2 READ cameraFront2 WRITE setCameraFront2)

	Q_PROPERTY(float camera_top_0 READ cameraTop0 WRITE setCameraTop0)
	Q_PROPERTY(float camera_top_1 READ cameraTop1 WRITE setCameraTop1)
	Q_PROPERTY(float camera_top_2 READ cameraTop2 WRITE setCameraTop2)

	Q_PROPERTY(QString context READ context WRITE setContext)
	Q_PROPERTY(QString identity READ identity WRITE setIdentity)

private:
	QScriptEngine e;
	QScriptEngineDebugger ed;
	QScriptValue qsvFetch;
	QScriptValue qsvTrylock;
	QScriptValue qsvUnlock;
	QScriptValue qsvThis;

	float avatar_pos_0;
	float avatar_pos_1;
	float avatar_pos_2;

	float avatar_front_0;
	float avatar_front_1;
	float avatar_front_2;

	float avatar_top_0;
	float avatar_top_1;
	float avatar_top_2;

	float camera_pos_0;
	float camera_pos_1;
	float camera_pos_2;

	float camera_front_0;
	float camera_front_1;
	float camera_front_2;

	float camera_top_0;
	float camera_top_1;
	float camera_top_2;
	
	QString qsContext;
	QString qsIdentity;

	DWORD dwPid;
	HANDLE hProcess;
	BYTE *pModule;

public:
	PositionScript(QString& fileName);
	int trylock();
	int fetch(float* avatar_pos, float* avatar_front, float* avatar_top, float* camera_pos, float* camera_front, float* camera_top, std::string& context, std::wstring& identity);
	void unlock();

	Q_INVOKABLE bool readBytes(qlonglong base, QString dest, quint32 len);
	Q_INVOKABLE qlonglong readPtr(qlonglong base);
	Q_INVOKABLE bool readFloat(qlonglong base, QString dest);
	Q_INVOKABLE qlonglong getModuleAddr(QString modname);
	Q_INVOKABLE bool initialize(const QString& procname, const QString& modname);

	bool bLoadSucessful;

public slots:
	float avatarPos0() const { return avatar_pos_0; };
	float avatarPos1() const { return avatar_pos_1; };
	float avatarPos2() const { return avatar_pos_2; };
	void setAvatarPos0(float p) { avatar_pos_0 = p; };
	void setAvatarPos1(float p) { avatar_pos_1 = p; };
	void setAvatarPos2(float p) { avatar_pos_2 = p; };

	float avatarFront0() const { return avatar_front_0; };
	float avatarFront1() const { return avatar_front_1; };
	float avatarFront2() const { return avatar_front_2; };
	void setAvatarFront0(float f) { avatar_front_0 = f; };
	void setAvatarFront1(float f) { avatar_front_1 = f; };
	void setAvatarFront2(float f) { avatar_front_2 = f; };

	float avatarTop0() const { return avatar_top_0; };
	float avatarTop1() const { return avatar_top_1; };
	float avatarTop2() const { return avatar_top_2; };
	void setAvatarTop0(float t) { avatar_top_0 = t; };
	void setAvatarTop1(float t) { avatar_top_1 = t; };
	void setAvatarTop2(float t) { avatar_top_2 = t; };

	float cameraPos0() const { return camera_pos_0; };
	float cameraPos1() const { return camera_pos_1; };
	float cameraPos2() const { return camera_pos_2; };
	void setCameraPos0(float p) { camera_pos_0 = p; };
	void setCameraPos1(float p) { camera_pos_1 = p; };
	void setCameraPos2(float p) { camera_pos_2 = p; };

	float cameraFront0() const { return camera_front_0; };
	float cameraFront1() const { return camera_front_1; };
	float cameraFront2() const { return camera_front_2; };
	void setCameraFront0(float f) { camera_front_0 = f; };
	void setCameraFront1(float f) { camera_front_1 = f; };
	void setCameraFront2(float f) { camera_front_2 = f; };

	float cameraTop0() const { return camera_top_0; };
	float cameraTop1() const { return camera_top_1; };
	float cameraTop2() const { return camera_top_2; };
	void setCameraTop0(float t) { camera_top_0 = t; };
	void setCameraTop1(float t) { camera_top_1 = t; };
	void setCameraTop2(float t) { camera_top_2 = t; };

	QString context() const { return qsContext; };
	QString identity() const { return qsIdentity; };
	void setContext(QString c) { qsContext = c; };
	void setIdentity(QString i) { qsIdentity = i; };
};

class Scripted : public QDialog, public Ui::Scripted {
	Q_OBJECT
private:
	PositionScript* ps;

protected:
	void changeEvent(QEvent* e);

public:
	Scripted(QWidget* parent = NULL);
	PositionScript *GetScript();
	void UnloadScript();

public slots:
	void on_qpbLoad_pressed();
	void on_qpbUnload_pressed();
};

#endif  // SCRIPTED_H

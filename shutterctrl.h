#ifndef SHUTTERCTRL_H
#define SHUTTERCTRL_H

#include <QScriptEngine>
#include <QScriptProgram>
#include <QScriptable>

bool openShutterDevice(const QString& dev);
bool closeShutterDevice();
bool setShutterStatus(int line, bool status);
bool setDio(int line, bool status);

inline QScriptValue openShutterDeviceWrapper(QScriptContext *context, QScriptEngine *se)
{
    QString x = context->argument(0).toString();
    return QScriptValue(se, openShutterDevice(x));
}

inline QScriptValue closeShutterDeviceWrapper(QScriptContext *context, QScriptEngine *se)
{
    return QScriptValue(se, closeShutterDevice());
}

inline QScriptValue setShutterStatusWrapper(QScriptContext *context, QScriptEngine *se)
{
   int line = context->argument(0).toInt32();
   bool status = context->argument(1).toBool();
   return QScriptValue(se, setShutterStatus(line, status));
}

inline QScriptValue setDioWrapper(QScriptContext *context, QScriptEngine *se)
{
   int line = context->argument(0).toInt32();
   bool status = context->argument(1).toBool();
   return QScriptValue(se, setDio(line, status));
}


#endif //SHUTTERCTRL_H

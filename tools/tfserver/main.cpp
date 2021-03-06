/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QTextCodec>
#include <QStringList>
#include <TWebApplication>
#include <TApplicationServer>
#include <TSystemGlobal>
#include <stdlib.h>
#include "tsystemglobal.h"
#include "signalhandler.h"
using namespace TreeFrog;

#define CTRL_C_OPTION  "--ctrlc-enable"
#define SOCKET_OPTION  "-s"


static void messageOutput(QtMsgType type, const char *msg)
{
    switch (type) {
    case QtFatalMsg:
    case QtCriticalMsg:
        tSystemError("%s", msg);
        break;
    case QtWarningMsg:
        tSystemWarn("%s", msg);
        break;
    case QtDebugMsg:
        tSystemDebug("%s", msg);
        break;
    default:
        break;
    }
}

#if defined(Q_OS_UNIX)
static void writeFailure(const void *data, int size)
{
    tSystemError("%s", QByteArray((const char *)data, size).data());
}
#endif

static QHash<QString, QString> convertArgs(const QStringList &args)
{
    QHash<QString, QString> hash;
    for (int i = 1; i < args.count(); ++i) {
        if (args.value(i).startsWith('-')) {
            if (args.value(i + 1).startsWith('-')) {
                hash.insert(args.value(i), QString());
            } else {
                hash.insert(args.value(i), args.value(i + 1));
                ++i;
            }
        }
    }
    return hash;
}


int main(int argc, char *argv[])
{
    TWebApplication webapp(argc, argv);
    int ret = -1;
    QString arg;
    TApplicationServer *server = 0;

    // Setup loggers
    tSetupSystemLoggers();
    tSetupLoggers();
    qInstallMsgHandler(messageOutput);

    QHash<QString, QString> args = convertArgs(QCoreApplication::arguments());

#if defined(Q_OS_UNIX)
    webapp.watchUnixSignal(SIGTERM);
    if (!args.contains(CTRL_C_OPTION)) {
        webapp.ignoreUnixSignal(SIGINT);
    }

    // Setup signal handlers for SIGSEGV, SIGILL, SIGFPE, SIGABRT and SIGBUS
    setupFailureWriter(writeFailure);
    setupSignalHandler();

#elif defined(Q_OS_WIN)
    if (!args.contains(CTRL_C_OPTION)) {
        webapp.ignoreConsoleSignal();
    }
#endif

    // Sets the app locale
    QString loc = webapp.appSettings().value("Locale").toString().trimmed();
    if (!loc.isEmpty()) {
        QLocale locale(loc);
        QLocale::setDefault(locale);
        tSystemInfo("Application's default locale: %s", qPrintable(locale.name()));
    }
    
    // Sets codec
    QTextCodec *codec = webapp.codecForInternal();
    QTextCodec::setCodecForTr(codec);
    QTextCodec::setCodecForLocale(codec);
    QTextCodec::setCodecForCStrings(codec);
    tSystemDebug("setCodecForTr: %s", codec->name().data());
    tSystemDebug("setCodecForCStrings: %s", codec->name().data());

    if (!webapp.webRootExists()) {
        tSystemError("No such directory");
        goto finish;
    }
    tSystemDebug("Web Root: %s", qPrintable(webapp.webRootPath()));

    if (!webapp.appSettingsFileExists()) {
        tSystemError("Settings file not found");
        goto finish;
    }
  
    server = new TApplicationServer(&webapp);
    arg = args.value(SOCKET_OPTION);
    if (!arg.isEmpty()) {
        // Sets a listening socket descriptor
        int sd = arg.toInt();
        if (sd > 0) {
            if (server->setSocketDescriptor(sd)) {
                tSystemDebug("Set socket descriptor: %d", sd);
            } else {
                tSystemError("Failed to set socket descriptor: %d", sd);
                goto finish;
            }
        } else {
            tSystemError("Invalid socket descriptor: %d", sd);
            goto finish;
        }
    }
    
    if (!server->open()) {
        tSystemError("Server open failed");
        goto finish;
    }

    ret = webapp.exec();

finish:
    _exit(ret);
    return ret;
}

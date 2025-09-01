#include "LogUtil.h"

#include <QFile>
#include <QMutex>
#include <QTextStream>
#include <QThread>

namespace {
    QFile* g_logFile = nullptr;
    QMutex g_logMutex;

    void messageHandler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg)
    {
        Q_UNUSED(ctx);
        if (!g_logFile || !g_logFile->isOpen()) {
            return; // 回落到默认 handler 的同时避免递归调用
        }
        QMutexLocker locker(&g_logMutex);
        QTextStream ts(g_logFile);
        ts.setCodec("UTF-8");
        const char* t = "[I]";
        switch (type) {
        case QtDebugMsg:    t = "[D]"; break;
        case QtInfoMsg:     t = "[I]"; break;
        case QtWarningMsg:  t = "[W]"; break;
        case QtCriticalMsg: t = "[E]"; break;
        case QtFatalMsg:    t = "[F]"; break;
        }
        ts << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz")
           << ' ' << t << ' ' << msg << '\n';
        ts.flush();
    }
}

namespace TCM {

void installFileLogger(const QString& filePath, bool append)
{
    QMutexLocker locker(&g_logMutex);
    if (g_logFile) {
        return; // 已安装
    }
    g_logFile = new QFile(filePath);
    const QIODevice::OpenMode mode = QIODevice::WriteOnly | QIODevice::Text | (append ? QIODevice::Append : QIODevice::Truncate);
    if (!g_logFile->open(mode)) {
        delete g_logFile; g_logFile = nullptr; return;
    }
    qInstallMessageHandler(messageHandler);
}

void uninstallFileLogger()
{
    QMutexLocker locker(&g_logMutex);
    qInstallMessageHandler(nullptr);
    if (g_logFile) {
        g_logFile->flush();
        g_logFile->close();
        delete g_logFile;
        g_logFile = nullptr;
    }
}

} // namespace TCM

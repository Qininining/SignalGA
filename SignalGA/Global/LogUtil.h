#ifndef GLOBAL_LOGUTIL_H
#define GLOBAL_LOGUTIL_H

#include <QtGlobal>
#include <QDateTime>
#include <QDebug>

// 统一日志前缀：时间 + 级别 + 文件:行 + 函数
#define LOG_PREFIX(levelTag) \
    QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"), levelTag, __FILE__, ":", __LINE__, __func__, "-"

// 使用流式写法：LOGD() << "message" << value;
#define LOGD() (qDebug().noquote()   << LOG_PREFIX("[D]"))
#define LOGI() (qInfo().noquote()    << LOG_PREFIX("[I]"))
#define LOGW() (qWarning().noquote() << LOG_PREFIX("[W]"))
#define LOGE() (qCritical().noquote()<< LOG_PREFIX("[E]"))

namespace TCM {

// 安装文件日志（可选）。若不调用则仅输出到 Qt 默认处理（控制台/调试器）。
// 注意：qInstallMessageHandler 为进程级别全局；仅在主线程初始化一次。
void installFileLogger(const QString& filePath, bool append = true);

// 卸载自定义日志处理器，恢复 Qt 默认
void uninstallFileLogger();

} // namespace TCM

#endif // GLOBAL_LOGUTIL_H

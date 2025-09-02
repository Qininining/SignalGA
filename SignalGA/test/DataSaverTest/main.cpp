#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QDateTime>
#include <QElapsedTimer>

#include "../../Data/DataSaver/DataSaver.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    DataSaver saver;
    QObject::connect(&saver, &DataSaver::errorOccurred, [](const QString& m){ qWarning() << "Error:" << m; });
    QObject::connect(&saver, &DataSaver::fileOpened, [](const QString& p){ qInfo() << "Opened:" << p; });
    QObject::connect(&saver, &DataSaver::fileClosed, [](const QString& p){ qInfo() << "Closed:" << p; });

    // 输出目录
    saver.setBaseDir("test/DataSaverTest/out");

    // 准备 CSV 表头
    saver.ensureCsv("HighRate", "Int", {"ts","v1","v2","v3","v4"});

    // 写入策略：5kHz，持续 15 秒（可调 10–30 秒）；批量缓冲 + 定期 flush
    saver.setAutoFlush(false);
    saver.setBufferLimitBytes(256 * 1024); // 256KB 阈值

    const int sampleRate = 5000;        // 5kHz
    const int durationSec = 15;         // 可改 10~30
    const qint64 totalSamples = static_cast<qint64>(sampleRate) * durationSec;

    QVector<int> row;
    row.reserve(5);

    QElapsedTimer timer;
    timer.start();
    qint64 nextTickNs = 0; // 目标时间（纳秒）

    qint64 written = 0;
    const qint64 flushIntervalNs = 100'000'000; // 每 100ms 主动 flush 一次
    qint64 nextFlushNs = flushIntervalNs;

    for (qint64 i = 0; i < totalSamples; ++i) {
        // 目标节拍：每个样本间隔 200 微秒（5kHz）
        nextTickNs = (i + 1) * 200'000; // 200us = 200,000ns （QElapsedTimer 纳秒精度）

        // 组装示例数据（5 个 int）
        row.clear();
        const int t = static_cast<int>(i);
        row << t << (t & 0xFF) << (t % 1000) << (t % 500) << (t % 7);
        saver.writeInts("HighRate", "Int", row);
        ++written;

        // 定期 flush，避免进程异常退出导致数据未落盘
        const qint64 elapsedNs = timer.nsecsElapsed();
        if (elapsedNs >= nextFlushNs) {
            saver.flush("HighRate", "Int");
            nextFlushNs += flushIntervalNs;
        }

        // 简单忙等对齐节拍（控制台测试可接受；生产建议用生产者/消费者）
        while (timer.nsecsElapsed() < nextTickNs) {
            // 可插入 _mm_pause() 等指令，简化为空循环
        }
    }

    saver.flushAll();
    saver.closeAll();

    const qint64 usedMs = timer.elapsed();
    const double seconds = usedMs / 1000.0;
    const double rate = seconds > 0 ? (written / seconds) : 0.0;
    qInfo() << "Written samples:" << written << ", seconds:" << seconds << ", avg rate:" << rate << "Hz";
    return 0;
}

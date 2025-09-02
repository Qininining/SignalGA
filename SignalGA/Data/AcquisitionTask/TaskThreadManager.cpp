#include "TaskThreadManager.h"

#include <QDebug>
#include <QDateTime>
#include <QMetaObject>
#include <QByteArray>

#include "../DataSaver/DataSaver.h"
#include "../../Drivers/ForceSensor/ForceSensor.h"

TaskThreadManager::TaskThreadManager(QObject* parent)
    : QObject(parent) {
}

TaskThreadManager::~TaskThreadManager() {
    stop();
    teardown();
}

void TaskThreadManager::start() {
    if (m_running) return;

    // 创建 ForceSensor 所在线程
    if (!m_sensorThread) m_sensorThread = new QThread();
    if (!m_forceSensor) {
        m_forceSensor = new ForceSensor(m_portName, m_sensCH1, m_sensCH2);
        m_forceSensor->moveToThread(m_sensorThread);
        // 在线程启动后连接串口
        connect(m_sensorThread, &QThread::started, this, [this]() {
            if (m_forceSensor) m_forceSensor->connect();
        });
        // 线程结束时自动断开
        connect(m_sensorThread, &QThread::finished, this, [this]() {
            if (m_forceSensor) m_forceSensor->disConnect();
        });
        connect(m_forceSensor, &ForceSensor::forceDataReady,
                this, &TaskThreadManager::onForceData);
    }

    // 配置并准备 DataSaver
    if (!m_saver) m_saver = new DataSaver(this);
    m_saver->setBaseDir(m_baseDir);
    if (m_saveEnabled) {
        // 力传感字段表头（如果后续也写入 raw，则可另设一个 group）
        m_saver->ensureCsv(m_kind, m_group, {"ts_us", "channel", "absoluteForce", "relativeForce"});
        m_saver->setAutoFlush(false);
        m_saver->setBufferLimitBytes(256 * 1024);
    }

    // 创建并连接 ForceSensor（内部管理）
    if (!m_forceSensor) {
        m_forceSensor = new ForceSensor(m_portName, m_sensCH1, m_sensCH2);
        // 尝试连接，使用缺省 921600/8N1 参数
        m_forceSensor->connect();
        connect(m_forceSensor, &ForceSensor::forceDataReady,
                this, &TaskThreadManager::onForceData);
    }

    m_running = true;
    m_sensorThread->start();
    emit started();
}

void TaskThreadManager::stop() {
    if (!m_running) return;
    m_running = false;
    // 停止传感器线程
    if (m_sensorThread) {
        m_sensorThread->quit();
        m_sensorThread->wait();
    }

    // 刷新/关闭保存
    if (m_saver && m_saveEnabled) {
        m_saver->flush(m_kind, m_group);
        // 不立即 closeAll，让 teardown 统一处理
    }
}

void TaskThreadManager::teardown() {
    if (m_sensorThread) {
        m_sensorThread->deleteLater();
        m_sensorThread = nullptr;
    }

    if (m_saver) {
        if (m_saveEnabled) {
            m_saver->flush(m_kind, m_group);
            m_saver->close(m_kind, m_group);
        }
        // m_saver 自身作为 this 子对象，无需手动 delete
    }

    if (m_forceSensor) {
        m_forceSensor->deleteLater();
        m_forceSensor = nullptr;
    }
}

void TaskThreadManager::onForceData(int channel, double absoluteForce, double relativeForce, long long timestampUs) {
    if (!m_saveEnabled) return;
    if (!m_saver) return;
    // 高速写入：避免 QStringList 临时对象过多，直接拼接
    QString ts = QString::number(timestampUs);
    QString ch = QString::number(channel);
    QString abs = QString::number(absoluteForce, 'f', 6);
    QString rel = QString::number(relativeForce, 'f', 6);
    m_saver->writeRow(m_kind, m_group, {ts, ch, abs, rel});
}

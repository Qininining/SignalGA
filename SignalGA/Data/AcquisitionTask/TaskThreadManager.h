#pragma once

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QTimer>
#include <QByteArray>
#include <QString>
#include <atomic>

// 前向声明，避免头文件依赖过重
class DataSaver;

// 前向声明，按需与 Drivers 模块交互
class Scanner;
class ForceSensor;

// 任务线程管理：提供启动/停止、状态、与 UI/控制层对接
class TaskThreadManager : public QObject {
    Q_OBJECT
public:
    explicit TaskThreadManager(QObject* parent = nullptr);
    ~TaskThreadManager() override;

    bool isRunning() const { return m_running; }
    void setDataSaveEnabled(bool enabled) { m_saveEnabled = enabled; }
    void setDataSaverBaseDir(const QString& dir) { m_baseDir = dir; }
    void setCsvKindGroup(const QString& kind, const QString& group) { m_kind = kind; m_group = group; }
    // ForceSensor 配置（由本类内部创建，不外部注入）
    void setForceSensorPort(const QString& port) { m_portName = port; }
    void setForceSensorSensitivity(double ch1, double ch2) { m_sensCH1 = ch1; m_sensCH2 = ch2; }

public slots:
    void start();
    void stop();

signals:
    void started();
    void stopped();
    void errorOccurred(const QString& message, int code = 0);

private:
    void teardown();
    Q_SLOT void onForceData(int channel, double absoluteForce, double relativeForce, long long timestampUs);

private:
    bool m_running { false };
    QThread* m_sensorThread { nullptr };

    // DataSaver 集成
    DataSaver* m_saver { nullptr };
    bool m_saveEnabled { true };
    QString m_baseDir { QStringLiteral("Data/Output") };
    QString m_kind { QStringLiteral("Acquisition") };
    QString m_group { QStringLiteral("Raw") };

    // ForceSensor 管理
    ForceSensor* m_forceSensor { nullptr };
    QString m_portName { QStringLiteral("COM1") }; // 默认口名，可通过 setForceSensorPort 配置
    double m_sensCH1 { 1.0 };
    double m_sensCH2 { 1.0 };
};


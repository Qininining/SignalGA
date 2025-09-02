#pragma once

#include <QObject>
#include <QFile>
#include <QHash>
#include <QTextStream>
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QVector>

// DataSaver: 将不同“种类(kind)”与“组(group)”的数据分别保存到对应的 CSV 文件
// 文件命名：<baseDir>/<kind>/<group>.csv
// 每种类一个目录，每组一个 csv。支持写入表头与按行追加。
class DataSaver : public QObject {
    Q_OBJECT
public:
    explicit DataSaver(QObject* parent = nullptr);
    ~DataSaver() override;

    // 设置根目录（默认 Data/Output）
    void setBaseDir(const QString& baseDir);
    QString baseDir() const { return m_baseDir; }

    // 确保对应 kind/group 的 CSV 已打开；若需要则创建目录和文件
    // header 非空时，且文件新建/为空时会写表头
    Q_INVOKABLE bool ensureCsv(const QString& kind, const QString& group, const QStringList& header = {});

    // 写入一行（会自动转义逗号与引号）
    Q_INVOKABLE bool writeRow(const QString& kind, const QString& group, const QStringList& columns);

    // 写入原始文本行（调用者自行保证格式）
    Q_INVOKABLE bool writeRawLine(const QString& kind, const QString& group, const QString& rawLine);

    // 高速写入：整型列，适合 5kHz 采样（避免高开销格式化与逐行刷盘）
    Q_INVOKABLE bool writeInts(const QString& kind, const QString& group, const QVector<int>& columns);
    // 高速写入：浮点列（double），可指定精度，默认 6 位
    Q_INVOKABLE bool writeDoubles(const QString& kind, const QString& group, const QVector<double>& columns, int precision = 6);


    // 刷新控制（默认不自动 flush，按批量阈值写入）
    void setAutoFlush(bool enabled);        // 若开启，将在每次写入后 flush（不建议在高频下启用）
    void setBufferLimitBytes(int bytes);    // 达到阈值时自动 flush（默认 64KB）
    Q_INVOKABLE void flush(const QString& kind, const QString& group);
    Q_INVOKABLE void flushAll();

    // 关闭某个组或全部
    Q_INVOKABLE void close(const QString& kind, const QString& group);
    Q_INVOKABLE void closeAll();

signals:
    void errorOccurred(const QString& message);
    void fileOpened(const QString& path);
    void fileClosed(const QString& path);

private:
    struct CsvFile {
        QFile file;
    QTextStream stream;
        bool wroteHeader { false };
    qsizetype pendingBytes { 0 }; // 自上次 flush 以来已写入的字节数
    CsvFile() : stream(&file) {}
    };

    QString csvPath(const QString& kind, const QString& group) const;
    QString escapeCsv(const QString& field) const;

private:
    QString m_baseDir;
    // key: kind + "|" + group
    QHash<QString, CsvFile*> m_files;

    bool m_autoFlush { false };
    int m_bufferLimitBytes { 64 * 1024 };
};

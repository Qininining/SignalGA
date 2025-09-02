#include "DataSaver.h"

#include <QFileInfo>

static QString makeKey(const QString& kind, const QString& group) {
    return kind + "|" + group;
}

DataSaver::DataSaver(QObject* parent)
    : QObject(parent)
    , m_baseDir("Data/Output") {
}

DataSaver::~DataSaver() {
    closeAll();
}

void DataSaver::setBaseDir(const QString& baseDir) {
    m_baseDir = baseDir;
}

QString DataSaver::csvPath(const QString& kind, const QString& group) const {
    return QString("%1/%2/%3.csv").arg(m_baseDir, kind, group);
}

bool DataSaver::ensureCsv(const QString& kind, const QString& group, const QStringList& header) {
    const QString key = makeKey(kind, group);
    if (m_files.contains(key)) {
        return true;
    }

    const QString path = csvPath(kind, group);
    QFileInfo info(path);
    QDir dir = info.dir();
    if (!dir.exists() && !dir.mkpath(".")) {
        emit errorOccurred(QStringLiteral("无法创建目录: %1").arg(dir.absolutePath()));
        return false;
    }

    CsvFile* csv = new CsvFile();
    csv->file.setFileName(path);
    const bool isNew = !info.exists() || info.size() == 0;
    if (!csv->file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        delete csv;
        emit errorOccurred(QStringLiteral("无法打开文件: %1").arg(path));
        return false;
    }

    m_files.insert(key, csv);
    emit fileOpened(path);

    if (isNew && !header.isEmpty()) {
        // 写表头
        QStringList escaped;
        escaped.reserve(header.size());
        for (const auto& h : header) escaped << escapeCsv(h);
    csv->stream << escaped.join(',') << '\n';
    // 不立即 flush，减少 IO 次数
    csv->pendingBytes += escaped.join(',').size() + 1; // 估计字节数（UTF-16->UTF-8 差异忽略，对阈值控制足够）
        csv->wroteHeader = true;
    }

    return true;
}

QString DataSaver::escapeCsv(const QString& field) const {
    // RFC4180：若包含逗号、引号或换行，用双引号包裹，并将内部引号翻倍。
    bool needQuote = field.contains(',') || field.contains('"') || field.contains('\n') || field.contains('\r');
    QString s = field;
    if (s.contains('"')) s.replace("\"", "\"\"");
    if (needQuote) {
        s.prepend('"');
        s.append('"');
    }
    return s;
}

bool DataSaver::writeRow(const QString& kind, const QString& group, const QStringList& columns) {
    if (!ensureCsv(kind, group)) return false;
    const QString key = makeKey(kind, group);
    CsvFile* csv = m_files.value(key, nullptr);
    if (!csv || !csv->file.isOpen()) {
        emit errorOccurred(QStringLiteral("文件未打开: %1/%2").arg(kind, group));
        return false;
    }

    QStringList escaped;
    escaped.reserve(columns.size());
    for (const auto& c : columns) escaped << escapeCsv(c);

    const QString line = escaped.join(',');
    csv->stream << line << '\n';
    csv->pendingBytes += line.size() + 1;
    if (m_autoFlush || csv->pendingBytes >= m_bufferLimitBytes) {
        csv->stream.flush();
        csv->pendingBytes = 0;
    }
    return true;
}

bool DataSaver::writeRawLine(const QString& kind, const QString& group, const QString& rawLine) {
    if (!ensureCsv(kind, group)) return false;
    const QString key = makeKey(kind, group);
    CsvFile* csv = m_files.value(key, nullptr);
    if (!csv || !csv->file.isOpen()) {
        emit errorOccurred(QStringLiteral("文件未打开: %1/%2").arg(kind, group));
        return false;
    }

    csv->stream << rawLine << '\n';
    csv->pendingBytes += rawLine.size() + 1;
    if (m_autoFlush || csv->pendingBytes >= m_bufferLimitBytes) {
        csv->stream.flush();
        csv->pendingBytes = 0;
    }
    return true;
}

bool DataSaver::writeDoubles(const QString& kind, const QString& group, const QVector<double>& columns, int precision) {
    if (!ensureCsv(kind, group)) return false;
    const QString key = makeKey(kind, group);
    CsvFile* csv = m_files.value(key, nullptr);
    if (!csv || !csv->file.isOpen()) {
        emit errorOccurred(QStringLiteral("文件未打开: %1/%2").arg(kind, group));
        return false;
    }

    QString line;
    line.reserve(columns.size() * (precision + 4));
    for (int i = 0; i < columns.size(); ++i) {
        if (i) line.append(',');
        line.append(QString::number(columns[i], 'f', precision));
    }
    csv->stream << line << '\n';
    csv->pendingBytes += line.size() + 1;
    if (m_autoFlush || csv->pendingBytes >= m_bufferLimitBytes) {
        csv->stream.flush();
        csv->pendingBytes = 0;
    }
    return true;
}



void DataSaver::close(const QString& kind, const QString& group) {
    const QString key = makeKey(kind, group);
    CsvFile* csv = m_files.take(key);
    if (!csv) return;
    const QString p = csv->file.fileName();
    if (csv->file.isOpen()) {
        if (csv->pendingBytes > 0) csv->stream.flush();
        csv->file.close();
    }
    delete csv;
    emit fileClosed(p);
}

void DataSaver::closeAll() {
    const auto keys = m_files.keys();
    for (const auto& k : keys) {
        CsvFile* csv = m_files.take(k);
        if (!csv) continue;
        const QString p = csv->file.fileName();
        if (csv->file.isOpen()) {
            if (csv->pendingBytes > 0) csv->stream.flush();
            csv->file.close();
        }
        delete csv;
        emit fileClosed(p);
    }
}

void DataSaver::setAutoFlush(bool enabled) {
    m_autoFlush = enabled;
}

void DataSaver::setBufferLimitBytes(int bytes) {
    if (bytes < 0) bytes = 0;
    m_bufferLimitBytes = bytes;
}

void DataSaver::flush(const QString& kind, const QString& group) {
    const QString key = makeKey(kind, group);
    CsvFile* csv = m_files.value(key, nullptr);
    if (!csv) return;
    csv->stream.flush();
    csv->pendingBytes = 0;
}

void DataSaver::flushAll() {
    for (auto it = m_files.begin(); it != m_files.end(); ++it) {
        CsvFile* csv = it.value();
        if (!csv) continue;
        csv->stream.flush();
        csv->pendingBytes = 0;
    }
}

bool DataSaver::writeInts(const QString& kind, const QString& group, const QVector<int>& columns) {
    if (!ensureCsv(kind, group)) return false;
    const QString key = makeKey(kind, group);
    CsvFile* csv = m_files.value(key, nullptr);
    if (!csv || !csv->file.isOpen()) {
        emit errorOccurred(QStringLiteral("文件未打开: %1/%2").arg(kind, group));
        return false;
    }

    // 快速拼接，避免 QStringList 临时对象
    QString line;
    line.reserve(columns.size() * 6); // 粗略预估：每个 int 约 6~11 字符，按 6 起步
    for (int i = 0; i < columns.size(); ++i) {
        if (i) line.append(',');
        line.append(QString::number(columns[i]));
    }

    csv->stream << line << '\n';
    csv->pendingBytes += line.size() + 1;
    if (m_autoFlush || csv->pendingBytes >= m_bufferLimitBytes) {
        csv->stream.flush();
        csv->pendingBytes = 0;
    }
    return true;
}

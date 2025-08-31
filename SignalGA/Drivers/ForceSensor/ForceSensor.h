#ifndef FORCESENSOR_H
#define FORCESENSOR_H

#include "SerialCommon.h" // 确保包含 SerialCommon 基类的定义
#include <QByteArray>
#include <QString>
#include <QDebug>

// 串口通信协议相关的常量，用于提高代码可读性和可维护性
namespace ForceSensorConstants {
const int PACKET_SINGLE_CHANNEL_SIZE = 10; // 单通道数据包的完整字节长度，例如 "XXXXXX0b\r\n"
const int PACKET_DUAL_CHANNEL_SIZE = 20;   // 双通道数据包的完整字节长度，例如 "XXXXXX0b\r\nYYYYYY0d\r\n"
const char CHANNEL_ID_1_CHAR = 'b';        // 通道 1 在数据包中的标识符字符
const char CHANNEL_ID_2_CHAR = 'd';        // 通道 2 在数据包中的标识符字符
const QByteArray MESSAGE_TERMINATOR = "\r\n"; // 数据包的结束符 (回车+换行)
const int HEX_VALUE_LENGTH = 6;            // 数据包中十六进制力值部分的长度
}

class ForceSensor : public SerialCommon
{
    Q_OBJECT // 声明为 Qt 对象，支持信号与槽机制

public:
    // 构造函数：初始化传感器，设置串口名称和两个通道的灵敏度
    ForceSensor(const QString &portName, double sensitivityCH1, double sensitivityCH2);
    // 析构函数：负责资源的清理，使用 override 关键字明确表示重写基类虚函数
    ~ForceSensor() override;

    // 连接到串口，使用构造函数中指定的端口名和默认的串口参数（921600, 8N1）
    bool connect();
    // 连接到串口，允许自定义所有串口参数
    bool connect(const QString &portName,
                 qint32 baudRate,
                 QSerialPort::DataBits dataBits,
                 QSerialPort::Parity parity,
                 QSerialPort::StopBits stopBits);
    // 断开串口连接
    bool disConnect();

    // 为指定通道设置零点参考值。
    // num: 如果大于 0，则直接使用此值作为零点；如果小于等于 0，则尝试使用当前通道的力值作为零点。
    // channel: 指定要设置的通道（1 或 2）。
    // 返回 true 表示成功设置，false 表示失败（例如通道无效，或当前力值不适合作为零点）。
    bool setReferenceZero(int num, int channel);

    // 为指定通道设置灵敏度。
    // sensitivity: 灵敏度值，必须大于 0。
    // channel: 指定要设置的通道（1 或 2）。
    // 返回 true 表示成功设置，false 表示灵敏度无效或通道无效。
    bool setSensitivity(double sensitivity, int channel);
    // 获取指定通道的灵敏度。
    // sensitivity: 输出参数，用于存储获取到的灵敏度。
    // channel: 指定要获取的通道（1 或 2）。
    // 返回 true 表示成功获取，false 表示通道无效。
    bool getSensitivity(double &sensitivity, int channel);

    // 读取指定通道的力值。
    // channel: 指定要读取的通道（1 或 2）。
    // isRelative: 如果为 true，则返回相对于零点参考的力值；否则，返回原始（绝对）力值。
    // force: 输出参数，用于存储计算后的力值。
    // 返回 true 表示成功读取，false 表示通道无效。
    bool getForce(int channel, bool isRelative, double &force);

signals:
    // 新增信号：当成功处理并计算出力值时发出。
    // channel: 哪个通道的力值。
    // absoluteForce: 绝对力值。
    // relativeForce: 相对于零点的力值。
    void forceDataReady(int channel, double absoluteForce, double relativeForce);

private slots:
    // 重写 SerialCommon 的 readData 槽函数。当串口有新数据可读时，此槽函数会被触发。
    void readData() override;

private:
    // 内部结构体，用于封装每个力传感器通道的私有数据和状态
    struct ChannelData {
        int referenceZero = 0;              // 该通道的零点参考值
        bool forceReferceFlagSet = false;   // 标志，表示该通道的零点参考是否已设定
        int lastProcessedRawForce = 0;      // 上一次成功处理的原始力值，用于处理负值异常
        int currentRawForce = 0;            // 最新处理的原始力值
        double sensitivity = 0.0;           // 该通道的灵敏度（例如，单位力对应的传感器读数）
    };

    ChannelData channelData_[2]; // 包含两个 ChannelData 实例的数组，分别代表通道 1 和通道 2

    QString portName_;           // 存储串口的名称
    QByteArray buffer_;          // 内部缓冲区，用于累积从串口接收到的不完整数据包

    // 私有辅助函数：处理单个通道的原始力数据。
    // rawForce: 从传感器读取到的原始力值。
    // channelIndex: 对应 channelData_ 数组的索引（0 或 1）。
    // 返回经过零点参考和负值处理后的力值。
    int processRawForceData(int rawForce, int channelIndex);

    // 私有辅助函数：解析一个完整的传感器数据包（10字节或20字节）。
    // packet: 待解析的完整数据包。
    // 如果成功解析并处理了数据包中的力值，返回 true；否则返回 false。
    bool parseAndProcessDataPacket(const QByteArray &packet);

    // 私有辅助函数：处理内部缓冲区中累积的数据。
    // 它会尝试从缓冲区中识别并提取完整的传感器数据包，然后调用 parseAndProcessDataPacket 进行处理。
    void processReceivedBuffer();

    // 私有辅助函数：将十六进制字符串安全转换为整数，并进行错误检查。
    // hexString: 待转换的十六进制字符串。
    // result: 输出参数，存储转换后的整数结果。
    // 成功转换返回 true，失败返回 false。
    bool tryConvertHexToInt(const QByteArray& hexString, int& result);
};

#endif // FORCESENSOR_H

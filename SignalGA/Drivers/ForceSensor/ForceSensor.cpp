#include "ForceSensor.h"
#include <QDebug>
#include <QDateTime> // 尽管在当前优化版本中未直接用于数据处理，但如果将来需要时间戳可保留

// 构造函数实现
ForceSensor::ForceSensor(const QString &portName, double sensitivityCH1, double sensitivityCH2)
    : SerialCommon() // 调用基类 SerialCommon 的构造函数
    , portName_(portName)
{
    // 初始化通道 1 的数据
    channelData_[0].sensitivity = sensitivityCH1;
    channelData_[0].referenceZero = 0;
    channelData_[0].forceReferceFlagSet = false;
    channelData_[0].lastProcessedRawForce = 0;
    channelData_[0].currentRawForce = 0;

    // 初始化通道 2 的数据
    channelData_[1].sensitivity = sensitivityCH2;
    channelData_[1].referenceZero = 0;
    channelData_[1].forceReferceFlagSet = false;
    channelData_[1].lastProcessedRawForce = 0;
    channelData_[1].currentRawForce = 0;
    // 启动高分辨率计时器
    highResTimer_.start();
}

// 析构函数实现
ForceSensor::~ForceSensor()
{
    // 基类的析构函数会负责处理串口的关闭
}

// 重写 SerialCommon 的 readData 槽函数
void ForceSensor::readData()
{
    QByteArray newData = serial->readAll(); // 从串口读取所有可用数据
    buffer_.append(newData);                 // 将新数据追加到内部缓冲区
    // qDebug() << "接收到原始数据 (十六进制): " << newData.toHex(); // 用于调试接收到的原始十六进制数据
    processReceivedBuffer();                 // 处理累积的缓冲区数据
}

// 处理单个通道的原始力数据
int ForceSensor::processRawForceData(int rawForce, int channelIndex)
{
    // 确保通道索引有效
    if (channelIndex < 0 || channelIndex >= 2) {
        qDebug() << "错误: processRawForceData 中通道索引无效:" << channelIndex;
        return rawForce; // 如果通道无效，返回原始力值
    }

    ChannelData& ch = channelData_[channelIndex]; // 获取对应通道的数据引用
    int currentProcessedForce = rawForce;

    // 如果零点参考未设置且当前力值非负，则设置零点参考
    if (!ch.forceReferceFlagSet && currentProcessedForce >= 0) {
        ch.referenceZero = currentProcessedForce;
        ch.forceReferceFlagSet = true;
        qDebug() << "通道" << (channelIndex + 1) << "零点参考设置为:" << ch.referenceZero;
    }

    // 如果当前力值是负数，则使用上次有效的已处理力值
    // 这假定负的原始数据表示传感器读数异常或不稳定
    if (currentProcessedForce < 0) {
        currentProcessedForce = ch.lastProcessedRawForce;
        qDebug() << "通道" << (channelIndex + 1) << "接收到负原始力，使用上次有效值:" << currentProcessedForce;
    }

    ch.lastProcessedRawForce = currentProcessedForce; // 更新上次有效力值
    ch.currentRawForce = currentProcessedForce;       // 更新当前处理后的原始力值

    return currentProcessedForce;
}

// 辅助函数：将十六进制字符串安全转换为整数，并进行错误检查
bool ForceSensor::tryConvertHexToInt(const QByteArray& hexString, int& result) {
    bool ok;
    result = hexString.toLongLong(&ok, 16); // 尝试将十六进制字符串转换为长整型
    if (!ok) {
        qDebug() << "将十六进制字符串 '" << hexString << "' 转换为数字失败。";
    }
    return ok;
}


// 解析一个完整的传感器数据包
bool ForceSensor::parseAndProcessDataPacket(const QByteArray &packet)
{
    int rawForceCH1 = -1, rawForceCH2 = -1; // 存储解析出的原始力值
    char channelID1 = '\0', channelID2 = '\0'; // 存储解析出的通道 ID
    bool success = false; // 标记解析和处理是否成功

    if (packet.size() == ForceSensorConstants::PACKET_SINGLE_CHANNEL_SIZE) {
        // 处理单通道数据包格式: "XXXXXX0C\r\n"
        if (packet[ForceSensorConstants::HEX_VALUE_LENGTH] == '0' && // 检查力值后是否为 '0'
            packet.endsWith(ForceSensorConstants::MESSAGE_TERMINATOR)) // 检查是否以 "\r\n" 结尾
        {
            QByteArray hexValue = packet.mid(0, ForceSensorConstants::HEX_VALUE_LENGTH); // 提取十六进制力值部分
            channelID1 = packet[ForceSensorConstants::HEX_VALUE_LENGTH + 1]; // 提取通道 ID 字符

            if (tryConvertHexToInt(hexValue, rawForceCH1)) { // 尝试将十六进制转换为整数
                if (channelID1 == ForceSensorConstants::CHANNEL_ID_1_CHAR) {
                    processRawForceData(rawForceCH1, 0); // 处理通道 1 的数据
                    // 成功处理后，发射信号
                    double absForce, relForce;
                    getForce(1, false, absForce); // 获取绝对力值
                    getForce(1, true, relForce);  // 获取相对力值
                    // 单通道：为该包生成微秒时间戳
                    const long long tsUs = highResTimer_.isValid() ? highResTimer_.nsecsElapsed() / 1000 : 0;
                    emit forceDataReady(1, absForce, relForce, tsUs);
                    success = true;
                } else if (channelID1 == ForceSensorConstants::CHANNEL_ID_2_CHAR) {
                    processRawForceData(rawForceCH1, 1); // 处理通道 2 的数据
                    // 成功处理后，发射信号
                    double absForce, relForce;
                    getForce(2, false, absForce); // 获取绝对力值
                    getForce(2, true, relForce);  // 获取相对力值
                    // 单通道：为该包生成微秒时间戳
                    const long long tsUs = highResTimer_.isValid() ? highResTimer_.nsecsElapsed() / 1000 : 0;
                    emit forceDataReady(2, absForce, relForce, tsUs);
                    success = true;
                } else {
                    qDebug() << "错误: 单通道数据包中未知通道ID:" << channelID1;
                }
            }
        } else {
            qDebug() << "错误: 无效的单通道数据包格式:" << packet.toHex();
        }
    } else if (packet.size() == ForceSensorConstants::PACKET_DUAL_CHANNEL_SIZE) {
        // 处理双通道数据包格式: "XXXXXX0C1\r\nYYYYYY0C2\r\n"
        // 检查关键位置的固定字符 '0'，以及换行符位置，确保格式正确
        if (packet[ForceSensorConstants::HEX_VALUE_LENGTH] == '0' && // 第一个力值后的 '0'
            packet[ForceSensorConstants::HEX_VALUE_LENGTH + 2] == ForceSensorConstants::MESSAGE_TERMINATOR.at(0) && // 第一个力值后的 '\r'
            packet[ForceSensorConstants::HEX_VALUE_LENGTH + 3] == ForceSensorConstants::MESSAGE_TERMINATOR.at(1) && // 第一个力值后的 '\n'
            // 第二个力值前的 '0' 位于索引 16 (0-based)
            packet[ForceSensorConstants::HEX_VALUE_LENGTH + ForceSensorConstants::MESSAGE_TERMINATOR.size() + 2 + ForceSensorConstants::HEX_VALUE_LENGTH] == '0' &&
            packet.endsWith(ForceSensorConstants::MESSAGE_TERMINATOR)) // 整个包以 "\r\n" 结束
        {
            QByteArray hexValue1 = packet.mid(0, ForceSensorConstants::HEX_VALUE_LENGTH); // 提取第一个十六进制力值
            channelID1 = packet[ForceSensorConstants::HEX_VALUE_LENGTH + 1]; // 提取第一个通道 ID

            // 第二个力值从索引 10 开始 (HEX_VALUE_LENGTH (6) + '0' (1) + 'C1' (1) + '\r' (1) + '\n' (1) = 10 字节)
            QByteArray hexValue2 = packet.mid(10, ForceSensorConstants::HEX_VALUE_LENGTH); // 提取第二个十六进制力值
            channelID2 = packet[10 + ForceSensorConstants::HEX_VALUE_LENGTH + 1]; // 提取第二个通道 ID


            if (tryConvertHexToInt(hexValue1, rawForceCH1) && tryConvertHexToInt(hexValue2, rawForceCH2)) {
                // 根据通道 ID 判断哪个值属于哪个通道
                if (channelID1 == ForceSensorConstants::CHANNEL_ID_1_CHAR && channelID2 == ForceSensorConstants::CHANNEL_ID_2_CHAR) {
                    processRawForceData(rawForceCH1, 0); // 处理通道 1
                    processRawForceData(rawForceCH2, 1); // 处理通道 2
                    success = true;
                    // 为双通道包复用同一微秒时间戳
                    const long long tsUs = highResTimer_.isValid() ? highResTimer_.nsecsElapsed() / 1000 : 0;
                    // 发射两个通道的信号
                    double absForce1, relForce1;
                    getForce(1, false, absForce1);
                    getForce(1, true, relForce1);
                    emit forceDataReady(1, absForce1, relForce1, tsUs);

                    double absForce2, relForce2;
                    getForce(2, false, absForce2);
                    getForce(2, true, relForce2);
                    emit forceDataReady(2, absForce2, relForce2, tsUs);

                } else if (channelID1 == ForceSensorConstants::CHANNEL_ID_2_CHAR && channelID2 == ForceSensorConstants::CHANNEL_ID_1_CHAR) {
                    processRawForceData(rawForceCH2, 0); // 处理通道 1 (因为数据包中顺序反了)
                    processRawForceData(rawForceCH1, 1); // 处理通道 2
                    success = true;
                    // 为双通道包复用同一微秒时间戳
                    const long long tsUs = highResTimer_.isValid() ? highResTimer_.nsecsElapsed() / 1000 : 0;
                    // 发射两个通道的信号 (注意顺序)
                    double absForce1, relForce1;
                    getForce(1, false, absForce1);
                    getForce(1, true, relForce1);
                    emit forceDataReady(1, absForce1, relForce1, tsUs);

                    double absForce2, relForce2;
                    getForce(2, false, absForce2);
                    getForce(2, true, relForce2);
                    emit forceDataReady(2, absForce2, relForce2, tsUs);
                } else {
                    qDebug() << "错误: 双通道数据包中未知或不匹配的通道ID:" << channelID1 << channelID2;
                }
            }
        } else {
            qDebug() << "错误: 无效的双通道数据包格式:" << packet.toHex();
        }
    } else {
        qDebug() << "警告: 未知的数据包大小或格式:" << packet.size() << "字节, 数据:" << packet.toHex();
    }
    return success;
}

// 处理内部缓冲区中累积的数据，提取并处理完整的传感器数据包
void ForceSensor::processReceivedBuffer()
{
    // 循环处理，直到缓冲区中不再包含完整的消息帧
    while (buffer_.size() >= ForceSensorConstants::PACKET_SINGLE_CHANNEL_SIZE) {
        int packetEndIndex = buffer_.indexOf(ForceSensorConstants::MESSAGE_TERMINATOR); // 查找消息结束符

        if (packetEndIndex != -1) {
            // 找到了一个潜在的消息结束符
            int packetSizeWithTerminator = packetEndIndex + ForceSensorConstants::MESSAGE_TERMINATOR.size();

            // 提取潜在的数据包
            QByteArray potentialPacket = buffer_.left(packetSizeWithTerminator);

            // 检查提取出的数据包长度是否符合预期的单通道或双通道包长度
            if (potentialPacket.size() == ForceSensorConstants::PACKET_SINGLE_CHANNEL_SIZE ||
                potentialPacket.size() == ForceSensorConstants::PACKET_DUAL_CHANNEL_SIZE)
            {
                // 如果数据包长度符合预期，尝试解析并处理
                if (parseAndProcessDataPacket(potentialPacket)) {
                    // 成功处理后，从缓冲区中移除该数据包
                    buffer_.remove(0, packetSizeWithTerminator);
                } else {
                    // 数据包长度符合，但内容无效。丢弃此数据包以防止无限循环。
                    qDebug() << "数据包内容无效，丢弃:" << potentialPacket.toHex();
                    buffer_.remove(0, packetSizeWithTerminator);
                    // 考虑到可能的数据错位，这里中断循环并等待更多数据，尝试重新同步
                    break;
                }
            } else if (potentialPacket.size() > ForceSensorConstants::PACKET_DUAL_CHANNEL_SIZE) {
                // 如果数据包比预期的双通道数据包还大，可能包含多个消息或格式严重错误。
                // 此时，简单地丢弃已找到的终止符之前的数据，并尝试重新同步。
                qDebug() << "数据包过大或格式错误，丢弃部分数据以重新同步。长度:" << potentialPacket.size() << "数据:" << potentialPacket.toHex();
                buffer_.remove(0, packetSizeWithTerminator); // 移除这部分可能损坏的数据
                break; // 中断，等待更多数据以重新同步
            } else { // potentialPacket.size() < PACKET_SINGLE_CHANNEL_SIZE
                // 理论上，如果找到了终止符，数据包至少应该有单通道的最小长度。
                // 如果出现这种情况，说明数据流异常或逻辑错误。清空缓冲区以避免问题。
                qDebug() << "不完整的数据以终止符结尾，丢弃。数据:" << potentialPacket.toHex();
                buffer_.clear(); // 清空缓冲区，避免异常数据累积
                break;
            }
        } else {
            // 缓冲区中没有找到完整的消息终止符。等待更多数据。
            // 如果缓冲区在没有终止符的情况下变得过大，可能表示数据流有问题。
            if (buffer_.size() > ForceSensorConstants::PACKET_DUAL_CHANNEL_SIZE * 2) { // 设置一个任意阈值
                qDebug() << "缓冲区在没有终止符的情况下变得过大，可能数据错位。清空缓冲区。";
                buffer_.clear(); // 清空以防止无限增长
            }
            break; // 没有完整的包，等待更多数据
        }
    }
}

// 使用自定义设置连接串口
bool ForceSensor::connect(const QString &portName,
                          qint32 baudRate,
                          QSerialPort::DataBits dataBits,
                          QSerialPort::Parity parity,
                          QSerialPort::StopBits stopBits)
{
    // 调用基类的 open 方法来实际打开串口
    if (!SerialCommon::open(portName, baudRate, dataBits, parity, stopBits)) {
        qDebug() << "力传感器: 无法打开串口。";
        return false;
    }

    // 成功连接后，重置零点参考标志和清除内部数据缓冲区，确保状态干净
    channelData_[0].forceReferceFlagSet = false;
    channelData_[1].forceReferceFlagSet = false;
    buffer_.clear(); // 清空缓冲区
    qDebug() << "力传感器: 成功连接到" << portName;
    return true;
}

// 使用默认设置连接串口 (端口名来自构造函数，波特率/校验位等固定)
bool ForceSensor::connect()
{
    // 默认串口参数: 921600 波特率, 8 数据位, 无校验位, 1 停止位
    if (!SerialCommon::open(portName_, 921600, QSerialPort::Data8, QSerialPort::NoParity, QSerialPort::OneStop)) {
        qDebug() << "力传感器: 无法使用默认设置打开串口。";
        return false;
    }

    // 成功连接后，重置零点参考标志和清除内部数据缓冲区
    channelData_[0].forceReferceFlagSet = false;
    channelData_[1].forceReferceFlagSet = false;
    buffer_.clear(); // 清空缓冲区
    qDebug() << "力传感器: 成功连接到" << portName_;
    return true;
}

// 断开串口连接
bool ForceSensor::disConnect()
{
    if (SerialCommon::close()) { // 调用基类的 close 方法来关闭串口
        // 断开连接后，重置零点参考标志
        channelData_[0].forceReferceFlagSet = false;
        channelData_[1].forceReferceFlagSet = false;
        qDebug() << "力传感器: 已断开连接。";
        return true;
    }
    return false; // 如果串口已经关闭或关闭失败，返回 false
}

// 为指定通道设置零点参考值
bool ForceSensor::setReferenceZero(int num, int channel)
{
    if (channel < 1 || channel > 2) {
        qDebug() << "设置零点参考失败: 通道无效。必须是 1 或 2。";
        return false;
    }

    ChannelData& ch = channelData_[channel - 1]; // 根据通道号获取对应的 ChannelData 实例

    if (num > 0) {
        ch.referenceZero = num;
        ch.forceReferceFlagSet = true;
        qDebug() << "通道" << channel << "零点参考已显式设置为:" << num;
        return true;
    } else {
        // 如果传入的 num 小于等于 0，则尝试使用当前处理后的力值作为零点参考
        if (ch.currentRawForce >= 0) { // 只有当当前处理后的力值非负时才允许设置为零点
            ch.referenceZero = ch.currentRawForce;
            ch.forceReferceFlagSet = true;
            qDebug() << "通道" << channel << "零点参考设置为当前处理的力值:" << ch.currentRawForce;
            return true;
        } else {
            qDebug() << "设置通道" << channel << "零点参考失败: 当前力值非正 (或 0)。";
            return false;
        }
    }
}

// 设置指定通道的灵敏度
bool ForceSensor::setSensitivity(double sensitivity, int channel)
{
    if (sensitivity <= 0) {
        qDebug() << "设置灵敏度失败: 必须大于 0。";
        return false;
    }
    if (channel < 1 || channel > 2) {
        qDebug() << "设置灵敏度失败: 通道无效。必须是 1 或 2。";
        return false;
    }

    channelData_[channel - 1].sensitivity = sensitivity; // 根据通道号设置对应的灵敏度
    qDebug() << "通道" << channel << "灵敏度设置为:" << sensitivity;
    return true;
}

// 获取指定通道的灵敏度
bool ForceSensor::getSensitivity(double &sensitivity, int channel)
{
    if (channel < 1 || channel > 2) {
        qDebug() << "获取灵敏度失败: 通道无效。必须是 1 或 2。";
        sensitivity = 0.0; // 如果通道无效，将输出参数设为默认值
        return false;
    }
    sensitivity = channelData_[channel - 1].sensitivity; // 根据通道号获取对应的灵敏度
    return true;
}


// 读取指定通道的力值 (绝对值或相对值)
bool ForceSensor::getForce(int channel, bool isRelative, double &force)
{
    if (channel < 1 || channel > 2) {
        qDebug() << "获取力值失败: 通道无效。必须是 1 或 2。";
        force = 0.0; // 如果通道无效，将输出参数设为默认值
        return false;
    }

    const ChannelData& ch = channelData_[channel - 1]; // 获取对应通道的常量引用

    if (isRelative) {
        // 计算相对力值: (当前原始力值 - 零点参考) * 灵敏度
        force = static_cast<double>(ch.currentRawForce - ch.referenceZero) * ch.sensitivity;
    } else {
        // 计算绝对力值: 当前原始力值 * 灵敏度
        // 根据 `processRawForceData` 的逻辑，`currentRawForce` 最终会是非负的。
        force = static_cast<double>(ch.currentRawForce) * ch.sensitivity;
    }
    return true;
}

#ifndef SCANNER_H
#define SCANNER_H

#include "SCANControl.h"

class Scanner
{
public:
    explicit Scanner(const char* ID, SCAN_INDEX channelIndex);
    ~Scanner();


public:
    bool connect();
    bool disConnect();

    bool gotoPositionAbsolute(signed int position);
    bool gotoPositionRelative(signed int diff);

    bool getPosition(int &position);

    bool scanMoveAbsolute(unsigned int target, unsigned int scanStep = 1000, unsigned int scanDelay = 1000);
    bool scanMoveRelative(int diff, unsigned int scanStep = 1, unsigned int scanDelay = 100);

    bool getVoltage(unsigned int &voltage);

    bool getMotionInfo();
    bool getsta(SCAN_STATUS &sta);

    bool findSystem();


// private slots:
//     void update();

private:
    SCAN_STATUS error_; ///< 最近一次操作的错误状态。
    const char* deviceID_; ///< 设备ID。
    SCAN_INDEX ntHandle_; ///< 用于与NTControl库交互的句柄。
    SCAN_INDEX channelIndex_; ///< 运动平台对应的通道索引。
    bool isOpen;///< 当前连接状态。
    unsigned int motionSta_;
    int position_;
    unsigned int voltage_;


    // QTimer *updateTimer_;
};

#endif // SCANNER_H

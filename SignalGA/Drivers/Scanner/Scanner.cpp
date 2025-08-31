#include "Scanner.h"
#include <QDebug>

Scanner::Scanner(const char* ID, SCAN_INDEX channelIndex)
    : deviceID_(ID)
    , channelIndex_(channelIndex)
    , isOpen(0)
    , motionSta_(0)
    , position_(0)
    , voltage_(0)
{

}

Scanner::~Scanner()
{

}


bool Scanner::connect()
{
    error_ = SCAN_OpenSystem(&ntHandle_, deviceID_, "sync");
    if (error_){
        qDebug() << "Open SCAN system: error_: " << error_ << "ntHandle_:" << ntHandle_;
    }
    else{
        isOpen = 1;
        qDebug() << "Open SCAN system  successfully!" << "ntHandle:" << ntHandle_ << "channel:" <<channelIndex_;
    }
    return error_;
}


bool Scanner::disConnect()
{
    if(isOpen){
        error_ = SCAN_CloseSystem(ntHandle_);
        if (error_){
            qDebug() << "Close NT system: error_: " << error_ << "ntHandle_:" << ntHandle_;
            isOpen = 0;
        }
        else{
            qDebug() << "Close NT system  successfully!" << "ntHandle_:" << ntHandle_;
            isOpen = 0;
        }
    }
    else
        qDebug() << "Close NT system: error_: " << "Platform not Connected";
    return error_;
}

bool Scanner::findSystem()
{
    char outBuffer[4096];
    unsigned int bufferSize = sizeof(outBuffer);
    SCAN_STATUS result = SCAN_FindSystems("", outBuffer, &bufferSize);
    if(result == SCAN_OK){
        // outBuffer holds the locator strings, separated by '\n'
        // bufferSize holds the number of bytes written to outBuffer
        qDebug() << "findSystem:" << outBuffer;
    }
    else{
        qDebug() << "find 0 System";
    }
    return true;
}


bool Scanner::getVoltage(unsigned int &voltage)
{
    error_ = SCAN_GetVoltageLevel_S(ntHandle_,channelIndex_,&voltage);
    if(error_ != SCAN_OK)
        qDebug() << "Scanner::getVoltage error:" << error_;
    return error_;
}


// ●channelIndex (unsigned 32bit)，  输入 - 通道索引，索引值从0开始。
// ●diff (signed 32bit)，            输入 – 相对目标位置。有效输入范围为-262143...262143。 如果控制器最终得到的绝对扫描目标超过-262143...262143的有效输入范围，则扫描运动将在边界处停止。
// ●scanStep(unsigned 32bit)，       输入 - 扫描步数。将当前位置与目标位置分为scanStep个步数。范围2-20000。
// ●scanDelay(unsigned 32bit)，      输入 - 两步数之间的延时。US级，范围：1-65535。
bool Scanner::scanMoveAbsolute(unsigned int target, unsigned int scanStep, unsigned int scanDelay)
{
    error_ = SCAN_ScanMoveAbsolute_S(ntHandle_, channelIndex_, target, scanStep, scanDelay);
    if(error_ != SCAN_OK)
        qDebug() << "Scanner::ScanMoveAbsolute error_:" << error_;
    return error_;
}

bool Scanner::scanMoveRelative(int diff, unsigned int scanStep, unsigned int scanDelay)
{
    error_ = SCAN_ScanMoveRelative_S(ntHandle_, channelIndex_, diff, scanStep, scanDelay);
    if(error_ != SCAN_OK)
        qDebug() << "Scanner::ScanMoveRelative error_:" << error_;
    return error_;
}

#ifndef GLOBAL_ERRORCODE_H
#define GLOBAL_ERRORCODE_H

#include <type_traits>

namespace TCM {

// 统一错误码定义
enum class ErrorCode : int {
    OK = 0,

    Unknown = 1,
    InvalidArgument = 2,
    OutOfRange = 3,
    Unsupported = 4,

    // I/O & 系统
    IOError = 100,
    PermissionDenied = 101,
    NotFound = 102,
    Timeout = 103,
    Busy = 104,

    // 串口相关
    SerialOpenFailed = 200,
    SerialNotOpen = 201,
    SerialReadError = 202,
    SerialWriteError = 203,

    // 协议/解析
    InvalidPacket = 300,
    ParseError = 301,
    ChecksumError = 302,

    // 内部错误
    InternalError = 900
};

// 可选：将 ErrorCode 转为整型
inline constexpr int toInt(ErrorCode c) noexcept {
    return static_cast<std::underlying_type<ErrorCode>::type>(c);
}

// 文本化（简短英文名，用于日志/what()）
inline const char* toString(ErrorCode c) noexcept {
    switch (c) {
    case ErrorCode::OK: return "OK";
    case ErrorCode::Unknown: return "Unknown";
    case ErrorCode::InvalidArgument: return "InvalidArgument";
    case ErrorCode::OutOfRange: return "OutOfRange";
    case ErrorCode::Unsupported: return "Unsupported";
    case ErrorCode::IOError: return "IOError";
    case ErrorCode::PermissionDenied: return "PermissionDenied";
    case ErrorCode::NotFound: return "NotFound";
    case ErrorCode::Timeout: return "Timeout";
    case ErrorCode::Busy: return "Busy";
    case ErrorCode::SerialOpenFailed: return "SerialOpenFailed";
    case ErrorCode::SerialNotOpen: return "SerialNotOpen";
    case ErrorCode::SerialReadError: return "SerialReadError";
    case ErrorCode::SerialWriteError: return "SerialWriteError";
    case ErrorCode::InvalidPacket: return "InvalidPacket";
    case ErrorCode::ParseError: return "ParseError";
    case ErrorCode::ChecksumError: return "ChecksumError";
    case ErrorCode::InternalError: return "InternalError";
    default: return "(Unknown)";
    }
}

} // namespace TCM

#endif // GLOBAL_ERRORCODE_H

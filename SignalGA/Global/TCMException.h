#ifndef GLOBAL_TCMEXCEPTION_H
#define GLOBAL_TCMEXCEPTION_H

#include <exception>
#include <string>
#include <sstream>
#include <utility>

#include "ErrorCode.h"

namespace TCM {

class TCMException : public std::exception {
public:
    // 借鉴简洁风格的额外构造，均为 noexcept
    TCMException() noexcept = default;

    explicit TCMException(ErrorCode code) noexcept
        : code_(code) {
        buildWhat_();
    }

    TCMException(ErrorCode code, std::string message) noexcept
        : code_(code), message_(std::move(message)) {
        buildWhat_();
    }

    // 带上下文的完整构造
    explicit TCMException(ErrorCode code,
                          std::string message,
                          std::string file,
                          int line,
                          std::string function) noexcept
        : code_(code), message_(std::move(message)), file_(std::move(file)), line_(line), function_(std::move(function)) {
        buildWhat_();
    }

    // 拷贝/移动与赋值
    TCMException(const TCMException&) noexcept = default;
    TCMException(TCMException&&) noexcept = default;
    TCMException& operator=(const TCMException&) noexcept = default;
    TCMException& operator=(TCMException&&) noexcept = default;

    const char* what() const noexcept override { return what_.c_str(); }

    ErrorCode code() const noexcept { return code_; }
    const std::string& message() const noexcept { return message_; }
    const std::string& file() const noexcept { return file_; }
    int line() const noexcept { return line_; }
    const std::string& function() const noexcept { return function_; }

private:
    ErrorCode code_ { ErrorCode::Unknown };
    std::string message_;
    std::string file_;
    int line_ {0};
    std::string function_;
    std::string what_;

    void buildWhat_() noexcept {
        std::ostringstream oss;
        oss << "[" << toString(code_) << "]";
        if (!message_.empty()) {
            oss << ' ' << message_;
        }
        if (!file_.empty()) {
            oss << " @" << file_ << ":" << line_;
        }
        if (!function_.empty()) {
            oss << " (" << function_ << ")";
        }
        what_ = oss.str();
    }
};

// 便捷宏：抛出带上下文的异常
#if !defined(TCM_THROW)
#  define TCM_THROW(code, message) \
    throw ::TCM::TCMException((code), (message), __FILE__, __LINE__, __func__)

// 无消息便捷版
#if !defined(TCM_THROW_CODE)
#  define TCM_THROW_CODE(code) \
    throw ::TCM::TCMException((code), std::string{}, __FILE__, __LINE__, __func__)
#endif
#endif

} // namespace TCM

#endif // GLOBAL_TCMEXCEPTION_H

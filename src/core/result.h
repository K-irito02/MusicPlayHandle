#ifndef RESULT_H
#define RESULT_H

#include <QString>
#include <QVariant>
#include <optional>

/**
 * @brief 结果类型模式实现
 * @tparam T 成功时返回的数据类型
 * @details 用于统一处理成功和错误状态，避免异常处理的复杂性
 */
template<typename T>
class Result
{
public:
    /**
     * @brief 创建成功结果
     * @param value 成功时的值
     * @return 成功结果对象
     */
    static Result success(const T& value) {
        Result result;
        result.m_success = true;
        result.m_value = value;
        return result;
    }
    
    /**
     * @brief 创建成功结果（移动语义）
     * @param value 成功时的值
     * @return 成功结果对象
     */
    static Result success(T&& value) {
        Result result;
        result.m_success = true;
        result.m_value = std::move(value);
        return result;
    }
    
    /**
     * @brief 创建错误结果
     * @param message 错误信息
     * @param code 错误代码（可选）
     * @return 错误结果对象
     */
    static Result error(const QString& message, int code = -1) {
        Result result;
        result.m_success = false;
        result.m_errorMessage = message;
        result.m_errorCode = code;
        return result;
    }
    
    /**
     * @brief 检查是否成功
     * @return 是否成功
     */
    bool isSuccess() const {
        return m_success;
    }
    
    /**
     * @brief 检查是否失败
     * @return 是否失败
     */
    bool isError() const {
        return !m_success;
    }
    
    /**
     * @brief 获取成功时的值
     * @return 值的引用
     * @warning 仅在isSuccess()为true时调用
     */
    const T& value() const {
        Q_ASSERT(m_success && "Attempting to get value from error result");
        return m_value.value();
    }
    
    /**
     * @brief 获取成功时的值（移动语义）
     * @return 值
     * @warning 仅在isSuccess()为true时调用
     */
    T takeValue() {
        Q_ASSERT(m_success && "Attempting to get value from error result");
        return std::move(m_value.value());
    }
    
    /**
     * @brief 获取错误信息
     * @return 错误信息
     */
    QString errorMessage() const {
        return m_errorMessage;
    }
    
    /**
     * @brief 获取错误代码
     * @return 错误代码
     */
    int errorCode() const {
        return m_errorCode;
    }
    
    /**
     * @brief 获取值或默认值
     * @param defaultValue 默认值
     * @return 成功时返回值，失败时返回默认值
     */
    T valueOr(const T& defaultValue) const {
        return m_success ? m_value.value() : defaultValue;
    }
    
    /**
     * @brief 链式操作：如果成功则执行函数
     * @tparam Func 函数类型
     * @param func 要执行的函数
     * @return 新的Result对象
     */
    template<typename Func>
    auto map(Func&& func) -> Result<decltype(func(std::declval<T>()))> {
        using ReturnType = decltype(func(std::declval<T>()));
        if (m_success) {
            try {
                return Result<ReturnType>::success(func(m_value.value()));
            } catch (const std::exception& e) {
                return Result<ReturnType>::error(QString::fromStdString(e.what()));
            }
        }
        return Result<ReturnType>::error(m_errorMessage, m_errorCode);
    }
    
    /**
     * @brief 链式操作：如果成功则执行返回Result的函数
     * @tparam Func 函数类型
     * @param func 要执行的函数
     * @return 新的Result对象
     */
    template<typename Func>
    auto flatMap(Func&& func) -> decltype(func(std::declval<T>())) {
        if (m_success) {
            return func(m_value.value());
        }
        using ReturnType = decltype(func(std::declval<T>()));
        return ReturnType::error(m_errorMessage, m_errorCode);
    }
    
private:
    bool m_success = false;
    std::optional<T> m_value;
    QString m_errorMessage;
    int m_errorCode = -1;
    
    Result() = default;
};

/**
 * @brief void类型的特化版本
 */
template<>
class Result<void>
{
public:
    static Result success() {
        Result result;
        result.m_success = true;
        return result;
    }
    
    static Result error(const QString& message, int code = -1) {
        Result result;
        result.m_success = false;
        result.m_errorMessage = message;
        result.m_errorCode = code;
        return result;
    }
    
    bool isSuccess() const { return m_success; }
    bool isError() const { return !m_success; }
    QString errorMessage() const { return m_errorMessage; }
    int errorCode() const { return m_errorCode; }
    
private:
    bool m_success = false;
    QString m_errorMessage;
    int m_errorCode = -1;
};

// 便利类型别名
using VoidResult = Result<void>;

#endif // RESULT_H
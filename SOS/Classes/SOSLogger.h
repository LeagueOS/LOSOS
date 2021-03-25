#pragma once
#include <string>
#include <memory>

#define CVAR_LOG_LEVEL "SOS_LogLevel"
enum class ESOSLogLevel
{
    None = 0,
    Error,
    Warn,
    Info
};

class SOSLogger
{
public:
    static void Log(const std::string& Message, ESOSLogLevel Level);

private:
    static std::shared_ptr<class CVarManagerWrapper> CvarManager;
    static std::shared_ptr<int> LogLevel;

public:
    static void Init(std::shared_ptr<class CVarManagerWrapper> InCvarManager);
    static ESOSLogLevel GetLogLevel() { return static_cast<ESOSLogLevel>(*LogLevel); }
    static std::string GetLogLevelOptions();
};

// GLOBAL LOGGING //
#define LOG_ERROR(x) SOSLogger::Log(x, ESOSLogLevel::Error);
#define LOG_WARN(x)  SOSLogger::Log(x, ESOSLogLevel::Warn);
#define LOG_INFO(x)  SOSLogger::Log(x, ESOSLogLevel::Info);

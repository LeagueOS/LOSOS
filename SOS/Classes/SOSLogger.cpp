#include "SOSLogger.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"

std::shared_ptr<CVarManagerWrapper> SOSLogger::CvarManager = nullptr;
std::shared_ptr<int> SOSLogger::LogLevel = std::make_shared<int>(0);

void SOSLogger::Init(std::shared_ptr<CVarManagerWrapper> InCvarManager)
{
    SOSLogger::CvarManager = InCvarManager;
    CvarManager->registerCvar(CVAR_LOG_LEVEL, std::to_string((int)ESOSLogLevel::Error), "How much info to print to the console", true).bindTo(LogLevel);
}

void SOSLogger::Log(const std::string& Message, ESOSLogLevel Level)
{
    if(!CvarManager || (int)Level > *LogLevel) { return; }

    switch(Level)
    {
        case ESOSLogLevel::Error: { return CvarManager->log("!!ERROR: " + Message); }
        case ESOSLogLevel::Warn:  { return CvarManager->log("*WARN: "   + Message); }
        case ESOSLogLevel::Info:  { return CvarManager->log("INFO: "    + Message); }
    }
}

std::string SOSLogger::GetLogLevelOptions()
{
    std::string Output;
    
    Output += "No Logging@" + std::to_string((int)ESOSLogLevel::None)  + "&";
    Output += "Errors@"     + std::to_string((int)ESOSLogLevel::Error) + "&";
    Output += "Warnings@"   + std::to_string((int)ESOSLogLevel::Warn)  + "&";
    Output += "All@"        + std::to_string((int)ESOSLogLevel::Info);

    return Output;
}

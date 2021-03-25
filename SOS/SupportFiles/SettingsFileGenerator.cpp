#include "Main/SOS.h"
#include "Classes/SOSLogger.h"
#include <fstream>

#define nl(x) SettingsFile << std::string(x) << '\n'
#define blank SettingsFile << '\n'
#define cv(x) std::string(x)

void SOS::GenerateSettingsFile()
{
    std::ofstream SettingsFile(GlobalGameWrapper->GetBakkesModPath() / "plugins" / "settings" / "SOS.set");

    nl("SOS");
    blank;
    nl("9|Version: " + cv(SOS_VERSION));
    blank;
    nl("1|Enable|" + cv(CVAR_ENABLED));
    nl("7|");
    nl("1|Send Nameplate Info|" + cv(CVAR_USE_NAMEPLATES));
    blank;
    nl("4|Websocket Message Rate (milliseconds)|" + cv(CVAR_MESSAGE_RATE) + "|15|2000");
    nl("6|Log Level|" + cv(CVAR_LOG_LEVEL) + "|" + SOSLogger::GetLogLevelOptions());
    blank;
    nl("9|Original dll code by gboddin, modified and extended by SimpleAOB and CinderBlock");
    

    SettingsFile.close();
    cvarManager->executeCommand("cl_settings_refreshplugins");
}

#include "Main/SOS.h"
#include <fstream>

#define nl(x) SettingsFile << std::string(x) << '\n'
#define blank SettingsFile << '\n'
#define cv(x) std::string(x)

std::string BuildLogLevelOptions();

void SOS::GenerateSettingsFile()
{
    std::ofstream SettingsFile(GlobalGameWrapper->GetBakkesModPath() / "plugins" / "settings" / "SOS.set");

    nl("SOS");
    blank;
    nl("1|Enable|" + cv(CVAR_ENABLED));
    nl("7|");
    nl("1|Send Nameplate Info|" + cv(CVAR_USE_NAMEPLATES));
    blank;
    nl("4|Websocket Message Rate (milliseconds)|" + cv(CVAR_MESSAGE_RATE) + "|15|2000");
    nl("6|Log Level|" + cv(CVAR_LOG_LEVEL) + "|" + BuildLogLevelOptions());
    blank;
    nl("9|Original dll code by gboddin, modified and extended by SimpleAOB and CinderBlock");
    

    SettingsFile.close();
    cvarManager->executeCommand("cl_settings_refreshplugins");
}

std::string BuildLogLevelOptions()
{
    std::string Output = "No Logging@0";

    for(int i = 1; i <= 3; ++i)
    {
        std::string numstring = std::to_string(i);
        Output += "&Level " + numstring + "&" + numstring; 
    }

    return Output;
}

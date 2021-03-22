#include "SOSUtils.h"

float SOSUtils::ToKPH(float RawSpeed)
{
    //Raw speed from game is cm/s
    return (RawSpeed * 0.036f);
}

void SOSUtils::GetNameAndID(PriWrapper PRI, std::string& name, std::string& ID)
{
    //Use this whenever you need a player's name and ID in a JSON object
    if (PRI.IsNull())
    {
        name = "";
        ID = "";
    }
    else 
    {
        name = PRI.GetPlayerName().IsNull() ? "" : PRI.GetPlayerName().ToString();
        ID = name + '_' + std::to_string(PRI.GetSpectatorShortcut());
    }
}

#pragma once
#include "bakkesmod/plugin/bakkesmodplugin.h"

namespace SOSUtils
{
    float ToKPH(float RawSpeed);
    void GetNameAndID(PriWrapper PRI, std::string& name, std::string& ID);
}

#pragma once
#include "bakkesmod/plugin/bakkesmodplugin.h"

class Logger
{
    //@TODO: Move the logging macros into this class as publicly accessible
    //Similar to singleton, but it needs to have a shared_ptr of cvarManager
        //If singleton style, needs to have the ability to be destroyed in onUnload so plugin can reload
};

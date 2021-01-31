#pragma once
#include "bakkesmod/plugin/bakkesmodplugin.h"

extern std::shared_ptr<CVarManagerWrapper> globalCvarManager;

// MACROS //
#define SOS_VERSION "1.6.0-beta.1"

#define SHOULDLOG 0
#if SHOULDLOG
    #define LOGC(x) globalCvarManager->log(x)
#else
    #define LOGC(x)
#endif


// STRUCTS //

struct LastTouchInfo
{
    std::string playerID;
    float speed = 0;
};

struct DummyStatEventContainer
{
    uintptr_t Receiver;
    uintptr_t Victim;
    uintptr_t StatEvent;
};

struct BallHitGoalParams
{
    uintptr_t GoalPointer;
    Vector HitLocation;
};

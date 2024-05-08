#pragma once
#include "bakkesmod/plugin/bakkesmodplugin.h"

extern std::shared_ptr<CVarManagerWrapper> globalCvarManager;

// MACROS //

#define LOSOS_VERSION "2.0.0"

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

struct DebugText
{
    std::string Text;
    LinearColor Color = {0, 255, 0, 255};
};

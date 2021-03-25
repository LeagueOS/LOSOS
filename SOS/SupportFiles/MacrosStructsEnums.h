#pragma once
#include "bakkesmod/plugin/bakkesmodplugin.h"

extern std::shared_ptr<class CVarManagerWrapper> GlobalCvarManager;
extern std::shared_ptr<class GameWrapper> GlobalGameWrapper;

// MACROS -- SOS_LogLevel defined in Logger.h //
#define SOS_VERSION "2.0"

#define CVAR_ENABLED        "SOS_bEnabled"
#define CVAR_USE_NAMEPLATES "SOS_bUseNameplates"
#define CVAR_PORT           "SOS_Port"
#define CVAR_MESSAGE_RATE   "SOS_MessageRate"
#define CVAR_DEBUG_RENDER   "SOS_bDebugRender"


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

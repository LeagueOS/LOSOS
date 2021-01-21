#pragma once
#pragma comment( lib, "PluginSDK.lib" )

#include "Classes/WebsocketManager.h"
#include "Classes/NameplatesManager.h"
#include "Classes/BallSpeedManager.h"
#include "Classes/ClockManager.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "MacrosStructsEnums.h"

// FORWARD DECLARATIONS
namespace json { class JSON; }

// SOS CLASS
class SOS : public BakkesMod::Plugin::BakkesModPlugin
{
public:
    void onLoad() override;
    void onUnload() override;

private:
    // CVARS
    std::shared_ptr<bool>  cvarEnabled;
    std::shared_ptr<bool>  cvarUseBase64;
    std::shared_ptr<int>   cvarPort;
    std::shared_ptr<float> cvarUpdateRate;

    // MANAGERS
    std::shared_ptr<WebsocketManager> Websocket;
    std::shared_ptr<NameplatesManager> Nameplates;
    std::shared_ptr<BallSpeedManager> BallSpeed;
    std::shared_ptr<ClockManager> Clock;

    // ORIGINAL SOS VARIABLES
    bool firstCountdownHit = false;
    bool matchCreated = false;
    bool isCurrentlySpectating = false;
    bool bInGoalReplay = false;

    // GOAL SCORED VARIABLES
    LastTouchInfo lastTouch;
    Vector2F GoalImpactLocation = {0,0}; // top-left (0,0) bottom right (1,1)
    Vector2F GetGoalImpactLocation(BallWrapper ball, void* params);

    // MAIN FUNCTION (GameState.cpp)
    void UpdateGameState(CanvasWrapper canvas);
    void GetGameStateInfo(CanvasWrapper canvas, json::JSON& state);

    // HOOKS (EventHooks.cpp)
    void HookAllEvents();
    void HookViewportTick(CanvasWrapper canvas);
    void HookBallExplode();
    void HookOnHitGoal(BallWrapper ball, void* params);
    void HookInitTeams();
    void HookMatchCreated();
    void HookMatchDestroyed();
    void HookMatchEnded();
    void HookCountdownInit();
    void HookPodiumStart();
    void HookReplayCreated();
    void HookGoalReplayStart();
    void HookGoalReplayEnd();
    void HookStatEvent(ServerWrapper caller, void* params);

    // TIME HOOKS
    void HookOnTimeUpdated();
    void HookOnOvertimeStarted(); // Is this not needed?
    void HookOnPauseChanged();
    void HookCarBallHit(CarWrapper car, void* params);

    // DATA GATHERING FUNCTIONS (GameState.cpp)
    void GetPlayerInfo(json::JSON& state, ServerWrapper server);
    void GetIndividualPlayerInfo(json::JSON& state, PriWrapper pri);
    void GetTeamInfo(json::JSON& state, ServerWrapper server);
    void GetGameTimeInfo(json::JSON& state, ServerWrapper server);
    void GetBallInfo(json::JSON& state, ServerWrapper server);
    void GetCurrentBallSpeed();
    void GetWinnerInfo(json::JSON& state, ServerWrapper server);
    void GetArenaInfo(json::JSON& state);
    void GetCameraInfo(json::JSON& state);
    void GetNameplateInfo(CanvasWrapper canvas);
    void GetLastTouchInfo(CarWrapper car, void* params);
    void GetStatEventInfo(ServerWrapper caller, void* params);
};
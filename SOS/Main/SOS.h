#pragma once
#pragma comment( lib, "PluginSDK.lib" )

#include "Classes/WebsocketManager.h"
#include "Classes/NameplatesManager.h"
#include "Classes/BallSpeedManager.h"
#include "Classes/ClockManager.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "SupportFiles/MacrosStructsEnums.h"
#include "json.hpp"

#include "States/GameState.h"
#include <map>

using json = nlohmann::json;

// SOS CLASS
class SOS : public BakkesMod::Plugin::BakkesModPlugin
{
public:
    void onLoad() override;
    void onUnload() override;

    bool ShouldRun();
    bool IsSafeMode(int CurrentPlaylist, const std::vector<int>& SafePlaylists);

    void GenerateSettingsFile();

private:
    std::string CurrentMatchGuid;

    // CVARS
    std::shared_ptr<bool>  bEnabled;
    std::shared_ptr<int>   Port;
    std::shared_ptr<float> UpdateRate;
    std::shared_ptr<bool>  bDebugRender;

    // MANAGERS
    std::shared_ptr<WebsocketManager> Websocket;
    std::shared_ptr<NameplatesManager> Nameplates;
    std::shared_ptr<BallSpeedManager> BallSpeed;
    std::shared_ptr<ClockManager> Clock;

    // STATES
    std::map<EGameState, GameState*> States;
    EGameState CurrentState;
    EGameState PreviousState;

    // GOAL SCORED VARIABLES
    LastTouchInfo lastTouch;
    Vector2F GoalImpactLocation = {0,0}; // top-left (0,0) bottom right (1,1)
	void GetGameStateInfo(CanvasWrapper canvas, json& state);
    Vector2F GetGoalImpactLocation(BallWrapper ball, void* params);

    // MAIN FUNCTION (GameState.cpp)
    void UpdateGameState(CanvasWrapper canvas);

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
    void HookRoundStarted();
    void HookPodiumStart();
    void HookReplayCreated();
    void HookGoalReplayStart();
    void HookGoalReplayEnd();
    void HookStatEvent(ServerWrapper caller, void* params);
    void HookReplayScoreDataChanged(ActorWrapper caller);

    // TIME HOOKS
    void HookOnTimeUpdated();
    void HookOnOvertimeStarted(); // Is this not needed?
    void HookOnPauseChanged();
    void HookCarBallHit(CarWrapper car, void* params);
    void SetBallHit(bool bHit);

    // DATA GATHERING FUNCTIONS (GameState.cpp)
    void GetPlayerInfo(json& state, ServerWrapper server);
    void GetIndividualPlayerInfo(json& state, PriWrapper pri);
    void GetTeamInfo(json& state, ServerWrapper server);
    void GetGameTimeInfo(json& state, ServerWrapper server);
    void GetBallInfo(json& state, ServerWrapper server);
    void GetCurrentBallSpeed();
    void GetWinnerInfo(json& state, ServerWrapper server);
    void GetArenaInfo(json& state);
    void GetCameraInfo(json& state);
    void GetNameplateInfo(CanvasWrapper canvas);
    void GetLastTouchInfo(CarWrapper car, void* params);
    void GetStatEventInfo(ServerWrapper caller, void* params);

    // DEMO TRACKER
    std::map<std::string, int> DemolitionCountMap;
    void DemoCounterIncrement(std::string playerId);
    int DemoCounterGetCount(std::string playerId);

    // DEBUGGING HELP
    void DebugRender(CanvasWrapper Canvas);
    void DrawTextVector(CanvasWrapper Canvas, Vector2 StartPosition, const std::vector<DebugText>& TextVector);
};
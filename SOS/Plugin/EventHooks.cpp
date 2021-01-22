#include "SOS.h"
#include "SOSUtils.h"
#include "json.hpp"


void SOS::HookAllEvents()
{
    using namespace std::placeholders;

    //UPDATE GAME STATE EVERY TICK
    gameWrapper->RegisterDrawable(std::bind(&SOS::HookViewportTick, this, _1));

    //CLOCK EVENTS
    gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.OnGameTimeUpdated", std::bind(&SOS::HookOnTimeUpdated, this));
    gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.StartOvertime", std::bind(&SOS::HookOnOvertimeStarted, this));
    gameWrapper->HookEvent("Function Engine.WorldInfo.EventPauseChanged", std::bind(&SOS::HookOnPauseChanged, this));
    gameWrapper->HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.EventHitBall", std::bind(&SOS::HookCarBallHit, this, _1, _2));

    //GAME EVENTS
    gameWrapper->HookEventPost("Function TAGame.Team_TA.PostBeginPlay", std::bind(&SOS::HookInitTeams, this));
    gameWrapper->HookEvent("Function TAGame.GameInfo_Replay_TA.InitGame", std::bind(&SOS::HookReplayCreated, this));
    gameWrapper->HookEventPost("Function TAGame.GameEvent_Soccar_TA.Destroyed", std::bind(&SOS::HookMatchDestroyed, this));
    gameWrapper->HookEventPost("Function GameEvent_Soccar_TA.Countdown.BeginState", std::bind(&SOS::HookCountdownInit, this));
    gameWrapper->HookEvent("Function TAGame.Ball_TA.Explode", std::bind(&SOS::HookBallExplode, this));
    gameWrapper->HookEventWithCaller<BallWrapper>("Function TAGame.Ball_TA.OnHitGoal", std::bind(&SOS::HookOnHitGoal, this, _1, _2));
    gameWrapper->HookEventPost("Function GameEvent_Soccar_TA.ReplayPlayback.BeginState", std::bind(&SOS::HookGoalReplayStart, this));
    gameWrapper->HookEventPost("Function GameEvent_Soccar_TA.ReplayPlayback.EndState", std::bind(&SOS::HookGoalReplayEnd, this));
    gameWrapper->HookEventPost("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded", std::bind(&SOS::HookMatchEnded, this));
    gameWrapper->HookEventPost("Function GameEvent_Soccar_TA.PodiumSpotlight.BeginState", std::bind(&SOS::HookPodiumStart, this));
    gameWrapper->HookEventWithCaller<ServerWrapper>("Function TAGame.PRI_TA.ClientNotifyStatTickerMessage", std::bind(&SOS::HookStatEvent, this, _1, _2));
}


// GAME STATE //
void SOS::HookViewportTick(CanvasWrapper canvas)
{
    if(!*cvarEnabled || !SOSUtils::ShouldRun(gameWrapper)) { return; }

    UpdateGameState(canvas);
}


// CLOCK EVENTS //
void SOS::HookOnTimeUpdated()
{
    if(!*cvarEnabled || !SOSUtils::ShouldRun(gameWrapper)) { return; }

    //Unpauses the clock if it's paused and updates its time
    Clock->OnClockUpdated();
}

void SOS::HookOnOvertimeStarted()
{
    if(!*cvarEnabled || !SOSUtils::ShouldRun(gameWrapper)) { return; }
    
    Clock->OnOvertimeStarted();
}

void SOS::HookOnPauseChanged()
{
    if(!*cvarEnabled || !SOSUtils::ShouldRun(gameWrapper)) { return; }

    if(gameWrapper->IsPaused())
    {
        Clock->StopClock();
    }
    else
    {
        Clock->StartClock();
    }
}

void SOS::HookCarBallHit(CarWrapper car, void* params)
{
    GetLastTouchInfo(car, params);

    if(!*cvarEnabled || !SOSUtils::ShouldRun(gameWrapper)) { return; }

    //Unpauses the clock the moment someone hits the ball on kickoff
    if(!Clock->IsClockRunning() && !bInGoalReplay)
    {
        Clock->StartClock();
    }
}


// GAME EVENTS //
void SOS::HookInitTeams()
{
    static int NumTimesCalled = 0;

    //"Function TAGame.Team_TA.PostBeginPlay" is called twice rapidly, once for each team
    // Only initialize lobby on the second hook once both teams are ready

    ++NumTimesCalled;
    if(NumTimesCalled >= 2)
    {
        //Set a delay so that everything can be filled in before trying to initialize
        gameWrapper->SetTimeout([this](GameWrapper* gw)
        {
            if(SOSUtils::ShouldRun(gameWrapper))
            {
                HookMatchCreated();
            }
        }, .05f);
        
        NumTimesCalled = 0;
    }

    //Reset call counter after 2 seconds in case it never got through the >= 2 check
    if(NumTimesCalled != 0)
    {
        gameWrapper->SetTimeout([this](GameWrapper* gw){ NumTimesCalled = 0; }, 2.f);
    }
}

void SOS::HookMatchCreated()
{
    // Called by HookInitTeams //

    LOGC(" -------------- MATCH CREATED -------------- ");

    Clock->ResetClock();
    matchCreated = true;
    DemolitionCountMap.clear();
    Websocket->SendEvent("game:match_created", "game_match_created");
}

void SOS::HookReplayCreated()
{
    LOGC(" -------------- REPLAY CREATED -------------- ");

    Clock->ResetClock();
    matchCreated = true;
    Websocket->SendEvent("game:replay_created", "game_replay_created");
}

void SOS::HookMatchDestroyed()
{
    bInGoalReplay = false;
    matchCreated = false;
    firstCountdownHit = false;
    isCurrentlySpectating = false;
    Clock->ResetClock();
    DemolitionCountMap.clear();

    Websocket->SendEvent("game:match_destroyed", "game_match_destroyed");
}

void SOS::HookCountdownInit()
{
    if (!firstCountdownHit && SOSUtils::ShouldRun(gameWrapper))
    {
        firstCountdownHit = true;
        Websocket->SendEvent("game:initialized", "initialized");
    }

    Websocket->SendEvent("game:pre_countdown_begin", "pre_game_countdown_begin");
    Websocket->SendEvent("game:post_countdown_begin", "post_game_countdown_begin");
}

void SOS::HookBallExplode()
{
    if (!*cvarEnabled || !matchCreated) { return; }

    BallSpeed->LockBallSpeed();
    Clock->StopClock();
    
    //Notify that the goal replay will end soon
    if(bInGoalReplay)
    {
        LOGC("Sending ReplayWillEnd Event");
        Websocket->SendEvent("game:replay_will_end", "game_replay_will_end");
    }
}

void SOS::HookOnHitGoal(BallWrapper ball, void* params)
{
    //Lock the current ball speed into the cache
    BallSpeed->LockBallSpeed();

    GoalImpactLocation = GetGoalImpactLocation(ball, params);
}

void SOS::HookGoalReplayStart()
{
    bInGoalReplay = true;
    Websocket->SendEvent("game:replay_start", "game_replay_start");
}

void SOS::HookGoalReplayEnd()
{
    bInGoalReplay = false;
    Websocket->SendEvent("game:replay_end", "game_replay_end");
}

void SOS::HookMatchEnded()
{
    bInGoalReplay = false;
    matchCreated = false;
    firstCountdownHit = false;
    isCurrentlySpectating = false;
    Clock->ResetClock();

    json::JSON winnerData;
    winnerData["winner_team_num"] = NULL;

    ServerWrapper server = SOSUtils::GetCurrentGameState(gameWrapper);
    if (!server.IsNull())
    {
        TeamWrapper winner = server.GetMatchWinner();
        if (!winner.IsNull())
        {
            winnerData["winner_team_num"] = winner.GetTeamNum();
        }
    }

    Websocket->SendEvent("game:match_ended", winnerData);
}

void SOS::HookPodiumStart()
{
    Websocket->SendEvent("game:podium_start", "game_podium_start");
}

void SOS::HookStatEvent(ServerWrapper caller, void* params)
{
    GetStatEventInfo(caller, params);
}
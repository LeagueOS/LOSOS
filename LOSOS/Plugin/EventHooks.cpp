#include "LOSOS.h"
#include "LOSOSUtils.h"
#include "json.hpp"

void LOSOS::HookAllEvents()
{
    using namespace std::placeholders;

    //UPDATE GAME STATE EVERY TICK
    gameWrapper->RegisterDrawable(std::bind(&LOSOS::HookViewportTick, this, _1));

    //CLOCK EVENTS
    gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.OnGameTimeUpdated", std::bind(&LOSOS::HookOnTimeUpdated, this));
    gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.OnOvertimeUpdated", std::bind(&LOSOS::HookOnOvertimeStarted, this));
    gameWrapper->HookEvent("Function Engine.WorldInfo.EventPauseChanged", std::bind(&LOSOS::HookOnPauseChanged, this));
    gameWrapper->HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.EventHitBall", std::bind(&LOSOS::HookCarBallHit, this, _1, _2));
   
    //GAME EVENTS
    gameWrapper->HookEventPost("Function TAGame.Team_TA.PostBeginPlay", std::bind(&LOSOS::HookInitTeams, this));
    gameWrapper->HookEvent("Function TAGame.GameInfo_Replay_TA.InitGame", std::bind(&LOSOS::HookReplayCreated, this));
    gameWrapper->HookEventPost("Function TAGame.GameEvent_Soccar_TA.Destroyed", std::bind(&LOSOS::HookMatchDestroyed, this));
    gameWrapper->HookEventPost("Function GameEvent_Soccar_TA.Countdown.BeginState", std::bind(&LOSOS::HookCountdownInit, this));
    gameWrapper->HookEvent("Function GameEvent_Soccar_TA.Active.StartRound", std::bind(&LOSOS::HookRoundStarted, this));
    gameWrapper->HookEvent("Function TAGame.Ball_TA.Explode", std::bind(&LOSOS::HookBallExplode, this));
    gameWrapper->HookEventWithCaller<BallWrapper>("Function TAGame.Ball_TA.OnHitGoal", std::bind(&LOSOS::HookOnHitGoal, this, _1, _2));
    gameWrapper->HookEventPost("Function GameEvent_Soccar_TA.ReplayPlayback.BeginState", std::bind(&LOSOS::HookGoalReplayStart, this));
    gameWrapper->HookEventPost("Function GameEvent_Soccar_TA.ReplayPlayback.EndState", std::bind(&LOSOS::HookGoalReplayEnd, this));
    gameWrapper->HookEventPost("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded", std::bind(&LOSOS::HookMatchEnded, this));
    gameWrapper->HookEventPost("Function GameEvent_Soccar_TA.PodiumSpotlight.BeginState", std::bind(&LOSOS::HookPodiumStart, this));
    gameWrapper->HookEventPost("Function GameEvent_Soccar_TA.PodiumSpotlight.EndState", std::bind(&LOSOS::HookPodiumEnd, this));
    gameWrapper->HookEventWithCaller<ServerWrapper>("Function TAGame.PRI_TA.ClientNotifyStatTickerMessage", std::bind(&LOSOS::HookStatEvent, this, _1, _2));
    gameWrapper->HookEventWithCallerPost<ActorWrapper>("Function TAGame.ReplayDirector_TA.OnScoreDataChanged", std::bind(&LOSOS::HookReplayScoreDataChanged, this, _1));

    //SPECTATING EVENTS - used to auto hide Replay UI items
    gameWrapper->HookEventPost("Function TAGame.GFxData_ReplayViewer_TA.InitCameraModes", std::bind(&LOSOS::HookHUDVis, this));
}

void LOSOS::HookHUDVis()
{
    if (!*cvarAutoHideGUI) { return; }

    cvarManager->log("Setting HUD and Matchinfo to hidden");

    // Thanks NotMartinn for adding these years ago... :-)
    cvarManager->executeCommand("replay_gui hud 0");
    cvarManager->executeCommand("replay_gui matchinfo 0");
    if (*cvarNameplates)
    {
        cvarManager->executeCommand("replay_gui names 0");
    }
}

// GAME STATE //
void LOSOS::HookViewportTick(CanvasWrapper canvas)
{
    if(!*cvarEnabled || !LOSOSUtils::ShouldRun(gameWrapper)) { return; }

    UpdateGameState(canvas);
    DebugRender(canvas);
}


// CLOCK EVENTS //
void LOSOS::HookOnTimeUpdated()
{
    if(!*cvarEnabled || !LOSOSUtils::ShouldRun(gameWrapper)) { return; }

    //Limit clock updating to only happen within the bounds of normal gameplay
    if(!matchCreated || bInGoalReplay || bInPreReplayLimbo || gameWrapper->IsPaused()) { return; }

    //Unpauses the clock if it's paused and updates its time
    Clock->OnClockUpdated();
}

void LOSOS::HookOnOvertimeStarted()
{
    if(!*cvarEnabled || !LOSOSUtils::ShouldRun(gameWrapper)) { return; }
    
    Clock->OnOvertimeStarted();
}

void LOSOS::HookOnPauseChanged()
{
    if(!*cvarEnabled || !LOSOSUtils::ShouldRun(gameWrapper)) { return; }

    if(gameWrapper->IsPaused())
    {
        Clock->StopClock();
    }
    else
    {
        if(bPendingRestartFromKickoff)
        {
            //Admin uses "restart from kickoff"
            //Don't start clock now. Let HookRoundStart do that
            bPendingRestartFromKickoff = false;
        }
        else
        {
            //Admin doesn't use "restart from kickoff"
            //PauseChanged automatically fires after the 3 second unpause countdown, no extra delay needed
            Clock->StartClock(false);
        }
    }
}

void LOSOS::HookCarBallHit(CarWrapper car, void* params)
{
    GetLastTouchInfo(car, params);

    if(!*cvarEnabled || !LOSOSUtils::ShouldRun(gameWrapper)) { return; }

    SetBallHit(true);
}

void LOSOS::SetBallHit(bool bHit)
{
    //Sets bBallHasBeenHit to true and starts clock if it needs to be started (i.e. kickoff)
    //Only run this part if the ball has not been hit yet
    if(bHit && !bBallHasBeenHit)
    {
        if(!Clock->IsClockRunning() && !bInGoalReplay)
        {
            Clock->StartClock(true);
        }
    }
    
    bBallHasBeenHit = bHit;
}


// GAME EVENTS //
void LOSOS::HookInitTeams()
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
            if(LOSOSUtils::ShouldRun(gameWrapper))
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

void LOSOS::HookMatchCreated()
{
    // Called by HookInitTeams //

    LOGC(" -------------- MATCH CREATED -------------- ");

    ServerWrapper server = LOSOSUtils::GetCurrentGameState(gameWrapper);
    CurrentMatchGuid = server.GetMatchGUID();
    Clock->UpdateCurrentMatchGuid(CurrentMatchGuid);

    Clock->ResetClock();
    matchCreated = true;
    DemolitionCountMap.clear();

    // Team names can only be changed after the teams have been initialized (HookInitTeams)
    Websocket->AssignTeamNames();

    json event;
    event["match_guid"] = CurrentMatchGuid;
    Websocket->SendEvent("game:match_created", event);
}

void LOSOS::HookReplayCreated()
{
    LOGC(" -------------- REPLAY CREATED -------------- ");

    Clock->ResetClock();
    matchCreated = true;

    json event;
    event["match_guid"] = CurrentMatchGuid;
    Websocket->SendEvent("game:replay_created", event);
}

void LOSOS::HookMatchDestroyed()
{
    bInGoalReplay = false;
    bInPreReplayLimbo = false;
    matchCreated = false;
    firstCountdownHit = false;
    isCurrentlySpectating = false;
    bPendingRestartFromKickoff = false;
    Clock->ResetClock();
    DemolitionCountMap.clear();

    json event;
    event["match_guid"] = CurrentMatchGuid;
    Websocket->SendEvent("game:match_destroyed", event);
}

void LOSOS::HookCountdownInit()
{
    //When match admin resets from kickoff, the new countdown starts but it starts paused
    //It will continue when admin unpauses
    if(gameWrapper->IsPaused())
    {
        bPendingRestartFromKickoff = true;
    }
	
    json event;
    event["match_guid"] = CurrentMatchGuid;

    if (!firstCountdownHit && LOSOSUtils::ShouldRun(gameWrapper))
    {
        firstCountdownHit = true;
        Websocket->SendEvent("game:initialized", event);
    }

    Websocket->SendEvent("game:pre_countdown_begin", event);
    Websocket->SendEvent("game:post_countdown_begin", event);
}

void LOSOS::HookRoundStarted()
{
    bPendingRestartFromKickoff = false;
    bInGoalReplay = false;

    if(!*cvarEnabled || !LOSOSUtils::ShouldRun(gameWrapper)) { return; }
    
    //Mark the ball as unhit for the kickoff
    SetBallHit(false);

    //Set the delay for the timer to start if the ball hasn't been hit yet
    //Default delay in game is 5 seconds
    gameWrapper->SetTimeout(std::bind(&LOSOS::SetBallHit, this, true), 5.f);

    json event;
    event["match_guid"] = CurrentMatchGuid;
    Websocket->SendEvent("game:round_started_go", "game_round_started_go");
}

void LOSOS::HookBallExplode()
{
    BallSpeed->LockBallSpeed();
    Clock->StopClock();

    if (!*cvarEnabled || !matchCreated) { return; }
    
    //Notify that the goal replay will end soon
    if(bInGoalReplay)
    {
        LOGC("Sending ReplayWillEnd Event");

        json event;
        event["match_guid"] = CurrentMatchGuid;
        Websocket->SendEvent("game:replay_will_end", event);
    }
    else
    {
        bInPreReplayLimbo = true;
    }
}

void LOSOS::HookOnHitGoal(BallWrapper ball, void* params)
{
    //Lock the current ball speed into the cache
    BallSpeed->LockBallSpeed();
    Clock->StopClock();

    GoalImpactLocation = GetGoalImpactLocation(ball, params);
}

void LOSOS::HookGoalReplayStart()
{
    Clock->StopClock();
    bInGoalReplay = true;
    bInPreReplayLimbo = false;
    Websocket->SendEvent("game:replay_start", "game_replay_start");

    json event;
    event["match_guid"] = CurrentMatchGuid;
    Websocket->SendEvent("game:replay_start", event);
}

void LOSOS::HookGoalReplayEnd()
{
    bInGoalReplay = false;

    json event;
    event["match_guid"] = CurrentMatchGuid;
    Websocket->SendEvent("game:replay_end", event);
}

void LOSOS::HookMatchEnded()
{
    bInGoalReplay = false;
    bInPreReplayLimbo = false;
    matchCreated = false;
    firstCountdownHit = false;
    isCurrentlySpectating = false;
    bPendingRestartFromKickoff = false;
    Clock->ResetClock();

    json winnerData;
    winnerData["match_guid"] = CurrentMatchGuid;
    winnerData["winner_team_num"] = NULL;

    ServerWrapper server = LOSOSUtils::GetCurrentGameState(gameWrapper);
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

void LOSOS::HookPodiumStart()
{
    json event;
    event["match_guid"] = CurrentMatchGuid;
    Websocket->SendEvent("game:podium_start", event);
}

void LOSOS::HookPodiumEnd()
{
    json event;
    event["match_guid"] = CurrentMatchGuid;
    Websocket->SendEvent("game:podium_end", event);
}

void LOSOS::HookStatEvent(ServerWrapper caller, void* params)
{
    GetStatEventInfo(caller, params);
}

void LOSOS::HookReplayScoreDataChanged(ActorWrapper caller)
{
    ReplayDirectorWrapper RDW(caller.memory_address);
    ReplayScoreData ScoreData = RDW.GetReplayScoreData();

    //If ScoredBy is null, that likely means this call was just to reset values
    if(ScoreData.ScoredBy == 0) { return; }

    PriWrapper ScoredBy(ScoreData.ScoredBy);
    std::string ScorerName, ScorerID;
    LOSOSUtils::GetNameAndID(ScoredBy, ScorerName, ScorerID);

    PriWrapper AssistedBy(ScoreData.AssistedBy);
    std::string AssisterName, AssisterID;
    LOSOSUtils::GetNameAndID(AssistedBy, AssisterName, AssisterID);

    json goalScoreData;
    goalScoreData["goalspeed"] = LOSOSUtils::ToKPH(ScoreData.Speed);
    goalScoreData["goaltime"] = ScoreData.Time;
    goalScoreData["impact_location"]["X"] = GoalImpactLocation.X; // Set in HookOnHitGoal
    goalScoreData["impact_location"]["Y"] = GoalImpactLocation.Y; // Set in HookOnHitGoal
    goalScoreData["scorer"]["name"] = ScorerName;
    goalScoreData["scorer"]["id"] = ScorerID;
    goalScoreData["scorer"]["teamnum"] = ScoreData.ScoreTeam;
    goalScoreData["assister"]["name"] = AssisterName;
    goalScoreData["assister"]["id"] = AssisterID;
    goalScoreData["ball_last_touch"]["player"] = lastTouch.playerID; // Set in HookCarBallHit
    goalScoreData["ball_last_touch"]["speed"] = lastTouch.speed;     // Set in HookCarBallHit
    Websocket->SendEvent("game:goal_scored", goalScoreData);
}
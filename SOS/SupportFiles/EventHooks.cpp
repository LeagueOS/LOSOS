#include "Main/SOS.h"
#include "SOSUtils.h"
#include "json.hpp"
#include <chrono>

/*

    NOTE: As boilerplate, all of these functions only call their respective state functions.
    Some of these functions shouldn't call a state function and should instead directly call TransitionTo

*/

void SOS::HookAllEvents()
{
    using namespace std::placeholders;

    //UPDATE GAME STATE EVERY TICK
    gameWrapper->RegisterDrawable(std::bind(&SOS::HookViewportTick, this, _1));

    //GAME EVENTS
    gameWrapper->HookEventPost("Function TAGame.Team_TA.PostBeginPlay", std::bind(&SOS::HookInitTeams, this));
    gameWrapper->HookEvent("Function TAGame.GameInfo_Replay_TA.InitGame", std::bind(&SOS::HookReplayCreated, this));
    gameWrapper->HookEventPost("Function TAGame.GameEvent_Soccar_TA.Destroyed", std::bind(&SOS::HookMatchDestroyed, this));
    gameWrapper->HookEventPost("Function GameEvent_Soccar_TA.Countdown.BeginState", std::bind(&SOS::HookCountdownInit, this));
    gameWrapper->HookEvent("Function GameEvent_Soccar_TA.Active.StartRound", std::bind(&SOS::HookRoundStarted, this));
    gameWrapper->HookEvent("Function TAGame.Ball_TA.Explode", std::bind(&SOS::HookBallExplode, this));
    gameWrapper->HookEventWithCaller<BallWrapper>("Function TAGame.Ball_TA.OnHitGoal", std::bind(&SOS::HookOnHitGoal, this, _1, _2));
    gameWrapper->HookEventPost("Function GameEvent_Soccar_TA.ReplayPlayback.BeginState", std::bind(&SOS::HookGoalReplayStart, this));
    gameWrapper->HookEventPost("Function GameEvent_Soccar_TA.ReplayPlayback.EndState", std::bind(&SOS::HookGoalReplayEnd, this));
    gameWrapper->HookEventPost("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded", std::bind(&SOS::HookMatchEnded, this));
    gameWrapper->HookEventPost("Function GameEvent_Soccar_TA.PodiumSpotlight.BeginState", std::bind(&SOS::HookPodiumStart, this));
    gameWrapper->HookEventWithCaller<ServerWrapper>("Function TAGame.PRI_TA.ClientNotifyStatTickerMessage", std::bind(&SOS::HookStatEvent, this, _1, _2));
    gameWrapper->HookEventWithCallerPost<ActorWrapper>("Function TAGame.ReplayDirector_TA.OnScoreDataChanged", std::bind(&SOS::HookReplayScoreDataChanged, this, _1));

    //CLOCK EVENTS
    gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.OnGameTimeUpdated", std::bind(&SOS::HookOnTimeUpdated, this));
    gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.OnOvertimeUpdated", std::bind(&SOS::HookOnOvertimeStarted, this));
    gameWrapper->HookEvent("Function Engine.WorldInfo.EventPauseChanged", std::bind(&SOS::HookOnPauseChanged, this));
    gameWrapper->HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.EventHitBall", std::bind(&SOS::HookCarBallHit, this, _1, _2));
}


// GAME STATE //
void SOS::HookViewportTick(CanvasWrapper canvas)
{
    if(!ShouldRun()) { return; }

    UpdateGameState(canvas);
    DebugRender(canvas);
}


// CLOCK EVENTS //
void SOS::HookOnTimeUpdated()
{
    if(!ShouldRun()) { return; }

    States[CurrentState]->OnTimeUpdated();
}

void SOS::HookOnOvertimeStarted()
{
    if(!ShouldRun()) { return; }

    States[CurrentState]->OnOvertimeStarted();
}

void SOS::HookOnPauseChanged()
{
    if(!ShouldRun()) { return; }

    States[CurrentState]->OnPauseChanged();
}

void SOS::HookCarBallHit(CarWrapper car, void* params)
{
    if(!ShouldRun()) { return; }

    States[CurrentState]->OnCarHitBall(car, params);
}

void SOS::SetBallHit(bool bHit)
{
    if(!ShouldRun()) { return; }

    // Whoops, this wasn't a function hook. Look at how 1.6 did this
}


// GAME EVENTS //
void SOS::HookInitTeams()
{
    if(!ShouldRun()) { return; }

    static int NumTimesCalled = 0;

    //"Function TAGame.Team_TA.PostBeginPlay" is called twice rapidly, once for each team
    // Only initialize lobby on the second hook once both teams are ready

    ++NumTimesCalled;
    if(NumTimesCalled >= 2)
    {
        //Set a delay so that everything can be filled in before trying to initialize
        gameWrapper->SetTimeout([this](GameWrapper* gw)
        {
            if(ShouldRun())
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

    if(!ShouldRun()) { return; }

    States[CurrentState]->OnMatchCreated();
}

void SOS::HookReplayCreated()
{
    if(!ShouldRun()) { return; }

    States[CurrentState]->OnReplayCreated();
}

void SOS::HookMatchDestroyed()
{
    if(!ShouldRun()) { return; }

    States[CurrentState]->OnMatchDestroyed();
}

void SOS::HookCountdownInit()
{
    if(!ShouldRun()) { return; }

    States[CurrentState]->OnCountdownInit();
}

void SOS::HookRoundStarted()
{
    if(!ShouldRun()) { return; }

    States[CurrentState]->OnRoundStarted();
}

void SOS::HookBallExplode()
{
    if(!ShouldRun()) { return; }

    States[CurrentState]->OnBallExplode();
}

void SOS::HookOnHitGoal(BallWrapper ball, void* params)
{
    if(!ShouldRun()) { return; }

    States[CurrentState]->OnBallHitGoal(ball, params);
}

void SOS::HookGoalReplayStart()
{
    if(!ShouldRun()) { return; }

    States[CurrentState]->OnGoalReplayStart();
}

void SOS::HookGoalReplayEnd()
{
    if(!ShouldRun()) { return; }

    States[CurrentState]->OnGoalReplayEnd();
}

void SOS::HookMatchEnded()
{
    if(!ShouldRun()) { return; }

    States[CurrentState]->OnMatchEnded();
}

void SOS::HookPodiumStart()
{
    if(!ShouldRun()) { return; }

    States[CurrentState]->OnPodiumStart();
}

void SOS::HookStatEvent(ServerWrapper caller, void* params)
{
    if(!ShouldRun()) { return; }

    GetStatEventInfo(caller, params);

    States[CurrentState]->OnStatEvent(caller, params);
}

void SOS::HookReplayScoreDataChanged(ActorWrapper caller)
{
    if(!ShouldRun()) { return; }

    ReplayDirectorWrapper RDW(caller.memory_address);
    ReplayScoreData ScoreData = RDW.GetReplayScoreData();

    //If ScoredBy is null, that likely means this call was just to reset values
    if(ScoreData.ScoredBy == 0) { return; }

    PriWrapper ScoredBy(ScoreData.ScoredBy);
    std::string ScorerName, ScorerID;
    SOSUtils::GetNameAndID(ScoredBy, ScorerName, ScorerID);

    PriWrapper AssistedBy(ScoreData.AssistedBy);
    std::string AssisterName, AssisterID;
    SOSUtils::GetNameAndID(AssistedBy, AssisterName, AssisterID);

    json goalScoreData;
    goalScoreData["goalspeed"] = SOSUtils::ToKPH(ScoreData.Speed);
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

    States[CurrentState]->OnReplayScoreDataChanged(caller);
}
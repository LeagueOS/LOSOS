#pragma once
#include "bakkesmod/plugin/bakkesmodplugin.h"

enum class EGameState
{
    NoGame = 0,
    AdminPaused,
    Kickoff,
    Gameplay,
    PreGoalReplay,
    GoalReplay,
    Overtime,
    Podium
};

class GameState
{
public:
    GameState(EGameState InStateType, const char* InStateName);

    EGameState GetStateType()  { return StateType; }
    const char* GetStateName() { return StateName; }

    //Tick hooks
    virtual void Tick()                                          {};
    virtual void TickNameplates()                                {};

    //Event hooks
    virtual void OnBallExplode()                                 {};
    virtual void OnBallHitGoal(BallWrapper ball, void* params)   {};
    virtual void OnInitTeams()                                   {};
    virtual void OnMatchCreated()                                {};
    virtual void OnMatchDestroyed()                              {};
    virtual void OnMatchEnded()                                  {};
    virtual void OnCountdownInit()                               {};
    virtual void OnRoundStarted()                                {};
    virtual void OnPodiumStart()                                 {};
    virtual void OnReplayCreated()                               {};
    virtual void OnGoalReplayStart()                             {};
    virtual void OnGoalReplayEnd()                               {};
    virtual void OnStatEvent(ServerWrapper caller, void* params) {};
    virtual void OnReplayScoreDataChanged(ActorWrapper caller)   {};

    //Time hooks
    virtual void OnTimeUpdated()                                 {};
    virtual void OnOvertimeStarted()                             {};
    virtual void OnPauseChanged()                                {};
    virtual void OnCarHitBall(CarWrapper car, void* params)      {};

protected:
    EGameState StateType;
    const char* StateName;
};

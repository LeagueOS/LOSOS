#include "ClockManager.h"
#include "Plugin/SOSUtils.h"

ClockManager::ClockManager(std::shared_ptr<GameWrapper> InGameWrapper)
    : gameWrapper(InGameWrapper) {}

void ClockManager::StartClock()
{
    //When overtime starts OnTimeUpdated is called, but we don't want the clock starting then
    if(bOvertimeStarted)
    {
        bOvertimeStarted = false;
        return;
    }

    //Clock is already started
    if(bActive)
    {
        return;
    }

    //Start the clock
    bActive = true;

    //Get the first delta time when clock starts
    GetTime(true);
}

void ClockManager::StopClock()
{
    bActive = false;
}

void ClockManager::ResetClock()
{
    StopClock();
    ReadClockTime = 0;
    DeltaAggregate = 0.f;
}

void ClockManager::OnClockUpdated()
{
    if(!bActive)
    {
        StartClock();
    }

    ServerWrapper Server = SOSUtils::GetCurrentGameState(gameWrapper);
    if(Server.IsNull()) { return; }

    //Get the current second from the game. Can't get float time
    ReadClockTime = Server.GetSecondsRemaining();

    //Since this function should only be called when the game time decimal hits 0, reset the aggregate
    DeltaAggregate = 0.f;
}

float ClockManager::GetTime(bool bResetCurrentDelta)
{
    using namespace std::chrono;

    //This line is called once
    static steady_clock::time_point SnapshotTime = steady_clock::now();
    
    //Reset the current delta when clock has been unpaused
    if(bResetCurrentDelta)
    {
        SnapshotTime = steady_clock::now();
    }

    //Calculate new delta if the clock is running
    if(bActive)
    {
        auto CurrentTime = steady_clock::now();

        float Delta = duration_cast<duration<float>>(CurrentTime - SnapshotTime).count();
        DeltaAggregate += Delta;

        SnapshotTime = CurrentTime;
    }

    //Check the current game status
    ServerWrapper Server = SOSUtils::GetCurrentGameState(gameWrapper);
    if(!Server.IsNull())
    {
        bIsOvertime = Server.GetbOverTime();
        bIsUnlimitedTime = Server.GetbUnlimitedTime();
    }

    //Calculate the current time value
    float OutputTime = 0.f;
    if(bIsOvertime || bIsUnlimitedTime)
    {
        OutputTime = static_cast<float>(ReadClockTime) + DeltaAggregate;
    }
    else
    {
        OutputTime = static_cast<float>(ReadClockTime) - DeltaAggregate;
    }

    return OutputTime;
}

void ClockManager::OnOvertimeStarted()
{
    StopClock();
    ResetClock();
    bOvertimeStarted = true;
}

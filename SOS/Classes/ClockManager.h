#pragma once
#include "bakkesmod/plugin/bakkesmodplugin.h"

class ClockManager
{
public:
    ClockManager(std::shared_ptr<GameWrapper> InGameWrapper);

    void StartClock(bool bResetAggregate);
    void StopClock();
    bool IsClockRunning() { return bActive; }

    void ResetClock();

    void OnClockUpdated();
    float GetTime(bool bResetCurrentDelta = false);

    void OnOvertimeStarted();

private:
    ClockManager() = delete; // No default constructor

    std::shared_ptr<GameWrapper> gameWrapper;

    bool bActive = false;
    bool bIsOvertime = false;
    bool bIsUnlimitedTime = false;
    bool bOvertimeStarted = false;

    int ReadClockTime = 0;
    float DeltaAggregate = 0.f;
};

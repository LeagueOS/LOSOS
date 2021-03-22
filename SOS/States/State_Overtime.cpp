#include "State_Overtime.h"

State_Overtime::State_Overtime()
{
    //Override StateType and StateName that were set in the State_Gameplay constructor
    StateType = EGameState::Overtime;
    StateName = "Overtime";
}

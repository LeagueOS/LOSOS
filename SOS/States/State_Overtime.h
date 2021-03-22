#pragma once
#include "State_Gameplay.h"

//Inherits from State_Gameplay because everything except for time works the same
class State_Overtime : public State_Gameplay
{
public:
    State_Overtime();
};

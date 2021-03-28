#include "IGameState.h"

IGameState::IGameState(EGameState InStateType, const char* InStateName)
    : StateType(InStateType), StateName(InStateName) {}

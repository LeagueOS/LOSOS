#include "GameState.h"

GameState::GameState(EGameState InStateType, const char* InStateName)
    : StateType(InStateType), StateName(InStateName) {}

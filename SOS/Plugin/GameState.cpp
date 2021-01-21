#include "SOS.h"
#include "SOSUtils.h"
#include "json.hpp"
#include "RenderingTools.h"
#include "utils/parser.h"
#include "bakkesmod/wrappers/GameObject/Stats/StatEventWrapper.h"

void SOS::UpdateGameState(CanvasWrapper canvas)
{
    //Initialize JSON objects
    json::JSON state;
    state["event"] = "gamestate";
    state["players"] = json::Object();
    state["game"] = json::Object();

    //Might want to change this to take MatchCreated into account?
    state["hasGame"] = true;

    GetGameStateInfo(canvas, state);
    Websocket->SendEvent("game:update_state", state);
}

void SOS::GetGameStateInfo(CanvasWrapper canvas, json::JSON& state)
{
    using namespace std::chrono;

    //Server is nullchecked in the viewport tick call to UpdateGameState
    ServerWrapper server = SOSUtils::GetCurrentGameState(gameWrapper);

    //Get ball speed every tick
    GetCurrentBallSpeed();

    //Initialize delta timers. These static lines only get called once
    static auto LastGameStateCallTime = steady_clock::now();
    static auto LastNameplatesCallTime = steady_clock::now();

    //Get deltas since the last time these functions were called
    float TimeSinceLastGameStateCall = duration_cast<duration<float>>(steady_clock::now() - LastGameStateCallTime).count();
    float TimeSinceLastNameplatesCall = duration_cast<duration<float>>(steady_clock::now() - LastNameplatesCallTime).count();

    //Game State
    if(TimeSinceLastGameStateCall >= (*cvarUpdateRate / 1000.f))
    {
        GetPlayerInfo(state, server);
        GetTeamInfo(state, server);
        GetGameTimeInfo(state, server);
        GetBallInfo(state, server);
        GetWinnerInfo(state, server);
        GetArenaInfo(state);
        GetCameraInfo(state);
        
        LastGameStateCallTime = steady_clock::now();
    }

    //Nameplates - twice the rate of game state for better positioning fidelity?
    //Previously: if((TimeSinceLastNameplatesCall * 1000) < 11.11f) for 90fps rate
    if(TimeSinceLastNameplatesCall >= ((*cvarUpdateRate * 0.5f) / 1000.f))
    {
        GetNameplateInfo(canvas);

        LastNameplatesCallTime = steady_clock::now();
    }
}

void SOS::GetPlayerInfo(json::JSON& state, ServerWrapper server)
{
    ArrayWrapper<PriWrapper> PRIs = server.GetPRIs();
    for (int i = 0; i < PRIs.Count(); ++i)
    {
        PriWrapper pri = PRIs.Get(i);
        if (pri.IsNull()) { continue; }
        if (pri.IsSpectator() || pri.GetTeamNum() == 255) { continue; }
        GetIndividualPlayerInfo(state, pri);
    }
}

void SOS::GetIndividualPlayerInfo(json::JSON& state, PriWrapper pri)
{
    std::string name, id;
    SOSUtils::GetNameAndID(pri, name, id);

    state["players"][id] = json::Object();

    state["players"][id]["name"] = name;
    state["players"][id]["id"] = id;
    state["players"][id]["primaryID"] = std::to_string(pri.GetUniqueIdWrapper().GetUID()); // TODO: Needs to be replaced once UniqueIDWrapper gets better sorted out
    state["players"][id]["shortcut"] = pri.GetSpectatorShortcut();
    state["players"][id]["team"] = pri.GetTeamNum();
    state["players"][id]["score"] = pri.GetMatchScore();
    state["players"][id]["goals"] = pri.GetMatchGoals();
    state["players"][id]["shots"] = pri.GetMatchShots();
    state["players"][id]["assists"] = pri.GetMatchAssists();
    state["players"][id]["saves"] = pri.GetMatchSaves();
    state["players"][id]["touches"] = pri.GetBallTouches();
    state["players"][id]["cartouches"] = pri.GetCarTouches();
    //state["players"][id]["demos"] = pri.GetMatchDemolishes(); //Always returns 0

    CarWrapper car = pri.GetCar();

    //Car is null, return
    if(car.IsNull())
    {
        state["players"][id]["hasCar"] = false;
        state["players"][id]["speed"] = 0;
        state["players"][id]["boost"] = 0;
        state["players"][id]["isSonic"] = false;

        state["players"][id]["isDead"] = false;
        state["players"][id]["attacker"] = "";

		return;
	}

	Vector carLocation = car.GetLocation();
	state["players"][id]["x"] = carLocation.X;
	state["players"][id]["y"] = carLocation.Y;
	state["players"][id]["z"] = carLocation.Z;

	Rotator carRotation = car.GetRotation();
	state["players"][id]["roll"] = carRotation.Roll;
	state["players"][id]["pitch"] = carRotation.Pitch;
	state["players"][id]["yaw"] = carRotation.Yaw;

	state["players"][id]["onWall"] = car.IsOnWall();
	state["players"][id]["onGround"] = car.IsOnGround();

	//Check if player is powersliding
	ControllerInput controller = car.GetInput();
	state["players"][id]["isPowersliding"] = controller.Handbrake && car.IsOnGround();

    if(car.GetbHidden())
    {
        state["players"][id]["isDead"] = true;
        state["players"][id]["attacker"] = "";

        PriWrapper att = car.GetAttackerPRI(); // Attacker is only set on local player???
        if(!att.IsNull())
        {
            std::string attName, attID;
            SOSUtils::GetNameAndID(pri, attName, attID);
            state["players"][id]["attacker"] = attID;
        }
    }
    else
    {
        state["players"][id]["isDead"] = false;
        state["players"][id]["attacker"] = "";
    }

    float boost = car.GetBoostComponent().IsNull() ? 0 : car.GetBoostComponent().GetPercentBoostFull();

    state["players"][id]["hasCar"] = true;
    state["players"][id]["speed"] = static_cast<int>(SOSUtils::ToKPH(car.GetVelocity().magnitude()) + .5f);
    state["players"][id]["boost"] = static_cast<int>(boost * 100);
    state["players"][id]["isSonic"] = car.GetbSuperSonic() ? true : false;
}

void SOS::GetTeamInfo(json::JSON& state, ServerWrapper server)
{
    //Not enough teams
    if (server.GetTeams().Count() != 2)
    {
        state["game"]["teams"][0]["name"] = "BLUE";
        state["game"]["teams"][0]["score"] = 0;
        state["game"]["teams"][0]["color_primary"] = "000000";
        state["game"]["teams"][0]["color_secondary"] = "000000";
        state["game"]["teams"][1]["name"] = "ORANGE";
        state["game"]["teams"][1]["score"] = 0;
        state["game"]["teams"][1]["color_primary"] = "000000";
        state["game"]["teams"][1]["color_secondary"] = "000000";
        return;
    }

    TeamWrapper team0 = server.GetTeams().Get(0);
    if (!team0.IsNull())
    {
        state["game"]["teams"][0]["name"] = team0.GetCustomTeamName().IsNull() ? "BLUE" : team0.GetCustomTeamName().ToString();
        state["game"]["teams"][0]["score"] = team0.GetScore();
        state["game"]["teams"][0]["color_primary"] = get_hex_from_color(team0.GetPrimaryColor() * 255.f).substr(1,6);
        state["game"]["teams"][0]["color_secondary"] = get_hex_from_color(team0.GetSecondaryColor() * 255.f).substr(1,6);
    }
    else
    {
        state["game"]["teams"][0]["name"] = "BLUE";
        state["game"]["teams"][0]["score"] = 0;
        state["game"]["teams"][0]["color_primary"] = "000000";
        state["game"]["teams"][0]["color_secondary"] = "000000";
    }

    TeamWrapper team1 = server.GetTeams().Get(1);
    if (!team1.IsNull())
    {
        state["game"]["teams"][1]["name"] = team1.GetCustomTeamName().IsNull() ? "ORANGE" : team1.GetCustomTeamName().ToString();
        state["game"]["teams"][1]["score"] = team1.GetScore();
        state["game"]["teams"][1]["color_primary"] = get_hex_from_color(team1.GetPrimaryColor() * 255.f).substr(1,6);
        state["game"]["teams"][1]["color_secondary"] = get_hex_from_color(team1.GetSecondaryColor() * 255.f).substr(1,6);
    }
    else
    {
        state["game"]["teams"][1]["name"] = "ORANGE";
        state["game"]["teams"][1]["score"] = 0;
        state["game"]["teams"][1]["color_primary"] = "000000";
        state["game"]["teams"][1]["color_secondary"] = "000000";
    }
}

void SOS::GetGameTimeInfo(json::JSON& state, ServerWrapper server)
{
    float OutputTime = !firstCountdownHit ? 300.f : Clock->GetTime();

    state["game"]["time"] = OutputTime;
    state["game"]["isOT"] = (bool)server.GetbOverTime();

    if (gameWrapper->IsInReplay())
    {
        state["game"]["frame"] = gameWrapper->GetGameEventAsReplay().GetCurrentReplayFrame();
        state["game"]["elapsed"] = gameWrapper->GetGameEventAsReplay().GetReplayTimeElapsed();
    }

    LOGC(std::to_string(OutputTime));
}

void SOS::GetBallInfo(json::JSON& state, ServerWrapper server)
{
    BallWrapper ball = server.GetBall();

    //Ball is null
    if (ball.IsNull())
    {
        state["game"]["ballSpeed"] = 0;
        state["game"]["ballTeam"] = 255;
        state["game"]["isReplay"] = false;
        return;
    }

	//Get ball info
	state["game"]["ballSpeed"] = static_cast<int>(BallSpeed->GetCachedBallSpeed());
	state["game"]["ballTeam"] = ball.GetHitTeamNum();
	state["game"]["isReplay"] = ball.GetbReplayActor() ? true : false;

	//Get Ball Location
	Vector ballLocation = ball.GetLocation();
	state["game"]["ballX"] = ballLocation.X;
	state["game"]["ballY"] = ballLocation.Y;
	state["game"]["ballZ"] = ballLocation.Z;
}

void SOS::GetWinnerInfo(json::JSON& state, ServerWrapper server)
{
    TeamWrapper winner = server.GetGameWinner();

    //Winning team is null
    if (winner.IsNull())
    {
        state["game"]["hasWinner"] = false;
        state["game"]["winner"] = "";
        return;
    }

    //Get the winning team
    state["game"]["hasWinner"] = true;
    state["game"]["winner"] = winner.GetCustomTeamName().IsNull() ? "" : winner.GetCustomTeamName().ToString();
}

void SOS::GetArenaInfo(json::JSON& state)
{
    state["game"]["arena"] = gameWrapper->GetCurrentMap();
}

void SOS::GetCameraInfo(json::JSON& state)
{
    CameraWrapper cam = gameWrapper->GetCamera();

    //Camera is null
    if (cam.IsNull())
    {
        state["game"]["hasTarget"] = false;
        state["game"]["target"] = "";
        return;
    }

    PriWrapper specPri = PriWrapper(reinterpret_cast<std::uintptr_t>(cam.GetViewTarget().PRI));

    //Target PRI is null
    if (specPri.IsNull())
    {
        state["game"]["hasTarget"] = false;
        state["game"]["target"] = "";
        return;
    }

    //Target PRI is the local player
    if (specPri.IsLocalPlayerPRI())
    {
        state["game"]["hasTarget"] = false;
        state["game"]["target"] = "";
        return;
    }

    //Get the target's name
    std::string targetName, targetID;
    SOSUtils::GetNameAndID(specPri, targetName, targetID);
    state["game"]["hasTarget"] = true;
    state["game"]["target"] = targetID;
}

void SOS::GetNameplateInfo(CanvasWrapper canvas)
{
    #ifdef USE_NAMEPLATES
    
    if(!SOSUtils::ShouldRun(gameWrapper)) { return; }
    CameraWrapper camera = gameWrapper->GetCamera();
    if(camera.IsNull()) { return; }
    ServerWrapper server = SOSUtils::GetCurrentGameState(gameWrapper);
    if(server.IsNull()) { return; }

    //Create nameplates JSON object
    json::JSON nameplatesState;
    nameplatesState["nameplates"] = json::Object();
    
    //Get nameplate info and send through websocket
    Nameplates->GetNameplateInfo(canvas, camera, server, nameplatesState);
    Websocket->SendEvent("game:nameplate_tick", nameplatesState);
    
    #endif
}


// CALLED BY EVENT HOOKS //
void SOS::GetCurrentBallSpeed()
{
    if (!SOSUtils::ShouldRun(gameWrapper)) { return; }
    ServerWrapper server = SOSUtils::GetCurrentGameState(gameWrapper);
    if (server.IsNull()) { return; }
    BallWrapper ball = server.GetBall();
    if (ball.IsNull()) { return; }

    BallSpeed->UpdateBallSpeed(SOSUtils::ToKPH(ball.GetVelocity().magnitude()) + .5f);
}

void SOS::GetLastTouchInfo(CarWrapper car, void* params)
{
    if(!SOSUtils::ShouldRun(gameWrapper)) { return; }

    //Local struct
    struct HitBallParams
    {
      uintptr_t Car; //CarWrapper
      uintptr_t Ball; //BallWrapper
      Vector HitLocation;
      Vector HitNormal;  
    };
    auto* CastParams = (HitBallParams*)params;
    if(car.IsNull() || !CastParams->Car || !CastParams->Ball) { return; }

    PriWrapper PRI = car.GetPRI();
    if(PRI.IsNull()) { return; }
    
    //Get player info
    std::string playerName, playerID;
    SOSUtils::GetNameAndID(PRI, playerName, playerID);

    //Build ball touch event
    json::JSON ballTouchEvent;
    ballTouchEvent["player"]["name"] = playerName;
    ballTouchEvent["player"]["id"] = playerID;
    ballTouchEvent["ball"]["pre_hit_speed"] = BallSpeed->GetCachedBallSpeed();
    ballTouchEvent["ball"]["location"]["X"] = CastParams->HitLocation.X;
    ballTouchEvent["ball"]["location"]["Y"] = CastParams->HitLocation.Y;
    ballTouchEvent["ball"]["location"]["Z"] = CastParams->HitLocation.Z;

    //Set values. Speed is delayed by 1 tick so it can get the new speed accurately
    lastTouch.playerID = playerID;

    gameWrapper->SetTimeout([this, ballTouchEvent](GameWrapper*) mutable
    {
        lastTouch.speed = BallSpeed->GetCachedBallSpeed();
        ballTouchEvent["ball"]["post_hit_speed"] = BallSpeed->GetCachedBallSpeed();
        Websocket->SendEvent("game:ball_hit", ballTouchEvent);
    }, 0.01f);
}

Vector2F SOS::GetGoalImpactLocation(BallWrapper ball, void* params)
{
    // Goal Real Size: (1920, 752) - Extends past posts and ground
    // Goal Scoreable Zone: (1800, 640)
    // Ball Radius: ~93

    BallHitGoalParams* newParams = (BallHitGoalParams*)params;
    GoalWrapper goal(newParams->GoalPointer);

    if(goal.memory_address == NULL) { return {0,0}; }

    //Get goal direction and correct rounding errors. Results should either be 0 or +-1
    Vector GoalDirection = RT::Matrix3(goal.GetGoalOrientation().GetRotation()).forward;        
    GoalDirection.X = abs(GoalDirection.X) < .5f ? 0.f : GoalDirection.X / abs(GoalDirection.X);
    GoalDirection.Y = abs(GoalDirection.Y) < .5f ? 0.f : GoalDirection.Y / abs(GoalDirection.Y);
    GoalDirection.Z = abs(GoalDirection.Z) < .5f ? 0.f : GoalDirection.Z / abs(GoalDirection.Z);

    static const Vector2F GoalSize = {1800, 640}; // Scoreable zone
    float HitX = ((newParams->HitLocation.X * GoalDirection.Y) + (GoalSize.X / 2)) / GoalSize.X;
    float HitY = (GoalSize.Y - newParams->HitLocation.Z) / GoalSize.Y;

    return Vector2F{ HitX, HitY };
}

void SOS::GetStatEventInfo(ServerWrapper caller, void* params)
{
    auto tArgs = (DummyStatEventContainer*)params;

    auto statEvent = StatEventWrapper(tArgs->StatEvent);
    auto label = statEvent.GetLabel();
    auto eventStr = label.ToString();

    //Victim info
    auto victim = PriWrapper(tArgs->Victim);
    std::string victimName, victimID;
    SOSUtils::GetNameAndID(victim, victimName, victimID);
    
    //Receiver info
    auto receiver = PriWrapper(tArgs->Receiver);
    std::string receiverName, receiverID;
    SOSUtils::GetNameAndID(receiver, receiverName, receiverID); 

    //General statfeed event
    json::JSON statfeed;
    statfeed["type"] = eventStr;
    statfeed["main_target"]["name"] = receiverName;
    statfeed["main_target"]["id"] = receiverID;
    statfeed["secondary_target"]["name"] = victimName;
    statfeed["secondary_target"]["id"] = victimID;
    Websocket->SendEvent("game:statfeed_event", statfeed);

    //Goal statfeed event
    if (eventStr == "Goal")
    {
        json::JSON goalScoreData;
        goalScoreData["goalspeed"] = BallSpeed->GetCachedBallSpeed();
        goalScoreData["impact_location"]["X"] = GoalImpactLocation.X;
        goalScoreData["impact_location"]["Y"] = GoalImpactLocation.Y;
        goalScoreData["scorer"]["name"] = receiverName;
        goalScoreData["scorer"]["id"] = receiverID;
        goalScoreData["scorer"]["teamnum"] = receiver.IsNull() ? -1 : receiver.GetTeamNum();
        goalScoreData["ball_last_touch"]["player"] = lastTouch.playerID;
        goalScoreData["ball_last_touch"]["speed"] = lastTouch.speed;
        Websocket->SendEvent("game:goal_scored", goalScoreData);
    }
}
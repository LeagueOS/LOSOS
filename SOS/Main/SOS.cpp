#include "SOS.h"
#include "States/State_AdminPaused.h"
#include "States/State_Gameplay.h"
#include "States/State_GoalReplay.h"
#include "States/State_Kickoff.h"
#include "States/State_NoGame.h"
#include "States/State_Overtime.h"
#include "States/State_Podium.h"
#include "States/State_PreGoalReplay.h"

/*
    This is a modified version of DanMB's GameStateApi: https://github.com/DanMB/GameStateApi
    A lot of features merged in from the original SOS plugin: https://gitlab.com/bakkesplugins/sos/sos-plugin
    
    - Modified by CinderBlock
    - Thanks to Martinn for the Stat Feed code (and inadvertently, demolitions)
*/

BAKKESMOD_PLUGIN(SOS, "Simple Overlay System", SOS_VERSION, PLUGINTYPE_THREADED)

//Declare global pointers
std::shared_ptr<CVarManagerWrapper> GlobalCvarManager;
std::shared_ptr<GameWrapper> GlobalGameWrapper;
std::shared_ptr<int> SOSLogLevel;

void SOS::onLoad()
{
    //Assign global pointers. SOSLogLevel assigned in cvars
    GlobalCvarManager = cvarManager;
    GlobalGameWrapper = gameWrapper;

    //Register cvars
    SOSLogLevel  = std::make_shared<int>(0);
    bEnabled     = std::make_shared<bool>(false);
    Port         = std::make_shared<int>(49122);
    UpdateRate   = std::make_shared<float>(100.0f);
    bDebugRender = std::make_shared<bool>(false);
    GlobalCvarManager->registerCvar(CVAR_LOG_LEVEL,    "1",     "How much info to print to the console", true, true, 0, true, 3).bindTo(SOSLogLevel);
    GlobalCvarManager->registerCvar(CVAR_ENABLED,      "1",     "Enable SOS plugin", true).bindTo(bEnabled);
    GlobalCvarManager->registerCvar(CVAR_PORT,         "49122", "Websocket port for SOS overlay plugin", true).bindTo(Port);
    GlobalCvarManager->registerCvar(CVAR_MESSAGE_RATE, "100",   "Rate at which to send events to websocket (milliseconds)", true, true, 5.0f, true, 2000.0f).bindTo(UpdateRate);
    GlobalCvarManager->registerCvar(CVAR_DEBUG_RENDER, "0",     "Enables on-screen debug text for SOS", true).bindTo(bDebugRender);

    //Create managers
    Websocket  = std::make_shared<WebsocketManager>(cvarManager, *Port);
    BallSpeed  = std::make_shared<BallSpeedManager>(gameWrapper);
    Clock      = std::make_shared<ClockManager>(gameWrapper, Websocket);
    Nameplates = std::make_shared<NameplatesManager>();

    //Initialize states
    States[EGameState::NoGame]        = new State_NoGame();
    States[EGameState::AdminPaused]   = new State_AdminPaused();
    States[EGameState::Kickoff]       = new State_Kickoff();
    States[EGameState::Gameplay]      = new State_Gameplay();
    States[EGameState::PreGoalReplay] = new State_PreGoalReplay();
    States[EGameState::GoalReplay]    = new State_GoalReplay();
    States[EGameState::Overtime]      = new State_Overtime();
    States[EGameState::Podium]        = new State_Podium();

    //Set the current state
    PreviousState = EGameState::NoGame;
    CurrentState  = EGameState::NoGame;

    //Register event hooks
    HookAllEvents();

    //Check if there is a game currently active one second after the plugin has loaded
    gameWrapper->SetTimeout([this](GameWrapper* gw)
    {
        if(ShouldRun())
        {
            HookMatchCreated();
        }
    }, 1.f);

    //Run websocket server. Locks onLoad thread
    Websocket->StartServer();
}

void SOS::onUnload()
{
    // Clean up all memory so plugin can unload and reload //

    //Clean up websocket
    Websocket->StopServer();

    //Clean up states
    for(auto& State : States)
    {
        delete State.second;
        State.second = nullptr;
    }
}

bool SOS::ShouldRun()
{
    if(!*bEnabled)
    {
        return false;
    }

    //Check if server exists
    ServerWrapper Server = GlobalGameWrapper->GetCurrentGameState();
    if(Server.IsNull())
    {
        LOG3("server.IsNull(): (need false) true");
        return false;
    }

    //Allow in replay mode
    if(GlobalGameWrapper->IsInReplay())
    {
        return true;
    }

    //Check if player is spectating
    if(!GlobalGameWrapper->GetLocalCar().IsNull())
    {
        LOG3("GetLocalCar().IsNull(): (need true) false");
        return false;
    }

    //Check if server playlist exists
    GameSettingPlaylistWrapper GSPW = Server.GetPlaylist();
    if(GSPW.memory_address == NULL)
    {
        LOG3("server.GetPlaylist().memory_address == NULL: (need false) true");
        return false;
    }

    //Check if server playlist is valid
    // 6:  Private Match
    // 22: Custom Tournaments
    // 24: LAN Match
    static const std::vector<int> SafePlaylists = {6, 22, 24};
    int CurrentPlaylistID = GSPW.GetPlaylistId();
    if(!IsSafeMode(CurrentPlaylistID, SafePlaylists))
    {
        if(*SOSLogLevel >= 3)
        {
            std::string NotSafeMessage;
            NotSafeMessage += "server.GetPlaylist().GetPlaylistId(): (need ";
        
            //Add list of safe modes to string
            for(const auto& Playlist : SafePlaylists)
            {
                NotSafeMessage += std::to_string(Playlist) + ", ";
            }

            //Remove last ", "
            NotSafeMessage.pop_back();
            NotSafeMessage.pop_back();

            NotSafeMessage += ") " + std::to_string(CurrentPlaylistID);
            LOG3(NotSafeMessage);
        }

        return false;
    }

    return true;
}

bool SOS::IsSafeMode(int CurrentPlaylist, const std::vector<int>& SafePlaylists)
{
    for(const auto& SafePlaylist : SafePlaylists)
    {
        if (CurrentPlaylist == SafePlaylist)
        {
            return true;
        }
    }

    return false;
}

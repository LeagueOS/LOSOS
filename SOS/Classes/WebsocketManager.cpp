#include "WebsocketManager.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "json.hpp"
#include "utils/parser.h"


// PUBLIC FUNCTIONS //
WebsocketManager::WebsocketManager(std::shared_ptr<CVarManagerWrapper> InCvarManager, int InListenPort)
    : cvarManager(InCvarManager), ListenPort(InListenPort) {}

void WebsocketManager::SendEvent(std::string eventName, const json::JSON &jsawn)
{
    json::JSON event;
    event["event"] = eventName;
    event["data"] = jsawn;
    SendWebSocketPayload(event.dump());
}

void WebsocketManager::StartServer()
{
    using namespace std::placeholders;

    cvarManager->log("Starting WebSocket server");

    ws_connections = new ConnectionSet();
    ws_server = new PluginServer();
    
    cvarManager->log("Starting asio");
    ws_server->init_asio();
    ws_server->set_open_handler(websocketpp::lib::bind(&WebsocketManager::OnWsOpen, this, _1));
    ws_server->set_close_handler(websocketpp::lib::bind(&WebsocketManager::OnWsClose, this, _1));
    ws_server->set_message_handler(websocketpp::lib::bind(&WebsocketManager::OnWsMsg, this, _1, _2));
    ws_server->set_http_handler(websocketpp::lib::bind(&WebsocketManager::OnHttpRequest, this, _1));
    
    cvarManager->log("Starting listen on port " + std::to_string(ListenPort));
    ws_server->listen(ListenPort);
    
    cvarManager->log("Starting accepting connections");
    ws_server->start_accept();
    ws_server->run();
}

void WebsocketManager::StopServer()
{
    cvarManager->log("[SOS] Stopping websocket server");

    if (ws_server)
    {
        ws_server->stop();
        ws_server->stop_listening();
        delete ws_server;
        ws_server = nullptr;
    }

    if (ws_connections)
    {
        ws_connections->clear();
        delete ws_connections;
        ws_connections = nullptr;
    }
}


// PRIVATE FUNCTIONS //
void WebsocketManager::OnWsMsg(connection_hdl hdl, PluginServer::message_ptr msg)
{
    SendWebSocketPayload(msg->get_payload());
}

void WebsocketManager::OnHttpRequest(websocketpp::connection_hdl hdl)
{
    PluginServer::connection_ptr connection = ws_server->get_con_from_hdl(hdl);
    connection->append_header("Content-Type", "application/json");
    connection->append_header("Server", "SOS/1.4.0");

    if (connection->get_resource() == "/init")
    {
        json::JSON data;
        data["event"] = "init";
        data["data"] = "json here";

        connection->set_body(data.dump());
        connection->set_status(websocketpp::http::status_code::ok);

        return;
    }

    connection->set_body("Not found");
    connection->set_status(websocketpp::http::status_code::not_found);
}

void WebsocketManager::SendWebSocketPayload(std::string payload)
{
    // broadcast to all connections
    try
    {
        std::string output = payload;

        if (bUseBase64)
        {
            output = base64_encode((const unsigned char*)payload.c_str(), (unsigned int)payload.size());
        }

        for (const connection_hdl& it : *ws_connections)
        {
            ws_server->send(it, output, websocketpp::frame::opcode::text);
        }
    }
    catch (std::exception e)
    {
        cvarManager->log("An error occured sending websocket event: " + std::string(e.what()));
    }
}

/*
TODOs:
1) Break this out into a seperate file, to be called from the user's main.cpp
   Maybe call it framework.cpp.
*/
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <Hash.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>
#include <AsyncJson.h>
#include "TaskQueue.hpp"
#include "app.hpp"

// SKETCH BEGIN
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");
DNSServer dnsServer;
TaskQueue tasks;
Ticker sntpPoller;
App app(tasks);

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    if (type == WS_EVT_CONNECT)
    {
        client->printf("Hello Client %u :)", client->id());
        client->ping();
    }
    else if (type == WS_EVT_DISCONNECT)
    {
    }
    else if (type == WS_EVT_ERROR)
    {
    }
    else if (type == WS_EVT_PONG)
    {
    }
    else if (type == WS_EVT_DATA)
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        String msg = "";
        if (info->final && info->index == 0 && info->len == len)
        {
            //the whole message is in a single frame and we got all of it's data
            if (info->opcode == WS_TEXT)
            {
                for (size_t i = 0; i < info->len; i++)
                {
                    msg += (char)data[i];
                }
            }
            else
            {
                char buff[3];
                for (size_t i = 0; i < info->len; i++)
                {
                    sprintf(buff, "%02x ", (uint8_t)data[i]);
                    msg += buff;
                }
            }
        }
        else
        {
            //message is comprised of multiple frames or the frame is split into multiple packets
            if (info->opcode == WS_TEXT)
            {
                for (size_t i = 0; i < info->len; i++)
                {
                    msg += (char)data[i];
                }
            }
            else
            {
                char buff[3];
                for (size_t i = 0; i < info->len; i++)
                {
                    sprintf(buff, "%02x ", (uint8_t)data[i]);
                    msg += buff;
                }
            }

            if ((info->index + len) == info->len)
            {
                if (info->final)
                {
                    if (info->message_opcode == WS_TEXT)
                        client->text("I got your text message");
                    else
                        client->binary("I got your binary message");
                }
            }
        }
    }
}

const char *http_username = "admin";
const char *http_password = "admin";
String myHostname;
WiFiEventHandler _onStationModeGotIpHandler;
WiFiEventHandler _onStationModeDisconnectedHandler;

/**
TODO BW: 
- Startup as just Station.  Begin a timer for, say, 5 seconds.
- When a WiFi connection is made, stop the timer (if running).
- When the timer expires, if there is no WiFi connection, then start the soft AP and captive portal.
- Is there a way to immediately detect no WiFi credentials? If so, then skip straight to soft AP on init if that's the case.
  Yes, use WiFi.SSID() and if it is not empty, then assume we have a configuration.
  
- Implement a password for the soft AP.
- Deal with being unable to connect to a station due to invalid SSID or password.
- Implement workaround for https://github.com/esp8266/Arduino/issues/1615 .  
  I'm thinking that if a client connects to the captive portal (eg. we are not connected as STA to anything),
  then immediately stop the STA connect attempts `WiFi.disconnect();`
- Deal with losing connection to station.  I'm thinking this should set the timer again, just like at bootup.
  Maybe in this case the timer should be much longer?  This gets tricky with regard to security. This is one of those
  cases where you probably need to require pushing a physical button on the device.  
  ESP.eraseConfig() should blow away the SSID and password stored in flash.
- Implement scan, using AJAX.  Use async scan!  WiFiManager is synchronous (sucks).
*/
void initializeWiFi()
{
    myHostname = "ESP-" + String(ESP.getChipId(), HEX);
    WiFi.hostname(myHostname);

    _onStationModeGotIpHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP &eventInfo) {
        Serial.println("Connection established to AP. Setting mode(WIFI_STA).");
        dnsServer.stop();
        WiFi.softAPdisconnect();
        WiFi.mode(WIFI_STA);
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");
        
        if(time(nullptr) == 0)
        {
            sntpPoller.attach_ms(500, []
            { 
                if(time(nullptr) != 0)
                {
                    sntpPoller.detach();
                    app.notifyTimeAvailable();
                }
            });
        }
    });

    _onStationModeDisconnectedHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected &) {
        Serial.println("Lost connection to AP. TODO: start WIFI_AP_STA after delay.");
    });

    // Start WiFi as both Access Point (for captive portal config) and station.
    // Once we connect to another AP as a station, then we kill the captive portal.
    WiFi.mode(WIFI_AP_STA);
    delay(100);
    WiFi.softAP(myHostname.c_str());

    // This starts the STA connection, using existing SSID/password config.
    WiFi.begin();

    // Setup the DNS server redirecting all the domains to the apIP
    dnsServer.stop();
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", WiFi.softAPIP());
}

void handleWifiPageRequest(AsyncWebServerRequest *request)
{
    request->send(SPIFFS, "/wifi.html");
}

void sendJsonResponse(AsyncWebServerRequest &request, std::function<void(JsonObject& jsonResponse)> getJson) 
{
    auto * response = new AsyncJsonResponse();
    auto & jsonResponse = response->getRoot();
    getJson(jsonResponse);
    response->setLength();
    request.send(response);    // This will free the response once the request is complete.
}

void handleWifiScan(AsyncWebServerRequest *request)
{
    WiFi.scanNetworksAsync([request](int count) 
    {
        sendJsonResponse(*request, [count](JsonObject& jsonResponse) 
        {
            JsonArray& accessPoints = jsonResponse.createNestedArray("accessPoints");
            for(int index = 0; index < count; ++index) 
            {
                JsonObject& accessPoint = accessPoints.createNestedObject();
                accessPoint["SSID"] = WiFi.SSID(index);
                accessPoint["Encryption"] = WiFi.encryptionType(index);
                accessPoint["RSSI"] = WiFi.RSSI(index);
            }
        });
    }, true);
}

void handleWifiStatus(AsyncWebServerRequest *request) {
    sendJsonResponse(*request, [](JsonObject& json)
    {
        json["status"] = (int)WiFi.status();
        json["SSID"] = WiFi.SSID();
        json["RSSI"] = WiFi.RSSI();
        json["IpAddr"] = WiFi.localIP().toString();
    });
}

void handleWifiSave(AsyncWebServerRequest *request)
{
    auto ssid = request->arg("s");
    auto password = request->arg("p");

    String response = "You configured: SSID=" + ssid + ", password=" + password;
    response += "<br/>";
    response += "Previously configured SSID=" + WiFi.SSID();

    request->send(200, "text/plain", response);

    // Disconnect from the current access point, then start connect with the new config.
    //
    WiFi.disconnect();

    // Just save the new parameters.
    //
    WiFi.begin(ssid.c_str(), password.c_str(), 0, nullptr, false);
    
    // Then startup WiFi with our special sequence;
    //
    initializeWiFi();
}

void handleWifiForget(AsyncWebServerRequest *request)
{    
    request->send(200);

    WiFi.disconnect(true);
    initializeWiFi();
}

void setup()
{
    // Set the serial port to TX only, so we can use the RX pin (D9 / GPIO3) for other purposes.
    //
    //Serial.begin(115200, SerialConfig::SERIAL_8N1, SerialMode::SERIAL_TX_ONLY);
    Serial.setDebugOutput(false);

    initializeWiFi();

    ArduinoOTA.setHostname(myHostname.c_str());
    ArduinoOTA.begin();

    MDNS.addService("http", "tcp", 80);
    //MDNS.addServiceTxt("Your App Name", "NoProto", "Key", "Value");

    SPIFFS.begin();

    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    events.onConnect([](AsyncEventSourceClient *client) {
        client->send("hello!", NULL, millis(), 1000);
    });
    server.addHandler(&events);

    server.addHandler(new SPIFFSEditor(http_username, http_password));

    server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", String(ESP.getFreeHeap()));
    });

    server.serveStatic("/", SPIFFS, "/").setDefaultFile("default.html");

    server.on("/wifi/scan", &handleWifiScan);
    server.on("/wifisave", &handleWifiSave);
    server.on("/wifi/status", &handleWifiStatus);
    server.on("/wifi/forget", &handleWifiForget);

    // TODO BW: Extract method.
    for(auto& appApi: app.getApi()) 
    {
        auto function = appApi.Function;
        auto path = "/app/" + appApi.Path;

        auto* handler = new AsyncCallbackJsonWebHandler(path.c_str(), [function](AsyncWebServerRequest *request, JsonVariant &json) {
            sendJsonResponse(*request, [function, json](JsonObject& jsonResponse) {
                function(json.as<JsonObject>(), jsonResponse);
            });
        });

        server.addHandler(handler);        
    }

    server.onNotFound([](AsyncWebServerRequest *request) {
        // If this is a request coming over the softAP, then send the captive portal (WiFi config)
        //
        if(request->client()->localIP() == WiFi.softAPIP())
        {
            handleWifiPageRequest(request);
        }
        else
        {
            // Otherwise the usual 404.
            //
            request->send(404);
        }                
    });
    server.onFileUpload([](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) {
    });
    server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    });
    server.begin();

    app.StatusUpdated.connect([](const JsonObject & json)
    {
        auto textLength = json.measureLength();
        auto * pBuffer = ws.makeBuffer(textLength);
        if(!pBuffer) return;

        json.printTo(reinterpret_cast<char*>(pBuffer->get()), textLength + 1);
        ws.textAll(pBuffer);
    });

    app.begin();
}

void loop()
{
    dnsServer.processNextRequest();
    ArduinoOTA.handle();    
    tasks.handle();
}
#pragma once
#include <Framework.hpp>
#include <Arduino.h>
#include <Esp.h>
#include <pins_arduino.h>
#include <ArduinoJson.h>
#include <FunctionalInterrupt.h>
#include <FS.h>
#include <time.h>
#include <chrono>
#include <sys/time.h>
#include <TaskQueue.hpp>
#include <Timer.hpp>

/**
*/
class App : public Framework
{    
public:
    
    /**
    GPIO pin options

    D0 / GPIO16 -- Pulses high during sleep and reset. Might be connected to the RESET line on some boards 
                   (but doesn't appear to be on the ones I have). Built in pull-*down*.
                   "USER" or "RST".  Pushing RST does reset the MCU!  Need to depopulate some resistors to use GPIO16.
    *D1 / GPIO5 -- Safe. PUR.
    *D2 / GPIO4 -- Safe. PUR.
    *D3 / GPIO0 -- Boot Option. Is pulled high externally on dev boards. Must remain pulled high on power on.
                   Also FLASH button NodeMCU boards.
    D4 / GPIO2 -- Boot Option. Is pulled high externally on dev boards. Must remain pulled high on power on.
                   Also LED on ESP-12 module. Sends debug data at boot time.  
    *D5 / GPIO14 -- Safe, unless using HW SPI.
    *D6 / GPIO12 -- Safe, unless using HW SPI.
    *D7 / GPIO13 -- Safe, unless using HW SPI. This is MTCK, I2SI_BCK, HSPID/MOSI, and U0CTS.
    *D8 / GPIO15 -- Boot Option. Is pulled low externally on dev boards. Must remain pulled low on power on.
    *D9 / GPIO3 -- RXD. Safe to use if Serial is never used, or is used only with SERIAL_TX_ONLY.
    D10 / GPIO1 -- TXD. Cannot use if Serial in use.

    GPIO9 and GPIO10 are unavailable due to use as part of interface to external flash.
    */
    enum Pins
    {
        OutExample1 = D8,
        InExample1 = D3
    };

    App()
    {
    }

    const std::vector<AppApi> getApi() override 
    {
        return std::vector<AppApi>
        {
            // AppApi("wiggle", [this](JsonObject &, JsonObject&) { wiggle(); }),
            AppApi("getStatus", [this](JsonObject &, JsonObject& jsonResponse) { getStatus(jsonResponse); }),
            // AppApi("setFeedPumpEnable", [this](JsonObject & jsonRequest, JsonObject& jsonResponse) {
            //     setFeedPumpEnable(jsonRequest["enable"].as<bool>());
            //     jsonResponse["result"] = "all good";
            // })
        };
    }

    void begin() override
    {
    }

    void onTimeAvailable() override 
    {
    }
    
private:    
    void notifyStatusUpdate()
    {
        DynamicJsonBuffer buffer;
        auto & json = buffer.createObject();

        getStatus(json);

        StatusUpdated.emit(json);
    }

    /**
    TODO BW: This is terribly inefficient. We are sending everything all the time.
    This should only be done on the initial client request to pull all.  
    Then, only push discrete updates.  Consider adding a method that checks the existing value (as cached in a JSON
    object) to see if there is a change, and if so, add that to the JSON update object.
    - Will need some kind of change-tracking wrapper object.
    */
    void getStatus(JsonObject & json) 
    {        
        json["$function"] = "statusUpdate";
        json["status"] = 777;
    }
};
#pragma once

#include <Ticker.h>
#include <functional>
#include "TaskQueue.hpp"

/**
Wrapper class around Ticker that allows use of std::functions and thus lambdas,
and executes callback in a task handler.
*/
class Timer 
{    
    Ticker _ticker;
    std::function<void(void)> _callback;
    TaskQueue & _tasks;

    static void TickerCallback(Timer * timer)
    {
        timer->OnTickerCallback();
    }

    void OnTickerCallback()
    {
        _tasks.add([this]() { _callback(); });
    }

public:
    Timer(TaskQueue & tasks)
    : _tasks(tasks)
    {
    }

    void once_ms(uint32_t milliseconds, std::function<void(void)> callback)
    {
        _callback = callback;
        _ticker.once_ms(milliseconds, &Timer::TickerCallback, this);        
    }

    void attach_ms(uint32_t milliseconds, std::function<void(void)> callback)
    {
        _callback = callback;
        _ticker.attach_ms(milliseconds, &Timer::TickerCallback, this);        
    }

    void detach()
    {
        _ticker.detach();
    }
};
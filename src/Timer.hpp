#pragma once

#include <Ticker.h>
#include <functional>

/**
Wrapper class around Ticker that allows use of std::functions and thus lambdas.
*/
class Timer 
{    
    Ticker _ticker;
    std::function<void(void)> _callback;

    static void TickerCallback(Timer * timer)
    {
        timer->_callback();
    }

public:
    void once_ms(uint32_t milliseconds, std::function<void(void)> callback)
    {
        _callback = callback;
        _ticker.once_ms(milliseconds, &Timer::TickerCallback, this);        
    }

    void detach()
    {
        _ticker.detach();
    }
};
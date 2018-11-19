#pragma once

#include <Arduino.h>

enum Polarity { ActiveHigh, ActiveLow };

class DigitalInput
{
    int _channel;
    bool _state;
    bool _hasChanged;
    int _activeState;
    bool _pullup;

    void SetState(bool state)
    {
        _hasChanged = _state != state;
        _state = state;
    }

public:
    DigitalInput(int channel, Polarity polarity = Polarity::ActiveHigh, bool pullup = true)
    :   _channel(channel),
        _activeState(polarity == Polarity::ActiveHigh ? HIGH : LOW),
        _pullup(pullup)
    {
    }

    // Make the class non-copyable.
    DigitalInput(const DigitalInput&) = delete;
    DigitalInput& operator=(const DigitalInput&) = delete;

    int Channel() const { return _channel; }

    bool State() const { return _state; }

    bool HasChanged() const { return _hasChanged; }

    void Initialize()
    {
        pinMode(_channel, _pullup ? INPUT_PULLUP : INPUT);
        Scan();
    }

    void Scan()
    {        
        SetState(digitalRead(_channel) == _activeState);
    }
};

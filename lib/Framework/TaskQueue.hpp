#pragma once

#include <deque>
#include <functional>
#include <Arduino.h>

class TaskQueue
{    
    std::deque<std::function<void()>> _queue;

    /**
    Atomic (hopefully) flag indicating if there is work available.
    We still need to disable interrupts before removing from the queue.
    */
    volatile bool _tasksAvailable = false;

public:    
    void handle()
    {
        if(!_tasksAvailable) return;

        noInterrupts();

        if(_queue.empty())
        {
            _tasksAvailable = false;
            interrupts();
            
            return;
        }

        auto function = _queue.front();
        _queue.pop_front();
        _tasksAvailable = !_queue.empty();
        interrupts();    
        function();
    }

    void add(std::function<void()> function)
    {
        noInterrupts();
        _queue.push_back(function);
        _tasksAvailable = true;
        interrupts();
    }
};
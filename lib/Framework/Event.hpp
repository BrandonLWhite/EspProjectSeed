#pragma once

#include <functional>
#include <list>

template <class TEventArg> class Event 
{
public:
    typedef std::function<void(const TEventArg &)> HandlerType;
    typedef std::list<HandlerType> HandlersType;
    typedef typename HandlersType::const_iterator Connection;

    Connection connect(const HandlerType & handler)
    {        
        _handlers.push_front(handler);
        return _handlers.cbegin();
    }

    void disconnect(const Connection & connection)
    {
        _handlers.erase(connection);
    }

    void emit(const TEventArg & arg)
    {
        for(auto & handler : _handlers)
        {
            handler(arg);
        }
    }
private:    
    HandlersType _handlers;
};
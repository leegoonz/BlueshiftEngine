#pragma once

#include "Containers/Array.h"

BE_NAMESPACE_BEGIN

class Signal;
class SignalObject;
class SignalDef;

using SignalCallback = void (SignalObject::*)();

class BE_API SignalObject {
    friend class Signal;

public:
    /// Signal connection type
    enum ConnectionType {
        Direct                  = 0,        /// The callback is invoked immediately, when the signal is emitted.
        Queued                  = BIT(0),   /// The callback is queued
        Unique                  = BIT(1),   /// Connect() will fail if the same signal is already connected to the same slot for the same pair of SignalObject
    };

    SignalObject();
    virtual ~SignalObject();

                                /// Checks if a signal is already connected to the receiver object with given callback function
    bool                        IsConnected(const SignalDef *sigdef, SignalObject *receiver, SignalCallback function) const;
                                /// Checks if a signal is already connected to the receiver object
    bool                        IsConnected(const SignalDef *sigdef, SignalObject *receiver) const;

                                /// Connects a signal to receiver's callback function with given connection type
    bool                        Connect(const SignalDef *sigdef, SignalObject *receiver, SignalCallback function, int connectionType = Direct);
                                /// Disconnects a signal to receiver's callback function
    bool                        Disconnect(const SignalDef *sigdef, SignalObject *receiver, SignalCallback function);
                                /// Disconnects a signal to receiver
    bool                        Disconnect(const SignalDef *sigdef, SignalObject *receiver);
                                /// Disconnects a signal
    bool                        Disconnect(const SignalDef *sigdef);

                                /// Emits a signal
    template <typename... Args>
    bool                        EmitSignal(const SignalDef *sigdef, Args&&... args);
        
                                /// Tests if all signals are blocked on this object
    bool                        SignalsBlocked() const { return signalBlocked; }
                                /// Blocks all signals on this object
    bool                        BlockSignals(bool block);

private:
    bool                        ExecuteCallback(const SignalCallback &callback, int numArgs, intptr_t *data);
    bool                        EmitSignalArgs(const SignalDef *sigdef, int numArgs, ...);

    struct Connection {
        const SignalDef *       signalDef;
        int                     connectionType;
        SignalObject *          sender;
        SignalObject *          receiver;
        SignalCallback          function;
    };
    
    Array<Connection *>         subscriptions;
    Array<Connection *>         publications;
    bool                        signalBlocked;
};

BE_INLINE SignalObject::SignalObject() {
    signalBlocked = false;
}

template <typename... Args>
BE_INLINE bool SignalObject::EmitSignal(const SignalDef *sigdef, Args&&... args) {
    static_assert(is_assignable_all<EventArg, Args...>::value, "args is not assignable to EventArg");
    return EmitSignalArgs(sigdef, sizeof...(args), address_of(EventArg(std::forward<Args>(args)))...);
}

BE_NAMESPACE_END
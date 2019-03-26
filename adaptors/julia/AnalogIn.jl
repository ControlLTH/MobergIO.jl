mutable struct AnalogInChannel
    context::Ptr{Nothing}
    read::Ptr{Nothing}
end

mutable struct AnalogIn
    moberg::Moberg
    index::UInt32
    channel::AnalogInChannel
    AnalogIn(moberg::Moberg, index::Unsigned) = (
        channel = AnalogInChannel(0,0);
        checkOK(ccall((:moberg_analog_in_open, "libmoberg"),
                       Status,
                       (Moberg, Cint, Ref{AnalogInChannel}),
                       moberg, index, channel));
        self = new(moberg, index, channel);
        finalizer(close, self);
        self
    )
end

function close(ain::AnalogIn)
    # println("closing $(ain)")
    checkOK(ccall((:moberg_analog_in_close, "libmoberg"),
                  Status,
                  (Moberg, Cint, AnalogInChannel),
                  ain.moberg, ain.index, ain.channel))
end

function read(ain::AnalogIn)
    result = Ref{Cdouble}(0.0)
    checkOK(ccall(ain.channel.read,
                  Status,
                  (Ptr{Nothing}, Ptr{Cdouble}),
                  ain.channel.context, result))
    return result[]
end


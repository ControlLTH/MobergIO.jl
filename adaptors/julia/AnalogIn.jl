mutable struct AnalogInChannel
    context::Ptr{Nothing}
    read::Ptr{Nothing}
end

mutable struct AnalogIn
    moberg::Ptr{Nothing}
    index::UInt32
    channel::AnalogInChannel
    function AnalogIn(moberg::Moberg, index::Unsigned)
        channel = AnalogInChannel(0,0)
        moberg_handle = moberg.handle
        checkOK(ccall((:moberg_analog_in_open, "libmoberg"),
                       Status,
                       (Ptr{Nothing}, Cint, Ref{AnalogInChannel}),
                       moberg_handle, index, channel))
        self = new(moberg_handle, index, channel)
        finalizer(close, self)
        self
    end
end

function close(ain::AnalogIn)
    DEBUG && println("closing $(ain)")
    checkOK(ccall((:moberg_analog_in_close, "libmoberg"),
                  Status,
                  (Ptr{Nothing}, Cint, AnalogInChannel),
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

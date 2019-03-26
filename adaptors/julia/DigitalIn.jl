mutable struct DigitalInChannel
    context::Ptr{Nothing}
    read::Ptr{Nothing}
end

mutable struct DigitalIn
    moberg::Moberg
    index::UInt32
    channel::DigitalInChannel
    DigitalIn(moberg::Moberg, index::Unsigned) = (
        channel = DigitalInChannel(0,0);
        checkOK(ccall((:moberg_digital_in_open, "libmoberg"),
                       Status,
                       (Moberg, Cint, Ref{DigitalInChannel}),
                       moberg, index, channel));
        self = new(moberg, index, channel);
        finalizer(close, self);
        self
    )
end

function close(din::DigitalIn)
    # println("closing $(din)")
    checkOK(ccall((:moberg_digital_in_close, "libmoberg"),
                  Status,
                  (Moberg, Cint, DigitalInChannel),
                  din.moberg, din.index, din.channel))
end

function read(din::DigitalIn)
    result = Ref{Cint}(0)
    checkOK(ccall(din.channel.read,
                  Status,
                  (Ptr{Nothing}, Ptr{Cint}),
                  din.channel.context, result))
    return result[] != 0
end


mutable struct EncoderInChannel
    context::Ptr{Nothing}
    read::Ptr{Nothing}
end

mutable struct EncoderIn
    moberg::Moberg
    index::UInt32
    channel::EncoderInChannel
    function EncoderIn(moberg::Moberg, index::Unsigned)
        channel = EncoderInChannel(0,0)
        checkOK(ccall((:moberg_encoder_in_open, "libmoberg"),
                       Status,
                       (Moberg, Cint, Ref{EncoderInChannel}),
                       moberg, index, channel))
        self = new(moberg, index, channel)
        finalizer(close, self)
        self
    end
end

function close(ein::EncoderIn)
    # println("closing $(ein)")
    checkOK(ccall((:moberg_encoder_in_close, "libmoberg"),
                  Status,
                  (Moberg, Cint, EncoderInChannel),
                  ein.moberg, ein.index, ein.channel))
end

function read(ein::EncoderIn)
    result = Ref{Clong}(0)
    checkOK(ccall(ein.channel.read,
                  Status,
                  (Ptr{Nothing}, Ptr{Clong}),
                  ein.channel.context, result))
    return result[]
end

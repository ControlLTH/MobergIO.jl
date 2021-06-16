"""
    encoder_in = EncoderIn(moberg::Moberg, index::Unsigned)

Example usage:

    encoder_in = EncoderIn(m, UInt32(40))
    result = read(encoder_in)
"""
mutable struct EncoderIn <: AbstractMobergIn
    moberg::Ptr{Nothing}
    index::UInt32
    channel::MobergInChannel
    function EncoderIn(moberg::Moberg, index::Unsigned)
        channel = MobergInChannel(0,0)
        moberg_handle = moberg.handle
        checkOK(ccall((:moberg_encoder_in_open, "libmoberg"),
                       Status,
                       (Ptr{Nothing}, Cint, Ref{MobergInChannel}),
                       moberg_handle, index, channel))
        self = new(moberg_handle, index, channel)
        finalizer(close, self)
        self
    end
end

function close(ein::EncoderIn)
    DEBUG && println("closing $(ein)")
    checkOK(ccall((:moberg_encoder_in_close, "libmoberg"),
                  Status,
                  (Ptr{Nothing}, Cint, MobergInChannel),
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

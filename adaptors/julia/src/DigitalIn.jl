mutable struct DigitalIn <: AbstractMobergIn
    moberg::Ptr{Nothing}
    index::UInt32
    channel::MobergInChannel
    function DigitalIn(moberg::Moberg, index::Unsigned)
        channel = MobergInChannel(0,0)
        moberg_handle = moberg.handle
        checkOK(ccall((:moberg_digital_in_open, "libmoberg"),
                       Status,
                       (Ptr{Nothing}, Cint, Ref{MobergInChannel}),
                       moberg_handle, index, channel));
        self = new(moberg_handle, index, channel)
        finalizer(close, self)
        self
    end
end

function close(din::DigitalIn)
    DEBUG && println("closing $(din)")
    checkOK(ccall((:moberg_digital_in_close, "libmoberg"),
                  Status,
                  (Ptr{Nothing}, Cint, MobergInChannel),
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

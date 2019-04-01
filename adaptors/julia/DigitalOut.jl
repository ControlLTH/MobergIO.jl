mutable struct DigitalOutChannel
    context::Ptr{Nothing}
    write::Ptr{Nothing}
end

mutable struct DigitalOut
    moberg::Ptr{Nothing}
    index::UInt32
    channel::DigitalOutChannel
    function DigitalOut(moberg::Moberg, index::Unsigned)
        channel = DigitalOutChannel(0,0)
        moberg_handle = moberg.handle
        checkOK(ccall((:moberg_digital_out_open, "libmoberg"),
                       Status,
                       (Ptr{Nothing}, Cint, Ref{DigitalOutChannel}),
                       moberg_handle, index, channel))
        self = new(moberg_handle, index, channel)
        finalizer(close, self)
        self
    end
end

function close(dout::DigitalOut)
    DEBUG && println("closing $(dout)")
    checkOK(ccall((:moberg_digital_out_close, "libmoberg"),
                  Status,
                  (Ptr{Nothing}, Cint, DigitalOutChannel),
                  dout.moberg, dout.index, dout.channel))
end

function write(dout::DigitalOut, value::Bool)
    result = Ref{Cint}(0)
    checkOK(ccall(dout.channel.write,
                  Status,
                  (Ptr{Nothing}, Cint, Ptr{Cint}),
                  dout.channel.context, value ? 1 : 0, result))
    return result[] != 0
end

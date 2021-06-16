mutable struct AnalogOut <: AbstractMobergOut
    moberg::Ptr{Nothing}
    index::UInt32
    channel::MobergOutChannel
    function AnalogOut(moberg::Moberg, index::Unsigned)
        channel = MobergOutChannel(0,0)
        moberg_handle = moberg.handle
        checkOK(ccall((:moberg_analog_out_open, "libmoberg"),
                       Status,
                       (Ptr{Nothing}, Cint, Ref{MobergOutChannel}),
                       moberg_handle, index, channel));
        self = new(moberg_handle, index, channel)
        finalizer(close, self)
        self
    end
end

function close(aout::AnalogOut)
    DEBUG && println("closing $(aout)")
    checkOK(ccall((:moberg_analog_out_close, "libmoberg"),
                  Status,
                  (Ptr{Nothing}, Cint, MobergOutChannel),
                  aout.moberg, aout.index, aout.channel))
end

function write(aout::AnalogOut, value::Cdouble)
    result = Ref{Cdouble}(0.0)
    checkOK(ccall(aout.channel.write,
                  Status,
                  (Ptr{Nothing}, Cdouble, Ptr{Cdouble}),
                  aout.channel.context, value, result))
    return result[];
end

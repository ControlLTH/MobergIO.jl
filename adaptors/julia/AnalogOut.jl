mutable struct AnalogOutChannel
    context::Ptr{Nothing}
    write::Ptr{Nothing}
end

mutable struct AnalogOut
    moberg::Moberg
    index::UInt32
    channel::AnalogOutChannel
    function AnalogOut(moberg::Moberg, index::Unsigned)
        channel = AnalogOutChannel(0,0)
        checkOK(ccall((:moberg_analog_out_open, "libmoberg"),
                       Status,
                       (Moberg, Cint, Ref{AnalogOutChannel}),
                       moberg, index, channel));
        self = new(moberg, index, channel)
        finalizer(close, self)
        self
    end
end

function close(aout::AnalogOut)
    # println("closing $(aout)")
    checkOK(ccall((:moberg_analog_out_close, "libmoberg"),
                  Status,
                  (Moberg, Cint, AnalogOutChannel),
                  aout.moberg, aout.index, aout.channel))
end

function write(aout::AnalogOut, value::Cdouble)
    checkOK(ccall(aout.channel.write,
                  Status,
                  (Ptr{Nothing}, Cdouble),
                  aout.channel.context, value))
end

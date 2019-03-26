mutable struct DigitalOutChannel
    context::Ptr{Nothing}
    write::Ptr{Nothing}
end

mutable struct DigitalOut
    moberg::Moberg
    index::UInt32
    channel::DigitalOutChannel
    DigitalOut(moberg::Moberg, index::Unsigned) = (
        channel = DigitalOutChannel(0,0);
        checkOK(ccall((:moberg_digital_out_open, "libmoberg"),
                       Status,
                       (Moberg, Cint, Ref{DigitalOutChannel}),
                       moberg, index, channel));
        self = new(moberg, index, channel);
        finalizer(close, self);
        self
    )
end

function close(dout::DigitalOut)
    # println("closing $(dout)")
    checkOK(ccall((:moberg_digital_out_close, "libmoberg"),
                  Status,
                  (Moberg, Cint, DigitalOutChannel),
                  dout.moberg, dout.index, dout.channel))
end;

function write(dout::DigitalOut, value::Bool)
    checkOK(ccall(dout.channel.write,
                  Status,
                  (Ptr{Nothing}, Cint),
                  dout.channel.context, value ? 1 : 0))
end

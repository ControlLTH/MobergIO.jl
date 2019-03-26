module MobergIO


struct Status
    result::Clong
end

function checkOK(status::Status)
    if status.result != 0
        error("Moberg call failed with errno $(status.result)")
    end
end

mutable struct Moberg
    handle::Ptr{Nothing}
end

function Moberg() 
    handle = ccall((:moberg_new, "libmoberg"), Ptr{Moberg}, ())
    m = Moberg(handle)
    finalizer(close, m)
    m
end

function close(h::Moberg)
    ccall((:moberg_free, "libmoberg"), Nothing, (Moberg,), h)
end

include("AnalogIn.jl")
include("AnalogOut.jl")
include("DigitalIn.jl")
include("DigitalOut.jl")
include("EncoderIn.jl")

end

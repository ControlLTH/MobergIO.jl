module MobergIO

export Moberg

import Base: close, read, write

abstract type AbstractMobergIO end
abstract type AbstractMobergIn <: AbstractMobergIO end
abstract type AbstractMobergOut <: AbstractMobergIO end

"""
    close(io::AbstractMobergIO)
""" close

"""
    result = read(io::AbstractMobergIn)
""" read

"""
    write(io::AbstractMobergIn)
""" write

const DEBUG = false

struct Status
    result::Clong
end

function checkOK(status::Status)
    if status.result != 0
        error("Moberg call failed with errno $(status.result)")
    end
end

mutable struct MobergOutChannel
    context::Ptr{Nothing}
    write::Ptr{Nothing}
end

mutable struct MobergInChannel
    context::Ptr{Nothing}
    read::Ptr{Nothing}
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
    DEBUG && println("Destroy $(h)")
    ccall((:moberg_free, "libmoberg"), Nothing, (Moberg,), h)
    h.handle = Ptr{Nothing}(0)
end

include("AnalogIn.jl")
include("AnalogOut.jl")
include("DigitalIn.jl")
include("DigitalOut.jl")
include("EncoderIn.jl")

end

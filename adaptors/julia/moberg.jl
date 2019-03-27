#!/usr/bin/julia

struct MobergStatus
    result::Clong
end

function checkOK(status::MobergStatus)
    if status.result != 0
        error("Moberg call failed with errno $(status.result)")
    end
end

mutable struct Moberg
    handle::Ptr{Nothing}
end

function Moberg()
    handle = ccall((:moberg_new, "libmoberg"), Ptr{Nothing}, ())
    println(handle)
    m = Moberg(handle)
    finalizer(m) do h
        println(h)
        ccall((:moberg_free, "libmoberg"), Nothing, (Ptr{Nothing},),
              h.handle)
    end
    m
end

mutable struct MobergAnalogIn
    context::Ptr{Nothing}
    do_read::Ptr{Nothing}
    handle::Moberg
    function MobergAnalogIn(m::Moberg, index::Unsigned)
        self = new()
        checkOK(ccall((:moberg_analog_in_open, "libmoberg"),
                       MobergStatus,
                       (Ptr{Nothing}, Cint, Ref{MobergAnalogIn}),
                       m.handle, index, self))
        self.handle = m
        finalizer(self) do channel
            println(channel)
            ccall((:moberg_analog_in_close, "libmoberg"),
                  MobergStatus,
                  (Ptr{Nothing}, Cint, MobergAnalogIn),
                  channel.handle.handle, index, self)
        end
        self
    end
end

mutable struct MobergAnalogOut
    context::Ptr{Nothing}
    do_write::Ptr{Nothing}
    handle::Moberg
    function MobergAnalogOut(m::Moberg, index::Unsigned)
        self = new()
        checkOK(ccall((:moberg_analog_out_open, "libmoberg"),
                       MobergStatus,
                       (Ptr{Nothing}, Cint, Ref{MobergAnalogOut}),
                       m.handle, index, self))
        self.handle = m
        finalizer(self) do channel
            println(channel)
            ccall((:moberg_analog_out_close, "libmoberg"),
                  MobergStatus,
                  (Ptr{Nothing}, Cint, MobergAnalogOut),
                  channel.handle.handle, index, self)
        end
        self
    end
end

function read(ain::MobergAnalogIn)
    result = Ref{Cdouble}(0.0)
    checkOK(ccall(ain.do_read,
                   MobergStatus,
                   (Ptr{Nothing}, Ptr{Cdouble}),
                   ain.context, result))
    return result[]
end

function write(aout::MobergAnalogOut, value::Cdouble)
    checkOK(ccall(aout.do_write,
                   MobergStatus,
                   (Ptr{Nothing}, Cdouble),
                   aout.context, value))
end

function test()
    m = Moberg()
    println(m)

    for v in -10.0:2.0:10
        for i in 30:31
            try
                aout = MobergAnalogOut(m, Unsigned(i))
                value = v + i - 32
                write(aout, value)
                print("$value ")
            catch
                println("analog_out $i does not exist")
            end
        end
        println()
        sleep(0.1)
        for j in 30:33
            try
                ain = MobergAnalogIn(m, Unsigned(j))
                println(read(ain))
            catch
                println("analog_in $j does not exist")
            end
        end
        println()
        GC.gc()
    end
end

test()

println("DONE")
GC.gc()
GC.gc() # See https://github.com/JuliaCI/BenchmarkTools.jl/blob/af35d0513fe1e336ad0d8b9a35f924e8461aefa2/src/execution.jl#L1
println("Really DONE")

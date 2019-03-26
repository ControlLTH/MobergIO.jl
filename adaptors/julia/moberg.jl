#!/usr/bin/julia

struct moberg_status
    result::Clong
end

function check_OK(status::moberg_status)
    if status.result != 0
        error("Moberg call failed with errno $(status.result)")
    end
end

mutable struct moberg
    handle::Ptr{Nothing}
end

function moberg() 
    handle = ccall((:moberg_new, "libmoberg"), Ptr{Nothing}, ())
    println(handle)
    m = moberg(handle)
    function close(h::moberg)
        println(h)
        ccall((:moberg_free, "libmoberg"), Nothing, (Ptr{Nothing},),
              h.handle)
    end
    finalizer(close, m)
    m
end

mutable struct moberg_analog_in
    context::Ptr{Nothing}
    do_read::Ptr{Nothing}
    handle::moberg
    moberg_analog_in(m::moberg, index::Unsigned) = (
        self = new();
        check_OK(ccall((:moberg_analog_in_open, "libmoberg"),
                       moberg_status,
                       (Ptr{Nothing}, Cint, Ref{moberg_analog_in}),
                       m.handle, index, self));
        self.handle = m;
        function close(channel::moberg_analog_in)
            println(channel)
            ccall((:moberg_analog_in_close, "libmoberg"),
                  moberg_status,
                  (Ptr{Nothing}, Cint, moberg_analog_in),
                  channel.handle.handle, index, self)
        end;
        finalizer(close, self);
        self
    )
end

mutable struct moberg_analog_out
    context::Ptr{Nothing}
    do_write::Ptr{Nothing}
    handle::moberg
    moberg_analog_out(m::moberg, index::Unsigned) = (
        self = new();
        check_OK(ccall((:moberg_analog_out_open, "libmoberg"),
                       moberg_status,
                       (Ptr{Nothing}, Cint, Ref{moberg_analog_out}),
                       m.handle, index, self));
        self.handle = m;
        function close(channel::moberg_analog_out)
            println(channel)
            ccall((:moberg_analog_out_close, "libmoberg"),
                  moberg_status,
                  (Ptr{Nothing}, Cint, moberg_analog_out),
                  channel.handle.handle, index, self)
        end;
        finalizer(close, self);
        self
    )
end

function read(ain::moberg_analog_in)
    result = Ref{Cdouble}(0.0)
    check_OK(ccall(ain.do_read,
                   moberg_status,
                   (Ptr{Nothing}, Ptr{Cdouble}),
                   ain.context, result))
    return result[]
end

function write(aout::moberg_analog_out, value::Cdouble)
    check_OK(ccall(aout.do_write,
                   moberg_status,
                   (Ptr{Nothing}, Cdouble),
                   aout.context, value))
end

function test()
    m = moberg()
    println(m)
    
    for v in -10.0:2.0:10 
        for i in 30:31
            try
                aout = moberg_analog_out(m, Unsigned(i))
                value = v + i - 32;
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
                ain = moberg_analog_in(m, Unsigned(j))
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
println("Really DONE")

#!/usr/bin/julia

push!(LOAD_PATH, ".")

using MobergIO
import MobergIO: read, write

function test()
    m = MobergIO.Moberg()
    println(stderr,m)
    
    for v in -10.0:2.0:10 
        for i in 0:1
            try
                aout = MobergIO.AnalogOut(m, Unsigned(i))
                value = v + i;
                write(aout, value)
                print("$value ")
            catch ex
                println(stderr,"analog_out $i does not exist $(ex)")
            end
        end
        println(stderr)
        flush(stderr)
        sleep(0.01)
        for j in 0:3
            try
                ain = MobergIO.AnalogIn(m, Unsigned(j))
                println(stderr,read(ain))
            catch ex
                println(stderr,"analog_in $j does not exist $(ex)")
            end
            flush(stderr)
        end
        println(stderr)
        flush(stderr)
    end
    for v in false:true
        for i in 0:6
            try
                dout = MobergIO.DigitalOut(m, Unsigned(i))
                value = xor(v, isodd(i))
                write(dout, value)
                print("$value ")
            catch ex
                println(stderr,"digital_out $i does not exist $(ex)")
            end
            flush(stderr)
        end
        println(stderr)
        flush(stderr)
        for i in 0:6
            try
                din = MobergIO.DigitalIn(m, Unsigned(i))
                print("$(read(din)) ")
            catch ex
                println(stderr,"digital_in $i does not exist $(ex)")
            end
            flush(stderr)
        end
        println(stderr)
        println(stderr)
        sleep(0.01)
    end
end

test()

println(stderr,"DONE")
flush(stderr)
GC.gc()
println(stderr,"....")
flush(stderr)
GC.gc() # See https://github.com/JuliaCI/BenchmarkTools.jl/blob/af35d0513fe1e336ad0d8b9a35f924e8461aefa2/src/execution.jl#L1
println(stderr,"Really DONE")
flush(stderr)

#!/usr/bin/julia

push!(LOAD_PATH, ".")

using MobergIO
import MobergIO: read, write

function test()
    m = MobergIO.Moberg()
    println(m)
    
    for v in -10.0:2.0:10 
        for i in 0:1
            try
                aout = MobergIO.AnalogOut(m, Unsigned(i))
                value = v + i;
                write(aout, value)
                print("$value ")
            catch ex
                println("analog_out $i does not exist $(ex)")
            end
        end
        println()
        sleep(0.01)
        for j in 0:3
            try
                ain = MobergIO.AnalogIn(m, Unsigned(j))
                println(read(ain))
            catch ex
                println("analog_in $j does not exist $(ex)")
            end
        end
        println()
#        GC.gc()
    end
    for v in false:true
        for i in 0:6
            try
                dout = MobergIO.DigitalOut(m, Unsigned(i))
                value = xor(v, isodd(i))
                write(dout, value)
                print("$value ")
            catch ex
                println("digital_out $i does not exist $(ex)")
            end
        end
        println()
        for i in 0:6
            try
                din = MobergIO.DigitalIn(m, Unsigned(i))
                print("$(read(din)) ")
            catch ex
                println("digital_out $i does not exist $(ex)")
            end
        end
        println()
        println()
        sleep(0.01)
    end
end

test()

println("DONE")
GC.gc()
GC.gc() # See https://github.com/JuliaCI/BenchmarkTools.jl/blob/af35d0513fe1e336ad0d8b9a35f924e8461aefa2/src/execution.jl#L1
println("Really DONE")

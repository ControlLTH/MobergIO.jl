#!/usr/bin/julia

push!(LOAD_PATH, ".")

using MobergIO
import MobergIO: read, write

function test()
    m = MobergIO.Moberg()
    println(m)
    
    for v in -10.0:2.0:10 
        for i in 30:31
            try
                aout = MobergIO.AnalogOut(m, Unsigned(i))
                value = v + i - 32;
                write(aout, value)
                print("$value ")
            catch ex
                println("analog_out $i does not exist $(ex)")
            end
        end
        println()
        sleep(0.01)
        for j in 30:33
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
        for i in 30:36
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
        for i in 30:36
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
println("Really DONE")

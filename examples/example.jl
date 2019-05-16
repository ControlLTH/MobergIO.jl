import MobergIO
using MobergIO: read, write

function scan()
    result = [[], [], [], [], []]
    m = MobergIO.Moberg() 
    for (i, f) = enumerate([MobergIO.AnalogIn, MobergIO.AnalogOut,
                          MobergIO.DigitalIn, MobergIO.DigitalOut,
                          MobergIO.EncoderIn])
        for j in range(0, stop=100)
            try
                push!(result[i], ((j, f(m, Unsigned(j)))))
            catch ex
            end
        end
    end
    result
end
            
function main()
    channels = scan()
    for c = channels
        println(length(c))
    end
    for c = channels[2]
        println("$(c[1]) $(write(c[2], Inf)), $(write(c[2], -Inf)), $(write(c[2], 0.0))")
    end
end
    
main()

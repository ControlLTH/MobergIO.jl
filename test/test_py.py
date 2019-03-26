import moberg

m = moberg.Moberg()

ain = [ m.analog_in(i) for i in range(8) ]
aout = [ m.analog_out(i) for i in range(8) ]
din = [ m.digital_in(i) for i in range(8) ]
dout = [ m.digital_out(i) for i in range(8) ]
ein = [ m.encoder_in(i) for i in range(8) ]
for v in range(10):
    aout[0].write(v)
    print([ c.read() for c in ain ])
for v in range(10):
    for c,w in zip(dout, range(100)):
        dout[w].write(v & (1<<w))
    print([ c.read() for c in din ])
    print([ c.read() for c in ein ])
    pass


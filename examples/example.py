#!/usr/bin/python3

import moberg
import math

def scan():
    result = [[], [], [], [], []]
    m = moberg.Moberg()
    for i, f in enumerate([m.analog_in, m.analog_out,
                           m.digital_in, m.digital_out,
                           m.encoder_in]):
        for j in range(0, 100):
            try:
                result[i].append((j, f(j)))
                pass
            except:
                pass
            pass
        pass
    return result

def main():
    channels = scan()
    for c in channels:
        print(len(c))
        pass
    for c in channels[1]:
        print(c[0], c[1].write(math.inf), c[1].write(-math.inf), c[1].write(0))
    pass
    
if __name__ == '__main__':
    main()

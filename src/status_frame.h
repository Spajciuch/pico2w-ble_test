#include <main.cpp>

class StatusFrame {
    StatusFrame(RadioStatus s)
    {
        //data[0] = (s.volume << 2) | ((s.frequency >> 9) & 0x3);
        //data[1] = (s.frequency & 0x1FD)>>1 ;
        //data[2] = (s.frequency & 0x1 << 7) | (s.rssi & 0x80);

        // reszta to RDS z obciętym /0 na końcu
    }

    const uint8_t data[11];
};
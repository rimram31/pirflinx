#include <iostream>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "OokDecoder.h"
#include "AlectoDecoder.h"

AlectoDecoderV4::AlectoDecoderV4 () {
    decoderName = "ALECTO_V4";
}

AlectoDecoderV4::AlectoDecoderV4 (OokDecoder *pdecoder) {
    decoderName = "ALECTO_V4";
    copyDecoder(pdecoder);
}

AlectoDecoderV4::~AlectoDecoderV4 () {}

int AlectoDecoderV4::decode (uint32_t width) {
    
    switch (state) {
        case UNKNOWN:
        case OK:
            if (width > 3000 && width <= 4000) state = T1;
            else if (width > 1500 && width <= 2100) state = T0;
            else return -1;
            break;
        case T0:
        case T1:
            if (width > 550) return -1;
            gotBit(state==T1 ? 1: 0);
            break;
        case READY_FOR_STOP_BIT:
            if (width > 4000) return 1;
            else return -1;
    }
    if (total_bits == 36) state = READY_FOR_STOP_BIT;
    return 0;
}

char *AlectoDecoderV4::display () {
    byte rc=0;
    byte rc2=0;
    int temperature=0;
    int humidity=0;
    
    reverseBits();
    
    rc = data[0];
    rc2 = data[1];
    temperature = (data[2] << 4) + ((data[3] & 0xf0) >> 4);
    humidity = ((data[3] & 0x0f) << 4) + ((data[4] & 0xf0) >> 4);
    
    /**
    //fix 12 bit signed number conversion
    if ((temperature & 0x800) == 0x800) {
     temperature=4096-temperature;                 // fix for minus temperatures
     if (temperature > 0x258) return false;        // temperature out of range ( > 60.0 degrees) 
        temperature=temperature | 0x8000;             // turn highest bit on for minus values
    } else {
        if (temperature > 0x258) return false;        // temperature out of range ( > 60.0 degrees) 
    }**/
      
    sprintf(pbuffer, "20;%02X;Alecto V4;ID=%02x%02x;TEMP=%04x;HUM=%02d;\n",0,rc,rc2,temperature,humidity);    
    return pbuffer;
}

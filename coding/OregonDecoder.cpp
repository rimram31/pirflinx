#include <iostream>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "OokDecoder.h"
#include "OregonDecoder.h"

OregonDecoder::OregonDecoder () {
}
OregonDecoder::~OregonDecoder () {}

char *OregonDecoder::display() {
    uint8_t osdata[13];               

    byte rc=0;
    int temp = 0;
    byte hum = 0;
    int comfort = 0;
    int baro = 0;
    int forecast = 0;
    int uv = 0;
    int wdir = 0;
    int wspeed = 0;
    int awspeed = 0;
    int rain = 0;
    int raintot = 0;
    
    // Start
    pbuffer[0] = '\0';
    
	const byte* data = getData(pos);
	for (byte i = 0; i < pos; ++i) {
        if (i < 13) osdata[i]=data[i];
	}
    
    unsigned int id=(osdata[0]<<8)+ (osdata[1]);
    rc=osdata[0];
    // ==================================================================================
    // Process the various device types:
    // ==================================================================================
    // Oregon V1 packet structure
    // SL-109H, AcuRite 09955
    // TEMP + CRC
    // ==================================================================================
    // 8487101C
    // 84+87+10=11B > 1B+1 = 1C
    if (!strcmp(decoderName,"OREGON_V1")) {       // OSV1 
        int sum = osdata[0]+osdata[1]+osdata[2];  // max. value is 0x2FD
        sum= (sum &0xff) + (sum>>8);              // add overflow to low byte
        if (osdata[3] != (sum & 0xff) ) {
            //Serial.println("CRC Error"); 
            return "";
        }
        // -------------       
        //temp = ((osdata[2]>>4) * 100)  + ((osdata[1] & 0x0F) * 10) + ((osdata[1] >> 4));
        //if ((osdata[2] & 0x02) == 2) temp=temp | 0x8000;  // bit 1 set when temp is negative, set highest bit on temp valua
        temp = ((osdata[2] & 0x0F) * 100)  + ((osdata[1] >> 4) * 10) + ((osdata[1] & 0x0F));
        if ((osdata[2] & 0x20) == 0x20) temp=temp | 0x8000;  // bit 1 set when temp is negative, set highest bit on temp valua
        // ----------------------------------
        // Output
        // ----------------------------------
        sprintf(pbuffer,"20;%02X;OregonV1;ID=00%02X;TEMP=%04x;BAT=%s;\n",0,(rc)&0xcf,temp,(osdata[2] & 0x80) ? "LOW":"OK");
    } 
    // ==================================================================================
    // ea4c  Outside (Water) Temperature: THC238, THC268, THN132N, THWR288A, THRN122N, THN122N, AW129, AW131
    // TEMP + BAT + CRC
    // ca48  Pool (Water) Temperature: THWR800
    // 0a4d  Indoor Temperature: THR128, THR138, THC138 
    // ==================================================================================
    // OSV2 EA4C20725C21D083 // THN132N
    // OSV2 EA4C101360193023 // THN132N
    // OSV2 EA4C40F85C21D0D4 // THN132N
    // OSV2 EA4C20809822D013
    //      0123456789012345
    //      0 1 2 3 4 5 6 7
    if (id == 0xea4c || id == 0xca48 || id == 0x0a4d) {
        byte sum=(osdata[7]&0x0f) <<4; 
        sum=sum+(osdata[6]>>4);
        // -------------       
        temp = ((osdata[5]>>4) * 100)  + ((osdata[5] & 0x0F) * 10) + ((osdata[4] >> 4));
        if ((osdata[6] & 0x0F) >= 8) temp=temp | 0x8000;
        // ----------------------------------
        // Output
        // ----------------------------------
        sprintf(pbuffer,"20;%02X;Oregon Temp;ID=%02X%02X;TEMP=%04x;BAT=%s;\n",0,osdata[3],osdata[2],temp,((osdata[4] & 0x0c) >= 4) ? "LOW":"OK");
    } else
    // ==================================================================================
    // 1a2d  Indoor Temp/Hygro: THGN122N, THGN123N, THGR122NX, THGR228N, THGR238, THGR268, THGR122X
    // 1a3d  Outside Temp/Hygro: THGR918, THGRN228NX, THGN500
    // fa28  Indoor Temp/Hygro: THGR810
    // *aac  Outside Temp/Hygro: RTGR328N
    // ca2c  Outside Temp/Hygro: THGR328N
    // fab8  Outside Temp/Hygro: WTGR800
    // TEMP + HUM sensor + BAT + CRC
    // ==================================================================================
    // OSV2 AACC13783419008250AD[RTGR328N,...] Id:78 ,Channel:0 ,temp:19.30 ,hum:20 ,bat:10
    // OSV2 1A2D40C4512170463EE6[THGR228N,...] Id:C4 ,Channel:3 ,temp:21.50 ,hum:67 ,bat:90
    // OSV2 1A2D1072512080E73F2C[THGR228N,...] Id:72 ,Channel:1 ,temp:20.50 ,hum:78 ,bat:90
    // OSV2 1A2D103742197005378E // THGR228N
    // OSV3 FA28A428202290834B46 // 
    // OSV3 FA2814A93022304443BE // THGR810
    // OSV2 1A2D1002 02060552A4C
    //      1A3D10D91C273083..
    //      1A3D10D90C284083..
    //      01234567890123456789
    //      0 1 2 3 4 5 
    // F+A+2+8+1+4+A+9+3+0+2+2+3+0+4+4=4d-a=43 
    if (id == 0xfa28 || id == 0x1a2d || id == 0x1a3d || (id&0xfff)==0xACC || id == 0xca2c || id == 0xfab8 ) {
        struct toto {
             int value;
        };
        // -------------       
        temp = ((osdata[5]>>4) * 100)  + ((osdata[5] & 0x0F) * 10) + ((osdata[4] >> 4));
        if ((osdata[6] & 0x0F) >= 8) temp=temp | 0x8000;
        // -------------       
        hum = ((osdata[7] & 0x0F)*16)+ (osdata[6] >> 4);
        // ----------------------------------
        // Output
        // ----------------------------------
        sprintf(pbuffer,"20;%02X;Oregon TempHygro;ID=%02X%02X;TEMP=%04x;HUM=%02x;BAT=%s;\n",0,osdata[1],osdata[3],temp,hum,((osdata[4] & 0x0F) >= 4) ? "LOW":"OK");
    } else
        // ==================================================================================
        // 5a5d  Indoor Temp/Hygro/Baro: Huger - BTHR918
        // 5a6d  Indoor Temp/Hygro/Baro: BTHR918N, BTHR968. BTHG968 
        // TEMP + HUM + BARO + FORECAST + BAT
        // NO CRC YET
        // ==================================================================================
        // 5A 6D 00 7A 10 23 30 83 86 31
        // 5+a+6+d+7+a+1+2+3+3+8+3=47 -a=3d  +8=4f +8+6=55      
        // 5+a+6+d+7+a+1+2+3+3=3c-a=32
        // 5+a+6+d+7+a+1+2+3+3+0+8+3+8+6=55 -a =4b +3=4e !=1
        // 0  1  2  3  4  5  6  7  8  9
        if(id == 0x5a6d || id == 0x5a5d || id == 0x5d60) {
        // -------------       
        temp = ((osdata[5]>>4) * 100)  + ((osdata[5] & 0x0F) * 10) + ((osdata[4] >> 4));
        if ((osdata[6] & 0x0F) >= 8) temp=temp | 0x8000;
        // -------------       
        hum = ((osdata[7] & 0x0F)*10)+ (osdata[6] >> 4);
        
        //0: normal, 4: comfortable, 8: dry, C: wet
        int tmp_comfort = osdata[7] >> 4;
        if (tmp_comfort == 0x00)
			comfort=0;
		else if (tmp_comfort == 0x04)
			comfort=1;
		else if (tmp_comfort == 0x08)
			comfort=2;
		else if (tmp_comfort == 0x0C)
			comfort=3;
			
        // -------------       
        baro = (osdata[8] + 856);  // max value = 1111 / 0x457
        
        //2: cloudy, 3: rainy, 6: partly cloudy, C: sunny
        int tmp_forecast = osdata[9]>>4;
        if (tmp_forecast == 0x02)
			forecast = 3;
        else if (tmp_forecast == 0x03)
			forecast = 4;
        else if (tmp_forecast == 0x06)
			forecast = 2;
        else if (tmp_forecast == 0x0C)
			forecast = 1;
        
        // ----------------------------------
        // Output
        // ----------------------------------
        sprintf(pbuffer,"20;%02X;Oregon BTHR;ID=%02X%02X;TEMP=%04x;HUM=%02x;HSTATUS=%d;BARO=%04x;BFORECAST=%d;\n",0,rc,osdata[2],temp,hum,comfort,baro,forecast);
    }
    return pbuffer;
    
}

OregonDecoderV1::OregonDecoderV1 () {
    decoderName = "OREGON_V1";
}
OregonDecoderV1::OregonDecoderV1 (OokDecoder *pdecoder) {
    decoderName = "OREGON_V1";
    copyDecoder(pdecoder);
}
OregonDecoderV1::~OregonDecoderV1 () {}

int OregonDecoderV1::decode(uint32_t width) {
    if (900 <= width && width < 3400) {
        byte w = width >= 2000;
        switch (state) {
            case UNKNOWN:                   // Detect preamble
                if (w == 0)  ++flip;
                else         return -1;
                break;
            case OK:
                if (w == 0)  state = T0;
                else         manchester(1);
                break;
            case T0:
                if (w == 0)  manchester(0);
                else         return -1;
                break;
            default:						// Unexpected state
                return -1;                                
        }
        return (pos == 4) ? 1 : 0; // Messages are fixed-size
    }
    if (width >= 3400) {
        if (flip < 10 || flip > 50)
            return -1; // No preamble
        switch (state) {
            case UNKNOWN:
                // First sync pulse, lowering edge
                state = T1;
                break;
            case T1:
                // Second sync pulse, lowering edge
                state = T2;
                break;
            case T2:
                // Last sync pulse, determines the first bit!
                if (width <= 5900) {
                    state = T0;
                    flip = 1;
                } else {
                    state = OK;
                    flip = 0;
                    manchester(0);
                }
            break;
        }
        return 0;
    }
    return -1;
}

OregonDecoderV2::OregonDecoderV2 () {
    decoderName = "OREGON_V2";
}
OregonDecoderV2::OregonDecoderV2 (OokDecoder *pdecoder) {
    decoderName = "OREGON_V2";
    copyDecoder(pdecoder);
}
OregonDecoderV2::~OregonDecoderV2 () {}

// -------------------------------------
// add one bit to the packet data buffer
// -------------------------------------
void OregonDecoderV2::gotBit(char value) {
    if (!(total_bits & 0x01)) {
        data[pos] = (data[pos] >> 1) | (value ? 0x80 : 00);
    }
    total_bits++;
    pos = total_bits >> 4;
    if (pos >= sizeof data) {
        resetDecoder();
        return;
    }
    state = OK;
}
// -------------------------------------
int OregonDecoderV2::decode(uint32_t width) {
    //if (200 <= width && width < 1200) {
    //	byte w = width >= 675;
    if (150 <= width && width < 1200) {
        byte w = width >= 600;
        switch (state) {
            case UNKNOWN:
                if (w != 0) {
                    // Long pulse
                    ++flip;
                } else if (w == 0 && 24 <= flip) {
                    // Short pulse, start bit
                    flip = 0;
                    state = T0;
                } else {
                    // Reset decoder
                    return -1;
                }
                break;
            case OK:
                if (w == 0) {
                    // Short pulse
                    state = T0;
                } else {
                    // Long pulse
                    manchester(1);
                }
                break;
            case T0:
                if (w == 0) {
                    // Second short pulse
                    manchester(0);
                } else {
                    // Reset decoder
                    return -1;
                }
                break;
        }
        //printf("width = %d - state = %d - flip %d\n",width,state,flip);
    } else if (width >= 2500 && pos >= 8) {
        return 1;
    } else {
        return -1;
    }
    return total_bits == 160 ? 1 : 0;
}

OregonDecoderV3::OregonDecoderV3 () {
    decoderName = "OREGON_V3";
}
OregonDecoderV3::OregonDecoderV3 (OokDecoder *pdecoder) {
    decoderName = "OREGON_V3";
    copyDecoder(pdecoder);
}
OregonDecoderV3::~OregonDecoderV3 () {}

// -------------------------------------
// add one bit to the packet data buffer
// -------------------------------------
void OregonDecoderV3::gotBit(char value) {
    data[pos] = (data[pos] >> 1) | (value ? 0x80 : 00);
    total_bits++;
    pos = total_bits >> 3;
    if (pos >= sizeof data) {
        resetDecoder();
        return;
    }
    state = OK;
}
// -------------------------------------
int OregonDecoderV3::decode(uint32_t width) {
    if (200 <= width && width < 1200) {
        byte w = width >= 675;
        switch (state) {
            case UNKNOWN:
                if (w == 0)  ++flip;
                else if (32 <= flip) {
                    flip = 1;
                    manchester(1);
                } else  return -1;
                break;
            case OK:
                if (w == 0)  state = T0;
                else         manchester(1);
                break;
            case T0:
                if (w == 0)  manchester(0);
                else         return -1;
                break;
        }
    } else {
        return -1;
    }
    return  total_bits == 80 ? 1 : 0;
}

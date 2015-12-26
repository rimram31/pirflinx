#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "OokDecoder.h"
#include "KakuDecoder.h"

KakuDecoder::KakuDecoder () {
    decoderName = "KAKU";
}

KakuDecoder::KakuDecoder (OokDecoder *pdecoder) {
    decoderName = "KAKU";
    copyDecoder(pdecoder);
}

KakuDecoder::~KakuDecoder () {}

int KakuDecoder::decode (uint32_t width) {
    if ( ( 180 <= width && width < 450) || (850 <= width && width < 1250) ) {
        byte w = width >= 700;
        switch (state) {
            case UNKNOWN:
            case OK:
                if (w == 0)   state = T0;
                else          return -1;
                break;
            case T0:
                if (w)        state = T1;
                else          return -1;
                break;
            case T1:
                state += w + 1;
                break;
            case T2:
                if (w)        gotBit(0);
                else          return -1;
                break;
            case T3:
                if (w == 0)   gotBit(1);
                else          return -1;
                break;
        }
        if (total_bits >= 12) state = READY_FOR_STOP_BIT;
    } else  {
        if (width > 5000 && state==READY_FOR_STOP_BIT)
            return 1;
        else return -1;
    }
    return 0;
}

char *KakuDecoder::display () {
    byte command=0;
    byte housecode=0;
    byte unitcode=0;
   
    unsigned long bitstream = (data[1] << 8) + data[0];

// Hack ... (see RFLink ...)
int signaltype = 4; 
    if (signaltype == 4) {                        // Sartano 
        // ----------------------------------      // Sartano 
        housecode = ((bitstream) & 0x0000001FL);       // .......11111b
        unitcode = (((bitstream) & 0x000003E0L) >> 5); // ..1111100000b
        housecode = ~housecode;                    // Sartano housecode is 5 bit ('A' - '`')
        housecode &= 0x0000001FL;                  // Translate housecode so that all jumpers off = 'A' and all jumpers on = '`'
        housecode += 0x41;
        switch(unitcode) {                         // Translate unit code into button number 1 - 5
            case 0x1E:                     // E=1110
                      unitcode = 1;
                      break;
            case 0x1D:                     // D=1101
                      unitcode = 2;
                      break;
            case 0x1B:                     // B=1011
                      unitcode = 3;
                      break;
            case 0x17:                     // 7=0111
                      unitcode = 4;
                      break;
            case 0x0F:                     // f=1111 
                      unitcode = 5;
                      break;
            default:
                     break;
                     
        }	  
        if ( ((bitstream >> 10) & 0x03) == 2) {
            command = 1; // On
        } else if ( ((bitstream >> 10) & 0x03) == 1){
            command = 0;// Off
        }		  
    }
    sprintf(pbuffer, "20;%02X;AB400D;ID=%02x;SWITCH=%d;CMD=%s;\n",0,housecode,unitcode,(command == 1) ? "ON": "OFF");
    return pbuffer;
}

void Arc_Send(unsigned long bitstream, struct RawSignalStruct *pRawSignal) { 
    int fpulse = 360;                               // Pulse width in microseconds
    int fretrans = 7;                               // Number of code retransmissions
    uint32_t fdatabit;
    uint32_t fdatamask = 0x00000001;
    uint32_t fsendbuff;
    
    int x=1;

    fsendbuff = bitstream;
        
    // Send command
    for (int i = 0; i < 12; i++) {              // Arc packet is 12 bits 
        // read data bit
        fdatabit = fsendbuff & fdatamask;       // Get most right bit
        fsendbuff = (fsendbuff >> 1);           // Shift right
        // PT2262 data can be 0, 1 or float. Only 0 and float is used by regular ARC
        if (fdatabit != fdatamask) {            // Write 0
            pRawSignal->Pulses[x++] = fpulse;
            pRawSignal->Pulses[x++] = 3*fpulse;
            pRawSignal->Pulses[x++] = fpulse;
            pRawSignal->Pulses[x++] = 3*fpulse;
        } else {                                // Write float
            pRawSignal->Pulses[x++] = fpulse;
            pRawSignal->Pulses[x++] = 3*fpulse;
            pRawSignal->Pulses[x++] = 3*fpulse;
            pRawSignal->Pulses[x++] = fpulse;
        }
    }
 
    // Send sync bit
    pRawSignal->Pulses[x++] = fpulse;
    pRawSignal->Pulses[x++] = 31*fpulse;
     
    pRawSignal->Number=x; 
     
    pRawSignal->Repeats = 5; // Leave default 3 value ?
    pRawSignal->Delay = 20; // Short delay as there is already a sync bit
    
}

int str2cmd(char *command) {
    if(strcasecmp(command,"ON") == 0) return VALUE_ON;
    if(strcasecmp(command,"OFF") == 0) return VALUE_OFF;
    if(strcasecmp(command,"ALLON") == 0) return VALUE_ALLON;
    if(strcasecmp(command,"ALLOFF") == 0) return VALUE_ALLOFF;
    if(strcasecmp(command,"PAIR") == 0) return VALUE_PAIR;
    return false;
}

int KakuDecoder::TX (char *InputBuffer_Serial, struct RawSignalStruct *pRawSignal) {
        int success=false;
        
        unsigned long bitstream=0L;
        byte command=0;
        uint32_t housecode = 0;
        uint32_t unitcode = 0;
        byte Home=0;                             // KAKU home A..P
        byte Address=0;                          // KAKU Address 1..16
        byte c=0;
        byte x=0;
        // ==========================================================================
        //10;AB400D;00004d;1;OFF;                     
        //012345678901234567890
        // ==========================================================================
        if (strncasecmp(InputBuffer_Serial+3,"AB400D;",7) == 0) { // KAKU Command eg. Kaku;A1;On
          if (InputBuffer_Serial[16] != ';') return false;
           x=17;                                    // character pointer
           InputBuffer_Serial[12]=0x30;
           InputBuffer_Serial[13]=0x78;             // Get home from hexadecimal value 
           InputBuffer_Serial[16]=0x00;             // Get home from hexadecimal value 
           Home=strtoul(InputBuffer_Serial+12,NULL,0);     // KAKU home A is intern 0 
           if (Home < 0x61)                         // take care of upper/lower case
              Home=Home - 'A';
           else 
           if (Home < 0x81)                         // take care of upper/lower case
              Home=Home - 'a';
           else {
              return false;                       // invalid value
           }
           while((c=InputBuffer_Serial[x++])!=';'){ // Address: 1 to 16/32
              if(c>='0' && c<='9'){Address=Address*10;Address=Address+c-'0';}
           }
           command = str2cmd(InputBuffer_Serial+x)==VALUE_ON; // ON/OFF command
           housecode = ~Home;
           housecode &= 0x0000001FL;
           unitcode=Address;
           if ((unitcode  >= 1) && (unitcode <= 5) ) {
              bitstream = housecode & 0x0000001FL;
              if (unitcode == 1) bitstream |= 0x000003C0L;
              else if (unitcode == 2) bitstream |= 0x000003A0L;
              else if (unitcode == 3) bitstream |= 0x00000360L;
              else if (unitcode == 4) bitstream |= 0x000002E0L;
              else if (unitcode == 5) bitstream |= 0x000001E0L;

              if (command) bitstream |= 0x00000800L;
              else bitstream |= 0x00000400L;					
           }
           Arc_Send(bitstream,pRawSignal);
           success=true;
        }
        return success;
}



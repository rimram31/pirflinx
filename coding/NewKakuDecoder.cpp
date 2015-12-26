#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "OokDecoder.h"
#include "NewKakuDecoder.h"

NewKakuDecoder::NewKakuDecoder () {
        decoderName = "NEWKAKU";
}

NewKakuDecoder::NewKakuDecoder (OokDecoder *pdecoder) {
    decoderName = "NEWKAKU";
    copyDecoder(pdecoder);
}

NewKakuDecoder::~NewKakuDecoder () {}

int NewKakuDecoder::decode (uint32_t width) {
    // Start bit ...
     if (state == UNKNOWN && (180 <= width && width < 450)) {
        state = START_BIT;
    } else if (state == START_BIT && (2250 <= width)) {
        state = OK;
//    } else if ((state != UNKNOWN) && (( 100 <= width && width < 450) || (950 <= width && width < 1250))) {
    } else if ((state != UNKNOWN) && (( 100 <= width && width < 450) || (950 <= width && width < 1300))) {
        byte w = width >= 700;
        switch (state) {
            case UNKNOWN:
            case OK:
                if (w == 0)   state = T0;
                else          return -1;
                break;
            case T0:
                state = T1;
                break;
            case T1:
                state = T2;
                break;
            case T2:
                if (w)        gotBit(0);
                else          gotBit(1);
                break;
        }
        if (total_bits>=32) return 1;
    } else return -1;
    return 0;
}

static char *commandOrders[] = {"OFF", "ON", "", "ALLOFF", "ALLON"};

char *NewKakuDecoder::display () {
    
    reverseBits();

    unsigned long bitstream = (((((data[0] << 8) + data[1]) << 8) + data[2]) << 8) + data[3];
    
    int command = (bitstream >> 4) & 0x03;
    if (command > 1) command ++;    
    if (total_bits > 32) {  // DIM -> 2 octets de plus
        byte dim = (data[4] << 8) + data[5];
        sprintf(pbuffer,"20;%02X;NewKaku;ID=%08lx;SWITCH=%x;CMD=SET_LEVEL=%d;\n",0,((bitstream) >> 6),((bitstream)&0x0f)+1,dim);
    } else {
        sprintf(pbuffer,"20;%02X;NewKaku;ID=%08lx;SWITCH=%x;CMD=%s;\n",0,((bitstream) >> 6),((bitstream)&0x0f)+1,commandOrders[command]);
    }
    return pbuffer;
}

#define NewKAKU_1T                   250        // us
#define NewKAKU_2T                   290
#define NewKAKU_mT                   650/RAWSIGNAL_SAMPLE_RATE // us, approx. in between 1T and 4T 
#define NewKAKU_4T                   NewKAKU_1T*5        // 1250 us
#define NewKAKU_8T                   NewKAKU_1T*10       // 2500 us, Tijd van de space na de startbit

int NewKakuDecoder::TX (char *InputBuffer_Serial, struct RawSignalStruct *pRawSignal) {    
        int success=false;
        //10;NewKaku;123456;3;ON;                   // ON, OFF, ALLON, ALLOFF, ALL 99, 99      
        //10;NewKaku;0cac142;2;ON;
        //10;NewKaku;050515;f;OFF;
        //10;NewKaku;2100fed;1;ON;
        //10;NewKaku;000001;10;ON;
        //10;NewKaku;306070b;f;ON;
        //10;NewKaku;306070b;10;ON;
        //01234567890123456789012
        if (strncasecmp(InputBuffer_Serial+3,"NEWKAKU;",8) == 0) { 
           byte x=18;                               // pointer to the switch number
           if (InputBuffer_Serial[17] != ';') {
              if (InputBuffer_Serial[18] != ';') {
                 return false;
              } else {
                 x=19;
              }
           }
          
           unsigned long bitstream=0L;
           unsigned long tempaddress=0L;
           byte cmd=0;
           byte c=0;
           byte Address=0;                          // Address 1..16
            
           // -----
           InputBuffer_Serial[ 9]=0x30;             // Get NEWKAKU/AC main address part from hexadecimal value 
           InputBuffer_Serial[10]=0x78;             
           InputBuffer_Serial[x-1]=0x00;            
           tempaddress=strtoul(InputBuffer_Serial+9,NULL,0);
           // -----
           //while((c=InputBuffer_Serial[x++])!=';'){ // Address: 1 to 16
           //   if(c>='0' && c<='9'){Address=Address*10;Address=Address+c-'0';}
           //}
           InputBuffer_Serial[x-2]=0x30;            // Get unit number from hexadecimal value 
           InputBuffer_Serial[x-1]=0x78;            // x points to the first character of the unit number 
           if (InputBuffer_Serial[x+1] == ';') {
              InputBuffer_Serial[x+1]=0x00;
              cmd=2;
           } else {
              if (InputBuffer_Serial[x+2] == ';') {
                 InputBuffer_Serial[x+2]=0x00;            
                 cmd=3;
              } else {
                 return false;
              }
           }
           Address=strtoul(InputBuffer_Serial+(x-2),NULL,0);  // NewKAKU unit number
           if (Address > 16) return false;          // invalid address
           Address--;                               // 1 to 16 -> 0 to 15 (transmitted value is 1 less than shown values)
           x=x+cmd;                                 // point to on/off/dim command part
           // -----
           tempaddress=(tempaddress <<6) + Address; // Complete transmitted address
           // -----
           cmd=str2cmd(InputBuffer_Serial+x);       // Get ON/OFF etc. command
           if (cmd == false) {                      // Not a valid command received? ON/OFF/ALLON/ALLOFF
              cmd=strtoul(InputBuffer_Serial+x,NULL,0);    // get DIM value
           }
           // --------------- NEWKAKU SEND ------------
           //unsigned long bitstream=0L;
           byte i=1;
           x=0;                                     // aantal posities voor pulsen/spaces in RawSignal
        
           // bouw het KAKU adres op. Er zijn twee mogelijkheden: Een adres door de gebruiker opgegeven binnen het bereik van 0..255 of een lange hex-waarde
           //if (tempaddress<=255)
           //   bitstream=1|(tempaddress<<6);                             // Door gebruiker gekozen adres uit de Nodo_code toevoegen aan adres deel van de KAKU code. 
           //else
           bitstream=tempaddress & 0xFFFFFFCF;                          // adres geheel over nemen behalve de twee bits 5 en 6 die het schakel commando bevatten.

           if (cmd == VALUE_ON || cmd == VALUE_OFF) {
              bitstream|=(cmd == VALUE_ON)<<4;                          // bit-5 is het on/off commando in KAKU signaal
              x=130;                                                    // verzend startbit + 32-bits = 130
           } else
              x=146;                                                    // verzend startbit + 32-bits = 130 + 4dimbits = 146

           // bitstream bevat nu de KAKU-bits die verzonden moeten worden.
//           for(i=3;i<=x;i++) {
//                pRawSignal->Pulses[i]=NewKAKU_1T;  // De meeste tijden in signaal zijn T. Vul alle pulstijden met deze waarde. Later worden de 4T waarden op hun plek gezet
//           }
           i=1;
           pRawSignal->Pulses[i++]=NewKAKU_1T;         // Start bit 250 + 2600 s.
           pRawSignal->Pulses[i++]=NewKAKU_8T;
           byte y=31;                                                   // bit uit de bitstream
           while(i<x) {
               if ((bitstream>>(y--))&1) {
                    // un bit a 1 c'est 250 + 1250 + 250 + 290
                    pRawSignal->Pulses[i++]=NewKAKU_1T;
                    pRawSignal->Pulses[i++]=NewKAKU_4T;
                    pRawSignal->Pulses[i++]=NewKAKU_1T;
                    pRawSignal->Pulses[i++]=NewKAKU_2T;
               } else {
                    // Un bit a 0 c'est 250 + 290 + 250 + 1250
                    pRawSignal->Pulses[i++]=NewKAKU_1T;
                    pRawSignal->Pulses[i++]=NewKAKU_2T;
                    pRawSignal->Pulses[i++]=NewKAKU_1T;
                    pRawSignal->Pulses[i++]=NewKAKU_4T;
               }
//             if ((bitstream>>(y--))&1)
//                pRawSignal->Pulses[i+1]=NewKAKU_4T;    // Bit=1; // T,4T,T,T
//             else
//                pRawSignal->Pulses[i+3]=NewKAKU_4T;    // Bit=0; // T,T,T,4T
    
//             if (x==146) {                                              // als het een dim opdracht betreft
//                if (i==111)                                             // Plaats van de Commando-bit uit KAKU 
//                   pRawSignal->Pulses[i+3]=NewKAKU_1T; // moet een T,T,T,T zijn bij een dim commando.
//                if (i==127) {                                           // als alle pulsen van de 32-bits weggeschreven zijn
//                   bitstream=(unsigned long)cmd;                        //  nog vier extra dim-bits om te verzenden. 
//                   y=3;
//                }
//             }
//             i+=4;
           }
           // On remet un pulse bit derrière ... car ça enchaine par un 250 + "silence ..." (fait par delay pour nous)
           pRawSignal->Pulses[i++]=NewKAKU_1T;
           pRawSignal->Number=i;
           //pRawSignal->Number=i-1;                                          // aantal bits*2 die zich in het opgebouwde RawSignal bevinden

           success=true;
        }
        pRawSignal->Repeats = 5; // Leave default 3 value ?
        pRawSignal->Delay = 9900L; // Leave default 900 value ?
        // --------------------------------------
        return success;
}

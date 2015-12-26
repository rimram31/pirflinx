#include <stdio.h>
#include <memory.h>
#include <pigpio.h>

#include "rfHandlerV2.h"

#include "coding/OokDecoder.h"
#include "coding/KakuDecoder.h"
#include "coding/NewKakuDecoder.h"
#include "coding/OregonDecoder.h"
#include "coding/AlectoDecoder.h"
#include "Logger.h"

#define MAX_STACK_ELEM 10
typedef struct {
    int first, last;
    OokDecoder *list[MAX_STACK_ELEM];
} stackMatches_t;

static stackMatches_t matches = { 0, 0 };

static int b_Receive;

#define MAX_DECODERS 20
OokDecoder *decoders[MAX_DECODERS];
int ndecoders = 0;
void addDecoder(OokDecoder *pdecoder) {
    decoders[ndecoders++] = pdecoder;
}
void deleteDecoders() {
    for(int i=0; i!=ndecoders;i++)
        delete(decoders[i]);
}
void resetDecoders() {
    for(int i=0; i!=ndecoders;i++)
        decoders[i]->resetDecoder();
}
OokDecoder *newDecoder(OokDecoder *pdecoder) {
    char *decoderName = pdecoder->getDecoderName();
    if (!strcmp(decoderName,"KAKU")) return new KakuDecoder(pdecoder);
    if (!strcmp(decoderName,"NEWKAKU")) return new NewKakuDecoder(pdecoder);
    if (!strcmp(decoderName,"OREGON_V1")) return new OregonDecoderV1(pdecoder);
    if (!strcmp(decoderName,"OREGON_V2")) return new OregonDecoderV2(pdecoder);
    if (!strcmp(decoderName,"OREGON_V3")) return new OregonDecoderV3(pdecoder);
    if (!strcmp(decoderName,"ALECTO_V4")) return new AlectoDecoderV4(pdecoder);
    return NULL;
}

rfHandlerV2::rfHandlerV2(int dataPin, int trSelPin, int rsstPin, int pwdnPin) {
    
    this->dataPin = dataPin;
    this->trSelPin = trSelPin;
    this->rsstPin = rsstPin;
    this->pwdnPin = pwdnPin;

    /* Data & rsst pin as input */
    gpioSetMode(this->dataPin, PI_INPUT);
    gpioSetMode(this->rsstPin, PI_INPUT);
    /* Trsel as output */
    gpioSetMode(this->trSelPin, PI_OUTPUT);
	if (this->pwdnPin != -1) {
        gpioSetMode(this->pwdnPin, PI_OUTPUT);
        /* power on */
        gpioWrite(this->pwdnPin,0);
    }

    /* 5ms max gap RSST_PIN last pulse */
    //gpioSetWatchdog(this->rsstPin, TIME_OUT);
    /* monitor RSST_PIN level changes */
    //gpioSetAlertFunc(this->rsstPin, alertRsst);
    // Arm data monitoring
    gpioSetAlertFunc(this->dataPin, alertData);

    addDecoder(new KakuDecoder());
    addDecoder(new NewKakuDecoder());
    addDecoder(new OregonDecoderV1());
    addDecoder(new OregonDecoderV2());
    addDecoder(new OregonDecoderV3());
    addDecoder(new AlectoDecoderV4());
    
    // Receive by default (send "on demand")
    setReceiveMode();

}

rfHandlerV2::~rfHandlerV2() {
    deleteDecoders();
}

void rfHandlerV2::setReceiveMode() {
    /* link device in receive mode TRSEL -> low */
    gpioWrite(this->trSelPin,0);    
    /* data gpio in input mode */
    gpioSetMode(this->dataPin, PI_INPUT);
    gpioDelay(1000); /* 1ms delay ? */

    b_Receive = true;
    resetDecoders();
    
}

void rfHandlerV2::setSendMode() {
    b_Receive = false;
    /* link device in receive mode TRSEL -> high */
    gpioWrite(this->trSelPin,1);    
    gpioDelay(1000); /* 1ms delay ? */
    /* data gpio in output mode */
    gpioSetMode(this->dataPin, PI_OUTPUT);
}

OokDecoder *rfHandlerV2::popMatches() {
    OokDecoder *pmatch = NULL;
    if (matches.first != matches.last) {
        // Copy pulses in last position
        pmatch = matches.list[matches.first];
        if (++matches.first == MAX_STACK_ELEM)
            matches.first = 0;
    }
    return pmatch;
}

void rfHandlerV2::pushMatches(OokDecoder *match) {
    int futureLast = matches.last;
    
    if (++futureLast == MAX_STACK_ELEM)
        futureLast = 0;
    // Not full stack ?
    if (futureLast !=  matches.first) {   
        matches.list[matches.last] = match;
        matches.last = futureLast;
    }
}

static uint32_t lastTick;
static OokDecoder *pdecoderFound = NULL;

void rfHandlerV2::alertData(int gpio, int level, uint32_t tick) { 
    uint32_t pulse = tick - lastTick;
    
    if (b_Receive) {
        // Debug
        //pulses.level[pulses.npulses] = level;
        //pulses.tick[pulses.npulses++] = tick;
        //if (pulses.npulses >= MAX_PULSES)
        //   pulses.npulses = 0;
    
    //if (pulse < 32000 ) {     
        int idecoder;   
        for(idecoder = 0; idecoder!=ndecoders; idecoder++) {
            OokDecoder *pdecoder = decoders[idecoder];
//printf("nextPulse %ld\n",pulse);            
            if (pdecoder->nextPulse(pulse)) {
               // Ok found
               pdecoderFound = pdecoder;
               break;
            }
        }
        if (pdecoderFound!=NULL) {
            //printf("!!! Found %s\n",pdecoderFound->getDecoderName());
            pushMatches(newDecoder(pdecoderFound));
            resetDecoders();
            pdecoderFound = NULL;
        }
    //} else {
    //    resetDecoders();
    //}
    }
    lastTick = tick;
    
}

void displayPulses(pulses_t *p_pulses) {
    int i;
    
    printf("Pulses count = %d : ",p_pulses->npulses);
    for (i=0; i!= p_pulses->npulses; i++) {
        //printf("%d,%d,%d;",p_pulses->tick[i],p_pulses->level[i],(i>0)?p_pulses->tick[i]-p_pulses->tick[i-1]:-1);
        printf("%d,",p_pulses->tick[i]);
    }
    printf("\n");
}

extern int g_RepeatDefault;
extern long g_DelayDefault;

/**
 *  Send RF pulses
 * 
 */
void rfHandlerV2::sendRF(int nrepeats, uint32_t delay, pulses_t *p_pulses) {
    
    setSendMode();
    
    int bigRepeat = g_RepeatDefault;
    int bigDelay = g_DelayDefault;
    while(bigRepeat--) {
        for (int irepeat = 0; irepeat != nrepeats; irepeat++) {
//displayPulses(p_pulses);
            for (int i=0; i!= p_pulses->npulses; i++) {
                gpioWrite(this->dataPin,p_pulses->level[i]);
                if (p_pulses->tick[i] > 0)
                    gpioDelay(p_pulses->tick[i]);
            }
            gpioWrite(this->dataPin,0);
            gpioDelay(delay);
//printf("delay = %ld\n",delay);        
        }
        if (bigRepeat>0) {
            gpioDelay(bigDelay);
//printf("Big delay = %ld\n",bigDelay);        
        }
    }
    setReceiveMode(); 
      
}

// Compatibility ... to be modified ...
static char inputBuffer[60];
static struct RawSignalStruct RawSignal = {0,0,0,0,0,0L};

void rfHandlerV2::sendRFMessage(char *message) {

    // Repeat default & delay
    //RawSignal.Repeats = 3;
    //RawSignal.Delay = SEND_REPEAT_DELAY;
    
    RawSignal.Repeats = g_RepeatDefault;
    RawSignal.Delay = g_DelayDefault;
    
    strcpy(inputBuffer,message);
    for(int i=0; i!=ndecoders;i++) {
        if (decoders[i]->TX(inputBuffer,&RawSignal)) {
            pulses_t pulses;
            pulses.npulses = 0;
            int ilevel = 1;
            for (int i=1;i<RawSignal.Number;i++) {
                pulses.tick[pulses.npulses] = RawSignal.Pulses[i];
                pulses.level[pulses.npulses] = ilevel;
                pulses.npulses++;
                ilevel = !ilevel;
            }
            // Send with repetition
            sendRF(RawSignal.Repeats, (uint32_t) RawSignal.Delay, &pulses);
            break;
        }
    }    
}

#ifndef RFNEWHANDLER_H_
#define RFNEWHANDLER_H_

#include "coding/OokDecoder.h"

#define MAX_PULSES 500
//#define SEND_REPEAT_DELAY 20 ???
#define SEND_REPEAT_DELAY 9000

typedef struct {
   int npulses;
   uint32_t tick[MAX_PULSES];
   int      level[MAX_PULSES];
} pulses_t;

class rfHandlerV2 {

    private:
    
        int dataPin;
        int trSelPin;
        int rsstPin;
        int pwdnPin;
    
	public:
    
		rfHandlerV2(int dataPin, int trSelPin, int rsstPin, int pwdnPin);
		~rfHandlerV2();
        
        void setReceiveMode();
        void setSendMode();
        
        void sendRFMessage(char *message);
        void sendRF(int nrepeats, uint32_t delay, pulses_t *p_pulses);
        
        static void alertRsst(int gpio, int level, uint32_t tick);
        static void alertData(int gpio, int level, uint32_t tick);
        
        static OokDecoder *popMatches();
        static void pushMatches(OokDecoder *match);
        
};

#endif /* RFNEWHANDLER_H_ */

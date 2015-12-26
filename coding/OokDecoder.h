#ifndef _OOK_H_
#define _OOK_H_

typedef uint16_t word;
typedef uint8_t byte;

#define OOK_MAX_DATA_LEN 25
#define OOK_MAX_STR_LEN	 100

#define PRINT_BUFFER_SIZE 60

#include "main.h"

class OokDecoder {

	protected:
        char *decoderName;
		byte total_bits, bits, flip, state, pos, data[OOK_MAX_DATA_LEN];
        char pbuffer[PRINT_BUFFER_SIZE];  
		
    virtual int  decode (uint32_t width) =0;
    void copyDecoder(OokDecoder *pdecoder);

public:

#ifdef __DEBUG__    
    // Debug
    pulses_t pulses;
#endif        

    enum { UNKNOWN, T0, T1, T2, T3, OK, DONE, START_BIT, READY_FOR_STOP_BIT };
	
    OokDecoder();
    OokDecoder(OokDecoder *pdecoder);
    virtual ~OokDecoder();

    bool nextPulse (uint32_t width);
    bool isDone() const;
    
    char *getDecoderName() const;
    byte getState () const;

    const byte* getData (byte& count) const;
    void resetDecoder ();

    // store a bit using Manchester encoding (used with flip ...)
    void manchester (char value);
    void alignTail (byte max =0);
    void reverseBits ();
    void reverseNibbles ();
    void reverseData ();

    void done ();

    virtual void gotBit (char value);
    virtual char *display () = 0;

    virtual int TX (char *, struct RawSignalStruct *);
	
};
#endif

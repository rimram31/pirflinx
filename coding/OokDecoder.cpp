#include <iostream>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "OokDecoder.h"

OokDecoder::OokDecoder () {
    resetDecoder();
}

OokDecoder::OokDecoder (OokDecoder *pdecoder) {
    resetDecoder();
    copyDecoder(pdecoder);
}

OokDecoder::~OokDecoder () {}

int OokDecoder::TX (char *, struct RawSignalStruct *) {
    return 0;
}

// -------------------------------------
bool OokDecoder::nextPulse(uint32_t width) {
    if (state != DONE) {
        switch (decode(width)) {
            case -1: resetDecoder(); break;
            case 1:  done(); break;
        }
    }
#ifdef __DEBUG__
    if (pulses.npulses < MAX_PULSES)
        pulses.tick[pulses.npulses++] = width;
#endif    
    return isDone();
}
// -------------------------------------
bool OokDecoder::isDone() const { return state == DONE; }

// -------------------------------------
byte OokDecoder::getState() const { return state; }

// -------------------------------------
char *OokDecoder::getDecoderName() const { return decoderName; }

// -------------------------------------
const byte* OokDecoder::getData(byte& count) const {
    count = pos;
    return data;
}

// -------------------------------------
void OokDecoder::copyDecoder(OokDecoder *pdecoder) {
    total_bits = pdecoder->total_bits;
    bits = pdecoder->bits;
    flip = pdecoder->flip;
    state = pdecoder->state;
    pos = pdecoder->pos;
    memcpy((void *)data,(const void *)pdecoder->data,OOK_MAX_DATA_LEN);
    strcpy(pbuffer,pdecoder->pbuffer);
#ifdef __DEBUG__    
    memcpy((void *)&pulses,(const void *)&(pdecoder->pulses),sizeof(pulses));
#endif
}

// -------------------------------------
void OokDecoder::resetDecoder() {
    total_bits = bits = pos = flip = 0;
    state = UNKNOWN;
#ifdef __DEBUG__    
    pulses.npulses = 0;
#endif
}
// -------------------------------------
// add one bit to the packet data buffer
// -------------------------------------
void OokDecoder::gotBit(char value) {
    total_bits++;
    byte *ptr = data + pos;
    *ptr = (*ptr >> 1) | (value << 7);

    if (++bits >= 8) {
        bits = 0;
        if (++pos >= sizeof data) {
            resetDecoder();
            return;
        }
    }
    state = OK;
}
// -------------------------------------
// store a bit using Manchester encoding
// -------------------------------------
void OokDecoder::manchester(char value) {
    flip ^= value; // manchester code, long pulse flips the bit
    gotBit(flip);
}
// move bits to the front so that all the bits are aligned to the end
void OokDecoder::alignTail (byte max) {
    // align bits
    if (bits != 0) {
        data[pos] >>= 8 - bits;
        for (byte i = 0; i < pos; ++i)
        data[i] = (data[i] >> bits) | (data[i+1] << (8 - bits));
        bits = 0;
    }
    // optionally shift bytes down if there are too many of 'em
    if (max > 0 && pos > max) {
        byte n = pos - max;
        pos = max;
        for (byte i = 0; i < pos; ++i)
            data[i] = data[i+n];
    }
}

/* Reverse bits order last becoming first */
void OokDecoder::reverseData () {
    // reverse octets
    int start = 0;
    int end = pos-1;
    while ( start < end ) {
        byte b = data[start];
        data[start] = data[end];
        data[end] = b;
        start++;
        end--;
    }
}

void OokDecoder::reverseBits () {
    for (byte i = 0; i < pos; i++) {
        byte b = data[i];
        for (byte j = 0; j < 8; ++j) {
            data[i] = (data[i] << 1) | (b & 1);
            b >>= 1;
        }
    }
}

void OokDecoder::reverseNibbles () {
    for (byte i = 0; i < pos; i++)
        data[i] = (data[i] << 4) | (data[i] >> 4);
}
// -------------------------------------
void OokDecoder::done() {
    while (bits)
        gotBit(0); // padding
    state = DONE;
}

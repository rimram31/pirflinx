#ifndef _OREGON_H_
#define _OREGON_H_

class OregonDecoder : public OokDecoder {
    public:
        OregonDecoder();
        ~OregonDecoder();
        char *display ();
};

class OregonDecoderV1 : public OregonDecoder {
    public:
        OregonDecoderV1();
        OregonDecoderV1(OokDecoder *pdecoder);
        ~OregonDecoderV1();
        int decode (uint32_t width);
};

class OregonDecoderV2 : public OregonDecoder {
    public:
        OregonDecoderV2();
        OregonDecoderV2(OokDecoder *pdecoder);
        ~OregonDecoderV2();
        void gotBit (char value);
        int decode (uint32_t width);
};

class OregonDecoderV3 : public OregonDecoder {
    public:
        OregonDecoderV3();
        OregonDecoderV3(OokDecoder *pdecoder);
        ~OregonDecoderV3();
        void gotBit (char value);
        int decode (uint32_t width);
};

#endif

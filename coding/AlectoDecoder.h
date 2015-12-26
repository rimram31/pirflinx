#ifndef _ALECTO_H_
#define _ALECTO_H_

class AlectoDecoderV4 : public OokDecoder {
    public:
        AlectoDecoderV4();
        AlectoDecoderV4(OokDecoder *pdecoder);
        ~AlectoDecoderV4();
        int decode (uint32_t width);
        char *display ();
};

#endif

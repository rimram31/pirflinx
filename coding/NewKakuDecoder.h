#ifndef _NEWKAKU_H_
#define _NEWKAKU_H_

class NewKakuDecoder : public OokDecoder {
    public:
        NewKakuDecoder();
        NewKakuDecoder(OokDecoder *pdecoder);
        ~NewKakuDecoder();
        int decode (uint32_t width);
        char *display ();
        int TX (char *, struct RawSignalStruct *);
};

#endif

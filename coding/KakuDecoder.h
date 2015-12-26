#ifndef _KAKU_H_
#define _KAKU_H_

class KakuDecoder : public OokDecoder {
    public:
        KakuDecoder();
        KakuDecoder(OokDecoder *pdecoder);
        ~KakuDecoder();
        int decode (uint32_t width);
        char *display ();
        int TX (char *, struct RawSignalStruct *);
};

#endif

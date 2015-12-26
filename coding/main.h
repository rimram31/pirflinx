#ifndef _MAIN_H_
#define _MAIN_H_

//#define __DEBUG__

#ifdef __DEBUG__
#define MAX_PULSES 500
typedef struct {
   int npulses;
   uint32_t tick[MAX_PULSES];
   int      level[MAX_PULSES];
} pulses_t;
#endif
typedef uint8_t byte;

// For TX compatibility (to be removed ...)

#define VALUE_PAIR                      44
#define VALUE_ALLOFF                    55
#define VALUE_OFF                       74
#define VALUE_ON                        75
#define VALUE_ALLON                     141

#define RAW_BUFFER_SIZE                  512                                    // Maximum number of pulses that is received in one go.
struct RawSignalStruct {
  int  Number;                                                                  // Number of pulses, times two as every pulse has a mark and a space.
  uint8_t Repeats;                                                                 // Number of re-transmits on transmit actions.
  uint16_t Delay;                                                                   // Delay in ms. after transmit of a single RF pulse packet
  uint8_t Multiply;                                                                // Pulses[] * Multiply is the real pulse time in microseconds 
  unsigned long Time;                                                           // Timestamp indicating when the signal was received (millis())
  uint16_t Pulses[RAW_BUFFER_SIZE+2];                                               // Table with the measured pulses in microseconds divided by RawSignal.Multiply. (halves RAM usage)
                                                                            // First pulse is located in element 1. Element 0 is used for special purposes, like signalling the use of a specific plugin
};

//void RawSendRF(void);
int str2cmd(char *command);

#endif

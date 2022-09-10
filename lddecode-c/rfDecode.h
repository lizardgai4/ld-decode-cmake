#ifndef RFDECODE_H
#define RFDECODE_H

#include <iostream>
#include <map>
#include <string>

class RFDecode {
    /*The core RF decoding code.

    This decoder uses FFT overlap-save processing(1) to allow for parallel processing and combination of
    operations.

    Video filter signal path:
    - FFT/iFFT stage 1: RF BPF (i.e. 3.5-13.5mhz NTSC) * hilbert filter
    - phase unwrapping
    - FFT stage 2, which is processed into multiple final products:
      - Regular video output
      - 0.5mhz LPF (used for HSYNC)
      - For fine-tuning HSYNC: NTSC: 3.5x mhz filtered signal, PAL: 3.75mhz pilot signal

    Analogue audio filter signal path:

        The audio signal path is actually more complex in some ways, since it reduces a
        multi-msps signal down to <100khz.  A two stage processing system is used which
        reduces the frequency in each stage.

        Stage 1 performs the audio RF demodulation per block typically with 32x decimation,
        while stage 2 is run once the entire frame is demodulated and decimates by 4x.

    EFM filtering simply applies RF front end filters that massage the output so that ld-process-efm
    can do the actual work.

    references:
    1 - https://en.wikipedia.org/wiki/Overlap–save_method
     */

public:
    explicit RFDecode(int frequencies[], std::string system, int blocklen, bool decode_digital_audio,
    int decode_analog_audio, bool has_analog_audio, std::map<std::string,bool> extraBools, std::map<std::string,int> extraInts,
    std::map<std::string,float> extraFloats);

    void computeFilters();

    void computeEfmFilter();

    void computeVideoFilters();

    void computeAudioFilters();

    float ireToHz(float ire);

    float hzToIre(float hz);

    std::string* demodBlock();

    std::string* runfilterAudioPhase2();

    void audioPhase2(float mtfLevel);

    //Semantic ints for sysParams (NTSC and PAL)
    static const unsigned short FPS = 0;
    static const unsigned short FSC_MHZ = 1;
    static const unsigned short PILOT_MHZ = 2;
    static const unsigned short FRAME_LINES = 3;
    static const unsigned short FIELD_LINES_1 = 4;
    static const unsigned short FIELD_LINES_2 = 5;
    static const unsigned short LINE_PERIOD = 6;
    static const unsigned short IRE0 = 7;
    static const unsigned short HZ_IRE = 8;
    static const unsigned short VSYNC_IRE = 9;
    static const unsigned short AUDIO_LFREQ = 10;
    static const unsigned short AUDIO_RFREQ = 11;
    static const unsigned short COLORBURST_US_1 = 12;
    static const unsigned short COLORBURST_US_2 = 13;
    static const unsigned short ACTIVE_VIDEO_US_1 = 14;
    static const unsigned short ACTIVE_VIDEO_US_2 = 15;
    static const unsigned short BLACKSNR_SLICE_LINE = 16;
    static const unsigned short BLACKSNR_SLICE_BEGINNING = 17;
    static const unsigned short BLACKSNR_SLICE_LENGTH = 18;
    static const unsigned short FIRST_FIELD_H_1 = 19;
    static const unsigned short FIRST_FIELD_H_2 = 20;
    static const unsigned short NUM_PULSES = 21;
    static const unsigned short HSYNC_PULSE_US = 22;
    static const unsigned short EQ_PULSE_US = 23;
    static const unsigned short VSYNC_PULSE_US = 24;
    static const unsigned short OUTPUT_ZERO = 25;
    static const unsigned short FIELD_PHASES = 26;
    static const unsigned short OUTLINELEN = 27;
    static const unsigned short OUTLINELEN_PILOT = 28;
    static const unsigned short OUTFREQ = 29;

    //Semantic ints for rfParams (NTSC and PAL)
    static const unsigned short AUDIO_NOTCHWIDTH = 0;
    static const unsigned short AUDIO_NOTCHORDER = 1;
    static const unsigned short DEEMP_LOW = 2;
    static const unsigned short DEEMP_HIGH = 3;
    static const unsigned short VIDEO_BPF_LOW = 4;
    static const unsigned short VIDEO_BPF_HIGH = 5;
    static const unsigned short VIDEO_BPF_ORDER = 6;
    static const unsigned short VIDEO_LPF_FREQ = 7;
    static const unsigned short VIDEO_LPF_ORDER = 8;
    static const unsigned short MTF_BASEMULT = 9;
    static const unsigned short MTF_POLEDIST = 10;
    static const unsigned short MTF_FREQ = 11;
    static const unsigned short VIDEO_HPF_FREQ = 12;
    static const unsigned short VIDEO_HPF_ORDER = 13;
    static const unsigned short AUDIO_FILTERWIDTH = 14;
    static const unsigned short AUDIO_FILTERORDER = 15;

private:
    int blocklen;
    int blockcut;
    int blockcutEnd;

    std::string system;
    bool ntscColorNotchFilter;
    bool palV4300DNotchFilter;
    bool lowband;
    bool decodeAnalogAudio;
    bool hasAnalogAudio;

    float freq;
    float freqHalf;
    float freqHz;
    float freqHzHalf;

    float mtfMult;
    float mtfOffset;

    float* sysParams;

    float* decoderParams;

    float fw;
    float deempMult1;
    float deempMult2;

    float deempLow;
    float deempHigh;

    float lineLen;
    float hsyncTolerance;
    
    bool decodeDigitalAudio;
    bool decodeAnalogAudio;
};

#endif
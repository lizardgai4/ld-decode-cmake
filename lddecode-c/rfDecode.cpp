/**
 * The core RF decoding code.

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

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <cfloat>
#include <complex>
#include <fftw3.h>

#include "spline.h" //from https://kluge.in-chemnitz.de/opensource/spline/ https://github.com/ttk592/spline
#include "butterworth.h" //from https://github.com/ruohoruotsi/Butterworth-Filter-Design

#include "rfDecode.h"

RFDecode::RFDecode(int frequencies[], std::string _system, int _blocklen, bool decode_digital_audio,
                int decode_analog_audio, bool has_analog_audio, std::map<std::string,bool> extraBools,
                std::map<std::string,int> extraInts, std::map<std::string,float> extraFloats)
{
    /*Initialize the RF decoder object.

        inputfreq -- frequency of raw RF data (in Msps)
        system    -- Which system is in use (PAL or NTSC)
        blocklen  -- Block length for FFT processing
        decode_digital_audio -- Whether to apply EFM filtering
        decode_analog_audio  -- Whether or not to decode analog(ue) audio
        has_analog_audio     -- Whether or not analog(ue) audio channels are on the disk

        extra_options -- Dictionary of additional options (typically boolean) - these include:
          - PAL_V4300D_NotchFilter - cut 8.5mhz spurious signal
          - NTSC_ColorNotchFilter:  notch filter on decoded video to reduce color 'wobble'
          - lowband: Substitute different decode settings for lower-bandwidth disks

    */

    blocklen = _blocklen;
    blockcut = 1024; // ???
    blockcutEnd = 0;
    system = _system;

    /*due to the way main.cpp works, we're guaranteed to have these.
      No need for extraBools.count("some_parameter")*/
    ntscColorNotchFilter = extraBools["NTSC_ColorNotchFilter"];
    palV4300DNotchFilter = extraBools["PAL_V4300D_NotchFilter"];
    lowband = extraBools["lowband"];

    freq = frequencies[0];
    freqHalf = freq / 2;
    freqHz = freq * 10e6;
    freqHzHalf = freqHz / 2;

    mtfMult = 1.0;
    mtfOffset = 0;

    decodeAnalogAudio = true;

    /**
     * SysParams and DecoderParams
     */

    //Parenthesis are to make equations stand out from numbers
    //-100.0 means "will be set after initialization"

    //These are invariant parameters for PAL and NTSC
    float sysParamsNTSC[30] = {
        -100.0, //FPS
        (315.0 / 88.0), //fsc_mhz
        (315.0 / 88.0), //pilot_mhz
        525.0, //frame_lines
        263.0, //field_lines[0]
        262.0, //field_lines[1]
        -100.0, //line_period
        8100000.0, //ire0
        (1700000.0 / 140.0), //hz_ire
        -40.0, //vsync_ire
        //From the spec - audio frequencies are multiples of the (color) line rate
        (1000000.0 * 315.0 / 88.0 / 227.5) * 146.25, //audio_lfreq
        //NOTE: this changes to 2.88mhz on AC3 disks
        (1000000.0 * 315.0 / 88.0 / 227.5) * 178.75, //audio_rfreq
        5.3, //colorburstUS[0]
        7.8, //colorburstUS[1]
        9.45, //activeVideoUS[0]
        (63.555 - 1.0), //activeVideoUS[1]
        //Known-good area for computing black SNR - for NTSC pull from VSYNC
        1.0, //blacksnr_slice line
        10.0, //blacksnr_slice beginning
        20.0, //blacksnr_slice length
        //In NTSC framing, the distances between the first/last eq pulses and the
        //corresponding next lines are different.
        0.5, //firstFieldH[0]
        1.0, //firstFieldH[1]
        6.0, //numPulses, number of equalization pulses per section
        4.7, //hsyncPulseUS
        2.3, //eqPulseUS
        27.1, //vsyncPulseUS
        //What 0 IRE/0V should be in digitaloutput
        1024.0, //outputZero
        4.0, //fieldPhases
        -100, //outlinelen
        -100, //outlinelen_pilot
        -100 //outfreq
    };

    //calculates NTSC params
    sysParamsNTSC[LINE_PERIOD] = 1.0 / (sysParamsNTSC[FSC_MHZ] / 227.5);
    sysParamsNTSC[ACTIVE_VIDEO_US_2] = sysParamsNTSC[LINE_PERIOD] - 1.0;

    sysParamsNTSC[FPS] = 10e6 / (525.0 * sysParamsNTSC[LINE_PERIOD]);

    sysParamsNTSC[OUTLINELEN] = (int)(sysParamsNTSC[LINE_PERIOD] * 4.0 * sysParamsNTSC[FSC_MHZ]);
    sysParamsNTSC[OUTFREQ] = 4.0 * sysParamsNTSC[FSC_MHZ];

    //PAL params

    float sysParamsPAL[30] = {
        25.0, //FPS
        ((1.0 / 64.0) * 283.75) + (25.0 / 10e6), //fsc_mhz
        3.75, //pilot_mhz
        625.0, //frame_lines
        312.0, //field_lines[0]
        313.0, //field_lines[1]
        64.0, //line_period
        7100000.0, //ire0
        (800000.0 / 100.0), //hz_ire
        -100.0, //vsync_ire
        //From the spec - audio frequencies are multiples of the (color) line rate
        (1000000.0 / 64.0) * 43.75, //audio_lfreq
        //NOTE: this changes to 2.88mhz on AC3 disks
        (1000000.0 / 64.0) * 68.25, //audio_rfreq
        5.6, //colorburstUS[0]
        7.85, //colorburstUS[1]
        10.5, //activeVideoUS[0]
        64.0 - 1.5, //activeVideoUS[1]
        //Known-good area for computing black SNR - for NTSC pull from VSYNC
        22.0, //blacksnr_slice line
        12.0, //blacksnr_slice beginning
        50.0, //blacksnr_slice length
        //In NTSC framing, the distances between the first/last eq pulses and the
        //corresponding next lines are different.
        1.0, //firstFieldH[0]
        0.5, //firstFieldH[1]
        5.0, //numPulses, number of equalization pulses per section
        4.7, //hsyncPulseUS
        2.35, //eqPulseUS
        27.3, //vsyncPulseUS
        //What 0 IRE/0V should be in digitaloutput
        256.0, //outputZero
        8.0, //fieldPhases
        -100.0, //outlinelen
        -100.0, //outlinelen_pilot
        -100.0 //outfreq
    };

    //calculates PAL params

    sysParamsPAL[OUTLINELEN] = (int)(sysParamsPAL[LINE_PERIOD] * 4.0 * sysParamsPAL[FSC_MHZ]);
    sysParamsPAL[OUTLINELEN_PILOT] = (int)(sysParamsPAL[LINE_PERIOD] * 4.0 * sysParamsPAL[PILOT_MHZ]);
    sysParamsPAL[OUTFREQ] = 4.0 * sysParamsPAL[FSC_MHZ];

    sysParamsPAL[VSYNC_IRE] = -0.3 * (100.0 / 0.7);

    //RF params for NTSC and PAL
    float rfParamsNTSC[16] = {
        350000.0, //audio_notchwidth
        2.0, //audio_notchorder
        120e-9, //video_deemp[0]
        320e-9, //video_deemp[1]
        3400000.0, //video_bpf_low
        13800000.0, //video_bpf_high
        4.0, //video_bpf_order
        4500000.0, //video_lpf_freq
        6.0, //video_lpf_order
        0.4, //MTF_basemult
        0.9, //MTF_poledist
        12.2, //MTF_freq
        10000000.0, //video_hpf_freq
        4.0, //video_hpf_order
        150000.0, //audio_filterwidth
        512.0 //audio_filterorder
    };

    float rfParamsNTSClowband[16] = {
        350000.0, //audio_notchwidth
        2.0, //audio_notchorder
        120e-9, //video_deemp[0]
        320e-9, //video_deemp[1]
        3800000.0, //video_bpf_low
        12500000.0, //video_bpf_high
        4.0, //video_bpf_order
        4200000.0, //video_lpf_freq
        6.0, //video_lpf_order
        0.4, //MTF_basemult
        0.9, //MTF_poledist
        12.2, //MTF_freq
        10000000.0, //video_hpf_freq
        4.0, //video_hpf_order
        150000.0, //audio_filterwidth
        512.0 //audio_filterorder
    };

    float rfParamsPAL[16] = {
        200000.0, //audio_notchwidth
        2.0, //audio_notchorder
        100e-9, //video_deemp[0]
        400e-9, //video_deemp[1]
        2300000.0, //video_bpf_low
        13500000.0, //video_bpf_high
        2.0, //video_bpf_order
        5200000.0, //video_lpf_freq
        7.0, //video_lpf_order
        1.0, //MTF_basemult
        0.7, //MTF_poledist
        10.0, //MTF_freq
        10000000.0, //video_hpf_freq
        4.0, //video_hpf_order
        100000.0, //audio_filterwidth
        900.0 //audio_filterorder
    };

    float rfParamsPALlowband[16] = {
        200000.0, //audio_notchwidth
        2.0, //audio_notchorder
        100e-9, //video_deemp[0]
        400e-9, //video_deemp[1]
        3200000.0, //video_bpf_low
        13000000.0, //video_bpf_high
        1.0, //video_bpf_order
        4800000.0, //video_lpf_freq
        7.0, //video_lpf_order
        1.0, //MTF_basemult
        0.7, //MTF_poledist
        10.0, //MTF_freq
        10000000.0, //video_hpf_freq
        4.0, //video_hpf_order
        100000.0, //audio_filterwidth
        900.0 //audio_filterorder
    };

    if(system == "NTSC") {
        sysParams = &sysParamsNTSC[0];
        if(lowband) {
            decoderParams = &rfParamsNTSClowband[0];
        } else {
            decoderParams = &rfParamsNTSC[0];
        }
    } else if (system == "PAL") {
        sysParams = &sysParamsPAL[0];
        if(lowband) {
            decoderParams = &rfParamsPALlowband[0];
        } else {
            decoderParams = &rfParamsPAL[0];
        }
    }

    hasAnalogAudio = has_analog_audio;

    if(extraFloats.count("audio_filterwidth") && extraFloats["audio_filterwidth"] > 0.0) {
        decoderParams[AUDIO_FILTERWIDTH] = extraFloats["audio_filterwidth"];
    }

    if(extraFloats.count("deemp_adjust")) {
        deempMult1 = extraFloats["deemp_adjust"];
        deempMult2 = extraFloats["deemp_adjust"];
    }

    if(extraFloats.count("deemp_low") && extraFloats["deemp_low"] > 0) {
        decoderParams[DEEMP_LOW] = extraFloats["deemp_low"];
    }

    if(extraFloats.count("deemp_high") && extraFloats["deemp_low"] > 0) {
        decoderParams[DEEMP_HIGH] = extraFloats["deemp_high"];
    }

    lineLen = (int)(freqHz / (10e6 / sysParams[LINE_PERIOD]));

    /*How much horizontal sync position can deviate from previous/expected position
    and still be interpreted as a horizontal sync pulse.
    Too high tolerance may result in false positive sync pulses, too low may end up missing them.
    Tapes will need a wider tolerance than laserdiscs due to head switch etc.*/

    hsyncTolerance = 0.4;

    decodeDigitalAudio = decode_digital_audio;

    decodeAnalogAudio = decode_analog_audio;

    //computeFilters();

    /*The 0.5mhz filter is rolled back to align with the data, so there
    are a few unusable samples at the end.*/
    //blockcutEnd = filters["F05_offset"];
};

void RFDecode::computeFilters() {
    computeVideoFilters();

    if(decodeAnalogAudio != 0) {
        computeAudioFilters();
    }

    if(decodeDigitalAudio) {
        computeEfmFilter();
    }

    //computeDelays();
}

void RFDecode::computeEfmFilter() {
    /*Frequency-domain equalisation filter for the LaserDisc EFM signal.
    This was inspired by the input signal equaliser in WSJT-X, described in
    Steven J. Franke and Joseph H. Taylor, "The MSK144 Protocol for
    Meteor-Scatter Communication", QEX July/August 2017.
    <http://physics.princeton.edu/pulsar/k1jt/MSK144_Protocol_QEX.pdf>

    This improved EFM filter was devised by Adam Sampson (@atsampson)*/

    //Frequency bands
    std::vector<double> freqs = linspace(0.0, 1900000.0, 11);
    double freqPerBin = freqHz / blocklen;

    //Amplitude and phase adjustments for each band.
    //These values were adjusted empirically based on a selection of NTSC and PAL samples.

    std::vector<double> amp = {0.0, 0.215, 0.41, 0.73, 0.98, 1.03, 0.99, 0.81, 0.59, 0.42, 0.0};
    std::vector<double> phase = {0.0, -0.92, -1.03, -1.11, -1.2, -1.2, -1.2, -1.2, -1.05, -0.95, -0.8};

    for(int i = 0; i < sizeof(phase) / sizeof(int); i++) {
        phase[i] *= 1.25;
    }

    //Compute filter coefficients for the given FFTFilter.
    //Anything above the highest frequency is left as zero.
    std::vector<std::complex<double>> coeffs;

    //Generate the frequency-domain coefficients by cubic interpolation between the equaliser values.
    tk::spline aInterp(freqs, amp);
    tk::spline pInterp(freqs, phase);

    int nonzeroBins = (int)(freqs.back() / freqPerBin) + 1;

    //bin_freqs = np.arange(nonzero_bins) * freq_per_bin
    std::vector<double> binFreqs = arange(nonzeroBins, freqPerBin);
    //bin_amp = a_interp(bin_freqs)
    std::vector<double> binAmp;
    for(auto number : binFreqs) { binAmp.push_back(aInterp.solve(number).front()); }
    //bin_phase = p_interp(bin_freqs)
    std::vector<double> binPhase;
    for(auto number : binFreqs) { binAmp.push_back(pInterp.solve(number).front()); }

    //Scale by the amplitude, rotate by the phase
    /*coeffs[:nonzero_bins] = bin_amp * (
        np.cos(bin_phase) + (complex(0, -1) * np.sin(bin_phase))
    )*/
    int i = 0;
    for( ; i < nonzeroBins; i++) {
        filtersFefm.push_back(binAmp[i] * 8 * (
            cos(binPhase[i]) + (std::complex<double>(0, -1) * sin(binPhase[i]))
        ));
    }

    //Anything above the highest frequency is left as zero.
    for( ; i < blocklen; i++) {
        filtersFefm.push_back(std::complex<double>(0,0));
    }
}

//helper function for computeEfmFilter()
std::vector<double> linspace(double start, double end, int num)
{
    std::vector<double> linspaced;

    int delta = (end - start) / (num - 1);

    for(int i=0; i < num-1; ++i) {
        linspaced.push_back(start + delta * i);
    }
    linspaced.push_back(end); // I want to ensure that start and end
                              // are exactly the same as the input
    return linspaced;
}

//another helper function for computeEfmFilter()
std::vector<double> arange(int end, double num)
{
    std::vector<double> aranged;

    for(int i=0; i < end; i++) {
        aranged.push_back(i * num);
    }

    return aranged;
}

void RFDecode::computeVideoFilters() {
    int filterOrder = 1;
    double overallGain = 1.0;
    
    vector <Biquad> coeffs;  // second-order sections (sos)
    Butterworth butterworth;
    
    bool designedCorrectly = butterworth.hiPass(
        freqHz,         // fs
        1,              // freq1, unused in hiPass
        (10/freqHalf),  // freq2
        filterOrder,
        coeffs,
        overallGain
    );
    
    //******************************************************************************
    //  MATLAB coefficients: first section
    //******************************************************************************
    
    double b0 = 1.0;
    double b1 = -2.0;
    double b2 = 1.0;
    // double a0 = 1.0;
    double a1 = -1.96762058043629 * (-1);
    double a2 = 0.97261960500367  * (-1);
    
    int i = 0;
    
    /*REQUIRE(abs(b0 - coeffs[i].b0) <= EPSILON);
    REQUIRE(abs(b1 - coeffs[i].b1) <= EPSILON);
    REQUIRE(abs(b2 - coeffs[i].b2) <= EPSILON);
    REQUIRE(abs(a1 - coeffs[i].a1) <= EPSILON);
    REQUIRE(abs(a2 - coeffs[i].a2) <= EPSILON);*/

    //self.Filters["Frfhpf"] = filtfft(Frfhpf, self.blocklen)
}

void RFDecode::computeAudioFilters() {}

float RFDecode::ireToHz(float ire) {}

float RFDecode::hzToIre(float hz) {}

std::string* RFDecode::demodBlock() {
    //rv["rfhpf"] = npfft.ifft(indata_fft * self.Filters["Frfhpf"]).real
}

std::string* RFDecode::runfilterAudioPhase2() {}

void RFDecode::audioPhase2(float mtfLevel) {}
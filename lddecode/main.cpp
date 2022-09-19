/************************************************************************

    main.cpp

    ld-decode - C++ port of the Python app ld-decode
    Copyright (C) 2018-2020 Simon Inns
    Copyright (C) 2019-2022 Adam Sampson
    Copyright (C) 2021 Chad Page
    Copyright (C) 2021 Phillip Blucas

    This file is part of ld-decode.

    ld-decode is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

************************************************************************/

#include <Python.h>
#include <QCoreApplication>
#include <QDebug>
#include <QtGlobal>
#include <QCommandLineParser>
#include <QThread>
#include <fstream>
#include <memory>
#include <iostream>

int main(int argc, char *argv[])
{
    // Install the local debug message handler
    //setDebug(true);
    //qInstallMessageHandler(debugOutputHandler);

    QCoreApplication a(argc, argv);

    // Set application name and version
    QCoreApplication::setApplicationName("ld-decode");
    QCoreApplication::setApplicationVersion(QString("Branch: %1 / Commit: %2").arg(APP_BRANCH, APP_COMMIT));
    QCoreApplication::setOrganizationDomain("domesday86.com");

    // Set up the command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription(
                "ld-decode - C++ port of the Python app ld-decode\n"
                "\n"
                "(c)2018-2020 Simon Inns\n"
                "(c)2019-2021 Adam Sampson\n"
                "(c)2018-2021 Chad Page\n"
                "(c)2021 Phillip Blucas\n"
                "GPLv3 Open-Source - github: https://github.com/happycube/ld-decode");
    parser.addHelpOption();
    parser.addVersionOption();

    // -- General options --

    // Add the standard debug options --debug and --quiet
    //addStandardDebugOptions(parser);

    /* Option to select start frame (sequential) (-s)
    QCommandLineOption startFrameOption(QStringList() << "s" << "start",
                QCoreApplication::translate("main", "rough jump to frame n of capture (default is 0)"),
                QCoreApplication::translate("main", "number"));
    parser.addOption(startFrameOption);

    // Option to select length (-l)
    QCommandLineOption lengthOption(QStringList() << "l" << "length",
                QCoreApplication::translate("main", "limit length to n frames"),
                QCoreApplication::translate("main", "number"));
    parser.addOption(lengthOption);

    // Option to seek to a specific frame
    QCommandLineOption seekFrameOption(QStringList() << "S" << "seek",
                QCoreApplication::translate("main", "seek to frame n of capture"),
                QCoreApplication::translate("main", "number"));
    parser.addOption(seekFrameOption);

    // Option to end at a specific frame
    /*QCommandLineOption endFrameOption(QStringList() << "E" << "end",
                QCoreApplication::translate("main", "cutting: last frame"),
                QCoreApplication::translate("main", "number"));
    parser.addOption(endFrameOption);*

    // Specify if something is in PAL format
    QCommandLineOption palOption(QStringList() << "PAL" << "p" << "pal",
                QCoreApplication::translate("main", "source is in PAL format"),
    parser.addOption(palOption);

    // Specify if something is in NTSC format
    QCommandLineOption ntscOption(QStringList() << "NTSC" << "n" << "ntsc",
                QCoreApplication::translate("main", "source is in NTSC format"),
    parser.addOption(ntscOption);

    // Specify if something is in NTSC-J format
    QCommandLineOption ntscJOption(QStringList() << "NTSCJ" << "j",
                QCoreApplication::translate("main", "source is in NTSC-J (IRE 0 black) format"),
    parser.addOption(ntscJOption);

    // Option to cut 10-bit format to 16-bit format
    /*QCommandLineOption endFrameOption(QStringList() << "c" << "cut",
                QCoreApplication::translate("main", "cut (to r16) instead of decode"),
    parser.addOption(endFrameOption);*

    // MTF compensation multiplier
    QCommandLineOption mtfOption(QStringList() << "m" << "MTF",
                QCoreApplication::translate("main", "mtf compensation multiplier"),
                QCoreApplication::translate("main", "number"));
    parser.addOption(mtfOption);

    // MTF compensation offset
    QCommandLineOption mtfOffsetOption(QStringList() << "MTF_offset",
                QCoreApplication::translate("main", "mtf compensation offset"),
                QCoreApplication::translate("main", "number"));
    parser.addOption(mtfOffsetOption);

    // Disable Automatic Gain Control
    QCommandLineOption noAGCOption(QStringList() << "noAGC",
                QCoreApplication::translate("main", "disable automatic gain control"),
    parser.addOption(noAGCOption);

    // Disable Dropout Detector
    QCommandLineOption noDODOption(QStringList() << "noDOD",
                QCoreApplication::translate("main", "disable dropout detector"),
    parser.addOption(noDODOption);

    // Disable Eight to Fourteen Modulation
    QCommandLineOption noEFMOption(QStringList() << "noEFM",
                QCoreApplication::translate("main", "Disable EFM (Eight to Fourteen Modulation) front end"),
    parser.addOption(noEFMOption);

    // Write filtered but otherwise pre-processed EFM data
    QCommandLineOption preEFMOption(QStringList() << "preEFM",
                QCoreApplication::translate("main", "Write filtered but otherwise pre-processed EFM data"),
    parser.addOption(preEFMOption);

    // Disable analog audio
    QCommandLineOption noAnalogAudioOption(QStringList() << "disable_analog_audio" << "disable_analogue_audio" << "daa",
                QCoreApplication::translate("main", "Write filtered but otherwise pre-processed EFM data"),
    parser.addOption(noAnalogAudioOption);

    // jump to precise sample # in the file
    QCommandLineOption startFilelocOption(QStringList() << "start_fileloc",
                QCoreApplication::translate("main", "jump to precise sample # in the file"),
                QCoreApplication::translate("main", "number"));
    parser.addOption(startFilelocOption);

    // ignore leadout
    QCommandLineOption ignroeLeadoutOption(QStringList() << "ignore_leadout",
                QCoreApplication::translate("main", "continue decoding after lead-out seen"),
    parser.addOption(ignroeLeadoutOption);

    // verbose VITS
    QCommandLineOption verboseVITSOption(QStringList() << "verboseVITS",
                QCoreApplication::translate("main", "Enable additional JSON fields"),
    parser.addOption(verboseVITSOption);

    // rf time-base corrected
    QCommandLineOption RFTBCOption(QStringList() << "RF_TBC",
                QCoreApplication::translate("main", "Create a .tbc.ldf file with TBC'd RF"),
    parser.addOption(RFTBCOption);

    // lowband for noisier discs
    QCommandLineOption lowbandOption(QStringList() << "lowband",
                QCoreApplication::translate("main", "Use more restricted RF settings for noisier discs"),
    parser.addOption(lowbandOption);

    // NTSC color notch filter
    QCommandLineOption ntscColorNotchOption(QStringList() << "NTSC_color_notch_filter" << "N",
                QCoreApplication::translate("main", "Mitigate interference from analog audio in reds in NTSC captures"),
    parser.addOption(ntscColorNotchOption);

    // V4300D notch filter
    QCommandLineOption v4300DOption(QStringList() << "V4300D_notch_filter" << "V",
                QCoreApplication::translate("main", "LD-V4300D PAL/digital audio captures: remove spurious ~8.5mhz signal"),
    parser.addOption(v4300DOption);

    // Deemphasis level multiplier
    QCommandLineOption deempAdjustOption(QStringList() << "d" << "deemp_adjust",
                QCoreApplication::translate("main", "Deemphasis level multiplier"),
                QCoreApplication::translate("main", "number"));
    parser.addOption(deempAdjustOption);

    // Deemphasis low coefficient
    QCommandLineOption deempLowOption(QStringList() << "deemp_low",
                QCoreApplication::translate("main", "Deemphasis low coefficient"),
                QCoreApplication::translate("main", "number"));
    parser.addOption(deempLowOption);

    // Deemphasis high coefficient
    QCommandLineOption deempHighOption(QStringList() << "deemp_high",
                QCoreApplication::translate("main", "Deemphasis high coefficient"),
                QCoreApplication::translate("main", "number"));
    parser.addOption(deempHighOption);

    // number of CPU threads to use
    QCommandLineOption threadsOption(QStringList() << "threads" << "t",
                QCoreApplication::translate("main", "number of CPU threads to use"),
                QCoreApplication::translate("main", "number"));
    parser.addOption(threadsOption);

    // RF sampling frequency in source file (default is 40MHz)
    QCommandLineOption frequencyOption(QStringList() << "frequency" << "f",
                QCoreApplication::translate("main", "RF sampling frequency in source file (default is 40MHz)"),
                QCoreApplication::translate("main", "number"));
    parser.addOption(frequencyOption);

    // RF sampling frequency in source file (default is 44100hz)
    QCommandLineOption analogAudioFrequencyOption(QStringList() << "analog-audio-frequency",
                QCoreApplication::translate("main", "RF sampling frequency in source file (default is 44100hz)"),
                QCoreApplication::translate("main", "number"));
    parser.addOption(analogAudioFrequencyOption);

    // Video BPF high end frequency
    QCommandLineOption videoBPFOption(QStringList() << "video_bpf_high",
                QCoreApplication::translate("main", "Video BPF high end frequency"),
                QCoreApplication::translate("main", "number"));
    parser.addOption(videoBPFOption);
    
    // Video LPF frequency
    QCommandLineOption videoLPFOption(QStringList() << "video_lpf",
                QCoreApplication::translate("main", "Video low-pass filter frequency"),
                QCoreApplication::translate("main", "number"));
    parser.addOption(videoLPFOption);

    // Video LPF filter order
    QCommandLineOption videoLPFOrderOption(QStringList() << "video_lpf_order",
                QCoreApplication::translate("main", "Video low-pass filter order"),
                QCoreApplication::translate("main", "number"));
    parser.addOption(videoLPFOrderOption);

    // Analog audio filter width
    QCommandLineOption audioFilterwidthOption(QStringList() << "audio_filterwidth",
                QCoreApplication::translate("main", "Analog audio filter width"),
                QCoreApplication::translate("main", "number"));
    parser.addOption(audioFilterwidthOption);

    // -- Positional arguments --

    // Positional argument to specify input video file
    parser.addPositionalArgument("input", QCoreApplication::translate("main", "source file"));

    // Positional argument to specify output video file
    parser.addPositionalArgument("output", QCoreApplication::translate("main", "base name for destination files"));*/

    // Process the command line options and arguments given by the user
    parser.process(a);

    // Standard logging options
    //processStandardDebugOptions(parser);

    // Get the arguments from the parser
    QString inputFileName;
    QString outputFileName = "-";
    QStringList positionalArguments = parser.positionalArguments();
    if (positionalArguments.count() == 2) {
        inputFileName = positionalArguments.at(0);
        outputFileName = positionalArguments.at(1);
    } else if (positionalArguments.count() == 1) {
        inputFileName = positionalArguments.at(0);
        //std::cout << inputFileName.toStdString() << std::endl;
    } else {
        // Quit with error
        qCritical("You must specify the input ldf file");
        return -1;
    }

    // Check filename arguments are reasonable
    if (inputFileName == "-") {//} && !parser.isSet(inputJsonOption)) {
        // Quit with error
        qCritical("With piped input, you must also specify the input JSON file");
        return -1;
    }
    if (inputFileName == outputFileName && outputFileName != "-") {
        // Quit with error
        qCritical("Input and output files cannot be the same");
        return -1;
    }

    /*qint32 startFrame = -1;
    qint32 length = -1;
    qint32 maxThreads = QThread::idealThreadCount();
    //PalColour::Configuration palConfig;
    //Comb::Configuration combConfig;
    OutputWriter::Configuration outputConfig;

    if (parser.isSet(lengthOption)) {
        length = parser.value(lengthOption).toInt();

        if (length < 1) {
            // Quit with error
            qCritical("Specified length must be greater than zero frames");
            return -1;
        }
    }

    if (parser.isSet(threadsOption)) {
        maxThreads = parser.value(threadsOption).toInt();

        if (maxThreads < 1) {
            // Quit with error
            qCritical("Specified number of threads must be greater than zero");
            return -1;
        }
    }

    //Options that involve booleans
    std::map<std::string,bool> extraBools;
    QString audioPipe = "None";

    extraBools.insert(pair<std::string,bool>("useAGC",!parser.isSet(noAGCOption)));
    extraBools.insert(pair<std::string,bool>("write_RF_TBC",parser.isSet(RFTBCOption)));
    extraBools.insert(pair<std::string,bool>("write_pre_efm",parser.isSet(preEFMOption)));
    extraBools.insert(pair<std::string,bool>("lowband",parser.isSet(lowbandOption)));
    extraBools.insert(pair<std::string,bool>("verboseVITS",parser.isSet(verboseVITSOption)));
    
    QString standard;
    float blackIRE = 0.0;

    //Decide if it's PAL
    if (parser.isSet(palOption)) {
        standard = "PAL";

        if (parser.isSet(ntscOption) || parser.isSet(ntscJOption)) {
            // Quit with error
            qCritical("ERROR: Can only be PAL or NTSC");
            return -1;
        }

        extraBools.insert(pair<std::string,bool>("PAL_V4300D_NotchFilter",paser.isSet(v4300DOption)));
    }
    else {
        standard = "NTSC";

        extraBools.insert(pair<std::string,bool>("NTSC_ColorNotchFilter",parser.isSet(ntscColorNotchOption)));

        if(!parser.isSet(ntscJOption)) {
            blackIRE = 7.5;
        }
    }
    
    //Options that involve integers
    std::map<std::string,int> extraInts;

    if (parser.isSet(seekFrameOption)) {
        extraInts.insert(pair<std::string,int>("seek",paser.value(deempAdjustOption)));
    }

    //Options that involve sampling frequencies
    int inputFrequency = 40000000;
    int frequencies [] = {40000000, 0, 0, 0}
    if (parser.isSet(frequencyOption)) {
        inputFrequency = parser.value(frequencyOption);
    }

    if (parser.isSet(videoBPFOption)) {
        frequencies[1] = parser.value(videoBPFOption);
    }

    if (parser.isSet(videoLPFOption)) {
        frequencies[2] = parser.value(videoLPFOption);
    }

    if (parser.isSet(videoLPFOrderOption)) {
        frequencies[3] = parser.value(videoLPFOrderOption);
    }

    //Options that involve floats
    std::map<std::string,float> extraFloats;

    if (parser.isSet(deempAdjustOption)) {
        extraFloats.insert(pair<std::string,float>("deemp_adjust",paser.value(deempAdjustOption)));
    }

    if (parser.isSet(deempLowOption)) {
        extraFloats.insert(pair<std::string,float>("deemp_low",parser.value(deempLowOption)));
    }

    if (parser.isSet(deempHighOption)) {
        extraFloats.insert(pair<std::string,float>("deemp_high",parser.value(deempHighOption)));
    }

    if (parser.isSet(audioFilterwidthOption)) {
        extraFloats.insert(pair<std::string,float>("audio_filterwidth",parser.value(audioFilterwidthOption)));
    }

    if (parser.isSet(startFilelocOption)) {
        extraFloats.insert(pair<std::string,float>("start_fileloc"),parser.value(startFilelocOption));
    }

    if (parser.isSet(mtfOption)) {
        extraFloats.insert(pair<std::string,float>("mtf_offset"),parser.value(mtfOption));
    }

    if (parser.isSet(mtfOffsetOption)) {
        extraFloats.insert(pair<std::string,float>("start_fileloc"),parser.value(mtfOffsetOption));
    }*/

    //Python execution goes here
    Py_Initialize();

    const unsigned short inputLength = inputFileName.length();

    wchar_t input[512];

    int irrelevent = inputFileName.toWCharArray(input);
    ~irrelevent;

    wchar_t* params[3];
    params[0] = L"ld-decode";
    params[1] = input;
    params[2] = L"-";

    Py_SetProgramName(L"ld-decode");

    PySys_SetArgv(3, params);

    FILE* ldDecodePython;
    ldDecodePython = fopen("ld-decode", "r");

    PyRun_AnyFile(ldDecodePython, "ld-decode");
    
    if(Py_FinalizeEx() < 0) {
        exit(120);
    }
    // Load the raw RF capture
    /*LDdecode ldd = new LDdecode(/*all sorts of variables, including Dropout Detector and Analog Audio);
    if (!ldd.read(inputFileName, inputFrequency)) {
        qInfo() << "Unable to open ldf file";
        return -1;
    }

    bool done = false;

    while(!done && ldd.fields_written < length) {
        //ignore leadout is referenced here
    }

    cout << "\nCompleted: saving JSON and exiting";*/

    return 0;
}
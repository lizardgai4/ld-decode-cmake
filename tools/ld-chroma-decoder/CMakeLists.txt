# Library for chroma decoders

add_library(lddecode-chroma STATIC
    comb.cpp
    componentframe.cpp
    framecanvas.cpp
    outputwriter.cpp
    palcolour.cpp
    sourcefield.cpp
    transformpal.cpp
    transformpal2d.cpp
    transformpal3d.cpp
)

target_include_directories(lddecode-chroma PUBLIC .)

if(USE_QWT)
    if(UNIX AND NOT APPLE)
        target_link_libraries(lddecode-chroma PRIVATE Qt::Core PkgConfig::FFTW lddecode-library)
    endif()
    if(WIN32)
        set(FFTW_LIBRARY "C:\\Program Files\\fftw")
        target_link_libraries(lddecode-chroma PRIVATE Qt::Core FFTW lddecode-library)
    endif()
endif()

# ld-chroma-decoder

add_executable(ld-chroma-decoder
    decoder.cpp
    decoderpool.cpp
    main.cpp
    monodecoder.cpp
    ntscdecoder.cpp
    paldecoder.cpp
)

if(UNIX AND NOT APPLE)
    pkg_check_modules(FFTW REQUIRED IMPORTED_TARGET
        fftw3
    )
    target_link_libraries(ld-chroma-decoder PRIVATE Qt::Core PkgConfig::FFTW lddecode-library lddecode-chroma)
endif()
if(WIN32)
    target_link_libraries(ld-chroma-decoder PRIVATE Qt::Core FFTW lddecode-library lddecode-chroma)
endif()

install(TARGETS ld-chroma-decoder)
add_executable(lddecode-c
    main.cpp
)

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
message("Python3_INCLUDE_DIRS: ${Python3_INCLUDE_DIRS}")
message("Python3_LIBRARY_DIRS: ${Python3_LIBRARY_DIRS}")
include_directories(lddecode-c ${Python3_INCLUDE_DIRS})
link_libraries(lddecode-c "${Python3_LIBRARY_DIRS}/config-3.10-x86_64-linux-gnu")
target_link_libraries(lddecode-c PRIVATE Qt::Core)

install(TARGETS lddecode-c)

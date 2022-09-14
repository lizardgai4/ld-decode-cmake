add_executable(lddecode-c
    main.cpp
)

find_package(Python3 REQUIRED COMPONENTS Development)

include_directories(lddecode-c ${Python3_INCLUDE_DIRS})
target_link_libraries(lddecode-c PRIVATE Qt::Core ${Python3_LIBRARIES})

install(TARGETS lddecode-c)

find_package(Boost REQUIRED COMPONENTS program_options)
find_package(fmt REQUIRED)

add_executable(main main.cpp)
target_link_libraries(main edge-tts ${Boost_LIBRARIES} fmt::fmt ssl crypto)

find_package(sndfile REQUIRED)

add_executable(play play.cpp)
target_link_libraries(play ${Boost_LIBRARIES} ${SndFile_LIBRARY} portaudio)

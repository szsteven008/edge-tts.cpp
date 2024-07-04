find_package(Boost REQUIRED COMPONENTS program_options)
find_package(fmt REQUIRED)

add_executable(main main.cpp)
target_link_libraries(main edge-tts ${Boost_LIBRARIES} fmt::fmt ssl crypto)
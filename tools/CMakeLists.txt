# copyright defined in abieos/LICENSE.txt

find_package(Threads)

add_executable(name name.cpp)
target_link_libraries(name abieos ${CMAKE_THREAD_LIBS_INIT})

add_executable(generate_hex_from_json util_generate_hex_from_json.cpp ../src/abieos.cpp)
target_link_libraries(generate_hex_from_json abieos ${CMAKE_THREAD_LIBS_INIT})

add_executable(generate_json_from_hex util_generate_json_from_hex.cpp ../src/abieos.cpp)
target_link_libraries(generate_json_from_hex abieos ${CMAKE_THREAD_LIBS_INIT})

add_custom_command( TARGET name POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:name> ${CMAKE_CURRENT_BINARY_DIR}/name2num )
add_custom_command( TARGET name POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:name> ${CMAKE_CURRENT_BINARY_DIR}/num2name )


string(REPLACE " " ";" REPLACED_FLAGS ${CXX_FLAGS})

add_library(ljson03 ljson.cc)
add_executable(ljson_test03 ljson_test.cc)
target_compile_options(ljson03 PUBLIC ${REPLACED_FLAGS})
target_link_libraries(ljson_test03 PRIVATE ljson03)

get_target_property(MAIN_CFLAGS ljson03 COMPILE_OPTIONS)
message(STATUS "ljson03 ${MAIN_CFLAGS}")
get_target_property(EXEC_CFLAGS ljson_test03 COMPILE_OPTIONS)
message(STATUS "ljson_test03 ${EXEC_CFLAGS}")

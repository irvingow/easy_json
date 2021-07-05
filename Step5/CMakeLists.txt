
string(REPLACE " " ";" REPLACED_FLAGS ${CXX_FLAGS})

add_library(ljson05 ljson.cc)
add_executable(ljson_test05 ljson_test.cc)
target_compile_options(ljson05 PUBLIC ${REPLACED_FLAGS})
target_link_libraries(ljson_test05 PRIVATE ljson05)

get_target_property(MAIN_CFLAGS ljson05 COMPILE_OPTIONS)
message(STATUS "ljson05 ${MAIN_CFLAGS}")
get_target_property(EXEC_CFLAGS ljson_test05 COMPILE_OPTIONS)
message(STATUS "ljson_test05 ${EXEC_CFLAGS}")


string(REPLACE " " ";" REPLACED_FLAGS ${CXX_FLAGS})

add_library(ljson06 ljson.cc)
add_executable(ljson_test06 ljson_test.cc)
target_compile_options(ljson06 PUBLIC ${REPLACED_FLAGS})
target_link_libraries(ljson_test06 PRIVATE ljson06)

get_target_property(MAIN_CFLAGS ljson06 COMPILE_OPTIONS)
message(STATUS "ljson06 ${MAIN_CFLAGS}")
get_target_property(EXEC_CFLAGS ljson_test06 COMPILE_OPTIONS)
message(STATUS "ljson_test06 ${EXEC_CFLAGS}")



string(REPLACE " " ";" REPLACED_FLAGS ${CXX_FLAGS})

add_library(ljson07 ljson.cc)
add_executable(ljson_test07 ljson_test.cc)
target_compile_options(ljson07 PUBLIC ${REPLACED_FLAGS})
target_link_libraries(ljson_test07 PRIVATE ljson07)

get_target_property(MAIN_CFLAGS ljson07 COMPILE_OPTIONS)
message(STATUS "ljson07 ${MAIN_CFLAGS}")
get_target_property(EXEC_CFLAGS ljson_test07 COMPILE_OPTIONS)
message(STATUS "ljson_test07 ${EXEC_CFLAGS}")


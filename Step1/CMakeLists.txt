
string(REPLACE " " ";" REPLACED_FLAGS ${CXX_FLAGS})

add_library(ljson01 ljson.cc)
add_executable(ljson_test01 ljson_test.cc)
target_compile_options(ljson01 PRIVATE ${REPLACED_FLAGS})
target_link_libraries(ljson_test01 PRIVATE ljson01)

get_target_property(MAIN_CFLAGS ljson01 COMPILE_OPTIONS)
message(STATUS "ljson01 ${MAIN_CFLAGS}")
get_target_property(EXEC_CFLAGS ljson_test01 COMPILE_OPTIONS)
message(STATUS "ljson_test01 ${EXEC_CFLAGS}")

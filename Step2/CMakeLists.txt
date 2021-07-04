
string(REPLACE " " ";" REPLACED_FLAGS ${CXX_FLAGS})

add_library(ljson02 ljson.cc)
add_executable(ljson_test02 ljson_test.cc)
target_compile_options(ljson02 PRIVATE ${REPLACED_FLAGS})
target_link_libraries(ljson_test02 PUBLIC ljson02)

get_target_property(MAIN_CFLAGS ljson02 COMPILE_OPTIONS)
message(STATUS "ljson02 ${MAIN_CFLAGS}")
get_target_property(EXEC_CFLAGS ljson_test02 COMPILE_OPTIONS)
message(STATUS "ljson_test02 ${EXEC_CFLAGS}")

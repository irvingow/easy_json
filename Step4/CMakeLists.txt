
string(REPLACE " " ";" REPLACED_FLAGS ${CXX_FLAGS})

add_library(ljson04 ljson.cc)
add_executable(ljson_test04 ljson_test.cc)
target_compile_options(ljson04 PUBLIC ${REPLACED_FLAGS})
target_link_libraries(ljson_test04 PRIVATE ljson04)

get_target_property(MAIN_CFLAGS ljson04 COMPILE_OPTIONS)
message(STATUS "ljson04 ${MAIN_CFLAGS}")
get_target_property(EXEC_CFLAGS ljson_test04 COMPILE_OPTIONS)
message(STATUS "ljson_test04 ${EXEC_CFLAGS}")

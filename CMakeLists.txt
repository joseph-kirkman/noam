cmake_minimum_required(VERSION 3.9)
project(noam C)

set(CMAKE_C_STANDARD 99)
include_directories(include)
add_executable(noam noam.h noam.c include/noam_buffer.h src/noam_buffer.c include/noam_dict.h src/noam_dict.c src/noam_utility.c include/noam_utility.h src/noam_lexer.c include/noam_lexer.h src/noam_expression.c include/noam_expression.h src/noam_statement.c include/noam_statement.h include/noam_symbol.h src/noam_symbol.c src/noam_parser.c include/noam_parser.h)
target_link_libraries(noam)
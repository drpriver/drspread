cmake_minimum_required(VERSION 3.14) # This is a guess
project(DrSpread VERSION 1.0.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

if(WIN32)
add_compile_options("-D_CRT_NONSTDC_NO_DEPRECATE" "-D_CRT_SECURE_NO_WARNINGS")
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
add_compile_options("/wd4244" "/wd5105" "/wd4146" "/wd4267" "/wd4018" "/wd4141" "/wd4163")
endif()
elseif(APPLE)
elseif(UNIX)
set(LIBM_LIBRARIES m)
endif()

add_library(drspread-dylib
    SHARED
	drspread.c
)
set_target_properties(drspread-dylib
    PROPERTIES OUTPUT_NAME drspread
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION}
	PUBLIC_HEADER "drspread.h"
)
target_compile_definitions(drspread-dylib 
	PRIVATE 
	DRSP_DYLIB=1
)
add_library(drspread-test-dylib
    SHARED
	drspread.c
)
set_target_properties(drspread-test-dylib
    PROPERTIES OUTPUT_NAME drspread-test
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION}
	PUBLIC_HEADER "drspread.h"
)
target_compile_definitions(drspread-test-dylib 
	PRIVATE 
	DRSP_DYLIB=1
	DRSP_INTRINS=1
	DRSP_TEST_DYLINK=1
)
add_library(drspread-lib
    STATIC
	drspread.c
)
if(WIN32)
set_target_properties(drspread-lib
    PROPERTIES
	OUTPUT_NAME drspread-static
)
else()
set_target_properties(drspread-lib
    PROPERTIES
	OUTPUT_NAME drspread
)
endif()
set_target_properties(drspread-lib
	PROPERTIES
    VERSION ${PROJECT_VERSION}
)

add_executable(drspread drspread_cli.c)
add_executable(drsp drspread_tui.c)
target_link_libraries(drspread-lib ${LIBM_LIBRARIES})
target_link_libraries(drspread-dylib ${LIBM_LIBRARIES})
target_link_libraries(drspread-test-dylib ${LIBM_LIBRARIES})

install(TARGETS drsp DESTINATION bin)
install(TARGETS drspread-lib LIBRARY DESTINATION lib)
install(TARGETS drspread-dylib
		LIBRARY DESTINATION lib
		PUBLIC_HEADER DESTINATION include/DrSpread)

add_executable(test-drspread TestDrSpread.c)
add_executable(test-drspread-dy-link TestDrSpread.c)
target_link_libraries(test-drspread-dy-link drspread-test-dylib)
target_compile_definitions(test-drspread-dy-link 
	PRIVATE 
	DRSP_TEST_DYLINK=1
)

enable_testing()
add_test(test-drspread test-drspread)
add_test(test-drspread-dy-link test-drspread-dy-link)


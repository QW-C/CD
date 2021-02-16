cmake_minimum_required(VERSION 3.11)

add_definitions(-DUNICODE -D_UNICODE)
add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(CD)

add_subdirectory(CD)

function(link_and_copy EXECUTABLE)
	add_executable(${EXECUTABLE} WIN32)
	target_include_directories(${EXECUTABLE} PRIVATE ${PROJECT_SOURCE_DIR}/Scenes)

	target_include_directories(${EXECUTABLE} PRIVATE ${PROJECT_SOURCE_DIR}/CD)
	target_link_libraries(${EXECUTABLE} PRIVATE CD)

	file(GLOB_RECURSE SHARED_LIBRARIES ${PROJECT_SOURCE_DIR}/External/*.dll)
	add_custom_command(
		TARGET ${EXECUTABLE}
		PRE_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SHARED_LIBRARIES} $<TARGET_FILE_DIR:${EXECUTABLE}>
	)

	add_custom_command(
		TARGET ${EXECUTABLE}
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/CD/CD/Resources ${CMAKE_CURRENT_BINARY_DIR}
	)
endfunction()

add_subdirectory(Scenes)
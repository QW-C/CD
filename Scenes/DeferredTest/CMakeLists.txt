link_and_copy(DeferredTest)

set(DEFERRED_SRC Main.cpp Main.hpp)
source_group("src" FILES ${DEFERRED_SRC})

target_sources(DeferredTest PRIVATE ${DEFERRED_SRC})

add_custom_command(
	TARGET DeferredTest
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/Scenes/Assets ${CMAKE_CURRENT_BINARY_DIR}
)
list(APPEND INC
	"Debug.h"
	"File.h"
	"Mixer.h"
	"Menu.h"
	"OpenGL.h"
	"Resources.h"
	"System.h"
	"Thread.h"
)

if(WIN32)
	list(APPEND SRC
		"Debug.cpp"
		"File.cpp"
		"Mixer.cpp"
		"Menu.cpp"
		"System.cpp"
		"Thread.cpp"
	)
else()
	list(APPEND SRC
		"Debug.cpp"
		"File.cpp"
		"Mixer.cpp"
		"Menu.cpp"
		"System.cpp"
		"Thread.cpp"
	)
	if(APPLE)
		list(APPEND SRC "NativeMenuBarMacOS.mm")
		list(APPEND INC "NativeMenuBarMacOS.h")
	endif()
endif()

add_library(System STATIC ${SRC} ${INC})

target_link_libraries(System PRIVATE Core Editor OpenGL::GL)
target_link_libraries(System PUBLIC SDL3::SDL3)

if(APPLE)
	find_library(APPKIT_FRAMEWORK AppKit REQUIRED)
	target_link_libraries(System PRIVATE ${APPKIT_FRAMEWORK})
endif()

if(WIN32)
	add_executable(ArrowVortex WIN32 System.cpp "${CMAKE_CURRENT_SOURCE_DIR}/ArrowVortex.rc")
else()
	add_executable(ArrowVortex System.cpp)
endif()

if(FFMPEG_FOUND)
	target_link_directories(ArrowVortex PRIVATE ${FFMPEG_LIBRARY_DIRS})
endif()

add_custom_command(
	TARGET ArrowVortex POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${PROJECT_SOURCE_DIR}/bin
		$<TARGET_FILE_DIR:ArrowVortex>)

target_link_libraries(ArrowVortex PRIVATE System)

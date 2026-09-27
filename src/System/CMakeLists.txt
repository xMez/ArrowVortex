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
		list(APPEND SRC "SystemMacOS.mm" "NativeMenuBarMacOS.mm")
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

if(APPLE)
	set(MACOS_ICON "${CMAKE_CURRENT_SOURCE_DIR}/ArrowVortex.icns")
	set_source_files_properties("${MACOS_ICON}" PROPERTIES
		MACOSX_PACKAGE_LOCATION "Resources"
	)
	target_sources(ArrowVortex PRIVATE "${MACOS_ICON}")
	set_target_properties(ArrowVortex PROPERTIES
		MACOSX_BUNDLE TRUE
		MACOSX_BUNDLE_BUNDLE_NAME "ArrowVortex"
		MACOSX_BUNDLE_GUI_IDENTIFIER "com.arrowvortex.ArrowVortex"
		MACOSX_BUNDLE_BUNDLE_VERSION "1"
		MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
		MACOSX_BUNDLE_ICON_FILE "ArrowVortex.icns"
	)
endif()

if(FFMPEG_FOUND)
	target_link_directories(ArrowVortex PRIVATE ${FFMPEG_LIBRARY_DIRS})
endif()

if(APPLE)
	add_custom_command(
		TARGET ArrowVortex POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			${PROJECT_SOURCE_DIR}/bin
			$<TARGET_BUNDLE_CONTENT_DIR:ArrowVortex>/Resources)
else()
	add_custom_command(
		TARGET ArrowVortex POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			${PROJECT_SOURCE_DIR}/bin
			$<TARGET_FILE_DIR:ArrowVortex>)
endif()

target_link_libraries(ArrowVortex PRIVATE System)

cmake_minimum_required(VERSION 3.28)
project(InteractivePathPlanning VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# link the C++ runtime into the exe so it runs on its own on Windows
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# The app goes in build/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# wxWidgets API grabbing
# download the api if it hasnt been installed already

include(FetchContent)

# static libraries, without the optional libraries this app doesn't use
set(wxBUILD_SHARED OFF)
set(wxUSE_AUI OFF)
set(wxUSE_HTML OFF)
set(wxUSE_MEDIACTRL OFF)
set(wxUSE_OPENGL OFF)
set(wxUSE_PROPGRID OFF)
set(wxUSE_RIBBON OFF)
set(wxUSE_RICHTEXT OFF)
set(wxUSE_STC OFF)
set(wxUSE_WEBVIEW OFF)
set(wxUSE_WXHTML_HELP OFF)
set(wxUSE_XRC OFF)

FetchContent_Declare(wxWidgets
	URL https://github.com/wxWidgets/wxWidgets/releases/download/v3.2.11/wxWidgets-3.2.11.tar.bz2
	URL_HASH SHA256=6a129015bce2e914e4bf61ec4411854ad962801d47e92f2eb8340adb6a90af08
	EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(wxWidgets)

# App

set(APP_DIR InteractivePathPlanning)

# every .h and .cpp in the app folder, checked again on each build so new files get picked up
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
	${APP_DIR}/*.cpp
	${APP_DIR}/*.h
)

# resources for Windows variant
if(WIN32)
	list(APPEND SOURCES ${APP_DIR}/Resources/InteractivePathPlanning.rc)
endif()

# Files packed into the app: the name the code uses, then the file
include(cmake/EmbedFiles.cmake)
set(EMBEDDED_CPP "${CMAKE_CURRENT_BINARY_DIR}/generated/EmbeddedFiles.cpp")
embed_files(${EMBEDDED_CPP}
	APP_ICON          ${APP_DIR}/Resources/app.ico
	ICON_TILE_STATE   ${APP_DIR}/Resources/Icons/TileState_Black.svg
	ICON_COST_PAINTER ${APP_DIR}/Resources/Icons/CostPainter.svg
	ICON_ANNOTATE     ${APP_DIR}/Resources/Icons/Annotate.svg
	ICON_MAP          ${APP_DIR}/Resources/Icons/Map.svg
	ICON_VIEW         ${APP_DIR}/Resources/Icons/View.svg
	ICON_CHEVRON_DOWN ${APP_DIR}/Resources/Icons/ChevronDown.svg
	ICON_CHEVRON_LEFT ${APP_DIR}/Resources/Icons/ChevronLeft.svg
	ICON_FIND_PATH    ${APP_DIR}/Resources/Icons/FindPath.svg
	ICON_PLAY         ${APP_DIR}/Resources/Icons/Play.svg
	ICON_PAUSE        ${APP_DIR}/Resources/Icons/Pause.svg
	ICON_STEP         ${APP_DIR}/Resources/Icons/Step.svg
	ICON_CLEAR        ${APP_DIR}/Resources/Icons/Clear.svg
	ICON_RESET        ${APP_DIR}/Resources/Icons/Reset.svg
	ICON_HELP         ${APP_DIR}/Resources/Icons/Help.svg
)

add_executable(InteractivePathPlanning WIN32 MACOSX_BUNDLE ${SOURCES} ${EMBEDDED_CPP})
target_include_directories(InteractivePathPlanning PRIVATE ${APP_DIR})
target_link_libraries(InteractivePathPlanning PRIVATE wx::core wx::base)

if(MSVC)
	# the manifest comes from the .rc file, so the linker doesn't make another one
	target_link_options(InteractivePathPlanning PRIVATE /MANIFEST:NO)
	target_compile_options(InteractivePathPlanning PRIVATE /W3 /MP)
endif()

if(APPLE)
	set_target_properties(InteractivePathPlanning PROPERTIES
		OUTPUT_NAME "Interactive Path Planning"
		MACOSX_BUNDLE_BUNDLE_NAME "Interactive Path Planning"
		MACOSX_BUNDLE_GUI_IDENTIFIER "local.InteractivePathPlanning"
		MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/${APP_DIR}/Resources/macOS/Info.plist.in")
endif()

# Visual Studio: filters that match the folders, and F5 runs the app
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/${APP_DIR}" FILES ${SOURCES})
source_group("Generated" FILES ${EMBEDDED_CPP})
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT InteractivePathPlanning)

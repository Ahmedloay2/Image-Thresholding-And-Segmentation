cmake_minimum_required(VERSION 3.16)
project(ImageThresholdingAndSegmentation LANGUAGES CXX)

# ── C++ Standard ────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Project Directories ─────────────────────────────────────────
set(PROJECT_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
set(PROJECT_SRC_DIR     "${CMAKE_SOURCE_DIR}/src")
set(PROJECT_UI_DIR      "${CMAKE_SOURCE_DIR}/ui")

# ── Qt Automatic Code Generation ────────────────────────────────
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS "${PROJECT_UI_DIR}")

# ── Dependencies ────────────────────────────────────────────────
find_package(OpenCV REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)
find_package(OpenMP REQUIRED)

# ── Source Files ────────────────────────────────────────────────
file(GLOB_RECURSE SRC_FILES
    "${PROJECT_SRC_DIR}/*.cpp"
)

file(GLOB_RECURSE HEADER_FILES
    "${PROJECT_INCLUDE_DIR}/*.hpp"
    "${PROJECT_INCLUDE_DIR}/*.h"
)

file(GLOB_RECURSE UI_FILES
    "${PROJECT_UI_DIR}/*.ui"
)

# ── Executable ──────────────────────────────────────────────────
add_executable(${PROJECT_NAME}
    ${SRC_FILES}
    ${HEADER_FILES}
    ${UI_FILES}
)

# ── Include Directories ─────────────────────────────────────────
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIRS}
)

# ── Link Libraries ──────────────────────────────────────────────
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${OpenCV_LIBS}
    Qt6::Widgets
    Qt6::Concurrent
    OpenMP::OpenMP_CXX
)

# ── OpenCV DLL Deployment (Windows) ─────────────────────────────
if(WIN32)
    foreach(opencv_lib ${OpenCV_LIBS})
        if(TARGET ${opencv_lib})
            get_target_property(OPENCV_LIB_PATH ${opencv_lib} IMPORTED_LOCATION_DEBUG)
            if(NOT OPENCV_LIB_PATH)
                get_target_property(OPENCV_LIB_PATH ${opencv_lib} IMPORTED_LOCATION_RELEASE)
            endif()

            if(OPENCV_LIB_PATH)
                get_filename_component(OPENCV_LIB_DIR "${OPENCV_LIB_PATH}" DIRECTORY)
                string(REPLACE "/lib" "/bin" OPENCV_BIN_DIR "${OPENCV_LIB_DIR}")

                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${OPENCV_BIN_DIR}"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                    COMMENT "Copying OpenCV DLLs..."
                )
                break()
            endif()
        endif()
    endforeach()
endif()

# ── Qt DLL Deployment (Windows) ─────────────────────────────────
if(WIN32)
    get_target_property(QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QMAKE_EXECUTABLE}" DIRECTORY)

    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt
        HINTS "${QT_BIN_DIR}"
    )

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${WINDEPLOYQT_EXECUTABLE}
                --no-translations
                --compiler-runtime
                $<$<CONFIG:Debug>:--debug>
                $<$<CONFIG:Release>:--release>
                "$<TARGET_FILE:${PROJECT_NAME}>"
            COMMENT "Deploying Qt dependencies..."
        )
    endif()
endif()

# ── Visual Studio Startup Project ───────────────────────────────
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME}
)

# ── Optional: Organize Source Groups in Visual Studio ───────────
source_group(TREE ${PROJECT_SRC_DIR} PREFIX "Source Files" FILES ${SRC_FILES})
source_group(TREE ${PROJECT_INCLUDE_DIR} PREFIX "Header Files" FILES ${HEADER_FILES})
source_group(TREE ${PROJECT_UI_DIR} PREFIX "UI Files" FILES ${UI_FILES})
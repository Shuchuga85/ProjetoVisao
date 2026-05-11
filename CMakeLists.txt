cmake_minimum_required(VERSION 3.20)
project(MyApp C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OPENCV_INCLUDE "C:/opencv/build/include")
set(OPENCV_LIB_DIR "C:/opencv/build/x64/vc16/lib")

add_executable(MyApp
    main.cpp
    vc.c
)

target_include_directories(MyApp PRIVATE ${OPENCV_INCLUDE})
target_link_directories(MyApp PRIVATE ${OPENCV_LIB_DIR})
target_link_libraries(MyApp PRIVATE
    $<$<CONFIG:Debug>:opencv_world4130d>
    $<$<CONFIG:Release>:opencv_world4130>
)

# Copy OpenCV world DLL next to the executable (debug vs release variant)
add_custom_command(TARGET MyApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "C:/opencv/build/x64/vc16/bin/opencv_world4130$<$<CONFIG:Debug>:d>.dll"
        $<TARGET_FILE_DIR:MyApp>
)

# Collect source images
file(GLOB IMAGE_FILES "${CMAKE_SOURCE_DIR}/assets/*")

add_custom_command(TARGET MyApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:MyApp>/assets
)

foreach(img ${IMAGE_FILES})
    add_custom_command(TARGET MyApp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${img}
            $<TARGET_FILE_DIR:MyApp>/assets
    )
endforeach()

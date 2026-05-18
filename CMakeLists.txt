cmake_minimum_required(VERSION 3.20)
project(MyApp C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configurações do OpenCV
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

# ---------------------------------------------------------------------
# CONFIGURAÇÕES DE ALTA PERFORMANCE (MSVC)
# ---------------------------------------------------------------------
if(MSVC)
    target_compile_options(MyApp PRIVATE
        $<$<CONFIG:Release>:
            /O2         # Velocidade máxima
            /Oi         # Gera funções intrínsecas
            /Ot         # Favorece código rápido em vez de pequeno
            /Oy         # Omite ponteiro de frame (liberta um registo)
            /fp:fast    # Matemática rápida (equivalente a -ffast-math)
            /arch:AVX2  # Otimização p/ CPUs modernos (equivalente a -march)
            /Qpar       # Auto-paralelização de ciclos (ajuda em loop unrolling)
        >
    )
endif()
# ---------------------------------------------------------------------

# Copiar DLL do OpenCV após o Build
add_custom_command(TARGET MyApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "C:/opencv/build/x64/vc16/bin/opencv_world4130$<$<CONFIG:Debug>:d>.dll"
        $<TARGET_FILE_DIR:MyApp>
)

# Colecionar e copiar imagens de assets
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
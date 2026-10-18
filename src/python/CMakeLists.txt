file(GLOB MICROPYTHON_CORE CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/third_party/micropython/py/*.c")
add_executable(thinkcad_micropython ${MICROPYTHON_CORE}
    "${PROJECT_SOURCE_DIR}/third_party/micropython/port/embed_util.c"
    "${PROJECT_SOURCE_DIR}/third_party/micropython/shared/runtime/gchelper_generic.c"
    micropython_main.c hostsleep.cpp)
set_target_properties(thinkcad_micropython PROPERTIES OUTPUT_NAME thinkcad-micropython
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}" C_STANDARD 11 C_STANDARD_REQUIRED ON AUTOMOC OFF)
target_include_directories(thinkcad_micropython PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/third_party/micropython")
if(MSVC)
    # Pinned upstream Windows-port shim, preserving unmodified MicroPython core.
    target_include_directories(thinkcad_micropython PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/msvc")
    target_compile_definitions(thinkcad_micropython PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
else()
    target_link_libraries(thinkcad_micropython PRIVATE m)
endif()
if(WIN32)
    install(TARGETS thinkcad_micropython RUNTIME DESTINATION .)
    install(FILES "${PROJECT_SOURCE_DIR}/third_party/micropython/LICENSE" DESTINATION licenses RENAME MicroPython_LICENSE.txt)
else()
    install(TARGETS thinkcad_micropython RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES "${PROJECT_SOURCE_DIR}/third_party/micropython/LICENSE" DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/thinkcad RENAME MicroPython_LICENSE.txt)
endif()

CPMAddPackage(
        NAME prometheus-cpp
        GITHUB_REPOSITORY jupp0r/prometheus-cpp
        GIT_TAG 052fd18c8e27e4f9fb3e183382ccfba4257f0305
        OPTIONS
        "ENABLE_COMPRESSION OFF"
        "ENABLE_PUSH OFF"
        "BUILD_SHARED_LIBS OFF"
) # defines: prometheus-cpp

if (prometheus-cpp_ADDED)
    set(EXTERNAL_LIBS ${EXTERNAL_LIBS} prometheus-cpp::core prometheus-cpp::pull)
    target_include_directories(xi_map SYSTEM INTERFACE
            ${prometheus-cpp_SOURCE_DIR}/core/include
            ${prometheus-cpp_BINARY_DIR}/core/include
            ${prometheus-cpp_SOURCE_DIR}/pull/include
            ${prometheus-cpp_BINARY_DIR}/pull/include
    )
    target_link_libraries(xi_map PRIVATE prometheus-cpp::core prometheus-cpp::pull)
endif ()

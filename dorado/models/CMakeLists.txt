add_library(dorado_models_lib STATIC
    kits.cpp
    kits.h
    metadata.cpp
    metadata.h
    model_downloader.h
    model_downloader.cpp
    models.cpp
    models.h
)

if (APPLE)
    target_sources(dorado_models_lib PRIVATE model_downloader_foundation.mm)
endif()
if (NOT IOS)
    target_link_libraries(dorado_models_lib PRIVATE OpenSSL::SSL)
endif()
target_compile_definitions(dorado_models_lib
    PRIVATE
        DORADO_MODELS_HAS_FOUNDATION=$<BOOL:${APPLE}>
        DORADO_MODELS_HAS_HTTPLIB=$<NOT:$<BOOL:${IOS}>>
        DORADO_MODELS_HAS_CURL_EXE=$<NOT:$<BOOL:${IOS}>>
)

target_link_libraries(dorado_models_lib
    PRIVATE
        dorado_utils
        elzip
        spdlog::spdlog
)

target_include_directories(dorado_models_lib
    SYSTEM
    PRIVATE
        ${DORADO_3RD_PARTY_SOURCE}/cpp-httplib
)

enable_warnings_as_errors(dorado_models_lib)

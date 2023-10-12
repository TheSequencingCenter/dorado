add_library(dorado_models_lib STATIC
    models.cpp
    models.h
)

target_link_libraries(dorado_models_lib
    PRIVATE
        OpenSSL::SSL
        elzip
)

target_include_directories(dorado_models_lib
    SYSTEM
    PRIVATE
        ${DORADO_3RD_PARTY_SOURCE}/cpp-httplib
        ${DORADO_3RD_PARTY_SOURCE}/spdlog/include
)
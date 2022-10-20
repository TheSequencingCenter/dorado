OPTION(BUILD_KOI_FROM_SOURCE OFF)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR WIN32)

    if(BUILD_KOI_FROM_SOURCE)
        message(STATUS "Building Koi from source")

        set(KOI_DIR "${DORADO_3RD_PARTY}/koi")

        if(NOT EXISTS ${KOI_DIR})
            if(DEFINED GITLAB_CI_TOKEN)
                message("Cloning Koi using CI token")
                execute_process(COMMAND git clone https://gitlab-ci-token:${GITLAB_CI_TOKEN}@git.oxfordnanolabs.local/machine-learning/koi.git ${KOI_DIR})
            else()
                message("Cloning Koi using ssh")
                execute_process(COMMAND git clone git@git.oxfordnanolabs.local:machine-learning/koi.git ${KOI_DIR})
            endif()
            execute_process(COMMAND git checkout 6f3a5b827434e275f38791a8f2e5244a2f2ff2ff WORKING_DIRECTORY ${KOI_DIR})
        endif()
        add_subdirectory(${KOI_DIR}/koi/lib)

        set(KOI_INCLUDE ${KOI_DIR}/koi/lib)
        set(KOI_LIBRARIES koi)
    else()
        message(STATUS "Using prebuilt Koi from Box")
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            download_and_extract(https://nanoporetech.box.com/shared/static/v53gamf7odn8wrovjoumy9hxt18yyurt.gz koi_lib)
            file(GLOB KOI_DIR "${DORADO_3RD_PARTY}/koi_lib/*")
            set(KOI_LIBRARIES ${KOI_DIR}/lib/libkoi.a)
        elseif(WIN32)
            download_and_extract(https://nanoporetech.box.com/shared/static/mlgvz6b00zaanirue6lx8nrti38ltj32.zip koi_lib)
            file(GLOB KOI_DIR "${DORADO_3RD_PARTY}/koi_lib/*")
            set(KOI_LIBRARIES ${KOI_DIR}/lib/koi.lib)
        endif()
        message(STATUS "KOI_DIR is ${KOI_DIR}")
        set(KOI_INCLUDE ${KOI_DIR}/include)
        message(STATUS "KOI_INCLUDE is ${KOI_INCLUDE}")
        execute_process(COMMAND ls ${KOI_INCLUDE})     
    endif()
endif()

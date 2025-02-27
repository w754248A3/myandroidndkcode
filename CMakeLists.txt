cmake_minimum_required(VERSION 3.16)

project(myandroidndkcode CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(MY_DIR_PATH ${PROJECT_SOURCE_DIR})

function(my_set_common_properties target)

    # set_target_properties(${target} PROPERTIES 
    #                         CMAKE_CXX_STANDARD 20
    #                         CMAKE_CXX_STANDARD_REQUIRED ON)
  
    #target_compile_definitions(${target} PUBLIC)
   
    target_compile_options(${target} PUBLIC
        -pedantic-errors
        -Woverflow
        -Wconversion
        -Werror
        -Wextra
        -Wall
        -Wpedantic 
        -Wuninitialized
        -Wsometimes-uninitialized
        -Werror=uninitialized
        -Wmissing-field-initializers
        -Wgnu-empty-initializer
        #-Wmaybe-uninitialized
        #-fconcepts
    )

    #target_link_options(${target} PUBLIC -static)
   
    target_include_directories(${target} PUBLIC
        ${MY_DIR_PATH}/include
        #${MY_DIR_PATH}/../CppHttpFileServer/include
    )


    


    # target_link_libraries(${target} PUBLIC
    #     torrent-rasterbar
    #     boost_atomic-mt

    # )

   

endfunction()


function(my_set_target_out targetname)
    # 设置输出目录
    set_target_properties(${targetname} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${MY_DIR_PATH}/out
    OUTPUT_NAME "${targetname}"
    )
endfunction()



add_executable(test ./test.cpp)


my_set_common_properties(test)

my_set_target_out(test)


add_executable(proxytarget ./proxy.cpp)


my_set_common_properties(proxytarget)

my_set_target_out(proxytarget)


add_executable(rwloop ./rwloop.cpp)


my_set_common_properties(rwloop)

my_set_target_out(rwloop)

# cmake/warnings.cmake

function(set_project_warnings target warnings_as_errors)
    # Определяем тип цели
    get_target_property(_type ${target} TYPE)
    if(NOT _type)
        message(FATAL_ERROR "Target '${target}' not found in set_project_warnings()")
    endif()

    # Выбираем ключевое слово для target_compile_options
    set(_scope PRIVATE)
    if(_type STREQUAL "INTERFACE_LIBRARY")
        set(_scope INTERFACE)
    elseif(_type STREQUAL "OBJECT_LIBRARY")
        set(_scope PRIVATE)
    elseif(_type STREQUAL "STATIC_LIBRARY"
        OR _type STREQUAL "SHARED_LIBRARY"
        OR _type STREQUAL "EXECUTABLE"
        OR _type STREQUAL "MODULE_LIBRARY")
        set(_scope PRIVATE)
    endif()

    if(MSVC)
        target_compile_options(${target} ${_scope} /W4 /permissive- /Zc:__cplusplus)
        if(warnings_as_errors)
            target_compile_options(${target} ${_scope} /WX)
        endif()
    else()
        target_compile_options(${target} ${_scope} -Wall -Wextra -Wpedantic)
        if(warnings_as_errors)
            target_compile_options(${target} ${_scope} -Werror)
        endif()
    endif()
endfunction()

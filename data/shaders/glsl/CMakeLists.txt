cmake_minimum_required(VERSION 3.4.1)
set(SHADERS
    base
    hybridRayTracing
    triangle)
# Build all shaders
function(buildShaders)
    foreach(SHADER ${SHADERS})
        message("build ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER} ...")
        execute_process(COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/compileshaders.py ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER})
    endforeach(SHADER)
endfunction(buildShaders)
buildShaders()
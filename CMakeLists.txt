cmake_minimum_required (VERSION 3.8)

project ("NapCatNative")

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# 平台宏定义
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    set(_WIN_PLATFORM_ true)
    add_definitions(-D _WIN_PLATFORM_)
endif()

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    set(_LINUX_PLATFORM_ true)
    add_definitions(-D _LINUX_PLATFORM_)
endif()

# 引入NAPI支持
include_directories(${CMAKE_JS_INC})

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/MoeHoo/include/")

include_directories("${CMAKE_SOURCE_DIR}/node_modules/node-api-headers/include")


file(GLOB SOURCE_FILES "./src/*.cpp")
file(GLOB HEADER_FILES "./include/*.h")

add_executable (NapCatNative SOURCE_FILES HEADER_FILES)

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})

if(MSVC AND CMAKE_JS_NODELIB_DEF AND CMAKE_JS_NODELIB_TARGET)
  execute_process(COMMAND ${CMAKE_AR} /def:${CMAKE_JS_NODELIB_DEF} /out:${CMAKE_JS_NODELIB_TARGET} ${CMAKE_STATIC_LINKER_FLAGS})
endif()
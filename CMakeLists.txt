# pt-thread/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

# 定义 PT-Thread 库
add_library(ptthread STATIC)

# 收集 PT-Thread 源文件
file(GLOB_RECURSE RTT_SOURCES "src/*.c" "components/drivers/ipc/*.c" "components/drivers/serial/*.c")

# 添加源文件到库
target_sources(ptthread PRIVATE ${RTT_SOURCES})

# 设置包含目录
target_include_directories(ptthread PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/components/drivers/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/components/drivers/include/ipc/
    ${CMAKE_CURRENT_SOURCE_DIR}/components/drivers/include/dirvers/
)

target_link_libraries(ptthread PUBLIC libserialport)

# 如果需要，可以在这里定义编译选项
target_compile_definitions(ptthread PUBLIC)

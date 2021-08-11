cmake_minimum_required(VERSION 2.8)

project(ShmBufferTool)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(Protobuf_INCLUDE_DIR "/usr/local/include/")
set(Protobuf_LIBRARY "/usr/local/lib/")
set(Protobuf_SRC_ROOT_FOLDER "/usr/local/lib")
message(STATUS "Protobuf_INCLUDE_DIR = ${Protobuf_INCLUDE_DIR}")
message(STATUS "Protobuf_LIBRARY = ${Protobuf_LIBRARY}")
message(STATUS "Protobuf_SRC_ROOT_FOLDER = ${Protobuf_SRC_ROOT_FOLDER}")
find_package(Protobuf 3.12.3 REQUIRED)

message(STATUS "CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")
file(GLOB Protofiles "${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto")
PROTOBUF_GENERATE_CPP(ProtoSources ProtoHeaders ${Protofiles})


add_executable(${PROJECT_NAME}_write shm_write.cpp
                                     proto/data.proto
                                     ${ProtoSources}
                                     )

target_link_libraries(${PROJECT_NAME}_write
                                     /usr/local/lib/libprotobuf.a
                                     atomic
                                     pthread
                                     )


add_executable(${PROJECT_NAME}_read shm_read.cpp
                                  proto/data.proto
                                  ${ProtoSources}
                                  )

target_link_libraries(${PROJECT_NAME}_read
                                  /usr/local/lib/libprotobuf.a
                                  atomic
                                  pthread
                                  )

add_executable(${PROJECT_NAME}_callback_read shm_callback_read.cpp
                                proto/data.proto
                                ${ProtoSources}
                                )

target_link_libraries(${PROJECT_NAME}_callback_read
                                /usr/local/lib/libprotobuf.a
                                atomic
                                pthread
                                )

add_executable(${PROJECT_NAME}_class_callback_read shm_class_callback_read.cpp
                                proto/data.proto
                                ${ProtoSources}
                                )

target_link_libraries(${PROJECT_NAME}_class_callback_read
                                /usr/local/lib/libprotobuf.a
                                atomic
                                pthread
                                )

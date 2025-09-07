# Job Queue Library
cmake_minimum_required(VERSION 3.10)

# Create job_queue static library
add_library(job_queue STATIC
    JobQueue.cpp
    ../storage/EmailService.cpp
)

# Include directories
target_include_directories(job_queue 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(job_queue
    PUBLIC
        mongodb_instance
        common
        ${REDIS_LIBRARIES}
    PRIVATE
        mongo::bsoncxx_shared 
        mongo::mongocxx_shared
        ${CURL_LIBRARIES}
        Threads::Threads
)

# Add Redis include directories if available
if(REDIS_AVAILABLE)
    target_include_directories(job_queue PRIVATE ${REDIS_INCLUDE_DIRS})
    target_compile_definitions(job_queue PRIVATE REDIS_AVAILABLE)
endif()

# Compiler definitions
target_compile_definitions(job_queue PRIVATE
    BSONCXX_STATIC
    MONGOCXX_STATIC
)

# Install target
install(TARGETS job_queue
    EXPORT JobQueueTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Export configuration
install(EXPORT JobQueueTargets
    FILE JobQueueTargets.cmake
    NAMESPACE SearchEngine::
    DESTINATION lib/cmake/SearchEngine
)

# Scoring library
add_library(scoring STATIC
    SearchScorer.cpp
)

target_include_directories(scoring PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(scoring
    PUBLIC
        common
)

# Link Redis libraries if available
if(REDIS_AVAILABLE)
    target_link_libraries(scoring PUBLIC ${REDIS_LIBRARIES})
    target_compile_definitions(scoring PUBLIC REDIS_AVAILABLE)
endif() 
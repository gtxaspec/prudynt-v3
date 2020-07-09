set(WYZEDOM_SOURCE
  main.cpp
  Encoder.cpp
  Logger.cpp
  FrameWaterfall.cpp
)

add_executable(${PROJECT_NAME} ${WYZEDOM_SOURCE})
target_include_directories(${PROJECT_NAME} PUBLIC ../blob/include)
target_link_directories(${PROJECT_NAME} PUBLIC ../blob/lib)
target_link_libraries(${PROJECT_NAME} imp alog rt)

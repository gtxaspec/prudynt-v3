set(WYZEDOM_SOURCE
  main.cpp
  Encoder.cpp
  Logger.cpp
  FrameWaterfall.cpp
)

add_executable(${PROJECT_NAME} ${WYZEDOM_SOURCE})
target_include_directories(${PROJECT_NAME} PUBLIC
  ../3rdparty/install/include
  ../3rdparty/install/include/liveMedia
  ../3rdparty/install/include/groupsock
  ../3rdparty/install/include/UsageEnvironment
  ../3rdparty/install/include/BasicUsageEnvironment
  ../blob/include
)
target_link_directories(${PROJECT_NAME} PUBLIC
  ../3rdparty/install/lib
  ../blob/lib
)
target_link_libraries(${PROJECT_NAME}
  pthread
  imp alog rt crypto ssl
  liveMedia
  groupsock
  BasicUsageEnvironment
  UsageEnvironment
)

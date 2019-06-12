cmake_minimum_required(VERSION 2.8)
project( DisplayImage )

MESSAGE ("\n==> Finding 3rd Party Packages")
find_package( OpenCV REQUIRED )

SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -W" )
SET( CMAKE_C_FLAGS_RELEASE "-O3" )
SET( CMAKE_C_FLAGS_DEBUG "-g3" )

SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -std=c++11" )
SET( CMAKE_CXX_FLAGS_RELEASE "-O3" )
SET( CMAKE_CXX_FLAGS_DEBUG "-g3" )

SET( BASE_DIR ${CMAKE_SOURCE_DIR} )
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BASE_DIR}/bin )

add_executable( lossy_lhe lossy_lhe.cpp )
target_link_libraries( lossy_lhe ${OpenCV_LIBS} )

add_executable( jpneg_lossy jpneg_lossy.cpp )
target_link_libraries( jpneg_lossy ${OpenCV_LIBS} )

add_executable( PlayerCamera PlayerCamera.cpp )
target_link_libraries( PlayerCamera ${OpenCV_LIBS} )

add_executable( PlayerYUV PlayerYUV.cpp )
target_link_libraries( PlayerYUV ${OpenCV_LIBS} )

add_executable( jpneg_ls jpneg_ls.cpp )
target_link_libraries( jpneg_ls ${OpenCV_LIBS} )

add_executable( lhe lhe.cpp )
target_link_libraries( lhe ${OpenCV_LIBS} )


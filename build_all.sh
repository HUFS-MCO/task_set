#!/bin/bash

echo "Compiling all processes with -O0 -std=c++17..."

CXXFLAGS="-O0 -std=c++17 -static-libstdc++ -static-libgcc"

g++ $CXXFLAGS -o planner planner_main.cpp
g++ $CXXFLAGS -o ekf ekf_main.cpp
g++ $CXXFLAGS -o localization localization_main.cpp
g++ $CXXFLAGS -o lidar lidar_main.cpp

echo "Build complete."

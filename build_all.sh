#!/bin/bash

echo "Compiling all processes with -O0 -std=c++17..."

g++ -O0 -std=c++17 -o planner planner_main.cpp
g++ -O0 -std=c++17 -o ekf ekf_main.cpp
g++ -O0 -std=c++17 -o localization localization_main.cpp
g++ -O0 -std=c++17 -o lidar lidar_main.cpp

echo "Build complete."

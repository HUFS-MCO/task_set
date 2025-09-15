#!/bin/bash

./planner &
./ekf &
./localization &
./lidar &

wait
echo "All processes finished."


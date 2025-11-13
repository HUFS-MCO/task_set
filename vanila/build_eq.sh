#!/bin/bash
# Build script for eq_task with SCHED_DEADLINE support

echo "🔨 Compiling eq_task with SCHED_DEADLINE support..."

g++ -O0 -std=c++17 -pthread \
    -o eq_task \
    eq_task.cpp \
    -static-libstdc++ -static-libgcc

if [ $? -eq 0 ]; then
    echo "✅ Build successful!"
    echo ""
    echo "  ./eq_task"
else
    echo "❌ Build failed!"
    exit 1
fi

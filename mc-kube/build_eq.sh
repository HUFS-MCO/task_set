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
    echo "To run with SCHED_DEADLINE (requires CAP_SYS_NICE):"
    echo "  sudo setcap cap_sys_nice=eip ./eq_task"
    echo "  ./eq_task"
    echo ""
    echo "Or run with sudo:"
    echo "  sudo ./eq_task"
else
    echo "❌ Build failed!"
    exit 1
fi

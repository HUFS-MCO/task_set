FROM gcc:latest

# Install necessary tools
RUN apt-get update && apt-get install -y \
    curl \
    libcap2-bin \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy all source files from rt-kube directory
COPY dummy_task.hpp /app/
COPY time_util.hpp /app/
COPY uds_util.hpp /app/
COPY eq_task.cpp /app/
COPY build_eq.sh /app/

# Make build script executable and build
RUN chmod +x /app/build_eq.sh && /app/build_eq.sh

# Grant CAP_SYS_NICE capability to the binary (for SCHED_DEADLINE)
RUN setcap cap_sys_nice=eip /app/eq_task

# Verify the build
RUN ls -lh /app/eq_task

# Run the task
CMD ["/app/eq_task"]

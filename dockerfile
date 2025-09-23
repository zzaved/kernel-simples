FROM ubuntu:20.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    nasm \
    gcc-multilib \
    qemu-system-x86 \
    make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /kernel

# Copy source files
COPY . .

# Build kernel
RUN make clean && make

# Default command to run QEMU
CMD ["make", "run"]
# Docker para compilar o kernel em i386 (usando amd64 via emulação se necessário)
FROM --platform=linux/amd64 debian:stable-slim

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential \
    gcc-multilib \
    nasm \
    make \
    binutils \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

# Reproducible Linux toolchain for the Week 3 Formative Assignment.
#
# The assignment targets Linux (ELF binaries, strace, the CPython C-API). This
# image pins every tool used so the command outputs in this repository can be
# regenerated on any machine -- including Apple-Silicon macOS via Docker.
#
#   docker build -t week3-linux .
#   docker run --rm -v "$PWD":/work -w /work week3-linux ./run_all.sh
#
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        binutils \
        strace \
        file \
        python3 \
        python3-dev \
        python3-setuptools \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /work
CMD ["/bin/bash"]

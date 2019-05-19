# docker build -t 'rpmc' -m 16g .
FROM ubuntu:16.04

RUN dpkg --add-architecture i386

RUN apt-get update

RUN apt-get -f dist-upgrade

RUN apt-get upgrade -qy


RUN apt-get -qy install \
    xz-utils            \
    tar                 \
    wget                \
    cmake               \
    g++                 \
    python3 python3-dev python3-pip \
    git                 \
    gcc-multilib g++-multilib   \
    libc++-dev          \
    libc6:i386 libstdc++6:i386 libc++-dev:i386



WORKDIR /root
RUN git clone https://github.com/llvm/llvm-project.git

WORKDIR /root/llvm-project
RUN mkdir build

WORKDIR /root/llvm-project/build

RUN echo 'add_clang_subdirectory(rpmc)' > /root/llvm-project/clang/tools/CMakeLists.txt
RUN mkdir /root/llvm-project/clang/tools/
COPY rpmc/rpmc /root/llvm-project/clang/tools/

RUN cmake -G "Unix Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCLANG_DEFAULT_CXX_STDLIB=libc++ -DCMAKE_BUILD_TYPE="Release" -DLLVM_TARGETS_TO_BUILD=X86 -DLLVM_ENABLE_PROJECTS="clang" ../llvm

RUN make -j8

RUN make install

FROM ubuntu:19.04 AS run-dependencies

RUN apt-get update && apt-get install -y \
    libclang-dev \
    clang \
  && rm -rf /var/lib/apt/lists/*

FROM run-dependencies AS build-dependencies

RUN apt-get update && apt-get install -y \
    llvm-dev \
    cmake \
    python3 \
    libboost-program-options-dev \
    libboost-filesystem-dev \
  && rm -rf /var/lib/apt/lists/*

FROM build-dependencies AS built

COPY . /src

RUN mkdir /build

WORKDIR /build

RUN cmake ../src && cmake --build . --target standardese_tool

FROM run-dependencies AS standardese

RUN useradd -ms /bin/bash standardese

COPY --from=built /build/tool/standardese /usr/local/bin/standardese

USER standardese
WORKDIR /home/standardese

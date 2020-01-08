FROM ubuntu:19.04 AS run-dependencies

# We make an effort here to shrink the resulting image by dropping all the
# static libraries.  It would probably be better not to install any of these
# libraries and build a static standardese binary.
RUN apt-get update && apt-get install -y \
    libclang-dev \
    clang \
  && rm -rf /var/lib/apt/lists/* \
  && find /usr/lib -name '*.a' -exec rm \{\} \;

FROM ubuntu:19.04 AS build-dependencies

RUN apt-get update && apt-get install -y \
    libclang-dev \
    clang \
    llvm-dev \
    cmake \
    python3 \
    libboost-program-options-dev \
    libboost-filesystem-dev \
  && rm -rf /var/lib/apt/lists/*

FROM build-dependencies AS built

ARG NUM_THREADS=1

COPY . /src

RUN mkdir /build

WORKDIR /build

RUN cmake ../src
RUN cmake --build . --target standardese_tool -j $NUM_THREADS

FROM run-dependencies AS standardese

RUN useradd -ms /bin/bash standardese

COPY --from=built /build/tool/standardese /usr/local/bin/standardese

USER standardese
WORKDIR /home/standardese

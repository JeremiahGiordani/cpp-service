FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libwebsocketpp-dev \
    libjsoncpp-dev \
    libyaml-cpp-dev \
    libboost-all-dev \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY include/ ./include/
COPY src/ ./src/
COPY CMakeLists.txt ./
COPY config/ ./config/

# Build the service
RUN mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc)

# Install the service
RUN cd build && make install

# Create runtime directory structure
RUN mkdir -p /etc/sar_atr /var/log/sar_atr

# Copy config to runtime location
RUN cp config/service_config.yaml /etc/sar_atr/

# Set the entrypoint
ENTRYPOINT ["/usr/local/bin/sar_atr_service"]
CMD ["/etc/sar_atr/service_config.yaml"]

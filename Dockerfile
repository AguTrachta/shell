FROM ubuntu:20.04

# Set environment variable for non-interactive installation
ENV DEBIAN_FRONTEND=noninteractive

# Install base packages and dependencies
RUN apt-get update && apt-get install -y \
    cmake \
    gcc \
    g++ \
    libmicrohttpd-dev \
    libreadline-dev \
    sudo \
    git \
    python3-pip

# Install Conan
RUN pip3 install conan

# Clone and fix Prometheus repository
RUN git clone https://github.com/digitalocean/prometheus-client-c.git
RUN sed -i 's/&promhttp_handler/(MHD_AccessHandlerCallback)promhttp_handler/' prometheus-client-c/promhttp/src/promhttp.c

# Build Prometheus libraries
RUN mkdir -p prometheus-client-c/buildprom && cd prometheus-client-c/buildprom && cmake ../prom && make && sudo make install
RUN mkdir -p prometheus-client-c/buildpromhttp && cd prometheus-client-c/buildpromhttp && cmake ../promhttp && make && sudo make install

# Create build directory and run Conan to install dependencies
WORKDIR /usr/src/app
COPY . /usr/src/app

RUN mkdir build && cd build && \
    conan profile detect --force && \
    conan install .. --build=missing

# Compile the project
WORKDIR /usr/src/app/build
RUN cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j2 && \
    cd ..

# Set the working directory back to /usr/src/app for convenience
WORKDIR /usr/src/app

CMD ["/bin/bash"]




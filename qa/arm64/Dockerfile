FROM arm64v8/debian:trixie

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    git \
    ca-certificates \
    libwxgtk3.2-dev \
    libwxgtk-webview3.2-dev \
    libboost-all-dev \
    libcairo2-dev \
    libcurl4-openssl-dev \
    libgit2-dev \
    libsecret-1-dev \
    libsqlite3-dev \
    unixodbc-dev \
    swig \
    python3-dev \
    python3-wxgtk4.0 \
    libprotobuf-dev \
    protobuf-compiler \
    libngspice0-dev \
    ngspice \
    libocct-data-exchange-dev \
    libocct-draw-dev \
    libocct-foundation-dev \
    libocct-modeling-algorithms-dev \
    libocct-modeling-data-dev \
    libocct-ocaf-dev \
    libocct-visualization-dev \
    nlohmann-json3-dev \
    libharfbuzz-dev \
    libfontconfig1-dev \
    libfreetype-dev \
    libglib2.0-dev \
    libnng-dev \
    libglm-dev \
    libspnav-dev \
    libegl-dev \
    libgles-dev \
    libpixman-1-dev \
    libgtk-3-dev \
    libpoppler-glib-dev \
    libwayland-dev \
    wayland-protocols \
    ccache \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

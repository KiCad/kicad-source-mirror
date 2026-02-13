FROM fedora:rawhide

RUN dnf install -y --setopt=install_weak_deps=False \
    cmake \
    ninja-build \
    gcc-c++ \
    swig \
    boost-devel \
    glm-devel \
    gtk3-devel \
    libcurl-devel \
    libgit2-devel \
    libngspice-devel \
    ngspice \
    libsecret-devel \
    libzstd-devel \
    nng-devel \
    opencascade-devel \
    protobuf-compiler \
    protobuf-devel \
    unixODBC-devel \
    wxGTK-devel \
    zlib-devel \
    python3-devel \
    python3-wxpython4 \
    cairo-devel \
    pixman-devel \
    harfbuzz-devel \
    fontconfig-devel \
    freetype-devel \
    glib2-devel \
    poppler-glib-devel \
    wayland-devel \
    wayland-protocols-devel \
    libspnav-devel \
    json-devel \
    ccache \
    && dnf clean all

WORKDIR /src

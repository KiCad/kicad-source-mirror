FROM registry.gitlab.com/kicad/kicad-ci/source_containers/master/fedora:37 as build-doxygen-env
USER root

WORKDIR /src
COPY . ./

RUN ls
RUN mkdir build && cd build

WORKDIR /src/build

RUN cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DKICAD_USE_OCC=ON \
      -DKICAD_SCRIPTING_WXPYTHON=ON
RUN make doxygen-docs
RUN make doxygen-python


FROM scratch as output-image

COPY --from=build-doxygen-env /src/doxygen/out/html /doxygen-docs_html
COPY --from=build-doxygen-env /src/build/pcbnew/doxygen-python/html /doxygen-python_html

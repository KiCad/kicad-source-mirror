This directory contains the source code and includes for the GLEW library
(https://github.com/nigels-com/glew). This library is normally dynamically
linked to KiCad on all platforms, but wxWidgets 3.1.5+ on Linux requires
GLEW compiled with EGL support and most distributions do not supply this
(since compiling GLEW for EGL is mutually exclusive with GLEW for X11).

The source files are generated from the GLEW repo, or can be pulled from
a recent GLEW release.

As of November 10, 2020 this GLEW version was pulled from its GitHub
repository and is what should be version upstream version 2.2.0.

This library is licensed under BSD and MIT licenses, with the actual
license text given in the LICENSE file in this directory.

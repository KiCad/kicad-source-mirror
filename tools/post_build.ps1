#!/usr/bin/env pwsh

# This script is used to perform post-build actions for development.
# It installs the build, copies necessary .pdb and .dll files to the output directory,
# and copies Python tools from the vcpkg installation.

$PROJECT_ROOT = git rev-parse --show-toplevel

$BUILD_ROOT = "$PROJECT_ROOT/build"

$BIN_DIR = "$BUILD_ROOT/out/bin/"

$VCPKG_INSTALL_ROOT = "$BUILD_ROOT/vcpkg_installed"

# Attempt to load System.Drawing.Common to avoid runtime errors
try {
    Add-Type -AssemblyName System.Drawing.Common -ErrorAction Stop | Out-Null
} catch {
}

cmake --install $BUILD_ROOT

Get-ChildItem -Path $PROJECT_ROOT -Recurse -Filter *.pdb | Where-Object { $_.FullName -notmatch 'build/out/bin' } | ForEach-Object {
    $destination = Join-Path -Path $BIN_DIR -ChildPath $_.Name
    if ($_.FullName -ne $destination) {
        Copy-Item -Path $_.FullName -Destination $BIN_DIR -Force
    }
}

Get-ChildItem -Path $VCPKG_INSTALL_ROOT -Recurse -Filter *.dll | ForEach-Object {
    $destination = Join-Path -Path $BIN_DIR -ChildPath $_.Name
    if ($_.FullName -ne $destination) {
        Copy-Item -Path $_.FullName -Destination $BIN_DIR -Force
    }
}

Copy-Item -Path "$VCPKG_INSTALL_ROOT/x64-windows/tools/python3/*" -Destination $BIN_DIR -Recurse -Force
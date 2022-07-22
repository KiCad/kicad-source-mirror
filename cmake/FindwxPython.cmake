# CMake script for finding wxPython/Phoenix library

# Copyright (C) 2018 CERN
# Author: Maciej Suminski <maciej.suminski@cern.ch>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, you may find one here:
# http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
# or you may search the http://www.gnu.org website for the version 2 license,
# or you may write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

# Exported variables:
#   WXPYTHON_VERSION: wxPython/Phoenix version,
#                     normally 3.x.x for wxPython, 4.x.x for Phoenix
#   WXPYTHON_FLAVOR: 'Phoenix' or 'wxPython'
#   WXPYTHON_TOOLKIT: base library toolkit (e.g. gtk2, gtk3, msw, osx_cocoa)
#   WXPYTHON_WXVERSION: wxWidgets version used by wxPython/Phoenix

# Create a CMake list containing wxPython version
set( _py_cmd "import wx;print(wx.version())" )

# Add user specified Python site package path
if( PYTHON_SITE_PACKAGE_PATH )
    set( _py_site_path
        "import sys;sys.path.insert(0, \"${PYTHON_SITE_PACKAGE_PATH}\");" )
endif()

if( VCPKG_TOOLCHAIN )
# python 3.8+ requires us to use python to add additional load directories (PATH no longer supported)
# vcpkg does not copy all the dlls into the python folder so we need this for development
# as the wxpython modules need the wxwidgets library DLLs to load
    set( _py_dll "import os;os.add_dll_directory(\"${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin\");os.add_dll_directory(\"${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin\");" )
else()
    set( _py_dll "" )
endif()

execute_process( COMMAND ${PYTHON_EXECUTABLE} -c "${_py_dll}${_py_site_path}${_py_cmd}"
    RESULT_VARIABLE _WXPYTHON_VERSION_RESULT
    OUTPUT_VARIABLE _WXPYTHON_VERSION_FOUND
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

# Check to see if any version of wxPython is installed on the system.
if( _WXPYTHON_VERSION_RESULT GREATER 0 )
    message( FATAL_ERROR "wxPython/Phoenix does not appear to be installed on the system" )
endif()


# Turn the version string to a list for easier processing
set( _WXPYTHON_VERSION_LIST ${_WXPYTHON_VERSION_FOUND})
separate_arguments( _WXPYTHON_VERSION_LIST )
list( LENGTH _WXPYTHON_VERSION_LIST _VERSION_LIST_LEN )

if( _VERSION_LIST_LEN LESS 3 )
    message( FATAL_ERROR "Unknown wxPython/Phoenix version string: ${_WXPYTHON_VERSION_FOUND}" )
endif()

# wxPython style, e.g. '3.0.2.0;gtk3;(classic) or Pheonix style: e.g. 4.0.1;gtk3;(phoenix)
list( GET _WXPYTHON_VERSION_LIST 0 WXPYTHON_VERSION )
list( GET _WXPYTHON_VERSION_LIST 1 WXPYTHON_TOOLKIT )
list( GET _WXPYTHON_VERSION_LIST 2 WXPYTHON_FLAVOR )

# Determine wxWidgets version used by wxPython/Phoenix
if( WXPYTHON_FLAVOR MATCHES "phoenix" )
    # 4.0.1;gtk3;(phoenix) does not contain wxWidgets version, request it explicitly

    set( _py_cmd "import wx;print(wx.wxWidgets_version.split(' ')[1])")
    execute_process( COMMAND ${PYTHON_EXECUTABLE} -c "${_py_dll}${_py_site_path}${_py_cmd}"
        RESULT_VARIABLE WXPYTHON_WXVERSION_RESULT
        OUTPUT_VARIABLE WXPYTHON_WXVERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )

    if( NOT WXPYTHON_WXVERSION_RESULT EQUAL 0 )
        set( WXPYTHON_WXVERSION "3.0.2" )
        message( WARNING "Could not determine wxWidgets version used by Phoenix, "
            "requesting ${WXPYTHON_WXVERSION}" )
    endif()

    set( WXPYTHON_FLAVOR "Phoenix" )

elseif( WXPYTHON_FLAVOR MATCHES "classic" )
    # 3.0.2.0;gtk3;(classic) has the wxWidgets version in the first part
    set( WXPYTHON_WXVERSION ${WXPYTHON_VERSION} )
    set( WXPYTHON_FLAVOR "wxPython" )

else()
    message( FATAL_ERROR "Unknown wxPython/Phoenix type: ${WXPYTHON_FLAVOR}")
endif()


# Fix an incosistency between the toolkit names reported by wx.version() and wx-config for cocoa
if( WXPYTHON_TOOLKIT STREQUAL "osx-cocoa" )
    set( WXPYTHON_TOOLKIT "osx_cocoa" )
endif()

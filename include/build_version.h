/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/* Date for kicad build version */

#ifndef KICAD_BUILD_VERSION_H
#define KICAD_BUILD_VERSION_H

#include <kicommon.h>
#include <tuple>

class wxString;

/**
 * Get the full KiCad version string. This string contains platform-specific information
 * added by the packagers. It is created by CMake in the KICAD_FULL_VERSION variable.
 *
 * @return the full version string
 */
KICOMMON_API wxString GetBuildVersion();

/**
 * Get the KiCad version string without the information added by the packagers.
 * It is created by CMake in the KICAD_VERSION variable.
 *
 * @return the base version string
 */
KICOMMON_API wxString GetBaseVersion();

/**
 * @return the bitness name string (like "Little endian")
 */
KICOMMON_API wxString GetPlatformGetBitnessName();

/**
 * Get the semantic version string for KiCad defined inside the KiCadVersion.cmake file in
 * the variable KICAD_SEMANTIC_VERSION.
 *
 * @return the semantic version string
 */
KICOMMON_API wxString GetSemanticVersion();

/**
 * Get only the major and minor version in a string major.minor.
 * This is extracted by CMake from the KICAD_SEMANTIC_VERSION variable.
 *
 * @return the major and minor version as a string
 */
KICOMMON_API wxString GetMajorMinorVersion();

/**
 * Get the major, minor and patch version in a string major.minor.patch
 * This is extracted by CMake from the KICAD_SEMANTIC_VERSION variable.
 *
 * @return the major.minor.patch version as a string
 */
KICOMMON_API wxString GetMajorMinorPatchVersion();

/**
 * Get the build date as a string.
 *
 * @return the build date string
 */
KICOMMON_API wxString GetBuildDate();

/**
 * Get the commit hash as a string.
 *
 * @return the commit hash string
 */
KICOMMON_API wxString GetCommitHash();

/**
 * Get the build version numbers as a tuple
 *
 * @return A tuple with three ints for major/minor/patch revisions
 */
KICOMMON_API const std::tuple<int, int, int>& GetMajorMinorPatchTuple();

/**
 * Check if the build is meant to be nightly
 * @return true if running nightly build
 */
KICOMMON_API bool IsNightlyVersion();

/**
 * A setter for OpenGL info when it's initialized.
 */
KICOMMON_API void SetOpenGLInfo( const char* aRenderer, const char* aVendor, const char* aVersion );

/**
 * Create a version info string for bug reports and the about dialog
 * @param aTitle is the application title to include at the top of the report
 * @param aBrief = true to condense information for the bug report URL
 * @param aHtml = true to use a minimal HTML format, false for plan text
 * @return the version info string
 */
KICOMMON_API wxString GetVersionInfoData( const wxString& aTitle, bool aHtml = false,
                                          bool aBrief = false );

#endif  // KICAD_BUILD_VERSION_H

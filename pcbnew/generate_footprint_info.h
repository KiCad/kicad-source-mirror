/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <optional>

#include <wx/string.h>

class FOOTPRINT_LIBRARY_ADAPTER;
class FOOTPRINT;
class LIB_ID;

/**
 * Return an HTML page describing a #LIB_ID in a footprint library. This is suitable for inclusion
 * in a wxHtmlWindow.
 */
wxString GenerateFootprintInfo( FOOTPRINT_LIBRARY_ADAPTER* aAdapter, LIB_ID const& aLibId );

/**
 * Get a URL to the documentation for a #LIB_ID in a #FP_LIB_TABLE. This is suitable for opening
 * in a web browser. Currently, for want of a proper home in the format, this is usually
 * found in the "description" field of the footprint.
 *
 * @return The URL, or std::nullopt if no URL is available.
 */
std::optional<wxString> GetFootprintDocumentationURL( const FOOTPRINT& aFootprint );


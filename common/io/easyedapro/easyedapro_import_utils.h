/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef EASYEDAPRO_IMPORT_UTILS_H_
#define EASYEDAPRO_IMPORT_UTILS_H_

#include <set>
#include <wx/stream.h>
#include <wx/string.h>
#include <lib_id.h>
#include <nlohmann/json_fwd.hpp>

struct IMPORT_PROJECT_DESC;

#define EASY_IT_CONTINUE return false
#define EASY_IT_BREAK return true

namespace EASYEDAPRO
{

wxString ShortenLibName( wxString aProjectName );

LIB_ID ToKiCadLibID( const wxString& aLibName, const wxString& aLibReference );

std::vector<IMPORT_PROJECT_DESC> ProjectToSelectorDialog( const nlohmann::json& aProject,
                                                          bool                  aPcbOnly = false,
                                                          bool                  aSchOnly = false );

nlohmann::json FindJsonFile( const wxString& aZipFileName, const std::set<wxString>& aFileNames );

nlohmann::json ReadProjectOrDeviceFile( const wxString& aZipFileName );

void IterateZipFiles(
        const wxString&                                                         aFileName,
        std::function<bool( const wxString&, const wxString&, wxInputStream& )> aCallback );

std::vector<nlohmann::json> ParseJsonLines( wxInputStream& aInput, const wxString& aSource );

/**
 * Multiple document types (e.g. footprint and PCB) can be put into a single file, separated by
 * empty line.
 */
std::vector<std::vector<nlohmann::json>> ParseJsonLinesWithSeparation( wxInputStream&  aInput,
                                                                       const wxString& aSource );

std::map<wxString, wxString> AnyMapToStringMap( const std::map<wxString, nlohmann::json>& aInput );

} // namespace EASYEDAPRO


#endif // EASYEDAPRO_IMPORT_UTILS_H_

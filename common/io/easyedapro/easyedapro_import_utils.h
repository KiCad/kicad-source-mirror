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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef EASYEDAPRO_IMPORT_UTILS_H_
#define EASYEDAPRO_IMPORT_UTILS_H_

#include <functional>
#include <map>
#include <set>
#include <vector>
#include <wx/stream.h>
#include <wx/string.h>
#include <io/easyedapro/easyedapro_parser.h>
#include <lib_id.h>
#include <nlohmann/json_fwd.hpp>

struct IMPORT_PROJECT_DESC;

#define EASY_IT_CONTINUE return false
#define EASY_IT_BREAK return true

namespace EASYEDAPRO
{

class V3_DOC_PARSER;

wxString ShortenLibName( wxString aProjectName );

LIB_ID ToKiCadLibID( const wxString& aLibName, const wxString& aLibReference );

std::vector<IMPORT_PROJECT_DESC> ProjectToSelectorDialog( const nlohmann::json& aProject, bool aPcbOnly = false,
                                                          bool                  aSchOnly = false );

nlohmann::json FindJsonFile( const wxString& aZipFileName, const std::set<wxString>& aFileNames );

nlohmann::json ReadProjectOrDeviceFile( const wxString& aZipFileName );

void IterateZipFiles( const wxString&                                                         aFileName,
        std::function<bool( const wxString&, const wxString&, wxInputStream& )> aCallback );

std::vector<nlohmann::json> ParseJsonLines( wxInputStream& aInput, const wxString& aSource );

/**
 * Multiple document types (e.g. footprint and PCB) can be put into a single file, separated by
 * empty line.
 */
std::vector<std::vector<nlohmann::json>> ParseJsonLinesWithSeparation( wxInputStream& aInput, const wxString& aSource );

std::map<wxString, wxString> AnyMapToStringMap( const std::map<wxString, nlohmann::json>& aInput );

/**
 * Build a minimal legacy-style project index from parsed v3 raw documents.
 *
 * The result is compatible with ProjectToSelectorDialog and legacy metadata readers, and
 * contains at least schematics / boards / pcbs. When @a aIncludeLibraryMetadata is true,
 * symbols / footprints / devices maps are also populated from their META rows.
 */
nlohmann::json BuildV3ProjectIndexFromRawDocs( const V3_DOC_PARSER& aParser, bool aIncludeLibraryMetadata = true );

std::map<wxString, BLOB> BuildV3BlobMap( const V3_DOC_PARSER& aParser );

wxString GetV3LibraryItemTitle( const nlohmann::json& aMetadata, const wxString& aUuid );

/**
 * Build KiCad keywords from a v3 tags object (parent_tag / child_tag name fields).
 */
wxString KeywordsFromV3Tags( const nlohmann::json& aTags );

/**
 * Resolve EasyEDA `={Var}` / `={A}text{B}` field expressions against device attributes.
 */
wxString ResolveDeviceFieldVariables( const wxString& aInput, const std::map<wxString, wxString>& aDeviceAttributes );

/** Replace EasyEDA temperature glyph (℃) with °C. */
wxString NormalizeEasyEDAText( wxString aText );

/**
 * Allocate a unique library item name, recording it in @a aUsedNames.
 */
wxString MakeUniqueLibName( std::set<wxString>& aUsedNames, const wxString& aName, const wxString& aFallback );

/**
 * Stable project-lib item names keyed by Device UUID (one entry per Device).
 * Also writes project["device_lib_names"] for place-time symbol LIB_ID lookup.
 * When @a aUsedNames is non-null, it receives the allocated names (for orphan uniqueness).
 */
std::map<wxString, wxString> BuildV3DeviceLibNames( nlohmann::json&                       aProject,
                                                    const std::map<wxString, PRJ_DEVICE>& aDevices,
                                                    std::set<wxString>*                   aUsedNames = nullptr );

wxString LookupV3DeviceLibName( const nlohmann::json& aProject, const wxString& aDeviceUuid );

struct V3_DEVICE_DATA
{
    bool                         found = false;
    wxString                     description;
    std::map<wxString, wxString> attributes;
};

V3_DEVICE_DATA GetV3DeviceData( const nlohmann::json& aProject, const wxString& aDeviceUuid );

/**
 * Preferred Value text: Value attribute, else Name, with variables resolved.
 */
wxString ResolveV3DeviceValueText( const std::map<wxString, wxString>& aDeviceAttributes );

/**
 * Invoke @a aCallback for each non-empty whitelisted Device field (resolved + normalized).
 * When @a aIncludeValue is false, skips the Value key (callers often handle it separately).
 */
void ForEachImportedDeviceField( const std::map<wxString, wxString>& aDeviceAttributes, bool aIncludeValue,
                                 const std::function<void( const wxString& aKey, const wxString& aValue )>& aCallback );

/**
 * Build the schematic-library name map for a v3 .elibz2.
 *
 * Prefers devices from the library index when present (device2.json); otherwise falls
 * back to symbols.  Each entry's @a symbolUuid is the geometry document to parse;
 * @a device is populated only for device-backed entries.
 */
struct V3_SYMBOL_LIB_ITEM
{
    wxString   symbolUuid;
    PRJ_DEVICE device;
    bool       hasDevice = false;
};

std::map<wxString, V3_SYMBOL_LIB_ITEM> BuildV3SymbolLibraryMap( const V3_DOC_PARSER& aParser );

std::map<wxString, wxString> BuildV3LibraryItemMap( const V3_DOC_PARSER& aParser, const char* aIndexKey,
                                                    const wxString& aDocType );

} // namespace EASYEDAPRO


#endif // EASYEDAPRO_IMPORT_UTILS_H_

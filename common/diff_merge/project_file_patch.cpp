/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <diff_merge/project_file_patch.h>
#include <diff_merge/kicad_diff_types.h>

#include <json_common.h>
#include <kiplatform/io.h>

#include <wx/filefn.h>
#include <wx/log.h>
#include <wx/stdstream.h>
#include <wx/wfstream.h>

#include <unordered_map>


namespace KICAD_DIFF
{

namespace
{

/// JSON pointer path for each DOC_PROP key persisted in `.kicad_pro`. Pointer
/// strings are RFC 6901 (leading slash, segments separated by `/`). DOC_PROPs
/// not handled here do NOT live in the project file (rules / lib-tables go to
/// sibling files; board-scoped props go through the document file).
std::optional<std::string> docPropToPointer( const wxString& aDocProp, DOC_KIND aKind )
{
    static const std::unordered_map<wxString, std::string> table = {
        // BOARD_DESIGN_SETTINGS is a NESTED_SETTINGS instance registered at
        // "design_settings" under "board"; its rule_severities param therefore
        // serializes to /board/design_settings/rule_severities.
        { DOC_PROP_DRC_SEVERITIES, "/board/design_settings/rule_severities" },

        // ERC severities are a NESTED_SETTINGS instance registered at "erc".
        { DOC_PROP_ERC_SEVERITIES, "/erc" },

        // NET_SETTINGS is a NESTED_SETTINGS instance registered at "net_settings".
        { DOC_PROP_NET_CLASSES,    "/net_settings" },
    };

    if( aDocProp == DOC_PROP_DRAWING_SHEET )
    {
        if( aKind == DOC_KIND::SCH )
            return "/schematic/page_layout_descr_file";

        return "/pcbnew/page_layout_descr_file";
    }

    auto it = table.find( aDocProp );

    if( it == table.end() )
        return std::nullopt;

    return it->second;
}

} // namespace


std::optional<std::string> DocPropJsonPointer( const wxString& aDocProp, DOC_KIND aKind )
{
    return docPropToPointer( aDocProp, aKind );
}


std::optional<std::string> DocPropJsonPointer( const wxString& aDocProp )
{
    return DocPropJsonPointer( aDocProp, DOC_KIND::PCB );
}


bool ApplyProjectFilePatch( nlohmann::json&       aTarget,
                            const nlohmann::json& aSource,
                            const wxString&       aDocProp,
                            DOC_KIND              aKind )
{
    std::optional<std::string> pointer = DocPropJsonPointer( aDocProp, aKind );

    if( !pointer )
        return false;

    try
    {
        nlohmann::json::json_pointer ptr( *pointer );

        if( !aSource.contains( ptr ) )
            return false;

        // The assignment resolves the pointer against aTarget and can throw when
        // an intermediate node is not an object (corrupt output file); keep it
        // inside the guard so a malformed target degrades to the caller's full
        // SaveProjectCopy fallback rather than escaping uncaught.
        aTarget[ptr] = aSource[ptr];
    }
    catch( const std::exception& )
    {
        return false;
    }

    return true;
}


bool ApplyProjectFilePatch( nlohmann::json&       aTarget,
                            const nlohmann::json& aSource,
                            const wxString&       aDocProp )
{
    return ApplyProjectFilePatch( aTarget, aSource, aDocProp, DOC_KIND::PCB );
}


bool ApplyProjectFilePatches( const wxString&           aOutputProPath,
                              const nlohmann::json&     aSource,
                              const std::set<wxString>& aDocProps,
                              DOC_KIND                  aKind )
{
    if( aDocProps.empty() )
        return true;

    nlohmann::json target;

    if( wxFileExists( aOutputProPath ) )
    {
        wxFFileInputStream fileStream( aOutputProPath );

        if( !fileStream.IsOk() )
            return false;

        if( fileStream.GetLength() == 0 )
        {
            target = nlohmann::json::object();
        }
        else
        {
            try
            {
                // Parse the raw UTF-8 byte stream directly (matching
                // JSON_SETTINGS::LoadFromFile).  Round-tripping through
                // wxString would re-encode via the C locale and corrupt
                // non-ASCII content.
                wxStdInputStream stdStream( fileStream );
                target = nlohmann::json::parse( stdStream, nullptr, true, /* ignore_comments */ true );
            }
            catch( const std::exception& )
            {
                // Existing file is unparseable.  Refuse to overwrite it with
                // the patched source; the caller's full SaveProjectCopy
                // fallback can take over.
                return false;
            }
        }
    }
    else
    {
        // No pre-existing output file -- start from source and patch the
        // requested fields onto it.  The end result is identical to a full
        // copy in this case, which is the right behaviour: we want the
        // merge's chosen-side values in place.
        target = aSource;
    }

    for( const wxString& docProp : aDocProps )
    {
        if( !ApplyProjectFilePatch( target, aSource, docProp, aKind ) )
            return false;
    }

    // Write atomically so a crash or power loss mid-write can't corrupt the
    // .kicad_pro (matches JSON_SETTINGS::SaveToFile).  dump( 2 ) emits UTF-8;
    // write its bytes verbatim so the on-disk content is byte-identical.
    std::string payload = target.dump( 2 );

    return KIPLATFORM::IO::AtomicWriteFile( aOutputProPath, payload.data(), payload.size() );
}


bool ApplyProjectFilePatches( const wxString&           aOutputProPath,
                              const nlohmann::json&     aSource,
                              const std::set<wxString>& aDocProps )
{
    return ApplyProjectFilePatches( aOutputProPath, aSource, aDocProps, DOC_KIND::PCB );
}

} // namespace KICAD_DIFF

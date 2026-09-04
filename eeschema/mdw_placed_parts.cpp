/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "mdw_placed_parts.h"

#include <diff_merge/sym_digest.h>
#include <erc/erc_settings.h>
#include <io/io_mgr.h>
#include <json_common.h>
#include <mdw_part_field.h>
#include <project.h>
#include <project/project_file.h>
#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <schematic_settings.h>
#include <wx_filename.h>

#include <wx/filename.h>

#include <map>
#include <memory>
#include <tuple>


namespace
{

/// A schematic bound to a project replaces its ERC and schematic settings, so an open editor's are put back
struct BORROWED_SETTINGS
{
    explicit BORROWED_SETTINGS( PROJECT_FILE& aFile ) :
            m_file( aFile ),
            m_erc( aFile.m_ErcSettings ),
            m_schematic( aFile.m_SchematicSettings )
    {
    }

    ~BORROWED_SETTINGS()
    {
        delete m_file.m_ErcSettings;
        delete m_file.m_SchematicSettings;
        m_file.m_ErcSettings = m_erc;
        m_file.m_SchematicSettings = m_schematic;
    }

    PROJECT_FILE&       m_file;
    ERC_SETTINGS*       m_erc;
    SCHEMATIC_SETTINGS* m_schematic;
};


/// The schematic saved at aFile with every top-level sheet the project declares
bool loadSchematic( const wxString& aFile, PROJECT& aProject, SCHEMATIC& aSchematic, PROGRESS_REPORTER* aReporter )
{
    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );

    pi->SetProgressReporter( aReporter );

    SCH_SHEET* root = pi->LoadSchematicFile( aFile, &aSchematic );

    // An incomplete hierarchy would leave its parts unchecked
    if( !root || !pi->GetError().IsEmpty() )
        return false;

    std::vector<SCH_SHEET*> topLevelSheets = aSchematic.GetTopLevelSheets();
    bool rootIsTopLevel = std::find( topLevelSheets.begin(), topLevelSheets.end(), root ) != topLevelSheets.end();
    bool rootIsVirtualRoot = root == &aSchematic.Root() || root->IsVirtualRootSheet();
    bool changed = false;

    if( !rootIsTopLevel && !rootIsVirtualRoot )
    {
        topLevelSheets = { root };
        changed = true;
    }

    if( !rootIsVirtualRoot )
    {
        // A flat hierarchy names its other top-level sheets only in the project file
        wxFileName rootFile( aFile );
        rootFile.Normalize( FN_NORMALIZE_FLAGS );

        for( const TOP_LEVEL_SHEET_INFO& info : aProject.GetProjectFile().GetTopLevelSheets() )
        {
            wxFileName fn( aProject.GetProjectPath(), info.filename );
            fn.Normalize( FN_NORMALIZE_FLAGS );

            // The project's path may be spelled differently from aFile, which must not load twice
            if( fn == rootFile )
                continue;

            SCH_SHEET* sibling = pi->LoadSchematicFile( fn.GetFullPath(), &aSchematic );

            if( !sibling || !pi->GetError().IsEmpty() )
                return false;

            // Sheet instance paths below a top-level sheet start at the uuid the project file gives it
            if( info.uuid != niluuid )
                const_cast<KIID&>( sibling->m_Uuid ) = info.uuid;

            sibling->SetName( info.name );
            topLevelSheets.push_back( sibling );
            changed = true;
        }
    }

    if( changed )
        aSchematic.SetTopLevelSheets( topLevelSheets );

    aSchematic.RefreshHierarchy();

    if( aSchematic.RootScreen()->GetFileFormatVersionAtLoad() < 20221002 )
        aSchematic.Hierarchy().UpdateSymbolInstanceData( aSchematic.RootScreen()->GetSymbolInstances() );

    return true;
}

} // namespace


std::optional<std::string> SchCollectPlacedParts( const wxString& aFile, PROJECT& aProject,
                                                  PROGRESS_REPORTER* aReporter )
{
    BORROWED_SETTINGS settings( aProject.GetProjectFile() );
    SCHEMATIC         schematic( &aProject );

    schematic.CreateDefaultScreens();

    try
    {
        if( !loadSchematic( aFile, aProject, schematic, aReporter ) )
            return std::nullopt;
    }
    catch( ... )
    {
        // A malformed file can throw more than IO_ERROR, which must not reach Release Design's UI thread
        return std::nullopt;
    }

    nlohmann::json parts = nlohmann::json::array();

    // Units and sheet instances placing one library symbol of a screen share its digest
    std::map<std::tuple<const SCH_SCREEN*, wxString, wxString>, wxString> digests;

    for( const SCH_SHEET_PATH& sheet : schematic.Hierarchy() )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            wxString    libId = symbol->GetLibId().Format().wx_str();
            auto [digest, added] =
                    digests.try_emplace( { sheet.LastScreen(), symbol->GetSchSymbolLibraryName(), libId } );

            if( added )
                digest->second = KICAD_DIFF::PlacedSymbolDigest( *symbol );

            nlohmann::json part = { { "reference", symbol->GetRef( &sheet ).utf8_string() },
                                    { "lib_id", libId.utf8_string() },
                                    { "digest", digest->second.utf8_string() },
                                    { "power", symbol->IsPower() } };

            if( std::optional<MDW_PART_IDENTITY> identity = MdwReadPartField( *symbol ) )
            {
                part["part"] = { { "source", identity->part.source.utf8_string() },
                                 { "part_id", identity->part.item_id.utf8_string() },
                                 { "revision_id", identity->revision_id.utf8_string() } };
            }

            parts.push_back( std::move( part ) );
        }
    }

    return parts.dump();
}

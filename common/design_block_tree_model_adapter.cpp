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
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <pgm_base.h>
#include <kiface_base.h>
#include <eda_base_frame.h>
#include <core/kicad_algo.h>
#include <settings/common_settings.h>
#include <project/project_file.h>
#include <wx/log.h>
#include <wx/tokenzr.h>
#include <settings/app_settings.h>
#include <string_utils.h>
#include <eda_pattern_match.h>
#include <design_block.h>
#include <design_block_library_adapter.h>
#include <design_block_tree_model_adapter.h>

wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER>
DESIGN_BLOCK_TREE_MODEL_ADAPTER::Create( EDA_BASE_FRAME* aParent, DESIGN_BLOCK_LIBRARY_ADAPTER* aLibs,
                                         APP_SETTINGS_BASE::LIB_TREE& aSettings,
                                         TOOL_INTERACTIVE* aContextMenuTool )
{
    auto* adapter = new DESIGN_BLOCK_TREE_MODEL_ADAPTER( aParent, aLibs, aSettings, aContextMenuTool );
    return wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER>( adapter );
}


DESIGN_BLOCK_TREE_MODEL_ADAPTER::DESIGN_BLOCK_TREE_MODEL_ADAPTER( EDA_BASE_FRAME* aParent, DESIGN_BLOCK_LIBRARY_ADAPTER* aLibs,
                                                                  APP_SETTINGS_BASE::LIB_TREE& aSettings,
                                                                  TOOL_INTERACTIVE*            aContextMenuTool ) :
        LIB_TREE_MODEL_ADAPTER( aParent, wxT( "pinned_design_block_libs" ),
                                Kiface().KifaceSettings()->m_DesignBlockChooserPanel.tree ),
        m_libs( aLibs ),
        m_frame( aParent ),
        m_contextMenuTool( aContextMenuTool )
{
}


void DESIGN_BLOCK_TREE_MODEL_ADAPTER::AddLibraries( EDA_BASE_FRAME* aParent )
{
    COMMON_SETTINGS* cfg = Pgm().GetCommonSettings();
    PROJECT_FILE&    project = aParent->Prj().GetProjectFile();
    LIBRARY_MANAGER& manager = Pgm().GetLibraryManager();

    for( const LIBRARY_TABLE_ROW* row : manager.Rows( LIBRARY_TABLE_TYPE::DESIGN_BLOCK ) )
    {
        if( row->Hidden() )
            continue;

        wxString libName = row->Nickname();

        bool pinned = alg::contains( cfg->m_Session.pinned_design_block_libs, libName )
                      || alg::contains( project.m_PinnedDesignBlockLibs, libName );

        DoAddLibrary( libName, row->Description(), getDesignBlocks( aParent, libName ), pinned, true );
    }

    m_tree.AssignIntrinsicRanks( m_shownColumns );
}


void DESIGN_BLOCK_TREE_MODEL_ADAPTER::ClearLibraries()
{
    m_tree.Clear();
}


std::vector<LIB_TREE_ITEM*> DESIGN_BLOCK_TREE_MODEL_ADAPTER::getDesignBlocks( EDA_BASE_FRAME* aParent,
                                                                              const wxString& aLibName )
{
    std::vector<LIB_TREE_ITEM*> libList;

    DESIGN_BLOCK_LIBRARY_ADAPTER* libs = m_frame->Prj().DesignBlockLibs();

    std::vector<DESIGN_BLOCK*> blocks = libs->GetDesignBlocks( aLibName );

    for( DESIGN_BLOCK* block : blocks )
    {
        LIB_ID id = block->GetLIB_ID();
        id.SetLibNickname( aLibName );
        block->SetLibId( id );
        libList.emplace_back( block );
    }

    return libList;
}


wxString DESIGN_BLOCK_TREE_MODEL_ADAPTER::GenerateInfo( LIB_ID const& aLibId, int aUnit )
{
    static const wxString DescriptionFormat = wxT(
            "<b>__NAME__</b>"
            "__DESC__"
            "__KEY__"
            "<hr><table border=0>"
            "__FIELDS__"
            "</table>" );

    static const wxString DescFormat =      wxS( "<br>%s" );
    static const wxString KeywordsFormat =  wxS( "<br>" ) + _( "Keywords" ) + wxS( ": %s" );

    static const wxString FieldFormat = wxT(
            "<tr>"
            "   <td><b>__FIELD_NAME__</b></td>"
            "   <td>__FIELD_VALUE__</td>"
            "</tr>" );


    if( !aLibId.IsValid() )
        return wxEmptyString;

    const DESIGN_BLOCK* db = nullptr;

    try
    {
        db = m_libs->GetEnumeratedDesignBlock( aLibId.GetLibNickname(), aLibId.GetLibItemName() );
    }
    catch( const IO_ERROR& ioe )
    {
        wxLogError( _( "Error loading design block %s from library '%s'." ) + wxS( "\n%s" ),
                    aLibId.GetLibItemName().wx_str(), aLibId.GetLibNickname().wx_str(), ioe.What() );

        return wxEmptyString;
    }

    wxString html = DescriptionFormat;

    if( db )
    {
        wxString name = aLibId.GetLibItemName();
        wxString desc = db->GetLibDescription();
        wxString keywords = db->GetKeywords();

        html.Replace( "__NAME__", EscapeHTML( name ) );

        wxString esc_desc = EscapeHTML( UnescapeString( desc ) );

        // Add line breaks
        esc_desc.Replace( wxS( "\n" ), wxS( "<br>" ) );

        // Add links
        esc_desc = LinkifyHTML( esc_desc );

        if( esc_desc.IsEmpty() )
            html.Replace( "__DESC__", wxEmptyString );
        else
            html.Replace( "__DESC__", wxString::Format( DescFormat, esc_desc ) );

        if( keywords.IsEmpty() )
            html.Replace( "__KEY__", wxEmptyString );
        else
            html.Replace( "__KEY__", wxString::Format( KeywordsFormat, EscapeHTML( keywords ) ) );

        wxString fieldTable;

        for( const auto& [key, value] : db->GetFields() )
        {
            wxString fieldRow = FieldFormat;
            fieldRow.Replace( wxS( "__FIELD_NAME__" ), EscapeHTML( key ) );
            fieldRow.Replace( wxS( "__FIELD_VALUE__" ), EscapeHTML( value ) );
            fieldTable += fieldRow;
        }

        html.Replace( "__FIELDS__", fieldTable );

        // Design blocks (unlike symbols and footprints) are not cached.  We own the pointer.
        delete db;
    }
    else
    {
        html.Printf( _( "Error loading design block %s from library '%s'." ) + wxS( "\n" ),
                     aLibId.GetLibItemName().wx_str(), aLibId.GetLibNickname().wx_str() );
    }

    return html;
}


TOOL_INTERACTIVE* DESIGN_BLOCK_TREE_MODEL_ADAPTER::GetContextMenuTool()
{
    return m_contextMenuTool;
}

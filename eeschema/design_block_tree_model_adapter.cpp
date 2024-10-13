/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2024 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <string_utils.h>
#include <eda_pattern_match.h>
#include <design_block.h>
#include <design_block_lib_table.h>
#include <design_block_info.h>
#include <design_block_tree_model_adapter.h>
#include <tools/sch_design_block_control.h>

wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER>
DESIGN_BLOCK_TREE_MODEL_ADAPTER::Create( EDA_BASE_FRAME* aParent, LIB_TABLE* aLibs )
{
    auto* adapter = new DESIGN_BLOCK_TREE_MODEL_ADAPTER( aParent, aLibs );
    adapter->m_frame = aParent;
    return wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER>( adapter );
}


DESIGN_BLOCK_TREE_MODEL_ADAPTER::DESIGN_BLOCK_TREE_MODEL_ADAPTER( EDA_BASE_FRAME* aParent,
                                                                  LIB_TABLE*      aLibs ) :
        LIB_TREE_MODEL_ADAPTER( aParent, wxT( "pinned_design_block_libs" ),
                                Kiface().KifaceSettings() ),
        m_libs( (DESIGN_BLOCK_LIB_TABLE*) aLibs )
{
}


void DESIGN_BLOCK_TREE_MODEL_ADAPTER::AddLibraries( EDA_BASE_FRAME* aParent )
{
    COMMON_SETTINGS* cfg = Pgm().GetCommonSettings();
    PROJECT_FILE&    project = aParent->Prj().GetProjectFile();

    for( const wxString& libName : m_libs->GetLogicalLibs() )
    {
        const DESIGN_BLOCK_LIB_TABLE_ROW* library = nullptr;

        try
        {
            library = m_libs->FindRow( libName, true );
        }
        catch( ... )
        {
            // Skip loading this library, if not exists/ not found
            continue;
        }
        bool pinned = alg::contains( cfg->m_Session.pinned_design_block_libs, libName )
                      || alg::contains( project.m_PinnedDesignBlockLibs, libName );

        DoAddLibrary( libName, library->GetDescr(), getDesignBlocks( aParent, libName ), pinned,
                      true );
    }

    m_tree.AssignIntrinsicRanks();
}


void DESIGN_BLOCK_TREE_MODEL_ADAPTER::ClearLibraries()
{
    m_tree.Clear();
}


std::vector<LIB_TREE_ITEM*>
DESIGN_BLOCK_TREE_MODEL_ADAPTER::getDesignBlocks( EDA_BASE_FRAME* aParent,
                                                  const wxString& aLibName )
{
    std::vector<LIB_TREE_ITEM*> libList;

    auto fullListStart = DESIGN_BLOCK_LIB_TABLE::GetGlobalList().GetList().begin();
    auto fullListEnd = DESIGN_BLOCK_LIB_TABLE::GetGlobalList().GetList().end();

    std::unique_ptr<DESIGN_BLOCK_INFO> dummy =
            std::make_unique<DESIGN_BLOCK_INFO_IMPL>( aLibName, wxEmptyString );

    // List is sorted, so use a binary search to find the range of footnotes for our library
    auto libBounds = std::equal_range(
            fullListStart, fullListEnd, dummy,
            []( const std::unique_ptr<DESIGN_BLOCK_INFO>& a,
                const std::unique_ptr<DESIGN_BLOCK_INFO>& b )
            {
                return StrNumCmp( a->GetLibNickname(), b->GetLibNickname(), false ) < 0;
            } );

    for( auto i = libBounds.first; i != libBounds.second; ++i )
        libList.push_back( i->get() );

    return libList;
}


wxString DESIGN_BLOCK_TREE_MODEL_ADAPTER::GenerateInfo( LIB_ID const& aLibId, int aUnit )
{
    const wxString DescriptionFormat = wxT(
            "<b>__NAME__</b>"
            "<br>__DESC__"
            "<hr><table border=0>"
            "__FIELDS__"
            "</table>" );

    const wxString KeywordsFormat = wxT(
            "<tr>"
            "   <td><b>" + _( "Keywords" ) + "</b></td>"
            "   <td>__KEYWORDS__</td>"
            "</tr>" );

    const wxString DocFormat = wxT(
            "<tr>"
            "   <td><b>" + _( "Documentation" ) + "</b></td>"
            "   <td><a href=\"__HREF__\">__TEXT__</a></td>"
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
                    aLibId.GetLibItemName().wx_str(), aLibId.GetLibNickname().wx_str(),
                    ioe.What() );

        return wxEmptyString;
    }

    wxString html = DescriptionFormat;

    if( db )
    {
        wxString name = aLibId.GetLibItemName();
        wxString desc = db->GetLibDescription();
        wxString keywords = db->GetKeywords();
        wxString doc;

        // It is currently common practice to store a documentation link in the description.
        size_t idx = desc.find( wxT( "http:" ) );

        if( idx == wxString::npos )
            idx = desc.find( wxT( "https:" ) );

        if( idx != wxString::npos )
        {
            int nesting = 0;

            for( auto chit = desc.begin() + idx; chit != desc.end(); ++chit )
            {
                int ch = *chit;

                // Break on invalid URI characters
                if( ch <= 0x20 || ch >= 0x7F || ch == '"' )
                    break;

                // Check for nesting parentheses, e.g. (Body style from: https://this.url/part.pdf)
                if( ch == '(' )
                    ++nesting;
                else if( ch == ')' && --nesting < 0 )
                    break;

                doc += ch;
            }

            // Trim trailing punctuation
            static wxString punct = wxS( ".,:;" );

            if( punct.find( doc.Last() ) != wxString::npos )
                doc = doc.Left( doc.Length() - 1 );
        }

        wxString esc_desc = EscapeHTML( UnescapeString( desc ) );

        // Add line breaks
        esc_desc.Replace( wxS( "\n" ), wxS( "<br>" ) );

        // Add links
        esc_desc = LinkifyHTML( esc_desc );

        html.Replace( "__DESC__", esc_desc );
        html.Replace( "__NAME__", EscapeHTML( name ) );

        wxString keywordsHtml = KeywordsFormat;
        keywordsHtml.Replace( "__KEYWORDS__", EscapeHTML( keywords ) );

        wxString docHtml = DocFormat;
        docHtml.Replace( "__HREF__", doc );

        if( doc.Length() > 75 )
            doc = doc.Left( 72 ) + wxT( "..." );

        docHtml.Replace( "__TEXT__", EscapeHTML( doc ) );

        html.Replace( "__FIELDS__", keywordsHtml + docHtml );
    }

    return html;
}


TOOL_INTERACTIVE* DESIGN_BLOCK_TREE_MODEL_ADAPTER::GetContextMenuTool()
{
    return m_frame->GetToolManager()->GetTool<SCH_DESIGN_BLOCK_CONTROL>();
}

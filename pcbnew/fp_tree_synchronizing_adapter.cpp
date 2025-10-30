/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <pgm_base.h>
#include <project/project_file.h>
#include <fp_tree_synchronizing_adapter.h>
#include <footprint_edit_frame.h>
#include <footprint_preview_panel.h>
#include <footprint_library_adapter.h>
#include <footprint_info_impl.h>
#include <gal/graphics_abstraction_layer.h>
#include <project_pcb.h>
#include <string_utils.h>
#include <board.h>
#include <footprint.h>
#include <tool/tool_manager.h>
#include <tools/footprint_editor_control.h>
#include <wx/settings.h>


wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER>
FP_TREE_SYNCHRONIZING_ADAPTER::Create( FOOTPRINT_EDIT_FRAME* aFrame, FOOTPRINT_LIBRARY_ADAPTER* aLibs )
{
    auto* adapter = new FP_TREE_SYNCHRONIZING_ADAPTER( aFrame, aLibs );
    return wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER>( adapter );
}


FP_TREE_SYNCHRONIZING_ADAPTER::FP_TREE_SYNCHRONIZING_ADAPTER( FOOTPRINT_EDIT_FRAME* aFrame,
                                                              FOOTPRINT_LIBRARY_ADAPTER* aLibs ) :
        FP_TREE_MODEL_ADAPTER( aFrame, aLibs ),
        m_frame( aFrame )
{
}


TOOL_INTERACTIVE* FP_TREE_SYNCHRONIZING_ADAPTER::GetContextMenuTool()
{
    return m_frame->GetToolManager()->GetTool<FOOTPRINT_EDITOR_CONTROL>();
}


bool FP_TREE_SYNCHRONIZING_ADAPTER::IsContainer( const wxDataViewItem& aItem ) const
{
    const LIB_TREE_NODE* node = ToNode( aItem );
    return node ? node->m_Type == LIB_TREE_NODE::TYPE::LIBRARY : true;
}


#define PROGRESS_INTERVAL_MILLIS 33     // 30 FPS refresh rate

void FP_TREE_SYNCHRONIZING_ADAPTER::Sync( FOOTPRINT_LIBRARY_ADAPTER* aLibs )
{
    m_libs = aLibs;

    // Process already stored libraries
    for( auto it = m_tree.m_Children.begin(); it != m_tree.m_Children.end(); )
    {
        const wxString& name = it->get()->m_Name;

        try
        {
            // Remove the library if it no longer exists or it exists in both the global and the
            // project library but the project library entry is disabled.
            if( !m_libs->HasLibrary( name, true )
                || m_libs->HasLibrary( name, true ) != m_libs->HasLibrary( name, false ) )
            {
                it = deleteLibrary( it );
                continue;
            }

            updateLibrary( *(LIB_TREE_NODE_LIBRARY*) it->get() );
        }
        catch( ... )
        {
            // If the library isn't found, remove it
            it = deleteLibrary( it );
            continue;
        }

        ++it;
    }

    // Look for new libraries
    COMMON_SETTINGS* cfg = Pgm().GetCommonSettings();
    PROJECT_FILE&    project = m_frame->Prj().GetProjectFile();
    size_t           count = m_libMap.size();

    for( const wxString& libName : m_libs->GetLibraryNames() )
    {
        if( m_libMap.count( libName ) == 0 )
        {
            if( std::optional<wxString> optDesc = PROJECT_PCB::FootprintLibAdapter( &m_frame->Prj() )->
                    GetLibraryDescription( libName ) )
            {
                bool pinned = alg::contains( cfg->m_Session.pinned_fp_libs, libName )
                                || alg::contains( project.m_PinnedFootprintLibs, libName );

                DoAddLibrary( libName, *optDesc, getFootprints( libName ), pinned, true );
                m_libMap.insert( libName  );
            }
        }
    }

    if( m_libMap.size() > count )
        m_tree.AssignIntrinsicRanks( m_shownColumns );
}


int FP_TREE_SYNCHRONIZING_ADAPTER::GetLibrariesCount() const
{
    return m_libs->GetLibraryNames().size();
}


void FP_TREE_SYNCHRONIZING_ADAPTER::updateLibrary( LIB_TREE_NODE_LIBRARY& aLibNode )
{
    std::vector<LIB_TREE_ITEM*> footprints = getFootprints( aLibNode.m_Name );

    // remove the common part from the footprints list
    for( auto nodeIt = aLibNode.m_Children.begin(); nodeIt != aLibNode.m_Children.end();  )
    {
        // Since the list is sorted we can use a binary search to speed up searches within
        // libraries with lots of footprints.
        FOOTPRINT_INFO_IMPL dummy( wxEmptyString, (*nodeIt)->m_Name );
        auto footprintIt = std::lower_bound( footprints.begin(), footprints.end(), &dummy,
                []( LIB_TREE_ITEM* a, LIB_TREE_ITEM* b )
                {
                    return StrNumCmp( a->GetName(), b->GetName(), false ) < 0;
                } );

        if( footprintIt != footprints.end() && dummy.GetName() == (*footprintIt)->GetName() )
        {
            // footprint exists both in the lib tree and the footprint info list; just
            // update the node data
            static_cast<LIB_TREE_NODE_ITEM*>( nodeIt->get() )->Update( *footprintIt );
            footprints.erase( footprintIt );
            ++nodeIt;
        }
        else
        {
            // node does not exist in the library manager, remove the corresponding node
            nodeIt = aLibNode.m_Children.erase( nodeIt );
        }
    }

    // now the footprint list contains only new aliases that need to be added to the tree
    for( LIB_TREE_ITEM* footprint : footprints )
        aLibNode.AddItem( footprint );

    aLibNode.AssignIntrinsicRanks( m_shownColumns );
    m_libMap.insert( aLibNode.m_Name );
}


LIB_TREE_NODE::PTR_VECTOR::iterator
FP_TREE_SYNCHRONIZING_ADAPTER::deleteLibrary( LIB_TREE_NODE::PTR_VECTOR::iterator& aLibNodeIt )
{
    LIB_TREE_NODE* node = aLibNodeIt->get();
    m_libMap.erase( node->m_Name );
    auto it = m_tree.m_Children.erase( aLibNodeIt );
    return it;
}


wxDataViewItem FP_TREE_SYNCHRONIZING_ADAPTER::GetCurrentDataViewItem()
{
    return FindItem( m_frame->GetLoadedFPID() );
}


void FP_TREE_SYNCHRONIZING_ADAPTER::GetValue( wxVariant& aVariant, wxDataViewItem const& aItem,
                                              unsigned int aCol ) const
{
    if( IsFrozen() )
    {
        aVariant = wxEmptyString;
        return;
    }

    LIB_TREE_NODE* node = ToNode( aItem );

    switch( aCol )
    {
    case NAME_COL:
    {
        if( node->m_LibId == m_frame->GetLoadedFPID() && !m_frame->IsCurrentFPFromBoard() )
        {
            // Do not use GetLoadedFPID(); it returns m_footprintNameWhenLoaded.
            node->m_Name =
                    m_frame->GetBoard()->GetFirstFootprint()->GetFPID().GetUniStringLibItemName();

            // mark modified part with an asterisk
            if( m_frame->GetScreen()->IsContentModified() )
                aVariant = node->m_Name + wxT( " *" );
            else
                aVariant = node->m_Name;
        }
        else if( node->m_Pinned )
        {
            aVariant = GetPinningSymbol() + node->m_Name;
        }
        else
        {
            aVariant = node->m_Name;
        }

        break;
    }

    case DESC_COL:
    {
        if( node->m_LibId == m_frame->GetLoadedFPID() && !m_frame->IsCurrentFPFromBoard() )
        {
            node->m_Desc = m_frame->GetBoard()->GetFirstFootprint()->GetLibDescription();
        }
        else if( node->m_Type == LIB_TREE_NODE::TYPE::LIBRARY )
        {
            if( std::optional<wxString> optDesc = PROJECT_PCB::FootprintLibAdapter( &m_frame->Prj() )->
                    GetLibraryDescription( node->m_LibId.GetLibNickname() ) )
            {
                node->m_Desc = *optDesc;
            }
        }

        wxString descStr = UnescapeString( node->m_Desc );
        descStr.Replace( wxS( "\n" ), wxS( " " ) ); // Clear line breaks

        aVariant = descStr;
        break;
    }

    default: // column == -1 is used for default Compare function
        aVariant = node->m_Name;
        break;
    }
}


bool FP_TREE_SYNCHRONIZING_ADAPTER::GetAttr( wxDataViewItem const& aItem, unsigned int aCol,
                                             wxDataViewItemAttr& aAttr ) const
{
    if( IsFrozen() )
        return false;

    // change attributes only for the name field
    if( aCol != 0 )
        return false;

    // don't link to a board footprint, even if the FPIDs match
    if( m_frame->IsCurrentFPFromBoard() )
        return false;

    LIB_TREE_NODE* node = ToNode( aItem );
    wxCHECK( node, false );

    switch( node->m_Type )
    {
    case LIB_TREE_NODE::TYPE::LIBRARY:
        if( node->m_Name == m_frame->GetLoadedFPID().GetLibNickname().wx_str() )
        {
            // mark the current library if it's collapsed
            if( !m_widget->IsExpanded( ToItem( node ) ) )
            {
                aAttr.SetStrikethrough( true );   // LIB_TREE_RENDERER uses strikethrough as a
                                                  // proxy for "is canvas item"
            }

            // mark modified libs with bold font
            if( m_frame->GetScreen()->IsContentModified() && !m_frame->IsCurrentFPFromBoard() )
                aAttr.SetBold( true );
        }
        break;

    case LIB_TREE_NODE::TYPE::ITEM:
        if( node->m_LibId == m_frame->GetLoadedFPID() )
        {
            // mark the current (on-canvas) part
            aAttr.SetStrikethrough( true );     // LIB_TREE_RENDERER uses strikethrough as a
                                                // proxy for "is canvas item"

            // mark modified part with bold font
            if( m_frame->GetScreen()->IsContentModified() && !m_frame->IsCurrentFPFromBoard() )
                aAttr.SetBold( true );
        }
        break;

    default:
        return false;
    }

    return true;
}


bool FP_TREE_SYNCHRONIZING_ADAPTER::HasPreview( const wxDataViewItem& aItem )
{
    LIB_TREE_NODE* node = ToNode( aItem );
    wxCHECK( node, false );

    return node->m_Type == LIB_TREE_NODE::TYPE::ITEM && node->m_LibId != m_frame->GetLoadedFPID();
}


static const wxString c_previewName = wxS( "fpHoverPreview" );


void FP_TREE_SYNCHRONIZING_ADAPTER::ShowPreview( wxWindow* aParent, const wxDataViewItem& aItem )
{
    LIB_TREE_NODE* node = ToNode( aItem );
    wxCHECK( node, /* void */ );

    wxWindow* previewWindow = wxWindow::FindWindowByName( c_previewName, aParent );
    FOOTPRINT_PREVIEW_PANEL* preview = dynamic_cast<FOOTPRINT_PREVIEW_PANEL*>( previewWindow );

    if( !preview )
    {
        wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );
        aParent->SetSizer( mainSizer );

        preview = FOOTPRINT_PREVIEW_PANEL::New( &m_frame->Kiway(), aParent, m_frame );

        preview->SetName( c_previewName );
        preview->GetGAL()->SetAxesEnabled( false );

        mainSizer->Add( preview, 1, wxEXPAND | wxALL, 1 );
        aParent->Layout();
    }

    preview->DisplayFootprint( node->m_LibId );
}


void FP_TREE_SYNCHRONIZING_ADAPTER::ShutdownPreview( wxWindow* aParent )
{
    wxWindow* previewWindow = wxWindow::FindWindowByName( c_previewName, aParent );

    if( FOOTPRINT_PREVIEW_PANEL* preview = dynamic_cast<FOOTPRINT_PREVIEW_PANEL*>( previewWindow ) )
    {
        preview->GetCanvas()->SetEvtHandlerEnabled( false );
        preview->GetCanvas()->StopDrawing();
    }
}

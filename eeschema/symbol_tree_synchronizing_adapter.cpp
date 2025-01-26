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

#include "symbol_tree_synchronizing_adapter.h"

#include <wx/settings.h>

#include <pgm_base.h>
#include <project/project_file.h>
#include <lib_symbol_library_manager.h>
#include <symbol_lib_table.h>
#include <tools/symbol_editor_control.h>
#include <project_sch.h>
#include <string_utils.h>
#include <symbol_preview_widget.h>
#include <widgets/wx_panel.h>


wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER>
SYMBOL_TREE_SYNCHRONIZING_ADAPTER::Create( SYMBOL_EDIT_FRAME* aParent,
                                           SYMBOL_LIBRARY_MANAGER* aLibMgr )
{
    auto* adapter = new SYMBOL_TREE_SYNCHRONIZING_ADAPTER( aParent, aLibMgr );
    return wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER>( adapter );
}


SYMBOL_TREE_SYNCHRONIZING_ADAPTER::SYMBOL_TREE_SYNCHRONIZING_ADAPTER( SYMBOL_EDIT_FRAME* aParent,
                                                                      SYMBOL_LIBRARY_MANAGER* aLibMgr ) :
        LIB_TREE_MODEL_ADAPTER( aParent, "pinned_symbol_libs",
                                aParent->GetViewerSettingsBase()->m_LibTree ),
        m_frame( aParent ),
        m_libMgr( aLibMgr ),
        m_lastSyncHash( -1 )
{
}


TOOL_INTERACTIVE* SYMBOL_TREE_SYNCHRONIZING_ADAPTER::GetContextMenuTool()
{
    return m_frame->GetToolManager()->GetTool<SYMBOL_EDITOR_CONTROL>();
}


bool SYMBOL_TREE_SYNCHRONIZING_ADAPTER::IsContainer( const wxDataViewItem& aItem ) const
{
    const LIB_TREE_NODE* node = ToNode( aItem );
    return node ? node->m_Type == LIB_TREE_NODE::TYPE::LIBRARY : true;
}


#define PROGRESS_INTERVAL_MILLIS 120

void SYMBOL_TREE_SYNCHRONIZING_ADAPTER::Sync( const wxString& aForceRefresh,
                                              std::function<void( int, int, const wxString& )> aProgressCallback )
{
    wxLongLong nextUpdate = wxGetUTCTimeMillis() + (PROGRESS_INTERVAL_MILLIS / 2);

    m_lastSyncHash = m_libMgr->GetHash();
    int i = 0, max = GetLibrariesCount();

    // Process already stored libraries
    for( auto it = m_tree.m_Children.begin(); it != m_tree.m_Children.end(); )
    {
        const wxString& name = it->get()->m_Name;

        if( wxGetUTCTimeMillis() > nextUpdate )
        {
            aProgressCallback( i, max, name );
            nextUpdate = wxGetUTCTimeMillis() + PROGRESS_INTERVAL_MILLIS;
        }

        // TODO(JE) library tables
#if 0
        // There is a bug in SYMBOL_LIBRARY_MANAGER::LibraryExists() that uses the buffered
        // modified libraries before the symbol library table which prevents the library from
        // being removed from the tree control.
        if( !m_libMgr->LibraryExists( name, true )
          || !PROJECT_SCH::SchSymbolLibTable( &m_frame->Prj() )->HasLibrary( name, true )
          || PROJECT_SCH::SchSymbolLibTable( &m_frame->Prj() )->FindRow( name, true )
                   != PROJECT_SCH::SchSymbolLibTable( &m_frame->Prj() )->FindRow( name, false )
          || name == aForceRefresh )
        {
            it = deleteLibrary( it );
            continue;
        }
        else
#endif
        {
            updateLibrary( *(LIB_TREE_NODE_LIBRARY*) it->get() );
        }

        ++it;
        ++i;
    }

    // Look for new libraries
    //
    COMMON_SETTINGS* cfg = Pgm().GetCommonSettings();
    PROJECT_FILE&    project = m_frame->Prj().GetProjectFile();

    for( const wxString& libName : m_libMgr->GetLibraryNames() )
    {
        if( m_libHashes.count( libName ) == 0 )
        {
            if( wxGetUTCTimeMillis() > nextUpdate )
            {
                aProgressCallback( i++, max, libName );
                nextUpdate = wxGetUTCTimeMillis() + PROGRESS_INTERVAL_MILLIS;
            }

            SYMBOL_LIB_TABLE_ROW* library = m_libMgr->GetLibrary( libName );
            bool pinned = alg::contains( cfg->m_Session.pinned_symbol_libs, libName )
                            || alg::contains( project.m_PinnedSymbolLibs, libName );

            LIB_TREE_NODE_LIBRARY& lib_node = DoAddLibraryNode( libName, library->GetDescr(),
                                                                pinned );

            updateLibrary( lib_node );
        }
    }

    m_tree.AssignIntrinsicRanks( m_shownColumns );
}


int SYMBOL_TREE_SYNCHRONIZING_ADAPTER::GetLibrariesCount() const
{
    int count = LIB_TREE_MODEL_ADAPTER::GetLibrariesCount();

    for( const wxString& libName : m_libMgr->GetLibraryNames() )
    {
        if( m_libHashes.count( libName ) == 0 )
            ++count;
    }

    return count;
}


void SYMBOL_TREE_SYNCHRONIZING_ADAPTER::updateLibrary( LIB_TREE_NODE_LIBRARY& aLibNode )
{
    auto hashIt = m_libHashes.find( aLibNode.m_Name );

    if( hashIt == m_libHashes.end() )
    {
        // add a new library
        for( LIB_SYMBOL* symbol : m_libMgr->EnumerateSymbols( aLibNode.m_Name ) )
            aLibNode.AddItem( symbol );
    }
    else if( hashIt->second != m_libMgr->GetLibraryHash( aLibNode.m_Name ) )
    {
        // update an existing library
        std::list<LIB_SYMBOL*> symbols = m_libMgr->EnumerateSymbols( aLibNode.m_Name );

        // remove the common part from the aliases list
        for( auto nodeIt = aLibNode.m_Children.begin(); nodeIt != aLibNode.m_Children.end(); /**/ )
        {
            auto symbolIt = std::find_if( symbols.begin(), symbols.end(),
                    [&] ( const LIB_SYMBOL* a )
                    {
                        return a->GetName() == (*nodeIt)->m_LibId.GetLibItemName().wx_str();
                    } );

            if( symbolIt != symbols.end() )
            {
                // alias exists both in the symbol tree and the library manager,
                // update only the node data.
                static_cast<LIB_TREE_NODE_ITEM*>( nodeIt->get() )->Update( *symbolIt );
                symbols.erase( symbolIt );
                ++nodeIt;
            }
            else
            {
                // node does not exist in the library manager, remove the corresponding node
                nodeIt = aLibNode.m_Children.erase( nodeIt );
            }
        }

        // now the aliases list contains only new aliases that need to be added to the tree
        for( LIB_SYMBOL* symbol : symbols )
            aLibNode.AddItem( symbol );
    }

    aLibNode.AssignIntrinsicRanks( m_shownColumns );
    m_libHashes[aLibNode.m_Name] = m_libMgr->GetLibraryHash( aLibNode.m_Name );
}


LIB_TREE_NODE::PTR_VECTOR::iterator
SYMBOL_TREE_SYNCHRONIZING_ADAPTER::deleteLibrary( LIB_TREE_NODE::PTR_VECTOR::iterator& aLibNodeIt )
{
    LIB_TREE_NODE* node = aLibNodeIt->get();
    m_libHashes.erase( node->m_Name );
    return m_tree.m_Children.erase( aLibNodeIt );
}


wxDataViewItem SYMBOL_TREE_SYNCHRONIZING_ADAPTER::GetCurrentDataViewItem()
{
    if( m_frame->GetCurSymbol() )
        return FindItem( m_frame->GetCurSymbol()->GetLibId() );

    return wxDataViewItem();
}


void SYMBOL_TREE_SYNCHRONIZING_ADAPTER::GetValue( wxVariant& aVariant, wxDataViewItem const& aItem,
                                                  unsigned int aCol ) const
{
    if( IsFrozen() )
    {
        aVariant = wxEmptyString;
        return;
    }

    LIB_TREE_NODE* node = ToNode( aItem );
    wxASSERT( node );

    switch( aCol )
    {
    case NAME_COL:
        if( m_frame->GetCurSymbol() && m_frame->GetCurSymbol()->GetLibId() == node->m_LibId )
            node->m_Name = m_frame->GetCurSymbol()->GetLibId().GetUniStringLibItemName();

        if( node->m_Pinned )
            aVariant = GetPinningSymbol() + UnescapeString( node->m_Name );
        else
            aVariant = UnescapeString( node->m_Name );

        // mark modified items with an asterisk
        if( node->m_Type == LIB_TREE_NODE::TYPE::LIBRARY )
        {
            if( m_libMgr->IsLibraryModified( node->m_Name ) )
                aVariant = aVariant.GetString() + " *";
        }
        else if( node->m_Type == LIB_TREE_NODE::TYPE::ITEM )
        {
            if( m_libMgr->IsSymbolModified( node->m_Name, node->m_Parent->m_Name ) )
                aVariant = aVariant.GetString() + " *";
        }

        break;

    default:
        if( m_colIdxMap.count( aCol ) )
        {
            if( node->m_Type == LIB_TREE_NODE::TYPE::LIBRARY )
            {
                LIB_SYMBOL_LIBRARY_MANAGER& libMgr = m_frame->GetLibManager();
                SYMBOL_LIB_TABLE_ROW*   lib = libMgr.GetLibrary( node->m_LibId.GetLibNickname() );

                if( lib )
                    node->m_Desc = lib->GetDescr();

                if( !m_libMgr->IsLibraryLoaded( node->m_Name ) )
                    aVariant = _( "(failed to load)" ) + wxS( " " ) + aVariant.GetString();
                else if( m_libMgr->IsLibraryReadOnly( node->m_Name ) )
                    aVariant = _( "(read-only)" ) + wxS( " " ) + aVariant.GetString();
            }

            const wxString& key = m_colIdxMap.at( aCol );

            if( m_frame->GetCurSymbol() && m_frame->GetCurSymbol()->GetLibId() == node->m_LibId )
            {
                node->m_Desc = m_frame->GetCurSymbol()->GetDescription();
            }

            wxString valueStr;

            if( key == wxT( "Description" ) )
                valueStr = node->m_Desc;
            else if( node->m_Fields.count( key ) )
                valueStr = node->m_Fields.at( key );
            else
                valueStr = wxEmptyString;

            valueStr.Replace( wxS( "\n" ), wxS( " " ) ); // Clear line breaks

            if( !aVariant.GetString().IsEmpty() )
            {
                if( !valueStr.IsEmpty() )
                    aVariant = valueStr + wxS( " - " ) + aVariant.GetString();
            }
            else
            {
                aVariant = valueStr;
            }
        }
        break;
    }
}


bool SYMBOL_TREE_SYNCHRONIZING_ADAPTER::GetAttr( wxDataViewItem const& aItem, unsigned int aCol,
                                                 wxDataViewItemAttr& aAttr ) const
{
    if( IsFrozen() )
        return false;

    LIB_TREE_NODE* node = ToNode( aItem );
    wxCHECK( node, false );

    // Mark both columns of unloaded libraries using grey text color (to look disabled)
    if( node->m_Type == LIB_TREE_NODE::TYPE::LIBRARY && !m_libMgr->IsLibraryLoaded( node->m_Name ) )
    {
        aAttr.SetColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT  ) );
        return true;
    }

    // The remaining attributes are only for the name column
    if( aCol != NAME_COL )
        return false;

    LIB_SYMBOL* curSymbol = m_frame->GetCurSymbol();

    switch( node->m_Type )
    {
    case LIB_TREE_NODE::TYPE::LIBRARY:
        // mark modified libs with bold font
        aAttr.SetBold( m_libMgr->IsLibraryModified( node->m_Name ) );

        // mark the current library if it's collapsed
        if( curSymbol && curSymbol->GetLibId().GetLibNickname() == node->m_LibId.GetLibNickname() )
        {
            if( !m_widget->IsExpanded( ToItem( node ) ) )
            {
                aAttr.SetStrikethrough( true );   // LIB_TREE_RENDERER uses strikethrough as a
                                                  // proxy for "is canvas item"
            }
        }

        break;

    case LIB_TREE_NODE::TYPE::ITEM:
        // mark modified part with bold font
        aAttr.SetBold( m_libMgr->IsSymbolModified( node->m_Name, node->m_Parent->m_Name ) );

        // mark aliases with italic font
        aAttr.SetItalic( !node->m_IsRoot );

        // mark the current (on-canvas) part
        if( curSymbol && curSymbol->GetLibId() == node->m_LibId )
        {
            aAttr.SetStrikethrough( true );   // LIB_TREE_RENDERER uses strikethrough as a
                                              // proxy for "is canvas item"
        }

        break;

    default:
        return false;
    }

    return true;
}


bool SYMBOL_TREE_SYNCHRONIZING_ADAPTER::HasPreview( const wxDataViewItem& aItem )
{
    LIB_TREE_NODE* node = ToNode( aItem );
    wxCHECK( node, false );

    return node->m_Type == LIB_TREE_NODE::TYPE::ITEM && node->m_LibId != m_frame->GetTargetLibId();
}


static const wxString c_previewName = wxS( "symHoverPreview" );


void SYMBOL_TREE_SYNCHRONIZING_ADAPTER::ShowPreview( wxWindow*             aParent,
                                                     const wxDataViewItem& aItem )
{
    LIB_TREE_NODE* node = ToNode( aItem );
    wxCHECK( node, /* void */ );

    SYMBOL_PREVIEW_WIDGET* preview = dynamic_cast<SYMBOL_PREVIEW_WIDGET*>(
            wxWindow::FindWindowByName( c_previewName, aParent ) );

    if( !preview )
    {
        wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );
        aParent->SetSizer( mainSizer );

        WX_PANEL* panel = new WX_PANEL( aParent );
        panel->SetBorders( true, true, true, true );
        panel->SetBorderColor( KIGFX::COLOR4D::BLACK );

        wxBoxSizer* panelSizer = new wxBoxSizer( wxVERTICAL );
        panel->SetSizer( panelSizer );

        EDA_DRAW_PANEL_GAL::GAL_TYPE backend = m_frame->GetCanvas()->GetBackend();
        preview = new SYMBOL_PREVIEW_WIDGET( panel, &m_frame->Kiway(), false, backend );
        preview->SetName( c_previewName );
        preview->SetLayoutDirection( wxLayout_LeftToRight );

        panelSizer->Add( preview, 1, wxEXPAND | wxALL, 1 );
        mainSizer->Add( panel, 1, wxEXPAND, 0 );
        aParent->Layout();
    }

    preview->DisplaySymbol( node->m_LibId, node->m_Unit );
}


void SYMBOL_TREE_SYNCHRONIZING_ADAPTER::ShutdownPreview( wxWindow* aParent )
{
    wxWindow* previewWindow = wxWindow::FindWindowByName( c_previewName, aParent );

    if( SYMBOL_PREVIEW_WIDGET* preview = dynamic_cast<SYMBOL_PREVIEW_WIDGET*>( previewWindow ) )
    {
        preview->GetCanvas()->SetEvtHandlerEnabled( false );
        preview->GetCanvas()->StopDrawing();
    }
}

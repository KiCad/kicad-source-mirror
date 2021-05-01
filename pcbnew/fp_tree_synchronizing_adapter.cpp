/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fp_tree_synchronizing_adapter.h>
#include <footprint_edit_frame.h>
#include <fp_lib_table.h>
#include <footprint_info_impl.h>
#include <kicad_string.h>
#include <board.h>
#include <footprint.h>
#include <tool/tool_manager.h>
#include <tools/footprint_editor_control.h>
#include <wx/settings.h>


wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER>
FP_TREE_SYNCHRONIZING_ADAPTER::Create( FOOTPRINT_EDIT_FRAME* aFrame, FP_LIB_TABLE* aLibs )
{
    auto* adapter = new FP_TREE_SYNCHRONIZING_ADAPTER( aFrame, aLibs );
    return wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER>( adapter );
}


FP_TREE_SYNCHRONIZING_ADAPTER::FP_TREE_SYNCHRONIZING_ADAPTER( FOOTPRINT_EDIT_FRAME* aFrame,
                                                              FP_LIB_TABLE* aLibs ) :
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
    return node ? node->m_Type == LIB_TREE_NODE::LIB : true;
}


#define PROGRESS_INTERVAL_MILLIS 66

void FP_TREE_SYNCHRONIZING_ADAPTER::Sync()
{
    // Process already stored libraries
    for( auto it = m_tree.m_Children.begin(); it != m_tree.m_Children.end(); )
    {
        const wxString& name = it->get()->m_Name;

        // Remove the library if it no longer exists or it exists in both the global and the
        // project library but the project library entry is disabled.
        if( !m_libs->HasLibrary( name, true )
          || ( m_libs->FindRow( name, true ) != m_libs->FindRow( name, false ) ) )
        {
            it = deleteLibrary( it );
            continue;
        }

        updateLibrary( *(LIB_TREE_NODE_LIB*) it->get() );
        ++it;
    }

    // Look for new libraries
    size_t count = m_libMap.size();

    for( const auto& libName : m_libs->GetLogicalLibs() )
    {
        if( m_libMap.count( libName ) == 0 )
        {
            const FP_LIB_TABLE_ROW* library = m_libs->FindRow( libName, true );

            DoAddLibrary( libName, library->GetDescr(), getFootprints( libName ), true );
            m_libMap.insert( libName  );
        }
    }

    if( m_libMap.size() > count )
        m_tree.AssignIntrinsicRanks();
}


int FP_TREE_SYNCHRONIZING_ADAPTER::GetLibrariesCount() const
{
    return GFootprintTable.GetCount();
}


void FP_TREE_SYNCHRONIZING_ADAPTER::updateLibrary( LIB_TREE_NODE_LIB& aLibNode )
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
            static_cast<LIB_TREE_NODE_LIB_ID*>( nodeIt->get() )->Update( *footprintIt );
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
    for( auto footprint : footprints )
        aLibNode.AddItem( footprint );

    aLibNode.AssignIntrinsicRanks();
    m_libMap.insert( aLibNode.m_Name );
}


LIB_TREE_NODE::PTR_VECTOR::iterator FP_TREE_SYNCHRONIZING_ADAPTER::deleteLibrary(
            LIB_TREE_NODE::PTR_VECTOR::iterator& aLibNodeIt )
{
    LIB_TREE_NODE* node = aLibNodeIt->get();
    m_libMap.erase( node->m_Name );
    auto it = m_tree.m_Children.erase( aLibNodeIt );
    return it;
}


void FP_TREE_SYNCHRONIZING_ADAPTER::GetValue( wxVariant& aVariant, wxDataViewItem const& aItem,
                                              unsigned int aCol ) const
{
    if( IsFrozen() )
    {
        aVariant = wxEmptyString;
        return;
    }

    auto node = ToNode( aItem );

    switch( aCol )
    {
    case 0:
        if( node->m_LibId == m_frame->GetLoadedFPID() && !m_frame->IsCurrentFPFromBoard() )
        {
            node->m_Name = m_frame->GetLoadedFPID().GetLibItemName();

            // mark modified part with an asterisk
            if( m_frame->GetScreen()->IsModify() )
                aVariant = node->m_Name + " *";
            else
                aVariant = node->m_Name;
        }
        else if( node->m_Pinned )
            aVariant = GetPinningSymbol() + node->m_Name;
        else
            aVariant = node->m_Name;
        break;

    case 1:
        if( node->m_LibId == m_frame->GetLoadedFPID() && !m_frame->IsCurrentFPFromBoard() )
        {
            node->m_Desc = m_frame->GetBoard()->GetFirstFootprint()->GetDescription();
        }
        else if( node->m_Type == LIB_TREE_NODE::LIB )
        {
            try
            {
                const FP_LIB_TABLE_ROW* lib =
                        GFootprintTable.FindRow( node->m_LibId.GetLibNickname() );

                if( lib )
                    node->m_Desc = lib->GetDescr();
            }
            catch( IO_ERROR& )
            {
            }
        }

        aVariant = node->m_Desc;
        break;

    default:    // column == -1 is used for default Compare function
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

    auto node = ToNode( aItem );
    wxCHECK( node, false );

    switch( node->m_Type )
    {
    case LIB_TREE_NODE::LIB:
        if( node->m_Name == m_frame->GetLoadedFPID().GetLibNickname() )
        {
#ifdef __WXGTK__
            // The native wxGTK+ impl ignores background colour, so set the text colour
            // instead.  Works reasonably well in dark themes, less well in light ones....
            aAttr.SetColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
#else
            aAttr.SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
            aAttr.SetColour( wxSystemSettings::GetColour( wxSYS_COLOUR_LISTBOXHIGHLIGHTTEXT  ) );
#endif

            // mark modified libs with bold font
            if( m_frame->GetScreen()->IsModify() && !m_frame->IsCurrentFPFromBoard() )
                aAttr.SetBold( true );
        }
        break;

    case LIB_TREE_NODE::LIBID:
        if( node->m_LibId == m_frame->GetLoadedFPID() )
        {
#ifdef __WXGTK__
            // The native wxGTK+ impl ignores background colour, so set the text colour
            // instead.  Works reasonably well in dark themes, less well in light ones....
            aAttr.SetColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
#else
            aAttr.SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
            aAttr.SetColour( wxSystemSettings::GetColour( wxSYS_COLOUR_LISTBOXHIGHLIGHTTEXT  ) );
#endif

            // mark modified part with bold font
            if( m_frame->GetScreen()->IsModify() && !m_frame->IsCurrentFPFromBoard() )
                aAttr.SetBold( true );
        }
        break;

    default:
        return false;
    }

    return true;
}



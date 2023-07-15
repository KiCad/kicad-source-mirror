/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright (C) 2014-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <eda_base_frame.h>
#include <eda_pattern_match.h>
#include <kiface_base.h>
#include <lib_tree_model_adapter.h>
#include <project/project_file.h>
#include <settings/app_settings.h>
#include <widgets/ui_common.h>
#include <wx/tokenzr.h>
#include <wx/wupdlock.h>
#include <string_utils.h>


static const int kDataViewIndent = 20;


wxDataViewItem LIB_TREE_MODEL_ADAPTER::ToItem( const LIB_TREE_NODE* aNode )
{
    return wxDataViewItem( const_cast<void*>( static_cast<void const*>( aNode ) ) );
}


LIB_TREE_NODE* LIB_TREE_MODEL_ADAPTER::ToNode( wxDataViewItem aItem )
{
    return static_cast<LIB_TREE_NODE*>( aItem.GetID() );
}


LIB_TREE_MODEL_ADAPTER::LIB_TREE_MODEL_ADAPTER( EDA_BASE_FRAME* aParent,
                                                const wxString& aPinnedKey ) :
        m_parent( aParent ),
        m_filter( SYM_FILTER_NONE ),
        m_sort_mode( BEST_MATCH ),
        m_show_units( true ),
        m_preselect_unit( 0 ),
        m_freeze( 0 ),
        m_widget( nullptr )
{
    // Default column widths.  Do not translate these names.
    m_colWidths[ _HKI( "Item" ) ] = 300;
    m_colWidths[ _HKI( "Description" ) ] = 600;

    m_availableColumns = { _HKI( "Item" ), _HKI( "Description" ) };

    APP_SETTINGS_BASE* cfg = Kiface().KifaceSettings();

    for( const std::pair<const wxString, int>& pair : cfg->m_LibTree.column_widths )
        m_colWidths[pair.first] = pair.second;

    m_shownColumns = cfg->m_LibTree.columns;

    if( m_shownColumns.empty() )
        m_shownColumns = {  _HKI( "Item" ), _HKI( "Description" ) };

    if( m_shownColumns[0] != _HKI( "Item" ) )
        m_shownColumns.insert( m_shownColumns.begin(), _HKI( "Item" ) );
}


LIB_TREE_MODEL_ADAPTER::~LIB_TREE_MODEL_ADAPTER()
{}


void LIB_TREE_MODEL_ADAPTER::SaveSettings()
{
    if( m_widget )
    {
        APP_SETTINGS_BASE* cfg = Kiface().KifaceSettings();

        cfg->m_LibTree.columns = GetShownColumns();
        cfg->m_LibTree.column_widths.clear();

        for( const std::pair<const wxString, wxDataViewColumn*>& pair : m_colNameMap )
            cfg->m_LibTree.column_widths[pair.first] = pair.second->GetWidth();
    }
}


void LIB_TREE_MODEL_ADAPTER::SetFilter( SYM_FILTER_TYPE aFilter )
{
    m_filter = aFilter;
}


void LIB_TREE_MODEL_ADAPTER::ShowUnits( bool aShow )
{
    m_show_units = aShow;
}


void LIB_TREE_MODEL_ADAPTER::SetPreselectNode( const LIB_ID& aLibId, int aUnit )
{
    m_preselect_lib_id = aLibId;
    m_preselect_unit = aUnit;
}


LIB_TREE_NODE_LIB& LIB_TREE_MODEL_ADAPTER::DoAddLibraryNode( const wxString& aNodeName,
                                                             const wxString& aDesc, bool pinned )
{
    LIB_TREE_NODE_LIB& lib_node = m_tree.AddLib( aNodeName, aDesc );

    lib_node.m_Pinned = pinned;

    return lib_node;
}


void LIB_TREE_MODEL_ADAPTER::DoAddLibrary( const wxString& aNodeName, const wxString& aDesc,
                                           const std::vector<LIB_TREE_ITEM*>& aItemList,
                                           bool pinned, bool presorted )
{
    LIB_TREE_NODE_LIB& lib_node = DoAddLibraryNode( aNodeName, aDesc, pinned );

    for( LIB_TREE_ITEM* item: aItemList )
        lib_node.AddItem( item );

    lib_node.AssignIntrinsicRanks( presorted );
}


void LIB_TREE_MODEL_ADAPTER::UpdateSearchString( const wxString& aSearch, bool aState )
{
    {
        wxWindowUpdateLocker updateLock( m_widget );

        // Even with the updateLock, wxWidgets sometimes ties its knickers in a knot trying to
        // run a wxdataview_selection_changed_callback() on a row that has been deleted.
        // https://bugs.launchpad.net/kicad/+bug/1756255
        m_widget->UnselectAll();

        // This collapse is required before the call to "Freeze()" below.  Once Freeze()
        // is called, GetParent() will return nullptr.  While this works for some calls, it
        // segfaults when we have any expanded elements b/c the sub units in the tree don't
        // have explicit references that are maintained over a search
        // The tree will be expanded again below when we get our matches
        //
        // Also note that this cannot happen when we have deleted a symbol as GTK will also
        // iterate over the tree in this case and find a symbol that has an invalid link
        // and crash https://gitlab.com/kicad/code/kicad/-/issues/6910
        if( !aState && !aSearch.IsNull() && m_tree.m_Children.size() )
        {
            for( std::unique_ptr<LIB_TREE_NODE>& child: m_tree.m_Children )
                m_widget->Collapse( wxDataViewItem( &*child ) );
        }

        // DO NOT REMOVE THE FREEZE/THAW. This freeze/thaw is a flag for this model adapter
        // that tells it when it shouldn't trust any of the data in the model. When set, it will
        // not return invalid data to the UI, since this invalid data can cause crashes.
        // This is different than the update locker, which locks the UI aspects only.
        Freeze();
        BeforeReset();

        m_tree.ResetScore();

        wxStringTokenizer tokenizer( aSearch );

        while( tokenizer.HasMoreTokens() )
        {
            wxString lib;
            wxString term = tokenizer.GetNextToken().Lower();

            if( term.Contains( ":" ) )
            {
                lib = term.BeforeFirst( ':' );
                term = term.AfterFirst( ':' );
            }

            EDA_COMBINED_MATCHER matcher( term, CTX_LIBITEM );

            m_tree.UpdateScore( matcher, lib );
        }

        m_tree.SortNodes( m_sort_mode == BEST_MATCH );
        AfterReset();
        Thaw();
    }

    const LIB_TREE_NODE* firstMatch = ShowResults();

    if( firstMatch )
    {
        wxDataViewItem item = ToItem( firstMatch );
        m_widget->Select( item );

        // Make sure the *parent* item is visible. The selected item is the first (shown) child
        // of the parent. So it's always right below the parent, and this way the user can also
        // see what library the selected part belongs to, without having a case where the selection
        // is off the screen (unless the window is a single row high, which is unlikely).
        //
        // This also happens to circumvent https://bugs.launchpad.net/kicad/+bug/1804400 which
        // appears to be a GTK+3 bug.
        {
            wxDataViewItem parent = GetParent( item );

            if( parent.IsOk() )
                m_widget->EnsureVisible( parent );
        }

        m_widget->EnsureVisible( item );
    }
}


void LIB_TREE_MODEL_ADAPTER::AttachTo( wxDataViewCtrl* aDataViewCtrl )
{
    m_widget = aDataViewCtrl;
    aDataViewCtrl->SetIndent( kDataViewIndent );
    aDataViewCtrl->AssociateModel( this );
    recreateColumns();
}


void LIB_TREE_MODEL_ADAPTER::recreateColumns()
{
    m_widget->ClearColumns();

    m_columns.clear();
    m_colIdxMap.clear();
    m_colNameMap.clear();

    // The Item column is always shown
    doAddColumn( wxT( "Item" ) );

    for( const wxString& colName : m_shownColumns )
    {
        if( !m_colNameMap.count( colName ) )
            doAddColumn( colName, colName == wxT( "Description" ) );
    }
}


void LIB_TREE_MODEL_ADAPTER::resortTree()
{
    Freeze();
    BeforeReset();

    m_tree.SortNodes( m_sort_mode == BEST_MATCH );

    AfterReset();
    Thaw();
}


void LIB_TREE_MODEL_ADAPTER::PinLibrary( LIB_TREE_NODE* aTreeNode )
{
    m_parent->Prj().PinLibrary( aTreeNode->m_LibId.GetLibNickname(), isSymbolModel() );
    aTreeNode->m_Pinned = true;

    resortTree();
    m_widget->EnsureVisible( ToItem( aTreeNode ) );
}


void LIB_TREE_MODEL_ADAPTER::UnpinLibrary( LIB_TREE_NODE* aTreeNode )
{
    m_parent->Prj().UnpinLibrary( aTreeNode->m_LibId.GetLibNickname(), isSymbolModel() );
    aTreeNode->m_Pinned = false;

    resortTree();
    // Keep focus at top when unpinning
}


wxDataViewColumn* LIB_TREE_MODEL_ADAPTER::doAddColumn( const wxString& aHeader, bool aTranslate )
{
    wxString translatedHeader = aTranslate ? wxGetTranslation( aHeader ) : aHeader;

    // The extent of the text doesn't take into account the space on either side
    // in the header, so artificially pad it
    wxSize headerMinWidth = KIUI::GetTextSize( translatedHeader + wxT( "MMM" ), m_widget );

    if( !m_colWidths.count( aHeader ) || m_colWidths[aHeader] < headerMinWidth.x )
        m_colWidths[aHeader] = headerMinWidth.x;

    int index = m_columns.size();

    wxDataViewColumn* ret = m_widget->AppendTextColumn( translatedHeader, index,
                                                        wxDATAVIEW_CELL_INERT,
                                                        m_colWidths[aHeader] );
    ret->SetMinWidth( headerMinWidth.x );

    m_columns.emplace_back( ret );
    m_colNameMap[aHeader] = ret;
    m_colIdxMap[m_columns.size() - 1] = aHeader;

    return ret;
}


void LIB_TREE_MODEL_ADAPTER::addColumnIfNecessary( const wxString& aHeader )
{
    if( m_colNameMap.count( aHeader ) )
        return;

    // Columns will be created later
    m_colNameMap[aHeader] = nullptr;
    m_availableColumns.emplace_back( aHeader );
}


void LIB_TREE_MODEL_ADAPTER::SetShownColumns( const std::vector<wxString>& aColumnNames )
{
    bool recreate = m_shownColumns != aColumnNames;

    m_shownColumns = aColumnNames;

    if( recreate && m_widget )
        recreateColumns();
}


LIB_ID LIB_TREE_MODEL_ADAPTER::GetAliasFor( const wxDataViewItem& aSelection ) const
{
    const LIB_TREE_NODE* node = ToNode( aSelection );
    return node ? node->m_LibId : LIB_ID();
}


int LIB_TREE_MODEL_ADAPTER::GetUnitFor( const wxDataViewItem& aSelection ) const
{
    const LIB_TREE_NODE* node = ToNode( aSelection );
    return node ? node->m_Unit : 0;
}


LIB_TREE_NODE::TYPE LIB_TREE_MODEL_ADAPTER::GetTypeFor( const wxDataViewItem& aSelection ) const
{
    const LIB_TREE_NODE* node = ToNode( aSelection );
    return node ? node->m_Type : LIB_TREE_NODE::INVALID;
}


LIB_TREE_NODE* LIB_TREE_MODEL_ADAPTER::GetTreeNodeFor( const wxDataViewItem& aSelection ) const
{
    return ToNode( aSelection );
}


int LIB_TREE_MODEL_ADAPTER::GetItemCount() const
{
    int n = 0;

    for( const std::unique_ptr<LIB_TREE_NODE>& lib: m_tree.m_Children )
        n += lib->m_Children.size();

    return n;
}


wxDataViewItem LIB_TREE_MODEL_ADAPTER::FindItem( const LIB_ID& aLibId )
{
    for( std::unique_ptr<LIB_TREE_NODE>& lib: m_tree.m_Children )
    {
        if( lib->m_Name != aLibId.GetLibNickname() )
            continue;

        // if part name is not specified, return the library node
        if( aLibId.GetLibItemName() == "" )
            return ToItem( lib.get() );

        for( std::unique_ptr<LIB_TREE_NODE>& alias: lib->m_Children )
        {
            if( alias->m_Name == aLibId.GetLibItemName() )
                return ToItem( alias.get() );
        }

        break;  // could not find the part in the requested library
    }

    return wxDataViewItem();
}


wxDataViewItem LIB_TREE_MODEL_ADAPTER::GetCurrentDataViewItem()
{
    return FindItem( m_preselect_lib_id );
}


unsigned int LIB_TREE_MODEL_ADAPTER::GetChildren( const wxDataViewItem&   aItem,
                                                  wxDataViewItemArray&    aChildren ) const
{
    const LIB_TREE_NODE* node = ( aItem.IsOk() ? ToNode( aItem ) : &m_tree );
    unsigned int         count = 0;

    if( node->m_Type == LIB_TREE_NODE::TYPE::ROOT
            || node->m_Type == LIB_TREE_NODE::LIB
            || ( m_show_units && node->m_Type == LIB_TREE_NODE::TYPE::LIBID ) )
    {
        for( std::unique_ptr<LIB_TREE_NODE> const& child: node->m_Children )
        {
            if( child->m_Score > 0 )
            {
                aChildren.Add( ToItem( &*child ) );
                ++count;
            }
        }
    }

    return count;
}


void LIB_TREE_MODEL_ADAPTER::FinishTreeInitialization()
{
    wxDataViewColumn* col        = nullptr;
    size_t            idx        = 0;
    int               totalWidth = 0;
    wxString          header;

    for( ; idx < m_columns.size() - 1; idx++ )
    {
        wxASSERT( m_colIdxMap.count( idx ) );
    
        col    = m_columns[idx];
        header = m_colIdxMap[idx];

        wxASSERT( m_colWidths.count( header ) );

        col->SetWidth( m_colWidths[header] );
        totalWidth += col->GetWidth();
    }

    int remainingWidth = m_widget->GetSize().x - totalWidth;
    header = m_columns[idx]->GetTitle();

    m_columns[idx]->SetWidth( std::max( m_colWidths[header], remainingWidth ) );
}


void LIB_TREE_MODEL_ADAPTER::RefreshTree()
{
    // Yes, this is an enormous hack.  But it works on all platforms, it doesn't suffer
    // the On^2 sorting issues that ItemChanged() does on OSX, and it doesn't lose the
    // user's scroll position (which re-attaching or deleting/re-inserting columns does).
    static int walk = 1;

    std::vector<int> widths;

    for( const wxDataViewColumn* col : m_columns )
        widths.emplace_back( col->GetWidth() );

    wxASSERT( widths.size() );

    // Only use the widths read back if they are non-zero.
    // GTK returns the displayed width of the column, which is not calculated immediately
    if( widths[0] > 0 )
    {
        size_t i = 0;

        for( const auto& [ colName, colPtr ] : m_colNameMap )
            m_colWidths[ colName ] = widths[i++];
    }

    auto colIt = m_colWidths.begin();

    colIt->second += walk;
    colIt++;

    if( colIt != m_colWidths.end() )
        colIt->second -= walk;

    for( const auto& [ colName, colPtr ] : m_colNameMap )
    {
        if( colPtr == m_columns[0] )
            continue;

        wxASSERT( m_colWidths.count( colName ) );
        colPtr->SetWidth( m_colWidths[ colName ] );
    }

    walk = -walk;
}


bool LIB_TREE_MODEL_ADAPTER::HasContainerColumns( const wxDataViewItem& aItem ) const
{
    return IsContainer( aItem );
}


bool LIB_TREE_MODEL_ADAPTER::IsContainer( const wxDataViewItem& aItem ) const
{
    LIB_TREE_NODE* node = ToNode( aItem );
    return node ? node->m_Children.size() : true;
}


wxDataViewItem LIB_TREE_MODEL_ADAPTER::GetParent( const wxDataViewItem& aItem ) const
{
    if( m_freeze )
        return ToItem( nullptr );

    LIB_TREE_NODE* node   = ToNode( aItem );
    LIB_TREE_NODE* parent = node ? node->m_Parent : nullptr;

    // wxDataViewModel has no root node, but rather top-level elements have
    // an invalid (null) parent.
    if( !node || !parent || parent->m_Type == LIB_TREE_NODE::TYPE::ROOT )
        return ToItem( nullptr );
    else
        return ToItem( parent );
}


void LIB_TREE_MODEL_ADAPTER::GetValue( wxVariant&              aVariant,
                                       const wxDataViewItem&   aItem,
                                       unsigned int            aCol ) const
{
    if( IsFrozen() )
    {
        aVariant = wxEmptyString;
        return;
    }

    LIB_TREE_NODE* node = ToNode( aItem );
    wxCHECK( node, /* void */ );

    switch( aCol )
    {
    case NAME_COL:
        if( node->m_Pinned )
            aVariant = GetPinningSymbol() + UnescapeString( node->m_Name );
        else
            aVariant = UnescapeString( node->m_Name );

        break;

    default:
        if( m_colIdxMap.count( aCol ) )
        {
            const wxString& key = m_colIdxMap.at( aCol );

            if( node->m_Fields.count( key ) )
                aVariant = node->m_Fields.at( key );
            else if( key == wxT( "Description" ) )
                aVariant = node->m_Desc;
            else
                aVariant = wxEmptyString;
        }

        break;
    }
}


bool LIB_TREE_MODEL_ADAPTER::GetAttr( const wxDataViewItem&   aItem,
                                      unsigned int            aCol,
                                      wxDataViewItemAttr&     aAttr ) const
{
    if( IsFrozen() )
        return false;

    LIB_TREE_NODE* node = ToNode( aItem );
    wxCHECK( node, false );

    if( node->m_Type == LIB_TREE_NODE::LIBID )
    {
        if( !node->m_IsRoot && aCol == 0 )
        {
            // Names of non-root aliases are italicized
            aAttr.SetItalic( true );
            return true;
        }
    }

    return false;
}


void recursiveDescent( LIB_TREE_NODE& aNode, const std::function<bool( const LIB_TREE_NODE* )>& f )
{
    for( std::unique_ptr<LIB_TREE_NODE>& node: aNode.m_Children )
    {
        if( !f( node.get() ) )
            break;

        recursiveDescent( *node, f );
    }
}


const LIB_TREE_NODE* LIB_TREE_MODEL_ADAPTER::ShowResults()
{
    const LIB_TREE_NODE* firstMatch = nullptr;

    // Expand parents of leaf nodes with some level of matching
    recursiveDescent( m_tree,
            [&]( const LIB_TREE_NODE* n )
            {
                if( n->m_Type == LIB_TREE_NODE::TYPE::LIBID && n->m_Score > 1 )
                {
                    if( !firstMatch )
                        firstMatch = n;
                    else if( n->m_Score > firstMatch->m_Score )
                        firstMatch = n;

                    m_widget->ExpandAncestors( ToItem( n ) );
                }

                return true;    // keep going to expand ancestors of all found items
            } );

    // If no matches, find and show the preselect node
    if( !firstMatch && m_preselect_lib_id.IsValid() )
    {
        recursiveDescent( m_tree,
                [&]( const LIB_TREE_NODE* n )
                {
                    if( n->m_Type == LIB_TREE_NODE::LIBID
                              && ( n->m_Children.empty() || !m_preselect_unit )
                              && m_preselect_lib_id == n->m_LibId )
                    {
                        firstMatch = n;
                        m_widget->ExpandAncestors( ToItem( n ) );
                        return false;
                    }
                    else if( n->m_Type == LIB_TREE_NODE::UNIT
                              && ( m_preselect_unit && m_preselect_unit == n->m_Unit )
                              && m_preselect_lib_id == n->m_Parent->m_LibId )
                    {
                        firstMatch = n;
                        m_widget->ExpandAncestors( ToItem( n ) );
                        return false;
                    }

                    return true;
                } );
    }

    // If still no matches expand a single library if there is only one
    if( !firstMatch )
    {
        recursiveDescent( m_tree,
                [&]( const LIB_TREE_NODE* n )
                {
                    if( n->m_Type == LIB_TREE_NODE::TYPE::LIBID
                            && n->m_Parent->m_Parent->m_Children.size() == 1 )
                    {
                        firstMatch = n;
                        m_widget->ExpandAncestors( ToItem( n ) );
                        return false;
                    }

                    return true;
                } );
    }

    return firstMatch;
}



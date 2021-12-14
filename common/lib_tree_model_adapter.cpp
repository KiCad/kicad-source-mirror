/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright (C) 2014-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <config_params.h>
#include <lib_tree_model_adapter.h>
#include <project/project_file.h>
#include <settings/app_settings.h>
#include <widgets/ui_common.h>
#include <wx/tokenzr.h>
#include <wx/wupdlock.h>
#include <string_utils.h>


#define PINNED_ITEMS_KEY      wxT( "PinnedItems" )

static const int kDataViewIndent = 20;


wxDataViewItem LIB_TREE_MODEL_ADAPTER::ToItem( const LIB_TREE_NODE* aNode )
{
    return wxDataViewItem( const_cast<void*>( static_cast<void const*>( aNode ) ) );
}


LIB_TREE_NODE* LIB_TREE_MODEL_ADAPTER::ToNode( wxDataViewItem aItem )
{
    return static_cast<LIB_TREE_NODE*>( aItem.GetID() );
}


unsigned int LIB_TREE_MODEL_ADAPTER::IntoArray( const LIB_TREE_NODE& aNode,
                                                wxDataViewItemArray& aChildren )
{
    unsigned int n = 0;

    for( std::unique_ptr<LIB_TREE_NODE> const& child: aNode.m_Children )
    {
        if( child->m_Score > 0 )
        {
            aChildren.Add( ToItem( &*child ) );
            ++n;
        }
    }

    return n;
}


LIB_TREE_MODEL_ADAPTER::LIB_TREE_MODEL_ADAPTER( EDA_BASE_FRAME* aParent,
                                                const wxString& aPinnedKey ) :
        m_parent( aParent ),
        m_filter( SYM_FILTER_NONE ),
        m_show_units( true ),
        m_preselect_unit( 0 ),
        m_freeze( 0 ),
        m_col_part( nullptr ),
        m_col_desc( nullptr ),
        m_widget( nullptr ),
        m_pinnedLibs(),
        m_pinnedKey( aPinnedKey )
{
    // Default column widths
    m_colWidths[PART_COL] = 360;
    m_colWidths[DESC_COL] = 2000;

    APP_SETTINGS_BASE* cfg = Kiface().KifaceSettings();
    m_colWidths[PART_COL] = cfg->m_LibTree.column_width;

    // Read the pinned entries from the project config
    PROJECT_FILE& project = m_parent->Kiway().Prj().GetProjectFile();

    std::vector<wxString>& entries = ( m_pinnedKey == "pinned_symbol_libs" ) ?
                                             project.m_PinnedSymbolLibs :
                                             project.m_PinnedFootprintLibs;

    for( const wxString& entry : entries )
        m_pinnedLibs.push_back( entry );
}


LIB_TREE_MODEL_ADAPTER::~LIB_TREE_MODEL_ADAPTER()
{}


void LIB_TREE_MODEL_ADAPTER::SaveColWidths()
{
    if( m_widget )
    {
        APP_SETTINGS_BASE* cfg = Kiface().KifaceSettings();
        cfg->m_LibTree.column_width = m_widget->GetColumn( PART_COL )->GetWidth();
    }
}


void LIB_TREE_MODEL_ADAPTER::SavePinnedItems()
{
    PROJECT_FILE& project = m_parent->Kiway().Prj().GetProjectFile();

    std::vector<wxString>& entries = ( m_pinnedKey == "pinned_symbol_libs" ) ?
                                     project.m_PinnedSymbolLibs :
                                     project.m_PinnedFootprintLibs;

    entries.clear();
    m_pinnedLibs.clear();

    for( std::unique_ptr<LIB_TREE_NODE>& child: m_tree.m_Children )
    {
        if( child->m_Pinned )
        {
            m_pinnedLibs.push_back( child->m_LibId.GetLibNickname() );
            entries.push_back( child->m_LibId.GetLibNickname() );
        }
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
                                                             const wxString& aDesc )
{
    LIB_TREE_NODE_LIB& lib_node = m_tree.AddLib( aNodeName, aDesc );

    lib_node.m_Pinned = m_pinnedLibs.Index( lib_node.m_LibId.GetLibNickname() ) != wxNOT_FOUND;

    return lib_node;
}


void LIB_TREE_MODEL_ADAPTER::DoAddLibrary( const wxString& aNodeName, const wxString& aDesc,
                                           const std::vector<LIB_TREE_ITEM*>& aItemList,
                                           bool presorted )
{
    LIB_TREE_NODE_LIB& lib_node = DoAddLibraryNode( aNodeName, aDesc );

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

        for( std::unique_ptr<LIB_TREE_NODE>& child: m_tree.m_Children )
        {
            if( child->m_Pinned )
                child->m_Score *= 2;
        }

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

            EDA_COMBINED_MATCHER matcher( term );

            m_tree.UpdateScore( matcher, lib );
        }

        m_tree.SortNodes();
        AfterReset();
        Thaw();
    }

    LIB_TREE_NODE* bestMatch = ShowResults();

    if( !bestMatch )
        bestMatch = ShowPreselect();

    if( !bestMatch )
        bestMatch = ShowSingleLibrary();

    if( bestMatch )
    {
        wxDataViewItem item = wxDataViewItem( bestMatch );
        m_widget->Select( item );

        // Make sure the *parent* item is visible. The selected item is the
        // first (shown) child of the parent. So it's always right below the parent,
        // and this way the user can also see what library the selected part belongs to,
        // without having a case where the selection is off the screen (unless the
        // window is a single row high, which is unlikely)
        //
        // This also happens to circumvent https://bugs.launchpad.net/kicad/+bug/1804400
        // which appears to be a GTK+3 bug.
        {
            wxDataViewItem parent = GetParent( item );

            if( parent.IsOk() )
                item = parent;
        }

        m_widget->EnsureVisible( item );
    }
}


void LIB_TREE_MODEL_ADAPTER::AttachTo( wxDataViewCtrl* aDataViewCtrl )
{
    wxString partHead = _( "Item" );
    wxString descHead = _( "Description" );

    // The extent of the text doesn't take into account the space on either side
    // in the header, so artificially pad it
    wxSize partHeadMinWidth = KIUI::GetTextSize( partHead + "MMM", aDataViewCtrl );

    // Ensure the part column is wider than the smallest allowable width
    if( m_colWidths[PART_COL] < partHeadMinWidth.x )
        m_colWidths[PART_COL] = partHeadMinWidth.x;

    m_widget = aDataViewCtrl;
    aDataViewCtrl->SetIndent( kDataViewIndent );
    aDataViewCtrl->AssociateModel( this );
    aDataViewCtrl->ClearColumns();

    m_col_part = aDataViewCtrl->AppendTextColumn( partHead, PART_COL, wxDATAVIEW_CELL_INERT,
                                                  m_colWidths[PART_COL] );
    m_col_desc = aDataViewCtrl->AppendTextColumn( descHead, DESC_COL, wxDATAVIEW_CELL_INERT,
                                                  m_colWidths[DESC_COL] );

    m_col_part->SetMinWidth( partHeadMinWidth.x );
}


LIB_ID LIB_TREE_MODEL_ADAPTER::GetAliasFor( const wxDataViewItem& aSelection ) const
{
    const LIB_TREE_NODE* node = ToNode( aSelection );

    LIB_ID emptyId;

    if( !node )
        return emptyId;

    return node->m_LibId;
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


unsigned int LIB_TREE_MODEL_ADAPTER::GetChildren( const wxDataViewItem&   aItem,
                                                  wxDataViewItemArray&    aChildren ) const
{
    const LIB_TREE_NODE* node = ( aItem.IsOk() ? ToNode( aItem ) : &m_tree );

    if( node->m_Type == LIB_TREE_NODE::TYPE::ROOT
            || node->m_Type == LIB_TREE_NODE::LIB
            || ( m_show_units && node->m_Type == LIB_TREE_NODE::TYPE::LIBID ) )
    {
        return IntoArray( *node, aChildren );
    }
    else
    {
        return 0;
    }

}


void LIB_TREE_MODEL_ADAPTER::FinishTreeInitialization()
{
    m_col_part->SetWidth( m_colWidths[PART_COL] );
    m_col_desc->SetWidth( m_colWidths[DESC_COL] );
}


void LIB_TREE_MODEL_ADAPTER::RefreshTree()
{
    // Yes, this is an enormous hack.  But it works on all platforms, it doesn't suffer
    // the On^2 sorting issues that ItemChanged() does on OSX, and it doesn't lose the
    // user's scroll position (which re-attaching or deleting/re-inserting columns does).
    static int walk = 1;

    int partWidth = m_col_part->GetWidth();
    int descWidth = m_col_desc->GetWidth();

    // Only use the widths read back if they are non-zero.
    // GTK returns the displayed width of the column, which is not calculated immediately
    if( descWidth > 0 )
    {
        m_colWidths[PART_COL] = partWidth;
        m_colWidths[DESC_COL] = descWidth;
    }

    m_colWidths[PART_COL] += walk;
    m_colWidths[DESC_COL] -= walk;

    m_col_part->SetWidth( m_colWidths[PART_COL] );
    m_col_desc->SetWidth( m_colWidths[DESC_COL] );
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
    wxASSERT( node );

    switch( aCol )
    {
    default:    // column == -1 is used for default Compare function
    case 0:
        aVariant = UnescapeString( node->m_Name );
        break;
    case 1:
        aVariant = node->m_Desc;
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
    wxASSERT( node );

    if( node->m_Type != LIB_TREE_NODE::LIBID )
    {
        // Currently only aliases are formatted at all
        return false;
    }

    if( !node->m_IsRoot && aCol == 0 )
    {
        // Names of non-root aliases are italicized
        aAttr.SetItalic( true );
        return true;
    }
    else
    {
        return false;
    }
}


void LIB_TREE_MODEL_ADAPTER::FindAndExpand( LIB_TREE_NODE& aNode,
                                            std::function<bool( const LIB_TREE_NODE* )> aFunc,
                                            LIB_TREE_NODE** aHighScore )
{
    for( std::unique_ptr<LIB_TREE_NODE>& node: aNode.m_Children )
    {
        if( aFunc( &*node ) )
        {
            wxDataViewItem item = wxDataViewItem( &*node );
            m_widget->ExpandAncestors( item );

            if( !(*aHighScore) || node->m_Score > (*aHighScore)->m_Score )
                (*aHighScore) = &*node;
        }

        FindAndExpand( *node, aFunc, aHighScore );
    }
}


LIB_TREE_NODE* LIB_TREE_MODEL_ADAPTER::ShowResults()
{
    LIB_TREE_NODE* highScore = nullptr;

    FindAndExpand( m_tree,
                   []( LIB_TREE_NODE const* n )
                   {
                       // return leaf nodes with some level of matching
                       return n->m_Type == LIB_TREE_NODE::TYPE::LIBID && n->m_Score > 1;
                   },
                   &highScore );

    return highScore;
}


LIB_TREE_NODE* LIB_TREE_MODEL_ADAPTER::ShowPreselect()
{
    LIB_TREE_NODE* highScore = nullptr;

    if( !m_preselect_lib_id.IsValid() )
        return highScore;

    FindAndExpand( m_tree,
            [&]( LIB_TREE_NODE const* n )
            {
                if( n->m_Type == LIB_TREE_NODE::LIBID && ( n->m_Children.empty() ||
                                                           !m_preselect_unit ) )
                    return m_preselect_lib_id == n->m_LibId;
                else if( n->m_Type == LIB_TREE_NODE::UNIT && m_preselect_unit )
                    return m_preselect_lib_id == n->m_Parent->m_LibId &&
                            m_preselect_unit == n->m_Unit;
                else
                    return false;
            },
            &highScore );

    return highScore;
}


LIB_TREE_NODE* LIB_TREE_MODEL_ADAPTER::ShowSingleLibrary()
{
    LIB_TREE_NODE* highScore = nullptr;

    FindAndExpand( m_tree,
                   []( LIB_TREE_NODE const* n )
                   {
                       return n->m_Type == LIB_TREE_NODE::TYPE::LIBID &&
                              n->m_Parent->m_Parent->m_Children.size() == 1;
                   },
                   &highScore );

    return highScore;
}

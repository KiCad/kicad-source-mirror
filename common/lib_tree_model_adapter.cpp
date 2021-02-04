/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright (C) 2014-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <lib_tree_model_adapter.h>

#include <eda_pattern_match.h>

#include <wx/progdlg.h>
#include <wx/tokenzr.h>
#include <wx/wupdlock.h>


static const int kDataViewIndent = 20;


/**
 * Convert CMP_TREE_NODE -> wxDataViewItem
 */
wxDataViewItem LIB_TREE_MODEL_ADAPTER::ToItem( LIB_TREE_NODE const* aNode )
{
    return wxDataViewItem( const_cast<void*>( static_cast<void const*>( aNode ) ) );
}


/**
 * Convert wxDataViewItem -> CMP_TREE_NODE
 */
LIB_TREE_NODE const* LIB_TREE_MODEL_ADAPTER::ToNode( wxDataViewItem aItem )
{
    return static_cast<LIB_TREE_NODE const*>( aItem.GetID() );
}


/**
 * Convert CMP_TREE_NODE's children to wxDataViewItemArray
 */
unsigned int LIB_TREE_MODEL_ADAPTER::IntoArray( LIB_TREE_NODE const& aNode,
                                                wxDataViewItemArray& aChildren )
{
    unsigned int n = 0;

    for( auto const& child: aNode.Children )
    {
        if( child->Score > 0 )
        {
            aChildren.Add( ToItem( &*child ) );
            ++n;
        }
    }

    return n;
}


LIB_TREE_MODEL_ADAPTER::LIB_TREE_MODEL_ADAPTER()
    :m_filter( CMP_FILTER_NONE ),
     m_show_units( true ),
     m_preselect_unit( 0 ),
     m_freeze( 0 ),
     m_col_part( nullptr ),
     m_col_desc( nullptr ),
     m_widget( nullptr )
{}


LIB_TREE_MODEL_ADAPTER::~LIB_TREE_MODEL_ADAPTER()
{}


void LIB_TREE_MODEL_ADAPTER::SetFilter( CMP_FILTER_TYPE aFilter )
{
    m_filter = aFilter;
}


void LIB_TREE_MODEL_ADAPTER::ShowUnits( bool aShow )
{
    m_show_units = aShow;
}


void LIB_TREE_MODEL_ADAPTER::UpdateWidth( int aCol )
{
    auto col = m_widget->GetColumn( aCol );

    if( col )
    {
        col->SetWidth( ColWidth( m_tree, aCol, col->GetTitle() ) );
    }
}


void LIB_TREE_MODEL_ADAPTER::SetPreselectNode( LIB_ID const& aLibId, int aUnit )
{
    m_preselect_lib_id = aLibId;
    m_preselect_unit = aUnit;
}


void LIB_TREE_MODEL_ADAPTER::DoAddLibrary( wxString const& aNodeName, wxString const& aDesc,
                                           std::vector<LIB_TREE_ITEM*> const& aItemList,
                                           bool presorted )
{
    auto& lib_node = m_tree.AddLib( aNodeName, aDesc );

    lib_node.VisLen = wxTheApp->GetTopWindow()->GetTextExtent( lib_node.Name ).x;

    for( auto item: aItemList )
    {
        if( item )
        {
            auto& child_node = lib_node.AddItem( item );
            child_node.VisLen = wxTheApp->GetTopWindow()->GetTextExtent( child_node.Name ).x;
        }
    }

    lib_node.AssignIntrinsicRanks( presorted );
}


void LIB_TREE_MODEL_ADAPTER::UpdateSearchString( wxString const& aSearch, bool aState )
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
        if( !aState && !aSearch.IsNull() && m_tree.Children.size() )
        {
            for( std::unique_ptr<LIB_TREE_NODE>& child: m_tree.Children )
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
            const wxString term = tokenizer.GetNextToken().Lower();
            EDA_COMBINED_MATCHER matcher( term );

            m_tree.UpdateScore( matcher );
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
        auto item = wxDataViewItem( bestMatch );
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

    UpdateWidth( 0 );
}


void LIB_TREE_MODEL_ADAPTER::AttachTo( wxDataViewCtrl* aDataViewCtrl )
{
    m_widget = aDataViewCtrl;
    aDataViewCtrl->SetIndent( kDataViewIndent );
    aDataViewCtrl->AssociateModel( this );
    aDataViewCtrl->ClearColumns();

    wxString part_head = _( "Item" );
    wxString desc_head = _( "Description" );

    m_col_part = aDataViewCtrl->AppendTextColumn( part_head, 0, wxDATAVIEW_CELL_INERT, 360 );
    m_col_desc = aDataViewCtrl->AppendTextColumn( desc_head, 1, wxDATAVIEW_CELL_INERT, 2000 );
}


LIB_ID LIB_TREE_MODEL_ADAPTER::GetAliasFor( const wxDataViewItem& aSelection ) const
{
    auto node = ToNode( aSelection );

    LIB_ID emptyId;

    if( !node )
        return emptyId;

    return node->LibId;
}


int LIB_TREE_MODEL_ADAPTER::GetUnitFor( const wxDataViewItem& aSelection ) const
{
    auto node = ToNode( aSelection );
    return node ? node->Unit : 0;
}


LIB_TREE_NODE::TYPE LIB_TREE_MODEL_ADAPTER::GetTypeFor( const wxDataViewItem& aSelection ) const
{
    auto node = ToNode( aSelection );
    return node ? node->Type : LIB_TREE_NODE::INVALID;
}


int LIB_TREE_MODEL_ADAPTER::GetItemCount() const
{
    int n = 0;

    for( auto& lib: m_tree.Children )
        n += lib->Children.size();

    return n;
}


wxDataViewItem LIB_TREE_MODEL_ADAPTER::FindItem( const LIB_ID& aLibId )
{
    for( auto& lib: m_tree.Children )
    {
        if( lib->Name != aLibId.GetLibNickname() )
            continue;

        // if part name is not specified, return the library node
        if( aLibId.GetLibItemName() == "" )
            return ToItem( lib.get() );

        for( auto& alias: lib->Children )
        {
            if( alias->Name == aLibId.GetLibItemName() )
                return ToItem( alias.get() );
        }

        break;  // could not find the part in the requested library
    }

    return wxDataViewItem();
}


unsigned int LIB_TREE_MODEL_ADAPTER::GetChildren( wxDataViewItem const&   aItem,
                                                  wxDataViewItemArray&    aChildren ) const
{
    auto node = ( aItem.IsOk() ? ToNode( aItem ) : &m_tree );

    if( node->Type != LIB_TREE_NODE::TYPE::LIBID
            || ( m_show_units && node->Type == LIB_TREE_NODE::TYPE::LIBID ) )
        return IntoArray( *node, aChildren );
    else
        return 0;
}


bool LIB_TREE_MODEL_ADAPTER::HasContainerColumns( wxDataViewItem const& aItem ) const
{
    return IsContainer( aItem );
}


bool LIB_TREE_MODEL_ADAPTER::IsContainer( wxDataViewItem const& aItem ) const
{
    auto node = ToNode( aItem );
    return node ? node->Children.size() : true;
}


wxDataViewItem LIB_TREE_MODEL_ADAPTER::GetParent( wxDataViewItem const& aItem ) const
{
    if( m_freeze )
        return ToItem( nullptr );

    auto node = ToNode( aItem );
    auto parent = node ? node->Parent : nullptr;

    // wxDataViewModel has no root node, but rather top-level elements have
    // an invalid (null) parent.
    if( !node || !parent || parent->Type == LIB_TREE_NODE::TYPE::ROOT )
        return ToItem( nullptr );
    else
        return ToItem( parent );
}


void LIB_TREE_MODEL_ADAPTER::GetValue( wxVariant&              aVariant,
                                       wxDataViewItem const&   aItem,
                                       unsigned int            aCol ) const
{
    if( IsFrozen() )
    {
        aVariant = wxEmptyString;
        return;
    }

    auto node = ToNode( aItem );
    wxASSERT( node );

    switch( aCol )
    {
    default:    // column == -1 is used for default Compare function
    case 0:
        aVariant = node->Name;
        break;
    case 1:
        aVariant = node->Desc;
        break;
    }
}


bool LIB_TREE_MODEL_ADAPTER::GetAttr( wxDataViewItem const&   aItem,
                                      unsigned int            aCol,
                                      wxDataViewItemAttr&     aAttr ) const
{
    if( IsFrozen() )
        return false;

    auto node = ToNode( aItem );
    wxASSERT( node );

    if( node->Type != LIB_TREE_NODE::LIBID )
    {
        // Currently only aliases are formatted at all
        return false;
    }

    if( !node->IsRoot && aCol == 0 )
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


int LIB_TREE_MODEL_ADAPTER::ColWidth( LIB_TREE_NODE& aTree, int aCol, wxString const& aHeading )
{
    if( aCol == 0 )
    {
        int padding = m_widget->GetTextExtent( "M" ).x;
        int indent = m_widget->GetIndent();
        int longest = m_widget->GetTextExtent( aHeading ).x;

        for( auto& node : aTree.Children )
        {
            auto item = ToItem( &*node );

            if( !item.IsOk() )
                continue;

            if( node->Score > 0 )
            {
                // Ensure the text size is up to date:
                if( node->VisLen == 0 )
                    node->VisLen = m_widget->GetTextExtent( node->Name ).x;

                longest = std::max( longest, node->VisLen + padding + indent );
            }

            if( !m_widget->IsExpanded( item ) )
                continue;

            for( auto& childNode : node->Children )
            {
                if( childNode->Score > 0 )
                {
                    if( childNode->VisLen == 0 )
                        childNode->VisLen = m_widget->GetTextExtent( childNode->Name ).x;

                    longest = std::max( longest, childNode->VisLen + padding + 2 * indent );
                }
            }
        }

        return longest;
    }
    else
        return 2000;
}


void LIB_TREE_MODEL_ADAPTER::FindAndExpand( LIB_TREE_NODE& aNode,
                                            std::function<bool( LIB_TREE_NODE const* )> aFunc,
                                            LIB_TREE_NODE** aHighScore )
{
    for( auto& node: aNode.Children )
    {
        if( aFunc( &*node ) )
        {
            auto item = wxDataViewItem( &*node );
            m_widget->ExpandAncestors( item );

            if( !(*aHighScore) || node->Score > (*aHighScore)->Score )
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
                       return n->Type == LIB_TREE_NODE::TYPE::LIBID && n->Score > 1;
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
                if( n->Type == LIB_TREE_NODE::LIBID && ( n->Children.empty() || !m_preselect_unit ) )
                    return m_preselect_lib_id == n->LibId;
                else if( n->Type == LIB_TREE_NODE::UNIT && m_preselect_unit )
                    return m_preselect_lib_id == n->Parent->LibId && m_preselect_unit == n->Unit;
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
                       return n->Type == LIB_TREE_NODE::TYPE::LIBID &&
                              n->Parent->Parent->Children.size() == 1;
                   },
                   &highScore );

    return highScore;
}

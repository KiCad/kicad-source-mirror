/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright (C) 2014-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <eda_pattern_match.h>
#include <kiface_i.h>
#include <lib_tree_model_adapter.h>
#include <wx/tokenzr.h>
#include <wx/wupdlock.h>


#define LIST_COLUMN_WIDTH_KEY wxT( "SelectorColumnWidth" )

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
{
    // Default column widths
    m_colWidths[PART_COL] = 360;
    m_colWidths[DESC_COL] = 2000;

    m_config = Kiface().KifaceSettings();
    m_configPrefix = typeid( this ).name();

    // Read the column width from the config
    int colWidth = 0;

    if( m_config->Read( m_configPrefix + LIST_COLUMN_WIDTH_KEY, &colWidth ) )
        m_colWidths[PART_COL] = colWidth;
}


LIB_TREE_MODEL_ADAPTER::~LIB_TREE_MODEL_ADAPTER()
{}


void LIB_TREE_MODEL_ADAPTER::SaveColWidths()
{
    if( m_widget )
    {
        int colWidth = m_widget->GetColumn( PART_COL )->GetWidth();
        m_config->Write( m_configPrefix + LIST_COLUMN_WIDTH_KEY, colWidth );
    }
    else
    {
        wxLogDebug( "Error saving column size, tree view doesn't exist" );
    }
}


void LIB_TREE_MODEL_ADAPTER::SetFilter( CMP_FILTER_TYPE aFilter )
{
    m_filter = aFilter;
}


void LIB_TREE_MODEL_ADAPTER::ShowUnits( bool aShow )
{
    m_show_units = aShow;
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
    LIB_TREE_NODE_LIB& lib_node = m_tree.AddLib( aNodeName, aDesc );

    for( LIB_TREE_ITEM* item: aItemList )
        lib_node.AddItem( item );

    lib_node.AssignIntrinsicRanks( presorted );
}


void LIB_TREE_MODEL_ADAPTER::UpdateSearchString( wxString const& aSearch )
{
    m_tree.ResetScore();

    wxStringTokenizer tokenizer( aSearch );

    while( tokenizer.HasMoreTokens() )
    {
        const wxString term = tokenizer.GetNextToken().Lower();
        EDA_COMBINED_MATCHER matcher( term );

        m_tree.UpdateScore( matcher );
    }

    m_tree.SortNodes();

    {
        wxWindowUpdateLocker updateLock( m_widget );

        // Even with the updateLock, wxWidgets sometimes ties its knickers in
        // a knot when trying to run a wxdataview_selection_changed_callback()
        // on a row that has been deleted.
        // https://bugs.launchpad.net/kicad/+bug/1756255
        m_widget->UnselectAll();

        Cleared();
#ifndef __WINDOWS__
        // The fastest method to update wxDataViewCtrl is to rebuild from
        // scratch by calling Cleared(). Linux requires to reassociate model to
        // display data, but Windows will create multiple associations.
        AttachTo( m_widget );
#endif
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
}


void LIB_TREE_MODEL_ADAPTER::AttachTo( wxDataViewCtrl* aDataViewCtrl )
{
    wxString partHead = _( "Item" );
    wxString descHead = _( "Description" );

    if( aDataViewCtrl->GetColumnCount() > 0 )
    {
        int partWidth = aDataViewCtrl->GetColumn( PART_COL )->GetWidth();
        int descWidth = aDataViewCtrl->GetColumn( DESC_COL )->GetWidth();

        // Only use the widths read back if they are non-zero.
        // GTK returns the displayed width of the column, which is not calculated immediately
        // this leads to cases of 0 column width if the user types too fast in the filter
        if( descWidth > 0 )
        {
            m_colWidths[PART_COL] = partWidth;
            m_colWidths[DESC_COL] = descWidth;
        }
    }

    m_widget = aDataViewCtrl;
    aDataViewCtrl->SetIndent( kDataViewIndent );
    aDataViewCtrl->AssociateModel( this );
    aDataViewCtrl->ClearColumns();

    m_col_part = aDataViewCtrl->AppendTextColumn( partHead, PART_COL, wxDATAVIEW_CELL_INERT,
                                                  m_colWidths[PART_COL] );
    m_col_desc = aDataViewCtrl->AppendTextColumn( descHead, DESC_COL, wxDATAVIEW_CELL_INERT,
                                                  m_colWidths[DESC_COL] );
}


LIB_ID LIB_TREE_MODEL_ADAPTER::GetAliasFor( const wxDataViewItem& aSelection ) const
{
    const LIB_TREE_NODE* node = ToNode( aSelection );

    LIB_ID emptyId;

    if( !node )
        return emptyId;

    return node->LibId;
}


int LIB_TREE_MODEL_ADAPTER::GetUnitFor( const wxDataViewItem& aSelection ) const
{
    const LIB_TREE_NODE* node = ToNode( aSelection );
    return node ? node->Unit : 0;
}


LIB_TREE_NODE::TYPE LIB_TREE_MODEL_ADAPTER::GetTypeFor( const wxDataViewItem& aSelection ) const
{
    const LIB_TREE_NODE* node = ToNode( aSelection );
    return node ? node->Type : LIB_TREE_NODE::INVALID;
}


int LIB_TREE_MODEL_ADAPTER::GetItemCount() const
{
    int n = 0;

    for( const std::unique_ptr<LIB_TREE_NODE>& lib: m_tree.Children )
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

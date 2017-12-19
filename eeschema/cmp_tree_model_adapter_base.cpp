/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright (C) 2014-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <cmp_tree_model_adapter_base.h>

#include <eda_pattern_match.h>

#include <wx/progdlg.h>
#include <wx/tokenzr.h>
#include <wx/wupdlock.h>


CMP_TREE_MODEL_ADAPTER_BASE::WIDTH_CACHE CMP_TREE_MODEL_ADAPTER_BASE::m_width_cache;


static const int kDataViewIndent = 20;


/**
 * Convert CMP_TREE_NODE -> wxDataViewItem
 */
wxDataViewItem CMP_TREE_MODEL_ADAPTER_BASE::ToItem( CMP_TREE_NODE const* aNode )
{
    return wxDataViewItem( const_cast<void*>( static_cast<void const*>( aNode ) ) );
}


/**
 * Convert wxDataViewItem -> CMP_TREE_NODE
 */
CMP_TREE_NODE const* CMP_TREE_MODEL_ADAPTER_BASE::ToNode( wxDataViewItem aItem )
{
    return static_cast<CMP_TREE_NODE const*>( aItem.GetID() );
}


/**
 * Convert CMP_TREE_NODE's children to wxDataViewItemArray
 */
unsigned int CMP_TREE_MODEL_ADAPTER_BASE::IntoArray(
        CMP_TREE_NODE const& aNode, wxDataViewItemArray& aChildren )
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


CMP_TREE_MODEL_ADAPTER_BASE::CMP_TREE_MODEL_ADAPTER_BASE()
    :m_filter( CMP_FILTER_NONE ),
     m_show_units( true ),
     m_preselect_unit( 0 ),
     m_col_part( nullptr ),
     m_col_desc( nullptr ),
     m_widget( nullptr )
{}


CMP_TREE_MODEL_ADAPTER_BASE::~CMP_TREE_MODEL_ADAPTER_BASE()
{}


void CMP_TREE_MODEL_ADAPTER_BASE::SetFilter( CMP_FILTER_TYPE aFilter )
{
    m_filter = aFilter;
}


void CMP_TREE_MODEL_ADAPTER_BASE::ShowUnits( bool aShow )
{
    m_show_units = aShow;
}


void CMP_TREE_MODEL_ADAPTER_BASE::SetPreselectNode( LIB_ID const& aLibId, int aUnit )
{
    m_preselect_lib_id = aLibId;
    m_preselect_unit = aUnit;
}


void CMP_TREE_MODEL_ADAPTER_BASE::AddLibrariesWithProgress(
        const std::vector<wxString>& aNicknames, wxWindow* aParent )
{
    auto* prg = new wxProgressDialog(
            _( "Loading symbol libraries" ),
            wxEmptyString,
            aNicknames.size(),
            aParent );

    unsigned int ii = 0;

    for( auto nickname : aNicknames )
    {
        prg->Update( ii++, wxString::Format( _( "Loading library \"%s\"" ), nickname ) );
        AddLibrary( nickname );
    }

    prg->Destroy();
}


void CMP_TREE_MODEL_ADAPTER_BASE::AddAliasList(
            wxString const&         aNodeName,
            std::vector<LIB_ALIAS*> const&  aAliasList )
{
    auto& lib_node = m_tree.AddLib( aNodeName );

    for( auto a: aAliasList )
    {
        lib_node.AddAlias( a );
    }

    lib_node.AssignIntrinsicRanks();
    m_tree.AssignIntrinsicRanks();
}


void CMP_TREE_MODEL_ADAPTER_BASE::UpdateSearchString( wxString const& aSearch )
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

        Cleared();
#ifndef __WINDOWS__
        // The fastest method to update wxDataViewCtrl is to rebuild from
        // scratch by calling Cleared(). Linux requires to reassociate model to
        // display data, but Windows will create multiple associations.
        AttachTo( m_widget );
#endif
    }

    ShowResults() || ShowPreselect() || ShowSingleLibrary();
}


void CMP_TREE_MODEL_ADAPTER_BASE::AttachTo( wxDataViewCtrl* aDataViewCtrl )
{
    m_widget = aDataViewCtrl;
    aDataViewCtrl->SetIndent( kDataViewIndent );
    aDataViewCtrl->AssociateModel( this );
    aDataViewCtrl->ClearColumns();

    wxString part_head = _( "Part" );
    wxString desc_head = _( "Desc" );

    m_col_part = aDataViewCtrl->AppendTextColumn( part_head, 0, wxDATAVIEW_CELL_INERT,
                ColWidth( m_tree, 0, part_head ) );
    m_col_desc = aDataViewCtrl->AppendTextColumn( desc_head, 1, wxDATAVIEW_CELL_INERT,
                ColWidth( m_tree, 1, desc_head ) );
    m_col_part->SetSortOrder( 0 );
}


LIB_ID CMP_TREE_MODEL_ADAPTER_BASE::GetAliasFor( const wxDataViewItem& aSelection ) const
{
    auto node = ToNode( aSelection );

    LIB_ID emptyId;

    if( !node )
        return emptyId;

    return node->LibId;
}


int CMP_TREE_MODEL_ADAPTER_BASE::GetUnitFor( const wxDataViewItem& aSelection ) const
{
    auto node = ToNode( aSelection );
    return node ? node->Unit : 0;
}


CMP_TREE_NODE::TYPE CMP_TREE_MODEL_ADAPTER_BASE::GetTypeFor( const wxDataViewItem& aSelection ) const
{
    auto node = ToNode( aSelection );
    return node ? node->Type : CMP_TREE_NODE::INVALID;
}


int CMP_TREE_MODEL_ADAPTER_BASE::GetComponentsCount() const
{
    int n = 0;

    for( auto& lib: m_tree.Children )
    {
        for( auto& alias: lib->Children )
        {
            (void) alias;
            ++n;
        }
    }

    return n;
}


wxDataViewItem CMP_TREE_MODEL_ADAPTER_BASE::FindItem( const LIB_ID& aLibId )
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


unsigned int CMP_TREE_MODEL_ADAPTER_BASE::GetChildren(
            wxDataViewItem const&   aItem,
            wxDataViewItemArray&    aChildren ) const
{
    auto node = ( aItem.IsOk() ? ToNode( aItem ) : &m_tree );

    if( node->Type != CMP_TREE_NODE::TYPE::LIBID
            || ( m_show_units && node->Type == CMP_TREE_NODE::TYPE::LIBID ) )
        return IntoArray( *node, aChildren );
    else
        return 0;
}


bool CMP_TREE_MODEL_ADAPTER_BASE::HasContainerColumns( wxDataViewItem const& aItem ) const
{
    return IsContainer( aItem );
}


bool CMP_TREE_MODEL_ADAPTER_BASE::IsContainer( wxDataViewItem const& aItem ) const
{
    auto node = ToNode( aItem );
    return node ? node->Children.size() : true;
}


wxDataViewItem CMP_TREE_MODEL_ADAPTER_BASE::GetParent( wxDataViewItem const& aItem ) const
{
    auto node = ToNode( aItem );
    auto parent = node ? node->Parent : nullptr;

    // wxDataViewModel has no root node, but rather top-level elements have
    // an invalid (null) parent.
    if( !node || !parent || parent->Type == CMP_TREE_NODE::TYPE::ROOT )
    {
        return ToItem( nullptr );
    }
    else
    {
        return ToItem( parent );
    }
}


void CMP_TREE_MODEL_ADAPTER_BASE::GetValue(
            wxVariant&              aVariant,
            wxDataViewItem const&   aItem,
            unsigned int            aCol ) const
{
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


bool CMP_TREE_MODEL_ADAPTER_BASE::GetAttr(
            wxDataViewItem const&   aItem,
            unsigned int            aCol,
            wxDataViewItemAttr&     aAttr ) const
{
    auto node = ToNode( aItem );
    wxASSERT( node );

    if( node->Type != CMP_TREE_NODE::LIBID )
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


int CMP_TREE_MODEL_ADAPTER_BASE::ColWidth( CMP_TREE_NODE& aTree, int aCol, wxString const& aHeading )
{
    const int indent = aCol ? 0 : kDataViewIndent;

    int min_width = WidthFor( aHeading, aCol );
    int width = std::max( aTree.Score > 0 ? WidthFor( aTree, aCol ) : 0, min_width );

    if( aTree.Score > 0 )
    {
        for( auto& node: aTree.Children )
        {
            width = std::max( width, ColWidth( *node, aCol, aHeading ) + indent );
        }
    }

    return width;
}


int CMP_TREE_MODEL_ADAPTER_BASE::WidthFor( CMP_TREE_NODE& aNode, int aCol )
{
    auto result = m_width_cache.find( aNode.Name );

    if( result != m_width_cache.end() )
    {
        return result->second[aCol];
    }
    else
    {
        int wname = m_widget->GetTextExtent( aNode.Name ).x + kDataViewIndent;
        int wdesc = m_widget->GetTextExtent( aNode.Desc ).x;

        auto& val = m_width_cache[aNode.Name];
        val.push_back( wname );
        val.push_back( wdesc );
        return val[aCol];
    }
}


int CMP_TREE_MODEL_ADAPTER_BASE::WidthFor( wxString const& aHeading, int aCol )
{
    static std::vector<int> widths;

    for( int i = (int) widths.size(); i <= aCol; ++i )
    {
        widths.push_back( 0 );
    }

    if( widths[aCol] == 0 )
    {
        widths[aCol] = m_widget->GetTextExtent( aHeading ).x;
    }

    return widths[aCol];
}


bool CMP_TREE_MODEL_ADAPTER_BASE::FindAndExpand(
        CMP_TREE_NODE& aNode,
        std::function<bool( CMP_TREE_NODE const* )> aFunc )
{
    for( auto& node: aNode.Children )
    {
        if( aFunc( &*node ) )
        {
            auto item = wxDataViewItem(
                    const_cast<void*>( static_cast<void const*>( &*node ) ) );
            m_widget->ExpandAncestors( item );
            m_widget->EnsureVisible( item );
            m_widget->Select( item );
            return true;
        }
        else if( FindAndExpand( *node, aFunc ) )
        {
            return true;
        }
    }

    return false;
}


bool CMP_TREE_MODEL_ADAPTER_BASE::ShowResults()
{
    return FindAndExpand( m_tree,
            []( CMP_TREE_NODE const* n )
            {
                return n->Type == CMP_TREE_NODE::TYPE::LIBID && n->Score > 1;
            } );
}


bool CMP_TREE_MODEL_ADAPTER_BASE::ShowPreselect()
{
    if( !m_preselect_lib_id.IsValid() )
        return false;

    return FindAndExpand( m_tree,
            [&]( CMP_TREE_NODE const* n )
            {
                if( n->Type == CMP_TREE_NODE::LIBID && ( n->Children.empty() || !m_preselect_unit ) )
                    return m_preselect_lib_id == n->LibId;
                else if( n->Type == CMP_TREE_NODE::UNIT && m_preselect_unit )
                    return m_preselect_lib_id == n->Parent->LibId && m_preselect_unit == n->Unit;
                else
                    return false;
            } );
}


bool CMP_TREE_MODEL_ADAPTER_BASE::ShowSingleLibrary()
{
    return FindAndExpand( m_tree,
            []( CMP_TREE_NODE const* n )
            {
                return n->Type == CMP_TREE_NODE::TYPE::LIBID &&
                       n->Parent->Parent->Children.size() == 1;
            } );
}

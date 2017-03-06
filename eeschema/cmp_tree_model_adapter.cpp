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

#include <cmp_tree_model_adapter.h>

#include <class_library.h>
#include <eda_pattern_match.h>
#include <wx/tokenzr.h>


CMP_TREE_MODEL_ADAPTER::WIDTH_CACHE CMP_TREE_MODEL_ADAPTER::m_width_cache;


static const int kDataViewIndent = 20;


/**
 * Convert CMP_TREE_NODE -> wxDataViewItem
 */
static wxDataViewItem ToItem( CMP_TREE_NODE const* aNode )
{
    return wxDataViewItem( const_cast<void*>( static_cast<void const*>( aNode ) ) );
}


/**
 * Convert wxDataViewItem -> CMP_TREE_NODE
 */
static CMP_TREE_NODE const* ToNode( wxDataViewItem aItem )
{
    return static_cast<CMP_TREE_NODE const*>( aItem.GetID() );
}


/**
 * Convert CMP_TREE_NODE's children to wxDataViewItemArray
 */
static unsigned int IntoArray( CMP_TREE_NODE const& aNode, wxDataViewItemArray& aChildren )
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


CMP_TREE_MODEL_ADAPTER::PTR CMP_TREE_MODEL_ADAPTER::Create( PART_LIBS* aLibs )
{
    auto adapter = new CMP_TREE_MODEL_ADAPTER( aLibs );
    auto container = CMP_TREE_MODEL_ADAPTER::PTR( adapter );
    return container;
}


CMP_TREE_MODEL_ADAPTER::CMP_TREE_MODEL_ADAPTER( PART_LIBS* aLibs )
    :m_filter( CMP_FILTER_NONE ),
     m_show_units( true ),
     m_libs( aLibs ),
     m_preselect_unit( 0 )
{}


CMP_TREE_MODEL_ADAPTER::~CMP_TREE_MODEL_ADAPTER()
{}


void CMP_TREE_MODEL_ADAPTER::SetFilter( CMP_FILTER_TYPE aFilter )
{
    m_filter = aFilter;
}


void CMP_TREE_MODEL_ADAPTER::ShowUnits( bool aShow )
{
    m_show_units = aShow;
}


void CMP_TREE_MODEL_ADAPTER::SetPreselectNode( wxString const& aName, int aUnit )
{
    m_preselect_name = aName;
    m_preselect_unit = aUnit;
}


void CMP_TREE_MODEL_ADAPTER::AddLibrary( PART_LIB& aLib )
{
    if( m_filter == CMP_FILTER_POWER )
    {
        wxArrayString all_aliases;
        aLib.GetEntryTypePowerNames( all_aliases );
        AddAliasList( aLib.GetName(), all_aliases, &aLib );
    }
    else
    {
        std::vector<LIB_ALIAS*> all_aliases;
        aLib.GetAliases( all_aliases );
        AddAliasList( aLib.GetName(), all_aliases, &aLib );
    }

    m_tree.AssignIntrinsicRanks();
}


void CMP_TREE_MODEL_ADAPTER::AddAliasList(
            wxString const&         aNodeName,
            wxArrayString const&    aAliasNameList,
            PART_LIB*               aOptionalLib )
{
    std::vector<LIB_ALIAS*> alias_list;

    for( const wxString& name: aAliasNameList )
    {
        LIB_ALIAS* a;

        if( aOptionalLib )
            a = aOptionalLib->FindAlias( name );
        else
            a = m_libs->FindLibraryAlias( LIB_ID( wxEmptyString, name ), wxEmptyString );

        if( a )
            alias_list.push_back( a );
    }

    AddAliasList( aNodeName, alias_list, aOptionalLib );
}


void CMP_TREE_MODEL_ADAPTER::AddAliasList(
            wxString const&         aNodeName,
            std::vector<LIB_ALIAS*> const&  aAliasList,
            PART_LIB*               aOptionalLib )
{
    auto& lib_node = m_tree.AddLib( aNodeName );

    for( auto a: aAliasList )
    {
        lib_node.AddAlias( a );
    }

    lib_node.AssignIntrinsicRanks();
    m_tree.AssignIntrinsicRanks();
}


void CMP_TREE_MODEL_ADAPTER::UpdateSearchString( wxString const& aSearch )
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
    Cleared();
    AttachTo( m_widget );

    ShowResults() || ShowPreselect() || ShowSingleLibrary();
}


void CMP_TREE_MODEL_ADAPTER::AttachTo( wxDataViewCtrl* aDataViewCtrl )
{
    m_widget = aDataViewCtrl;
    aDataViewCtrl->Freeze();
    aDataViewCtrl->SetIndent( kDataViewIndent );
    aDataViewCtrl->AssociateModel( this );
    aDataViewCtrl->ClearColumns();
    m_col_part = aDataViewCtrl->AppendTextColumn( _( "Part" ), 0, wxDATAVIEW_CELL_INERT,
                ColWidth( m_tree, 0 ) );
    m_col_desc = aDataViewCtrl->AppendTextColumn( _( "Description" ), 1, wxDATAVIEW_CELL_INERT,
                ColWidth( m_tree, 1 ) );
    aDataViewCtrl->Thaw();
}


LIB_ALIAS* CMP_TREE_MODEL_ADAPTER::GetAliasFor( wxDataViewItem aSelection ) const
{
    auto node = ToNode( aSelection );
    return node ? node->Alias : nullptr;
}


int CMP_TREE_MODEL_ADAPTER::GetUnitFor( wxDataViewItem aSelection ) const
{
    auto node = ToNode( aSelection );
    return node ? node->Unit : 0;
}


int CMP_TREE_MODEL_ADAPTER::GetComponentsCount() const
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


bool CMP_TREE_MODEL_ADAPTER::HasContainerColumns( wxDataViewItem const& aItem ) const
{
    return IsContainer( aItem );
}


bool CMP_TREE_MODEL_ADAPTER::IsContainer( wxDataViewItem const& aItem ) const
{
    auto node = ToNode( aItem );
    return node ? node->Children.size() : true;
}


wxDataViewItem CMP_TREE_MODEL_ADAPTER::GetParent( wxDataViewItem const& aItem ) const
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


unsigned int CMP_TREE_MODEL_ADAPTER::GetChildren(
            wxDataViewItem const&   aItem,
            wxDataViewItemArray&    aChildren ) const
{
    auto node = ( aItem.IsOk() ? ToNode( aItem ) : &m_tree );

    if( node->Type != CMP_TREE_NODE::TYPE::ALIAS || m_show_units )
        return IntoArray( *node, aChildren );
    else
        return 0;
}


void CMP_TREE_MODEL_ADAPTER::GetValue(
            wxVariant&              aVariant,
            wxDataViewItem const&   aItem,
            unsigned int            aCol ) const
{
    auto node = ToNode( aItem );
    wxASSERT( node );

    switch( aCol )
    {
    case 0:
        aVariant = node->Name;
        break;
    case 1:
        aVariant = node->Desc;
        break;
    default:
        wxFAIL_MSG( "Invalid column ID!" );
    }
}


int CMP_TREE_MODEL_ADAPTER::Compare(
        wxDataViewItem const& aFirst, wxDataViewItem const& aSecond,
        unsigned int aCol, bool aAscending ) const
{
    auto node1 = ToNode( aFirst );
    auto node2 = ToNode( aSecond );

    if( aAscending )
        return -CMP_TREE_NODE::Compare(*node1, *node2);
    else
        return CMP_TREE_NODE::Compare(*node1, *node2);
}


int CMP_TREE_MODEL_ADAPTER::ColWidth( CMP_TREE_NODE& aNode, int aCol )
{
    const int indent = aCol ? 0 : kDataViewIndent;
    int max_width = aNode.Score > 0 ? WidthFor( aNode, aCol ) : 0;

    for( auto& node: aNode.Children )
    {
        if( aNode.Score > 0 )
            max_width = std::max( max_width, ColWidth( *node, aCol ) + indent );
    }

    return max_width;
}


int CMP_TREE_MODEL_ADAPTER::WidthFor( CMP_TREE_NODE& aNode, int aCol )
{
    auto result = m_width_cache.find( &aNode );

    if( result != m_width_cache.end() )
    {
        return result->second[aCol];
    }
    else
    {
        int wname = m_widget->GetTextExtent( aNode.Name ).x + kDataViewIndent;
        int wdesc = m_widget->GetTextExtent( aNode.Desc ).x;

        m_width_cache[&aNode][0] = wname;
        m_width_cache[&aNode][1] = wdesc;
        return m_width_cache[&aNode][aCol];
    }
}


bool CMP_TREE_MODEL_ADAPTER::ShowResults()
{
    return FindAndExpand( m_tree,
            []( CMP_TREE_NODE const* n )
            {
                return n->Type == CMP_TREE_NODE::TYPE::ALIAS && n->Score > 1;
            } );
}


bool CMP_TREE_MODEL_ADAPTER::ShowPreselect()
{
    if( m_preselect_name == wxEmptyString )
        return false;

    return FindAndExpand( m_tree,
            [&]( CMP_TREE_NODE const* n )
            {
                return m_preselect_name == n->Name && m_preselect_unit == n->Unit;
            } );
}


bool CMP_TREE_MODEL_ADAPTER::ShowSingleLibrary()
{
    return FindAndExpand( m_tree,
            []( CMP_TREE_NODE const* n )
            {
                return n->Type == CMP_TREE_NODE::TYPE::ALIAS &&
                       n->Parent->Parent->Children.size() == 1;
            } );
}

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright (C) 2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <lib_tree_model.h>

#include <algorithm>
#include <core/kicad_algo.h>
#include <eda_pattern_match.h>
#include <lib_tree_item.h>
#include <pgm_base.h>
#include <string_utils.h>



void LIB_TREE_NODE::RebuildSearchTerms( const std::vector<wxString>& aShownColumns )
{
    m_SearchTerms = m_sourceSearchTerms;

    for( const auto& [name, value] : m_Fields )
    {
        if( alg::contains( aShownColumns, name ) )
            m_SearchTerms.push_back( SEARCH_TERM( value, 4 ) );
    }
}


void LIB_TREE_NODE::AssignIntrinsicRanks( const std::vector<wxString>& aShownColumns, bool presorted )
{
    for( std::unique_ptr<LIB_TREE_NODE>& child: m_Children )
        child->RebuildSearchTerms( aShownColumns );

    std::vector<LIB_TREE_NODE*> sort_buf;

    if( presorted )
    {
        int max = m_Children.size() - 1;

        for( int i = 0; i <= max; ++i )
            m_Children[i]->m_IntrinsicRank = max - i;
    }
    else
    {
        for( std::unique_ptr<LIB_TREE_NODE>& child: m_Children )
            sort_buf.push_back( child.get() );

        std::sort( sort_buf.begin(), sort_buf.end(),
                []( LIB_TREE_NODE* a, LIB_TREE_NODE* b ) -> bool
                {
                    return StrNumCmp( a->m_Name, b->m_Name, true ) > 0;
                } );

        for( int i = 0; i < (int) sort_buf.size(); ++i )
            sort_buf[i]->m_IntrinsicRank = i;
    }
}


void LIB_TREE_NODE::SortNodes( bool aUseScores )
{
    std::sort( m_Children.begin(), m_Children.end(),
            [&]( std::unique_ptr<LIB_TREE_NODE>& a, std::unique_ptr<LIB_TREE_NODE>& b )
            {
                return Compare( *a, *b, aUseScores );
            } );

    for( std::unique_ptr<LIB_TREE_NODE>& node: m_Children )
        node->SortNodes( aUseScores );
}


bool LIB_TREE_NODE::Compare( LIB_TREE_NODE const& aNode1, LIB_TREE_NODE const& aNode2,
                             bool aUseScores )
{
    if( aNode1.m_Type != aNode2.m_Type )
        return aNode1.m_Type < aNode2.m_Type;

    // Recently used sorts at top
    if( aNode1.m_IsRecentlyUsedGroup )
    {
        if( aNode2.m_IsRecentlyUsedGroup )
        {
            // Make sure "-- Recently Used" is always at the top
            // Start by checking the name of aNode2, because we want to satisfy the irreflexive
            // property of the strict weak ordering.
            if( aNode2.m_IsRecentlyUsedGroup )
                return false;
            else if( aNode1.m_IsRecentlyUsedGroup )
                return true;

            return aNode1.m_IntrinsicRank > aNode2.m_IntrinsicRank;
        }
        else
        {
            return true;
        }
    }
    else if( aNode2.m_Name.StartsWith( wxT( "-- " ) ) )
    {
        return false;
    }

    // Pinned nodes go next
    if( aNode1.m_Pinned && !aNode2.m_Pinned )
        return true;
    else if( aNode2.m_Pinned && !aNode1.m_Pinned )
        return false;

    if( aUseScores && aNode1.m_Score != aNode2.m_Score )
        return aNode1.m_Score > aNode2.m_Score;

    if( aNode1.m_IntrinsicRank != aNode2.m_IntrinsicRank )
        return aNode1.m_IntrinsicRank > aNode2.m_IntrinsicRank;

    return reinterpret_cast<const void*>( &aNode1 ) < reinterpret_cast<const void*>( &aNode2 );
}


LIB_TREE_NODE::LIB_TREE_NODE()
    : m_Parent( nullptr ),
      m_Type( TYPE::INVALID ),
      m_IntrinsicRank( 0 ),
      m_Score( 0 ),
      m_Pinned( false ),
      m_PinCount( 0 ),
      m_Unit( 0 ),
      m_IsRoot( false ),
      m_IsRecentlyUsedGroup( false ),
      m_IsAlreadyPlacedGroup( false )
{}


LIB_TREE_NODE_UNIT::LIB_TREE_NODE_UNIT( LIB_TREE_NODE* aParent, LIB_TREE_ITEM* aItem, int aUnit )
{
    m_Parent = aParent;
    m_Type = TYPE::UNIT;

    m_Unit = aUnit;
    m_LibId = aParent->m_LibId;
    m_Name = aItem->GetUnitName( aUnit );

    m_IntrinsicRank = -aUnit;
}


void LIB_TREE_NODE_UNIT::UpdateScore( const std::vector<std::unique_ptr<EDA_COMBINED_MATCHER>>& aMatchers,
                                      std::function<bool( LIB_TREE_NODE& aNode )>* aFilter )
{
    m_Score = 1;

    // aMatchers test results are inherited from parent
    if( !aMatchers.empty() )
        m_Score = m_Parent->m_Score;

    if( aFilter && !(*aFilter)(*this) )
        m_Score = 0;
}


LIB_TREE_NODE_ITEM::LIB_TREE_NODE_ITEM( LIB_TREE_NODE* aParent, LIB_TREE_ITEM* aItem )
{
    m_Type = TYPE::ITEM;
    m_Parent = aParent;

    m_LibId.SetLibNickname( aItem->GetLibNickname() );
    m_LibId.SetLibItemName( aItem->GetName() );

    m_Name = aItem->GetName();
    m_Desc = aItem->GetDesc();
    m_Footprint = aItem->GetFootprint();
    m_PinCount = aItem->GetPinCount();

    aItem->GetChooserFields( m_Fields );

    m_sourceSearchTerms = aItem->GetSearchTerms();

    m_IsRoot = aItem->IsRoot();

    if( aItem->GetSubUnitCount() > 1 )
    {
        for( int u = 1; u <= aItem->GetSubUnitCount(); ++u )
            AddUnit( aItem, u );
    }
}


LIB_TREE_NODE_UNIT& LIB_TREE_NODE_ITEM::AddUnit( LIB_TREE_ITEM* aItem, int aUnit )
{
    LIB_TREE_NODE_UNIT* unit = new LIB_TREE_NODE_UNIT( this, aItem, aUnit );
    m_Children.push_back( std::unique_ptr<LIB_TREE_NODE>( unit ) );
    return *unit;
}


void LIB_TREE_NODE_ITEM::Update( LIB_TREE_ITEM* aItem )
{
    m_LibId.SetLibNickname( aItem->GetLIB_ID().GetLibNickname() );
    m_LibId.SetLibItemName( aItem->GetName() );

    m_Name = aItem->GetName();
    m_Desc = aItem->GetDesc();

    aItem->GetChooserFields( m_Fields );

    m_sourceSearchTerms = aItem->GetSearchTerms();

    m_IsRoot = aItem->IsRoot();
    m_Children.clear();

    for( int u = 1; u <= aItem->GetSubUnitCount(); ++u )
        AddUnit( aItem, u );
}


void LIB_TREE_NODE_ITEM::UpdateScore( const std::vector<std::unique_ptr<EDA_COMBINED_MATCHER>>& aMatchers,
                                      std::function<bool( LIB_TREE_NODE& aNode )>* aFilter )
{
    m_Score = 1;

    for( const std::unique_ptr<EDA_COMBINED_MATCHER>& matcher : aMatchers )
    {
        int score = matcher->ScoreTerms( m_SearchTerms );

        if( score == 0 )
        {
            m_Score = 0;
            break;
        }

        m_Score += score;
    }

    if( aFilter && !(*aFilter)(*this) )
        m_Score = 0;

    for( std::unique_ptr<LIB_TREE_NODE>& child: m_Children )
        child->UpdateScore( aMatchers, aFilter );
}


LIB_TREE_NODE_LIBRARY::LIB_TREE_NODE_LIBRARY( LIB_TREE_NODE* aParent, wxString const& aName,
                                              wxString const& aDesc )
{
    m_Type = TYPE::LIBRARY;
    m_Name = aName;
    m_Desc = aDesc;
    m_Parent = aParent;
    m_LibId.SetLibNickname( aName );

    m_SearchTerms.emplace_back( SEARCH_TERM( aName, 8 ) );
}


LIB_TREE_NODE_ITEM& LIB_TREE_NODE_LIBRARY::AddItem( LIB_TREE_ITEM* aItem )
{
    LIB_TREE_NODE_ITEM* item = new LIB_TREE_NODE_ITEM( this, aItem );
    m_Children.push_back( std::unique_ptr<LIB_TREE_NODE>( item ) );
    return *item;
}


void LIB_TREE_NODE_LIBRARY::UpdateScore( const std::vector<std::unique_ptr<EDA_COMBINED_MATCHER>>& aMatchers,
                                         std::function<bool( LIB_TREE_NODE& aNode )>* aFilter )
{
    if( m_Children.empty() )
    {
        m_Score = 1;

        for( const std::unique_ptr<EDA_COMBINED_MATCHER>& matcher : aMatchers )
        {
            int score = matcher->ScoreTerms( m_SearchTerms );

            if( score == 0 )
            {
                m_Score = 0;
                break;
            }

            m_Score += score;
        }
    }
    else
    {
        m_Score = 0;

        for( std::unique_ptr<LIB_TREE_NODE>& child: m_Children )
        {
            child->UpdateScore( aMatchers, aFilter );
            m_Score = std::max( m_Score, child->m_Score );
        }
    }
}


LIB_TREE_NODE_ROOT::LIB_TREE_NODE_ROOT()
{
    m_Type = TYPE::ROOT;
}


LIB_TREE_NODE_LIBRARY& LIB_TREE_NODE_ROOT::AddLib( wxString const& aName, wxString const& aDesc )
{
    LIB_TREE_NODE_LIBRARY* lib = new LIB_TREE_NODE_LIBRARY( this, aName, aDesc );
    m_Children.push_back( std::unique_ptr<LIB_TREE_NODE>( lib ) );
    return *lib;
}


void LIB_TREE_NODE_ROOT::RemoveGroup( bool aRecentlyUsedGroup, bool aAlreadyPlacedGroup )
{
    m_Children.erase( std::remove_if( m_Children.begin(), m_Children.end(),
                                      [&]( std::unique_ptr<LIB_TREE_NODE>& aNode )
                                      {
                                          if( aRecentlyUsedGroup && aNode->m_IsRecentlyUsedGroup )
                                              return true;

                                          if( aAlreadyPlacedGroup && aNode->m_IsAlreadyPlacedGroup )
                                              return true;

                                          return false;
                                      } ),
                      m_Children.end() );
}


void LIB_TREE_NODE_ROOT::Clear()
{
    m_Children.clear();
}


void LIB_TREE_NODE_ROOT::UpdateScore( const std::vector<std::unique_ptr<EDA_COMBINED_MATCHER>>& aMatchers,
                                      std::function<bool( LIB_TREE_NODE& aNode )>* aFilter )
{
    for( std::unique_ptr<LIB_TREE_NODE>& child: m_Children )
        child->UpdateScore( aMatchers, aFilter );
}


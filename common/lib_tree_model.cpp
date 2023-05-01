/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *lib_tree_model
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

#include <lib_tree_model.h>

#include <algorithm>
#include <eda_pattern_match.h>
#include <lib_tree_item.h>
#include <pgm_base.h>
#include <string_utils.h>

// Each node gets this lowest score initially, without any matches applied.  Matches will then
// increase this score.  This way, an empty search string will result in all components being
// displayed as they have the minimum score. However, in that case, we avoid expanding all the
// nodes asd the result is very unspecific.
static const unsigned kLowestDefaultScore = 1;


void LIB_TREE_NODE::ResetScore()
{
    for( std::unique_ptr<LIB_TREE_NODE>& child: m_Children )
        child->ResetScore();

    m_Score = kLowestDefaultScore;
}


void LIB_TREE_NODE::AssignIntrinsicRanks( bool presorted )
{
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
    if( aNode1.m_Name.StartsWith( wxT( "-- " ) ) )
    {
        if( aNode2.m_Name.StartsWith( wxT( "-- " ) ) )
        {
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
      m_Type( INVALID ),
      m_IntrinsicRank( 0 ),
      m_Score( kLowestDefaultScore ),
      m_Pinned( false ),
      m_Unit( 0 ),
      m_IsRoot( false )
{}


LIB_TREE_NODE_UNIT::LIB_TREE_NODE_UNIT( LIB_TREE_NODE* aParent, LIB_TREE_ITEM* aItem, int aUnit )
{
    static void* locale = nullptr;
    static wxString namePrefix;

    // Fetching translations can take a surprising amount of time when loading libraries,
    // so only do it when necessary.
    if( Pgm().GetLocale() != locale )
    {
        namePrefix = _( "Unit" );
        locale = Pgm().GetLocale();
    }

    m_Parent = aParent;
    m_Type = UNIT;

    m_Unit = aUnit;
    m_LibId = aParent->m_LibId;

    m_Name = namePrefix + " " + aItem->GetUnitReference( aUnit );

    if( aItem->HasUnitDisplayName( aUnit ) )
        m_Desc = aItem->GetUnitDisplayName( aUnit );
    else
        m_Desc = wxEmptyString;

    m_IntrinsicRank = -aUnit;
}


LIB_TREE_NODE_LIB_ID::LIB_TREE_NODE_LIB_ID( LIB_TREE_NODE* aParent, LIB_TREE_ITEM* aItem )
{
    m_Type = LIBID;
    m_Parent = aParent;

    m_LibId.SetLibNickname( aItem->GetLibNickname() );
    m_LibId.SetLibItemName( aItem->GetName() );

    m_Name = aItem->GetName();
    m_Desc = aItem->GetDescription();
    m_Footprint = aItem->GetFootprint();

    aItem->GetChooserFields( m_Fields );

    m_SearchTerms = aItem->GetSearchTerms();

    m_IsRoot = aItem->IsRoot();

    if( aItem->GetUnitCount() > 1 )
    {
        for( int u = 1; u <= aItem->GetUnitCount(); ++u )
            AddUnit( aItem, u );
    }
}


LIB_TREE_NODE_UNIT& LIB_TREE_NODE_LIB_ID::AddUnit( LIB_TREE_ITEM* aItem, int aUnit )
{
    LIB_TREE_NODE_UNIT* unit = new LIB_TREE_NODE_UNIT( this, aItem, aUnit );
    m_Children.push_back( std::unique_ptr<LIB_TREE_NODE>( unit ) );
    return *unit;
}


void LIB_TREE_NODE_LIB_ID::Update( LIB_TREE_ITEM* aItem )
{
    m_LibId.SetLibNickname( aItem->GetLibId().GetLibNickname() );
    m_LibId.SetLibItemName( aItem->GetName() );

    m_Name = aItem->GetName();
    m_Desc = aItem->GetDescription();

    aItem->GetChooserFields( m_Fields );

    m_SearchTerms = aItem->GetSearchTerms();

    m_IsRoot = aItem->IsRoot();
    m_Children.clear();

    for( int u = 1; u <= aItem->GetUnitCount(); ++u )
        AddUnit( aItem, u );
}


void LIB_TREE_NODE_LIB_ID::UpdateScore( EDA_COMBINED_MATCHER& aMatcher, const wxString& aLib )
{
    if( m_Score <= 0 )
        return; // Leaf nodes without scores are out of the game.

    if( !aLib.IsEmpty() && m_Parent->m_Name.Lower() != aLib )
    {
        m_Score = 0;
        return;
    }

    m_Score = aMatcher.ScoreTerms( m_SearchTerms );
}


LIB_TREE_NODE_LIB::LIB_TREE_NODE_LIB( LIB_TREE_NODE* aParent, wxString const& aName,
                                      wxString const& aDesc )
{
    m_Type = LIB;
    m_Name = aName;
    m_Desc = aDesc;
    m_Parent = aParent;
    m_LibId.SetLibNickname( aName );
}


LIB_TREE_NODE_LIB_ID& LIB_TREE_NODE_LIB::AddItem( LIB_TREE_ITEM* aItem )
{
    LIB_TREE_NODE_LIB_ID* item = new LIB_TREE_NODE_LIB_ID( this, aItem );
    m_Children.push_back( std::unique_ptr<LIB_TREE_NODE>( item ) );
    return *item;
}


void LIB_TREE_NODE_LIB::UpdateScore( EDA_COMBINED_MATCHER& aMatcher, const wxString& aLib )
{
    m_Score = 0;

    // We need to score leaf nodes, which are usually (but not always) children.

    if( m_Children.size() )
    {
        for( std::unique_ptr<LIB_TREE_NODE>& child: m_Children )
        {
            child->UpdateScore( aMatcher, aLib );
            m_Score = std::max( m_Score, child->m_Score );
        }
    }
    else
    {
        // No children; we are a leaf.

        m_Score = aMatcher.ScoreTerms( m_SearchTerms );
    }
}


LIB_TREE_NODE_ROOT::LIB_TREE_NODE_ROOT()
{
    m_Type = ROOT;
}


LIB_TREE_NODE_LIB& LIB_TREE_NODE_ROOT::AddLib( wxString const& aName, wxString const& aDesc )
{
    LIB_TREE_NODE_LIB* lib = new LIB_TREE_NODE_LIB( this, aName, aDesc );
    m_Children.push_back( std::unique_ptr<LIB_TREE_NODE>( lib ) );
    return *lib;
}


void LIB_TREE_NODE_ROOT::UpdateScore( EDA_COMBINED_MATCHER& aMatcher, const wxString& aLib )
{
    for( std::unique_ptr<LIB_TREE_NODE>& child: m_Children )
        child->UpdateScore( aMatcher, aLib );
}


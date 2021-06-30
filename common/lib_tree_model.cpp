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

#include <lib_tree_model.h>

#include <algorithm>
#include <eda_pattern_match.h>
#include <lib_tree_item.h>
#include <utility>
#include <pgm_base.h>
#include <kicad_string.h>

// Each node gets this lowest score initially, without any matches applied.
// Matches will then increase this score depending on match quality.  This way,
// an empty search string will result in all components being displayed as they
// have the minimum score. However, in that case, we avoid expanding all the
// nodes asd the result is very unspecific.
static const unsigned kLowestDefaultScore = 1;


// Creates a score depending on the position of a string match. If the position
// is 0 (= prefix match), this returns the maximum score. This degrades until
// pos == max, which returns a score of 0; Evertyhing else beyond that is just
// 0. Only values >= 0 allowed for position and max.
//
// @param aPosition is the position a string has been found in a substring.
// @param aMaximum is the maximum score this function returns.
// @return position dependent score.
static int matchPosScore(int aPosition, int aMaximum)
{
    return ( aPosition < aMaximum ) ? aMaximum - aPosition : 0;
}


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


void LIB_TREE_NODE::SortNodes()
{
    std::sort( m_Children.begin(), m_Children.end(),
               []( std::unique_ptr<LIB_TREE_NODE>& a, std::unique_ptr<LIB_TREE_NODE>& b )
               {
                   return Compare( *a, *b ) > 0;
               } );

    for( std::unique_ptr<LIB_TREE_NODE>& node: m_Children )
        node->SortNodes();
}


int LIB_TREE_NODE::Compare( LIB_TREE_NODE const& aNode1, LIB_TREE_NODE const& aNode2 )
{
    if( aNode1.m_Type != aNode2.m_Type )
        return 0;

    if( aNode1.m_Score != aNode2.m_Score )
        return aNode1.m_Score - aNode2.m_Score;

    if( aNode1.m_Parent != aNode2.m_Parent )
        return 0;

    return aNode1.m_IntrinsicRank - aNode2.m_IntrinsicRank;
}


LIB_TREE_NODE::LIB_TREE_NODE()
    : m_Parent( nullptr ),
      m_Type( INVALID ),
      m_IntrinsicRank( 0 ),
      m_Score( kLowestDefaultScore ),
      m_Pinned( false ),
      m_Normalized( false ),
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
    m_Desc = wxEmptyString;
    m_MatchName = wxEmptyString;

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

    m_MatchName = aItem->GetName();
    m_SearchText = aItem->GetSearchText();
    m_Normalized = false;

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
    // Update is called when the names match, so just update the other fields.

    m_LibId.SetLibNickname( aItem->GetLibId().GetLibNickname() );

    m_Desc = aItem->GetDescription();

    m_SearchText = aItem->GetSearchText();
    m_Normalized = false;

    m_IsRoot = aItem->IsRoot();
    m_Children.clear();

    for( int u = 1; u <= aItem->GetUnitCount(); ++u )
        AddUnit( aItem, u );
}


void LIB_TREE_NODE_LIB_ID::UpdateScore( EDA_COMBINED_MATCHER& aMatcher )
{
    if( m_Score <= 0 )
        return; // Leaf nodes without scores are out of the game.

    if( !m_Normalized )
    {
        m_MatchName = UnescapeString( m_MatchName ).Lower();
        m_SearchText = m_SearchText.Lower();
        m_Normalized = true;
    }

    // Keywords and description we only count if the match string is at
    // least two characters long. That avoids spurious, low quality
    // matches. Most abbreviations are at three characters long.
    int found_pos = EDA_PATTERN_NOT_FOUND;
    int matchers_fired = 0;

    if( aMatcher.GetPattern() == m_MatchName )
    {
        m_Score += 1000;  // exact match. High score :)
    }
    else if( aMatcher.Find( m_MatchName, matchers_fired, found_pos ) )
    {
        // Substring match. The earlier in the string the better.
        m_Score += matchPosScore( found_pos, 20 ) + 20;
    }
    else if( aMatcher.Find( m_Parent->m_MatchName, matchers_fired, found_pos ) )
    {
        m_Score += 19;   // parent name matches.         score += 19
    }
    else if( aMatcher.Find( m_SearchText, matchers_fired, found_pos ) )
    {
        // If we have a very short search term (like one or two letters),
        // we don't want to accumulate scores if they just happen to be in
        // keywords or description as almost any one or two-letter
        // combination shows up in there.
        if( aMatcher.GetPattern().length() >= 2 )
        {
            // For longer terms, we add scores 1..18 for positional match
            // (higher in the front, where the keywords are).
            m_Score += matchPosScore( found_pos, 17 ) + 1;
        }
    }
    else
    {
        // No match. That's it for this item.
        m_Score = 0;
    }

    // More matchers = better match
    m_Score += 2 * matchers_fired;
}


LIB_TREE_NODE_LIB::LIB_TREE_NODE_LIB( LIB_TREE_NODE* aParent, wxString const& aName,
                                      wxString const& aDesc )
{
    m_Type = LIB;
    m_Name = aName;
    m_MatchName = aName.Lower();
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


void LIB_TREE_NODE_LIB::UpdateScore( EDA_COMBINED_MATCHER& aMatcher )
{
    m_Score = 0;

    // We need to score leaf nodes, which are usually (but not always) children.

    if( m_Children.size() )
    {
    for( std::unique_ptr<LIB_TREE_NODE>& child: m_Children )
        {
            child->UpdateScore( aMatcher );
            m_Score = std::max( m_Score, child->m_Score );
        }
    }
    else
    {
        // No children; we are a leaf.
        int found_pos = EDA_PATTERN_NOT_FOUND;
        int matchers_fired = 0;

        if( aMatcher.GetPattern() == m_MatchName )
        {
            m_Score += 1000;  // exact match. High score :)
        }
        else if( aMatcher.Find( m_MatchName, matchers_fired, found_pos ) )
        {
            // Substring match. The earlier in the string the better.
            m_Score += matchPosScore( found_pos, 20 ) + 20;
        }

        // More matchers = better match
        m_Score += 2 * matchers_fired;
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


void LIB_TREE_NODE_ROOT::UpdateScore( EDA_COMBINED_MATCHER& aMatcher )
{
    for( std::unique_ptr<LIB_TREE_NODE>& child: m_Children )
        child->UpdateScore( aMatcher );
}


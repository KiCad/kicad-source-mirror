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
    for( auto& child: Children )
        child->ResetScore();

    Score = kLowestDefaultScore;
}


void LIB_TREE_NODE::AssignIntrinsicRanks( bool presorted )
{
    std::vector<LIB_TREE_NODE*> sort_buf;

    if( presorted )
    {
        int max = Children.size() - 1;

        for( int i = 0; i <= max; ++i )
            Children[i]->IntrinsicRank = max - i;
    }
    else
    {
        for( auto const& node: Children )
            sort_buf.push_back( &*node );

        std::sort( sort_buf.begin(), sort_buf.end(),
                []( LIB_TREE_NODE* a, LIB_TREE_NODE* b ) -> bool
                    { return StrNumCmp( a->Name, b->Name, true ) > 0; } );

        for( int i = 0; i < (int) sort_buf.size(); ++i )
            sort_buf[i]->IntrinsicRank = i;
    }
}


void LIB_TREE_NODE::SortNodes()
{
    std::sort( Children.begin(), Children.end(),
            []( std::unique_ptr<LIB_TREE_NODE> const& a, std::unique_ptr<LIB_TREE_NODE> const& b )
                { return Compare( *a, *b ) > 0; } );

    for( auto& node: Children )
    {
        node->SortNodes();
    }
}


int LIB_TREE_NODE::Compare( LIB_TREE_NODE const& aNode1, LIB_TREE_NODE const& aNode2 )
{
    if( aNode1.Type != aNode2.Type )
        return 0;

    if( aNode1.Score != aNode2.Score )
        return aNode1.Score - aNode2.Score;

    if( aNode1.Parent != aNode2.Parent )
        return 0;

    return aNode1.IntrinsicRank - aNode2.IntrinsicRank;
}


LIB_TREE_NODE::LIB_TREE_NODE()
    : Parent( nullptr ),
      Type( INVALID ),
      IntrinsicRank( 0 ),
      Score( kLowestDefaultScore ),
      Normalized( false ),
      Unit( 0 ),
      IsRoot( false ),
      VisLen( 0 )
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

    Parent = aParent;
    Type = UNIT;

    Unit = aUnit;
    LibId = aParent->LibId;

    Name = namePrefix + " " + aItem->GetUnitReference( aUnit );
    Desc = wxEmptyString;
    MatchName = wxEmptyString;

    IntrinsicRank = -aUnit;
}


LIB_TREE_NODE_LIB_ID::LIB_TREE_NODE_LIB_ID( LIB_TREE_NODE* aParent, LIB_TREE_ITEM* aItem )
{
    Type = LIBID;
    Parent = aParent;

    LibId.SetLibNickname( aItem->GetLibNickname() );
    LibId.SetLibItemName( aItem->GetName () );

    Name = aItem->GetName();
    Desc = aItem->GetDescription();

    MatchName = aItem->GetName();
    SearchText = aItem->GetSearchText();
    Normalized = false;

    IsRoot = aItem->IsRoot();

    if( aItem->GetUnitCount() > 1 )
    {
        for( int u = 1; u <= aItem->GetUnitCount(); ++u )
            AddUnit( aItem, u );
    }
}


LIB_TREE_NODE_UNIT& LIB_TREE_NODE_LIB_ID::AddUnit( LIB_TREE_ITEM* aItem, int aUnit )
{
    LIB_TREE_NODE_UNIT* unit = new LIB_TREE_NODE_UNIT( this, aItem, aUnit );
    Children.push_back( std::unique_ptr<LIB_TREE_NODE>( unit ) );
    return *unit;
}


void LIB_TREE_NODE_LIB_ID::Update( LIB_TREE_ITEM* aItem )
{
    // Update is called when the names match, so just update the other fields.

    LibId.SetLibNickname( aItem->GetLibId().GetLibNickname() );

    Desc = aItem->GetDescription();

    SearchText = aItem->GetSearchText();
    Normalized = false;

    IsRoot = aItem->IsRoot();
    Children.clear();

    for( int u = 1; u <= aItem->GetUnitCount(); ++u )
        AddUnit( aItem, u );
}


void LIB_TREE_NODE_LIB_ID::UpdateScore( EDA_COMBINED_MATCHER& aMatcher )
{
    if( Score <= 0 )
        return; // Leaf nodes without scores are out of the game.

    if( !Normalized )
    {
        MatchName = MatchName.Lower();
        SearchText = SearchText.Lower();
        Normalized = true;
    }

    // Keywords and description we only count if the match string is at
    // least two characters long. That avoids spurious, low quality
    // matches. Most abbreviations are at three characters long.
    int found_pos = EDA_PATTERN_NOT_FOUND;
    int matchers_fired = 0;

    if( aMatcher.GetPattern() == MatchName )
    {
        Score += 1000;  // exact match. High score :)
    }
    else if( aMatcher.Find( MatchName, matchers_fired, found_pos ) )
    {
        // Substring match. The earlier in the string the better.
        Score += matchPosScore( found_pos, 20 ) + 20;
    }
    else if( aMatcher.Find( Parent->MatchName, matchers_fired, found_pos ) )
    {
        Score += 19;   // parent name matches.         score += 19
    }
    else if( aMatcher.Find( SearchText, matchers_fired, found_pos ) )
    {
        // If we have a very short search term (like one or two letters),
        // we don't want to accumulate scores if they just happen to be in
        // keywords or description as almost any one or two-letter
        // combination shows up in there.
        if( aMatcher.GetPattern().length() >= 2 )
        {
            // For longer terms, we add scores 1..18 for positional match
            // (higher in the front, where the keywords are).
            Score += matchPosScore( found_pos, 17 ) + 1;
        }
    }
    else
    {
        // No match. That's it for this item.
        Score = 0;
    }

    // More matchers = better match
    Score += 2 * matchers_fired;
}


LIB_TREE_NODE_LIB::LIB_TREE_NODE_LIB( LIB_TREE_NODE* aParent, wxString const& aName,
                                      wxString const& aDesc )
{
    Type = LIB;
    Name = aName;
    MatchName = aName.Lower();
    Desc = aDesc;
    Parent = aParent;
    LibId.SetLibNickname( aName );
}


LIB_TREE_NODE_LIB_ID& LIB_TREE_NODE_LIB::AddItem( LIB_TREE_ITEM* aItem )
{
    LIB_TREE_NODE_LIB_ID* item = new LIB_TREE_NODE_LIB_ID( this, aItem );
    Children.push_back( std::unique_ptr<LIB_TREE_NODE>( item ) );
    return *item;
}


void LIB_TREE_NODE_LIB::UpdateScore( EDA_COMBINED_MATCHER& aMatcher )
{
    Score = 0;

    // We need to score leaf nodes, which are usually (but not always) children.

    if( Children.size() )
    {
        for( auto& child: Children )
        {
            child->UpdateScore( aMatcher );
            Score = std::max( Score, child->Score );
        }
    }
    else
    {
        // No children; we are a leaf.
        int found_pos = EDA_PATTERN_NOT_FOUND;
        int matchers_fired = 0;

        if( aMatcher.GetPattern() == MatchName )
        {
            Score += 1000;  // exact match. High score :)
        }
        else if( aMatcher.Find( MatchName, matchers_fired, found_pos ) )
        {
            // Substring match. The earlier in the string the better.
            Score += matchPosScore( found_pos, 20 ) + 20;
        }

        // More matchers = better match
        Score += 2 * matchers_fired;
    }
}


LIB_TREE_NODE_ROOT::LIB_TREE_NODE_ROOT()
{
    Type = ROOT;
}


LIB_TREE_NODE_LIB& LIB_TREE_NODE_ROOT::AddLib( wxString const& aName, wxString const& aDesc )
{
    LIB_TREE_NODE_LIB* lib = new LIB_TREE_NODE_LIB( this, aName, aDesc );
    Children.push_back( std::unique_ptr<LIB_TREE_NODE>( lib ) );
    return *lib;
}


void LIB_TREE_NODE_ROOT::UpdateScore( EDA_COMBINED_MATCHER& aMatcher )
{
    for( auto& child: Children )
        child->UpdateScore( aMatcher );
}


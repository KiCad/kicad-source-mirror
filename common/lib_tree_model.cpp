/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright (C) 2023 CERN
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



void LIB_TREE_NODE::ResetScore()
{
    for( std::unique_ptr<LIB_TREE_NODE>& child: m_Children )
        child->ResetScore();

    m_Score = 0;
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
            // Make sure -- Recently Used is always at the top
            // Start by checking the name of aNode2, because we
            // want to satisfy the irreflexive property of the
            // strict weak ordering.
            if( aNode2.m_Name.StartsWith( wxT( "-- Recently Used" ) ) )
                return false;
            else if( aNode1.m_Name.StartsWith( wxT( "-- Recently Used" ) ) )
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
    m_Type = TYPE::UNIT;

    m_Unit = aUnit;
    m_LibId = aParent->m_LibId;

    m_Name = namePrefix + " " + aItem->GetUnitReference( aUnit );

    if( aItem->HasUnitDisplayName( aUnit ) )
        m_Desc = aItem->GetUnitDisplayName( aUnit );
    else
        m_Desc = wxEmptyString;

    m_IntrinsicRank = -aUnit;
}


void LIB_TREE_NODE_UNIT::UpdateScore( EDA_COMBINED_MATCHER* aMatcher, const wxString& aLib,
                                      std::function<bool( LIB_TREE_NODE& aNode )>* aFilter )
{
    // aMatcher test results are inherited from parent
    if( aMatcher )
        m_Score = m_Parent->m_Score;

    // aFilter test is subtractive
    if( aFilter && !(*aFilter)(*this) )
        m_Score = 0;

    // show all nodes if no search/filter/etc. criteria are given
    if( !aMatcher && aLib.IsEmpty() && ( !aFilter || (*aFilter)(*this) ) )
        m_Score = 1;
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

    m_SearchTerms = aItem->GetSearchTerms();

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

    m_SearchTerms = aItem->GetSearchTerms();

    m_IsRoot = aItem->IsRoot();
    m_Children.clear();

    for( int u = 1; u <= aItem->GetSubUnitCount(); ++u )
        AddUnit( aItem, u );
}


void LIB_TREE_NODE_ITEM::UpdateScore( EDA_COMBINED_MATCHER* aMatcher, const wxString& aLib,
                                      std::function<bool( LIB_TREE_NODE& aNode )>* aFilter )
{
    // aMatcher test is additive, but if we don't match the given term at all, it nulls out
    if( aMatcher )
    {
        int currentScore = aMatcher->ScoreTerms( m_SearchTerms );

        // This is a hack: the second phase of search in the adapter will look for a tokenized
        // LIB_ID and send the lib part down here.  While we generally want to prune ourselves
        // out here (by setting score to -1) the first time we fail to match a search term,
        // we want to give the same search term a second chance if it has been split from a library
        // name.
        if( ( m_Score >= 0 || !aLib.IsEmpty() ) && currentScore > 0 )
            m_Score += currentScore;
        else
            m_Score = -1;   // Item has failed to match this term, rule it out
    }

    // aFilter test is subtractive
    if( aFilter && !(*aFilter)(*this) )
        m_Score = 0;

    // show all nodes if no search/filter/etc. criteria are given
    if( !aMatcher && aLib.IsEmpty() && ( !aFilter || (*aFilter)(*this) ) )
        m_Score = 1;

    for( std::unique_ptr<LIB_TREE_NODE>& child: m_Children )
        child->UpdateScore( aMatcher, aLib, aFilter );
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


void LIB_TREE_NODE_LIBRARY::UpdateScore( EDA_COMBINED_MATCHER* aMatcher, const wxString& aLib,
                                         std::function<bool( LIB_TREE_NODE& aNode )>* aFilter )
{
    int maxChildScore = 0;

    for( std::unique_ptr<LIB_TREE_NODE>& child: m_Children )
    {
        child->UpdateScore( aMatcher, aLib, aFilter );
        maxChildScore = std::max( maxChildScore, child->m_Score );
    }

    // Each time UpdateScore is called for a library, child (item) scores may go up or down.
    // If the all go down to zero, we need to make sure to drop the library from the list.
    if( maxChildScore > 0 )
        m_Score = std::max( m_Score, maxChildScore );
    else
        m_Score = 0;

    // aLib test is additive, but only when we've already accumulated some score from children
    if( !aLib.IsEmpty()
        && m_Name.Lower().Matches( aLib )
        && ( m_Score > 0 || m_Children.empty() ) )
    {
        m_Score += 1;
    }

    // aMatcher test is additive
    if( aMatcher )
    {
        int ownScore = aMatcher->ScoreTerms( m_SearchTerms );
        m_Score += ownScore;

        // If we have a hit on a library, show all children in that library
        if( maxChildScore <= 0 && ownScore > 0 )
        {
            for( std::unique_ptr<LIB_TREE_NODE>& child: m_Children )
                child->ForceScore( 1 );
        }
    }

    // show all nodes if no search/filter/etc. criteria are given
    if( m_Children.empty() && !aMatcher && aLib.IsEmpty() && ( !aFilter || (*aFilter)(*this) ) )
        m_Score = 1;
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


void LIB_TREE_NODE_ROOT::UpdateScore( EDA_COMBINED_MATCHER* aMatcher, const wxString& aLib,
                                      std::function<bool( LIB_TREE_NODE& aNode )>* aFilter )
{
    for( std::unique_ptr<LIB_TREE_NODE>& child: m_Children )
        child->UpdateScore( aMatcher, aLib, aFilter );
}


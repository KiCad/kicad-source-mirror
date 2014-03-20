/* -*- c++ -*-
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright (C) 2014 KiCad Developers, see change_log.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */
#include <component_tree_search_container.h>

#include <algorithm>
#include <boost/foreach.hpp>
#include <set>

#include <wx/string.h>
#include <wx/tokenzr.h>
#include <wx/treectrl.h>
#include <wx/arrstr.h>

#include <class_library.h>
#include <macros.h>

// Each node gets this lowest score initially, without any matches applied. Matches
// will then increase this score depending on match quality.
// This way, an empty search string will result in all components being displayed as they
// have the minimum score. However, in that case, we avoid expanding all the nodes asd the
// result is very unspecific.
static const unsigned kLowestDefaultScore = 1;

struct COMPONENT_TREE_SEARCH_CONTAINER::TREE_NODE
{
    // Levels of nodes.
    enum NODE_TYPE {
        TYPE_LIB,
        TYPE_ALIAS,
        TYPE_UNIT
    };

    TREE_NODE(NODE_TYPE aType, TREE_NODE* aParent, LIB_ALIAS* aAlias,
              const wxString& aName, const wxString& aDisplayInfo,
              const wxString& aSearchText )
        : Type( aType ),
          Parent( aParent ), Alias( aAlias ), Unit( 0 ),
          DisplayName( aName ),
          DisplayInfo( aDisplayInfo ),
          MatchName( aName.Lower() ),
          SearchText( aSearchText.Lower() ),
          MatchScore( 0 ), PreviousScore( 0 )
    {
    }

    const NODE_TYPE Type;         ///< Type of node in the hierarchy.
    TREE_NODE* const Parent;      ///< NULL if library, pointer to parent when component/alias.
    LIB_ALIAS* const Alias;       ///< Component alias associated with this entry.
    int Unit;                     ///< Part number; Assigned: >= 1; default = 0
    const wxString DisplayName;   ///< Exact name as displayed to the user.
    const wxString DisplayInfo;   ///< Additional info displayed in the tree (description..)

    const wxString MatchName;     ///< Preprocessed: lowercased display name.
    const wxString SearchText;    ///< Other text (keywords, description..) to search in.

    unsigned MatchScore;          ///< Result-Score after UpdateSearchTerm()
    unsigned PreviousScore;       ///< Optimization: used to see if we need any tree update.
    wxTreeItemId TreeId;          ///< Tree-ID if stored in the tree (if MatchScore > 0).
};


// Sort tree nodes by reverse match-score (bigger is first), then alphabetically.
// Library (i.e. the ones that don't have a parent) are always sorted before any
// leaf nodes. Component
bool COMPONENT_TREE_SEARCH_CONTAINER::scoreComparator( const TREE_NODE* a1, const TREE_NODE* a2 )
{
    if( a1->Type != a2->Type )
        return a1->Type < a2->Type;

    if( a1->MatchScore != a2->MatchScore )
        return a1->MatchScore > a2->MatchScore;  // biggest first.

    if( a1->Parent != a2->Parent )
        return scoreComparator( a1->Parent, a2->Parent );

    return a1->MatchName.Cmp( a2->MatchName ) < 0;
}


COMPONENT_TREE_SEARCH_CONTAINER::COMPONENT_TREE_SEARCH_CONTAINER()
    : tree( NULL ), libraries_added( 0 ), preselect_unit_number( -1 )
{
}


COMPONENT_TREE_SEARCH_CONTAINER::~COMPONENT_TREE_SEARCH_CONTAINER()
{
    BOOST_FOREACH( TREE_NODE* node, nodes )
        delete node;
    nodes.clear();
}


void COMPONENT_TREE_SEARCH_CONTAINER::SetPreselectNode( const wxString& aComponentName,
                                                        int aUnit )
{
    preselect_node_name = aComponentName.Lower();
    preselect_unit_number = aUnit;
}


void COMPONENT_TREE_SEARCH_CONTAINER::SetTree( wxTreeCtrl* aTree )
{
    tree = aTree;
    UpdateSearchTerm( wxEmptyString );
}


void COMPONENT_TREE_SEARCH_CONTAINER::AddLibrary( CMP_LIBRARY& aLib )
{
    wxArrayString all_aliases;

    aLib.GetEntryNames( all_aliases );
    AddAliasList( aLib.GetName(), all_aliases, &aLib );
    ++libraries_added;
}


void COMPONENT_TREE_SEARCH_CONTAINER::AddAliasList( const wxString& aNodeName,
                                                    const wxArrayString& aAliasNameList,
                                                    CMP_LIBRARY* aOptionalLib )
{
    TREE_NODE* const lib_node = new TREE_NODE( TREE_NODE::TYPE_LIB,  NULL, NULL,
                                               aNodeName, wxEmptyString, wxEmptyString );
    nodes.push_back( lib_node );

    BOOST_FOREACH( const wxString& aName, aAliasNameList )
    {
        LIB_ALIAS* a;

        if( aOptionalLib )
            a = aOptionalLib->FindAlias( aName );
        else
            a = CMP_LIBRARY::FindLibraryEntry( aName, wxEmptyString );

        if( a == NULL )
            continue;

        wxString search_text;
        search_text = ( a->GetKeyWords().empty() ) ? wxT("        ") : a->GetKeyWords();
        search_text += a->GetDescription();

        wxString display_info;

        if( !a->GetDescription().empty() )
        {
            // Preformatting. Unfortunately, the tree widget doesn't have columns
            // and using tabs does not work very well or does not work at all
            // (depending on OS versions). So indent with spaces in fixed-font width.

            // The 98%-ile of length of strings found in the standard library is 15
            // characters. Use this as a reasonable cut-off point for aligned indentation.
            // For the few component names longer than that, the description is indented a
            // bit more.
            // The max found in the default lib would be 20 characters, but that creates too
            // much visible whitespace for the less extreme component names.
            const int COLUMN_DESCR_POS = 15;
            const int indent_len = COLUMN_DESCR_POS - a->GetName().length();
            display_info = wxString::Format( wxT( " %*s [ %s ]" ),
                                             indent_len > 0 ? indent_len : 0, wxT( "" ),
                                             GetChars( a->GetDescription() ) );
        }

        TREE_NODE* alias_node = new TREE_NODE( TREE_NODE::TYPE_ALIAS, lib_node,
                                               a, a->GetName(), display_info, search_text );
        nodes.push_back( alias_node );

        if( a->GetComponent()->IsMulti() )    // Add all units as sub-nodes.
        {
            for( int u = 1; u <= a->GetComponent()->GetPartCount(); ++u )
            {
                wxString unitName = _("Unit");
                unitName += wxT( " " ) + LIB_COMPONENT::SubReference( u, false );
                TREE_NODE* unit_node = new TREE_NODE( TREE_NODE::TYPE_UNIT,
                                                      alias_node, a,
                                                      unitName,
                                                      wxEmptyString, wxEmptyString );
                unit_node->Unit = u;
                nodes.push_back( unit_node );
            }
        }
    }
}


LIB_ALIAS* COMPONENT_TREE_SEARCH_CONTAINER::GetSelectedAlias( int* aUnit )
{
    if( tree == NULL )
        return NULL;

    const wxTreeItemId& select_id = tree->GetSelection();

    BOOST_FOREACH( TREE_NODE* node, nodes )
    {
        if( node->MatchScore > 0 && node->TreeId == select_id ) {
            if( aUnit && node->Unit > 0 )
                *aUnit = node->Unit;
            return node->Alias;
        }
    }
    return NULL;
}


// Creates a score depending on the position of a string match. If the position
// is 0 (= prefix match), this returns the maximum score. This degrades until
// pos == max, which returns a score of 0;
// Evertyhing else beyond that is just 0. Only values >= 0 allowed for position and max.
//
// @param aPosition is the position a string has been found in a substring.
// @param aMaximum is the maximum score this function returns.
// @return position dependent score.
static int matchPosScore(int aPosition, int aMaximum)
{
    return ( aPosition < aMaximum ) ? aMaximum - aPosition : 0;
}


void COMPONENT_TREE_SEARCH_CONTAINER::UpdateSearchTerm( const wxString& aSearch )
{
    if( tree == NULL )
        return;

    // We score the list by going through it several time, essentially with a complexity
    // of O(n). For the default library of 2000+ items, this typically takes less than 5ms
    // on an i5. Good enough, no index needed.

    // Initial AND condition: Leaf nodes are considered to match initially.
    BOOST_FOREACH( TREE_NODE* node, nodes )
    {
        node->PreviousScore = node->MatchScore;
        node->MatchScore = ( node->Type == TREE_NODE::TYPE_LIB ) ? 0 : kLowestDefaultScore;
    }

    // Create match scores for each node for all the terms, that come space-separated.
    // Scoring adds up values for each term according to importance of the match. If a term does
    // not match at all, the result is thrown out of the results (AND semantics).
    // From high to low
    //   - Exact match for a ccmponent name gives the highest score, trumping all.
    //   - A positional score depending of where a term is found as substring; prefix-match: high.
    //   - substring-match in library name.
    //   - substring match in keywords and descriptions with positional score. Keywords come
    //     first so contribute more to the score.
    //
    // This is of course subject to tweaking.
    wxStringTokenizer tokenizer( aSearch );

    while ( tokenizer.HasMoreTokens() )
    {
        const wxString term = tokenizer.GetNextToken().Lower();

        BOOST_FOREACH( TREE_NODE* node, nodes )
        {
            if( node->Type != TREE_NODE::TYPE_ALIAS )
                continue;      // Only aliases are actually scored here.

            if( node->MatchScore == 0)
                continue;   // Leaf node without score are out of the game.

            // Keywords and description we only count if the match string is at
            // least two characters long. That avoids spurious, low quality
            // matches. Most abbreviations are at three characters long.
            int found_pos;

            if( term == node->MatchName )
                node->MatchScore += 1000;  // exact match. High score :)
            else if( (found_pos = node->MatchName.Find( term ) ) != wxNOT_FOUND )
            {
                // Substring match. The earlier in the string the better.  score += 20..40
                node->MatchScore += matchPosScore( found_pos, 20 ) + 20;
            }
            else if( node->Parent->MatchName.Find( term ) != wxNOT_FOUND )
                node->MatchScore += 19;   // parent name matches.         score += 19
            else if( ( found_pos = node->SearchText.Find( term ) ) != wxNOT_FOUND )
            {
                // If we have a very short search term (like one or two letters), we don't want
                // to accumulate scores if they just happen to be in keywords or description as
                // almost any one or two-letter combination shows up in there.
                // For longer terms, we add scores 1..18 for positional match (higher in the
                // front, where the keywords are).                        score += 0..18
                node->MatchScore += ( ( term.length() >= 2 )
                                       ? matchPosScore( found_pos, 17 ) + 1
                                       : 0 );
            }
            else
                node->MatchScore = 0;    // No match. That's it for this item.
        }
    }

    // Library nodes have the maximum score seen in any of their children.
    // Alias nodes have the score of their parents.
    unsigned highest_score_seen = 0;
    bool any_change = false;

    BOOST_FOREACH( TREE_NODE* node, nodes )
    {
        switch( node->Type )
        {
        case TREE_NODE::TYPE_ALIAS:
            {
                any_change |= (node->PreviousScore != node->MatchScore);
                // Update library score.
                node->Parent->MatchScore = std::max( node->Parent->MatchScore, node->MatchScore );
                highest_score_seen = std::max( highest_score_seen, node->MatchScore );
            }
            break;

        case TREE_NODE::TYPE_UNIT:
            node->MatchScore = node->Parent->MatchScore;
            break;

        default:
            break;
        }
    }

    // The tree update might be slow, so we want to bail out if there is no change.
    if( !any_change )
        return;

    // Now: sort all items according to match score, libraries first.
    std::sort( nodes.begin(), nodes.end(), scoreComparator );

    // Fill the tree with all items that have a match. Re-arranging, adding and removing changed
    // items is pretty complex, so we just re-build the whole tree.
    tree->Freeze();
    tree->DeleteAllItems();
    const wxTreeItemId root_id = tree->AddRoot( wxEmptyString );
    const TREE_NODE* first_match = NULL;
    const TREE_NODE* preselected_node = NULL;

    BOOST_FOREACH( TREE_NODE* node, nodes )
    {
        if( node->MatchScore == 0 )
            continue;

        // If we have nodes that go beyond the default score, suppress nodes that
        // have the default score. That can happen if they have an honary += 0 score due to
        // some one-letter match in the keyword or description. In this case, we prefer matches
        // that just have higher scores. Improves relevancy and performance as the tree has to
        // display less items.
        if( highest_score_seen > kLowestDefaultScore && node->MatchScore == kLowestDefaultScore )
            continue;

        wxString node_text;
#if 0
        // Node text with scoring information for debugging
        node_text.Printf( wxT("%s (s=%u)%s"), GetChars(node->DisplayName),
                          node->MatchScore, GetChars( node->DisplayInfo ));
#else
        node_text = node->DisplayName + node->DisplayInfo;
#endif
        node->TreeId = tree->AppendItem( node->Parent ? node->Parent->TreeId : root_id,
                                         node_text );

        // If we are a nicely scored alias, we want to have it visible. Also, if there
        // is only a single library in this container, we want to have it unfolded
        // (example: power library).
        if( node->Type == TREE_NODE::TYPE_ALIAS
             && ( node->MatchScore > kLowestDefaultScore || libraries_added == 1 ) )
        {
            tree->EnsureVisible( node->TreeId );

            if( first_match == NULL )
                first_match = node;   // First, highest scoring: the "I am feeling lucky" element.
        }

        // The first node that matches our pre-select criteria is choosen. 'First node'
        // means, it shows up in the history, as the history node is displayed very first
        // (by virtue of alphabetical ordering)
        if( preselected_node == NULL
             && node->Type == TREE_NODE::TYPE_ALIAS
             && node->MatchName == preselect_node_name )
            preselected_node = node;

        // Refinement in case we come accross a matching unit node.
        if( preselected_node != NULL && preselected_node->Type == TREE_NODE::TYPE_ALIAS
             && node->Parent == preselected_node
             && preselect_unit_number >= 1 && node->Unit == preselect_unit_number )
            preselected_node = node;
    }

    if( first_match )                      // Highest score search match pre-selected.
        tree->SelectItem( first_match->TreeId );
    else if( preselected_node )            // No search, so history item preselected.
        tree->SelectItem( preselected_node->TreeId );

    tree->Thaw();
}

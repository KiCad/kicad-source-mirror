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
    TREE_NODE(TREE_NODE* aParent, CMP_LIBRARY* aOwningLib,
              const wxString& aName, const wxString& aDisplayInfo,
              const wxString& aSearchText,
              bool aNormallyExpanded = false)
        : Parent( aParent ),
          Lib( aOwningLib ),
          NormallyExpanded( aNormallyExpanded ),
          Name( aName ),
          DisplayInfo( aDisplayInfo ),
          MatchName( aName.Lower() ),
          SearchText( aSearchText.Lower() ),
          MatchScore( 0 ), PreviousScore( 0 )
    {
    }

    TREE_NODE* const Parent;      ///< NULL if library, pointer to lib-node when component.
    CMP_LIBRARY* const Lib;       ///< Owning library of this component.
    const bool NormallyExpanded;  ///< If this is a parent node, should it be unfolded ?
    const wxString Name;          ///< Exact name as displayed to the user.
    const wxString DisplayInfo;   ///< Additional info displayed in the tree (description..)

    const wxString MatchName;     ///< Preprocessed: lowercased display name.
    const wxString SearchText;    ///< Other text (keywords, description..) to search in.

    unsigned MatchScore;          ///< Result-Score after UpdateSearchTerm()
    unsigned PreviousScore;       ///< Optimization: used to see if we need any tree update.
    wxTreeItemId TreeId;          ///< Tree-ID if stored in the tree (if MatchScore > 0).
};


// Sort tree nodes by reverse match-score (bigger is first), then alphabetically.
// Library nodes (i.e. the ones that don't have a parent) are always sorted before any
// leaf nodes.
bool COMPONENT_TREE_SEARCH_CONTAINER::scoreComparator( const TREE_NODE* a1, const TREE_NODE* a2 )
{
    if ( a1->Parent == NULL && a2->Parent != NULL )
        return true;

    if ( a1->Parent != NULL && a2->Parent == NULL )
        return false;

    if ( a1->MatchScore != a2->MatchScore )
        return a1->MatchScore > a2->MatchScore;  // biggest first.

    if (a1->Parent != a2->Parent)
        return a1->Parent->MatchName.Cmp(a2->Parent->MatchName) < 0;

    return a1->MatchName.Cmp( a2->MatchName ) < 0;
}

COMPONENT_TREE_SEARCH_CONTAINER::COMPONENT_TREE_SEARCH_CONTAINER()
    : tree( NULL )
{
}


COMPONENT_TREE_SEARCH_CONTAINER::~COMPONENT_TREE_SEARCH_CONTAINER()
{
    BOOST_FOREACH( TREE_NODE* node, nodes )
        delete node;
    nodes.clear();
}


void COMPONENT_TREE_SEARCH_CONTAINER::SetPreselectNode( const wxString& aComponentName )
{
    preselect_node_name = aComponentName.Lower();
}


void COMPONENT_TREE_SEARCH_CONTAINER::SetTree( wxTreeCtrl* aTree )
{
    tree = aTree;
    UpdateSearchTerm( wxEmptyString );
}


void COMPONENT_TREE_SEARCH_CONTAINER::AddLibrary( CMP_LIBRARY& aLib )
{
    wxArrayString all_comp;

    aLib.GetEntryNames( all_comp );
    AddComponentList( aLib.GetName(), all_comp, &aLib, false );
}


void COMPONENT_TREE_SEARCH_CONTAINER::AddComponentList( const wxString& aNodeName,
                                                        const wxArrayString& aComponentNameList,
                                                        CMP_LIBRARY* aOptionalLib,
                                                        bool aNormallyExpanded )
{
    TREE_NODE* parent_node = new TREE_NODE( NULL, NULL, aNodeName, wxEmptyString, wxEmptyString,
                                            aNormallyExpanded );

    nodes.push_back( parent_node );

    BOOST_FOREACH( const wxString& cName, aComponentNameList )
    {
        LIB_COMPONENT *c;

        if (aOptionalLib)
            c = aOptionalLib->FindComponent( cName );
        else
            c = CMP_LIBRARY::FindLibraryComponent( cName, wxEmptyString );

        if (c == NULL)
            continue;

        wxString keywords, descriptions;
        wxString display_info;

        for ( size_t i = 0; i < c->GetAliasCount(); ++i )
        {
            LIB_ALIAS *a = c->GetAlias( i );
            keywords += a->GetKeyWords();
            descriptions += a->GetDescription();

            if ( display_info.empty() && !a->GetDescription().empty() )
            {
                // Preformatting. Unfortunately, the tree widget doesn't have columns
                display_info.Printf( wxT(" %s[ %s ]"),
                                     ( cName.length() <= 8 ) ? wxT("\t\t") : wxT("\t"),
                                     GetChars( a->GetDescription() ) );
            }
        }

        // If there are no keywords, we give a couple of characters whitespace penalty. We want
        // a component with a search-term found in the keywords score slightly higher than another
        // component without keywords, but that term in the descriptions.
        wxString search_text = ( !keywords.empty() ) ? keywords : wxT("        ");
        search_text += descriptions;
        nodes.push_back( new TREE_NODE( parent_node, c->GetLibrary(),
                                        cName, display_info, search_text ) );
    }
}


LIB_COMPONENT* COMPONENT_TREE_SEARCH_CONTAINER::GetSelectedComponent()
{
    const wxTreeItemId& select_id = tree->GetSelection();
    BOOST_FOREACH( TREE_NODE* node, nodes )
    {
        if ( node->MatchScore > 0 && node->TreeId == select_id && node->Lib )
            return node->Lib->FindComponent( node->Name );
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
    if ( tree == NULL )
        return;

    // We score the list by going through it several time, essentially with a complexity
    // of O(n). For the default library of 2000+ items, this typically takes less than 5ms
    // on an i5. Good enough, no index needed.

    // Initial AND condition: Leaf nodes are considered to match initially.
    BOOST_FOREACH( TREE_NODE* node, nodes )
    {
        node->PreviousScore = node->MatchScore;
        node->MatchScore = node->Parent ? kLowestDefaultScore : 0;  // start-match for leafs.
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
            if ( node->Parent == NULL)
                continue;      // Library nodes are not scored here.

            if ( node->MatchScore == 0)
                continue;   // Leaf node without score are out of the game.

            // Keywords and description we only count if the match string is at
            // least two characters long. That avoids spurious, low quality
            // matches. Most abbreviations are at three characters long.
            int found_pos;

            if ( term == node->MatchName )
                node->MatchScore += 1000;  // exact match. High score :)
            else if ( (found_pos = node->MatchName.Find( term ) ) != wxNOT_FOUND )
            {
                // Substring match. The earlier in the string the better.  score += 20..40
                node->MatchScore += matchPosScore( found_pos, 20 ) + 20;
            }
            else if ( node->Parent->MatchName.Find( term ) != wxNOT_FOUND )
                node->MatchScore += 19;   // parent name matches.         score += 19
            else if ( ( found_pos = node->SearchText.Find( term ) ) != wxNOT_FOUND )
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

    // Parent nodes have the maximum score seen in any of their children.
    unsigned highest_score_seen = 0;
    bool any_change = false;
    BOOST_FOREACH( TREE_NODE* node, nodes )
    {
        if ( node->Parent == NULL )
            continue;

        any_change |= (node->PreviousScore != node->MatchScore);
        node->Parent->MatchScore = std::max( node->Parent->MatchScore, node->MatchScore );
        highest_score_seen = std::max( highest_score_seen, node->MatchScore );
    }


    // The tree update might be slow, so we want to bail out if there is no change.
    if ( !any_change )
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
        if ( node->MatchScore == 0 )
            continue;

        // If we have nodes that go beyond the default score, suppress nodes that
        // have the default score. That can happen if they have an honary += 0 score due to
        // some one-letter match in the keyword or description. In this case, we prefer matches
        // that just have higher scores. Improves relevancy and performance as the tree has to
        // display less items.
        if ( highest_score_seen > kLowestDefaultScore && node->MatchScore == kLowestDefaultScore )
            continue;

        const bool isLeaf = ( node->Parent != NULL );
        wxString node_text;
#if 0
        // Node text with scoring information for debugging
        node_text.Printf( wxT("%s (s=%u)%s"), GetChars(node->Name),
                          node->MatchScore, GetChars(node->DisplayInfo));
#else
        node_text = node->Name + node->DisplayInfo;
#endif
        node->TreeId = tree->AppendItem( !isLeaf ? root_id : node->Parent->TreeId, node_text );

        // If we are a leaf node, we might need to expand.
        if ( isLeaf )
        {
            if ( node->MatchScore > kLowestDefaultScore )
            {
                tree->EnsureVisible( node->TreeId );

                if ( first_match == NULL )
                    first_match = node;   // The "I am feeling lucky" element.
            }

            if ( preselected_node == NULL && node->MatchName == preselect_node_name )
                preselected_node = node;
        }

        if ( !isLeaf && node->NormallyExpanded )
            tree->Expand( node->TreeId );
    }

    if ( first_match )                      // Highest score search match pre-selected.
        tree->SelectItem( first_match->TreeId );
    else if ( preselected_node )            // No search, so history item preselected.
        tree->SelectItem( preselected_node->TreeId );

    tree->Thaw();
}

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

#ifndef _CMP_TREE_MODEL_H
#define _CMP_TREE_MODEL_H

#include <vector>
#include <memory>
#include <wx/string.h>


class EDA_COMBINED_MATCHER;
class TREE_NODE;
class LIB_ALIAS;


/**
 * Model class in the component selector Model-View-Adapter (mediated MVC)
 * architecture. The other pieces are in:
 *
 * - Adapter: CMP_TREE_MODEL_ADAPTER in eeschema/cmp_tree_model_adapter.h
 * - View:
 *   - DIALOG_CHOOSE_COMPONENT in eeschema/dialogs/dialog_choose_component.h
 *   - wxDataViewCtrl
 *
 * This model is populated from LIB_ALIASes; helper methods in the adapter
 * can accept entire libraries.
 *
 * Quick summary of methods used to populate this class:
 * - `CMP_TREE_NODE_ROOT::AddLib()` - add a new, empty library to the root
 * - `CMP_TREE_NODE_LIB::AddAlias()` - add an alias and its units from a
 *      given LIB_ALIAS*
 *
 * Quick summary of methods used to drive this class:
 *
 * - `UpdateScore()` - accumulate scores recursively given a new search token
 * - `ResetScore()` - reset scores recursively for a new search string
 * - `AssignIntrinsicRanks()` - calculate and cache the initial sort order
 * - `SortNodes()` - recursively sort the tree by score
 * - `Compare()` - compare two nodes; used by `SortNodes()`
 *
 * The data in the model is meant to be accessed directly. Quick summary of
 * data members:
 *
 * - `Parent` - parent node, or nullptr if root
 * - `Children` - vector of unique_ptrs to children
 * - `Type` - ROOT, LIB, ALIAS, or UNIT
 * - `IntrinsicRank` - cached initial sort order
 * - `Score` - score taking into account search terms. Zero means irrelevant and
 *      should be hidden.
 * - `Name` - name of the library/alias/unit, to be displayed
 * - `Desc` - description of the alias, to be displayed
 * - `MatchName` - Name, normalized to lowercase for matching
 * - `SearchText` - normalized composite of keywords and description
 * - `Alias` - the LIB_ALIAS* this alias or unit is from, or nullptr
 * - `Unit` - the unit number, or zero for non-units
 */
class CMP_TREE_NODE {
public:
    enum TYPE {
        ROOT, LIB, ALIAS, UNIT
    };

    CMP_TREE_NODE*                              Parent;     ///< Parent node or null
    std::vector<std::unique_ptr<CMP_TREE_NODE>> Children;   ///< List of child nodes
    enum TYPE                                   Type;       ///< Node type

    /**
     * The rank of the item before any search terms are applied. This is
     * a fairly expensive sort (involving string compares) so it helps to
     * store the result of that sort.
     */
    int IntrinsicRank;

    /// The score of an item resulting from the search algorithm.
    int Score;

    wxString    Name;           ///< Actual name of the part
    wxString    Desc;           ///< Description to be displayed
    wxString    MatchName;      ///< Normalized name for matching
    wxString    SearchText;     ///< Descriptive text to search

    LIB_ALIAS*  Alias;          ///< Actual LIB_ALIAS*, or null
    int         Unit;           ///< Actual unit, or zero


    /**
     * Update the score for this part. This is accumulative - it will be
     * called once per search term.
     *
     * @param aMatcher  an EDA_COMBINED_MATCHER initialized with the search
     *                  term
     */
    virtual void UpdateScore( EDA_COMBINED_MATCHER& aMatcher ) = 0;

    /**
     * Initialize score to kLowestDefaultScore, recursively.
     */
    void ResetScore();

    /**
     * Store intrinsic ranks on all children of this node. See IntrinsicRank
     * member doc for more information.
     */
    void AssignIntrinsicRanks();

    /**
     * Sort child nodes quickly and recursively (IntrinsicRanks must have been set).
     */
    void SortNodes();

    /**
     * Compare two nodes. Returns negative if aNode1 < aNode2, zero if aNode1 ==
     * aNode2, or positive if aNode1 > aNode2.
     */
    static int Compare( CMP_TREE_NODE const& aNode1, CMP_TREE_NODE const& aNode2 );

    CMP_TREE_NODE();
    virtual ~CMP_TREE_NODE() {}
};


/**
 * Node type: unit of component.
 */
class CMP_TREE_NODE_UNIT: public CMP_TREE_NODE
{

public:
    /**
     * The addresses of CMP_TREE_NODEs are used as unique IDs for the
     * wxDataViewModel, so don't let them be copied around.
     */
    CMP_TREE_NODE_UNIT( CMP_TREE_NODE_UNIT const& _ ) = delete;
    void operator=( CMP_TREE_NODE_UNIT const& _ ) = delete;


    /**
     * Construct a unit node.
     *
     * The name of the unit will be "Unit %s", where %s is aUnit formatted
     * by LIB_PART::SubReference.
     *
     * @param aParent   parent node, should be a CMP_TREE_NODE_ALIAS
     * @param aUnit     unit number
     */
    CMP_TREE_NODE_UNIT( CMP_TREE_NODE* aParent, int aUnit );


    /**
     * Do nothing, units just take the parent's score
     */
    virtual void UpdateScore( EDA_COMBINED_MATCHER& aMatcher ) override {}
};


/**
 * Node type: alias.
 */
class CMP_TREE_NODE_ALIAS: public CMP_TREE_NODE
{
public:
    /**
     * The addresses of CMP_TREE_NODEs are used as unique IDs for the
     * wxDataViewModel, so don't let them be copied around.
     */
    CMP_TREE_NODE_ALIAS( CMP_TREE_NODE_ALIAS const& _ ) = delete;
    void operator=( CMP_TREE_NODE_ALIAS const& _ ) = delete;


    /**
     * Construct an alias node.
     *
     * All fields will be populated from the LIB_ALIAS, including children
     * (unit nodes will be generated automatically).
     *
     * @param aParent   parent node, should be a CMP_TREE_NODE_LIB
     * @param aAlias    LIB_ALIAS to populate the node
     */
    CMP_TREE_NODE_ALIAS( CMP_TREE_NODE* aParent, LIB_ALIAS* aAlias );


    /**
     * Perform the actual search.
     */
    virtual void UpdateScore( EDA_COMBINED_MATCHER& aMatcher ) override;

protected:
    /**
     * Add a new unit to the component and return it.
     *
     * This should not be used directly, as the constructor adds all units.
     */
    CMP_TREE_NODE_UNIT& AddUnit( int aUnit );
};


/**
 * Node type: library
 */
class CMP_TREE_NODE_LIB: public CMP_TREE_NODE
{
public:
    /**
     * The addresses of CMP_TREE_NODEs are used as unique IDs for the
     * wxDataViewModel, so don't let them be copied around.
     */
    CMP_TREE_NODE_LIB( CMP_TREE_NODE_LIB const& _ ) = delete;
    void operator=( CMP_TREE_NODE_LIB const& _ ) = delete;


    /**
     * Construct an empty library node.
     *
     * @param aParent   parent node, should be a CMP_TREE_NODE_ROOT
     * @param aName     display name of the library
     */
    CMP_TREE_NODE_LIB( CMP_TREE_NODE* aParent, wxString const& aName );

    /**
     * Construct a new alias node, add it to this library, and return it.
     *
     * @param aAlias    LIB_ALIAS to provide data
     */
    CMP_TREE_NODE_ALIAS& AddAlias( LIB_ALIAS* aAlias );

    virtual void UpdateScore( EDA_COMBINED_MATCHER& aMatcher ) override;
};


/**
 * Node type: root
 */
class CMP_TREE_NODE_ROOT: public CMP_TREE_NODE
{
public:
    /**
     * The addresses of CMP_TREE_NODEs are used as unique IDs for the
     * wxDataViewModel, so don't let them be copied around.
     */
    CMP_TREE_NODE_ROOT( CMP_TREE_NODE_ROOT const& _ ) = delete;
    void operator=( CMP_TREE_NODE_ROOT const& _ ) = delete;

    /**
     * Construct the root node. Root nodes have no properties.
     */
    CMP_TREE_NODE_ROOT();

    /**
     * Construct an empty library node, add it to the root, and return it.
     */
    CMP_TREE_NODE_LIB& AddLib( wxString const& aName );

    virtual void UpdateScore( EDA_COMBINED_MATCHER& aMatcher ) override;
};


#endif // _CMP_TREE_MODEL_H

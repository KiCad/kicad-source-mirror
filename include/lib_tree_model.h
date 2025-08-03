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

#ifndef LIB_TREE_MODEL_H
#define LIB_TREE_MODEL_H

#include <vector>
#include <map>
#include <memory>
#include <wx/string.h>
#include <eda_pattern_match.h>
#include <lib_tree_item.h>


/**
 * Model class in the component selector Model-View-Adapter (mediated MVC)
 * architecture. The other pieces are in:
 *
 * - Adapter: LIB_TREE_MODEL_ADAPTER in common/lib_tree_model_adapter.h
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
 * - `m_IntrinsicRank` - cached initial sort order
 * - `m_Score` - score taking into account search terms. Zero means irrelevant and
 *      should be hidden.
 * - `Name` - name of the library/alias/unit, to be displayed
 * - `Desc` - description of the alias, to be displayed
 * - `m_MatchName` - Name, normalized to lowercase for matching
 * - `m_SearchText` - normalized composite of keywords and description
 * - `LibId` - the #LIB_ID this alias or unit is from, or not valid
 * - `Unit` - the unit number, or zero for non-units
 */
class LIB_TREE_NODE
{
public:
    /**
     * Update the score for this part. This is accumulative - it will be
     * called once per search term.
     *
     * @param aMatcher  an EDA_COMBINED_MATCHER initialized with the search term
     */
    virtual void UpdateScore( const std::vector<std::unique_ptr<EDA_COMBINED_MATCHER>>& aMatchers,
                              std::function<bool( LIB_TREE_NODE& aNode )>* aFilter ) = 0;

    /**
     * Rebuild search terms from source search terms and shown fields.
     */
    void RebuildSearchTerms( const std::vector<wxString>& aShownColumns );

    /**
     * Store intrinsic ranks on all children of this node. See m_IntrinsicRank
     * member doc for more information.
     */
    void AssignIntrinsicRanks( const std::vector<wxString>& aShownColumns, bool presorted = false );

    /**
     * Sort child nodes quickly and recursively (IntrinsicRanks must have been set).
     */
    void SortNodes( bool aUseScores );

    /**
     * Compare two nodes. Returns true if aNode1 < aNode2.
     */
    static bool Compare( LIB_TREE_NODE const& aNode1, LIB_TREE_NODE const& aNode2,
                         bool aUseScores );

    LIB_TREE_NODE();
    virtual ~LIB_TREE_NODE() {}

    enum class TYPE
    {
        ROOT,
        LIBRARY,
        ITEM,
        UNIT,
        INVALID
    };

    typedef std::vector<std::unique_ptr<LIB_TREE_NODE>> PTR_VECTOR;

    LIB_TREE_NODE*  m_Parent;     // Parent node or null
    PTR_VECTOR      m_Children;   // List of child nodes
    enum TYPE       m_Type;       // Node type

    /**
     * The rank of the item before any search terms are applied. This is
     * a fairly expensive sort (involving string compares) so it helps to
     * store the result of that sort.
     */
    int         m_IntrinsicRank;

    int         m_Score;       // The score of an item resulting from the search algorithm.
    bool        m_Pinned;      // Item should appear at top when there is no search string

    wxString    m_Name;        // Actual name of the part
    wxString    m_Desc;        // Description to be displayed
    wxString    m_Footprint;   // Footprint ID as a string (ie: the footprint field text)
    int         m_PinCount;    // Pin count from symbol, or unique pad count from footprint

    std::vector<SEARCH_TERM>     m_SearchTerms;    /// List of weighted search terms
    std::map<wxString, wxString> m_Fields;         /// @see LIB_TREE_ITEMS::GetChooserFields

    LIB_ID      m_LibId;       // LIB_ID determined by the parent library nickname and alias name.
    int         m_Unit;        // Actual unit, or zero
    bool        m_IsRoot;      // Indicates if the symbol is a root symbol instead of an alias.

    bool        m_IsRecentlyUsedGroup;
    bool        m_IsAlreadyPlacedGroup;

protected:
    std::vector<SEARCH_TERM> m_sourceSearchTerms;
};


/**
 * Node type: unit of component.
 */
class LIB_TREE_NODE_UNIT: public LIB_TREE_NODE
{
public:
    /**
     * The addresses of CMP_TREE_NODEs are used as unique IDs for the
     * wxDataViewModel, so don't let them be copied around.
     */
    LIB_TREE_NODE_UNIT( LIB_TREE_NODE_UNIT const& _ ) = delete;
    void operator=( LIB_TREE_NODE_UNIT const& _ ) = delete;

    /**
     * Construct a unit node.
     *
     * The name of the unit will be "Unit %s", where %s is aUnit formatted
     * by LIB_PART::SubReference.
     *
     * @param aParent   parent node, should be a CMP_TREE_NODE_ALIAS
     * @param aItem     parent item
     * @param aUnit     unit number
     */
    LIB_TREE_NODE_UNIT( LIB_TREE_NODE* aParent, LIB_TREE_ITEM* aItem, int aUnit );

    void UpdateScore( const std::vector<std::unique_ptr<EDA_COMBINED_MATCHER>>& aMatchers,
                      std::function<bool( LIB_TREE_NODE& aNode )>* aFilter ) override;
};


/**
 * Node type: #LIB_ID.
 */
class LIB_TREE_NODE_ITEM : public LIB_TREE_NODE
{
public:
    /**
     * The addresses of CMP_TREE_NODEs are used as unique IDs for the
     * wxDataViewModel, so don't let them be copied around.
     */
    LIB_TREE_NODE_ITEM( LIB_TREE_NODE_ITEM const& _ ) = delete;
    void operator=( LIB_TREE_NODE_ITEM const& _ ) = delete;

    /**
     * Construct a #LIB_ID node.
     *
     * All fields will be populated from the LIB_ALIAS, including children
     * (unit nodes will be generated automatically).  This does not keep
     * the pointer to the #LIB_ALIAS object because at any time, a #LIB_ALIAS
     * can be remove from a library which will result in an invalid pointer.
     * The alias must be resolved at the time of use.  Anything else is a bug.
     *
     * @param aParent   parent node, should be a CMP_TREE_NODE_LIB
     * @param aItem     LIB_COMPONENT to populate the node.
     */
    LIB_TREE_NODE_ITEM( LIB_TREE_NODE* aParent, LIB_TREE_ITEM* aItem );

    /**
     * Update the node using data from a LIB_ALIAS object.
     */
    void Update( LIB_TREE_ITEM* aItem );

    /**
     * Perform the actual search.
     */
    void UpdateScore( const std::vector<std::unique_ptr<EDA_COMBINED_MATCHER>>& aMatchers,
                      std::function<bool( LIB_TREE_NODE& aNode )>* aFilter ) override;

protected:
    /**
     * Add a new unit to the component and return it.
     *
     * This should not be used directly, as the constructor adds all units.
     */
    LIB_TREE_NODE_UNIT& AddUnit( LIB_TREE_ITEM* aItem, int aUnit );
};


/**
 * Node type: library
 */
class LIB_TREE_NODE_LIBRARY : public LIB_TREE_NODE
{
public:
    /**
     * The addresses of CMP_TREE_NODEs are used as unique IDs for the
     * wxDataViewModel, so don't let them be copied around.
     */
    LIB_TREE_NODE_LIBRARY( LIB_TREE_NODE_LIBRARY const& _ ) = delete;
    void operator=( LIB_TREE_NODE_LIBRARY const& _ ) = delete;

    /**
     * Construct an empty library node.
     *
     * @param aParent   parent node, should be a CMP_TREE_NODE_ROOT
     * @param aName     display name of the library
     * @param aDesc     a description of the library
     */
    LIB_TREE_NODE_LIBRARY( LIB_TREE_NODE* aParent, const wxString& aName, const wxString& aDesc );

    /**
     * Construct a new alias node, add it to this library, and return it.
     *
     * @param aItem    LIB_COMPONENT to provide data
     */
    LIB_TREE_NODE_ITEM& AddItem( LIB_TREE_ITEM* aItem );

    void UpdateScore( const std::vector<std::unique_ptr<EDA_COMBINED_MATCHER>>& aMatchers,
                      std::function<bool( LIB_TREE_NODE& aNode )>* aFilter ) override;
};


/**
 * Node type: root
 */
class LIB_TREE_NODE_ROOT: public LIB_TREE_NODE
{
public:
    /**
     * The addresses of CMP_TREE_NODEs are used as unique IDs for the
     * wxDataViewModel, so don't let them be copied around.
     */
    LIB_TREE_NODE_ROOT( LIB_TREE_NODE_ROOT const& _ ) = delete;
    void operator=( LIB_TREE_NODE_ROOT const& _ ) = delete;

    /**
     * Construct the root node. Root nodes have no properties.
     */
    LIB_TREE_NODE_ROOT();

    /**
     * Construct an empty library node, add it to the root, and return it.
     */
    LIB_TREE_NODE_LIBRARY& AddLib( wxString const& aName, wxString const& aDesc );

    /**
     * Remove a library node from the root.
     */
    void RemoveGroup( bool aRecentlyUsedGroup, bool aAlreadyPlacedGroup );

    /**
     * Clear the tree
     */
    void Clear();

    void UpdateScore( const std::vector<std::unique_ptr<EDA_COMBINED_MATCHER>>& aMatchers,
                      std::function<bool( LIB_TREE_NODE& aNode )>* aFilter ) override;
};


#endif // LIB_TREE_MODEL_H

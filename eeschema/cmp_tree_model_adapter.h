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

#ifndef _CMP_TREE_MODEL_ADAPTER_H
#define _CMP_TREE_MODEL_ADAPTER_H

#include <cmp_tree_model.h>

#include <wx/hashmap.h>
#include <wx/dataview.h>
#include <vector>
#include <functional>

class LIB_ALIAS;
class PART_LIB;
class PART_LIBS;


/**
 * Adapter class in the component selector Model-View-Adapter (mediated MVC)
 * architecture. The other pieces are in:
 *
 * - Model: CMP_TREE_NODE and descendants in eeschema/cmp_tree_model.h
 * - View:
 *   - DIALOG_CHOOSE_COMPONENT in eeschema/dialogs/dialog_choose_component.h
 *   - wxDataViewCtrl
 *
 * This adapter presents the interface specified by wxDataViewModel to the
 * wxDataViewCtrl:
 *
 *                       +---+                      +------------------+
 *     +---+  Generates  | A |                      |       VIEW       |
 *     | M |  from libs  | D |   wxDataViewModel    |------------------|
 *     | O | <---------- | A | <------------------> |  wxDataViewCtrl  |
 *     | D |             | P |                      |------------------|
 *     | E | <---------> | T | <------------------- |    wxTextCtrl    |
 *     | L | UpdateScore | E | UpdateSearchString() |------------------|
 *     +---+             | R |                      |                  |
 *                       +---+                      +------------------+
 *
 * Because this adapter is a wxDataViewModel, it is reference-counted by
 * wxObject. To ensure this interface is used correctly, the constructor
 * is private; CMP_TREE_MODEL_ADAPTER should be created by the static
 * factory method CMP_TREE_MODEL_ADAPTER::Create().
 *
 * Quick summary of methods used to drive this class:
 *
 * - `SetFilter()` - set whether the view is restricted to power parts
 * - `ShowUnits()` - set whether units are displayed
 * - `SetPreselectNode()` - set a node to highlight when not searching
 * - `AddLibrary()` - populate the model with all aliases in a library
 * - `AddAliasList()` - populate the model with a specific list of aliases
 *
 * Quick summary of methods used by the View:
 *
 * - `UpdateSearchString()` - pass in the user's search text
 * - `AttachTo()` - pass in the wxDataViewCtrl
 * - `GetAliasFor()` - get the LIB_ALIAS* for a selected item
 * - `GetUnitFor()` - get the unit for a selected item
 * - `GetComponentsCount()` - count the aliases loaded
 *
 * Methods implemented as part of wxDataViewModel:
 *
 * - `HasContainerColumns()` - whether a parent item has more than one column
 * - `IsContainer()` - whether an item is a parent
 * - `GetParent()` - return the parent of an item, or invalid if root
 * - `GetChildren()` - get the children of an item
 * - `GetColumnCount()` - get the number of columns in the view
 * - `GetColumnType()` - get the data type shown in each column
 * - `GetValue()` - get the data shown in a cell
 * - `SetValue()` - edit the data in a cell (does nothing)
 * - `GetAttr()` - get any per-item formatting
 * - `Compare()` - compare two rows, for sorting
 * - `HasDefaultCompare()` - whether sorted by default
 */
class CMP_TREE_MODEL_ADAPTER: public wxDataViewModel
{
public:

    /**
     * Reference-counting container for a pointer to CMP_TREE_MODEL_ADAPTER.
     */
    typedef wxObjectDataPtr<CMP_TREE_MODEL_ADAPTER> PTR;

    /**
     * Destructor. Do NOT delete this class manually; it is reference-counted
     * by wxObject.
     */
    ~CMP_TREE_MODEL_ADAPTER();

    /**
     * Factory function: create a model adapter in a reference-counting
     * container.
     *
     * @param aLibs library set from which parts will be loaded
     */
    static PTR Create( PART_LIBS* aLibs );

    /**
     * This enum allows a selective filtering of components to list
     */
    enum CMP_FILTER_TYPE
    {
        CMP_FILTER_NONE,        ///< no filtering
        CMP_FILTER_POWER,       ///< list components flagged PWR
    };

    /**
     * Set the component filter type. Must be set before adding libraries
     *
     * @param aFilter   if CMP_FILTER_POWER, only power parts are loaded
     */
    void SetFilter( CMP_FILTER_TYPE aFilter );

    /**
     * Whether or not to show units. May be set at any time; updates at the next
     * UpdateSearchString()
     *
     * @param aShow if true, units are displayed
     */
    void ShowUnits( bool aShow );

    /**
     * Set the component name to be selected if there are no search results.
     * May be set at any time; updates at the next UpdateSearchString().
     *
     * @param aName     component name to be selected
     * @param aUnit     unit to be selected, if > 0 (0 selects the alias itself)
     */
    void SetPreselectNode( wxString const& aName, int aUnit );

    /**
     * Add all the components and their aliases in this library. To be called
     * in the setup phase.
     *
     * @param aLib  reference to a library
     */
    void AddLibrary( PART_LIB& aLib );

    /**
     * Add the given list of components, by name. To be called in the setup
     * phase.
     *
     * @param aNodeName         the parent node the components will appear under
     * @param aAliasNameList    list of alias names
     * @param aOptionalLib      library to look up names in (null = global)
     */
    void AddAliasList(
            wxString const&         aNodeName,
            wxArrayString const&    aAliasNameList,
            PART_LIB*               aOptionalLib = nullptr );

    /**
     * Add the given list of components by alias. To be called in the setup
     * phase.
     *
     * @param aNodeName         the parent node the components will appear under
     * @param aAliasNameList    list of aliases
     * @param aOptionalLib      library to look up names in (null = global)
     */
    void AddAliasList(
            wxString const&         aNodeName,
            std::vector<LIB_ALIAS*> const&  aAliasList,
            PART_LIB*               aOptionalLib = nullptr );

    /**
     * Set the search string provided by the user.
     *
     * @param aSearch   full, unprocessed search text
     */
    void UpdateSearchString( wxString const& aSearch );

    /**
     * Attach to a wxDataViewCtrl and initialize it. This will set up columns
     * and associate the model via the adapter.
     *
     * @param aDataViewCtrl the view component in the dialog
     */
    void AttachTo( wxDataViewCtrl* aDataViewCtrl );

    /**
     * Return the alias for the given item.
     *
     * @param aSelection    item from the wxDataViewCtrl
     *                      (see wxDataViewCtrl::GetSelection())
     *
     * @return alias, or nullptr if none is selected
     */
    LIB_ALIAS* GetAliasFor( wxDataViewItem aSelection ) const;

    /**
     * Return the unit for the given item.
     *
     * @param aSelection    item from the wxDataViewCtrl
     *                      (see wxDataViewCtrl::GetSelection())
     *
     * @return Unit, or zero if the alias itself is selected. Return valid is
     *         invalid if GetAliasFor() returns nullptr.
     */
    int GetUnitFor( wxDataViewItem aSelection ) const;

    /**
     * Return the number of components loaded in the tree.
     */
    int GetComponentsCount() const;

protected:

    /**
     * Constructor; takes a set of libraries to be included in the search.
     */
    CMP_TREE_MODEL_ADAPTER( PART_LIBS* aLibs );

    /**
     * Check whether a container has columns too
     */
    virtual bool HasContainerColumns( wxDataViewItem const& aItem ) const override;

    /**
     * Check whether an item can have children.
     */
    virtual bool IsContainer( wxDataViewItem const& aItem ) const override;

    /**
     * Get the parent of an item.
     *
     * @param aItem item to get the parent of
     * @return parent of aItem, or an invalid wxDataViewItem if parent is root
     */
    virtual wxDataViewItem GetParent( wxDataViewItem const& aItem ) const override;

    /**
     * Populate a list of all the children of an item
     *
     * @return number of children
     */
    virtual unsigned int GetChildren(
            wxDataViewItem const&   aItem,
            wxDataViewItemArray&    aChildren ) const override;

    /**
     * Return the number of columns in the model
     */
    virtual unsigned int GetColumnCount() const override { return 2; }

    /**
     * Return the type of data stored in the column
     *
     * @return type of data as indicated by wxVariant::GetType()
     */
    virtual wxString GetColumnType( unsigned int aCol ) const override { return "string"; }

    /**
     * Get the value of an item.
     *
     * @param aVariant  wxVariant to receive the data
     * @param aItem     item whose data will be placed into aVariant
     * @param aCol      column number of the data
     */
    virtual void GetValue(
            wxVariant&              aVariant,
            wxDataViewItem const&   aItem,
            unsigned int            aCol ) const override;

    /**
     * Set the value of an item. Does nothing - this model doesn't support
     * editing.
     */
    virtual bool SetValue(
            wxVariant const&        aVariant,
            wxDataViewItem const&   aItem,
            unsigned int            aCol ) override { return false; }

    /**
     * Get any formatting for an item.
     *
     * @param aItem     item to get formatting for
     * @param aCol      column number of interest
     * @param aAttr     receiver for attributes
     * @return          true iff the item has non-default attributes
     */
    virtual bool GetAttr(
            wxDataViewItem const&   aItem,
            unsigned int            aCol,
            wxDataViewItemAttr&     aAttr ) const override;

private:
    CMP_FILTER_TYPE     m_filter;
    bool                m_show_units;
    PART_LIBS*          m_libs;
    wxString            m_preselect_name;
    int                 m_preselect_unit;

    CMP_TREE_NODE_ROOT  m_tree;

    wxDataViewColumn*   m_col_part;
    wxDataViewColumn*   m_col_desc;
    wxDataViewCtrl*     m_widget;

    WX_DECLARE_STRING_HASH_MAP( std::vector<int>, WIDTH_CACHE );

    static WIDTH_CACHE m_width_cache;

    /**
     * Compute the width required for the given column of a node and its
     * children.
     *
     * @param aNode - root node of the tree
     * @param aCol - column number
     * @param aHeading - heading text, to set the minimum width
     */
    int ColWidth( CMP_TREE_NODE& aTree, int aCol, wxString const& aHeading );

    /**
     * Return the width required to display a single row's aCol text.
     * This is cached for efficiency as it's very slow on some platforms
     * (*cough* macOS)
     */
    int WidthFor( CMP_TREE_NODE& aNode, int aCol );

    /**
     * Return the width required to display a column's heading. This is
     * cached by column number for the same reason as the width per cell.
     */
    int WidthFor( wxString const& aHeading, int aCol );

    /**
     * Find any results worth highlighting and expand them, according to given
     * criteria (f(CMP_TREE_NODE const*) -> bool)
     *
     * @return whether a node was expanded
     */
    bool FindAndExpand(
            CMP_TREE_NODE& aNode,
            std::function<bool( CMP_TREE_NODE const* )> aFunc );

    /**
     * Find and expand successful search results
     */
    bool ShowResults();

    /**
     * Find and expand preselected node
     */
    bool ShowPreselect();

    /**
     * Find and expand a library if there is only one
     */
    bool ShowSingleLibrary();
};

#endif // _CMP_TREE_MODEL_ADAPTER_H

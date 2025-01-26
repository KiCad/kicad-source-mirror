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

#ifndef LIB_TREE_MODEL_ADAPTER_H
#define LIB_TREE_MODEL_ADAPTER_H

#include <eda_base_frame.h>
#include <lib_id.h>
#include <lib_tree_model.h>
#include <settings/app_settings.h>
#include <wx/hashmap.h>
#include <wx/dataview.h>
#include <wx/headerctrl.h>
#include <vector>
#include <functional>
#include <set>
#include <map>

/**
 * Adapter class in the symbol selector Model-View-Adapter (mediated MVC)
 * architecture. The other pieces are in:
 *
 * - Model: SYM_TREE_NODE and descendants in eeschema/cmp_tree_model.h
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
 * is private; LIB_TREE_MODEL_ADAPTER should be created by the static
 * factory method LIB_TREE_MODEL_ADAPTER::Create().
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

#include <project.h>

;
class TOOL_INTERACTIVE;
class EDA_BASE_FRAME;


class LIB_TREE_MODEL_ADAPTER: public wxDataViewModel
{
public:
    /**
     * @return a unicode string to mark a node name like a pinned library name.
     * This is not an ASCII7 char, but a unicode char.
     */
    static const wxString GetPinningSymbol()
    {
        return wxString::FromUTF8( "☆ " );
    }

public:
    /**
     * Destructor. Do NOT delete this class manually; it is reference-counted
     * by wxObject.
     */
    ~LIB_TREE_MODEL_ADAPTER();

    /**
     * This enum defines the order of the default columns in the tree view
     */
    enum TREE_COLS
    {
        NAME_COL = 0,   ///< Library or library item name column
        DESC_COL,       ///< Library or library description column
        NUM_COLS        ///< The number of default tree columns
    };

    enum SORT_MODE
    {
        BEST_MATCH = 0,
        ALPHABETIC
    };

    /**
     * Save the column widths to the config file. This requires the tree view to still be
     * valid.
     */
    void SaveSettings();

    /**
     * Set the filter. Must be set before adding libraries.
     */
    void SetFilter( std::function<bool( LIB_TREE_NODE& aNode )>* aFilter ) { m_filter = aFilter; }

    /**
     * Return the active filter.
     */
    std::function<bool( LIB_TREE_NODE& aNode )>* GetFilter() const { return m_filter; }

    void SetSortMode( SORT_MODE aMode ) { m_sort_mode = aMode; }
    SORT_MODE GetSortMode() const { return m_sort_mode; }

    /**
     * Whether or not to show units. May be set at any time; updates at the next
     * UpdateSearchString()
     *
     * @param aShow if true, units are displayed
     */
    void ShowUnits( bool aShow );

    /**
     * Set the symbol name to be selected if there are no search results.
     * May be set at any time; updates at the next UpdateSearchString().
     *
     * @param aLibId    symbol #LIB_ID to be selected
     * @param aUnit     unit to be selected, if > 0 (0 selects the alias itself)
     */
    void SetPreselectNode( const LIB_ID& aLibId, int aUnit );

    /**
     * Add the given list of symbols by alias. To be called in the setup
     * phase.
     *
     * @param aNodeName    the parent node the symbols will appear under
     * @param aDesc        the description field of the parent node
     * @param aItemList    list of symbols
     */
    LIB_TREE_NODE_LIBRARY& DoAddLibrary( const wxString& aNodeName, const wxString& aDesc,
                                         const std::vector<LIB_TREE_ITEM*>& aItemList,
                                         bool pinned, bool presorted );

    /**
     * Remove one of the system groups from the library.
     */
    void RemoveGroup( bool aRecentlyUsedGroup, bool aAlreadyPlacedGroup );

    std::vector<wxString> GetAvailableColumns() const { return m_availableColumns; }

    std::vector<wxString> GetShownColumns() const { return m_shownColumns; }

    std::vector<wxString> GetOpenLibs() const;
    void                  OpenLibs( const std::vector<wxString>& aLibs );

    /// Registers a function to be called whenever new lazy-loaded library content is available
    void RegisterLazyLoadHandler( std::function<void()>&& aHandler )
    {
        m_lazyLoadHandler = aHandler;
    }

    /**
     * Sets which columns are shown in the widget.  Invalid column names are discarded.
     * @param aColumnNames is an ordered list of column names to show
     */
    void SetShownColumns( const std::vector<wxString>& aColumnNames );

    /**
     * Sort the tree and assign ranks after adding libraries.
     */
    void AssignIntrinsicRanks()
    {
        m_tree.AssignIntrinsicRanks( m_shownColumns );

        for( const std::unique_ptr<LIB_TREE_NODE>& child : m_tree.m_Children )
            child->AssignIntrinsicRanks( m_shownColumns );
    }

    /**
     * Set the search string provided by the user.
     *
     * @param aSearch   full, unprocessed search text
     * @param aState    if true, we are keeping the state and so we shouldn't collapse the tree
     */
    void UpdateSearchString( const wxString& aSearch, bool aState );

    /**
     * Attach to a wxDataViewCtrl and initialize it. This will set up columns
     * and associate the model via the adapter.
     *
     * @param aDataViewCtrl the view symbol in the dialog
     */
    void AttachTo( wxDataViewCtrl* aDataViewCtrl );

    /**
     * A final-stage initialization to be called after the window hierarchy has been realized
     * and the window sizes set.
     */
    void FinishTreeInitialization();

    /**
     * Return the alias for the given item.
     *
     * @param aSelection    item from the wxDataViewCtrl
     *                      (see wxDataViewCtrl::GetSelection())
     *
     * @return alias, or nullptr if none is selected
     */
    LIB_ID GetAliasFor( const wxDataViewItem& aSelection ) const;

    /**
     * Return the unit for the given item.
     *
     * @param aSelection    item from the wxDataViewCtrl
     *                      (see wxDataViewCtrl::GetSelection())
     *
     * @return Unit, or zero if the alias itself is selected. Return valid is
     *         invalid if GetAliasFor() returns nullptr.
     */
    int GetUnitFor( const wxDataViewItem& aSelection ) const;

    /**
     * Return node type for the given item.
     *
     * @param aSelection    item from the wxDataViewCtrl
     *                      (see wxDataViewCtrl::GetSelection())
     *
     * @return Type of the selected node, might be INVALID.
     */
    LIB_TREE_NODE::TYPE GetTypeFor( const wxDataViewItem& aSelection ) const;

    LIB_TREE_NODE* GetTreeNodeFor( const wxDataViewItem& aSelection ) const;

    virtual wxString GenerateInfo( const LIB_ID& aLibId, int aUnit ) { return wxEmptyString; }

    virtual bool HasPreview( const wxDataViewItem& aItem ) { return false; }
    virtual void ShowPreview( wxWindow* aParent, const wxDataViewItem& aItem ) {}
    virtual void ShutdownPreview( wxWindow* aParent ) {}

    TOOL_DISPATCHER* GetToolDispatcher() const { return m_parent->GetToolDispatcher(); }

    /**
     * Return the number of symbols loaded in the tree.
     */
    int GetItemCount() const;

    /**
     * Return the number of libraries loaded in the tree.
     */
    virtual int GetLibrariesCount() const
    {
        return m_tree.m_Children.size();
    }

    /**
     * Returns tree item corresponding to part.
     *
     * @param aLibId specifies the part and library name to be searched for.
     * @return Tree data item representing the part. Might be invalid if nothings was found.
     */
    wxDataViewItem FindItem( const LIB_ID& aLibId );

    virtual wxDataViewItem GetCurrentDataViewItem();

    /**
     * Populate a list of all the children of an item
     *
     * @return number of children
     */
    unsigned int GetChildren( const wxDataViewItem& aItem,
                              wxDataViewItemArray& aChildren ) const override;

    // Freezing/Thawing.  Used when updating the table model so that we don't try and fetch
    // values during updating.  Primarily a problem on OSX which doesn't pay attention to the
    // wxDataViewCtrl's freeze count when updating the keyWindow.
    void Freeze() { m_freeze++; }
    void Thaw() { m_freeze--; }
    bool IsFrozen() const { return m_freeze; }

    void RefreshTree();

    // Allows subclasses to nominate a context menu handler.
    virtual TOOL_INTERACTIVE* GetContextMenuTool() { return nullptr; }

    void PinLibrary( LIB_TREE_NODE* aTreeNode );
    void UnpinLibrary( LIB_TREE_NODE* aTreeNode );

    void ShowChangedLanguage();

protected:
    /**
     * Convert #SYM_TREE_NODE -> wxDataViewItem.
     */
    static wxDataViewItem ToItem( const LIB_TREE_NODE* aNode );

    /**
     * Convert wxDataViewItem -> #SYM_TREE_NODE.
     */
    static LIB_TREE_NODE* ToNode( wxDataViewItem aItem );

    /**
     * Create the adapter.
     *
     * @param aParent is the parent frame
     * @param aPinnedKey is the key to load the pinned libraries list from the project file
     * @param aSettingsStruct is the settings structure to load column visibility settings from
     */
    LIB_TREE_MODEL_ADAPTER( EDA_BASE_FRAME* aParent, const wxString& aPinnedKey,
                            APP_SETTINGS_BASE::LIB_TREE& aSettingsStruct );

    LIB_TREE_NODE_LIBRARY& DoAddLibraryNode( const wxString& aNodeName, const wxString& aDesc,
                                             bool pinned );

    /**
     * Check whether a container has columns too
     */
    bool HasContainerColumns( const wxDataViewItem& aItem ) const override;

    /**
     * Check whether an item can have children.
     */
    bool IsContainer( const wxDataViewItem& aItem ) const override;

    /**
     * Get the parent of an item.
     *
     * @return parent of aItem, or an invalid wxDataViewItem if parent is root
     */
    wxDataViewItem GetParent( const wxDataViewItem& aItem ) const override;

    unsigned int GetColumnCount() const override { return m_columns.size(); }

    /**
     * Return the type of data stored in the column as indicated by wxVariant::GetType()
     */
    wxString GetColumnType( unsigned int aCol ) const override { return "string"; }

    /**
     * Get the value of an item.
     *
     * @param aVariant  wxVariant to receive the data
     * @param aItem     item whose data will be placed into aVariant
     * @param aCol      column number of the data
     */
    void GetValue( wxVariant&              aVariant,
                   const wxDataViewItem&   aItem,
                   unsigned int            aCol ) const override;

    /**
     * Set the value of an item. Does nothing - this model doesn't support
     * editing.
     */
    bool SetValue( const wxVariant&        aVariant,
                   const wxDataViewItem&   aItem,
                   unsigned int            aCol ) override { return false; }

    /**
     * Get any formatting for an item.
     *
     * @param aItem     item to get formatting for
     * @param aCol      column number of interest
     * @param aAttr     receiver for attributes
     * @return          true if the item has non-default attributes
     */
    bool GetAttr( const wxDataViewItem&   aItem,
                  unsigned int            aCol,
                  wxDataViewItemAttr&     aAttr ) const override;

    virtual PROJECT::LIB_TYPE_T getLibType() = 0;

    void resortTree();

private:
    /**
     * Find and expand successful search results.  Return the best match (if any).
     */
    const LIB_TREE_NODE* ShowResults();

    wxDataViewColumn* doAddColumn( const wxString& aHeader, bool aTranslate = true );

protected:
    void addColumnIfNecessary( const wxString& aHeader );

    void recreateColumns();
    void createMissingColumns();

    LIB_TREE_NODE_ROOT           m_tree;
    std::map<unsigned, wxString> m_colIdxMap;
    std::vector<wxString>        m_availableColumns;

    wxDataViewCtrl*              m_widget;
    std::vector<wxString>        m_shownColumns;   // Stored in display order
    std::function<void()>        m_lazyLoadHandler;

private:
    EDA_BASE_FRAME*              m_parent;
    APP_SETTINGS_BASE::LIB_TREE& m_cfg;

    SORT_MODE                    m_sort_mode;
    bool                         m_show_units;
    LIB_ID                       m_preselect_lib_id;
    int                          m_preselect_unit;
    int                          m_freeze;

    std::function<bool( LIB_TREE_NODE& aNode )>* m_filter;

    std::vector<wxDataViewColumn*>               m_columns;
    std::map<wxString, wxDataViewColumn*>        m_colNameMap;
    std::map<wxString, int>                      m_colWidths;
};

#endif // LIB_TREE_MODEL_ADAPTER_H


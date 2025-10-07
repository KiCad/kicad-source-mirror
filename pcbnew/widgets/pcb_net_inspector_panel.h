/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#pragma once

#include <board.h>
#include <id.h>
#include <project/project_local_settings.h>
#include <widgets/net_inspector_panel.h>

#include <optional>
#include <vector>

class PCB_EDIT_FRAME;
class NETINFO_ITEM;
class BOARD;
class BOARD_ITEM;
class CN_ITEM;
class PCB_TRACK;
class EDA_COMBINED_MATCHER;

/**
 * PCB net inspection panel
 *
 * Provides a read-only view of net information, such as routed lengths. Data is updated after every change of board
 * items.
 */
class PCB_NET_INSPECTOR_PANEL : public NET_INSPECTOR_PANEL, public BOARD_LISTENER
{
public:
    PCB_NET_INSPECTOR_PANEL( wxWindow* parent, PCB_EDIT_FRAME* aFrame );
    virtual ~PCB_NET_INSPECTOR_PANEL();

    /**
     * Updates the netlist based on global board changes (e.g. stackup definition)
     *
     * Called by PCB_EDIT_FRAME after displaying the Board Setup dialog
     */
    void OnParentSetupChanged() override;

    /*
     * BOARD_LISTENER implementation
     */
    void OnBoardItemAdded( BOARD& aBoard, BOARD_ITEM* aBoardItem ) override;
    void OnBoardItemsAdded( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems ) override;
    void OnBoardItemRemoved( BOARD& aBoard, BOARD_ITEM* aBoardItem ) override;
    void OnBoardItemsRemoved( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems ) override;
    void OnBoardNetSettingsChanged( BOARD& aBoard ) override;
    void OnBoardItemChanged( BOARD& aBoard, BOARD_ITEM* aBoardItem ) override;
    void OnBoardItemsChanged( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems ) override;
    void OnBoardHighlightNetChanged( BOARD& aBoard ) override;
    void OnBoardCompositeUpdate( BOARD& aBoard, std::vector<BOARD_ITEM*>& aAddedItems,
                                 std::vector<BOARD_ITEM*>& aRemovedItems,
                                 std::vector<BOARD_ITEM*>& aChangedItems ) override;

    /// Update panel when board is changed
    void OnBoardChanged() override;

    /// Prepare the panel when shown in the editor
    void OnShowPanel() override;

    /// Persist the net inspector configuration to project / global settings
    void SaveSettings() override;

protected:
    /// Reloads strings on an application language change
    void OnLanguageChangedImpl() override;

    /*
     * UI events
     */
    void OnSearchTextChanged( wxCommandEvent& event ) override;
    void OnConfigButton( wxCommandEvent& event ) override;
    void OnExpandCollapseRow( wxCommandEvent& event );
    void OnHeaderContextMenu( wxCommandEvent& event );
    void OnNetsListContextMenu( wxDataViewEvent& event );
    void OnNetsListItemActivated( wxDataViewEvent& event );
    void OnColumnSorted( wxDataViewEvent& event );

private:
    /// Updates displayed statistics for the given nets
    void updateNets( const std::vector<NETINFO_ITEM*>& aNets ) const;

    /// Unified handling of added / deleted / modified board items
    void updateBoardItems( const std::vector<BOARD_ITEM*>& aBoardItems );

    /*
     * Helper methods for returning formatted data
     */
    static wxString formatNetCode( const NETINFO_ITEM* aNet );
    static wxString formatNetName( const NETINFO_ITEM* aNet );
    static wxString formatCount( unsigned int aValue );
    wxString formatLength( int64_t aValue ) const;
    wxString formatDelay( int64_t aValue ) const;

    /// Generates a sub-menu for the show / hide columns submenu
    void generateShowHideColumnMenu( wxMenu* target );

    /// Fetches an ordered (by NetCode) list of all board connectivity items
    std::vector<CN_ITEM*> relevantConnectivityItems() const;

    /// Filter to determine whether a board net should be included in the net inspector
    bool netFilterMatches( NETINFO_ITEM* aNet, PANEL_NET_INSPECTOR_SETTINGS* cfg = nullptr ) const;

    /// Rebuilds the net inspector list, removing all previous entries
    void buildNetsList( bool rebuildColumns = false );

    /// Build the required columns in the net inspector grid
    void buildColumns();

    /**
     * Adjust the sizing of list columns
     *
     * @param cfg the PANEL_NET_INSPECTOR_SETTINGS from which to read column widths
    */
    void adjustListColumnSizes( PANEL_NET_INSPECTOR_SETTINGS* cfg ) const;

    /**
     * Sets the sort column in the grid to that showing the given model ID column
     *
     * @param sortingColumnId The model ID of the column to sort by
     * @param sortOrderAsc True for ascending sort, False for descending sort
     * @returns true if the column was found
    */
    bool restoreSortColumn( int sortingColumnId, bool sortOrderAsc ) const;

    /**
     * Fetches the displayed grid view column for the given model column ID
     *
     * @param columnId The ID (from column static IDs enum) to find
     * @returns Pointer to the wxDataViewColumn, or nullptr if not found
    */
    wxDataViewColumn* getDisplayedColumnForModelField( int columnId ) const;

    /// Generates a CSV report from currently disaplyed data
    void generateReport();

    /// Highlight the currently selected net
    void highlightSelectedNets();

    /// Handle an application-level change of units
    void onUnitsChanged( wxCommandEvent& event );

    /// Handle a net row(s) context menu selection
    void onContextMenuSelection( wxCommandEvent& event );

    /// Adds a new user-specified net to the board
    void onAddNet();

    /// Renames a selected net
    void onRenameSelectedNet();

    /// Deletes a selected net
    void onDeleteSelectedNet();

    /// Adds a custom display grouping of nets
    void onAddGroup();

    /// Removes a custom display grouping
    void onRemoveSelectedGroup();

    /// Clears highlighting from nets
    void onClearHighlighting();

    /// Forward declaration: container class for a set of net data
    class LIST_ITEM;

    /**
     * Calculates the length statistics for each given netcode
     *
     * @param aNetCodes is the list of netcodes to calculate statistics for. This must be sorted in ascending
     *                  netcode order
     * @param aIncludeZeroPadNets determines whether the results should include nets with no pads
     * @returns a map of net code to net length detail objects
    */
    std::vector<std::unique_ptr<LIST_ITEM>> calculateNets( const std::vector<NETINFO_ITEM*>& aNetCodes,
                                                           bool aIncludeZeroPadNets ) const;

    /// Ordered comparison of LIST_ITEMs by net code
    struct LIST_ITEM_NETCODE_CMP_LESS;

    /// Ordered comparison of LIST_ITEMs by group number
    struct LIST_ITEM_GROUP_NUMBER_CMP_LESS;

    using LIST_ITEM_ITER = std::vector<std::unique_ptr<LIST_ITEM>>::iterator;
    using LIST_ITEM_CONST_ITER = std::vector<std::unique_ptr<LIST_ITEM>>::const_iterator;

    /// Refreshes displayed data for the given rows
    void updateDisplayedRowValues( const std::optional<LIST_ITEM_ITER>& aRow ) const;

    /// Parent BOARD
    BOARD*          m_board = nullptr;

    /// Owning edit frame
    PCB_EDIT_FRAME* m_frame = nullptr;

    /// Data model which holds LIST_ITEMs
    class DATA_MODEL;

    /// The bound data model to display
    wxObjectDataPtr<DATA_MODEL> m_dataModel;
    friend DATA_MODEL;

    /*
     * Status flags set during reporting and net rebuild operations
     */
    bool m_inReporting = false;
    bool m_inBuildNetsList = false;

    /*
     * Status flags to indicate whether a board has been loaded in this control's
     * lifetime. Required as on PCB_EDIT_FRAME construction, there are multiple events
     * triggered which would usually result in saving settings and re-loading the board.
     * However, before the board loads the frame is in an inconsistent state: The project
     * settings are available, but the board is not yet loaded. This results in overwriting
     * settings calculated from an empty board. We do not save settings until the first
     * board load operation has occured.
     */
    bool m_boardLoaded = false;
    bool m_boardLoading = false;

    /*
     * Flags to indicate whether certain events should be disabled during programmatic updates
     */
    bool m_rowExpanding = false;
    bool m_highlightingNets = false;

    /*
     * Configuration flags - these are all persisted to the project storage
     */
    bool m_filterByNetName = true;
    bool m_filterByNetclass = true;
    bool m_showZeroPadNets = false;
    bool m_showUnconnectedNets = false;
    bool m_groupByNetclass = false;
    bool m_groupByConstraint = false;
    bool m_showTimeDomainDetails = false;

    /// Custom net grouping rules
    std::vector<std::unique_ptr<EDA_COMBINED_MATCHER>> m_custom_group_rules;

    /// CSV output control
    enum class CSV_COLUMN_DESC : int
    {
        CSV_NONE = 0,
        CSV_QUOTE = 1 << 0
    };

    /// Column metadata
    struct COLUMN_DESC
    {
        COLUMN_DESC( unsigned aNum, PCB_LAYER_ID aLayer, const wxString& aDisp,
                     const wxString& aCsv, CSV_COLUMN_DESC aFlags, bool aHasUnits ) :
                num( aNum ),
                layer( aLayer ), display_name( aDisp ), csv_name( aCsv ), csv_flags( aFlags ),
                has_units( aHasUnits )
        {
        }

        unsigned int    num;
        PCB_LAYER_ID    layer;
        wxString        display_name;
        wxString        csv_name;
        CSV_COLUMN_DESC csv_flags;
        bool            has_units;

        operator unsigned int() const { return num; }
    };

    /**
     * All displayed (or hidden) columns
     */
    std::vector<COLUMN_DESC> m_columns;

    /// Column static IDs. Used to refer to columns as user re-ordering can occur
    enum
    {
        COLUMN_NAME = 0,
        COLUMN_NETCLASS,
        COLUMN_TOTAL_LENGTH,
        COLUMN_VIA_COUNT,
        COLUMN_VIA_LENGTH,
        COLUMN_BOARD_LENGTH,
        COLUMN_PAD_DIE_LENGTH,
        COLUMN_PAD_COUNT,
        COLUMN_LAST_STATIC_COL = COLUMN_PAD_COUNT
    };

    /// Popup menu item IDs
    enum POPUP_MENU_OPTIONS
    {
        ID_ADD_NET = ID_POPUP_MENU_START,
        ID_RENAME_NET,
        ID_DELETE_NET,
        ID_ADD_GROUP,
        ID_GROUP_BY_CONSTRAINT,
        ID_GROUP_BY_NETCLASS,
        ID_FILTER_BY_NET_NAME,
        ID_FILTER_BY_NETCLASS,
        ID_REMOVE_SELECTED_GROUP,
        ID_REMOVE_GROUPS,
        ID_SHOW_ZERO_NET_PADS,
        ID_SHOW_UNCONNECTED_NETS,
        ID_GENERATE_REPORT,
        ID_HIGHLIGHT_SELECTED_NETS,
        ID_CLEAR_HIGHLIGHTING,
        ID_SHOW_TIME_DOMAIN_DETAILS,
        ID_LAST_STATIC_MENU = ID_CLEAR_HIGHLIGHTING,
        ID_HIDE_COLUMN,
    };
};

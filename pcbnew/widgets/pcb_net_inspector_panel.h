/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PCB_NET_INSPECTOR_H
#define PCB_NET_INSPECTOR_H

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

/**
 * Net inspection panel for pcbnew
 *
 * Provides a read-only view of net information, such as routed lengths. Data is updated after
 * every change of board items. Note that there is not always a 1:1 relationship between Nets and
 * displayed items in the inspector.. This can be the case where there is a constraint which
 * selects sub-sections of nets, for example consider a netclass used for a fly-by-routing
 * adddress bus. There could be two constraints, e.g.:
 * <p>
 *      FROM/TO=IC1-IC2, Netclass=DDR_ADDR, Net=ADDR_0
 *      FROM/TO=IC2-IC3, Netclass=DDR_ADDR, Net=ADDR_0
 * <p>
 * In this instance, a single address net within the DDR_ADDR netclass could have three entries in
 * the inspector, each tracking a different set of net statistics:
 * <p>
 *      1. The whole net
 *      2. IC1-IC2
 *      3. IC2-IC3
 * <p>
 * In this instance, all sub-nets as a result of a constraint will be grouped by the constraint. 
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
    virtual void OnParentSetupChanged() override;

    /*
     * BOARD_LISTENER implementation
     */
    virtual void OnBoardItemAdded( BOARD& aBoard, BOARD_ITEM* aBoardItem ) override;
    virtual void OnBoardItemsAdded( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems ) override;
    virtual void OnBoardItemRemoved( BOARD& aBoard, BOARD_ITEM* aBoardItem ) override;
    virtual void OnBoardItemsRemoved( BOARD&                    aBoard,
                                      std::vector<BOARD_ITEM*>& aBoardItems ) override;
    virtual void OnBoardNetSettingsChanged( BOARD& aBoard ) override;
    virtual void OnBoardItemChanged( BOARD& aBoard, BOARD_ITEM* aBoardItem ) override;
    virtual void OnBoardItemsChanged( BOARD&                    aBoard,
                                      std::vector<BOARD_ITEM*>& aBoardItems ) override;
    virtual void OnBoardHighlightNetChanged( BOARD& aBoard ) override;
    virtual void OnBoardCompositeUpdate( BOARD& aBoard, std::vector<BOARD_ITEM*>& aAddedItems,
                                         std::vector<BOARD_ITEM*>& aRemovedItems,
                                         std::vector<BOARD_ITEM*>& aDeletedItems ) override;

    /**
     * Prepare the panel when shown in the editor
     */
    virtual void OnShowPanel() override;

    /**
     * Persist the net inspector configuration to project / global settings
     */
    virtual void SaveSettings() override;

protected:
    /**
     * Reloads strings on an application language change
     */
    virtual void OnLanguageChangedImpl() override;

    /*
     * UI events
     */
    virtual void OnSearchTextChanged( wxCommandEvent& event ) override;
    virtual void OnConfigButton( wxCommandEvent& event ) override;
    void         OnExpandCollapseRow( wxCommandEvent& event );
    void         OnHeaderContextMenu( wxCommandEvent& event );
    void         OnNetsListContextMenu( wxDataViewEvent& event );
    void         OnNetsListItemActivated( wxDataViewEvent& event );
    void         OnColumnSorted( wxDataViewEvent& event );

private:
    /*
     * Helper methods for returning fornatted data
     */
    wxString formatNetCode( const NETINFO_ITEM* aNet ) const;
    wxString formatNetName( const NETINFO_ITEM* aNet ) const;
    wxString formatCount( unsigned int aValue ) const;
    wxString formatLength( int64_t aValue ) const;

    /**
     * Generates a sub-menu for the show / hide columns submenu
     */
    void generateShowHideColumnMenu( wxMenu* target );

    /**
     * Filters connectivity items from a board update to remove those not related to
     * net / track metrics
     */
    std::vector<CN_ITEM*> relevantConnectivityItems() const;

    /**
     * Filter to determine whether a board net should be included in the net inspector
     */
    bool netFilterMatches( NETINFO_ITEM* aNet, PANEL_NET_INSPECTOR_SETTINGS* cfg = nullptr ) const;

    /**
     * Updates the stored LIST_ITEMs for a given updated board net item
     */
    void updateNet( NETINFO_ITEM* aNet );

    /**
     * Calculates the length of a via from the board stackup
     */
    unsigned int calculateViaLength( const PCB_TRACK* ) const;

    void buildNetsList( bool rebuildColumns = false );
    void buildColumns();
    void setColumnWidths();

    /**
     * Adjust the sizing of list columns
     * 
     * @param cfg the PANEL_NET_INSPECTOR_SETTINGS from which to read column widths
    */
    void adjustListColumnSizes( PANEL_NET_INSPECTOR_SETTINGS* cfg );

    /**
     * Sets the sort column in the grid to that showing the given model ID column
     * 
     * @param sortingColumnId The model ID of the column to sort by
     * @param sortOrderAsc True for ascending sort, False for descending sort
     * @returns true if the column was found
    */
    bool restoreSortColumn( int sortingColumnId, bool sortOrderAsc );

    /**
     * Fetches the displayed grid view column for the given model column ID
     * 
     * @param columnId The ID (from column static IDs enum) to find
     * @returns Pointer to the wxDataViewColumn, or nullptr if not found
    */
    wxDataViewColumn* getDisplayedColumnForModelField( int columnId );

    /**
     * Generates a CSV report from currently disaplyed data
     */
    void generateReport();

    /**
     * Highlight the currently selected net
     */
    void highlightSelectedNets();

    void onUnitsChanged( wxCommandEvent& event );
    void onBoardChanged( wxCommandEvent& event );
    void onSettingsMenu( wxCommandEvent& event );
    void onItemContextMenu( wxCommandEvent& event );
    void onAddNet();
    void onRenameSelectedNet();
    void onDeleteSelectedNet();
    void onRemoveSelectedGroup();
    void onAddGroup();
    void onClearHighlighting();

    /**
     * Container class for a set of net data
     */
    class LIST_ITEM;

    /**
     * Ordered comparison of LIST_ITEMs by net code
     */
    struct LIST_ITEM_NETCODE_CMP_LESS;

    /**
     * Ordered comparison of LIST_ITEMs by group number
     */
    struct LIST_ITEM_GROUP_NUMBER_CMP_LESS;

    using LIST_ITEM_ITER = std::vector<std::unique_ptr<LIST_ITEM>>::iterator;
    using LIST_ITEM_CONST_ITER = std::vector<std::unique_ptr<LIST_ITEM>>::const_iterator;

    /**
     * Constructs a LIST_ITEM for storage in the data model from a board net item
     */
    std::unique_ptr<LIST_ITEM> buildNewItem( NETINFO_ITEM* aNet, unsigned int aPadCount,
                                             const std::vector<CN_ITEM*>& aCNItems );

    void updateDisplayedRowValues( const std::optional<LIST_ITEM_ITER>& aRow );

    // special zero-netcode item. Unconnected pads etc might use different
    // (dummy) NETINFO_ITEM. Redirect all of them to this item, which we get
    // from the board object in buildNetsList.
    NETINFO_ITEM* m_zero_netitem;

    /*
     * Current board and parent edit frame
     */
    BOARD*          m_brd = nullptr;
    PCB_EDIT_FRAME* m_frame = nullptr;

    /**
     * Data model which holds LIST_ITEMs
     */
    class DATA_MODEL;

    /*
     * The bound data model to display
     */
    wxObjectDataPtr<DATA_MODEL> m_data_model;
    friend DATA_MODEL;

    /*
     * Status flags set during reporting and net rebuild operations
     */
    bool m_in_reporting = false;
    bool m_in_build_nets_list = false;

    /*
     * Status flags to indicate whether a board has been loaded in this control's
     * lifetime. Required as on PCB_EDIT_FRAME construction, there are multiple events
     * triggered which would usually result in saving settings and re-loading the board.
     * However, before the board loads the frame is in an inconsistent state: The project
     * settings are available, but the board is not yet loaded. This results in overwriting
     * settings calculated from an empty board. We do not save settings until the first
     * board load operation has occured.
     */
    bool m_board_loaded = false;
    bool m_board_loading = false;

    /*
     * Flags to indicate whether certain events should be disabled during programmatic updates
     */
    bool m_row_expanding = false;
    bool m_highlighting_nets = false;

    /*
     * Configuration flags - these are all persisted to the project storage
     */
    bool m_filter_by_net_name = true;
    bool m_filter_by_netclass = true;
    bool m_show_zero_pad_nets = false;
    bool m_show_unconnected_nets = false;
    bool m_group_by_netclass = false;
    bool m_group_by_constraint = false;

    int m_num_copper_layers = 0;

    std::vector<wxString> m_custom_group_rules;

    /**
     * CSV output control
     */
    enum class CSV_COLUMN_DESC : int
    {
        CSV_NONE = 0,
        CSV_QUOTE = 1 << 0
    };

    /**
     * Column metadata
     */
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

    /*
     * Column static IDs. Used to refer to columns as use re-ordering can occur.
     */
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

    /*
     * Popup menu item IDs
     */
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
        ID_LAST_STATIC_MENU = ID_CLEAR_HIGHLIGHTING,
        ID_HIDE_COLUMN,
    };
};

#endif

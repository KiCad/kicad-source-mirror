/**
 * @file dialog_design_rules.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2009 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2009-2018 KiCad Developers, see change_log.txt for contributors.
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


/* functions relative to the design rules editor
 */
#include <fctsys.h>
#include <class_drawpanel.h>
#include <base_units.h>
#include <confirm.h>
#include <pcbnew.h>
#include <pcb_edit_frame.h>
#include <board_design_settings.h>
#include <bitmaps.h>

#include <pcbnew_id.h>
#include <class_track.h>
#include <macros.h>
#include <html_messagebox.h>
#include <dialog_text_entry.h>

#include <dialog_design_rules.h>
#include <wx/generic/gridctrl.h>
#include <dialog_design_rules_aux_helper_class.h>

#include <tool/tool_event.h>
#include <tool/tool_manager.h>

// Column labels for net lists
#define NET_TITLE       _( "Net" )
#define CLASS_TITLE     _( "Class" )

// Field Positions on rules grid
enum {
    GRID_NAME = 0,
    GRID_CLEARANCE,
    GRID_TRACKSIZE,
    GRID_VIASIZE,
    GRID_VIADRILL,
    GRID_uVIASIZE,
    GRID_uVIADRILL,
    GRID_DIFF_PAIR_WIDTH,
    GRID_DIFF_PAIR_GAP
};

const wxString DIALOG_DESIGN_RULES::wildCard = _( "* (Any)" );

// dialog should remember its previously selected tab
int DIALOG_DESIGN_RULES::s_LastTabSelection = -1;

// methods for the helper class NETS_LIST_CTRL

wxString NETS_LIST_CTRL::OnGetItemText( long item, long column ) const
{
    if( column == 0 )
    {
        if( item < (long) m_Netnames.GetCount() )
            return m_Netnames[item];
        else
            return wxEmptyString;
    }
    else if( item < (long) m_Classnames.GetCount() )
        return m_Classnames[item];

    return wxEmptyString;
}


void NETS_LIST_CTRL::SetRowItems( unsigned        aRow,
                                  const wxString& aNetname,
                                  const wxString& aNetclassName )
{
    // insert blanks if aRow is larger than existing row count
    unsigned cnt = m_Netnames.GetCount();

    if( cnt <= aRow )
        m_Netnames.Add( wxEmptyString, aRow - cnt + 1 );

    cnt = m_Classnames.GetCount();

    if( cnt <= aRow )
        m_Classnames.Add( wxEmptyString, aRow - cnt + 1 );

    if( (int)aRow <= GetItemCount() )
        SetItemCount( aRow + 1 );

    m_Netnames[aRow]   = aNetname;
    m_Classnames[aRow] = aNetclassName;
}


DIALOG_DESIGN_RULES::DIALOG_DESIGN_RULES( PCB_EDIT_FRAME* parent ) :
    DIALOG_DESIGN_RULES_BASE( parent ),
    m_trackMinWidth( parent, m_TrackMinWidthTitle, m_SetTrackMinWidthCtrl, m_TrackMinWidthUnits, true, 0 ),
    m_viaMinDiameter( parent, m_ViaMinTitle, m_SetViasMinSizeCtrl, m_ViaMinUnits, true, 0 ),
    m_viaMinDrill( parent, m_ViaMinDrillTitle, m_SetViasMinDrillCtrl, m_ViaMinDrillUnits, true, 0 ),
    m_microViaMinDiameter( parent, m_MicroViaMinSizeTitle, m_SetMicroViasMinSizeCtrl, m_MicroViaMinSizeUnits, true, 0 ),
    m_microViaMinDrill( parent, m_MicroViaMinDrillTitle, m_SetMicroViasMinDrillCtrl, m_MicroViaMinDrillUnits, true, 0 )
{
    m_Parent = parent;

    m_gridErrorGrid = nullptr;

    // Loading the grid messes up the column widths set in wxFormBuilder
    m_originalColWidths = new int[ m_grid->GetNumberCols() ];

    for( int i = 0; i < m_grid->GetNumberCols(); ++i )
        m_originalColWidths[ i ] = m_grid->GetColSize( i );

    wxListItem column0;
    wxListItem column1;

    column0.Clear();
    column1.Clear();

    column0.SetMask( wxLIST_MASK_TEXT );
    column1.SetMask( wxLIST_MASK_TEXT );

    column0.SetText( NET_TITLE );
    column1.SetText( CLASS_TITLE );

    m_leftListCtrl->InsertColumn( 0, column0 );
    m_leftListCtrl->InsertColumn( 1, column1 );
    m_leftListCtrl->SetColumnWidth( 0, wxLIST_AUTOSIZE );
    m_leftListCtrl->SetColumnWidth( 1, wxLIST_AUTOSIZE );

    m_rightListCtrl->InsertColumn( 0, column0 );
    m_rightListCtrl->InsertColumn( 1, column1 );
    m_rightListCtrl->SetColumnWidth( 0, wxLIST_AUTOSIZE );
    m_rightListCtrl->SetColumnWidth( 1, wxLIST_AUTOSIZE );

    // if user has been into the dialog before, go back to same tab
    if( s_LastTabSelection != -1 )
    {
        m_DRnotebook->SetSelection( s_LastTabSelection );
    }

    m_addButton->SetBitmap( KiBitmap( small_plus_xpm ) );
    m_removeButton->SetBitmap( KiBitmap( trash_xpm ) );
    m_moveUpButton->SetBitmap( KiBitmap( small_up_xpm ) );
    m_moveDownButton->SetBitmap( KiBitmap( small_down_xpm ) );

    m_sdbSizer1OK->SetDefault();

    // Allow tabbing out of grid controls.
    m_grid->SetTabBehaviour( wxGrid::Tab_Leave );
    m_gridViaSizeList->SetTabBehaviour( wxGrid::Tab_Leave );
    m_gridTrackWidthList->SetTabBehaviour( wxGrid::Tab_Leave );

    // wxFormBuilder doesn't include this event...
    m_grid->Connect( wxEVT_GRID_CELL_CHANGING, wxGridEventHandler( DIALOG_DESIGN_RULES::OnNetclassGridCellChanging ), NULL, this );

    m_panelNetClassesEditor->Layout();
    m_panelGolbalDesignRules->Layout();
    Fit();

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}


DIALOG_DESIGN_RULES::~DIALOG_DESIGN_RULES()
{
    delete [] m_originalColWidths;

    m_grid->Disconnect( wxEVT_GRID_CELL_CHANGING, wxGridEventHandler( DIALOG_DESIGN_RULES::OnNetclassGridCellChanging ), NULL, this );
}


bool DIALOG_DESIGN_RULES::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    m_Pcb = m_Parent->GetBoard();
    m_BrdSettings = &m_Pcb->GetDesignSettings();

    // Initialize the Rules List
    transferNetclassesToWindow();

    // Reassure that all nets have net classes assigned
    m_Pcb->BuildListOfNets();

    // copy all NETs into m_AllNets by adding them as NETCUPs.

    // @todo go fix m_Pcb->SynchronizeNetsAndNetClasses() so that the netcode==0 is not
    //       present in the BOARD::m_NetClasses
    NETCLASSES& netclasses = m_BrdSettings->m_NetClasses;
    NETCLASSPTR netclass = netclasses.GetDefault();

    // Initialize list of nets for Default Net Class
    for( NETCLASS::iterator name = netclass->begin();  name != netclass->end();  ++name )
    {
        m_AllNets.push_back( NETCUP( *name, netclass->GetName() ) );
    }

    // Initialize list of nets for others (custom) Net Classes
    for( NETCLASSES::const_iterator nc = netclasses.begin();  nc != netclasses.end();  ++nc )
    {
        netclass = nc->second;

        for( NETCLASS::const_iterator name = netclass->begin();  name != netclass->end();  ++name )
        {
            m_AllNets.push_back( NETCUP( *name, netclass->GetName() ) );
        }
    }

    rebuildNetclassDropdowns();
    transferGlobalRulesToWindow();

    return true;
}


void DIALOG_DESIGN_RULES::transferGlobalRulesToWindow()
{
    m_trackMinWidth.SetValue( m_BrdSettings->m_TrackMinWidth );
    m_viaMinDiameter.SetValue(m_BrdSettings->m_ViasMinSize );
    m_viaMinDrill.SetValue( m_BrdSettings->m_ViasMinDrill );

    m_OptAllowBlindBuriedVias->SetValue( m_BrdSettings->m_BlindBuriedViaAllowed );
    m_OptAllowMicroVias->SetValue( m_BrdSettings->m_MicroViasAllowed );
    CheckAllowMicroVias();

    m_microViaMinDiameter.SetValue( m_BrdSettings->m_MicroViasMinSize );
    m_microViaMinDrill.SetValue( m_BrdSettings->m_MicroViasMinDrill );

    // Initialize Vias and Tracks sizes lists.
    // note we display only extra values, never the current netclass value.
    // (the first value in history list)
    m_TracksWidthList = m_BrdSettings->m_TrackWidthList;
    m_TracksWidthList.erase( m_TracksWidthList.begin() );       // remove the netclass value
    m_ViasDimensionsList = m_BrdSettings->m_ViasDimensionsList;
    m_ViasDimensionsList.erase( m_ViasDimensionsList.begin() ); // remove the netclass value
    InitDimensionsLists();
}


void DIALOG_DESIGN_RULES::InitDimensionsLists()
{
    wxString msg;

    // Fill cells with actual values:
    m_gridViaSizeList->SetCellValue( 0, 0, wxEmptyString );
    m_gridViaSizeList->SetCellValue( 0, 1, wxEmptyString );
    m_gridTrackWidthList->SetCellValue( 0, 0, wxEmptyString );

    for( unsigned ii = 0; ii < m_TracksWidthList.size(); ii++ )
    {
        msg = StringFromValue( GetUserUnits(), m_TracksWidthList[ii], true, true );
        m_gridTrackWidthList->SetCellValue( ii, 0, msg  );
    }

    for( unsigned ii = 0; ii < m_ViasDimensionsList.size(); ii++ )
    {
        msg = StringFromValue( GetUserUnits(), m_ViasDimensionsList[ii].m_Diameter, true, true );
        m_gridViaSizeList->SetCellValue( ii, 0, msg );

        if( m_ViasDimensionsList[ii].m_Drill > 0 )
        {
            msg = StringFromValue( GetUserUnits(), m_ViasDimensionsList[ii].m_Drill, true, true );
            m_gridViaSizeList->SetCellValue( ii, 1, msg );
        }
    }
}


// Sort comparison function (helper for makePointers() )
static bool sortByClassThenName( NETCUP* a, NETCUP* b )
{
    // return a < b
    if( a->clazz < b->clazz )
        return true;

    // inside the same class, sort by net name:
    if( a->clazz == b->clazz )
    {
        if( a->net < b->net )
            return true;
    }

    return false;
}


void DIALOG_DESIGN_RULES::makePointers( PNETCUPS* aList, const wxString& aNetClassName )
{
    aList->clear();

    if( wildCard == aNetClassName )
    {
        for( NETCUPS::iterator n = m_AllNets.begin();  n != m_AllNets.end();  ++n )
        {
            aList->push_back( &*n );
        }

        sort( aList->begin(), aList->end(), sortByClassThenName );

        // could use a different sort order for wildCard case.
    }
    else
    {
        for( NETCUPS::iterator n = m_AllNets.begin();  n != m_AllNets.end();  ++n )
        {
            if( n->clazz == aNetClassName )
                aList->push_back( &*n );
        }

        sort( aList->begin(), aList->end(), sortByClassThenName );
    }
}


void DIALOG_DESIGN_RULES::FillListBoxWithNetNames( NETS_LIST_CTRL* aListCtrl,
                                                   const wxString& aNetClass )
{
    aListCtrl->ClearList();

    PNETCUPS ptrList;

    // get a subset of m_AllNets in pointer form, sorted as desired.
    makePointers( &ptrList, aNetClass );

    // Add netclass info to m_Netnames and m_Classnames wxArrayString buffers
    // aListCtrl uses wxLC_VIRTUAL option, so this is fast
    wxClientDC sDC( aListCtrl );
    int row = 0;
    // recompute the column widths here, after setting texts
    int net_colsize = sDC.GetTextExtent( NET_TITLE ).x;
    int class_colsize = sDC.GetTextExtent( CLASS_TITLE ).x;

    for( PNETCUPS::iterator i = ptrList.begin();  i!=ptrList.end();  ++i, ++row )
    {
        wxSize   net_needed = sDC.GetTextExtent( (*i)->net );
        wxSize   class_needed = sDC.GetTextExtent( (*i)->clazz );
        net_colsize = std::max( net_colsize, net_needed.x );
        class_colsize = std::max( class_colsize, class_needed.x );
        aListCtrl->SetRowItems( row, (*i)->net, (*i)->clazz );
    }

    int margin = sDC.GetTextExtent( wxT( "XX" ) ).x;
    aListCtrl->SetColumnWidth( 0, net_colsize + margin );
    aListCtrl->SetColumnWidth( 1, class_colsize + margin );
    aListCtrl->Refresh();
}


/* Populates drop-downs with the list of net classes
 */
void DIALOG_DESIGN_RULES::rebuildNetclassDropdowns()
{
    wxString rightSelected = m_rightClassChoice->GetStringSelection();
    wxString leftSelected = m_leftClassChoice->GetStringSelection();

    m_rightClassChoice->Clear();
    m_leftClassChoice->Clear();

    m_rightClassChoice->Append( wildCard );
    m_leftClassChoice->Append( wildCard );

    for( int ii = 0; ii < m_grid->GetNumberRows(); ii++ )
    {
        m_rightClassChoice->Append( m_grid->GetCellValue( ii, GRID_NAME ) );
        m_leftClassChoice->Append( m_grid->GetCellValue( ii, GRID_NAME ) );
    }

    // Reselect previous choice if still available; otherwise the wildcard
    m_rightClassChoice->Select( std::max( 0, m_rightClassChoice->FindString( rightSelected ) ) );
    m_leftClassChoice->Select( std::max( 0, m_leftClassChoice->FindString( leftSelected ) ) );

    FillListBoxWithNetNames( m_leftListCtrl, m_leftClassChoice->GetStringSelection() );
    FillListBoxWithNetNames( m_rightListCtrl, m_rightClassChoice->GetStringSelection() );
}


/* Initialize the rules list from board
 */

static void class2gridRow( EDA_UNITS_T aUnits, wxGrid* grid, int row, const NETCLASSPTR& nc )
{
    wxString msg;

    grid->SetCellValue( row, GRID_NAME, nc->GetName() );

    msg = StringFromValue( aUnits, nc->GetClearance(), true, true );
    grid->SetCellValue( row, GRID_CLEARANCE, msg );

    msg = StringFromValue( aUnits, nc->GetTrackWidth(), true, true );
    grid->SetCellValue( row, GRID_TRACKSIZE, msg );

    msg = StringFromValue( aUnits, nc->GetViaDiameter(), true, true );
    grid->SetCellValue( row, GRID_VIASIZE, msg );

    msg = StringFromValue( aUnits, nc->GetViaDrill(), true, true );
    grid->SetCellValue( row, GRID_VIADRILL, msg );

    msg = StringFromValue( aUnits, nc->GetuViaDiameter(), true, true );
    grid->SetCellValue( row, GRID_uVIASIZE, msg );

    msg = StringFromValue( aUnits, nc->GetuViaDrill(), true, true );
    grid->SetCellValue( row, GRID_uVIADRILL, msg );

    msg = StringFromValue( aUnits, nc->GetDiffPairGap(), true, true );
    grid->SetCellValue( row, GRID_DIFF_PAIR_GAP, msg );

    msg = StringFromValue( aUnits, nc->GetDiffPairWidth(),true, true );
    grid->SetCellValue( row, GRID_DIFF_PAIR_WIDTH, msg );

}


void DIALOG_DESIGN_RULES::transferNetclassesToWindow()
{
    NETCLASSES& netclasses = m_BrdSettings->m_NetClasses;

    // the +1 is for the Default NETCLASS.
    if( netclasses.GetCount() + 1 > (unsigned) m_grid->GetNumberRows() )
        m_grid->AppendRows( netclasses.GetCount() + 1 - m_grid->GetNumberRows() );

    // enter the Default NETCLASS.
    class2gridRow( GetUserUnits(), m_grid, 0, netclasses.GetDefault() );

    // make the Default NETCLASS name read-only
    wxGridCellAttr* cellAttr = m_grid->GetOrCreateCellAttr( 0, GRID_NAME );
    cellAttr->SetReadOnly();
    cellAttr->DecRef();

    // enter other netclasses
    int row = 1;

    for( NETCLASSES::iterator i = netclasses.begin();  i != netclasses.end();  ++i, ++row )
        class2gridRow( GetUserUnits(), m_grid, row, i->second );
}


static void gridRow2class( EDA_UNITS_T aUnits, wxGrid* grid, int row, const NETCLASSPTR& nc )
{
    nc->SetName( grid->GetCellValue( row, GRID_NAME ) );

#define MYCELL( col )   \
    ValueFromString( aUnits, grid->GetCellValue( row, col ) )

    nc->SetClearance( MYCELL( GRID_CLEARANCE ) );
    nc->SetTrackWidth( MYCELL( GRID_TRACKSIZE ) );
    nc->SetViaDiameter( MYCELL( GRID_VIASIZE ) );
    nc->SetViaDrill( MYCELL( GRID_VIADRILL ) );
    nc->SetuViaDiameter( MYCELL( GRID_uVIASIZE ) );
    nc->SetuViaDrill( MYCELL( GRID_uVIADRILL ) );
    nc->SetDiffPairGap( MYCELL( GRID_DIFF_PAIR_GAP ) );
    nc->SetDiffPairWidth( MYCELL( GRID_DIFF_PAIR_WIDTH ) );
}


void DIALOG_DESIGN_RULES::CopyNetclassesToBoard()
{
    // Commit any pending in-place edits and close the editor
    m_grid->DisableCellEditControl();

    NETCLASSES& netclasses = m_BrdSettings->m_NetClasses;

    // Remove all netclasses from board. We'll copy new list after
    netclasses.Clear();

    // Copy the default NetClass:
    gridRow2class( GetUserUnits(), m_grid, 0, netclasses.GetDefault() );

    // Copy other NetClasses :
    for( int row = 1; row < m_grid->GetNumberRows();  ++row )
    {
        NETCLASSPTR nc = std::make_shared<NETCLASS>( m_grid->GetCellValue( row, GRID_NAME ) );

        if( m_BrdSettings->m_NetClasses.Add( nc ) )
            gridRow2class( GetUserUnits(), m_grid, row, nc );
    }

    // Now read all nets and push them in the corresponding netclass net buffer
    for( NETCUPS::const_iterator netcup = m_AllNets.begin(); netcup != m_AllNets.end(); ++netcup )
    {
        NETCLASSPTR nc = netclasses.Find( netcup->clazz );
        wxASSERT( nc );
        nc->Add( netcup->net );
    }

    m_Pcb->SynchronizeNetsAndNetClasses();
}


void DIALOG_DESIGN_RULES::CopyGlobalRulesToBoard()
{
    // Update tracks minimum values for DRC
    m_BrdSettings->m_TrackMinWidth = m_trackMinWidth.GetValue();

    // Update vias minimum values for DRC
    m_BrdSettings->m_ViasMinSize = m_viaMinDiameter.GetValue();
    m_BrdSettings->m_ViasMinDrill = m_viaMinDrill.GetValue();

    m_BrdSettings->m_BlindBuriedViaAllowed = m_OptAllowBlindBuriedVias->GetValue();
    m_BrdSettings->m_MicroViasAllowed = m_OptAllowMicroVias->GetValue();

    // Update microvias minimum values for DRC
    m_BrdSettings->m_MicroViasMinSize = m_microViaMinDiameter.GetValue();
    m_BrdSettings->m_MicroViasMinDrill = m_microViaMinDrill.GetValue();
}


void DIALOG_DESIGN_RULES::CopyDimensionsListsToBoard()
{
    wxString msg;

    // Commit any pending in-place edits and close editors from grid controls
    m_gridTrackWidthList->DisableCellEditControl();
    m_gridViaSizeList->DisableCellEditControl();

    // Reinitialize m_TrackWidthList
    m_TracksWidthList.clear();

    for( int row = 0; row < m_gridTrackWidthList->GetNumberRows();  ++row )
    {
        msg = m_gridTrackWidthList->GetCellValue( row, 0 );

        if( msg.IsEmpty() )
            continue;

        int value = ValueFromString( GetUserUnits(), msg );
        m_TracksWidthList.push_back( value );
    }

    // Sort new list by by increasing value
    sort( m_TracksWidthList.begin(), m_TracksWidthList.end() );

    // Reinitialize m_ViasDimensionsList
    m_ViasDimensionsList.clear();

    for( int row = 0; row < m_gridViaSizeList->GetNumberRows();  ++row )
    {
        msg = m_gridViaSizeList->GetCellValue( row, 0 );

        if( msg.IsEmpty() )
            continue;

        int value = ValueFromString( GetUserUnits(), msg );
        VIA_DIMENSION via_dim;
        via_dim.m_Diameter = value;
        msg = m_gridViaSizeList->GetCellValue( row, 1 );

        if( !msg.IsEmpty() )
        {
            value = ValueFromString( GetUserUnits(), msg );
            via_dim.m_Drill = value;
        }

        m_ViasDimensionsList.push_back( via_dim );
    }

    // Sort new list by by increasing value
    sort( m_ViasDimensionsList.begin(), m_ViasDimensionsList.end() );

    std::vector<int>* tlist = &m_BrdSettings->m_TrackWidthList;

    // Remove old "custom" sizes
    tlist->erase( tlist->begin() + 1, tlist->end() );

    // Add new "custom" sizes
    tlist->insert( tlist->end(), m_TracksWidthList.begin(), m_TracksWidthList.end() );

    // Reinitialize m_ViaSizeList
    std::vector<VIA_DIMENSION>* vialist = &m_BrdSettings->m_ViasDimensionsList;
    vialist->erase( vialist->begin() + 1, vialist->end() );
    vialist->insert( vialist->end(), m_ViasDimensionsList.begin(), m_ViasDimensionsList.end() );
}


void DIALOG_DESIGN_RULES::OnNotebookPageChanged( wxNotebookEvent& event )
{
    s_LastTabSelection = event.GetSelection();

    // Skip() allows OSX to properly refresh controls.
    event.Skip();
}


bool DIALOG_DESIGN_RULES::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    if( !validateData() )
        return false;

    CopyNetclassesToBoard();
    CopyGlobalRulesToBoard();
    CopyDimensionsListsToBoard();
    m_BrdSettings->SetCurrentNetClass( NETCLASS::Default );

    //this event causes the routing tool to reload its design rules information
    TOOL_MANAGER* toolManager = m_Parent->GetToolManager();
    TOOL_EVENT event( TC_COMMAND, TA_MODEL_CHANGE, AS_ACTIVE );
    toolManager->ProcessEvent( event );

    return true;
}


bool DIALOG_DESIGN_RULES::validateNetclassName( int aRow, wxString aName, bool focusFirst )
{
    aName.Trim( true );
    aName.Trim( false );

    if( aName.IsEmpty() )
    {
        setGridError( m_grid, _( "Netclass name cannot be empty." ), aRow, GRID_NAME );
        return false;
    }

    for( int ii = 0; ii < m_grid->GetNumberRows(); ii++ )
    {
        if( ii != aRow && m_grid->GetRowLabelValue( ii ).CmpNoCase( aName ) == 0 )
        {
            if( focusFirst )
                setGridError( m_grid, _( "Netclass name already in use." ), aRow, GRID_NAME );
            else
                setGridError( m_grid, _( "Netclass name already in use." ), ii, GRID_NAME );
            return false;
        }
    }

    return true;
}


void DIALOG_DESIGN_RULES::OnNetclassGridCellChanging( wxGridEvent& event )
{
    if( event.GetCol() == GRID_NAME )
    {
        if( validateNetclassName( event.GetRow(), event.GetString() ) )
            m_netclassesDirty = true;
        else
            event.Veto();
    }
}


void DIALOG_DESIGN_RULES::OnAddNetclassClick( wxCommandEvent& event )
{
    m_grid->AppendRows();

    // Copy values of the default class:
    int irow = m_grid->GetNumberRows() - 1;

    for( int icol = 1; icol < m_grid->GetNumberCols(); icol++ )
    {
        wxString value;
        value = m_grid->GetCellValue( 0, icol );
        m_grid->SetCellValue( irow, icol, value );
    }

    rebuildNetclassDropdowns();
}


void DIALOG_DESIGN_RULES::OnRemoveNetclassClick( wxCommandEvent& event )
{
    int curRow   = m_grid->GetGridCursorRow();

    if( curRow == 0 )
    {
        DisplayErrorMessage( this, _( "The default net class is required." ) );
        return;
    }

    // reset the net class to default for members of the removed class
    wxString classname = m_grid->GetCellValue( curRow, GRID_NAME );
    swapNetClass( classname, NETCLASS::Default );

    m_grid->DeleteRows( curRow, 1 );

    rebuildNetclassDropdowns();
}


void DIALOG_DESIGN_RULES::AdjustNetclassGridColumns( int aWidth )
{
    int fixedColsWidth = 0;

    for( int i = 1; i < m_grid->GetNumberCols(); i++ )
    {
        m_grid->SetColSize( i, m_originalColWidths[ i ] );
        fixedColsWidth += m_originalColWidths[ i ];
    }

    m_grid->SetColSize( 0, aWidth - fixedColsWidth - 4 );
}


void DIALOG_DESIGN_RULES::OnSizeNetclassGrid( wxSizeEvent& event )
{
    AdjustNetclassGridColumns( event.GetSize().GetX() );

    event.Skip();
}


void DIALOG_DESIGN_RULES::setGridError( wxGrid* aGrid, const wxString& aMsg, int aRow, int aCol )
{
    m_gridErrorGrid = aGrid;
    m_gridErrorMsg = aMsg;
    m_gridErrorRow = aRow;
    m_gridErrorCol = aCol;

    if( m_gridErrorGrid->GetParent() != m_DRnotebook->GetCurrentPage() )
        m_DRnotebook->AdvanceSelection();
}


void DIALOG_DESIGN_RULES::OnUpdateUI( wxUpdateUIEvent& event )
{
    if( m_netclassesDirty )
    {
        rebuildNetclassDropdowns();
        m_netclassesDirty = false;
    }

    // Handle a grid error.  This is delayed to OnUpdateUI so that we can change focus
    // even when the original validation was triggered from a killFocus event, and so
    // that the corresponding notebook page can be shown in the background when triggered
    // from an OK.
    if( m_gridErrorGrid )
    {
        // We will re-enter this routine when the error dialog is displayed, so make
        // sure we don't keep putting up more dialogs.
        wxGrid* grid = m_gridErrorGrid;
        m_gridErrorGrid = nullptr;

        DisplayErrorMessage( this, m_gridErrorMsg );

        grid->GoToCell( m_gridErrorRow, m_gridErrorCol );
        grid->SetFocus();
    }
}


void DIALOG_DESIGN_RULES::CheckAllowMicroVias()
{
    bool enabled = m_OptAllowMicroVias->GetValue();

    m_microViaMinDiameter.Enable( enabled );
    m_microViaMinDrill.Enable( enabled );
}

/**
 * Function OnAllowMicroVias
 * is called whenever the AllowMicroVias checkbox is toggled
 */
void DIALOG_DESIGN_RULES::OnAllowMicroVias( wxCommandEvent& event )
{
    CheckAllowMicroVias();
}

void DIALOG_DESIGN_RULES::OnMoveUpSelectedNetClass( wxCommandEvent& event )
{
    // Commit any pending in-place edits and close the editor
    m_grid->DisableCellEditControl();

    int curRow   = m_grid->GetGridCursorRow();

    if( curRow < 2 )
    {
        wxBell();
        return;
    }

    // Swap the rule and the previous rule
    for( int icol = 0; icol < m_grid->GetNumberCols(); icol++ )
    {
        wxString curr_value     = m_grid->GetCellValue( curRow, icol );
        wxString previous_value = m_grid->GetCellValue( curRow - 1, icol );
        m_grid->SetCellValue( curRow, icol, previous_value );
        m_grid->SetCellValue( curRow - 1, icol, curr_value );
    }

    m_grid->SetGridCursor( curRow - 1, m_grid->GetGridCursorCol() );
    m_grid->MakeCellVisible( m_grid->GetGridCursorRow(), m_grid->GetGridCursorCol() );

    m_netclassesDirty = true;
}


void DIALOG_DESIGN_RULES::OnMoveDownSelectedNetClass( wxCommandEvent& event )
{
    // Commit any pending in-place edits and close the editor
    m_grid->DisableCellEditControl();

    int curRow   = m_grid->GetGridCursorRow();

    if( curRow == 0 || curRow == m_grid->GetNumberRows() - 1 )
    {
        wxBell();
        return;
    }

    // Swap the rule and the next rule
    for( int icol = 0; icol < m_grid->GetNumberCols(); icol++ )
    {
        wxString curr_value = m_grid->GetCellValue( curRow, icol );
        wxString next_value = m_grid->GetCellValue( curRow + 1, icol );
        m_grid->SetCellValue( curRow, icol, next_value );
        m_grid->SetCellValue( curRow + 1, icol, curr_value );
    }

    m_grid->SetGridCursor( curRow + 1, m_grid->GetGridCursorCol() );
    m_grid->MakeCellVisible( m_grid->GetGridCursorRow(), m_grid->GetGridCursorCol() );

    m_netclassesDirty = true;
}


void DIALOG_DESIGN_RULES::OnLeftCBSelection( wxCommandEvent& event )
{
    FillListBoxWithNetNames( m_leftListCtrl, m_leftClassChoice->GetStringSelection() );

    if( m_leftClassChoice->GetStringSelection() ==  m_rightClassChoice->GetStringSelection() )
    {
        m_buttonRightToLeft->Enable( false );
        m_buttonLeftToRight->Enable( false );
    }
    else
    {
        m_buttonRightToLeft->Enable( true );
        m_buttonLeftToRight->Enable( true );
    }
}


void DIALOG_DESIGN_RULES::OnRightCBSelection( wxCommandEvent& event )
{
    FillListBoxWithNetNames( m_rightListCtrl, m_rightClassChoice->GetStringSelection() );

    if( m_leftClassChoice->GetStringSelection() ==  m_rightClassChoice->GetStringSelection() )
    {
        m_buttonRightToLeft->Enable( false );
        m_buttonLeftToRight->Enable( false );
    }
    else
    {
        m_buttonRightToLeft->Enable( true );
        m_buttonLeftToRight->Enable( true );
    }
}


void DIALOG_DESIGN_RULES::moveSelectedItems( NETS_LIST_CTRL* src, const wxString& newClassName )
{
    wxListItem item;
    wxString   netName;

    item.m_mask |= wxLIST_MASK_TEXT;       // Validate the member m_text of the wxListItem item

    for( int row = 0;  row < src->GetItemCount();  ++row )
    {
        if( !src->GetItemState( row, wxLIST_STATE_SELECTED ) )
            continue;

        item.SetColumn( 0 );
        item.SetId( row );

        src->GetItem( item );
        netName = item.GetText();

        setNetClass( netName, newClassName == wildCard ? NETCLASS::Default : newClassName );
    }
}


void DIALOG_DESIGN_RULES::OnRightToLeftCopyButton( wxCommandEvent& event )
{
    wxString newClassName = m_leftClassChoice->GetStringSelection();

    moveSelectedItems( m_rightListCtrl, newClassName );

    FillListBoxWithNetNames( m_leftListCtrl, m_leftClassChoice->GetStringSelection() );
    FillListBoxWithNetNames( m_rightListCtrl, m_rightClassChoice->GetStringSelection() );
}


void DIALOG_DESIGN_RULES::OnLeftToRightCopyButton( wxCommandEvent& event )
{
    wxString newClassName = m_rightClassChoice->GetStringSelection();

    moveSelectedItems( m_leftListCtrl, newClassName );

    FillListBoxWithNetNames( m_leftListCtrl, m_leftClassChoice->GetStringSelection() );
    FillListBoxWithNetNames( m_rightListCtrl, m_rightClassChoice->GetStringSelection() );
}


void DIALOG_DESIGN_RULES::OnLeftSelectAllButton( wxCommandEvent& event )
{
    for( int ii = 0; ii < m_leftListCtrl->GetItemCount(); ii++ )
        m_leftListCtrl->SetItemState( ii, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
}


void DIALOG_DESIGN_RULES::OnRightSelectAllButton( wxCommandEvent& event )
{
    for( int ii = 0; ii < m_rightListCtrl->GetItemCount(); ii++ )
        m_rightListCtrl->SetItemState( ii, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
}


void DIALOG_DESIGN_RULES::setNetClass( const wxString& aNetName, const wxString& aClassName )
{
    for( NETCUPS::iterator i = m_AllNets.begin();  i != m_AllNets.end();  ++i )
    {
        if( i->net == aNetName )
        {
            i->clazz = aClassName;
            break;
        }
    }
}


bool DIALOG_DESIGN_RULES::validateData()
{
    wxString msg;
    int      minViaDia = m_viaMinDiameter.GetValue();
    int      minViaDrill = m_viaMinDrill.GetValue();
    int      minUViaDia = m_microViaMinDiameter.GetValue();
    int      minUViaDrill = m_microViaMinDrill.GetValue();
    int      minTrackWidth = m_trackMinWidth.GetValue();

    // Test net class parameters.
    for( int row = 0; row < m_grid->GetNumberRows(); row++ )
    {
        wxString netclassName = m_grid->GetCellValue( row, GRID_NAME );
        netclassName.Trim( true );
        netclassName.Trim( false );

        if( !validateNetclassName( row, netclassName, false ) )
            return false;

        if( ValueFromString( GetUserUnits(), m_grid->GetCellValue( row, GRID_TRACKSIZE ) )
                < minTrackWidth )
        {
            msg.Printf( _( "Track width less than minimum track width (%s)." ),
                        StringFromValue( GetUserUnits(), minTrackWidth, true, true ) );
            setGridError( m_grid, msg, row, GRID_TRACKSIZE );
            return false;
        }

        if( ValueFromString( GetUserUnits(), m_grid->GetCellValue( row, GRID_DIFF_PAIR_WIDTH ) )
                < minTrackWidth )
        {
            msg.Printf( _( "Differential pair width less than minimum track width (%s)." ),
                        StringFromValue( GetUserUnits(), minTrackWidth, true, true ) );
            setGridError( m_grid, msg, row, GRID_DIFF_PAIR_WIDTH );
            return false;
        }

        // Test vias
        if( ValueFromString( GetUserUnits(), m_grid->GetCellValue( row, GRID_VIASIZE ) )
                < minViaDia )
        {
            msg.Printf( _( "Via diameter less than minimum via diameter (%s)." ),
                        StringFromValue( GetUserUnits(), minViaDia, true, true ) );
            setGridError( m_grid, msg, row, GRID_VIASIZE );
            return false;
        }

        if( ValueFromString( GetUserUnits(), m_grid->GetCellValue( row, GRID_VIADRILL ) )
                >= ValueFromString( GetUserUnits(), m_grid->GetCellValue( row, GRID_VIASIZE ) ) )
        {
            msg = _( "Via drill larger than via diameter." );
            setGridError( m_grid, msg, row, GRID_VIADRILL );
            return false;
        }

        if( ValueFromString( GetUserUnits(), m_grid->GetCellValue( row, GRID_VIADRILL ) )
                < minViaDrill )
        {
            msg.Printf( _( "Via drill less than minimum via drill (%s)." ),
                        StringFromValue( GetUserUnits(), minViaDrill, true, true ) );
            setGridError( m_grid, msg, row, GRID_VIADRILL );
            return false;
        }

        // Test Micro vias
        if( ValueFromString( GetUserUnits(), m_grid->GetCellValue( row, GRID_uVIASIZE ) )
                < minUViaDia )
        {
            msg.Printf( _( "Microvia diameter less than minimum microvia diameter (%s)." ),
                        StringFromValue( GetUserUnits(), minUViaDia, true, true ) );
            setGridError( m_grid, msg, row, GRID_uVIASIZE );
            return false;
        }

        if( ValueFromString( GetUserUnits(), m_grid->GetCellValue( row, GRID_uVIADRILL ) )
                >= ValueFromString( GetUserUnits(), m_grid->GetCellValue( row, GRID_uVIASIZE ) ) )
        {
            msg = _( "Microvia drill larger than microvia diameter." );
            setGridError( m_grid, msg, row, GRID_uVIADRILL );
            return false;
        }

        if( ValueFromString( GetUserUnits(), m_grid->GetCellValue( row, GRID_uVIADRILL ) )
                < minUViaDrill )
        {
            msg.Printf( _( "Microvia drill less than minimum microvia drill (%s)." ),
                        StringFromValue( GetUserUnits(), minUViaDrill, true, true ) );
            setGridError( m_grid, msg, row, GRID_uVIADRILL );
            return false;
        }
    }

    // Test custom tracks
    for( int row = 0; row < m_gridTrackWidthList->GetNumberRows();  ++row )
    {
        wxString tvalue = m_gridTrackWidthList->GetCellValue( row, 0 );

        if( tvalue.IsEmpty() )
            continue;

        if( ValueFromString( GetUserUnits(), tvalue ) < minTrackWidth )
        {
            msg.Printf( _( "Track width less than minimum track width (%s)." ),
                        StringFromValue( GetUserUnits(), minTrackWidth, true, true ) );
            setGridError( m_gridTrackWidthList, msg, row, 0 );
            return false;
        }
    }

    // Test custom vias
    for( int row = 0; row < m_gridViaSizeList->GetNumberRows();  ++row )
    {
        wxString viaDia = m_gridViaSizeList->GetCellValue( row, 0 );

        if( viaDia.IsEmpty() )
            continue;

        if( ValueFromString( GetUserUnits(), viaDia ) < minViaDia )
        {
            msg.Printf( _( "Via diameter less than minimum via diameter (%s)." ),
                        StringFromValue( GetUserUnits(), minViaDia, true, true ) );
            setGridError( m_gridViaSizeList, msg, row, 0 );
            return false;
        }

        wxString viaDrill = m_gridViaSizeList->GetCellValue( row, 1 );

        if( viaDrill.IsEmpty() )
        {
            msg = _( "No via drill defined." );
            setGridError( m_gridViaSizeList, msg, row, 1 );
            return false;
        }

        if( ValueFromString( GetUserUnits(), viaDrill ) < minViaDrill )
        {
            msg.Printf( _( "Via drill less than minimum via drill (%s)." ),
                        StringFromValue( GetUserUnits(), minViaDrill, true, true ) );
            setGridError( m_gridViaSizeList, msg, row, 1 );
            return false;
        }

        if( ValueFromString( GetUserUnits(), viaDrill )
                >= ValueFromString( GetUserUnits(), viaDia ) )
        {
            msg = _( "Via drill larger than via diameter." );
            setGridError( m_gridViaSizeList, msg, row, 1 );
            return false;
        }
    }

    return true;
}





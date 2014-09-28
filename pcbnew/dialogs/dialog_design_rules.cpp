/**
 * @file dialog_design_rules.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2009 Jean-Pierre Charras, jean-pierre.charras@gpisa-lab.inpg.fr
 * Copyright (C) 2009 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2009 KiCad Developers, see change_log.txt for contributors.
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
#include <wxPcbStruct.h>
#include <class_board_design_settings.h>

#include <pcbnew_id.h>
#include <class_track.h>
#include <macros.h>

#include <dialog_design_rules.h>
#include <wx/generic/gridctrl.h>
#include <dialog_design_rules_aux_helper_class.h>

#include <boost/make_shared.hpp>

// Column labels for net lists
#define NET_TITLE       _( "Net" )
#define CLASS_TITLE     _( "Class" )

// Field Positions on rules grid
enum {
    GRID_CLEARANCE,
    GRID_TRACKSIZE,
    GRID_VIASIZE,
    GRID_VIADRILL,
    GRID_uVIASIZE,
    GRID_uVIADRILL
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


/**
 * Function EnsureGridColumnWidths
 * resizes all the columns in a wxGrid based only on the requirements of the
 * column titles and not on the grid cell requirements, assuming that the grid
 * cell width requirements are narrower than the column title requirements.
 */

// @todo: maybe move this to common.cpp if it works.
void EnsureGridColumnWidths( wxGrid* aGrid )
{
    wxScreenDC sDC;

    sDC.SetFont( aGrid->GetLabelFont() );

    int colCount = aGrid->GetNumberCols();

    for( int col = 0; col<colCount;  ++col )
    {
        // add two spaces to the text and size it.
        wxString colText = aGrid->GetColLabelValue( col ) + wxT( "  " );

        wxSize   needed = sDC.GetTextExtent( colText );

        // set the width of this column
        aGrid->SetColSize( col, needed.x );
    }
}


DIALOG_DESIGN_RULES::DIALOG_DESIGN_RULES( PCB_EDIT_FRAME* parent ) :
    DIALOG_DESIGN_RULES_BASE( parent )
{
    m_Parent = parent;
    SetAutoLayout( true );

    EnsureGridColumnWidths( m_grid );   // override any column widths set by wxformbuilder.

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

    InitDialogRules();
    Layout();
    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );

    // Allow tabbing out of grid controls.  Only available on wxWidgets 2.9.5 or later.
#if wxCHECK_VERSION( 2, 9, 5 )
    m_grid->SetTabBehaviour( wxGrid::Tab_Leave );
    m_gridViaSizeList->SetTabBehaviour( wxGrid::Tab_Leave );
    m_gridTrackWidthList->SetTabBehaviour( wxGrid::Tab_Leave );
#endif

    Center();
}


void DIALOG_DESIGN_RULES::PrintCurrentSettings()
{
    wxString msg, value;

    m_MessagesList->AppendToPage( _( "<b>Current general settings:</b><br>" ) );

    // Display min values:
    value = StringFromValue( g_UserUnit, m_BrdSettings->m_TrackMinWidth, true );
    msg.Printf( _( "Minimum value for tracks width: <b>%s</b><br>\n" ), GetChars( value ) );
    m_MessagesList->AppendToPage( msg );

    value = StringFromValue( g_UserUnit, m_BrdSettings->m_ViasMinSize, true );
    msg.Printf( _( "Minimum value for vias diameter: <b>%s</b><br>\n" ), GetChars( value ) );
    m_MessagesList->AppendToPage( msg );

    value = StringFromValue( g_UserUnit, m_BrdSettings->m_MicroViasMinSize, true );
    msg.Printf( _( "Minimum value for microvias diameter: <b>%s</b><br>\n" ), GetChars( value ) );
    m_MessagesList->AppendToPage( msg );
}


void DIALOG_DESIGN_RULES::InitDialogRules()
{
    SetFocus();
    SetReturnCode( 0 );

    m_Pcb = m_Parent->GetBoard();
    m_BrdSettings = &m_Pcb->GetDesignSettings();

    // Initialize the Rules List
    InitRulesList();

    // copy all NETs into m_AllNets by adding them as NETCUPs.

    // @todo go fix m_Pcb->SynchronizeNetsAndNetClasses() so that the netcode==0 is not present in the BOARD::m_NetClasses
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

    InitializeRulesSelectionBoxes();
    InitGlobalRules();

    PrintCurrentSettings();
}


void DIALOG_DESIGN_RULES::InitGlobalRules()
{
    AddUnitSymbol( *m_ViaMinTitle );
    AddUnitSymbol( *m_ViaMinDrillTitle );
    AddUnitSymbol( *m_MicroViaMinSizeTitle );
    AddUnitSymbol( *m_MicroViaMinDrillTitle );
    AddUnitSymbol( *m_TrackMinWidthTitle );

    PutValueInLocalUnits( *m_SetViasMinSizeCtrl, m_BrdSettings->m_ViasMinSize );
    PutValueInLocalUnits( *m_SetViasMinDrillCtrl, m_BrdSettings->m_ViasMinDrill );

    if( m_BrdSettings->m_BlindBuriedViaAllowed )
        m_OptViaType->SetSelection( 1 );

    m_AllowMicroViaCtrl->SetSelection( m_BrdSettings->m_MicroViasAllowed ? 1 : 0 );
    PutValueInLocalUnits( *m_SetMicroViasMinSizeCtrl, m_BrdSettings->m_MicroViasMinSize );
    PutValueInLocalUnits( *m_SetMicroViasMinDrillCtrl, m_BrdSettings->m_MicroViasMinDrill );
    PutValueInLocalUnits( *m_SetTrackMinWidthCtrl, m_BrdSettings->m_TrackMinWidth );

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

    // Compute the column widths here, after setting texts
    msg = wxT("000000.000000"); // This is a very long text to display values.
                                // Actual values are shorter.
    m_gridViaSizeList->SetCellValue( 0, 0, msg );
    m_gridViaSizeList->SetCellValue( 0, 1, msg );
    m_gridTrackWidthList->SetCellValue( 0, 0, msg );
    m_gridViaSizeList->SetColMinimalWidth( 0, 150 );
    m_gridViaSizeList->SetColMinimalWidth( 1, 150 );
    m_gridViaSizeList->AutoSizeColumns( true );
    m_gridTrackWidthList->SetColMinimalWidth( 0, 150 );
    m_gridTrackWidthList->AutoSizeColumns( true );

    // Fill cells with actual values:
    m_gridViaSizeList->SetCellValue( 0, 0, wxEmptyString );
    m_gridViaSizeList->SetCellValue( 0, 1, wxEmptyString );
    m_gridTrackWidthList->SetCellValue( 0, 0, wxEmptyString );

    for( unsigned ii = 0; ii < m_TracksWidthList.size(); ii++ )
    {
        msg = StringFromValue( g_UserUnit, m_TracksWidthList[ii], false );
        m_gridTrackWidthList->SetCellValue( ii, 0, msg  );
    }

    for( unsigned ii = 0; ii < m_ViasDimensionsList.size(); ii++ )
    {
        msg = StringFromValue( g_UserUnit, m_ViasDimensionsList[ii].m_Diameter, false );
        m_gridViaSizeList->SetCellValue( ii, 0, msg );

        if( m_ViasDimensionsList[ii].m_Drill > 0 )
        {
            msg = StringFromValue( g_UserUnit, m_ViasDimensionsList[ii].m_Drill, false );
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

#if 0 && defined(DEBUG)
    int r = 0;
    for( PNETCUPS::iterator i = ptrList.begin();  i!=ptrList.end();  ++i, ++r )
    {
        printf( "[%d]: %s  %s\n", r, TO_UTF8( (*i)->net ), TO_UTF8( (*i)->clazz ) );
    }

#endif

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


/* Populates combo boxes with the list of existing net classes
 */
void DIALOG_DESIGN_RULES::InitializeRulesSelectionBoxes()
{
    m_rightClassChoice->Clear();
    m_leftClassChoice->Clear();

    m_rightClassChoice->Append( wildCard );
    m_leftClassChoice->Append( wildCard );

    for( int ii = 0; ii < m_grid->GetNumberRows(); ii++ )
    {
        m_rightClassChoice->Append( m_grid->GetRowLabelValue( ii ) );
        m_leftClassChoice->Append( m_grid->GetRowLabelValue( ii ) );
    }

    m_rightClassChoice->Select( 0 );
    m_leftClassChoice->Select( 0 );

    m_buttonRightToLeft->Enable( false );
    m_buttonLeftToRight->Enable( false );;

    FillListBoxWithNetNames( m_leftListCtrl, m_leftClassChoice->GetStringSelection() );
    FillListBoxWithNetNames( m_rightListCtrl, m_rightClassChoice->GetStringSelection() );
}


/* Initialize the rules list from board
 */

static void class2gridRow( wxGrid* grid, int row, NETCLASSPTR nc )
{
    wxString msg;

    // label is netclass name
    grid->SetRowLabelValue( row, nc->GetName() );

    msg = StringFromValue( g_UserUnit, nc->GetClearance() );
    grid->SetCellValue( row, GRID_CLEARANCE, msg );

    msg = StringFromValue( g_UserUnit, nc->GetTrackWidth() );
    grid->SetCellValue( row, GRID_TRACKSIZE, msg );

    msg = StringFromValue( g_UserUnit, nc->GetViaDiameter() );
    grid->SetCellValue( row, GRID_VIASIZE, msg );

    msg = StringFromValue( g_UserUnit, nc->GetViaDrill() );
    grid->SetCellValue( row, GRID_VIADRILL, msg );

    msg = StringFromValue( g_UserUnit, nc->GetuViaDiameter() );
    grid->SetCellValue( row, GRID_uVIASIZE, msg );

    msg = StringFromValue( g_UserUnit, nc->GetuViaDrill() );
    grid->SetCellValue( row, GRID_uVIADRILL, msg );
}


void DIALOG_DESIGN_RULES::InitRulesList()
{
    NETCLASSES& netclasses = m_BrdSettings->m_NetClasses;

    // the +1 is for the Default NETCLASS.
    if( netclasses.GetCount() + 1 > (unsigned) m_grid->GetNumberRows() )
    {
        m_grid->AppendRows( netclasses.GetCount() + 1 - m_grid->GetNumberRows() );
    }

    // enter the Default NETCLASS.
    class2gridRow( m_grid, 0, netclasses.GetDefault() );

    // enter others netclasses
    int row = 1;

    for( NETCLASSES::iterator i = netclasses.begin();  i!=netclasses.end();  ++i, ++row )
    {
        NETCLASSPTR netclass = i->second;

        class2gridRow( m_grid, row, netclass );
    }
}


static void gridRow2class( wxGrid* grid, int row, NETCLASSPTR nc )
{
#define MYCELL( col )   \
    ValueFromString( g_UserUnit, grid->GetCellValue( row, col ) )

    nc->SetClearance( MYCELL( GRID_CLEARANCE ) );
    nc->SetTrackWidth( MYCELL( GRID_TRACKSIZE ) );
    nc->SetViaDiameter( MYCELL( GRID_VIASIZE ) );
    nc->SetViaDrill( MYCELL( GRID_VIADRILL ) );
    nc->SetuViaDiameter( MYCELL( GRID_uVIASIZE ) );
    nc->SetuViaDrill( MYCELL( GRID_uVIADRILL ) );
}


void DIALOG_DESIGN_RULES::CopyRulesListToBoard()
{
    NETCLASSES& netclasses = m_BrdSettings->m_NetClasses;

    // Remove all netclasses from board. We'll copy new list after
    netclasses.Clear();

    // Copy the default NetClass:
    gridRow2class( m_grid, 0, netclasses.GetDefault() );

    // Copy other NetClasses :
    for( int row = 1; row < m_grid->GetNumberRows();  ++row )
    {
        NETCLASSPTR nc = boost::make_shared<NETCLASS>( m_grid->GetRowLabelValue( row ) );

        if( !m_BrdSettings->m_NetClasses.Add( nc ) )
        {
            // this netclass cannot be added because an other netclass with the same name exists
            // Should not occur because OnAddNetclassClick() tests for existing NetClass names
            wxString msg;
            msg.Printf( wxT( "CopyRulesListToBoard(): The NetClass \"%s\" already exists. Skip" ),
                        GetChars( m_grid->GetRowLabelValue( row ) ) );
            wxMessageBox( msg );

            continue;
        }

        gridRow2class( m_grid, row, nc );
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
    m_BrdSettings->m_BlindBuriedViaAllowed = m_OptViaType->GetSelection() > 0;

    // Update vias minimum values for DRC
    m_BrdSettings->m_ViasMinSize = ValueFromTextCtrl( *m_SetViasMinSizeCtrl );
    m_BrdSettings->m_ViasMinDrill = ValueFromTextCtrl( *m_SetViasMinDrillCtrl );

    m_BrdSettings->m_MicroViasAllowed = m_AllowMicroViaCtrl->GetSelection() == 1;

    // Update microvias minimum values for DRC
    m_BrdSettings->m_MicroViasMinSize = ValueFromTextCtrl( *m_SetMicroViasMinSizeCtrl );
    m_BrdSettings->m_MicroViasMinDrill = ValueFromTextCtrl( *m_SetMicroViasMinDrillCtrl );

    // Update tracks minimum values for DRC
    m_BrdSettings->m_TrackMinWidth = ValueFromTextCtrl( *m_SetTrackMinWidthCtrl );
}


void DIALOG_DESIGN_RULES::CopyDimensionsListsToBoard()
{
    wxString msg;

    // Reinitialize m_TrackWidthList
    m_TracksWidthList.clear();

    for( int row = 0; row < m_gridTrackWidthList->GetNumberRows();  ++row )
    {
        msg = m_gridTrackWidthList->GetCellValue( row, 0 );

        if( msg.IsEmpty() )
            continue;

        int value = ValueFromString( g_UserUnit, msg );
        m_TracksWidthList.push_back( value );
    }

    // Sort new list by by increasing value
    sort( m_TracksWidthList.begin(), m_TracksWidthList.end() );

    // Reinitialize m_TrackWidthList
    m_ViasDimensionsList.clear();

    for( int row = 0; row < m_gridViaSizeList->GetNumberRows();  ++row )
    {
        msg = m_gridViaSizeList->GetCellValue( row, 0 );

        if( msg.IsEmpty() )
            continue;

        int           value = ValueFromString( g_UserUnit, msg );
        VIA_DIMENSION via_dim;
        via_dim.m_Diameter = value;
        msg = m_gridViaSizeList->GetCellValue( row, 1 );

        if( !msg.IsEmpty() )
        {
            value = ValueFromString( g_UserUnit, msg );
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


void DIALOG_DESIGN_RULES::OnCancelButtonClick( wxCommandEvent& event )
{
    s_LastTabSelection = m_DRnotebook->GetSelection();

    EndModal( wxID_CANCEL );
}


void DIALOG_DESIGN_RULES::OnOkButtonClick( wxCommandEvent& event )
{
    s_LastTabSelection = m_DRnotebook->GetSelection();

    if( !TestDataValidity() )
    {
        DisplayError( this, _( "Errors detected, Abort" ) );
        return;
    }

    CopyRulesListToBoard();
    CopyGlobalRulesToBoard();
    CopyDimensionsListsToBoard();

    EndModal( wxID_OK );

    m_BrdSettings->SetCurrentNetClass( NETCLASS::Default );
}


void DIALOG_DESIGN_RULES::OnAddNetclassClick( wxCommandEvent& event )
{
    wxString          class_name;

    wxTextEntryDialog dlg( this, _( "New Net Class Name:" ), wxEmptyString, class_name );

    if( dlg.ShowModal() != wxID_OK )
        return; // canceled by user

    class_name = dlg.GetValue();
    class_name.Trim( true );
    class_name.Trim( false );

    if( class_name.IsEmpty() )
        return;         // empty name not allowed

    // The name must dot exists:
    for( int ii = 0; ii < m_grid->GetNumberRows(); ii++ )
    {
        wxString value;
        value = m_grid->GetRowLabelValue( ii );

        if( class_name.CmpNoCase( value ) == 0 )       // Already exists!
        {
            DisplayError( this, _( "This NetClass is already existing, cannot add it; Aborted" ) );
            return;
        }
    }

    m_grid->AppendRows();
    m_grid->SetRowLabelValue( m_grid->GetNumberRows() - 1, class_name );

    // Copy values of the default class:
    int irow = m_grid->GetNumberRows() - 1;

    for( int icol = 0; icol < m_grid->GetNumberCols(); icol++ )
    {
        wxString value;
        value = m_grid->GetCellValue( 0, icol );
        m_grid->SetCellValue( irow, icol, value );
    }

    InitializeRulesSelectionBoxes();
}


// Sort function for wxArrayInt. Items (ints) are sorted by decreasing value
// used in DIALOG_DESIGN_RULES::OnRemoveNetclassClick
int sort_int( int* first, int* second )
{
    return *second - *first;
}


void DIALOG_DESIGN_RULES::OnRemoveNetclassClick( wxCommandEvent& event )
{
    wxArrayInt select = m_grid->GetSelectedRows();

    // Sort selection by decreasing index order:
    select.Sort( sort_int );
    bool reinit = false;

    // rows labels are not removed when deleting rows: they are not deleted.
    // So we must store them, remove corresponding labels and reinit them
    wxArrayString labels;

    for( int ii = 0; ii < m_grid->GetNumberRows(); ii++ )
        labels.Add( m_grid->GetRowLabelValue( ii ) );

    // Delete rows from last to first (this is the order wxArrayInt select after sorting) )
    // This order is Ok when removing rows
    for( unsigned ii = 0; ii < select.GetCount(); ii++ )
    {
        int grid_row = select[ii];

        if( grid_row != 0 )   // Do not remove the default class
        {
            wxString classname = m_grid->GetRowLabelValue( grid_row );
            m_grid->DeleteRows( grid_row );
            labels.RemoveAt( grid_row );  // Remove corresponding row label
            reinit = true;

            // reset the net class to default for members of the removed class
            swapNetClass( classname, NETCLASS::Default );
        }
        else
            wxMessageBox( _( "The default Netclass cannot be removed" ) );
    }

    if( reinit )
    {
        // Reinit labels :
        for( unsigned ii = 1; ii < labels.GetCount(); ii++ )
            m_grid->SetRowLabelValue( ii, labels[ii] );

        InitializeRulesSelectionBoxes();
    }
}


void DIALOG_DESIGN_RULES::OnMoveUpSelectedNetClass( wxCommandEvent& event )
{
    // Cannot move up rules if we have 1 or 2 rules only
    if( m_grid->GetNumberRows() < 3 )
        return;

    wxArrayInt select = m_grid->GetSelectedRows();
    bool       reinit = false;

    for( unsigned irow = 0; irow < select.GetCount(); irow++ )
    {
        int ii = select[irow];
        if( ii < 2 )            // The default netclass *must* be the first netclass
            continue;           // so we cannot move up line 0 and 1

        // Swap the rule and the previous rule
        wxString curr_value, previous_value;

        for( int icol = 0; icol < m_grid->GetNumberCols(); icol++ )
        {
            reinit         = true;
            curr_value     = m_grid->GetCellValue( ii, icol );
            previous_value = m_grid->GetCellValue( ii - 1, icol );
            m_grid->SetCellValue( ii, icol, previous_value );
            m_grid->SetCellValue( ii - 1, icol, curr_value );
        }

        curr_value     = m_grid->GetRowLabelValue( ii );
        previous_value = m_grid->GetRowLabelValue( ii - 1 );
        m_grid->SetRowLabelValue( ii, previous_value );
        m_grid->SetRowLabelValue( ii - 1, curr_value );
    }

    if( reinit )
        InitializeRulesSelectionBoxes();
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
        m_buttonLeftToRight->Enable( false );;
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


bool DIALOG_DESIGN_RULES::TestDataValidity()
{
    bool result = true;

    m_MessagesList->SetPage( wxEmptyString );     // Clear message list

    wxString msg;

    int      minViaDia = ValueFromTextCtrl( *m_SetViasMinSizeCtrl );
    int      minViaDrill = ValueFromTextCtrl( *m_SetViasMinDrillCtrl );
    int      minUViaDia = ValueFromTextCtrl( *m_SetMicroViasMinSizeCtrl );
    int      minUViaDrill = ValueFromTextCtrl( *m_SetMicroViasMinDrillCtrl );
    int      minTrackWidth = ValueFromTextCtrl( *m_SetTrackMinWidthCtrl );
    int      maxval = 1000 * IU_PER_MILS;   // a max value for tracks and vias sizes (1 inch)



    for( int row = 0; row < m_grid->GetNumberRows(); row++ )
    {
        int tracksize = ValueFromString( g_UserUnit,
                                         m_grid->GetCellValue( row, GRID_TRACKSIZE ) );
        if( tracksize < minTrackWidth )
        {
            result = false;
            msg.Printf( _( "%s: <b>Track Size</b> &lt; <b>Min Track Size</b><br>" ),
                        GetChars( m_grid->GetRowLabelValue( row ) ) );

            m_MessagesList->AppendToPage( msg );
        }

        // Test vias
        int viadia = ValueFromString( g_UserUnit,
                                      m_grid->GetCellValue( row, GRID_VIASIZE ) );

        if( viadia < minViaDia )
        {
            result = false;
            msg.Printf( _( "%s: <b>Via Diameter</b> &lt; <b>Minimun Via Diameter</b><br>" ),
                        GetChars( m_grid->GetRowLabelValue( row ) ) );

            m_MessagesList->AppendToPage( msg );
        }

        int viadrill = ValueFromString( g_UserUnit,
                                        m_grid->GetCellValue( row, GRID_VIADRILL ) );

        if( viadrill >= viadia )
        {
            result = false;
            msg.Printf( _( "%s: <b>Via Drill</b> &ge; <b>Via Dia</b><br>" ),
                        GetChars( m_grid->GetRowLabelValue( row ) ) );

            m_MessagesList->AppendToPage( msg );
        }

        if( viadrill < minViaDrill )
        {
            result = false;
            msg.Printf( _( "%s: <b>Via Drill</b> &lt; <b>Min Via Drill</b><br>" ),
                        GetChars( m_grid->GetRowLabelValue( row ) ) );

            m_MessagesList->AppendToPage( msg );
        }

        // Test Micro vias
        int muviadia = ValueFromString( g_UserUnit,
                                        m_grid->GetCellValue( row, GRID_uVIASIZE ) );

        if( muviadia < minUViaDia )
        {
            result = false;
            msg.Printf( _( "%s: <b>MicroVia Diameter</b> &lt; <b>MicroVia Min Diameter</b><br>" ),
                        GetChars( m_grid->GetRowLabelValue( row ) ) );

            m_MessagesList->AppendToPage( msg );
        }

        int muviadrill = ValueFromString( g_UserUnit,
                                          m_grid->GetCellValue( row, GRID_uVIADRILL ) );

        if( muviadrill >= muviadia )
        {
            result = false;
            msg.Printf( _( "%s: <b>MicroVia Drill</b> &ge; <b>MicroVia Dia</b><br>" ),
                        GetChars( m_grid->GetRowLabelValue( row ) ) );

            m_MessagesList->AppendToPage( msg );
        }

        if( muviadrill < minUViaDrill )
        {
            result = false;
            msg.Printf( _( "%s: <b>MicroVia Drill</b> &lt; <b>MicroVia Min Drill</b><br>" ),
                        GetChars( m_grid->GetRowLabelValue( row ) ) );

            m_MessagesList->AppendToPage( msg );
        }
    }

    // Test list of values for specific vias and tracks
    // Test tracks
    for( int row = 0; row < m_gridTrackWidthList->GetNumberRows();  ++row )
    {
        wxString tvalue = m_gridTrackWidthList->GetCellValue( row, 0 );

        if( tvalue.IsEmpty() )
            continue;

        int tracksize = ValueFromString( g_UserUnit, tvalue );

        if( tracksize < minTrackWidth )
        {
            result = false;
            msg.Printf( _( "<b>Extra Track %d Size</b> %s &lt; <b>Min Track Size</b><br>" ),
                        row + 1, GetChars( tvalue ) );

            m_MessagesList->AppendToPage( msg );
        }

        if( tracksize > maxval )
        {
            result = false;
            msg.Printf( _( "<b>Extra Track %d Size</b> %s &gt; <b>1 inch!</b><br>" ),
                        row + 1, GetChars( tvalue ) );

            m_MessagesList->AppendToPage( msg );
        }
    }

    // Test vias
    for( int row = 0; row < m_gridViaSizeList->GetNumberRows();  ++row )
    {
        wxString tvalue = m_gridViaSizeList->GetCellValue( row, 0 );

        if( tvalue.IsEmpty() )
            continue;

        int viadia = ValueFromString( g_UserUnit, tvalue );
        int viadrill = 0;
        wxString drlvalue = m_gridViaSizeList->GetCellValue( row, 1 );

        if( !drlvalue.IsEmpty() )
            viadrill = ValueFromString( g_UserUnit, drlvalue );

        if( viadia < minViaDia )
        {
            result = false;
            msg.Printf( _( "<b>Extra Via %d Size</b> %s &lt; <b>Min Via Size</b><br>" ),
                        row + 1, GetChars( tvalue ) );

            m_MessagesList->AppendToPage( msg );
        }

        if( viadia <= viadrill )
        {
            result = false;
            msg.Printf( _( "<b>Extra Via %d Size</b> %s &le; <b> Drill Size</b> %s<br>" ),
                        row + 1, GetChars( tvalue ), GetChars( drlvalue ) );

            m_MessagesList->AppendToPage( msg );
        }

        // Test for a reasonable via size:
        if( viadia > maxval )    // 1 inch!
        {
            result = false;
            msg.Printf( _( "<b>Extra Via %d Size</b>%s &gt; <b>1 inch!</b><br>" ),
                        row + 1, GetChars( tvalue ) );

            m_MessagesList->AppendToPage( msg );
        }
    }

    return result;
}

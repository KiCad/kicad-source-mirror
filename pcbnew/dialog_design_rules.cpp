/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_design_rules.cpp
/////////////////////////////////////////////////////////////////////////////

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2004-2009 Jean-Pierre Charras, jean-pierre.charras@inpg.fr
 * Copyright (C) 2009 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2009 Kicad Developers, see change_log.txt for contributors.
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


/* functions relatives to the design rules editor
 */
#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"

#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"

#include "pcbnew_id.h"
#include "dialog_design_rules.h"
#include "wx/generic/gridctrl.h"


// Field Positions on rules grid
enum {
    GRID_CLEARANCE,
    GRID_TRACKSIZE,
    GRID_VIASIZE,
    GRID_VIADRILL,
    GRID_uVIASIZE,
    GRID_uVIADRILL,
};

const wxString DIALOG_DESIGN_RULES::wildCard = _("* (Any)");


/***********************************************************************************/
DIALOG_DESIGN_RULES::DIALOG_DESIGN_RULES( WinEDA_PcbFrame* parent ) :
    DIALOG_DESIGN_RULES_BASE( parent )
/***********************************************************************************/
{
    m_Parent = parent;

    wxListItem  column0;
    wxListItem  column1;

    column0.Clear();
    column1.Clear();

    column0.SetImage( -1 );
    column1.SetImage( -1 );

    column0.SetText( _( "Net" ) );
    column1.SetText( _( "Class" ) );

    m_leftListCtrl->InsertColumn( 0, column0 );
    m_leftListCtrl->InsertColumn( 1, column1 );
    m_leftListCtrl->SetColumnWidth( 0, wxLIST_AUTOSIZE );
    m_leftListCtrl->SetColumnWidth( 1, wxLIST_AUTOSIZE );

    m_rightListCtrl->InsertColumn( 0, column0 );
    m_rightListCtrl->InsertColumn( 1, column1 );
    m_rightListCtrl->SetColumnWidth( 0, wxLIST_AUTOSIZE );
    m_rightListCtrl->SetColumnWidth( 1, wxLIST_AUTOSIZE );


    InitDialogRules();
    SetAutoLayout( true );
    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}


/* Display on m_MessagesList the current global settings:
 * minimal values for tracks, vias, clearance ...
 */
void DIALOG_DESIGN_RULES::PrintCurrentSettings( )
{
    wxString msg, value;
    int internal_units = m_Parent->m_InternalUnits;

    m_MessagesList->AppendToPage(_("<b>Current general settings:</b><br>") );

    // Display min values:
    value = ReturnStringFromValue( g_UnitMetric, g_DesignSettings.m_TrackMinWidth, internal_units, true );
    msg.Printf(_("Minimum value for tracks width: <b>%s</b><br>\n"), GetChars( value ) );
    m_MessagesList->AppendToPage(msg);

    value = ReturnStringFromValue( g_UnitMetric, g_DesignSettings.m_ViasMinSize, internal_units, true );
    msg.Printf(_("Minimum value for vias diameter: <b>%s</b><br>\n"), GetChars( value ) );
    m_MessagesList->AppendToPage(msg);

    value = ReturnStringFromValue( g_UnitMetric, g_DesignSettings.m_MicroViasMinSize, internal_units, true );
    msg.Printf(_("Minimum value for microvias diameter: <b>%s</b><br>\n"), GetChars( value ) );
    m_MessagesList->AppendToPage(msg);

}


/**************************************/
void DIALOG_DESIGN_RULES::InitDialogRules()
/**************************************/
{
    SetFocus();
    SetReturnCode( 0 );

    // Initialize the layers grid:
    m_Pcb = m_Parent->GetBoard();

    // Initialize the Rules List
    InitRulesList();

    // copy all NETs into m_AllNets by adding them as NETCUPs.

    // @todo go fix m_Pcb->SynchronizeNetsAndNetClasses() so that the netcode==0 is not present in the BOARD::m_NetClasses


    NETCLASS*   netclass;

    NETCLASSES& netclasses = m_Pcb->m_NetClasses;

    netclass = netclasses.GetDefault();

    // Initialize list of nets for Default Net Class
    for( NETCLASS::const_iterator name = netclass->begin();  name != netclass->end();  ++name )
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

    PrintCurrentSettings( );
}

// Sort comparison function
static bool sortByClassThenName( NETCUP* a, NETCUP* b )
{
    // return a < b

    if( a->clazz < b->clazz )
        return true;

    if( a->net < b->net )
        return true;

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


void DIALOG_DESIGN_RULES::setRowItem( wxListCtrl* aListCtrl, int aRow, NETCUP* aNetAndClass )
{
    wxASSERT( aRow >= 0 );

    // insert blanks if aRow is larger than existing count
    while( aRow >= aListCtrl->GetItemCount() )
    {
        long ndx = aListCtrl->InsertItem( aListCtrl->GetItemCount(), wxEmptyString );

        wxASSERT( ndx >= 0 );

        aListCtrl->SetItem( ndx, 1, wxEmptyString );
    }

    aListCtrl->SetItem( aRow, 0, aNetAndClass->net   );
    aListCtrl->SetItem( aRow, 1, aNetAndClass->clazz );

    // recompute the column widths here, after setting texts
    aListCtrl->SetColumnWidth( 0, wxLIST_AUTOSIZE );
    aListCtrl->SetColumnWidth( 1, wxLIST_AUTOSIZE );
}


/**
 * Function FillListBoxWithNetNames
 * populates aListCtrl with net names and class names from m_AllNets in a two column display.
 */
void DIALOG_DESIGN_RULES::FillListBoxWithNetNames( wxListCtrl* aListCtrl, const wxString& aNetClass )
{
    aListCtrl->DeleteAllItems();

    PNETCUPS    ptrList;

    // get a subset of m_AllNets in pointer form, sorted as desired.
    makePointers( &ptrList, aNetClass );

#if 0 && defined(DEBUG)
    int r = 0;
    for( PNETCUPS::iterator i = ptrList.begin();  i!=ptrList.end();  ++i, ++r )
    {
        printf("[%d]: %s  %s\n", r, CONV_TO_UTF8( (*i)->net ), CONV_TO_UTF8( (*i)->clazz ) );
    }
#endif

    // to speed up inserting we hide the control temporarily
    aListCtrl->Hide();

    int row = 0;
    for( PNETCUPS::iterator i = ptrList.begin();  i!=ptrList.end();  ++i, ++row )
    {
        setRowItem( aListCtrl, row, *i );
    }

    aListCtrl->Show();
}


/* Initialize the combo boxes by the list of existing net classes
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

static void class2gridRow( wxGrid* grid, int row, NETCLASS* nc, int units )
{
    wxString msg;

    // label is netclass name
    grid->SetRowLabelValue( row, nc->GetName() );

    msg = ReturnStringFromValue( g_UnitMetric, nc->GetClearance(), units );
    grid->SetCellValue( row, GRID_CLEARANCE, msg );

    msg = ReturnStringFromValue( g_UnitMetric, nc->GetTrackWidth(), units );
    grid->SetCellValue( row, GRID_TRACKSIZE, msg );

    msg = ReturnStringFromValue( g_UnitMetric, nc->GetViaDiameter(), units );
    grid->SetCellValue( row, GRID_VIASIZE, msg );

    msg = ReturnStringFromValue( g_UnitMetric, nc->GetViaDrill(), units );
    grid->SetCellValue( row, GRID_VIADRILL, msg );

    msg = ReturnStringFromValue( g_UnitMetric, nc->GetuViaDiameter(), units );
    grid->SetCellValue( row, GRID_uVIASIZE, msg );

    msg = ReturnStringFromValue( g_UnitMetric, nc->GetuViaDrill(), units );
    grid->SetCellValue( row, GRID_uVIADRILL, msg );
}

/** Function InitRulesList()
 * Fill the grid showing current rules with values
 */
void DIALOG_DESIGN_RULES::InitRulesList()
{
    NETCLASSES& netclasses = m_Pcb->m_NetClasses;

    // the +1 is for the Default NETCLASS.
    if( netclasses.GetCount()+1 > (unsigned) m_grid->GetNumberRows() )
    {
        m_grid->AppendRows( netclasses.GetCount()+1 - m_grid->GetNumberRows() );
    }

    // enter the Default NETCLASS.
    class2gridRow( m_grid, 0, netclasses.GetDefault(), m_Parent->m_InternalUnits );

    // enter others netclasses
    int row = 1;
    for( NETCLASSES::iterator i=netclasses.begin();  i!=netclasses.end();  ++i, ++row )
    {
        NETCLASS* netclass = i->second;

        class2gridRow( m_grid, row, netclass, m_Parent->m_InternalUnits );
    }
}


static void gridRow2class( wxGrid* grid, int row, NETCLASS* nc, int units )
{
#define MYCELL(col)   \
    ReturnValueFromString( g_UnitMetric, grid->GetCellValue( row, col ), units )

    nc->SetClearance(    MYCELL( GRID_CLEARANCE ) );
    nc->SetTrackWidth(   MYCELL( GRID_TRACKSIZE ) );
    nc->SetViaDiameter(  MYCELL( GRID_VIASIZE ) );
    nc->SetViaDrill(     MYCELL( GRID_VIADRILL ) );
    nc->SetuViaDiameter( MYCELL( GRID_uVIASIZE ) );
    nc->SetuViaDrill(    MYCELL( GRID_uVIADRILL ) );
}


/* Copy the rules list from grid to board
 */
void DIALOG_DESIGN_RULES::CopyRulesListToBoard()
{
    NETCLASSES& netclasses = m_Pcb->m_NetClasses;

    // Remove all netclasses from board. We'll copy new list after
    netclasses.Clear();

    // Copy the default NetClass:
    gridRow2class( m_grid, 0, netclasses.GetDefault(), m_Parent->m_InternalUnits );

    // Copy other NetClasses :
    for( int row = 1; row < m_grid->GetNumberRows();  ++row )
    {
        NETCLASS* nc = new NETCLASS( m_Pcb, m_grid->GetRowLabelValue( row ) );

        if( !m_Pcb->m_NetClasses.Add( nc ) )
        {
            // this netclass cannot be added because an other netclass with the same name exists
            // Should not occur because OnAddNetclassClick() tests for existing NetClass names
            wxString msg;
            msg.Printf( wxT("CopyRulesListToBoard(): The NetClass \"%s\" already exists. Skip"),
                GetChars( m_grid->GetRowLabelValue( row ) ) );
            wxMessageBox( msg );
            delete nc;
            continue;
        }

        gridRow2class( m_grid, row, nc, m_Parent->m_InternalUnits );
    }

    // Now read all nets and push them in the corresponding netclass net buffer
    for( NETCUPS::const_iterator netcup = m_AllNets.begin(); netcup != m_AllNets.end(); ++netcup )
    {
        NETCLASS* nc = netclasses.Find( netcup->clazz );
        wxASSERT( nc );
        nc->Add( netcup->net );
    }

    m_Pcb->SynchronizeNetsAndNetClasses();
}


/*****************************************************************/
void DIALOG_DESIGN_RULES::OnCancelButtonClick( wxCommandEvent& event )
/*****************************************************************/
{
    EndModal( wxID_CANCEL );
}


/**************************************************************************/
void DIALOG_DESIGN_RULES::OnOkButtonClick( wxCommandEvent& event )
/**************************************************************************/
{
    if( !TestDataValidity() )
    {
        DisplayError( this, _( "Errors detected, Abort" ) );
        return;
    }

    CopyRulesListToBoard();

    EndModal( wxID_OK );
}


/**************************************************************************/
void DIALOG_DESIGN_RULES::OnAddNetclassClick( wxCommandEvent& event )
/**************************************************************************/
{
    wxString class_name;

    if( Get_Message( _( "New Net Class Name:" ),
                     wxEmptyString,
                     class_name,
                     this ) )
        return;

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
    m_grid->SetRowLabelValue(
        m_grid->GetNumberRows() - 1,
        class_name );

    // Copy values of the previous class:
    int irow = m_grid->GetNumberRows() - 1;
    for( int icol = 0; icol < m_grid->GetNumberCols(); icol++ )
    {
        wxString value;
        value = m_grid->GetCellValue( irow - 1, icol );
        m_grid->SetCellValue( irow, icol, value );
    }

    InitializeRulesSelectionBoxes();
}


/**************************************************************************/
void DIALOG_DESIGN_RULES::OnRemoveNetclassClick( wxCommandEvent& event )
/**************************************************************************/
{
    wxArrayInt select = m_grid->GetSelectedRows();

    for( int ii = select.GetCount() - 1; ii >= 0; ii-- )
    {
        int grid_row = select[ii];
        if(  grid_row != 0 )   // Do not remove the default class
        {
            wxString classname = m_grid->GetRowLabelValue( grid_row );

            m_grid->DeleteRows( grid_row );

            // reset the net class to default for members of the removed class
            swapNetClass( classname, NETCLASS::Default );
        }
    }

    InitializeRulesSelectionBoxes();
}

/*
 * Called on "Move Up" button click
 * the selected(s) rules are moved up
 * The default netclass is always the first rule
 */
void DIALOG_DESIGN_RULES::OnMoveUpSelectedNetClass( wxCommandEvent& event )
{
    // Cannot move up rules if we have 1 or 2 rules only
    if( m_grid->GetNumberRows() < 3 )
        return;
    wxArrayInt select = m_grid->GetSelectedRows();

    bool reinit = false;
    for( unsigned irow = 0; irow < select.GetCount(); irow++ )
    {
        int ii = select[irow];
        if( ii < 2 )   // The default netclass *must* be the first netclass
            continue;           // so we cannot move up line 0 and 1
        // Swap the rule and the previous rule
        wxString curr_value, previous_value;
        for( int icol = 0; icol < m_grid->GetNumberCols(); icol++ )
        {
            reinit = true;
            curr_value = m_grid->GetCellValue( ii, icol );
            previous_value = m_grid->GetCellValue( ii-1, icol );
            m_grid->SetCellValue( ii, icol, previous_value );
            m_grid->SetCellValue( ii-1, icol, curr_value );
        }
        curr_value = m_grid->GetRowLabelValue( ii );
        previous_value = m_grid->GetRowLabelValue( ii-1 );
        m_grid->SetRowLabelValue(ii, previous_value );
        m_grid->SetRowLabelValue(ii-1, curr_value );
    }

    if( reinit )
        InitializeRulesSelectionBoxes();
}


/*
 * Called on the left Choice Box selection
 */
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


/*
 * Called on the Right Choice Box selection
 */
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


void DIALOG_DESIGN_RULES::moveSelectedItems( wxListCtrl* src, const wxString& newClassName )
{
    wxListItem  item;
    wxString    netName;
    item.m_mask |= wxLIST_MASK_TEXT ;      // Validate the member m_text of the wxListItem item

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


/* Called on clicking the left "select all" button:
 * select alls items of the left netname list lisxt box
 */
void DIALOG_DESIGN_RULES::OnLeftSelectAllButton( wxCommandEvent& event )
{
    for( int ii = 0; ii < m_leftListCtrl->GetItemCount(); ii++ )
        m_leftListCtrl->SetItemState( ii, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
}


/* Called on clicking the right "select all" button:
 * select alls items of the right netname list lisxt box
 */
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


/* TestDataValidity
 * Performs a control of data validity
 * set the background of a bad cell in RED and display an info message
 * @return true if Ok, false if error
 */
bool DIALOG_DESIGN_RULES::TestDataValidity()
{
    bool result = true;

    m_MessagesList->SetPage(wxEmptyString);     // Clear message list

    wxString    msg;

    for( int row = 0; row < m_grid->GetNumberRows(); row++ )
    {
        int tracksize = ReturnValueFromString( g_UnitMetric,
                                       m_grid->GetCellValue( row, GRID_TRACKSIZE ),
                                       m_Parent->m_InternalUnits );
        if( tracksize < g_DesignSettings.m_TrackMinWidth )
        {
            result = false;
            msg.Printf( _( "%s: <b>Track Size</b> &lt; <b>Min Track Size</b><br>" ),
                GetChars( m_grid->GetRowLabelValue(row)) );

            m_MessagesList->AppendToPage( msg );
        }

        // Test vias
        int viadia = ReturnValueFromString( g_UnitMetric,
                                       m_grid->GetCellValue( row, GRID_VIASIZE ),
                                       m_Parent->m_InternalUnits );

        if( viadia < g_DesignSettings.m_ViasMinSize )
        {
            result = false;
            msg.Printf( _( "%s: <b>Via Diameter</b> &lt; <b>Minimun Via Diameter</b><br>" ),
                GetChars( m_grid->GetRowLabelValue(row)) );

            m_MessagesList->AppendToPage( msg );
        }

        int viadrill = ReturnValueFromString( g_UnitMetric,
                                          m_grid->GetCellValue( row, GRID_VIADRILL ),
                                          m_Parent->m_InternalUnits );
        if( viadrill && viadrill >= viadia )
        {
            result = false;
            msg.Printf( _( "%s: <b>Via Drill</b> &ge; <b>Via Dia</b><br>" ),
                GetChars( m_grid->GetRowLabelValue(row)) );

            m_MessagesList->AppendToPage( msg );
        }

        // Test Micro vias
        int muviadia = ReturnValueFromString( g_UnitMetric,
                                       m_grid->GetCellValue( row, GRID_uVIASIZE ),
                                       m_Parent->m_InternalUnits );

        if( muviadia < g_DesignSettings.m_MicroViasMinSize )
        {
            result = false;
            msg.Printf( _( "%s: <b>MicroVia Diameter</b> &lt; <b>Minimun MicroVia Diameter</b><br>" ),
                GetChars( m_grid->GetRowLabelValue(row)) );

            m_MessagesList->AppendToPage( msg );
        }

        int muviadrill = ReturnValueFromString( g_UnitMetric,
                                          m_grid->GetCellValue( row, GRID_uVIADRILL ),
                                          m_Parent->m_InternalUnits );
        if( muviadrill && muviadrill >= muviadia )
        {
            result = false;
            msg.Printf( _( "%s: <b>MicroVia Drill</b> &ge; <b>MicroVia Dia</b><br>" ),
                GetChars( m_grid->GetRowLabelValue(row)) );

            m_MessagesList->AppendToPage( msg );
        }
    }

    return result;
}

/////////////////////////////////////////////////////////////////////////////

// Name:        dialog_design_rules.cpp
// Author:      jean-pierre Charras
/////////////////////////////////////////////////////////////////////////////

/* functions relatives to the design rules editor
 */
#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"

#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"

#include "id.h"
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


    Init();
    SetAutoLayout( true );
    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}


/********************************************************************/
void DIALOG_DESIGN_RULES::Init()
/********************************************************************/
{
    SetFocus();
    SetReturnCode( 0 );

    // Initialize the layers grid:
    m_Pcb = m_Parent->GetBoard();

    // Initialize the Rules List
    InitRulesList();

    // copy all NETs into m_AllNets by adding them as NETCUPs.

    NETCLASS*   netclass;

    NETCLASSES& netclasses = m_Pcb->m_NetClasses;

    netclass = netclasses.GetDefault();

    for( NETCLASS::const_iterator name = netclass->begin();  name != netclass->end();  ++name )
    {
        m_AllNets.push_back( NETCUP( *name, netclass->GetName() ) );
    }

    for( NETCLASSES::const_iterator nc = netclasses.begin();  nc != netclasses.end();  ++nc )
    {
        netclass = nc->second;

        for( NETCLASS::const_iterator name = netclass->begin();  name != netclass->end();  ++name )
        {
            m_AllNets.push_back( NETCUP( *name, netclass->GetName() ) );
        }
    }

    InitializeRulesSelectionBoxes();
}

// Sort comparison function
static bool sortByClassThenName( NETCUP* a, NETCUP* b )
{
    // return a < b

    if( a->clazz < b->clazz )
        return true;

    if( a->net < a->net )
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
 * populates aListCtrl with net names members of the aNetclassIndex net class
 * the "Client Data pointer" is used to store the index of nets in the net lists
 */
void DIALOG_DESIGN_RULES::FillListBoxWithNetNames( wxListCtrl* aListCtrl, const wxString& aNetClass )
{
    aListCtrl->ClearAll();

#if 1
    PNETCUPS    ptrList;

    makePointers( &ptrList, aNetClass );

#if defined(DEBUG)
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

#endif

}


/* Initialize the combno boxes by the list of existing net classes
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

void DIALOG_DESIGN_RULES::InitRulesList()
{
    NETCLASSES& netclasses = m_Pcb->m_NetClasses;

    // the +1 is for the Default NETCLASS.
    if( netclasses.GetCount()+1 > (unsigned) m_grid->GetNumberRows() )
    {
        m_grid->AppendRows( netclasses.GetCount()+1 - m_grid->GetNumberRows() );
    }

    class2gridRow( m_grid, 0, netclasses.GetDefault(), m_Parent->m_InternalUnits );

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


/* Copy the rules list to board
 */
void DIALOG_DESIGN_RULES::CopyRulesListToBoard()
{
    NETCLASSES& netclasses = m_Pcb->m_NetClasses;

    netclasses.Clear();

    // gridRow2class( wxGrid* grid, int row, NETCLASS* nc, int units )

    gridRow2class( m_grid, 0, netclasses.GetDefault(), m_Parent->m_InternalUnits );

    for( int row = 1; row < m_grid->GetNumberRows();  ++row )
    {
        NETCLASS* nc = new NETCLASS( m_Pcb, m_grid->GetRowLabelValue( row ) );

        if( !m_Pcb->m_NetClasses.Add( nc ) )
        {
            // @todo: put up an error message here.

            delete nc;
            continue;
        }

        gridRow2class( m_grid, row, nc, m_Parent->m_InternalUnits );
    }

    for( NETCUPS::const_iterator netcup = m_AllNets.begin();  netcup != m_AllNets.end();  ++netcup )
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
    EndModal( 0 );
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
        if( select[ii] != 0 )   // Do not remove the default class
        {
            wxString classname = m_grid->GetRowLabelValue( ii );

            m_grid->DeleteRows( select[ii] );

            // reset the net class to default for members of the removed class
            swapNetClass( classname, NETCLASS::Default );
        }
    }

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


/* Called on clicking the "<<<" or Copy Right to Left button:
 * Selected items are moved from the right list to the left list
 */

void DIALOG_DESIGN_RULES::OnRightToLeftCopyButton( wxCommandEvent& event )
{
    wxString oldClassName = m_leftClassChoice->GetStringSelection();
    wxString newClassName = m_rightClassChoice->GetStringSelection();

    wxASSERT( oldClassName != wxEmptyString );
    wxASSERT( newClassName != wxEmptyString );

    for( int row = 0;  row < m_rightListCtrl->GetItemCount();  ++row )
    {
        if( !m_rightListCtrl->GetItemState( row, wxLIST_STATE_SELECTED ) )
            continue;

/*
        @todo: get the netName, call setNetClass()
        wxString netName = m_rightListCtrl->OnGetItemText( row, 0 );

        setNetClass( netName, newClassName == wildCard ? NETCLASS::Default : newClassName );
*/
    }

    FillListBoxWithNetNames( m_leftListCtrl, m_leftClassChoice->GetStringSelection() );
    FillListBoxWithNetNames( m_rightListCtrl, m_rightClassChoice->GetStringSelection() );
}


/* Called on clicking the ">>>" or Copy Left to Right button:
 * Selected items are moved from the left list to the right list
 */
void DIALOG_DESIGN_RULES::OnLeftToRightCopyButton( wxCommandEvent& event )
{
    // @todo factor code from above, or combine the two functions.

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
        int viadia = ReturnValueFromString( g_UnitMetric,
                                       m_grid->GetCellValue(
                                           row, GRID_VIASIZE ),
                                       m_Parent->m_InternalUnits );

        int viadrill = ReturnValueFromString( g_UnitMetric,
                                          m_grid->GetCellValue(
                                          row, GRID_VIADRILL ),
                                          m_Parent->m_InternalUnits );
        if( viadrill && viadrill >= viadia )
        {
            result = false;
            msg.Printf( _( "%s: <b>Via Drill</b> &ge; <b>Via Dia</b><br>" ),
                GetChars( m_grid->GetRowLabelValue(row)) );

            m_MessagesList->AppendToPage( msg );
        }
    }

    return result;
}

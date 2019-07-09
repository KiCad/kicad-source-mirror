/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2009 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2009-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <base_units.h>
#include <bitmaps.h>
#include <board_design_settings.h>
#include <confirm.h>
#include <grid_tricks.h>
#include <panel_setup_netclasses.h>
#include <pcb_edit_frame.h>
#include <tool/tool_manager.h>
#include <widgets/wx_grid.h>

// Columns of netclasses grid
enum {
    GRID_NAME = 0,
    GRID_CLEARANCE,
    GRID_TRACKSIZE,
    GRID_VIASIZE,
    GRID_VIADRILL,
    GRID_uVIASIZE,
    GRID_uVIADRILL,
    GRID_DIFF_PAIR_WIDTH,
    GRID_DIFF_PAIR_GAP,
    GRID_DIFF_PAIR_VIA_GAP
};


PANEL_SETUP_NETCLASSES::PANEL_SETUP_NETCLASSES( PAGED_DIALOG* aParent, PCB_EDIT_FRAME* aFrame,
                                                PANEL_SETUP_FEATURE_CONSTRAINTS* aConstraintsPanel ) :
    PANEL_SETUP_NETCLASSES_BASE( aParent->GetTreebook() )
{
    m_Parent = aParent;
    m_Frame = aFrame;
    m_Pcb = m_Frame->GetBoard();
    m_BrdSettings = &m_Pcb->GetDesignSettings();
    m_ConstraintsPanel = aConstraintsPanel;

    m_netclassesDirty = true;

    // Figure out the smallest the netclass membership pane can ever be so that nothing is cutoff
    // and force it to be that size.
    m_membershipSize = GetSize();
    m_membershipSize.y -= m_netclassesPane->GetSize().y;
    m_membershipSize.x = -1;
    m_membershipPane->SetMinSize( m_membershipSize );
    m_membershipPane->SetMaxSize( m_membershipSize );

    // Prevent Size events from firing before we are ready
    Freeze();
    m_originalColWidths = new int[ m_netclassGrid->GetNumberCols() ];
    // Calculate a min best size to handle longest usual numeric values:
    // (The 'M' large char is used to give a margin)
    int min_best_width = m_netclassGrid->GetTextExtent( "555,555555 milsM" ).x;

    for( int i = 0; i < m_netclassGrid->GetNumberCols(); ++i )
    {
        // We calculate the column min size only from texts sizes, not using the initial col width
        // as this initial width is sometimes strange depending on the language (wxGrid bug?)
        int min_width =  m_netclassGrid->GetVisibleWidth( i, true, true, false );
        m_netclassGrid->SetColMinimalWidth( i, min_width );
        // We use a "best size" >= min_best_width
        m_originalColWidths[ i ] = std::max( min_width, min_best_width );
        m_netclassGrid->SetColSize( i, m_originalColWidths[ i ] );
    }

    // Be sure the column labels are readable
    m_netclassGrid->EnsureColLabelsVisible();

    // Membership combobox editors require a bit more room, so increase the row size of
    // all our grids for consistency
    m_netclassGrid->SetDefaultRowSize( m_netclassGrid->GetDefaultRowSize() + 4 );
    m_membershipGrid->SetDefaultRowSize( m_membershipGrid->GetDefaultRowSize() + 4 );

    m_netclassGrid->PushEventHandler( new GRID_TRICKS( m_netclassGrid ) );
    m_membershipGrid->PushEventHandler( new GRID_TRICKS( m_membershipGrid ) );

    m_netclassGrid->SetSelectionMode( wxGrid::wxGridSelectRows );
    m_membershipGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    // Set up the net name column of the netclass membership grid to read-only
    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetReadOnly( true );
    m_membershipGrid->SetColAttr( 0, attr );

    m_addButton->SetBitmap( KiBitmap( small_plus_xpm ) );
    m_removeButton->SetBitmap( KiBitmap( trash_xpm ) );

    // wxFormBuilder doesn't include this event...
    m_netclassGrid->Connect( wxEVT_GRID_CELL_CHANGING, wxGridEventHandler( PANEL_SETUP_NETCLASSES::OnNetclassGridCellChanging ), NULL, this );

    Thaw();
}


PANEL_SETUP_NETCLASSES::~PANEL_SETUP_NETCLASSES()
{
    delete [] m_originalColWidths;

    // Delete the GRID_TRICKS.
    m_netclassGrid->PopEventHandler( true );
    m_membershipGrid->PopEventHandler( true );

    m_netclassGrid->Disconnect( wxEVT_GRID_CELL_CHANGING, wxGridEventHandler( PANEL_SETUP_NETCLASSES::OnNetclassGridCellChanging ), NULL, this );
}


static void netclassToGridRow( EDA_UNITS_T aUnits, wxGrid* aGrid, int aRow, const NETCLASSPTR& nc )
{
    aGrid->SetCellValue( aRow, GRID_NAME, nc->GetName() );

#define SET_MILS_CELL( col, val ) \
    aGrid->SetCellValue( aRow, col, StringFromValue( aUnits, val, true, true ) )

    SET_MILS_CELL( GRID_CLEARANCE, nc->GetClearance() );
    SET_MILS_CELL( GRID_TRACKSIZE, nc->GetTrackWidth() );
    SET_MILS_CELL( GRID_VIASIZE, nc->GetViaDiameter() );
    SET_MILS_CELL( GRID_VIADRILL, nc->GetViaDrill() );
    SET_MILS_CELL( GRID_uVIASIZE, nc->GetuViaDiameter() );
    SET_MILS_CELL( GRID_uVIADRILL, nc->GetuViaDrill() );
    SET_MILS_CELL( GRID_DIFF_PAIR_WIDTH, nc->GetDiffPairWidth() );
    SET_MILS_CELL( GRID_DIFF_PAIR_GAP, nc->GetDiffPairGap() );
    // 6.0 TODO: SET_MILS_CELL( GRID_DIFF_PAIR_VIA_GAP, nc->GetDiffPairViaGap() );
}


bool PANEL_SETUP_NETCLASSES::TransferDataToWindow()
{
    NETCLASSES& netclasses = m_BrdSettings->m_NetClasses;
    NETCLASSPTR netclass = netclasses.GetDefault();

    if( m_netclassGrid->GetNumberRows() )
        m_netclassGrid->DeleteRows( 0, m_netclassGrid->GetNumberRows() );
    m_netclassGrid->AppendRows( netclasses.GetCount() + 1 ); // + 1 for default netclass

    // enter the Default NETCLASS.
    netclassToGridRow( m_Frame->GetUserUnits(), m_netclassGrid, 0, netclass );

    // make the Default NETCLASS name read-only
    wxGridCellAttr* cellAttr = m_netclassGrid->GetOrCreateCellAttr( 0, GRID_NAME );
    cellAttr->SetReadOnly();
    cellAttr->DecRef();

    // enter other netclasses
    int row = 1;

    for( NETCLASSES::iterator i = netclasses.begin();  i != netclasses.end();  ++i, ++row )
        netclassToGridRow( m_Frame->GetUserUnits(), m_netclassGrid, row, i->second );

    // ensure that all nets have net classes assigned
    m_Pcb->BuildListOfNets();

    if( m_membershipGrid->GetNumberRows() )
        m_membershipGrid->DeleteRows( 0, m_membershipGrid->GetNumberRows() );

    for( NETINFO_ITEM* net : m_Pcb->GetNetInfo() )
    {
        if( net->GetNet() > 0 && net->IsCurrent() )
            addNet( UnescapeString( net->GetNetname() ), net->GetNetClass()->GetName() );
    }

    return true;
}


void PANEL_SETUP_NETCLASSES::addNet( wxString netName, const wxString& netclass )
{
    int i = m_membershipGrid->GetNumberRows();

    m_membershipGrid->AppendRows( 1 );

    m_membershipGrid->SetCellValue( i, 0, netName );
    m_membershipGrid->SetCellValue( i, 1, netclass );
}


/* Populates drop-downs with the list of net classes
 */
void PANEL_SETUP_NETCLASSES::rebuildNetclassDropdowns()
{
    m_membershipGrid->CommitPendingChanges( true );

    wxArrayString netclassNames;

    for( int ii = 0; ii < m_netclassGrid->GetNumberRows(); ii++ )
    {
        wxString netclassName = m_netclassGrid->GetCellValue( ii, GRID_NAME );
        if( !netclassName.IsEmpty() )
            netclassNames.push_back( netclassName );
    }

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetEditor( new wxGridCellChoiceEditor( netclassNames ) );
    m_membershipGrid->SetColAttr( 1, attr );

    m_assignNetClass->Set( netclassNames );

    netclassNames.Insert( wxEmptyString, 0 );
    m_netClassFilter->Set( netclassNames );
}


static void gridRowToNetclass( EDA_UNITS_T aUnits, wxGrid* grid, int row, const NETCLASSPTR& nc )
{
    nc->SetName( grid->GetCellValue( row, GRID_NAME ) );

#define MYCELL( col )   \
    ValueFromString( aUnits, grid->GetCellValue( row, col ), true )

    nc->SetClearance( MYCELL( GRID_CLEARANCE ) );
    nc->SetTrackWidth( MYCELL( GRID_TRACKSIZE ) );
    nc->SetViaDiameter( MYCELL( GRID_VIASIZE ) );
    nc->SetViaDrill( MYCELL( GRID_VIADRILL ) );
    nc->SetuViaDiameter( MYCELL( GRID_uVIASIZE ) );
    nc->SetuViaDrill( MYCELL( GRID_uVIADRILL ) );
    nc->SetDiffPairWidth( MYCELL( GRID_DIFF_PAIR_WIDTH ) );
    nc->SetDiffPairGap( MYCELL( GRID_DIFF_PAIR_GAP ) );
    // 6.0 TODO: nc->SetDiffPairViaGap( MYCELL( GRID_DIFF_PAIR_VIA_GAP ) );
}


bool PANEL_SETUP_NETCLASSES::TransferDataFromWindow()
{
    if( !validateData() )
        return false;

    NETCLASSES& netclasses = m_BrdSettings->m_NetClasses;

    // Remove all netclasses from board. We'll copy new list after
    netclasses.Clear();

    // Copy the default NetClass:
    gridRowToNetclass( m_Frame->GetUserUnits(), m_netclassGrid, 0, netclasses.GetDefault());

    // Copy other NetClasses :
    for( int row = 1; row < m_netclassGrid->GetNumberRows();  ++row )
    {
        NETCLASSPTR nc = std::make_shared<NETCLASS>( m_netclassGrid->GetCellValue( row, GRID_NAME ) );

        if( m_BrdSettings->m_NetClasses.Add( nc ) )
            gridRowToNetclass( m_Frame->GetUserUnits(), m_netclassGrid, row, nc );
    }

    // Now read all nets and push them in the corresponding netclass net buffer
    for( int row = 0; row < m_membershipGrid->GetNumberRows(); ++row )
    {
        NETCLASSPTR nc = netclasses.Find( m_membershipGrid->GetCellValue( row, 1 ) );

        if( nc )
            nc->Add( m_membershipGrid->GetCellValue( row, 0 ) );
    }

    m_Pcb->SynchronizeNetsAndNetClasses();
    m_BrdSettings->SetCurrentNetClass( NETCLASS::Default );

    if( auto toolmgr = m_Frame->GetToolManager() )
        toolmgr->ResetTools( TOOL_BASE::MODEL_RELOAD );

    return true;
}


bool PANEL_SETUP_NETCLASSES::validateNetclassName( int aRow, wxString aName, bool focusFirst )
{
    aName.Trim( true );
    aName.Trim( false );

    if( aName.IsEmpty() )
    {
        wxString msg =  _( "Netclass must have a name." );
        m_Parent->SetError( msg, this, m_netclassGrid, aRow, GRID_NAME );
        return false;
    }

    for( int ii = 0; ii < m_netclassGrid->GetNumberRows(); ii++ )
    {
        if( ii != aRow && m_netclassGrid->GetRowLabelValue( ii ).CmpNoCase( aName ) == 0 )
        {
            wxString msg = _( "Netclass name already in use." );
            m_Parent->SetError( msg, this, m_netclassGrid, focusFirst ? aRow : ii, GRID_NAME );
            return false;
        }
    }

    return true;
}


void PANEL_SETUP_NETCLASSES::OnNetclassGridCellChanging( wxGridEvent& event )
{
    if( event.GetCol() == GRID_NAME )
    {
        if( validateNetclassName( event.GetRow(), event.GetString() ) )
            m_netclassesDirty = true;
        else
            event.Veto();
    }
}


void PANEL_SETUP_NETCLASSES::OnAddNetclassClick( wxCommandEvent& event )
{
    if( !m_netclassGrid->CommitPendingChanges() )
        return;

    int row = m_netclassGrid->GetNumberRows();
    m_netclassGrid->AppendRows();

    // Copy values of the default class:
    for( int col = 1; col < m_netclassGrid->GetNumberCols(); col++ )
        m_netclassGrid->SetCellValue( row, col, m_netclassGrid->GetCellValue( 0, col ) );

    m_netclassGrid->MakeCellVisible( row, 0 );
    m_netclassGrid->SetGridCursor( row, 0 );

    m_netclassGrid->EnableCellEditControl( true );
    m_netclassGrid->ShowCellEditControl();

    m_netclassesDirty = true;
}


void PANEL_SETUP_NETCLASSES::OnRemoveNetclassClick( wxCommandEvent& event )
{
    if( !m_netclassGrid->CommitPendingChanges() )
        return;

    int curRow = m_netclassGrid->GetGridCursorRow();

    if( curRow < 0 )
        return;
    else if( curRow == 0 )
    {
        DisplayErrorMessage( this, _( "The default net class is required." ) );
        return;
    }

    // reset the net class to default for members of the removed class
    wxString classname = m_netclassGrid->GetCellValue( curRow, GRID_NAME );

    for( int row = 0; row < m_membershipGrid->GetNumberRows(); ++row )
    {
        if( m_membershipGrid->GetCellValue( row, 1 ) == classname )
            m_membershipGrid->SetCellValue( row, 1, NETCLASS::Default );
    }

    m_netclassGrid->DeleteRows( curRow, 1 );

    m_netclassGrid->MakeCellVisible( std::max( 0, curRow-1 ), m_netclassGrid->GetGridCursorCol() );
    m_netclassGrid->SetGridCursor( std::max( 0, curRow-1 ), m_netclassGrid->GetGridCursorCol() );

    m_netclassesDirty = true;
}


void PANEL_SETUP_NETCLASSES::AdjustNetclassGridColumns( int aWidth )
{
    // Account for scroll bars
    aWidth -= ( m_netclassGrid->GetSize().x - m_netclassGrid->GetClientSize().x );

    for( int i = 1; i < m_netclassGrid->GetNumberCols(); i++ )
    {
        m_netclassGrid->SetColSize( i, m_originalColWidths[ i ] );
        aWidth -= m_originalColWidths[ i ];
    }

    m_netclassGrid->SetColSize( 0, std::max( aWidth, m_originalColWidths[ 0 ] ) );
}


void PANEL_SETUP_NETCLASSES::OnSizeNetclassGrid( wxSizeEvent& event )
{
    AdjustNetclassGridColumns( event.GetSize().GetX() );

    event.Skip();
}


void PANEL_SETUP_NETCLASSES::AdjustMembershipGridColumns( int aWidth )
{
    // Account for scroll bars
    aWidth -= ( m_membershipGrid->GetSize().x - m_membershipGrid->GetClientSize().x );

    // Set className column width to original className width from netclasses grid
    int classNameWidth = m_originalColWidths[ 0 ];
    m_membershipGrid->SetColSize( 1, m_originalColWidths[ 0 ] );
    m_membershipGrid->SetColSize( 0, std::max( aWidth - classNameWidth, classNameWidth ) );
}


void PANEL_SETUP_NETCLASSES::OnSizeMembershipGrid( wxSizeEvent& event )
{
    AdjustMembershipGridColumns( event.GetSize().GetX() );

    event.Skip();
}


void PANEL_SETUP_NETCLASSES::doApplyFilters( bool aShowAll )
{
    if( !m_membershipGrid->CommitPendingChanges() )
        return;

    wxString netClassFilter = m_netClassFilter->GetStringSelection();
    wxString netFilter = m_netNameFilter->GetValue().MakeLower();

    if( !netFilter.IsEmpty() )
        netFilter = wxT( "*" ) + netFilter + wxT( "*" );

    for( int row = 0; row < m_membershipGrid->GetNumberRows(); ++row )
    {
        wxString net = m_membershipGrid->GetCellValue( row, 0 );
        wxString netClass = m_membershipGrid->GetCellValue( row, 1 );
        bool show = true;

        if( !aShowAll )
        {
            if( !netFilter.IsEmpty() && !net.MakeLower().Matches( netFilter ) )
                show = false;

            if( !netClassFilter.IsEmpty() && netClass != netClassFilter )
                show = false;
        }

        if( show )
            m_membershipGrid->ShowRow( row );
        else
            m_membershipGrid->HideRow( row );
    }
}


void PANEL_SETUP_NETCLASSES::doAssignments( bool aAssignAll )
{
    if( !m_membershipGrid->CommitPendingChanges() )
        return;

    wxArrayInt selectedRows = m_membershipGrid->GetSelectedRows();

    for( int row = 0; row < m_membershipGrid->GetNumberRows(); ++row )
    {
        if( !m_membershipGrid->IsRowShown( row ) )
            continue;

        if( !aAssignAll && selectedRows.Index( row ) == wxNOT_FOUND )
            continue;

        m_membershipGrid->SetCellValue( row, 1, m_assignNetClass->GetStringSelection() );
    }
}


void PANEL_SETUP_NETCLASSES::OnUpdateUI( wxUpdateUIEvent& event )
{
    if( m_netclassesDirty )
    {
        rebuildNetclassDropdowns();
        m_netclassesDirty = false;
    }

    // Recompute the desired size for the two content panes. We cannot leave this sizing to
    // wxWidgets because it wants to shrink the membership panel to an unusable size when the
    // netlist panel grows, and also it introduces undesired artifacts when the window is resized
    // and the panes can grow/shrink.
    wxSize netclassSize = GetClientSize();
    netclassSize.y -= m_membershipSize.y;

    m_netclassesPane->SetMinSize( netclassSize );
    m_netclassesPane->SetMaxSize( netclassSize );
    Layout();
}


int PANEL_SETUP_NETCLASSES::getNetclassValue( int aRow, int aCol )
{
    return ValueFromString( m_Frame->GetUserUnits(), m_netclassGrid->GetCellValue( aRow, aCol ), true );
}


bool PANEL_SETUP_NETCLASSES::validateData()
{
    if( !m_netclassGrid->CommitPendingChanges() || !m_membershipGrid->CommitPendingChanges() )
        return false;

    wxString msg;
    int minViaDia = m_ConstraintsPanel->m_viaMinSize.GetValue();
    int minViaDrill = m_ConstraintsPanel->m_viaMinDrill.GetValue();
    int minUViaDia = m_ConstraintsPanel->m_uviaMinSize.GetValue();
    int minUViaDrill = m_ConstraintsPanel->m_uviaMinDrill.GetValue();
    int minTrackWidth = m_ConstraintsPanel->m_trackMinWidth.GetValue();

    // Test net class parameters.
    for( int row = 0; row < m_netclassGrid->GetNumberRows(); row++ )
    {
        wxString netclassName = m_netclassGrid->GetCellValue( row, GRID_NAME );
        netclassName.Trim( true );
        netclassName.Trim( false );

        if( !validateNetclassName( row, netclassName, false ) )
            return false;

        if( getNetclassValue( row, GRID_TRACKSIZE ) < minTrackWidth )
        {
            msg.Printf( _( "Track width less than minimum track width (%s)." ),
                        StringFromValue( m_Frame->GetUserUnits(), minTrackWidth, true, true ) );
            m_Parent->SetError( msg, this, m_netclassGrid, row, GRID_TRACKSIZE );
            return false;
        }

        if( getNetclassValue( row, GRID_DIFF_PAIR_WIDTH ) < minTrackWidth )
        {
            msg.Printf( _( "Differential pair width less than minimum track width (%s)." ),
                        StringFromValue( m_Frame->GetUserUnits(), minTrackWidth, true, true ) );
            m_Parent->SetError( msg, this, m_netclassGrid, row, GRID_DIFF_PAIR_WIDTH );
            return false;
        }

        // Test vias
        if( getNetclassValue( row, GRID_VIASIZE ) < minViaDia )
        {
            msg.Printf( _( "Via diameter less than minimum via diameter (%s)." ),
                        StringFromValue( m_Frame->GetUserUnits(), minViaDia, true, true ) );
            m_Parent->SetError( msg, this, m_netclassGrid, row, GRID_VIASIZE );
            return false;
        }

        if( getNetclassValue( row, GRID_VIADRILL ) >= getNetclassValue( row, GRID_VIASIZE ) )
        {
            msg = _( "Via drill larger than via diameter." );
            m_Parent->SetError( msg, this, m_netclassGrid, row, GRID_VIADRILL );
            return false;
        }

        if( getNetclassValue( row, GRID_VIADRILL ) < minViaDrill )
        {
            msg.Printf( _( "Via drill less than minimum via drill (%s)." ),
                        StringFromValue( m_Frame->GetUserUnits(), minViaDrill, true, true ) );
            m_Parent->SetError( msg, this, m_netclassGrid, row, GRID_VIADRILL );
            return false;
        }

        // Test Micro vias
        if( getNetclassValue( row, GRID_uVIASIZE ) < minUViaDia )
        {
            msg.Printf( _( "Microvia diameter less than minimum microvia diameter (%s)." ),
                        StringFromValue( m_Frame->GetUserUnits(), minUViaDia, true, true ) );
            m_Parent->SetError( msg, this, m_netclassGrid, row, GRID_uVIASIZE );
            return false;
        }

        if( getNetclassValue( row, GRID_uVIADRILL ) >= getNetclassValue( row, GRID_uVIASIZE ) )
        {
            msg = _( "Microvia drill larger than microvia diameter." );
            m_Parent->SetError( msg, this, m_netclassGrid, row, GRID_uVIADRILL );
            return false;
        }

        if( getNetclassValue( row, GRID_uVIADRILL ) < minUViaDrill )
        {
            msg.Printf( _( "Microvia drill less than minimum microvia drill (%s)." ),
                        StringFromValue( m_Frame->GetUserUnits(), minUViaDrill, true, true ) );
            m_Parent->SetError( msg, this, m_netclassGrid, row, GRID_uVIADRILL );
            return false;
        }
    }

    return true;
}


void PANEL_SETUP_NETCLASSES::ImportSettingsFrom( BOARD* aBoard )
{
    // Note: do not change the board, as we need to get the current nets from it for
    // netclass memberships.  All the netclass definitions and dimension lists are in
    // the BOARD_DESIGN_SETTINGS.

    BOARD_DESIGN_SETTINGS* savedSettings = m_BrdSettings;

    m_BrdSettings = &aBoard->GetDesignSettings();
    TransferDataToWindow();

    m_netclassGrid->ForceRefresh();
    m_membershipGrid->ForceRefresh();

    m_BrdSettings = savedSettings;
}



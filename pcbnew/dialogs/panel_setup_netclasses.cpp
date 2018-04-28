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


#include <fctsys.h>
#include <class_drawpanel.h>
#include <base_units.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <board_design_settings.h>
#include <bitmaps.h>
#include <widgets/wx_grid.h>

#include <panel_setup_netclasses.h>


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
    PANEL_SETUP_NETCLASSES_BASE( aParent )
{
    m_Parent = aParent;
    m_Frame = aFrame;
    m_Pcb = m_Frame->GetBoard();
    m_BrdSettings = &m_Pcb->GetDesignSettings();
    m_ConstraintsPanel = aConstraintsPanel;

    m_netclassesDirty = true;

    m_originalColWidths = new int[ m_netclassGrid->GetNumberCols() ];

    for( int i = 0; i < m_netclassGrid->GetNumberCols(); ++i )
        m_originalColWidths[ i ] = m_netclassGrid->GetColSize( i );

    // Membership combobox editors require a bit more room, so increase the row size of
    // all our grids for consistency
    m_netclassGrid->SetDefaultRowSize(    m_netclassGrid->GetDefaultRowSize()    + 4 );
    m_membershipGrid->SetDefaultRowSize(  m_membershipGrid->GetDefaultRowSize()  + 4 );
    m_trackWidthsGrid->SetDefaultRowSize( m_trackWidthsGrid->GetDefaultRowSize() + 4 );
    m_viaSizesGrid->SetDefaultRowSize(    m_viaSizesGrid->GetDefaultRowSize()    + 4 );
    m_diffPairsGrid->SetDefaultRowSize(   m_diffPairsGrid->GetDefaultRowSize()   + 4 );

    m_textNetFilter->SetHint( _( "Net filter" ) );

    // Set up the net name column of the netclass membership grid to read-only
    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetReadOnly( true );
    m_membershipGrid->SetColAttr( 0, attr );

    m_addButton->SetBitmap( KiBitmap( small_plus_xpm ) );
    m_removeButton->SetBitmap( KiBitmap( trash_xpm ) );

    // wxFormBuilder doesn't include this event...
    m_netclassGrid->Connect( wxEVT_GRID_CELL_CHANGING, wxGridEventHandler( PANEL_SETUP_NETCLASSES::OnNetclassGridCellChanging ), NULL, this );
}


PANEL_SETUP_NETCLASSES::~PANEL_SETUP_NETCLASSES()
{
    delete [] m_originalColWidths;

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

    // Reassure that all nets have net classes assigned
    m_Pcb->BuildListOfNets();

    if( m_membershipGrid->GetNumberRows() )
        m_membershipGrid->DeleteRows( 0, m_membershipGrid->GetNumberRows() );

    // Initialize list of nets for Default Net Class
    for( NETCLASS::iterator name = netclass->begin();  name != netclass->end();  ++name )
    {
        if( name->IsEmpty() )
            // @TODO go fix m_Pcb->SynchronizeNetsAndNetClasses() so that the netcode==0
            //       is not present in the BOARD::m_NetClasses
            continue;
        else
            addNet( *name, netclass->GetName() );
    }

    // Initialize list of nets for others (custom) Net Classes
    for( NETCLASSES::const_iterator nc = netclasses.begin();  nc != netclasses.end();  ++nc )
    {
        netclass = nc->second;

        for( NETCLASS::const_iterator name = netclass->begin();  name != netclass->end();  ++name )
            addNet( *name, netclass->GetName() );
    }

    TransferDimensionListsToWindow();

    return true;
}


void PANEL_SETUP_NETCLASSES::addNet( wxString netName, const wxString& netclass )
{
    int i = m_membershipGrid->GetNumberRows();

    m_membershipGrid->AppendRows( 1 );

    m_membershipGrid->SetCellValue( i, 0, netName );
    m_membershipGrid->SetCellValue( i, 1, netclass );
}


void PANEL_SETUP_NETCLASSES::TransferDimensionListsToWindow()
{
#define SETCELL( grid, row, col, val ) \
    grid->SetCellValue( row, col, StringFromValue( m_Frame->GetUserUnits(), val, true, true ) )

    m_trackWidthsGrid->ClearGrid();
    m_viaSizesGrid->ClearGrid();
    m_diffPairsGrid->ClearGrid();

    // Skip the first item, which is the current netclass value
    for( unsigned ii = 1; ii < m_BrdSettings->m_TrackWidthList.size(); ii++ )
    {
        SETCELL( m_trackWidthsGrid, ii-1, 0, m_BrdSettings->m_TrackWidthList[ii] );
    }

    // Skip the first item, which is the current netclass value
    for( unsigned ii = 1; ii < m_BrdSettings->m_ViasDimensionsList.size(); ii++ )
    {
        SETCELL( m_viaSizesGrid, ii-1, 0, m_BrdSettings->m_ViasDimensionsList[ii].m_Diameter );

        if( m_BrdSettings->m_ViasDimensionsList[ii].m_Drill > 0 )
            SETCELL( m_viaSizesGrid, ii-1, 1, m_BrdSettings->m_ViasDimensionsList[ii].m_Drill );
    }

    // Skip the first item, which is the current netclass value
    for( unsigned ii = 1; ii < m_BrdSettings->m_DiffPairDimensionsList.size(); ii++ )
    {
        SETCELL( m_diffPairsGrid, ii-1, 0, m_BrdSettings->m_DiffPairDimensionsList[ii].m_Width );

        if( m_BrdSettings->m_DiffPairDimensionsList[ii].m_Gap > 0 )
            SETCELL( m_diffPairsGrid, ii-1, 1, m_BrdSettings->m_DiffPairDimensionsList[ii].m_Gap );

        if( m_BrdSettings->m_DiffPairDimensionsList[ii].m_ViaGap > 0 )
            SETCELL( m_diffPairsGrid, ii-1, 2, m_BrdSettings->m_DiffPairDimensionsList[ii].m_ViaGap );
    }
}


/* Populates drop-downs with the list of net classes
 */
void PANEL_SETUP_NETCLASSES::rebuildNetclassDropdowns()
{
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


void PANEL_SETUP_NETCLASSES::CopyNetclassesToBoard()
{
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
}


void PANEL_SETUP_NETCLASSES::CopyDimensionsListsToBoard()
{
    wxString                         msg;
    std::vector<int>                 trackWidths;
    std::vector<VIA_DIMENSION>       vias;
    std::vector<DIFF_PAIR_DIMENSION> diffPairs;

    for( int row = 0; row < m_trackWidthsGrid->GetNumberRows();  ++row )
    {
        msg = m_trackWidthsGrid->GetCellValue( row, 0 );

        if( !msg.IsEmpty() )
            trackWidths.push_back( ValueFromString( m_Frame->GetUserUnits(), msg, true ) );
    }

    for( int row = 0; row < m_viaSizesGrid->GetNumberRows();  ++row )
    {
        msg = m_viaSizesGrid->GetCellValue( row, 0 );

        if( !msg.IsEmpty() )
        {
            VIA_DIMENSION via_dim;
            via_dim.m_Diameter = ValueFromString( m_Frame->GetUserUnits(), msg, true );

            msg = m_viaSizesGrid->GetCellValue( row, 1 );

            if( !msg.IsEmpty() )
                via_dim.m_Drill = ValueFromString( m_Frame->GetUserUnits(), msg, true );

            vias.push_back( via_dim );
        }
    }

    for( int row = 0; row < m_viaSizesGrid->GetNumberRows();  ++row )
    {
        msg = m_diffPairsGrid->GetCellValue( row, 0 );

        if( !msg.IsEmpty() )
        {
            DIFF_PAIR_DIMENSION diffPair_dim;
            diffPair_dim.m_Width = ValueFromString( m_Frame->GetUserUnits(), msg, true );

            msg = m_diffPairsGrid->GetCellValue( row, 1 );
            diffPair_dim.m_Gap = ValueFromString( m_Frame->GetUserUnits(), msg, true );

            msg = m_diffPairsGrid->GetCellValue( row, 2 );

            if( !msg.IsEmpty() )
                diffPair_dim.m_ViaGap = ValueFromString( m_Frame->GetUserUnits(), msg, true );

            diffPairs.push_back( diffPair_dim );
        }
    }

    // Sort lists by increasing value
    sort( trackWidths.begin(), trackWidths.end() );
    sort( vias.begin(), vias.end() );
    sort( diffPairs.begin(), diffPairs.end() );

    trackWidths.insert( trackWidths.begin(), m_BrdSettings->m_TrackWidthList[ 0 ] );
    m_BrdSettings->m_TrackWidthList = trackWidths;

    vias.insert( vias.begin(), m_BrdSettings->m_ViasDimensionsList[ 0 ] );
    m_BrdSettings->m_ViasDimensionsList = vias;

    diffPairs.insert( diffPairs.begin(), m_BrdSettings->m_DiffPairDimensionsList[ 0 ] );
    m_BrdSettings->m_DiffPairDimensionsList = diffPairs;
}


bool PANEL_SETUP_NETCLASSES::TransferDataFromWindow()
{
    if( !validateData() )
        return false;

    CopyNetclassesToBoard();
    CopyDimensionsListsToBoard();
    m_BrdSettings->SetCurrentNetClass( NETCLASS::Default );

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
    m_netclassGrid->DisableCellEditControl();

    int curRow = m_netclassGrid->GetGridCursorRow();

    if( !m_netclassGrid->HasFocus() || curRow < 0 )
    {
        m_netclassGrid->SetFocus();
        return;
    }
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

    curRow = std::max( 0, curRow - 1 );
    m_netclassGrid->MakeCellVisible( curRow, m_netclassGrid->GetGridCursorCol() );
    m_netclassGrid->SetGridCursor( curRow, m_netclassGrid->GetGridCursorCol() );

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


void PANEL_SETUP_NETCLASSES::OnFilterChanged( wxCommandEvent& event )
{
    wxString filter = m_textNetFilter->GetValue().MakeLower();

    if( filter.IsEmpty() )
        filter = wxT( "*" );
    else
        filter = wxT( "*" ) + filter + wxT( "*" );

    for( int row = 0; row < m_membershipGrid->GetNumberRows(); ++row )
    {
        if( m_membershipGrid->GetCellValue( row, 0 ).MakeLower().Matches( filter )
                || m_membershipGrid->GetCellValue( row, 1 ).MakeLower().Matches( filter ) )
            m_membershipGrid->ShowRow( row );
        else
            m_membershipGrid->HideRow( row );
    }
}


void PANEL_SETUP_NETCLASSES::OnUpdateUI( wxUpdateUIEvent& event )
{
    if( m_netclassesDirty )
    {
        rebuildNetclassDropdowns();
        m_netclassesDirty = false;
    }
}


int PANEL_SETUP_NETCLASSES::getNetclassValue( int aRow, int aCol )
{
    return ValueFromString( m_Frame->GetUserUnits(), m_netclassGrid->GetCellValue( aRow, aCol ), true );
}


bool PANEL_SETUP_NETCLASSES::validateData()
{
    // Commit any pending in-place edits and close editors from grid controls
    m_netclassGrid->DisableCellEditControl();
    m_membershipGrid->DisableCellEditControl();
    m_trackWidthsGrid->DisableCellEditControl();
    m_viaSizesGrid->DisableCellEditControl();
    m_diffPairsGrid->DisableCellEditControl();

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

    // Test custom tracks
    for( int row = 0; row < m_trackWidthsGrid->GetNumberRows();  ++row )
    {
        wxString tvalue = m_trackWidthsGrid->GetCellValue( row, 0 );

        if( tvalue.IsEmpty() )
            continue;

        if( ValueFromString( m_Frame->GetUserUnits(), tvalue ) < minTrackWidth )
        {
            msg.Printf( _( "Track width less than minimum track width (%s)." ),
                        StringFromValue( m_Frame->GetUserUnits(), minTrackWidth, true, true ) );
            m_Parent->SetError( msg, this, m_trackWidthsGrid, row, 0 );
            return false;
        }
    }

    // Test custom vias
    for( int row = 0; row < m_viaSizesGrid->GetNumberRows();  ++row )
    {
        wxString viaDia = m_viaSizesGrid->GetCellValue( row, 0 );

        if( viaDia.IsEmpty() )
            continue;

        if( ValueFromString( m_Frame->GetUserUnits(), viaDia ) < minViaDia )
        {
            msg.Printf( _( "Via diameter less than minimum via diameter (%s)." ),
                        StringFromValue( m_Frame->GetUserUnits(), minViaDia, true, true ) );
            m_Parent->SetError( msg, this, m_viaSizesGrid, row, 0 );
            return false;
        }

        wxString viaDrill = m_viaSizesGrid->GetCellValue( row, 1 );

        if( viaDrill.IsEmpty() )
        {
            msg = _( "No via drill defined." );
            m_Parent->SetError( msg, this, m_viaSizesGrid, row, 1 );
            return false;
        }

        if( ValueFromString( m_Frame->GetUserUnits(), viaDrill ) < minViaDrill )
        {
            msg.Printf( _( "Via drill less than minimum via drill (%s)." ),
                        StringFromValue( m_Frame->GetUserUnits(), minViaDrill, true, true ) );
            m_Parent->SetError( msg, this, m_viaSizesGrid, row, 1 );
            return false;
        }

        if( ValueFromString( m_Frame->GetUserUnits(), viaDrill )
                >= ValueFromString( m_Frame->GetUserUnits(), viaDia ) )
        {
            msg = _( "Via drill larger than via diameter." );
            m_Parent->SetError( msg, this, m_viaSizesGrid, row, 1 );
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



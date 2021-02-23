/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2009 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2009-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <algorithm>

#include <pgm_base.h>
#include <base_units.h>
#include <bitmaps.h>
#include <netclass.h>
#include <confirm.h>
#include <grid_tricks.h>
#include <dialogs/panel_setup_netclasses.h>
#include <tool/tool_manager.h>
#include <widgets/wx_grid.h>
#include <kicad_string.h>
#include <widgets/grid_color_swatch_helpers.h>
#include <widgets/grid_icon_text_helpers.h>
#include <widgets/grid_text_helpers.h>

// PCBNEW columns of netclasses grid
enum {
    GRID_NAME = 0,

    GRID_FIRST_PCBNEW,
    GRID_CLEARANCE = GRID_FIRST_PCBNEW,
    GRID_TRACKSIZE,
    GRID_VIASIZE,
    GRID_VIADRILL,
    GRID_uVIASIZE,
    GRID_uVIADRILL,
    GRID_DIFF_PAIR_WIDTH,
    GRID_DIFF_PAIR_GAP,

    GRID_FIRST_EESCHEMA,
    GRID_WIREWIDTH = GRID_FIRST_EESCHEMA,
    GRID_BUSWIDTH,
    GRID_SCHEMATIC_COLOR,
    GRID_LINESTYLE,

    GRID_END
};


// These are conceptually constexpr
std::vector<BITMAP_DEF> g_lineStyleIcons;
wxArrayString           g_lineStyleNames;


PANEL_SETUP_NETCLASSES::PANEL_SETUP_NETCLASSES( PAGED_DIALOG* aParent, NETCLASSES* aNetclasses,
                                                const std::vector<wxString>& aNetNames,
                                                bool aIsEEschema ) :
        PANEL_SETUP_NETCLASSES_BASE( aParent->GetTreebook() ),
        m_Parent( aParent ),
        m_netclasses( aNetclasses ),
        m_netNames( aNetNames ),
        m_hoveredCol( -1 )
{
    if( g_lineStyleIcons.empty() )
    {
        g_lineStyleIcons.push_back( stroke_solid_xpm );
        g_lineStyleNames.push_back( _( "Solid" ) );
        g_lineStyleIcons.push_back( stroke_dash_xpm );
        g_lineStyleNames.push_back( _( "Dashed" ) );
        g_lineStyleIcons.push_back( stroke_dot_xpm );
        g_lineStyleNames.push_back( _( "Dotted" ) );
        g_lineStyleIcons.push_back( stroke_dashdot_xpm );
        g_lineStyleNames.push_back( _( "Dash-Dot" ) );
    }

    m_netclassesDirty = true;

    // Prevent Size events from firing before we are ready
    Freeze();
    m_netclassGrid->BeginBatch();
    m_membershipGrid->BeginBatch();

    m_originalColWidths = new int[ m_netclassGrid->GetNumberCols() ];
    // Calculate a min best size to handle longest usual numeric values:
    int min_best_width = m_netclassGrid->GetTextExtent( "555,555555 mils" ).x;

    for( int i = 0; i < m_netclassGrid->GetNumberCols(); ++i )
    {
        // We calculate the column min size only from texts sizes, not using the initial col width
        // as this initial width is sometimes strange depending on the language (wxGrid bug?)
        int min_width =  m_netclassGrid->GetVisibleWidth( i, true, true, false );

        if( i == GRID_LINESTYLE )
            min_best_width *= 1.5;

        m_netclassGrid->SetColMinimalWidth( i, min_width );

        // We use a "best size" >= min_best_width
        m_originalColWidths[ i ] = std::max( min_width, min_best_width );
        m_netclassGrid->SetColSize( i, m_originalColWidths[ i ] );
    }

    if( aIsEEschema )
    {
        for( int i = GRID_FIRST_PCBNEW; i < GRID_FIRST_EESCHEMA; ++i )
        {
            m_netclassGrid->HideCol( i );
            m_originalColWidths[ i ] = 0;
        }

        wxGridCellAttr* attr = new wxGridCellAttr;
        attr->SetRenderer( new GRID_CELL_COLOR_RENDERER( aParent ) );
        attr->SetEditor( new GRID_CELL_COLOR_SELECTOR( aParent, m_netclassGrid ) );
        m_netclassGrid->SetColAttr( GRID_SCHEMATIC_COLOR, attr );

        attr = new wxGridCellAttr;
        attr->SetRenderer( new GRID_CELL_ICON_TEXT_RENDERER( g_lineStyleIcons, g_lineStyleNames ) );
        attr->SetEditor( new GRID_CELL_ICON_TEXT_POPUP( g_lineStyleIcons, g_lineStyleNames ) );
        m_netclassGrid->SetColAttr( GRID_LINESTYLE, attr );
    }
    else
    {
        for( int i = GRID_FIRST_EESCHEMA; i < GRID_END; ++i )
        {
            m_netclassGrid->HideCol( i );
            m_originalColWidths[ i ] = 0;
        }
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
    attr->SetRenderer( new GRID_CELL_ESCAPED_TEXT_RENDERER );
    m_membershipGrid->SetColAttr( 0, attr );

    COMMON_SETTINGS* cfg = Pgm().GetCommonSettings();
    m_splitter->SetSashPosition( cfg->m_NetclassPanel.sash_pos );

    m_addButton->SetBitmap( KiBitmap( small_plus_xpm ) );
    m_removeButton->SetBitmap( KiBitmap( small_trash_xpm ) );

    // wxFormBuilder doesn't include this event...
    m_netclassGrid->Connect( wxEVT_GRID_CELL_CHANGING,
                             wxGridEventHandler( PANEL_SETUP_NETCLASSES::OnNetclassGridCellChanging ),
                             NULL, this );

    // Handle tooltips for grid
    m_netclassGrid->GetGridColLabelWindow()->Bind( wxEVT_MOTION,
                                                   &PANEL_SETUP_NETCLASSES::OnNetclassGridMouseEvent,
                                                   this );

    m_netclassGrid->EndBatch();
    m_membershipGrid->EndBatch();
    Thaw();
}


PANEL_SETUP_NETCLASSES::~PANEL_SETUP_NETCLASSES()
{
    COMMON_SETTINGS* cfg = Pgm().GetCommonSettings();
    cfg->m_NetclassPanel.sash_pos = m_splitter->GetSashPosition();

    delete [] m_originalColWidths;

    // Delete the GRID_TRICKS.
    m_netclassGrid->PopEventHandler( true );
    m_membershipGrid->PopEventHandler( true );

    m_netclassGrid->Disconnect( wxEVT_GRID_CELL_CHANGING,
                                wxGridEventHandler( PANEL_SETUP_NETCLASSES::OnNetclassGridCellChanging ),
                                NULL, this );
}


static void netclassToGridRow( EDA_UNITS aUnits, wxGrid* aGrid, int aRow, const NETCLASSPTR& nc )
{
    aGrid->SetCellValue( aRow, GRID_NAME, nc->GetName() );

#define SET_MILS_CELL( col, val ) \
    aGrid->SetCellValue( aRow, col, StringFromValue( aUnits, val, true ) )

    SET_MILS_CELL( GRID_CLEARANCE, nc->GetClearance() );
    SET_MILS_CELL( GRID_TRACKSIZE, nc->GetTrackWidth() );
    SET_MILS_CELL( GRID_VIASIZE, nc->GetViaDiameter() );
    SET_MILS_CELL( GRID_VIADRILL, nc->GetViaDrill() );
    SET_MILS_CELL( GRID_uVIASIZE, nc->GetuViaDiameter() );
    SET_MILS_CELL( GRID_uVIADRILL, nc->GetuViaDrill() );
    SET_MILS_CELL( GRID_DIFF_PAIR_WIDTH, nc->GetDiffPairWidth() );
    SET_MILS_CELL( GRID_DIFF_PAIR_GAP, nc->GetDiffPairGap() );

    SET_MILS_CELL( GRID_WIREWIDTH, nc->GetWireWidth() );
    SET_MILS_CELL( GRID_BUSWIDTH, nc->GetBusWidth() );

    wxString colorAsString = nc->GetSchematicColor().ToWxString( wxC2S_CSS_SYNTAX );
    aGrid->SetCellValue( aRow, GRID_SCHEMATIC_COLOR, colorAsString );
    aGrid->SetCellValue( aRow, GRID_LINESTYLE, g_lineStyleNames[ nc->GetLineStyle() ] );
}


bool PANEL_SETUP_NETCLASSES::TransferDataToWindow()
{
    std::map<wxString, wxString> netToNetclassMap;
    std::map<wxString, wxString> staleNetMap;

    for( const wxString& candidate : m_netNames )
        netToNetclassMap[ candidate ] = wxEmptyString;

    if( m_netclassGrid->GetNumberRows() )
        m_netclassGrid->DeleteRows( 0, m_netclassGrid->GetNumberRows() );

    m_netclassGrid->AppendRows((int) m_netclasses->GetCount() + 1 ); // + 1 for default netclass

    // enter the Default NETCLASS.
    netclassToGridRow( m_Parent->GetUserUnits(), m_netclassGrid, 0, m_netclasses->GetDefault() );

    // make the Default NETCLASS name read-only
    wxGridCellAttr* cellAttr = m_netclassGrid->GetOrCreateCellAttr( 0, GRID_NAME );
    cellAttr->SetReadOnly();
    cellAttr->DecRef();

    // enter other netclasses
    int row = 1;

    for( NETCLASSES::iterator i = m_netclasses->begin(); i != m_netclasses->end(); ++i, ++row )
    {
        NETCLASSPTR netclass = i->second;

        netclassToGridRow( m_Parent->GetUserUnits(), m_netclassGrid, row, netclass );

        for( const wxString& net : *netclass )
        {
            if( netToNetclassMap.count( net ) )
                netToNetclassMap[ net ] = i->second->GetName();
            else
                staleNetMap[ net ] = i->second->GetName();
        }
    }

    if( m_membershipGrid->GetNumberRows() )
        m_membershipGrid->DeleteRows( 0, m_membershipGrid->GetNumberRows() );

    // add currently-assigned and candidate netnames to membership lists
    for( const std::pair<const wxString, wxString>& ii : netToNetclassMap )
        addNet( ii.first, ii.second, false );

    for( const std::pair<const wxString, wxString>& ii : staleNetMap )
        addNet( ii.first, ii.second, true );

    return true;
}


void PANEL_SETUP_NETCLASSES::addNet( const wxString& netName, const wxString& netclass,
                                     bool aStale )
{
    int i = m_membershipGrid->GetNumberRows();

    m_membershipGrid->AppendRows( 1 );

    m_membershipGrid->SetCellValue( i, 0, netName );

    if( aStale )
    {
        wxColour color = wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT );
        m_membershipGrid->SetCellTextColour( i, 0, color );
    }

    if( netclass.IsEmpty() )
        m_membershipGrid->SetCellValue( i, 1, NETCLASS::Default );
    else
        m_membershipGrid->SetCellValue( i, 1, netclass );
}


/*
 * Populates drop-downs with the list of net classes
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


static void gridRowToNetclass( EDA_UNITS aUnits, wxGrid* grid, int row, const NETCLASSPTR& nc )
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
    nc->SetDiffPairWidth( MYCELL( GRID_DIFF_PAIR_WIDTH ) );
    nc->SetDiffPairGap( MYCELL( GRID_DIFF_PAIR_GAP ) );

    nc->SetWireWidth( MYCELL( GRID_WIREWIDTH ) );
    nc->SetBusWidth( MYCELL( GRID_BUSWIDTH ) );

    nc->SetSchematicColor( wxColour( grid->GetCellValue( row, GRID_SCHEMATIC_COLOR ) ) );
    nc->SetLineStyle( g_lineStyleNames.Index( grid->GetCellValue( row, GRID_LINESTYLE ) ) );
}


bool PANEL_SETUP_NETCLASSES::TransferDataFromWindow()
{
    if( !Validate() )
        return false;

    m_netclasses->Clear();

    // Copy the default NetClass:
    gridRowToNetclass( m_Parent->GetUserUnits(), m_netclassGrid, 0, m_netclasses->GetDefault() );

    // Copy other NetClasses:
    for( int row = 1; row < m_netclassGrid->GetNumberRows();  ++row )
    {
        NETCLASSPTR nc = std::make_shared<NETCLASS>( m_netclassGrid->GetCellValue( row, GRID_NAME ) );

        if( m_netclasses->Add( nc ) )
            gridRowToNetclass( m_Parent->GetUserUnits(), m_netclassGrid, row, nc );
    }

    // Now read all nets and push them in the corresponding netclass net buffer
    for( int row = 0; row < m_membershipGrid->GetNumberRows(); ++row )
    {
        const wxString& netname = m_membershipGrid->GetCellValue( row, 0 );
        const wxString& classname = m_membershipGrid->GetCellValue( row, 1 );

        if( classname != NETCLASS::Default )
        {
            const NETCLASSPTR& nc = m_netclasses->Find( classname );

            if( nc )
                nc->Add( netname );
        }
    }

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
        if( ii != aRow && m_netclassGrid->GetCellValue( ii, GRID_NAME ).CmpNoCase( aName ) == 0 )
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
        {
            wxString oldName = m_netclassGrid->GetCellValue( event.GetRow(), GRID_NAME );
            wxString newName = event.GetString();

            if( !oldName.IsEmpty() )
            {
                for( int row = 0; row < m_membershipGrid->GetNumberRows(); ++row )
                {
                    if( m_membershipGrid->GetCellValue( row, 1 ) == oldName )
                        m_membershipGrid->SetCellValue( row, 1, newName );
                }
            }

            m_netclassesDirty = true;
        }
        else
        {
            event.Veto();
        }
    }
}


void PANEL_SETUP_NETCLASSES::OnNetclassGridMouseEvent( wxMouseEvent& aEvent )
{
    int col = m_netclassGrid->XToCol( aEvent.GetPosition().x );

    if( aEvent.Moving() || aEvent.Entering() )
    {
        aEvent.Skip();

        if( col == wxNOT_FOUND )
        {
            m_netclassGrid->GetGridColLabelWindow()->UnsetToolTip();
            return;
        }

        if( col == m_hoveredCol )
            return;

        m_hoveredCol = col;

        wxString tip;

        switch( col )
        {
        case GRID_CLEARANCE:        tip = _( "Minimum copper clearance" );      break;
        case GRID_TRACKSIZE:        tip = _( "Minimum track width" );           break;
        case GRID_VIASIZE:          tip = _( "Via pad diameter" );              break;
        case GRID_VIADRILL:         tip = _( "Via plated hole diameter" );      break;
        case GRID_uVIASIZE:         tip = _( "Microvia pad diameter" );         break;
        case GRID_uVIADRILL:        tip = _( "Microvia plated hole diameter" ); break;
        case GRID_DIFF_PAIR_WIDTH:  tip = _( "Differential pair track width" ); break;
        case GRID_DIFF_PAIR_GAP:    tip = _( "Differential pair gap" );         break;
        case GRID_WIREWIDTH:        tip = _( "Schematic wire thickness" );      break;
        case GRID_BUSWIDTH:         tip = _( "Bus wire thickness" );            break;
        case GRID_SCHEMATIC_COLOR:  tip = _( "Schematic wire color" );          break;
        case GRID_LINESTYLE:        tip = _( "Schematic wire line style" );     break;
        }

        m_netclassGrid->GetGridColLabelWindow()->UnsetToolTip();
        m_netclassGrid->GetGridColLabelWindow()->SetToolTip( tip );
    }
    else if( aEvent.Leaving() )
    {
        m_netclassGrid->GetGridColLabelWindow()->UnsetToolTip();
        aEvent.Skip();
    }

    aEvent.Skip();
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
    {
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

    m_netclassGrid->SetColSize( 0, std::max( aWidth - 2, m_originalColWidths[ 0 ] ) );
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


void PANEL_SETUP_NETCLASSES::onmembershipPanelSize( wxSizeEvent& event )
{
    // When a class name choice widget is selected (activated), in
    // wxGrid m_membershipGrid, resizing its wxGrid parent is not taken in account
    // by the widget until it is deselected and stay in the old position.
    // So we deselect it if this is the case
    // Note also this is made here, not in OnSizeMembershipGrid because on Linux
    // there are a lot of wxSizeEvent send to m_membershipGrid when opening a choice widget
    int c_row = m_membershipGrid->GetGridCursorRow();
    int c_col = m_membershipGrid->GetGridCursorCol();

    if( c_row >= 0 && c_col == 1 )    // this means the class name choice widget is selected (opened)
        m_membershipGrid->SetGridCursor( c_row, 0 );    // Close it

    event.Skip();
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
}


bool PANEL_SETUP_NETCLASSES::Validate()
{
    if( !m_netclassGrid->CommitPendingChanges() || !m_membershipGrid->CommitPendingChanges() )
        return false;

    wxString msg;

    // Test net class parameters.
    for( int row = 0; row < m_netclassGrid->GetNumberRows(); row++ )
    {
        wxString netclassName = m_netclassGrid->GetCellValue( row, GRID_NAME );
        netclassName.Trim( true );
        netclassName.Trim( false );

        if( !validateNetclassName( row, netclassName, false ) )
            return false;
    }

    return true;
}


void PANEL_SETUP_NETCLASSES::ImportSettingsFrom( NETCLASSES* aNetclasses )
{
    NETCLASSES* savedSettings = m_netclasses;

    m_netclasses = aNetclasses;
    TransferDataToWindow();

    rebuildNetclassDropdowns();

    m_netclassGrid->ForceRefresh();
    m_membershipGrid->ForceRefresh();

    m_netclasses = savedSettings;
}



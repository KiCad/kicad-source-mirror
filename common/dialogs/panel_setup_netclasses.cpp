/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2009 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2009-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <eda_draw_frame.h>
#include <bitmaps.h>
#include <netclass.h>
#include <confirm.h>
#include <grid_tricks.h>
#include <dialogs/panel_setup_netclasses.h>
#include <dialogs/wx_html_report_box.h>
#include <tool/tool_manager.h>
#include <widgets/wx_grid.h>
#include <string_utils.h>
#include <widgets/grid_color_swatch_helpers.h>
#include <widgets/grid_icon_text_helpers.h>
#include <wx/treebook.h>
#include <project/net_settings.h>


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

constexpr int EESCHEMA_COL_OFFSET = GRID_FIRST_EESCHEMA - GRID_FIRST_PCBNEW;


std::vector<BITMAPS> g_lineStyleIcons;
wxArrayString        g_lineStyleNames;


PANEL_SETUP_NETCLASSES::PANEL_SETUP_NETCLASSES( PAGED_DIALOG* aParent, EDA_DRAW_FRAME* aFrame,
                                                std::shared_ptr<NET_SETTINGS> aNetSettings,
                                                const std::vector<wxString>& aNetNames,
                                                bool aIsEEschema ) :
        PANEL_SETUP_NETCLASSES_BASE( aParent->GetTreebook() ),
        m_frame( aFrame ),
        m_parent( aParent ),
        m_isEEschema( aIsEEschema ),
        m_netSettings( aNetSettings ),
        m_netNames( aNetNames ),
        m_hoveredCol( -1 ),
        m_lastNetclassGridWidth( -1 )
{
    // Clear and re-load each time.  Language (or darkmode) might have changed.
    g_lineStyleIcons.clear();
    g_lineStyleNames.clear();

    g_lineStyleIcons.push_back( BITMAPS::stroke_solid );
    g_lineStyleNames.push_back( _( "Solid" ) );
    g_lineStyleIcons.push_back( BITMAPS::stroke_dash );
    g_lineStyleNames.push_back( _( "Dashed" ) );
    g_lineStyleIcons.push_back( BITMAPS::stroke_dot );
    g_lineStyleNames.push_back( _( "Dotted" ) );
    g_lineStyleIcons.push_back( BITMAPS::stroke_dashdot );
    g_lineStyleNames.push_back( _( "Dash-Dot" ) );
    g_lineStyleIcons.push_back( BITMAPS::stroke_dashdotdot );
    g_lineStyleNames.push_back( _( "Dash-Dot-Dot" ) );

    m_netclassesDirty = true;

    // Prevent Size events from firing before we are ready
    Freeze();
    m_netclassGrid->BeginBatch();
    m_assignmentGrid->BeginBatch();

    if( m_isEEschema )
    {
        m_netclassGrid->DeleteCols( GRID_FIRST_PCBNEW, GRID_FIRST_EESCHEMA - GRID_FIRST_PCBNEW );

        wxGridCellAttr* attr = new wxGridCellAttr;
        attr->SetRenderer( new GRID_CELL_COLOR_RENDERER( aParent ) );
        attr->SetEditor( new GRID_CELL_COLOR_SELECTOR( aParent, m_netclassGrid ) );
        m_netclassGrid->SetColAttr( GRID_SCHEMATIC_COLOR - EESCHEMA_COL_OFFSET, attr );

        attr = new wxGridCellAttr;
        attr->SetRenderer( new GRID_CELL_ICON_TEXT_RENDERER( g_lineStyleIcons, g_lineStyleNames ) );
        attr->SetEditor( new GRID_CELL_ICON_TEXT_POPUP( g_lineStyleIcons, g_lineStyleNames ) );
        m_netclassGrid->SetColAttr( GRID_LINESTYLE - EESCHEMA_COL_OFFSET, attr );

        m_colorDefaultHelpText->SetFont( KIUI::GetInfoFont( this ).Italic() );
    }
    else
    {
        m_netclassGrid->DeleteCols( GRID_FIRST_EESCHEMA, GRID_END - GRID_FIRST_EESCHEMA );

        m_colorDefaultHelpText->Hide();
    }

    m_originalColWidths = new int[ m_netclassGrid->GetNumberCols() ];
    // Calculate a min best size to handle longest usual numeric values:
    int min_best_width = m_netclassGrid->GetTextExtent( "555,555555 mils" ).x;

    for( int i = 0; i < m_netclassGrid->GetNumberCols(); ++i )
    {
        // We calculate the column min size only from texts sizes, not using the initial col width
        // as this initial width is sometimes strange depending on the language (wxGrid bug?)
        int min_width =  m_netclassGrid->GetVisibleWidth( i, true, true );

        if( i == GRID_LINESTYLE )
            min_best_width *= 1.5;

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
    m_assignmentGrid->SetDefaultRowSize( m_assignmentGrid->GetDefaultRowSize() + 4 );

    m_netclassGrid->PushEventHandler( new GRID_TRICKS( m_netclassGrid ) );
    m_assignmentGrid->PushEventHandler( new GRID_TRICKS( m_assignmentGrid ) );

    m_netclassGrid->SetSelectionMode( wxGrid::wxGridSelectRows );
    m_assignmentGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    COMMON_SETTINGS* cfg = Pgm().GetCommonSettings();
    m_splitter->SetSashPosition( cfg->m_NetclassPanel.sash_pos );

    m_addButton->SetBitmap( KiBitmap( BITMAPS::small_plus ) );
    m_removeButton->SetBitmap( KiBitmap( BITMAPS::small_trash ) );

    m_addAssignmentButton->SetBitmap( KiBitmap( BITMAPS::small_plus ) );
    m_removeAssignmentButton->SetBitmap( KiBitmap( BITMAPS::small_trash ) );

    // wxFormBuilder doesn't include this event...
    m_netclassGrid->Connect( wxEVT_GRID_CELL_CHANGING,
                             wxGridEventHandler( PANEL_SETUP_NETCLASSES::OnNetclassGridCellChanging ),
                             nullptr, this );

    // Handle tooltips for grid
    m_netclassGrid->GetGridColLabelWindow()->Bind( wxEVT_MOTION,
                                                   &PANEL_SETUP_NETCLASSES::OnNetclassGridMouseEvent,
                                                   this );

    m_frame->Bind( UNITS_CHANGED, &PANEL_SETUP_NETCLASSES::onUnitsChanged, this );

    m_netclassGrid->EndBatch();
    m_assignmentGrid->EndBatch();
    Thaw();

    m_matchingNets->SetFont( KIUI::GetInfoFont( this ) );
}


PANEL_SETUP_NETCLASSES::~PANEL_SETUP_NETCLASSES()
{
    COMMON_SETTINGS* cfg = Pgm().GetCommonSettings();
    cfg->m_NetclassPanel.sash_pos = m_splitter->GetSashPosition();

    delete [] m_originalColWidths;

    // Delete the GRID_TRICKS.
    m_netclassGrid->PopEventHandler( true );
    m_assignmentGrid->PopEventHandler( true );

    m_netclassGrid->Disconnect( wxEVT_GRID_CELL_CHANGING,
                                wxGridEventHandler( PANEL_SETUP_NETCLASSES::OnNetclassGridCellChanging ),
                                nullptr, this );

    m_frame->Unbind( UNITS_CHANGED, &PANEL_SETUP_NETCLASSES::onUnitsChanged, this );
}


void PANEL_SETUP_NETCLASSES::onUnitsChanged( wxCommandEvent& aEvent )
{
    std::shared_ptr<NET_SETTINGS> tempNetSettings = std::make_shared<NET_SETTINGS>( nullptr, "" );
    std::shared_ptr<NET_SETTINGS> saveNetSettings = m_netSettings;

    m_netSettings = tempNetSettings;

    TransferDataFromWindow();
    TransferDataToWindow();

    m_netSettings = saveNetSettings;

    aEvent.Skip();
}


bool PANEL_SETUP_NETCLASSES::TransferDataToWindow()
{
    EDA_UNITS units = m_frame->GetUserUnits();
    int       row = 0;

    auto setCell =
            [&]( int aRow, int aCol, int aValue )
            {
                m_netclassGrid->SetCellValue( aRow, aCol, StringFromValue( units, aValue, true ) );
            };

    auto netclassToGridRow =
            [&]( int aRow, const std::shared_ptr<NETCLASS>& nc )
            {
                m_netclassGrid->SetCellValue( aRow, GRID_NAME, nc->GetName() );

                if( m_isEEschema )
                {
                    setCell( aRow, GRID_WIREWIDTH - EESCHEMA_COL_OFFSET, nc->GetWireWidth() );
                    setCell( aRow, GRID_BUSWIDTH - EESCHEMA_COL_OFFSET, nc->GetBusWidth() );

                    wxString colorAsString = nc->GetSchematicColor().ToWxString( wxC2S_CSS_SYNTAX );
                    m_netclassGrid->SetCellValue( aRow, GRID_SCHEMATIC_COLOR - EESCHEMA_COL_OFFSET,
                                                  colorAsString );

                    int lineStyleIdx = std::max( 0, nc->GetLineStyle() );

                    if( lineStyleIdx >= (int) g_lineStyleNames.size() )
                        lineStyleIdx = 0;

                    m_netclassGrid->SetCellValue( aRow, GRID_LINESTYLE - EESCHEMA_COL_OFFSET,
                                                  g_lineStyleNames[ lineStyleIdx ] );
                }
                else
                {
                    setCell( aRow, GRID_CLEARANCE, nc->GetClearance() );
                    setCell( aRow, GRID_TRACKSIZE, nc->GetTrackWidth() );
                    setCell( aRow, GRID_VIASIZE, nc->GetViaDiameter() );
                    setCell( aRow, GRID_VIADRILL, nc->GetViaDrill() );
                    setCell( aRow, GRID_uVIASIZE, nc->GetuViaDiameter() );
                    setCell( aRow, GRID_uVIADRILL, nc->GetuViaDrill() );
                    setCell( aRow, GRID_DIFF_PAIR_WIDTH, nc->GetDiffPairWidth() );
                    setCell( aRow, GRID_DIFF_PAIR_GAP, nc->GetDiffPairGap() );
                }
            };

    m_netclassGrid->ClearRows();

    // enter the Default NETCLASS.
    m_netclassGrid->AppendRows( 1 );
    netclassToGridRow( row++, m_netSettings->m_DefaultNetClass );

    // make the Default NETCLASS name read-only
    wxGridCellAttr* cellAttr = m_netclassGrid->GetOrCreateCellAttr( 0, GRID_NAME );
    cellAttr->SetReadOnly();
    cellAttr->DecRef();

    // enter other netclasses
    m_netclassGrid->AppendRows( (int) m_netSettings->m_NetClasses.size() );

    for( const auto& [ name, netclass ] : m_netSettings->m_NetClasses )
        netclassToGridRow( row++, netclass );

    m_assignmentGrid->ClearRows();
    m_assignmentGrid->AppendRows( m_netSettings->m_NetClassPatternAssignments.size() );
    row = 0;

    for( const auto& [ matcher, netclassName ] :  m_netSettings->m_NetClassPatternAssignments )
    {
        m_assignmentGrid->SetCellValue( row, 0, matcher->GetPattern() );
        m_assignmentGrid->SetCellValue( row, 1, netclassName );
        row++;
    }

    return true;
}


/*
 * Populates drop-downs with the list of net classes
 */
void PANEL_SETUP_NETCLASSES::rebuildNetclassDropdowns()
{
    m_assignmentGrid->CommitPendingChanges( true );

    wxArrayString netclassNames;

    for( int ii = 0; ii < m_netclassGrid->GetNumberRows(); ii++ )
    {
        wxString netclassName = m_netclassGrid->GetCellValue( ii, GRID_NAME );

        if( !netclassName.IsEmpty() )
            netclassNames.push_back( netclassName );
    }

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetEditor( new wxGridCellChoiceEditor( netclassNames ) );
    m_assignmentGrid->SetColAttr( 1, attr );
}


bool PANEL_SETUP_NETCLASSES::TransferDataFromWindow()
{
    if( !Validate() )
        return false;

    EDA_UNITS units = m_frame->GetUserUnits();
    int       row = 0;

    auto getCell =
            [this, units]( int aRow, int aCol ) -> long long int
            {
                return ValueFromString( units, m_netclassGrid->GetCellValue( aRow, aCol ) );
            };

    auto getCellStr =
            [this]( int aRow, int aCol ) -> wxString
            {
                return m_netclassGrid->GetCellValue( aRow, aCol );
            };

    auto gridRowToNetclass =
            [&]( int aRow, const std::shared_ptr<NETCLASS>& nc )
            {
                nc->SetName( m_netclassGrid->GetCellValue( aRow, GRID_NAME ) );

                if( m_isEEschema )
                {
                    nc->SetWireWidth( getCell( aRow, GRID_WIREWIDTH - EESCHEMA_COL_OFFSET ) );
                    nc->SetBusWidth( getCell( aRow, GRID_BUSWIDTH - EESCHEMA_COL_OFFSET ) );

                    wxString color = getCellStr( aRow, GRID_SCHEMATIC_COLOR - EESCHEMA_COL_OFFSET );
                    nc->SetSchematicColor( wxColour( color ) );

                    wxString lineStyle = getCellStr( aRow, GRID_LINESTYLE - EESCHEMA_COL_OFFSET );
                    nc->SetLineStyle( g_lineStyleNames.Index( lineStyle ) );
                    wxASSERT_MSG( nc->GetLineStyle() >= 0, "Line style name not found." );
                }
                else
                {
                    nc->SetClearance( getCell( aRow, GRID_CLEARANCE ) );
                    nc->SetTrackWidth( getCell( aRow, GRID_TRACKSIZE ) );
                    nc->SetViaDiameter( getCell( aRow, GRID_VIASIZE ) );
                    nc->SetViaDrill( getCell( aRow, GRID_VIADRILL ) );
                    nc->SetuViaDiameter( getCell( aRow, GRID_uVIASIZE ) );
                    nc->SetuViaDrill( getCell( aRow, GRID_uVIADRILL ) );
                    nc->SetDiffPairWidth( getCell( aRow, GRID_DIFF_PAIR_WIDTH ) );
                    nc->SetDiffPairGap( getCell( aRow, GRID_DIFF_PAIR_GAP ) );
                }
            };

    m_netSettings->m_NetClasses.clear();

    // Copy the default NetClass:
    gridRowToNetclass( row++, m_netSettings->m_DefaultNetClass );

    // Copy other NetClasses:
    for( row = 1; row < m_netclassGrid->GetNumberRows(); ++row )
    {
        auto nc = std::make_shared<NETCLASS>( m_netclassGrid->GetCellValue( row, GRID_NAME ) );
        gridRowToNetclass( row, nc );
        m_netSettings->m_NetClasses[ nc->GetName() ] = nc;
    }

    m_netSettings->m_NetClassPatternAssignments.clear();

    for( row = 0; row < m_assignmentGrid->GetNumberRows(); ++row )
    {
        wxString pattern = m_assignmentGrid->GetCellValue( row, 0 );
        wxString netclass = m_assignmentGrid->GetCellValue( row, 1 );

        m_netSettings->m_NetClassPatternAssignments.push_back(
                {
                    std::make_unique<EDA_COMBINED_MATCHER>( pattern, CTX_NETCLASS ),
                    netclass
                } );
    }

    return true;
}


bool PANEL_SETUP_NETCLASSES::validateNetclassName( int aRow, const wxString& aName,
                                                   bool focusFirst )
{
    wxString tmp = aName;

    tmp.Trim( true );
    tmp.Trim( false );

    if( tmp.IsEmpty() )
    {
        wxString msg =  _( "Netclass must have a name." );
        m_parent->SetError( msg, this, m_netclassGrid, aRow, GRID_NAME );
        return false;
    }

    for( int ii = 0; ii < m_netclassGrid->GetNumberRows(); ii++ )
    {
        if( ii != aRow && m_netclassGrid->GetCellValue( ii, GRID_NAME ).CmpNoCase( tmp ) == 0 )
        {
            wxString msg = _( "Netclass name already in use." );
            m_parent->SetError( msg, this, m_netclassGrid, focusFirst ? aRow : ii, GRID_NAME );
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
                for( int row = 0; row < m_assignmentGrid->GetNumberRows(); ++row )
                {
                    if( m_assignmentGrid->GetCellValue( row, 1 ) == oldName )
                        m_assignmentGrid->SetCellValue( row, 1, newName );
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

        if( m_isEEschema && col > GRID_NAME )
            col += EESCHEMA_COL_OFFSET;

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
    if( m_isEEschema )
    {
        for( int col = GRID_FIRST_EESCHEMA; col < GRID_END; col++ )
        {
            wxString defaultValue = m_netclassGrid->GetCellValue( 0, col - EESCHEMA_COL_OFFSET );
            m_netclassGrid->SetCellValue( row, col - EESCHEMA_COL_OFFSET, defaultValue );
        }
    }
    else
    {
        for( int col = GRID_FIRST_PCBNEW; col < GRID_FIRST_EESCHEMA; col++ )
        {
            wxString defaultValue = m_netclassGrid->GetCellValue( 0, col );
            m_netclassGrid->SetCellValue( row, col, defaultValue );
        }
    }

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

    for( int row = 0; row < m_assignmentGrid->GetNumberRows(); ++row )
    {
        if( m_assignmentGrid->GetCellValue( row, 1 ) == classname )
            m_assignmentGrid->SetCellValue( row, 1, NETCLASS::Default );
    }

    m_netclassGrid->DeleteRows( curRow, 1 );

    m_netclassGrid->MakeCellVisible( std::max( 0, curRow-1 ), m_netclassGrid->GetGridCursorCol() );
    m_netclassGrid->SetGridCursor( std::max( 0, curRow-1 ), m_netclassGrid->GetGridCursorCol() );

    m_netclassesDirty = true;
}


void PANEL_SETUP_NETCLASSES::AdjustNetclassGridColumns( int aWidth )
{
    if( aWidth != m_lastNetclassGridWidth )
    {
        m_lastNetclassGridWidth = aWidth;

        // Account for scroll bars
        aWidth -= ( m_netclassGrid->GetSize().x - m_netclassGrid->GetClientSize().x );

        for( int i = 1; i < m_netclassGrid->GetNumberCols(); i++ )
        {
            m_netclassGrid->SetColSize( i, m_originalColWidths[ i ] );
            aWidth -= m_originalColWidths[ i ];
        }

        m_netclassGrid->SetColSize( 0, std::max( aWidth - 2, m_originalColWidths[ 0 ] ) );
    }
}


void PANEL_SETUP_NETCLASSES::OnSizeNetclassGrid( wxSizeEvent& event )
{
    AdjustNetclassGridColumns( event.GetSize().GetX() );

    event.Skip();
}


void PANEL_SETUP_NETCLASSES::OnAddAssignmentClick( wxCommandEvent& event )
{
    if( !m_assignmentGrid->CommitPendingChanges() )
        return;

    int row = m_assignmentGrid->GetNumberRows();
    m_assignmentGrid->AppendRows();

    m_assignmentGrid->SetCellValue( row, 1, m_netSettings->m_DefaultNetClass->GetName() );

    m_assignmentGrid->MakeCellVisible( row, 0 );
    m_assignmentGrid->SetGridCursor( row, 0 );

    m_assignmentGrid->EnableCellEditControl( true );
    m_assignmentGrid->ShowCellEditControl();
}


void PANEL_SETUP_NETCLASSES::OnRemoveAssignmentClick( wxCommandEvent& event )
{
    if( !m_assignmentGrid->CommitPendingChanges() )
        return;

    int curRow = m_assignmentGrid->GetGridCursorRow();

    if( curRow < 0 )
        return;

    m_assignmentGrid->DeleteRows( curRow, 1 );

    if( m_assignmentGrid->GetNumberRows() > 0 )
    {
        m_assignmentGrid->MakeCellVisible( std::max( 0, curRow-1 ), 0 );
        m_assignmentGrid->SetGridCursor( std::max( 0, curRow-1 ), 0 );
    }
}


void PANEL_SETUP_NETCLASSES::AdjustAssignmentGridColumns( int aWidth )
{
    // Account for scroll bars
    aWidth -= ( m_assignmentGrid->GetSize().x - m_assignmentGrid->GetClientSize().x );

    // Set className column width to original className width from netclasses grid
    int classNameWidth = m_originalColWidths[ 0 ];
    m_assignmentGrid->SetColSize( 1, m_originalColWidths[ 0 ] );
    m_assignmentGrid->SetColSize( 0, std::max( aWidth - classNameWidth, classNameWidth ) );
}


void PANEL_SETUP_NETCLASSES::OnSizeAssignmentGrid( wxSizeEvent& event )
{
    AdjustAssignmentGridColumns( event.GetSize().GetX());

    event.Skip();
}


void PANEL_SETUP_NETCLASSES::OnUpdateUI( wxUpdateUIEvent& event )
{
    if( m_netclassesDirty )
    {
        rebuildNetclassDropdowns();
        m_netclassesDirty = false;
    }

    int      row = m_assignmentGrid->GetGridCursorRow();
    wxString pattern = m_assignmentGrid->GetCellValue( row, 0 );

    if( m_assignmentGrid->GetGridCursorCol() == 0 && m_assignmentGrid->IsCellEditControlShown() )
    {
        wxGridCellEditor* cellEditor = m_assignmentGrid->GetCellEditor( row, 0 );

        if( wxTextEntry* txt = dynamic_cast<wxTextEntry*>( cellEditor->GetControl() ) )
            pattern = txt->GetValue();

        cellEditor->DecRef();
    }

    if( pattern != m_lastPattern )
    {
        m_matchingNets->Clear();

        if( !pattern.IsEmpty() )
        {
            EDA_COMBINED_MATCHER matcher( pattern, CTX_NETCLASS );

            m_matchingNets->Report( wxString::Format( _( "<b>Nets matching '%s':</b>" ),
                                                      pattern ) );

            for( const wxString& net : m_netNames )
            {
                int matches;
                int offset;

                if( matcher.Find( net, matches, offset ) && offset == 0 )
                    m_matchingNets->Report( net );
            }
        }

        m_matchingNets->Flush();
        m_lastPattern = pattern;
    }
}


bool PANEL_SETUP_NETCLASSES::Validate()
{
    if( !m_netclassGrid->CommitPendingChanges() || !m_assignmentGrid->CommitPendingChanges() )
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


void PANEL_SETUP_NETCLASSES::ImportSettingsFrom( const std::shared_ptr<NET_SETTINGS>& aNetSettings )
{
    std::shared_ptr<NET_SETTINGS> savedSettings = m_netSettings;

    m_netSettings = aNetSettings;
    TransferDataToWindow();

    rebuildNetclassDropdowns();

    m_netclassGrid->ForceRefresh();
    m_assignmentGrid->ForceRefresh();

    m_netSettings = savedSettings;
}



/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2009 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009 Dick Hollenbeck, dick@softplc.com
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <limits>
#include <wx/wupdlock.h>
#include <pgm_base.h>
#include <eda_draw_frame.h>
#include <bitmaps.h>
#include <netclass.h>
#include <gal/painter.h>
#include <grid_tricks.h>
#include <dialogs/panel_setup_netclasses.h>
#include <tool/tool_manager.h>
#include <pcb_painter.h>
#include <board_design_settings.h>
#include <string_utils.h>
#include <view/view.h>
#include <widgets/grid_color_swatch_helpers.h>
#include <widgets/grid_icon_text_helpers.h>
#include <widgets/wx_html_report_box.h>
#include <widgets/wx_panel.h>
#include <widgets/std_bitmap_button.h>
#include <project/net_settings.h>
#include <confirm.h>


// columns of netclasses grid
enum
{
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
    GRID_DELAY_PROFILE,
    GRID_PCB_COLOR,

    GRID_FIRST_EESCHEMA,
    GRID_WIREWIDTH = GRID_FIRST_EESCHEMA,
    GRID_BUSWIDTH,
    GRID_SCHEMATIC_COLOR,
    GRID_LINESTYLE,

    GRID_END
};

std::vector<BITMAPS> g_lineStyleIcons;
wxArrayString        g_lineStyleNames;


PANEL_SETUP_NETCLASSES::PANEL_SETUP_NETCLASSES( wxWindow* aParentWindow, EDA_DRAW_FRAME* aFrame,
                                                std::shared_ptr<NET_SETTINGS> aNetSettings,
                                                const std::set<wxString>& aNetNames, bool aIsEEschema ) :
        PANEL_SETUP_NETCLASSES_BASE( aParentWindow ),
        m_frame( aFrame ),
        m_isEEschema( aIsEEschema ),
        m_netSettings( std::move( aNetSettings ) ),
        m_netNames( aNetNames ),
        m_lastCheckedTicker( 0 ),
        m_hoveredCol( -1 ),
        m_lastNetclassGridWidth( -1 ),
        m_sortAsc( false ),
        m_sortCol( 0 )
{
    // Clear and re-load each time.  Language (or darkmode) might have changed.
    g_lineStyleIcons.clear();
    g_lineStyleNames.clear();

    g_lineStyleIcons.push_back( BITMAPS::stroke_none );
    g_lineStyleNames.push_back( _( "<Not defined>" ) );
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

    m_schUnitsProvider = std::make_unique<UNITS_PROVIDER>( schIUScale, m_frame->GetUserUnits() );
    m_pcbUnitsProvider = std::make_unique<UNITS_PROVIDER>( pcbIUScale, m_frame->GetUserUnits() );

    m_netclassesPane->SetBorders( true, false, false, false );
    m_membershipPane->SetBorders( true, false, false, false );

    // Prevent Size events from firing before we are ready
    wxWindowUpdateLocker updateLock( this );

    m_netclassGrid->BeginBatch();
    m_netclassGrid->SetUseNativeColLabels();
    m_assignmentGrid->BeginBatch();
    m_assignmentGrid->SetUseNativeColLabels();

    m_splitter->SetMinimumPaneSize( FromDIP( m_splitter->GetMinimumPaneSize() ) );

    wxASSERT( m_netclassGrid->GetNumberCols() == GRID_END );

    // Calculate a min best size to handle longest usual numeric values:
    int const min_best_width = m_netclassGrid->GetTextExtent( "555,555555 mils" ).x;

    for( int i = 0; i < m_netclassGrid->GetNumberCols(); ++i )
    {
        // We calculate the column min size only from texts sizes, not using the initial col width
        // as this initial width is sometimes strange depending on the language (wxGrid bug?)
        int const min_width =  m_netclassGrid->GetVisibleWidth( i, true, true );

        int const weighted_min_best_width = ( i == GRID_LINESTYLE ) ? min_best_width * 3 / 2
                                                                    : min_best_width;

        // We use a "best size" >= min_best_width
        m_originalColWidths[ i ] = std::max( min_width, weighted_min_best_width );
        m_netclassGrid->SetColSize( i, m_originalColWidths[ i ] );

        if( i >= GRID_FIRST_EESCHEMA )
            m_netclassGrid->SetUnitsProvider( m_schUnitsProvider.get(), i );
        else
            m_netclassGrid->SetUnitsProvider( m_pcbUnitsProvider.get(), i );
    }

    if( m_isEEschema )
        m_netclassGrid->ShowHideColumns( "0 11 12 13 14" );
    else
        m_netclassGrid->ShowHideColumns( "0 1 2 3 4 5 6 7 8 9 10" );

    m_shownColumns = m_netclassGrid->GetShownColumns();

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetRenderer( new GRID_CELL_COLOR_RENDERER( PAGED_DIALOG::GetDialog( this ) ) );
    attr->SetEditor( new GRID_CELL_COLOR_SELECTOR( PAGED_DIALOG::GetDialog( this ), m_netclassGrid ) );
    m_netclassGrid->SetColAttr( GRID_SCHEMATIC_COLOR, attr );

    attr = new wxGridCellAttr;
    attr->SetRenderer( new GRID_CELL_COLOR_RENDERER( PAGED_DIALOG::GetDialog( this ) ) );
    attr->SetEditor( new GRID_CELL_COLOR_SELECTOR( PAGED_DIALOG::GetDialog( this ), m_netclassGrid ) );
    m_netclassGrid->SetColAttr( GRID_PCB_COLOR, attr );

    attr = new wxGridCellAttr;
    attr->SetRenderer( new GRID_CELL_ICON_TEXT_RENDERER( g_lineStyleIcons, g_lineStyleNames ) );
    attr->SetEditor( new GRID_CELL_ICON_TEXT_POPUP( g_lineStyleIcons, g_lineStyleNames ) );
    m_netclassGrid->SetColAttr( GRID_LINESTYLE, attr );

    if( m_isEEschema )
    {
        m_importColorsButton->Hide();
    }
    else
    {
        m_colorDefaultHelpText->SetLabel( _( "Set color to transparent to use layer default color." ) );
        m_colorDefaultHelpText->GetParent()->Layout();
    }

    m_colorDefaultHelpText->SetFont( KIUI::GetInfoFont( this ).Italic() );

    m_netclassGrid->SetAutoEvalCols( { GRID_WIREWIDTH,
                                       GRID_BUSWIDTH,
                                       GRID_CLEARANCE,
                                       GRID_TRACKSIZE,
                                       GRID_VIASIZE,
                                       GRID_VIADRILL,
                                       GRID_uVIASIZE,
                                       GRID_uVIADRILL,
                                       GRID_DIFF_PAIR_WIDTH,
                                       GRID_DIFF_PAIR_GAP } );

    // Be sure the column labels are readable
    m_netclassGrid->EnsureColLabelsVisible();

    m_netclassGrid->PushEventHandler( new GRID_TRICKS( m_netclassGrid ) );
    m_assignmentGrid->PushEventHandler( new GRID_TRICKS( m_assignmentGrid ) );

    m_netclassGrid->SetSelectionMode( wxGrid::wxGridSelectRows );
    m_assignmentGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    m_addButton->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_removeButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    m_addAssignmentButton->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_removeAssignmentButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    m_moveUpButton->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );
    m_moveDownButton->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );

    // wxFormBuilder doesn't include this event...
    m_netclassGrid->Connect( wxEVT_GRID_CELL_CHANGING,
                             wxGridEventHandler( PANEL_SETUP_NETCLASSES::OnNetclassGridCellChanging ),
                             nullptr, this );

    // Handle tooltips for grid
    m_netclassGrid->GetGridColLabelWindow()->Bind( wxEVT_MOTION, &PANEL_SETUP_NETCLASSES::OnNetclassGridMouseEvent,
                                                   this );

    // Allow sorting assignments by column
    m_assignmentGrid->Connect( wxEVT_GRID_LABEL_LEFT_CLICK,
                               wxGridEventHandler( PANEL_SETUP_NETCLASSES::OnNetclassAssignmentSort ),
                               nullptr, this );

    m_frame->Bind( EDA_EVT_UNITS_CHANGED, &PANEL_SETUP_NETCLASSES::onUnitsChanged, this );

    m_netclassGrid->EndBatch();
    m_assignmentGrid->EndBatch();

    Bind( wxEVT_IDLE,
          [this]( wxIdleEvent& aEvent )
          {
              // Careful of consuming CPU in an idle event handler.  Check the ticker first to
              // see if there's even a possibility of the netclasses having changed.
              if( m_frame->Prj().GetNetclassesTicker() > m_lastCheckedTicker )
              {
                  wxWindow* dialog = wxGetTopLevelParent( this );
                  wxWindow* topLevelFocus = wxGetTopLevelParent( wxWindow::FindFocus() );

                  if( topLevelFocus == dialog && m_lastLoaded != m_netSettings->GetNetclasses() )
                      checkReload();
              }
          } );

    m_matchingNets->SetFont( KIUI::GetInfoFont( this ) );
}


PANEL_SETUP_NETCLASSES::~PANEL_SETUP_NETCLASSES()
{
    // Delete the GRID_TRICKS.
    m_netclassGrid->PopEventHandler( true );
    m_assignmentGrid->PopEventHandler( true );

    m_netclassGrid->Disconnect( wxEVT_GRID_CELL_CHANGING,
                                wxGridEventHandler( PANEL_SETUP_NETCLASSES::OnNetclassGridCellChanging ),
                                nullptr, this );

    m_assignmentGrid->Disconnect( wxEVT_GRID_LABEL_LEFT_CLICK,
                                  wxGridEventHandler( PANEL_SETUP_NETCLASSES::OnNetclassAssignmentSort ),
                                  nullptr, this );

    m_frame->Unbind( EDA_EVT_UNITS_CHANGED, &PANEL_SETUP_NETCLASSES::onUnitsChanged, this );
}


void PANEL_SETUP_NETCLASSES::loadNetclasses()
{
    auto netclassToGridRow =
            [&]( int aRow, const NETCLASS* nc )
            {
                m_netclassGrid->SetCellValue( aRow, GRID_NAME, nc->GetName() );
                m_netclassGrid->SetCellValue( aRow, GRID_DELAY_PROFILE, nc->GetTuningProfile() );

                m_netclassGrid->SetOptionalUnitValue( aRow, GRID_WIREWIDTH, nc->GetWireWidthOpt() );
                m_netclassGrid->SetOptionalUnitValue( aRow, GRID_BUSWIDTH, nc->GetBusWidthOpt() );

                wxString colorAsString = nc->GetSchematicColor().ToCSSString();
                m_netclassGrid->SetCellValue( aRow, GRID_SCHEMATIC_COLOR, colorAsString );

                if( nc->HasLineStyle() )
                {
                    int lineStyleIdx = std::max( 0, nc->GetLineStyle() );

                    if( lineStyleIdx >= (int) g_lineStyleNames.size() + 1 )
                        lineStyleIdx = 0;

                    m_netclassGrid->SetCellValue( aRow, GRID_LINESTYLE, g_lineStyleNames[lineStyleIdx + 1] );
                }
                else
                {
                    // <Not defined> line style in list.
                    m_netclassGrid->SetCellValue( aRow, GRID_LINESTYLE, g_lineStyleNames[0] );
                }

                m_netclassGrid->SetOptionalUnitValue( aRow, GRID_CLEARANCE, nc->GetClearanceOpt() );
                m_netclassGrid->SetOptionalUnitValue( aRow, GRID_TRACKSIZE, nc->GetTrackWidthOpt() );
                m_netclassGrid->SetOptionalUnitValue( aRow, GRID_VIASIZE, nc->GetViaDiameterOpt() );
                m_netclassGrid->SetOptionalUnitValue( aRow, GRID_VIADRILL, nc->GetViaDrillOpt() );
                m_netclassGrid->SetOptionalUnitValue( aRow, GRID_uVIASIZE, nc->GetuViaDiameterOpt() );
                m_netclassGrid->SetOptionalUnitValue( aRow, GRID_uVIADRILL, nc->GetuViaDrillOpt() );
                m_netclassGrid->SetOptionalUnitValue( aRow, GRID_DIFF_PAIR_WIDTH, nc->GetDiffPairWidthOpt() );
                m_netclassGrid->SetOptionalUnitValue( aRow, GRID_DIFF_PAIR_GAP, nc->GetDiffPairGapOpt() );

                colorAsString = nc->GetPcbColor().ToCSSString();
                m_netclassGrid->SetCellValue( aRow, GRID_PCB_COLOR, colorAsString );

                if( nc->IsDefault() )
                {
                    m_netclassGrid->SetReadOnly( aRow, GRID_NAME );
                    m_netclassGrid->SetReadOnly( aRow, GRID_PCB_COLOR );
                    m_netclassGrid->SetReadOnly( aRow, GRID_SCHEMATIC_COLOR );
                    m_netclassGrid->SetReadOnly( aRow, GRID_LINESTYLE );
                }

                setNetclassRowNullableEditors( aRow, nc->IsDefault() );
            };

    // Get the netclasses sorted by priority
    std::vector<const NETCLASS*> netclasses;
    netclasses.reserve( m_netSettings->GetNetclasses().size() );

    for( const auto& [name, netclass] : m_netSettings->GetNetclasses() )
        netclasses.push_back( netclass.get() );

    std::sort( netclasses.begin(), netclasses.end(),
               []( const NETCLASS* nc1, const NETCLASS* nc2 )
               {
                   return nc1->GetPriority() < nc2->GetPriority();
               } );

    // Enter user-defined netclasses
    m_netclassGrid->ClearRows();
    m_netclassGrid->AppendRows( static_cast<int>( netclasses.size() ) );

    int row = 0;

    for( const NETCLASS* nc : netclasses )
        netclassToGridRow( row++, nc );

    // Enter the Default netclass.
    m_netclassGrid->AppendRows( 1 );
    netclassToGridRow( row, m_netSettings->GetDefaultNetclass().get() );

    m_assignmentGrid->ClearRows();
    m_assignmentGrid->AppendRows( m_netSettings->GetNetclassPatternAssignments().size() );

    row = 0;

    for( const auto& [matcher, netclassName] : m_netSettings->GetNetclassPatternAssignments() )
    {
        m_assignmentGrid->SetCellValue( row, 0, matcher->GetPattern() );
        m_assignmentGrid->SetCellValue( row, 1, netclassName );
        row++;
    }
}


void PANEL_SETUP_NETCLASSES::setNetclassRowNullableEditors( int aRowId, bool aIsDefault )
{
    // Set nullable editors
    auto setCellEditor =
            [this, aRowId, aIsDefault]( int aCol )
            {
                GRID_CELL_MARK_AS_NULLABLE* cellEditor;

                if( aIsDefault )
                    cellEditor = new GRID_CELL_MARK_AS_NULLABLE( false );
                else
                    cellEditor = new GRID_CELL_MARK_AS_NULLABLE( true );

                wxGridCellAttr* attr = m_netclassGrid->GetOrCreateCellAttr( aRowId, aCol );
                attr->SetEditor( cellEditor );
                attr->DecRef();
            };

    setCellEditor( GRID_WIREWIDTH );
    setCellEditor( GRID_BUSWIDTH );
    setCellEditor( GRID_CLEARANCE );
    setCellEditor( GRID_TRACKSIZE );
    setCellEditor( GRID_VIASIZE );
    setCellEditor( GRID_VIADRILL );
    setCellEditor( GRID_VIADRILL );
    setCellEditor( GRID_uVIASIZE );
    setCellEditor( GRID_uVIADRILL );
    setCellEditor( GRID_DIFF_PAIR_WIDTH );
    setCellEditor( GRID_DIFF_PAIR_GAP );
}


void PANEL_SETUP_NETCLASSES::checkReload()
{
    // MUST update the ticker before calling IsOK (or we'll end up re-entering through the idle
    // event until we crash the stack).
    m_lastCheckedTicker = m_frame->Prj().GetTextVarsTicker();

    if( IsOK( m_parent, _( "The netclasses have been changed outside the Setup dialog.\n"
                           "Do you wish to reload them?" ) ) )
    {
        m_lastLoaded = m_netSettings->GetNetclasses();
        loadNetclasses();
    }
}


void PANEL_SETUP_NETCLASSES::onUnitsChanged( wxCommandEvent& aEvent )
{
    std::shared_ptr<NET_SETTINGS> tempNetSettings = std::make_shared<NET_SETTINGS>( nullptr, "" );
    std::shared_ptr<NET_SETTINGS> saveNetSettings = m_netSettings;

    m_netSettings = tempNetSettings;

    TransferDataFromWindow();

    m_schUnitsProvider->SetUserUnits( m_frame->GetUserUnits() );
    m_pcbUnitsProvider->SetUserUnits( m_frame->GetUserUnits() );

    TransferDataToWindow();

    m_netSettings = saveNetSettings;

    aEvent.Skip();
}


bool PANEL_SETUP_NETCLASSES::TransferDataToWindow()
{
    m_lastLoaded = m_netSettings->GetNetclasses();
    m_lastCheckedTicker = m_frame->Prj().GetNetclassesTicker();

    loadNetclasses();
    AdjustAssignmentGridColumns( GetSize().x * 3 / 5 );

    return true;
}


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

    auto gridRowToNetclass =
            [&]( int aRow, const std::shared_ptr<NETCLASS>& nc )
            {
                if( nc->IsDefault() )
                    nc->SetPriority( std::numeric_limits<int>::max() );
                else
                    nc->SetPriority( aRow );

                nc->SetName( m_netclassGrid->GetCellValue( aRow, GRID_NAME ) );
                nc->SetTuningProfile( m_netclassGrid->GetCellValue( aRow, GRID_DELAY_PROFILE ) );

                nc->SetWireWidth( m_netclassGrid->GetOptionalUnitValue( aRow, GRID_WIREWIDTH ) );
                nc->SetBusWidth( m_netclassGrid->GetOptionalUnitValue( aRow, GRID_BUSWIDTH ) );

                wxString lineStyle = m_netclassGrid->GetCellValue( aRow, GRID_LINESTYLE );
                int      lineIdx = g_lineStyleNames.Index( lineStyle );

                if( lineIdx == 0 )
                    nc->SetLineStyle( std::optional<int>() );
                else
                    nc->SetLineStyle( lineIdx - 1 );

                wxASSERT_MSG( lineIdx >= 0, "Line style name not found." );

                // clang-format off
                nc->SetClearance( m_netclassGrid->GetOptionalUnitValue( aRow, GRID_CLEARANCE ) );
                nc->SetTrackWidth( m_netclassGrid->GetOptionalUnitValue( aRow, GRID_TRACKSIZE ) );
                nc->SetViaDiameter( m_netclassGrid->GetOptionalUnitValue( aRow, GRID_VIASIZE ) );
                nc->SetViaDrill( m_netclassGrid->GetOptionalUnitValue( aRow, GRID_VIADRILL ) );
                nc->SetuViaDiameter( m_netclassGrid->GetOptionalUnitValue( aRow, GRID_uVIASIZE ) );
                nc->SetuViaDrill( m_netclassGrid->GetOptionalUnitValue( aRow, GRID_uVIADRILL ) );
                nc->SetDiffPairWidth( m_netclassGrid->GetOptionalUnitValue( aRow, GRID_DIFF_PAIR_WIDTH ) );
                nc->SetDiffPairGap( m_netclassGrid->GetOptionalUnitValue( aRow, GRID_DIFF_PAIR_GAP ) );
                // clang-format on

                if( !nc->IsDefault() )
                {
                    wxString color = m_netclassGrid->GetCellValue( aRow, GRID_PCB_COLOR );
                    KIGFX::COLOR4D newPcbColor( color );

                    if( newPcbColor != KIGFX::COLOR4D::UNSPECIFIED )
                        nc->SetPcbColor( newPcbColor );

                    color = m_netclassGrid->GetCellValue( aRow, GRID_SCHEMATIC_COLOR );
                    KIGFX::COLOR4D newSchematicColor( color );

                    if( newSchematicColor != KIGFX::COLOR4D::UNSPECIFIED )
                        nc->SetSchematicColor( newSchematicColor );
                }
            };

    m_netSettings->ClearNetclasses();

    // Copy the default NetClass:
    gridRowToNetclass( m_netclassGrid->GetNumberRows() - 1, m_netSettings->GetDefaultNetclass() );

    // Copy other NetClasses:
    for( int row = 0; row < m_netclassGrid->GetNumberRows() - 1; ++row )
    {
        auto nc = std::make_shared<NETCLASS>( m_netclassGrid->GetCellValue( row, GRID_NAME ), false );
        gridRowToNetclass( row, nc );
        m_netSettings->SetNetclass( nc->GetName(), nc );
    }

    m_netSettings->ClearNetclassPatternAssignments();
    m_netSettings->ClearAllCaches();

    for( int row = 0; row < m_assignmentGrid->GetNumberRows(); ++row )
    {
        wxString pattern = m_assignmentGrid->GetCellValue( row, 0 );
        wxString netclass = m_assignmentGrid->GetCellValue( row, 1 );

        m_netSettings->SetNetclassPatternAssignment( pattern, netclass );
    }

    return true;
}


bool PANEL_SETUP_NETCLASSES::validateNetclassName( int aRow, const wxString& aName, bool focusFirst )
{
    wxString tmp = aName;

    tmp.Trim( true );
    tmp.Trim( false );

    if( tmp.IsEmpty() )
    {
        wxString msg =  _( "Netclass must have a name." );
        PAGED_DIALOG::GetDialog( this )->SetError( msg, this, m_netclassGrid, aRow, GRID_NAME );
        return false;
    }

    for( int ii = 0; ii < m_netclassGrid->GetNumberRows(); ii++ )
    {
        if( ii != aRow && m_netclassGrid->GetCellValue( ii, GRID_NAME ).CmpNoCase( tmp ) == 0 )
        {
            wxString msg = _( "Netclass name already in use." );
            PAGED_DIALOG::GetDialog( this )->SetError( msg, this, m_netclassGrid, focusFirst ? aRow : ii,
                                                       GRID_NAME );
            return false;
        }
    }

    return true;
}


bool PANEL_SETUP_NETCLASSES::validateNetclassClearance( int aRow )
{
    // Clip clearance.  This is not a nag; we can end up with overflow errors and very poor
    // performance if the clearance is too large.

    std::optional<int> clearance = m_netclassGrid->GetOptionalUnitValue( aRow, GRID_CLEARANCE );

    if( clearance.has_value() && clearance.value() > MAXIMUM_CLEARANCE )
    {
        wxString msg = wxString::Format( _( "Clearance was too large.  It has been clipped to %s." ),
                                         m_frame->StringFromValue( MAXIMUM_CLEARANCE, true ) );
        PAGED_DIALOG::GetDialog( this )->SetError( msg, this, m_netclassGrid, aRow, GRID_CLEARANCE );
        m_netclassGrid->SetUnitValue( aRow, GRID_CLEARANCE, MAXIMUM_CLEARANCE );
        return false;
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
    else if( event.GetCol() == GRID_CLEARANCE )
    {
        validateNetclassClearance( event.GetRow() );
    }
}

void PANEL_SETUP_NETCLASSES::OnNetclassAssignmentSort( wxGridEvent& event )
{
    event.Skip();

    if( !m_assignmentGrid->CommitPendingChanges() )
        return;

    if( ( event.GetCol() < 0 ) || ( event.GetCol() >= m_assignmentGrid->GetNumberCols() ) )
        return;

    // Toggle sort order if the same column is clicked
    if( event.GetCol() != m_sortCol )
    {
        m_sortCol = event.GetCol();
        m_sortAsc = true;
    }
    else
    {
        m_sortAsc = !m_sortAsc;
    }

    std::vector<std::pair<wxString, wxString>> netclassesassignments;
    netclassesassignments.reserve( m_assignmentGrid->GetNumberRows() );

    for( int row = 0; row < m_assignmentGrid->GetNumberRows(); ++row )
    {
        netclassesassignments.emplace_back( m_assignmentGrid->GetCellValue( row, 0 ),
                                            m_assignmentGrid->GetCellValue( row, 1 ) );
    }

    std::sort( netclassesassignments.begin(), netclassesassignments.end(),
               [this]( const std::pair<wxString, wxString>& assign1,
                       const std::pair<wxString, wxString>& assign2 )
               {
                   const wxString& str1 = ( m_sortCol == 0 ) ? assign1.first : assign1.second;
                   const wxString& str2 = ( m_sortCol == 0 ) ? assign2.first : assign2.second;
                   return m_sortAsc ? ( str1 < str2 ) : ( str1 > str2 );
               } );

    m_assignmentGrid->ClearRows();
    m_assignmentGrid->AppendRows( netclassesassignments.size() );

    int row = 0;

    for( const auto& [pattern, netclassName] : netclassesassignments )
    {
        m_assignmentGrid->SetCellValue( row, 0, pattern );
        m_assignmentGrid->SetCellValue( row, 1, netclassName );
        row++;
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
        case GRID_PCB_COLOR:        tip = _( "PCB netclass color" );            break;
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
    m_netclassGrid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                m_netclassGrid->InsertRows();

                // Set defaults where required
                wxString colorAsString = KIGFX::COLOR4D::UNSPECIFIED.ToCSSString();
                m_netclassGrid->SetCellValue( 0, GRID_PCB_COLOR, colorAsString );
                m_netclassGrid->SetCellValue( 0, GRID_SCHEMATIC_COLOR, colorAsString );
                m_netclassGrid->SetCellValue( 0, GRID_LINESTYLE, g_lineStyleNames[0] );

                // Set the row nullable editors
                setNetclassRowNullableEditors( 0, false );

                m_netclassesDirty = true;
                return { 0, GRID_NAME };
            } );
}


void PANEL_SETUP_NETCLASSES::OnRemoveNetclassClick( wxCommandEvent& event )
{
    m_netclassGrid->OnDeleteRows(
            [&]( int row )
            {
                if( row == m_netclassGrid->GetNumberRows() - 1 )
                {
                    DisplayErrorMessage( wxGetTopLevelParent( this ), _( "The default net class is required." ) );
                    return false;
                }

                return true;
            },
            [&]( int row )
            {
                // reset the net class to default for members of the removed class
                wxString classname = m_netclassGrid->GetCellValue( row, GRID_NAME );

                for( int assignment = 0; assignment < m_assignmentGrid->GetNumberRows(); ++assignment )
                {
                    if( m_assignmentGrid->GetCellValue( assignment, 1 ) == classname )
                        m_assignmentGrid->SetCellValue( assignment, 1, NETCLASS::Default );
                }

                m_netclassGrid->DeleteRows( row, 1 );
                m_netclassesDirty = true;
            } );
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
            if( m_netclassGrid->GetColSize( i ) > 0 )
            {
                m_netclassGrid->SetColSize( i, m_originalColWidths[ i ] );
                aWidth -= m_originalColWidths[ i ];
            }
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
    m_assignmentGrid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                int row = m_assignmentGrid->GetNumberRows();
                m_assignmentGrid->AppendRows();
                m_assignmentGrid->SetCellValue( row, 1, m_netSettings->GetDefaultNetclass()->GetName() );
                return { row, 0 };
            } );
}


void PANEL_SETUP_NETCLASSES::OnRemoveAssignmentClick( wxCommandEvent& event )
{
    m_assignmentGrid->OnDeleteRows(
            [&]( int row )
            {
                m_assignmentGrid->DeleteRows( row, 1 );
            } );
}


void PANEL_SETUP_NETCLASSES::OnImportColorsClick( wxCommandEvent& event )
{
    const std::map<wxString, std::shared_ptr<NETCLASS>>& netclasses =
            m_netSettings->GetNetclasses();

    for( int row = 0; row < m_netclassGrid->GetNumberRows() - 1; ++row )
    {
        wxString netclassName = m_netclassGrid->GetCellValue( row, GRID_NAME );

        if( netclasses.find( netclassName ) != netclasses.end() )
        {
            const KIGFX::COLOR4D ncColor = netclasses.at( netclassName )->GetSchematicColor();
            m_netclassGrid->SetCellValue( row, GRID_PCB_COLOR, ncColor.ToCSSString() );
        }
    }
}


void PANEL_SETUP_NETCLASSES::AdjustAssignmentGridColumns( int aWidth )
{
    // Account for scroll bars
    aWidth -= ( m_assignmentGrid->GetSize().x - m_assignmentGrid->GetClientSize().x );

    int classNameWidth = 160;
    m_assignmentGrid->SetColSize( 1, classNameWidth );
    m_assignmentGrid->SetColSize( 0, std::max( aWidth - classNameWidth, classNameWidth ) );
}


void PANEL_SETUP_NETCLASSES::OnSizeAssignmentGrid( wxSizeEvent& event )
{
    AdjustAssignmentGridColumns( event.GetSize().GetX() );

    event.Skip();
}


void PANEL_SETUP_NETCLASSES::OnUpdateUI( wxUpdateUIEvent& event )
{
    if( m_netclassesDirty )
    {
        rebuildNetclassDropdowns();
        m_netclassesDirty = false;
    }

    if( m_shownColumns != m_netclassGrid->GetShownColumns() )
    {
        AdjustNetclassGridColumns( GetSize().x - 1 );
        m_shownColumns = m_netclassGrid->GetShownColumns();
    }

    if( m_assignmentGrid->GetNumberRows() == 0 )
        return;

    wxString pattern;
    int      row = m_assignmentGrid->GetGridCursorRow();
    int      col = m_assignmentGrid->GetGridCursorCol();

    if( row >= 0 )
        pattern = m_assignmentGrid->GetCellValue( row, 0 );

    if( col == 0 && m_assignmentGrid->IsCellEditControlShown() )
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

            m_matchingNets->Report( wxString::Format( _( "<b>Nets matching '%s':</b>" ), pattern ) );

            for( const wxString& net : m_netNames )
            {
                if( matcher.StartsWith( net ) )
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

    // Test net class parameters.
    for( int row = 0; row < m_netclassGrid->GetNumberRows(); row++ )
    {
        wxString netclassName = m_netclassGrid->GetCellValue( row, GRID_NAME );
        netclassName.Trim( true );
        netclassName.Trim( false );

        if( !validateNetclassName( row, netclassName, false ) )
            return false;

        if( !validateNetclassClearance( row ) )
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

    m_netSettings = std::move( savedSettings );
}


void PANEL_SETUP_NETCLASSES::OnMoveNetclassUpClick( wxCommandEvent& event )
{
    m_netclassGrid->OnMoveRowUp(
            [&]( int row )
            {
                // Can't move the Default netclass
                return row != m_netclassGrid->GetNumberRows() - 1;
            },
            [&]( int row )
            {
                m_netclassGrid->SwapRows( row, row - 1 );
                m_netclassesDirty = true;
            } );
}


void PANEL_SETUP_NETCLASSES::OnMoveNetclassDownClick( wxCommandEvent& event )
{
    m_netclassGrid->OnMoveRowDown(
            [&]( int row )
            {
                // Can't move the Default netclass
                return row + 1 != m_netclassGrid->GetNumberRows() - 1;
            },
            [&]( int row )
            {
                m_netclassGrid->SwapRows( row, row + 1 );
                m_netclassesDirty = true;
            } );
}


void PANEL_SETUP_NETCLASSES::UpdateDelayProfileNames( const std::vector<wxString>& aNames ) const
{
    wxArrayString profileNames;
    profileNames.push_back( wxEmptyString );
    std::ranges::for_each( aNames,
                           [&]( const wxString& aName )
                           {
                               profileNames.push_back( aName );
                           } );

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetEditor( new wxGridCellChoiceEditor( profileNames, false ) );
    m_netclassGrid->SetColAttr( GRID_DELAY_PROFILE, attr );
}

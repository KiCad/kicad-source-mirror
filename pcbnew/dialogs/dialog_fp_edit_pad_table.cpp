/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include "dialog_fp_edit_pad_table.h"

#include <wx/display.h>
#include <wx/dcclient.h>
#include <pcb_shape.h>
#include <widgets/wx_grid.h>
#include <widgets/grid_text_helpers.h>
#include <widgets/grid_combobox.h>
#include <base_units.h>
#include <units_provider.h>
#include <board.h>
#include <footprint.h>
#include <footprint_edit_frame.h>
#include <grid_tricks.h>
#include <pin_numbers.h>
#include <board_commit.h>


// Helper to map shape string to PAD_SHAPE
static PAD_SHAPE ShapeFromString( const wxString& shape )
{
    if( shape == _( "Oval" ) )                return PAD_SHAPE::OVAL;
    if( shape == _( "Rectangle" ) )           return PAD_SHAPE::RECTANGLE;
    if( shape == _( "Trapezoid" ) )           return PAD_SHAPE::TRAPEZOID;
    if( shape == _( "Rounded rectangle" ) )   return PAD_SHAPE::ROUNDRECT;
    if( shape == _( "Chamfered rectangle" ) ) return PAD_SHAPE::CHAMFERED_RECT;
    if( shape == _( "Custom shape" ) )        return PAD_SHAPE::CUSTOM;

    return PAD_SHAPE::CIRCLE;
}


DIALOG_FP_EDIT_PAD_TABLE::DIALOG_FP_EDIT_PAD_TABLE( PCB_BASE_FRAME* aParent, FOOTPRINT* aFootprint ) :
        DIALOG_FP_EDIT_PAD_TABLE_BASE( (wxWindow*) aParent ),
        m_frame( aParent ),
        m_footprint( aFootprint ),
        m_unitsProvider( std::make_unique<UNITS_PROVIDER>( pcbIUScale, GetUserUnits() ) ),
        m_summaryDirty( true )
{
    CaptureOriginalPadState();

    // The base class created a single placeholder row; resize the grid to fit the pads.
    if( m_grid->GetNumberRows() > 0 )
        m_grid->DeleteRows( 0, m_grid->GetNumberRows() );

    if( !m_originalPads.empty() )
        m_grid->AppendRows( m_originalPads.size() );

    // Constrain summary label widths so they ellipsize rather than push the layout around
    // when long pin-number summaries (or duplicate lists) are produced.
    const int summaryW = m_pin_numbers_summary->GetCharWidth() * 30;

    m_pin_numbers_summary->SetMinSize( wxSize( summaryW, -1 ) );
    m_pin_numbers_summary->SetMaxSize( wxSize( summaryW, -1 ) );

    m_duplicate_pins->SetMinSize( wxSize( summaryW, -1 ) );
    m_duplicate_pins->SetMaxSize( wxSize( summaryW, -1 ) );

    wxGridCellAttr* attr;

    // Type column editor (attribute)
    attr = new wxGridCellAttr;
    wxArrayString typeNames;
    typeNames.push_back( _( "Through-hole" ) ); // PTH
    typeNames.push_back( _( "SMD" ) );          // SMD
    typeNames.push_back( _( "Connector" ) );    // CONN SMD? (use CONN?)
    typeNames.push_back( _( "NPTH" ) );         // NPTH
    typeNames.push_back( _( "Aperture" ) );     // inferred copper-less
    attr->SetEditor( new GRID_CELL_COMBOBOX( typeNames ) );
    m_grid->SetColAttr( COL_TYPE, attr );

    attr = new wxGridCellAttr;
    wxArrayString shapeNames;
    shapeNames.push_back( PAD::ShowPadShape( PAD_SHAPE::CIRCLE ) );
    shapeNames.push_back( PAD::ShowPadShape( PAD_SHAPE::OVAL ) );
    shapeNames.push_back( PAD::ShowPadShape( PAD_SHAPE::RECTANGLE ) );
    shapeNames.push_back( PAD::ShowPadShape( PAD_SHAPE::TRAPEZOID ) );
    shapeNames.push_back( PAD::ShowPadShape( PAD_SHAPE::ROUNDRECT ) );
    shapeNames.push_back( PAD::ShowPadShape( PAD_SHAPE::CHAMFERED_RECT ) );
    shapeNames.push_back( PAD::ShowPadShape( PAD_SHAPE::CUSTOM ) );
    attr->SetEditor( new GRID_CELL_COMBOBOX( shapeNames ) );
    m_grid->SetColAttr( COL_SHAPE, attr );

    attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_TEXT_EDITOR() );
    m_grid->SetColAttr( COL_POS_X, attr );

    attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_TEXT_EDITOR() );
    m_grid->SetColAttr( COL_POS_Y, attr );

    attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_TEXT_EDITOR() );
    m_grid->SetColAttr( COL_SIZE_X, attr );

    attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_TEXT_EDITOR() );
    m_grid->SetColAttr( COL_SIZE_Y, attr );

    // Drill X
    attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_TEXT_EDITOR() );
    m_grid->SetColAttr( COL_DRILL_X, attr );

    // Drill Y
    attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_TEXT_EDITOR() );
    m_grid->SetColAttr( COL_DRILL_Y, attr );

    // Pad->Die Length
    m_grid->SetAutoEvalColUnits( COL_P2D_LENGTH, m_unitsProvider->GetUnitsFromType( EDA_DATA_TYPE::DISTANCE ) );

    // Pad->Die Delay
    m_grid->SetAutoEvalColUnits( COL_P2D_DELAY, m_unitsProvider->GetUnitsFromType( EDA_DATA_TYPE::TIME ) );

    m_grid->SetUnitsProvider( m_unitsProvider.get(), COL_POS_X );
    m_grid->SetUnitsProvider( m_unitsProvider.get(), COL_POS_Y );
    m_grid->SetUnitsProvider( m_unitsProvider.get(), COL_SIZE_X );
    m_grid->SetUnitsProvider( m_unitsProvider.get(), COL_SIZE_Y );
    m_grid->SetUnitsProvider( m_unitsProvider.get(), COL_DRILL_X );
    m_grid->SetUnitsProvider( m_unitsProvider.get(), COL_DRILL_Y );
    m_grid->SetAutoEvalCols( { COL_POS_X, COL_POS_Y,
                               COL_SIZE_X, COL_SIZE_Y,
                               COL_DRILL_X, COL_DRILL_Y,
                               COL_P2D_LENGTH,
                               COL_P2D_DELAY } );

    // add Cut, Copy, and Paste to wxGrid
    m_grid->PushEventHandler( new GRID_TRICKS( m_grid ) );

    SetupStandardButtons();

    Layout();
    finishDialogSettings();

    // Cap the initial height so the dialog does not grow off-screen for footprints
    // with many pads. The grid grows to fill the available space via wxEXPAND.
    // Use the parent window to find the display since this dialog isn't shown yet.
    int displayIdx = wxDisplay::GetFromWindow( aParent );

    if( displayIdx == wxNOT_FOUND )
        displayIdx = 0;

    wxRect displayArea = wxDisplay( (unsigned int) displayIdx ).GetClientArea();
    wxSize dlgSize = GetSize();
    int    maxH = ( displayArea.height * 4 ) / 5;

    if( dlgSize.y > maxH )
    {
        dlgSize.y = maxH;
        SetSize( dlgSize );

        // Reset minimum height so the user can resize the capped dialog freely.
        // The minimum width from finishDialogSettings() is still honoured.
        wxSize minSz = GetMinSize();
        minSz.y = -1;
        SetMinSize( minSz );

        Centre();
    }
}


DIALOG_FP_EDIT_PAD_TABLE::~DIALOG_FP_EDIT_PAD_TABLE()
{
    if( m_cancelled )
        RestoreOriginalPadState();

    // destroy GRID_TRICKS before m_grid.
    m_grid->PopEventHandler( true );
}


bool DIALOG_FP_EDIT_PAD_TABLE::TransferDataToWindow()
{
    if( !m_footprint )
        return false;

    int row = 0;

    for( const auto pad : m_originalPads | std::views::keys )
    {
        if( row >= m_grid->GetNumberRows() )
            continue;

        m_grid->SetCellValue( row, COL_NUMBER, pad->GetNumber() );

        // Pad attribute to string
        wxString attrStr;

        switch( pad->GetAttribute() )
        {
        case PAD_ATTRIB::PTH:  attrStr = _( "Through-hole" ); break;
        case PAD_ATTRIB::SMD:  attrStr = _( "SMD" );          break;
        case PAD_ATTRIB::CONN: attrStr = _( "Connector" );    break;
        case PAD_ATTRIB::NPTH: attrStr = _( "NPTH" );         break;
        default:               attrStr = _( "Through-hole" ); break;
        }

        int      size_x = pad->GetSize( PADSTACK::ALL_LAYERS ).x;
        int      size_y = pad->GetSize( PADSTACK::ALL_LAYERS ).y;
        wxString padShape = pad->ShowPadShape( PADSTACK::ALL_LAYERS );

        pad->Padstack().ForEachUniqueLayer(
                [&]( PCB_LAYER_ID aLayer )
                {
                    if( pad->GetSize( aLayer ).x != size_x )
                        size_x = -1;

                    if( pad->GetSize( aLayer ).y != size_y )
                        size_y = -1;

                    if( pad->ShowPadShape( aLayer ) != padShape )
                        padShape = INDETERMINATE_STATE;
                } );

        if( pad->IsAperturePad() )
            attrStr = _( "Aperture" );

        m_grid->SetCellValue( row, COL_TYPE, attrStr );
        m_grid->SetCellValue( row, COL_SHAPE, padShape );
        m_grid->SetCellValue( row, COL_POS_X, m_unitsProvider->StringFromValue( pad->GetPosition().x, true ) );
        m_grid->SetCellValue( row, COL_POS_Y, m_unitsProvider->StringFromValue( pad->GetPosition().y, true ) );
        m_grid->SetCellValue( row, COL_SIZE_X, size_x >= 0 ? m_unitsProvider->StringFromValue( size_x, true )
                                                           : INDETERMINATE_STATE );
        m_grid->SetCellValue( row, COL_SIZE_Y, size_y >= 0 ? m_unitsProvider->StringFromValue( size_y, true )
                                                           : INDETERMINATE_STATE );

        // Drill values (only meaningful for PTH or NPTH). Leave empty otherwise.
        if( pad->GetAttribute() == PAD_ATTRIB::PTH || pad->GetAttribute() == PAD_ATTRIB::NPTH )
        {
            VECTOR2I drill = pad->GetDrillSize();

            if( drill.x > 0 )
                m_grid->SetCellValue( row, COL_DRILL_X, m_unitsProvider->StringFromValue( drill.x, true ) );

            if( drill.y > 0 )
                m_grid->SetCellValue( row, COL_DRILL_Y, m_unitsProvider->StringFromValue( drill.y, true ) );
        }
        else
        {
            // For non-PTH pads, drill columns are not applicable.
            m_grid->SetReadOnly( row, COL_DRILL_X, true );
            m_grid->SetReadOnly( row, COL_DRILL_Y, true );
        }

        // Pad to die metrics
        if( pad->GetPadToDieLength() )
            m_grid->SetUnitValue( row, COL_P2D_LENGTH, pad->GetPadToDieLength() );

        if( pad->GetPadToDieDelay() )
            m_grid->SetUnitValue( row, COL_P2D_DELAY, pad->GetPadToDieDelay() );

        setRowNullableEditors( row );

        row++;
    }

    // Auto size the data columns first to get reasonable initial widths
    m_grid->AutoSizeColumns();

    // Ensure the Shape column (index 1) is wide enough for the longest translated
    // shape text plus the dropdown arrow / padding. We compute a max text width
    // using a device context and add a platform neutral padding.
    {
        wxClientDC dc( m_grid );
        dc.SetFont( m_grid->GetFont() );

        wxArrayString shapeNames;
        shapeNames.push_back( PAD::ShowPadShape( PAD_SHAPE::CIRCLE ) );
        shapeNames.push_back( PAD::ShowPadShape( PAD_SHAPE::OVAL ) );
        shapeNames.push_back( PAD::ShowPadShape( PAD_SHAPE::RECTANGLE ) );
        shapeNames.push_back( PAD::ShowPadShape( PAD_SHAPE::TRAPEZOID ) );
        shapeNames.push_back( PAD::ShowPadShape( PAD_SHAPE::ROUNDRECT ) );
        shapeNames.push_back( PAD::ShowPadShape( PAD_SHAPE::CHAMFERED_RECT ) );
        shapeNames.push_back( PAD::ShowPadShape( PAD_SHAPE::CUSTOM ) );

        int maxWidth = 0;

        for( const wxString& str : shapeNames )
        {
            int w, h;
            dc.GetTextExtent( str, &w, &h );
            maxWidth = std::max( maxWidth, w );
        }

        // Add padding for internal cell margins + dropdown control.
        int padding = FromDIP( 30 ); // heuristic: 2*margin + arrow button
        m_grid->SetColSize( COL_SHAPE, maxWidth + padding );
    }

    // Record initial proportions for proportional resizing later.
    InitColumnProportions();

    // Run an initial proportional resize using current client size so columns
    // respect proportions immediately.
    wxSizeEvent sizeEvt( GetSize(), GetId() );
    CallAfter(
            [this, sizeEvt]
            {
                wxSizeEvent evt( sizeEvt );
                this->OnSize( evt );
            } );

    // If pads exist, select the first row to show initial highlight
    if( m_grid->GetNumberRows() > 0 )
    {
        m_grid->SetGridCursor( 0, 0 );

        // Construct event with required parameters (id, type, obj, row, col,...)
        wxGridEvent ev( m_grid->GetId(), wxEVT_GRID_SELECT_CELL, m_grid, 0, 0, -1, -1, true );
        OnSelectCell( ev );
    }

    return true;
}


void DIALOG_FP_EDIT_PAD_TABLE::setRowNullableEditors( int aRowId ) const
{
    // Set nullable editors
    auto setCellEditor =
            [this, aRowId]( int aCol )
            {
                GRID_CELL_MARK_AS_NULLABLE* cellEditor = new GRID_CELL_MARK_AS_NULLABLE( true );
                wxGridCellAttr*             attr = m_grid->GetOrCreateCellAttr( aRowId, aCol );
                attr->SetEditor( cellEditor );
                attr->DecRef();
            };

    setCellEditor( COL_P2D_LENGTH );
    setCellEditor( COL_P2D_DELAY );
}


void DIALOG_FP_EDIT_PAD_TABLE::CaptureOriginalPadState()
{
    m_originalPads.clear();

    if( !m_footprint )
        return;

    for( PAD* pad : m_footprint->Pads() )
    {
        PAD_SNAPSHOT snap( pad );
        snap.number        = pad->GetNumber();
        snap.position      = pad->GetPosition();
        snap.padstack      = pad->Padstack();
        snap.attribute     = pad->GetAttribute();
        snap.padToDieLength= pad->GetPadToDieLength();
        snap.padToDieDelay = pad->GetPadToDieDelay();

        m_originalPads.try_emplace( pad, std::move( snap ) );
    }
}


void DIALOG_FP_EDIT_PAD_TABLE::RestoreOriginalPadState()
{
    if( !m_footprint )
        return;

    const PCB_BASE_FRAME* base = dynamic_cast<PCB_BASE_FRAME*>( GetParent() );
    PCB_DRAW_PANEL_GAL* canvas = base ? base->GetCanvas() : nullptr;

    for( PAD* pad : m_footprint->Pads() )
    {
        if( !m_originalPads.contains( pad ) )
            continue;

        const PAD_SNAPSHOT& snap = m_originalPads.at( pad );
        pad->SetNumber( snap.number );
        pad->SetPosition( snap.position );
        pad->SetPadstack( snap.padstack );
        pad->SetAttribute( snap.attribute );
        pad->SetPadToDieLength( snap.padToDieLength );
        pad->SetPadToDieDelay( snap.padToDieDelay );
        pad->ClearBrightened();

        if( canvas )
            canvas->GetView()->Update( pad, KIGFX::REPAINT );
    }

    if( canvas )
    {
        canvas->GetView()->MarkTargetDirty( KIGFX::TARGET_OVERLAY );
        canvas->ForceRefresh();
    }

    m_summaryDirty = true;
}


bool DIALOG_FP_EDIT_PAD_TABLE::TransferDataFromWindow()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    if( !m_footprint )
        return true;

    RestoreOriginalPadState();
    BOARD_COMMIT commit( m_frame );

    int row = 0;

    for( PAD* pad : m_originalPads | std::views::keys )
    {
        commit.Modify( pad );
        pad->SetNumber( m_grid->GetCellValue( row, COL_NUMBER ) );

        wxString typeStr = m_grid->GetCellValue( row, COL_TYPE );

        if( typeStr == _( "Through-hole" ) )
            pad->SetAttribute( PAD_ATTRIB::PTH );
        else if( typeStr == _( "SMD" ) )
            pad->SetAttribute( PAD_ATTRIB::SMD );
        else if( typeStr == _( "Connector" ) )
            pad->SetAttribute( PAD_ATTRIB::CONN );
        else if( typeStr == _( "NPTH" ) )
            pad->SetAttribute( PAD_ATTRIB::NPTH );
        // Aperture derived by copper-less layers; do not overwrite attribute here.

        wxString shape = m_grid->GetCellValue( row, COL_SHAPE );

        if( shape != INDETERMINATE_STATE )
        {
            PAD_SHAPE newShape = ShapeFromString( shape );

            pad->Padstack().ForEachUniqueLayer(
                    [&]( PCB_LAYER_ID aLayer )
                    {
                        pad->SetShape( aLayer, newShape );
                    } );
        }

        VECTOR2I pos;
        pos.x = m_grid->GetUnitValue( row, COL_POS_X );
        pos.y = m_grid->GetUnitValue( row, COL_POS_Y );
        pad->SetPosition( pos );

        wxString size_x_value = m_grid->GetCellValue( row, COL_SIZE_X );

        if( size_x_value != INDETERMINATE_STATE )
        {
            int size_x = m_grid->GetUnitValue( row, COL_SIZE_X );

            pad->Padstack().ForEachUniqueLayer(
                    [&]( PCB_LAYER_ID aLayer )
                    {
                        VECTOR2I size( size_x, pad->GetSize( aLayer ).y );
                        pad->SetSize( aLayer, size );
                    } );
        }

        wxString size_y_value = m_grid->GetCellValue( row, COL_SIZE_Y );

        if( size_y_value != INDETERMINATE_STATE )
        {
            int size_y = m_grid->GetUnitValue( row, COL_SIZE_Y );

            pad->Padstack().ForEachUniqueLayer(
                    [&]( PCB_LAYER_ID aLayer )
                    {
                        VECTOR2I size( pad->GetSize( aLayer ).x, size_y );
                        pad->SetSize( aLayer, size );
                    } );
        }

        // Drill sizes (only if attribute allows)
        if( pad->GetAttribute() == PAD_ATTRIB::PTH || pad->GetAttribute() == PAD_ATTRIB::NPTH )
        {
            int drill_x = m_grid->GetUnitValue( row, COL_DRILL_X );
            int drill_y = m_grid->GetUnitValue( row, COL_DRILL_Y );

            if( drill_x > 0 || drill_y > 0 )
            {
                if( drill_x <= 0 )
                    drill_x = drill_y;

                if( drill_y <= 0 )
                    drill_y = drill_x;

                pad->SetDrillSize( { drill_x, drill_y } );
            }
        }

        // Pad->Die
        const wxString delayStr = m_grid->GetCellValue( row, COL_P2D_DELAY );
        const wxString lenStr = m_grid->GetCellValue( row, COL_P2D_LENGTH );

        if( !lenStr.IsEmpty() )
            pad->SetPadToDieLength( m_grid->GetUnitValue( row, COL_P2D_LENGTH ) );
        else
            pad->SetPadToDieLength( 0 );

        if( !delayStr.IsEmpty() )
            pad->SetPadToDieDelay( m_grid->GetUnitValue( row, COL_P2D_DELAY ) );
        else
            pad->SetPadToDieDelay( 0 );

        row++;
    }

    commit.Push( _( "Edit Pads" ) );
    m_frame->Refresh();

    return true;
}


void DIALOG_FP_EDIT_PAD_TABLE::InitColumnProportions()
{
    m_colProportions.clear();
    m_minColWidths.clear();

    if( !m_grid )
        return;

    // Only consider the actual data columns (all of them since row labels are hidden)
    int cols = m_grid->GetNumberCols();
    int total = 0;
    std::vector<int> widths;
    widths.reserve( cols );

    for( int c = 0; c < cols; ++c )
    {
        int w = m_grid->GetColSize( c );
        widths.push_back( w );
        total += w;
    }

    if( total <= 0 )
        return;

    for( int w : widths )
    {
        m_colProportions.push_back( (double) w / (double) total );
        m_minColWidths.push_back( w );
    }
}


void DIALOG_FP_EDIT_PAD_TABLE::OnSize( wxSizeEvent& aEvent )
{
    if( m_colProportions.empty() )
    {
        aEvent.Skip();
        return;
    }

    // Compute available total width for columns and resize keeping proportions.
    int cols = m_grid->GetNumberCols();
    int available = 0;

    for( int c = 0; c < cols; ++c )
        available += m_grid->GetColSize( c );

    // Use client size of grid minus scrollbar estimate to better distribute.
    int clientW = m_grid->GetClientSize().x;

    if( clientW > 0 )
        available = clientW; // prefer actual client width

    int used = 0;

    for( int c = 0; c < cols; ++c )
    {
        int target = (int) std::round( m_colProportions[c] * available );
        target = std::max( target, m_minColWidths[c] );

        // Defer last column to absorb rounding diff.
        if( c == cols - 1 )
            target = std::max( available - used, m_minColWidths[c] );

        m_grid->SetColSize( c, target );
        used += target;
    }

    aEvent.Skip();
}


void DIALOG_FP_EDIT_PAD_TABLE::OnCharHook( wxKeyEvent& aEvent )
{
    if( m_grid->IsCellEditControlShown() && m_grid->GetGridCursorCol() == COL_NUMBER )
        m_summaryDirty = true;

    DIALOG_SHIM::OnCharHook( aEvent );
}


void DIALOG_FP_EDIT_PAD_TABLE::OnCellChanged( wxGridEvent& aEvent )
{
    int row = aEvent.GetRow();
    int col = aEvent.GetCol();

    if( !m_footprint )
        return;

    PAD* target = getPadForRow( row );

    if( !target )
        return;

    bool needCanvasRefresh = false;

    switch( col )
    {
    case COL_NUMBER:
        target->SetNumber( m_grid->GetCellValue( row, col ) );
        needCanvasRefresh = true;
        m_summaryDirty = true;
        break;

    case COL_TYPE:
    {
        wxString typeStr = m_grid->GetCellValue( row, col );
        PAD_ATTRIB newAttr = target->GetAttribute();

        if( typeStr == _( "Through-hole" ) )
            newAttr = PAD_ATTRIB::PTH;
        else if( typeStr == _( "SMD" ) )
            newAttr = PAD_ATTRIB::SMD;
        else if( typeStr == _( "Connector" ) )
            newAttr = PAD_ATTRIB::CONN;
        else if( typeStr == _( "NPTH" ) )
            newAttr = PAD_ATTRIB::NPTH;

        if( newAttr != target->GetAttribute() )
        {
            target->SetAttribute( newAttr );

            // Toggle drill columns read-only state dynamically.
            bool drillsEditable = ( newAttr == PAD_ATTRIB::PTH || newAttr == PAD_ATTRIB::NPTH );
            m_grid->SetReadOnly( row, COL_DRILL_X, !drillsEditable );
            m_grid->SetReadOnly( row, COL_DRILL_Y, !drillsEditable );
            needCanvasRefresh = true;
        }

        break;
    }

    case COL_SHAPE:
        target->SetShape( PADSTACK::ALL_LAYERS, ShapeFromString( m_grid->GetCellValue( row, col ) ) );
        needCanvasRefresh = true;
        break;

    case COL_POS_X:
    case COL_POS_Y:
    {
        VECTOR2I pos = target->GetPosition();

        if( col == COL_POS_X )
            pos.x = m_grid->GetUnitValue( row, col );
        else
            pos.y = m_grid->GetUnitValue( row, col );

        target->SetPosition( pos );
        needCanvasRefresh = true;
        break;
    }

    case COL_SIZE_X:
    case COL_SIZE_Y:
    {
        VECTOR2I size = target->GetSize( PADSTACK::ALL_LAYERS );

        if( col == COL_SIZE_X )
            size.x = m_grid->GetUnitValue( row, col );
        else
            size.y = m_grid->GetUnitValue( row, col );

        target->SetSize( PADSTACK::ALL_LAYERS, size );
        needCanvasRefresh = true;
        break;
    }

    case COL_DRILL_X:
    case COL_DRILL_Y:
    {
        if( target->GetAttribute() == PAD_ATTRIB::PTH || target->GetAttribute() == PAD_ATTRIB::NPTH )
        {
            int dx = m_grid->GetUnitValue( row, COL_DRILL_X );
            int dy = m_grid->GetUnitValue( row, COL_DRILL_Y );

            if( dx > 0 || dy > 0 )
            {
                if( dx <= 0 )
                    dx = dy;

                if( dy <= 0 )
                    dy = dx;

                target->SetDrillSize( { dx, dy } );
                needCanvasRefresh = true;
            }
        }

        break;
    }

    case COL_P2D_LENGTH:
        if( !m_grid->GetCellValue( row, col ).IsEmpty() )
            target->SetPadToDieLength( m_grid->GetUnitValue( row, col ) );

        break;

    case COL_P2D_DELAY:
        if( !m_grid->GetCellValue( row, col ).IsEmpty() )
            target->SetPadToDieDelay( m_grid->GetUnitValue( row, col ) );

        break;

    default:
        break;
    }

    // Request redraw (simple approach)
    target->SetDirty();

    if( needCanvasRefresh )
    {
        if( PCB_BASE_FRAME* base = dynamic_cast<PCB_BASE_FRAME*>( GetParent() ) )
            base->GetCanvas()->ForceRefresh();
    }
}


void DIALOG_FP_EDIT_PAD_TABLE::OnSelectCell( wxGridEvent& aEvent )
{
    int row = aEvent.GetRow();

    if( !m_footprint )
        return;

    PCB_BASE_FRAME* base = dynamic_cast<PCB_BASE_FRAME*>( GetParent() );
    PCB_DRAW_PANEL_GAL* canvas = base ? base->GetCanvas() : nullptr;

    // Clear existing pad selections
    for( PAD* pad : m_footprint->Pads() )
    {
        if( pad->IsBrightened() )
        {
            pad->ClearBrightened();

            if( canvas )
                canvas->GetView()->Update( pad, KIGFX::REPAINT );
        }
    }

    PAD* pad = getPadForRow( row );

    if( !pad )
        return;

    pad->SetBrightened();

    if( canvas )
    {
        canvas->GetView()->Update( pad, KIGFX::REPAINT );
        canvas->ForceRefresh();
    }
}


void DIALOG_FP_EDIT_PAD_TABLE::OnUpdateUI( wxUpdateUIEvent& aEvent )
{
    if( m_summaryDirty )
    {
        if( m_grid->IsCellEditControlShown() && m_grid->GetGridCursorCol() == COL_NUMBER )
        {
            int  row = m_grid->GetGridCursorRow();
            int  col = m_grid->GetGridCursorCol();

            PAD* target = getPadForRow( row );

            if( !target )
                return;

            wxGridCellEditor* editor = m_grid->GetCellEditor( row, col );

            if( editor )
            {
                target->SetNumber( editor->GetValue() );
                editor->DecRef();
            }
        }

        updateSummary();
        m_summaryDirty = false;
    }
}


void DIALOG_FP_EDIT_PAD_TABLE::OnCancel( wxCommandEvent& aEvent )
{
    m_cancelled = true;
    aEvent.Skip();
}


void DIALOG_FP_EDIT_PAD_TABLE::updateSummary()
{
    PIN_NUMBERS pinNumbers;

    for( PAD* pad : m_footprint->Pads() )
    {
        if( pad->GetNumber().Length() )
            pinNumbers.insert( pad->GetNumber() );
    }

    const wxString summary = pinNumbers.GetSummary();
    const wxString duplicates = pinNumbers.GetDuplicates();

    m_pin_numbers_summary->SetLabel( summary );
    m_pin_numbers_summary->SetToolTip( summary );
    m_pin_count->SetLabel( wxString::Format( wxT( "%u" ), (unsigned) m_footprint->Pads().size() ) );
    m_duplicate_pins->SetLabel( duplicates );
    m_duplicate_pins->SetToolTip( duplicates );

    Layout();
}


PAD* DIALOG_FP_EDIT_PAD_TABLE::getPadForRow( const int aRowId ) const
{
    if( static_cast<size_t>( aRowId ) >= m_originalPads.size() )
        return nullptr;

    const auto targetItr = std::next( m_originalPads.begin(), aRowId );
    return targetItr->first;
}

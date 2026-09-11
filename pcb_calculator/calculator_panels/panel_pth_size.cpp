/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <calculator_panels/panel_pth_size.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include <wx/clipbrd.h>
#include <wx/dcmemory.h>
#include <wx/image.h>
#include <wx/log.h>

#include <base_units.h>
#include <bitmaps.h>
#include <math/util.h>
#include <pcb_calculator_settings.h>
#include <pcb_calculator_utils.h>
#include <pth_hole_size.h>
#include <render_utils.h>


namespace
{
// Indexes of the items in the FBP shape and rounding choice controls.  The order is fixed by
// the FBP, so keep these in sync with the choice item lists.
enum class SHAPE_CHOICE
{
    ROUND,
    SQUARE,
    RECTANGULAR
};

enum class ROUNDING_CHOICE
{
    UP,
    NEAREST
};

// Read a control as a size in mm and convert it to pcb IU.  Returns false if the text is not a
// finite number.
bool readMm( const wxTextCtrl* aCtrl, int& aIU )
{
    const double mm = DoubleFromString( aCtrl->GetValue() );

    if( !std::isfinite( mm ) )
        return false;

    aIU = pcbIUScale.mmToIU( mm );
    return true;
}

// Format an IU value as a millimetre string.
wxString formatMm( int aIU )
{
    return wxString::Format( wxT( "%.3g" ), pcbIUScale.IUTomm( aIU ) );
}

struct PTH_PREVIEW_OPTIONS
{
    bool     m_showMinLeadSize = false;
    bool     m_showMinHoleSize = false;
    bool     m_showMaxHoleSize = false;
    wxColour m_backgroundColour; // Background the pad is drawn against for alpha blending
};

/*
 * Draw a sketch of a plated through hole pad per the given sizes,
 * with the configured tolerance overlays.
 */
void drawPthSizePreview( wxBitmap& aBitmap, const PTH_LEAD_DEF& aLead, int aHoleIU, int aHoleMinIU, int aHoleMaxIU,
                         int aPadIU, const PTH_PREVIEW_OPTIONS& aOptions )
{
    wxMemoryDC dc( aBitmap );

    dc.SetBackground( aOptions.m_backgroundColour );
    dc.Clear();

    const wxColour copperColor( 0xfc, 0xb2, 0x3c );
    const wxColour padOutlineColor( 0x80, 0x80, 0x80 );
    const wxColour leadColor( 0xc8, 0xc8, 0xc8 );
    const wxColour minLeadColor( 0x80, 0x80, 0x80 );
    const wxColour minMaxHoleColor( 0x00, 0x60, 0xf0 );

    if( aPadIU > 0 )
    {
        const int    size = std::min( aBitmap.GetWidth(), aBitmap.GetHeight() );
        const double maxFeatureIU = std::max( aPadIU, aHoleMaxIU );

        const int outlineWidth = 1; // px

        const double  scale = static_cast<double>( size - 2 * outlineWidth ) / maxFeatureIU;
        const wxPoint centre( aBitmap.GetWidth() / 2, aBitmap.GetHeight() / 2 );

        const auto toPx = [scale]( int aIU ) -> int
        {
            return KiROUND( aIU * scale );
        };

        const int padRadius = toPx( aPadIU ) / 2;
        const int holeRadius = toPx( aHoleIU ) / 2;

        // Copper pad with the drilled hole cut out.
        dc.SetBrush( wxBrush( copperColor ) );
        dc.SetPen( wxPen( copperColor ) );
        dc.DrawCircle( centre, padRadius );

        dc.SetBrush( wxBrush( aOptions.m_backgroundColour ) );
        dc.SetPen( wxPen( aOptions.m_backgroundColour ) );
        dc.DrawCircle( centre, holeRadius );

        // Lead cross-section in the middle of the hole.
        const bool isRectangular = aLead.m_shape == PTH_LEAD_SHAPE::RECTANGULAR;
        const int  leadWidth = toPx( aLead.m_maxX );
        const int  leadHeight = toPx( isRectangular ? aLead.m_maxY : aLead.m_maxX );

        dc.SetBrush( wxBrush( leadColor ) );
        dc.SetPen( wxPen( leadColor, outlineWidth ) );

        if( aLead.m_shape == PTH_LEAD_SHAPE::ROUND )
            dc.DrawCircle( centre, leadWidth / 2 );
        else
            dc.DrawRectangle( centre.x - leadWidth / 2, centre.y - leadHeight / 2, leadWidth, leadHeight );

        // Minimum lead cross-section, shown dashed to indicate the tolerance band.
        if( aOptions.m_showMinLeadSize )
        {
            const int minLeadWidth = toPx( aLead.m_minX );
            const int minLeadHeight = toPx( isRectangular ? aLead.m_minY : aLead.m_minX );

            dc.SetBrush( *wxTRANSPARENT_BRUSH );
            dc.SetPen( wxPen( minLeadColor, outlineWidth, wxPENSTYLE_SHORT_DASH ) );

            if( aLead.m_shape == PTH_LEAD_SHAPE::ROUND )
                dc.DrawCircle( centre, minLeadWidth / 2 );
            else
                dc.DrawRectangle( centre.x - minLeadWidth / 2, centre.y - minLeadHeight / 2, minLeadWidth,
                                  minLeadHeight );
        }

        dc.SetBrush( *wxTRANSPARENT_BRUSH );
        const wxPen outlinePen( padOutlineColor, outlineWidth );
        dc.SetPen( outlinePen );
        dc.DrawCircle( centre, padRadius );
        dc.DrawCircle( centre, holeRadius );

        // Allowable hole size range, shown as dashed circles.
        if( aOptions.m_showMinHoleSize )
        {
            const wxPen holeMinRangePen( minMaxHoleColor, outlineWidth, wxPENSTYLE_SHORT_DASH );
            dc.SetPen( holeMinRangePen );
            dc.DrawCircle( centre, toPx( aHoleMinIU ) / 2 );
        }

        if( aOptions.m_showMaxHoleSize )
        {
            const wxPen holeMaxRangePen( minMaxHoleColor, outlineWidth, wxPENSTYLE_LONG_DASH );
            dc.SetPen( holeMaxRangePen );
            dc.DrawCircle( centre, toPx( aHoleMaxIU ) / 2 );
        }
    }

    dc.SelectObject( wxNullBitmap );
}

// Render the preview to a bitmap with a transparent background, so the control's own themed
// background shows through the area round the pad and inside the drilled hole.  This means the
// preview follows theme changes without needing to be re-rendered.
wxBitmap makePthSizePreviewBitmap( const PTH_LEAD_DEF& aLead, int aHoleIU, int aHoleMinIU, int aHoleMaxIU, int aPadIU,
                                   bool aShowMinLeadSize, bool aShowMinHoleSize, bool aShowMaxHoleSize, int aPixelSize )
{
    // Render the pad once on white and once on black, then recover the per-pixel coverage alpha
    // from the two renders.  This gives properly feathered anti-aliased edges over the
    // transparent background without relying on a key colour.
    const auto render = [&]( const wxColour& aBackground )
    {
        wxBitmap bitmap( aPixelSize, aPixelSize );

        PTH_PREVIEW_OPTIONS options;
        options.m_showMinLeadSize = aShowMinLeadSize;
        options.m_showMinHoleSize = aShowMinHoleSize;
        options.m_showMaxHoleSize = aShowMaxHoleSize;
        options.m_backgroundColour = aBackground;

        drawPthSizePreview( bitmap, aLead, aHoleIU, aHoleMinIU, aHoleMaxIU, aPadIU, options );

        return bitmap.ConvertToImage();
    };

    const wxImage imageOnWhite = render( *wxWHITE );
    const wxImage imageOnBlack = render( *wxBLACK );

    const tl::expected<wxImage, std::string> alphaImage = CreateAlphaImageFromTwoRenders( imageOnWhite, imageOnBlack );

    if( !alphaImage )
    {
        wxLogError( "Failed to create alpha image for PTH size preview: %s", alphaImage.error() );
        return wxBitmap( imageOnWhite );
    }

    return wxBitmap( *alphaImage );
}

// Pad a string to a fixed character width so the report columns line up in a monospace font.
// Longer strings are left as-is.
wxString padRight( const wxString& aText, size_t aWidth )
{
    wxString text = aText;

    if( text.length() < aWidth )
        text += wxString( ' ', aWidth - text.length() );

    return text;
}

/*
 * Generate a text report of the hole size calculation, suitable for copying to the clipboard.
 */
wxString pthResultReportText( const PTH_HOLE_SIZE_STANDARD* aStandard, int aLevel, const PTH_LEAD_DEF& aLead,
                              int aAnnularIU, const PTH_HOLE_SIZE_RESULT& aRange, int aHoleIU, int aPadIU )
{
    // The report rows: label/value pairs for the table, or a single-cell row for a full-width
    // line (used for the blank separator between the inputs and the calculated sizes).  The
    // text is only rendered once all rows are known, so the label column can be sized to fit.
    std::vector<std::vector<wxString>> rows;

    wxString leadShapeText;
    wxString leadSizeText;

    switch( aLead.m_shape )
    {
    case PTH_LEAD_SHAPE::ROUND:
        leadShapeText = _( "Round" );
        leadSizeText = wxString::Format( _( "%s \u2013 %s mm" ), formatMm( aLead.m_minX ), formatMm( aLead.m_maxX ) );
        break;

    case PTH_LEAD_SHAPE::SQUARE:
        leadShapeText = _( "Square" );
        leadSizeText = wxString::Format( _( "%s \u2013 %s mm" ), formatMm( aLead.m_minX ), formatMm( aLead.m_maxX ) );
        break;

    case PTH_LEAD_SHAPE::RECTANGULAR:
        leadShapeText = _( "Rectangular" );
        leadSizeText = wxString::Format( _( "%s \u2013 %s mm x %s \u2013 %s mm" ), formatMm( aLead.m_minX ),
                                         formatMm( aLead.m_maxX ), formatMm( aLead.m_minY ), formatMm( aLead.m_maxY ) );
        break;
    }

    rows.push_back( { _( "Standard:" ), aStandard->GetDisplayName() } );
    rows.push_back( { _( "Production level:" ), aStandard->GetLevelName( aLevel ) } );
    rows.push_back( { _( "Lead shape:" ), leadShapeText } );
    rows.push_back( { _( "Lead size:" ), leadSizeText } );
    rows.push_back(
            { _( "Effective lead size:" ), wxString::Format( _( "%s \u2013 %s mm" ), formatMm( aRange.m_leadMin ),
                                                             formatMm( aRange.m_leadMax ) ) } );

    rows.push_back( { wxEmptyString } );

    rows.push_back( { _( "Minimum hole size:" ), wxString::Format( _( "%s mm" ), formatMm( aRange.m_holeMin ) ) } );
    rows.push_back( { _( "Maximum hole size:" ), wxString::Format( _( "%s mm" ), formatMm( aRange.m_holeMax ) ) } );
    rows.push_back( { _( "Recommended hole size:" ), wxString::Format( _( "%s mm" ), formatMm( aHoleIU ) ) } );
    rows.push_back( { _( "Hole clearance over max. lead:" ),
                      wxString::Format( _( "%s mm" ), formatMm( aHoleIU - aRange.m_leadMax ) ) } );
    rows.push_back( { _( "Annular ring:" ), wxString::Format( _( "%s mm" ), formatMm( aAnnularIU ) ) } );
    rows.push_back( { _( "Pad diameter:" ), wxString::Format( _( "%s mm" ), formatMm( aPadIU ) ) } );

    // Pad the labels to the widest one so all the values start on the same column.
    size_t labelWidth = 0;

    for( const auto& row : rows )
    {
        if( row.size() > 1 )
            labelWidth = std::max( labelWidth, row[0].length() );
    }

    wxString text;

    for( const auto& row : rows )
    {
        if( row.size() > 1 )
            text += padRight( row[0], labelWidth ) + wxT( " " ) + row[1] + wxT( "\n" );
        else
            text += row[0] + wxT( "\n" );
    }

    return text;
}

} // namespace


PANEL_PTH_SIZE::PANEL_PTH_SIZE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style,
                                const wxString& name ) :
        PANEL_PTH_SIZE_BASE( parent, id, pos, size, style, name )
{
    populateStandardChoice();

    updateStandard();

    m_shapeChoice->SetSelection( 0 );
    updateLeadShapeUI();

    // Default values (mm), a typical round component lead.
    m_sizeXMinCtrl->SetValue( wxT( "0.6" ) );
    m_sizeXMaxCtrl->SetValue( wxT( "0.7" ) );
    m_sizeYMinCtrl->SetValue( wxT( "0.25" ) );
    m_sizeYMaxCtrl->SetValue( wxT( "0.3" ) );
    m_annularCtrl->SetValue( wxT( "0.4" ) );
    m_sizeRoundingCtrl->SetValue( wxT( "0.05" ) );

    // Show the minimum lead size by default; the hole size range overlays are opt-in.
    m_cbShowMinLeadSize->SetValue( true );

    // The copy button puts the current report text on the clipboard.
    m_bpCopyAll->SetBitmap( KiBitmapBundle( BITMAPS::copy, 16 ) );
    m_bpCopyAll->SetToolTip( _( "Copy the report to the clipboard" ) );

    // Monospace so the columns of the report line up.
    m_ResultReport->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
                                     wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

    m_ResultReport->SetMinSize( wxSize( -1, ToPhys( 250 ) ) );

    recalculate();

    GetSizer()->SetSizeHints( this );
}


PANEL_PTH_SIZE::~PANEL_PTH_SIZE()
{
}


void PANEL_PTH_SIZE::LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    // No persisted settings yet; the panel starts from its default values.
}


void PANEL_PTH_SIZE::SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    // No persisted settings yet.
}


void PANEL_PTH_SIZE::ThemeChanged()
{
    // Bitmap icons are theme dependent (light/dark variants), so re-resolve the copy icon for
    // the new theme.
    m_bpCopyAll->SetBitmap( KiBitmapBundle( BITMAPS::copy ) );

    // The preview bitmap has a transparent background, so the control's themed background shows
    // through it; just make sure the control repaints for the new theme.
    m_bitmapPreview->Refresh();
}


void PANEL_PTH_SIZE::OnControlsChanged( wxCommandEvent& aEvent )
{
    if( aEvent.GetEventObject() == m_shapeChoice )
        updateLeadShapeUI();
    else if( aEvent.GetEventObject() == m_standardChoice )
        updateStandard();

    m_level = m_levelChoice->GetSelection();

    recalculate();
}


void PANEL_PTH_SIZE::OnValueChanged( wxCommandEvent& aEvent )
{
    recalculate();
}


void PANEL_PTH_SIZE::populateStandardChoice()
{
    m_standardChoice->Clear();

    for( const auto& standard : GetPthHoleSizeStandards() )
        m_standardChoice->Append( standard->GetDisplayName() );

    m_standardChoice->SetSelection( 0 );
}


void PANEL_PTH_SIZE::updateStandard()
{
    const int   selection = m_standardChoice->GetSelection();
    const auto& standards = GetPthHoleSizeStandards();

    if( selection >= 0 && selection < static_cast<int>( standards.size() ) )
    {
        m_holeSizeStandard = standards[selection];
        m_staticTextSummaryTitle->SetLabel(
                wxString::Format( _( "%s Summary" ), standards[selection]->GetDisplayName() ) );
    }

    if( !m_holeSizeStandard )
        return;

    m_levelChoice->Clear();

    for( int i = 0; i < m_holeSizeStandard->GetLevelCount(); ++i )
        m_levelChoice->Append( m_holeSizeStandard->GetLevelName( i ) );

    m_level = std::min( 1, m_holeSizeStandard->GetLevelCount() - 1 );
    m_levelChoice->SetSelection( m_level );

    updateSummaryTable();
}


void PANEL_PTH_SIZE::updateSummaryTable()
{
    if( !m_holeSizeStandard )
        return;

    // A bit of a hack because other standards may not work exectly like IPC-2222B,
    // but until we have more standards, just show the IPC-2222B summary table here and think
    // about how to generalize it later.
    wxGrid* const grid = m_ipc2222Summary;
    const int     levelCount = m_holeSizeStandard->GetLevelCount();

    // Keep one grid column per density level of the standard.
    const int colCount = grid->GetNumberCols();

    if( levelCount > colCount )
        grid->AppendCols( levelCount - colCount );
    else if( levelCount < colCount )
        grid->DeleteCols( levelCount, colCount - levelCount );

    for( int level = 0; level < levelCount; ++level )
    {
        grid->SetColLabelValue( level, m_holeSizeStandard->GetLevelName( level ) );

        // The smallest allowable hole is the largest lead size plus the level allowance, and the
        // largest allowable hole is the smallest lead size plus the level allowance.
        grid->SetCellValue( 0, level,
                            wxString::Format( _( "Maximum lead diameter + %smm" ),
                                              formatMm( m_holeSizeStandard->GetMinHoleAddend( level ) ) ) );

        grid->SetCellValue( 1, level,
                            wxString::Format( _( "Minimum lead diameter + %smm" ),
                                              formatMm( m_holeSizeStandard->GetMaxHoleAddend( level ) ) ) );
    }

    grid->AutoSizeColumns();
}


void PANEL_PTH_SIZE::updateLeadShapeUI()
{
    const SHAPE_CHOICE shape = static_cast<SHAPE_CHOICE>( m_shapeChoice->GetSelection() );
    const bool         showSizeY = shape == SHAPE_CHOICE::RECTANGULAR;

    switch( shape )
    {
    case SHAPE_CHOICE::ROUND:
        m_leadShape = PTH_LEAD_SHAPE::ROUND;
        m_staticTextSizeX->SetLabel( _( "Diameter:" ) );
        break;

    case SHAPE_CHOICE::SQUARE:
        m_leadShape = PTH_LEAD_SHAPE::SQUARE;
        m_staticTextSizeX->SetLabel( _( "Side length:" ) );
        break;

    default:
        m_leadShape = PTH_LEAD_SHAPE::RECTANGULAR;
        m_staticTextSizeX->SetLabel( _( "Size X:" ) );
        break;
    }

    m_staticTextSizeY->Show( showSizeY );
    m_sizeYMinCtrl->Show( showSizeY );
    m_sizeYMaxCtrl->Show( showSizeY );
    m_staticTextSizeYUnit->Show( showSizeY );

    Layout();
}


void PANEL_PTH_SIZE::recalculate()
{
    if( !m_holeSizeStandard || m_level < 0 )
    {
        m_ResultReport->SetValue( wxEmptyString );
        return;
    }

    PTH_LEAD_DEF lead;
    int          annularIU = 0;
    int          roundingStepIU = 0;

    lead.m_shape = m_leadShape;

    bool valid = true;

    valid &= readMm( m_sizeXMinCtrl, lead.m_minX );
    valid &= readMm( m_sizeXMaxCtrl, lead.m_maxX );
    valid &= lead.m_minX > 0 && lead.m_minX <= lead.m_maxX;

    if( m_leadShape == PTH_LEAD_SHAPE::RECTANGULAR )
    {
        valid &= readMm( m_sizeYMinCtrl, lead.m_minY );
        valid &= readMm( m_sizeYMaxCtrl, lead.m_maxY );
        valid &= lead.m_minY > 0 && lead.m_minY <= lead.m_maxY;
    }

    valid &= readMm( m_annularCtrl, annularIU ) && annularIU >= 0;
    valid &= readMm( m_sizeRoundingCtrl, roundingStepIU ) && roundingStepIU >= 0;

    // Don't leave stale results in the report when an input is incomplete or invalid.
    if( !valid )
    {
        m_ResultReport->SetValue( wxEmptyString );
        return;
    }

    const ROUNDING_CHOICE   roundingChoice = static_cast<ROUNDING_CHOICE>( m_choiceRounding->GetSelection() );
    const PTH_HOLE_ROUNDING rounding =
            roundingChoice == ROUNDING_CHOICE::UP ? PTH_HOLE_ROUNDING::UP : PTH_HOLE_ROUNDING::NEAREST;

    PTH_HOLE_SIZE_RESULT range = m_holeSizeStandard->ComputeHoleSize( m_level, lead );

    const int hole = ComputeRecommendedPthHoleSize( range, roundingStepIU, rounding );
    const int pad = hole + 2 * annularIU;

    m_lastLead = lead;
    m_lastRange = range;
    m_lastHoleIU = hole;
    m_lastPadIU = pad;

    m_ResultReport->SetValue( pthResultReportText( m_holeSizeStandard, m_level, lead, annularIU, range, hole, pad ) );

    m_holeSizeCtrl->SetValue( formatMm( hole ) );
    m_padSizeCtrl->SetValue( formatMm( pad ) );

    refreshPreview();

    Layout();
}


void PANEL_PTH_SIZE::refreshPreview()
{
    constexpr int PREVIEW_SIZE = 200;

    const int    size = ToPhys( PREVIEW_SIZE );
    const double scale = GetDPIScaleFactor();

    wxBitmap bitmap = makePthSizePreviewBitmap(
            m_lastLead, m_lastHoleIU, m_lastRange.m_holeMin, m_lastRange.m_holeMax, m_lastPadIU,
            m_cbShowMinLeadSize->GetValue(), m_cbShowMinHoleSize->GetValue(), m_cbShowMaxHoleSize->GetValue(), size );

    bitmap.SetScaleFactor( scale );

    m_bitmapPreview->SetBitmap( bitmap );
}


void PANEL_PTH_SIZE::OnPreviewSettingCb( wxCommandEvent& aEvent )
{
    refreshPreview();
}


void PANEL_PTH_SIZE::OnCopyReportText( wxCommandEvent& aEvent )
{
    if( wxTheClipboard->Open() )
    {
        wxTheClipboard->SetData( new wxTextDataObject( m_ResultReport->GetValue() ) );
        wxTheClipboard->Close();
    }
}

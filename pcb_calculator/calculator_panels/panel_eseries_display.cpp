/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <math.h>

#include <calculator_panels/panel_eseries_display.h>
#include <pcb_calculator_settings.h>
#include <string_utils.h>
#include <wx/msgdlg.h>
#include <wx/settings.h> // for IsDark
#include <eseries.h>

#include <i18n_utility.h> // For _HKI definition in eseries_display_help.h
wxString eseries_display_help =
#include "eseries_display_help.h"


PANEL_ESERIES_DISPLAY::PANEL_ESERIES_DISPLAY( wxWindow * parent, wxWindowID id,
                                              const wxPoint& pos, const wxSize& size,
                                              long style, const wxString& name ) :
        PANEL_ESERIES_DISPLAY_BASE( parent, id, pos, size, style, name )
{
    // show markdown stuff in lower help panel
    wxString msg;

    ConvertMarkdown2Html( wxGetTranslation( eseries_display_help ), msg );
    m_panelESeriesHelp->SetPage( msg );

    // Calculate E1,E3,E6,E12,E24,E48,E96 column background colours
    // Get appearances. Note that I believe these panels will not be
    // re-constructed on a mode change so this only works if you open
    // the window *after* you set your dark mode.
    const wxSystemAppearance appearances = wxSystemSettings::GetAppearance();
    const bool selectDark = appearances.IsDark();

    recalculateColumnColours( selectDark );

    // Lay out the smaller tree containing E1,E3,E6, and E12
    populateE112Tree();

    // Set colours of smaller tree.
    recolourE112Tree();

    // Lay out the larger tree containing E24, R48 and E96
    populateE2496Tree();

    // Set colours of larger tree.
    recolourE2496Tree();

    // Needed on wxWidgets 3.0 to ensure sizers are correctly set
    GetSizer()->SetSizeHints( this );

    // Make the grid lines disappear into the window background
    // making the value boxes appear to be separated from each other.
    // Also force text to always be black so it is visible against the light
    // colored cells in both light and dark modes.
    wxColour gridLineColour = parent ? parent->GetBackgroundColour() : wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );

    m_GridEseries112->SetDefaultCellTextColour( *wxBLACK );
    m_GridEseries112->SetGridLineColour( gridLineColour );
    m_GridEseries112->EnableGridLines( true );

    m_GridEseries112->SetColLabelSize( wxGRID_AUTOSIZE );
    m_GridEseries112->AutoSize();

    m_GridEseries2496->SetDefaultCellTextColour( *wxBLACK );
    m_GridEseries2496->SetGridLineColour( gridLineColour );
    m_GridEseries2496->EnableGridLines( true );

    m_GridEseries2496->SetColLabelSize( wxGRID_AUTOSIZE );
    m_GridEseries2496->AutoSize();

    Layout();
}


PANEL_ESERIES_DISPLAY::~PANEL_ESERIES_DISPLAY()
{
}


void PANEL_ESERIES_DISPLAY::ThemeChanged()
{
    // Calculate E1,E3,E6,E12,E24,E48,E96 column background colours
    // Get appearances.
    const wxSystemAppearance appearances = wxSystemSettings::GetAppearance();
    const bool selectDark = appearances.IsDark();

    recalculateColumnColours( selectDark );

    // Set colours of smaller tree.
    recolourE112Tree();

    // Set colours of larger tree.
    recolourE2496Tree();
}


void PANEL_ESERIES_DISPLAY::SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
}


void PANEL_ESERIES_DISPLAY::LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
}


void PANEL_ESERIES_DISPLAY::recalculateColumnColours( bool aDarkModeOn )
{
    // if in a dark mode then darken colors significantly so that
    // the white numerals on top stand out better
    const int colourAdjust = aDarkModeOn ? s_darkAdjustValue : 100;

    m_colourE1Column = wxColour( s_cE1BGR ).ChangeLightness( colourAdjust );
    const wxColour colourE3Column( wxColour( s_cE3BGR ).ChangeLightness( colourAdjust ) );
    const wxColour colourE6Column( wxColour( s_cE6BGR ).ChangeLightness( colourAdjust ) );
    const wxColour colourE12Column( wxColour( s_cE12BGR ).ChangeLightness( colourAdjust ) );
    const wxColour colourE24Column( wxColour( s_cE24BGR ).ChangeLightness( colourAdjust ) );
    const wxColour colourE48Column( wxColour( s_cE48BGR ).ChangeLightness( colourAdjust ) );
    const wxColour colourE96Column( wxColour( s_cE96BGR ).ChangeLightness( colourAdjust ) );

    m_colourE3Pair[0] = colourE3Column;
    m_colourE3Pair[1] = colourE3Column.ChangeLightness( s_altAdjustValue );
    m_colourE6Pair[0] = colourE6Column;
    m_colourE6Pair[1] = colourE6Column.ChangeLightness( s_altAdjustValue );
    m_colourE12Pair[0] = colourE12Column;
    m_colourE12Pair[1] = colourE12Column.ChangeLightness( s_altAdjustValue );
    m_colourE24Pair[0] = colourE24Column;
    m_colourE24Pair[1] = colourE24Column.ChangeLightness( s_altAdjustValue );
    m_colourE48Pair[0] = colourE48Column;
    m_colourE48Pair[1] = colourE48Column.ChangeLightness( s_altAdjustValue );
    m_colourE96Pair[0] = colourE96Column;
    m_colourE96Pair[1] = colourE96Column.ChangeLightness( s_altAdjustValue );

    s_colourMatching = m_GridEseries2496->GetLabelBackgroundColour();
}


void PANEL_ESERIES_DISPLAY::populateE112Tree()
{
    // Lay out E1, R3, E6, E12 tree.
    ESERIES::ESERIES_VALUES eSeries12 = ESERIES::ESERIES_VALUES( ESERIES::E12 );

    int      row = 0;
    wxString value;

    m_GridEseries112->BeginBatch();

    m_GridEseries112->DeleteCols( 0, m_GridEseries112->GetNumberCols() );
    m_GridEseries112->DeleteRows( 0, m_GridEseries112->GetNumberRows() );
    m_GridEseries112->AppendCols( 4 );  // E1, E3, E6, E12
    m_GridEseries112->AppendRows( 12 ); // Sufficient rows for all of E12

    m_GridEseries112->SetColLabelValue( 0, "E1" );
    m_GridEseries112->SetColLabelValue( 1, "E3" );
    m_GridEseries112->SetColLabelValue( 2, "E6" );
    m_GridEseries112->SetColLabelValue( 3, "E12" );

    value = wxString( "" ) << eSeries12[0];

    m_GridEseries112->SetCellValue( 0, 0, value );
    m_GridEseries112->SetCellSize( 0, 0, 12, 1 );

    for( const uint16_t& seriesEntry : eSeries12 )
    {
        value = wxString( "" ) << seriesEntry;

        if( 0 == row % 4 )
        {
            // Set E3 column
            m_GridEseries112->SetCellValue( row, 1, value );
            m_GridEseries112->SetCellSize( row, 1, 4, 1 );
        }

        if( 0 == row % 2 )
        {
            // Set E6 column
            m_GridEseries112->SetCellValue( row, 2, value );
            m_GridEseries112->SetCellSize( row, 2, 2, 1 );
        }

        // Set E12 column
        m_GridEseries112->SetCellValue( row, 3, value );

        ++row;
    }

    for( int i = 0; i < 3; ++i )
    {
        // Resize data columns to fit data and set as minimum size
        m_GridEseries112->AutoSizeColumn( i, true );
    }

    m_GridEseries112->EndBatch();
}


void PANEL_ESERIES_DISPLAY::populateE2496Tree()
{
    // Lay out E24, E48, E96 tree.
    ESERIES::ESERIES_VALUES eSeries24 = ESERIES::ESERIES_VALUES( ESERIES::E24 );
    ESERIES::ESERIES_VALUES eSeries96 = ESERIES::ESERIES_VALUES( ESERIES::E96 );

    // This is the number of times the table is wrapped around into another
    // column. Like in a newspaper. The term column is not used since
    // the grid already has columns. The 96 in the second calculation
    // is the number of entries in the largest series displayed (E96)
    constexpr unsigned int numStripes = 4;
    constexpr unsigned int stripeHeight = 96 / numStripes;

    int      idx = 0;
    int      stripe = 0;
    wxString value;

    m_GridEseries2496->BeginBatch();

    m_GridEseries2496->DeleteCols( 0, m_GridEseries2496->GetNumberCols() );
    m_GridEseries2496->DeleteRows( 0, m_GridEseries2496->GetNumberRows() );

    // Column headers are E24, E48, E96, -
    // repeated numStripes times over. And the last - is omitted.
    m_GridEseries2496->AppendCols( numStripes * 4 - 1 );
    m_GridEseries2496->AppendRows( stripeHeight );

    m_GridEseries2496->SetColLabelValue( 0 + 0, "E24" );
    m_GridEseries2496->SetColLabelValue( 1 + 0, "E48" );
    m_GridEseries2496->SetColLabelValue( 2 + 0, "E96" );
    m_GridEseries2496->SetColLabelValue( 3 + 0, "-" );

    m_GridEseries2496->SetColLabelValue( 0 + 4, "E24" );
    m_GridEseries2496->SetColLabelValue( 1 + 4, "E48" );
    m_GridEseries2496->SetColLabelValue( 2 + 4, "E96" );
    m_GridEseries2496->SetColLabelValue( 3 + 4, "-" );

    m_GridEseries2496->SetColLabelValue( 0 + 8, "E24" );
    m_GridEseries2496->SetColLabelValue( 1 + 8, "E48" );
    m_GridEseries2496->SetColLabelValue( 2 + 8, "E96" );
    m_GridEseries2496->SetColLabelValue( 3 + 8, "-" );

    m_GridEseries2496->SetColLabelValue( 0 + 12, "E24" );
    m_GridEseries2496->SetColLabelValue( 1 + 12, "E48" );
    m_GridEseries2496->SetColLabelValue( 2 + 12, "E96" );

    for( const uint16_t& seriesEntry : eSeries24 )
    {
        value = wxString( "" ) << seriesEntry;

        int row = ( idx * 4 ) % stripeHeight;
        int colOffset = ( ( idx * 4 ) / stripeHeight ) * 4;

        m_GridEseries2496->SetCellValue( row, colOffset, value );
        m_GridEseries2496->SetCellSize( row, colOffset, 4, 1 );

        ++idx;
    }

    idx = 0;

    for( const uint16_t& seriesEntry : eSeries96 )
    {
        value = wxString( "" ) << seriesEntry;

        int row = idx % stripeHeight;
        int colOffset = ( idx / stripeHeight ) * 4;

        if( 0 == row % 2 )
        {
            // Set E48 column
            m_GridEseries2496->SetCellValue( row, colOffset + 1, value );
            m_GridEseries2496->SetCellSize( row, colOffset + 1, 2, 1 );
        }

        // Set E96 column
        m_GridEseries2496->SetCellValue( row, colOffset + 2, value );
        ++idx;
    }

    // cause the areas between stripes to appear more empty
    // by making them a single cell the height of the stripe
    // this causes the horizontal row lines rows to disappear
    for( unsigned int stripeGap = 1; stripeGap < numStripes; ++stripeGap )
    {
        const int stripeColumn = -1 + stripeGap * 4;

        m_GridEseries2496->SetCellSize( 0, stripeColumn, stripeHeight, 1 );
    }

    for( int i = 0; i < numStripes * 4 - 1; ++i )
    {
        // Resize the column to fit data and set as minimum size
        m_GridEseries2496->AutoSizeColumn( i, true );
    }

    m_GridEseries2496->EndBatch();
}


void PANEL_ESERIES_DISPLAY::recolourE112Tree()
{
    const unsigned int numRows = m_GridEseries112->GetNumberRows();

    m_GridEseries112->BeginBatch();

    // Colouring E1 is easy. There is only one item.
    m_GridEseries112->SetCellBackgroundColour( 0, 0, m_colourE1Column );

    bool alternateColour = false;
    int  cellWidth = 0, cellHeight = 0;

    // Colour E3 column
    for( int row = 0; row < numRows; row += cellHeight )
    {
        m_GridEseries112->SetCellBackgroundColour( row, 1, m_colourE3Pair[alternateColour] );

        // get height for iteration above
        m_GridEseries112->GetCellSize( row, 1, &cellHeight, &cellWidth );
        alternateColour = !alternateColour;

        if( cellHeight < 1 )
            cellHeight = 1; // Do not loop forever, always make progress.
    }

    // Colour E6 column
    alternateColour = false;

    for( int row = 0; row < numRows; row += cellHeight )
    {
        m_GridEseries112->SetCellBackgroundColour( row, 2, m_colourE6Pair[alternateColour] );

        // get height for iteration above
        m_GridEseries112->GetCellSize( row, 2, &cellHeight, &cellWidth );
        alternateColour = !alternateColour;

        if( cellHeight < 1 )
            cellHeight = 1; // Do not loop forever, always make progress.
    }

    // Colour E12 column
    alternateColour = false;

    for( int row = 0; row < numRows; row += cellHeight )
    {
        m_GridEseries112->SetCellBackgroundColour( row, 3, m_colourE12Pair[alternateColour] );

        // get height for iteration above
        m_GridEseries112->GetCellSize( row, 3, &cellHeight, &cellWidth );
        alternateColour = !alternateColour;

        if( cellHeight < 1 )
            cellHeight = 1; // Do not loop forever, always make progress.
    }

    m_GridEseries112->EndBatch();
}


void PANEL_ESERIES_DISPLAY::recolourE2496Tree()
{
    const unsigned int     numRows = m_GridEseries2496->GetNumberRows();
    constexpr unsigned int numStripes = 4;
    constexpr unsigned int stripeHeight = 96 / numStripes;

    m_GridEseries2496->BeginBatch();

    bool alternateColour = false;
    int  cellWidth = 0, cellHeight = 0;

    // Colour E24 columns
    for( int row = 0; row < numRows; row += cellHeight )
    {
        for( int stripe = 0; stripe < numStripes; ++stripe )
        {
            m_GridEseries2496->SetCellBackgroundColour( row, 0 + 4 * stripe,
                                                        m_colourE24Pair[alternateColour] );
        }
        // get height for iteration above
        m_GridEseries2496->GetCellSize( row, 0, &cellHeight, &cellWidth );
        alternateColour = !alternateColour;

        if( cellHeight < 1 )
            cellHeight = 1; // Do not loop forever, always make progress.
    }

    // Colour E48 columns
    alternateColour = false;

    for( int row = 0; row < numRows; row += cellHeight )
    {
        for( int stripe = 0; stripe < numStripes; ++stripe )
        {
            m_GridEseries2496->SetCellBackgroundColour( row, 1 + 4 * stripe,
                                                        m_colourE48Pair[alternateColour] );
        }
        // get height for iteration above
        m_GridEseries2496->GetCellSize( row, 1, &cellHeight, &cellWidth );
        alternateColour = !alternateColour;

        if( cellHeight < 1 )
            cellHeight = 1; // Do not loop forever, always make progress.
    }

    // Colour E96 columns
    alternateColour = false;

    for( int row = 0; row < numRows; row += cellHeight )
    {
        for( int stripe = 0; stripe < numStripes; ++stripe )
        {
            m_GridEseries2496->SetCellBackgroundColour( row, 2 + 4 * stripe,
                                                        m_colourE96Pair[alternateColour] );
        }
        // get height for iteration above
        m_GridEseries2496->GetCellSize( row, 2, &cellHeight, &cellWidth );
        alternateColour = !alternateColour;

        if( cellHeight < 1 )
            cellHeight = 1; // Do not loop forever, always make progress.
    }

    // recolour empty areas between columns to match label background
    // to make them less eye-catching
    for( unsigned int stripeGap = 1; stripeGap < numStripes; ++stripeGap )
    {
        const int stripeColumn = -1 + stripeGap * 4;

        m_GridEseries2496->SetCellBackgroundColour( 0, stripeColumn, s_colourMatching );
    }

    m_GridEseries2496->EndBatch();
}

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
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

#include <calculator_panels/panel_board_class.h>
#include <pcb_calculator_settings.h>
#include <widgets/unit_selector.h>
#include <units_scales.h>

PANEL_BOARD_CLASS::PANEL_BOARD_CLASS( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                                      const wxSize& size, long style, const wxString& name ) :
        PANEL_BOARD_CLASS_BASE( parent, id, pos, size, style, name )
{
    // Autosize the row label column to be sure label are not truncated
    m_gridClassesValuesDisplay->SetRowLabelSize( wxGRID_AUTOSIZE );

    // Needed on wxWidgets 3.0 to ensure sizers are correctly set
    GetSizer()->SetSizeHints( this );
}


PANEL_BOARD_CLASS::~PANEL_BOARD_CLASS()
{
}


void PANEL_BOARD_CLASS::ThemeChanged()
{
    // TODO: Only the grid needs updating, but it isn't done automatically (https://trac.wxwidgets.org/ticket/19279)
}


void PANEL_BOARD_CLASS::SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    aCfg->m_BoardClassUnits = m_BoardClassesUnitsSelector->GetSelection();
}


void PANEL_BOARD_CLASS::LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    m_BoardClassesUnitsSelector->SetSelection( aCfg->m_BoardClassUnits );
    BoardClassesUpdateData( m_BoardClassesUnitsSelector->GetUnitScale() );
}


// A helper class to handle min values
// Values are in meters.
// Note : use -1.0 when a value is irrelevant in a class
class BOARD_MIN_SIZE_VALUES
{
public:
    int    m_Class;                 // Class Id
    double m_Lines;                 // min copper lines width
    double m_Clearance;             // min dist between copper lines
    double m_ViaDiamDiff;           // Min value for diff between Via diameter
                                    // and its hole diameter
    double m_PadDiamDiffPlated;     // Min value for diff between Pad diameter
                                    // and its hole diameter (plated)
    double m_PadDiamDiffNotPlated;  // Min value for diff between Pad diameter
                                    // and its hole diameter (not plated)
public:
    BOARD_MIN_SIZE_VALUES( int aClass, double aLines, double aClearance, double aViaDiffPlated,
                           double aPadDiffPlated , double aPadDiffNotPlated ) :
            m_Class( aClass ),
            m_Lines( aLines ),
            m_Clearance( aClearance ),
            m_ViaDiamDiff( aViaDiffPlated ),
            m_PadDiamDiffPlated( aPadDiffPlated ),
            m_PadDiamDiffNotPlated( aPadDiffNotPlated )
    {
    }
};

#define BRDCLASS_COUNT 6
static BOARD_MIN_SIZE_VALUES clist[BRDCLASS_COUNT] =
{
    // class 1
    BOARD_MIN_SIZE_VALUES(1, 0.80*UNIT_MM, 0.68*UNIT_MM,
            -1.0,
             1.19*UNIT_MM, 1.57*UNIT_MM ),
    // class 2
    BOARD_MIN_SIZE_VALUES(1, 0.50*UNIT_MM, 0.50*UNIT_MM,
            -1.0,
            0.78*UNIT_MM, 1.13*UNIT_MM ),
    // class 3
    BOARD_MIN_SIZE_VALUES(1, 0.31*UNIT_MM, 0.31*UNIT_MM,
            0.45*UNIT_MM,
            0.6*UNIT_MM, 0.90*UNIT_MM ),
    // class 4
    BOARD_MIN_SIZE_VALUES(1, 0.21*UNIT_MM, 0.21*UNIT_MM,
            0.34*UNIT_MM,
            0.49*UNIT_MM, -1.0 ),
    // class 5
    BOARD_MIN_SIZE_VALUES(1, 0.15*UNIT_MM, 0.15*UNIT_MM,
            0.24*UNIT_MM,
            0.39*UNIT_MM, -1.0 ),
    // class 6
    BOARD_MIN_SIZE_VALUES(1, 0.12*UNIT_MM, 0.12*UNIT_MM,
            0.20*UNIT_MM,
            0.35*UNIT_MM, -1.0 )
};


void PANEL_BOARD_CLASS::OnBoardClassesUnitsSelection( wxCommandEvent& event )
{
    BoardClassesUpdateData( m_BoardClassesUnitsSelector->GetUnitScale() );
}


void PANEL_BOARD_CLASS::BoardClassesUpdateData( double aUnitScale )
{
    wxString txt;
    #define FMT wxT("%g")
    #define NO_VALUE wxT("--")

    for( int ii = 0; ii < BRDCLASS_COUNT; ii ++ )
    {
        // Display min tracks width
        if( clist[ii].m_Lines > -1.0 )
            txt.Printf( FMT, clist[ii].m_Lines / aUnitScale);
        else
            txt = NO_VALUE;
        m_gridClassesValuesDisplay->SetCellValue(0, ii, txt );

        // Display min clearance
        if( clist[ii].m_Clearance > -1.0 )
            txt.Printf( FMT, clist[ii].m_Clearance / aUnitScale);
        else
            txt = NO_VALUE;

        m_gridClassesValuesDisplay->SetCellValue(1, ii, txt );

        // Display min Via diam diff
        if( clist[ii].m_ViaDiamDiff > -1.0 )
            txt.Printf( FMT, clist[ii].m_ViaDiamDiff / aUnitScale);
        else
            txt = NO_VALUE;

        m_gridClassesValuesDisplay->SetCellValue(2, ii, txt );

        // Display min Pad diam diff (plated)
        if( clist[ii].m_PadDiamDiffPlated > -1.0 )
            txt.Printf( FMT, clist[ii].m_PadDiamDiffPlated / aUnitScale);
        else
            txt = NO_VALUE;

        m_gridClassesValuesDisplay->SetCellValue(3, ii, txt );

        // Display min Pad diam diff (non plated)
        if( clist[ii].m_PadDiamDiffNotPlated > -1.0 )
            txt.Printf( FMT, clist[ii].m_PadDiamDiffNotPlated / aUnitScale);
        else
            txt = NO_VALUE;

        m_gridClassesValuesDisplay->SetCellValue(4, ii, txt );
    }
}

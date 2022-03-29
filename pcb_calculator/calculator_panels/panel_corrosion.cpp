/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2022 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <calculator_panels/panel_corrosion.h>
#include <pcb_calculator_settings.h>

#define CORROSION_VOLTAGE_1 0.3
#define CORROSION_VOLTAGE_2 0.5
#define CORROSION_VOLTAGE_3 0.8


CORROSION_TABLE_ENTRY::CORROSION_TABLE_ENTRY( wxString aName, wxString aSymbol, double aPot )
{
    m_potential = aPot;
    m_name = aName;
    m_symbol = aSymbol;
}

PANEL_CORROSION::PANEL_CORROSION( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                                  const wxSize& size, long style, const wxString& name ) :
        PANEL_CORROSION_BASE( parent, id, pos, size, style, name )
{
    m_entries.clear();
    m_entries.push_back( CORROSION_TABLE_ENTRY( wxT( "Platinum" ), "Pt", -0.57 ) );
    m_entries.push_back( CORROSION_TABLE_ENTRY( wxT( "Gold" ), "Au", -0.44 ) );
    m_entries.push_back( CORROSION_TABLE_ENTRY( wxT( "Titanium" ), "Ti", -0.32 ) );
    m_entries.push_back(
            CORROSION_TABLE_ENTRY( wxT( "Stainless steel 18-9" ), "X8CrNiS18-9", -0.32 ) );
    m_entries.push_back( CORROSION_TABLE_ENTRY( wxT( "Silver" ), "Ag", -0.22 ) );
    m_entries.push_back( CORROSION_TABLE_ENTRY( wxT( "Mercury" ), "Hg", -0.22 ) );
    m_entries.push_back( CORROSION_TABLE_ENTRY( wxT( "Nickel" ), "Ni", -0.14 ) );
    m_entries.push_back( CORROSION_TABLE_ENTRY( wxT( "Copper" ), "Cu", 0.0 ) );
    m_entries.push_back( CORROSION_TABLE_ENTRY( wxT( "Copper-Aluminium" ), "CuAl10", 0.03 ) );
    m_entries.push_back( CORROSION_TABLE_ENTRY( wxT( "Brass" ), "CuZn39Pb", 0.08 ) );
    m_entries.push_back( CORROSION_TABLE_ENTRY( wxT( "Bronze" ), "CuSn12", 0.2 ) );
    m_entries.push_back( CORROSION_TABLE_ENTRY( wxT( "Tin" ), "CuSn12", 0.23 ) );
    m_entries.push_back( CORROSION_TABLE_ENTRY( wxT( "Lead" ), "Pb", 0.27 ) );
    m_entries.push_back( CORROSION_TABLE_ENTRY( wxT( "Aluminium-Copper" ), "AlCu4Mg", 0.37 ) );
    m_entries.push_back( CORROSION_TABLE_ENTRY( wxT( "Cast iron" ), "", 0.38 ) );
    m_entries.push_back( CORROSION_TABLE_ENTRY( wxT( "Carbon steel" ), "", 0.43 ) );
    m_entries.push_back( CORROSION_TABLE_ENTRY( wxT( "Aluminium" ), "Al", 0.52 ) );
    m_entries.push_back( CORROSION_TABLE_ENTRY( wxT( "Cadmium" ), "Al", 0.53 ) );
    m_entries.push_back( CORROSION_TABLE_ENTRY( wxT( "Iron" ), "Fe", 0.535 ) );
    m_entries.push_back( CORROSION_TABLE_ENTRY( wxT( "Chrome" ), "Fe", 0.63 ) );
    m_entries.push_back( CORROSION_TABLE_ENTRY( wxT( "Zinc" ), "Zn", 0.83 ) );
    m_entries.push_back( CORROSION_TABLE_ENTRY( wxT( "Manganese" ), "Mn", 0.9 ) );
    m_entries.push_back( CORROSION_TABLE_ENTRY( wxT( "Magnesium" ), "Mg", 1.38 ) );

    // Resize the table

    m_table->DeleteCols( 0, m_table->GetNumberCols() );
    m_table->DeleteRows( 0, m_table->GetNumberRows() );
    m_table->AppendCols( m_entries.size() );
    m_table->AppendRows( m_entries.size() );

    // Fill the table with data
    int i = 0;

    wxColour color_ok( 189, 255, 189 );
    wxColour color_w1( 255, 255, 157 );
    wxColour color_w2( 250, 191, 9 );
    wxColour color_w3( 255, 83, 83 );

    wxColour color_text( 0, 0, 0 );
    wxString value;


    for( CORROSION_TABLE_ENTRY entryA : m_entries )
    {
        int j = 0;

        wxString label = entryA.m_name;

        if( entryA.m_symbol.size() > 0 )
        {
            label += " (" + entryA.m_symbol + ")";
        }
        m_table->SetRowLabelValue( i, label );
        m_table->SetColLabelValue( i, label );

        for( CORROSION_TABLE_ENTRY entryB : m_entries )
        {
            double diff = entryA.m_potential - entryB.m_potential;
            value = "";
            value << diff * 1000; // Let's display it in mV instead of V.
            m_table->SetCellValue( i, j, value );

            // Overide anything that could come from a dark them
            m_table->SetCellTextColour( i, j, color_text );

            if( abs( diff ) > CORROSION_VOLTAGE_3 )
            {
                m_table->SetCellBackgroundColour( i, j, color_w3 );
            }
            else if( abs( diff ) > CORROSION_VOLTAGE_2 )
            {
                m_table->SetCellBackgroundColour( i, j, color_w2 );
            }
            else if( abs( diff ) > CORROSION_VOLTAGE_1 )
            {
                m_table->SetCellBackgroundColour( i, j, color_w1 );
            }
            else
            {
                m_table->SetCellBackgroundColour( i, j, color_ok );
            }
            j++;
        }
        i++;
    }

    m_table->SetColLabelTextOrientation( wxVERTICAL );

    m_table->SetColLabelSize( wxGRID_AUTOSIZE );
    m_table->SetRowLabelSize( wxGRID_AUTOSIZE );
    m_table->AutoSizeColumns();
    m_table->AutoSizeRows();
}


PANEL_CORROSION::~PANEL_CORROSION()
{
}

void PANEL_CORROSION::ThemeChanged()
{
}


void PANEL_CORROSION::LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
}


void PANEL_CORROSION::SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
}

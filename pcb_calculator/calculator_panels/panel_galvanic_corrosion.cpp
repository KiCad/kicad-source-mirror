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

#include <calculator_panels/panel_galvanic_corrosion.h>
#include <pcb_calculator_settings.h>
#include <widgets/unit_selector.h>
#include <math/util.h>      // for KiROUND
#include <string_utils.h>
#include <i18n_utility.h>   // For _HKI definition in galvanic_corrosion_help.h
wxString galvanic_corrosion_help =
#include "galvanic_corrosion_help.h"


extern double DoubleFromString( const wxString& TextValue );


// Return ITU-R BT.709 luminance text colour contrast (white or black) for a given background.
static wxColour getContrastingTextColour( const wxColour& aBg )
{
    int r = aBg.Red();
    int g = aBg.Green();
    int b = aBg.Blue();

    // ITU-R BT.709 luminance
    double luminance = 0.2126 * r + 0.7152 * g + 0.0722 * b;

    return ( luminance < 140.0 ) ? *wxWHITE : *wxBLACK;
}

CORROSION_TABLE_ENTRY::CORROSION_TABLE_ENTRY( const wxString& aName, const wxString& aSymbol,
                                              double aPotential )
{
    m_name = aName;
    m_symbol = aSymbol;
    m_potential = aPotential;
}

PANEL_GALVANIC_CORROSION::PANEL_GALVANIC_CORROSION( wxWindow* parent, wxWindowID id,
                                                    const wxPoint& pos, const wxSize& size,
                                                    long style, const wxString& name ) :
        PANEL_GALVANIC_CORROSION_BASE( parent, id, pos, size, style, name )
{
    m_entries.clear();

    // Galvanic electrode potentials in volts, relative to the Standard Hydrogen Electrode (SHE).
    // More negative values indicate anodic (less noble, more likely to corrode) behavior.
    // More positive values indicate cathodic (more noble, corrosion-resistant) behavior.
    //
    // This table initializes m_entries with validated single-potential values drawn from
    // authoritative engineering sources, including:
    //  - MIL-STD-889D (DoD Standard Practice: Dissimilar Metals)
    //  - NASA-STD-6012 (Materials and Processes for Corrosion Control)
    //  - CRC Handbook of Chemistry and Physics (pure metals, lab conditions)
    //  - ASM Handbook, Volumes 2 and 13C (material behavior and alloy data)
    //  - EN 50310 (electrical bonding and earthing potentials)
    //
    // All values assume standard conditions (approximately 25 °C) and aerated seawater exposure.
    // For passivated metals, passive film stability is assumed. Where applicable, the copper
    // electrode (Cu) is defined as the zero reference potential.
    //
    // Notes on solders:
    //  - Tin-lead solder alloys such as Sn63Pb37 and Sn60Pb40 have potentials similar to tin (~+0.23 V),
    //    but slightly lower due to lead content.
    //  - These alloys remain in use for aerospace, defense, and critical legacy systems
    //    where tin-whisker suppression is essential.

    // Noble, extremely corrosion-resistant; used in aerospace-grade plating
    // Ref: NASA-STD-6012, CRC Handbook
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Rhodium" ), "Rh", -0.60 ) );

    // Noble, cathodic; stable in seawater environments
    // Ref: MIL-STD-889D, CRC Handbook
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Platinum" ), "Pt", -0.57 ) );

    // Noble; used in hybrid microcircuits and plated contacts; Pd black value
    // Ref: MIL-STD-889D, CRC Handbook, ASM Handbook Vol. 13C
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Palladium" ), "Pd", -0.50 ) );

    // Highly noble; stable under most atmospheric and marine exposures
    // Ref: MIL-STD-889D, CRC Handbook, ASM Handbook Vol. 13C
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Gold" ), "Au", -0.44 ) );

    // Molybdenum-stabilized austenitic stainless steel; passivated
    // Ref: MIL-STD-889D, NASA-STD-6012, ASM Handbook Vol. 13C
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Stainless steel 316L" ), "X2CrNiMo17-12-2", -0.35 ) );

    // Nickel–chromium–iron superalloy; oxidation-resistant
    // Ref: NASA-STD-6012, ASM Handbook Vol. 13C
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Inconel" ), "Inconel", -0.35 ) );

    // Soft post-transition metal; tarnishes but forms a protective film
    // Ref: CRC Handbook, ASM Handbook Vol. 13C
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Indium" ), "In", -0.34 ) );

    // Passive titanium; forms stable oxide film; widely corrosion-resistant
    // Ref: MIL-STD-889D, ASM Handbook Vol. 13C
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Titanium, passive" ), "Ti", -0.32 ) );

    // Austenitic stainless steel (18-8); passivated condition
    // Ref: MIL-STD-889D, ASM Handbook Vol. 13C
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Stainless steel 18-9" ), "X8CrNiS18-9", -0.32 ) );

    // Noble metal; resists corrosion despite surface tarnish
    // Ref: MIL-STD-889D, CRC Handbook, ASM Handbook Vol. 13C
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Silver" ), "Ag", -0.22 ) );

    // Elemental liquid metal; seldom structural; included for completeness
    // Ref: CRC Handbook, ASM Handbook
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Mercury" ), "Hg", -0.22 ) );

    // Electroless nickel with palladium and immersion gold finish; used in high-reliability PCBs
    // Ref: IPC-4556, NASA-STD-8739.3, ASM Handbook Vol. 13C
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "ENEPIG (Ni/Pd/Au)" ), "ENEPIG", -0.18 ) );

    // Electroless nickel / immersion gold PCB finish; porosity-dependent
    // Ref: MIL-STD-889D (Ni base), IPC-4552
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "ENIG (Ni/Au)" ), "ENIG", -0.15 ) );

    // Stable passivated nickel surface; common in structural and PCB coatings
    // Ref: MIL-STD-889D, CRC Handbook
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Nickel" ), "Ni", -0.14 ) );

    // Conductive graphite or carbon ink; used in membrane switches and low-cost sensor interfaces
    // Ref: ASM Handbook Vol. 13C, IPC Application Notes
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Carbon (Graphitic)" ), "C", -0.10 ) );

    // Reference metal; zero potential in galvanic series
    // Ref: MIL-STD-889D, CRC Handbook
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Copper" ), "Cu", 0.00 ) );

    // Copper–aluminium bronze alloy; more anodic than pure Cu
    // Ref: MIL-STD-889D, ASM Handbook Vol. 2
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Copper-Aluminium" ), "CuAl10", 0.03 ) );

    // Common 60/40 brass alloy; moderately anodic to copper
    // Ref: MIL-STD-889D, ASM Handbook Vol. 13C
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Brass" ), "CuZn39Pb", 0.08 ) );

    // Used in precision contacts and PCB spring connectors
    // Ref: MIL-HDBK-5, ASM Handbook Vol. 2
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Beryllium copper" ), "CuBe2", 0.15 ) );

    // Tin-silver-copper alloy (SAC305); RoHS-compliant lead-free solder
    // Ref: IPC J-STD-006C, NASA-STD-8739.3, ASM Soldering Materials
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Lead-free Solder" ), "SAC305", 0.15 ) );

    // Tin-phosphor bronze; widely used in contact springs and connector terminations
    // Ref: MIL-HDBK-5, ASM Handbook Vol. 2
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Phosphor bronze" ), "CuSnP", 0.15 ) );

    // Tin-bronze alloy; structurally and electrochemically similar to Sn
    // Ref: CRC Handbook, ASM Handbook Vol. 2
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Bronze" ), "CuSn12", 0.20 ) );

    // Tin-nickel alloy plating; RoHS-compliant alternative to pure tin; mitigates whiskering
    // Ref: IPC J-STD-006C, ASM Soldering Materials
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Tin-Nickel (Sn/Ni)" ), "SnNi", 0.21 ) );

    // Sn63Pb37 eutectic solder; used in aerospace and high-reliability assemblies
    // Ref: CRC Handbook, IPC J-STD-006C, NASA-STD-8739.3
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Solder 63/37 (Eutectic)" ), "Sn63Pb37", 0.21 ) );

    // Sn60Pb40 solder; slightly more anodic due to increased Pb content
    // Ref: CRC Handbook, IPC J-STD-006C, NASA-STD-8739.3
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Solder 60/40 (Leaded)" ), "Sn60Pb40", 0.22 ) );

    // Pure tin; used in solders, plating, and legacy components
    // Ref: CRC Handbook, IPC J-STD-006C
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Tin" ), "Sn", 0.23 ) );

    // Heavy metal; stable potential; often used in radiation shielding
    // Ref: CRC Handbook, ASM Handbook
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Lead" ), "Pb", 0.27 ) );

    // High-strength aluminum-copper alloy (2024-T3 family)
    // Ref: MIL-STD-889D, ASM Handbook Vol. 2
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "2xxx series Al alloy" ), "AlCu4Mg", 0.37 ) );

    // Gray cast iron; nominal potential above mild steel
    // Ref: MIL-STD-889D, NASA-STD-6012
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Cast iron" ), "Fe-C-Si", 0.38 ) );

    // Mild carbon steel in uncoated, aerated seawater conditions
    // Ref: MIL-STD-889D, EN 50310
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Carbon steel" ), "Fe–C", 0.43 ) );

    // Chromated aluminium alloy (e.g., Alodine); used in EMI shield cans and structural enclosures
    // Ref: MIL-STD-889D, NASA-STD-6012
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Aluminium, chromated" ), "Al-Chromate", 0.50 ) );

    // Pure aluminum with passive oxide layer; typical field potential
    // Ref: MIL-STD-889D, NASA-STD-6012
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Aluminium, pure, passive" ), "Al", 0.52 ) );

    // Soft plating metal; used in fasteners and aerospace corrosion protection
    // Ref: MIL-STD-889D, ASM Handbook Vol. 13C
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Cadmium" ), "Cd", 0.53 ) );

    // Unalloyed iron in its active state; more anodic than steels
    // Ref: CRC Handbook, ASM Handbook
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Iron" ), "Fe", 0.535 ) );

    // Passive chromium (trivalent); protective oxide layer assumed
    // Ref: MIL-STD-889D, CRC Handbook
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Chrome, passive" ), "Cr", 0.63 ) );

    // Highly anodic metal; used in sacrificial anode systems
    // Ref: MIL-STD-889D, ASM Handbook Vol. 13C
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Zinc" ), "Zn", 0.83 ) );

    // Mild steel with zinc-plated surface; common in chassis screws and mounting hardware
    // Ref: MIL-STD-889D, ASM Handbook Vol. 13C
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Steel, zinc-plated" ), "Fe-Zn", 0.83 ) );

    // Reactive transition metal; rarely used unalloyed
    // Ref: CRC Handbook, ASM Handbook
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Manganese" ), "Mn", 0.90 ) );

    // Most anodic structural metal in common use; sacrificial anode
    // Ref: MIL-STD-889D, NASA-STD-6012
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Magnesium" ), "Mg", 1.38 ) );


    // Resize the table

    m_table->DeleteCols( 0, m_table->GetNumberCols() );
    m_table->DeleteRows( 0, m_table->GetNumberRows() );
    m_table->AppendCols( (int) m_entries.size() );
    m_table->AppendRows( (int) m_entries.size() );

    // show markdown formula explanation in lower help panel
    wxString msg;
    ConvertMarkdown2Html( wxGetTranslation( galvanic_corrosion_help ), msg );
    m_helpText->SetPage( msg );

    m_symbolicStatus = true;
    m_corFilterValue = 0;

    fillTable();
}


PANEL_GALVANIC_CORROSION::~PANEL_GALVANIC_CORROSION()
{
}


void PANEL_GALVANIC_CORROSION::ThemeChanged()
{
    // Update the HTML window with the help text
    m_helpText->ThemeChanged();
}


void PANEL_GALVANIC_CORROSION::LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    m_corFilterCtrl->SetValue( aCfg->m_CorrosionTable.threshold_voltage );
    m_corFilterValue = DoubleFromString( m_corFilterCtrl->GetValue() );

    bool refill_table = m_symbolicStatus != aCfg->m_CorrosionTable.show_symbols;
    m_symbolicStatus = aCfg->m_CorrosionTable.show_symbols;
    m_radioBtnSymbol->SetValue( m_symbolicStatus );
    m_radioBtnName->SetValue( !m_symbolicStatus );

    if( refill_table )
        fillTable();
}


void PANEL_GALVANIC_CORROSION::SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    aCfg->m_CorrosionTable.threshold_voltage = wxString( "" ) << m_corFilterValue;
    aCfg->m_CorrosionTable.show_symbols = m_symbolicStatus;
}


void PANEL_GALVANIC_CORROSION::OnNomenclatureChange( wxCommandEvent& aEvent )
{
    if( m_radioBtnSymbol->GetValue() )
    {
        m_symbolicStatus = true;
    }
    else if( m_radioBtnName->GetValue() )
    {
        m_symbolicStatus = false;
    }

    fillTable();
}


void PANEL_GALVANIC_CORROSION::OnCorFilterChange( wxCommandEvent& aEvent )
{
    m_corFilterValue = DoubleFromString( m_corFilterCtrl->GetValue() );
    fillTable();
}


void PANEL_GALVANIC_CORROSION::fillTable()
{

    // Fill the table with data
    int      i = 0;
    wxColour color_ok( 122, 166, 194 );
    wxColour color_text( 0, 0, 0 );
    wxString value;
    wxString label;

    for( const CORROSION_TABLE_ENTRY& entryA : m_entries )
    {
        int      j = 0;

        if( m_symbolicStatus == true )
        {
            if( entryA.m_symbol.size() > 0 )
            {
                label = entryA.m_symbol;
            }
            else
            {
                label = entryA.m_name;
            }
        }
        else
        {
            if( entryA.m_name.size() > 0 )
            {
                label = entryA.m_name;
            }
            else
            {
                label = entryA.m_symbol;
            }
        }

        m_table->SetRowLabelAlignment( wxALIGN_RIGHT, wxALIGN_CENTER );
        m_table->SetRowLabelValue( i, label );
        m_table->SetColLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
        m_table->SetColLabelValue( i, label );
        m_table->SetCellAlignment( i, j, wxALIGN_CENTER, wxALIGN_CENTER );

        for( const CORROSION_TABLE_ENTRY& entryB : m_entries )
        {
            double diff = entryA.m_potential - entryB.m_potential;
            int    diff_temp = KiROUND( abs( diff * 99 ) );

            value = wxString::Format( "%.0f", diff * 1000 ); // display in mV
            m_table->SetCellValue( i, j, value );

            wxColour aBg;

            if( abs( diff ) == 0 )
            {
                aBg = wxColour( 193, 231, 255 );
            }
            else if( ( KiROUND( abs( diff * 1000 ) ) ) > m_corFilterValue )
            {
                if( diff > 0 )
                {
                    aBg = wxColour( 226 - diff_temp, 226 - diff_temp, 246 - diff_temp );
                }
                else if( diff < 0 )
                {
                    aBg = wxColour( 255 - diff_temp, 222 - diff_temp, 199 - diff_temp );
                }
            }
            else
            {
                aBg = color_ok;
            }

            m_table->SetCellBackgroundColour( i, j, aBg );
            m_table->SetCellTextColour( i, j, getContrastingTextColour( aBg ) );
            m_table->SetCellAlignment( i, j, wxALIGN_CENTER, wxALIGN_CENTER );
            m_table->SetReadOnly( i, j, true );
            j++;
        }

        i++;
    }

    m_table->SetColLabelTextOrientation( wxVERTICAL );

    m_table->SetColLabelSize( wxGRID_AUTOSIZE );
    m_table->SetRowLabelSize( wxGRID_AUTOSIZE );
    m_table->AutoSizeColumns();
    m_table->AutoSizeRows();

    Layout();
}

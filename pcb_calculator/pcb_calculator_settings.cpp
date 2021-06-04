/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <array>
#include <pcb_calculator_settings.h>
#include <settings/common_settings.h>
#include <settings/json_settings_internals.h>
#include <settings/parameters.h>
#include <wx/config.h>


///! Update the schema version whenever a migration is required
const int pcbCalculatorSchemaVersion = 0;


PCB_CALCULATOR_SETTINGS::PCB_CALCULATOR_SETTINGS() :
        APP_SETTINGS_BASE( "pcb_calculator", pcbCalculatorSchemaVersion ),
        m_Attenuators(),
        m_BoardClassUnits( 0 ),
        m_ColorCodeTolerance( 0 ),
        m_Electrical(),
        m_LastPage( 0 ),
        m_Regulators(),
        m_TrackWidth(),
        m_TransLine(),
        m_ViaSize()
{
    // Build settings:
    m_params.emplace_back( new PARAM<int>( "board_class_units", &m_BoardClassUnits, 0 ) );

    m_params.emplace_back( new PARAM<int>( "color_code_tolerance", &m_ColorCodeTolerance, 0 ) );

    m_params.emplace_back( new PARAM<int>( "last_page", &m_LastPage, 0 ) );

    m_params.emplace_back( new PARAM<int>( "translines.type", &m_TransLine.type, 0 ) );

    m_params.emplace_back( new PARAM<int>( "attenuators.type", &m_Attenuators.type, 0 ) );

    const std::array<std::string, 4> att_names = { "att_pi", "att_tee",
                                                   "att_bridge", "att_splitter" };

    for( const auto& att_name : att_names )
    {
        std::string path = "attenuators." + att_name;
        m_Attenuators.attenuators[ att_name ] = ATTENUATOR();
        ATTENUATOR* att = &m_Attenuators.attenuators[ att_name ];

        m_params.emplace_back( new PARAM<double>( path + ".attenuation", &att->attenuation, 6.0 ) );
        m_params.emplace_back( new PARAM<double>( path + ".zin", &att->zin, 50.0 ) );
        m_params.emplace_back( new PARAM<double>( path + ".zout", &att->zout, 50.0 ) );
    }

    m_params.emplace_back( new PARAM<int>( "electrical.spacing_units",
            &m_Electrical.spacing_units, 0 ) );

    m_params.emplace_back( new PARAM<wxString>( "electrical.spacing_voltage",
            &m_Electrical.spacing_voltage, "500" ) );

    m_params.emplace_back( new PARAM<wxString>( "regulators.r1", &m_Regulators.r1, "10" ) );

    m_params.emplace_back( new PARAM<wxString>( "regulators.r2", &m_Regulators.r2, "10" ) );

    m_params.emplace_back( new PARAM<wxString>( "regulators.vref", &m_Regulators.vref, "3" ) );

    m_params.emplace_back( new PARAM<wxString>( "regulators.vout", &m_Regulators.vout, "12" ) );

    m_params.emplace_back( new PARAM<wxString>( "regulators.data_file",
            &m_Regulators.data_file, "" ) );

    m_params.emplace_back( new PARAM<wxString>( "regulators.selected_regulator",
            &m_Regulators.selected_regulator, "" ) );

    m_params.emplace_back( new PARAM<int>( "regulators.type", &m_Regulators.type, 0 ) );

    m_params.emplace_back( new PARAM<int>( "regulators.last_param", &m_Regulators.last_param, 0 ) );

    m_params.emplace_back( new PARAM<wxString>( "track_width.current",
            &m_TrackWidth.current, "1.0" ) );

    m_params.emplace_back( new PARAM<wxString>( "track_width.delta_tc",
            &m_TrackWidth.delta_tc, "10.0" ) );

    m_params.emplace_back( new PARAM<wxString>( "track_width.track_len",
            &m_TrackWidth.track_len, "20" ) );

    m_params.emplace_back( new PARAM<int>( "track_width.track_len_units",
            &m_TrackWidth.track_len_units, 0 ) );

    m_params.emplace_back( new PARAM<wxString>( "track_width.resistivity",
            &m_TrackWidth.resistivity, "1.72e-8" ) );

    m_params.emplace_back( new PARAM<wxString>( "track_width.ext_track_width",
            &m_TrackWidth.ext_track_width, "0.2" ) );

    m_params.emplace_back( new PARAM<int>( "track_width.ext_track_width_units",
            &m_TrackWidth.ext_track_width_units, 0 ) );

    m_params.emplace_back( new PARAM<wxString>( "track_width.ext_track_thickness",
            &m_TrackWidth.ext_track_thickness, "0.035" ) );

    m_params.emplace_back( new PARAM<int>( "track_width.ext_track_thickness_units",
            &m_TrackWidth.ext_track_thickness_units, 0 ) );

    m_params.emplace_back( new PARAM<wxString>( "track_width.int_track_width",
            &m_TrackWidth.int_track_width, "0.2" ) );

    m_params.emplace_back( new PARAM<int>( "track_width.int_track_width_units",
            &m_TrackWidth.int_track_width_units, 0 ) );

    m_params.emplace_back( new PARAM<wxString>( "track_width.int_track_thickness",
            &m_TrackWidth.int_track_thickness, "0.035" ) );

    m_params.emplace_back( new PARAM<int>( "track_width.int_track_thickness_units",
            &m_TrackWidth.int_track_thickness_units, 0 ) );

    {
        const std::array<std::string, 8> transline_names = { "MicroStrip", "CoPlanar", "GrCoPlanar",
                "RectWaveGuide", "Coax", "Coupled_MicroStrip", "StripLine", "TwistedPair" };

        for( const auto& name : transline_names )
        {
            m_TransLine.param_values[ name ] = TL_PARAM_MAP();
            m_TransLine.param_units[ name ] = TL_PARAM_UNITS_MAP();

            std::string path = "trans_line." + name + ".";

            m_params.emplace_back( new PARAM_MAP<double>( path + "values",
                    &m_TransLine.param_values.at( name ), {} ) );

            m_params.emplace_back( new PARAM_MAP<int>( path + "units",
                    &m_TransLine.param_units.at( name ), {} ) );
        }
    }

    m_params.emplace_back( new PARAM<wxString>( "via_size.hole_diameter",
            &m_ViaSize.hole_diameter, "0.4" ) );

    m_params.emplace_back( new PARAM<int>( "via_size.hole_diameter_units",
            &m_ViaSize.hole_diameter_units, 0 ) );

    m_params.emplace_back( new PARAM<wxString>( "via_size.thickness",
            &m_ViaSize.thickness, "0.035" ) );

    m_params.emplace_back( new PARAM<int>( "via_size.thickness_units",
            &m_ViaSize.thickness_units, 0 ) );

    m_params.emplace_back( new PARAM<wxString>( "via_size.length",
            &m_ViaSize.length, "1.6" ) );

    m_params.emplace_back( new PARAM<int>( "via_size.length_units", &m_ViaSize.length_units, 0 ) );

    m_params.emplace_back( new PARAM<wxString>( "via_size.pad_diameter",
            &m_ViaSize.pad_diameter, "0.6" ) );

    m_params.emplace_back( new PARAM<int>( "via_size.pad_diameter_units",
            &m_ViaSize.pad_diameter_units, 0 ) );

    m_params.emplace_back( new PARAM<wxString>( "via_size.clearance_diameter",
            &m_ViaSize.clearance_diameter, "1.0" ) );

    m_params.emplace_back( new PARAM<int>( "via_size.clearance_diameter_units",
            &m_ViaSize.clearance_diameter_units, 0 ) );

    m_params.emplace_back( new PARAM<wxString>( "via_size.characteristic_impedance",
            &m_ViaSize.characteristic_impedance, "50" ) );

    m_params.emplace_back( new PARAM<int>( "via_size.characteristic_impedance_units",
            &m_ViaSize.characteristic_impedance_units, 0 ) );

    m_params.emplace_back( new PARAM<wxString>( "via_size.applied_current",
            &m_ViaSize.applied_current, "1" ) );

    m_params.emplace_back( new PARAM<wxString>( "via_size.plating_resistivity",
            &m_ViaSize.plating_resistivity, "1.72e-8" ) );

    m_params.emplace_back( new PARAM<wxString>( "via_size.permittivity",
            &m_ViaSize.permittivity, "4.5" ) );

    m_params.emplace_back( new PARAM<wxString>( "via_size.temp_rise",
            &m_ViaSize.temp_rise, "10" ) );

    m_params.emplace_back( new PARAM<wxString>( "via_size.pulse_rise_time",
            &m_ViaSize.pulse_rise_time, "1" ) );
}


bool PCB_CALCULATOR_SETTINGS::MigrateFromLegacy( wxConfigBase* aCfg )
{
    bool ret = APP_SETTINGS_BASE::MigrateFromLegacy( aCfg );

    ret &= fromLegacy<int>(  aCfg, "BrdClass_selection", "board_class_units" );
    ret &= fromLegacy<int>(  aCfg, "CC_selection",       "color_code_tolerance" );
    ret &= fromLegacy<int>(  aCfg, "Page_selection",     "last_page" );

    ret &= fromLegacy<int>(  aCfg, "Att_selection",     "attenuators.type" );

    {
        nlohmann::json::json_pointer ptr =
                JSON_SETTINGS_INTERNALS::PointerFromString( "attenuators" );

        const std::array<std::string, 4> att_names = { "att_pi", "att_tee",
                                                       "att_bridge", "att_splitter" };
        double val = 0;

        for( const auto& att : att_names )
        {
            aCfg->SetPath( "Attenuators/" + att );
            ptr.push_back( att );

            if( aCfg->Read( "Attenuation", &val ) )
                ( *m_internals )[ptr]["attenuation"] = val;

            if( aCfg->Read( "Zin", &val ) )
                ( *m_internals )[ptr]["zin"] = val;

            if( aCfg->Read( "Zout", &val ) )
                ( *m_internals )[ptr]["zout"] = val;

            ptr.pop_back();
            aCfg->SetPath( "../.." );
        }
    }

    ret &= fromLegacy<int>(   aCfg, "ElectSpacing_selection", "electrical.spacing_units" );
    ret &= fromLegacyString(  aCfg, "ElectSpacing_voltage",   "electrical.spacing_voltage" );

    ret &= fromLegacyString( aCfg, "RegulR1",           "regulators.r1" );
    ret &= fromLegacyString( aCfg, "RegulR2",           "regulators.r2" );
    ret &= fromLegacyString( aCfg, "RegulVREF",         "regulators.vref" );
    ret &= fromLegacyString( aCfg, "RegulVOUT",         "regulators.vout" );
    ret &= fromLegacyString( aCfg, "DataFilename",      "regulators.data_file" );
    ret &= fromLegacyString( aCfg, "RegulName",         "regulators.selected_regulator" );
    ret &= fromLegacy<int>(  aCfg, "RegulType",         "regulators.type" );
    ret &= fromLegacy<int>(  aCfg, "RegulLastParam",    "regulators.last_param" );

    ret &= fromLegacyString( aCfg, "TW_Track_Current",       "track_width.current" );
    ret &= fromLegacyString( aCfg, "TW_Delta_TC",            "track_width.delta_tc" );
    ret &= fromLegacyString( aCfg, "TW_Track_Len",           "track_width.track_len" );
    ret &= fromLegacy<int>(  aCfg, "TW_Track_Len_Unit",      "track_width.track_len_units" );
    ret &= fromLegacyString( aCfg, "TW_Resistivity",         "track_width.resistivity" );
    ret &= fromLegacyString( aCfg, "TW_ExtTrack_Width",      "track_width.ext_track_width" );
    ret &= fromLegacy<int>(  aCfg, "TW_ExtTrack_Width_Unit", "track_width.ext_track_width_units" );
    ret &= fromLegacyString( aCfg, "TW_ExtTrack_Thickness",  "track_width.ext_track_thickness" );
    ret &= fromLegacy<int>(  aCfg, "TW_ExtTrack_Thickness_Unit",
        "track_width.ext_track_thickness_units" );
    ret &= fromLegacyString( aCfg, "TW_IntTrack_Width",      "track_width.int_track_width" );
    ret &= fromLegacy<int>(  aCfg, "TW_IntTrack_Width_Unit", "track_width.int_track_width_units" );
    ret &= fromLegacyString( aCfg, "TW_IntTrack_Thickness",  "track_width.int_track_thickness" );
    ret &= fromLegacy<int>(  aCfg, "TW_IntTrack_Thickness_Unit",
            "track_width.int_track_thickness_units" );

    ret &= fromLegacy<int>(  aCfg, "Transline_selection",    "trans_line.selection" );

    {
        nlohmann::json::json_pointer ptr =
                JSON_SETTINGS_INTERNALS::PointerFromString( "trans_line" );

        wxString key;
        double   value    = 0;
        int      units    = 0;

        const std::array<std::string, 8> transline_names = { "MicroStrip", "CoPlanar", "GrCoPlanar",
                "RectWaveGuide", "Coax", "Coupled_MicroStrip", "StripLine", "TwistedPair" };

        for( const auto& name : transline_names )
        {
            long index = 0;
            aCfg->SetPath( name );
            ptr.push_back( name );

            while( aCfg->GetNextEntry( key, index ) )
            {
                // Keys look like "translineprmN" and "translineprmNunit"
                wxString dest = key;
                dest.Replace( "translineprm", wxEmptyString );

                if( dest.EndsWith( "unit" ) )
                {
                    dest.Replace( "unit", wxEmptyString );
                    aCfg->Read( key, &units );
                    ptr.push_back( "units" );

                    ( *m_internals )[ptr].push_back( { { dest.ToStdString(), units } } );

                    ptr.pop_back();
                }
                else
                {
                    aCfg->Read( key, &value );
                    ptr.push_back( "values" );

                    ( *m_internals )[ptr].push_back( { { dest.ToStdString(), value } } );

                    ptr.pop_back();
                }
            }

            ptr.pop_back();
            aCfg->SetPath( ".." );
        }
    }

    ret &= fromLegacyString( aCfg, "VS_Hole_Dia",                 "via_size.hole_diameter" );
    ret &= fromLegacy<int>(  aCfg, "VS_Hole_Dia_Unit",            "via_size.hole_diameter_units" );
    ret &= fromLegacyString( aCfg, "VS_Plating_Thickness",        "via_size.thickness" );
    ret &= fromLegacy<int>(  aCfg, "VS_Plating_Thickness_Unit",   "via_size.thickness_units" );
    ret &= fromLegacyString( aCfg, "VS_Via_Length",               "via_size.length" );
    ret &= fromLegacy<int>(  aCfg, "VS_Via_Length_Unit",          "via_size.length_units" );
    ret &= fromLegacyString( aCfg, "VS_Pad_Dia",                  "via_size.pad_diameter" );
    ret &= fromLegacy<int>(  aCfg, "VS_Pad_Dia_Unit",             "via_size.pad_diameter_units" );
    ret &= fromLegacyString( aCfg, "VS_Clearance_Dia",            "via_size.clearance_diameter" );
    ret &= fromLegacy<int>(  aCfg, "VS_Clearance_Dia_Unit",
            "via_size.clearance_diameter_units" );
    ret &= fromLegacyString( aCfg, "VS_Characteristic_Impedance",
            "via_size.characteristic_impedance" );
    ret &= fromLegacy<int>(  aCfg, "VS_Characteristic_Impedance_Unit",
            "via_size.characteristic_impedance_units" );
    ret &= fromLegacyString( aCfg, "VS_Current",                  "via_size.applied_current" );
    ret &= fromLegacyString( aCfg, "VS_Resistivity",              "via_size.plating_resistivity" );
    ret &= fromLegacyString( aCfg, "VS_Permittivity",             "via_size.permittivity" );
    ret &= fromLegacyString( aCfg, "VS_Temperature_Differential", "via_size.temp_rise" );
    ret &= fromLegacyString( aCfg, "VS_Pulse_Rise_Time",          "via_size.pulse_rise_time" );

    return ret;
}

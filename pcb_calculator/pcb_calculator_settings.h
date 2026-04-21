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

#ifndef _PCB_CALCULATOR_SETTINGS_H
#define _PCB_CALCULATOR_SETTINGS_H

#include <array>
#include <unordered_map>
#include <settings/app_settings.h>

// Deafult value for TI LM317
#define DEFAULT_REGULATOR_RESTOL "1"
#define DEFAULT_REGULATOR_R1 "0.240"
#define DEFAULT_REGULATOR_R2 "0.720"
#define DEFAULT_REGULATOR_VREF_MIN "1.20"
#define DEFAULT_REGULATOR_VREF_TYP "1.25"
#define DEFAULT_REGULATOR_VREF_MAX "1.30"
#define DEFAULT_REGULATOR_VOUT_TYP "5"
#define DEFAULT_REGULATOR_IADJ_TYP "50"
#define DEFAULT_REGULATOR_IADJ_MAX "100"

class PCB_CALCULATOR_SETTINGS : public APP_SETTINGS_BASE
{
public:
    struct ATTENUATOR
    {
        double attenuation;
        double zin;
        double zout;
    };

    struct ATTENUATORS
    {
        int type;
        std::unordered_map<std::string, ATTENUATOR> attenuators;
    };

    struct ELECTRICAL
    {
        int spacing_units;
        wxString spacing_voltage;

        double iec60664_ratedVoltage;
        int    iec60664_OVC;
        double iec60664_RMSvoltage;
        double iec60664_transientOV;
        double iec60664_peakOV;
        int    iec60664_insulationType;
        int    iec60664_pollutionDegree;
        int    iec60664_materialGroup;
        int    iec60664_pcbMaterial;
        double iec60664_altitude;
    };

    struct REGULATORS
    {
        wxString resTol;
        wxString r1;
        wxString r2;
        wxString vrefMin;
        wxString vrefTyp;
        wxString vrefMax;
        wxString voutTyp;
        wxString iadjTyp;
        wxString iadjMax;
        wxString data_file;
        wxString selected_regulator;
        int type;
        int last_param;
    };

    struct CABLE_SIZE
    {
        int diameterUnit;
        int linResUnit;
        int frequencyUnit;
        int lengthUnit;
        int currentDensityChoice;
        wxString conductorMaterialResitivity;
        wxString conductorTemperature;
        wxString conductorThermalCoef;
    };

    struct WAVELENGTH
    {
        double permittivity;
        double permeability;
        double frequency;
        int    frequencyUnit;
        int    periodUnit;
        int    wavelengthVacuumUnit;
        int    wavelengthMediumUnit;
        int    speedUnit;
    };

    struct TRACK_WIDTH
    {
        wxString current;
        wxString delta_tc;
        wxString track_len;
        int      track_len_units;
        wxString resistivity;
        wxString ext_track_width;
        int      ext_track_width_units;
        wxString ext_track_thickness;
        int      ext_track_thickness_units;
        wxString int_track_width;
        int      int_track_width_units;
        wxString int_track_thickness;
        int      int_track_thickness_units;
    };

    /// Map of TRANSLINE_PRM id to value
    typedef std::map<std::string, double> TL_PARAM_MAP;

    /// Map of TRANSLINE_PRM id to units selection
    typedef std::map<std::string, int> TL_PARAM_UNITS_MAP;

    struct TRANSMISSION_LINE
    {
        int type = 0;

        /// Dielectric dispersion model.  0 = CONSTANT (default), 1 = Djordjevic-Sarkar causal.
        int dielectric_model = 0;

        /// Spec frequency at which EpsilonR and TanD are specified (user-entered text).
        wxString spec_frequency = wxT( "1" );

        /// UNIT_SELECTOR_FREQUENCY index: 0 = GHz, 1 = MHz, 2 = kHz, 3 = Hz.
        int spec_frequency_unit = 0;

        /// Soldermask / LPI overlay correction.  0 = disabled (default; math stays
        /// bit-identical to the pre-mask baseline), 1 = Wan and Hoorfar 2000 / Svacina
        /// 1992 filling factor applied with Bahl and Stuchly 1980 air-replacement
        /// decomposition.
        int soldermask_present = 0;

        /// Cured mask thickness in metres.  20 um is typical for modern green LPI.
        double soldermask_thickness = 20.0e-6;

        /// Mask relative permittivity.  3.5 is representative of green LPI.
        double soldermask_epsilonr = 3.5;

        /// Mask loss tangent.  0.025 is the nominal LPI value; high-grade solder resist
        /// products quote 0.02 - 0.035.
        double soldermask_tand = 0.025;

        /// CPW / CBCPW only.  1 = mask fills the coplanar slots (standard LPI process),
        /// 0 = selective mask that covers only the traces.
        int soldermask_fills_gaps = 1;

        /// Transline parameters, per transline type
        std::map<std::string, TL_PARAM_MAP> param_values;

        /// Transline parameter units selections, per transline type
        std::map<std::string, TL_PARAM_UNITS_MAP> param_units;
    };

    struct VIA_SIZE
    {
        wxString hole_diameter;
        int      hole_diameter_units;
        wxString thickness;
        int      thickness_units;
        wxString length;
        int      length_units;
        wxString pad_diameter;
        int      pad_diameter_units;
        wxString clearance_diameter;
        int      clearance_diameter_units;
        wxString characteristic_impedance;
        int      characteristic_impedance_units;
        wxString applied_current;
        wxString plating_resistivity;
        wxString permittivity;
        wxString temp_rise;
        wxString pulse_rise_time;
    };

    struct CORROSION_TABLE
    {
        wxString threshold_voltage;
        bool     show_symbols;
    };

    PCB_CALCULATOR_SETTINGS();

    virtual ~PCB_CALCULATOR_SETTINGS() {}

    virtual bool MigrateFromLegacy( wxConfigBase* aLegacyConfig ) override;

protected:
    virtual std::string getLegacyFrameName() const override { return "pcb_calculator"; }

public:
    ATTENUATORS m_Attenuators;

    int m_BoardClassUnits;

    int m_ColorCodeTolerance;

    ELECTRICAL m_Electrical;

    int m_LastPage;

    REGULATORS m_Regulators;

    CABLE_SIZE m_cableSize;

    WAVELENGTH m_wavelength;

    TRACK_WIDTH m_TrackWidth;

    TRANSMISSION_LINE m_TransLine;

    VIA_SIZE m_ViaSize;

    CORROSION_TABLE m_CorrosionTable;
};

#endif

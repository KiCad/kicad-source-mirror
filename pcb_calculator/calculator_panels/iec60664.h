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


// Based on IEC60664-1 : 2020-05
#ifndef IEC_60664_H
#define IEC_60664_H
#include <stdexcept>
#include <math.h>

class IEC60664
{
public:
    IEC60664(){};
    ~IEC60664(){};


    enum class POLLUTION_DEGREE
    {
        PD1 = 1,
        PD2,
        PD3,
        PD4
    };

    enum class OV_CATEGORY
    {
        OV_I = 1,
        OV_II,
        OV_III,
        OV_IV
    };

    enum class MATERIAL_GROUP
    {
        MG_I = 1,
        MG_II,
        MG_IIIa,
        MG_IIIb,
        NOT_INSULATING
    };

    enum class INSULATION_TYPE
    {
        FUNCTIONAL = 1,
        BASIC,
        REINFORCED
    };

    enum class FIELD
    {
        INHOMOGENEOUS = 1,
        HOMOGENEOUS
    };

    enum class WAVEFORM
    {
        AC_50_OR_60HZ = 1,
        IMPULSE_1_2_50
    };


    void SetAltitude( const double aAltitude ) { m_altitude = aAltitude; };
    void SetPollutionDegree( const POLLUTION_DEGREE aPD ) { m_pollutionDegree = aPD; };
    void SetPCBMaterial( const bool aPCB ) { m_pcbMaterial = aPCB; };
    void SetMaterialGroup( const MATERIAL_GROUP aMG ) { m_materialGroup = aMG; };
    void SetFrequency( const double aF ) { m_frequency = aF; };
    void SetInsulationType( const INSULATION_TYPE aIns ) { m_insulationType = aIns; };
    void SetOvervoltageCategory( const OV_CATEGORY aOV ) { m_overvoltageCat = aOV; };
    void SetField( const FIELD aField ) { m_field = aField; };
    void SetRatedVoltage( const double aV ) { m_ratedVoltage = aV; };
    void SetRMSVoltage( const double aV ) { m_RMSvoltage = aV; };
    void SetPeakVoltage( const double aV ) { m_peakVoltage = aV; };
    void SetTransientVoltage( const double aV ) { m_transientVoltage = aV; };


    bool Compute();

    double GetMinGrooveWidth() { return m_minGrooveWidth; };
    double GetClearanceDistance() { return m_clearance; };
    double GetCreepageDistance() { return m_creepage; };

    double GetRatedImpulseWithstandVoltage();

private:
    double m_ratedVoltage = 100e3;
    double m_frequency = 50;
    bool   m_coated = false;
    bool   m_pcbMaterial = false;

    double           m_RMSvoltage = 100e3;
    double           m_peakVoltage = 100e3;
    double           m_transientVoltage = 100e3;
    double           m_altitude = 100e3;
    double           m_clearance = -1;
    double           m_creepage = -1;
    double           m_minGrooveWidth = -1;
    FIELD            m_field = FIELD::INHOMOGENEOUS;
    OV_CATEGORY      m_overvoltageCat = OV_CATEGORY::OV_IV;
    MATERIAL_GROUP   m_materialGroup = MATERIAL_GROUP::NOT_INSULATING;
    POLLUTION_DEGREE m_pollutionDegree = POLLUTION_DEGREE::PD3;
    INSULATION_TYPE  m_insulationType = INSULATION_TYPE::REINFORCED;


    double         GetMinGrooveWidth( const POLLUTION_DEGREE aPD, const double aDistIso = 100 );
    double         GetClearanceAltitudeCorrectionFactor( const double aAltitude );
    double         GetClearanceForRMSVoltage( const double aVRMS,
                                              const FIELD  aField = FIELD::INHOMOGENEOUS );
    MATERIAL_GROUP GetMaterialGroupFromCTI( const double aCTI );
    double         GetClearanceToWithstandTransientVoltage( const double           aVoltage,
                                                            const POLLUTION_DEGREE aPD,
                                                            const FIELD            aField );
    double         GetClearanceToWithstandPeaks( const double aVoltage, const FIELD aField );
    double         GetBasicCreepageDistance( const double aVoltage, const POLLUTION_DEGREE aPD,
                                             const MATERIAL_GROUP aMG );
    double         ComputeClearanceDistance( const POLLUTION_DEGREE aPD, const FIELD aField,
                                             const double aAltitude );
    double         ComputeCreepageDistance( const POLLUTION_DEGREE aPD, const MATERIAL_GROUP aMG );
};

#endif
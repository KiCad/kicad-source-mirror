/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SPICE_MODEL_H
#define SPICE_MODEL_H

#include <map>
#include <stdexcept>
#include <enum_vector.h>
#include <sch_field.h>
#include <lib_field.h>
#include <sim/ngspice.h>
#include <sim/spice_value.h>
#include <wx/string.h>

class SPICE_MODEL
{
public:
    static constexpr auto DEVICE_TYPE_FIELD = "Model_Device";
    static constexpr auto TYPE_FIELD = "Model_Type";
    static constexpr auto FILE_FIELD = "Model_File";
    static constexpr auto PARAMS_FIELD = "Model_Params";

    /*struct PARAM
    {
        SPICE_VALUE value;
        wxString description;
        wxString unit;
    };*/


    DEFINE_ENUM_CLASS_WITH_ITERATOR( DEVICE_TYPE, 
        NONE,

        RESISTOR,
        CAPACITOR,
        INDUCTOR,
        TLINE,

        DIODE,
        BJT,
        JFET,
        MESFET,
        MOSFET,

        VSOURCE,
        ISOURCE,

        SUBCIRCUIT,
        CODEMODEL,
        RAWSPICE
    )

    struct DEVICE_TYPE_INFO
    {
        wxString fieldValue;
        wxString description;
    };

    static DEVICE_TYPE_INFO DeviceTypeInfo( DEVICE_TYPE aDeviceType )
    {
        switch( aDeviceType )
        {
        case DEVICE_TYPE::NONE:       return {"",           ""};
        case DEVICE_TYPE::RESISTOR:   return {"RESISTOR",   "Resistor"};
        case DEVICE_TYPE::CAPACITOR:  return {"CAPACITOR",  "Capacitor"};
        case DEVICE_TYPE::INDUCTOR:   return {"INDUCTOR",   "Inductor"};
        case DEVICE_TYPE::TLINE:      return {"TLINE",      "Transmission Line"};
        case DEVICE_TYPE::DIODE:      return {"DIODE",      "Diode"};
        case DEVICE_TYPE::BJT:        return {"BJT",        "BJT"};
        case DEVICE_TYPE::JFET:       return {"JFET",       "JFET"};
        case DEVICE_TYPE::MOSFET:     return {"MOSFET",     "MOSFET"};
        case DEVICE_TYPE::MESFET:     return {"MESFET",     "MESFET"};
        case DEVICE_TYPE::VSOURCE:    return {"VSOURCE",    "Voltage Source"};
        case DEVICE_TYPE::ISOURCE:    return {"ISOURCE",    "Current Source"};
        case DEVICE_TYPE::SUBCIRCUIT: return {"SUBCIRCUIT", "Subcircuit"};
        case DEVICE_TYPE::CODEMODEL:  return {"CODEMODEL",  "Code Model"};
        case DEVICE_TYPE::RAWSPICE:   return {"RAWSPICE",   "Raw Spice Element"};
        case DEVICE_TYPE::_ENUM_END:  break;
        }

        wxFAIL;
        return {};
    }


    DEFINE_ENUM_CLASS_WITH_ITERATOR( TYPE,
        NONE,

        RESISTOR_IDEAL,
        RESISTOR_SEMICONDUCTOR,

        CAPACITOR_IDEAL,
        CAPACITOR_SEMICONDUCTOR,

        INDUCTOR_IDEAL,
        INDUCTOR_IDEAL_COIL,

        TLINE_LOSSY,
        TLINE_LOSSLESS,
        TLINE_DISTRIBUTED_RC,
        TLINE_KSPICE_LOSSY,

        DIODE,

        BJT_GUMMEL_POON,
        BJT_VBIC,
        //BJT_MEXTRAM,
        BJT_HICUM_L2,
        //BJT_HICUM_L0,

        JFET_SHICHMAN_HODGES,
        JFET_PARKER_SKELLERN,

        MESFET_STATZ,
        MESFET_YTTERDAL,
        MESFET_HFET1,
        MESFET_HFET2,

        MOSFET_MOS1,
        MOSFET_MOS2,
        MOSFET_MOS3,
        MOSFET_BSIM1,
        MOSFET_BSIM2,
        MOSFET_MOS6,
        MOSFET_MOS9,
        MOSFET_BSIM3,
        MOSFET_B4SOI,
        MOSFET_BSIM4,
        //MOSFET_EKV2_6,
        //MOSFET_PSP,
        MOSFET_B3SOIFD,
        MOSFET_B3SOIDD,
        MOSFET_B3SOIPD,
        //MOSFET_STAG,
        MOSFET_HISIM2,
        MOSFET_HISIM_HV,

        VSOURCE,
        ISOURCE,

        SUBCIRCUIT,
        CODEMODEL,
        RAWSPICE
    )

    struct TYPE_INFO
    {
        DEVICE_TYPE deviceType;
        //NGSPICE::MODEL_TYPE ngspiceModelType;
        wxString ngspicePrimitive;
        unsigned int ngspiceLevel;
        wxString fieldValue;
        wxString description;
    };

    static TYPE_INFO TypeInfo( TYPE aType )
    { 
        switch( aType )
        { 
        case TYPE::NONE:                    return { DEVICE_TYPE::NONE,       "",  0,  "",                "" };

        case TYPE::RESISTOR_IDEAL:          return { DEVICE_TYPE::RESISTOR,   "R", 0,  "IDEAL",           "Ideal model" };
        case TYPE::RESISTOR_SEMICONDUCTOR:  return { DEVICE_TYPE::RESISTOR,   "R", 0,  "SEMICONDUCTOR",   "Semiconductor model" };

        case TYPE::CAPACITOR_IDEAL:         return { DEVICE_TYPE::CAPACITOR,  "C", 0,  "IDEAL",           "Ideal model" };
        case TYPE::CAPACITOR_SEMICONDUCTOR: return { DEVICE_TYPE::CAPACITOR,  "C", 0,  "SEMICONDUCTOR",   "Semiconductor model" };

        case TYPE::INDUCTOR_IDEAL:          return { DEVICE_TYPE::INDUCTOR,   "L", 0,  "IDEAL",           "Ideal model" };
        case TYPE::INDUCTOR_IDEAL_COIL:     return { DEVICE_TYPE::INDUCTOR,   "L", 0,  "LOSSLESS_COIL",   "Lossless coil model" };
    
        case TYPE::TLINE_LOSSY:             return { DEVICE_TYPE::TLINE,      "O", 0,  "LOSSY",           "Lossy model" };
        case TYPE::TLINE_LOSSLESS:          return { DEVICE_TYPE::TLINE,      "T", 0,  "LOSSLESS",        "Lossless model" };
        case TYPE::TLINE_DISTRIBUTED_RC:    return { DEVICE_TYPE::TLINE,      "U", 0,  "DISTRIBUTED_RC",  "Uniformly distributed RC model" };
        case TYPE::TLINE_KSPICE_LOSSY:      return { DEVICE_TYPE::TLINE,      "Y", 0,  "KSPICE_LOSSY",    "KSPICE lossy model" };

        case TYPE::DIODE:                   return { DEVICE_TYPE::DIODE,      "D", 0,  "",                "" };
        
        case TYPE::BJT_GUMMEL_POON:         return { DEVICE_TYPE::BJT,        "Q", 1,  "GUMMEL_POON",     "Gummel-Poon model" };
        case TYPE::BJT_VBIC:                return { DEVICE_TYPE::BJT,        "Q", 4,  "VBIC",            "VBIC model" };
        //case TYPE::BJT_MEXTRAM:             return { DEVICE_TYPE::BJT,        "Q", 6,  "MEXTRAM",         "MEXTRAM model" };
        case TYPE::BJT_HICUM_L2:            return { DEVICE_TYPE::BJT,        "Q", 8,  "HICUM_L2",        "HICUM Level 2 model" };
        //case TYPE::BJT_HICUM_L0:            return { DEVICE_TYPE::BJT,        "Q", 7,  "HICUM_L0",        "HICUM Level 0 model" };

        case TYPE::JFET_SHICHMAN_HODGES:    return { DEVICE_TYPE::JFET,       "J", 1,  "SHICHMAN_HODGES", "Shichman-Hodges model" };
        case TYPE::JFET_PARKER_SKELLERN:    return { DEVICE_TYPE::JFET,       "J", 2,  "PARKER_SKELLERN", "Parker-Skellern model" };

        case TYPE::MESFET_STATZ:            return { DEVICE_TYPE::MESFET,     "Z", 1,  "STATZ",           "Statz model" };
        case TYPE::MESFET_YTTERDAL:         return { DEVICE_TYPE::MESFET,     "Z", 2,  "YTTERDAL",        "Ytterdal model" };
        case TYPE::MESFET_HFET1:            return { DEVICE_TYPE::MESFET,     "Z", 5,  "HFET1",           "HFET1 model" };
        case TYPE::MESFET_HFET2:            return { DEVICE_TYPE::MESFET,     "Z", 6,  "HFET2",           "HFET2 model" };

        case TYPE::MOSFET_MOS1:             return { DEVICE_TYPE::MOSFET,     "M", 1,  "MOS1",            "Classical quadratic model (MOS1)" };
        case TYPE::MOSFET_MOS2:             return { DEVICE_TYPE::MOSFET,     "M", 2,  "MOS2",            "Grove-Frohman model (MOS2)" };
        case TYPE::MOSFET_MOS3:             return { DEVICE_TYPE::MOSFET,     "M", 3,  "MOS3",            "MOS3 model" };
        case TYPE::MOSFET_BSIM1:            return { DEVICE_TYPE::MOSFET,     "M", 4,  "BSIM1",           "BSIM1 model" };
        case TYPE::MOSFET_BSIM2:            return { DEVICE_TYPE::MOSFET,     "M", 5,  "BSIM2",           "BSIM2 model" };
        case TYPE::MOSFET_MOS6:             return { DEVICE_TYPE::MOSFET,     "M", 6,  "MOS6",            "MOS6 model" };
        case TYPE::MOSFET_BSIM3:            return { DEVICE_TYPE::MOSFET,     "M", 8,  "BSIM3",           "BSIM3 model" };
        case TYPE::MOSFET_MOS9:             return { DEVICE_TYPE::MOSFET,     "M", 9,  "MOS9",            "MOS9 model" };
        case TYPE::MOSFET_B4SOI:            return { DEVICE_TYPE::MOSFET,     "M", 10, "B4SOI",           "BSIM4 SOI model (B4SOI)" };
        case TYPE::MOSFET_BSIM4:            return { DEVICE_TYPE::MOSFET,     "M", 14, "BSIM4",           "BSIM4 model" };
        //case TYPE::MOSFET_EKV2_6:           return { DEVICE_TYPE::MOSFET,     "M", 44, "EKV2.6",          "EKV2.6 model" };
        //case TYPE::MOSFET_PSP:              return { DEVICE_TYPE::MOSFET,     "M", 45, "PSP",             "PSP model" };
        case TYPE::MOSFET_B3SOIFD:          return { DEVICE_TYPE::MOSFET,     "M", 55, "B3SOIFD",         "B3SOIFD (BSIM3 fully depleted SOI) model" };
        case TYPE::MOSFET_B3SOIDD:          return { DEVICE_TYPE::MOSFET,     "M", 56, "B3SOIDD",         "B3SOIDD (BSIM3 SOI, both fully and partially depleted) model" };
        case TYPE::MOSFET_B3SOIPD:          return { DEVICE_TYPE::MOSFET,     "M", 57, "B3SOIPD",         "B3SOIPD (BSIM3 partially depleted SOI) model" };
        //case TYPE::MOSFET_STAG:             return { DEVICE_TYPE::MOSFET,     "M", 60, "STAG",            "STAG model" };
        case TYPE::MOSFET_HISIM2:           return { DEVICE_TYPE::MOSFET,     "M", 68, "HiSIM2",          "HiSIM2 model" };
        case TYPE::MOSFET_HISIM_HV:         return { DEVICE_TYPE::MOSFET,     "M", 73, "HiSIM_HV",        "HiSIM_HV model" };

        case TYPE::VSOURCE:                 return { DEVICE_TYPE::VSOURCE,    "V", 0,  "",                "" };
        case TYPE::ISOURCE:                 return { DEVICE_TYPE::ISOURCE,    "V", 0,  "",                "" };

        case TYPE::SUBCIRCUIT:              return { DEVICE_TYPE::SUBCIRCUIT, "X", 0,  "",                "" };
        case TYPE::CODEMODEL:               return { DEVICE_TYPE::CODEMODEL,  "A", 0,  "",                "" };
        case TYPE::RAWSPICE:                return { DEVICE_TYPE::RAWSPICE,   "",  0,  "",                "" };

        case TYPE::_ENUM_END:               break;
         }

        wxFAIL;
        return {  };
     }

    template <typename T>
    static TYPE ReadTypeFromFields( const std::vector<T>* aFields );


    SPICE_MODEL( TYPE aType );

    template <typename T>
    SPICE_MODEL( const std::vector<T>* aFields );

    SPICE_MODEL( const wxString& aCode );


    template <typename T>
    void WriteFields( std::vector<T>* aFields );

    void WriteCode( wxString& aCode );


    wxString GetFile() { return m_file; }
    void SetFile( const wxString& aFile ) { m_file = aFile; }


private:
    TYPE m_type;
    wxString m_file;
    std::map<wxString, double> m_params;


    template <typename T>
    static wxString getFieldValue( const std::vector<T>* aFields, const wxString& aFieldName );

    template <typename T>
    static void setFieldValue( std::vector<T>* aFields, const wxString& aFieldName,
                               const wxString& aValue );


    wxString generateParamValuePairs();
    void parseParamValuePairs( const wxString& aParamValuePairs );
};

#endif /* SPICE_MODEL_H */

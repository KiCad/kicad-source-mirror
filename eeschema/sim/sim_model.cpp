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

#include <iterator>
#include <sim/sim_model.h>
#include <sim/sim_model_ideal.h>
#include <sim/sim_model_behavioral.h>
#include <sim/sim_model_source.h>
#include <sim/sim_model_subcircuit.h>
#include <sim/sim_model_codemodel.h>
#include <sim/sim_model_rawspice.h>
#include <sim/sim_model_ngspice.h>
#include <pegtl.hpp>
#include <pegtl/contrib/parse_tree.hpp>
#include <locale_io.h>
#include <lib_symbol.h>

using DEVICE_TYPE = SIM_MODEL::DEVICE_TYPE;
using TYPE = SIM_MODEL::TYPE;


namespace SIM_MODEL_PARSER
{
    using namespace SIM_MODEL_GRAMMAR;


    template <typename Rule> struct paramValuePairsSelector : std::false_type {};

    template <> struct paramValuePairsSelector<param> : std::true_type {};
    template <> struct paramValuePairsSelector<number<SIM_VALUE_BASE::TYPE::INT, NOTATION::SI>>
        : std::true_type {};
    template <> struct paramValuePairsSelector<number<SIM_VALUE_BASE::TYPE::FLOAT, NOTATION::SI>>
        : std::true_type {};
    template <> struct paramValuePairsSelector<number<SIM_VALUE_BASE::TYPE::INT, NOTATION::SPICE>>
        : std::true_type {};
    template <> struct paramValuePairsSelector<number<SIM_VALUE_BASE::TYPE::FLOAT, NOTATION::SPICE>>
        : std::true_type {};


    template <typename Rule> struct spiceUnitSelector : std::false_type {};

    template <> struct spiceUnitSelector<modelName> : std::true_type {};
    template <> struct spiceUnitSelector<dotModel> : std::true_type {};
    template <> struct spiceUnitSelector<dotModelType> : std::true_type {};
    template <> struct spiceUnitSelector<param> : std::true_type {};
    template <> struct spiceUnitSelector<number<SIM_VALUE_BASE::TYPE::INT, NOTATION::SI>>
        : std::true_type {};
    template <> struct spiceUnitSelector<number<SIM_VALUE_BASE::TYPE::FLOAT, NOTATION::SI>>
        : std::true_type {};
    template <> struct spiceUnitSelector<number<SIM_VALUE_BASE::TYPE::INT, NOTATION::SPICE>>
        : std::true_type {};
    template <> struct spiceUnitSelector<number<SIM_VALUE_BASE::TYPE::FLOAT, NOTATION::SPICE>>
        : std::true_type {};

    template <> struct spiceUnitSelector<dotSubckt> : std::true_type {};


    template <typename Rule> struct pinSequenceSelector : std::false_type {};
    template <> struct pinSequenceSelector<pinNumber> : std::true_type {};
}


SIM_MODEL::DEVICE_INFO SIM_MODEL::DeviceTypeInfo( DEVICE_TYPE aDeviceType )
{
    switch( aDeviceType )
    {
    case DEVICE_TYPE::NONE:       return { "",           ""                  };
    case DEVICE_TYPE::RESISTOR:   return { "RESISTOR",   "Resistor"          };
    case DEVICE_TYPE::CAPACITOR:  return { "CAPACITOR",  "Capacitor"         };
    case DEVICE_TYPE::INDUCTOR:   return { "INDUCTOR",   "Inductor"          };
    case DEVICE_TYPE::TLINE:      return { "TLINE",      "Transmission Line" };
    case DEVICE_TYPE::SWITCH:     return { "SWITCH",     "Switch"            };

    case DEVICE_TYPE::DIODE:      return { "DIODE",      "Diode"             };
    case DEVICE_TYPE::NPN:        return { "NPN",        "NPN BJT"           };
    case DEVICE_TYPE::PNP:        return { "PNP",        "PNP BJT"           };

    case DEVICE_TYPE::NJF:        return { "NJF",        "N-Channel JFET"    };
    case DEVICE_TYPE::PJF:        return { "PJF",        "P-Channel JFET"    };

    case DEVICE_TYPE::NMOS:       return { "NMOS",       "N-Channel MOSFET"  };
    case DEVICE_TYPE::PMOS:       return { "PMOS",       "P-Channel MOSFET"  };
    case DEVICE_TYPE::NMES:       return { "NMES",       "N-Channel MESFET"  };
    case DEVICE_TYPE::PMES:       return { "PMES",       "P-Channel MESFET"  };

    case DEVICE_TYPE::VSOURCE:    return { "VSOURCE",    "Voltage Source"    };
    case DEVICE_TYPE::ISOURCE:    return { "ISOURCE",    "Current Source"    };

    case DEVICE_TYPE::SUBCIRCUIT: return { "SUBCIRCUIT", "Subcircuit"        };
    case DEVICE_TYPE::CODEMODEL:  return { "CODEMODEL",  "Code Model"        };
    case DEVICE_TYPE::RAWSPICE:   return { "RAWSPICE",   "Raw Spice Element" };
    case DEVICE_TYPE::_ENUM_END:  break;
    }

    wxFAIL;
    return {};
}


SIM_MODEL::INFO SIM_MODEL::TypeInfo( TYPE aType )
{
    switch( aType )
    { 
    case TYPE::NONE:                   return { DEVICE_TYPE::NONE,       "",                ""                           };

    case TYPE::RESISTOR_IDEAL:         return { DEVICE_TYPE::RESISTOR,   "IDEAL",           "Ideal"                      };
    case TYPE::RESISTOR_ADVANCED:      return { DEVICE_TYPE::RESISTOR,   "ADVANCED",        "Advanced"                   };
    case TYPE::RESISTOR_BEHAVIORAL:    return { DEVICE_TYPE::RESISTOR,   "BEHAVIORAL",      "Behavioral"                 };

    case TYPE::CAPACITOR_IDEAL:        return { DEVICE_TYPE::CAPACITOR,  "IDEAL",           "Ideal"                      };
    case TYPE::CAPACITOR_ADVANCED:     return { DEVICE_TYPE::CAPACITOR,  "ADVANCED",        "Advanced"                   };
    case TYPE::CAPACITOR_BEHAVIORAL:   return { DEVICE_TYPE::CAPACITOR,  "BEHAVIORAL",      "Behavioral"                 };

    case TYPE::INDUCTOR_IDEAL:         return { DEVICE_TYPE::INDUCTOR,   "IDEAL",           "Ideal"                      };
    case TYPE::INDUCTOR_ADVANCED:      return { DEVICE_TYPE::INDUCTOR,   "ADVANCED",        "Advanced"                   };
    case TYPE::INDUCTOR_BEHAVIORAL:    return { DEVICE_TYPE::INDUCTOR,   "BEHAVIORAL",      "Behavioral"                 };

    case TYPE::TLINE_LOSSY:            return { DEVICE_TYPE::TLINE,      "LOSSY",           "Lossy"                      };
    case TYPE::TLINE_LOSSLESS:         return { DEVICE_TYPE::TLINE,      "LOSSLESS",        "Lossless"                   };
    case TYPE::TLINE_UNIFORM_RC:       return { DEVICE_TYPE::TLINE,      "UNIFORM_RC",      "Uniform RC"                 };
    case TYPE::TLINE_KSPICE:           return { DEVICE_TYPE::TLINE,      "KSPICE",          "KSPICE"                     };

    case TYPE::SWITCH_VCTRL:           return { DEVICE_TYPE::SWITCH,     "VCTRL",           "Voltage-controlled"         };
    case TYPE::SWITCH_ICTRL:           return { DEVICE_TYPE::SWITCH,     "ICTRL",           "Current-controlled"         };

    case TYPE::DIODE:                  return { DEVICE_TYPE::DIODE,      "",                ""                           };
    
    case TYPE::NPN_GUMMEL_POON:        return { DEVICE_TYPE::NPN,        "GUMMEL_POON",     "Gummel-Poon"                };
    case TYPE::PNP_GUMMEL_POON:        return { DEVICE_TYPE::PNP,        "GUMMEL_POON",     "Gummel-Poon"                };
    case TYPE::NPN_VBIC:               return { DEVICE_TYPE::NPN,        "VBIC",            "VBIC"                       };
    case TYPE::PNP_VBIC:               return { DEVICE_TYPE::PNP,        "VBIC",            "VBIC"                       };
    //case TYPE::BJT_MEXTRAM:          return {};
    case TYPE::NPN_HICUM_L2:           return { DEVICE_TYPE::NPN,        "HICUM_L2",        "HICUM Level 2"              };
    case TYPE::PNP_HICUM_L2:           return { DEVICE_TYPE::PNP,        "HICUM_L2",        "HICUM Level 2"              };
    //case TYPE::BJT_HICUM_L0:         return {};

    case TYPE::NJF_SHICHMAN_HODGES:    return { DEVICE_TYPE::NJF,        "SHICHMAN_HODGES", "Shichman-Hodges"            };
    case TYPE::PJF_SHICHMAN_HODGES:    return { DEVICE_TYPE::PJF,        "SHICHMAN_HODGES", "Shichman-Hodges"            };
    case TYPE::NJF_PARKER_SKELLERN:    return { DEVICE_TYPE::NJF,        "PARKER_SKELLERN", "Parker-Skellern"            };
    case TYPE::PJF_PARKER_SKELLERN:    return { DEVICE_TYPE::PJF,        "PARKER_SKELLERN", "Parker-Skellern"            };

    case TYPE::NMES_STATZ:             return { DEVICE_TYPE::NMES,       "STATZ",           "Statz"                      };
    case TYPE::PMES_STATZ:             return { DEVICE_TYPE::PMES,       "STATZ",           "Statz"                      };
    case TYPE::NMES_YTTERDAL:          return { DEVICE_TYPE::NMES,       "YTTERDAL",        "Ytterdal"                   };
    case TYPE::PMES_YTTERDAL:          return { DEVICE_TYPE::PMES,       "YTTERDAL",        "Ytterdal"                   };
    case TYPE::NMES_HFET1:             return { DEVICE_TYPE::NMES,       "HFET1",           "HFET1"                      };
    case TYPE::PMES_HFET1:             return { DEVICE_TYPE::PMES,       "HFET1",           "HFET1"                      };
    case TYPE::PMES_HFET2:             return { DEVICE_TYPE::NMES,       "HFET2",           "HFET2"                      };
    case TYPE::NMES_HFET2:             return { DEVICE_TYPE::PMES,       "HFET2",           "HFET2"                      };

    case TYPE::NMOS_MOS1:              return { DEVICE_TYPE::NMOS,       "MOS1",            "Classical quadratic (MOS1)" };
    case TYPE::PMOS_MOS1:              return { DEVICE_TYPE::PMOS,       "MOS1",            "Classical quadratic (MOS1)" };
    case TYPE::NMOS_MOS2:              return { DEVICE_TYPE::NMOS,       "MOS2",            "Grove-Frohman (MOS2)"       };
    case TYPE::PMOS_MOS2:              return { DEVICE_TYPE::PMOS,       "MOS2",            "Grove-Frohman (MOS2)"       };
    case TYPE::NMOS_MOS3:              return { DEVICE_TYPE::NMOS,       "MOS3",            "MOS3"                       };
    case TYPE::PMOS_MOS3:              return { DEVICE_TYPE::PMOS,       "MOS3",            "MOS3"                       };
    case TYPE::NMOS_BSIM1:             return { DEVICE_TYPE::NMOS,       "BSIM1",           "BSIM1"                      };
    case TYPE::PMOS_BSIM1:             return { DEVICE_TYPE::PMOS,       "BSIM1",           "BSIM1"                      };
    case TYPE::NMOS_BSIM2:             return { DEVICE_TYPE::NMOS,       "BSIM2",           "BSIM2"                      };
    case TYPE::PMOS_BSIM2:             return { DEVICE_TYPE::PMOS,       "BSIM2",           "BSIM2"                      };
    case TYPE::NMOS_MOS6:              return { DEVICE_TYPE::NMOS,       "MOS6",            "MOS6"                       };
    case TYPE::PMOS_MOS6:              return { DEVICE_TYPE::PMOS,       "MOS6",            "MOS6"                       };
    case TYPE::NMOS_BSIM3:             return { DEVICE_TYPE::NMOS,       "BSIM3",           "BSIM3"                      };
    case TYPE::PMOS_BSIM3:             return { DEVICE_TYPE::PMOS,       "BSIM3",           "BSIM3"                      };
    case TYPE::NMOS_MOS9:              return { DEVICE_TYPE::NMOS,       "MOS9",            "MOS9"                       };
    case TYPE::PMOS_MOS9:              return { DEVICE_TYPE::PMOS,       "MOS9",            "MOS9"                       };
    case TYPE::NMOS_B4SOI:             return { DEVICE_TYPE::NMOS,       "B4SOI",           "BSIM4 SOI (B4SOI)"          };
    case TYPE::PMOS_B4SOI:             return { DEVICE_TYPE::PMOS,       "B4SOI",           "BSIM4 SOI (B4SOI)"          };
    case TYPE::NMOS_BSIM4:             return { DEVICE_TYPE::NMOS,       "BSIM4",           "BSIM4"                      };
    case TYPE::PMOS_BSIM4:             return { DEVICE_TYPE::PMOS,       "BSIM4",           "BSIM4"                      };
    //case TYPE::NMOS_EKV2_6:          return {};
    //case TYPE::PMOS_EKV2_6:          return {};
    //case TYPE::NMOS_PSP:             return {};
    //case TYPE::PMOS_PSP:             return {};
    case TYPE::NMOS_B3SOIFD:           return { DEVICE_TYPE::NMOS,       "B3SOIFD",         "B3SOIFD (BSIM3 FD-SOI)"     };
    case TYPE::PMOS_B3SOIFD:           return { DEVICE_TYPE::PMOS,       "B3SOIFD",         "B3SOIFD (BSIM3 FD-SOI)"     };
    case TYPE::NMOS_B3SOIDD:           return { DEVICE_TYPE::NMOS,       "B3SOIDD",         "B3SOIDD (BSIM3 SOI)"        };
    case TYPE::PMOS_B3SOIDD:           return { DEVICE_TYPE::PMOS,       "B3SOIDD",         "B3SOIDD (BSIM3 SOI)"        };
    case TYPE::NMOS_B3SOIPD:           return { DEVICE_TYPE::NMOS,       "B3SOIPD",         "B3SOIPD (BSIM3 PD-SOI)"     };
    case TYPE::PMOS_B3SOIPD:           return { DEVICE_TYPE::PMOS,       "B3SOIPD",         "B3SOIPD (BSIM3 PD-SOI)"     };
    //case TYPE::NMOS_STAG:            return {};
    //case TYPE::PMOS_STAG:            return {};
    case TYPE::NMOS_HISIM2:            return { DEVICE_TYPE::NMOS,       "HISIM2",          "HiSIM2"                     };
    case TYPE::PMOS_HISIM2:            return { DEVICE_TYPE::PMOS,       "HISIM2",          "HiSIM2"                     };
    case TYPE::NMOS_HISIM_HV1:         return { DEVICE_TYPE::NMOS,       "HISIM_HV1",       "HiSIM_HV1"                  };
    case TYPE::PMOS_HISIM_HV1:         return { DEVICE_TYPE::PMOS,       "HISIM_HV1",       "HiSIM_HV1"                  };
    case TYPE::NMOS_HISIM_HV2:         return { DEVICE_TYPE::NMOS,       "HISIM_HV2",       "HiSIM_HV2"                  };
    case TYPE::PMOS_HISIM_HV2:         return { DEVICE_TYPE::PMOS,       "HISIM_HV2",       "HiSIM_HV2"                  };

    case TYPE::VSOURCE_PULSE:          return { DEVICE_TYPE::VSOURCE,    "PULSE",           "Pulse"                      };
    case TYPE::VSOURCE_SIN:            return { DEVICE_TYPE::VSOURCE,    "SINCE",           "Sine"                       };
    case TYPE::VSOURCE_EXP:            return { DEVICE_TYPE::VSOURCE,    "EXP",             "Exponential"                };
    case TYPE::VSOURCE_SFAM:           return { DEVICE_TYPE::VSOURCE,    "SFAM",            "Single-frequency AM"        };
    case TYPE::VSOURCE_SFFM:           return { DEVICE_TYPE::VSOURCE,    "SFFM",            "Single-frequency FM"        };
    case TYPE::VSOURCE_PWL:            return { DEVICE_TYPE::VSOURCE,    "PWL",             "Piecewise linear"           };
    case TYPE::VSOURCE_WHITE_NOISE:    return { DEVICE_TYPE::VSOURCE,    "WHITE_NOISE",     "White Noise"                };
    case TYPE::VSOURCE_PINK_NOISE:     return { DEVICE_TYPE::VSOURCE,    "PINK_NOISE",      "Pink Noise (1/f)"           };
    case TYPE::VSOURCE_BURST_NOISE:    return { DEVICE_TYPE::VSOURCE,    "BURST_NOISE",     "Burst Noise"                };
    case TYPE::VSOURCE_RANDOM_UNIFORM: return { DEVICE_TYPE::VSOURCE,    "RANDOM_UNIFORM",  "Random uniform"             };
    case TYPE::VSOURCE_RANDOM_NORMAL:  return { DEVICE_TYPE::VSOURCE,    "RANDOM_NORMAL",   "Random normal"              };
    case TYPE::VSOURCE_RANDOM_EXP:     return { DEVICE_TYPE::VSOURCE,    "RANDOM_EXP",      "Random exponential"         };
    case TYPE::VSOURCE_RANDOM_POISSON: return { DEVICE_TYPE::VSOURCE,    "RANDOM_POISSON",  "Random Poisson"             };
    case TYPE::VSOURCE_BEHAVIORAL:     return { DEVICE_TYPE::VSOURCE,    "BEHAVIORAL",      "Behavioral"                 };

    case TYPE::ISOURCE_PULSE:          return { DEVICE_TYPE::ISOURCE,    "PULSE",           "Pulse"                      };
    case TYPE::ISOURCE_SIN:            return { DEVICE_TYPE::ISOURCE,    "SINCE",           "Sine"                       };
    case TYPE::ISOURCE_EXP:            return { DEVICE_TYPE::ISOURCE,    "EXP",             "Exponential"                };
    case TYPE::ISOURCE_SFAM:           return { DEVICE_TYPE::ISOURCE,    "SFAM",            "Single-frequency AM"        };
    case TYPE::ISOURCE_SFFM:           return { DEVICE_TYPE::ISOURCE,    "SFFM",            "Single-frequency FM"        };
    case TYPE::ISOURCE_PWL:            return { DEVICE_TYPE::ISOURCE,    "PWL",             "Piecewise linear"           };
    case TYPE::ISOURCE_WHITE_NOISE:    return { DEVICE_TYPE::ISOURCE,    "WHITE_NOISE",     "White Noise"                };
    case TYPE::ISOURCE_PINK_NOISE:     return { DEVICE_TYPE::ISOURCE,    "PINK_NOISE",      "Pink Noise (1/f)"           };
    case TYPE::ISOURCE_BURST_NOISE:    return { DEVICE_TYPE::ISOURCE,    "BURST_NOISE",     "Burst Noise"                };
    case TYPE::ISOURCE_RANDOM_UNIFORM: return { DEVICE_TYPE::ISOURCE,    "RANDOM_UNIFORM",  "Random uniform"             };
    case TYPE::ISOURCE_RANDOM_NORMAL:  return { DEVICE_TYPE::ISOURCE,    "RANDOM_NORMAL",   "Random normal"              };
    case TYPE::ISOURCE_RANDOM_EXP:     return { DEVICE_TYPE::ISOURCE,    "RANDOM_EXP",      "Random exponential"         };
    case TYPE::ISOURCE_RANDOM_POISSON: return { DEVICE_TYPE::ISOURCE,    "RANDOM_POISSON",  "Random Poisson"             };
    case TYPE::ISOURCE_BEHAVIORAL:     return { DEVICE_TYPE::ISOURCE,    "BEHAVIORAL",      "Behavioral"                 };

    case TYPE::SUBCIRCUIT:             return { DEVICE_TYPE::SUBCIRCUIT, "",                ""                           };
    case TYPE::CODEMODEL:              return { DEVICE_TYPE::CODEMODEL,  "",                ""                           };
    case TYPE::RAWSPICE:               return { DEVICE_TYPE::RAWSPICE,   "",                ""                           };

    case TYPE::_ENUM_END:              break;
    }

    wxFAIL;
    return {};
}


SIM_MODEL::SPICE_INFO SIM_MODEL::SpiceInfo( TYPE aType )
{
    switch( aType )
    {
    case TYPE::RESISTOR_IDEAL:         return { "R", ""        };
    case TYPE::RESISTOR_ADVANCED:      return { "R", "R"       };
    case TYPE::RESISTOR_BEHAVIORAL:    return { "R", "",       "",        0,  true   };

    case TYPE::CAPACITOR_IDEAL:        return { "C", ""        };
    case TYPE::CAPACITOR_ADVANCED:     return { "C", "C",      };
    case TYPE::CAPACITOR_BEHAVIORAL:   return { "C", "",       "",        0,  true   };

    case TYPE::INDUCTOR_IDEAL:         return { "L", ""        };
    case TYPE::INDUCTOR_ADVANCED:      return { "L", "L"       };
    case TYPE::INDUCTOR_BEHAVIORAL:    return { "L", "",       "",        0,  true   };
    
    case TYPE::TLINE_LOSSY:            return { "O", "LTRA"    };
    case TYPE::TLINE_LOSSLESS:         return { "T"  };
    case TYPE::TLINE_UNIFORM_RC:       return { "U"  };
    case TYPE::TLINE_KSPICE:           return { "Y"  };
    
    case TYPE::SWITCH_VCTRL:           return { "S", "switch"  };
    case TYPE::SWITCH_ICTRL:           return { "W", "cswitch" };

    case TYPE::DIODE:                  return { "D", "D"       };

    case TYPE::NPN_GUMMEL_POON:        return { "Q", "NPN",    "",        1   };
    case TYPE::PNP_GUMMEL_POON:        return { "Q", "PNP",    "",        1   };

    case TYPE::NPN_VBIC:               return { "Q", "NPN",    "",        4   };
    case TYPE::PNP_VBIC:               return { "Q", "PNP",    "",        4   };

    case TYPE::NPN_HICUM_L2:           return { "Q", "NPN",    "",        8   };
    case TYPE::PNP_HICUM_L2:           return { "Q", "PNP",    "",        8   };

    case TYPE::NJF_SHICHMAN_HODGES:    return { "M", "NJF",    "",        1   };
    case TYPE::PJF_SHICHMAN_HODGES:    return { "M", "PJF",    "",        1   };
    case TYPE::NJF_PARKER_SKELLERN:    return { "M", "NJF",    "",        2   };
    case TYPE::PJF_PARKER_SKELLERN:    return { "M", "PJF",    "",        2   };

    case TYPE::NMES_STATZ:             return { "Z", "NMF",    "",        1   };
    case TYPE::PMES_STATZ:             return { "Z", "PMF",    "",        1   };
    case TYPE::NMES_YTTERDAL:          return { "Z", "NMF",    "",        2   };
    case TYPE::PMES_YTTERDAL:          return { "Z", "PMF",    "",        2   };
    case TYPE::NMES_HFET1:             return { "Z", "NMF",    "",        5   };
    case TYPE::PMES_HFET1:             return { "Z", "PMF",    "",        5   };
    case TYPE::PMES_HFET2:             return { "Z", "NMF",    "",        6   };
    case TYPE::NMES_HFET2:             return { "Z", "PMF",    "",        6   };

    case TYPE::NMOS_MOS1:              return { "M", "NMOS",   "",        1   };
    case TYPE::PMOS_MOS1:              return { "M", "PMOS",   "",        1   };
    case TYPE::NMOS_MOS2:              return { "M", "NMOS",   "",        2   };
    case TYPE::PMOS_MOS2:              return { "M", "PMOS",   "",        2   };
    case TYPE::NMOS_MOS3:              return { "M", "NMOS",   "",        3   };
    case TYPE::PMOS_MOS3:              return { "M", "PMOS",   "",        3   };
    case TYPE::NMOS_BSIM1:             return { "M", "NMOS",   "",        4   };
    case TYPE::PMOS_BSIM1:             return { "M", "PMOS",   "",        4   };
    case TYPE::NMOS_BSIM2:             return { "M", "NMOS",   "",        5   };
    case TYPE::PMOS_BSIM2:             return { "M", "PMOS",   "",        5   };
    case TYPE::NMOS_MOS6:              return { "M", "NMOS",   "",        6   };
    case TYPE::PMOS_MOS6:              return { "M", "PMOS",   "",        6   };
    case TYPE::NMOS_BSIM3:             return { "M", "NMOS",   "",        8   };
    case TYPE::PMOS_BSIM3:             return { "M", "PMOS",   "",        8   };
    case TYPE::NMOS_MOS9:              return { "M", "NMOS",   "",        9   };
    case TYPE::PMOS_MOS9:              return { "M", "PMOS",   "",        9   };
    case TYPE::NMOS_B4SOI:             return { "M", "NMOS",   "",        10  };
    case TYPE::PMOS_B4SOI:             return { "M", "PMOS",   "",        10  };
    case TYPE::NMOS_BSIM4:             return { "M", "NMOS",   "",        14  };
    case TYPE::PMOS_BSIM4:             return { "M", "PMOS",   "",        14  };
    //case TYPE::NMOS_EKV2_6:          return {};
    //case TYPE::PMOS_EKV2_6:          return {};
    //case TYPE::NMOS_PSP:             return {};
    //case TYPE::PMOS_PSP:             return {};
    case TYPE::NMOS_B3SOIFD:           return { "M", "NMOS",   "",        55  };
    case TYPE::PMOS_B3SOIFD:           return { "M", "PMOS",   "",        55  };
    case TYPE::NMOS_B3SOIDD:           return { "M", "NMOS",   "",        56  };
    case TYPE::PMOS_B3SOIDD:           return { "M", "PMOS",   "",        56  };
    case TYPE::NMOS_B3SOIPD:           return { "M", "NMOS",   "",        57  };
    case TYPE::PMOS_B3SOIPD:           return { "M", "PMOS",   "",        57  };
    //case TYPE::NMOS_STAG:            return {};
    //case TYPE::PMOS_STAG:            return {};
    case TYPE::NMOS_HISIM2:            return { "M", "NMOS",   "",        68  };
    case TYPE::PMOS_HISIM2:            return { "M", "PMOS",   "",        68  };
    case TYPE::NMOS_HISIM_HV1:         return { "M", "NMOS",   "",        73, false, "1.2.4" };
    case TYPE::PMOS_HISIM_HV1:         return { "M", "PMOS",   "",        73, false, "1.2.4" };
    case TYPE::NMOS_HISIM_HV2:         return { "M", "NMOS",   "",        73, false, "2.2.0" };
    case TYPE::PMOS_HISIM_HV2:         return { "M", "PMOS",   "",        73, false, "2.2.0" };

    case TYPE::VSOURCE_PULSE:          return { "V", "",       "PULSE"    };
    case TYPE::VSOURCE_SIN:            return { "V", "",       "SIN"      };
    case TYPE::VSOURCE_EXP:            return { "V", "",       "EXP"      };
    case TYPE::VSOURCE_SFAM:           return { "V", "",       "AM"       };
    case TYPE::VSOURCE_SFFM:           return { "V", "",       "SFFM"     };
    case TYPE::VSOURCE_PWL:            return { "V", "",       "PWL"      };
    case TYPE::VSOURCE_WHITE_NOISE:    return { "V", "",       "TRNOISE"  };
    case TYPE::VSOURCE_PINK_NOISE:     return { "V", "",       "TRNOISE"  };
    case TYPE::VSOURCE_BURST_NOISE:    return { "V", "",       "TRNOISE"  };
    case TYPE::VSOURCE_RANDOM_UNIFORM: return { "V", "",       "TRRANDOM" };
    case TYPE::VSOURCE_RANDOM_NORMAL:  return { "V", "",       "TRRANDOM" };
    case TYPE::VSOURCE_RANDOM_EXP:     return { "V", "",       "TRRANDOM" };
    case TYPE::VSOURCE_RANDOM_POISSON: return { "V", "",       "TRRANDOM" };
    case TYPE::VSOURCE_BEHAVIORAL:     return { "B"  };

    case TYPE::ISOURCE_PULSE:          return { "V", "",       "PULSE"    };
    case TYPE::ISOURCE_SIN:            return { "V", "",       "SIN"      };
    case TYPE::ISOURCE_EXP:            return { "V", "",       "EXP"      };
    case TYPE::ISOURCE_SFAM:           return { "V", "",       "AM"       };
    case TYPE::ISOURCE_SFFM:           return { "V", "",       "SFFM"     };
    case TYPE::ISOURCE_PWL:            return { "V", "",       "PWL"      };
    case TYPE::ISOURCE_WHITE_NOISE:    return { "V", "",       "TRNOISE"  };
    case TYPE::ISOURCE_PINK_NOISE:     return { "V", "",       "TRNOISE"  };
    case TYPE::ISOURCE_BURST_NOISE:    return { "V", "",       "TRNOISE"  };
    case TYPE::ISOURCE_RANDOM_UNIFORM: return { "V", "",       "TRRANDOM" };
    case TYPE::ISOURCE_RANDOM_NORMAL:  return { "V", "",       "TRRANDOM" };
    case TYPE::ISOURCE_RANDOM_EXP:     return { "V", "",       "TRRANDOM" };
    case TYPE::ISOURCE_RANDOM_POISSON: return { "V", "",       "TRRANDOM" };
    case TYPE::ISOURCE_BEHAVIORAL:     return { "B"  };

    case TYPE::SUBCIRCUIT:             return { "X"  };
    case TYPE::CODEMODEL:              return { "A"  };

    case TYPE::NONE:
    case TYPE::RAWSPICE:
        return {};

    case TYPE::_ENUM_END:
        break;
    }

    wxFAIL;
    return {};
}


TYPE SIM_MODEL::ReadTypeFromSpiceCode( const std::string& aSpiceCode )
{
    tao::pegtl::string_input<> in( aSpiceCode, "from_input" );
    std::unique_ptr<tao::pegtl::parse_tree::node> root;

    try
    {
        root = tao::pegtl::parse_tree::parse<SIM_MODEL_PARSER::spiceUnitGrammar,
                                             SIM_MODEL_PARSER::spiceUnitSelector>
            ( in );
    }
    catch( tao::pegtl::parse_error& e )
    {
        throw KI_PARAM_ERROR( wxString::Format( _( "Failed to parse '%s': %s" ), aSpiceCode,
                                                e.what() ) );
    }

    wxASSERT( root );

    for( const auto& node : root->children )
    {
        if( node->is_type<SIM_MODEL_PARSER::dotModel>() )
        {
            for( const auto& subnode : node->children )
            {
                if( subnode->is_type<SIM_MODEL_PARSER::modelName>() )
                {
                    // Do nothing.
                }
                else if( subnode->is_type<SIM_MODEL_PARSER::dotModelType>() )
                    return readTypeFromSpiceTypeString( subnode->string() );
                else
                {
                    wxFAIL_MSG( "Unhandled parse tree subnode" );
                    return TYPE::NONE;
                }
            }
        }
        else if( node->is_type<SIM_MODEL_PARSER::dotSubckt>() )
            return TYPE::SUBCIRCUIT;
        else
        {
            wxFAIL_MSG( "Unhandled parse tree node" );
            return TYPE::NONE;
        }
    }

    wxFAIL_MSG( "Could not derive type from Spice code" );
    return TYPE::NONE;
}


template TYPE SIM_MODEL::ReadTypeFromFields( const std::vector<SCH_FIELD>& aFields );
template TYPE SIM_MODEL::ReadTypeFromFields( const std::vector<LIB_FIELD>& aFields );

template <typename T>
TYPE SIM_MODEL::ReadTypeFromFields( const std::vector<T>& aFields )
{
    wxString typeFieldValue = GetFieldValue( &aFields, TYPE_FIELD );
    wxString deviceTypeFieldValue = GetFieldValue( &aFields, DEVICE_TYPE_FIELD );
    bool typeFound = false;

    for( TYPE type : TYPE_ITERATOR() )
    {
        if( typeFieldValue == TypeInfo( type ).fieldValue )
        {
            typeFound = true;

            if( deviceTypeFieldValue == DeviceTypeInfo( TypeInfo( type ).deviceType ).fieldValue )
                return type;
        }
    }

    // TODO: Return TYPE::NONE instead of throwing an exception.

    if( !typeFound )
        throw KI_PARAM_ERROR( wxString::Format( _( "Invalid '%s' field value: '%s'" ),
                                                TYPE_FIELD, typeFieldValue ) );

    throw KI_PARAM_ERROR( wxString::Format( _( "Invalid '%s' field value: '%s'" ),
                                            DEVICE_TYPE_FIELD, deviceTypeFieldValue ) );
}


std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( TYPE aType, int aSymbolPinCount )
{
    std::unique_ptr<SIM_MODEL> model = create( aType );

    // Passing nullptr to ReadDataFields will make it act as if all fields were empty.
    model->ReadDataFields( aSymbolPinCount, static_cast<const std::vector<void>*>( nullptr ) );
    return model;
}


std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const std::string& aSpiceCode )
{
    std::unique_ptr<SIM_MODEL> model = create( ReadTypeFromSpiceCode( aSpiceCode ) );
    
    if( !model->ReadSpiceCode( aSpiceCode ) )
    {
        // Demote to raw Spice element and try again.
        std::unique_ptr<SIM_MODEL> rawSpiceModel = create( TYPE::RAWSPICE );

        rawSpiceModel->ReadSpiceCode( aSpiceCode );
        return rawSpiceModel;
    }

    return model;
}


std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const SIM_MODEL& aBaseModel )
{
    std::unique_ptr<SIM_MODEL> model = create( aBaseModel.GetType() );

    model->SetBaseModel( aBaseModel );
    return model;
}


template std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( int aSymbolPinCount,
                                                       const std::vector<SCH_FIELD>& aFields );
template std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( int aSymbolPinCount,
                                                       const std::vector<LIB_FIELD>& aFields );

template <typename T>
std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( int aSymbolPinCount, const std::vector<T>& aFields )
{
    std::unique_ptr<SIM_MODEL> model = SIM_MODEL::create( ReadTypeFromFields( aFields ) );

    model->ReadDataFields( aSymbolPinCount, &aFields );
    return model;
}


template <typename T>
wxString SIM_MODEL::GetFieldValue( const std::vector<T>* aFields, const wxString& aFieldName )
{
    static_assert( std::is_same<T, SCH_FIELD>::value || std::is_same<T, LIB_FIELD>::value );

    if( !aFields )
        return wxEmptyString; // Should not happen, T=void specialization will be called instead.

    auto fieldIt = std::find_if( aFields->begin(), aFields->end(),
                                 [aFieldName]( const T& field )
                                 {
                                     return field.GetName() == aFieldName;
                                 } );

    if( fieldIt != aFields->end() )
        return fieldIt->GetText();

    return wxEmptyString;
}


// This specialization is used when no fields are passed.
template <>
wxString SIM_MODEL::GetFieldValue( const std::vector<void>* aFields, const wxString& aFieldName )
{
    return wxEmptyString;
}


template <typename T>
void SIM_MODEL::SetFieldValue( std::vector<T>& aFields, const wxString& aFieldName,
                               const wxString& aValue )
{
    static_assert( std::is_same<T, SCH_FIELD>::value || std::is_same<T, LIB_FIELD>::value );

    auto fieldIt = std::find_if( aFields.begin(), aFields.end(),
                                 [&]( const T& f )
                                 {
                                    return f.GetName() == aFieldName;
                                 } );

    if( fieldIt != aFields.end() )
    {
        if( aValue.IsEmpty() )
            aFields.erase( fieldIt );
        else
            fieldIt->SetText( aValue );

        return;
    }

    if( aValue.IsEmpty() )
        return;

    if constexpr( std::is_same<T, SCH_FIELD>::value )
    {
        wxASSERT( aFields.size() >= 1 );

        SCH_ITEM* parent = static_cast<SCH_ITEM*>( aFields.at( 0 ).GetParent() );
        aFields.emplace_back( wxPoint(), aFields.size(), parent, aFieldName );
    }
    else if constexpr( std::is_same<T, LIB_FIELD>::value )
        aFields.emplace_back( aFields.size(), aFieldName );

    aFields.back().SetText( aValue );
}


bool SIM_MODEL::ReadSpiceCode( const std::string& aSpiceCode )
{
    // The default behavior is to treat the Spice param=value pairs as the model parameters and
    // values (for many models the correspondence is not exact, so this function is overridden).
    
    tao::pegtl::string_input<> in( aSpiceCode, "from_input" );
    std::unique_ptr<tao::pegtl::parse_tree::node> root;
    
    try
    {
        root = tao::pegtl::parse_tree::parse<SIM_MODEL_PARSER::spiceUnitGrammar,
                                             SIM_MODEL_PARSER::spiceUnitSelector>
            ( in );
    }
    catch( tao::pegtl::parse_error& e )
    {
        return false;
    }


    wxASSERT( root );

    std::cout << "BEGIN" << std::endl; // DEBUG TRACE

    for( const auto& node : root->children )
    {
        std::cout << "node: " << node->string() << std::endl; // DEBUG TRACE
        if( node->is_type<SIM_MODEL_PARSER::dotModel>()
            || node->is_type<SIM_MODEL_PARSER::dotSubckt>() )
        {
            wxString paramName = "";

            for( const auto& subnode : node->children )
            {
                if( subnode->is_type<SIM_MODEL_PARSER::modelName>() )
                {
                    // Do nothing.
                }
                else if( subnode->is_type<SIM_MODEL_PARSER::dotModelType>() )
                {
                    // Do nothing.
                }
                else if( subnode->is_type<SIM_MODEL_PARSER::param>() )
                {
                    paramName = subnode->string();
                }
                // TODO: Do something with number<SIM_VALUE_BASE::TYPE::INT, ...>.
                // It doesn't seem too useful?
                else if( subnode->is_type<
                        SIM_MODEL_PARSER::number<SIM_VALUE_BASE::TYPE::FLOAT,
                                                 SIM_MODEL_PARSER::NOTATION::SPICE>>() )
                {
                    wxASSERT( !paramName.IsEmpty() );

                    if( !setParamFromSpiceCode( paramName, subnode->string() ) )
                        return false;
                }
                else
                {
                    wxFAIL_MSG( "Unhandled parse tree subnode" );
                    return false;
                }
            }
        }
        else
        {
            wxFAIL_MSG( "Unhandled parse tree node" );
            return false;
        }
    }

    std::cout << "END" << std::endl; // DEBUG TRACE

    m_spiceCode = aSpiceCode;
    return true;
}


template <typename T>
void SIM_MODEL::ReadDataFields( int aSymbolPinCount, const std::vector<T>* aFields )
{
    doReadDataFields( aSymbolPinCount, aFields );
}


template <>
void SIM_MODEL::ReadDataFields( int aSymbolPinCount, const std::vector<SCH_FIELD>* aFields )
{
    ReadDataSchFields( aSymbolPinCount, aFields );
}


template <>
void SIM_MODEL::ReadDataFields( int aSymbolPinCount, const std::vector<LIB_FIELD>* aFields )
{
    ReadDataLibFields( aSymbolPinCount, aFields );
}


void SIM_MODEL::ReadDataSchFields( int aSymbolPinCount, const std::vector<SCH_FIELD>* aFields )
{
    doReadDataFields( aSymbolPinCount, aFields );
}


void SIM_MODEL::ReadDataLibFields( int aSymbolPinCount, const std::vector<LIB_FIELD>* aFields )
{
    doReadDataFields( aSymbolPinCount, aFields );
}


template <>
void SIM_MODEL::WriteFields( std::vector<SCH_FIELD>& aFields )
{
    WriteDataSchFields( aFields );
}


template <>
void SIM_MODEL::WriteFields( std::vector<LIB_FIELD>& aFields )
{
    WriteDataLibFields( aFields );
}


void SIM_MODEL::WriteDataSchFields( std::vector<SCH_FIELD>& aFields )
{
    doWriteFields( aFields );
}


void SIM_MODEL::WriteDataLibFields( std::vector<LIB_FIELD>& aFields )
{
    doWriteFields( aFields );
}


wxString SIM_MODEL::GenerateSpiceIncludeLine( const wxString& aLibraryFilename ) const
{
    LOCALE_IO toggle;

    if( GetBaseModel() && !HasOverrides() )
        return wxString::Format( ".include \"%s\"\n", aLibraryFilename );

    return "";
}


wxString SIM_MODEL::GenerateSpiceModelLine( const wxString& aModelName ) const
{
    LOCALE_IO toggle;
    wxString result = "";
    wxString line = "";

    line << wxString::Format( ".model %s %s(\n+", aModelName, GetSpiceInfo().typeString );

    for( int paramIndex = 0; paramIndex < GetParamCount(); ++paramIndex )
    {
        const PARAM& param = GetParam( paramIndex );
        wxString valueStr = param.value->ToString();

        if( valueStr.IsEmpty() )
            continue;
        
        wxString append = "";

        append << " ";
        append << param.info.name;
        append << "=";
        append << param.value->ToString();

        if( line.Length() + append.Length() > 60 )
        {
            result << line + "\n";
            line = "+" + append;
        }
        else
            line << append;
    }

    result << line + ")\n";
    return result;
}


SIM_MODEL::SPICE_INFO SIM_MODEL::GetSpiceInfo() const
{
    return SpiceInfo( GetType() );
}


wxString SIM_MODEL::GenerateSpiceItemLine( const wxString& aRefName,
                                           const wxString& aModelName ) const
{
    return GenerateSpiceItemLine( aRefName, aModelName, getPinNames() );
}


wxString SIM_MODEL::GenerateSpiceItemLine( const wxString& aRefName,
                                           const wxString& aModelName,
                                           const std::vector<wxString>& aPinNetNames ) const
{
    wxString result = "";

    if( aRefName.Length() >= 1 && aRefName.StartsWith( GetSpiceInfo().itemType ) )
        result << aRefName << " ";
    else
        result << GetSpiceInfo().itemType << aRefName << " ";

    for( const wxString& pinNetName : aPinNetNames )
        result << pinNetName << " ";

    result << aModelName;

    return result;
}


wxString SIM_MODEL::GenerateSpicePreview( const wxString& aModelName ) const
{
    if( !m_spiceCode.IsEmpty() )
        return m_spiceCode; // `aModelName` is ignored in this case.

    if( GetBaseModel() && !HasOverrides() )
        return GetBaseModel()->GenerateSpicePreview( aModelName );

    wxString modelLine = GenerateSpiceModelLine( aModelName );

    if( !modelLine.IsEmpty() )
        return modelLine;

    return GenerateSpiceItemLine( "", aModelName );
}


void SIM_MODEL::AddParam( const PARAM::INFO& aInfo, bool aIsOtherVariant )
{
    m_params.emplace_back( aInfo );
}


const SIM_MODEL::PARAM& SIM_MODEL::GetParam( int aParamIndex ) const
{
    if( m_baseModel && m_params.at( aParamIndex ).value->ToString().IsEmpty() )
        return m_baseModel->GetParam( aParamIndex );
    else
        return m_params.at( aParamIndex );
}


const SIM_MODEL::PARAM& SIM_MODEL::GetUnderlyingParam( int aParamIndex ) const
{
    return m_params.at( aParamIndex );
}


const SIM_MODEL::PARAM& SIM_MODEL::GetBaseParam( int aParamIndex ) const
{
    if( m_baseModel )
        return m_baseModel->GetParam( aParamIndex );
    else
        return m_params.at( aParamIndex );
}


bool SIM_MODEL::SetParamValue( int aParamIndex, const wxString& aValue )
{
    // Models sourced from a library are immutable.
    if( !m_spiceCode.IsEmpty() )
        return false;

    m_params.at( aParamIndex ).value->FromString( aValue );
    return true;
}


bool SIM_MODEL::HasOverrides() const
{
    for( const PARAM& param : m_params )
    {
        if( !param.value->ToString().IsEmpty() )
            return true;
    }

    return false;
}


bool SIM_MODEL::HasNonPrincipalOverrides() const
{
    for( const PARAM& param : m_params )
    {
        if( param.info.category != PARAM::CATEGORY::PRINCIPAL
            && !param.value->ToString().IsEmpty() )
        {
            return true;
        }
    }

    return false;
}


SIM_MODEL::SIM_MODEL( TYPE aType ) : m_baseModel( nullptr ), m_type( aType )
{
}


std::unique_ptr<SIM_MODEL> SIM_MODEL::create( TYPE aType )
{
    switch( aType )
    {
    case TYPE::RESISTOR_IDEAL:
    case TYPE::CAPACITOR_IDEAL:
    case TYPE::INDUCTOR_IDEAL:
        return std::make_unique<SIM_MODEL_IDEAL>( aType );

    case TYPE::RESISTOR_BEHAVIORAL:
    case TYPE::CAPACITOR_BEHAVIORAL:
    case TYPE::INDUCTOR_BEHAVIORAL:
    case TYPE::VSOURCE_BEHAVIORAL:
    case TYPE::ISOURCE_BEHAVIORAL:
        return std::make_unique<SIM_MODEL_BEHAVIORAL>( aType );

    case TYPE::VSOURCE_PULSE:
    case TYPE::ISOURCE_PULSE:
    case TYPE::VSOURCE_SIN:
    case TYPE::ISOURCE_SIN:
    case TYPE::VSOURCE_EXP:
    case TYPE::ISOURCE_EXP:
    case TYPE::VSOURCE_SFAM:
    case TYPE::ISOURCE_SFAM:
    case TYPE::VSOURCE_SFFM:
    case TYPE::ISOURCE_SFFM:
    case TYPE::VSOURCE_PWL:
    case TYPE::ISOURCE_PWL:
    case TYPE::VSOURCE_WHITE_NOISE:
    case TYPE::ISOURCE_WHITE_NOISE:
    case TYPE::VSOURCE_PINK_NOISE:
    case TYPE::ISOURCE_PINK_NOISE:
    case TYPE::VSOURCE_BURST_NOISE:
    case TYPE::ISOURCE_BURST_NOISE:
    case TYPE::VSOURCE_RANDOM_UNIFORM:
    case TYPE::ISOURCE_RANDOM_UNIFORM:
    case TYPE::VSOURCE_RANDOM_NORMAL:
    case TYPE::ISOURCE_RANDOM_NORMAL:
    case TYPE::VSOURCE_RANDOM_EXP:
    case TYPE::ISOURCE_RANDOM_EXP:
    case TYPE::VSOURCE_RANDOM_POISSON:
    case TYPE::ISOURCE_RANDOM_POISSON:
        return std::make_unique<SIM_MODEL_SOURCE>( aType );

    case TYPE::SUBCIRCUIT:
        return std::make_unique<SIM_MODEL_SUBCIRCUIT>( aType );

    case TYPE::CODEMODEL:
        return std::make_unique<SIM_MODEL_CODEMODEL>( aType );

    case TYPE::RAWSPICE:
        return std::make_unique<SIM_MODEL_RAWSPICE>( aType );

    default:
        return std::make_unique<SIM_MODEL_NGSPICE>( aType );
    }
}


TYPE SIM_MODEL::readTypeFromSpiceTypeString( const std::string& aTypeString )
{
    for( TYPE type : TYPE_ITERATOR() )
    {
        if( SpiceInfo( type ).typeString == aTypeString )
            return type;
    }

    // If the type string is not recognized, demote to raw Spice element. This way the user won't
    // have an error if there is a type KiCad does not recognize.
    return TYPE::RAWSPICE;
}


template <typename T>
void SIM_MODEL::doReadDataFields( int aSymbolPinCount, const std::vector<T>* aFields )
{
    parsePinsField( aSymbolPinCount, GetFieldValue( aFields, PINS_FIELD ) );
    parseParamsField( GetFieldValue( aFields, PARAMS_FIELD ) );
}


template <typename T>
void SIM_MODEL::doWriteFields( std::vector<T>& aFields )
{
    SetFieldValue( aFields, DEVICE_TYPE_FIELD, generateDeviceTypeField() );
    SetFieldValue( aFields, TYPE_FIELD, generateTypeField() );
    SetFieldValue( aFields, PINS_FIELD, generatePinsField() );
    SetFieldValue( aFields, PARAMS_FIELD, generateParamsField( " " ) );
}


wxString SIM_MODEL::generateDeviceTypeField() const
{
    return DeviceTypeInfo( TypeInfo( m_type ).deviceType ).fieldValue;
}


wxString SIM_MODEL::generateTypeField() const
{
    return TypeInfo( m_type ).fieldValue;
}


wxString SIM_MODEL::generatePinsField() const
{
    wxString result = "";
    bool isFirst = true;

    for( int i = 0; i < GetPinCount(); ++i )
    {
        if( isFirst )
            isFirst = false;
        else
            result << " ";

        if( GetPin( i ).symbolPinNumber == PIN::NOT_CONNECTED )
            result << "X";
        else
            result << GetPin( i ).symbolPinNumber;
    }

    return result;
}


void SIM_MODEL::parsePinsField( int aSymbolPinCount, const wxString& aPinsField )
{
    // Default pin sequence: model pins are the same as symbol pins.
    // Excess model pins are set as Not Connected.
    for( int i = 0; i < static_cast<int>( getPinNames().size() ); ++i )
    {
        if( i < aSymbolPinCount )
            m_pins.push_back( { i + 1, getPinNames().at( i ) } );
        else
            m_pins.push_back( { PIN::NOT_CONNECTED, getPinNames().at( i ) } );
    }

    if( aPinsField.IsEmpty() )
        return;

    LOCALE_IO toggle;

    tao::pegtl::string_input<> in( aPinsField.ToStdString(), "from_input" );
    std::unique_ptr<tao::pegtl::parse_tree::node> root;

    try
    {
        root = tao::pegtl::parse_tree::parse<SIM_MODEL_PARSER::pinSequenceGrammar,
                                             SIM_MODEL_PARSER::pinSequenceSelector>( in );
    }
    catch( tao::pegtl::parse_error& e )
    {
        throw KI_PARAM_ERROR( wxString::Format( _( "Failed to parse model pin sequence: %s" ),
                                                e.what() ) );
    }

    wxASSERT( root );

    if( static_cast<int>( root->children.size() ) != GetPinCount() )
        throw KI_PARAM_ERROR( wxString::Format(
                              _( "The model pin sequence has a different number of values (%d) "
                                 "than the number of model pins (%d)" ) ) );

    for( unsigned int i = 0; i < root->children.size(); ++i )
    {
        if( root->children.at( i )->string() == "X" )
            SetPinSymbolPinNumber( static_cast<int>( i ), PIN::NOT_CONNECTED );
        else
            SetPinSymbolPinNumber( static_cast<int>( i ),
                                   std::stoi( root->children.at( i )->string() ) );
    }
}


wxString SIM_MODEL::generateParamsField( const wxString& aPairSeparator ) const
{
    bool isFirst = true;
    wxString result = "";

    for( const PARAM& param : m_params )
    {
        wxString valueStr = param.value->ToString();

        if( valueStr.IsEmpty() )
            continue;

        if( isFirst )
            isFirst = false;
        else
            result << " ";

        result << param.info.name;
        result << "=";
        result << param.value->ToString();
    }

    return result;
}


void SIM_MODEL::parseParamsField( const wxString& aParamsField )
{
    LOCALE_IO toggle;
    
    tao::pegtl::string_input<> in( aParamsField.ToStdString(), "from_input" );
    std::unique_ptr<tao::pegtl::parse_tree::node> root;

    try
    {
        // Using parse tree instead of actions because we don't care about performance that much,
        // and having a tree greatly simplifies some things. 
        root = tao::pegtl::parse_tree::parse<
            SIM_MODEL_PARSER::paramValuePairsGrammar<SIM_MODEL_PARSER::NOTATION::SI>,
            SIM_MODEL_PARSER::paramValuePairsSelector>
                ( in );
    }
    catch( tao::pegtl::parse_error& e )
    {
        throw KI_PARAM_ERROR( wxString::Format( _( "Failed to parse model parameters: %s" ),
                                                e.what() ) );
    }

    wxASSERT( root );

    wxString paramName = "";

    for( const auto& node : root->children )
    {
        if( node->is_type<SIM_MODEL_PARSER::param>() )
            paramName = node->string();
        // TODO: Do something with number<SIM_VALUE_BASE::TYPE::INT, ...>.
        // It doesn't seem too useful?
        else if( node->is_type<SIM_MODEL_PARSER::number<SIM_VALUE_BASE::TYPE::FLOAT,
                                                        SIM_MODEL_PARSER::NOTATION::SI>>() )
        {
            wxASSERT( !paramName.IsEmpty() );
            // TODO: Shouldn't be named "...fromSpiceCode" here...
            setParamFromSpiceCode( paramName, node->string() );
        }
        else
        {
            wxFAIL;
            return;
        }
    }
}


bool SIM_MODEL::setParamFromSpiceCode( const wxString& aParamName, const wxString& aParamValue )
{
    int i = 0;

    for(; i < GetParamCount(); ++i )
    {
        if( GetParam( i ).info.name == aParamName.Lower() )
            break;
    }

    if( i == GetParamCount() )
        return false; // No parameter with this name exists.

    try
    {
        SetParamValue( i, wxString( aParamValue ) );
    }
    catch( KI_PARAM_ERROR& e )
    {
        m_params.clear();
        return false;
    }

    return true;
}

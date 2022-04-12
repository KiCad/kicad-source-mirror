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
#include <sim/sim_model_subckt.h>
#include <sim/sim_model_xspice.h>
#include <sim/sim_model_spice.h>
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
    template <> struct paramValuePairsSelector<number<SIM_VALUE::TYPE::INT, NOTATION::SI>>
        : std::true_type {};
    template <> struct paramValuePairsSelector<number<SIM_VALUE::TYPE::FLOAT, NOTATION::SI>>
        : std::true_type {};
    template <> struct paramValuePairsSelector<number<SIM_VALUE::TYPE::INT, NOTATION::SPICE>>
        : std::true_type {};
    template <> struct paramValuePairsSelector<number<SIM_VALUE::TYPE::FLOAT, NOTATION::SPICE>>
        : std::true_type {};


    template <typename Rule> struct spiceUnitSelector : std::false_type {};

    template <> struct spiceUnitSelector<dotModel> : std::true_type {};
    template <> struct spiceUnitSelector<modelName> : std::true_type {};
    template <> struct spiceUnitSelector<dotModelType> : std::true_type {};
    template <> struct spiceUnitSelector<param> : std::true_type {};
    template <> struct spiceUnitSelector<number<SIM_VALUE::TYPE::INT, NOTATION::SPICE>>
        : std::true_type {};
    template <> struct spiceUnitSelector<number<SIM_VALUE::TYPE::FLOAT, NOTATION::SPICE>>
        : std::true_type {};

    template <> struct spiceUnitSelector<dotSubckt> : std::true_type {};


    template <typename Rule> struct pinSequenceSelector : std::false_type {};
    template <> struct pinSequenceSelector<pinNumber> : std::true_type {};
}


SIM_MODEL::DEVICE_INFO SIM_MODEL::DeviceTypeInfo( DEVICE_TYPE aDeviceType )
{
    switch( aDeviceType )
    {
    case DEVICE_TYPE::NONE:      return { "",       ""                  };
    case DEVICE_TYPE::R:         return { "R",      "Resistor"          };
    case DEVICE_TYPE::C:         return { "C",      "Capacitor"         };
    case DEVICE_TYPE::L:         return { "L",      "Inductor"          };
    case DEVICE_TYPE::TLINE:     return { "TLINE",  "Transmission Line" };
    case DEVICE_TYPE::SW:        return { "SW",     "Switch"            };

    case DEVICE_TYPE::D:         return { "D",      "Diode"             };
    case DEVICE_TYPE::NPN:       return { "NPN",    "NPN BJT"           };
    case DEVICE_TYPE::PNP:       return { "PNP",    "PNP BJT"           };

    case DEVICE_TYPE::NJFET:     return { "NJFET",  "N-channel JFET"    };
    case DEVICE_TYPE::PJFET:     return { "PJFET",  "P-channel JFET"    };

    case DEVICE_TYPE::NMOS:      return { "NMOS",   "N-channel MOSFET"  };
    case DEVICE_TYPE::PMOS:      return { "PMOS",   "P-channel MOSFET"  };
    case DEVICE_TYPE::NMES:      return { "NMES",   "N-channel MESFET"  };
    case DEVICE_TYPE::PMES:      return { "PMES",   "P-channel MESFET"  };

    case DEVICE_TYPE::V:         return { "V",      "Voltage Source"    };
    case DEVICE_TYPE::I:         return { "I",      "Current Source"    };

    case DEVICE_TYPE::SUBCKT:    return { "SUBCKT", "Subcircuit"        };
    case DEVICE_TYPE::XSPICE:    return { "XSPICE", "XSPICE Code Model" };
    case DEVICE_TYPE::SPICE:     return { "SPICE",  "Raw Spice Element" };
    case DEVICE_TYPE::_ENUM_END: break;
    }

    wxFAIL;
    return {};
}


SIM_MODEL::INFO SIM_MODEL::TypeInfo( TYPE aType )
{
    switch( aType )
    { 
    case TYPE::NONE:                 return { DEVICE_TYPE::NONE,   "",               ""                           };

    case TYPE::R:                    return { DEVICE_TYPE::R,      "",               "Ideal"                      };
    case TYPE::R_ADV:                return { DEVICE_TYPE::R,      "ADV",            "Advanced"                   };
    case TYPE::R_BEHAVIORAL:         return { DEVICE_TYPE::R,      "=",              "Behavioral"                 };

    case TYPE::C:                    return { DEVICE_TYPE::C,      "",               "Ideal"                      };
    case TYPE::C_ADV:                return { DEVICE_TYPE::C,      "ADV",            "Advanced"                   };
    case TYPE::C_BEHAVIORAL:         return { DEVICE_TYPE::C,      "=",              "Behavioral"                 };

    case TYPE::L:                    return { DEVICE_TYPE::L,      "",               "Ideal"                      };
    case TYPE::L_ADV:                return { DEVICE_TYPE::L,      "ADV",            "Advanced"                   };
    case TYPE::L_BEHAVIORAL:         return { DEVICE_TYPE::L,      "=",              "Behavioral"                 };

    case TYPE::TLINE_LOSSY:          return { DEVICE_TYPE::TLINE,  "",               "Lossy"                      };
    case TYPE::TLINE_LOSSLESS:       return { DEVICE_TYPE::TLINE,  "LOSSLESS",       "Lossless"                   };
    case TYPE::TLINE_URC:            return { DEVICE_TYPE::TLINE,  "URC",            "Uniform RC"                 };
    case TYPE::TLINE_KSPICE:         return { DEVICE_TYPE::TLINE,  "KSPICE",         "KSPICE"                     };

    case TYPE::SW_V:                 return { DEVICE_TYPE::SW,     "V",              "Voltage-controlled"         };
    case TYPE::SW_I:                 return { DEVICE_TYPE::SW,     "I",              "Current-controlled"         };

    case TYPE::D:                return { DEVICE_TYPE::D,      "",               ""                           };
    
    case TYPE::NPN_GUMMELPOON:       return { DEVICE_TYPE::NPN,    "GUMMELPOON",     "Gummel-Poon"                };
    case TYPE::PNP_GUMMELPOON:       return { DEVICE_TYPE::PNP,    "GUMMELPOON",     "Gummel-Poon"                };
    case TYPE::NPN_VBIC:             return { DEVICE_TYPE::NPN,    "VBIC",           "VBIC"                       };
    case TYPE::PNP_VBIC:             return { DEVICE_TYPE::PNP,    "VBIC",           "VBIC"                       };
    //case TYPE::BJT_MEXTRAM:          return {};
    case TYPE::NPN_HICUML2:          return { DEVICE_TYPE::NPN,    "HICUML2",        "HICUM Level 2"              };
    case TYPE::PNP_HICUML2:          return { DEVICE_TYPE::PNP,    "HICUML2",        "HICUM Level 2"              };
    //case TYPE::BJT_HICUM_L0:         return {};

    case TYPE::NJFET_SHICHMANHODGES: return { DEVICE_TYPE::NJFET,  "SHICHMANHODGES", "Shichman-Hodges"            };
    case TYPE::PJFET_SHICHMANHODGES: return { DEVICE_TYPE::PJFET,  "SHICHMANHODGES", "Shichman-Hodges"            };
    case TYPE::NJFET_PARKERSKELLERN: return { DEVICE_TYPE::NJFET,  "PARKERSKELLERN", "Parker-Skellern"            };
    case TYPE::PJFET_PARKERSKELLERN: return { DEVICE_TYPE::PJFET,  "PARKERSKELLERN", "Parker-Skellern"            };

    case TYPE::NMES_STATZ:           return { DEVICE_TYPE::NMES,   "STATZ",          "Statz"                      };
    case TYPE::PMES_STATZ:           return { DEVICE_TYPE::PMES,   "STATZ",          "Statz"                      };
    case TYPE::NMES_YTTERDAL:        return { DEVICE_TYPE::NMES,   "YTTERDAL",       "Ytterdal"                   };
    case TYPE::PMES_YTTERDAL:        return { DEVICE_TYPE::PMES,   "YTTERDAL",       "Ytterdal"                   };
    case TYPE::NMES_HFET1:           return { DEVICE_TYPE::NMES,   "HFET1",          "HFET1"                      };
    case TYPE::PMES_HFET1:           return { DEVICE_TYPE::PMES,   "HFET1",          "HFET1"                      };
    case TYPE::PMES_HFET2:           return { DEVICE_TYPE::NMES,   "HFET2",          "HFET2"                      };
    case TYPE::NMES_HFET2:           return { DEVICE_TYPE::PMES,   "HFET2",          "HFET2"                      };

    case TYPE::NMOS_MOS1:            return { DEVICE_TYPE::NMOS,   "MOS1",           "Classical quadratic (MOS1)" };
    case TYPE::PMOS_MOS1:            return { DEVICE_TYPE::PMOS,   "MOS1",           "Classical quadratic (MOS1)" };
    case TYPE::NMOS_MOS2:            return { DEVICE_TYPE::NMOS,   "MOS2",           "Grove-Frohman (MOS2)"       };
    case TYPE::PMOS_MOS2:            return { DEVICE_TYPE::PMOS,   "MOS2",           "Grove-Frohman (MOS2)"       };
    case TYPE::NMOS_MOS3:            return { DEVICE_TYPE::NMOS,   "MOS3",           "MOS3"                       };
    case TYPE::PMOS_MOS3:            return { DEVICE_TYPE::PMOS,   "MOS3",           "MOS3"                       };
    case TYPE::NMOS_BSIM1:           return { DEVICE_TYPE::NMOS,   "BSIM1",          "BSIM1"                      };
    case TYPE::PMOS_BSIM1:           return { DEVICE_TYPE::PMOS,   "BSIM1",          "BSIM1"                      };
    case TYPE::NMOS_BSIM2:           return { DEVICE_TYPE::NMOS,   "BSIM2",          "BSIM2"                      };
    case TYPE::PMOS_BSIM2:           return { DEVICE_TYPE::PMOS,   "BSIM2",          "BSIM2"                      };
    case TYPE::NMOS_MOS6:            return { DEVICE_TYPE::NMOS,   "MOS6",           "MOS6"                       };
    case TYPE::PMOS_MOS6:            return { DEVICE_TYPE::PMOS,   "MOS6",           "MOS6"                       };
    case TYPE::NMOS_BSIM3:           return { DEVICE_TYPE::NMOS,   "BSIM3",          "BSIM3"                      };
    case TYPE::PMOS_BSIM3:           return { DEVICE_TYPE::PMOS,   "BSIM3",          "BSIM3"                      };
    case TYPE::NMOS_MOS9:            return { DEVICE_TYPE::NMOS,   "MOS9",           "MOS9"                       };
    case TYPE::PMOS_MOS9:            return { DEVICE_TYPE::PMOS,   "MOS9",           "MOS9"                       };
    case TYPE::NMOS_B4SOI:           return { DEVICE_TYPE::NMOS,   "B4SOI",          "BSIM4 SOI (B4SOI)"          };
    case TYPE::PMOS_B4SOI:           return { DEVICE_TYPE::PMOS,   "B4SOI",          "BSIM4 SOI (B4SOI)"          };
    case TYPE::NMOS_BSIM4:           return { DEVICE_TYPE::NMOS,   "BSIM4",          "BSIM4"                      };
    case TYPE::PMOS_BSIM4:           return { DEVICE_TYPE::PMOS,   "BSIM4",          "BSIM4"                      };
    //case TYPE::NMOS_EKV2_6:          return {};
    //case TYPE::PMOS_EKV2_6:          return {};
    //case TYPE::NMOS_PSP:             return {};
    //case TYPE::PMOS_PSP:             return {};
    case TYPE::NMOS_B3SOIFD:         return { DEVICE_TYPE::NMOS,   "B3SOIFD",        "B3SOIFD (BSIM3 FD-SOI)"     };
    case TYPE::PMOS_B3SOIFD:         return { DEVICE_TYPE::PMOS,   "B3SOIFD",        "B3SOIFD (BSIM3 FD-SOI)"     };
    case TYPE::NMOS_B3SOIDD:         return { DEVICE_TYPE::NMOS,   "B3SOIDD",        "B3SOIDD (BSIM3 SOI)"        };
    case TYPE::PMOS_B3SOIDD:         return { DEVICE_TYPE::PMOS,   "B3SOIDD",        "B3SOIDD (BSIM3 SOI)"        };
    case TYPE::NMOS_B3SOIPD:         return { DEVICE_TYPE::NMOS,   "B3SOIPD",        "B3SOIPD (BSIM3 PD-SOI)"     };
    case TYPE::PMOS_B3SOIPD:         return { DEVICE_TYPE::PMOS,   "B3SOIPD",        "B3SOIPD (BSIM3 PD-SOI)"     };
    //case TYPE::NMOS_STAG:            return {};
    //case TYPE::PMOS_STAG:            return {};
    case TYPE::NMOS_HISIM2:          return { DEVICE_TYPE::NMOS,   "HISIM2",         "HiSIM2"                     };
    case TYPE::PMOS_HISIM2:          return { DEVICE_TYPE::PMOS,   "HISIM2",         "HiSIM2"                     };
    case TYPE::NMOS_HISIMHV1:        return { DEVICE_TYPE::NMOS,   "HISIMHV1",       "HiSIM_HV1"                  };
    case TYPE::PMOS_HISIMHV1:        return { DEVICE_TYPE::PMOS,   "HISIMHV1",       "HiSIM_HV1"                  };
    case TYPE::NMOS_HISIMHV2:        return { DEVICE_TYPE::NMOS,   "HISIMHV2",       "HiSIM_HV2"                  };
    case TYPE::PMOS_HISIMHV2:        return { DEVICE_TYPE::PMOS,   "HISIMHV2",       "HiSIM_HV2"                  };

    case TYPE::V_DC:                 return { DEVICE_TYPE::V,      "DC",             "DC",                        };
    case TYPE::V_SIN:                return { DEVICE_TYPE::V,      "SIN",            "Sine"                       };
    case TYPE::V_PULSE:              return { DEVICE_TYPE::V,      "PULSE",          "Pulse"                      };
    case TYPE::V_EXP:                return { DEVICE_TYPE::V,      "EXP",            "Exponential"                };
    case TYPE::V_SFAM:               return { DEVICE_TYPE::V,      "SFAM",           "Single-frequency AM"        };
    case TYPE::V_SFFM:               return { DEVICE_TYPE::V,      "SFFM",           "Single-frequency FM"        };
    case TYPE::V_PWL:                return { DEVICE_TYPE::V,      "PWL",            "Piecewise linear"           };
    case TYPE::V_WHITENOISE:         return { DEVICE_TYPE::V,      "WHITENOISE",     "White Noise"                };
    case TYPE::V_PINKNOISE:          return { DEVICE_TYPE::V,      "PINKNOISE",      "Pink Noise (1/f)"           };
    case TYPE::V_BURSTNOISE:         return { DEVICE_TYPE::V,      "BURSTNOISE",     "Burst Noise"                };
    case TYPE::V_RANDUNIFORM:        return { DEVICE_TYPE::V,      "RANDUNIFORM",    "Random uniform"             };
    case TYPE::V_RANDNORMAL:         return { DEVICE_TYPE::V,      "RANDNORMAL",     "Random normal"              };
    case TYPE::V_RANDEXP:            return { DEVICE_TYPE::V,      "RANDEXP",        "Random exponential"         };
    case TYPE::V_RANDPOISSON:        return { DEVICE_TYPE::V,      "RANDPOISSON",    "Random Poisson"             };
    case TYPE::V_BEHAVIORAL:         return { DEVICE_TYPE::V,      "=",              "Behavioral"                 };

    case TYPE::I_DC:                 return { DEVICE_TYPE::I,      "DC",             "DC",                        };
    case TYPE::I_SIN:                return { DEVICE_TYPE::I,      "SIN",            "Sine"                       };
    case TYPE::I_PULSE:              return { DEVICE_TYPE::I,      "PULSE",          "Pulse"                      };
    case TYPE::I_EXP:                return { DEVICE_TYPE::I,      "EXP",            "Exponential"                };
    case TYPE::I_SFAM:               return { DEVICE_TYPE::I,      "SFAM",           "Single-frequency AM"        };
    case TYPE::I_SFFM:               return { DEVICE_TYPE::I,      "SFFM",           "Single-frequency FM"        };
    case TYPE::I_PWL:                return { DEVICE_TYPE::I,      "PWL",            "Piecewise linear"           };
    case TYPE::I_WHITENOISE:         return { DEVICE_TYPE::I,      "WHITENOISE",     "White Noise"                };
    case TYPE::I_PINKNOISE:          return { DEVICE_TYPE::I,      "PINKNOISE",      "Pink Noise (1/f)"           };
    case TYPE::I_BURSTNOISE:         return { DEVICE_TYPE::I,      "BURSTNOISE",     "Burst Noise"                };
    case TYPE::I_RANDUNIFORM:        return { DEVICE_TYPE::I,      "RANDUNIFORM",    "Random uniform"             };
    case TYPE::I_RANDNORMAL:         return { DEVICE_TYPE::I,      "RANDNORMAL",     "Random normal"              };
    case TYPE::I_RANDEXP:            return { DEVICE_TYPE::I,      "RANDEXP",        "Random exponential"         };
    case TYPE::I_RANDPOISSON:        return { DEVICE_TYPE::I,      "RANDPOISSON",    "Random Poisson"             };
    case TYPE::I_BEHAVIORAL:         return { DEVICE_TYPE::I,      "=",              "Behavioral"                 };

    case TYPE::SUBCKT:               return { DEVICE_TYPE::SUBCKT, "",               ""                           };
    case TYPE::XSPICE:               return { DEVICE_TYPE::XSPICE, "",               ""                           };
    case TYPE::SPICE:                return { DEVICE_TYPE::SPICE,  "",               ""                           };

    case TYPE::_ENUM_END:             break;
    }

    wxFAIL;
    return {};
}


SIM_MODEL::SPICE_INFO SIM_MODEL::SpiceInfo( TYPE aType )
{
    switch( aType )
    {
    case TYPE::R:                    return { "R", ""        };
    case TYPE::R_ADV:                return { "R", "R"       };
    case TYPE::R_BEHAVIORAL:         return { "R", "",       "",        0,  true   };

    case TYPE::C:                    return { "C", ""        };
    case TYPE::C_ADV:                return { "C", "C",      };
    case TYPE::C_BEHAVIORAL:         return { "C", "",       "",        0,  true   };

    case TYPE::L:                    return { "L", ""        };
    case TYPE::L_ADV:                return { "L", "L"       };
    case TYPE::L_BEHAVIORAL:         return { "L", "",       "",        0,  true   };
    
    case TYPE::TLINE_LOSSY:          return { "O", "LTRA"    };
    case TYPE::TLINE_LOSSLESS:       return { "T"  };
    case TYPE::TLINE_URC:            return { "U"  };
    case TYPE::TLINE_KSPICE:         return { "Y"  };
    
    case TYPE::SW_V:                 return { "S", "switch"  };
    case TYPE::SW_I:                 return { "W", "cswitch" };

    case TYPE::D:                    return { "D", "D"       };

    case TYPE::NPN_GUMMELPOON:       return { "Q", "NPN",    "",        1   };
    case TYPE::PNP_GUMMELPOON:       return { "Q", "PNP",    "",        1   };

    case TYPE::NPN_VBIC:             return { "Q", "NPN",    "",        4   };
    case TYPE::PNP_VBIC:             return { "Q", "PNP",    "",        4   };

    case TYPE::NPN_HICUML2:          return { "Q", "NPN",    "",        8   };
    case TYPE::PNP_HICUML2:          return { "Q", "PNP",    "",        8   };

    case TYPE::NJFET_SHICHMANHODGES: return { "M", "NJF",    "",        1   };
    case TYPE::PJFET_SHICHMANHODGES: return { "M", "PJF",    "",        1   };
    case TYPE::NJFET_PARKERSKELLERN: return { "M", "NJF",    "",        2   };
    case TYPE::PJFET_PARKERSKELLERN: return { "M", "PJF",    "",        2   };

    case TYPE::NMES_STATZ:           return { "Z", "NMF",    "",        1   };
    case TYPE::PMES_STATZ:           return { "Z", "PMF",    "",        1   };
    case TYPE::NMES_YTTERDAL:        return { "Z", "NMF",    "",        2   };
    case TYPE::PMES_YTTERDAL:        return { "Z", "PMF",    "",        2   };
    case TYPE::NMES_HFET1:           return { "Z", "NMF",    "",        5   };
    case TYPE::PMES_HFET1:           return { "Z", "PMF",    "",        5   };
    case TYPE::PMES_HFET2:           return { "Z", "NMF",    "",        6   };
    case TYPE::NMES_HFET2:           return { "Z", "PMF",    "",        6   };

    case TYPE::NMOS_MOS1:            return { "M", "NMOS",   "",        1   };
    case TYPE::PMOS_MOS1:            return { "M", "PMOS",   "",        1   };
    case TYPE::NMOS_MOS2:            return { "M", "NMOS",   "",        2   };
    case TYPE::PMOS_MOS2:            return { "M", "PMOS",   "",        2   };
    case TYPE::NMOS_MOS3:            return { "M", "NMOS",   "",        3   };
    case TYPE::PMOS_MOS3:            return { "M", "PMOS",   "",        3   };
    case TYPE::NMOS_BSIM1:           return { "M", "NMOS",   "",        4   };
    case TYPE::PMOS_BSIM1:           return { "M", "PMOS",   "",        4   };
    case TYPE::NMOS_BSIM2:           return { "M", "NMOS",   "",        5   };
    case TYPE::PMOS_BSIM2:           return { "M", "PMOS",   "",        5   };
    case TYPE::NMOS_MOS6:            return { "M", "NMOS",   "",        6   };
    case TYPE::PMOS_MOS6:            return { "M", "PMOS",   "",        6   };
    case TYPE::NMOS_BSIM3:           return { "M", "NMOS",   "",        8   };
    case TYPE::PMOS_BSIM3:           return { "M", "PMOS",   "",        8   };
    case TYPE::NMOS_MOS9:            return { "M", "NMOS",   "",        9   };
    case TYPE::PMOS_MOS9:            return { "M", "PMOS",   "",        9   };
    case TYPE::NMOS_B4SOI:           return { "M", "NMOS",   "",        10  };
    case TYPE::PMOS_B4SOI:           return { "M", "PMOS",   "",        10  };
    case TYPE::NMOS_BSIM4:           return { "M", "NMOS",   "",        14  };
    case TYPE::PMOS_BSIM4:           return { "M", "PMOS",   "",        14  };
    //case TYPE::NMOS_EKV2_6:          return {};
    //case TYPE::PMOS_EKV2_6:          return {};
    //case TYPE::NMOS_PSP:             return {};
    //case TYPE::PMOS_PSP:             return {};
    case TYPE::NMOS_B3SOIFD:         return { "M", "NMOS",   "",        55  };
    case TYPE::PMOS_B3SOIFD:         return { "M", "PMOS",   "",        55  };
    case TYPE::NMOS_B3SOIDD:         return { "M", "NMOS",   "",        56  };
    case TYPE::PMOS_B3SOIDD:         return { "M", "PMOS",   "",        56  };
    case TYPE::NMOS_B3SOIPD:         return { "M", "NMOS",   "",        57  };
    case TYPE::PMOS_B3SOIPD:         return { "M", "PMOS",   "",        57  };
    //case TYPE::NMOS_STAG:            return {};
    //case TYPE::PMOS_STAG:            return {};
    case TYPE::NMOS_HISIM2:          return { "M", "NMOS",   "",        68  };
    case TYPE::PMOS_HISIM2:          return { "M", "PMOS",   "",        68  };
    case TYPE::NMOS_HISIMHV1:        return { "M", "NMOS",   "",        73, false, "1.2.4" };
    case TYPE::PMOS_HISIMHV1:        return { "M", "PMOS",   "",        73, false, "1.2.4" };
    case TYPE::NMOS_HISIMHV2:        return { "M", "NMOS",   "",        73, false, "2.2.0" };
    case TYPE::PMOS_HISIMHV2:        return { "M", "PMOS",   "",        73, false, "2.2.0" };

    case TYPE::V_DC:                 return { "V", ""        };
    case TYPE::V_SIN:                return { "V", "",       "SIN"      };
    case TYPE::V_PULSE:              return { "V", "",       "PULSE"    };
    case TYPE::V_EXP:                return { "V", "",       "EXP"      };
    case TYPE::V_SFAM:               return { "V", "",       "AM"       };
    case TYPE::V_SFFM:               return { "V", "",       "SFFM"     };
    case TYPE::V_PWL:                return { "V", "",       "PWL"      };
    case TYPE::V_WHITENOISE:         return { "V", "",       "TRNOISE"  };
    case TYPE::V_PINKNOISE:          return { "V", "",       "TRNOISE"  };
    case TYPE::V_BURSTNOISE:         return { "V", "",       "TRNOISE"  };
    case TYPE::V_RANDUNIFORM:        return { "V", "",       "TRRANDOM" };
    case TYPE::V_RANDNORMAL:         return { "V", "",       "TRRANDOM" };
    case TYPE::V_RANDEXP:            return { "V", "",       "TRRANDOM" };
    case TYPE::V_RANDPOISSON:        return { "V", "",       "TRRANDOM" };
    case TYPE::V_BEHAVIORAL:         return { "B"  };

    case TYPE::I_DC:                 return { "V", ""        };
    case TYPE::I_PULSE:              return { "V", "",       "PULSE"    };
    case TYPE::I_SIN:                return { "V", "",       "SIN"      };
    case TYPE::I_EXP:                return { "V", "",       "EXP"      };
    case TYPE::I_SFAM:               return { "V", "",       "AM"       };
    case TYPE::I_SFFM:               return { "V", "",       "SFFM"     };
    case TYPE::I_PWL:                return { "V", "",       "PWL"      };
    case TYPE::I_WHITENOISE:         return { "V", "",       "TRNOISE"  };
    case TYPE::I_PINKNOISE:          return { "V", "",       "TRNOISE"  };
    case TYPE::I_BURSTNOISE:         return { "V", "",       "TRNOISE"  };
    case TYPE::I_RANDUNIFORM:        return { "V", "",       "TRRANDOM" };
    case TYPE::I_RANDNORMAL:         return { "V", "",       "TRRANDOM" };
    case TYPE::I_RANDEXP:            return { "V", "",       "TRRANDOM" };
    case TYPE::I_RANDPOISSON:        return { "V", "",       "TRRANDOM" };
    case TYPE::I_BEHAVIORAL:         return { "B"  };

    case TYPE::SUBCKT:               return { "X"  };
    case TYPE::XSPICE:               return { "A"  };

    case TYPE::NONE:
    case TYPE::SPICE:
        return {};

    case TYPE::_ENUM_END:
        break;
    }

    wxFAIL;
    return {};
}


TYPE SIM_MODEL::ReadTypeFromSpiceCode( const std::string& aSpiceCode )
{
    tao::pegtl::string_input<> in( aSpiceCode, "from_content" );
    std::unique_ptr<tao::pegtl::parse_tree::node> root;

    try
    {
        root = tao::pegtl::parse_tree::parse<SIM_MODEL_PARSER::spiceUnitGrammar,
                                             SIM_MODEL_PARSER::spiceUnitSelector>
            ( in );
    }
    catch( const tao::pegtl::parse_error& e )
    {
        // TODO: Maybe announce an error somehow?
        return TYPE::NONE;
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
            return TYPE::SUBCKT;
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

    if( !typeFieldValue.IsEmpty() )
    {
        for( TYPE type : TYPE_ITERATOR() )
        {
            if( typeFieldValue == TypeInfo( type ).fieldValue )
            {
                if( deviceTypeFieldValue == DeviceTypeInfo( TypeInfo( type ).deviceType ).fieldValue )
                    return type;
            }
        }

        return TYPE::NONE;
    }

    // No type information. For passives we infer the model from the mandatory fields in this case.

    wxString ref = GetFieldValue( &aFields, REFERENCE_FIELD );

    if( ref.StartsWith( "R" ) )
        return TYPE::R;
    else if( ref.StartsWith( "C" ) )
        return TYPE::C;
    else if( ref.StartsWith( "L" ) )
        return TYPE::L;

    return TYPE::NONE;
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
        std::unique_ptr<SIM_MODEL> rawSpiceModel = create( TYPE::SPICE );

        rawSpiceModel->ReadSpiceCode( aSpiceCode );
        return rawSpiceModel;
    }

    return model;
}


template std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const SIM_MODEL& aBaseModel,
                                                       int aSymbolPinCount,
                                                       const std::vector<SCH_FIELD>& aFields );

template std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const SIM_MODEL& aBaseModel,
                                                       int aSymbolPinCount,
                                                       const std::vector<LIB_FIELD>& aFields );

template <typename T>
std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const SIM_MODEL& aBaseModel, int aSymbolPinCount,
                                              const std::vector<T>& aFields )
{
    std::unique_ptr<SIM_MODEL> model = create( aBaseModel.GetType() );

    model->SetBaseModel( aBaseModel );
    model->ReadDataFields( aSymbolPinCount, &aFields );
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
    
    tao::pegtl::string_input<> in( aSpiceCode, "from_content" );
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

    for( const auto& node : root->children )
    {
        if( node->is_type<SIM_MODEL_PARSER::dotModel>() )
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
                // TODO: Do something with number<SIM_VALUE::TYPE::INT, ...>.
                // It doesn't seem too useful?
                else if( subnode->is_type<
                        SIM_MODEL_PARSER::number<SIM_VALUE::TYPE::INT,
                                                 SIM_MODEL_PARSER::NOTATION::SPICE>>()
                    || subnode->is_type<
                        SIM_MODEL_PARSER::number<SIM_VALUE::TYPE::FLOAT,
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


wxString SIM_MODEL::GenerateSpiceModelLine( const wxString& aModelName ) const
{
    LOCALE_IO toggle;

    if( !HasOverrides() )
        return "";

    wxString result = "";
    wxString line = "";

    line << wxString::Format( ".model %s %s(\n+", aModelName, GetSpiceInfo().modelType );

    for( int paramIndex = 0; paramIndex < GetParamCount(); ++paramIndex )
    {
        const PARAM& param = GetParam( paramIndex );
        wxString valueStr = param.value->ToString();

        if( valueStr.IsEmpty() )
            continue;
        
        wxString append = " " + param.info.name + "=" + param.value->ToString();

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


wxString SIM_MODEL::GenerateSpiceItemName( const wxString& aRefName ) const
{
    if( !aRefName.IsEmpty() && aRefName.StartsWith( GetSpiceInfo().primitive ) )
        return aRefName;
    else
        return GetSpiceInfo().primitive + aRefName;
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
    result << GenerateSpiceItemName( aRefName ) << " ";

    for( int i = 0; i < GetPinCount(); ++i )
    {
        for( int j = 0; j < ( int ) aPinNetNames.size(); ++j )
        {
            int symbolPinNumber = j + 1;

            if( symbolPinNumber == GetPin( i ).symbolPinNumber )
                result << aPinNetNames[j] << " ";
        }
    }

    result << aModelName << " ";

    for( int i = 0; i < GetParamCount(); ++i )
    {
        const PARAM& param = GetParam( i );

        if( param.info.isInstanceParam )
            result << param.info.name << "=" << param.value->ToString() << " ";
    }

    result << "\n";
    return result;
}


wxString SIM_MODEL::GenerateSpiceTuningLine( const wxString& aSymbol ) const
{
    // TODO.
    return "";
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


SIM_MODEL::SPICE_INFO SIM_MODEL::GetSpiceInfo() const
{
    return SpiceInfo( GetType() );
}


std::vector<wxString> SIM_MODEL::GenerateSpiceCurrentNames( const wxString& aRefName ) const
{
    LOCALE_IO toggle;
    return { wxString::Format( "I(%s)", GenerateSpiceItemName( aRefName ) ) };
}


bool SIM_MODEL::ParsePinsField( int aSymbolPinCount, const wxString& aPinsField )
{
    // Default pin sequence: model pins are the same as symbol pins.
    // Excess model pins are set as Not Connected.
    for( int i = 0; i < static_cast<int>( getPinNames().size() ); ++i )
    {
        if( i < aSymbolPinCount )
            AddPin( { getPinNames().at( i ), i + 1 } );
        else
            AddPin( { getPinNames().at( i ), PIN::NOT_CONNECTED } );
    }

    if( aPinsField.IsEmpty() )
        return true;

    LOCALE_IO toggle;

    tao::pegtl::string_input<> in( aPinsField.ToStdString(), "from_content" );
    std::unique_ptr<tao::pegtl::parse_tree::node> root;

    try
    {
        root = tao::pegtl::parse_tree::parse<SIM_MODEL_PARSER::pinSequenceGrammar,
                                             SIM_MODEL_PARSER::pinSequenceSelector>( in );
    }
    catch( const tao::pegtl::parse_error& e )
    {
        return false;
    }

    wxASSERT( root );

    if( static_cast<int>( root->children.size() ) != GetPinCount() )
        return false;

    for( unsigned int i = 0; i < root->children.size(); ++i )
    {
        if( root->children.at( i )->string() == "X" )
            SetPinSymbolPinNumber( static_cast<int>( i ), PIN::NOT_CONNECTED );
        else
            SetPinSymbolPinNumber( static_cast<int>( i ),
                                   std::stoi( root->children.at( i )->string() ) );
    }

    return true;
}


void SIM_MODEL::AddPin( const PIN& aPin )
{
    m_pins.push_back( aPin );
}


int SIM_MODEL::FindModelPinNumber( int aSymbolPinNumber )
{
    for( int i = 0; i < GetPinCount(); ++i )
    {
        if( GetPin( i ).symbolPinNumber == aSymbolPinNumber )
            return i + 1;
    }

    return 0;
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


bool SIM_MODEL::SetParamValue( int aParamIndex, const wxString& aValue,
                               SIM_VALUE_GRAMMAR::NOTATION aNotation )
{
    // Models sourced from a library are immutable.
    if( !m_spiceCode.IsEmpty() )
        return false;

    return m_params.at( aParamIndex ).value->FromString( aValue, aNotation );
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
    case TYPE::R:
    case TYPE::C:
    case TYPE::L:
        return std::make_unique<SIM_MODEL_IDEAL>( aType );

    case TYPE::R_BEHAVIORAL:
    case TYPE::C_BEHAVIORAL:
    case TYPE::L_BEHAVIORAL:
    case TYPE::V_BEHAVIORAL:
    case TYPE::I_BEHAVIORAL:
        return std::make_unique<SIM_MODEL_BEHAVIORAL>( aType );

    case TYPE::V_DC:
    case TYPE::I_DC:
    case TYPE::V_SIN:
    case TYPE::I_SIN:
    case TYPE::V_PULSE:
    case TYPE::I_PULSE:
    case TYPE::V_EXP:
    case TYPE::I_EXP:
    case TYPE::V_SFAM:
    case TYPE::I_SFAM:
    case TYPE::V_SFFM:
    case TYPE::I_SFFM:
    case TYPE::V_PWL:
    case TYPE::I_PWL:
    case TYPE::V_WHITENOISE:
    case TYPE::I_WHITENOISE:
    case TYPE::V_PINKNOISE:
    case TYPE::I_PINKNOISE:
    case TYPE::V_BURSTNOISE:
    case TYPE::I_BURSTNOISE:
    case TYPE::V_RANDUNIFORM:
    case TYPE::I_RANDUNIFORM:
    case TYPE::V_RANDNORMAL:
    case TYPE::I_RANDNORMAL:
    case TYPE::V_RANDEXP:
    case TYPE::I_RANDEXP:
    case TYPE::V_RANDPOISSON:
    case TYPE::I_RANDPOISSON:
        return std::make_unique<SIM_MODEL_SOURCE>( aType );

    case TYPE::SUBCKT:
        return std::make_unique<SIM_MODEL_SUBCKT>( aType );

    case TYPE::XSPICE:
        return std::make_unique<SIM_MODEL_XSPICE>( aType );

    case TYPE::SPICE:
        return std::make_unique<SIM_MODEL_RAWSPICE>( aType );

    default:
        return std::make_unique<SIM_MODEL_NGSPICE>( aType );
    }
}


TYPE SIM_MODEL::readTypeFromSpiceTypeString( const std::string& aTypeString )
{
    for( TYPE type : TYPE_ITERATOR() )
    {
        if( SpiceInfo( type ).modelType == aTypeString )
            return type;
    }

    // If the type string is not recognized, demote to a raw Spice element. This way the user won't
    // have an error if there is a type KiCad does not recognize.
    return TYPE::SPICE;
}


template <typename T>
void SIM_MODEL::doReadDataFields( int aSymbolPinCount, const std::vector<T>* aFields )
{
    ParsePinsField( aSymbolPinCount, GetFieldValue( aFields, PINS_FIELD ) );
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


bool SIM_MODEL::parseParamsField( const wxString& aParamsField )
{
    LOCALE_IO toggle;
    
    tao::pegtl::string_input<> in( aParamsField.ToStdString(), "from_content" );
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
    catch( const tao::pegtl::parse_error& e )
    {
        return false;
    }

    wxASSERT( root );

    wxString paramName = "";

    for( const auto& node : root->children )
    {
        if( node->is_type<SIM_MODEL_PARSER::param>() )
            paramName = node->string();
        // TODO: Do something with number<SIM_VALUE::TYPE::INT, ...>.
        // It doesn't seem too useful?
        else if( node->is_type<SIM_MODEL_PARSER::number<SIM_VALUE::TYPE::FLOAT,
                                                        SIM_MODEL_PARSER::NOTATION::SI>>() )
        {
            wxASSERT( !paramName.IsEmpty() );
            // TODO: Shouldn't be named "...fromSpiceCode" here...
            setParamFromSpiceCode( paramName, node->string(), SIM_VALUE_GRAMMAR::NOTATION::SI );
        }
        else
        {
            wxFAIL;
            return false;
        }
    }

    return true;
}


bool SIM_MODEL::setParamFromSpiceCode( const wxString& aParamName, const wxString& aParamValue,
                                       SIM_VALUE_GRAMMAR::NOTATION aNotation )
{
    int i = 0;

    for(; i < GetParamCount(); ++i )
    {
        if( GetParam( i ).info.name == aParamName.Lower() )
            break;
    }

    if( i == GetParamCount() )
        return false; // No parameter with this name exists.

    return SetParamValue( i, wxString( aParamValue ), aNotation );
}

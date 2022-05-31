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
#include <sim/sim_model_behavioral.h>
#include <sim/sim_model_ideal.h>
#include <sim/sim_model_ngspice.h>
#include <sim/sim_model_passive.h>
#include <sim/sim_model_source.h>
#include <sim/sim_model_spice.h>
#include <sim/sim_model_subckt.h>
#include <sim/sim_model_tline.h>
#include <sim/sim_model_xspice.h>

#include <pegtl.hpp>
#include <pegtl/contrib/parse_tree.hpp>
#include <locale_io.h>
#include <lib_symbol.h>

using DEVICE_TYPE = SIM_MODEL::DEVICE_TYPE;
using TYPE = SIM_MODEL::TYPE;


namespace SIM_MODEL_PARSER
{
    using namespace SIM_MODEL_GRAMMAR;


    template <typename Rule> struct fieldParamValuePairsSelector : std::false_type {};

    template <> struct fieldParamValuePairsSelector<param> : std::true_type {};
    template <> struct fieldParamValuePairsSelector<number<SIM_VALUE::TYPE::INT, NOTATION::SI>>
        : std::true_type {};
    template <> struct fieldParamValuePairsSelector<number<SIM_VALUE::TYPE::FLOAT, NOTATION::SI>>
        : std::true_type {};
    template <> struct fieldParamValuePairsSelector<unquotedString> : std::true_type {};
    template <> struct fieldParamValuePairsSelector<quotedString> : std::true_type {};


    template <typename Rule> struct pinSequenceSelector : std::false_type {};
    template <> struct pinSequenceSelector<pinNumber> : std::true_type {};
}


namespace SIM_MODEL_SPICE_PARSER
{
    using namespace SPICE_GRAMMAR;


    template <typename Rule> struct spiceUnitSelector : std::false_type {};

    template <> struct spiceUnitSelector<dotModel> : std::true_type {};
    template <> struct spiceUnitSelector<modelName> : std::true_type {};
    template <> struct spiceUnitSelector<dotModelType> : std::true_type {};
    template <> struct spiceUnitSelector<param> : std::true_type {};
    template <> struct spiceUnitSelector<paramValue> : std::true_type {};

    template <> struct spiceUnitSelector<dotSubckt> : std::true_type {};
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

    case TYPE::TLINE_Z0:             return { DEVICE_TYPE::TLINE,  "Z0",             "Characteristic impedance"   };
    case TYPE::TLINE_RLGC:           return { DEVICE_TYPE::TLINE,  "RLGC",           "RLGC"                       };

    case TYPE::SW_V:                 return { DEVICE_TYPE::SW,     "V",              "Voltage-controlled"         };
    case TYPE::SW_I:                 return { DEVICE_TYPE::SW,     "I",              "Current-controlled"         };

    case TYPE::D:                    return { DEVICE_TYPE::D,      "",               ""                           };
    
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
    case TYPE::NMES_HFET2:           return { DEVICE_TYPE::NMES,   "HFET2",          "HFET2"                      };
    case TYPE::PMES_HFET2:           return { DEVICE_TYPE::PMES,   "HFET2",          "HFET2"                      };

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
    case TYPE::R_ADV:                return { "R", "r"       };
    case TYPE::R_BEHAVIORAL:         return { "R", "",       "",        "0",   false, true   };

    case TYPE::C:                    return { "C", ""        };
    case TYPE::C_ADV:                return { "C", "c",      };
    case TYPE::C_BEHAVIORAL:         return { "C", "",       "",        "0",   false, true   };

    case TYPE::L:                    return { "L", ""        };
    case TYPE::L_ADV:                return { "L", "l"       };
    case TYPE::L_BEHAVIORAL:         return { "L", "",       "",        "0",   false, true   };
    
    case TYPE::TLINE_Z0:             return { "T"  };
    case TYPE::TLINE_RLGC:           return { "O", "ltra"    };
    
    case TYPE::SW_V:                 return { "S", "switch"  };
    case TYPE::SW_I:                 return { "W", "cswitch" };

    case TYPE::D:                    return { "D", "d"       };

    case TYPE::NPN_GUMMELPOON:       return { "Q", "npn",    "",        "1",  true   };
    case TYPE::PNP_GUMMELPOON:       return { "Q", "pnp",    "",        "1",  true   };

    case TYPE::NPN_VBIC:             return { "Q", "npn",    "",        "4"   };
    case TYPE::PNP_VBIC:             return { "Q", "pnp",    "",        "4"   };

    case TYPE::NPN_HICUML2:          return { "Q", "npn",    "",        "8"   };
    case TYPE::PNP_HICUML2:          return { "Q", "pnp",    "",        "8"   };

    case TYPE::NJFET_SHICHMANHODGES: return { "M", "njf",    "",        "1"   };
    case TYPE::PJFET_SHICHMANHODGES: return { "M", "pjf",    "",        "1"   };
    case TYPE::NJFET_PARKERSKELLERN: return { "M", "njf",    "",        "2"   };
    case TYPE::PJFET_PARKERSKELLERN: return { "M", "pjf",    "",        "2"   };

    case TYPE::NMES_STATZ:           return { "Z", "nmf",    "",        "1"   };
    case TYPE::PMES_STATZ:           return { "Z", "pmf",    "",        "1"   };
    case TYPE::NMES_YTTERDAL:        return { "Z", "nmf",    "",        "2"   };
    case TYPE::PMES_YTTERDAL:        return { "Z", "pmf",    "",        "2"   };
    case TYPE::NMES_HFET1:           return { "Z", "nmf",    "",        "5"   };
    case TYPE::PMES_HFET1:           return { "Z", "pmf",    "",        "5"   };
    case TYPE::NMES_HFET2:           return { "Z", "nmf",    "",        "6"   };
    case TYPE::PMES_HFET2:           return { "Z", "pmf",    "",        "6"   };

    case TYPE::NMOS_MOS1:            return { "M", "nmos",   "",        "1"   };
    case TYPE::PMOS_MOS1:            return { "M", "pmos",   "",        "1"   };
    case TYPE::NMOS_MOS2:            return { "M", "nmos",   "",        "2"   };
    case TYPE::PMOS_MOS2:            return { "M", "pmos",   "",        "2"   };
    case TYPE::NMOS_MOS3:            return { "M", "nmos",   "",        "3"   };
    case TYPE::PMOS_MOS3:            return { "M", "pmos",   "",        "3"   };
    case TYPE::NMOS_BSIM1:           return { "M", "nmos",   "",        "4"   };
    case TYPE::PMOS_BSIM1:           return { "M", "pmos",   "",        "4"   };
    case TYPE::NMOS_BSIM2:           return { "M", "nmos",   "",        "5"   };
    case TYPE::PMOS_BSIM2:           return { "M", "pmos",   "",        "5"   };
    case TYPE::NMOS_MOS6:            return { "M", "nmos",   "",        "6"   };
    case TYPE::PMOS_MOS6:            return { "M", "pmos",   "",        "6"   };
    case TYPE::NMOS_BSIM3:           return { "M", "nmos",   "",        "8"   };
    case TYPE::PMOS_BSIM3:           return { "M", "pmos",   "",        "8"   };
    case TYPE::NMOS_MOS9:            return { "M", "nmos",   "",        "9"   };
    case TYPE::PMOS_MOS9:            return { "M", "pmos",   "",        "9"   };
    case TYPE::NMOS_B4SOI:           return { "M", "nmos",   "",        "10"  };
    case TYPE::PMOS_B4SOI:           return { "M", "pmos",   "",        "10"  };
    case TYPE::NMOS_BSIM4:           return { "M", "nmos",   "",        "14"  };
    case TYPE::PMOS_BSIM4:           return { "M", "pmos",   "",        "14"  };
    //case TYPE::NMOS_EKV2_6:          return {};
    //case TYPE::PMOS_EKV2_6:          return {};
    //case TYPE::NMOS_PSP:             return {};
    //case TYPE::PMOS_PSP:             return {};
    case TYPE::NMOS_B3SOIFD:         return { "M", "nmos",   "",        "55"  };
    case TYPE::PMOS_B3SOIFD:         return { "M", "pmos",   "",        "55"  };
    case TYPE::NMOS_B3SOIDD:         return { "M", "nmos",   "",        "56"  };
    case TYPE::PMOS_B3SOIDD:         return { "M", "pmos",   "",        "56"  };
    case TYPE::NMOS_B3SOIPD:         return { "M", "nmos",   "",        "57"  };
    case TYPE::PMOS_B3SOIPD:         return { "M", "pmos",   "",        "57"  };
    //case TYPE::NMOS_STAG:            return {};
    //case TYPE::PMOS_STAG:            return {};
    case TYPE::NMOS_HISIM2:          return { "M", "nmos",   "",        "68"  };
    case TYPE::PMOS_HISIM2:          return { "M", "pmos",   "",        "68"  };
    case TYPE::NMOS_HISIMHV1:        return { "M", "nmos",   "",        "73", true,  false, "1.2.4" };
    case TYPE::PMOS_HISIMHV1:        return { "M", "pmos",   "",        "73", true,  false, "1.2.4" };
    case TYPE::NMOS_HISIMHV2:        return { "M", "nmos",   "",        "73", true,  false, "2.2.0" };
    case TYPE::PMOS_HISIMHV2:        return { "M", "pmos",   "",        "73", true,  false, "2.2.0" };

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
        root = tao::pegtl::parse_tree::parse<SIM_MODEL_SPICE_PARSER::spiceUnitGrammar,
                                             SIM_MODEL_SPICE_PARSER::spiceUnitSelector>
            ( in );
    }
    catch( const tao::pegtl::parse_error& e )
    {
        wxLogDebug( "%s", e.what() );
        return TYPE::NONE;
    }

    wxASSERT( root );

    for( const auto& node : root->children )
    {
        if( node->is_type<SIM_MODEL_SPICE_PARSER::dotModel>() )
        {
            wxString paramName;
            wxString typeString;
            wxString level;
            wxString version;

            for( const auto& subnode : node->children )
            {
                if( subnode->is_type<SIM_MODEL_SPICE_PARSER::modelName>() )
                {
                    // Do nothing.
                }
                else if( subnode->is_type<SIM_MODEL_SPICE_PARSER::dotModelType>() )
                {
                    typeString = subnode->string();
                    TYPE type = readTypeFromSpiceStrings( typeString );

                    if( type != TYPE::SPICE )
                        return type;
                }
                else if( subnode->is_type<SIM_MODEL_SPICE_PARSER::param>() )
                {
                    paramName = subnode->string();
                }
                else if( subnode->is_type<SIM_MODEL_SPICE_PARSER::paramValue>() )
                {
                    wxASSERT( paramName != "" );

                    if( paramName == "level" )
                        level = subnode->string();
                    else if( paramName == "version" ) // TODO! This isn't a number!
                        version = subnode->string();
                }
                else
                {
                    wxFAIL_MSG( "Unhandled parse tree subnode" );
                    return TYPE::NONE;
                }
            }

            // Type was not determined from Spice type string alone, so now we take `level` and
            // `version` variables into account too. This is suboptimal since we read the model
            // twice this way, and moreover the code is now somewhat duplicated.

            return readTypeFromSpiceStrings( typeString, level, version, false );
        }
        else if( node->is_type<SIM_MODEL_SPICE_PARSER::dotSubckt>() )
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
    wxString deviceTypeFieldValue = GetFieldValue( &aFields, DEVICE_TYPE_FIELD );
    wxString typeFieldValue = GetFieldValue( &aFields, TYPE_FIELD );

    if( !deviceTypeFieldValue.IsEmpty() )
    {
        for( TYPE type : TYPE_ITERATOR() )
        {
            if( typeFieldValue == TypeInfo( type ).fieldValue )
            {
                if( deviceTypeFieldValue == DeviceTypeInfo( TypeInfo( type ).deviceType ).fieldValue )
                    return type;
            }
        }
    }

    if( !typeFieldValue.IsEmpty() )
        return TYPE::NONE;

    // No type information. For passives we infer the model from the mandatory fields in this case.
    TYPE typeFromRef = InferTypeFromRef( GetFieldValue( &aFields, REFERENCE_FIELD ) );
    if( typeFromRef != TYPE::NONE )
        return typeFromRef;

    // Finally, try to infer the model from legacy fields, if present.
    return InferTypeFromLegacyFields( aFields );
}


TYPE SIM_MODEL::InferTypeFromRef( const wxString& aRef )
{
    static std::map<wxString, TYPE> refPrefixToType = {
        { "R", TYPE::R },
        { "C", TYPE::C },
        { "L", TYPE::L },
        { "VDC", TYPE::V_DC },
        { "VSIN", TYPE::V_SIN },
        { "VPULSE", TYPE::V_PULSE },
        { "VEXP", TYPE::V_EXP },
        { "VSFAM", TYPE::V_SFAM },
        { "VSFFM", TYPE::V_SFFM },
        { "VPWL", TYPE::V_PWL },
        { "VWHITENOISE", TYPE::V_WHITENOISE },
        { "VPINKNOISE", TYPE::V_PINKNOISE },
        { "VBURSTNOISE", TYPE::V_BURSTNOISE },
        { "VRANDUNIFORM", TYPE::V_RANDUNIFORM },
        { "VRANDNORMAL", TYPE::V_RANDNORMAL },
        { "VRANDEXP", TYPE::V_RANDEXP },
        { "VRANDPOISSON", TYPE::V_RANDPOISSON },
        { "VBEHAVIORAL", TYPE::V_BEHAVIORAL },
        { "IDC", TYPE::I_DC },
        { "ISIN", TYPE::I_SIN },
        { "IPULSE", TYPE::I_PULSE },
        { "IEXP", TYPE::I_EXP },
        { "ISFAM", TYPE::I_SFAM },
        { "ISFFM", TYPE::I_SFFM },
        { "IPWL", TYPE::I_PWL },
        { "IWHITENOISE", TYPE::I_WHITENOISE },
        { "IPINKNOISE", TYPE::I_PINKNOISE },
        { "IBURSTNOISE", TYPE::I_BURSTNOISE },
        { "IRANDUNIFORM", TYPE::I_RANDUNIFORM },
        { "IRANDNORMAL", TYPE::I_RANDNORMAL },
        { "IRANDEXP", TYPE::I_RANDEXP },
        { "IRANDPOISSON", TYPE::I_RANDPOISSON },
        { "IBEHAVIORAL", TYPE::I_BEHAVIORAL }
    };

    for( auto&& [prefix, type] : refPrefixToType )
    {
        if( aRef.StartsWith( prefix ) )
            return type;
    }

    return TYPE::NONE;
}


template <typename T>
TYPE SIM_MODEL::InferTypeFromLegacyFields( const std::vector<T>& aFields )
{
    if( !GetFieldValue( &aFields, SIM_MODEL_SPICE::LEGACY_TYPE_FIELD ).IsEmpty()
        || !GetFieldValue( &aFields, SIM_MODEL_SPICE::LEGACY_MODEL_FIELD ).IsEmpty()
        || !GetFieldValue( &aFields, SIM_MODEL_SPICE::LEGACY_ENABLED_FIELD ).IsEmpty()
        || !GetFieldValue( &aFields, SIM_MODEL_SPICE::LEGACY_LIB_FIELD ).IsEmpty() )
    {
        return TYPE::SPICE;
    }
    else
        return TYPE::NONE;
}


std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( TYPE aType, unsigned aSymbolPinCount )
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
        wxLogDebug( "%s", model->GetErrorMessage() );
        // Demote to raw Spice element and try again.
        std::unique_ptr<SIM_MODEL> rawSpiceModel = create( TYPE::SPICE );

        rawSpiceModel->ReadSpiceCode( aSpiceCode );
        return rawSpiceModel;
    }

    return model;
}


template std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const SIM_MODEL& aBaseModel,
                                                       unsigned aSymbolPinCount,
                                                       const std::vector<SCH_FIELD>& aFields );

template std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const SIM_MODEL& aBaseModel,
                                                       unsigned aSymbolPinCount,
                                                       const std::vector<LIB_FIELD>& aFields );

template <typename T>
std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const SIM_MODEL& aBaseModel, unsigned aSymbolPinCount,
                                              const std::vector<T>& aFields )
{
    std::unique_ptr<SIM_MODEL> model = create( aBaseModel.GetType() );

    model->SetBaseModel( aBaseModel );
    model->ReadDataFields( aSymbolPinCount, &aFields );
    return model;
}


template std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( unsigned aSymbolPinCount,
                                                       const std::vector<SCH_FIELD>& aFields );
template std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( unsigned aSymbolPinCount,
                                                       const std::vector<LIB_FIELD>& aFields );

template <typename T>
std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( unsigned aSymbolPinCount,
                                              const std::vector<T>& aFields )
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
        root = tao::pegtl::parse_tree::parse<SIM_MODEL_SPICE_PARSER::spiceUnitGrammar,
                                             SIM_MODEL_SPICE_PARSER::spiceUnitSelector>
            ( in );
    }
    catch( tao::pegtl::parse_error& e )
    {
        m_errorMessage = e.what();
        return false;
    }


    wxASSERT( root );

    for( const auto& node : root->children )
    {
        if( node->is_type<SIM_MODEL_SPICE_PARSER::dotModel>() )
        {
            wxString paramName = "";

            for( const auto& subnode : node->children )
            {
                if( subnode->is_type<SIM_MODEL_SPICE_PARSER::modelName>() )
                {
                    // Do nothing.
                }
                else if( subnode->is_type<SIM_MODEL_SPICE_PARSER::dotModelType>() )
                {
                    // Do nothing.
                }
                else if( subnode->is_type<SIM_MODEL_SPICE_PARSER::param>() )
                {
                    paramName = subnode->string();
                }
                else if( subnode->is_type<SIM_MODEL_SPICE_PARSER::paramValue>() )
                {
                    wxASSERT( !paramName.IsEmpty() );

                    if( !SetParamFromSpiceCode( paramName, subnode->string() ) )
                    {
                        m_errorMessage =
                            wxString::Format( _( "Failed to set parameter '%s' to '%s'" ),
                                              paramName,
                                              subnode->string() );
                        return false;
                    }
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
void SIM_MODEL::ReadDataFields( unsigned aSymbolPinCount, const std::vector<T>* aFields )
{
    doReadDataFields( aSymbolPinCount, aFields );
}


template <>
void SIM_MODEL::ReadDataFields( unsigned aSymbolPinCount, const std::vector<SCH_FIELD>* aFields )
{
    ReadDataSchFields( aSymbolPinCount, aFields );
}


template <>
void SIM_MODEL::ReadDataFields( unsigned aSymbolPinCount, const std::vector<LIB_FIELD>* aFields )
{
    ReadDataLibFields( aSymbolPinCount, aFields );
}


void SIM_MODEL::ReadDataSchFields( unsigned aSymbolPinCount, const std::vector<SCH_FIELD>* aFields )
{
    doReadDataFields( aSymbolPinCount, aFields );
}


void SIM_MODEL::ReadDataLibFields( unsigned aSymbolPinCount, const std::vector<LIB_FIELD>* aFields )
{
    doReadDataFields( aSymbolPinCount, aFields );
}


template <>
void SIM_MODEL::WriteFields( std::vector<SCH_FIELD>& aFields ) const
{
    WriteDataSchFields( aFields );
}


template <>
void SIM_MODEL::WriteFields( std::vector<LIB_FIELD>& aFields ) const
{
    WriteDataLibFields( aFields );
}


void SIM_MODEL::WriteDataSchFields( std::vector<SCH_FIELD>& aFields ) const
{
    doWriteFields( aFields );
}


void SIM_MODEL::WriteDataLibFields( std::vector<LIB_FIELD>& aFields ) const
{
    doWriteFields( aFields );
}


wxString SIM_MODEL::GenerateSpiceModelLine( const wxString& aModelName ) const
{
    LOCALE_IO toggle;

    if( !HasOverrides() || !requiresSpiceModel() )
        return "";

    wxString result = "";
    wxString line = "";

    line << wxString::Format( ".model %s %s(\n+", aModelName, GetSpiceInfo().modelType );

    for( unsigned paramIndex = 0; paramIndex < GetParamCount(); ++paramIndex )
    {
        const PARAM& param = GetParam( paramIndex );
        wxString valueStr = param.value->ToString( SIM_VALUE_GRAMMAR::NOTATION::SPICE );

        if( valueStr.IsEmpty() )
            continue;
        
        wxString append = " " + param.info.name + "=" + valueStr;

        if( line.Length() + append.Length() > 60 )
        {
            result << line + "\n";
            line = "+" + append;
        }
        else
            line << append;
    }

    result << line + " )\n";
    return result;
}


wxString SIM_MODEL::GenerateSpiceItemName( const wxString& aRefName ) const
{
    if( !aRefName.IsEmpty() && aRefName.StartsWith( GetSpiceInfo().itemType ) )
        return aRefName;
    else
        return GetSpiceInfo().itemType + aRefName;
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

    for( const PIN& pin : GetPins() )
    {
        for( unsigned i = 0; i < aPinNetNames.size(); ++i )
        {
            unsigned symbolPinNumber = i + 1;
            
            if( symbolPinNumber == pin.symbolPinNumber )
                result << aPinNetNames[i] << " ";
        }
    }

    if( requiresSpiceModel() )
        result << aModelName << " ";

    for( const PARAM& param : GetParams() )
    {
        if( param.info.isSpiceInstanceParam )
            result << param.info.name << "="
                << param.value->ToString( SIM_VALUE_GRAMMAR::NOTATION::SPICE ) << " ";
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


std::vector<wxString> SIM_MODEL::GenerateSpiceCurrentNames( const wxString& aRefName ) const
{
    LOCALE_IO toggle;
    return { wxString::Format( "I(%s)", GenerateSpiceItemName( aRefName ) ) };
}


void SIM_MODEL::AddPin( const PIN& aPin )
{
    m_pins.push_back( aPin );
}


unsigned SIM_MODEL::FindModelPinNumber( unsigned aSymbolPinNumber )
{
    for( unsigned i = 0; i < GetPinCount(); ++i )
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


std::vector<std::reference_wrapper<const SIM_MODEL::PIN>> SIM_MODEL::GetPins() const
{
    std::vector<std::reference_wrapper<const PIN>> pins;

    for( unsigned i = 0; i < GetPinCount(); ++i )
        pins.emplace_back( GetPin( i ) );

    return pins;
}


const SIM_MODEL::PARAM& SIM_MODEL::GetParam( unsigned aParamIndex ) const
{
    if( m_baseModel && m_params.at( aParamIndex ).value->ToString() == "" )
        return m_baseModel->GetParam( aParamIndex );
    else
        return m_params.at( aParamIndex );
}


const SIM_MODEL::PARAM* SIM_MODEL::FindParam( const wxString& aParamName ) const
{
    std::vector<std::reference_wrapper<const PARAM>> params = GetParams();

    auto it = std::find_if( params.begin(), params.end(),
                            [aParamName]( const PARAM& param )
                            {
                                return param.info.name == aParamName.Lower();
                            } );

    if( it == params.end() )
        return nullptr;

    return &it->get();
}


std::vector<std::reference_wrapper<const SIM_MODEL::PARAM>> SIM_MODEL::GetParams() const
{
    std::vector<std::reference_wrapper<const PARAM>> params;

    for( unsigned i = 0; i < GetParamCount(); ++i )
        params.emplace_back( GetParam( i ) );

    return params;
}


const SIM_MODEL::PARAM& SIM_MODEL::GetUnderlyingParam( unsigned aParamIndex ) const
{
    return m_params.at( aParamIndex );
}


const SIM_MODEL::PARAM& SIM_MODEL::GetBaseParam( unsigned aParamIndex ) const
{
    if( m_baseModel )
        return m_baseModel->GetParam( aParamIndex );
    else
        return m_params.at( aParamIndex );
}


bool SIM_MODEL::SetParamValue( unsigned aParamIndex, const wxString& aValue,
                               SIM_VALUE_GRAMMAR::NOTATION aNotation )
{
    // Models sourced from a library are immutable.
    if( m_spiceCode != "" )
        return false;

    return m_params.at( aParamIndex ).value->FromString( aValue, aNotation );
}


bool SIM_MODEL::HasOverrides() const
{
    for( const PARAM& param : m_params )
    {
        if( param.value->ToString() != "" )
            return true;
    }

    return false;
}


bool SIM_MODEL::HasNonInstanceOverrides() const
{
    for( const PARAM& param : m_params )
    {
        if( !param.info.isInstanceParam && param.value->ToString() != "" )
            return true;
    }

    return false;
}


SIM_MODEL::SIM_MODEL( TYPE aType ) : m_baseModel( nullptr ), m_type( aType )
{
}


template void SIM_MODEL::WriteInferredDataFields( std::vector<SCH_FIELD>& aFields,
                                                  const wxString& aValue ) const;
template void SIM_MODEL::WriteInferredDataFields( std::vector<LIB_FIELD>& aFields,
                                                  const wxString& aValue ) const;

template <typename T>
void SIM_MODEL::WriteInferredDataFields( std::vector<T>& aFields, const wxString& aValue ) const
{
    if( GetPinCount() == 2
        && GetPin( 0 ).symbolPinNumber == 1
        && GetPin( 1 ).symbolPinNumber == 2 )
    {
        SetFieldValue( aFields, PINS_FIELD, "" );
    }

    SetFieldValue( aFields, VALUE_FIELD, aValue );
    SetFieldValue( aFields, DEVICE_TYPE_FIELD, "" );
    SetFieldValue( aFields, TYPE_FIELD, "" );
    SetFieldValue( aFields, PARAMS_FIELD, "" );
}


wxString SIM_MODEL::GenerateParamValuePair( const PARAM& aParam, bool& aIsFirst ) const
{
    wxString result = "";

    if( aIsFirst )
        aIsFirst = false;
    else
        result << " ";

    result << aParam.info.name + "=" + aParam.value->ToString();
    return result;
}


wxString SIM_MODEL::GenerateParamsField( const wxString& aPairSeparator ) const
{
    bool isFirst = true;
    wxString result = "";

    for( const PARAM& param : m_params )
    {
        if( param.value->ToString() == "" )
            continue;

        result << GenerateParamValuePair( param, isFirst );
    }

    return result;
}


bool SIM_MODEL::ParseParamsField( const wxString& aParamsField )
{
    LOCALE_IO toggle;

    tao::pegtl::string_input<> in( aParamsField.ToStdString(), "from_content" );
    std::unique_ptr<tao::pegtl::parse_tree::node> root;

    try
    {
        // Using parse tree instead of actions because we don't care about performance that much,
        // and having a tree greatly simplifies some things. 
        root = tao::pegtl::parse_tree::parse<
            SIM_MODEL_PARSER::fieldParamValuePairsGrammar,
            SIM_MODEL_PARSER::fieldParamValuePairsSelector>
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
        else if( node->is_type<SIM_MODEL_PARSER::number<SIM_VALUE::TYPE::INT,
                                                        SIM_MODEL_PARSER::NOTATION::SI>>()
            || node->is_type<SIM_MODEL_PARSER::number<SIM_VALUE::TYPE::FLOAT,
                                                      SIM_MODEL_PARSER::NOTATION::SI>>()
            || node->is_type<SIM_MODEL_PARSER::unquotedString>() )
        {
            wxASSERT( paramName != "" );
            // TODO: Shouldn't be named "...fromSpiceCode" here...

            SetParamFromSpiceCode( paramName, node->string(), SIM_VALUE_GRAMMAR::NOTATION::SI );
        }
        else if( node->is_type<SIM_MODEL_PARSER::quotedString>() )
        {
            wxASSERT( !paramName.IsEmpty() );

            wxString str = node->string();

            // Unescape quotes.
            str.Replace( "\\\"", "\"" );

            SetParamFromSpiceCode( paramName, str, SIM_VALUE_GRAMMAR::NOTATION::SI );
        }
        else
        {
            wxFAIL;
            return false;
        }
    }

    return true;
}


bool SIM_MODEL::ParsePinsField( unsigned aSymbolPinCount, const wxString& aPinsField )
{
    // Default pin sequence: model pins are the same as symbol pins.
    // Excess model pins are set as Not Connected.
    for( unsigned i = 0; i < getPinNames().size(); ++i )
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

    if( root->children.size() != GetPinCount() )
        return false;

    for( unsigned i = 0; i < root->children.size(); ++i )
    {
        if( root->children.at( i )->string() == "X" )
            SetPinSymbolPinNumber( static_cast<int>( i ), PIN::NOT_CONNECTED );
        else
            SetPinSymbolPinNumber( static_cast<int>( i ),
                                   std::stoi( root->children.at( i )->string() ) );
    }

    return true;
}


bool SIM_MODEL::SetParamFromSpiceCode( const wxString& aParamName, const wxString& aParamValue,
                                       SIM_VALUE_GRAMMAR::NOTATION aNotation )
{
    std::vector<std::reference_wrapper<const PARAM>> params = GetParams();

    auto it = std::find_if( params.begin(), params.end(),
                            [aParamName]( const PARAM& param )
                            {
                                return param.info.name == aParamName.Lower();
                            } );
    
    if( it == params.end() )
        return false;

    return SetParamValue( it - params.begin(), aParamValue, aNotation );
}


std::unique_ptr<SIM_MODEL> SIM_MODEL::create( TYPE aType )
{
    switch( aType )
    {
    case TYPE::R:
    case TYPE::C:
    case TYPE::L:
        return std::make_unique<SIM_MODEL_IDEAL>( aType );

    case TYPE::R_ADV:
    case TYPE::C_ADV:
    case TYPE::L_ADV:
        return std::make_unique<SIM_MODEL_PASSIVE>( aType );

    case TYPE::R_BEHAVIORAL:
    case TYPE::C_BEHAVIORAL:
    case TYPE::L_BEHAVIORAL:
    case TYPE::V_BEHAVIORAL:
    case TYPE::I_BEHAVIORAL:
        return std::make_unique<SIM_MODEL_BEHAVIORAL>( aType );
    
    case TYPE::TLINE_Z0:
    case TYPE::TLINE_RLGC:
        return std::make_unique<SIM_MODEL_TLINE>( aType );

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
        return std::make_unique<SIM_MODEL_SPICE>( aType );

    default:
        return std::make_unique<SIM_MODEL_NGSPICE>( aType );
    }
}


TYPE SIM_MODEL::readTypeFromSpiceStrings( const wxString& aTypeString,
                                          const wxString& aLevel,
                                          const wxString& aVersion,
                                          bool aSkipDefaultLevel )
{
    std::unique_ptr<SIM_VALUE> readLevel = SIM_VALUE::Create( SIM_VALUE::TYPE::INT, aLevel );

    for( TYPE type : TYPE_ITERATOR() )
    {
        wxString typePrefix = SpiceInfo( type ).modelType;
        wxString level = SpiceInfo( type ).level;
        wxString version = SpiceInfo( type ).version;
        bool isDefaultLevel = SpiceInfo( type ).isDefaultLevel;

        if( typePrefix == "" )
            continue;
        
        // Check if `aTypeString` starts with `typePrefix`.
        if( aTypeString.Lower().StartsWith( typePrefix )
            && ( level == readLevel->ToString()
                 || ( !aSkipDefaultLevel && isDefaultLevel && aLevel == "" ) )
            && version == aVersion )
        {
            return type;
        }
    }

    // If the type string is not recognized, demote to a raw Spice element. This way the user won't
    // have an error if there is a type KiCad does not recognize.
    return TYPE::SPICE;
}


template <typename T>
void SIM_MODEL::doReadDataFields( unsigned aSymbolPinCount, const std::vector<T>* aFields )
{
    ParsePinsField( aSymbolPinCount, GetFieldValue( aFields, PINS_FIELD ) );
    ParseParamsField( GetFieldValue( aFields, PARAMS_FIELD ) );
}


template <typename T>
void SIM_MODEL::doWriteFields( std::vector<T>& aFields ) const
{
    SetFieldValue( aFields, DEVICE_TYPE_FIELD, generateDeviceTypeField() );
    SetFieldValue( aFields, TYPE_FIELD, generateTypeField() );
    SetFieldValue( aFields, PINS_FIELD, generatePinsField() );
    SetFieldValue( aFields, PARAMS_FIELD, GenerateParamsField( " " ) );
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

    for( unsigned i = 0; i < GetPinCount(); ++i )
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


bool SIM_MODEL::requiresSpiceModel() const
{
    for( const PARAM& param : GetParams() )
    {
        if( !param.info.isSpiceInstanceParam )
            return true;
    }

    return false;
}

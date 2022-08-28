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

#include <sim/sim_model.h>
#include <sim/sim_model_behavioral.h>
#include <sim/sim_model_ideal.h>
#include <sim/sim_model_ngspice.h>
#include <sim/sim_model_source.h>
#include <sim/sim_model_spice.h>
#include <sim/sim_model_subckt.h>
#include <sim/sim_model_switch.h>
#include <sim/sim_model_tline.h>
#include <sim/sim_model_xspice.h>

#include <sim/spice_grammar.h>
#include <locale_io.h>
#include <lib_symbol.h>
#include <pegtl.hpp>
#include <pegtl/contrib/parse_tree.hpp>

#include <iterator>

using DEVICE_TYPE = SIM_MODEL::DEVICE_TYPE_;
using TYPE = SIM_MODEL::TYPE;


namespace SIM_MODEL_PARSER
{
    using namespace SIM_MODEL_GRAMMAR;

    template <typename Rule> struct fieldParamValuePairsSelector : std::false_type {};

    template <> struct fieldParamValuePairsSelector<param> : std::true_type {};
    template <> struct fieldParamValuePairsSelector<quotedStringContent> : std::true_type {};
    template <> struct fieldParamValuePairsSelector<unquotedString> : std::true_type {};


    template <typename Rule> struct fieldFloatValueSelector : std::false_type {};
    template <> struct fieldFloatValueSelector<number<SIM_VALUE::TYPE_FLOAT, NOTATION::SI>>
        : std::true_type {};


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


SIM_MODEL::DEVICE_INFO SIM_MODEL::DeviceTypeInfo( DEVICE_TYPE_ aDeviceType )
{
    switch( aDeviceType )
    {
    case DEVICE_TYPE_::NONE:      return { "",       ""                  };
    case DEVICE_TYPE_::R:         return { "R",      "Resistor"          };
    case DEVICE_TYPE_::C:         return { "C",      "Capacitor"         };
    case DEVICE_TYPE_::L:         return { "L",      "Inductor"          };
    case DEVICE_TYPE_::TLINE:     return { "TLINE",  "Transmission Line" };
    case DEVICE_TYPE_::SW:        return { "SW",     "Switch"            };

    case DEVICE_TYPE_::D:         return { "D",      "Diode"             };
    case DEVICE_TYPE_::NPN:       return { "NPN",    "NPN BJT"           };
    case DEVICE_TYPE_::PNP:       return { "PNP",    "PNP BJT"           };

    case DEVICE_TYPE_::NJFET:     return { "NJFET",  "N-channel JFET"    };
    case DEVICE_TYPE_::PJFET:     return { "PJFET",  "P-channel JFET"    };

    case DEVICE_TYPE_::NMOS:      return { "NMOS",   "N-channel MOSFET"  };
    case DEVICE_TYPE_::PMOS:      return { "PMOS",   "P-channel MOSFET"  };
    case DEVICE_TYPE_::NMES:      return { "NMES",   "N-channel MESFET"  };
    case DEVICE_TYPE_::PMES:      return { "PMES",   "P-channel MESFET"  };

    case DEVICE_TYPE_::V:         return { "V",      "Voltage Source"    };
    case DEVICE_TYPE_::I:         return { "I",      "Current Source"    };

    case DEVICE_TYPE_::SUBCKT:    return { "SUBCKT", "Subcircuit"        };
    case DEVICE_TYPE_::XSPICE:    return { "XSPICE", "XSPICE Code Model" };
    case DEVICE_TYPE_::SPICE:     return { "SPICE",  "Raw Spice Element" };
    case DEVICE_TYPE_::_ENUM_END: break;
    }

    wxFAIL;
    return {};
}


SIM_MODEL::INFO SIM_MODEL::TypeInfo( TYPE aType )
{
    switch( aType )
    { 
    case TYPE::NONE:                 return { DEVICE_TYPE_::NONE,   "",               ""                           };

    case TYPE::R:                    return { DEVICE_TYPE_::R,      "",               "Ideal"                      };
    //case TYPE::R_ADV:                return { DEVICE_TYPE::R,      "ADV",            "Advanced"                   };
    case TYPE::R_BEHAVIORAL:         return { DEVICE_TYPE_::R,      "=",              "Behavioral"                 };

    case TYPE::C:                    return { DEVICE_TYPE_::C,      "",               "Ideal"                      };
    //case TYPE::C_ADV:                return { DEVICE_TYPE::C,      "ADV",            "Advanced"                   };
    case TYPE::C_BEHAVIORAL:         return { DEVICE_TYPE_::C,      "=",              "Behavioral"                 };

    case TYPE::L:                    return { DEVICE_TYPE_::L,      "",               "Ideal"                      };
    //case TYPE::L_ADV:                return { DEVICE_TYPE::L,      "ADV",            "Advanced"                   };
    case TYPE::L_BEHAVIORAL:         return { DEVICE_TYPE_::L,      "=",              "Behavioral"                 };

    case TYPE::TLINE_Z0:             return { DEVICE_TYPE_::TLINE,  "Z0",             "Characteristic impedance"   };
    case TYPE::TLINE_RLGC:           return { DEVICE_TYPE_::TLINE,  "RLGC",           "RLGC"                       };

    case TYPE::SW_V:                 return { DEVICE_TYPE_::SW,     "V",              "Voltage-controlled"         };
    case TYPE::SW_I:                 return { DEVICE_TYPE_::SW,     "I",              "Current-controlled"         };

    case TYPE::D:                    return { DEVICE_TYPE_::D,      "",               ""                           };
    
    case TYPE::NPN_GUMMELPOON:       return { DEVICE_TYPE_::NPN,    "GUMMELPOON",     "Gummel-Poon"                };
    case TYPE::PNP_GUMMELPOON:       return { DEVICE_TYPE_::PNP,    "GUMMELPOON",     "Gummel-Poon"                };
    case TYPE::NPN_VBIC:             return { DEVICE_TYPE_::NPN,    "VBIC",           "VBIC"                       };
    case TYPE::PNP_VBIC:             return { DEVICE_TYPE_::PNP,    "VBIC",           "VBIC"                       };
    //case TYPE::BJT_MEXTRAM:          return {};
    case TYPE::NPN_HICUM2:           return { DEVICE_TYPE_::NPN,    "HICUML2",        "HICUM level 2"              };
    case TYPE::PNP_HICUM2:           return { DEVICE_TYPE_::PNP,    "HICUML2",        "HICUM level 2"              };
    //case TYPE::BJT_HICUM_L0:         return {};

    case TYPE::NJFET_SHICHMANHODGES: return { DEVICE_TYPE_::NJFET,  "SHICHMANHODGES", "Shichman-Hodges"            };
    case TYPE::PJFET_SHICHMANHODGES: return { DEVICE_TYPE_::PJFET,  "SHICHMANHODGES", "Shichman-Hodges"            };
    case TYPE::NJFET_PARKERSKELLERN: return { DEVICE_TYPE_::NJFET,  "PARKERSKELLERN", "Parker-Skellern"            };
    case TYPE::PJFET_PARKERSKELLERN: return { DEVICE_TYPE_::PJFET,  "PARKERSKELLERN", "Parker-Skellern"            };

    case TYPE::NMES_STATZ:           return { DEVICE_TYPE_::NMES,   "STATZ",          "Statz"                      };
    case TYPE::PMES_STATZ:           return { DEVICE_TYPE_::PMES,   "STATZ",          "Statz"                      };
    case TYPE::NMES_YTTERDAL:        return { DEVICE_TYPE_::NMES,   "YTTERDAL",       "Ytterdal"                   };
    case TYPE::PMES_YTTERDAL:        return { DEVICE_TYPE_::PMES,   "YTTERDAL",       "Ytterdal"                   };
    case TYPE::NMES_HFET1:           return { DEVICE_TYPE_::NMES,   "HFET1",          "HFET1"                      };
    case TYPE::PMES_HFET1:           return { DEVICE_TYPE_::PMES,   "HFET1",          "HFET1"                      };
    case TYPE::NMES_HFET2:           return { DEVICE_TYPE_::NMES,   "HFET2",          "HFET2"                      };
    case TYPE::PMES_HFET2:           return { DEVICE_TYPE_::PMES,   "HFET2",          "HFET2"                      };

    case TYPE::NMOS_MOS1:            return { DEVICE_TYPE_::NMOS,   "MOS1",           "Classical quadratic (MOS1)" };
    case TYPE::PMOS_MOS1:            return { DEVICE_TYPE_::PMOS,   "MOS1",           "Classical quadratic (MOS1)" };
    case TYPE::NMOS_MOS2:            return { DEVICE_TYPE_::NMOS,   "MOS2",           "Grove-Frohman (MOS2)"       };
    case TYPE::PMOS_MOS2:            return { DEVICE_TYPE_::PMOS,   "MOS2",           "Grove-Frohman (MOS2)"       };
    case TYPE::NMOS_MOS3:            return { DEVICE_TYPE_::NMOS,   "MOS3",           "MOS3"                       };
    case TYPE::PMOS_MOS3:            return { DEVICE_TYPE_::PMOS,   "MOS3",           "MOS3"                       };
    case TYPE::NMOS_BSIM1:           return { DEVICE_TYPE_::NMOS,   "BSIM1",          "BSIM1"                      };
    case TYPE::PMOS_BSIM1:           return { DEVICE_TYPE_::PMOS,   "BSIM1",          "BSIM1"                      };
    case TYPE::NMOS_BSIM2:           return { DEVICE_TYPE_::NMOS,   "BSIM2",          "BSIM2"                      };
    case TYPE::PMOS_BSIM2:           return { DEVICE_TYPE_::PMOS,   "BSIM2",          "BSIM2"                      };
    case TYPE::NMOS_MOS6:            return { DEVICE_TYPE_::NMOS,   "MOS6",           "MOS6"                       };
    case TYPE::PMOS_MOS6:            return { DEVICE_TYPE_::PMOS,   "MOS6",           "MOS6"                       };
    case TYPE::NMOS_BSIM3:           return { DEVICE_TYPE_::NMOS,   "BSIM3",          "BSIM3"                      };
    case TYPE::PMOS_BSIM3:           return { DEVICE_TYPE_::PMOS,   "BSIM3",          "BSIM3"                      };
    case TYPE::NMOS_MOS9:            return { DEVICE_TYPE_::NMOS,   "MOS9",           "MOS9"                       };
    case TYPE::PMOS_MOS9:            return { DEVICE_TYPE_::PMOS,   "MOS9",           "MOS9"                       };
    case TYPE::NMOS_B4SOI:           return { DEVICE_TYPE_::NMOS,   "B4SOI",          "BSIM4 SOI (B4SOI)"          };
    case TYPE::PMOS_B4SOI:           return { DEVICE_TYPE_::PMOS,   "B4SOI",          "BSIM4 SOI (B4SOI)"          };
    case TYPE::NMOS_BSIM4:           return { DEVICE_TYPE_::NMOS,   "BSIM4",          "BSIM4"                      };
    case TYPE::PMOS_BSIM4:           return { DEVICE_TYPE_::PMOS,   "BSIM4",          "BSIM4"                      };
    //case TYPE::NMOS_EKV2_6:          return {};
    //case TYPE::PMOS_EKV2_6:          return {};
    //case TYPE::NMOS_PSP:             return {};
    //case TYPE::PMOS_PSP:             return {};
    case TYPE::NMOS_B3SOIFD:         return { DEVICE_TYPE_::NMOS,   "B3SOIFD",        "B3SOIFD (BSIM3 FD-SOI)"     };
    case TYPE::PMOS_B3SOIFD:         return { DEVICE_TYPE_::PMOS,   "B3SOIFD",        "B3SOIFD (BSIM3 FD-SOI)"     };
    case TYPE::NMOS_B3SOIDD:         return { DEVICE_TYPE_::NMOS,   "B3SOIDD",        "B3SOIDD (BSIM3 SOI)"        };
    case TYPE::PMOS_B3SOIDD:         return { DEVICE_TYPE_::PMOS,   "B3SOIDD",        "B3SOIDD (BSIM3 SOI)"        };
    case TYPE::NMOS_B3SOIPD:         return { DEVICE_TYPE_::NMOS,   "B3SOIPD",        "B3SOIPD (BSIM3 PD-SOI)"     };
    case TYPE::PMOS_B3SOIPD:         return { DEVICE_TYPE_::PMOS,   "B3SOIPD",        "B3SOIPD (BSIM3 PD-SOI)"     };
    //case TYPE::NMOS_STAG:            return {};
    //case TYPE::PMOS_STAG:            return {};
    case TYPE::NMOS_HISIM2:          return { DEVICE_TYPE_::NMOS,   "HISIM2",         "HiSIM2"                     };
    case TYPE::PMOS_HISIM2:          return { DEVICE_TYPE_::PMOS,   "HISIM2",         "HiSIM2"                     };
    case TYPE::NMOS_HISIMHV1:        return { DEVICE_TYPE_::NMOS,   "HISIMHV1",       "HiSIM_HV1"                  };
    case TYPE::PMOS_HISIMHV1:        return { DEVICE_TYPE_::PMOS,   "HISIMHV1",       "HiSIM_HV1"                  };
    case TYPE::NMOS_HISIMHV2:        return { DEVICE_TYPE_::NMOS,   "HISIMHV2",       "HiSIM_HV2"                  };
    case TYPE::PMOS_HISIMHV2:        return { DEVICE_TYPE_::PMOS,   "HISIMHV2",       "HiSIM_HV2"                  };

    case TYPE::V:                 return { DEVICE_TYPE_::V,      "DC",             "DC",                        };
    case TYPE::V_SIN:                return { DEVICE_TYPE_::V,      "SIN",            "Sine"                       };
    case TYPE::V_PULSE:              return { DEVICE_TYPE_::V,      "PULSE",          "Pulse"                      };
    case TYPE::V_EXP:                return { DEVICE_TYPE_::V,      "EXP",            "Exponential"                };
    /*case TYPE::V_SFAM:               return { DEVICE_TYPE::V,      "SFAM",           "Single-frequency AM"        };
    case TYPE::V_SFFM:               return { DEVICE_TYPE::V,      "SFFM",           "Single-frequency FM"        };*/
    case TYPE::V_PWL:                return { DEVICE_TYPE_::V,      "PWL",            "Piecewise linear"           };
    case TYPE::V_WHITENOISE:         return { DEVICE_TYPE_::V,      "WHITENOISE",     "White noise"                };
    case TYPE::V_PINKNOISE:          return { DEVICE_TYPE_::V,      "PINKNOISE",      "Pink noise (1/f)"           };
    case TYPE::V_BURSTNOISE:         return { DEVICE_TYPE_::V,      "BURSTNOISE",     "Burst noise"                };
    case TYPE::V_RANDUNIFORM:        return { DEVICE_TYPE_::V,      "RANDUNIFORM",    "Random uniform"             };
    case TYPE::V_RANDNORMAL:         return { DEVICE_TYPE_::V,      "RANDNORMAL",     "Random normal"              };
    case TYPE::V_RANDEXP:            return { DEVICE_TYPE_::V,      "RANDEXP",        "Random exponential"         };
    //case TYPE::V_RANDPOISSON:        return { DEVICE_TYPE::V,      "RANDPOISSON",    "Random Poisson"             };
    case TYPE::V_BEHAVIORAL:         return { DEVICE_TYPE_::V,      "=",              "Behavioral"                 };

    case TYPE::I:                 return { DEVICE_TYPE_::I,      "DC",             "DC",                        };
    case TYPE::I_SIN:                return { DEVICE_TYPE_::I,      "SIN",            "Sine"                       };
    case TYPE::I_PULSE:              return { DEVICE_TYPE_::I,      "PULSE",          "Pulse"                      };
    case TYPE::I_EXP:                return { DEVICE_TYPE_::I,      "EXP",            "Exponential"                };
    /*case TYPE::I_SFAM:               return { DEVICE_TYPE::I,      "SFAM",           "Single-frequency AM"        };
    case TYPE::I_SFFM:               return { DEVICE_TYPE::I,      "SFFM",           "Single-frequency FM"        };*/
    case TYPE::I_PWL:                return { DEVICE_TYPE_::I,      "PWL",            "Piecewise linear"           };
    case TYPE::I_WHITENOISE:         return { DEVICE_TYPE_::I,      "WHITENOISE",     "White noise"                };
    case TYPE::I_PINKNOISE:          return { DEVICE_TYPE_::I,      "PINKNOISE",      "Pink noise (1/f)"           };
    case TYPE::I_BURSTNOISE:         return { DEVICE_TYPE_::I,      "BURSTNOISE",     "Burst noise"                };
    case TYPE::I_RANDUNIFORM:        return { DEVICE_TYPE_::I,      "RANDUNIFORM",    "Random uniform"             };
    case TYPE::I_RANDNORMAL:         return { DEVICE_TYPE_::I,      "RANDNORMAL",     "Random normal"              };
    case TYPE::I_RANDEXP:            return { DEVICE_TYPE_::I,      "RANDEXP",        "Random exponential"         };
    //case TYPE::I_RANDPOISSON:        return { DEVICE_TYPE::I,      "RANDPOISSON",    "Random Poisson"             };
    case TYPE::I_BEHAVIORAL:         return { DEVICE_TYPE_::I,      "=",              "Behavioral"                 };

    case TYPE::SUBCKT:               return { DEVICE_TYPE_::SUBCKT, "",               ""                           };
    case TYPE::XSPICE:               return { DEVICE_TYPE_::XSPICE, "",               ""                           };
    case TYPE::SPICE:                return { DEVICE_TYPE_::SPICE,  "",               ""                           };

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
    //case TYPE::R_ADV:                return { "R", "r"       };
    case TYPE::R_BEHAVIORAL:         return { "R", "",       "",        "0",   false, true   };

    case TYPE::C:                    return { "C", ""        };
    //case TYPE::C_ADV:                return { "C", "c",      };
    case TYPE::C_BEHAVIORAL:         return { "C", "",       "",        "0",   false, true   };

    case TYPE::L:                    return { "L", ""        };
    //case TYPE::L_ADV:                return { "L", "l"       };
    case TYPE::L_BEHAVIORAL:         return { "L", "",       "",        "0",   false, true   };
    
    case TYPE::TLINE_Z0:             return { "T"  };
    case TYPE::TLINE_RLGC:           return { "O", "ltra"    };

    case TYPE::SW_V:                 return { "S", "sw"  };
    case TYPE::SW_I:                 return { "W", "csw" };

    case TYPE::D:                    return { "D", "d"       };

    case TYPE::NPN_GUMMELPOON:       return { "Q", "npn",    "",        "1",  true   };
    case TYPE::PNP_GUMMELPOON:       return { "Q", "pnp",    "",        "1",  true   };

    case TYPE::NPN_VBIC:             return { "Q", "npn",    "",        "4"   };
    case TYPE::PNP_VBIC:             return { "Q", "pnp",    "",        "4"   };

    case TYPE::NPN_HICUM2:           return { "Q", "npn",    "",        "8"   };
    case TYPE::PNP_HICUM2:           return { "Q", "pnp",    "",        "8"   };

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

    case TYPE::V:                    return { "V", ""        };
    case TYPE::V_SIN:                return { "V", "",       "SIN"      };
    case TYPE::V_PULSE:              return { "V", "",       "PULSE"    };
    case TYPE::V_EXP:                return { "V", "",       "EXP"      };
    /*case TYPE::V_SFAM:               return { "V", "",       "AM"       };
    case TYPE::V_SFFM:               return { "V", "",       "SFFM"     };*/
    case TYPE::V_PWL:                return { "V", "",       "PWL"      };
    case TYPE::V_WHITENOISE:         return { "V", "",       "TRNOISE"  };
    case TYPE::V_PINKNOISE:          return { "V", "",       "TRNOISE"  };
    case TYPE::V_BURSTNOISE:         return { "V", "",       "TRNOISE"  };
    case TYPE::V_RANDUNIFORM:        return { "V", "",       "TRRANDOM" };
    case TYPE::V_RANDNORMAL:         return { "V", "",       "TRRANDOM" };
    case TYPE::V_RANDEXP:            return { "V", "",       "TRRANDOM" };
    //case TYPE::V_RANDPOISSON:        return { "V", "",       "TRRANDOM" };
    case TYPE::V_BEHAVIORAL:         return { "B"  };

    case TYPE::I:                    return { "I", ""        };
    case TYPE::I_PULSE:              return { "I", "",       "PULSE"    };
    case TYPE::I_SIN:                return { "I", "",       "SIN"      };
    case TYPE::I_EXP:                return { "I", "",       "EXP"      };
    /*case TYPE::I_SFAM:               return { "V", "",       "AM"       };
    case TYPE::I_SFFM:               return { "V", "",       "SFFM"     };*/
    case TYPE::I_PWL:                return { "I", "",       "PWL"      };
    case TYPE::I_WHITENOISE:         return { "I", "",       "TRNOISE"  };
    case TYPE::I_PINKNOISE:          return { "I", "",       "TRNOISE"  };
    case TYPE::I_BURSTNOISE:         return { "I", "",       "TRNOISE"  };
    case TYPE::I_RANDUNIFORM:        return { "I", "",       "TRRANDOM" };
    case TYPE::I_RANDNORMAL:         return { "I", "",       "TRRANDOM" };
    case TYPE::I_RANDEXP:            return { "I", "",       "TRRANDOM" };
    //case TYPE::I_RANDPOISSON:        return { "I", "",       "TRRANDOM" };
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


TYPE SIM_MODEL::ReadTypeFromSpiceCode( const wxString& aSpiceCode )
{
    tao::pegtl::string_input<> in( aSpiceCode.ToUTF8(), "Spice_Code" );
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
                    else if( paramName == "version" )
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

    if( deviceTypeFieldValue != "" )
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

    if( typeFieldValue != "" )
        return TYPE::NONE;

    // No type information. Look for legacy (pre-V7) fields.

    TYPE typeFromLegacyFields = InferTypeFromLegacyFields( aFields );
    if( typeFromLegacyFields != TYPE::NONE )
        return typeFromLegacyFields;

    // Still no type information.
    // For passives we infer the model from the mandatory fields in this case.
    return InferTypeFromRefAndValue( GetFieldValue( &aFields, REFERENCE_FIELD ),
                                     GetFieldValue( &aFields, VALUE_FIELD ) );
}


TYPE SIM_MODEL::InferTypeFromRefAndValue( const wxString& aRef, const wxString& aValue )
{
    static std::map<wxString, TYPE> refPrefixToType = {
        { "R", TYPE::R },
        { "C", TYPE::C },
        { "L", TYPE::L },
        { "TLINE", TYPE::TLINE_Z0 },
        { "VSIN", TYPE::V_SIN },
        { "VPULSE", TYPE::V_PULSE },
        { "VEXP", TYPE::V_EXP },
        /*{ "VSFAM", TYPE::V_SFAM },
        { "VSFFM", TYPE::V_SFFM },*/
        { "VPWL", TYPE::V_PWL },
        { "VWHITENOISE", TYPE::V_WHITENOISE },
        { "VPINKNOISE", TYPE::V_PINKNOISE },
        { "VBURSTNOISE", TYPE::V_BURSTNOISE },
        { "VRANDUNIFORM", TYPE::V_RANDUNIFORM },
        { "VRANDNORMAL", TYPE::V_RANDNORMAL },
        { "VRANDEXP", TYPE::V_RANDEXP },
        //{ "VRANDPOISSON", TYPE::V_RANDPOISSON },
        { "ISIN", TYPE::I_SIN },
        { "IPULSE", TYPE::I_PULSE },
        { "IEXP", TYPE::I_EXP },
        /*{ "ISFAM", TYPE::I_SFAM },
        { "ISFFM", TYPE::I_SFFM },*/
        { "IPWL", TYPE::I_PWL },
        { "IWHITENOISE", TYPE::I_WHITENOISE },
        { "IPINKNOISE", TYPE::I_PINKNOISE },
        { "IBURSTNOISE", TYPE::I_BURSTNOISE },
        { "IRANDUNIFORM", TYPE::I_RANDUNIFORM },
        { "IRANDNORMAL", TYPE::I_RANDNORMAL },
        { "IRANDEXP", TYPE::I_RANDEXP },
        //{ "IRANDPOISSON", TYPE::I_RANDPOISSON },
    };

    TYPE type = TYPE::NONE;

    for( auto&& [curPrefix, curType] : refPrefixToType )
    {
        if( aRef.StartsWith( curPrefix ) )
        {
            type = curType;
            break;
        }
    }

    // We handle "V" and "I" later because it collides and std::map is unordered.

    if( type == TYPE::NONE && aRef.StartsWith( "V" ) )
        type = TYPE::V;

    if( type == TYPE::NONE && aRef.StartsWith( "I" ) )
        type = TYPE::I;

    wxString value = aValue;

    // Some types have to be inferred from Value field.
    switch( type )
    {
    case TYPE::R:
        if( value.Trim( false ).StartsWith( "=" ) )
            type = TYPE::R_BEHAVIORAL;
        break;

    case TYPE::C:
        if( value.Trim( false ).StartsWith( "=" ) )
            type = TYPE::C_BEHAVIORAL;
        break;

    case TYPE::L:
        if( value.Trim( false ).StartsWith( "=" ) )
            type = TYPE::L_BEHAVIORAL;
        break;

    case TYPE::V:
        if( value.Trim( false ).StartsWith( "=" ) )
            type = TYPE::V_BEHAVIORAL;
        break;

    case TYPE::I:
        if( value.Trim( false ).StartsWith( "=" ) )
            type = TYPE::I_BEHAVIORAL;
        break;

    case TYPE::TLINE_Z0:
        try
        {
            tao::pegtl::string_input<> in( aValue.ToUTF8(), "Value" );
            auto root = tao::pegtl::parse_tree::parse<
                SIM_MODEL_PARSER::fieldParamValuePairsGrammar,
                SIM_MODEL_PARSER::fieldParamValuePairsSelector>
                    ( in );

            for( const auto& node : root->children )
            {
                if( node->is_type<SIM_MODEL_PARSER::param>()
                    && (node->string() == "r" || node->string() == "R"
                        || node->string() == "c" || node->string() == "C"
                        || node->string() == "l" || node->string() == "L" ) )
                {
                    type = TYPE::TLINE_RLGC;
                    break;
                }
            }
        }
        catch( const tao::pegtl::parse_error& e )
        {
        }

        break;

    default:
        break;
    }

    return type;
}


template <typename T>
TYPE SIM_MODEL::InferTypeFromLegacyFields( const std::vector<T>& aFields )
{
    if( GetFieldValue( &aFields, SIM_MODEL_SPICE::LEGACY_TYPE_FIELD ) != ""
        || GetFieldValue( &aFields, SIM_MODEL_SPICE::LEGACY_MODEL_FIELD ) != ""
        || GetFieldValue( &aFields, SIM_MODEL_SPICE::LEGACY_ENABLED_FIELD ) != ""
        || GetFieldValue( &aFields, SIM_MODEL_SPICE::LEGACY_LIB_FIELD ) != "" )
    {
        return TYPE::SPICE;
    }
    else
        return TYPE::NONE;
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


std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( TYPE aType, unsigned aSymbolPinCount )
{
    std::unique_ptr<SIM_MODEL> model = create( aType );

    // Passing nullptr to ReadDataFields will make it act as if all fields were empty.
    model->ReadDataFields( aSymbolPinCount, static_cast<const std::vector<void>*>( nullptr ) );
    return model;
}


std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const wxString& aSpiceCode )
{
    std::unique_ptr<SIM_MODEL> model = create( ReadTypeFromSpiceCode( aSpiceCode ) );

    try
    {
        model->ReadSpiceCode( aSpiceCode );
    }
    catch( const IO_ERROR& e )
    {
        wxLogDebug( "%s", e.What() );
        // Demote to raw Spice element and try again.
        std::unique_ptr<SIM_MODEL> rawSpiceModel = create( TYPE::SPICE );

        rawSpiceModel->ReadSpiceCode( aSpiceCode );
        return rawSpiceModel;
    }

    return model;
}


std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const SIM_MODEL& aBaseModel,
                                              unsigned         aSymbolPinCount )
{
    std::unique_ptr<SIM_MODEL> model = create( aBaseModel.GetType() );

    model->SetBaseModel( aBaseModel );
    model->ReadDataFields( aSymbolPinCount, static_cast<const std::vector<void>*>( nullptr ) );
    return model;
}


template <typename T>
std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const SIM_MODEL& aBaseModel, unsigned aSymbolPinCount,
                                              const std::vector<T>& aFields )
{
    std::unique_ptr<SIM_MODEL> model = create( aBaseModel.GetType() );

    model->SetBaseModel( aBaseModel );
    model->ReadDataFields( aSymbolPinCount, &aFields );
    return model;
}

template std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const SIM_MODEL& aBaseModel,
                                                       unsigned aSymbolPinCount,
                                                       const std::vector<SCH_FIELD>& aFields );

template std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const SIM_MODEL& aBaseModel,
                                                       unsigned aSymbolPinCount,
                                                       const std::vector<LIB_FIELD>& aFields );


template <typename T>
std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( unsigned aSymbolPinCount,
                                              const std::vector<T>& aFields )
{
    std::unique_ptr<SIM_MODEL> model = SIM_MODEL::create( ReadTypeFromFields( aFields ) );

    model->ReadDataFields( aSymbolPinCount, &aFields );
    return model;
}

template std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( unsigned aSymbolPinCount,
                                                       const std::vector<SCH_FIELD>& aFields );
template std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( unsigned aSymbolPinCount,
                                                       const std::vector<LIB_FIELD>& aFields );


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


void SIM_MODEL::ReadSpiceCode( const wxString& aSpiceCode )
{
    // The default behavior is to treat the Spice param=value pairs as the model parameters and
    // values (for many models the correspondence is not exact, so this function is overridden).

    tao::pegtl::string_input<> in( aSpiceCode.ToUTF8(), "Spice_Code" );
    std::unique_ptr<tao::pegtl::parse_tree::node> root;

    try
    {
        root = tao::pegtl::parse_tree::parse<SIM_MODEL_SPICE_PARSER::spiceUnitGrammar,
                                             SIM_MODEL_SPICE_PARSER::spiceUnitSelector>
            ( in );
    }
    catch( tao::pegtl::parse_error& e )
    {
        THROW_IO_ERROR( e.what() );
    }

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
                        THROW_IO_ERROR( wxString::Format(
                                            _( "Failed to set parameter '%s' to '%s'" ),
                                            paramName,
                                            subnode->string() ) );
                    }
                }
                else
                {
                    wxFAIL_MSG( "Unhandled parse tree subnode" );
                }
            }
        }
        else
        {
            wxFAIL_MSG( "Unhandled parse tree node" );
        }
    }

    m_spiceCode = aSpiceCode;
}


wxString SIM_MODEL::GenerateSpiceModelLine( const wxString& aModelName ) const
{
    LOCALE_IO toggle;

    if( !HasSpiceNonInstanceOverrides() && !requiresSpiceModel() )
        return "";

    wxString result = "";
    wxString line = "";

    line << wxString::Format( ".model %s %s(\n+", aModelName, GetSpiceInfo().modelType );

    for( const PARAM& param : GetParams() )
    {
        if( param.info.isSpiceInstanceParam )
            continue;

        wxString name = ( param.info.spiceModelName == "" ) ?
            param.info.name : param.info.spiceModelName;
        wxString value = param.value->ToSpiceString();

        if( value == "" )
            continue;
        
        wxString appendix = " " + name + "=" + value;

        if( line.Length() + appendix.Length() > 60 )
        {
            result << line << "\n";
            line = "+" + appendix;
        }
        else
            line << appendix;
    }

    result << line + " )\n";
    return result;
}


wxString SIM_MODEL::GenerateSpiceItemName( const wxString& aRefName ) const
{
    if( aRefName != "" && aRefName.StartsWith( GetSpiceInfo().itemType ) )
        return aRefName;
    else
        return GetSpiceInfo().itemType + aRefName;
}


wxString SIM_MODEL::GenerateSpiceItemParamValuePair( const PARAM& aParam, bool& aIsFirst ) const
{
    wxString result;

    if( aIsFirst )
        aIsFirst = false;
    else
        result << " ";

    wxString name = ( aParam.info.spiceInstanceName == "" ) ?
        aParam.info.name : aParam.info.spiceInstanceName;
    wxString value = aParam.value->ToSpiceString();

    if( value != "" )
        result << name << "=" << value;

    return result;
}


wxString SIM_MODEL::GenerateSpiceItemLine( const wxString& aRefName,
                                           const wxString& aModelName ) const
{
    // Use linear symbol pin numbers enumeration. Used in model preview.

    std::vector<wxString> pinNumbers;

    for( int i = 0; i < GetPinCount(); ++i )
        pinNumbers.push_back( wxString::FromCDouble( i + 1 ) );

    return GenerateSpiceItemLine( aRefName, aModelName, pinNumbers );
}


wxString SIM_MODEL::GenerateSpiceItemLine( const wxString& aRefName,
                                           const wxString& aModelName,
                                           const std::vector<wxString>& aSymbolPinNumbers ) const
{
    std::vector<wxString> pinNames;

    for( const PIN& pin : GetPins() )
        pinNames.push_back( pin.name );

    return GenerateSpiceItemLine( aRefName, aModelName, aSymbolPinNumbers, pinNames );
}


wxString SIM_MODEL::GenerateSpiceItemLine( const wxString& aRefName,
                                           const wxString& aModelName,
                                           const std::vector<wxString>& aSymbolPinNumbers,
                                           const std::vector<wxString>& aPinNetNames ) const
{
    wxString result;
    result << GenerateSpiceItemName( aRefName ) << " ";

    int ncCounter = 0;

    for( const PIN& pin : GetSpicePins() )
    {
        auto it = std::find( aSymbolPinNumbers.begin(), aSymbolPinNumbers.end(),
                             pin.symbolPinNumber );

        if( it == aSymbolPinNumbers.end() )
        {
            LOCALE_IO toggle;
            result << wxString::Format( "NC-%s-%u ", aRefName, ncCounter++ );
        }
        else
        {
            long symbolPinIndex = std::distance( aSymbolPinNumbers.begin(), it );
            wxString netName = aPinNetNames.at( symbolPinIndex );
            result << netName << " ";
        }
    }

    if( requiresSpiceModel() )
        result << aModelName << " ";

    bool isFirst = false;

    for( const PARAM& param : GetParams() )
    {
        if( !param.info.isSpiceInstanceParam )
            continue;

        result << GenerateSpiceItemParamValuePair( param, isFirst );
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
    wxString spiceCode = GenerateSpiceModelLine( aModelName );

    if( spiceCode == "" )
        spiceCode = m_spiceCode;

    if( spiceCode == "" && GetBaseModel() )
        spiceCode = GetBaseModel()->m_spiceCode;

    wxString itemLine = GenerateSpiceItemLine( "", aModelName );
    if( spiceCode != "" )
        spiceCode << "\n";

    spiceCode << itemLine;
    return spiceCode.Trim();
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


int SIM_MODEL::FindModelPinIndex( const wxString& aSymbolPinNumber )
{
    for( int modelPinIndex = 0; modelPinIndex < GetPinCount(); ++modelPinIndex )
    {
        if( GetPin( modelPinIndex ).symbolPinNumber == aSymbolPinNumber )
            return modelPinIndex;
    }

    return PIN::NOT_CONNECTED;
}


void SIM_MODEL::AddParam( const PARAM::INFO& aInfo, bool aIsOtherVariant )
{
    m_params.emplace_back( aInfo, aIsOtherVariant );
}


std::vector<std::reference_wrapper<const SIM_MODEL::PIN>> SIM_MODEL::GetPins() const
{
    std::vector<std::reference_wrapper<const PIN>> pins;

    for( int modelPinIndex = 0; modelPinIndex < GetPinCount(); ++modelPinIndex )
        pins.emplace_back( GetPin( modelPinIndex ) );

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

    for( int i = 0; i < GetParamCount(); ++i )
        params.push_back( GetParam( i ) );

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


bool SIM_MODEL::SetParamValue( const wxString& aParamName, const wxString& aValue,
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

    return SetParamValue( it - params.begin(), aValue, aNotation );
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


bool SIM_MODEL::HasSpiceNonInstanceOverrides() const
{
    for( const PARAM& param : m_params )
    {
        if( !param.info.isSpiceInstanceParam && param.value->ToString() != "" )
            return true;
    }

    return false;
}


SIM_MODEL::SIM_MODEL( TYPE aType ) : m_baseModel( nullptr ), m_type( aType ),
                                     m_isEnabled( true ), m_isInferred( false )
{
}


void SIM_MODEL::CreatePins( unsigned aSymbolPinCount )
{
    // Default pin sequence: model pins are the same as symbol pins.
    // Excess model pins are set as Not Connected.
    // Note that intentionally nothing is added if `getPinNames()` returns an empty vector.

    // SIM_MODEL pins must be ordered by symbol pin numbers -- this is assumed by code that
    // accesses them.

    for( unsigned modelPinIndex = 0; modelPinIndex < getPinNames().size(); ++modelPinIndex )
    {
        if( modelPinIndex < aSymbolPinCount )
            AddPin( { getPinNames().at( modelPinIndex ), wxString::FromCDouble( modelPinIndex + 1 ) } );
        else
            AddPin( { getPinNames().at( modelPinIndex ), "" } );
    }
}


template void SIM_MODEL::WriteInferredDataFields( std::vector<SCH_FIELD>& aFields,
                                                  const wxString& aValue ) const;
template void SIM_MODEL::WriteInferredDataFields( std::vector<LIB_FIELD>& aFields,
                                                  const wxString& aValue ) const;

template <typename T>
void SIM_MODEL::WriteInferredDataFields( std::vector<T>& aFields, const wxString& aValue ) const
{
    if( GetPinCount() == 2
        && GetPin( 0 ).symbolPinNumber == "1"
        && GetPin( 1 ).symbolPinNumber == "2" )
    {
        SetFieldValue( aFields, PINS_FIELD, "" );
    }

    SetFieldValue( aFields, VALUE_FIELD, aValue );
    SetFieldValue( aFields, DEVICE_TYPE_FIELD, "" );
    SetFieldValue( aFields, TYPE_FIELD, "" );
    SetFieldValue( aFields, PARAMS_FIELD, "" );
    SetFieldValue( aFields, DISABLED_FIELD, "" );
}


wxString SIM_MODEL::GenerateParamValuePair( const PARAM& aParam, bool& aIsFirst ) const
{
    wxString result;

    if( aIsFirst )
        aIsFirst = false;
    else
        result << " ";

    wxString name = aParam.info.name;

    // Because of collisions with instance parameters, we append some model parameters with "_".
    if( aParam.info.name.EndsWith( "_" ) )
        name = aParam.info.name.BeforeLast( '_' );

    wxString value = aParam.value->ToString();
    if( value.Contains( " " ) )
        value = "\"" + value + "\"";

    result << aParam.info.name + "=" + value;
    return result;
}


wxString SIM_MODEL::GenerateParamsField( const wxString& aPairSeparator ) const
{
    wxString result;
    bool isFirst = true;

    for( const PARAM& param : m_params )
    {
        if( param.value->ToString() == "" )
            continue;

        result << GenerateParamValuePair( param, isFirst );
    }

    return result;
}


void SIM_MODEL::ParseParamsField( const wxString& aParamsField )
{
    LOCALE_IO toggle;

    tao::pegtl::string_input<> in( aParamsField.ToUTF8(), "Sim_Params" );
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
        THROW_IO_ERROR( e.what() );
    }

    wxString paramName;

    for( const auto& node : root->children )
    {
        if( node->is_type<SIM_MODEL_PARSER::param>() )
            paramName = node->string();
        // TODO: Do something with number<SIM_VALUE::TYPE_INT, ...>.
        // It doesn't seem too useful?
        else if( node->is_type<SIM_MODEL_PARSER::quotedStringContent>() 
            || node->is_type<SIM_MODEL_PARSER::unquotedString>() )
        {
            wxASSERT( paramName != "" );
            // TODO: Shouldn't be named "...fromSpiceCode" here...

            SetParamValue( paramName, node->string(), SIM_VALUE_GRAMMAR::NOTATION::SI );
        }
        else if( node->is_type<SIM_MODEL_PARSER::quotedString>() )
        {
            wxASSERT( !paramName.IsEmpty() );

            wxString str = node->string();

            // Unescape quotes.
            str.Replace( "\\\"", "\"" );

            SetParamValue( paramName, str, SIM_VALUE_GRAMMAR::NOTATION::SI );
        }
        else
        {
            wxFAIL;
        }
    }
}


void SIM_MODEL::ParsePinsField( unsigned aSymbolPinCount, const wxString& aPinsField )
{
    CreatePins( aSymbolPinCount );

    if( aPinsField == "" )
        return;

    tao::pegtl::string_input<> in( aPinsField.ToUTF8(), PINS_FIELD );
    std::unique_ptr<tao::pegtl::parse_tree::node> root;

    try
    {
        root = tao::pegtl::parse_tree::parse<SIM_MODEL_PARSER::pinSequenceGrammar,
                                             SIM_MODEL_PARSER::pinSequenceSelector>( in );
    }
    catch( const tao::pegtl::parse_error& e )
    {
        THROW_IO_ERROR( e.what() );
    }

    if( static_cast<int>( root->children.size() ) != GetPinCount() )
    {
        THROW_IO_ERROR( wxString::Format( _( "%s describes %lu pins, expected %u" ),
                                          PINS_FIELD,
                                          root->children.size(),
                                          GetPinCount() ) );
    }

    for( int pinIndex = 0; pinIndex < static_cast<int>( root->children.size() ); ++pinIndex )
    {
        if( root->children.at( pinIndex )->string() == "~" )
            SetPinSymbolPinNumber( pinIndex, "" );
        else
            SetPinSymbolPinNumber( pinIndex, root->children.at( pinIndex )->string() );
    }
}


void SIM_MODEL::ParseDisabledField( const wxString& aDisabledField )
{
    if( aDisabledField == "" )
        return;

    char c = aDisabledField.Lower()[0];

    if( c == 'y' || c == 't' || c == '1' )
        m_isEnabled = false;
}


bool SIM_MODEL::SetParamFromSpiceCode( const wxString& aParamName, const wxString& aParamValue,
                                       SIM_VALUE_GRAMMAR::NOTATION aNotation )
{
    return SetParamValue( aParamName, aParamValue, aNotation );
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
    
    case TYPE::TLINE_Z0:
    case TYPE::TLINE_RLGC:
        return std::make_unique<SIM_MODEL_TLINE>( aType );

    case TYPE::SW_V:
    case TYPE::SW_I:
        return std::make_unique<SIM_MODEL_SWITCH>( aType );

    case TYPE::V:
    case TYPE::I:
    case TYPE::V_SIN:
    case TYPE::I_SIN:
    case TYPE::V_PULSE:
    case TYPE::I_PULSE:
    case TYPE::V_EXP:
    case TYPE::I_EXP:
    /*case TYPE::V_SFAM:
    case TYPE::I_SFAM:
    case TYPE::V_SFFM:
    case TYPE::I_SFFM:*/
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
    //case TYPE::V_RANDPOISSON:
    //case TYPE::I_RANDPOISSON:
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
    std::unique_ptr<SIM_VALUE> readLevel = SIM_VALUE::Create( SIM_VALUE::TYPE_INT, aLevel );

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
    ParseDisabledField( GetFieldValue( aFields, DISABLED_FIELD ) );

    if( GetFieldValue( aFields, PARAMS_FIELD ) != "" )
    {
        ParsePinsField( aSymbolPinCount, GetFieldValue( aFields, PINS_FIELD ) );
        ParseParamsField( GetFieldValue( aFields, PARAMS_FIELD ) );
    }
    else
        InferredReadDataFields( aSymbolPinCount, aFields, true );
}


template <typename T>
void SIM_MODEL::InferredReadDataFields( unsigned aSymbolPinCount, const std::vector<T>* aFields,
                                        bool aAllowOnlyFirstValue,
                                        bool aAllowParamValuePairs )
{
    ParsePinsField( aSymbolPinCount, GetFieldValue( aFields, PINS_FIELD ) );

    if( InferTypeFromRefAndValue( GetFieldValue( aFields, REFERENCE_FIELD ),
                                  GetFieldValue( aFields, VALUE_FIELD ) ) != GetType() )
    {
        // Not an inferred model. Nothing to do here.
        return;
    }

    wxString valueField = GetFieldValue( aFields, VALUE_FIELD );

    if( aAllowParamValuePairs ) // The usual param-value pairs have precedence.
    {
        try
        {
            ParseParamsField( GetFieldValue( aFields, VALUE_FIELD ) );
        }
        catch( const IO_ERROR& e )
        {
            if( aAllowOnlyFirstValue )
                SetParamValue( 0, parseFieldFloatValue( valueField ) );
            else
                throw e;
        }
    }
    else if( aAllowOnlyFirstValue )
    {
        // This is reached only when model allows only the first value.
        SetParamValue( 0, parseFieldFloatValue( valueField ) );
    }

    m_isInferred = true;
}

template void SIM_MODEL::InferredReadDataFields( unsigned aSymbolPinCount,
                                                 const std::vector<SCH_FIELD>* aFields,
                                                 bool aAllowOnlyFirstValue,
                                                 bool aAllowParamValuePairs );
template void SIM_MODEL::InferredReadDataFields( unsigned aSymbolPinCount,
                                                 const std::vector<LIB_FIELD>* aFields,
                                                 bool aAllowOnlyFirstValue,
                                                 bool aAllowParamValuePairs );


template <typename T>
void SIM_MODEL::doWriteFields( std::vector<T>& aFields ) const
{
    SetFieldValue( aFields, DEVICE_TYPE_FIELD, generateDeviceTypeField() );
    SetFieldValue( aFields, TYPE_FIELD, generateTypeField() );
    SetFieldValue( aFields, PINS_FIELD, generatePinsField() );
    SetFieldValue( aFields, PARAMS_FIELD, GenerateParamsField( " " ) );
    SetFieldValue( aFields, DISABLED_FIELD, generateDisabledField() );
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

    for( const PIN& pin : GetPins() )
    {
        if( isFirst )
            isFirst = false;
        else
            result << " ";

        if( pin.symbolPinNumber == "" )
            result << "~";
        else
            result << pin.symbolPinNumber; // Note that it's numbered from 1.
    }

    return result;
}


wxString SIM_MODEL::generateDisabledField() const
{
    return m_isEnabled ? "" : "1";
}


wxString SIM_MODEL::parseFieldFloatValue( wxString aFieldFloatValue )
{
    try
    {
        tao::pegtl::string_input<> in( aFieldFloatValue.ToUTF8(), "Value" );
        auto root = tao::pegtl::parse_tree::parse<
                SIM_MODEL_PARSER::fieldFloatValueGrammar,
                SIM_MODEL_PARSER::fieldFloatValueSelector>
            ( in );

        return root->children[0]->string();
    }
    catch( const tao::pegtl::parse_error& )
    {
        THROW_IO_ERROR( wxString::Format( _( "Failed to infer model from Value '%s'" ),
                                          aFieldFloatValue ) );
    }
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

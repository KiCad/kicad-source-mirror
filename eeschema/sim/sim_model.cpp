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
#include <sim/sim_model_mutual_inductor.h>
#include <sim/sim_model_ngspice.h>
#include <sim/sim_model_kibis.h>
#include <sim/sim_model_source.h>
#include <sim/sim_model_raw_spice.h>
#include <sim/sim_model_subckt.h>
#include <sim/sim_model_switch.h>
#include <sim/sim_model_tline.h>
#include <sim/sim_model_xspice.h>

#include <lib_symbol.h>
#include <confirm.h>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>
#include <fmt/core.h>
#include <pegtl.hpp>
#include <pegtl/contrib/parse_tree.hpp>

#include <iterator>

using DEVICE_TYPE = SIM_MODEL::DEVICE_TYPE_;
using TYPE = SIM_MODEL::TYPE;


namespace SIM_MODEL_PARSER
{
    using namespace SIM_MODEL_GRAMMAR;

    template <typename Rule> struct fieldParamValuePairsSelector : std::false_type {};


    template <typename Rule> struct pinSequenceSelector : std::false_type {};
    template <> struct pinSequenceSelector<pinNumber> : std::true_type {};


    template <typename Rule> struct fieldFloatValueSelector : std::false_type {};
    template <> struct fieldFloatValueSelector<number<SIM_VALUE::TYPE_FLOAT, NOTATION::SI>>
        : std::true_type {};
    template <> struct fieldParamValuePairsSelector<param> : std::true_type {};
    template <> struct fieldParamValuePairsSelector<quotedStringContent> : std::true_type {};
    template <> struct fieldParamValuePairsSelector<unquotedString> : std::true_type {};


    template <typename Rule> struct fieldInferValueSelector : std::false_type {};
    template <> struct fieldInferValueSelector<fieldFloatValue> : std::true_type {};
    template <> struct fieldInferValueSelector<number<SIM_VALUE::TYPE_FLOAT, NOTATION::SI>> : std::true_type {};
    template <> struct fieldInferValueSelector<fieldInferValuePrefix> : std::true_type {};
    template <> struct fieldInferValueSelector<fieldParamValuePairs> : std::true_type {};
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

        case DEVICE_TYPE_::KIBIS:     return { "IBIS",  "Ibis Model" };

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
    case TYPE::R_BEHAVIORAL:         return { DEVICE_TYPE_::R,      "=",              "Behavioral"                 };

    case TYPE::C:                    return { DEVICE_TYPE_::C,      "",               "Ideal"                      };
    case TYPE::C_BEHAVIORAL:         return { DEVICE_TYPE_::C,      "=",              "Behavioral"                 };

    case TYPE::L:                    return { DEVICE_TYPE_::L,      "",               "Ideal"                      };
    case TYPE::L_MUTUAL:             return { DEVICE_TYPE_::L,      "MUTUAL",         "Mutual"                     };
    case TYPE::L_BEHAVIORAL:         return { DEVICE_TYPE_::L,      "=",              "Behavioral"                 };

    case TYPE::TLINE_Z0:             return { DEVICE_TYPE_::TLINE,  "",               "Characteristic impedance"   };
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

    case TYPE::V:                    return { DEVICE_TYPE_::V,      "",               "DC",                        };
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

    case TYPE::I:                    return { DEVICE_TYPE_::I,      "",               "DC",                        };
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

    case TYPE::KIBIS_DEVICE:         return { DEVICE_TYPE_::KIBIS,  "IBISDEVICE",     "Device"                     };
    case TYPE::KIBIS_DRIVER_DC:      return { DEVICE_TYPE_::KIBIS,  "IBISDRIVERDC",   "DC driver"                  };
    case TYPE::KIBIS_DRIVER_RECT:    return { DEVICE_TYPE_::KIBIS,  "IBISDRIVERRECT", "Rectangular wave driver"    };
    case TYPE::KIBIS_DRIVER_PRBS:    return { DEVICE_TYPE_::KIBIS,  "IBISDRIVERPRBS", "PRBS driver"                };
    case TYPE::KIBIS_DIFFDEVICE:     return { DEVICE_TYPE_::KIBIS,  "IBISDIFFDEVICE", "Differential device"        };
    case TYPE::KIBIS_DIFFDRIVER:     return { DEVICE_TYPE_::KIBIS,  "IBISDIFFDRIVER", "Differential driver"        };

    case TYPE::RAWSPICE:             return { DEVICE_TYPE_::SPICE,  "",               ""                           };

    case TYPE::_ENUM_END:            break;
    }

    wxFAIL;
    return {};
}


SIM_MODEL::SPICE_INFO SIM_MODEL::SpiceInfo( TYPE aType )
{
    switch( aType )
    {
    case TYPE::R:                    return { "R", ""        };
    case TYPE::R_BEHAVIORAL:         return { "R", "",       "",        "0",   false, true   };

    case TYPE::C:                    return { "C", ""        };
    case TYPE::C_BEHAVIORAL:         return { "C", "",       "",        "0",   false, true   };

    case TYPE::L:                    return { "L", ""        };
    case TYPE::L_MUTUAL:             return { "K", ""        };
    case TYPE::L_BEHAVIORAL:         return { "L", "",       "",        "0",   false, true   };

    //case TYPE::TLINE_Z0:             return { "T"  };
    case TYPE::TLINE_Z0:             return { "O", "LTRA"    };
    case TYPE::TLINE_RLGC:           return { "O", "LTRA"    };

    case TYPE::SW_V:                 return { "S", "SW"  };
    case TYPE::SW_I:                 return { "W", "CSW" };

    case TYPE::D:                    return { "D", "D"       };

    case TYPE::NPN_GUMMELPOON:       return { "Q", "NPN",    "",        "1",  true   };
    case TYPE::PNP_GUMMELPOON:       return { "Q", "PNP",    "",        "1",  true   };

    case TYPE::NPN_VBIC:             return { "Q", "NPN",    "",        "4"   };
    case TYPE::PNP_VBIC:             return { "Q", "PNP",    "",        "4"   };

    case TYPE::NPN_HICUM2:           return { "Q", "NPN",    "",        "8"   };
    case TYPE::PNP_HICUM2:           return { "Q", "PNP",    "",        "8"   };

    case TYPE::NJFET_SHICHMANHODGES: return { "M", "NJF",    "",        "1"   };
    case TYPE::PJFET_SHICHMANHODGES: return { "M", "PJF",    "",        "1"   };
    case TYPE::NJFET_PARKERSKELLERN: return { "M", "NJF",    "",        "2"   };
    case TYPE::PJFET_PARKERSKELLERN: return { "M", "PJF",    "",        "2"   };

    case TYPE::NMES_STATZ:           return { "Z", "NMF",    "",        "1"   };
    case TYPE::PMES_STATZ:           return { "Z", "PMF",    "",        "1"   };
    case TYPE::NMES_YTTERDAL:        return { "Z", "NMF",    "",        "2"   };
    case TYPE::PMES_YTTERDAL:        return { "Z", "PMF",    "",        "2"   };
    case TYPE::NMES_HFET1:           return { "Z", "NMF",    "",        "5"   };
    case TYPE::PMES_HFET1:           return { "Z", "PMF",    "",        "5"   };
    case TYPE::NMES_HFET2:           return { "Z", "NMF",    "",        "6"   };
    case TYPE::PMES_HFET2:           return { "Z", "PMF",    "",        "6"   };

    case TYPE::NMOS_MOS1:            return { "M", "NMOS",   "",        "1"   };
    case TYPE::PMOS_MOS1:            return { "M", "PMOS",   "",        "1"   };
    case TYPE::NMOS_MOS2:            return { "M", "NMOS",   "",        "2"   };
    case TYPE::PMOS_MOS2:            return { "M", "PMOS",   "",        "2"   };
    case TYPE::NMOS_MOS3:            return { "M", "NMOS",   "",        "3"   };
    case TYPE::PMOS_MOS3:            return { "M", "PMOS",   "",        "3"   };
    case TYPE::NMOS_BSIM1:           return { "M", "NMOS",   "",        "4"   };
    case TYPE::PMOS_BSIM1:           return { "M", "PMOS",   "",        "4"   };
    case TYPE::NMOS_BSIM2:           return { "M", "NMOS",   "",        "5"   };
    case TYPE::PMOS_BSIM2:           return { "M", "PMOS",   "",        "5"   };
    case TYPE::NMOS_MOS6:            return { "M", "NMOS",   "",        "6"   };
    case TYPE::PMOS_MOS6:            return { "M", "PMOS",   "",        "6"   };
    case TYPE::NMOS_BSIM3:           return { "M", "NMOS",   "",        "8"   };
    case TYPE::PMOS_BSIM3:           return { "M", "PMOS",   "",        "8"   };
    case TYPE::NMOS_MOS9:            return { "M", "NMOS",   "",        "9"   };
    case TYPE::PMOS_MOS9:            return { "M", "PMOS",   "",        "9"   };
    case TYPE::NMOS_B4SOI:           return { "M", "NMOS",   "",        "10"  };
    case TYPE::PMOS_B4SOI:           return { "M", "PMOS",   "",        "10"  };
    case TYPE::NMOS_BSIM4:           return { "M", "NMOS",   "",        "14"  };
    case TYPE::PMOS_BSIM4:           return { "M", "PMOS",   "",        "14"  };
    //case TYPE::NMOS_EKV2_6:          return {};
    //case TYPE::PMOS_EKV2_6:          return {};
    //case TYPE::NMOS_PSP:             return {};
    //case TYPE::PMOS_PSP:             return {};
    case TYPE::NMOS_B3SOIFD:         return { "M", "NMOS",   "",        "55"  };
    case TYPE::PMOS_B3SOIFD:         return { "M", "PMOS",   "",        "55"  };
    case TYPE::NMOS_B3SOIDD:         return { "M", "NMOS",   "",        "56"  };
    case TYPE::PMOS_B3SOIDD:         return { "M", "PMOS",   "",        "56"  };
    case TYPE::NMOS_B3SOIPD:         return { "M", "NMOS",   "",        "57"  };
    case TYPE::PMOS_B3SOIPD:         return { "M", "PMOS",   "",        "57"  };
    //case TYPE::NMOS_STAG:            return {};
    //case TYPE::PMOS_STAG:            return {};
    case TYPE::NMOS_HISIM2:          return { "M", "NMOS",   "",        "68"  };
    case TYPE::PMOS_HISIM2:          return { "M", "PMOS",   "",        "68"  };
    case TYPE::NMOS_HISIMHV1:        return { "M", "NMOS",   "",        "73", true,  false, "1.2.4" };
    case TYPE::PMOS_HISIMHV1:        return { "M", "PMOS",   "",        "73", true,  false, "1.2.4" };
    case TYPE::NMOS_HISIMHV2:        return { "M", "NMOS",   "",        "73", true,  false, "2.2.0" };
    case TYPE::PMOS_HISIMHV2:        return { "M", "PMOS",   "",        "73", true,  false, "2.2.0" };

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

    case TYPE::KIBIS_DEVICE:         return { "X"  };
    case TYPE::KIBIS_DRIVER_DC:      return { "X"  };
    case TYPE::KIBIS_DRIVER_RECT:    return { "X"  };
    case TYPE::KIBIS_DRIVER_PRBS:    return { "X"  };
    case TYPE::KIBIS_DIFFDEVICE:     return { "X"  };
    case TYPE::KIBIS_DIFFDRIVER:     return { "X"  };

    case TYPE::NONE:
    case TYPE::RAWSPICE:
        return {};

    case TYPE::_ENUM_END:
        break;
    }

    wxFAIL;
    return {};
}


template TYPE SIM_MODEL::ReadTypeFromFields( const std::vector<SCH_FIELD>& aFields );
template TYPE SIM_MODEL::ReadTypeFromFields( const std::vector<LIB_FIELD>& aFields );

template <typename T>
TYPE SIM_MODEL::ReadTypeFromFields( const std::vector<T>& aFields )
{
    std::string deviceTypeFieldValue = GetFieldValue( &aFields, DEVICE_TYPE_FIELD );
    std::string typeFieldValue = GetFieldValue( &aFields, TYPE_FIELD );

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
    // We try to infer the model from the mandatory fields in this case.
    return InferTypeFromRefAndValue( GetFieldValue( &aFields, REFERENCE_FIELD ),
                                     GetFieldValue( &aFields, VALUE_FIELD ) );
}


TYPE SIM_MODEL::InferTypeFromRefAndValue( const std::string& aRef, const std::string& aValue )
{
    std::string prefix;

    try
    {
        tao::pegtl::string_input<> in( aValue, VALUE_FIELD );
        auto root = tao::pegtl::parse_tree::parse<SIM_MODEL_PARSER::fieldInferValueGrammar,
                                                  SIM_MODEL_PARSER::fieldInferValueSelector,
                                                  tao::pegtl::nothing,
                                                  SIM_MODEL_PARSER::control>( in );

        for( const auto& node : root->children )
        {
            if( node->is_type<SIM_MODEL_PARSER::fieldInferValuePrefix>() )
                prefix = node->string();
        }
    }
    catch( const tao::pegtl::parse_error& )
    {
    }

    DEVICE_TYPE_ deviceType = DEVICE_TYPE_::NONE;

    if( boost::starts_with( aRef, "R" ) )
        deviceType  = DEVICE_TYPE_::R;
    else if( boost::starts_with( aRef, "C" ) )
        deviceType = DEVICE_TYPE_::C;
    else if( boost::starts_with( aRef, "L" ) )
        deviceType = DEVICE_TYPE_::L;
    else if( boost::starts_with( aRef, "V" ) )
        deviceType = DEVICE_TYPE_::V;
    else if( boost::starts_with( aRef, "I" ) )
        deviceType = DEVICE_TYPE_::I;
    else if( boost::starts_with( aRef, "TL" ) )
        deviceType = DEVICE_TYPE_::TLINE;
    else
        return TYPE::NONE;

    for( TYPE type : TYPE_ITERATOR() )
    {
        if( TypeInfo( type ).deviceType == deviceType && TypeInfo( type ).fieldValue == prefix )
            return type;
    }

    return TYPE::NONE;
}


template <typename T>
TYPE SIM_MODEL::InferTypeFromLegacyFields( const std::vector<T>& aFields )
{
    if( GetFieldValue( &aFields, SIM_MODEL_RAW_SPICE::LEGACY_TYPE_FIELD ) != ""
        || GetFieldValue( &aFields, SIM_MODEL_RAW_SPICE::LEGACY_MODEL_FIELD ) != ""
        || GetFieldValue( &aFields, SIM_MODEL_RAW_SPICE::LEGACY_ENABLED_FIELD ) != ""
        || GetFieldValue( &aFields, SIM_MODEL_RAW_SPICE::LEGACY_LIB_FIELD ) != "" )
    {
        return TYPE::RAWSPICE;
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
    std::unique_ptr<SIM_MODEL> model = Create( aType );

    // Passing nullptr to ReadDataFields will make it act as if all fields were empty.
    model->ReadDataFields( aSymbolPinCount, static_cast<const std::vector<void>*>( nullptr ) );
    return model;
}


std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const SIM_MODEL& aBaseModel,
                                              unsigned         aSymbolPinCount )
{
    std::unique_ptr<SIM_MODEL> model = Create( aBaseModel.GetType() );

    model->SetBaseModel( aBaseModel );
    model->ReadDataFields( aSymbolPinCount, static_cast<const std::vector<void>*>( nullptr ) );
    return model;
}


template <typename T>
std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const SIM_MODEL& aBaseModel, unsigned aSymbolPinCount,
                                              const std::vector<T>& aFields )
{
    TYPE type = ReadTypeFromFields( aFields );

    // If the model has a specified type, it takes priority over the type of its base class.
    if( type == TYPE::NONE )
        type = aBaseModel.GetType();

    std::unique_ptr<SIM_MODEL> model = Create( type );

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
    TYPE type = ReadTypeFromFields( aFields );

    if( type == TYPE::NONE )
        THROW_IO_ERROR( wxString::Format( _( "Failed to read simulation model from fields" ) ) );

    std::unique_ptr<SIM_MODEL> model = SIM_MODEL::Create( type );

    model->ReadDataFields( aSymbolPinCount, &aFields );
    return model;
}

template std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( unsigned aSymbolPinCount,
                                                       const std::vector<SCH_FIELD>& aFields );
template std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( unsigned aSymbolPinCount,
                                                       const std::vector<LIB_FIELD>& aFields );


template <typename T>
std::string SIM_MODEL::GetFieldValue( const std::vector<T>* aFields, const std::string& aFieldName )
{
    static_assert( std::is_same<T, SCH_FIELD>::value || std::is_same<T, LIB_FIELD>::value );

    if( !aFields )
        return ""; // Should not happen, T=void specialization will be called instead.

    auto it = std::find_if( aFields->begin(), aFields->end(),
                            [aFieldName]( const T& field )
                            {
                                return field.GetName() == aFieldName;
                            } );

    if( it != aFields->end() )
        return std::string( it->GetText().ToUTF8() );

    return "";
}


// This specialization is used when no fields are passed.
template <>
std::string SIM_MODEL::GetFieldValue( const std::vector<void>* aFields, const std::string& aFieldName )
{
    return "";
}


template <typename T>
void SIM_MODEL::SetFieldValue( std::vector<T>& aFields, const std::string& aFieldName,
                               const std::string& aValue )
{
    static_assert( std::is_same<T, SCH_FIELD>::value || std::is_same<T, LIB_FIELD>::value );

    auto fieldIt = std::find_if( aFields.begin(), aFields.end(),
                                 [&]( const T& f )
                                 {
                                    return f.GetName() == aFieldName;
                                 } );

    if( fieldIt != aFields.end() )
    {
        if( aValue == "" )
            aFields.erase( fieldIt );
        else
            fieldIt->SetText( aValue );

        return;
    }

    if( aValue == "" )
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


SIM_MODEL::~SIM_MODEL() = default;


void SIM_MODEL::AddPin( const PIN& aPin )
{
    m_pins.push_back( aPin );
}


int SIM_MODEL::FindModelPinIndex( const std::string& aSymbolPinNumber )
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


const SIM_MODEL::PARAM* SIM_MODEL::FindParam( const std::string& aParamName ) const
{
    std::vector<std::reference_wrapper<const PARAM>> params = GetParams();

    auto it = std::find_if( params.begin(), params.end(),
                            [aParamName]( const PARAM& param )
                            {
                                return param.info.name == boost::to_lower_copy( aParamName );
                            } );

    if( it == params.end() )
        return nullptr;

    return &it->get();
}


std::vector<std::reference_wrapper<const SIM_MODEL::PARAM>> SIM_MODEL::GetParams() const
{
    std::vector<std::reference_wrapper<const PARAM>> params;

    for( int i = 0; i < GetParamCount(); ++i )
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


void SIM_MODEL::SetParamValue( int aParamIndex, const SIM_VALUE& aValue )
{
    *m_params.at( aParamIndex ).value = aValue;
}


void SIM_MODEL::SetParamValue( int aParamIndex, const std::string& aValue,
                               SIM_VALUE::NOTATION aNotation )
{
    const SIM_VALUE& value = *GetParam( aParamIndex ).value;
    SetParamValue( aParamIndex, *SIM_VALUE::Create( value.GetType(), aValue, aNotation ) );
}


void SIM_MODEL::SetParamValue( const std::string& aParamName, const SIM_VALUE& aValue )
{
    std::vector<std::reference_wrapper<const PARAM>> params = GetParams();

    auto it = std::find_if( params.begin(), params.end(),
                            [aParamName]( const PARAM& param )
                            {
                                return param.info.name == boost::to_lower_copy( aParamName );
                            } );

    if( it == params.end() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Could not find a simulation model parameter named '%s'" ),
                                          aParamName ) );
    }

    SetParamValue( static_cast<int>( it - params.begin() ), aValue );
}


void SIM_MODEL::SetParamValue( const std::string& aParamName, const std::string& aValue,
                               SIM_VALUE::NOTATION aNotation )
{
    const PARAM* param = FindParam( aParamName );

    if( !param )
    {
        THROW_IO_ERROR( wxString::Format( _( "Could not find a simulation model parameter named '%s'" ),
                                          aParamName ) );
    }

    const SIM_VALUE& value = *FindParam( aParamName )->value;
    SetParamValue( aParamName, *SIM_VALUE::Create( value.GetType(), aValue, aNotation ) );
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


std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( TYPE aType )
{
    switch( aType )
    {
    case TYPE::R:
    case TYPE::C:
    case TYPE::L:
        return std::make_unique<SIM_MODEL_IDEAL>( aType );

    case TYPE::L_MUTUAL:
        return std::make_unique<SIM_MODEL_MUTUAL_INDUCTOR>();

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
        return std::make_unique<SIM_MODEL_SUBCKT>();

    case TYPE::XSPICE:
        return std::make_unique<SIM_MODEL_XSPICE>( aType );

    case TYPE::KIBIS_DEVICE:
    case TYPE::KIBIS_DRIVER_DC:
    case TYPE::KIBIS_DRIVER_RECT:
    case TYPE::KIBIS_DRIVER_PRBS:
    case TYPE::KIBIS_DIFFDEVICE:
    case TYPE::KIBIS_DIFFDRIVER:
        return std::make_unique<SIM_MODEL_KIBIS>( aType );

    case TYPE::RAWSPICE:
        return std::make_unique<SIM_MODEL_RAW_SPICE>();

    default:
        return std::make_unique<SIM_MODEL_NGSPICE>( aType );
    }
}


SIM_MODEL::SIM_MODEL( TYPE aType ) :
    SIM_MODEL( aType, std::make_unique<SPICE_GENERATOR>( *this ) )
{
}


SIM_MODEL::SIM_MODEL( TYPE aType, std::unique_ptr<SPICE_GENERATOR> aSpiceGenerator ) : 
    m_baseModel( nullptr ),
    m_spiceGenerator( std::move( aSpiceGenerator ) ),
    m_type( aType ),
    m_isEnabled( true ),
    m_isInferred( false )
{
}


void SIM_MODEL::CreatePins( unsigned aSymbolPinCount )
{
    // Default pin sequence: model pins are the same as symbol pins.
    // Excess model pins are set as Not Connected.
    // Note that intentionally nothing is added if `getPinNames()` returns an empty vector.

    // SIM_MODEL pins must be ordered by symbol pin numbers -- this is assumed by the code that
    // accesses them.

    for( unsigned modelPinIndex = 0; modelPinIndex < getPinNames().size(); ++modelPinIndex )
    {
        if( modelPinIndex < aSymbolPinCount )
            AddPin( { getPinNames().at( modelPinIndex ), fmt::format( "{}", modelPinIndex + 1 ) } );
        else
            AddPin( { getPinNames().at( modelPinIndex ), "" } );
    }
}


template void SIM_MODEL::WriteInferredDataFields( std::vector<SCH_FIELD>& aFields,
                                                  const std::string& aValue ) const;
template void SIM_MODEL::WriteInferredDataFields( std::vector<LIB_FIELD>& aFields,
                                                  const std::string& aValue ) const;

template <typename T>
void SIM_MODEL::WriteInferredDataFields( std::vector<T>& aFields, const std::string& aValue ) const
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


std::string SIM_MODEL::GenerateParamValuePair( const PARAM& aParam, bool& aIsFirst ) const
{
    std::string result;

    if( aIsFirst )
        aIsFirst = false;
    else
        result.append( " " );

    std::string name = aParam.info.name;

    // Because of collisions with instance parameters, we append some model parameters with "_".
    if( boost::ends_with( aParam.info.name, "_" ) )
        name = aParam.info.name.substr( 0, aParam.info.name.length() - 1);

    std::string value = aParam.value->ToString();
    if( value.find( " " ) != std::string::npos )
        value = "\"" + value + "\"";

    result.append( fmt::format( "{}={}", aParam.info.name, value ) );
    return result;
}


std::string SIM_MODEL::GenerateParamsField( const std::string& aPairSeparator ) const
{
    std::string result;
    bool isFirst = true;

    for( const PARAM& param : m_params )
    {
        if( param.value->ToString() == "" )
            continue;

        result.append( GenerateParamValuePair( param, isFirst ) );
    }

    return result;
}


void SIM_MODEL::ParseParamsField( const std::string& aParamsField )
{
    tao::pegtl::string_input<> in( aParamsField, "Sim_Params" );
    std::unique_ptr<tao::pegtl::parse_tree::node> root;

    try
    {
        // Using parse tree instead of actions because we don't care about performance that much,
        // and having a tree greatly simplifies some things.
        root = tao::pegtl::parse_tree::parse<
            SIM_MODEL_PARSER::fieldParamValuePairsGrammar,
            SIM_MODEL_PARSER::fieldParamValuePairsSelector,
            tao::pegtl::nothing,
            SIM_MODEL_PARSER::control>
                ( in );
    }
    catch( const tao::pegtl::parse_error& e )
    {
        THROW_IO_ERROR( e.what() );
    }

    std::string paramName;

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
            std::string str = node->string();

            // Unescape quotes.
            boost::replace_all( str, "\\\"", "\"" );

            SetParamValue( paramName, str, SIM_VALUE_GRAMMAR::NOTATION::SI );
        }
        else
        {
            wxFAIL;
        }
    }
}


void SIM_MODEL::ParsePinsField( unsigned aSymbolPinCount, const std::string& aPinsField )
{
    CreatePins( aSymbolPinCount );

    if( aPinsField == "" )
        return;

    tao::pegtl::string_input<> in( aPinsField, PINS_FIELD );
    std::unique_ptr<tao::pegtl::parse_tree::node> root;

    try
    {
        root = tao::pegtl::parse_tree::parse<SIM_MODEL_PARSER::pinSequenceGrammar,
                                             SIM_MODEL_PARSER::pinSequenceSelector,
                                             tao::pegtl::nothing,
                                             SIM_MODEL_PARSER::control>( in );
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


void SIM_MODEL::ParseDisabledField( const std::string& aDisabledField )
{
    if( aDisabledField == "" )
        return;

    char c = boost::to_lower_copy( aDisabledField )[0];

    if( c == 'y' || c == 't' || c == '1' )
        m_isEnabled = false;
}


template <typename T>
void SIM_MODEL::doReadDataFields( unsigned aSymbolPinCount, const std::vector<T>* aFields )
{
    ParseDisabledField( GetFieldValue( aFields, DISABLED_FIELD ) );
    ParsePinsField( aSymbolPinCount, GetFieldValue( aFields, PINS_FIELD ) );

    if( GetFieldValue( aFields, PARAMS_FIELD ) != "" )
        ParseParamsField( GetFieldValue( aFields, PARAMS_FIELD ) );
    else
        InferredReadDataFields( aSymbolPinCount, aFields, true );
}


template <typename T>
void SIM_MODEL::InferredReadDataFields( unsigned aSymbolPinCount, const std::vector<T>* aFields,
                                        bool aAllowOnlyFirstValue )
{
    // TODO: Make a subclass SIM_MODEL_NONE.
    if( GetType() == SIM_MODEL::TYPE::NONE )
        return;

    // TODO: Don't call this multiple times.
    if( InferTypeFromRefAndValue( GetFieldValue( aFields, REFERENCE_FIELD ),
                                  GetFieldValue( aFields, VALUE_FIELD ) ) != GetType() )
    {
        // Not an inferred model. Nothing to do here.
        return;
    }

    try
    {
        // TODO: Don't call this multiple times.
        tao::pegtl::string_input<> in( GetFieldValue( aFields, VALUE_FIELD ), VALUE_FIELD );
        auto root = tao::pegtl::parse_tree::parse<SIM_MODEL_PARSER::fieldInferValueGrammar,
                                                  SIM_MODEL_PARSER::fieldInferValueSelector,
                                                  tao::pegtl::nothing,
                                                  SIM_MODEL_PARSER::control>( in );

        for( const auto& node : root->children )
        {
            if( node->is_type<SIM_MODEL_PARSER::fieldParamValuePairs>() )
                ParseParamsField( node->string() );
            else if( node->is_type<SIM_MODEL_PARSER::fieldFloatValue>() )
            {
                if( aAllowOnlyFirstValue )
                {
                    for( const auto& subnode : node->children ) 
                    {
                        if( subnode->is_type<SIM_MODEL_PARSER::number<SIM_VALUE::TYPE_FLOAT,
                                                                      SIM_VALUE::NOTATION::SI>>() )
                        {
                            SetParamValue( 0, subnode->string() );
                        }
                    }
                }
            }
        }
    }
    catch( const tao::pegtl::parse_error& e )
    {
        THROW_IO_ERROR( e.what() );
    }

    m_isInferred = true;
}

template void SIM_MODEL::InferredReadDataFields( unsigned aSymbolPinCount,
                                                 const std::vector<SCH_FIELD>* aFields,
                                                 bool aAllowOnlyFirstValue );
template void SIM_MODEL::InferredReadDataFields( unsigned aSymbolPinCount,
                                                 const std::vector<LIB_FIELD>* aFields,
                                                 bool aAllowOnlyFirstValue );


template <typename T>
void SIM_MODEL::doWriteFields( std::vector<T>& aFields ) const
{
    SetFieldValue( aFields, DEVICE_TYPE_FIELD, generateDeviceTypeField() );
    SetFieldValue( aFields, TYPE_FIELD, generateTypeField() );
    SetFieldValue( aFields, PINS_FIELD, generatePinsField() );
    SetFieldValue( aFields, PARAMS_FIELD, GenerateParamsField( " " ) );
    SetFieldValue( aFields, DISABLED_FIELD, generateDisabledField() );
}


std::string SIM_MODEL::generateDeviceTypeField() const
{
    return DeviceTypeInfo( TypeInfo( m_type ).deviceType ).fieldValue;
}


std::string SIM_MODEL::generateTypeField() const
{
    return TypeInfo( m_type ).fieldValue;
}


std::string SIM_MODEL::generatePinsField() const
{
    std::string result = "";
    bool isFirst = true;

    for( const PIN& pin : GetPins() )
    {
        if( isFirst )
            isFirst = false;
        else
            result.append( " " );

        if( pin.symbolPinNumber == "" )
            result.append( "~" );
        else
            result.append( pin.symbolPinNumber ); // Note that it's numbered from 1.
    }

    return result;
}


std::string SIM_MODEL::generateDisabledField() const
{
    return m_isEnabled ? "" : "1";
}


bool SIM_MODEL::requiresSpiceModelLine() const
{
    for( const PARAM& param : GetParams() )
    {
        if( !param.info.isSpiceInstanceParam )
            return true;
    }

    return false;
}

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022 CERN
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
#include <lib_symbol.h>
#include <sch_symbol.h>
#include <confirm.h>
#include <string_utils.h>
#include <wx/regex.h>

#include <sim/sim_model.h>
#include <sim/sim_model_behavioral.h>
#include <sim/sim_model_ideal.h>
#include <sim/sim_model_l_mutual.h>
#include <sim/sim_model_ngspice.h>
#include <sim/sim_model_r_pot.h>
#include <sim/sim_model_kibis.h>
#include <sim/sim_model_source.h>
#include <sim/sim_model_raw_spice.h>
#include <sim/sim_model_subckt.h>
#include <sim/sim_model_switch.h>
#include <sim/sim_model_tline.h>
#include <sim/sim_model_xspice.h>
#include <sim/sim_lib_mgr.h>
#include <sim/sim_library_kibis.h>

#include <boost/algorithm/string/case_conv.hpp>
#include <fmt/core.h>
#include <pegtl/contrib/parse_tree.hpp>

#include <iterator>
#include "sim_model_spice_fallback.h"

using TYPE = SIM_MODEL::TYPE;


SIM_MODEL::DEVICE_INFO SIM_MODEL::DeviceInfo( DEVICE_T aDeviceType )
{
    switch( aDeviceType )
    {
        case DEVICE_T::NONE:      return { "",       "",                  true };
        case DEVICE_T::R:         return { "R",      "Resistor",          true };
        case DEVICE_T::C:         return { "C",      "Capacitor",         true };
        case DEVICE_T::L:         return { "L",      "Inductor",          true };
        case DEVICE_T::TLINE:     return { "TLINE",  "Transmission Line", true };
        case DEVICE_T::SW:        return { "SW",     "Switch",            true };

        case DEVICE_T::D:         return { "D",      "Diode",             true };
        case DEVICE_T::NPN:       return { "NPN",    "NPN BJT",           true };
        case DEVICE_T::PNP:       return { "PNP",    "PNP BJT",           true };

        case DEVICE_T::NJFET:     return { "NJFET",  "N-channel JFET",    true };
        case DEVICE_T::PJFET:     return { "PJFET",  "P-channel JFET",    true };

        case DEVICE_T::NMOS:      return { "NMOS",   "N-channel MOSFET",  true };
        case DEVICE_T::PMOS:      return { "PMOS",   "P-channel MOSFET",  true };
        case DEVICE_T::NMES:      return { "NMES",   "N-channel MESFET",  true };
        case DEVICE_T::PMES:      return { "PMES",   "P-channel MESFET",  true };

        case DEVICE_T::V:         return { "V",      "Voltage Source",    true };
        case DEVICE_T::I:         return { "I",      "Current Source",    true };

        case DEVICE_T::KIBIS:     return { "IBIS",  "IBIS Model",         false };

        case DEVICE_T::SUBCKT:    return { "SUBCKT", "Subcircuit",        false };
        case DEVICE_T::XSPICE:    return { "XSPICE", "XSPICE Code Model", true };
        case DEVICE_T::SPICE:     return { "SPICE",  "Raw Spice Element", true };

        default:    wxFAIL;       return {};
    }
}


SIM_MODEL::INFO SIM_MODEL::TypeInfo( TYPE aType )
{
    switch( aType )
    {
    case TYPE::NONE:                 return { DEVICE_T::NONE,   "",               ""                           };

    case TYPE::R:                    return { DEVICE_T::R,      "",               "Ideal"                      };
    case TYPE::R_POT:                return { DEVICE_T::R,      "POT",            "Potentiometer"              };
    case TYPE::R_BEHAVIORAL:         return { DEVICE_T::R,      "=",              "Behavioral"                 };

    case TYPE::C:                    return { DEVICE_T::C,      "",               "Ideal"                      };
    case TYPE::C_BEHAVIORAL:         return { DEVICE_T::C,      "=",              "Behavioral"                 };

    case TYPE::L:                    return { DEVICE_T::L,      "",               "Ideal"                      };
    case TYPE::L_MUTUAL:             return { DEVICE_T::L,      "MUTUAL",         "Mutual"                     };
    case TYPE::L_BEHAVIORAL:         return { DEVICE_T::L,      "=",              "Behavioral"                 };

    case TYPE::TLINE_Z0:             return { DEVICE_T::TLINE,  "",               "Characteristic impedance"   };
    case TYPE::TLINE_RLGC:           return { DEVICE_T::TLINE,  "RLGC",           "RLGC"                       };

    case TYPE::SW_V:                 return { DEVICE_T::SW,     "V",              "Voltage-controlled"         };
    case TYPE::SW_I:                 return { DEVICE_T::SW,     "I",              "Current-controlled"         };

    case TYPE::D:                    return { DEVICE_T::D,      "",               ""                           };

    case TYPE::NPN_VBIC:             return { DEVICE_T::NPN,    "VBIC",           "VBIC"                       };
    case TYPE::PNP_VBIC:             return { DEVICE_T::PNP,    "VBIC",           "VBIC"                       };
    case TYPE::NPN_GUMMELPOON:       return { DEVICE_T::NPN,    "GUMMELPOON",     "Gummel-Poon"                };
    case TYPE::PNP_GUMMELPOON:       return { DEVICE_T::PNP,    "GUMMELPOON",     "Gummel-Poon"                };
    //case TYPE::BJT_MEXTRAM:          return {};
    case TYPE::NPN_HICUM2:           return { DEVICE_T::NPN,    "HICUML2",        "HICUM level 2"              };
    case TYPE::PNP_HICUM2:           return { DEVICE_T::PNP,    "HICUML2",        "HICUM level 2"              };
    //case TYPE::BJT_HICUM_L0:         return {};

    case TYPE::NJFET_SHICHMANHODGES: return { DEVICE_T::NJFET,  "SHICHMANHODGES", "Shichman-Hodges"            };
    case TYPE::PJFET_SHICHMANHODGES: return { DEVICE_T::PJFET,  "SHICHMANHODGES", "Shichman-Hodges"            };
    case TYPE::NJFET_PARKERSKELLERN: return { DEVICE_T::NJFET,  "PARKERSKELLERN", "Parker-Skellern"            };
    case TYPE::PJFET_PARKERSKELLERN: return { DEVICE_T::PJFET,  "PARKERSKELLERN", "Parker-Skellern"            };

    case TYPE::NMES_STATZ:           return { DEVICE_T::NMES,   "STATZ",          "Statz"                      };
    case TYPE::PMES_STATZ:           return { DEVICE_T::PMES,   "STATZ",          "Statz"                      };
    case TYPE::NMES_YTTERDAL:        return { DEVICE_T::NMES,   "YTTERDAL",       "Ytterdal"                   };
    case TYPE::PMES_YTTERDAL:        return { DEVICE_T::PMES,   "YTTERDAL",       "Ytterdal"                   };
    case TYPE::NMES_HFET1:           return { DEVICE_T::NMES,   "HFET1",          "HFET1"                      };
    case TYPE::PMES_HFET1:           return { DEVICE_T::PMES,   "HFET1",          "HFET1"                      };
    case TYPE::NMES_HFET2:           return { DEVICE_T::NMES,   "HFET2",          "HFET2"                      };
    case TYPE::PMES_HFET2:           return { DEVICE_T::PMES,   "HFET2",          "HFET2"                      };

    case TYPE::NMOS_VDMOS:           return { DEVICE_T::NMOS,   "VDMOS",          "VDMOS"                      };
    case TYPE::PMOS_VDMOS:           return { DEVICE_T::PMOS,   "VDMOS",          "VDMOS"                      };
    case TYPE::NMOS_MOS1:            return { DEVICE_T::NMOS,   "MOS1",           "Classical quadratic (MOS1)" };
    case TYPE::PMOS_MOS1:            return { DEVICE_T::PMOS,   "MOS1",           "Classical quadratic (MOS1)" };
    case TYPE::NMOS_MOS2:            return { DEVICE_T::NMOS,   "MOS2",           "Grove-Frohman (MOS2)"       };
    case TYPE::PMOS_MOS2:            return { DEVICE_T::PMOS,   "MOS2",           "Grove-Frohman (MOS2)"       };
    case TYPE::NMOS_MOS3:            return { DEVICE_T::NMOS,   "MOS3",           "MOS3"                       };
    case TYPE::PMOS_MOS3:            return { DEVICE_T::PMOS,   "MOS3",           "MOS3"                       };
    case TYPE::NMOS_BSIM1:           return { DEVICE_T::NMOS,   "BSIM1",          "BSIM1"                      };
    case TYPE::PMOS_BSIM1:           return { DEVICE_T::PMOS,   "BSIM1",          "BSIM1"                      };
    case TYPE::NMOS_BSIM2:           return { DEVICE_T::NMOS,   "BSIM2",          "BSIM2"                      };
    case TYPE::PMOS_BSIM2:           return { DEVICE_T::PMOS,   "BSIM2",          "BSIM2"                      };
    case TYPE::NMOS_MOS6:            return { DEVICE_T::NMOS,   "MOS6",           "MOS6"                       };
    case TYPE::PMOS_MOS6:            return { DEVICE_T::PMOS,   "MOS6",           "MOS6"                       };
    case TYPE::NMOS_BSIM3:           return { DEVICE_T::NMOS,   "BSIM3",          "BSIM3"                      };
    case TYPE::PMOS_BSIM3:           return { DEVICE_T::PMOS,   "BSIM3",          "BSIM3"                      };
    case TYPE::NMOS_MOS9:            return { DEVICE_T::NMOS,   "MOS9",           "MOS9"                       };
    case TYPE::PMOS_MOS9:            return { DEVICE_T::PMOS,   "MOS9",           "MOS9"                       };
    case TYPE::NMOS_B4SOI:           return { DEVICE_T::NMOS,   "B4SOI",          "BSIM4 SOI (B4SOI)"          };
    case TYPE::PMOS_B4SOI:           return { DEVICE_T::PMOS,   "B4SOI",          "BSIM4 SOI (B4SOI)"          };
    case TYPE::NMOS_BSIM4:           return { DEVICE_T::NMOS,   "BSIM4",          "BSIM4"                      };
    case TYPE::PMOS_BSIM4:           return { DEVICE_T::PMOS,   "BSIM4",          "BSIM4"                      };
    //case TYPE::NMOS_EKV2_6:          return {};
    //case TYPE::PMOS_EKV2_6:          return {};
    //case TYPE::NMOS_PSP:             return {};
    //case TYPE::PMOS_PSP:             return {};
    case TYPE::NMOS_B3SOIFD:         return { DEVICE_T::NMOS,   "B3SOIFD",        "B3SOIFD (BSIM3 FD-SOI)"     };
    case TYPE::PMOS_B3SOIFD:         return { DEVICE_T::PMOS,   "B3SOIFD",        "B3SOIFD (BSIM3 FD-SOI)"     };
    case TYPE::NMOS_B3SOIDD:         return { DEVICE_T::NMOS,   "B3SOIDD",        "B3SOIDD (BSIM3 SOI)"        };
    case TYPE::PMOS_B3SOIDD:         return { DEVICE_T::PMOS,   "B3SOIDD",        "B3SOIDD (BSIM3 SOI)"        };
    case TYPE::NMOS_B3SOIPD:         return { DEVICE_T::NMOS,   "B3SOIPD",        "B3SOIPD (BSIM3 PD-SOI)"     };
    case TYPE::PMOS_B3SOIPD:         return { DEVICE_T::PMOS,   "B3SOIPD",        "B3SOIPD (BSIM3 PD-SOI)"     };
    //case TYPE::NMOS_STAG:            return {};
    //case TYPE::PMOS_STAG:            return {};
    case TYPE::NMOS_HISIM2:          return { DEVICE_T::NMOS,   "HISIM2",         "HiSIM2"                     };
    case TYPE::PMOS_HISIM2:          return { DEVICE_T::PMOS,   "HISIM2",         "HiSIM2"                     };
    case TYPE::NMOS_HISIMHV1:        return { DEVICE_T::NMOS,   "HISIMHV1",       "HiSIM_HV1"                  };
    case TYPE::PMOS_HISIMHV1:        return { DEVICE_T::PMOS,   "HISIMHV1",       "HiSIM_HV1"                  };
    case TYPE::NMOS_HISIMHV2:        return { DEVICE_T::NMOS,   "HISIMHV2",       "HiSIM_HV2"                  };
    case TYPE::PMOS_HISIMHV2:        return { DEVICE_T::PMOS,   "HISIMHV2",       "HiSIM_HV2"                  };

    case TYPE::V:                    return { DEVICE_T::V,      "DC",             "DC",                        };
    case TYPE::V_SIN:                return { DEVICE_T::V,      "SIN",            "Sine"                       };
    case TYPE::V_PULSE:              return { DEVICE_T::V,      "PULSE",          "Pulse"                      };
    case TYPE::V_EXP:                return { DEVICE_T::V,      "EXP",            "Exponential"                };
    /*case TYPE::V_SFAM:               return { DEVICE_TYPE::V,      "SFAM",           "Single-frequency AM"        };
    case TYPE::V_SFFM:               return { DEVICE_TYPE::V,      "SFFM",           "Single-frequency FM"        };*/
    case TYPE::V_PWL:                return { DEVICE_T::V,      "PWL",            "Piecewise linear"           };
    case TYPE::V_WHITENOISE:         return { DEVICE_T::V,      "WHITENOISE",     "White noise"                };
    case TYPE::V_PINKNOISE:          return { DEVICE_T::V,      "PINKNOISE",      "Pink noise (1/f)"           };
    case TYPE::V_BURSTNOISE:         return { DEVICE_T::V,      "BURSTNOISE",     "Burst noise"                };
    case TYPE::V_RANDUNIFORM:        return { DEVICE_T::V,      "RANDUNIFORM",    "Random uniform"             };
    case TYPE::V_RANDNORMAL:         return { DEVICE_T::V,      "RANDNORMAL",     "Random normal"              };
    case TYPE::V_RANDEXP:            return { DEVICE_T::V,      "RANDEXP",        "Random exponential"         };
    //case TYPE::V_RANDPOISSON:        return { DEVICE_TYPE::V,      "RANDPOISSON",    "Random Poisson"             };
    case TYPE::V_BEHAVIORAL:         return { DEVICE_T::V,      "=",              "Behavioral"                 };

    case TYPE::I:                    return { DEVICE_T::I,      "DC",             "DC",                        };
    case TYPE::I_SIN:                return { DEVICE_T::I,      "SIN",            "Sine"                       };
    case TYPE::I_PULSE:              return { DEVICE_T::I,      "PULSE",          "Pulse"                      };
    case TYPE::I_EXP:                return { DEVICE_T::I,      "EXP",            "Exponential"                };
    /*case TYPE::I_SFAM:               return { DEVICE_TYPE::I,      "SFAM",           "Single-frequency AM"        };
    case TYPE::I_SFFM:               return { DEVICE_TYPE::I,      "SFFM",           "Single-frequency FM"        };*/
    case TYPE::I_PWL:                return { DEVICE_T::I,      "PWL",            "Piecewise linear"           };
    case TYPE::I_WHITENOISE:         return { DEVICE_T::I,      "WHITENOISE",     "White noise"                };
    case TYPE::I_PINKNOISE:          return { DEVICE_T::I,      "PINKNOISE",      "Pink noise (1/f)"           };
    case TYPE::I_BURSTNOISE:         return { DEVICE_T::I,      "BURSTNOISE",     "Burst noise"                };
    case TYPE::I_RANDUNIFORM:        return { DEVICE_T::I,      "RANDUNIFORM",    "Random uniform"             };
    case TYPE::I_RANDNORMAL:         return { DEVICE_T::I,      "RANDNORMAL",     "Random normal"              };
    case TYPE::I_RANDEXP:            return { DEVICE_T::I,      "RANDEXP",        "Random exponential"         };
    //case TYPE::I_RANDPOISSON:        return { DEVICE_TYPE::I,      "RANDPOISSON",    "Random Poisson"             };
    case TYPE::I_BEHAVIORAL:         return { DEVICE_T::I,      "=",              "Behavioral"                 };

    case TYPE::SUBCKT:               return { DEVICE_T::SUBCKT, "",               ""                           };
    case TYPE::XSPICE:               return { DEVICE_T::XSPICE, "",               ""                           };

    case TYPE::KIBIS_DEVICE:         return { DEVICE_T::KIBIS,  "DEVICE",         "Device"                     };
    case TYPE::KIBIS_DRIVER_DC:      return { DEVICE_T::KIBIS,  "DCDRIVER",       "DC driver"                  };
    case TYPE::KIBIS_DRIVER_RECT:    return { DEVICE_T::KIBIS,  "RECTDRIVER",     "Rectangular wave driver"    };
    case TYPE::KIBIS_DRIVER_PRBS:    return { DEVICE_T::KIBIS,  "PRBSDRIVER",     "PRBS driver"                };

    case TYPE::RAWSPICE:             return { DEVICE_T::SPICE,  "",               ""                           };

    default:       wxFAIL;           return {};
    }
}


SIM_MODEL::SPICE_INFO SIM_MODEL::SpiceInfo( TYPE aType )
{
    switch( aType )
    {
    case TYPE::R:                    return { "R", ""             };
    case TYPE::R_POT:                return { "A", ""             };
    case TYPE::R_BEHAVIORAL:         return { "R", "",            "",        "0",  false, true   };

    case TYPE::C:                    return { "C", ""             };
    case TYPE::C_BEHAVIORAL:         return { "C", "",            "",        "0",  false, true   };

    case TYPE::L:                    return { "L", ""             };
    case TYPE::L_MUTUAL:             return { "K", ""             };
    case TYPE::L_BEHAVIORAL:         return { "L", "",            "",        "0",  false, true   };

    //case TYPE::TLINE_Z0:             return { "T"  };
    case TYPE::TLINE_Z0:             return { "O", "LTRA"         };
    case TYPE::TLINE_RLGC:           return { "O", "LTRA"         };

    case TYPE::SW_V:                 return { "S", "SW"           };
    case TYPE::SW_I:                 return { "W", "CSW"          };

    case TYPE::D:                    return { "D", "D"            };

    case TYPE::NPN_VBIC:             return { "Q", "NPN",         "",        "4"   };
    case TYPE::PNP_VBIC:             return { "Q", "PNP",         "",        "4"   };
    case TYPE::NPN_GUMMELPOON:       return { "Q", "NPN",         "",        "1",  true   };
    case TYPE::PNP_GUMMELPOON:       return { "Q", "PNP",         "",        "1",  true   };
    case TYPE::NPN_HICUM2:           return { "Q", "NPN",         "",        "8"   };
    case TYPE::PNP_HICUM2:           return { "Q", "PNP",         "",        "8"   };

    case TYPE::NJFET_SHICHMANHODGES: return { "M", "NJF",         "",        "1"   };
    case TYPE::PJFET_SHICHMANHODGES: return { "M", "PJF",         "",        "1"   };
    case TYPE::NJFET_PARKERSKELLERN: return { "M", "NJF",         "",        "2"   };
    case TYPE::PJFET_PARKERSKELLERN: return { "M", "PJF",         "",        "2"   };

    case TYPE::NMES_STATZ:           return { "Z", "NMF",         "",        "1"   };
    case TYPE::PMES_STATZ:           return { "Z", "PMF",         "",        "1"   };
    case TYPE::NMES_YTTERDAL:        return { "Z", "NMF",         "",        "2"   };
    case TYPE::PMES_YTTERDAL:        return { "Z", "PMF",         "",        "2"   };
    case TYPE::NMES_HFET1:           return { "Z", "NMF",         "",        "5"   };
    case TYPE::PMES_HFET1:           return { "Z", "PMF",         "",        "5"   };
    case TYPE::NMES_HFET2:           return { "Z", "NMF",         "",        "6"   };
    case TYPE::PMES_HFET2:           return { "Z", "PMF",         "",        "6"   };

    case TYPE::NMOS_VDMOS:           return { "M", "VDMOS NCHAN", };
    case TYPE::PMOS_VDMOS:           return { "M", "VDMOS PCHAN", };
    case TYPE::NMOS_MOS1:            return { "M", "NMOS",        "",        "1"   };
    case TYPE::PMOS_MOS1:            return { "M", "PMOS",        "",        "1"   };
    case TYPE::NMOS_MOS2:            return { "M", "NMOS",        "",        "2"   };
    case TYPE::PMOS_MOS2:            return { "M", "PMOS",        "",        "2"   };
    case TYPE::NMOS_MOS3:            return { "M", "NMOS",        "",        "3"   };
    case TYPE::PMOS_MOS3:            return { "M", "PMOS",        "",        "3"   };
    case TYPE::NMOS_BSIM1:           return { "M", "NMOS",        "",        "4"   };
    case TYPE::PMOS_BSIM1:           return { "M", "PMOS",        "",        "4"   };
    case TYPE::NMOS_BSIM2:           return { "M", "NMOS",        "",        "5"   };
    case TYPE::PMOS_BSIM2:           return { "M", "PMOS",        "",        "5"   };
    case TYPE::NMOS_MOS6:            return { "M", "NMOS",        "",        "6"   };
    case TYPE::PMOS_MOS6:            return { "M", "PMOS",        "",        "6"   };
    case TYPE::NMOS_BSIM3:           return { "M", "NMOS",        "",        "8"   };
    case TYPE::PMOS_BSIM3:           return { "M", "PMOS",        "",        "8"   };
    case TYPE::NMOS_MOS9:            return { "M", "NMOS",        "",        "9"   };
    case TYPE::PMOS_MOS9:            return { "M", "PMOS",        "",        "9"   };
    case TYPE::NMOS_B4SOI:           return { "M", "NMOS",        "",        "10"  };
    case TYPE::PMOS_B4SOI:           return { "M", "PMOS",        "",        "10"  };
    case TYPE::NMOS_BSIM4:           return { "M", "NMOS",        "",        "14"  };
    case TYPE::PMOS_BSIM4:           return { "M", "PMOS",        "",        "14"  };
    //case TYPE::NMOS_EKV2_6:          return {};
    //case TYPE::PMOS_EKV2_6:          return {};
    //case TYPE::NMOS_PSP:             return {};
    //case TYPE::PMOS_PSP:             return {};
    case TYPE::NMOS_B3SOIFD:         return { "M", "NMOS",        "",        "55"  };
    case TYPE::PMOS_B3SOIFD:         return { "M", "PMOS",        "",        "55"  };
    case TYPE::NMOS_B3SOIDD:         return { "M", "NMOS",        "",        "56"  };
    case TYPE::PMOS_B3SOIDD:         return { "M", "PMOS",        "",        "56"  };
    case TYPE::NMOS_B3SOIPD:         return { "M", "NMOS",        "",        "57"  };
    case TYPE::PMOS_B3SOIPD:         return { "M", "PMOS",        "",        "57"  };
    //case TYPE::NMOS_STAG:            return {};
    //case TYPE::PMOS_STAG:            return {};
    case TYPE::NMOS_HISIM2:          return { "M", "NMOS",        "",        "68"  };
    case TYPE::PMOS_HISIM2:          return { "M", "PMOS",        "",        "68"  };
    case TYPE::NMOS_HISIMHV1:        return { "M", "NMOS",        "",        "73", true,  false, "1.2.4" };
    case TYPE::PMOS_HISIMHV1:        return { "M", "PMOS",        "",        "73", true,  false, "1.2.4" };
    case TYPE::NMOS_HISIMHV2:        return { "M", "NMOS",        "",        "73", true,  false, "2.2.0" };
    case TYPE::PMOS_HISIMHV2:        return { "M", "PMOS",        "",        "73", true,  false, "2.2.0" };

    case TYPE::V:                    return { "V", "",            "DC"       };
    case TYPE::V_SIN:                return { "V", "",            "SIN"      };
    case TYPE::V_PULSE:              return { "V", "",            "PULSE"    };
    case TYPE::V_EXP:                return { "V", "",            "EXP"      };
    //case TYPE::V_SFAM:               return { "V", "",            "AM"       };
    //case TYPE::V_SFFM:               return { "V", "",            "SFFM"     };
    case TYPE::V_PWL:                return { "V", "",            "PWL"      };
    case TYPE::V_WHITENOISE:         return { "V", "",            "TRNOISE"  };
    case TYPE::V_PINKNOISE:          return { "V", "",            "TRNOISE"  };
    case TYPE::V_BURSTNOISE:         return { "V", "",            "TRNOISE"  };
    case TYPE::V_RANDUNIFORM:        return { "V", "",            "TRRANDOM" };
    case TYPE::V_RANDNORMAL:         return { "V", "",            "TRRANDOM" };
    case TYPE::V_RANDEXP:            return { "V", "",            "TRRANDOM" };
    //case TYPE::V_RANDPOISSON:        return { "V", "",            "TRRANDOM" };
    case TYPE::V_BEHAVIORAL:         return { "B"  };

    case TYPE::I:                    return { "I", "",            "DC"       };
    case TYPE::I_PULSE:              return { "I", "",            "PULSE"    };
    case TYPE::I_SIN:                return { "I", "",            "SIN"      };
    case TYPE::I_EXP:                return { "I", "",            "EXP"      };
    //case TYPE::I_SFAM:               return { "V", "",            "AM"       };
    //case TYPE::I_SFFM:               return { "V", "",            "SFFM"     };
    case TYPE::I_PWL:                return { "I", "",            "PWL"      };
    case TYPE::I_WHITENOISE:         return { "I", "",            "TRNOISE"  };
    case TYPE::I_PINKNOISE:          return { "I", "",            "TRNOISE"  };
    case TYPE::I_BURSTNOISE:         return { "I", "",            "TRNOISE"  };
    case TYPE::I_RANDUNIFORM:        return { "I", "",            "TRRANDOM" };
    case TYPE::I_RANDNORMAL:         return { "I", "",            "TRRANDOM" };
    case TYPE::I_RANDEXP:            return { "I", "",            "TRRANDOM" };
    //case TYPE::I_RANDPOISSON:        return { "I", "",            "TRRANDOM" };
    case TYPE::I_BEHAVIORAL:         return { "B"  };

    case TYPE::SUBCKT:               return { "X"  };
    case TYPE::XSPICE:               return { "A"  };

    case TYPE::KIBIS_DEVICE:         return { "X"  };
    case TYPE::KIBIS_DRIVER_DC:      return { "X"  };
    case TYPE::KIBIS_DRIVER_RECT:    return { "X"  };
    case TYPE::KIBIS_DRIVER_PRBS:    return { "X"  };

    case TYPE::NONE:
    case TYPE::RAWSPICE:             return {};

    default:       wxFAIL;           return {};
    }
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
                if( deviceTypeFieldValue == DeviceInfo( TypeInfo( type ).deviceType ).fieldValue )
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
    {
        return TYPE::NONE;
    }
}


template <>
void SIM_MODEL::ReadDataFields( const std::vector<SCH_FIELD>* aFields,
                                const std::vector<LIB_PIN*>& aPins )
{
    doReadDataFields( aFields, aPins );
}


template <>
void SIM_MODEL::ReadDataFields( const std::vector<LIB_FIELD>* aFields,
                                const std::vector<LIB_PIN*>& aPins )
{
    doReadDataFields( aFields, aPins );
}


template <>
void SIM_MODEL::WriteFields( std::vector<SCH_FIELD>& aFields ) const
{
    doWriteFields( aFields );
}


template <>
void SIM_MODEL::WriteFields( std::vector<LIB_FIELD>& aFields ) const
{
    doWriteFields( aFields );
}


std::unique_ptr<SIM_MODEL> SIM_MODEL::CreateFallback( TYPE aType, const std::string& aSpiceCode )
{
    std::unique_ptr<SIM_MODEL> model( new SIM_MODEL_SPICE_FALLBACK( aType, aSpiceCode ) );
    return model;
}


std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( TYPE aType, const std::vector<LIB_PIN*>& aPins )
{
    std::unique_ptr<SIM_MODEL> model = Create( aType );

    // Passing nullptr to ReadDataFields will make it act as if all fields were empty.
    model->ReadDataFields( static_cast<const std::vector<SCH_FIELD>*>( nullptr ), aPins );
    return model;
}


std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const SIM_MODEL& aBaseModel,
                                              const std::vector<LIB_PIN*>& aPins)
{
    std::unique_ptr<SIM_MODEL> model;

    if( dynamic_cast<const SIM_MODEL_SPICE_FALLBACK*>( &aBaseModel ) )
        model = CreateFallback( aBaseModel.GetType() );
    else
        model = Create( aBaseModel.GetType() );

    try
    {
        model->SetBaseModel( aBaseModel );
    }
    catch( IO_ERROR& err )
    {
        DisplayErrorMessage( nullptr, err.What() );
    }

    model->ReadDataFields( static_cast<const std::vector<SCH_FIELD>*>( nullptr ), aPins );
    return model;
}


template <typename T>
std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const SIM_MODEL& aBaseModel,
                                              const std::vector<LIB_PIN*>& aPins,
                                              const std::vector<T>& aFields )
{
    TYPE type = ReadTypeFromFields( aFields );

    // If the model has a specified type, it takes priority over the type of its base class.
    if( type == TYPE::NONE )
        type = aBaseModel.GetType();

    std::unique_ptr<SIM_MODEL> model;

    if( dynamic_cast<const SIM_MODEL_SPICE_FALLBACK*>( &aBaseModel ) )
        model = CreateFallback( type );
    else
        model = Create( type );

    try
    {
        model->SetBaseModel( aBaseModel );
    }
    catch( IO_ERROR& err )
    {
        DisplayErrorMessage( nullptr, err.What() );
    }

    model->ReadDataFields( &aFields, aPins );
    return model;
}

template std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const SIM_MODEL& aBaseModel,
                                                       const std::vector<LIB_PIN*>& aPins,
                                                       const std::vector<SCH_FIELD>& aFields );

template std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const SIM_MODEL& aBaseModel,
                                                       const std::vector<LIB_PIN*>& aPins,
                                                       const std::vector<LIB_FIELD>& aFields );


template <typename T>
std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const std::vector<T>& aFields,
                                              const std::vector<LIB_PIN*>& aPins )
{
    TYPE type = ReadTypeFromFields( aFields );
    std::unique_ptr<SIM_MODEL> model = SIM_MODEL::Create( type );

    model->ReadDataFields( &aFields, aPins );
    return model;
}

template std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const std::vector<SCH_FIELD>& aFields,
                                                       const std::vector<LIB_PIN*>& aPins );
template std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const std::vector<LIB_FIELD>& aFields,
                                                       const std::vector<LIB_PIN*>& aPins );


template <typename T>
std::string SIM_MODEL::GetFieldValue( const std::vector<T>* aFields, const std::string& aFieldName,
                                      bool aResolve )
{
    static_assert( std::is_same<T, SCH_FIELD>::value || std::is_same<T, LIB_FIELD>::value );

    if( !aFields )
        return ""; // Should not happen, T=void specialization will be called instead.

    for( const T& field : *aFields )
    {
        if( field.GetName() == aFieldName )
            return aResolve ? field.GetShownText().ToStdString() : field.GetText().ToStdString();
    }

    return "";
}


// This specialization is used when no fields are passed.
template <>
std::string SIM_MODEL::GetFieldValue( const std::vector<void>* aFields,
                                      const std::string& aFieldName, bool aResolve )
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
    {
        aFields.emplace_back( aFields.size(), aFieldName );
    }

    aFields.back().SetText( aValue );
}


template void SIM_MODEL::SetFieldValue<SCH_FIELD>( std::vector<SCH_FIELD>& aFields,
                                                   const std::string& aFieldName,
                                                   const std::string& aValue );
template void SIM_MODEL::SetFieldValue<LIB_FIELD>( std::vector<LIB_FIELD>& aFields,
                                                   const std::string& aFieldName,
                                                   const std::string& aValue );

SIM_MODEL::~SIM_MODEL() = default;


void SIM_MODEL::AddPin( const PIN& aPin )
{
    m_pins.push_back( aPin );
}


void SIM_MODEL::ClearPins()
{
    m_pins.clear();
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

    // Enums are initialized with their default values.
    if( aInfo.enumValues.size() >= 1 )
        m_params.back().value->FromString( aInfo.defaultValue );
}


std::vector<std::reference_wrapper<const SIM_MODEL::PIN>> SIM_MODEL::GetPins() const
{
    std::vector<std::reference_wrapper<const PIN>> pins;

    for( int modelPinIndex = 0; modelPinIndex < GetPinCount(); ++modelPinIndex )
        pins.emplace_back( GetPin( modelPinIndex ) );

    return pins;
}

void SIM_MODEL::SetPinSymbolPinNumber( int aPinIndex, const std::string& aSymbolPinNumber )
{
    m_pins.at( aPinIndex ).symbolPinNumber = aSymbolPinNumber;
}


void SIM_MODEL::SetPinSymbolPinNumber( const std::string& aPinName,
                                       const std::string& aSymbolPinNumber )
{
    for( PIN& pin : m_pins )
    {
        if( pin.name == aPinName )
        {
            pin.symbolPinNumber = aSymbolPinNumber;
            return;
        }
    }

    // If aPinName wasn't in fact a name, see if it's a raw (1-based) index.  This is required
    // for legacy files which didn't use pin names.
    int aPinIndex = (int) strtol( aPinName.c_str(), nullptr, 10 );

    if( aPinIndex < 1 )
    {
        THROW_IO_ERROR( wxString::Format( _( "Could not find a pin named '%s' in simulation "
                                             "model of type '%s'" ),
                                          aPinName,
                                          GetTypeInfo().fieldValue ) );
    }

    m_pins[ --aPinIndex /* convert to 0-based */ ].symbolPinNumber = aSymbolPinNumber;
}


const SIM_MODEL::PARAM& SIM_MODEL::GetParam( unsigned aParamIndex ) const
{
    if( m_baseModel && m_params.at( aParamIndex ).value->ToString() == "" )
        return m_baseModel->GetParam( aParamIndex );
    else
        return m_params.at( aParamIndex );
}


int SIM_MODEL::doFindParam( const std::string& aParamName ) const
{
    std::string lowerParamName = boost::to_lower_copy( aParamName );

    std::vector<std::reference_wrapper<const PARAM>> params = GetParams();

    for( int ii = 0; ii < (int) params.size(); ++ii )
    {
        if( params[ii].get().info.name == lowerParamName )
            return ii;
    }

    return -1;
}


const SIM_MODEL::PARAM* SIM_MODEL::FindParam( const std::string& aParamName ) const
{
    int idx = doFindParam( aParamName );

    return idx >= 0 ? &GetParam( idx ) : nullptr;
}


std::vector<std::reference_wrapper<const SIM_MODEL::PARAM>> SIM_MODEL::GetParams() const
{
    std::vector<std::reference_wrapper<const PARAM>> params;

    for( int i = 0; i < GetParamCount(); ++i )
        params.emplace_back( GetParam( i ) );

    return params;
}


const SIM_MODEL::PARAM& SIM_MODEL::GetParamOverride( unsigned aParamIndex ) const
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


void SIM_MODEL::SetParamValue( const std::string& aParamName, const std::string& aValue,
                               SIM_VALUE::NOTATION aNotation )
{
    int idx = doFindParam( aParamName );

    if( idx < 0 )
    {
        THROW_IO_ERROR( wxString::Format( _( "Could not find a parameter named '%s' in "
                                             "simulation model of type '%s'" ),
                                          aParamName,
                                          GetTypeInfo().fieldValue ) );
    }

    SetParamValue( idx, aValue, aNotation );
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

    case TYPE::R_POT:
        return std::make_unique<SIM_MODEL_R_POT>();

    case TYPE::L_MUTUAL:
        return std::make_unique<SIM_MODEL_L_MUTUAL>();

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
        return std::make_unique<SIM_MODEL_KIBIS>( aType );

    case TYPE::RAWSPICE:
        return std::make_unique<SIM_MODEL_RAW_SPICE>();

    default:
        return std::make_unique<SIM_MODEL_NGSPICE>( aType );
    }
}


SIM_MODEL::SIM_MODEL( TYPE aType ) :
        SIM_MODEL( aType, std::make_unique<SPICE_GENERATOR>( *this ),
                   std::make_unique<SIM_MODEL_SERIALIZER>( *this ) )
{
}


SIM_MODEL::SIM_MODEL( TYPE aType, std::unique_ptr<SPICE_GENERATOR> aSpiceGenerator ) :
        SIM_MODEL( aType, std::move( aSpiceGenerator ),
                   std::make_unique<SIM_MODEL_SERIALIZER>( *this ) )
{
}


SIM_MODEL::SIM_MODEL( TYPE aType, std::unique_ptr<SPICE_GENERATOR> aSpiceGenerator,
                      std::unique_ptr<SIM_MODEL_SERIALIZER> aSerializer ) :
        m_baseModel( nullptr ),
        m_serializer( std::move( aSerializer ) ),
        m_spiceGenerator( std::move( aSpiceGenerator ) ),
        m_type( aType ),
        m_isEnabled( true ),
        m_isStoredInValue( false )
{
}


void SIM_MODEL::createPins( const std::vector<LIB_PIN*>& aSymbolPins )
{
    // Default pin sequence: model pins are the same as symbol pins.
    // Excess model pins are set as Not Connected.
    // Note that intentionally nothing is added if `getPinNames()` returns an empty vector.

    // SIM_MODEL pins must be ordered by symbol pin numbers -- this is assumed by the code that
    // accesses them.

    std::vector<std::string> pinNames = getPinNames();

    for( unsigned modelPinIndex = 0; modelPinIndex < pinNames.size(); ++modelPinIndex )
    {
        if( modelPinIndex < aSymbolPins.size() )
        {
            AddPin( { pinNames.at( modelPinIndex ),
                      aSymbolPins[ modelPinIndex ]->GetNumber().ToStdString() } );
        }
        else
        {
            AddPin( { pinNames.at( modelPinIndex ), "" } );
        }
    }
}


template <typename T>
void SIM_MODEL::doReadDataFields( const std::vector<T>* aFields,
                                  const std::vector<LIB_PIN*>& aPins )
{
    bool diffMode = GetFieldValue( aFields, SIM_LIBRARY_KIBIS::DIFF_FIELD ) == "1";
    SwitchSingleEndedDiff( diffMode );

    try
    {
        m_serializer->ParseEnable( GetFieldValue( aFields, ENABLE_FIELD ) );

        createPins( aPins );
        m_serializer->ParsePins( GetFieldValue( aFields, PINS_FIELD ) );

        std::string paramsField = GetFieldValue( aFields, PARAMS_FIELD );

        if( !m_serializer->ParseParams( paramsField ) )
            m_serializer->ParseValue( GetFieldValue( aFields, VALUE_FIELD ) );
    }
    catch( IO_ERROR& err )
    {
        DisplayErrorMessage( nullptr, err.What() );
    }
}


template <typename T>
void SIM_MODEL::doWriteFields( std::vector<T>& aFields ) const
{
    SetFieldValue( aFields, DEVICE_TYPE_FIELD, m_serializer->GenerateDevice() );
    SetFieldValue( aFields, TYPE_FIELD, m_serializer->GenerateType() );

    SetFieldValue( aFields, ENABLE_FIELD, m_serializer->GenerateEnable() );
    SetFieldValue( aFields, PINS_FIELD, m_serializer->GeneratePins() );

    SetFieldValue( aFields, PARAMS_FIELD, m_serializer->GenerateParams() );

    if( IsStoredInValue() )
        SetFieldValue( aFields, VALUE_FIELD, m_serializer->GenerateValue() );
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


template <class T_symbol, class T_field>
bool SIM_MODEL::InferSimModel( T_symbol& aSymbol, std::vector<T_field>* aFields, bool aResolve,
                               SIM_VALUE_GRAMMAR::NOTATION aNotation, wxString* aDeviceType,
                               wxString* aModelType, wxString* aModelParams, wxString* aPinMap )
{
    // SPICE notation is case-insensitive and locale-insensitve.  This means it uses "Meg" for
    // mega (as both 'M' and 'm' must mean milli), and "." (always) for a decimal separator.
    //
    // KiCad's GUI uses the SI-standard 'M' for mega and 'm' for milli, and a locale-dependent
    // decimal separator.
    //
    // KiCad's Sim.* fields are in-between, using SI notation but a fixed decimal separator.
    //
    // So where does that leave inferred value fields?  Behavioural models must be passed in
    // straight, because we don't (at present) know how to parse them.
    //
    // However, behavioural models _look_ like SPICE code, so it's not a stretch to expect them
    // to _be_ SPICE code.  A passive capacitor model on the other hand, just looks like a
    // capacitance.  Some users might expect 3,3u to work, while others might expect 3,300uF to
    // work.
    //
    // Checking the locale isn't reliable because it assumes the current computer's locale is
    // the same as the locale the schematic was authored in -- something that isn't true, for
    // instance, when sharing designs over DIYAudio.com.
    //
    // However, even the E192 series of preferred values uses only 3 significant digits, so a ','
    // or '.' followed by 3 digits _could_ reasonably-reliably be interpreted as a thousands
    // separator.
    //
    // Or we could just say inferred values are locale-independent, with "." used as a decimal
    // separator and "," used as a thousands separator.  3,300uF works, but 3,3 does not.

    auto convertNotation =
            [&]( const wxString& units ) -> wxString
            {
                if( aNotation == SIM_VALUE_GRAMMAR::NOTATION::SPICE )
                {
                    if( units == wxT( "M" ) )
                        return wxT( "Meg" );
                }
                else if( aNotation == SIM_VALUE_GRAMMAR::NOTATION::SI )
                {
                    if( units.Capitalize() == wxT( "Meg" ) )
                        return wxT( "M" );
                }

                return units;
            };

    auto convertSeparators =
            []( wxString* mantissa )
            {
                mantissa->Replace( wxS( " " ), wxEmptyString );

                wxChar thousandsSeparator = '?';
                wxChar decimalSeparator = '?';
                int    length = (int) mantissa->length();
                int    radix = length;

                for( int ii = length - 1; ii >= 0; --ii )
                {
                    wxChar c = mantissa->GetChar( ii );

                    if( c == '.' || c == ',' )
                    {
                        if( ( radix - ii ) % 4 == 0 )
                        {
                            if( thousandsSeparator == '?' )
                            {
                                thousandsSeparator = c;
                                decimalSeparator = c == '.' ? ',' : '.';
                            }
                            else if( thousandsSeparator != c )
                            {
                                return false;
                            }
                        }
                        else
                        {
                            if( decimalSeparator == '?' )
                            {
                                decimalSeparator = c;
                                thousandsSeparator = c == '.' ? ',' : '.';
                                radix = ii;
                            }
                            else
                            {
                                return false;
                            }
                        }
                    }
                }

                mantissa->Replace( thousandsSeparator, wxEmptyString );
                mantissa->Replace( decimalSeparator, '.' );

                return true;
            };

    wxString              prefix = aSymbol.GetPrefix();
    wxString              value = GetFieldValue( aFields, VALUE_FIELD, aResolve );
    std::vector<LIB_PIN*> pins = aSymbol.GetAllLibPins();

    *aDeviceType = GetFieldValue( aFields, DEVICE_TYPE_FIELD, aResolve );
    *aModelType = GetFieldValue( aFields, TYPE_FIELD, aResolve );
    *aModelParams = GetFieldValue( aFields, PARAMS_FIELD, aResolve );
    *aPinMap = GetFieldValue( aFields, PINS_FIELD, aResolve );

    if( pins.size() != 2 )
        return false;

    if(   ( ( *aDeviceType == "R" || *aDeviceType == "L" || *aDeviceType == "C" )
            && aModelType->IsEmpty() )
       ||
          ( aDeviceType->IsEmpty()
            && aModelType->IsEmpty()
            && !value.IsEmpty()
            && ( prefix.StartsWith( "R" ) || prefix.StartsWith( "L" ) || prefix.StartsWith( "C" ) ) ) )
    {
        if( aModelParams->IsEmpty() )
        {
            wxRegEx idealVal( wxT( "^"
                                   "([0-9\\,\\. ]+)"
                                   "([fFpPnNuUmMkKgGtTμµ𝛍𝜇𝝁 ]|M(e|E)(g|G))?"
                                   "([fFhHΩΩ𝛀𝛺𝝮rR]|ohm)?"
                                   "([-1-9 ]*)"
                                   "([fFhHΩΩ𝛀𝛺𝝮rR]|ohm)?"
                                   "$" ) );

            if( idealVal.Matches( value ) )      // Ideal
            {
                wxString valueMantissa( idealVal.GetMatch( value, 1 ) );
                wxString valueExponent( idealVal.GetMatch( value, 2 ) );
                wxString valueFraction( idealVal.GetMatch( value, 6 ) );

                if( !convertSeparators( &valueMantissa ) )
                    return false;

                if( valueMantissa.Contains( wxT( "." ) ) || valueFraction.IsEmpty() )
                {
                    aModelParams->Printf( wxT( "%s=\"%s%s\"" ),
                                          prefix.Left(1).Lower(),
                                          valueMantissa,
                                          convertNotation( valueExponent ) );
                }
                else
                {
                    aModelParams->Printf( wxT( "%s=\"%s.%s%s\"" ),
                                          prefix.Left(1).Lower(),
                                          valueMantissa,
                                          valueFraction,
                                          convertNotation( valueExponent ) );
                }
            }
            else        // Behavioral
            {
                *aModelType = wxT( "=" );
                aModelParams->Printf( wxT( "%s=\"%s\"" ), prefix.Left(1).Lower(), value );
            }
        }

        if( aDeviceType->IsEmpty() )
            *aDeviceType = prefix.Left( 1 );

        if( aPinMap->IsEmpty() )
            aPinMap->Printf( wxT( "%s=+ %s=-" ), pins[0]->GetNumber(), pins[1]->GetNumber() );

        return true;
    }

    if(   ( ( *aDeviceType == wxT( "V" ) || *aDeviceType == wxT( "I" ) )
            && aModelType->IsEmpty() )
       ||
          ( aDeviceType->IsEmpty()
            && aModelType->IsEmpty()
            && !value.IsEmpty()
            && ( prefix.StartsWith( "V" ) || prefix.StartsWith( "I" ) ) )  )
    {
        if( aModelParams->IsEmpty() && !value.IsEmpty() )
        {
            if( value.StartsWith( wxT( "DC " ) ) )
                value = value.Right( value.Length() - 3 );

            wxRegEx sourceVal( wxT( "^"
                                    "([0-9\\,\\. ]+)"
                                    "([fFpPnNuUmMkKgGtTμµ𝛍𝜇𝝁 ]|M(e|E)(g|G))?"
                                    "([vVaA])?"
                                    "([-1-9 ]*)"
                                    "([vVaA])?"
                                    "$" ) );

            if( sourceVal.Matches( value ) )
            {
                wxString valueMantissa( sourceVal.GetMatch( value, 1 ) );
                wxString valueExponent( sourceVal.GetMatch( value, 2 ) );
                wxString valueFraction( sourceVal.GetMatch( value, 6 ) );

                if( !convertSeparators( &valueMantissa ) )
                    return false;

                if( valueMantissa.Contains( wxT( "." ) ) || valueFraction.IsEmpty() )
                {
                    aModelParams->Printf( wxT( "dc=\"%s%s\"" ),
                                          valueMantissa,
                                          convertNotation( valueExponent ) );
                }
                else
                {
                    aModelParams->Printf( wxT( "dc=\"%s.%s%s\"" ),
                                          valueMantissa,
                                          valueFraction,
                                          convertNotation( valueExponent ) );
                }
            }
            else
            {
                aModelParams->Printf( wxT( "dc=\"%s\"" ), value );
            }
        }

        if( aDeviceType->IsEmpty() )
            *aDeviceType = prefix.Left( 1 );

        if( aModelType->IsEmpty() )
            *aModelType = wxT( "DC" );

        if( aPinMap->IsEmpty() )
            aPinMap->Printf( wxT( "%s=+ %s=-" ), pins[0]->GetNumber(), pins[1]->GetNumber() );

        return true;
    }

    return false;
}


template bool SIM_MODEL::InferSimModel<SCH_SYMBOL, SCH_FIELD>( SCH_SYMBOL& aSymbol,
                                                               std::vector<SCH_FIELD>* aFields,
                                                               bool aResolve,
                                                               SIM_VALUE_GRAMMAR::NOTATION aNotation,
                                                               wxString* aDeviceType,
                                                               wxString* aModelType,
                                                               wxString* aModelParams,
                                                               wxString* aPinMap );
template bool SIM_MODEL::InferSimModel<LIB_SYMBOL, LIB_FIELD>( LIB_SYMBOL& aSymbol,
                                                               std::vector<LIB_FIELD>* aFields,
                                                               bool aResolve,
                                                               SIM_VALUE_GRAMMAR::NOTATION aNotation,
                                                               wxString* aDeviceType,
                                                               wxString* aModelType,
                                                               wxString* aModelParams,
                                                               wxString* aPinMap );


template <typename T_symbol, typename T_field>
void SIM_MODEL::MigrateSimModel( T_symbol& aSymbol, const PROJECT* aProject )
{
    if( aSymbol.FindField( SIM_MODEL::DEVICE_TYPE_FIELD )
        || aSymbol.FindField( SIM_MODEL::TYPE_FIELD )
        || aSymbol.FindField( SIM_MODEL::PINS_FIELD )
        || aSymbol.FindField( SIM_MODEL::PARAMS_FIELD ) )
    {
        // Has a V7 model field -- skip.
        return;
    }

    auto getSIValue =
            []( T_field* aField )
            {
                if( !aField )   // no, not really, but it keeps Coverity happy
                    return wxString( wxEmptyString );

                wxRegEx  regex( wxT( "([^a-z])(M)(e|E)(g|G)($|[^a-z])" ) );
                wxString value = aField->GetText();

                // Keep prefix, M, and suffix, but drop e|E and g|G
                regex.ReplaceAll( &value, wxT( "\\1\\2\\5" ) );

                return value;
            };

    auto generateDefaultPinMapFromSymbol =
            []( const std::vector<LIB_PIN*>& sourcePins )
            {
                wxString pinMap;

                // If we're creating the pinMap from the symbol it means we don't know what the
                // SIM_MODEL's pin names are, so just use indexes.

                for( unsigned ii = 0; ii < sourcePins.size(); ++ii )
                {
                    if( ii > 0 )
                        pinMap.Append( wxS( " " ) );

                    pinMap.Append( wxString::Format( wxT( "%s=%u" ),
                                                     sourcePins[ii]->GetNumber(),
                                                     ii + 1 ) );
                }

                return pinMap;
            };

    wxString              prefix = aSymbol.GetPrefix();
    T_field*              valueField = aSymbol.FindField( wxT( "Value" ) );
    std::vector<LIB_PIN*> sourcePins = aSymbol.GetAllLibPins();

    std::sort( sourcePins.begin(), sourcePins.end(),
               []( const LIB_PIN* lhs, const LIB_PIN* rhs )
               {
                   return StrNumCmp( lhs->GetNumber(), rhs->GetNumber(), true ) < 0;
               } );

    wxString spiceDeviceType;
    wxString spiceModel;
    wxString spiceType;
    wxString spiceLib;
    wxString pinMap;
    wxString spiceParams;
    bool     modelFromValueField = false;

    if( aSymbol.FindField( wxT( "Spice_Primitive" ) )
        || aSymbol.FindField( wxT( "Spice_Node_Sequence" ) )
        || aSymbol.FindField( wxT( "Spice_Model" ) )
        || aSymbol.FindField( wxT( "Spice_Netlist_Enabled" ) )
        || aSymbol.FindField( wxT( "Spice_Lib_File" ) ) )
    {
        if( T_field* primitiveField = aSymbol.FindField( wxT( "Spice_Primitive" ) ) )
        {
            spiceDeviceType = primitiveField->GetText();
            aSymbol.RemoveField( primitiveField );
        }

        if( T_field* nodeSequenceField = aSymbol.FindField( wxT( "Spice_Node_Sequence" ) ) )
        {
            const wxString  delimiters( "{:,; }" );
            const wxString& nodeSequence = nodeSequenceField->GetText();

            if( nodeSequence != "" )
            {
                wxStringTokenizer tkz( nodeSequence, delimiters );

                for( long modelPinNumber = 1; tkz.HasMoreTokens(); ++modelPinNumber )
                {
                    long symbolPinNumber = 1;
                    tkz.GetNextToken().ToLong( &symbolPinNumber );

                    if( modelPinNumber != 1 )
                        pinMap.Append( " " );

                    pinMap.Append( wxString::Format( "%ld=%ld", symbolPinNumber, modelPinNumber ) );
                }
            }

            aSymbol.RemoveField( nodeSequenceField );
        }

        if( T_field* modelField = aSymbol.FindField( wxT( "Spice_Model" ) ) )
        {
            spiceModel = getSIValue( modelField );
            aSymbol.RemoveField( modelField );
        }
        else
        {
            spiceModel = getSIValue( valueField );
            modelFromValueField = true;
        }

        if( T_field* netlistEnabledField = aSymbol.FindField( wxT( "Spice_Netlist_Enabled" ) ) )
        {
            wxString netlistEnabled = netlistEnabledField->GetText().Lower();

            if( netlistEnabled.StartsWith( wxT( "0" ) )
                || netlistEnabled.StartsWith( wxT( "n" ) )
                || netlistEnabled.StartsWith( wxT( "f" ) ) )
            {
                netlistEnabledField->SetName( SIM_MODEL::ENABLE_FIELD );
                netlistEnabledField->SetText( wxT( "0" ) );
            }
            else
            {
                aSymbol.RemoveField( netlistEnabledField );
            }
        }

        if( T_field* libFileField = aSymbol.FindField( wxT( "Spice_Lib_File" ) ) )
        {
            spiceLib = libFileField->GetText();
            aSymbol.RemoveField( libFileField );
        }
    }
    else
    {
        // Auto convert some legacy fields used in the middle of 7.0 development...

        if( T_field* legacyType = aSymbol.FindField( wxT( "Sim_Type" ) ) )
        {
            legacyType->SetName( SIM_MODEL::TYPE_FIELD );
        }

        if( T_field* legacyDevice = aSymbol.FindField( wxT( "Sim_Device" ) ) )
        {
            legacyDevice->SetName( SIM_MODEL::DEVICE_TYPE_FIELD );
        }

        if( T_field* legacyPins = aSymbol.FindField( wxT( "Sim_Pins" ) ) )
        {
            bool isPassive =   prefix.StartsWith( wxT( "R" ) )
                            || prefix.StartsWith( wxT( "L" ) )
                            || prefix.StartsWith( wxT( "C" ) );

            // Migrate pins from array of indexes to name-value-pairs
            wxArrayString pinIndexes;

            wxStringSplit( legacyPins->GetText(), pinIndexes, ' ' );

            if( isPassive && pinIndexes.size() == 2 && sourcePins.size() == 2 )
            {
                if( pinIndexes[0] == wxT( "2" ) )
                {
                    pinMap.Printf( wxT( "%s=- %s=+" ),
                                   sourcePins[0]->GetNumber(),
                                   sourcePins[1]->GetNumber() );
                }
                else
                {
                    pinMap.Printf( wxT( "%s=+ %s=-" ),
                                   sourcePins[0]->GetNumber(),
                                   sourcePins[1]->GetNumber() );
                }
            }
            else
            {
                for( unsigned ii = 0; ii < pinIndexes.size(); ++ii )
                {
                    if( ii > 0 )
                        pinMap.Append( wxS( " " ) );

                    pinMap.Append( wxString::Format( wxT( "%s=%s" ),
                                                     sourcePins[ii]->GetNumber(),
                                                     pinIndexes[ ii ] ) );
                }
            }

            legacyPins->SetName( SIM_MODEL::PINS_FIELD );
            legacyPins->SetText( pinMap );
        }

        if( T_field* legacyParams = aSymbol.FindField( wxT( "Sim_Params" ) ) )
        {
            legacyParams->SetName( SIM_MODEL::PARAMS_FIELD );
        }

        return;
    }

    spiceDeviceType = spiceDeviceType.Trim( true ).Trim( false );
    spiceModel = spiceModel.Trim( true ).Trim( false );
    spiceType = spiceType.Trim( true ).Trim( false );

    bool libraryModel = false;
    bool inferredModel = false;
    bool internalModel = false;

    if( !spiceLib.IsEmpty() )
    {
        SIM_LIB_MGR libMgr( aProject );

        try
        {
            std::vector<T_field> emptyFields;
            SIM_LIBRARY::MODEL model = libMgr.CreateModel( spiceLib, spiceModel.ToStdString(),
                                                           emptyFields, sourcePins );

            spiceParams = wxString( model.model.GetBaseModel()->Serializer().GenerateParams() );
            libraryModel = true;

            if( pinMap.IsEmpty() )
            {
                // Try to generate a default pin map from the SIM_MODEL's pins; if that fails,
                // generate one from the symbol's pins

                model.model.SIM_MODEL::createPins( sourcePins );
                pinMap = wxString( model.model.Serializer().GeneratePins() );

                if( pinMap.IsEmpty() )
                    pinMap = generateDefaultPinMapFromSymbol( sourcePins );
            }
        }
        catch( ... )
        {
            // Fall back to raw spice model
        }
    }
    else if( ( spiceDeviceType == "R" || spiceDeviceType == "L" || spiceDeviceType == "C" )
            && prefix.StartsWith( spiceDeviceType )
            && modelFromValueField )
    {
        inferredModel = true;
    }
    else
    {
        // See if we have a SPICE model such as "sin(0 1 60)" or "sin 0 1 60" that can be handled
        // by a built-in SIM_MODEL.

        wxStringTokenizer tokenizer( spiceModel, wxT( "() " ), wxTOKEN_STRTOK );

        if( tokenizer.HasMoreTokens() )
        {
            spiceType = tokenizer.GetNextToken();
            spiceType.MakeUpper();

            for( SIM_MODEL::TYPE type : SIM_MODEL::TYPE_ITERATOR() )
            {
                if( spiceDeviceType == SIM_MODEL::SpiceInfo( type ).itemType
                        && spiceType == SIM_MODEL::SpiceInfo( type ).inlineTypeString )
                {
                    try
                    {
                        std::unique_ptr<SIM_MODEL> model = SIM_MODEL::Create( type );

                        if( spiceType == wxT( "DC" ) && tokenizer.CountTokens() == 1 )
                        {
                            valueField->SetText( tokenizer.GetNextToken() );
                            modelFromValueField = false;
                        }
                        else
                        {
                            for( int ii = 0; tokenizer.HasMoreTokens(); ++ii )
                            {
                                model->SetParamValue( ii, tokenizer.GetNextToken().ToStdString(),
                                                      SIM_VALUE_GRAMMAR::NOTATION::SPICE );
                            }

                            spiceParams = wxString( model->Serializer().GenerateParams() );
                        }

                        internalModel = true;

                        if( pinMap.IsEmpty() )
                        {
                            // Generate a default pin map from the SIM_MODEL's pins
                            model->createPins( sourcePins );
                            pinMap = wxString( model->Serializer().GeneratePins() );
                        }
                    }
                    catch( ... )
                    {
                        // Fall back to raw spice model
                    }

                    break;
                }
            }
        }
    }

    if( libraryModel )
    {
        T_field libraryField( &aSymbol, -1, SIM_MODEL::LIBRARY_FIELD );
        libraryField.SetText( spiceLib );
        aSymbol.AddField( libraryField );

        T_field nameField( &aSymbol, -1, SIM_MODEL::NAME_FIELD );
        nameField.SetText( spiceModel );
        aSymbol.AddField( nameField );

        T_field paramsField( &aSymbol, -1, SIM_MODEL::PARAMS_FIELD );
        paramsField.SetText( spiceParams );
        aSymbol.AddField( paramsField );

        if( modelFromValueField )
            valueField->SetText( wxT( "${SIM.NAME}" ) );
    }
    else if( inferredModel )
    {
        // DeviceType is left in the reference designator and Model is left in the value field,
        // so there's nothing to do here....
    }
    else if( internalModel )
    {
        T_field deviceTypeField( &aSymbol, -1, SIM_MODEL::DEVICE_TYPE_FIELD );
        deviceTypeField.SetText( spiceDeviceType );
        aSymbol.AddField( deviceTypeField );

        T_field typeField( &aSymbol, -1, SIM_MODEL::TYPE_FIELD );
        typeField.SetText( spiceType );
        aSymbol.AddField( typeField );

        T_field paramsField( &aSymbol, -1, SIM_MODEL::PARAMS_FIELD );
        paramsField.SetText( spiceParams );
        aSymbol.AddField( paramsField );

        if( modelFromValueField )
            valueField->SetText( wxT( "${SIM.PARAMS}" ) );
    }
    else    // Insert a raw spice model as a substitute.
    {
        if( spiceDeviceType.IsEmpty() && spiceLib.IsEmpty() )
        {
            spiceParams = spiceModel;
        }
        else
        {
            spiceParams.Printf( wxT( "type=\"%s\" model=\"%s\" lib=\"%s\"" ),
                                spiceDeviceType, spiceModel, spiceLib );
        }

        T_field deviceTypeField( &aSymbol, -1, SIM_MODEL::DEVICE_TYPE_FIELD );
        deviceTypeField.SetText( SIM_MODEL::DeviceInfo( SIM_MODEL::DEVICE_T::SPICE ).fieldValue );
        aSymbol.AddField( deviceTypeField );

        T_field paramsField( &aSymbol, -1, SIM_MODEL::PARAMS_FIELD );
        paramsField.SetText( spiceParams );
        aSymbol.AddField( paramsField );

        if( modelFromValueField )
        {
            // Get the current Value field, after previous changes.
            valueField = aSymbol.FindField( wxT( "Value" ) );

            if( valueField )
                valueField->SetText( wxT( "${SIM.PARAMS}" ) );
        }

        // We know nothing about the SPICE model here, so we've got no choice but to generate
        // the default pin map from the symbol's pins.

        if( pinMap.IsEmpty() )
            pinMap = generateDefaultPinMapFromSymbol( sourcePins );
    }

    if( !pinMap.IsEmpty() )
    {
        T_field pinsField( &aSymbol, -1, SIM_MODEL::PINS_FIELD );
        pinsField.SetText( pinMap );
        aSymbol.AddField( pinsField );
    }
}


template void SIM_MODEL::MigrateSimModel<SCH_SYMBOL, SCH_FIELD>( SCH_SYMBOL& aSymbol,
                                                                 const PROJECT* aProject );
template void SIM_MODEL::MigrateSimModel<LIB_SYMBOL, LIB_FIELD>( LIB_SYMBOL& aSymbol,
                                                                 const PROJECT* aProject );

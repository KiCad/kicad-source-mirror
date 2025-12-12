/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022 CERN
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <ki_exception.h>
#include <lib_symbol.h>
#include <sch_symbol.h>
#include <string_utils.h>
#include <wx/regex.h>

#include <iterator>

#include <sim/sim_model.h>
#include <sim/sim_model_behavioral.h>
#include <sim/sim_model_ideal.h>
#include <sim/sim_model_l_mutual.h>
#include <sim/sim_model_ngspice.h>
#include <sim/sim_model_r_pot.h>
#include <sim/sim_model_ibis.h>
#include <sim/sim_model_source.h>
#include <sim/sim_model_raw_spice.h>
#include <sim/sim_model_subckt.h>
#include <sim/sim_model_switch.h>
#include <sim/sim_model_tline.h>
#include <sim/sim_model_xspice.h>
#include <sim/sim_lib_mgr.h>
#include <sim/sim_library_ibis.h>

#include <boost/algorithm/string.hpp>
#include <fmt/core.h>
#include <pegtl/contrib/parse_tree.hpp>


#include "sim_model_spice_fallback.h"

using TYPE = SIM_MODEL::TYPE;


SIM_MODEL::DEVICE_INFO SIM_MODEL::DeviceInfo( DEVICE_T aDeviceType )
{
    switch( aDeviceType )
    {
    //                             | fieldValue  | description              | showInMenu |
    //                             -------------------------------------------------------
    //
    case DEVICE_T::NONE:      return { "",        "",                             true  };
    case DEVICE_T::R:         return { "R",       "Resistor",                     true  };
    case DEVICE_T::C:         return { "C",       "Capacitor",                    true  };
    case DEVICE_T::L:         return { "L",       "Inductor",                     true  };
    case DEVICE_T::K:         return { "K",       "Mutual Inductance Statement",  true  };
    case DEVICE_T::TLINE:     return { "TLINE",   "Transmission Line",            true  };
    case DEVICE_T::SW:        return { "SW",      "Switch",                       true  };

    case DEVICE_T::D:         return { "D",       "Diode",                        true  };
    case DEVICE_T::NPN:       return { "NPN",     "NPN BJT",                      true  };
    case DEVICE_T::PNP:       return { "PNP",     "PNP BJT",                      true  };

    case DEVICE_T::NJFET:     return { "NJFET",   "N-channel JFET",               true  };
    case DEVICE_T::PJFET:     return { "PJFET",   "P-channel JFET",               true  };

    case DEVICE_T::NMOS:      return { "NMOS",    "N-channel MOSFET",             true  };
    case DEVICE_T::PMOS:      return { "PMOS",    "P-channel MOSFET",             true  };
    case DEVICE_T::NMES:      return { "NMES",    "N-channel MESFET",             true  };
    case DEVICE_T::PMES:      return { "PMES",    "P-channel MESFET",             true  };

    case DEVICE_T::V:         return { "V",       "Voltage Source",               true  };
    case DEVICE_T::I:         return { "I",       "Current Source",               true  };
    case DEVICE_T::E:         return { "E",       "Voltage Source",               false };
    case DEVICE_T::F:         return { "F",       "Current Source",               false };
    case DEVICE_T::G:         return { "G",       "Current Source",               false };
    case DEVICE_T::H:         return { "H",       "Voltage Source",               false };

    case DEVICE_T::KIBIS:     return { "IBIS",    "IBIS Model",                   false };

    case DEVICE_T::SUBCKT:    return { "SUBCKT",  "Subcircuit",                   false };
    case DEVICE_T::XSPICE:    return { "XSPICE",  "XSPICE Code Model",            true  };
    case DEVICE_T::SPICE:     return { "SPICE",   "Raw SPICE Element",            true  };

    default:    wxFAIL;       return {};
    }
}


SIM_MODEL::INFO SIM_MODEL::TypeInfo( TYPE aType )
{
    switch( aType )
    {
    //                                      | deviceType     |  fieldValue     |  description                   |
    //                                      ---------------------------------------------------------------------
    //
    case TYPE::NONE:                 return { DEVICE_T::NONE,   "",               ""                            };

    case TYPE::R:                    return { DEVICE_T::R,      "",               "Ideal"                       };
    case TYPE::R_POT:                return { DEVICE_T::R,      "POT",            "Potentiometer"               };
    case TYPE::R_BEHAVIORAL:         return { DEVICE_T::R,      "=",              "Behavioral"                  };

    case TYPE::C:                    return { DEVICE_T::C,      "",               "Ideal"                       };
    case TYPE::C_BEHAVIORAL:         return { DEVICE_T::C,      "=",              "Behavioral"                  };

    case TYPE::L:                    return { DEVICE_T::L,      "",               "Ideal"                       };
    case TYPE::L_BEHAVIORAL:         return { DEVICE_T::L,      "=",              "Behavioral"                  };

    case TYPE::K:                    return { DEVICE_T::K,      "",               "Mutual Inductance Statement" };

    case TYPE::TLINE_Z0:             return { DEVICE_T::TLINE,  "",               "Characteristic impedance"    };
    case TYPE::TLINE_RLGC:           return { DEVICE_T::TLINE,  "RLGC",           "RLGC"                        };

    case TYPE::SW_V:                 return { DEVICE_T::SW,     "V",              "Voltage-controlled"          };
    case TYPE::SW_I:                 return { DEVICE_T::SW,     "I",              "Current-controlled"          };

    case TYPE::D:                    return { DEVICE_T::D,      "",               ""                            };

    case TYPE::NPN_VBIC:             return { DEVICE_T::NPN,    "VBIC",           "VBIC"                        };
    case TYPE::PNP_VBIC:             return { DEVICE_T::PNP,    "VBIC",           "VBIC"                        };
    case TYPE::NPN_GUMMELPOON:       return { DEVICE_T::NPN,    "GUMMELPOON",     "Gummel-Poon"                 };
    case TYPE::PNP_GUMMELPOON:       return { DEVICE_T::PNP,    "GUMMELPOON",     "Gummel-Poon"                 };
  //case TYPE::BJT_MEXTRAM:          return {};
    case TYPE::NPN_HICUM2:           return { DEVICE_T::NPN,    "HICUML2",        "HICUM level 2"               };
    case TYPE::PNP_HICUM2:           return { DEVICE_T::PNP,    "HICUML2",        "HICUM level 2"               };
  //case TYPE::BJT_HICUM_L0:         return {};

    case TYPE::NJFET_SHICHMANHODGES: return { DEVICE_T::NJFET,  "SHICHMANHODGES", "Shichman-Hodges"             };
    case TYPE::PJFET_SHICHMANHODGES: return { DEVICE_T::PJFET,  "SHICHMANHODGES", "Shichman-Hodges"             };
    case TYPE::NJFET_PARKERSKELLERN: return { DEVICE_T::NJFET,  "PARKERSKELLERN", "Parker-Skellern"             };
    case TYPE::PJFET_PARKERSKELLERN: return { DEVICE_T::PJFET,  "PARKERSKELLERN", "Parker-Skellern"             };

    case TYPE::NMES_STATZ:           return { DEVICE_T::NMES,   "STATZ",          "Statz"                       };
    case TYPE::PMES_STATZ:           return { DEVICE_T::PMES,   "STATZ",          "Statz"                       };
    case TYPE::NMES_YTTERDAL:        return { DEVICE_T::NMES,   "YTTERDAL",       "Ytterdal"                    };
    case TYPE::PMES_YTTERDAL:        return { DEVICE_T::PMES,   "YTTERDAL",       "Ytterdal"                    };
    case TYPE::NMES_HFET1:           return { DEVICE_T::NMES,   "HFET1",          "HFET1"                       };
    case TYPE::PMES_HFET1:           return { DEVICE_T::PMES,   "HFET1",          "HFET1"                       };
    case TYPE::NMES_HFET2:           return { DEVICE_T::NMES,   "HFET2",          "HFET2"                       };
    case TYPE::PMES_HFET2:           return { DEVICE_T::PMES,   "HFET2",          "HFET2"                       };

    case TYPE::NMOS_VDMOS:           return { DEVICE_T::NMOS,   "VDMOS",          "VDMOS"                       };
    case TYPE::PMOS_VDMOS:           return { DEVICE_T::PMOS,   "VDMOS",          "VDMOS"                       };
    case TYPE::NMOS_MOS1:            return { DEVICE_T::NMOS,   "MOS1",           "Classical quadratic (MOS1)"  };
    case TYPE::PMOS_MOS1:            return { DEVICE_T::PMOS,   "MOS1",           "Classical quadratic (MOS1)"  };
    case TYPE::NMOS_MOS2:            return { DEVICE_T::NMOS,   "MOS2",           "Grove-Frohman (MOS2)"        };
    case TYPE::PMOS_MOS2:            return { DEVICE_T::PMOS,   "MOS2",           "Grove-Frohman (MOS2)"        };
    case TYPE::NMOS_MOS3:            return { DEVICE_T::NMOS,   "MOS3",           "MOS3"                        };
    case TYPE::PMOS_MOS3:            return { DEVICE_T::PMOS,   "MOS3",           "MOS3"                        };
    case TYPE::NMOS_BSIM1:           return { DEVICE_T::NMOS,   "BSIM1",          "BSIM1"                       };
    case TYPE::PMOS_BSIM1:           return { DEVICE_T::PMOS,   "BSIM1",          "BSIM1"                       };
    case TYPE::NMOS_BSIM2:           return { DEVICE_T::NMOS,   "BSIM2",          "BSIM2"                       };
    case TYPE::PMOS_BSIM2:           return { DEVICE_T::PMOS,   "BSIM2",          "BSIM2"                       };
    case TYPE::NMOS_MOS6:            return { DEVICE_T::NMOS,   "MOS6",           "MOS6"                        };
    case TYPE::PMOS_MOS6:            return { DEVICE_T::PMOS,   "MOS6",           "MOS6"                        };
    case TYPE::NMOS_BSIM3:           return { DEVICE_T::NMOS,   "BSIM3",          "BSIM3"                       };
    case TYPE::PMOS_BSIM3:           return { DEVICE_T::PMOS,   "BSIM3",          "BSIM3"                       };
    case TYPE::NMOS_MOS9:            return { DEVICE_T::NMOS,   "MOS9",           "MOS9"                        };
    case TYPE::PMOS_MOS9:            return { DEVICE_T::PMOS,   "MOS9",           "MOS9"                        };
    case TYPE::NMOS_B4SOI:           return { DEVICE_T::NMOS,   "B4SOI",          "BSIM4 SOI (B4SOI)"           };
    case TYPE::PMOS_B4SOI:           return { DEVICE_T::PMOS,   "B4SOI",          "BSIM4 SOI (B4SOI)"           };
    case TYPE::NMOS_BSIM4:           return { DEVICE_T::NMOS,   "BSIM4",          "BSIM4"                       };
    case TYPE::PMOS_BSIM4:           return { DEVICE_T::PMOS,   "BSIM4",          "BSIM4"                       };
  //case TYPE::NMOS_EKV2_6:          return {};
  //case TYPE::PMOS_EKV2_6:          return {};
  //case TYPE::NMOS_PSP:             return {};
  //case TYPE::PMOS_PSP:             return {};
    case TYPE::NMOS_B3SOIFD:         return { DEVICE_T::NMOS,   "B3SOIFD",        "B3SOIFD (BSIM3 FD-SOI)"      };
    case TYPE::PMOS_B3SOIFD:         return { DEVICE_T::PMOS,   "B3SOIFD",        "B3SOIFD (BSIM3 FD-SOI)"      };
    case TYPE::NMOS_B3SOIDD:         return { DEVICE_T::NMOS,   "B3SOIDD",        "B3SOIDD (BSIM3 SOI)"         };
    case TYPE::PMOS_B3SOIDD:         return { DEVICE_T::PMOS,   "B3SOIDD",        "B3SOIDD (BSIM3 SOI)"         };
    case TYPE::NMOS_B3SOIPD:         return { DEVICE_T::NMOS,   "B3SOIPD",        "B3SOIPD (BSIM3 PD-SOI)"      };
    case TYPE::PMOS_B3SOIPD:         return { DEVICE_T::PMOS,   "B3SOIPD",        "B3SOIPD (BSIM3 PD-SOI)"      };
  //case TYPE::NMOS_STAG:            return {};
  //case TYPE::PMOS_STAG:            return {};
    case TYPE::NMOS_HISIM2:          return { DEVICE_T::NMOS,   "HISIM2",         "HiSIM2"                      };
    case TYPE::PMOS_HISIM2:          return { DEVICE_T::PMOS,   "HISIM2",         "HiSIM2"                      };
    case TYPE::NMOS_HISIMHV1:        return { DEVICE_T::NMOS,   "HISIMHV1",       "HiSIM_HV1"                   };
    case TYPE::PMOS_HISIMHV1:        return { DEVICE_T::PMOS,   "HISIMHV1",       "HiSIM_HV1"                   };
    case TYPE::NMOS_HISIMHV2:        return { DEVICE_T::NMOS,   "HISIMHV2",       "HiSIM_HV2"                   };
    case TYPE::PMOS_HISIMHV2:        return { DEVICE_T::PMOS,   "HISIMHV2",       "HiSIM_HV2"                   };

    case TYPE::V:                    return { DEVICE_T::V,      "DC",             "DC",                         };
    case TYPE::V_SIN:                return { DEVICE_T::V,      "SIN",            "Sine"                        };
    case TYPE::V_PULSE:              return { DEVICE_T::V,      "PULSE",          "Pulse"                       };
    case TYPE::V_EXP:                return { DEVICE_T::V,      "EXP",            "Exponential"                 };
    case TYPE::V_AM:                 return { DEVICE_T::V,      "AM",             "Amplitude modulated"         };
    case TYPE::V_SFFM:               return { DEVICE_T::V,      "SFFM",           "Single-frequency FM"         };
    case TYPE::V_VCL:                return { DEVICE_T::E,      "",               "Voltage-controlled"          };
    case TYPE::V_CCL:                return { DEVICE_T::H,      "",               "Current-controlled"          };
    case TYPE::V_PWL:                return { DEVICE_T::V,      "PWL",            "Piecewise linear"            };
    case TYPE::V_WHITENOISE:         return { DEVICE_T::V,      "WHITENOISE",     "White noise"                 };
    case TYPE::V_PINKNOISE:          return { DEVICE_T::V,      "PINKNOISE",      "Pink noise (1/f)"            };
    case TYPE::V_BURSTNOISE:         return { DEVICE_T::V,      "BURSTNOISE",     "Burst noise"                 };
    case TYPE::V_RANDUNIFORM:        return { DEVICE_T::V,      "RANDUNIFORM",    "Random uniform"              };
    case TYPE::V_RANDGAUSSIAN:       return { DEVICE_T::V,      "RANDGAUSSIAN",   "Random Gaussian"             };
    case TYPE::V_RANDEXP:            return { DEVICE_T::V,      "RANDEXP",        "Random exponential"          };
    case TYPE::V_RANDPOISSON:        return { DEVICE_T::V,      "RANDPOISSON",    "Random Poisson"              };
    case TYPE::V_BEHAVIORAL:         return { DEVICE_T::V,      "=",              "Behavioral"                  };

    case TYPE::I:                    return { DEVICE_T::I,      "DC",             "DC",                         };
    case TYPE::I_SIN:                return { DEVICE_T::I,      "SIN",            "Sine"                        };
    case TYPE::I_PULSE:              return { DEVICE_T::I,      "PULSE",          "Pulse"                       };
    case TYPE::I_EXP:                return { DEVICE_T::I,      "EXP",            "Exponential"                 };
    case TYPE::I_AM:                 return { DEVICE_T::I,      "AM",             "Amplitude modulated"         };
    case TYPE::I_SFFM:               return { DEVICE_T::I,      "SFFM",           "Single-frequency FM"         };
    case TYPE::I_VCL:                return { DEVICE_T::G,      "",               "Voltage-controlled"          };
    case TYPE::I_CCL:                return { DEVICE_T::F,      "",               "Current-controlled"          };
    case TYPE::I_PWL:                return { DEVICE_T::I,      "PWL",            "Piecewise linear"            };
    case TYPE::I_WHITENOISE:         return { DEVICE_T::I,      "WHITENOISE",     "White noise"                 };
    case TYPE::I_PINKNOISE:          return { DEVICE_T::I,      "PINKNOISE",      "Pink noise (1/f)"            };
    case TYPE::I_BURSTNOISE:         return { DEVICE_T::I,      "BURSTNOISE",     "Burst noise"                 };
    case TYPE::I_RANDUNIFORM:        return { DEVICE_T::I,      "RANDUNIFORM",    "Random uniform"              };
    case TYPE::I_RANDGAUSSIAN:       return { DEVICE_T::I,      "RANDGAUSSIAN",   "Random Gaussian"             };
    case TYPE::I_RANDEXP:            return { DEVICE_T::I,      "RANDEXP",        "Random exponential"          };
    case TYPE::I_RANDPOISSON:        return { DEVICE_T::I,      "RANDPOISSON",    "Random Poisson"              };
    case TYPE::I_BEHAVIORAL:         return { DEVICE_T::I,      "=",              "Behavioral"                  };

    case TYPE::SUBCKT:               return { DEVICE_T::SUBCKT, "",               "Subcircuit"                  };
    case TYPE::XSPICE:               return { DEVICE_T::XSPICE, "",               ""                            };

    case TYPE::KIBIS_DEVICE:         return { DEVICE_T::KIBIS,  "DEVICE",         "Device"                      };
    case TYPE::KIBIS_DRIVER_DC:      return { DEVICE_T::KIBIS,  "DCDRIVER",       "DC driver"                   };
    case TYPE::KIBIS_DRIVER_RECT:    return { DEVICE_T::KIBIS,  "RECTDRIVER",     "Rectangular wave driver"     };
    case TYPE::KIBIS_DRIVER_PRBS:    return { DEVICE_T::KIBIS,  "PRBSDRIVER",     "PRBS driver"                 };

    case TYPE::RAWSPICE:             return { DEVICE_T::SPICE,  "",               ""                            };

    default:       wxFAIL;           return {};
    }
}


SIM_MODEL::SPICE_INFO SIM_MODEL::SpiceInfo( TYPE aType )
{
    switch( aType )
    {
        //                              | itemType | modelType | fnName    | level |isDefaultLvl|hasExpr|version|
        //                              -------------------------------------------------------------------------
    case TYPE::R:                    return { "R",  ""                                                           };
    case TYPE::R_POT:                return { "A",  ""                                                           };
    case TYPE::R_BEHAVIORAL:         return { "R",  "",            "",        "0",    false,     true            };

    case TYPE::C:                    return { "C",  ""                                                           };
    case TYPE::C_BEHAVIORAL:         return { "C",  "",            "",        "0",    false,     true            };

    case TYPE::L:                    return { "L",  ""                                                           };
    case TYPE::L_BEHAVIORAL:         return { "L",  "",            "",        "0",    false,     true            };

    case TYPE::K:                    return { "K",  ""                                                           };

    //case TYPE::TLINE_Z0:           return { "T"  };
    case TYPE::TLINE_Z0:             return { "O",  "LTRA"                                                       };
    case TYPE::TLINE_RLGC:           return { "O",  "LTRA"                                                       };

    case TYPE::SW_V:                 return { "S",  "SW"                                                         };
    case TYPE::SW_I:                 return { "W",  "CSW"                                                        };

    case TYPE::D:                    return { "D",  "D"                                                          };

    case TYPE::NPN_VBIC:             return { "Q",  "NPN",         "",        "4"                                };
    case TYPE::PNP_VBIC:             return { "Q",  "PNP",         "",        "4"                                };
    case TYPE::NPN_GUMMELPOON:       return { "Q",  "NPN",         "",        "1",    true                       };
    case TYPE::PNP_GUMMELPOON:       return { "Q",  "PNP",         "",        "1",    true                       };
    case TYPE::NPN_HICUM2:           return { "Q",  "NPN",         "",        "8"                                };
    case TYPE::PNP_HICUM2:           return { "Q",  "PNP",         "",        "8"                                };

    case TYPE::NJFET_SHICHMANHODGES: return { "J",  "NJF",         "",        "1",    true                       };
    case TYPE::PJFET_SHICHMANHODGES: return { "J",  "PJF",         "",        "1",    true                       };
    case TYPE::NJFET_PARKERSKELLERN: return { "J",  "NJF",         "",        "2"                                };
    case TYPE::PJFET_PARKERSKELLERN: return { "J",  "PJF",         "",        "2"                                };

    case TYPE::NMES_STATZ:           return { "Z",  "NMF",         "",        "1",    true                       };
    case TYPE::PMES_STATZ:           return { "Z",  "PMF",         "",        "1",    true                       };
    case TYPE::NMES_YTTERDAL:        return { "Z",  "NMF",         "",        "2"                                };
    case TYPE::PMES_YTTERDAL:        return { "Z",  "PMF",         "",        "2"                                };
    case TYPE::NMES_HFET1:           return { "Z",  "NMF",         "",        "5"                                };
    case TYPE::PMES_HFET1:           return { "Z",  "PMF",         "",        "5"                                };
    case TYPE::NMES_HFET2:           return { "Z",  "NMF",         "",        "6"                                };
    case TYPE::PMES_HFET2:           return { "Z",  "PMF",         "",        "6"                                };

    case TYPE::NMOS_VDMOS:           return { "M",  "VDMOS NCHAN"                                                };
    case TYPE::PMOS_VDMOS:           return { "M",  "VDMOS PCHAN"                                                };
    case TYPE::NMOS_MOS1:            return { "M",  "NMOS",        "",        "1",    true                       };
    case TYPE::PMOS_MOS1:            return { "M",  "PMOS",        "",        "1",    true                       };
    case TYPE::NMOS_MOS2:            return { "M",  "NMOS",        "",        "2"                                };
    case TYPE::PMOS_MOS2:            return { "M",  "PMOS",        "",        "2"                                };
    case TYPE::NMOS_MOS3:            return { "M",  "NMOS",        "",        "3"                                };
    case TYPE::PMOS_MOS3:            return { "M",  "PMOS",        "",        "3"                                };
    case TYPE::NMOS_BSIM1:           return { "M",  "NMOS",        "",        "4"                                };
    case TYPE::PMOS_BSIM1:           return { "M",  "PMOS",        "",        "4"                                };
    case TYPE::NMOS_BSIM2:           return { "M",  "NMOS",        "",        "5"                                };
    case TYPE::PMOS_BSIM2:           return { "M",  "PMOS",        "",        "5"                                };
    case TYPE::NMOS_MOS6:            return { "M",  "NMOS",        "",        "6"                                };
    case TYPE::PMOS_MOS6:            return { "M",  "PMOS",        "",        "6"                                };
    case TYPE::NMOS_BSIM3:           return { "M",  "NMOS",        "",        "8"                                };
    case TYPE::PMOS_BSIM3:           return { "M",  "PMOS",        "",        "8"                                };
    case TYPE::NMOS_MOS9:            return { "M",  "NMOS",        "",        "9"                                };
    case TYPE::PMOS_MOS9:            return { "M",  "PMOS",        "",        "9"                                };
    case TYPE::NMOS_B4SOI:           return { "M",  "NMOS",        "",        "10"                               };
    case TYPE::PMOS_B4SOI:           return { "M",  "PMOS",        "",        "10"                               };
    case TYPE::NMOS_BSIM4:           return { "M",  "NMOS",        "",        "14"                               };
    case TYPE::PMOS_BSIM4:           return { "M",  "PMOS",        "",        "14"                               };
  //case TYPE::NMOS_EKV2_6:          return {};
  //case TYPE::PMOS_EKV2_6:          return {};
  //case TYPE::NMOS_PSP:             return {};
  //case TYPE::PMOS_PSP:             return {};
    case TYPE::NMOS_B3SOIFD:         return { "M",  "NMOS",        "",        "55"                               };
    case TYPE::PMOS_B3SOIFD:         return { "M",  "PMOS",        "",        "55"                               };
    case TYPE::NMOS_B3SOIDD:         return { "M",  "NMOS",        "",        "56"                               };
    case TYPE::PMOS_B3SOIDD:         return { "M",  "PMOS",        "",        "56"                               };
    case TYPE::NMOS_B3SOIPD:         return { "M",  "NMOS",        "",        "57"                               };
    case TYPE::PMOS_B3SOIPD:         return { "M",  "PMOS",        "",        "57"                               };
  //case TYPE::NMOS_STAG:            return {};
  //case TYPE::PMOS_STAG:            return {};
    case TYPE::NMOS_HISIM2:          return { "M",  "NMOS",        "",        "68"                               };
    case TYPE::PMOS_HISIM2:          return { "M",  "PMOS",        "",        "68"                               };
    case TYPE::NMOS_HISIMHV1:        return { "M",  "NMOS",        "",        "73",   false,     false,  "1.2.4" };
    case TYPE::PMOS_HISIMHV1:        return { "M",  "PMOS",        "",        "73",   false,     false,  "1.2.4" };
    case TYPE::NMOS_HISIMHV2:        return { "M",  "NMOS",        "",        "73",   false,     false,  "2.2.0" };
    case TYPE::PMOS_HISIMHV2:        return { "M",  "PMOS",        "",        "73",   false,     false,  "2.2.0" };

    case TYPE::V:                    return { "V",  "",            "DC"                                          };
    case TYPE::V_SIN:                return { "V",  "",            "SIN"                                         };
    case TYPE::V_PULSE:              return { "V",  "",            "PULSE"                                       };
    case TYPE::V_EXP:                return { "V",  "",            "EXP"                                         };
    case TYPE::V_AM:                 return { "V",  "",            "AM"                                          };
    case TYPE::V_SFFM:               return { "V",  "",            "SFFM"                                        };
    case TYPE::V_VCL:                return { "E",  "",            ""                                            };
    case TYPE::V_CCL:                return { "H",  "",            ""                                            };
    case TYPE::V_PWL:                return { "V",  "",            "PWL"                                         };
    case TYPE::V_WHITENOISE:         return { "V",  "",            "TRNOISE"                                     };
    case TYPE::V_PINKNOISE:          return { "V",  "",            "TRNOISE"                                     };
    case TYPE::V_BURSTNOISE:         return { "V",  "",            "TRNOISE"                                     };
    case TYPE::V_RANDUNIFORM:        return { "V",  "",            "TRRANDOM"                                    };
    case TYPE::V_RANDGAUSSIAN:       return { "V",  "",            "TRRANDOM"                                    };
    case TYPE::V_RANDEXP:            return { "V",  "",            "TRRANDOM"                                    };
    case TYPE::V_RANDPOISSON:        return { "V",  "",            "TRRANDOM"                                    };
    case TYPE::V_BEHAVIORAL:         return { "B"                                                                };

    case TYPE::I:                    return { "I",  "",            "DC"                                          };
    case TYPE::I_PULSE:              return { "I",  "",            "PULSE"                                       };
    case TYPE::I_SIN:                return { "I",  "",            "SIN"                                         };
    case TYPE::I_EXP:                return { "I",  "",            "EXP"                                         };
    case TYPE::I_AM:                 return { "I",  "",            "AM"                                          };
    case TYPE::I_SFFM:               return { "I",  "",            "SFFM"                                        };
    case TYPE::I_VCL:                return { "G",  "",            ""                                            };
    case TYPE::I_CCL:                return { "F",  "",            ""                                            };
    case TYPE::I_PWL:                return { "I",  "",            "PWL"                                         };
    case TYPE::I_WHITENOISE:         return { "I",  "",            "TRNOISE"                                     };
    case TYPE::I_PINKNOISE:          return { "I",  "",            "TRNOISE"                                     };
    case TYPE::I_BURSTNOISE:         return { "I",  "",            "TRNOISE"                                     };
    case TYPE::I_RANDUNIFORM:        return { "I",  "",            "TRRANDOM"                                    };
    case TYPE::I_RANDGAUSSIAN:       return { "I",  "",            "TRRANDOM"                                    };
    case TYPE::I_RANDEXP:            return { "I",  "",            "TRRANDOM"                                    };
    case TYPE::I_RANDPOISSON:        return { "I",  "",            "TRRANDOM"                                    };
    case TYPE::I_BEHAVIORAL:         return { "B"                                                                };

    case TYPE::SUBCKT:               return { "X"                                                                };
    case TYPE::XSPICE:               return { "A"                                                                };

    case TYPE::KIBIS_DEVICE:         return { "X"                                                                };
    case TYPE::KIBIS_DRIVER_DC:      return { "X"                                                                };
    case TYPE::KIBIS_DRIVER_RECT:    return { "X"                                                                };
    case TYPE::KIBIS_DRIVER_PRBS:    return { "X"                                                                };

    case TYPE::NONE:
    case TYPE::RAWSPICE:             return {};

    default:       wxFAIL;           return {};
    }
}


TYPE SIM_MODEL::ReadTypeFromFields( const std::vector<SCH_FIELD>& aFields, bool aResolve, int aDepth,
                                    REPORTER& aReporter )
{
    std::string deviceTypeFieldValue = GetFieldValue( &aFields, SIM_DEVICE_FIELD, aResolve, aDepth );
    std::string typeFieldValue = GetFieldValue( &aFields, SIM_DEVICE_SUBTYPE_FIELD, aResolve, aDepth );

    if( !deviceTypeFieldValue.empty() )
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

    if( typeFieldValue.empty() )
        return TYPE::NONE;

    wxString reference = GetFieldValue( &aFields, FIELD_T::REFERENCE );

    if( !reference.IsEmpty() )
    {
        aReporter.Report( wxString::Format( _( "No simulation model definition found for "
                                               "symbol '%s'." ),
                          reference ),
                          RPT_SEVERITY_ERROR );
    }
    else
    {
        aReporter.Report( _( "No simulation model definition found." ),
                          RPT_SEVERITY_ERROR );
    }

    return TYPE::NONE;
}


void SIM_MODEL::ReadDataFields( const std::vector<SCH_FIELD>* aFields, bool aResolve, int aDepth,
                                const std::vector<SCH_PIN*>& aPins )
{
    bool diffMode = GetFieldValue( aFields, SIM_LIBRARY_IBIS::DIFF_FIELD, aResolve, aDepth ) == "1";
    SwitchSingleEndedDiff( diffMode );

    m_serializer->ParseEnable( GetFieldValue( aFields, SIM_LEGACY_ENABLE_FIELD_V7, aResolve, aDepth ) );

    createPins( aPins );
    m_serializer->ParsePins( GetFieldValue( aFields, SIM_PINS_FIELD, aResolve, aDepth ) );

    std::string paramsField = GetFieldValue( aFields, SIM_PARAMS_FIELD, aResolve, aDepth );

    if( !m_serializer->ParseParams( paramsField ) )
        m_serializer->ParseValue( GetFieldValue( aFields, SIM_VALUE_FIELD, aResolve, aDepth ) );
}


void SIM_MODEL::WriteFields( std::vector<SCH_FIELD>& aFields ) const
{
    // Remove duplicate fields: they are at the end of list
    for( size_t ii = aFields.size() - 1; ii > 0; ii-- )
    {
        wxString currFieldName = aFields[ii].GetName();

        auto end_candidate_list = aFields.begin() + ii - 1;

        auto fieldIt = std::find_if( aFields.begin(), end_candidate_list,
                                     [&]( const SCH_FIELD& f )
                                     {
                                        return f.GetName() == currFieldName;
                                     } );

        // If duplicate field found: remove current checked item
        if( fieldIt != end_candidate_list )
            aFields.erase( aFields.begin() + ii );
    }

    SetFieldValue( aFields, SIM_DEVICE_FIELD, m_serializer->GenerateDevice(), false );
    SetFieldValue( aFields, SIM_DEVICE_SUBTYPE_FIELD, m_serializer->GenerateDeviceSubtype(),
                   false );

    SetFieldValue( aFields, SIM_LEGACY_ENABLE_FIELD_V7, m_serializer->GenerateEnable(), false );
    SetFieldValue( aFields, SIM_PINS_FIELD, m_serializer->GeneratePins(), false );

    SetFieldValue( aFields, SIM_PARAMS_FIELD, m_serializer->GenerateParams(), false );

    if( IsStoredInValue() )
        SetFieldValue( aFields, SIM_VALUE_FIELD, m_serializer->GenerateValue(), false );
}


std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( TYPE aType, const std::vector<SCH_PIN*>& aPins,
                                              REPORTER& aReporter )
{
    std::unique_ptr<SIM_MODEL> model = Create( aType );

    try
    {
        // Passing nullptr to ReadDataFields will make it act as if all fields were empty.
        model->ReadDataFields( static_cast<const std::vector<SCH_FIELD>*>( nullptr ),
                               false, 0, aPins );
    }
    catch( IO_ERROR& )
    {
        wxFAIL_MSG( "Shouldn't throw reading empty fields!" );
    }

    return model;
}


std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const SIM_MODEL* aBaseModel,
                                              const std::vector<SCH_PIN*>& aPins,
                                              REPORTER& aReporter )
{
    std::unique_ptr<SIM_MODEL> model;

    if( aBaseModel )
    {
        TYPE type = aBaseModel->GetType();

        if( dynamic_cast<const SIM_MODEL_SPICE_FALLBACK*>( aBaseModel ) )
            model = std::make_unique<SIM_MODEL_SPICE_FALLBACK>( type );
        else if( dynamic_cast< const SIM_MODEL_RAW_SPICE*>( aBaseModel ) )
            model = std::make_unique<SIM_MODEL_RAW_SPICE>();
        else
            model = Create( type );

        model->SetBaseModel( *aBaseModel );
    }
    else  // No base model means the model wasn't found in the library, so create a fallback
    {
        model = std::make_unique<SIM_MODEL_SPICE_FALLBACK>( TYPE::NONE );
    }

    try
    {
        model->ReadDataFields( static_cast<const std::vector<SCH_FIELD>*>( nullptr ),
                               false, 0, aPins );
    }
    catch( IO_ERROR& )
    {
        wxFAIL_MSG( "Shouldn't throw reading empty fields!" );
    }

    return model;
}


std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const SIM_MODEL* aBaseModel,
                                              const std::vector<SCH_PIN*>& aPins,
                                              const std::vector<SCH_FIELD>& aFields,
                                              bool aResolve, int aDepth, REPORTER& aReporter )
{
    std::unique_ptr<SIM_MODEL> model;

    if( aBaseModel )
    {
        NULL_REPORTER devnull;
        TYPE          type = aBaseModel->GetType();
        TYPE          type_override = ReadTypeFromFields( aFields, aResolve, aDepth, devnull );

        // Check for an override in the case of IBIS models.
        // The other models require type to be set from the base model.
        if( dynamic_cast<const SIM_MODEL_IBIS*>( aBaseModel ) &&
            type_override != TYPE::NONE )
        {
            type = type_override;
        }

        if( dynamic_cast<const SIM_MODEL_SPICE_FALLBACK*>( aBaseModel ) )
            model = std::make_unique<SIM_MODEL_SPICE_FALLBACK>( type );
        else if( dynamic_cast< const SIM_MODEL_RAW_SPICE*>( aBaseModel ) )
            model = std::make_unique<SIM_MODEL_RAW_SPICE>();
        else
            model = Create( type );

        model->SetBaseModel( *aBaseModel );
    }
    else  // No base model means the model wasn't found in the library, so create a fallback
    {
        TYPE type = ReadTypeFromFields( aFields, aResolve, aDepth, aReporter );
        model = std::make_unique<SIM_MODEL_SPICE_FALLBACK>( type );
    }

    try
    {
        model->ReadDataFields( &aFields, aResolve, aDepth, aPins );
    }
    catch( IO_ERROR& err )
    {
        aReporter.Report( wxString::Format( _( "Error reading simulation model from symbol '%s':\n%s" ),
                                            GetFieldValue( &aFields, FIELD_T::REFERENCE ),
                                            err.Problem() ),
                          RPT_SEVERITY_ERROR );
    }

    return model;
}


std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const std::vector<SCH_FIELD>& aFields,
                                              bool aResolve, int aDepth,
                                              const std::vector<SCH_PIN*>& aPins, REPORTER& aReporter )
{
    TYPE type = ReadTypeFromFields( aFields, aResolve, aDepth, aReporter );
    std::unique_ptr<SIM_MODEL> model = SIM_MODEL::Create( type );

    try
    {
        model->ReadDataFields( &aFields, aResolve, aDepth, aPins );
    }
    catch( const IO_ERROR& parse_err )
    {
        if( !aResolve )
        {
            aReporter.Report( parse_err.What(), RPT_SEVERITY_ERROR );
            return model;
        }

        // Just because we can't parse it doesn't mean that a SPICE interpreter can't.  Fall
        // back to a raw spice code model.

        std::string modelData = GetFieldValue( &aFields, SIM_PARAMS_FIELD, aResolve, aDepth );

        if( modelData.empty() )
            modelData = GetFieldValue( &aFields, SIM_VALUE_FIELD, aResolve, aDepth );

        model = std::make_unique<SIM_MODEL_RAW_SPICE>( modelData );

        try
        {
            model->createPins( aPins );
            model->m_serializer->ParsePins( GetFieldValue( &aFields, SIM_PINS_FIELD, aResolve, aDepth ) );
        }
        catch( const IO_ERROR& err )
        {
            // We own the pin syntax, so if we can't parse it then there's an error.
            aReporter.Report( wxString::Format( _( "Error reading simulation model from symbol '%s':\n%s" ),
                                                GetFieldValue( &aFields, FIELD_T::REFERENCE ),
                                                err.Problem() ),
                              RPT_SEVERITY_ERROR );
        }
    }

    return model;
}


SIM_MODEL::~SIM_MODEL() = default;


void SIM_MODEL::AddPin( const SIM_MODEL_PIN& aPin )
{
    m_modelPins.push_back( aPin );
}


void SIM_MODEL::ClearPins()
{
    m_modelPins.clear();
}


int SIM_MODEL::FindModelPinIndex( const std::string& aSymbolPinNumber )
{
    for( int modelPinIndex = 0; modelPinIndex < GetPinCount(); ++modelPinIndex )
    {
        if( GetPin( modelPinIndex ).symbolPinNumber == aSymbolPinNumber )
            return modelPinIndex;
    }

    return SIM_MODEL_PIN::NOT_CONNECTED;
}


void SIM_MODEL::AddParam( const PARAM::INFO& aInfo )
{
    m_params.emplace_back( aInfo );

    // Enums are initialized with their default values.
    if( aInfo.enumValues.size() >= 1 )
        m_params.back().value = aInfo.defaultValue;
}


void SIM_MODEL::SetBaseModel( const SIM_MODEL& aBaseModel )
{
    wxASSERT_MSG( GetType() == aBaseModel.GetType(),
                  wxS( "Simulation model type must be the same as its base class!" ) );

    m_baseModel = &aBaseModel;
}


std::vector<std::reference_wrapper<const SIM_MODEL_PIN>> SIM_MODEL::GetPins() const
{
    std::vector<std::reference_wrapper<const SIM_MODEL_PIN>> pins;

    for( int modelPinIndex = 0; modelPinIndex < GetPinCount(); ++modelPinIndex )
        pins.emplace_back( GetPin( modelPinIndex ) );

    return pins;
}

void SIM_MODEL::AssignSymbolPinNumberToModelPin( int aModelPinIndex,
                                                 const wxString& aSymbolPinNumber )
{
    if( aModelPinIndex >= 0 && aModelPinIndex < (int) m_modelPins.size() )
        m_modelPins.at( aModelPinIndex ).symbolPinNumber = aSymbolPinNumber;
}


void SIM_MODEL::AssignSymbolPinNumberToModelPin( const std::string& aModelPinName,
                                                 const wxString& aSymbolPinNumber )
{
    for( SIM_MODEL_PIN& pin : m_modelPins )
    {
        if( pin.modelPinName == aModelPinName )
        {
            pin.symbolPinNumber = aSymbolPinNumber;
            return;
        }
    }

    // If aPinName wasn't in fact a name, see if it's a raw (1-based) index.  This is required
    // for legacy files which didn't use pin names.
    int pinIndex = (int) strtol( aModelPinName.c_str(), nullptr, 10 );

    if( pinIndex < 1 || pinIndex > (int) m_modelPins.size() )
        THROW_IO_ERROR( wxString::Format( _( "Unknown simulation model pin '%s'" ), aModelPinName ) );

    m_modelPins[ --pinIndex /* convert to 0-based */ ].symbolPinNumber = aSymbolPinNumber;
}


const SIM_MODEL::PARAM& SIM_MODEL::GetParam( unsigned aParamIndex ) const
{
    if( m_baseModel && m_params.at( aParamIndex ).value == "" )
        return m_baseModel->GetParam( aParamIndex );
    else
        return m_params.at( aParamIndex );
}


bool SIM_MODEL::PARAM::INFO::Matches( const std::string& aParamName ) const
{
    return boost::iequals( name, aParamName );
}


int SIM_MODEL::doFindParam( const std::string& aParamName ) const
{
    for( int ii = 0; ii < (int) GetParamCount(); ++ii )
    {
        if( GetParam( ii ).Matches( aParamName ) )
            return ii;
    }

    return -1;
}


const SIM_MODEL::PARAM* SIM_MODEL::FindParam( const std::string& aParamName ) const
{
    int idx = doFindParam( aParamName );

    return idx >= 0 ? &GetParam( idx ) : nullptr;
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


void SIM_MODEL::doSetParamValue( int aParamIndex, const std::string& aValue )
{
    m_params.at( aParamIndex ).value = aValue;
}


void SIM_MODEL::SetParamValue( int aParamIndex, const std::string& aValue,
                               SIM_VALUE::NOTATION aNotation )
{
    // Notation conversion is very slow.  Avoid if possible.

    auto plainNumber =
            []( const std::string& aString )
            {
                for( char c : aString )
                {
                    if( c != '.' && ( c < '0' || c > '9' )  )
                        return false;
                }

                return true;
            };


    if( aValue.find( ',' ) != std::string::npos )
    {
        doSetParamValue( aParamIndex, SIM_VALUE::ConvertNotation( aValue, aNotation,
                                                                  SIM_VALUE::NOTATION::SI ) );
    }
    else if( aNotation != SIM_VALUE::NOTATION::SI && !plainNumber( aValue ) )
    {
        doSetParamValue( aParamIndex, SIM_VALUE::ConvertNotation( aValue, aNotation,
                                                                  SIM_VALUE::NOTATION::SI ) );
    }
    else
    {
        doSetParamValue( aParamIndex, aValue );
    }
}


void SIM_MODEL::SetParamValue( const std::string& aParamName, const std::string& aValue,
                               SIM_VALUE::NOTATION aNotation )
{
    int idx = doFindParam( aParamName );

    if( idx < 0 )
        THROW_IO_ERROR( wxString::Format( "Unknown simulation model parameter '%s'", aParamName ) );

    SetParamValue( idx, aValue, aNotation );
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

    case TYPE::K:
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
    case TYPE::V_AM:
    case TYPE::I_AM:
    case TYPE::V_SFFM:
    case TYPE::I_SFFM:
    case TYPE::V_VCL:
    case TYPE::V_CCL:
    case TYPE::V_PWL:
    case TYPE::I_VCL:
    case TYPE::I_CCL:
    case TYPE::I_PWL:
    case TYPE::V_WHITENOISE:
    case TYPE::I_WHITENOISE:
    case TYPE::V_PINKNOISE:
    case TYPE::I_PINKNOISE:
    case TYPE::V_BURSTNOISE:
    case TYPE::I_BURSTNOISE:
    case TYPE::V_RANDUNIFORM:
    case TYPE::I_RANDUNIFORM:
    case TYPE::V_RANDGAUSSIAN:
    case TYPE::I_RANDGAUSSIAN:
    case TYPE::V_RANDEXP:
    case TYPE::I_RANDEXP:
    case TYPE::V_RANDPOISSON:
    case TYPE::I_RANDPOISSON:
        return std::make_unique<SIM_MODEL_SOURCE>( aType );

    case TYPE::SUBCKT:
        return std::make_unique<SIM_MODEL_SUBCKT>();

    case TYPE::XSPICE:
        return std::make_unique<SIM_MODEL_XSPICE>( aType );

    case TYPE::KIBIS_DEVICE:
    case TYPE::KIBIS_DRIVER_DC:
    case TYPE::KIBIS_DRIVER_RECT:
    case TYPE::KIBIS_DRIVER_PRBS:
        return std::make_unique<SIM_MODEL_IBIS>( aType );

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


void SIM_MODEL::createPins( const std::vector<SCH_PIN*>& aSymbolPins )
{
    // Default pin sequence: model pins are the same as symbol pins.
    // Excess model pins are set as Not Connected.
    // Note that intentionally nothing is added if `GetPinNames()` returns an empty vector.

    // SIM_MODEL pins must be ordered by symbol pin numbers -- this is assumed by the code that
    // accesses them.

    std::vector<std::string> pinNames = GetPinNames();

    for( unsigned modelPinIndex = 0; modelPinIndex < pinNames.size(); ++modelPinIndex )
    {
        wxString pinName = pinNames[ modelPinIndex ];
        bool     optional = false;

        if( pinName.StartsWith( '<' ) && pinName.EndsWith( '>' ) )
        {
            pinName = pinName.Mid( 1, pinName.Length() - 2 );
            optional = true;
        }

        if( modelPinIndex < aSymbolPins.size() )
        {
            AddPin( { pinNames.at( modelPinIndex ),
                      aSymbolPins[ modelPinIndex ]->GetNumber().ToStdString() } );
        }
        else if( !optional )
        {
            AddPin( { pinNames.at( modelPinIndex ), "" } );
        }
    }
}


bool SIM_MODEL::requiresSpiceModelLine( const SPICE_ITEM& aItem ) const
{
    // SUBCKTs are a single level; there's never a baseModel.
    if( m_type == TYPE::SUBCKT )
        return false;

    // Model must be written if there's no base model or the base model is an internal model
    if( !m_baseModel || aItem.baseModelName == "" )
        return true;

    for( int ii = 0; ii < GetParamCount(); ++ii )
    {
        const PARAM& param = m_params[ii];

        // Instance parameters are written in item lines
        if( param.info.isSpiceInstanceParam )
            continue;

        // Empty parameters are interpreted as default-value
        if ( param.value == "" )
            continue;

        if( const SIM_MODEL* baseModel = dynamic_cast<const SIM_MODEL*>( m_baseModel ) )
        {
            const std::string& baseValue = baseModel->m_params[ii].value;

            if( param.value == baseValue )
                continue;

            // One more check for equivalence, mostly for early 7.0 files which wrote all
            // parameters to the Sim.Params field in normalized format
            if( param.value == SIM_VALUE::Normalize( SIM_VALUE::ToDouble( baseValue ) ) )
                continue;

            // Overrides must be written
            return true;
        }
    }

    return false;
}


template <class T>
bool SIM_MODEL::InferSimModel( T& aSymbol, std::vector<SCH_FIELD>* aFields, bool aResolve, int aDepth,
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
                /// KiCad Spice PEGTL only handles ASCII
                /// Although these two look the same, they are U+03BC and U+00B5
                if( units == wxS( "µ" ) || units == wxS( "μ" ) )
                    return wxS( "u" );

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

                wxChar ambiguousSeparator = '?';
                wxChar thousandsSeparator = '?';
                bool   thousandsSeparatorFound = false;
                wxChar decimalSeparator = '?';
                bool   decimalSeparatorFound = false;
                int    digits = 0;

                for( int ii = (int) mantissa->length() - 1; ii >= 0; --ii )
                {
                    wxChar c = mantissa->GetChar( ii );

                    if( c >= '0' && c <= '9' )
                    {
                        digits += 1;
                    }
                    else if( c == '.' || c == ',' )
                    {
                        if( decimalSeparator != '?' || thousandsSeparator != '?' )
                        {
                            // We've previously found a non-ambiguous separator...

                            if( c == decimalSeparator )
                            {
                                if( thousandsSeparatorFound )
                                    return false;       // decimal before thousands
                                else if( decimalSeparatorFound )
                                    return false;       // more than one decimal
                                else
                                    decimalSeparatorFound = true;
                            }
                            else if( c == thousandsSeparator )
                            {
                                if( digits != 3 )
                                    return false;       // thousands not followed by 3 digits
                                else
                                    thousandsSeparatorFound = true;
                            }
                        }
                        else if( ambiguousSeparator != '?' )
                        {
                            // We've previously found a separator, but we don't know for sure
                            // which...

                            if( c == ambiguousSeparator )
                            {
                                // They both must be thousands separators
                                thousandsSeparator = ambiguousSeparator;
                                thousandsSeparatorFound = true;
                                decimalSeparator = c == '.' ? ',' : '.';
                            }
                            else
                            {
                                // The first must have been a decimal, and this must be a
                                // thousands.
                                decimalSeparator = ambiguousSeparator;
                                decimalSeparatorFound = true;
                                thousandsSeparator = c;
                                thousandsSeparatorFound = true;
                            }
                        }
                        else
                        {
                            // This is the first separator...

                            // If it's preceeded by a '0' (only), or if it's followed by some
                            // number of digits not equal to 3, then it -must- be a decimal
                            // separator.
                            //
                            // In all other cases we don't really know what it is yet.

                            if( ( ii == 1 && mantissa->GetChar( 0 ) == '0' ) || digits != 3 )
                            {
                                decimalSeparator = c;
                                decimalSeparatorFound = true;
                                thousandsSeparator = c == '.' ? ',' : '.';
                            }
                            else
                            {
                                ambiguousSeparator = c;
                            }
                        }

                        digits = 0;
                    }
                    else
                    {
                        digits = 0;
                    }
                }

                // If we found nothing difinitive then we have to assume SPICE-native syntax
                if( decimalSeparator == '?' && thousandsSeparator == '?' )
                {
                    decimalSeparator = '.';
                    thousandsSeparator = ',';
                }

                mantissa->Replace( thousandsSeparator, wxEmptyString );
                mantissa->Replace( decimalSeparator, '.' );

                return true;
            };

    wxString              prefix = aSymbol.GetPrefix();
    wxString              library = GetFieldValue( aFields, SIM_LIBRARY_FIELD, aResolve, aDepth );
    wxString              modelName = GetFieldValue( aFields, SIM_NAME_FIELD, aResolve, aDepth );
    wxString              value = GetFieldValue( aFields, SIM_VALUE_FIELD, aResolve, aDepth );
    std::vector<SCH_PIN*> pins = aSymbol.GetPins();

    // ensure the pins are sorted by number (not guaranteed in the symbol)
    // because the inferred spice model pin assignment here and elsewhere depends on
    // us maintaing the list of pins in order
    std::sort( pins.begin(), pins.end(),
               []( const SCH_PIN* a, const SCH_PIN* b )
               {
                   return a->GetNumber() < b->GetNumber();
               } );

    *aDeviceType = GetFieldValue( aFields, SIM_DEVICE_FIELD, aResolve, aDepth );
    *aModelType = GetFieldValue( aFields, SIM_DEVICE_SUBTYPE_FIELD, aResolve, aDepth );
    *aModelParams = GetFieldValue( aFields, SIM_PARAMS_FIELD, aResolve, aDepth );
    *aPinMap = GetFieldValue( aFields, SIM_PINS_FIELD, aResolve, aDepth );

    if( pins.size() != 2 )
        return false;

    if(   ( ( *aDeviceType == "R" || *aDeviceType == "L" || *aDeviceType == "C" )
            && aModelType->IsEmpty() )
       ||
          ( library.IsEmpty() && modelName.IsEmpty()
            && aDeviceType->IsEmpty()
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
                                          std::move( valueMantissa ),
                                          convertNotation( valueExponent ) );
                }
                else
                {
                    aModelParams->Printf( wxT( "%s=\"%s.%s%s\"" ),
                                          prefix.Left(1).Lower(),
                                          std::move( valueMantissa ),
                                          std::move( valueFraction ),
                                          convertNotation( valueExponent ) );
                }
            }
            else        // Behavioral
            {
                *aModelType = wxT( "=" );
                aModelParams->Printf( wxT( "%s=\"%s\"" ), prefix.Left(1).Lower(), std::move( value ) );
            }
        }

        if( aDeviceType->IsEmpty() )
            *aDeviceType = prefix.Left( 1 );

        if( aPinMap->IsEmpty() )
            aPinMap->Printf( wxT( "%s=+ %s=-" ), pins[0]->GetNumber(), pins[1]->GetNumber() );

        return true;
    }

    if(   ( ( *aDeviceType == wxT( "V" ) || *aDeviceType == wxT( "I" ) )
            && ( aModelType->IsEmpty() || *aModelType == wxT( "DC" ) ) )
       ||
          ( aDeviceType->IsEmpty()
            && aModelType->IsEmpty()
            && !value.IsEmpty()
            && ( prefix.StartsWith( "V" ) || prefix.StartsWith( "I" ) ) )  )
    {
        if( !value.IsEmpty() )
        {
            wxString param = "dc";

            if( value.StartsWith( wxT( "DC " ) ) )
            {
                value = value.Right( value.Length() - 3 );
            }
            else if( value.StartsWith( wxT( "AC " ) ) )
            {
                value = value.Right( value.Length() - 3 );
                param = "ac";
            }

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
                    aModelParams->Printf( wxT( "%s=\"%s%s\" %s" ),
                                          std::move( param ),
                                          std::move( valueMantissa ),
                                          convertNotation( valueExponent ),
                                          *aModelParams );
                }
                else
                {
                    aModelParams->Printf( wxT( "%s=\"%s.%s%s\" %s" ),
                                          std::move( param ),
                                          std::move( valueMantissa ),
                                          std::move( valueFraction ),
                                          convertNotation( valueExponent ),
                                          *aModelParams );
                }
            }
            else
            {
                aModelParams->Printf( wxT( "%s=\"%s\" %s" ),
                                      std::move( param ),
                                      std::move( value ),
                                      *aModelParams );
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


template bool SIM_MODEL::InferSimModel<SCH_SYMBOL>( SCH_SYMBOL& aSymbol, std::vector<SCH_FIELD>* aFields,
                                                    bool aResolve, int aDepth,
                                                    SIM_VALUE_GRAMMAR::NOTATION aNotation,
                                                    wxString* aDeviceType, wxString* aModelType,
                                                    wxString* aModelParams, wxString* aPinMap );
template bool SIM_MODEL::InferSimModel<LIB_SYMBOL>( LIB_SYMBOL& aSymbol, std::vector<SCH_FIELD>* aFields,
                                                    bool aResolve, int aDepth,
                                                    SIM_VALUE_GRAMMAR::NOTATION aNotation,
                                                    wxString* aDeviceType, wxString* aModelType,
                                                    wxString* aModelParams, wxString* aPinMap );


template <typename T>
void SIM_MODEL::MigrateSimModel( T& aSymbol, const PROJECT* aProject )
{
    class FIELD_INFO
    {
    public:
        FIELD_INFO()
        {
            m_Visible = false;
            m_Attributes.m_Size = VECTOR2I( DEFAULT_SIZE_TEXT * schIUScale.IU_PER_MILS,
                                            DEFAULT_SIZE_TEXT * schIUScale.IU_PER_MILS );
        };

        FIELD_INFO( const wxString& aText, SCH_FIELD* aField ) :
                m_Text( aText ),
                m_Visible( aField->IsVisible() ),
                m_Attributes( aField->GetAttributes() ),
                m_Pos( aField->GetPosition() )
        {}

        bool IsEmpty() const { return m_Text.IsEmpty(); }

        SCH_FIELD CreateField( T* aSymbol, const wxString& aFieldName )
        {
            SCH_FIELD field( aSymbol, FIELD_T::USER, aFieldName );

            field.SetText( m_Text );
            field.SetVisible( m_Visible );
            field.SetAttributes( m_Attributes );
            field.SetPosition( m_Pos );

            return field;
        }

    public:
        wxString        m_Text;
        bool            m_Visible;
        TEXT_ATTRIBUTES m_Attributes;
        VECTOR2I        m_Pos;
    };

    SCH_FIELD* existing_deviceField = aSymbol.GetField( SIM_DEVICE_FIELD );
    SCH_FIELD* existing_deviceSubtypeField = aSymbol.GetField( SIM_DEVICE_SUBTYPE_FIELD );
    SCH_FIELD* existing_pinsField = aSymbol.GetField( SIM_PINS_FIELD );
    SCH_FIELD* existing_paramsField = aSymbol.GetField( SIM_PARAMS_FIELD );

    wxString existing_deviceSubtype;

    if( existing_deviceSubtypeField )
        existing_deviceSubtype = existing_deviceSubtypeField->GetShownText( false ).Upper();

    if( existing_deviceField
        || existing_deviceSubtypeField
        || existing_pinsField
        || existing_paramsField )
    {
        // Has a current (V7+) model field.

        // Up until 7.0RC2 we used '+' and '-' for potentiometer pins, which doesn't match
        // SPICE.  Here we remap them to 'r0' and 'r1'.
        if( existing_deviceSubtype == wxS( "POT" ) )
        {
            if( existing_pinsField )
            {
                wxString pinMap = existing_pinsField->GetText();
                pinMap.Replace( wxS( "=+" ), wxS( "=r1" ) );
                pinMap.Replace( wxS( "=-" ), wxS( "=r0" ) );
                existing_pinsField->SetText( pinMap );
            }
        }

        // Up until 8.0RC1 random voltage/current sources were a bit of a mess.
        if( existing_deviceSubtype.StartsWith( wxS( "RAND" ) ) )
        {
            // Re-fetch value without resolving references.  If it's an indirect value then we
            // can't migrate it.
            existing_deviceSubtype = existing_deviceSubtypeField->GetText().Upper();

            if( existing_deviceSubtype.Replace( wxS( "NORMAL" ), wxS( "GAUSSIAN" ) ) )
                existing_deviceSubtypeField->SetText( existing_deviceSubtype );

            if( existing_paramsField )
            {
                wxString params = existing_paramsField->GetText().Lower();
                size_t   count = 0;

                // We used to support 'min' and 'max' instead of 'range' and 'offset', but we
                // wrote all 4 to the netlist which would cause ngspice to barf, so no one has
                // working documents with min and max specified.  Just delete them if they're
                // uninitialized.
                count += params.Replace( wxS( "min=0 " ), wxEmptyString );
                count += params.Replace( wxS( "max=0 " ), wxEmptyString );

                // We used to use 'dt', but the correct ngspice name is 'ts'.
                count += params.Replace( wxS( "dt=" ), wxS( "ts=" ) );

                if( count )
                    existing_paramsField->SetText( params );
            }
        }

        // Up until 8.0.1 we treated a mutual inductance statement as a type of inductor --
        // which is confusing because it doesn't represent a device at all.
        if( existing_deviceSubtype == wxS( "MUTUAL" ) )
        {
            if( existing_deviceSubtypeField )   // Can't be null, but Coverity doesn't know that
                aSymbol.RemoveField( existing_deviceSubtypeField );

            if( existing_deviceField )
            {
                existing_deviceField->SetText( wxS( "K" ) );
            }
            else
            {
                FIELD_INFO deviceFieldInfo;
                deviceFieldInfo.m_Text = wxS( "K" );

                SCH_FIELD deviceField = deviceFieldInfo.CreateField( &aSymbol, SIM_DEVICE_FIELD );
                aSymbol.AddField( deviceField );
            }
        }

        return;
    }

    auto getSIValue =
            []( SCH_FIELD* aField )
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
            []( const std::vector<SCH_PIN*>& sourcePins )
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
    SCH_FIELD*            valueField = aSymbol.GetField( FIELD_T::VALUE );
    std::vector<SCH_PIN*> sourcePins = aSymbol.GetPins();
    bool                  sourcePinsSorted = false;

    auto lazySortSourcePins =
            [&sourcePins, &sourcePinsSorted]()
            {
                if( !sourcePinsSorted )
                {
                    std::sort( sourcePins.begin(), sourcePins.end(),
                               []( const SCH_PIN* lhs, const SCH_PIN* rhs )
                               {
                                   return StrNumCmp( lhs->GetNumber(), rhs->GetNumber(), true ) < 0;
                               } );
                }

                sourcePinsSorted = true;
            };

    FIELD_INFO deviceInfo;
    FIELD_INFO modelInfo;
    FIELD_INFO deviceSubtypeInfo;
    FIELD_INFO libInfo;
    FIELD_INFO spiceParamsInfo;
    FIELD_INFO pinMapInfo;
    bool       modelFromValueField = false;

    if( aSymbol.GetField( SIM_LEGACY_PRIMITIVE_FIELD )
        || aSymbol.GetField( SIM_LEGACY_PINS_FIELD )
        || aSymbol.GetField( SIM_LEGACY_MODEL_FIELD )
        || aSymbol.GetField( SIM_LEGACY_ENABLE_FIELD )
        || aSymbol.GetField( SIM_LEGACY_LIBRARY_FIELD ) )
    {
        if( SCH_FIELD* primitiveField = aSymbol.GetField( SIM_LEGACY_PRIMITIVE_FIELD ) )
        {
            deviceInfo = FIELD_INFO( primitiveField->GetText(), primitiveField );
            aSymbol.RemoveField( primitiveField );
        }

        if( SCH_FIELD* nodeSequenceField = aSymbol.GetField( SIM_LEGACY_PINS_FIELD ) )
        {
            const wxString  delimiters( "{:,; }" );
            const wxString& nodeSequence = nodeSequenceField->GetText();
            wxString        pinMap;

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

            pinMapInfo = FIELD_INFO( pinMap, nodeSequenceField );
            aSymbol.RemoveField( nodeSequenceField );
        }

        if( SCH_FIELD* modelField = aSymbol.GetField( SIM_LEGACY_MODEL_FIELD ) )
        {
            modelInfo = FIELD_INFO( getSIValue( modelField ), modelField );
            aSymbol.RemoveField( modelField );
        }
        else if( valueField )
        {
            modelInfo = FIELD_INFO( getSIValue( valueField ), valueField );
            modelFromValueField = true;
        }

        if( SCH_FIELD* libFileField = aSymbol.GetField( SIM_LEGACY_LIBRARY_FIELD ) )
        {
            libInfo = FIELD_INFO( libFileField->GetText(), libFileField );
            aSymbol.RemoveField( libFileField );
        }
    }
    else
    {
        // Auto convert some legacy fields used in the middle of 7.0 development...

        if( SCH_FIELD* legacyType = aSymbol.GetField( wxT( "Sim_Type" ) ) )
        {
            legacyType->SetName( SIM_DEVICE_SUBTYPE_FIELD );
        }

        if( SCH_FIELD* legacyDevice = aSymbol.GetField( wxT( "Sim_Device" ) ) )
        {
            legacyDevice->SetName( SIM_DEVICE_FIELD );
        }

        if( SCH_FIELD* legacyPins = aSymbol.GetField( wxT( "Sim_Pins" ) ) )
        {
            bool isPassive =   prefix.StartsWith( wxT( "R" ) )
                            || prefix.StartsWith( wxT( "L" ) )
                            || prefix.StartsWith( wxT( "C" ) );

            // Migrate pins from array of indexes to name-value-pairs
            wxString      pinMap;
            wxArrayString pinIndexes;

            wxStringSplit( legacyPins->GetText(), pinIndexes, ' ' );

            lazySortSourcePins();

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
                for( unsigned ii = 0; ii < pinIndexes.size() && ii < sourcePins.size(); ++ii )
                {
                    if( ii > 0 )
                        pinMap.Append( wxS( " " ) );

                    pinMap.Append( wxString::Format( wxT( "%s=%s" ),
                                                     sourcePins[ii]->GetNumber(),
                                                     pinIndexes[ ii ] ) );
                }
            }

            legacyPins->SetName( SIM_PINS_FIELD );
            legacyPins->SetText( pinMap );
        }

        if( SCH_FIELD* legacyParams = aSymbol.GetField( wxT( "Sim_Params" ) ) )
        {
            legacyParams->SetName( SIM_PARAMS_FIELD );
        }

        return;
    }

    wxString device = deviceInfo.m_Text.Trim( true ).Trim( false );
    wxString lib = libInfo.m_Text.Trim( true ).Trim( false );
    wxString model = modelInfo.m_Text.Trim( true ).Trim( false );
    wxString modelLineParams;

    bool libraryModel = false;
    bool inferredModel = false;
    bool internalModel = false;

    if( !lib.IsEmpty() )
    {
        WX_STRING_REPORTER           reporter;
        SIM_LIB_MGR                  libMgr( aProject );
        std::vector<SCH_FIELD>       emptyFields;
        std::vector<EMBEDDED_FILES*> embeddedFilesStack;

        if constexpr (std::is_same_v<T, SCH_SYMBOL>)
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( &aSymbol );
            embeddedFilesStack.push_back( symbol->Schematic()->GetEmbeddedFiles() );
        }

        if( EMBEDDED_FILES* symbolEmbeddedFiles = aSymbol.GetEmbeddedFiles() )
        {
            embeddedFilesStack.push_back( symbolEmbeddedFiles );

            if constexpr (std::is_same_v<T, SCH_SYMBOL>)
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( &aSymbol );
                symbol->GetLibSymbolRef()->AppendParentEmbeddedFiles( embeddedFilesStack );
            }
            else if constexpr (std::is_same_v<T, LIB_SYMBOL>)
            {
                LIB_SYMBOL* symbol = static_cast<LIB_SYMBOL*>( &aSymbol );
                symbol->AppendParentEmbeddedFiles( embeddedFilesStack );
            }
        }

        libMgr.SetFilesStack( std::move( embeddedFilesStack ) );

        // Pull out any following parameters from model name
        model = model.BeforeFirst( ' ', &modelLineParams );
        modelInfo.m_Text = model;

        lazySortSourcePins();

        SIM_LIBRARY::MODEL simModel = libMgr.CreateModel( lib, model.ToStdString(),
                                                          emptyFields, false, 0,
                                                          sourcePins, reporter );

        if( reporter.HasMessage() )
            libraryModel = false;    // Fall back to raw spice model
        else
            libraryModel = true;

        if( pinMapInfo.IsEmpty() )
        {
            // Try to generate a default pin map from the SIM_MODEL's pins; if that fails,
            // generate one from the symbol's pins
            pinMapInfo.m_Text = wxString( simModel.model.Serializer().GeneratePins() );

            if( pinMapInfo.IsEmpty() )
                pinMapInfo.m_Text = generateDefaultPinMapFromSymbol( sourcePins );
        }
    }
    else if( ( device == wxS( "R" )
               || device == wxS( "L" )
               || device == wxS( "C" )
               || device == wxS( "V" )
               || device == wxS( "I" ) )
            && prefix.StartsWith( device )
            && modelFromValueField )
    {
        inferredModel = true;
    }
    else if( device == wxS( "V" ) || device == wxS( "I" ) )
    {
        // See if we have a SPICE time-dependent function such as "sin(0 1 60)" or "sin 0 1 60"
        // that can be handled by a built-in SIM_MODEL_SOURCE.

        wxStringTokenizer tokenizer( model, wxT( "() " ), wxTOKEN_STRTOK );

        if( tokenizer.HasMoreTokens() )
        {
            deviceSubtypeInfo.m_Text = tokenizer.GetNextToken();
            deviceSubtypeInfo.m_Text.MakeUpper();

            for( SIM_MODEL::TYPE type : SIM_MODEL::TYPE_ITERATOR() )
            {
                if( device == SIM_MODEL::SpiceInfo( type ).itemType
                        && deviceSubtypeInfo.m_Text == SIM_MODEL::SpiceInfo( type ).functionName )
                {
                    try
                    {
                        std::unique_ptr<SIM_MODEL> simModel = SIM_MODEL::Create( type );

                        if( deviceSubtypeInfo.m_Text == wxT( "DC" ) && tokenizer.CountTokens() == 1 )
                        {
                            wxCHECK( valueField, /* void */ );
                            valueField->SetText( tokenizer.GetNextToken() );
                            modelFromValueField = false;
                        }
                        else
                        {
                            for( int ii = 0; tokenizer.HasMoreTokens(); ++ii )
                            {
                                simModel->SetParamValue( ii, tokenizer.GetNextToken().ToStdString(),
                                                         SIM_VALUE_GRAMMAR::NOTATION::SPICE );
                            }

                            deviceSubtypeInfo.m_Text = SIM_MODEL::TypeInfo( type ).fieldValue;

                            spiceParamsInfo = modelInfo;
                            spiceParamsInfo.m_Text = wxString( simModel->Serializer().GenerateParams() );
                        }

                        internalModel = true;

                        if( pinMapInfo.IsEmpty() )
                        {
                            lazySortSourcePins();

                            // Generate a default pin map from the SIM_MODEL's pins
                            simModel->createPins( sourcePins );
                            pinMapInfo.m_Text = wxString( simModel->Serializer().GeneratePins() );
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
        SCH_FIELD libField = libInfo.CreateField( &aSymbol, SIM_LIBRARY_FIELD );
        aSymbol.AddField( libField );

        SCH_FIELD nameField = modelInfo.CreateField( &aSymbol, SIM_NAME_FIELD );
        aSymbol.AddField( nameField );

        if( !modelLineParams.IsEmpty() )
        {
            spiceParamsInfo = modelInfo;
            spiceParamsInfo.m_Pos.x += nameField.GetBoundingBox().GetWidth();
            spiceParamsInfo.m_Text = modelLineParams;

            BOX2I nameBBox = nameField.GetBoundingBox();
            int   nameWidth = nameBBox.GetWidth();

            // Add space between model name and additional parameters
            nameWidth += KiROUND( nameBBox.GetHeight() * 1.25 );

            if( nameField.GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
                spiceParamsInfo.m_Pos.x -= nameWidth;
            else
                spiceParamsInfo.m_Pos.x += nameWidth;

            SCH_FIELD paramsField = spiceParamsInfo.CreateField( &aSymbol, SIM_PARAMS_FIELD );
            aSymbol.AddField( paramsField );
        }

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
        SCH_FIELD deviceField = deviceInfo.CreateField( &aSymbol, SIM_DEVICE_FIELD );
        aSymbol.AddField( deviceField );

        if( !deviceSubtypeInfo.m_Text.IsEmpty() )
        {
            SCH_FIELD subtypeField = deviceSubtypeInfo.CreateField( &aSymbol, SIM_DEVICE_SUBTYPE_FIELD );
            aSymbol.AddField( subtypeField );
        }

        if( !spiceParamsInfo.IsEmpty() )
        {
            SCH_FIELD paramsField = spiceParamsInfo.CreateField( &aSymbol, SIM_PARAMS_FIELD );
            aSymbol.AddField( paramsField );
        }

        if( modelFromValueField )
            valueField->SetText( wxT( "${SIM.PARAMS}" ) );
    }
    else    // Insert a raw spice model as a substitute.
    {
        if( device.IsEmpty() && lib.IsEmpty() )
        {
            spiceParamsInfo = modelInfo;
        }
        else
        {
            spiceParamsInfo.m_Text.Printf( wxT( "type=\"%s\" model=\"%s\" lib=\"%s\"" ), device,
                                           model, lib );
        }

        deviceInfo.m_Text = SIM_MODEL::DeviceInfo( SIM_MODEL::DEVICE_T::SPICE ).fieldValue;

        SCH_FIELD deviceField = deviceInfo.CreateField( &aSymbol, SIM_DEVICE_FIELD );
        aSymbol.AddField( deviceField );

        SCH_FIELD paramsField = spiceParamsInfo.CreateField( &aSymbol, SIM_PARAMS_FIELD );
        aSymbol.AddField( paramsField );

        if( modelFromValueField )
        {
            // Get the current Value field, after previous changes.
            valueField = aSymbol.GetField( FIELD_T::VALUE );

            if( valueField )
                valueField->SetText( wxT( "${SIM.PARAMS}" ) );
        }

        // We know nothing about the SPICE model here, so we've got no choice but to generate
        // the default pin map from the symbol's pins.

        if( pinMapInfo.IsEmpty() )
        {
            lazySortSourcePins();
            pinMapInfo.m_Text = generateDefaultPinMapFromSymbol( sourcePins );
        }
    }

    if( !pinMapInfo.IsEmpty() )
    {
        SCH_FIELD pinsField = pinMapInfo.CreateField( &aSymbol, SIM_PINS_FIELD );
        aSymbol.AddField( pinsField );
    }
}


template void SIM_MODEL::MigrateSimModel<SCH_SYMBOL>( SCH_SYMBOL& aSymbol,
                                                      const PROJECT* aProject );
template void SIM_MODEL::MigrateSimModel<LIB_SYMBOL>( LIB_SYMBOL& aSymbol,
                                                      const PROJECT* aProject );

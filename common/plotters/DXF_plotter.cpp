/**
 * @file DXF_plotter.cpp
 * @brief Kicad: specialized plotter for DXF files format
 */
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

#include <ranges>
#include <plotters/plotter_dxf.h>
#include <macros.h>
#include <string_utils.h>
#include <convert_basic_shapes_to_polygon.h>
#include <geometry/shape_rect.h>
#include <trigo.h>
#include <fmt/core.h>
#include <algorithm>

/**
 * Oblique angle for DXF native text
 * (I don't remember if 15 degrees is the ISO value... it looks nice anyway)
 */
static const double DXF_OBLIQUE_ANGLE = 15;

// No support for linewidths in DXF
#define DXF_LINE_WIDTH DO_NOT_SET_LINE_WIDTH

/**
 * The layer/colors palette.
 *
 * The acad/DXF palette is divided in 3 zones:
 *
 *  - The primary colors (1 - 9)
 *  - An HSV zone (10-250, 5 values x 2 saturations x 10 hues
 *  - Greys (251 - 255)

 * There is *no* black... the white does it on paper, usually, and anyway it depends on the
 * plotter configuration, since DXF colors are meant to be logical only (they represent *both*
 * line color and width); later version with plot styles only complicate the matter!
 *
 * As usual, brown and magenta/purple are difficult to place since they are actually variations
 * of other colors.
 */
static const struct
{
    const char* name;
    int         index;
} acad_dxf_color_names[]=
{
    { "BLACK",             250 },
    { "RED",               14  },
    { "YELLOW",            52  },
    { "GREEN",             94  },
    { "CYAN",              134 },
    { "BLUE",              174 },
    { "MAGENTA",           214 },
    { "WHITE",             250 },
    { "BROWN",             54  },
    { "ORANGE",            32  },
    { "LIGHTRED",          12  },
    { "LIGHTYELLOW",       51  },
    { "LIGHTGREEN",        92  },
    { "LIGHTCYAN",         132 },
    { "LIGHTBLUE",         172 },
    { "LIGHTMAGENTA",      212 },
    { "LIGHTORANGE",       30  },
    { "LIGHTGRAY",         9   },
    { "DARKRED",           16  },
    { "DARKYELLOW",        41  },
    { "DARKGREEN",         96  },
    { "DARKCYAN",          136 },
    { "DARKBLUE",          176 },
    { "DARKMAGENTA",       216 },
    { "DARKBROWN",         56  },
    { "DARKORANGE",        34  },
    { "DARKGRAY",          8   },
    { "PURERED",           1   },
    { "PUREYELLOW",        2   },
    { "PUREGREEN",         3   },
    { "PURECYAN",          4   },
    { "PUREBLUE",          5   },
    { "PUREMAGENTA",       6   },
    { "PUREORANGE",        40  },
    { "PUREGRAY",          57  },
    { "REDONE",            11  },
    { "REDTWO",            13  },
    { "REDTHREE",          15  },
    { "REDFOUR",           17  },
    { "REDFIVE",           18  },
    { "REDSIX",            19  },
    { "ORANGEONE",         20  },
    { "ORANGETWO",         21  },
    { "ORANGETHREE",       22  },
    { "ORANGEFOUR",        23  },
    { "ORANGEFIVE",        24  },
    { "REDSEVEN",          25  },
    { "REDEIGHT",          26  },
    { "ORANGESIX",         27  },
    { "REDNINE",           28  },
    { "ORANGESEVEN",       29  },
    { "ORANGEEIGHT",       31  },
    { "ORANGENINE",        33  },
    { "ORANGETEN",         35  },
    { "ORANGEELEVEN",      36  },
    { "ORANGETWELVE",      37  },
    { "ORANGETHIRTEEN",    38  },
    { "ORANGEFOURTEEN",    39  },
    { "YELLOWONE",         42  },
    { "YELLOWTWO",         43  },
    { "YELLOWTHREE",       44  },
    { "YELLOWFOUR",        45  },
    { "YELLOWFIVE",        46  },
    { "YELLOWSIX",         47  },
    { "YELLOWSEVEN",       48  },
    { "YELLOWEIGHT",       49  },
    { "YELLOWNINE",        53  },
    { "YELLOWTEN",         55  },
    { "YELLOWELEVEN",      58  },
    { "YELLOWTWELVE",      59  },
    { "YELLOWTHIRTEEN",    60  },
    { "YELLOWFOURTEEN",    61  },
    { "GREENONE",          62  },
    { "GREENTWO",          63  },
    { "GREENTHREE",        64  },
    { "GREENFOUR",         65  },
    { "GREENFIVE",         66  },
    { "GREENSIX",          67  },
    { "GREENSEVEN",        68  },
    { "GREENEIGHT",        69  },
    { "GREENNINE",         70  },
    { "GREENTEN",          71  },
    { "GREENELEVEN",       72  },
    { "GREENTWELVE",       73  },
    { "GREENTHIRTEEN",     74  },
    { "GREENFOURTEEN",     75  },
    { "GREENFIFTEEN",      76  },
    { "GREENSIXTEEN",      77  },
    { "GREENSEVENTEEN",    78  },
    { "GREENEIGHTEEN",     79  },
    { "GREENNINETEEN",     80  },
    { "GREENTWENTY",       81  },
    { "GREENTWENTYONE",    82  },
    { "GREENTWENTYTWO",    83  },
    { "GREENTWENTYTHREE",  84  },
    { "GREENTWENTYFOUR",   85  },
    { "GREENTWENTYFIVE",   86  },
    { "GREENTWENTYSIX",    87  },
    { "GREENTWENTYSEVEN",  88  },
    { "GREENTWENTYEIGHT",  89  },
    { "GREENTWENTYNINE",   90  },
    { "GREENTHIRTY",       91  },
    { "GREENTHIRTYONE",    93  },
    { "GREENTHIRTYTWO",    95  },
    { "GREENTHIRTYTHREE",  97  },
    { "GREENTHIRTYFOUR",   98  },
    { "GREENTHIRTYFIVE",   99  },
    { "GREENTHIRTYSIX",    100 },
    { "GREENTHIRTYSEVEN",  101 },
    { "GREENTHIRTYEIGHT",  102 },
    { "GREENTHIRTYNINE",   103 },
    { "GREENFORTY",        104 },
    { "GREENFORTYONE",     105 },
    { "GREENFORTYTWO",     106 },
    { "GREENFORTYTHREE",   107 },
    { "GREENFORTYFOUR",    108 },
    { "GREENFORTYFIVE",    109 },
    { "GREENFORTYSIX",     110 },
    { "GREENFORTYSEVEN",   111 },
    { "GREENFORTYEIGHT",   112 },
    { "GREENFORTYNINE",    113 },
    { "GREENFIFTY",        114 },
    { "GREENFIFTYONE",     115 },
    { "GREENFIFTYTWO",     116 },
    { "GREENFIFTYTHREE",   117 },
    { "GREENFIFTYFOUR",    118 },
    { "GREENFIFTYFIVE",    119 },
    { "GREENFIFTYSIX",     120 },
    { "GREENFIFTYSEVEN",   121 },
    { "GREENFIFTYEIGHT",   122 },
    { "GREENFIFTYNINE",    123 },
    { "GREENSIXTY",        124 },
    { "GREENSIXTYONE",     125 },
    { "GREENSIXTYTWO",     126 },
    { "GREENSIXTYTHREE",   127 },
    { "GREENSIXTYFOUR",    128 },
    { "GREENSIXTYFIVE",    129 },
    { "CYANONE",           131 },
    { "CYANTWO",           133 },
    { "CYANTHREE",         135 },
    { "CYANFOUR",          137 },
    { "CYANFIVE",          138 },
    { "CYANSIX",           139 },
    { "CYANSEVEN",         140 },
    { "BLUEONE",           141 },
    { "BLUETWO",           142 },
    { "BLUETHREE",         143 },
    { "BLUEFOUR",          144 },
    { "BLUEFIVE",          145 },
    { "BLUESIX",           146 },
    { "BLUESEVEN",         147 },
    { "BLUEEIGHT",         148 },
    { "BLUENINE",          149 },
    { "BLUETEN",           150 },
    { "BLUEELEVEN",        151 },
    { "BLUETWELVE",        152 },
    { "BLUETHIRTEEN",      153 },
    { "BLUEFOURTEEN",      154 },
    { "BLUEFIFTEEN",       155 },
    { "BLUESIXTEEN",       156 },
    { "BLUESEVENTEEN",     157 },
    { "BLUEEIGHTEEN",      158 },
    { "BLUENINETEEN",      159 },
    { "BLUETWENTY",        160 },
    { "BLUETWENTYONE",     161 },
    { "BLUETWENTYTWO",     162 },
    { "BLUETWENTYTHREE",   163 },
    { "BLUETWENTYFOUR",    164 },
    { "BLUETWENTYFIVE",    165 },
    { "BLUETWENTYSIX",     166 },
    { "BLUETWENTYSEVEN",   167 },
    { "BLUETWENTYEIGHT",   168 },
    { "BLUETWENTYNINE",    169 },
    { "BLUETHIRTY",        170 },
    { "BLUETHIRTYONE",     171 },
    { "BLUETHIRTYTWO",     177 },
    { "BLUETHIRTYETHREE",  178 },
    { "BLUETHIRTYFOUR",    179 },
    { "VIOLETONE",         180 },
    { "VIOLETTWO",         181 },
    { "VIOLETTHREE",       182 },
    { "VIOLETFOUR",        183 },
    { "VIOLETFIVE",        184 },
    { "VIOLETSIX",         185 },
    { "VIOLETSEVEN",       186 },
    { "VIOLETEIGHT",       187 },
    { "VIOLETNINE",        188 },
    { "VIOLETTEN",         189 },
    { "VIOLETELEVEN",      190 },
    { "VIOLETTWELVE",      191 },
    { "VIOLETTHIRTEEN",    192 },
    { "VIOLETFOURTEEN",    193 },
    { "VIOLETFIFTEEN",     194 },
    { "VIOLETSIXTEEN",     195 },
    { "VIOLETSEVENTEEN",   196 },
    { "VIOLETEIGHTEEN",    197 },
    { "VIOLETNINETEEN",    198 },
    { "VIOLETTWENTY",      199 },
    { "VIOLETTWENTYONE",   200 },
    { "VIOLETTWENTYTWO",   201 },
    { "VIOLETTWENTYTHREE", 202 },
    { "VIOLETTWENTYFOUR",  203 },
    { "VIOLETTWENTYFIVE",  204 },
    { "VIOLETTWENTYSIX",   205 },
    { "VIOLETTWENTYSEVEN", 206 },
    { "VIOLETTWENTYEIGHT", 207 },
    { "VIOLETTWENTYNINE",  208 },
    { "VIOLETTHIRTY",      209 },
    { "MAGENTAONE",        210 },
    { "MAGENTATWO",        211 },
    { "MAGENTATHREE",      213 },
    { "MAGENTAFOUR",       215 },
    { "MAGENTAFIVE",       217 },
    { "MAGENTASIX",        218 },
    { "MAGENTASEVEN",      219 },
    { "MAGENTAEIGHT",      220 },
    { "MAGENTANINE",       221 },
    { "MAGENTATEN",        222 },
    { "MAGENTAELEVEN",     223 },
    { "MAGENTATWELVE",     224 },
    { "MAGENTATHIRTEEN",   225 },
    { "MAGENTAFOURTEEN",   226 },
    { "REDTEN",            227 },
    { "REDELEVEN",         228 },
    { "VIOLETFIFTEEN",     229 },
    { "REDTWELVE",         230 },
    { "REDTHIRTEEN",       231 },
    { "REDFOURTEEN",       232 },
    { "REDFIFTEEN",        233 },
    { "REDSIXTEEN",        234 },
    { "REDSEVENTEEN",      235 },
    { "REDEIGHTEEN",       236 },
    { "REDNINETEEN",       237 },
    { "REDTWENTY",         238 },
    { "REDTWENTYONE",      239 },
    { "REDTWENTYTWO",      240 },
    { "REDTWENTYTHREE",    241 },
    { "REDTWENTYFOUR",     242 },
    { "REDTWENTYFIVE",     243 },
    { "REDTWENTYSIX",      244 },
    { "REDTWENTYSEVEN",    245 },
    { "REDTWENTYEIGHT",    246 },
    { "REDTWENTYNINE",     247 },
    { "REDTHIRTY",         248 },
    { "REDTHIRTYONE",      249 },
    { "GRAYONE",           251 },
    { "GRAYTWO",           252 },
    { "GRAYTHREE",         253 },
    { "GRAYFOUR",          254 }
};

// Array of predefined DXF color values, each entry containing blue, green, red components and a corresponding color number.
static const struct
{
    int         blue;
    int         green;
    int         red;
    DXF_COLOR_T colorNumber;
} acad_dxf_color_values[] =
{
    { 0,    0,    0,    DXF_COLOR_T::BLACK              },
    { 0,    0,    127,  DXF_COLOR_T::RED,               },
    { 0,    165,  165,  DXF_COLOR_T::YELLOW,            },
    { 0,    127,  0,    DXF_COLOR_T::GREEN,             },
    { 127,  127,  0,    DXF_COLOR_T::CYAN,              },
    { 127,  0,    0,    DXF_COLOR_T::BLUE,              },
    { 127,  0,    127,  DXF_COLOR_T::MAGENTA,           },
    { 255,  255,  255,  DXF_COLOR_T::WHITE,             },
    { 0,    127,  127,  DXF_COLOR_T::BROWN,             },
    { 0,    82,   165,  DXF_COLOR_T::ORANGE,            },
    { 0,    0,    165,  DXF_COLOR_T::LIGHTRED,          },
    { 127,  255,  255,  DXF_COLOR_T::LIGHTYELLOW,       },
    { 0,    165,  0,    DXF_COLOR_T::LIGHTGREEN,        },
    { 165,  165,  0,    DXF_COLOR_T::LIGHTCYAN,         },
    { 165,  0,    0,    DXF_COLOR_T::LIGHTBLUE,         },
    { 165,  0,    165,  DXF_COLOR_T::LIGHTMAGENTA,      },
    { 0,    127,  255,  DXF_COLOR_T::LIGHTORANGE,       },
    { 192,  192,  192,  DXF_COLOR_T::LIGHTGRAY,         },
    { 0,    0,    76,   DXF_COLOR_T::DARKRED,           },
    { 127,  223,  255,  DXF_COLOR_T::DARKYELLOW,        },
    { 0,    76,   0,    DXF_COLOR_T::DARKGREEN,         },
    { 76,   76,   0,    DXF_COLOR_T::DARKCYAN,          },
    { 76,   0,    0,    DXF_COLOR_T::DARKBLUE,          },
    { 76,   0,    76,   DXF_COLOR_T::DARKMAGENTA,       },
    { 0,    76,   76,   DXF_COLOR_T::DARKBROWN,         },
    { 0,    63,   127,  DXF_COLOR_T::DARKORANGE,        },
    { 128,  128,  128,  DXF_COLOR_T::DARKGRAY,          },
    { 0,    0,    255,  DXF_COLOR_T::PURERED,           },
    { 0,    255,  255,  DXF_COLOR_T::PUREYELLOW,        },
    { 0,    255,  0,    DXF_COLOR_T::PUREGREEN,         },
    { 255,  255,  0,    DXF_COLOR_T::PURECYAN,          },
    { 255,  0,    0,    DXF_COLOR_T::PUREBLUE,          },
    { 255,  0,    255,  DXF_COLOR_T::PUREMAGENTA,       },
    { 0,    191,  255,  DXF_COLOR_T::PUREORANGE,        },
    { 38,   76,   76,   DXF_COLOR_T::PUREGRAY,          },
    { 127,  127,  255,  DXF_COLOR_T::REDONE,            },
    { 82,   82,   165,  DXF_COLOR_T::REDTWO,            },
    { 63,   63,   127,  DXF_COLOR_T::REDTHREE,          },
    { 38,   38,   76,   DXF_COLOR_T::REDFOUR,           },
    { 0,    0,    38,   DXF_COLOR_T::REDFIVE,           },
    { 19,   19,   38,   DXF_COLOR_T::REDSIX,            },
    { 0,    63,   255,  DXF_COLOR_T::ORANGEONE,         },
    { 127,  159,  255,  DXF_COLOR_T::ORANGETWO,         },
    { 0,    41,   165,  DXF_COLOR_T::ORANGETHREE,       },
    { 82,   103,  165,  DXF_COLOR_T::ORANGEFOUR,        },
    { 0,    31,   127,  DXF_COLOR_T::ORANGEFIVE,        },
    { 63,   79,   127,  DXF_COLOR_T::REDSEVEN,          },
    { 0,    19,   76,   DXF_COLOR_T::REDEIGHT,          },
    { 38,   47,   76,   DXF_COLOR_T::ORANGESIX,         },
    { 0,    9,    38,   DXF_COLOR_T::REDNINE,           },
    { 19,   23,   38,   DXF_COLOR_T::ORANGESEVEN,       },
    { 127,  191,  255,  DXF_COLOR_T::ORANGEEIGHT,       },
    { 82,   124,  165,  DXF_COLOR_T::ORANGENINE,        },
    { 63,   95,   127,  DXF_COLOR_T::ORANGETEN,         },
    { 0,    38,   76,   DXF_COLOR_T::ORANGEELEVEN,      },
    { 38,   57,   76,   DXF_COLOR_T::ORANGETWELVE,      },
    { 0,    19,   38,   DXF_COLOR_T::ORANGETHIRTEEN,    },
    { 19,   28,   38,   DXF_COLOR_T::ORANGEFOURTEEN,    },
    { 0,    124,  165,  DXF_COLOR_T::YELLOWONE,         },
    { 82,   145,  165,  DXF_COLOR_T::YELLOWTWO,         },
    { 0,    95,   127,  DXF_COLOR_T::YELLOWTHREE,       },
    { 63,   111,  127,  DXF_COLOR_T::YELLOWFOUR,        },
    { 0,    57,   76,   DXF_COLOR_T::YELLOWFIVE,        },
    { 38,   66,   76,   DXF_COLOR_T::YELLOWSIX,         },
    { 0,    28,   38,   DXF_COLOR_T::YELLOWSEVEN,       },
    { 19,   33,   38,   DXF_COLOR_T::YELLOWEIGHT,       },
    { 82,   165,  165,  DXF_COLOR_T::YELLOWNINE,        },
    { 63,   127,  127,  DXF_COLOR_T::YELLOWTEN,         },
    { 0,    38,   38,   DXF_COLOR_T::YELLOWELEVEN,      },
    { 19,   38,   38,   DXF_COLOR_T::YELLOWTWELVE,      },
    { 0,    255,  191,  DXF_COLOR_T::YELLOWTHIRTEEN,    },
    { 127,  255,  223,  DXF_COLOR_T::YELLOWFOURTEEN,    },
    { 0,    165,  124,  DXF_COLOR_T::GREENONE,          },
    { 82,   165,  145,  DXF_COLOR_T::GREENTWO,          },
    { 0,    127,  95,   DXF_COLOR_T::GREENTHREE,        },
    { 63,   127,  111,  DXF_COLOR_T::GREENFOUR,         },
    { 0,    76,   57,   DXF_COLOR_T::GREENFIVE,         },
    { 38,   76,   66,   DXF_COLOR_T::GREENSIX,          },
    { 0,    38,   28,   DXF_COLOR_T::GREENSEVEN,        },
    { 19,   38,   33,   DXF_COLOR_T::GREENEIGHT,        },
    { 0,    255,  127,  DXF_COLOR_T::GREENNINE,         },
    { 127,  255,  191,  DXF_COLOR_T::GREENTEN,          },
    { 0,    165,  82,   DXF_COLOR_T::GREENELEVEN,       },
    { 82,   165,  124,  DXF_COLOR_T::GREENTWELVE,       },
    { 0,    127,  63,   DXF_COLOR_T::GREENTHIRTEEN,     },
    { 63,   127,  95,   DXF_COLOR_T::GREENFOURTEEN,     },
    { 0,    76,   38,   DXF_COLOR_T::GREENFIFTEEN,      },
    { 38,   76,   57,   DXF_COLOR_T::GREENSIXTEEN,      },
    { 0,    38,   19,   DXF_COLOR_T::GREENSEVENTEEN,    },
    { 19,   38,   28,   DXF_COLOR_T::GREENEIGHTEEN,     },
    { 0,    255,  63,   DXF_COLOR_T::GREENNINETEEN,     },
    { 127,  255,  159,  DXF_COLOR_T::GREENTWENTY,       },
    { 0,    165,  41,   DXF_COLOR_T::GREENTWENTYONE,    },
    { 82,   165,  103,  DXF_COLOR_T::GREENTWENTYTWO,    },
    { 0,    127,  31,   DXF_COLOR_T::GREENTWENTYTHREE,  },
    { 63,   127,  79,   DXF_COLOR_T::GREENTWENTYFOUR,   },
    { 0,    76,   19,   DXF_COLOR_T::GREENTWENTYFIVE,   },
    { 38,   76,   47,   DXF_COLOR_T::GREENTWENTYSIX,    },
    { 0,    38,   9,    DXF_COLOR_T::GREENTWENTYSEVEN,  },
    { 19,   38,   23,   DXF_COLOR_T::GREENTWENTYEIGHT,  },
    { 0,    255,  0,    DXF_COLOR_T::GREENTWENTYNINE,   },
    { 127,  255,  127,  DXF_COLOR_T::GREENTHIRTY,       },
    { 82,   165,  82,   DXF_COLOR_T::GREENTHIRTYONE,    },
    { 63,   127,  63,   DXF_COLOR_T::GREENTHIRTYTWO,    },
    { 38,   76,   38,   DXF_COLOR_T::GREENTHIRTYTHREE,  },
    { 0,    38,   0,    DXF_COLOR_T::GREENTHIRTYFOUR,   },
    { 19,   38,   19,   DXF_COLOR_T::GREENTHIRTYFIVE,   },
    { 63,   255,  0,    DXF_COLOR_T::GREENTHIRTYSIX,    },
    { 159,  255,  127,  DXF_COLOR_T::GREENTHIRTYSEVEN,  },
    { 41,   165,  0,    DXF_COLOR_T::GREENTHIRTYEIGHT,  },
    { 103,  165,  82,   DXF_COLOR_T::GREENTHIRTYNINE,   },
    { 31,   127,  0,    DXF_COLOR_T::GREENFORTY,        },
    { 79,   127,  63,   DXF_COLOR_T::GREENFORTYONE,     },
    { 19,   76,   0,    DXF_COLOR_T::GREENFORTYTWO,     },
    { 47,   76,   38,   DXF_COLOR_T::GREENFORTYTHREE,   },
    { 9,    38,   0,    DXF_COLOR_T::GREENFORTYFOUR,    },
    { 23,   88,   19,   DXF_COLOR_T::GREENFORTYFIVE,    },
    { 127,  255,  0,    DXF_COLOR_T::GREENFORTYSIX,     },
    { 191,  255,  127,  DXF_COLOR_T::GREENFORTYSEVEN,   },
    { 82,   165,  0,    DXF_COLOR_T::GREENFORTYEIGHT,   },
    { 95,   127,  63,   DXF_COLOR_T::GREENFORTYNINE,    },
    { 63,   127,  0,    DXF_COLOR_T::GREENFIFTY,        },
    { 95,   127,  63,   DXF_COLOR_T::GREENFIFTYONE,     },
    { 38,   76,   0,    DXF_COLOR_T::GREENFIFTYTWO,     },
    { 57,   76,   38,   DXF_COLOR_T::GREENFIFTYTHREE,   },
    { 19,   38,   0,    DXF_COLOR_T::GREENFIFTYFOUR,    },
    { 28,   88,   19,   DXF_COLOR_T::GREENFIFTYFIVE,    },
    { 191,  255,  0,    DXF_COLOR_T::GREENFIFTYSIX,     },
    { 223,  255,  127,  DXF_COLOR_T::GREENFIFTYSEVEN,   },
    { 124,  165,  0,    DXF_COLOR_T::GREENFIFTYEIGHT,   },
    { 145,  165,  82,   DXF_COLOR_T::GREENFIFTYNINE,    },
    { 95,   127,  0,    DXF_COLOR_T::GREENSIXTY,        },
    { 111,  127,  63,   DXF_COLOR_T::GREENSIXTYONE,     },
    { 57,   76,   0,    DXF_COLOR_T::GREENSIXTYTWO,     },
    { 66,   76,   38,   DXF_COLOR_T::GREENSIXTYTHREE,   },
    { 28,   38,   0,    DXF_COLOR_T::GREENSIXTYFOUR,    },
    { 88,   88,   19,   DXF_COLOR_T::GREENSIXTYFIVE,    },
    { 255,  255,  127,  DXF_COLOR_T::CYANONE,           },
    { 165,  165,  82,   DXF_COLOR_T::CYANTWO,           },
    { 127,  127,  63,   DXF_COLOR_T::CYANTHREE,         },
    { 76,   76,   38,   DXF_COLOR_T::CYANFOUR,          },
    { 38,   38,   0,    DXF_COLOR_T::CYANFIVE,          },
    { 88,   88,   19,   DXF_COLOR_T::CYANSIX,           },
    { 255,  191,  0,    DXF_COLOR_T::CYANSEVEN,         },
    { 255,  223,  127,  DXF_COLOR_T::BLUEONE,           },
    { 165,  124,  0,    DXF_COLOR_T::BLUETWO,           },
    { 165,  145,  82,   DXF_COLOR_T::BLUETHREE,         },
    { 127,  95,   0,    DXF_COLOR_T::BLUEFOUR,          },
    { 127,  111,  63,   DXF_COLOR_T::BLUEFIVE,          },
    { 76,   57,   0,    DXF_COLOR_T::BLUESIX,           },
    { 126,  66,   38,   DXF_COLOR_T::BLUESEVEN,         },
    { 38,   28,   0,    DXF_COLOR_T::BLUEEIGHT,         },
    { 88,   88,   19,   DXF_COLOR_T::BLUENINE,          },
    { 255,  127,  0,    DXF_COLOR_T::BLUETEN,           },
    { 255,  191,  127,  DXF_COLOR_T::BLUEELEVEN,        },
    { 165,  82,   0,    DXF_COLOR_T::BLUETWELVE,        },
    { 165,  124,  82,   DXF_COLOR_T::BLUETHIRTEEN,      },
    { 127,  63,   0,    DXF_COLOR_T::BLUEFOURTEEN,      },
    { 127,  95,   63,   DXF_COLOR_T::BLUEFIFTEEN,       },
    { 76,   38,   0,    DXF_COLOR_T::BLUESIXTEEN,       },
    { 126,  57,   38,   DXF_COLOR_T::BLUESEVENTEEN,     },
    { 38,   19,   0,    DXF_COLOR_T::BLUEEIGHTEEN,      },
    { 88,   28,   19,   DXF_COLOR_T::BLUENINETEEN,      },
    { 255,  63,   0,    DXF_COLOR_T::BLUETWENTY,        },
    { 255,  159,  127,  DXF_COLOR_T::BLUETWENTYONE,     },
    { 165,  41,   0,    DXF_COLOR_T::BLUETWENTYTWO,     },
    { 165,  103,  82,   DXF_COLOR_T::BLUETWENTYTHREE,   },
    { 127,  31,   0,    DXF_COLOR_T::BLUETWENTYFOUR,    },
    { 127,  79,   63,   DXF_COLOR_T::BLUETWENTYFIVE,    },
    { 76,   19,   0,    DXF_COLOR_T::BLUETWENTYSIX,     },
    { 126,  47,   38,   DXF_COLOR_T::BLUETWENTYSEVEN,   },
    { 38,   9,    0,    DXF_COLOR_T::BLUETWENTYEIGHT,   },
    { 88,   23,   19,   DXF_COLOR_T::BLUETWENTYNINE,    },
    { 255,  0,    0,    DXF_COLOR_T::BLUETHIRTY,        },
    { 255,  127,  127,  DXF_COLOR_T::BLUETHIRTYONE,     },
    { 126,  38,   38,   DXF_COLOR_T::BLUETHIRTYTWO,     },
    { 38,   0,    0,    DXF_COLOR_T::BLUETHIRTYETHREE,  },
    { 88,   19,   19,   DXF_COLOR_T::BLUETHIRTYFOUR,    },
    { 255,  0,    63,   DXF_COLOR_T::VIOLETONE,         },
    { 255,  127,  159,  DXF_COLOR_T::VIOLETTWO,         },
    { 165,  0,    41,   DXF_COLOR_T::VIOLETTHREE,       },
    { 165,  82,   103,  DXF_COLOR_T::VIOLETFOUR,        },
    { 127,  0,    31,   DXF_COLOR_T::VIOLETFIVE,        },
    { 127,  63,   79,   DXF_COLOR_T::VIOLETSIX,         },
    { 76,   0,    19,   DXF_COLOR_T::VIOLETSEVEN,       },
    { 126,  38,   47,   DXF_COLOR_T::VIOLETEIGHT,       },
    { 38,   0,    9,    DXF_COLOR_T::VIOLETNINE,        },
    { 88,   19,   23,   DXF_COLOR_T::VIOLETTEN,         },
    { 255,  0,    127,  DXF_COLOR_T::VIOLETELEVEN,      },
    { 255,  127,  191,  DXF_COLOR_T::VIOLETTWELVE,      },
    { 165,  0,    82,   DXF_COLOR_T::VIOLETTHIRTEEN,    },
    { 165,  82,   124,  DXF_COLOR_T::VIOLETFOURTEEN,    },
    { 127,  0,    63,   DXF_COLOR_T::VIOLETFIFTEEN,     },
    { 127,  63,   95,   DXF_COLOR_T::VIOLETSIXTEEN,     },
    { 76,   0,    38,   DXF_COLOR_T::VIOLETSEVENTEEN,   },
    { 126,  38,   57,   DXF_COLOR_T::VIOLETEIGHTEEN,    },
    { 38,   0,    19,   DXF_COLOR_T::VIOLETNINETEEN,    },
    { 88,   19,   28,   DXF_COLOR_T::VIOLETTWENTY,      },
    { 255,  0,    191,  DXF_COLOR_T::VIOLETTWENTYONE,   },
    { 255,  127,  223,  DXF_COLOR_T::VIOLETTWENTYTWO,   },
    { 165,  0,    124,  DXF_COLOR_T::VIOLETTWENTYTHREE, },
    { 165,  82,   145,  DXF_COLOR_T::VIOLETTWENTYFOUR,  },
    { 127,  0,    95,   DXF_COLOR_T::VIOLETTWENTYFIVE,  },
    { 127,  63,   111,  DXF_COLOR_T::VIOLETTWENTYSIX,   },
    { 76,   0,    57,   DXF_COLOR_T::VIOLETTWENTYSEVEN, },
    { 76,   38,   66,   DXF_COLOR_T::VIOLETTWENTYEIGHT, },
    { 38,   0,    28,   DXF_COLOR_T::VIOLETTWENTYNINE,  },
    { 88,   19,   88,   DXF_COLOR_T::VIOLETTHIRTY,      },
    { 255,  0,    255,  DXF_COLOR_T::MAGENTAONE,        },
    { 255,  127,  255,  DXF_COLOR_T::MAGENTATWO,        },
    { 165,  82,   165,  DXF_COLOR_T::MAGENTATHREE,      },
    { 127,  63,   127,  DXF_COLOR_T::MAGENTAFOUR,       },
    { 76,   38,   76,   DXF_COLOR_T::MAGENTAFIVE,       },
    { 38,   0,    38,   DXF_COLOR_T::MAGENTASIX,        },
    { 88,   19,   88,   DXF_COLOR_T::MAGENTASEVEN,      },
    { 191,  0,    255,  DXF_COLOR_T::MAGENTAEIGHT,      },
    { 223,  127,  255,  DXF_COLOR_T::MAGENTANINE,       },
    { 124,  0,    165,  DXF_COLOR_T::MAGENTATEN,        },
    { 145,  82,   165,  DXF_COLOR_T::MAGENTAELEVEN,     },
    { 95,   0,    127,  DXF_COLOR_T::MAGENTATWELVE,     },
    { 111,  63,   127,  DXF_COLOR_T::MAGENTATHIRTEEN,   },
    { 57,   0,    76,   DXF_COLOR_T::MAGENTAFOURTEEN,   },
    { 66,   38,   76,   DXF_COLOR_T::REDTEN,            },
    { 28,   0,    38,   DXF_COLOR_T::REDELEVEN,         },
    { 88,   19,   88,   DXF_COLOR_T::VIOLETFIFTEEN,     },
    { 127,  0,    255,  DXF_COLOR_T::REDTWELVE,         },
    { 191,  127,  255,  DXF_COLOR_T::REDTHIRTEEN,       },
    { 82,   0,    165,  DXF_COLOR_T::REDFOURTEEN,       },
    { 124,  82,   165,  DXF_COLOR_T::REDFIFTEEN,        },
    { 63,   0,    127,  DXF_COLOR_T::REDSIXTEEN,        },
    { 95,   63,   127,  DXF_COLOR_T::REDSEVENTEEN,      },
    { 38,   0,    76,   DXF_COLOR_T::REDEIGHTEEN,       },
    { 57,   38,   76,   DXF_COLOR_T::REDNINETEEN,       },
    { 19,   0,    38,   DXF_COLOR_T::REDTWENTY,         },
    { 28,   19,   88,   DXF_COLOR_T::REDTWENTYONE,      },
    { 63,   0,    255,  DXF_COLOR_T::REDTWENTYTWO,      },
    { 159,  127,  255,  DXF_COLOR_T::REDTWENTYTHREE,    },
    { 41,   0,    165,  DXF_COLOR_T::REDTWENTYFOUR,     },
    { 103,  82,   165,  DXF_COLOR_T::REDTWENTYFIVE,     },
    { 31,   0,    127,  DXF_COLOR_T::REDTWENTYSIX,      },
    { 79,   63,   127,  DXF_COLOR_T::REDTWENTYSEVEN,    },
    { 19,   0,    76,   DXF_COLOR_T::REDTWENTYEIGHT,    },
    { 47,   38,   76,   DXF_COLOR_T::REDTWENTYNINE,     },
    { 9,    0,    38,   DXF_COLOR_T::REDTHIRTY,         },
    { 23,   19,   88,   DXF_COLOR_T::REDTHIRTYONE,      },
    { 101,  101,  101,  DXF_COLOR_T::GRAYONE,           },
    { 102,  102,  102,  DXF_COLOR_T::GRAYTWO,           },
    { 153,  153,  153,  DXF_COLOR_T::GRAYTHREE,         },
    { 204,  204,  204,  DXF_COLOR_T::GRAYFOUR,          }
};

static const char* getDXFLineType( LINE_STYLE aType )
{
    switch( aType )
    {
    case LINE_STYLE::DEFAULT:
    case LINE_STYLE::SOLID:
        return "CONTINUOUS";
    case LINE_STYLE::DASH:
        return "DASHED";
    case LINE_STYLE::DOT:
        return "DOTTED";
    case LINE_STYLE::DASHDOT:
        return "DASHDOT";
    case LINE_STYLE::DASHDOTDOT:
        return "DIVIDE";
    default:
        wxFAIL_MSG( "Unhandled LINE_STYLE" );
        return "CONTINUOUS";
    }
}

int DXF_PLOTTER::FindNearestLegacyColor( int aR, int aG, int aB )
{
    int nearestColorValueIndex = static_cast<int>(DXF_COLOR_T::BLACK);

    int nearestDistance = std::numeric_limits<int>::max();

    for( int trying = static_cast<int>(DXF_COLOR_T::BLACK); trying < static_cast<int>(DXF_COLOR_T::NBCOLORS); trying++ )
    {
        auto c = acad_dxf_color_values[trying];
        int  distance = ( aR - c.red ) * ( aR - c.red ) + ( aG - c.green ) * ( aG - c.green )
                       + ( aB - c.blue ) * ( aB - c.blue );

        if( distance < nearestDistance )
        {
            nearestDistance = distance;
            nearestColorValueIndex = trying;
        }
    }

    return nearestColorValueIndex;
}


/**
 * @brief Retrieves the current layer name or layer color name for DXF plotting.
 *
 * This function returns the appropriate layer name or layer color name depending on the specified
 * DXF_LAYER_OUTPUT_MODE. DXF files do not use RGB definitions for colors, so this function converts
 * the color to the nearest legacy color name acceptable in DXF files.
 *
 * @param mode The mode determining whether to return the layer name or layer color name.
 * @param layerId Optional parameter specifying the layer ID to use. If not provided, the current layer ID is used.
 * @return The layer name or color name as a wxString.
 *
 * The function operates in two main modes:
 * 1. Layer_Name or Current_Layer_Name: Returns the name of the specified layer or the current layer.
 *    - Searches through the `m_layersToExport` list to find the matching layer ID.
 *    - If the layer ID is found, returns the corresponding layer name.
 *    - If not found, defaults to "BLACK".
 *
 * 2. Layer_Color_Name or Current_Layer_Color_Name: Returns the color name of the specified layer or the current layer.
 *    - Retrieves the color of the layer from the render settings.
 *    - Finds the nearest legacy color that matches the layer's color.
 *    - Returns the name of the nearest legacy color.
 *
 * If the mode is unknown, returns "Unknown Mode".
 */
wxString DXF_PLOTTER::GetCurrentLayerName( DXF_LAYER_OUTPUT_MODE aMode, std::optional<PCB_LAYER_ID> alayerId )
{
    PCB_LAYER_ID actualLayerId = ( alayerId.has_value() ) ? alayerId.value() : m_layer;

    switch( aMode )
    {
    case DXF_LAYER_OUTPUT_MODE::Layer_Name:
    case DXF_LAYER_OUTPUT_MODE::Current_Layer_Name:
    {
        if( m_layersToExport.empty() )
        {
            COLOR4D layerColor = ( aMode == DXF_LAYER_OUTPUT_MODE::Current_Layer_Name )
                                         ? m_currentColor
                                         : RenderSettings()->GetLayerColor( actualLayerId );

            int color = FindNearestLegacyColor(
                    int( layerColor.r * 255 ), int( layerColor.g * 255 ), int( layerColor.b * 255 ) );
            return wxString( acad_dxf_color_names[color].name );
        }

        auto it = std::find_if( m_layersToExport.begin(), m_layersToExport.end(),
                                [actualLayerId]( const std::pair<int, wxString>& element )
                                {
                                    return element.first == actualLayerId;
                                } );

        return ( it != m_layersToExport.end() ) ? it->second : wxString( "BLACK" );
    }

    case DXF_LAYER_OUTPUT_MODE::Layer_Color_Name:
    case DXF_LAYER_OUTPUT_MODE::Current_Layer_Color_Name:
    {
        COLOR4D layerColor = ( aMode == DXF_LAYER_OUTPUT_MODE::Current_Layer_Color_Name )
                                     ? RenderSettings()->GetLayerColor( GetLayer() )
                                     : RenderSettings()->GetLayerColor( actualLayerId );

        int color = FindNearestLegacyColor(
                int( layerColor.r * 255 ), int( layerColor.g * 255 ), int( layerColor.b * 255 ) );
        wxString cname( acad_dxf_color_names[color].name );
        return cname;
    }

    default: return wxString( "Unknown Mode" );
    }
}


void DXF_PLOTTER::SetUnits( DXF_UNITS aUnit )
{
    m_plotUnits = aUnit;

    switch( aUnit )
    {
    case DXF_UNITS::MM:
        m_unitScalingFactor = 0.00254;
        m_measurementDirective = 1;
        break;

    case DXF_UNITS::INCH:
    default:
        m_unitScalingFactor = 0.0001;
        m_measurementDirective = 0;
    }
}


// convert aValue to a string, and remove trailing zeros
// In DXF files coordinates need a high precision: at least 9 digits when given
// in inches and 7 digits when in mm.
// So we use 16 digits and remove trailing 0 (if any)
static std::string formatCoord( double aValue )
{
    std::string buf;

    buf = fmt::format( "{:.16f}", aValue );

    // remove trailing zeros
    while( !buf.empty() && buf[buf.size() - 1] == '0' )
    {
        buf.pop_back();
    }

    return buf;
}


void DXF_PLOTTER::SetViewport( const VECTOR2I& aOffset, double aIusPerDecimil,
                               double aScale, bool aMirror )
{
    m_plotOffset  = aOffset;
    m_plotScale   = aScale;

    /* DXF paper is 'virtual' so there is no need of a paper size.
       Also this way we can handle the aux origin which can be useful
       (for example when aligning to a mechanical drawing) */
    m_paperSize.x = 0;
    m_paperSize.y = 0;

    /* Like paper size DXF units are abstract too. Anyway there is a
     * system variable (MEASUREMENT) which will be set to 0 to indicate
     * english units */
    m_IUsPerDecimil = aIusPerDecimil;
    m_iuPerDeviceUnit = 1.0 / aIusPerDecimil; // Gives a DXF in decimils
    m_iuPerDeviceUnit *= GetUnitScaling();    // Get the scaling factor for the current units

    m_plotMirror = false;                     // No mirroring on DXF
    m_currentColor = COLOR4D::BLACK;
}


bool DXF_PLOTTER::StartPlot( const wxString& aPageNumber )
{
    wxASSERT( m_outputFile );

    // DXF HEADER - Boilerplate
    // Defines the minimum for drawing i.e. the angle system and the
    // 4 linetypes (CONTINUOUS, DOTDASH, DASHED and DOTTED)
    fmt::print( m_outputFile,
                "  0\n"
                "SECTION\n"
                "  2\n"
                "HEADER\n"
                "  9\n"
                "$ANGBASE\n"
                "  50\n"
                "0.0\n"
                "  9\n"
                "$ANGDIR\n"
                "  70\n"
                "1\n"
                "  9\n"
                "$MEASUREMENT\n"
                "  70\n"
                "{}\n"
                "  0\n"
                "ENDSEC\n"
                "  0\n"
                "SECTION\n"
                "  2\n"
                "TABLES\n"
                "  0\n"
                "TABLE\n"
                "  2\n"
                "LTYPE\n"
                "  70\n"
                "4\n"
                "  0\n"
                "LTYPE\n"
                "  5\n"
                "40F\n"
                "  2\n"
                "CONTINUOUS\n"
                "  70\n"
                "0\n"
                "  3\n"
                "Solid line\n"
                "  72\n"
                "65\n"
                "  73\n"
                "0\n"
                "  40\n"
                "0.0\n"
                "  0\n"
                "LTYPE\n"
                "  5\n"
                "410\n"
                "  2\n"
                "DASHDOT\n"
                " 70\n"
                "0\n"
                "  3\n"
                "Dash Dot ____ _ ____ _\n"
                " 72\n"
                "65\n"
                " 73\n"
                "4\n"
                " 40\n"
                "2.0\n"
                " 49\n"
                "1.25\n"
                " 49\n"
                "-0.25\n"
                " 49\n"
                "0.25\n"
                " 49\n"
                "-0.25\n"
                "  0\n"
                "LTYPE\n"
                "  5\n"
                "411\n"
                "  2\n"
                "DASHED\n"
                " 70\n"
                "0\n"
                "  3\n"
                "Dashed __ __ __ __ __\n"
                " 72\n"
                "65\n"
                " 73\n"
                "2\n"
                " 40\n"
                "0.75\n"
                " 49\n"
                "0.5\n"
                " 49\n"
                "-0.25\n"
                "  0\n"
                "LTYPE\n"
                "  5\n"
                "43B\n"
                "  2\n"
                "DOTTED\n"
                " 70\n"
                "0\n"
                "  3\n"
                "Dotted .  .  .  .\n"
                " 72\n"
                "65\n"
                " 73\n"
                "2\n"
                " 40\n"
                "0.2\n"
                " 49\n"
                "0.0\n"
                " 49\n"
                "-0.2\n"
                "  0\n"
                "ENDTAB\n",
                GetMeasurementDirective() );

    // Text styles table
    // Defines 4 text styles, one for each bold/italic combination
    fmt::print( m_outputFile,
	            "  0\n"
	            "TABLE\n"
	            "  2\n"
	            "STYLE\n"
	            "  70\n"
	            "4\n" );

    static const char *style_name[4] = {"KICAD", "KICADB", "KICADI", "KICADBI"};

    for(int i = 0; i < 4; i++ )
    {
        fmt::print( m_outputFile,
                 "  0\n"
                 "STYLE\n"
                 "  2\n"
                 "{}\n"         // Style name
                 "  70\n"
                 "0\n"          // Standard flags
                 "  40\n"
                 "0\n"          // Non-fixed height text
                 "  41\n"
                 "1\n"          // Width factor (base)
                 "  42\n"
                 "1\n"          // Last height (mandatory)
                 "  50\n"
                 "{:g}\n"         // Oblique angle
                 "  71\n"
                 "0\n"          // Generation flags (default)
                 "  3\n"
                 // The standard ISO font (when kicad is build with it
                 // the dxf text in acad matches *perfectly*)
                 "isocp.shx\n", // Font name (when not bigfont)
                 // Apply a 15 degree angle to italic text
                 style_name[i], i < 2 ? 0 : DXF_OBLIQUE_ANGLE );
    }

    int numLayers = static_cast<int>( !m_layersToExport.empty() ? m_layersToExport.size() : static_cast<int>(DXF_COLOR_T::NBCOLORS) );

    // If printing in monochrome, only output the black layer
    if( !GetColorMode() && m_layersToExport.empty() )
        numLayers = 1;


    // Layer table - one layer per color
    fmt::print( m_outputFile,
             "  0\n"
             "ENDTAB\n"
             "  0\n"
             "TABLE\n"
             "  2\n"
             "LAYER\n"
             "  70\n"
             "{}\n", (int)numLayers );

    /* The layer/colors palette. The acad/DXF palette is divided in 3 zones:

       - The primary colors (1 - 9)
       - An HSV zone (10-250, 5 values x 2 saturations x 10 hues
       - Greys (251 - 255)
     */

    wxString layerName;
    int colorNumber;

    bool hasActualColor = false;
    COLOR4D actualColor;

    for( int i = 0; i < numLayers; i++ )
    {
        if( !m_layersToExport.empty() )
        {
            layerName = GetCurrentLayerName( DXF_LAYER_OUTPUT_MODE::Layer_Name, m_layersToExport.at( i ).first );
            wxString colorName = GetCurrentLayerName( DXF_LAYER_OUTPUT_MODE::Layer_Color_Name, m_layersToExport.at( i ).first );

            auto it = std::find_if( std::begin( acad_dxf_color_names ), std::end( acad_dxf_color_names ),
                                      [colorName](const auto& layer)
                                      {
                                          return std::strcmp( layer.name, colorName.ToStdString().c_str() ) == 0;
                                      });

            if( it != std::end( acad_dxf_color_names ) )
                colorNumber = it->index;
            else
                colorNumber = 7; // Default to white/black

            actualColor = RenderSettings()->GetLayerColor( m_layersToExport.at( i ).first );
            hasActualColor = true;
        }
        else
        {
            layerName = wxString( acad_dxf_color_names[i].name );
            colorNumber = acad_dxf_color_names[i].index;
        }

        fmt::print( m_outputFile,
                    "  0\n"
                    "LAYER\n"
                    "  2\n"
                    "{}\n"         // Layer name
                    "  70\n"
                    "0\n"          // Standard flags
                    "  62\n"
                    "{}\n",        // Color number
                    TO_UTF8( layerName ),
                    colorNumber );

        if( hasActualColor )
        {
            // Add the true color value as an extended data entry
            int r = static_cast<int>( actualColor.r * 255 );
            int g = static_cast<int>( actualColor.g * 255 );
            int b = static_cast<int>( actualColor.b * 255 );

            int trueColorValue = ( r << 16 ) | ( g << 8 ) | b;

            fmt::print( m_outputFile,
                        "  420\n"
                        "{}\n",
                        trueColorValue );
        }

        fmt::print( m_outputFile,
                    "  6\n"
                    "CONTINUOUS\n");// Linetype name
    }

    // End of layer table, begin entities
    fmt::print( m_outputFile,
           "  0\n"
           "ENDTAB\n"
           "  0\n"
           "ENDSEC\n"
           "  0\n"
           "SECTION\n"
           "  2\n"
           "ENTITIES\n" );

    return true;
}


bool DXF_PLOTTER::EndPlot()
{
    wxASSERT( m_outputFile );

    // DXF FOOTER
    fmt::print( m_outputFile,
            "  0\n"
           "ENDSEC\n"
           "  0\n"
           "EOF\n" );
    fclose( m_outputFile );
    m_outputFile = nullptr;

    return true;
}


void DXF_PLOTTER::SetColor( const COLOR4D& color )
{
    if( ( m_colorMode )
       || ( color == COLOR4D::BLACK )
       || ( color == COLOR4D::WHITE ) )
    {
        m_currentColor = color;
    }
    else
    {
        m_currentColor = COLOR4D::BLACK;
    }
}


void DXF_PLOTTER::Rect( const VECTOR2I& p1, const VECTOR2I& p2, FILL_T fill, int width,
                        int aCornerRadius )
{
    wxASSERT( m_outputFile );

    if( aCornerRadius > 0 )
    {
        BOX2I box( p1, VECTOR2I( p2.x - p1.x, p2.y - p1.y ) );
        box.Normalize();
        SHAPE_RECT rect( box );
        rect.SetRadius( aCornerRadius );
        PlotPoly( rect.Outline(), fill, width, nullptr );
        return;
    }

    if( p1 != p2 )
    {
        MoveTo( p1 );
        LineTo( VECTOR2I( p1.x, p2.y ) );
        LineTo( VECTOR2I( p2.x, p2.y ) );
        LineTo( VECTOR2I( p2.x, p1.y ) );
        FinishTo( VECTOR2I( p1.x, p1.y ) );
    }
    else
    {
        // Draw as a point
        wxString cLayerName = GetCurrentLayerName( DXF_LAYER_OUTPUT_MODE::Current_Layer_Name );

        VECTOR2D point_dev = userToDeviceCoordinates( p1 );

        fmt::print( m_outputFile, "0\nPOINT\n8\n{}\n10\n{}\n20\n",
                    TO_UTF8( cLayerName ),
                    formatCoord( point_dev.x ),
                    formatCoord( point_dev.y ) );
    }
}


void DXF_PLOTTER::Circle( const VECTOR2I& centre, int diameter, FILL_T fill, int width )
{
    wxASSERT( m_outputFile );
    double   radius = userToDeviceSize( diameter / 2 );
    VECTOR2D centre_dev = userToDeviceCoordinates( centre );

    wxString cLayerName = GetCurrentLayerName( DXF_LAYER_OUTPUT_MODE::Current_Layer_Name );

    if( radius > 0 )
    {
        if( fill == FILL_T::NO_FILL )
        {
            fmt::print( m_outputFile, "0\nCIRCLE\n8\n{}\n10\n{}\n20\n{}\n40\n{}\n",
                        TO_UTF8( cLayerName ),
                        formatCoord( centre_dev.x ),
                        formatCoord( centre_dev.y ),
                        formatCoord( radius ) );
        }
        else if( fill == FILL_T::FILLED_SHAPE )
        {
            double r = radius * 0.5;
            fmt::print( m_outputFile, "0\nPOLYLINE\n" );
            fmt::print( m_outputFile, "8\n{}\n66\n1\n70\n1\n", TO_UTF8( cLayerName ) );
            fmt::print( m_outputFile, "40\n{}\n41\n{}\n",
                                        formatCoord( radius ),
                                        formatCoord( radius ) );
            fmt::print( m_outputFile, "0\nVERTEX\n8\n{}\n", TO_UTF8( cLayerName ) );
            fmt::print( m_outputFile, "10\n{}\n 20\n{}\n42\n1.0\n",
                                        formatCoord( centre_dev.x-r ),
                                        formatCoord( centre_dev.y ) );
            fmt::print( m_outputFile, "0\nVERTEX\n8\n{}\n", TO_UTF8( cLayerName ) );
            fmt::print( m_outputFile, "10\n{}\n 20\n{}\n42\n1.0\n",
                                        formatCoord( centre_dev.x+r ),
                                        formatCoord( centre_dev.y ) );
            fmt::print( m_outputFile, "0\nSEQEND\n" );
        }
    }
    else
    {
        // Draw as a point
        fmt::print( m_outputFile, "0\nPOINT\n8\n{}\n10\n{}\n20\n{}\n", TO_UTF8( cLayerName ),
                    formatCoord( centre_dev.x ),
                    formatCoord( centre_dev.y ) );
    }
}


void DXF_PLOTTER::PlotPoly( const std::vector<VECTOR2I>& aCornerList, FILL_T aFill, int aWidth,
                            void* aData )
{
    if( aCornerList.size() <= 1 )
        return;

    unsigned last = aCornerList.size() - 1;

    // Plot outlines with lines (thickness = 0) to define the polygon
    if( aWidth <= 0 || aFill == FILL_T::NO_FILL  )
    {
        MoveTo( aCornerList[0] );

        for( unsigned ii = 1; ii < aCornerList.size(); ii++ )
            LineTo( aCornerList[ii] );

        // Close polygon if 'fill' requested
        if( aFill != FILL_T::NO_FILL )
        {
            if( aCornerList[last] != aCornerList[0] )
                LineTo( aCornerList[0] );
        }

        PenFinish();
        return;
    }

    // The polygon outline has thickness, and is filled
    // Build and plot the polygon which contains the initial
    // polygon and its thick outline
    SHAPE_POLY_SET  bufferOutline;
    SHAPE_POLY_SET  bufferPolybase;

    bufferPolybase.NewOutline();

    // enter outline as polygon:
    for( unsigned ii = 1; ii < aCornerList.size(); ii++ )
    {
        TransformOvalToPolygon( bufferOutline, aCornerList[ ii - 1 ], aCornerList[ ii ],
            aWidth, GetPlotterArcHighDef(), ERROR_INSIDE );
    }

    // enter the initial polygon:
    for( const VECTOR2I& corner : aCornerList )
        bufferPolybase.Append( corner );

    // Merge polygons to build the polygon which contains the initial
    // polygon and its thick outline

    // create the outline which contains thick outline:
    bufferPolybase.BooleanAdd( bufferOutline );
    bufferPolybase.Fracture();

    if( bufferPolybase.OutlineCount() < 1 )      // should not happen
        return;

    const SHAPE_LINE_CHAIN& path = bufferPolybase.COutline( 0 );

    if( path.PointCount() < 2 )           // should not happen
        return;

    // Now, output the final polygon to DXF file:
    last = path.PointCount() - 1;
    VECTOR2I point = path.CPoint( 0 );

    VECTOR2I startPoint( point.x, point.y );
    MoveTo( startPoint );

    for( int ii = 1; ii < path.PointCount(); ii++ )
    {
        point = path.CPoint( ii );
        LineTo( VECTOR2I( point.x, point.y ) );
    }

    // Close polygon, if needed
    point = path.CPoint( last );
    VECTOR2I endPoint( point.x, point.y );

    if( endPoint != startPoint )
        LineTo( startPoint );

    PenFinish();
}


std::vector<VECTOR2I> arcPts( const VECTOR2D& aCenter, const EDA_ANGLE& aStartAngle,
                              const EDA_ANGLE& aAngle, double aRadius )
{
    std::vector<VECTOR2I> pts;

    /*
     * Arcs are not so easily approximated by beziers (in the general case), so we approximate
     * them in the old way
     */
    EDA_ANGLE       startAngle = -aStartAngle;
    EDA_ANGLE       endAngle = startAngle - aAngle;
    VECTOR2I        start;
    VECTOR2I        end;
    const EDA_ANGLE delta( 5, DEGREES_T );   // increment to draw circles

    if( startAngle > endAngle )
        std::swap( startAngle, endAngle );

    // Usual trig arc plotting routine...
    start.x = KiROUND( aCenter.x + aRadius * ( -startAngle ).Cos() );
    start.y = KiROUND( aCenter.y + aRadius * ( -startAngle ).Sin() );
    pts.emplace_back( start );

    for( EDA_ANGLE ii = startAngle + delta; ii < endAngle; ii += delta )
    {
        end.x = KiROUND( aCenter.x + aRadius * ( -ii ).Cos() );
        end.y = KiROUND( aCenter.y + aRadius * ( -ii ).Sin() );
        pts.emplace_back( end );
    }

    end.x = KiROUND( aCenter.x + aRadius * ( -endAngle ).Cos() );
    end.y = KiROUND( aCenter.y + aRadius * ( -endAngle ).Sin() );
    pts.emplace_back( end );

    return pts;
}


void DXF_PLOTTER::PlotPoly( const SHAPE_LINE_CHAIN& aLineChain, FILL_T aFill, int aWidth, void* aData )
{
    std::set<size_t>      handledArcs;
    std::vector<VECTOR2I> cornerList;

    for( int ii = 0; ii < aLineChain.SegmentCount(); ++ii )
    {
        if( aLineChain.IsArcSegment( ii ) )
        {
            size_t arcIndex = aLineChain.ArcIndex( ii );

            if( !handledArcs.contains( arcIndex ) )
            {
                handledArcs.insert( arcIndex );
                const SHAPE_ARC& arc( aLineChain.Arc( arcIndex ) );
                std::vector<VECTOR2I> pts = arcPts( arc.GetCenter(), arc.GetStartAngle(),
                                                    arc.GetCentralAngle(), arc.GetRadius() );

                for( const VECTOR2I& pt : std::ranges::reverse_view( pts ) )
                    cornerList.emplace_back( pt );
            }
        }
        else
        {
            const SEG& seg( aLineChain.Segment( ii ) );
            cornerList.emplace_back( seg.A );
            cornerList.emplace_back( seg.B );
        }
    }

    if( aLineChain.IsClosed() && cornerList.front() != cornerList.back() )
        cornerList.emplace_back( aLineChain.CPoint( 0 ) );

    PlotPoly( cornerList, aFill, aWidth, aData );
}


void DXF_PLOTTER::PenTo( const VECTOR2I& pos, char plume )
{
    wxASSERT( m_outputFile );

    if( plume == 'Z' )
    {
        return;
    }

    VECTOR2D pos_dev = userToDeviceCoordinates( pos );
    VECTOR2D pen_lastpos_dev = userToDeviceCoordinates( m_penLastpos );

    if( m_penLastpos != pos && plume == 'D' )
    {
        wxASSERT( m_currentLineType >= LINE_STYLE::FIRST_TYPE
                  && m_currentLineType <= LINE_STYLE::LAST_TYPE );

        // DXF LINE
        wxString cLayerName = GetCurrentLayerName( DXF_LAYER_OUTPUT_MODE::Current_Layer_Name );

        const char* lname = getDXFLineType( static_cast<LINE_STYLE>( m_currentLineType ) );
        fmt::print( m_outputFile, "0\nLINE\n8\n{}\n6\n{}\n10\n{}\n20\n{}\n11\n{}\n21\n{}\n",
                    TO_UTF8( cLayerName ), lname,
                    formatCoord( pen_lastpos_dev.x ),
                    formatCoord( pen_lastpos_dev.y ),
                    formatCoord( pos_dev.x ),
                    formatCoord( pos_dev.y ) );
    }

    m_penLastpos = pos;
}


void DXF_PLOTTER::SetDash( int aLineWidth, LINE_STYLE aLineStyle )
{
    wxASSERT( aLineStyle >= LINE_STYLE::FIRST_TYPE
                && aLineStyle <= LINE_STYLE::LAST_TYPE );

    m_currentLineType = aLineStyle;
}


void DXF_PLOTTER::Arc( const VECTOR2D& aCenter, const EDA_ANGLE& aStartAngle,
                       const EDA_ANGLE& aAngle, double aRadius, FILL_T aFill, int aWidth )
{
    wxASSERT( m_outputFile );

    if( aRadius <= 0 )
        return;

    EDA_ANGLE startAngle = -aStartAngle;
    EDA_ANGLE endAngle = startAngle - aAngle;

    // In DXF, arcs are drawn CCW.
    // If startAngle > endAngle, it is CW. So transform it to CCW
    if( endAngle < startAngle )
        std::swap( startAngle, endAngle );

    VECTOR2D centre_device = userToDeviceCoordinates( aCenter );
    double   radius_device = userToDeviceSize( aRadius );

    // Emit a DXF ARC entity
    wxString cLayerName = GetCurrentLayerName( DXF_LAYER_OUTPUT_MODE::Current_Layer_Name );
    fmt::print( m_outputFile,
                "0\nARC\n8\n{}\n10\n{}\n20\n{}\n40\n{}\n50\n{:.8f}\n51\n{:.8f}\n",
                TO_UTF8( cLayerName ),
                formatCoord( centre_device.x ),
                formatCoord( centre_device.y ),
                formatCoord( radius_device ),
                startAngle.AsDegrees(),
                endAngle.AsDegrees() );
}


void DXF_PLOTTER::ThickSegment( const VECTOR2I& aStart, const VECTOR2I& aEnd, int aWidth, void* aData )
{
    const PLOT_PARAMS* cfg = static_cast<const PLOT_PARAMS*>( aData );

    if( cfg && cfg->GetDXFPlotMode() == SKETCH )
    {
        std::vector<VECTOR2I> cornerList;
        SHAPE_POLY_SET outlineBuffer;
        TransformOvalToPolygon( outlineBuffer, aStart, aEnd, aWidth, GetPlotterArcHighDef(),
                                ERROR_INSIDE );
        const SHAPE_LINE_CHAIN& path = outlineBuffer.COutline( 0 );

        cornerList.reserve( path.PointCount() );

        for( int jj = 0; jj < path.PointCount(); jj++ )
            cornerList.emplace_back( path.CPoint( jj ).x, path.CPoint( jj ).y );

        // Ensure the polygon is closed
        if( cornerList[0] != cornerList[cornerList.size() - 1] )
            cornerList.push_back( cornerList[0] );

        PlotPoly( cornerList, FILL_T::NO_FILL, DXF_LINE_WIDTH );
    }
    else
    {
        MoveTo( aStart );
        FinishTo( aEnd );
    }
}


void DXF_PLOTTER::ThickArc( const VECTOR2D& centre, const EDA_ANGLE& aStartAngle,
                            const EDA_ANGLE& aAngle, double aRadius, int aWidth, void* aData )
{
    const PLOT_PARAMS* cfg = static_cast<const PLOT_PARAMS*>( aData );

    if( cfg && cfg->GetDXFPlotMode() == SKETCH )
    {
        Arc( centre, aStartAngle, aAngle, aRadius - aWidth/2, FILL_T::NO_FILL, DXF_LINE_WIDTH );
        Arc( centre, aStartAngle, aAngle, aRadius + aWidth/2, FILL_T::NO_FILL, DXF_LINE_WIDTH );
    }
    else
    {
        Arc( centre, aStartAngle, aAngle, aRadius, FILL_T::NO_FILL, DXF_LINE_WIDTH );
    }
}


void DXF_PLOTTER::ThickRect( const VECTOR2I& p1, const VECTOR2I& p2, int width, void* aData )
{
    const PLOT_PARAMS* cfg = static_cast<const PLOT_PARAMS*>( aData );

    if( cfg && cfg->GetDXFPlotMode() == SKETCH )
    {
        VECTOR2I offsetp1( p1.x - width/2, p1.y - width/2 );
        VECTOR2I offsetp2( p2.x + width/2, p2.y + width/2 );
        Rect( offsetp1, offsetp2, FILL_T::NO_FILL, DXF_LINE_WIDTH, 0 );

        offsetp1.x += width;
        offsetp1.y += width;
        offsetp2.x -= width;
        offsetp2.y -= width;
        Rect( offsetp1, offsetp2, FILL_T::NO_FILL, DXF_LINE_WIDTH, 0 );
    }
    else
    {
        Rect( p1, p2, FILL_T::NO_FILL, DXF_LINE_WIDTH, 0 );
    }
}


void DXF_PLOTTER::ThickCircle( const VECTOR2I& pos, int diametre, int width, void* aData )
{
    const PLOT_PARAMS* cfg = static_cast<const PLOT_PARAMS*>( aData );

    if( cfg && cfg->GetDXFPlotMode() == SKETCH )
    {
        Circle( pos, diametre - width, FILL_T::NO_FILL, DXF_LINE_WIDTH );
        Circle( pos, diametre + width, FILL_T::NO_FILL, DXF_LINE_WIDTH );
    }
    else
    {
        Circle( pos, diametre, FILL_T::NO_FILL, DXF_LINE_WIDTH );
    }
}


void DXF_PLOTTER::FilledCircle( const VECTOR2I& pos, int diametre, void* aData )
{
    const PLOT_PARAMS* cfg = static_cast<const PLOT_PARAMS*>( aData );

    if( cfg && cfg->GetDXFPlotMode() == SKETCH )
        Circle( pos, diametre, FILL_T::NO_FILL, DXF_LINE_WIDTH );
    else
        Circle( pos, diametre, FILL_T::FILLED_SHAPE, 0 );
}


void DXF_PLOTTER::ThickPoly( const SHAPE_POLY_SET& aPoly, int aWidth, void* aData )
{
    const PLOT_PARAMS* cfg = static_cast<const PLOT_PARAMS*>( aData );

    if( cfg && cfg->GetDXFPlotMode() == SKETCH )
    {
        SHAPE_POLY_SET outline = aPoly.CloneDropTriangulation();
        outline.Inflate( aWidth / 2, CORNER_STRATEGY::ROUND_ALL_CORNERS, GetPlotterArcHighDef() );
        PLOTTER::PlotPoly( outline.COutline( 0 ), FILL_T::NO_FILL, DXF_LINE_WIDTH, aData );

        outline = aPoly.CloneDropTriangulation();
        outline.Deflate( aWidth / 2, CORNER_STRATEGY::ROUND_ALL_CORNERS, GetPlotterArcHighDef() );
        PLOTTER::PlotPoly( outline.COutline( 0 ), FILL_T::NO_FILL, DXF_LINE_WIDTH, aData );
    }
    else
    {
        PLOTTER::PlotPoly( aPoly.COutline( 0 ), FILL_T::NO_FILL, aWidth, aData );
    }
}


void DXF_PLOTTER::FlashPadOval( const VECTOR2I& aPos, const VECTOR2I& aSize,
                                const EDA_ANGLE& aOrient, void* aData )
{
    wxASSERT( m_outputFile );

    VECTOR2I  size( aSize );
    EDA_ANGLE orient( aOrient );

    /* The chip is reduced to an oval tablet with size.y > size.x
     * (Oval vertical orientation 0) */
    if( size.x > size.y )
    {
        std::swap( size.x, size.y );
        orient += ANGLE_90;
    }

    ThickOval( aPos, size, orient, DXF_LINE_WIDTH, aData );
}


void DXF_PLOTTER::FlashPadCircle( const VECTOR2I& pos, int diametre, void* aData )
{
    wxASSERT( m_outputFile );
    Circle( pos, diametre, FILL_T::NO_FILL, DXF_LINE_WIDTH );
}


void DXF_PLOTTER::FlashPadRect( const VECTOR2I& aPos, const VECTOR2I& aPadSize,
                                const EDA_ANGLE& aOrient, void* aData )
{
    wxASSERT( m_outputFile );

    VECTOR2I size, start, end;

    size.x = aPadSize.x / 2;
    size.y = aPadSize.y / 2;

    if( size.x < 0 )
        size.x = 0;

    if( size.y < 0 )
        size.y = 0;

    // If a dimension is zero, the trace is reduced to 1 line
    if( size.x == 0 )
    {
        start = VECTOR2I( aPos.x, aPos.y - size.y );
        end = VECTOR2I( aPos.x, aPos.y + size.y );
        RotatePoint( start, aPos, aOrient );
        RotatePoint( end, aPos, aOrient );
        MoveTo( start );
        FinishTo( end );
        return;
    }

    if( size.y == 0 )
    {
        start = VECTOR2I( aPos.x - size.x, aPos.y );
        end = VECTOR2I( aPos.x + size.x, aPos.y );
        RotatePoint( start, aPos, aOrient );
        RotatePoint( end, aPos, aOrient );
        MoveTo( start );
        FinishTo( end );
        return;
    }

    start = VECTOR2I( aPos.x - size.x, aPos.y - size.y );
    RotatePoint( start, aPos, aOrient );
    MoveTo( start );

    end = VECTOR2I( aPos.x - size.x, aPos.y + size.y );
    RotatePoint( end, aPos, aOrient );
    LineTo( end );

    end = VECTOR2I( aPos.x + size.x, aPos.y + size.y );
    RotatePoint( end, aPos, aOrient );
    LineTo( end );

    end = VECTOR2I( aPos.x + size.x, aPos.y - size.y );
    RotatePoint( end, aPos, aOrient );
    LineTo( end );

    FinishTo( start );
}


void DXF_PLOTTER::FlashPadRoundRect( const VECTOR2I& aPadPos, const VECTOR2I& aSize,
                                     int aCornerRadius, const EDA_ANGLE& aOrient, void* aData )
{
    SHAPE_POLY_SET outline;
    TransformRoundChamferedRectToPolygon( outline, aPadPos, aSize, aOrient, aCornerRadius, 0.0, 0,
                                          0, GetPlotterArcHighDef(), ERROR_INSIDE );

    // TransformRoundRectToPolygon creates only one convex polygon
    SHAPE_LINE_CHAIN& poly = outline.Outline( 0 );

    MoveTo( VECTOR2I( poly.CPoint( 0 ).x, poly.CPoint( 0 ).y ) );

    for( int ii = 1; ii < poly.PointCount(); ++ii )
        LineTo( VECTOR2I( poly.CPoint( ii ).x, poly.CPoint( ii ).y ) );

    FinishTo( VECTOR2I( poly.CPoint( 0 ).x, poly.CPoint( 0 ).y ) );
}


void DXF_PLOTTER::FlashPadCustom( const VECTOR2I& aPadPos, const VECTOR2I& aSize,
                                  const EDA_ANGLE& aOrient, SHAPE_POLY_SET* aPolygons,
                                  void* aData )
{
    for( int cnt = 0; cnt < aPolygons->OutlineCount(); ++cnt )
    {
        SHAPE_LINE_CHAIN& poly = aPolygons->Outline( cnt );

        MoveTo( VECTOR2I( poly.CPoint( 0 ).x, poly.CPoint( 0 ).y ) );

        for( int ii = 1; ii < poly.PointCount(); ++ii )
            LineTo( VECTOR2I( poly.CPoint( ii ).x, poly.CPoint( ii ).y ) );

        FinishTo( VECTOR2I( poly.CPoint( 0 ).x, poly.CPoint( 0 ).y ) );
    }
}


void DXF_PLOTTER::FlashPadTrapez( const VECTOR2I& aPadPos, const VECTOR2I* aCorners,
                                  const EDA_ANGLE& aPadOrient, void* aData )
{
    wxASSERT( m_outputFile );
    VECTOR2I coord[4]; /* coord actual corners of a trapezoidal trace */

    for( int ii = 0; ii < 4; ii++ )
    {
        coord[ii] = aCorners[ii];
        RotatePoint( coord[ii], aPadOrient );
        coord[ii] += aPadPos;
    }

    // Plot edge:
    MoveTo( coord[0] );
    LineTo( coord[1] );
    LineTo( coord[2] );
    LineTo( coord[3] );
    FinishTo( coord[0] );
}


void DXF_PLOTTER::FlashRegularPolygon( const VECTOR2I& aShapePos, int aRadius, int aCornerCount,
                                       const EDA_ANGLE& aOrient, void* aData )
{
    // Do nothing
    wxASSERT( 0 );
}


/**
 * Check if a given string contains non-ASCII characters.
 *
 * @param string String to check.
 * @return true if it contains some non-ASCII character, false if all characters are
 *         inside ASCII range (<=255).
 */
bool containsNonAsciiChars( const wxString& string )
{
    for( unsigned i = 0; i < string.length(); i++ )
    {
        wchar_t ch = string[i];

        if( ch > 255 )
            return true;
    }
    return false;
}


void DXF_PLOTTER::Text( const VECTOR2I&        aPos,
                        const COLOR4D&         aColor,
                        const wxString&        aText,
                        const EDA_ANGLE&       aOrient,
                        const VECTOR2I&        aSize,
                        enum GR_TEXT_H_ALIGN_T aH_justify,
                        enum GR_TEXT_V_ALIGN_T aV_justify,
                        int                    aWidth,
                        bool                   aItalic,
                        bool                   aBold,
                        bool                   aMultilineAllowed,
                        KIFONT::FONT*          aFont,
                        const KIFONT::METRICS& aFontMetrics,
                        void*                  aData )
{
    // Fix me: see how to use DXF text mode for multiline texts
    if( aMultilineAllowed && !aText.Contains( wxT( "\n" ) ) )
        aMultilineAllowed = false;  // the text has only one line.

    bool processSuperSub = aText.Contains( wxT( "^{" ) ) || aText.Contains( wxT( "_{" ) );

    if( m_textAsLines || containsNonAsciiChars( aText ) || aMultilineAllowed || processSuperSub )
    {
        // output text as graphics.
        // Perhaps multiline texts could be handled as DXF text entity
        // but I do not want spend time about this (JPC)
        PLOTTER::Text( aPos, aColor, aText, aOrient, aSize, aH_justify, aV_justify, aWidth, aItalic,
                       aBold, aMultilineAllowed, aFont, aFontMetrics, aData );
    }
    else
    {
        TEXT_ATTRIBUTES attrs;
        attrs.m_Halign = aH_justify;
        attrs.m_Valign =aV_justify;
        attrs.m_StrokeWidth = aWidth;
        attrs.m_Angle = aOrient;
        attrs.m_Italic = aItalic;
        attrs.m_Bold = aBold;
        attrs.m_Mirrored = aSize.x < 0;
        attrs.m_Multiline = false;
        plotOneLineOfText( aPos, aColor, aText, attrs );
    }
}


void DXF_PLOTTER::PlotText( const VECTOR2I&        aPos,
                            const COLOR4D&         aColor,
                            const wxString&        aText,
                            const TEXT_ATTRIBUTES& aAttributes,
                            KIFONT::FONT*          aFont,
                            const KIFONT::METRICS& aFontMetrics,
                            void*                  aData )
{
    TEXT_ATTRIBUTES attrs = aAttributes;

    // Fix me: see how to use DXF text mode for multiline texts
    if( attrs.m_Multiline && !aText.Contains( wxT( "\n" ) ) )
        attrs.m_Multiline = false;  // the text has only one line.

    bool processSuperSub = aText.Contains( wxT( "^{" ) ) || aText.Contains( wxT( "_{" ) );

    if( m_textAsLines || containsNonAsciiChars( aText ) || attrs.m_Multiline || processSuperSub )
    {
        // output text as graphics.
        // Perhaps multiline texts could be handled as DXF text entity
        // but I do not want spend time about that (JPC)
        PLOTTER::PlotText( aPos, aColor, aText, aAttributes, aFont, aFontMetrics, aData );
    }
    else
    {
        plotOneLineOfText( aPos, aColor, aText, attrs );
    }
}


void DXF_PLOTTER::plotOneLineOfText( const VECTOR2I& aPos, const COLOR4D& aColor,
                                     const wxString& aText, const TEXT_ATTRIBUTES& aAttributes )
{
    /* Emit text as a text entity. This loses formatting and shape but it's
       more useful as a CAD object */
    VECTOR2D origin_dev = userToDeviceCoordinates( aPos );
    SetColor( aColor );
    wxString cLayerName = GetCurrentLayerName( DXF_LAYER_OUTPUT_MODE::Current_Layer_Name );

    VECTOR2D size_dev = userToDeviceSize( aAttributes.m_Size );
    int h_code = 0, v_code = 0;

    switch( aAttributes.m_Halign )
    {
    case GR_TEXT_H_ALIGN_LEFT:   h_code = 0; break;
    case GR_TEXT_H_ALIGN_CENTER: h_code = 1; break;
    case GR_TEXT_H_ALIGN_RIGHT:  h_code = 2; break;
    case GR_TEXT_H_ALIGN_INDETERMINATE: wxFAIL_MSG( wxT( "Indeterminate state legal only in dialogs." ) ); break;
    }

    switch( aAttributes.m_Valign )
    {
    case GR_TEXT_V_ALIGN_TOP:    v_code = 3; break;
    case GR_TEXT_V_ALIGN_CENTER: v_code = 2; break;
    case GR_TEXT_V_ALIGN_BOTTOM: v_code = 1; break;
    case GR_TEXT_V_ALIGN_INDETERMINATE: wxFAIL_MSG( wxT( "Indeterminate state legal only in dialogs." ) ); break;
    }

    std::string textStyle = "KICAD";
    if( aAttributes.m_Bold )
    {
        if( aAttributes.m_Italic )
            textStyle = "KICADBI";
        else
            textStyle = "KICADB";
    }
    else if( aAttributes.m_Italic )
        textStyle = "KICADI";

    // Position, size, rotation and alignment
    // The two alignment point usages is somewhat idiot (see the DXF ref)
    // Anyway since we don't use the fit/aligned options, they're the same
    fmt::print( m_outputFile,
             "  0\n"
             "TEXT\n"
             "  7\n"
             "{}\n"          // Text style
             "  8\n"
             "{}\n"          // Layer name
             "  10\n"
             "{}\n"          // First point X
             "  11\n"
             "{}\n"          // Second point X
             "  20\n"
             "{}\n"          // First point Y
             "  21\n"
             "{}\n"          // Second point Y
             "  40\n"
             "{}\n"          // Text height
             "  41\n"
             "{}\n"          // Width factor
             "  50\n"
             "{:.8f}\n"        // Rotation
             "  51\n"
             "{:.8f}\n"        // Oblique angle
             "  71\n"
             "{}\n"          // Mirror flags
             "  72\n"
             "{}\n"          // H alignment
             "  73\n"
             "{}\n",         // V alignment
             aAttributes.m_Bold ? ( aAttributes.m_Italic ? "KICADBI" : "KICADB" )
                                : ( aAttributes.m_Italic ? "KICADI" : "KICAD" ),
             TO_UTF8( cLayerName ),
             formatCoord( origin_dev.x ),
             formatCoord( origin_dev.x ),
             formatCoord( origin_dev.y ),
             formatCoord( origin_dev.y ),
             formatCoord( size_dev.y ),
             formatCoord( fabs( size_dev.x / size_dev.y ) ),
             aAttributes.m_Angle.AsDegrees(),
             aAttributes.m_Italic ? DXF_OBLIQUE_ANGLE : 0,
             aAttributes.m_Mirrored ? 2 : 0, // X mirror flag
             h_code, v_code );

    /* There are two issue in emitting the text:
       - Our overline character (~) must be converted to the appropriate
       control sequence %%O or %%o
       - Text encoding in DXF is more or less unspecified since depends on
       the DXF declared version, the acad version reading it *and* some
       system variables to be put in the header handled only by newer acads
       Also before R15 unicode simply is not supported (you need to use
       bigfonts which are a massive PITA). Common denominator solution:
       use Latin1 (and however someone could choke on it, anyway). Sorry
       for the extended latin people. If somewant want to try fixing this
       recent version seems to use UTF-8 (and not UCS2 like the rest of
       Windows)

       XXX Actually there is a *third* issue: older DXF formats are limited
       to 255 bytes records (it was later raised to 2048); since I'm lazy
       and text so long is not probable I just don't implement this rule.
       If someone is interested in fixing this, you have to emit the first
       partial lines with group code 3 (max 250 bytes each) and then finish
       with a group code 1 (less than 250 bytes). The DXF refs explains it
       in no more details...
     */

    int braceNesting = 0;
    int overbarDepth = -1;

    fmt::print( m_outputFile, "  1\n" );

    for( unsigned int i = 0; i < aText.length(); i++ )
    {
        /* Here I do a bad thing: writing the output one byte at a time!
           but today I'm lazy and I have no idea on how to coerce a Unicode
           wxString to spit out latin1 encoded text ...

           At least stdio is *supposed* to do output buffering, so there is
           hope is not too slow */
        wchar_t ch = aText[i];

        if( ch > 255 )
        {
            // I can't encode this...
            putc( '?', m_outputFile );
        }
        else
        {
            if( aText[i] == '~' && i+1 < aText.length() && aText[i+1] == '{' )
            {
                fmt::print( m_outputFile, "%%o" );
                overbarDepth = braceNesting;

                // Skip the '{'
                i++;
                continue;
            }
            else if( aText[i] == '{' )
            {
                braceNesting++;
            }
            else if( aText[i] == '}' )
            {
                if( braceNesting > 0 )
                    braceNesting--;

                if( braceNesting == overbarDepth )
                {
                    fmt::print( m_outputFile, "%%O" );
                    overbarDepth = -1;
                    continue;
                }
            }

            putc( ch, m_outputFile );
        }
    }

    fmt::print( m_outputFile, "\n" );
}

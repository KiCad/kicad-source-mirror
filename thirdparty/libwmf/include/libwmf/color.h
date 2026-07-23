/* libwmf (<libwmf/color.h>): library for wmf conversion
   Copyright (C) 2000 - various; see CREDITS, ChangeLog, and sources

   The libwmf Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The libwmf Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the libwmf Library; see the file COPYING.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */


#ifndef LIBWMF_COLOR_H
#define LIBWMF_COLOR_H

#if defined (_WIN32) && !defined (LIBWMF_STATIC)
  #ifdef LIBWMF_EXPORTS
    #define LIBWMF_EXPORT __declspec(dllexport)
  #else
    #define LIBWMF_EXPORT __declspec(dllimport)
  #endif
#else
  #define LIBWMF_EXPORT
#endif

#include <libwmf/ipa.h>

/* These are the color names specified by W3C for the SVG standard
 */
extern LIBWMF_EXPORT wmfRGB wmf_aliceblue;
extern LIBWMF_EXPORT wmfRGB wmf_antiquewhite;
extern LIBWMF_EXPORT wmfRGB wmf_aqua;
extern LIBWMF_EXPORT wmfRGB wmf_aquamarine;
extern LIBWMF_EXPORT wmfRGB wmf_azure;
extern LIBWMF_EXPORT wmfRGB wmf_beige;
extern LIBWMF_EXPORT wmfRGB wmf_bisque;
extern LIBWMF_EXPORT wmfRGB wmf_black;
extern LIBWMF_EXPORT wmfRGB wmf_blanchedalmond;
extern LIBWMF_EXPORT wmfRGB wmf_blue;
extern LIBWMF_EXPORT wmfRGB wmf_blueviolet;
extern LIBWMF_EXPORT wmfRGB wmf_brown;
extern LIBWMF_EXPORT wmfRGB wmf_burlywood;
extern LIBWMF_EXPORT wmfRGB wmf_cadetblue;
extern LIBWMF_EXPORT wmfRGB wmf_chartreuse;
extern LIBWMF_EXPORT wmfRGB wmf_chocolate;
extern LIBWMF_EXPORT wmfRGB wmf_coral;
extern LIBWMF_EXPORT wmfRGB wmf_cornflowerblue;
extern LIBWMF_EXPORT wmfRGB wmf_cornsilk;
extern LIBWMF_EXPORT wmfRGB wmf_crimson;
extern LIBWMF_EXPORT wmfRGB wmf_cyan;
extern LIBWMF_EXPORT wmfRGB wmf_darkblue;
extern LIBWMF_EXPORT wmfRGB wmf_darkcyan;
extern LIBWMF_EXPORT wmfRGB wmf_darkgoldenrod;
extern LIBWMF_EXPORT wmfRGB wmf_darkgray;
extern LIBWMF_EXPORT wmfRGB wmf_darkgreen;
extern LIBWMF_EXPORT wmfRGB wmf_darkgrey;
extern LIBWMF_EXPORT wmfRGB wmf_darkkhaki;
extern LIBWMF_EXPORT wmfRGB wmf_darkmagenta;
extern LIBWMF_EXPORT wmfRGB wmf_darkolivegreen;
extern LIBWMF_EXPORT wmfRGB wmf_darkorange;
extern LIBWMF_EXPORT wmfRGB wmf_darkorchid;
extern LIBWMF_EXPORT wmfRGB wmf_darkred;
extern LIBWMF_EXPORT wmfRGB wmf_darksalmon;
extern LIBWMF_EXPORT wmfRGB wmf_darkseagreen;
extern LIBWMF_EXPORT wmfRGB wmf_darkslateblue;
extern LIBWMF_EXPORT wmfRGB wmf_darkslategray;
extern LIBWMF_EXPORT wmfRGB wmf_darkslategrey;
extern LIBWMF_EXPORT wmfRGB wmf_darkturquoise;
extern LIBWMF_EXPORT wmfRGB wmf_darkviolet;
extern LIBWMF_EXPORT wmfRGB wmf_deeppink;
extern LIBWMF_EXPORT wmfRGB wmf_deepskyblue;
extern LIBWMF_EXPORT wmfRGB wmf_dimgray;
extern LIBWMF_EXPORT wmfRGB wmf_dimgrey;
extern LIBWMF_EXPORT wmfRGB wmf_dodgerblue;
extern LIBWMF_EXPORT wmfRGB wmf_firebrick;
extern LIBWMF_EXPORT wmfRGB wmf_floralwhite;
extern LIBWMF_EXPORT wmfRGB wmf_forestgreen;
extern LIBWMF_EXPORT wmfRGB wmf_fuchsia;
extern LIBWMF_EXPORT wmfRGB wmf_gainsboro;
extern LIBWMF_EXPORT wmfRGB wmf_ghostwhite;
extern LIBWMF_EXPORT wmfRGB wmf_gold;
extern LIBWMF_EXPORT wmfRGB wmf_goldenrod;
extern LIBWMF_EXPORT wmfRGB wmf_gray;
extern LIBWMF_EXPORT wmfRGB wmf_grey;
extern LIBWMF_EXPORT wmfRGB wmf_green;
extern LIBWMF_EXPORT wmfRGB wmf_greenyellow;
extern LIBWMF_EXPORT wmfRGB wmf_honeydew;
extern LIBWMF_EXPORT wmfRGB wmf_hotpink;
extern LIBWMF_EXPORT wmfRGB wmf_indianred;
extern LIBWMF_EXPORT wmfRGB wmf_indigo;
extern LIBWMF_EXPORT wmfRGB wmf_ivory;
extern LIBWMF_EXPORT wmfRGB wmf_khaki;
extern LIBWMF_EXPORT wmfRGB wmf_lavender;
extern LIBWMF_EXPORT wmfRGB wmf_lavenderblush;
extern LIBWMF_EXPORT wmfRGB wmf_lawngreen;
extern LIBWMF_EXPORT wmfRGB wmf_lemonchiffon;
extern LIBWMF_EXPORT wmfRGB wmf_lightblue;
extern LIBWMF_EXPORT wmfRGB wmf_lightcoral;
extern LIBWMF_EXPORT wmfRGB wmf_lightcyan;
extern LIBWMF_EXPORT wmfRGB wmf_lightgoldenrodyellow;
extern LIBWMF_EXPORT wmfRGB wmf_lightgray;
extern LIBWMF_EXPORT wmfRGB wmf_lightgreen;
extern LIBWMF_EXPORT wmfRGB wmf_lightgrey;
extern LIBWMF_EXPORT wmfRGB wmf_lightpink;
extern LIBWMF_EXPORT wmfRGB wmf_lightsalmon;
extern LIBWMF_EXPORT wmfRGB wmf_lightseagreen;
extern LIBWMF_EXPORT wmfRGB wmf_lightskyblue;
extern LIBWMF_EXPORT wmfRGB wmf_lightslategray;
extern LIBWMF_EXPORT wmfRGB wmf_lightslategrey;
extern LIBWMF_EXPORT wmfRGB wmf_lightsteelblue;
extern LIBWMF_EXPORT wmfRGB wmf_lightyellow;
extern LIBWMF_EXPORT wmfRGB wmf_lime;
extern LIBWMF_EXPORT wmfRGB wmf_limegreen;
extern LIBWMF_EXPORT wmfRGB wmf_linen;
extern LIBWMF_EXPORT wmfRGB wmf_magenta;
extern LIBWMF_EXPORT wmfRGB wmf_maroon;
extern LIBWMF_EXPORT wmfRGB wmf_mediumaquamarine;
extern LIBWMF_EXPORT wmfRGB wmf_mediumblue;
extern LIBWMF_EXPORT wmfRGB wmf_mediumorchid;
extern LIBWMF_EXPORT wmfRGB wmf_mediumpurple;
extern LIBWMF_EXPORT wmfRGB wmf_mediumseagreen;
extern LIBWMF_EXPORT wmfRGB wmf_mediumslateblue;
extern LIBWMF_EXPORT wmfRGB wmf_mediumspringgreen;
extern LIBWMF_EXPORT wmfRGB wmf_mediumturquoise;
extern LIBWMF_EXPORT wmfRGB wmf_mediumvioletred;
extern LIBWMF_EXPORT wmfRGB wmf_midnightblue;
extern LIBWMF_EXPORT wmfRGB wmf_mintcream;
extern LIBWMF_EXPORT wmfRGB wmf_mistyrose;
extern LIBWMF_EXPORT wmfRGB wmf_moccasin;
extern LIBWMF_EXPORT wmfRGB wmf_navajowhite;
extern LIBWMF_EXPORT wmfRGB wmf_navy;
extern LIBWMF_EXPORT wmfRGB wmf_oldlace;
extern LIBWMF_EXPORT wmfRGB wmf_olive;
extern LIBWMF_EXPORT wmfRGB wmf_olivedrab;
extern LIBWMF_EXPORT wmfRGB wmf_orange;
extern LIBWMF_EXPORT wmfRGB wmf_orangered;
extern LIBWMF_EXPORT wmfRGB wmf_orchid;
extern LIBWMF_EXPORT wmfRGB wmf_palegoldenrod;
extern LIBWMF_EXPORT wmfRGB wmf_palegreen;
extern LIBWMF_EXPORT wmfRGB wmf_paleturquoise;
extern LIBWMF_EXPORT wmfRGB wmf_palevioletred;
extern LIBWMF_EXPORT wmfRGB wmf_papayawhip;
extern LIBWMF_EXPORT wmfRGB wmf_peachpuff;
extern LIBWMF_EXPORT wmfRGB wmf_peru;
extern LIBWMF_EXPORT wmfRGB wmf_pink;
extern LIBWMF_EXPORT wmfRGB wmf_plum;
extern LIBWMF_EXPORT wmfRGB wmf_powderblue;
extern LIBWMF_EXPORT wmfRGB wmf_purple;
extern LIBWMF_EXPORT wmfRGB wmf_red;
extern LIBWMF_EXPORT wmfRGB wmf_rosybrown;
extern LIBWMF_EXPORT wmfRGB wmf_royalblue;
extern LIBWMF_EXPORT wmfRGB wmf_saddlebrown;
extern LIBWMF_EXPORT wmfRGB wmf_salmon;
extern LIBWMF_EXPORT wmfRGB wmf_sandybrown;
extern LIBWMF_EXPORT wmfRGB wmf_seagreen;
extern LIBWMF_EXPORT wmfRGB wmf_seashell;
extern LIBWMF_EXPORT wmfRGB wmf_sienna;
extern LIBWMF_EXPORT wmfRGB wmf_silver;
extern LIBWMF_EXPORT wmfRGB wmf_skyblue;
extern LIBWMF_EXPORT wmfRGB wmf_slateblue;
extern LIBWMF_EXPORT wmfRGB wmf_slategray;
extern LIBWMF_EXPORT wmfRGB wmf_slategrey;
extern LIBWMF_EXPORT wmfRGB wmf_snow;
extern LIBWMF_EXPORT wmfRGB wmf_springgreen;
extern LIBWMF_EXPORT wmfRGB wmf_steelblue;
extern LIBWMF_EXPORT wmfRGB wmf_tan;
extern LIBWMF_EXPORT wmfRGB wmf_teal;
extern LIBWMF_EXPORT wmfRGB wmf_thistle;
extern LIBWMF_EXPORT wmfRGB wmf_tomato;
extern LIBWMF_EXPORT wmfRGB wmf_turquoise;
extern LIBWMF_EXPORT wmfRGB wmf_violet;
extern LIBWMF_EXPORT wmfRGB wmf_wheat;
extern LIBWMF_EXPORT wmfRGB wmf_white;
extern LIBWMF_EXPORT wmfRGB wmf_whitesmoke;
extern LIBWMF_EXPORT wmfRGB wmf_yellow;
extern LIBWMF_EXPORT wmfRGB wmf_yellowgreen;

#endif /* ! LIBWMF_COLOR_H */

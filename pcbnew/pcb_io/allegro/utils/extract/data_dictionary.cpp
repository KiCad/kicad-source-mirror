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

#include "data_dictionary.h"


using namespace ALLEGRO;
using IRV = EXTRACT_SPEC_PARSER::IR::VIEW_TYPE;

// Per the "Extract Data Dictionary table" in algrodsncmp.pdf
// Some fields aren't in the table, but are seen in files, so this may be a version thing.

// clang-format off
const std::unordered_map<std::string, std::set<EXTRACT_SPEC_PARSER::IR::VIEW_TYPE>> k_valid_views_for_fields = {
                                            // A                B                     C                 D                   E                F                     G                 H                    I               J          K               L
    { "APADFLASH",                          { IRV::COMPONENT,   IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,    IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,  IRV::SYMBOL,    IRV::BOARD,   IRV::CONNECTIVITY,   IRV::LAYER   } },
    { "APADHGHT",                           { IRV::COMPONENT,   IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,    IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,  IRV::SYMBOL,    IRV::BOARD,   IRV::CONNECTIVITY,   IRV::LAYER   } },
    { "APADSHAPE1",                         { IRV::COMPONENT,   IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,    IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,  IRV::SYMBOL,    IRV::BOARD,   IRV::CONNECTIVITY,   IRV::LAYER   } },
    { "APADSHAPENAME",                      { IRV::COMPONENT,   IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,    IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,  IRV::SYMBOL,    IRV::BOARD,   IRV::CONNECTIVITY,   IRV::LAYER   } },
    { "APADWIDTH",                          { IRV::COMPONENT,   IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,    IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,  IRV::SYMBOL,    IRV::BOARD,   IRV::CONNECTIVITY,   IRV::LAYER   } },
    { "APADXOFF",                           { IRV::COMPONENT,   IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,    IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,  IRV::SYMBOL,    IRV::BOARD,   IRV::CONNECTIVITY,   IRV::LAYER   } },
    { "APADYOFF",                           { IRV::COMPONENT,   IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,    IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,  IRV::SYMBOL,    IRV::BOARD,   IRV::CONNECTIVITY,   IRV::LAYER   } },

    { "BOARD_ACCURACY",                     { IRV::COMPONENT,   IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,    IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,  IRV::SYMBOL,    IRV::BOARD,   IRV::CONNECTIVITY,   IRV::LAYER   } },
    { "BOARD_DRC_STATUS",                   { IRV::COMPONENT,   IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,    IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,  IRV::SYMBOL,    IRV::BOARD,   IRV::CONNECTIVITY,   IRV::LAYER   } },
    { "BOARD_EXTENTS_X1",                   { IRV::COMPONENT,   IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,    IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,  IRV::SYMBOL,    IRV::BOARD,   IRV::CONNECTIVITY,   IRV::LAYER   } },

    { "BACKDRILL_TOP_LAYER",                {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                     IRV::FULL_GEOMETRY                                                                   } },
    { "BACKDRILL_BOTTOM_LAYER",             {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                     IRV::FULL_GEOMETRY                                                                   } },

    { "BOARD_EXTENTS_Y1",                   { IRV::COMPONENT,   IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,    IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,  IRV::SYMBOL,    IRV::BOARD,   IRV::CONNECTIVITY,   IRV::LAYER   } },
    { "BOARD_EXTENTS_X2",                   { IRV::COMPONENT,   IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,    IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,  IRV::SYMBOL,    IRV::BOARD,   IRV::CONNECTIVITY,   IRV::LAYER   } },
    { "BOARD_EXTENTS_Y2",                   { IRV::COMPONENT,   IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,    IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,  IRV::SYMBOL,    IRV::BOARD,   IRV::CONNECTIVITY,   IRV::LAYER   } },
    { "BOARD_LAYERS",                       { IRV::COMPONENT,   IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,    IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,  IRV::SYMBOL,    IRV::BOARD,   IRV::CONNECTIVITY,   IRV::LAYER   } },
    { "BOARD_NAME",                         { IRV::COMPONENT,   IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,    IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,  IRV::SYMBOL,    IRV::BOARD,   IRV::CONNECTIVITY,   IRV::LAYER   } },
    { "BOARD_UNITS",                        { IRV::COMPONENT,   IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,    IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,  IRV::SYMBOL,    IRV::BOARD,   IRV::CONNECTIVITY,   IRV::LAYER   } },

    { "CLASS",                              {                   IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                 } },
    { "COMP_CLASS",                         { IRV::COMPONENT,   IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,                          IRV::SYMBOL,                 IRV::CONNECTIVITY                 } },
    { "COMP_DEVICE_TYPE",                   { IRV::COMPONENT,   IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,                          IRV::SYMBOL,                 IRV::CONNECTIVITY                 } },
    { "COMP_MAX_POWER_DISS_DEVICE_INSTNCE", { IRV::COMPONENT,   IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,   IRV::SYMBOL,                 IRV::CONNECTIVITY                 } },
    { "COMP_PACKAGE",                       { IRV::COMPONENT,   IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,                          IRV::SYMBOL,                 IRV::CONNECTIVITY                 } },

    { "DRILL_FIGURE_CHAR",                  {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                     IRV::FULL_GEOMETRY                                                                    } },
    { "DRILL_FIGURE_HEIGHT",                {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                     IRV::FULL_GEOMETRY                                                                    } },
    { "DRILL_FIGURE_ROTATION",              {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                     IRV::FULL_GEOMETRY                                                                    } },
    { "DRILL_FIGURE_SHAPE",                 {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                     IRV::FULL_GEOMETRY                                                                    } },
    { "DRILL_FIGURE_WIDTH",                 {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                     IRV::FULL_GEOMETRY                                                                    } },
    { "DRILL_HOLE_NAME",                    {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                     IRV::FULL_GEOMETRY                                                                    } },
    { "DRILL_HOLE_NAME2",                   {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                     IRV::FULL_GEOMETRY                                                                    } },
    { "DRILL_HOLE_NEGTOL",                  {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                     IRV::FULL_GEOMETRY                                                                    } },
    { "DRILL_HOLE_PLATING",                 {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                     IRV::FULL_GEOMETRY                                                                    } },
    { "DRILL_HOLE_POSTOL",                  {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                     IRV::FULL_GEOMETRY                                                                    } },
    { "DRILL_HOLE_X",                       {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                     IRV::FULL_GEOMETRY                                                                    } },
    { "DRILL_HOLE_Y",                       {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                     IRV::FULL_GEOMETRY                                                                    } },
    { "DRILL_ARRAY_ROWS",                   {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                     IRV::FULL_GEOMETRY                                                                    } },
    { "DRILL_ARRAY_COLUMNS",                {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                     IRV::FULL_GEOMETRY                                                                    } },
    { "DRILL_ARRAY_CLEARANCE",              {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                     IRV::FULL_GEOMETRY                                                                    } },
    { "DRILL_ARRAY_LOCATIONS",              {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                     IRV::FULL_GEOMETRY                                                                    } },
    { "END_LAYER_NAME",                     {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD                                                                                                            } },
    { "END_LAYER_NUMBER",                   {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD                                                                                                            } },

    { "FIXFLAG",                            { IRV::COMPONENT,   IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,    IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,  IRV::SYMBOL,    IRV::BOARD,     IRV::CONNECTIVITY,   IRV::LAYER  } },

    { "FUNC_DES",                           {                   IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD                                                                                                            } },
    { "FUNC_DES_SORT",                      {                   IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD                                                                                                            } },
    { "FUNC_SLOT_NAME",                     {                   IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD                                                                                                            } },
    { "FUNC_TYPE",                          {                   IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD                                                                                                            } },

    { "GRAPHIC_DATA_NAME",                  {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,                                   IRV::CONNECTIVITY               } },
    { "GRAPHIC_DATA_NUMBER",                {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,                                   IRV::CONNECTIVITY               } },
    { "GRAPHIC_DATA_1",                     {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,                                   IRV::CONNECTIVITY               } },
    { "GRAPHIC_DATA_2",                     {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,                                   IRV::CONNECTIVITY               } },
    { "GRAPHIC_DATA_3",                     {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,                                   IRV::CONNECTIVITY               } },
    { "GRAPHIC_DATA_4",                     {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,                                   IRV::CONNECTIVITY               } },
    { "GRAPHIC_DATA_5",                     {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,                                   IRV::CONNECTIVITY               } },
    { "GRAPHIC_DATA_6",                     {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,                                   IRV::CONNECTIVITY               } },
    { "GRAPHIC_DATA_7",                     {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,                                   IRV::CONNECTIVITY               } },
    { "GRAPHIC_DATA_8",                     {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,                                   IRV::CONNECTIVITY               } },
    { "GRAPHIC_DATA_9",                     {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,                                   IRV::CONNECTIVITY               } },
    { "GRAPHIC_DATA_10",                    {                   IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,    IRV::FULL_GEOMETRY,                                   IRV::CONNECTIVITY               } },

    { "LAYER",                              {                                                                                                                                                                                                                  IRV::LAYER  } },
    { "LAYER_ARTWORK",                      {                                                                                                                                                                                                                  IRV::LAYER  } },
    { "LAYER_CONDUCTOR",                    {                                                                                                                                                                                                                  IRV::LAYER  } },
    { "LAYER_DIELECTRIC_CONSTANT",          {                                                                                                                                                                                                                  IRV::LAYER  } },
    { "LAYER_ELECTRICAL_CONDUCTIVITY",      {                                                                                                                                                                                                                  IRV::LAYER  } },
    { "LAYER_MATERIAL",                     {                                                                                                                                                                                                                  IRV::LAYER  } },
    { "LAYER_LOSS_TANGENT",                 {                                                                                                                                                                                                                  IRV::LAYER  } },
    { "LAYER_SORT",                         {                                                                                                                                                                                                                  IRV::LAYER  } },
    { "LAYER_SUBCLASS",                     {                                                                                                                                                                                                                  IRV::LAYER  } },
    { "LAYER_SHIELD_LAYER",                 {                                                                                                                                                                                                                  IRV::LAYER  } },
    { "LAYER_THERMAL_CONDUCTIVITY",         {                                                                                                                                                                                                                  IRV::LAYER  } },
    { "LAYER_THICKNESS",                    {                                                                                                                                                                                                                  IRV::LAYER  } },
    { "LAYER_TYPE",                         {                                                                                                                                                                                                                  IRV::LAYER  } },
    { "LAYER_USE",                          {                                                                                                                                                                                                                  IRV::LAYER  } },

    { "NET_CAPACITANCE",                    {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,   IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "NET_ETCH_LENGTH",                    {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,   IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "NET_ETCH_WIDTH_AVERAGE",             {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,   IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "NET_IMPEDANCE_AVERAGE",              {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,   IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "NET_IMPEDANCE_MAXIMUM",              {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,   IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "NET_IMPEDANCE_MINIMUM",              {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,   IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "NET_INDUCTANCE",                     {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,   IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "NET_MANHATTAN_LENGTH",               {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,   IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "NET_NAME",                           {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,   IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "NET_NAME_SORT",                      {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,   IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "NET_PATH_LENGTH",                    {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,   IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "NET_PROPAGATION_DELAY_ACTUAL",       {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,   IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "NET_RAT_CONNECT",                    {                    IRV::COMPONENT_PIN                                                                                                                                                                                         } },
    { "NET_RAT_SCHEDULE",                   {                    IRV::COMPONENT_PIN                                                                                                                                                                                         } },
    { "NET_RESISTANCE",                     {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,   IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "NET_STATUS",                         {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,   IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "NET_VIA_COUNT",                      {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,   IRV::NET,        IRV::COMPOSITE_PAD,   IRV::GEOMETRY,       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },

    { "NODE_CONNECTS",                      {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                    IRV::COMPOSITE_PAD,   IRV::GEOMETRY,       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "NODE_1_NUMBER",                      {                                                                                                                                                                                              IRV::CONNECTIVITY                } },
    { "NODE_2_NUMBER",                      {                                                                                                                                                                                              IRV::CONNECTIVITY                } },
    { "NODE_SORT",                          {                                                                                                                                                                                              IRV::CONNECTIVITY                } },

    { "PAD_FLASH",                          {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "PADHGHT",                            {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "PADSHAPE1",                          {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "PAD_SHAPE_NAME",                     {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "PAD_STACK_NAME",                     {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "PAD_STACK_SOURCE_NAME",              {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "PAD_TYPE",                           {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "PADWIDTH",                           {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "PADXOFF",                            {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "PADYOFF",                            {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },

    { "PIN_COMMON_CODE",                    {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "PIN_EDITED",                         {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "PIN_NAME",                           {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "PIN_NUMBER",                         {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "PIN_NUMBER_SORT",                    {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "PIN_ROTATION",                       {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "PIN_ROTATION_ABSOLUTE",              {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "PIN_SWAP_CODE",                      {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "PIN_TYPE",                           {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "PIN_X",                              {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "PIN_Y",                              {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    // Not in the documentation, but present in files
    { "PIN_GROUND",                         {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "PIN_POWER",                          {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },

    { "RAT_CONNECTED",                      {                                                                                                                                                                                              IRV::CONNECTIVITY                } },
    { "REC_NUMBER",                         {                                                                                                                       IRV::GEOMETRY,      IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "RECORD_TAG",                         {                                                                                                                       IRV::GEOMETRY,      IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },

    { "REFDES",                             { IRV::COMPONENT,    IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,                           IRV::SYMBOL,                  IRV::CONNECTIVITY                } },
    { "REFDES_SORT",                        { IRV::COMPONENT,    IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,  IRV::SYMBOL,                  IRV::CONNECTIVITY                } },

    { "SEG_CAPACITANCE",                    {                                                                                                                       IRV::GEOMETRY,      IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "SEG_IMPEDANCE",                      {                                                                                                                       IRV::GEOMETRY,      IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "SEG_INDUCTANCE",                     {                                                                                                                       IRV::GEOMETRY,      IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "SEG_PROPAGATION_DELAY",              {                                                                                                                       IRV::GEOMETRY,      IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "SEG_RESISTANCE",                     {                                                                                                                       IRV::GEOMETRY,      IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },

    { "START_LAYER_NAME",                   {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "START_LAYER_NUMBER",                 {                    IRV::COMPONENT_PIN,                    IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,                       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "SUBCLASS",                           {                                                                                                                       IRV::GEOMETRY,      IRV::FULL_GEOMETRY                                                                  } },

    { "SYM_BOX_X1",                         { IRV::COMPONENT,    IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,  IRV::SYMBOL,                  IRV::CONNECTIVITY                } },
    { "SYM_BOX_Y1",                         { IRV::COMPONENT,    IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,  IRV::SYMBOL,                  IRV::CONNECTIVITY                } },
    { "SYM_BOX_X2",                         { IRV::COMPONENT,    IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,  IRV::SYMBOL,                  IRV::CONNECTIVITY                } },
    { "SYM_BOX_Y2",                         { IRV::COMPONENT,    IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,  IRV::SYMBOL,                  IRV::CONNECTIVITY                } },
    { "SYM_CENTER_X",                       { IRV::COMPONENT,    IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,  IRV::SYMBOL,                  IRV::CONNECTIVITY                } },
    { "SYM_CENTER_Y",                       { IRV::COMPONENT,    IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,  IRV::SYMBOL,                  IRV::CONNECTIVITY                } },
    { "SYM_EXTENTS_X1",                     { IRV::COMPONENT,    IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,  IRV::SYMBOL,                  IRV::CONNECTIVITY                } },
    { "SYM_EXTENTS_Y1",                     { IRV::COMPONENT,    IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,  IRV::SYMBOL,                  IRV::CONNECTIVITY                } },
    { "SYM_EXTENTS_X2",                     { IRV::COMPONENT,    IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,  IRV::SYMBOL,                  IRV::CONNECTIVITY                } },
    { "SYM_EXTENTS_Y2",                     { IRV::COMPONENT,    IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,  IRV::SYMBOL,                  IRV::CONNECTIVITY                } },
    { "SYM_HAS_PIN_EDIT",                   { IRV::COMPONENT,    IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,  IRV::SYMBOL,                  IRV::CONNECTIVITY                } },
    { "SYM_MIRROR",                         { IRV::COMPONENT,    IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,  IRV::SYMBOL,                  IRV::CONNECTIVITY                } },
    { "SYM_NAME",                           { IRV::COMPONENT,    IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,  IRV::SYMBOL,                  IRV::CONNECTIVITY                } },
    { "SYM_ROTATE",                         { IRV::COMPONENT,    IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,  IRV::SYMBOL,                  IRV::CONNECTIVITY                } },
    { "SYM_TYPE",                           { IRV::COMPONENT,    IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,  IRV::SYMBOL,                  IRV::CONNECTIVITY                } },
    { "SYM_X",                              { IRV::COMPONENT,    IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,  IRV::SYMBOL,                  IRV::CONNECTIVITY                } },
    { "SYM_Y",                              { IRV::COMPONENT,    IRV::COMPONENT_PIN,   IRV::FUNCTION,   IRV::LOGICAL_PIN,                     IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,  IRV::SYMBOL,                  IRV::CONNECTIVITY                } },

    // These aren't in the Data Dictionary, but they seem to occur in view files
    { "SYM_LIBRARY_PATH",                   {                                                                                                                                                                 IRV::SYMBOL,                                                  } },

    { "TEST_POINT",                         {                    IRV::COMPONENT_PIN,                                                          IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },

    { "TRELFLASH",                          {                    IRV::COMPONENT_PIN,                     IRV::LOGICAL_PIN,                    IRV::COMPOSITE_PAD,                       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "TRELHGHT",                           {                    IRV::COMPONENT_PIN,                     IRV::LOGICAL_PIN,                    IRV::COMPOSITE_PAD,                       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "TRELSHAPE1",                         {                    IRV::COMPONENT_PIN,                     IRV::LOGICAL_PIN,                    IRV::COMPOSITE_PAD,                       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "TRELSHAPENAME",                      {                    IRV::COMPONENT_PIN,                     IRV::LOGICAL_PIN,                    IRV::COMPOSITE_PAD,                       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "TRELWIDTH",                          {                    IRV::COMPONENT_PIN,                     IRV::LOGICAL_PIN,                    IRV::COMPOSITE_PAD,                       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "TRELXOFF",                           {                    IRV::COMPONENT_PIN,                     IRV::LOGICAL_PIN,                    IRV::COMPOSITE_PAD,                       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "TRELYOFF",                           {                    IRV::COMPONENT_PIN,                     IRV::LOGICAL_PIN,                    IRV::COMPOSITE_PAD,                       IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },

    { "VIAFLAG",                            {                                                                                                 IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "VIA_MIRROR",                         {                                                                                                 IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "VIA_X",                              {                                                                                                 IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
    { "VIA_Y",                              {                                                                                                 IRV::COMPOSITE_PAD,   IRV::GEOMETRY,      IRV::FULL_GEOMETRY,                                IRV::CONNECTIVITY                } },
};
// clang-format on


bool ALLEGRO::IsValidFieldForView( const std::string& field_name, EXTRACT_SPEC_PARSER::IR::VIEW_TYPE aViewType )
{
    // Get the set of valid views for this field
    auto it = k_valid_views_for_fields.find( field_name );
    if( it == k_valid_views_for_fields.end() )
    {
        // Unknown field name
        return false;
    }

    const auto& valid_views = it->second;
    return valid_views.count( aViewType ) > 0;
}

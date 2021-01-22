/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2021 KiCad Developers, see change_log.txt for contributors.
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

#include <pin_type.h>
#include <lib_pin.h>
#include <core/arraydim.h>


// These are true singletons so it's OK for them to be globals.

static std::vector<BITMAP_DEF> g_typeIcons;
static wxArrayString           g_typeNames;

static std::vector<BITMAP_DEF> g_shapeIcons;
static wxArrayString           g_shapeNames;

static std::vector<BITMAP_DEF> g_orientationIcons;
static wxArrayString           g_orientationNames;


struct pinTypeStruct
{
    wxString             name;
    const BITMAP_OPAQUE* bitmap;
};

// clang-format off
const std::map<ELECTRICAL_PINTYPE, struct pinTypeStruct> pinTypes = {
    { ELECTRICAL_PINTYPE::PT_INPUT,        { _( "Input" ),             pintype_input_xpm } },
    { ELECTRICAL_PINTYPE::PT_OUTPUT,       { _( "Output" ),            pintype_output_xpm } },
    { ELECTRICAL_PINTYPE::PT_BIDI,         { _( "Bidirectional" ),     pintype_bidi_xpm } },
    { ELECTRICAL_PINTYPE::PT_TRISTATE,     { _( "Tri-state" ),         pintype_3states_xpm } },
    { ELECTRICAL_PINTYPE::PT_PASSIVE,      { _( "Passive" ),           pintype_passive_xpm } },
    { ELECTRICAL_PINTYPE::PT_NIC,          { _( "Free" ),              pintype_nic_xpm } },
    { ELECTRICAL_PINTYPE::PT_UNSPECIFIED,  { _( "Unspecified" ),       pintype_notspecif_xpm } },
    { ELECTRICAL_PINTYPE::PT_POWER_IN,     { _( "Power input" ),       pintype_powerinput_xpm } },
    { ELECTRICAL_PINTYPE::PT_POWER_OUT,    { _( "Power output" ),      pintype_poweroutput_xpm } },
    { ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR,{ _( "Open collector" ),    pintype_opencoll_xpm } },
    { ELECTRICAL_PINTYPE::PT_OPENEMITTER,  { _( "Open emitter" ),      pintype_openemit_xpm } },
    { ELECTRICAL_PINTYPE::PT_NC,           { _( "Unconnected" ),       pintype_noconnect_xpm } },
};
// clang-format on


struct pinShapeStruct
{
    wxString             name;
    const BITMAP_OPAQUE* bitmap;
};


// clang-format off
const std::map<GRAPHIC_PINSHAPE, struct pinShapeStruct> pinShapes = {
    { GRAPHIC_PINSHAPE::LINE,               { _( "Line" ),               pinshape_normal_xpm } },
    { GRAPHIC_PINSHAPE::INVERTED,           { _( "Inverted" ),           pinshape_invert_xpm } },
    { GRAPHIC_PINSHAPE::CLOCK,              { _( "Clock" ),              pinshape_clock_normal_xpm } },
    { GRAPHIC_PINSHAPE::INVERTED_CLOCK,     { _( "Inverted clock" ),     pinshape_clock_invert_xpm } },
    { GRAPHIC_PINSHAPE::INPUT_LOW,          { _( "Input low" ),          pinshape_active_low_input_xpm } },
    { GRAPHIC_PINSHAPE::CLOCK_LOW,          { _( "Clock low" ),          pinshape_clock_active_low_xpm } },
    { GRAPHIC_PINSHAPE::OUTPUT_LOW,         { _( "Output low" ),         pinshape_active_low_output_xpm } },
    { GRAPHIC_PINSHAPE::FALLING_EDGE_CLOCK, { _( "Falling edge clock" ), pinshape_clock_fall_xpm } },
    { GRAPHIC_PINSHAPE::NONLOGIC,           { _( "NonLogic" ),           pinshape_nonlogic_xpm } },
};
// clang-format on


static const int pin_orientation_codes[] =
{
    PIN_RIGHT,
    PIN_LEFT,
    PIN_UP,
    PIN_DOWN
};


// bitmaps to show pins orientations in dialog editor
// must have same order than pin_orientation_names
static const BITMAP_DEF iconsPinsOrientations[] =
{
    pinorient_right_xpm,
    pinorient_left_xpm,
    pinorient_up_xpm,
    pinorient_down_xpm,
};


#define PIN_ORIENTATION_CNT arrayDim( pin_orientation_codes )


// Helper functions to get the pin orientation name from pin_orientation_codes
// Note: the strings are *not* static because they are translated and must be built
// on the fly, to be properly translated

wxString PinOrientationName( unsigned aPinOrientationCode )
{
    /* Note: The following name lists are sentence capitalized per the GNOME UI
     *       standards for list controls.  Please do not change the capitalization
     *       of these strings unless the GNOME UI standards are changed.
     */
    const wxString pin_orientation_names[] =
    {
        _( "Right" ),
        _( "Left" ),
        _( "Up" ),
        _( "Down" ),
        wxT( "???" )
    };

    if( aPinOrientationCode > PIN_ORIENTATION_CNT )
        aPinOrientationCode = PIN_ORIENTATION_CNT;

    return pin_orientation_names[ aPinOrientationCode ];
}


int PinOrientationCode( int index )
{
    if( index >= 0 && index < (int) PIN_ORIENTATION_CNT )
        return pin_orientation_codes[ index ];

    return PIN_RIGHT;
}


int PinOrientationIndex( int code )
{
    size_t i;

    for( i = 0; i < PIN_ORIENTATION_CNT; i++ )
    {
        if( pin_orientation_codes[i] == code )
            return (int) i;
    }

    return wxNOT_FOUND;
}


void InitTables()
{
    for( unsigned i = 0; i < ELECTRICAL_PINTYPES_TOTAL; ++i )
    {
        g_typeIcons.push_back( ElectricalPinTypeGetBitmap( static_cast<ELECTRICAL_PINTYPE>( i ) ) );
        g_typeNames.push_back( ElectricalPinTypeGetText( static_cast<ELECTRICAL_PINTYPE>( i ) ) );
    }

    for( unsigned i = 0; i < GRAPHIC_PINSHAPES_TOTAL; ++i )
    {
        g_shapeIcons.push_back( PinShapeGetBitmap( static_cast<GRAPHIC_PINSHAPE>( i ) ) );
        g_shapeNames.push_back( PinShapeGetText( static_cast<GRAPHIC_PINSHAPE>( i ) ) );
    }

    for( unsigned i = 0; i < PIN_ORIENTATION_CNT; ++i )
    {
        g_orientationIcons.push_back( iconsPinsOrientations[ i ] );
        g_orientationNames.push_back( PinOrientationName( i ) );
    }
}


const wxArrayString& PinTypeNames()
{
    if( g_typeNames.empty() )
        InitTables();

    return g_typeNames;
}


const std::vector<BITMAP_DEF>& PinTypeIcons()
{
    if( g_typeIcons.empty() )
        InitTables();

    return g_typeIcons;
}


const wxArrayString& PinShapeNames()
{
    if( g_shapeNames.empty() )
        InitTables();

    return g_shapeNames;
}


const std::vector<BITMAP_DEF>& PinShapeIcons()
{
    if( g_shapeIcons.empty() )
        InitTables();

    return g_shapeIcons;
}


const wxArrayString& PinOrientationNames()
{
    if( g_orientationNames.empty() )
        InitTables();

    return g_orientationNames;
}


const std::vector<BITMAP_DEF>& PinOrientationIcons()
{
    if( g_orientationIcons.empty() )
        InitTables();

    return g_orientationIcons;
}


wxString ElectricalPinTypeGetText( ELECTRICAL_PINTYPE aType )
{
    auto findIt = pinTypes.find( aType );

    wxCHECK_MSG( findIt != pinTypes.end(), wxT( "???" ), "Could not find pin type in lookup map" );

    return findIt->second.name;
}


BITMAP_DEF ElectricalPinTypeGetBitmap( ELECTRICAL_PINTYPE aType )
{
    auto findIt = pinTypes.find( aType );

    wxCHECK_MSG( findIt != pinTypes.end(), nullptr, "Could not find pin type in lookup map" );

    return findIt->second.bitmap;
}


wxString PinShapeGetText( GRAPHIC_PINSHAPE aShape )
{
    auto findIt = pinShapes.find( aShape );

    wxCHECK_MSG( findIt != pinShapes.end(), wxT( "?" ), "Could not find pinshape in lookup map" );

    return findIt->second.name;
}


BITMAP_DEF PinShapeGetBitmap( GRAPHIC_PINSHAPE aShape )
{
    auto findIt = pinShapes.find( aShape );

    wxCHECK_MSG( findIt != pinShapes.end(), nullptr, "Could not find pinshape in lookup map" );

    return findIt->second.bitmap;
}



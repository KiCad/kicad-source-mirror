/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <layers_id_colors_and_visibility.h>
#include <pgm_base.h>
#include <settings/color_settings.h>
#include <settings/parameters.h>
#include <settings/settings_manager.h>


extern const char* traceSettings;

///! Update the schema version whenever a migration is required
const int colorsSchemaVersion = 1;


COLOR_SETTINGS::COLOR_SETTINGS( std::string aFilename ) :
        JSON_SETTINGS( std::move( aFilename ), SETTINGS_LOC::COLORS, colorsSchemaVersion ),
        m_overrideSchItemColors( false )
{

    m_params.emplace_back( new PARAM<wxString>( "meta.name", &m_displayName, "KiCad Default" ) );

    std::vector<COLOR4D> default_palette = {
            COLOR4D( RED ),
            COLOR4D( YELLOW ),
            COLOR4D( LIGHTMAGENTA ),
            COLOR4D( LIGHTRED ),
            COLOR4D( CYAN ),
            COLOR4D( GREEN ),
            COLOR4D( BLUE ),
            COLOR4D( DARKGRAY ),
            COLOR4D( MAGENTA ),
            COLOR4D( LIGHTGRAY ),
            COLOR4D( MAGENTA ),
            COLOR4D( RED ),
            COLOR4D( BROWN ),
            COLOR4D( LIGHTGRAY ),
            COLOR4D( BLUE ),
            COLOR4D( GREEN )
            };

    // TODO(JE) in actual usage, how long does the default palette need to be?
    m_params.emplace_back( new PARAM_LIST<COLOR4D>( "palette", &m_Palette, default_palette ) );

    m_params.emplace_back( new PARAM<bool>( "schematic.override_item_colors",
                                            &m_overrideSchItemColors, false ) );

#define CLR( x, y, z ) m_params.emplace_back( new COLOR_MAP_PARAM( x, y, z, &m_colors ) )

    CLR( "schematic.background",        LAYER_SCHEMATIC_BACKGROUND, COLOR4D( WHITE ) );
    CLR( "schematic.brightened",        LAYER_BRIGHTENED,           COLOR4D( PUREMAGENTA ) );
    CLR( "schematic.bus",               LAYER_BUS,                  COLOR4D( BLUE ) );
    CLR( "schematic.bus_junction",      LAYER_BUS_JUNCTION,         COLOR4D( BLUE ) );
    CLR( "schematic.component_body",    LAYER_DEVICE_BACKGROUND,    COLOR4D( LIGHTYELLOW ) );
    CLR( "schematic.component_outline", LAYER_DEVICE,               COLOR4D( RED ) );
    CLR( "schematic.cursor",            LAYER_SCHEMATIC_CURSOR,     COLOR4D( BLACK ) );
    CLR( "schematic.erc_error",         LAYER_ERC_ERR,              COLOR4D( PURERED ).WithAlpha( 0.8 ) );
    CLR( "schematic.erc_warning",       LAYER_ERC_WARN,             COLOR4D( PUREGREEN ).WithAlpha( 0.8 ) );
    CLR( "schematic.fields",            LAYER_FIELDS,               COLOR4D( MAGENTA ) );
    CLR( "schematic.grid",              LAYER_SCHEMATIC_GRID,       COLOR4D( DARKGRAY ) );
    CLR( "schematic.grid_axes",         LAYER_SCHEMATIC_GRID_AXES,  COLOR4D( BLUE ) );
    CLR( "schematic.hidden",            LAYER_HIDDEN,               COLOR4D( LIGHTGRAY ) );
    CLR( "schematic.junction",          LAYER_JUNCTION,             COLOR4D( GREEN ) );
    CLR( "schematic.label_global",      LAYER_GLOBLABEL,            COLOR4D( RED ) );
    CLR( "schematic.label_hier",        LAYER_HIERLABEL,            COLOR4D( BROWN ) );
    CLR( "schematic.label_local",       LAYER_LOCLABEL,             COLOR4D( BLACK ) );
    CLR( "schematic.net_name",          LAYER_NETNAM,               COLOR4D( DARKGRAY ) );
    CLR( "schematic.no_connect",        LAYER_NOCONNECT,            COLOR4D( BLUE ) );
    CLR( "schematic.note",              LAYER_NOTES,                COLOR4D( LIGHTBLUE ) );
    CLR( "schematic.pin",               LAYER_PIN,                  COLOR4D( RED ) );
    CLR( "schematic.pin_name",          LAYER_PINNAM,               COLOR4D( CYAN ) );
    CLR( "schematic.pin_number",        LAYER_PINNUM,               COLOR4D( RED ) );
    CLR( "schematic.reference",         LAYER_REFERENCEPART,        COLOR4D( CYAN ) );
    // Macs look better with a lighter shadow
#ifdef __WXMAC__
    CLR( "schematic.shadow",            LAYER_SELECTION_SHADOWS,    COLOR4D( .78, .92, 1.0, 0.8 ) );
#else
    CLR( "schematic.shadow",            LAYER_SELECTION_SHADOWS,    COLOR4D( .4, .7, 1.0, 0.8 ) );
#endif
    CLR( "schematic.sheet",             LAYER_SHEET,                COLOR4D( MAGENTA ) );
    CLR( "schematic.sheet_background",  LAYER_SHEET_BACKGROUND,     COLOR4D( WHITE ).WithAlpha( 0.0 ) );
    CLR( "schematic.sheet_filename",    LAYER_SHEETFILENAME,        COLOR4D( BROWN ) );
    CLR( "schematic.sheet_fields",      LAYER_SHEETFIELDS,          COLOR4D( MAGENTA ) );
    CLR( "schematic.sheet_label",       LAYER_SHEETLABEL,           COLOR4D( CYAN ) );
    CLR( "schematic.sheet_name",        LAYER_SHEETNAME,            COLOR4D( CYAN ) );
    CLR( "schematic.value",             LAYER_VALUEPART,            COLOR4D( CYAN ) );
    CLR( "schematic.wire",              LAYER_WIRE,                 COLOR4D( GREEN ) );
    CLR( "schematic.worksheet",         LAYER_SCHEMATIC_WORKSHEET,  COLOR4D( RED ) );

    CLR( "gerbview.axes",               LAYER_GERBVIEW_AXES,        COLOR4D( BLUE ) );
    CLR( "gerbview.background",         LAYER_GERBVIEW_BACKGROUND,  COLOR4D( BLACK ) );
    CLR( "gerbview.dcodes",             LAYER_DCODES,               COLOR4D( WHITE ) );
    CLR( "gerbview.grid",               LAYER_GERBVIEW_GRID,        COLOR4D( MAGENTA ) );
    CLR( "gerbview.negative_objects",   LAYER_NEGATIVE_OBJECTS,     COLOR4D( DARKGRAY ) );
    CLR( "gerbview.worksheet",          LAYER_GERBVIEW_WORKSHEET,   COLOR4D( BLUE ) );

    // TODO(JE) New default scheme for GerbView
    for( int i = 0, id = GERBVIEW_LAYER_ID_START;
         id < GERBER_DRAWLAYERS_COUNT + GERBVIEW_LAYER_ID_START; ++i, ++id )
    {
        CLR( "gerbview.layers." + std::to_string( i ), id,
                default_palette[ i % default_palette.size() ] );
    }

    CLR( "board.anchor",                   LAYER_ANCHOR,             COLOR4D( BLUE ) );
    CLR( "board.aux_items",                LAYER_AUX_ITEMS,          COLOR4D( WHITE ) );
    CLR( "board.background",               LAYER_PCB_BACKGROUND,     COLOR4D( BLACK ) );
    CLR( "board.cursor",                   LAYER_CURSOR,             COLOR4D( WHITE ) );
    CLR( "board.drc_error",                LAYER_DRC_ERROR,          COLOR4D( PURERED ) );
    CLR( "board.drc_warning",              LAYER_DRC_WARNING,        COLOR4D( PUREYELLOW ) );
    CLR( "board.footprint_text_back",      LAYER_MOD_TEXT_BK,        COLOR4D( BLUE ) );
    CLR( "board.footprint_text_front",     LAYER_MOD_TEXT_FR,        COLOR4D( LIGHTGRAY ) );
    CLR( "board.footprint_text_invisible", LAYER_MOD_TEXT_INVISIBLE, COLOR4D( LIGHTGRAY ) );
    CLR( "board.grid",                     LAYER_GRID,               COLOR4D( DARKGRAY ) );
    CLR( "board.grid_axes",                LAYER_GRID_AXES,          COLOR4D( LIGHTGRAY ) );
    CLR( "board.no_connect",               LAYER_NO_CONNECTS,        COLOR4D( BLUE ) );
    CLR( "board.pad_back",                 LAYER_PAD_BK,             COLOR4D( GREEN ) );
    CLR( "board.pad_front",                LAYER_PAD_FR,             COLOR4D( RED ) );
    CLR( "board.pad_plated_hole",          LAYER_PADS_PLATEDHOLES,   COLOR4D( YELLOW ) );
    CLR( "board.pad_through_hole",         LAYER_PADS_TH,            COLOR4D( YELLOW ) );
    CLR( "board.plated_hole",              LAYER_NON_PLATEDHOLES,    COLOR4D( YELLOW ) );
    CLR( "board.ratsnest",                 LAYER_RATSNEST,           COLOR4D( WHITE ) );
    CLR( "board.select_overlay",           LAYER_SELECT_OVERLAY,     COLOR4D( DARKRED ) );
    CLR( "board.through_via",              LAYER_VIA_THROUGH,        COLOR4D( LIGHTGRAY ) );
    CLR( "board.via",                      LAYER_VIAS,               COLOR4D( BLACK ) );
    CLR( "board.via_blind_buried",         LAYER_VIA_BBLIND,         COLOR4D( BROWN ) );
    CLR( "board.via_hole",                 LAYER_VIAS_HOLES,         COLOR4D( WHITE ) );
    CLR( "board.via_micro",                LAYER_VIA_MICROVIA,       COLOR4D( CYAN ) );
    CLR( "board.via_through",              LAYER_VIA_THROUGH,        COLOR4D( LIGHTGRAY ) );
    CLR( "board.worksheet",                LAYER_WORKSHEET,          COLOR4D( DARKRED ) );

    CLR( "board.copper.f",      F_Cu,       COLOR4D( RED ) );
    CLR( "board.copper.in1",    In1_Cu,     COLOR4D( YELLOW ) );
    CLR( "board.copper.in2",    In2_Cu,     COLOR4D( LIGHTMAGENTA ) );
    CLR( "board.copper.in3",    In3_Cu,     COLOR4D( LIGHTRED ) );
    CLR( "board.copper.in4",    In4_Cu,     COLOR4D( CYAN ) );
    CLR( "board.copper.in5",    In5_Cu,     COLOR4D( GREEN ) );
    CLR( "board.copper.in6",    In6_Cu,     COLOR4D( BLUE ) );
    CLR( "board.copper.in7",    In7_Cu,     COLOR4D( DARKGRAY ) );
    CLR( "board.copper.in8",    In8_Cu,     COLOR4D( MAGENTA ) );
    CLR( "board.copper.in9",    In9_Cu,     COLOR4D( LIGHTGRAY ) );
    CLR( "board.copper.in10",   In10_Cu,    COLOR4D( MAGENTA ) );
    CLR( "board.copper.in11",   In11_Cu,    COLOR4D( RED ) );
    CLR( "board.copper.in12",   In12_Cu,    COLOR4D( BROWN ) );
    CLR( "board.copper.in13",   In13_Cu,    COLOR4D( LIGHTGRAY ) );
    CLR( "board.copper.in14",   In14_Cu,    COLOR4D( BLUE ) );
    CLR( "board.copper.in15",   In15_Cu,    COLOR4D( GREEN ) );
    CLR( "board.copper.in16",   In16_Cu,    COLOR4D( RED ) );
    CLR( "board.copper.in17",   In17_Cu,    COLOR4D( YELLOW ) );
    CLR( "board.copper.in18",   In18_Cu,    COLOR4D( LIGHTMAGENTA ) );
    CLR( "board.copper.in19",   In19_Cu,    COLOR4D( LIGHTRED ) );
    CLR( "board.copper.in20",   In20_Cu,    COLOR4D( CYAN ) );
    CLR( "board.copper.in21",   In21_Cu,    COLOR4D( GREEN ) );
    CLR( "board.copper.in22",   In22_Cu,    COLOR4D( BLUE ) );
    CLR( "board.copper.in23",   In23_Cu,    COLOR4D( DARKGRAY ) );
    CLR( "board.copper.in24",   In24_Cu,    COLOR4D( MAGENTA ) );
    CLR( "board.copper.in25",   In25_Cu,    COLOR4D( LIGHTGRAY ) );
    CLR( "board.copper.in26",   In26_Cu,    COLOR4D( MAGENTA ) );
    CLR( "board.copper.in27",   In27_Cu,    COLOR4D( RED ) );
    CLR( "board.copper.in28",   In28_Cu,    COLOR4D( BROWN ) );
    CLR( "board.copper.in29",   In29_Cu,    COLOR4D( LIGHTGRAY ) );
    CLR( "board.copper.in30",   In30_Cu,    COLOR4D( BLUE ) );
    CLR( "board.copper.b",      B_Cu,       COLOR4D( GREEN ) );

    CLR( "board.b_adhes",       B_Adhes,    COLOR4D( BLUE ) );
    CLR( "board.f_adhes",       F_Adhes,    COLOR4D( MAGENTA ) );
    CLR( "board.b_paste",       B_Paste,    COLOR4D( LIGHTCYAN ) );
    CLR( "board.f_paste",       F_Paste,    COLOR4D( RED ) );
    CLR( "board.b_silks",       B_SilkS,    COLOR4D( MAGENTA ) );
    CLR( "board.f_silks",       F_SilkS,    COLOR4D( CYAN ) );
    CLR( "board.b_mask",        B_Mask,     COLOR4D( BROWN ) );
    CLR( "board.f_mask",        F_Mask,     COLOR4D( MAGENTA ) );
    CLR( "board.dwgs_user",     Dwgs_User,  COLOR4D( LIGHTGRAY ) );
    CLR( "board.cmts_user",     Cmts_User,  COLOR4D( BLUE ) );
    CLR( "board.eco1_user",     Eco1_User,  COLOR4D( GREEN ) );
    CLR( "board.eco2_user",     Eco2_User,  COLOR4D( YELLOW ) );
    CLR( "board.edge_cuts",     Edge_Cuts,  COLOR4D( YELLOW ) );
    CLR( "board.margin",        Margin,     COLOR4D( LIGHTMAGENTA ) );
    CLR( "board.b_crtyd",       B_CrtYd,    COLOR4D( DARKGRAY ) );
    CLR( "board.f_crtyd",       F_CrtYd,    COLOR4D( LIGHTGRAY ) );
    CLR( "board.b_fab",         B_Fab,      COLOR4D( BLUE ) );
    CLR( "board.f_fab",         F_Fab,      COLOR4D( DARKGRAY ) );

    // Colors for 3D viewer, which are used as defaults unless overridden by the board
    CLR( "3d_viewer.background_bottom", LAYER_3D_BACKGROUND_BOTTOM, COLOR4D( 0.4, 0.4, 0.5, 1.0 ) );
    CLR( "3d_viewer.background_top",    LAYER_3D_BACKGROUND_TOP,    COLOR4D( 0.8, 0.8, 0.9, 1.0 ) );
    CLR( "3d_viewer.board",             LAYER_3D_BOARD,             COLOR4D( 0.2, 0.17, 0.09, 1.0 ) );
    CLR( "3d_viewer.copper",            LAYER_3D_COPPER,            COLOR4D( 0.7, 0.61, 0.0, 1.0 ) );
    CLR( "3d_viewer.silkscreen_bottom", LAYER_3D_SILKSCREEN_BOTTOM, COLOR4D( 0.9, 0.9, 0.9, 1.0 ) );
    CLR( "3d_viewer.silkscreen_top",    LAYER_3D_SILKSCREEN_TOP,    COLOR4D( 0.9, 0.9, 0.9, 1.0 ) );
    CLR( "3d_viewer.soldermask",        LAYER_3D_SOLDERMASK,        COLOR4D( 0.08, 0.2, 0.14, 1.0 ) );
    CLR( "3d_viewer.solderpaste",       LAYER_3D_SOLDERPASTE,       COLOR4D( 0.5, 0.5, 0.5, 1.0 ) );
}


COLOR_SETTINGS::COLOR_SETTINGS( const COLOR_SETTINGS& aOther ) :
        JSON_SETTINGS( aOther.m_filename, SETTINGS_LOC::COLORS, colorsSchemaVersion )
{
    initFromOther( aOther );
}


COLOR_SETTINGS& COLOR_SETTINGS::operator=( const COLOR_SETTINGS &aOther )
{
    m_filename = aOther.m_filename;

    initFromOther( aOther );

    return *this;
}


void COLOR_SETTINGS::initFromOther( const COLOR_SETTINGS& aOther )
{
    m_displayName           = aOther.m_displayName;
    m_overrideSchItemColors = aOther.m_overrideSchItemColors;
    m_colors                = aOther.m_colors;
    m_defaultColors         = aOther.m_defaultColors;

    // Ensure default colors are present
    for( PARAM_BASE* param : aOther.m_params )
    {
        if( COLOR_MAP_PARAM* cmp = dynamic_cast<COLOR_MAP_PARAM*>( param ) )
            m_defaultColors[cmp->GetKey()] = cmp->GetDefault();
    }
}


bool COLOR_SETTINGS::MigrateFromLegacy( wxConfigBase* aCfg )
{
    return false;
}


bool COLOR_SETTINGS::Migrate()
{
    bool ret = true;
    int  filever = at( PointerFromString( "meta.version" ) ).get<int>();

    if( filever == 0 )
    {
        ret &= migrateSchema0to1();

        if( ret )
        {
            ( *this )[PointerFromString( "meta.version" )] = 1;
        }
    }

    return ret;
}


bool COLOR_SETTINGS::migrateSchema0to1()
{
    /**
     * Schema version 0 to 1:
     *
     * - Footprint editor settings are split out into a new file called "ThemeName (Footprints)"
     * - fpedit namespace is removed from the schema
     */

    if( !m_manager )
    {
        wxLogTrace( traceSettings, "Error: COLOR_SETTINGS migration cannot run unmanaged!" );
        return false;
    }

    nlohmann::json::json_pointer board( "/board" );
    nlohmann::json::json_pointer fpedit( "/fpedit" );

    if( !contains( fpedit ) )
    {
        wxLogTrace( traceSettings, "migrateSchema0to1: %s doesn't have fpedit settings; skipping.",
                m_filename );
        return true;
    }

    wxString filename = m_filename + wxT( "_footprints" );

    COLOR_SETTINGS* fpsettings = m_manager->AddNewColorSettings( filename );

    // Start out with a clone
    nlohmann::json::json_pointer root( "" );
    ( *fpsettings )[root] = at( root );

    // Footprint editor now just looks at the "board" namespace
    ( *fpsettings )[board] = fpsettings->at( fpedit );

    fpsettings->erase( "fpedit" );
    fpsettings->Load();
    fpsettings->SetName( fpsettings->GetName() + _( " (Footprints)" ) );
    m_manager->Save( fpsettings );

    // Now we can get rid of our own copy
    erase( "fpedit" );

    return true;
}


COLOR4D COLOR_SETTINGS::GetColor( int aLayer ) const
{
    if( m_colors.count( aLayer ) )
        return m_colors.at( aLayer );

    return COLOR4D::UNSPECIFIED;
}


COLOR4D COLOR_SETTINGS::GetDefaultColor( int aLayer )
{
    if( !m_defaultColors.count( aLayer ) )
    {
        COLOR_MAP_PARAM* p = nullptr;

        for( PARAM_BASE* param : m_params )
        {
            COLOR_MAP_PARAM* cmp = dynamic_cast<COLOR_MAP_PARAM*>( param );

            if( cmp && cmp->GetKey() == aLayer )
                p = cmp;
        }

        if( p )
            m_defaultColors[aLayer] = p->GetDefault();
        else
            m_defaultColors[aLayer] = COLOR4D::UNSPECIFIED;
    }

    return m_defaultColors.at( aLayer );
}


void COLOR_SETTINGS::SetColor( int aLayer, COLOR4D aColor )
{
    m_colors[ aLayer ] = aColor;
}

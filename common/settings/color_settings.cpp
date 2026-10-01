/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <layer_ids.h>
#include <settings/color_settings.h>
#include <settings/json_settings_internals.h>
#include <settings/parameters.h>
#include <settings/settings_manager.h>
#include <wx/log.h>
#include <wx/translation.h>

#include "builtin_color_themes.h"


///! Update the schema version whenever a migration is required
const int colorsSchemaVersion = 6;
const wxString COLOR_SETTINGS::COLOR_BUILTIN_DEFAULT = "_builtin_default";
const wxString COLOR_SETTINGS::COLOR_BUILTIN_CLASSIC = "_builtin_classic";


COLOR_SETTINGS::COLOR_SETTINGS( const wxString& aFilename, bool aAbsolutePath ) :
        JSON_SETTINGS( aFilename, SETTINGS_LOC::COLORS, colorsSchemaVersion ),
        m_overrideSchItemColors( false )
{
    if( aAbsolutePath )
        SetLocation( SETTINGS_LOC::NONE );

    m_params.emplace_back( new PARAM<wxString>( "meta.name", &m_displayName,
                                                wxS( "KiCad Default" ) ) );

    m_params.emplace_back( new PARAM<bool>( "schematic.override_item_colors",
                                            &m_overrideSchItemColors, false ) );

#define EMPLACE_COLOR_PARAM( x, y ) \
    wxASSERT( s_defaultTheme.count( y ) ); \
    m_params.emplace_back( new COLOR_MAP_PARAM( x, y, s_defaultTheme.at( y ), &m_colors ) );

    EMPLACE_COLOR_PARAM( "schematic.anchor",            LAYER_SCHEMATIC_ANCHOR       );
    EMPLACE_COLOR_PARAM( "schematic.aux_items",         LAYER_SCHEMATIC_AUX_ITEMS    );
    EMPLACE_COLOR_PARAM( "schematic.background",        LAYER_SCHEMATIC_BACKGROUND   );
    EMPLACE_COLOR_PARAM( "schematic.hovered",           LAYER_HOVERED                );
    EMPLACE_COLOR_PARAM( "schematic.brightened",        LAYER_BRIGHTENED             );
    EMPLACE_COLOR_PARAM( "schematic.bus",               LAYER_BUS                    );
    EMPLACE_COLOR_PARAM( "schematic.bus_junction",      LAYER_BUS_JUNCTION           );
    EMPLACE_COLOR_PARAM( "schematic.component_body",    LAYER_DEVICE_BACKGROUND      );
    EMPLACE_COLOR_PARAM( "schematic.component_outline", LAYER_DEVICE                 );
    EMPLACE_COLOR_PARAM( "schematic.cursor",            LAYER_SCHEMATIC_CURSOR       );
    EMPLACE_COLOR_PARAM( "schematic.dnp_marker",        LAYER_DNP_MARKER             );
    EMPLACE_COLOR_PARAM( "schematic.excluded_from_sim", LAYER_EXCLUDED_FROM_SIM      );
    EMPLACE_COLOR_PARAM( "schematic.erc_error",         LAYER_ERC_ERR                );
    EMPLACE_COLOR_PARAM( "schematic.erc_warning",       LAYER_ERC_WARN               );
    EMPLACE_COLOR_PARAM( "schematic.erc_exclusion",     LAYER_ERC_EXCLUSION          );
    EMPLACE_COLOR_PARAM( "schematic.fields",            LAYER_FIELDS                 );
    EMPLACE_COLOR_PARAM( "schematic.grid",              LAYER_SCHEMATIC_GRID         );
    EMPLACE_COLOR_PARAM( "schematic.grid_axes",         LAYER_SCHEMATIC_GRID_AXES    );
    EMPLACE_COLOR_PARAM( "schematic.hidden",            LAYER_HIDDEN                 );
    EMPLACE_COLOR_PARAM( "schematic.junction",          LAYER_JUNCTION               );
    EMPLACE_COLOR_PARAM( "schematic.label_global",      LAYER_GLOBLABEL              );
    EMPLACE_COLOR_PARAM( "schematic.label_hier",        LAYER_HIERLABEL              );
    EMPLACE_COLOR_PARAM( "schematic.label_local",       LAYER_LOCLABEL               );
    EMPLACE_COLOR_PARAM( "schematic.netclass_flag",     LAYER_NETCLASS_REFS          );
    EMPLACE_COLOR_PARAM( "schematic.drag_net_collision", LAYER_DRAG_NET_COLLISION    );
    EMPLACE_COLOR_PARAM( "schematic.rule_area",         LAYER_RULE_AREAS             );
    EMPLACE_COLOR_PARAM( "schematic.no_connect",        LAYER_NOCONNECT              );
    EMPLACE_COLOR_PARAM( "schematic.note",              LAYER_NOTES                  );
    EMPLACE_COLOR_PARAM( "schematic.private_note",      LAYER_PRIVATE_NOTES          );
    EMPLACE_COLOR_PARAM( "schematic.note_background",   LAYER_NOTES_BACKGROUND       );
    EMPLACE_COLOR_PARAM( "schematic.pin",               LAYER_PIN                    );
    EMPLACE_COLOR_PARAM( "schematic.pin_name",          LAYER_PINNAM                 );
    EMPLACE_COLOR_PARAM( "schematic.pin_number",        LAYER_PINNUM                 );
    EMPLACE_COLOR_PARAM( "schematic.reference",         LAYER_REFERENCEPART          );
    EMPLACE_COLOR_PARAM( "schematic.shadow",            LAYER_SELECTION_SHADOWS      );
    EMPLACE_COLOR_PARAM( "schematic.sheet",             LAYER_SHEET                  );
    EMPLACE_COLOR_PARAM( "schematic.sheet_background",  LAYER_SHEET_BACKGROUND       );
    EMPLACE_COLOR_PARAM( "schematic.sheet_filename",    LAYER_SHEETFILENAME          );
    EMPLACE_COLOR_PARAM( "schematic.sheet_fields",      LAYER_SHEETFIELDS            );
    EMPLACE_COLOR_PARAM( "schematic.sheet_label",       LAYER_SHEETLABEL             );
    EMPLACE_COLOR_PARAM( "schematic.sheet_name",        LAYER_SHEETNAME              );
    EMPLACE_COLOR_PARAM( "schematic.value",             LAYER_VALUEPART              );
    EMPLACE_COLOR_PARAM( "schematic.wire",              LAYER_WIRE                   );
    EMPLACE_COLOR_PARAM( "schematic.worksheet",         LAYER_SCHEMATIC_DRAWINGSHEET );
    EMPLACE_COLOR_PARAM( "schematic.page_limits",       LAYER_SCHEMATIC_PAGE_LIMITS  );
    EMPLACE_COLOR_PARAM( "schematic.op_voltages",       LAYER_OP_VOLTAGES            );
    EMPLACE_COLOR_PARAM( "schematic.op_currents",       LAYER_OP_CURRENTS            );

    EMPLACE_COLOR_PARAM( "gerbview.axes",               LAYER_GERBVIEW_AXES          );
    EMPLACE_COLOR_PARAM( "gerbview.background",         LAYER_GERBVIEW_BACKGROUND    );
    EMPLACE_COLOR_PARAM( "gerbview.dcodes",             LAYER_DCODES                 );
    EMPLACE_COLOR_PARAM( "gerbview.grid",               LAYER_GERBVIEW_GRID          );
    EMPLACE_COLOR_PARAM( "gerbview.negative_objects",   LAYER_NEGATIVE_OBJECTS       );
    EMPLACE_COLOR_PARAM( "gerbview.worksheet",          LAYER_GERBVIEW_DRAWINGSHEET  );
    EMPLACE_COLOR_PARAM( "gerbview.page_limits",        LAYER_GERBVIEW_PAGE_LIMITS   );

    for( int i = 0, id = GERBVIEW_LAYER_ID_START;
         id < GERBER_DRAWLAYERS_COUNT + GERBVIEW_LAYER_ID_START; ++i, ++id )
    {
        if( !s_defaultTheme.count( id ) )
        {
            wxLogTrace( "colors", "Missing default color for gerbview layer %d", id );
            continue;
        }

        m_params.emplace_back( new COLOR_MAP_PARAM( "gerbview.layers." + std::to_string( i ), id,
                                                    s_defaultTheme.at( id ), &m_colors ) );
    }

    EMPLACE_COLOR_PARAM( "board.anchor",                   LAYER_ANCHOR             );
    EMPLACE_COLOR_PARAM( "board.locked_shadow",            LAYER_LOCKED_ITEM_SHADOW );
    EMPLACE_COLOR_PARAM( "board.conflicts_shadow",         LAYER_CONFLICTS_SHADOW   );
    EMPLACE_COLOR_PARAM( "board.constraint_shadow",        LAYER_CONSTRAINT_SHADOW  );
    EMPLACE_COLOR_PARAM( "board.constraint_under",         LAYER_CONSTRAINT_UNDER   );
    EMPLACE_COLOR_PARAM( "board.constraint_well",          LAYER_CONSTRAINT_WELL    );
    EMPLACE_COLOR_PARAM( "board.constraint_over",          LAYER_CONSTRAINT_OVER    );
    EMPLACE_COLOR_PARAM( "board.aux_items",                LAYER_AUX_ITEMS          );
    EMPLACE_COLOR_PARAM( "board.background",               LAYER_PCB_BACKGROUND     );
    EMPLACE_COLOR_PARAM( "board.cursor",                   LAYER_CURSOR             );
    EMPLACE_COLOR_PARAM( "board.drc_error",                LAYER_DRC_ERROR          );
    EMPLACE_COLOR_PARAM( "board.drc_warning",              LAYER_DRC_WARNING        );
    EMPLACE_COLOR_PARAM( "board.drc_exclusion",            LAYER_DRC_EXCLUSION      );
    EMPLACE_COLOR_PARAM( "board.drc_highlighted",          LAYER_DRC_HIGHLIGHTED    );
    EMPLACE_COLOR_PARAM( "board.grid",                     LAYER_GRID               );
    EMPLACE_COLOR_PARAM( "board.grid_axes",                LAYER_GRID_AXES          );
    EMPLACE_COLOR_PARAM( "board.pad_plated_hole",          LAYER_PAD_PLATEDHOLES    );
    EMPLACE_COLOR_PARAM( "board.plated_hole",              LAYER_NON_PLATEDHOLES    );
    EMPLACE_COLOR_PARAM( "board.ratsnest",                 LAYER_RATSNEST           );
    EMPLACE_COLOR_PARAM( "board.via_hole",                 LAYER_VIA_HOLES          );
    EMPLACE_COLOR_PARAM( "board.via_hole_walls",           LAYER_VIA_HOLEWALLS      );
    EMPLACE_COLOR_PARAM( "board.worksheet",                LAYER_DRAWINGSHEET       );
    EMPLACE_COLOR_PARAM( "board.page_limits",              LAYER_PAGE_LIMITS        );
    EMPLACE_COLOR_PARAM( "board.outline_area",             LAYER_BOARD_OUTLINE_AREA );
    EMPLACE_COLOR_PARAM( "board.track_net_names",          NETNAMES_LAYER_ID_START  );
    EMPLACE_COLOR_PARAM( "board.pad_net_names",            LAYER_PAD_NETNAMES       );
    EMPLACE_COLOR_PARAM( "board.via_net_names",            LAYER_VIA_NETNAMES       );
    EMPLACE_COLOR_PARAM( "board.points",                   LAYER_POINTS             );
    EMPLACE_COLOR_PARAM( "board.subgrids",                 LAYER_SUBGRIDS          );
    EMPLACE_COLOR_PARAM( "board.via_stitching",            LAYER_VIA_STITCHING      );

    EMPLACE_COLOR_PARAM( "board.copper.f",      F_Cu    );
    EMPLACE_COLOR_PARAM( "board.copper.in1",    In1_Cu  );
    EMPLACE_COLOR_PARAM( "board.copper.in2",    In2_Cu  );
    EMPLACE_COLOR_PARAM( "board.copper.in3",    In3_Cu  );
    EMPLACE_COLOR_PARAM( "board.copper.in4",    In4_Cu  );
    EMPLACE_COLOR_PARAM( "board.copper.in5",    In5_Cu  );
    EMPLACE_COLOR_PARAM( "board.copper.in6",    In6_Cu  );
    EMPLACE_COLOR_PARAM( "board.copper.in7",    In7_Cu  );
    EMPLACE_COLOR_PARAM( "board.copper.in8",    In8_Cu  );
    EMPLACE_COLOR_PARAM( "board.copper.in9",    In9_Cu  );
    EMPLACE_COLOR_PARAM( "board.copper.in10",   In10_Cu );
    EMPLACE_COLOR_PARAM( "board.copper.in11",   In11_Cu );
    EMPLACE_COLOR_PARAM( "board.copper.in12",   In12_Cu );
    EMPLACE_COLOR_PARAM( "board.copper.in13",   In13_Cu );
    EMPLACE_COLOR_PARAM( "board.copper.in14",   In14_Cu );
    EMPLACE_COLOR_PARAM( "board.copper.in15",   In15_Cu );
    EMPLACE_COLOR_PARAM( "board.copper.in16",   In16_Cu );
    EMPLACE_COLOR_PARAM( "board.copper.in17",   In17_Cu );
    EMPLACE_COLOR_PARAM( "board.copper.in18",   In18_Cu );
    EMPLACE_COLOR_PARAM( "board.copper.in19",   In19_Cu );
    EMPLACE_COLOR_PARAM( "board.copper.in20",   In20_Cu );
    EMPLACE_COLOR_PARAM( "board.copper.in21",   In21_Cu );
    EMPLACE_COLOR_PARAM( "board.copper.in22",   In22_Cu );
    EMPLACE_COLOR_PARAM( "board.copper.in23",   In23_Cu );
    EMPLACE_COLOR_PARAM( "board.copper.in24",   In24_Cu );
    EMPLACE_COLOR_PARAM( "board.copper.in25",   In25_Cu );
    EMPLACE_COLOR_PARAM( "board.copper.in26",   In26_Cu );
    EMPLACE_COLOR_PARAM( "board.copper.in27",   In27_Cu );
    EMPLACE_COLOR_PARAM( "board.copper.in28",   In28_Cu );
    EMPLACE_COLOR_PARAM( "board.copper.in29",   In29_Cu );
    EMPLACE_COLOR_PARAM( "board.copper.in30",   In30_Cu );
    EMPLACE_COLOR_PARAM( "board.copper.b",      B_Cu    );

    EMPLACE_COLOR_PARAM( "board.b_adhes",       B_Adhes   );
    EMPLACE_COLOR_PARAM( "board.f_adhes",       F_Adhes   );
    EMPLACE_COLOR_PARAM( "board.b_paste",       B_Paste   );
    EMPLACE_COLOR_PARAM( "board.f_paste",       F_Paste   );
    EMPLACE_COLOR_PARAM( "board.b_silks",       B_SilkS   );
    EMPLACE_COLOR_PARAM( "board.f_silks",       F_SilkS   );
    EMPLACE_COLOR_PARAM( "board.b_mask",        B_Mask    );
    EMPLACE_COLOR_PARAM( "board.f_mask",        F_Mask    );
    EMPLACE_COLOR_PARAM( "board.dwgs_user",     Dwgs_User );
    EMPLACE_COLOR_PARAM( "board.cmts_user",     Cmts_User );
    EMPLACE_COLOR_PARAM( "board.eco1_user",     Eco1_User );
    EMPLACE_COLOR_PARAM( "board.eco2_user",     Eco2_User );
    EMPLACE_COLOR_PARAM( "board.edge_cuts",     Edge_Cuts );
    EMPLACE_COLOR_PARAM( "board.margin",        Margin    );
    EMPLACE_COLOR_PARAM( "board.b_crtyd",       B_CrtYd   );
    EMPLACE_COLOR_PARAM( "board.f_crtyd",       F_CrtYd   );
    EMPLACE_COLOR_PARAM( "board.b_fab",         B_Fab     );
    EMPLACE_COLOR_PARAM( "board.f_fab",         F_Fab     );
    EMPLACE_COLOR_PARAM( "board.user_1",        User_1    );
    EMPLACE_COLOR_PARAM( "board.user_2",        User_2    );
    EMPLACE_COLOR_PARAM( "board.user_3",        User_3    );
    EMPLACE_COLOR_PARAM( "board.user_4",        User_4    );
    EMPLACE_COLOR_PARAM( "board.user_5",        User_5    );
    EMPLACE_COLOR_PARAM( "board.user_6",        User_6    );
    EMPLACE_COLOR_PARAM( "board.user_7",        User_7    );
    EMPLACE_COLOR_PARAM( "board.user_8",        User_8    );
    EMPLACE_COLOR_PARAM( "board.user_9",        User_9    );
    EMPLACE_COLOR_PARAM( "board.user_10",       User_10   );
    EMPLACE_COLOR_PARAM( "board.user_11",       User_11   );
    EMPLACE_COLOR_PARAM( "board.user_12",       User_12   );
    EMPLACE_COLOR_PARAM( "board.user_13",       User_13   );
    EMPLACE_COLOR_PARAM( "board.user_14",       User_14   );
    EMPLACE_COLOR_PARAM( "board.user_15",       User_15   );
    EMPLACE_COLOR_PARAM( "board.user_16",       User_16   );
    EMPLACE_COLOR_PARAM( "board.user_17",       User_17   );
    EMPLACE_COLOR_PARAM( "board.user_18",       User_18   );
    EMPLACE_COLOR_PARAM( "board.user_19",       User_19   );
    EMPLACE_COLOR_PARAM( "board.user_20",       User_20   );
    EMPLACE_COLOR_PARAM( "board.user_21",       User_21   );
    EMPLACE_COLOR_PARAM( "board.user_22",       User_22   );
    EMPLACE_COLOR_PARAM( "board.user_23",       User_23   );
    EMPLACE_COLOR_PARAM( "board.user_24",       User_24   );
    EMPLACE_COLOR_PARAM( "board.user_25",       User_25   );
    EMPLACE_COLOR_PARAM( "board.user_26",       User_26   );
    EMPLACE_COLOR_PARAM( "board.user_27",       User_27   );
    EMPLACE_COLOR_PARAM( "board.user_28",       User_28   );
    EMPLACE_COLOR_PARAM( "board.user_29",       User_29   );
    EMPLACE_COLOR_PARAM( "board.user_30",       User_30   );
    EMPLACE_COLOR_PARAM( "board.user_31",       User_31   );
    EMPLACE_COLOR_PARAM( "board.user_32",       User_32   );
    EMPLACE_COLOR_PARAM( "board.user_33",       User_33   );
    EMPLACE_COLOR_PARAM( "board.user_34",       User_34   );
    EMPLACE_COLOR_PARAM( "board.user_35",       User_35   );
    EMPLACE_COLOR_PARAM( "board.user_36",       User_36   );
    EMPLACE_COLOR_PARAM( "board.user_37",       User_37   );
    EMPLACE_COLOR_PARAM( "board.user_38",       User_38   );
    EMPLACE_COLOR_PARAM( "board.user_39",       User_39   );
    EMPLACE_COLOR_PARAM( "board.user_40",       User_40   );
    EMPLACE_COLOR_PARAM( "board.user_41",       User_41   );
    EMPLACE_COLOR_PARAM( "board.user_42",       User_42   );
    EMPLACE_COLOR_PARAM( "board.user_43",       User_43   );
    EMPLACE_COLOR_PARAM( "board.user_44",       User_44   );
    EMPLACE_COLOR_PARAM( "board.user_45",       User_45   );

    // Colors for 3D viewer, which are used as defaults unless overridden by the board
    EMPLACE_COLOR_PARAM( "3d_viewer.background_bottom", LAYER_3D_BACKGROUND_BOTTOM );
    EMPLACE_COLOR_PARAM( "3d_viewer.background_top",    LAYER_3D_BACKGROUND_TOP    );
    EMPLACE_COLOR_PARAM( "3d_viewer.board",             LAYER_3D_BOARD             );
    EMPLACE_COLOR_PARAM( "3d_viewer.copper",            LAYER_3D_COPPER_TOP        );
    EMPLACE_COLOR_PARAM( "3d_viewer.silkscreen_bottom", LAYER_3D_SILKSCREEN_BOTTOM );
    EMPLACE_COLOR_PARAM( "3d_viewer.silkscreen_top",    LAYER_3D_SILKSCREEN_TOP    );
    EMPLACE_COLOR_PARAM( "3d_viewer.soldermask_bottom", LAYER_3D_SOLDERMASK_BOTTOM );
    EMPLACE_COLOR_PARAM( "3d_viewer.soldermask_top",    LAYER_3D_SOLDERMASK_TOP    );
    EMPLACE_COLOR_PARAM( "3d_viewer.solderpaste",       LAYER_3D_SOLDERPASTE       );
    EMPLACE_COLOR_PARAM( "3d_viewer.adhesive",          LAYER_3D_ADHESIVE          );
    EMPLACE_COLOR_PARAM( "3d_viewer.user_comments",     LAYER_3D_USER_COMMENTS     );
    EMPLACE_COLOR_PARAM( "3d_viewer.user_drawings",     LAYER_3D_USER_DRAWINGS     );
    EMPLACE_COLOR_PARAM( "3d_viewer.user_eco1",         LAYER_3D_USER_ECO1         );
    EMPLACE_COLOR_PARAM( "3d_viewer.user_eco2",         LAYER_3D_USER_ECO2         );
    EMPLACE_COLOR_PARAM( "3d_viewer.front_fab",         LAYER_3D_F_FAB             );
    EMPLACE_COLOR_PARAM( "3d_viewer.back_fab",          LAYER_3D_B_FAB             );
    EMPLACE_COLOR_PARAM( "3d_viewer.front_courtyard",   LAYER_3D_F_COURTYARD       );
    EMPLACE_COLOR_PARAM( "3d_viewer.back_courtyard",    LAYER_3D_B_COURTYARD       );

    for( int layer = LAYER_3D_USER_1; layer <= LAYER_3D_USER_45; ++layer )
    {
        int          idx = layer - LAYER_3D_USER_1;
        PCB_LAYER_ID pcb_layer = Map3DLayerToPCBLayer( layer );

        m_params.emplace_back( new COLOR_MAP_PARAM( "3d_viewer.user_" + std::to_string( idx + 1 ),
                                                    layer, s_defaultTheme.at( pcb_layer ),
                                                    &m_colors ) );
    }

    m_params.emplace_back( new COLOR_MAP_PARAM( "3d_viewer.f_fab", LAYER_3D_F_FAB,
                                                s_defaultTheme.at( F_Fab ), &m_colors ) );
    m_params.emplace_back( new COLOR_MAP_PARAM( "3d_viewer.b_fab", LAYER_3D_B_FAB,
                                                s_defaultTheme.at( B_Fab ), &m_colors ) );
    m_params.emplace_back( new COLOR_MAP_PARAM( "3d_viewer.f_courtyard", LAYER_3D_F_COURTYARD,
                                                s_defaultTheme.at( F_CrtYd ), &m_colors ) );
    m_params.emplace_back( new COLOR_MAP_PARAM( "3d_viewer.b_courtyard", LAYER_3D_B_COURTYARD,
                                                s_defaultTheme.at( B_CrtYd ), &m_colors ) );

    registerMigration( 0, 1, std::bind( &COLOR_SETTINGS::migrateSchema0to1, this ) );

    registerMigration( 1, 2,
            [&]()
            {
                // Fix LAYER_VIA_HOLES color - before version 2, this setting had no effect
                nlohmann::json::json_pointer ptr( "/board/via_hole" );

                ( *m_internals )[ptr] = COLOR4D( 0.5, 0.4, 0, 0.8 ).ToCSSString();

                return true;
            } );

    registerMigration( 2, 3,
            [&]()
            {
                // We don't support opacity in some 3D colors but some versions of 5.99 let
                // you set it.

                for( std::string path : { "3d_viewer.background_top",
                                          "3d_viewer.background_bottom",
                                          "3d_viewer.copper",
                                          "3d_viewer.silkscreen_top",
                                          "3d_viewer.silkscreen_bottom",
                                          "3d_viewer.solderpaste" } )
                {
                    if( std::optional<COLOR4D> optval = Get<COLOR4D>( path ) )
                        Set( path, optval->WithAlpha( 1.0 ) );
                }

                return true;
            } );

    registerMigration( 3, 4,
            [&]()
            {
                if( std::optional<COLOR4D> optval = Get<COLOR4D>( "board.grid" ) )
                    Set( "board.page_limits",  *optval );

                if( std::optional<COLOR4D> optval = Get<COLOR4D>( "schematic.grid" ) )
                    Set( "schematic.page_limits", *optval );

                return true;
            } );

    // this bump shouldn't have happened; add a no-op migration to avoid future issues
    registerMigration( 4, 5, []() { return true; } );

    registerMigration( 5, 6,
            [&]()
            {
                Set( "board.drc_highlighted", COLOR4D( PUREMAGENTA ) );
                return true;
            } );
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
    m_writeFile             = aOther.m_writeFile;

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
        wxLogTrace( traceSettings, wxT( "Error: COLOR_SETTINGS migration cannot run unmanaged!" ) );
        return false;
    }

    if( !Contains( "fpedit" ) )
    {
        wxLogTrace( traceSettings,
                    wxT( "migrateSchema0to1: %s doesn't have fpedit settings; skipping." ),
                    m_filename );
        return true;
    }

    wxString filename = GetFilename().BeforeLast( '.' ) + wxT( "_footprints" );

    COLOR_SETTINGS* fpsettings = m_manager->AddNewColorSettings( filename );
    fpsettings->SetLocation( GetLocation() );

    // Start out with a clone
    fpsettings->m_internals->CloneFrom( *m_internals );

    // Footprint editor now just looks at the "board" namespace
    fpsettings->Set( "board", fpsettings->At( "fpedit" ) );

    fpsettings->Internals()->erase( "fpedit" );
    fpsettings->Load();
    fpsettings->SetName( fpsettings->GetName() + wxS( " " ) + _( "(Footprints)" ) );
    m_manager->Save( fpsettings );

    // Now we can get rid of our own copy
    m_internals->erase( "fpedit" );

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
        else if( IsCopperLayer( aLayer ) )
            m_defaultColors[aLayer] = s_copperColors[aLayer % s_copperColors.size()];
        else
            m_defaultColors[aLayer] = s_userColors[aLayer % s_userColors.size()];
    }

    return m_defaultColors.at( aLayer );
}


void COLOR_SETTINGS::SetColor( int aLayer, const COLOR4D& aColor )
{
    m_colors[ aLayer ] = aColor;
}


std::vector<COLOR_SETTINGS*> COLOR_SETTINGS::CreateBuiltinColorSettings()
{
    COLOR_SETTINGS* defaultTheme = new COLOR_SETTINGS( COLOR_BUILTIN_DEFAULT );
    defaultTheme->SetName( _( "KiCad Default" ) );
    defaultTheme->m_writeFile = false;
    defaultTheme->Load();   // We can just get the colors out of the param defaults for this one

    COLOR_SETTINGS* classicTheme = new COLOR_SETTINGS( COLOR_BUILTIN_CLASSIC );
    classicTheme->SetName( _( "KiCad Classic" ) );
    classicTheme->m_writeFile = false;

    for( PARAM_BASE* param : classicTheme->m_params )
        delete param;

    classicTheme->m_params.clear(); // Disable load/store

    for( const std::pair<int, COLOR4D> entry : s_classicTheme )
        classicTheme->m_colors[entry.first] = entry.second;

    std::vector<COLOR_SETTINGS*> ret;

    ret.push_back( defaultTheme );
    ret.push_back( classicTheme );

    return ret;
}

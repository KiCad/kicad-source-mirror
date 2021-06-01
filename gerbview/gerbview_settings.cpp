/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <gerbview_settings.h>
#include <layers_id_colors_and_visibility.h>
#include <pgm_base.h>
#include <settings/common_settings.h>
#include <settings/parameters.h>
#include <settings/settings_manager.h>
#include <wx/config.h>


///! Update the schema version whenever a migration is required
const int gerbviewSchemaVersion = 0;


GERBVIEW_SETTINGS::GERBVIEW_SETTINGS() :
        APP_SETTINGS_BASE( "gerbview", gerbviewSchemaVersion ),
        m_Appearance(),
        m_BoardLayersCount( 2 )
{
    // Init settings:
    m_params.emplace_back( new PARAM<bool>( "appearance.show_border_and_titleblock",
            &m_Appearance.show_border_and_titleblock, false ) );

    m_params.emplace_back( new PARAM<bool>( "appearance.show_dcodes",
            &m_Appearance.show_dcodes, false ) );

    m_params.emplace_back( new PARAM<bool>( "appearance.show_negative_objects",
            &m_Appearance.show_negative_objects, false ) );

    m_params.emplace_back( new PARAM<wxString>( "appearance.page_type",
            &m_Appearance.page_type, "GERBER" ) );

    m_params.emplace_back( new PARAM_LIST<wxString>( "system.drill_file_history",
            &m_DrillFileHistory, {} ) );

    m_params.emplace_back( new PARAM_LIST<wxString>( "system.zip_file_history",
            &m_ZipFileHistory, {} ) );

    m_params.emplace_back( new PARAM_LIST<wxString>( "system.job_file_history",
            &m_JobFileHistory, {} ) );

    m_params.emplace_back( new PARAM_LIST<int>( "gerber_to_pcb_layers",
            &m_GerberToPcbLayerMapping, {} ) );

    m_params.emplace_back( new PARAM<int>( "gerber_to_pcb_copperlayers_count",
            &m_BoardLayersCount, 2 ) );
}


bool GERBVIEW_SETTINGS::MigrateFromLegacy( wxConfigBase* aCfg )
{
    bool ret = APP_SETTINGS_BASE::MigrateFromLegacy( aCfg );

    ret &= fromLegacy<bool>( aCfg,
        "ShowBorderAndTitleBlock", "appearance.show_border_and_titleblock" );

    ret &= fromLegacy<bool>( aCfg, "ShowDCodesOpt",          "appearance.show_dcodes" );
    ret &= fromLegacy<bool>( aCfg, "ShowNegativeObjectsOpt", "appearance.show_negative_objects" );
    ret &= fromLegacyString( aCfg, "PageSizeOpt",            "appearance.page_type" );

    auto migrate_files = [&] ( const std::string& aPath, const std::string& aDest )
    {
        int max_history_size = Pgm().GetCommonSettings()->m_System.file_history_size;
        wxString file, key;
        nlohmann::json js = nlohmann::json::array();

        aCfg->SetPath( aPath );

        for( int i = 0; i < max_history_size; i++ )
        {
            key.Printf( "file%d", i );
            file = aCfg->Read( key, wxEmptyString );

            if( !file.IsEmpty() )
                js.emplace_back( file.ToStdString() );
        }

        aCfg->SetPath( ".." );

        ( *this )[PointerFromString( aDest )] = js;
    };

    migrate_files( "drl_files", "system.drill_file_history" );
    migrate_files( "zip_files", "system.zip_file_history" );
    migrate_files( "job_files", "system.job_file_history" );

    {
        wxString key;
        int value = 0;
        nlohmann::json::json_pointer ptr = PointerFromString( "gerber_to_pcb_layers" );

        ( *this )[ptr] = nlohmann::json::array();

        for( int i = 0; i < GERBER_DRAWLAYERS_COUNT; i++ )
        {
            key.Printf( "GbrLyr%dToPcb", i );
            aCfg->Read( key, &value, UNSELECTED_LAYER );
            ( *this )[ptr].emplace_back( value );
        }
    }

    COLOR_SETTINGS* cs = Pgm().GetSettingsManager().GetMigratedColorSettings();

    auto migrateLegacyColor = [&] ( const std::string& aKey, int aLayerId ) {
        wxString str;

        if( aCfg->Read( aKey, &str ) )
            cs->SetColor( aLayerId, COLOR4D( str ) );
    };

    migrateLegacyColor( "BackgroundColorEx",       LAYER_GERBVIEW_BACKGROUND );
    migrateLegacyColor( "DCodeColorEx",            LAYER_DCODES );
    migrateLegacyColor( "GridColorEx",             LAYER_GERBVIEW_GRID );
    migrateLegacyColor( "NegativeObjectsColorEx",  LAYER_NEGATIVE_OBJECTS );
    migrateLegacyColor( "WorksheetColorEx",        LAYER_GERBVIEW_DRAWINGSHEET );

    wxString key;

    for( int i = 0, id = GERBVIEW_LAYER_ID_START;
         id < GERBER_DRAWLAYERS_COUNT + GERBVIEW_LAYER_ID_START; ++i, ++id )
    {
        key.Printf( "ColorLayer%dEx", i );
        migrateLegacyColor( key.ToStdString(), id );
    }

    Pgm().GetSettingsManager().SaveColorSettings( cs, "gerbview" );

    ( *this )[PointerFromString( "appearance.color_theme" )] = cs->GetFilename();

    return ret;
}

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

#include <gerbview_settings.h>
#include <layer_ids.h>
#include <pgm_base.h>
#include <settings/common_settings.h>
#include <settings/json_settings_internals.h>
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

    m_params.emplace_back( new PARAM<bool>( "appearance.show_page_limit",
            &m_Display.m_DisplayPageLimits, false ) );

    m_params.emplace_back( new PARAM<double>( "appearance.mode_opacity_value",
            &m_Display.m_OpacityModeAlphaValue, 0.6 ) );

    // WARNING: any change to this key MUST be reflected in GetFileHistories()
    m_params.emplace_back( new PARAM_LIST<wxString>( "system.drill_file_history",
            &m_DrillFileHistory, {} ) );

    // WARNING: any change to this key MUST be reflected in GetFileHistories()
    m_params.emplace_back( new PARAM_LIST<wxString>( "system.zip_file_history",
            &m_ZipFileHistory, {} ) );

    // WARNING: any change to this key MUST be reflected in GetFileHistories()
    m_params.emplace_back( new PARAM_LIST<wxString>( "system.job_file_history",
            &m_JobFileHistory, {} ) );

    m_params.emplace_back( new PARAM_LIST<int>( "gerber_to_pcb_layers",
            &m_GerberToPcbLayerMapping, {} ) );

    m_params.emplace_back( new PARAM<int>( "gerber_to_pcb_copperlayers_count",
            &m_BoardLayersCount, 2 ) );

    m_params.emplace_back( new PARAM<bool>( "excellon_defaults.unit_mm",
            &m_ExcellonDefaults.m_UnitsMM, false ) );

    m_params.emplace_back( new PARAM<bool>( "excellon_defaults.lz_format",
            &m_ExcellonDefaults.m_LeadingZero, true ) );

    m_params.emplace_back( new PARAM<int>( "excellon_defaults.mm_integer_len",
            &m_ExcellonDefaults.m_MmIntegerLen, FMT_INTEGER_MM, 2 , 6 ) );

    m_params.emplace_back( new PARAM<int>( "excellon_defaults.mm_mantissa_len",
            &m_ExcellonDefaults.m_MmMantissaLen, FMT_MANTISSA_MM, 2 , 6 ) );

    m_params.emplace_back( new PARAM<int>( "excellon_defaults.inch_integer_len",
            &m_ExcellonDefaults.m_InchIntegerLen, FMT_INTEGER_INCH, 2 , 6 ) );

    m_params.emplace_back( new PARAM<int>( "excellon_defaults.inch_mantissa_len",
            &m_ExcellonDefaults.m_InchMantissaLen, FMT_MANTISSA_INCH, 2 , 6 ) );
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
            key.Printf( wxT( "file%d" ), i );
            file = aCfg->Read( key, wxEmptyString );

            if( !file.IsEmpty() )
                js.emplace_back( file.ToStdString() );
        }

        aCfg->SetPath( wxT( ".." ) );

        Set( aDest, js );
    };

    migrate_files( "drl_files", "system.drill_file_history" );
    migrate_files( "zip_files", "system.zip_file_history" );
    migrate_files( "job_files", "system.job_file_history" );

    {
        wxString key;
        int value = 0;

        Set( "gerber_to_pcb_layers", nlohmann::json::array() );

        for( int i = 0; i < GERBER_DRAWLAYERS_COUNT; i++ )
        {
            key.Printf( wxT( "GbrLyr%dToPcb" ), i );
            aCfg->Read( key, &value, UNSELECTED_LAYER );
            At( "gerber_to_pcb_layers" ).emplace_back( value );
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
    migrateLegacyColor( "GridColorEx",             LAYER_GERBVIEW_PAGE_LIMITS );

    wxString key;

    for( int i = 0, id = GERBVIEW_LAYER_ID_START;
         id < GERBER_DRAWLAYERS_COUNT + GERBVIEW_LAYER_ID_START; ++i, ++id )
    {
        key.Printf( wxT( "ColorLayer%dEx" ), i );
        migrateLegacyColor( key.ToStdString(), id );
    }

    Pgm().GetSettingsManager().SaveColorSettings( cs, "gerbview" );

    Set( "appearance.color_theme", cs->GetFilename() );

    return ret;
}


std::map<std::string, nlohmann::json> GERBVIEW_SETTINGS::GetFileHistories()
{
    std::map<std::string, nlohmann::json> histories = JSON_SETTINGS::GetFileHistories();

    for( const std::string& candidate : { std::string("system.drill_file_history"),
                                          std::string("system.zip_file_history"),
                                          std::string("system.job_file_history") } )
    {
        if( Contains( candidate ) )
            histories[candidate] = GetJson( candidate ).value();
    }

    return histories;
}


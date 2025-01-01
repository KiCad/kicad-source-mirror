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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _COLOR_SETTINGS_H
#define _COLOR_SETTINGS_H

#include <unordered_map>

#include <gal/color4d.h>
#include <settings/json_settings.h>
#include <settings/parameters.h>

using KIGFX::COLOR4D;

/**
 * Color settings are a bit different than most of the settings objects in that there
 * can be more than one of them loaded at once.
 *
 * Each COLOR_SETTINGS object corresponds to a single color scheme file on disk.
 * The default color scheme is called "default" and will be created on first run.
 *
 * When users change color settings, they have the option of modifying the default scheme
 * or saving their changes to a new color scheme file.
 *
 * Each COLOR_SETTINGS defines all the settings used across all parts of KiCad, but it's not
 * necessary for the underlying theme file to contain all of them.  The color settings cascade,
 * so if a user chooses a non-default scheme for a certain application, and that non-default
 * scheme file is missing some color definitions, those will fall back to those from the "default"
 * scheme (which is either loaded from disk or created if missing)
 *
 * Each application (eeschema, gerbview, pcbnew) can have a different active color scheme selected.
 * The "child applications" (library editors) inherit from either eeschema or pcbnew.
 */
class KICOMMON_API COLOR_SETTINGS : public JSON_SETTINGS
{
public:
    explicit COLOR_SETTINGS( const wxString& aFilename = wxT( "user" ),
                             bool aAbsolutePath = false );

    virtual ~COLOR_SETTINGS() {}

    /**
     * Copy ctor provided for temporary manipulation of themes in the theme editor.
     * This will not copy the JSON_SETTINGS underlying data.
     */
    COLOR_SETTINGS( const COLOR_SETTINGS& aOther );

    COLOR_SETTINGS& operator=( const COLOR_SETTINGS &aOther );

    bool MigrateFromLegacy( wxConfigBase* aCfg ) override;

    COLOR4D GetColor( int aLayer ) const;

    COLOR4D GetDefaultColor( int aLayer );

    void SetColor( int aLayer, const COLOR4D& aColor );

    const wxString& GetName() const { return m_displayName; }
    void SetName( const wxString& aName ) { m_displayName = aName; }

    bool GetOverrideSchItemColors() const { return m_overrideSchItemColors; }
    void SetOverrideSchItemColors( bool aFlag ) { m_overrideSchItemColors = aFlag; }

    /**
     * Constructs and returns a list of color settings objects based on the built-in color themes.
     * These color settings are not backed by a file and cannot be modified by the user.
     * This is expected to be called by SETTINGS_MANAGER which will take ownership of the objects
     * and handle freeing them at the end of its lifetime.
     * @return a list of pointers COLOR_SETTINGS objects containing the default color theme(s)
     */
    static std::vector<COLOR_SETTINGS*> CreateBuiltinColorSettings();

    // Names for the built-in color settings
    static const wxString COLOR_BUILTIN_DEFAULT;
    static const wxString COLOR_BUILTIN_CLASSIC;

private:
    bool migrateSchema0to1();

    void initFromOther( const COLOR_SETTINGS& aOther );

private:
    wxString m_displayName;
    bool     m_overrideSchItemColors;

    /**
     * Map of all layer colors.
     * The key needs to be a valid layer ID, see layer_ids.h
     */
    std::unordered_map<int, COLOR4D> m_colors;

    std::unordered_map<int, COLOR4D> m_defaultColors;
};

class COLOR_MAP_PARAM : public PARAM_BASE
{
public:
    COLOR_MAP_PARAM( const std::string& aJsonPath, int aMapKey, COLOR4D aDefault,
                     std::unordered_map<int, COLOR4D>* aMap, bool aReadOnly = false ) :
            PARAM_BASE( aJsonPath, aReadOnly ), m_key( aMapKey ), m_default( aDefault ),
            m_map( aMap )
    {}

    void Load( const JSON_SETTINGS& aSettings, bool aResetIfMissing = true ) const override
    {
        if( m_readOnly )
            return;

        if( std::optional<COLOR4D> optval = aSettings.Get<COLOR4D>( m_path ) )
            ( *m_map )[ m_key ] = *optval;
        else if( aResetIfMissing )
            ( *m_map )[ m_key ] = m_default;
    }

    void Store( JSON_SETTINGS* aSettings ) const override
    {
        aSettings->Set<COLOR4D>( m_path, ( *m_map )[ m_key ] );
    }

    int GetKey() const
    {
        return m_key;
    }

    COLOR4D GetDefault() const
    {
        return m_default;
    }

    void SetDefault() override
    {
        ( *m_map )[ m_key ] = m_default;
    }

    bool MatchesFile( const JSON_SETTINGS& aSettings ) const override
    {
        if( std::optional<COLOR4D> optval = aSettings.Get<COLOR4D>( m_path ) )
            return m_map->count( m_key ) && ( *optval == m_map->at( m_key ) );

        // If the JSON doesn't exist, the map shouldn't exist either
        return !m_map->count( m_key );
    }

private:
    int m_key;

    COLOR4D m_default;

    std::unordered_map<int, COLOR4D>* m_map;
};

#endif

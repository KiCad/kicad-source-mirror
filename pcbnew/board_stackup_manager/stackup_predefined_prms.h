/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

/**
 * @file stackup_predefined_prms.h
 */

#ifndef STACKUP_PREDEFINED_PRMS_H
#define STACKUP_PREDEFINED_PRMS_H


#include <wx/string.h>
#include <wx/arrstr.h>
#include <wx/colour.h>

#include <layer_ids.h>
#include <i18n_utility.h>       // For _HKI definition
#include <gal/color4d.h>

#include <board_stackup_manager/board_stackup.h>

// Keyword used in file to identify the dielectric layer type
#define KEY_CORE wxT(  "core" )
#define KEY_PREPREG wxT( "prepreg" )

#define KEY_COPPER wxT( "copper" )

// key string used for not specified parameters
// Can be translated in dialogs, and is also a keyword outside dialogs
wxString inline NotSpecifiedPrm()
{
    return _HKI( "Not specified" );
}

/**
 * @return true if the param value is specified:
 * not empty
 * not NotSpecifiedPrm() value or its translation
 */
bool IsPrmSpecified( const wxString& aPrmValue );

#define DEFAULT_SOLDERMASK_OPACITY 0.83

// A reasonable Epsilon R value for solder mask dielectric
#define DEFAULT_EPSILON_R_SOLDERMASK 3.3

// A default Epsilon R value for silkscreen dielectric
#define DEFAULT_EPSILON_R_SILKSCREEN 1.0

// A minor struct to handle color in gerber job file and dialog
class FAB_LAYER_COLOR
{
public:
    FAB_LAYER_COLOR()
    {}

    FAB_LAYER_COLOR( const wxString& aColorName, const wxColor& aColor ) :
        m_colorName( aColorName ),
        m_color( aColor )
    {}

    const wxString& GetName() const
    {
        return m_colorName;
    }

    KIGFX::COLOR4D GetColor( BOARD_STACKUP_ITEM_TYPE aItemType ) const
    {
        if( aItemType == BS_ITEM_TYPE_SOLDERMASK )
            return m_color.WithAlpha( DEFAULT_SOLDERMASK_OPACITY );
        else
            return m_color.WithAlpha( 1.0 );
    }

    /**
     * @return a color name acceptable in gerber job file
     * one of normalized color name, or the string R<integer>G<integer>B<integer>
     * integer is a decimal value from 0 to 255
     */
    const wxString GetColorAsString() const;

private:
    wxString        m_colorName;    // the name (in job file) of the color
                                    // User values are the HTML encoded "#rrggbbaa"
                                    // RGB hexa value.
    KIGFX::COLOR4D  m_color;
};


/**
 * @return a wxArray of standard copper finish names.
 * @param aTranslate = false for the initial names, true for translated names
 */
wxArrayString GetStandardCopperFinishes( bool aTranslate );

/**
 * @return a list of standard FAB_LAYER_COLOR items for silkscreen and solder mask.
 */
const std::vector<FAB_LAYER_COLOR>& GetStandardColors( BOARD_STACKUP_ITEM_TYPE aType );

/**
 * @return the index of the user defined color in ColorStandardList
 */
int GetColorUserDefinedListIdx( BOARD_STACKUP_ITEM_TYPE aType );

inline KIGFX::COLOR4D GetDefaultUserColor( BOARD_STACKUP_ITEM_TYPE aType )
{
    return GetStandardColors( aType )[GetColorUserDefinedListIdx( aType )].GetColor( aType );
}

inline KIGFX::COLOR4D GetStandardColor( BOARD_STACKUP_ITEM_TYPE aType, int aIdx )
{
    return GetStandardColors( aType )[ aIdx ].GetColor( aType );
}

inline const wxString& GetStandardColorName( BOARD_STACKUP_ITEM_TYPE aType, int aIdx )
{
    return GetStandardColors( aType )[ aIdx ].GetName();
}

inline bool IsCustomColorIdx( BOARD_STACKUP_ITEM_TYPE aType, int aIdx )
{
    return aIdx == GetColorUserDefinedListIdx( aType );
}

/**
 * @return true if aName is a color name acceptable in gerber job files
 * @param aName is a color name like red, blue... (case insensitive)
 */
bool IsColorNameNormalized( const wxString& aName );


#endif      // #ifndef STACKUP_PREDEFINED_PRMS_H

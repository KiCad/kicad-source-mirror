/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <layers_id_colors_and_visibility.h>

// Keyword used in file to identify the dielectric layer type
#define KEY_CORE "core"
#define KEY_PREPREG "prepreg"

#define KEY_COPPER "copper"

// key string used for not specified parameters
#define NOT_SPECIFIED "Undefined"

// String in wxChoice to use user defined material or solder mask color
#define USER_DEFINED "user defined"

// A minor struct to handle color in gerber job file and dialog
struct FAB_LAYER_COLOR
{
    wxString m_ColorName;   // the name (in job file) of the color
                            // User values are the HTML coding #rrggbb hexa value.
    wxColor m_Color;        // the color in r,g,b values (0..255)

    FAB_LAYER_COLOR() {}
    FAB_LAYER_COLOR( const wxString& aColorName, const wxColor& aColor )
        : m_ColorName( aColorName ), m_Color( aColor )
    {}
};


// A minor struct to handle substrates prms in gerber job file and dialog
struct FAB_SUBSTRATE
{
    wxString m_Name;            // the name (in job file) of material
    double m_EpsilonR;          // the epsilon r of this material
    double m_LossTangent;       // the loss tangent (tanD) of this material
    wxString FormatEpsilonR();  // return a wxString to print/display Epsilon R
    wxString FormatLossTangent();// return a wxString to print/display Loss Tangent
};


// A struct to handle a list of substrates prms in gerber job file and dialogs
class FAB_SUBSTRATE_LIST
{
    ///> The list of available substrates. It contians at least predefined substrates
    std::vector<FAB_SUBSTRATE> m_substrateList;

public:
    FAB_SUBSTRATE_LIST();

    /**
     * @return the number of substrates in list
     */
    int GetCount() { return (int)m_substrateList.size(); }

    /**
     * @return the substrate in list of index aIdx
     *  if incorrect return nullptr
     * @param aIdx is the index in substrate list.
     */
    FAB_SUBSTRATE* GetSubstrate( int aIdx );

    /**
     * @return the substrate in list of name aName
     *  if not found return nullptr
     * @param aName is the name of the substrate in substrate list.
     * the comparison is case insensitve
     */
    FAB_SUBSTRATE* GetSubstrate( const wxString& aName );
};


/**
 * @return a wxArray of standard copper finish names.
 * @param aTranslate = false for the initial names, true for translated names
 */
wxArrayString GetCopperFinishStandardList( bool aTranslate );

/**
 * @return a list of standard FAB_LAYER_COLOR items for silkscreen and solder mask.
 */
const FAB_LAYER_COLOR* GetColorStandardList();

/**
 * @return the count of colors in ColorStandardList
 */
int GetColorStandardListCount();

/**
 * @return the index of the user defined color in ColorStandardList
 */
int GetColorUserDefinedListIdx();

#endif      // #ifndef STACKUP_PREDEFINED_PRMS_H

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

// key string used for not specified parameters
#define NOT_SPECIFIED "Undefined"

// A minor struct to handle color in gerber job file and dialog
struct FAB_LAYER_COLOR
{
    wxString m_ColorName;   // the name (in job file) of the color
    wxColor m_Color;        // the color in r,g,b values (0..255)
};


// A minor struct to handle substrates prms in gerber job file and dialog
struct FAB_SUBSTRATE
{
    wxString m_Name;        // the name (in job file) of material
    double m_EpsilonR;      // the epsilon r of this material
    double m_Loss;          // the loss (tanD) of this material
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
 * @return a list of standard material items for dielectric.
 */
const FAB_SUBSTRATE* GetSubstrateMaterialStandardList();

#endif      // #ifndef STACKUP_PREDEFINED_PRMS_H

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __dialog_enum_pads__
#define __dialog_enum_pads__

/**
 * @file dialog_enum_pads.h
 */

#include "dialog_enum_pads_base.h"

#include <optional>

/**
 * @brief Parameters for sequential pad numbering
 *
 * This is used by, say, the pad enumeration dialog to pass parameters to the
 * pad enumeration tool for the sequential numbering mode.
 */
struct SEQUENTIAL_PAD_ENUMERATION_PARAMS
{
    SEQUENTIAL_PAD_ENUMERATION_PARAMS() : m_start_number( 1 ), m_step( 1 ) {}

    /// @brief Starting number for pad names
    int m_start_number;

    /// @brief Step between pad numbers
    int m_step;

    /// @brief Optional prefix for pad names
    std::optional<wxString> m_prefix;
};


/**
 * @brief Dialog for enumerating pads
 *
 * This dialog allows the user to set parameters for the sequential
 * pad re-numbering tool.
 */
class DIALOG_ENUM_PADS : public DIALOG_ENUM_PADS_BASE
{
public:
    DIALOG_ENUM_PADS( wxWindow* aParent, SEQUENTIAL_PAD_ENUMERATION_PARAMS& aParams );

private:
    /// @brief Transfer data from the dialog to the params
    bool TransferDataFromWindow() override;

    /// @brief The parameters that will be updated when the dialog is closed with OK
    SEQUENTIAL_PAD_ENUMERATION_PARAMS& m_params;
};

#endif // __dialog_enum_pads__

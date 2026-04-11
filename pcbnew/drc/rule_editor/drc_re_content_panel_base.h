/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DRC_RE_CLASSES_H_
#define DRC_RE_CLASSES_H_

#include <wx/arrstr.h>
#include <wx/bitmap.h>
#include <wx/chartype.h>


#include <algorithm>
#include <vector>
#include <board_design_settings.h>

#include "drc_rule_editor_enums.h"

class DRC_RULE_EDITOR_CONTENT_PANEL_BASE
{
public:
    DRC_RULE_EDITOR_CONTENT_PANEL_BASE() = default;

    virtual ~DRC_RULE_EDITOR_CONTENT_PANEL_BASE() = default;

    virtual bool TransferDataToWindow() = 0;
    virtual bool TransferDataFromWindow() = 0;

    virtual bool ValidateInputs( int* aErrorCount, wxString* aValidationMessage ) = 0;

    virtual wxString GenerateRule( const RULE_GENERATION_CONTEXT& aContext ) = 0;
};

#endif // DRC_RE_CLASSES_H_

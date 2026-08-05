/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef GEOM_FIELD_HELPERS_H
#define GEOM_FIELD_HELPERS_H

#include <memory>
#include <vector>

#include <origin_transforms.h>

class EDA_DRAW_FRAME;
class UNIT_BINDER;
class wxGridBagSizer;
class wxString;
class wxTextCtrl;

struct BOUND_CONTROL
{
    std::unique_ptr<UNIT_BINDER> m_Binder;
    wxTextCtrl*                  m_Ctrl;
};

void AddXYPointToSizer( EDA_DRAW_FRAME& aFrame, wxGridBagSizer& aSizer, int row, int col,
                        const wxString& aName, bool aRelative,
                        std::vector<BOUND_CONTROL>& aBoundCtrls );

void AddFieldToSizer( EDA_DRAW_FRAME& aFrame, wxGridBagSizer& aSizer, int row, int col,
                      const wxString& aName, ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType,
                      bool aIsAngle, std::vector<BOUND_CONTROL>& aBoundCtrls );

#endif

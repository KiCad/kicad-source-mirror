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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wx/propgrid/property.h>


/**
 * Enhanced renderer to work around some limitations in wxWidgets 3.0 capabilities
 */
class PG_CELL_RENDERER : public wxPGDefaultRenderer
{
public:
    PG_CELL_RENDERER();
    virtual ~PG_CELL_RENDERER() = default;

    bool Render( wxDC &aDC, const wxRect &aRect, const wxPropertyGrid *aGrid,
                 wxPGProperty *aProperty, int aColumn, int aItem, int aFlags ) const override;
};
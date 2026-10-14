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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <widgets/properties_panel.h>

class PL_EDITOR_FRAME;
class PL_PAGE_PROPERTIES;
class PL_ITEM_PROPERTIES;
class PG_UNIT_EDITOR;

class PL_PROPERTIES_PANEL : public PROPERTIES_PANEL
{
public:
    explicit PL_PROPERTIES_PANEL( PL_EDITOR_FRAME* aFrame );
    ~PL_PROPERTIES_PANEL() override;

    void UpdateData() override;

protected:
    void          rebuildProperties( const SELECTION& aSelection ) override;
    wxPGProperty* createPGProperty( const PROPERTY_BASE* aProperty ) const override;
    void          valueChanging( wxPropertyGridEvent& aEvent ) override;
    void          valueChanged( wxPropertyGridEvent& aEvent ) override;

private:
    void resetDefaults();
    void refreshDrawing();

    PL_EDITOR_FRAME*                                 m_editor;
    PG_UNIT_EDITOR*                                  m_unitEditor;
    std::unique_ptr<PL_PAGE_PROPERTIES>              m_pageProperties;
    std::vector<std::unique_ptr<PL_ITEM_PROPERTIES>> m_itemProperties;
    SELECTION                                        m_selection;
};

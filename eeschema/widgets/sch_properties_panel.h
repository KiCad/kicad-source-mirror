/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SCH_PROPERTIES_PANEL_H
#define SCH_PROPERTIES_PANEL_H

#include <widgets/properties_panel.h>

class SELECTION;
class SCHEMATIC;
class SCH_BASE_FRAME;
class PROPERTY_MANAGER;
class PG_UNIT_EDITOR;
class PG_CHECKBOX_EDITOR;
class PG_COLOR_EDITOR;

class SCH_PROPERTIES_PANEL : public PROPERTIES_PANEL
{
public:
    SCH_PROPERTIES_PANEL( wxWindow* aParent, SCH_BASE_FRAME* aFrame );

    virtual ~SCH_PROPERTIES_PANEL();

    void UpdateData() override;

    void AfterCommit() override;

protected:
    wxPGProperty* createPGProperty( const PROPERTY_BASE* aProperty ) const override;

    PROPERTY_BASE* getPropertyFromEvent( const wxPropertyGridEvent& aEvent ) const;

    void valueChanging( wxPropertyGridEvent& aEvent ) override;
    void valueChanged( wxPropertyGridEvent& aEvent ) override;

    ///< Regenerates caches of list properties
    void updateLists( const SCHEMATIC& aSchematic );

    SCH_BASE_FRAME* m_frame;
    PROPERTY_MANAGER& m_propMgr;
    PG_UNIT_EDITOR* m_unitEditorInstance;
    PG_CHECKBOX_EDITOR* m_checkboxEditorInstance;
    PG_COLOR_EDITOR* m_colorEditorInstance;

    wxPGChoices m_nets;
};

#endif /* PCB_PROPERTIES_PANEL_H */

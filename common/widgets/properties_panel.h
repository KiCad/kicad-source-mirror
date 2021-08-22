/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */
#ifndef PROPERTIES_PANEL_H
#define PROPERTIES_PANEL_H

#include <wx/panel.h>
#include <wx/propgrid/propgrid.h>

#include <vector>

class EDA_BASE_FRAME;
class SELECTION;
class PROPERTY_BASE;
class wxStaticText;

class PROPERTIES_PANEL : public wxPanel
{
public:
    PROPERTIES_PANEL( wxWindow* aParent, EDA_BASE_FRAME* aFrame );

    virtual ~PROPERTIES_PANEL()
    {
    }

    virtual void UpdateData() = 0;

    wxPropertyGrid* GetPropertyGrid()
    {
        return m_grid;
    }

    int PropertiesCount() const
    {
        return m_displayed.size();
    }

    const std::vector<PROPERTY_BASE*>& Properties() const
    {
        return m_displayed;
    }

protected:
    virtual void update( const SELECTION& aSelection );
    virtual wxPGProperty* createPGProperty( const PROPERTY_BASE* aProperty ) const = 0;

    // Event handlers
    virtual void valueChanging( wxPropertyGridEvent& aEvent ) {}
    virtual void valueChanged( wxPropertyGridEvent& aEvent ) {}
    void onShow( wxShowEvent& aEvent );

    std::vector<PROPERTY_BASE*> m_displayed;
    wxPropertyGrid* m_grid;
    EDA_BASE_FRAME* m_frame;
    wxStaticText* m_caption;
};

#endif /* PROPERTIES_PANEL_H */

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
#include <memory>

class EDA_BASE_FRAME;
class EDA_ITEM;
class SELECTION;
class PROPERTY_BASE;
class wxStaticText;

class PROPERTIES_PANEL : public wxPanel
{
public:
    PROPERTIES_PANEL( wxWindow* aParent, EDA_BASE_FRAME* aFrame );

    virtual ~PROPERTIES_PANEL();

    virtual void UpdateData() = 0;

    virtual void AfterCommit() {}

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

    void RecalculateSplitterPos();

    void SetSplitterProportion( float aProportion );
    float SplitterProportion() const { return m_splitter_key_proportion; }

protected:
    /**
     * Generates the property grid for a given selection of items.
     *
     * @param aSelection is a set of items to show properties for.
     */
    virtual void rebuildProperties( const SELECTION& aSelection );

    virtual wxPGProperty* createPGProperty( const PROPERTY_BASE* aProperty ) const = 0;

    // Event handlers
    virtual void valueChanging( wxPropertyGridEvent& aEvent ) { aEvent.Skip(); }
    virtual void valueChanged( wxPropertyGridEvent& aEvent ) { aEvent.Skip(); }
    void onCharHook( wxKeyEvent& aEvent );
    void onShow( wxShowEvent& aEvent );

    virtual void OnLanguageChanged( wxCommandEvent& aEvent );

    /**
     * Utility to fetch a property value and convert to wxVariant
     * Precondition: aItem is known to have property aProperty
     * @return true if conversion succeeded
     */
    bool getItemValue( EDA_ITEM* aItem, PROPERTY_BASE* aProperty, wxVariant& aValue );

    /**
     * Processes a selection and determines whether the given property should be available or not
     * and what the common value should be for the items in the selection.
     * @param aSelection is a set of EDA_ITEMs to process
     * @param aProperty is the property to look up
     * @param aValue will be filled with the value common to the selection, or null if different
     * @param aWritable will be set to whether or not the property can be written for the selection
     * @return true if the property is available for all the items in the selection
     */
    bool extractValueAndWritability( const SELECTION& aSelection, const wxString& aPropName,
                                     wxVariant& aValue, bool& aWritable, wxPGChoices& aChoices );

public:
    int                         m_SuppressGridChangeEvents;

protected:
    std::vector<PROPERTY_BASE*> m_displayed;    // no ownership of pointers
    wxPropertyGrid*             m_grid;
    EDA_BASE_FRAME*             m_frame;
    wxStaticText*               m_caption;

    /// Proportion of the grid column splitter that is used for the key column (0.0 - 1.0)
    float m_splitter_key_proportion;
};

#endif /* PROPERTIES_PANEL_H */

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PCB_PROPERTIES_PANEL_H
#define PCB_PROPERTIES_PANEL_H

#include <widgets/properties_panel.h>
#include <set>

class SELECTION;
class BOARD;
class PCB_BASE_EDIT_FRAME;
class PROPERTY_MANAGER;
class PG_UNIT_EDITOR;
class PG_CHECKBOX_EDITOR;
class PG_RATIO_EDITOR;
class PG_NET_SELECTOR_EDITOR;
class PG_FPID_EDITOR;
class PG_URL_EDITOR;

class PCB_PROPERTIES_PANEL : public PROPERTIES_PANEL
{
public:
    PCB_PROPERTIES_PANEL( wxWindow* aParent, PCB_BASE_EDIT_FRAME* aFrame );

    virtual ~PCB_PROPERTIES_PANEL();

    void UpdateData() override;

    void AfterCommit() override;

protected:
    void rebuildProperties( const SELECTION& aSelection ) override;
    wxPGProperty* createPGProperty( const PROPERTY_BASE* aProperty ) const override;
    bool getItemValue( EDA_ITEM* aItem, PROPERTY_BASE* aProperty, wxVariant& aValue ) override;

    PROPERTY_BASE* getPropertyFromEvent( const wxPropertyGridEvent& aEvent ) const;

    void valueChanging( wxPropertyGridEvent& aEvent ) override;
    void valueChanged( wxPropertyGridEvent& aEvent ) override;

    ///< Regenerates caches storing layer and net names
    void updateLists( const BOARD* aBoard );

    /**
     * Get the current selection from the selection tool.
     * If the selection is empty and we're in the footprint editor, returns the footprint instead.
     *
     * @param aSelection [out] reference to a SELECTION pointer that will be set to the selection
     * @param aFallbackSelection [out] local SELECTION object for fallback footprint selection
     * @return const SELECTION& reference to the selection (either real selection or fallback)
     */
    const SELECTION& getSelection( SELECTION& aFallbackSelection );

    /**
     * Get the front item of the current selection.
     * If the selection is empty and we're in the footprint editor, returns the footprint instead.
     *
     * @return EDA_ITEM* pointer to the front item, or nullptr if no selection
     */
    EDA_ITEM* getFrontItem();

protected:
    PCB_BASE_EDIT_FRAME* m_frame;
    PROPERTY_MANAGER&    m_propMgr;
    PG_UNIT_EDITOR*      m_unitEditorInstance;
    PG_CHECKBOX_EDITOR*  m_checkboxEditorInstance;
    PG_RATIO_EDITOR*     m_ratioEditorInstance;
    PG_NET_SELECTOR_EDITOR* m_netSelectorEditorInstance;
    PG_FPID_EDITOR*      m_fpEditorInstance;
    PG_URL_EDITOR*       m_urlEditorInstance;

    static std::set<wxString> m_currentFieldNames;
    wxPGChoices m_nets;
};

#endif /* PCB_PROPERTIES_PANEL_H */

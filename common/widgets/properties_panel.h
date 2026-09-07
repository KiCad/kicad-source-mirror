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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef PROPERTIES_PANEL_H
#define PROPERTIES_PANEL_H

#include <wx/panel.h>
#include <wx/propgrid/propgrid.h>

#include <bitmaps/bitmaps_list.h>
#include <vector>
#include <memory>
#include <functional>

class EDA_BASE_FRAME;
class EDA_ITEM;
class SELECTION;
class PROPERTY_BASE;
class BITMAP_BUTTON;
class wxPropertyGridEvent;
class wxStaticText;

enum PROPERTIES_PANEL_CONTEXT_MENU_IDS
{
    ID_CTX_ADD_FIELD = wxID_HIGHEST + 1000,
    ID_CTX_ADD_CUSTOM_PROPERTY,
    ID_CTX_REMOVE_FIELD,
    ID_CTX_REMOVE_CUSTOM_PROPERTY,
};

class PROPERTIES_PANEL : public wxPanel
{
    friend class PROPERTIES_PANEL_GRID;

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

    virtual bool isKeyEditable( const wxPGProperty* aPGProp ) const { return false; }
    virtual bool isKeyNameInUse( const wxString& aName ) const { return false; }
    virtual void onKeyRenamed( const wxString& aOldName, const wxString& aNewName ) {}

    virtual void onLabelEditBegin( wxPropertyGridEvent& aEvent );
    virtual void onLabelEditEnding( wxPropertyGridEvent& aEvent );

    virtual bool buildContextMenu( wxMenu& aMenu, wxPGProperty* aPGProp ) { return false; }
    virtual void onNewItemLeftBlank( const wxString& aKey ) {}
    void onRightClick( wxPropertyGridEvent& aEvent );

    void beginLabelEdit( const wxString& aKey, bool aStartBlank = false );

    /// Synchronously ends any in-progress label edit; potentially canceling a newly added row
    void settlePendingLabelEdit();

    /**
     * Utility to fetch a property value and convert to wxVariant
     * Precondition: aItem is known to have property aProperty
     * @return true if conversion succeeded
     */
    virtual bool getItemValue( EDA_ITEM* aItem, PROPERTY_BASE* aProperty, wxVariant& aValue );

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

    /**
     * Registers an overlay button on a category caption row.  The button is shown
     * when its category row is on-screen and selection-appropriate (the optional
     * enable predicate is consulted; it is re-evaluated on every rebuild).
     *
     * TODO it would be nice to remove this hack by getting upstream wxWidgets to support
     * customizing/subclassing the property group widgets
     *
     * @param aGroupKey is the untranslated group name (e.g. _HKI( "Fields" )).
     * @param aTooltip is the tooltip text (translated by the caller).
     * @param aBitmap is the icon shown on the button.
     * @param aAction is invoked when the button is clicked.
     * @param aEnableFunc optionally controls visibility (evaluated on each rebuild).
     * @param aForceCategory optionally forces the category caption row to be emitted
     *        during rebuilds (only when aEnableFunc, if set, returns true).
     */
    void addCategoryButton( const wxString& aGroupKey, const wxString& aTooltip,
                            BITMAPS aBitmap, std::function<void()> aAction,
                            std::function<bool()> aEnableFunc = nullptr,
                            bool aForceCategory = false );

    /// Updates overlay button visibility/positions; called after rebuilds and on scroll.
    void updateCategoryButtons();

    /// Hides all overlay buttons (used when no selection or a rebuild is deferred).
    void hideCategoryButtons();

    void positionCategoryButtons();

    ///< Find the caption row for the given untranslated group name, if present.
    wxPGProperty* categoryForGroup( const wxString& aGroupKey ) const;

    wxStaticText*               m_caption;

public:
    int                         m_SuppressGridChangeEvents;

protected:
    std::vector<PROPERTY_BASE*> m_displayed;    // no ownership of pointers
    wxPropertyGrid*             m_grid;
    EDA_BASE_FRAME*             m_frame;

    /// Proportion of the grid column splitter that is used for the key column (0.0 - 1.0)
    float m_splitter_key_proportion;

    wxString m_editingOriginalLabel;

    wxString m_contextMenuPropertyName;

    struct CATEGORY_BUTTON
    {
        BITMAP_BUTTON*           button;
        wxString                 groupKey;      ///< untranslated group name of host category
        std::function<void()>    action;
        std::function<bool()>    enableFunc;    ///< optional extra enable predicate

        ///< Emit the category caption row even when it has no properties; the row is
        ///< only forced on rebuilds where enableFunc() (if set) returns true.
        bool                     forceCategory = false;
    };

    std::vector<CATEGORY_BUTTON> m_categoryButtons;

    /// True while settling a pending label edit synchronously (see settlePendingLabelEdit).
    bool m_resolvingPendingKey = false;

    /// Key of a freshly-added blank field/custom property awaiting a name from the user.
    wxString m_pendingNewKey;
};


class SUPPRESS_GRID_CHANGED_EVENTS
{
public:
    SUPPRESS_GRID_CHANGED_EVENTS( PROPERTIES_PANEL* aPanel );
    ~SUPPRESS_GRID_CHANGED_EVENTS();

private:
    PROPERTIES_PANEL* m_panel;
};

#endif /* PROPERTIES_PANEL_H */

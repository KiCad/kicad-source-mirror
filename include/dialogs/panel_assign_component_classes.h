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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#ifndef PANEL_ASSIGN_COMPONENT_CLASSES_H
#define PANEL_ASSIGN_COMPONENT_CLASSES_H


#include <panel_assign_component_classes_base.h>

#include <unordered_set>
#include <vector>

#include <dialog_shim.h>
#include <pcb_edit_frame.h>
#include <project/component_class_settings.h>


/**************************************************************************************************
 *
 * PANEL_ASSIGN_COMPONENT_CLASSES
 *
 *************************************************************************************************/

/**
 * Top-level panel for dynamic component class assignment configuration. Nests a
 * PANEL_COMPONENT_CLASS_ASSIGNMENT panel for each set of component class assignment conditions
 */
class PANEL_COMPONENT_CLASS_ASSIGNMENT;

class PANEL_ASSIGN_COMPONENT_CLASSES : public PANEL_ASSIGN_COMPONENT_CLASSES_BASE
{
public:
    PANEL_ASSIGN_COMPONENT_CLASSES( wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame,
                                    std::shared_ptr<COMPONENT_CLASS_SETTINGS> aSettings,
                                    DIALOG_SHIM*                              aDlg );

    ~PANEL_ASSIGN_COMPONENT_CLASSES() override;

    /// Loads current component class assignments from the board settings
    bool TransferDataToWindow() override;

    /// Saves the component class assignments to the board settings
    bool TransferDataFromWindow() override;

    /// Loads component class assignments from the given settings object
    void ImportSettingsFrom( const std::shared_ptr<COMPONENT_CLASS_SETTINGS>& aOtherSettings );

    /// Adds a new component class assignment rule
    void OnAddAssignmentClick( wxCommandEvent& event ) override;

    /// Removes a given component class assignment rule
    void RemoveAssignment( PANEL_COMPONENT_CLASS_ASSIGNMENT* aPanel );

    /// Returns references for all currently selected footprints
    const std::vector<wxString>& GetSelectionRefs() const { return m_selectionRefs; }

    /// Returns names of all fields present in footprints
    const std::vector<wxString>& GetFieldNames() const { return m_fieldNames; }

    /// Returns names of all sheets present in footprints
    const std::vector<wxString>& GetSheetNames() const { return m_sheetNames; }

    /// Gets the active edit frame
    PCB_EDIT_FRAME* GetFrame() const { return m_frame; }

    /// Validates that all assignment rules can compile successfully
    bool Validate() override;

private:
    /// Adds a new component class assignment rule
    PANEL_COMPONENT_CLASS_ASSIGNMENT* addAssignment();

    /// Scrolls the panel to specified assignment rule
    void scrollToAssignment( const PANEL_COMPONENT_CLASS_ASSIGNMENT* aAssignment ) const;

    /// The parent dialog
    DIALOG_SHIM* m_dlg;

    /// The active edit frame
    PCB_EDIT_FRAME* m_frame;

    /// Vector of all currently displayed assignment rules
    std::vector<PANEL_COMPONENT_CLASS_ASSIGNMENT*> m_assignments;

    /// The active settings object
    std::shared_ptr<COMPONENT_CLASS_SETTINGS> m_componentClassSettings;

    /// All currently selected footprint references
    std::vector<wxString> m_selectionRefs;

    /// All footprint fields names present on the board
    std::vector<wxString> m_fieldNames;

    /// All sheet names present on the board
    std::vector<wxString> m_sheetNames;

    /// The list of all currently present component class assignments
    wxBoxSizer* m_assignmentsList;
};


/**************************************************************************************************
 *
 * CONDITION_DATA
 *
 *************************************************************************************************/

/**
 * Class used to provide a unified interface from condition panels
 * Handles determining the type of condition in use, and getting and settings the field data
 */
class CONDITION_DATA
{
public:
    CONDITION_DATA( const COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE aCondition,
                    wxTextEntry* aPrimary, wxTextEntry* aSecondary = nullptr ) :
            m_conditionType( aCondition ), m_primaryCtrl( aPrimary ), m_secondaryCtrl( aSecondary )
    {
    }

    virtual ~CONDITION_DATA() {};

    /// Gets the type of condition (e.g. Reference, Side) this data represents
    virtual COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE GetConditionType() const
    {
        return m_conditionType;
    }

    /// Gets the primary data member for the condition (e.g. Reference, Side)
    virtual wxString GetPrimaryField() const;

    /// Sets the primary data member for the condition (e.g. Reference, Side)
    virtual void SetPrimaryField( const wxString& aVal );

    /// Gets the primary data member for the condition (e.g. FOOTPRITNT field  value)
    virtual wxString GetSecondaryField() const;

    /// Sets the primary data member for the condition (e.g. FOOTPRITNT field
    /// value)
    virtual void SetSecondaryField( const wxString& aVal );

private:
    /// The type of condition being referenced
    COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE m_conditionType;

    /// The primary data field in the condition panel
    wxTextEntry* m_primaryCtrl;

    /// The Secondary data field in the condition panel
    wxTextEntry* m_secondaryCtrl;
};


/**************************************************************************************************
 *
 * PANEL_COMPONENT_CLASS_ASSIGNMENT
 *
 *************************************************************************************************/

/**
* Panel which configures a set of conditions for a component class assignment rule
*/
class PANEL_COMPONENT_CLASS_ASSIGNMENT : public PANEL_COMPONENT_CLASS_ASSIGNMENT_BASE
{
public:
    /// IDs for match type popup menu
    enum ADD_MATCH_POPUP
    {
        ID_REFERENCE = wxID_HIGHEST + 1,
        ID_FOOTPRINT,
        ID_SIDE,
        ID_ROTATION,
        ID_FOOTPRINT_FIELD,
        ID_SHEET_NAME,
        ID_CUSTOM
    };

    PANEL_COMPONENT_CLASS_ASSIGNMENT( wxWindow*                       aParent,
                                      PANEL_ASSIGN_COMPONENT_CLASSES* aPanelParent,
                                      DIALOG_SHIM*                    aDlg );
    ~PANEL_COMPONENT_CLASS_ASSIGNMENT();

    /// Deletes this component class assignment rule
    void OnDeleteAssignmentClick( wxCommandEvent& event ) override;

    /// Adds a match condition to this component class assignment rule
    void OnAddConditionClick( wxCommandEvent& event ) override;

    /// Highlights footprints matching this set of conditions
    void OnHighlightItemsClick( wxCommandEvent& event ) override;

    /// Adds a condition to this component class assignment rule
    CONDITION_DATA* AddCondition( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE aCondition );

    /// Removes a given condition from this component class assignment rule (note: called from the child
    /// condition panel)
    void RemoveCondition( wxPanel* aMatch );

    const std::vector<CONDITION_DATA*>& GetConditions() const { return m_matches; }

    /// Sets the resulting component class for this assignment
    void SetComponentClass( const wxString& aComponentClass ) const;

    /// Gets the resulting component class for this assignment
    const wxString GetComponentClass() const;

    /// Sets the boolean operator applied to all assignment conditions
    void
    SetConditionsOperator( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITIONS_OPERATOR aCondition ) const;

    /// Gets the boolean operator applied to all assignment conditions
    COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITIONS_OPERATOR GetConditionsOperator() const;

    /// Converts the UI representation in to the internal assignment data representation
    COMPONENT_CLASS_ASSIGNMENT_DATA GenerateAssignmentData() const;

protected:
    /// Handles add match condition popup menu selections
    void onMenu( wxCommandEvent& aEvent );

    /// The top-level configuration panel which owns this assignment rule
    PANEL_ASSIGN_COMPONENT_CLASSES* m_parentPanel;

    /// The sizer containing match condition panels
    wxStaticBoxSizer* m_matchesList;

    /// Set containing all currently configured match condition types
    std::unordered_set<COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE> m_conditionTypes;

    /// The parent configuration dialog
    DIALOG_SHIM* m_dlg;

    /// All match conditions for this component class assignment rule
    std::vector<CONDITION_DATA*> m_matches;
};


/**************************************************************************************************
 *
 * PANEL_COMPONENT_CLASS_CONDITION_REFERENCE
 *
 *************************************************************************************************/

/**
 * Configures matching based on footprint reference
 */
class PANEL_COMPONENT_CLASS_CONDITION_REFERENCE
        : public PANEL_COMPONENT_CLASS_CONDITION_REFERENCE_BASE,
          public CONDITION_DATA
{
public:
    /// IDs for match type popup menu
    enum IMPORT_POPUP_IDS
    {
        ID_IMPORT_REFS = wxID_HIGHEST + 1
    };

    explicit PANEL_COMPONENT_CLASS_CONDITION_REFERENCE( wxWindow* aParent );

    void OnDeleteConditionClick( wxCommandEvent& event ) override;

    void OnImportRefsClick( wxCommandEvent& event ) override;
    void SetSelectionRefs( const std::vector<wxString>& aRefs ) { m_selectionRefs = aRefs; }

protected:
    /// Handles import references popup menu selections
    void onMenu( wxCommandEvent& aEvent );

    PANEL_COMPONENT_CLASS_ASSIGNMENT* m_panelParent;
    std::vector<wxString>             m_selectionRefs;
};


/**************************************************************************************************
 *
 * PANEL_COMPONENT_CLASS_CONDITION_FOOTPRINT
 *
 *************************************************************************************************/

/**
 * Configures matching based on footprint library identifier
 */
class PANEL_COMPONENT_CLASS_CONDITION_FOOTPRINT
        : public PANEL_COMPONENT_CLASS_CONDITION_FOOTPRINT_BASE,
          public CONDITION_DATA
{
public:
    explicit PANEL_COMPONENT_CLASS_CONDITION_FOOTPRINT( wxWindow* aParent, DIALOG_SHIM* aDlg );

    void OnDeleteConditionClick( wxCommandEvent& event ) override;
    void OnShowLibraryClick( wxCommandEvent& event ) override;

protected:
    PANEL_COMPONENT_CLASS_ASSIGNMENT* m_panelParent;
    DIALOG_SHIM* m_dlg;
};


/**************************************************************************************************
 *
 * PANEL_COMPONENT_CLASS_CONDITION_SIDE
 *
 *************************************************************************************************/

/**
 * Configures matching based on which side of the board a footprint is on
 */
class PANEL_COMPONENT_CLASS_CONDITION_SIDE : public PANEL_COMPONENT_CLASS_CONDITION_SIDE_BASE,
                                             public CONDITION_DATA
{
public:
    explicit PANEL_COMPONENT_CLASS_CONDITION_SIDE( wxWindow* aParent );

    void OnDeleteConditionClick( wxCommandEvent& event ) override;

protected:
    PANEL_COMPONENT_CLASS_ASSIGNMENT* m_panelParent;
};


/**************************************************************************************************
 *
 * PANEL_COMPONENT_CLASS_CONDITION_ROTATION
 *
 *************************************************************************************************/

/**
 * Configures matching based on footprint rotation
 */
class PANEL_COMPONENT_CLASS_CONDITION_ROTATION
        : public PANEL_COMPONENT_CLASS_CONDITION_ROTATION_BASE,
          public CONDITION_DATA
{
public:
    explicit PANEL_COMPONENT_CLASS_CONDITION_ROTATION( wxWindow* aParent );

    void OnDeleteConditionClick( wxCommandEvent& event ) override;

protected:
    PANEL_COMPONENT_CLASS_ASSIGNMENT* m_panelParent;
};


/**************************************************************************************************
 *
 * PANEL_COMPONENT_CLASS_CONDITION_FIELD
 *
 *************************************************************************************************/

/**
 * Configures matching based on footprint field contents
 */
class PANEL_COMPONENT_CLASS_CONDITION_FIELD : public PANEL_COMPONENT_CLASS_CONDITION_FIELD_BASE,
                                              public CONDITION_DATA
{
public:
    explicit PANEL_COMPONENT_CLASS_CONDITION_FIELD( wxWindow* aParent );

    void OnDeleteConditionClick( wxCommandEvent& event ) override;

    void SetFieldsList( const std::vector<wxString>& aFields );

protected:
    PANEL_COMPONENT_CLASS_ASSIGNMENT* m_panelParent;
};


/**************************************************************************************************
 *
 * PANEL_COMPONENT_CLASS_CONDITION_CUSTOM
 *
 *************************************************************************************************/

/**
 * Configures matching based on a custom DRC expression
 */
class PANEL_COMPONENT_CLASS_CONDITION_CUSTOM : public PANEL_COMPONENT_CLASS_CONDITION_CUSTOM_BASE,
                                               public CONDITION_DATA
{
public:
    explicit PANEL_COMPONENT_CLASS_CONDITION_CUSTOM( wxWindow* aParent );

    void OnDeleteConditionClick( wxCommandEvent& event ) override;

protected:
    PANEL_COMPONENT_CLASS_ASSIGNMENT* m_panelParent;
};


/**************************************************************************************************
 *
 * PANEL_COMPONENT_CLASS_SHEET
 *
 *************************************************************************************************/

/**
 * Configures matching based on a custom DRC expression
 */
class PANEL_COMPONENT_CLASS_CONDITION_SHEET : public PANEL_COMPONENT_CLASS_CONDITION_SHEET_BASE,
                                              public CONDITION_DATA
{
public:
    explicit PANEL_COMPONENT_CLASS_CONDITION_SHEET( wxWindow* aParent );

    void OnDeleteConditionClick( wxCommandEvent& event ) override;

    void SetSheetsList( const std::vector<wxString>& aSheets );

protected:
    PANEL_COMPONENT_CLASS_ASSIGNMENT* m_panelParent;
};

#endif //PANEL_ASSIGN_COMPONENT_CLASSES_H

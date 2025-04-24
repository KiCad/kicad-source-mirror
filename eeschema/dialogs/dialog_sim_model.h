/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef DIALOG_SIM_MODEL_H
#define DIALOG_SIM_MODEL_H
#include <sim/kibis/kibis.h>

#include <dialog_sim_model_base.h>
#include <netlist_exporter_spice.h>
#include <scintilla_tricks.h>

#include <sim/sim_model.h>
#include <sim/sim_library.h>
#include <sim/sim_library_ibis.h>
#include <sch_symbol.h>

class EMBEDDED_FILES;

// Some probable wxWidgets bugs encountered when writing this class:
// 1. There are rendering problems with wxPropertyGrid on Linux, GTK, Xorg when
//    wxPG_NATIVE_DOUBLE_BUFFERING flag is not set.
// 2. wxPropertyGridManager->ShowHeader() segfaults when called from this dialog's constructor.

template <typename T>
class DIALOG_SIM_MODEL : public DIALOG_SIM_MODEL_BASE
{
public:
    enum PARAM_COLUMN
    {
        DESCRIPTION = 0,
        VALUE,
        UNIT,
        DEFAULT,
        TYPE,
        END_
    };

    enum PIN_COLUMN
    {
        SYMBOL = 0,
        MODEL
    };

    DIALOG_SIM_MODEL( wxWindow* aParent, EDA_BASE_FRAME* aFrame, T& aSymbol,
                      std::vector<SCH_FIELD>& aFields );

    ~DIALOG_SIM_MODEL();

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void updateWidgets();
    void updateIbisWidgets( SIM_MODEL* aModel );
    void updateBuiltinModelWidgets( SIM_MODEL* aModel );
    void updateModelParamsTab( SIM_MODEL* aModel );
    void updateModelCodeTab( SIM_MODEL* aModel );
    void updatePinAssignments( SIM_MODEL* aModel, bool aForceUpdatePins );

    void removeOrphanedPinAssignments( SIM_MODEL* aModel );

    bool loadLibrary( const wxString& aLibraryPath, REPORTER& aReporter,
                      bool aForceReload = false );

    void addParamPropertyIfRelevant( SIM_MODEL* aModel, int aParamIndex );
    wxPGProperty* newParamProperty( SIM_MODEL* aModel, int aParamIndex ) const;

    int findSymbolPinRow( const wxString& aSymbolPinNumber ) const;

    SIM_MODEL& curModel() const;
    const SIM_LIBRARY* library() const;

    wxString getSymbolPinString( int aSymbolPinNumber ) const;
    wxString getModelPinString( SIM_MODEL* aModel, int aModelPinIndex ) const;
    int getModelPinIndex( const wxString& aModelPinString ) const;

    void onRadioButton( wxCommandEvent& aEvent ) override;
    void onLibraryPathText( wxCommandEvent& aEvent ) override;
    void onLibraryPathTextEnter( wxCommandEvent& aEvent ) override;
    void onLibraryPathTextKillFocus( wxFocusEvent& aEvent ) override;
    void onBrowseButtonClick( wxCommandEvent& aEvent ) override;
    void onFilterCharHook( wxKeyEvent& aKeyStroke ) override;
    void onModelFilter( wxCommandEvent& aEvent ) override;
    void onModelNameChoice( wxCommandEvent& aEvent ) override;
    void onPinCombobox( wxCommandEvent& event ) override;
    void onPinComboboxTextEnter( wxCommandEvent& event ) override;
    void onPinModelCombobox( wxCommandEvent& event ) override;
    void onPinModelComboboxTextEnter( wxCommandEvent& event ) override;
    void onDeviceTypeChoice( wxCommandEvent& aEvent ) override;
    void onWaveformChoice( wxCommandEvent& aEvent ) override;
    void onTypeChoice( wxCommandEvent& aEvent ) override;
    void onPageChanging( wxNotebookEvent& event ) override;
    void onPinAssignmentsGridCellChange( wxGridEvent& aEvent ) override;
    void onPinAssignmentsGridSize( wxSizeEvent& aEvent ) override;
    void onDifferentialCheckbox( wxCommandEvent& event ) override;
    void onSizeParamGrid( wxSizeEvent& event ) override;

    void onParamGridSetFocus( wxFocusEvent& aEvent );
    void onParamGridSelectionChange( wxPropertyGridEvent& aEvent );
    void onUpdateUI( wxUpdateUIEvent& aEvent );

    void adjustParamGridColumns( int aWidth, bool aForce );

    bool isIbisLoaded() { return dynamic_cast<const SIM_LIBRARY_IBIS*>( library() ); }

private:
    EDA_BASE_FRAME*              m_frame;
    T&                           m_symbol;
    std::vector<SCH_FIELD>&      m_fields;

    std::vector<EMBEDDED_FILES*> m_filesStack;
    SIM_LIB_MGR                  m_libraryModelsMgr;
    SIM_LIB_MGR                  m_builtinModelsMgr;
    wxString                     m_prevLibrary;
    const SIM_MODEL*             m_prevModel;

    std::map<wxString, int>            m_modelListBoxEntryToLibraryIdx;

    std::vector<SCH_PIN*>                          m_sortedPartPins; ///< Pins of the current part.
    std::map<SIM_MODEL::DEVICE_T, SIM_MODEL::TYPE> m_curModelTypeOfDeviceType;
    SIM_MODEL::TYPE                                m_curModelType;

    SCINTILLA_TRICKS*       m_scintillaTricksCode;
    SCINTILLA_TRICKS*       m_scintillaTricksSubckt;

    wxPGProperty*           m_firstCategory;            // Used to add principal parameters to root.
    wxPGProperty*           m_prevParamGridSelection;

    int                     m_lastParamGridWidth;
};

#endif /* DIALOG_SIM_MODEL_H */

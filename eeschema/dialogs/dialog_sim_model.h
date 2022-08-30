/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialog_sim_model_base.h>
#include <netlist_exporter_spice.h>
#include <scintilla_tricks.h>

#include <sim/sim_model.h>
#include <sim/sim_library.h>
#include <sch_symbol.h>


// Some probable wxWidgets bugs encountered when writing this class:
// 1. There are rendering problems with wxPropertyGrid on Linux, GTK, Xorg when
//    wxPG_NATIVE_DOUBLE_BUFFERING flag is not set.
// 2. wxPropertyGridManager->ShowHeader() segfaults when called from this dialog's constructor.

template <typename T>
class DIALOG_SIM_MODEL : public DIALOG_SIM_MODEL_BASE
{
public:
    class MODEL_NAME_VALIDATOR : public wxTextValidator
    {
    public:
        MODEL_NAME_VALIDATOR() : wxTextValidator( wxFILTER_INCLUDE_LIST )
        {
        }

        wxObject* Clone() const override
        {
            return new MODEL_NAME_VALIDATOR( *this );
        }

        wxString IsValid( const wxString& aStr ) const override
        {
            if( m_includes.Index( aStr ) == wxNOT_FOUND )
                return wxString::Format( _( "No model named '%s' in library." ), aStr );

            return wxString();
        }
    };

    enum class PARAM_COLUMN : int { DESCRIPTION, VALUE, UNIT, DEFAULT, TYPE, END_ };
    enum class PIN_COLUMN : int { SYMBOL, MODEL };

    DIALOG_SIM_MODEL( wxWindow* aParent, SCH_SYMBOL& aSymbol, std::vector<T>& aSchFields );

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void updateWidgets();
    void updateModelParamsTab();
    void updateModelCodeTab();
    void updatePinAssignments();

    void removeOrphanedPinAssignments();

    void loadLibrary( const wxString& aFilePath );
    
    void addParamPropertyIfRelevant( int aParamIndex );
    wxPGProperty* newParamProperty( int aParamIndex ) const;

    int findSymbolPinRow( const wxString& aSymbolPinNumber ) const;

    SIM_MODEL& curModel() const;
    std::shared_ptr<SIM_MODEL> curModelSharedPtr() const;

    wxString getSymbolPinString( int aSymbolPinNumber ) const;
    wxString getModelPinString( int aModelPinIndex ) const;
    int getModelPinIndex( const wxString& aModelPinString ) const;
    wxArrayString getModelPinChoices() const;

    void onRadioButton( wxCommandEvent& aEvent ) override;
    void onBrowseButtonClick( wxCommandEvent& aEvent ) override;
    void onModelNameCombobox( wxCommandEvent& aEvent ) override;
    void onModelNameComboboxKillFocus( wxFocusEvent& event ) override;
    void onModelNameComboboxTextEnter( wxCommandEvent& event ) override;
    void onOverrideCheckbox( wxCommandEvent& aEvent ) override;
    void onDeviceTypeChoice( wxCommandEvent& aEvent ) override;
    void onTypeChoice( wxCommandEvent& aEvent ) override;
    void onParamGridChanged( wxPropertyGridEvent& aEvent ) override;
    void onCodePreviewSetFocus( wxFocusEvent& aEvent ) override;
    void onPinAssignmentsGridCellChange( wxGridEvent& aEvent ) override;
    void onPinAssignmentsGridSize( wxSizeEvent& aEvent ) override;
    void onExcludeSymbolCheckbox( wxCommandEvent& aEvent ) override;

    void onLibraryPathUpdate( wxUpdateUIEvent& aEvent ) override;
    void onBrowseButtonUpdate( wxUpdateUIEvent& aEvent ) override;
    void onModelNameComboboxUpdate( wxUpdateUIEvent& aEvent ) override;
    void onOverrideCheckboxUpdate( wxUpdateUIEvent& aEvent ) override;
    void onDeviceTypeChoiceUpdate( wxUpdateUIEvent& aEvent ) override;
    void onTypeChoiceUpdate( wxUpdateUIEvent& aEvent ) override;

    void onParamGridSetFocus( wxFocusEvent& aEvent );
    void onParamGridSelectionChange( wxPropertyGridEvent& aEvent );


    SCH_SYMBOL& m_symbol;
    std::vector<T>& m_fields;

    std::vector<std::shared_ptr<SIM_MODEL>>            m_models;
    std::vector<LIB_PIN*>                              m_sortedSymbolPins;
    std::map<SIM_MODEL::DEVICE_TYPE_, SIM_MODEL::TYPE> m_curModelTypeOfDeviceType;
    SIM_MODEL::TYPE                                    m_curModelType = SIM_MODEL::TYPE::NONE;

    std::shared_ptr<SIM_LIBRARY> m_library;
    std::vector<std::shared_ptr<SIM_MODEL>> m_libraryModels;
    const SIM_MODEL* m_prevModel;

    MODEL_NAME_VALIDATOR m_modelNameValidator;
    wxPGProperty* m_firstCategory; // Used to add principal parameters to root.
    std::unique_ptr<SCINTILLA_TRICKS> m_scintillaTricks;

    wxPGProperty* m_prevParamGridSelection;
    bool m_wasCodePreviewUpdated;
};

#endif /* DIALOG_SIM_MODEL_H */

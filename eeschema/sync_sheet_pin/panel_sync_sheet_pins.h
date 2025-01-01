/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
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

#ifndef PANEL_SYNC_SHEET_PINS_H
#define PANEL_SYNC_SHEET_PINS_H

#include "panel_sync_sheet_pins_base.h"
#include "sch_sheet_path.h"

#include <map>
#include <wx/dataview.h>

class SCH_SHEET;
class wxNotebook;
class SCH_HIERLABEL;
class SCH_SHEET_PIN;
class SHEET_SYNCHRONIZATION_MODEL;
class EDA_ITEM;
class SHEET_SYNCHRONIZATION_AGENT;
using SHEET_SYNCHRONIZATION_MODEL_PTR = wxObjectDataPtr<SHEET_SYNCHRONIZATION_MODEL>;

using SYNC_SHEET_PINT_MODELS = std::map<int, SHEET_SYNCHRONIZATION_MODEL_PTR>;
using SYNC_SHEET_PINT_VIEWS = std::map<int, wxDataViewCtrl*>;

class PANEL_SYNC_SHEET_PINS : public PANEL_SYNC_SHEET_PINS_BASE
{
public:
    enum class SYNC_DIRECTION
    {
        USE_LABEL_AS_TEMPLATE,
        USE_PIN_AS_TEMPLATE
    };


    PANEL_SYNC_SHEET_PINS( wxWindow* aParent, SCH_SHEET* aSheet, wxNotebook* aNoteBook, int aIndex,
                           SHEET_SYNCHRONIZATION_AGENT& aAgent, const SCH_SHEET_PATH& aPath );

    ~PANEL_SYNC_SHEET_PINS() override;

    bool HasUndefinedSheetPing() const;

    void UpdateForms();

    SHEET_SYNCHRONIZATION_MODEL_PTR GetModel( int aKind ) const;

    const wxString& GetSheetFileName() const;

    SCH_SHEET_PATH const& GetSheetPath() const { return m_path; }

protected:
    void OnViewSheetPinCellClicked( wxDataViewEvent& event ) override;

    void OnBtnAddSheetPinsClicked( wxCommandEvent& event ) override;

    void OnBtnRmPinsClicked( wxCommandEvent& event ) override;

    void OnBtnUsePinAsTemplateClicked( wxCommandEvent& event ) override;

    void OnBtnUseLabelAsTemplateClicked( wxCommandEvent& event ) override;

    void OnBtnUndoClicked( wxCommandEvent& event ) override;

    void OnViewSheetLabelCellClicked( wxDataViewEvent& event ) override;

    void OnBtnAddLabelsClicked( wxCommandEvent& event ) override;

    void OnBtnRmLabelsClicked( wxCommandEvent& event ) override;

    void OnViewMatchedCellClicked( wxDataViewEvent& event ) override;

    void PostProcessModelSelection( int aIdex, wxDataViewItem const& aItem );

    void GenericSync( SYNC_DIRECTION direction );

    void UpdatePageImage() const;


private:
    SCH_SHEET*                   m_sheet;
    wxNotebook*                  m_noteBook;
    int                          m_index;
    wxString                     m_sheetFileName;
    SYNC_SHEET_PINT_MODELS       m_models;
    SHEET_SYNCHRONIZATION_AGENT& m_agent;
    SCH_SHEET_PATH               m_path;
    SYNC_SHEET_PINT_VIEWS        m_views;
};

#endif

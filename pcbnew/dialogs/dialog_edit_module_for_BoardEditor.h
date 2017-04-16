/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2015 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef __DIALOG_EDIT_MODULE_FOR_BOARDEDITOR__
#define __DIALOG_EDIT_MODULE_FOR_BOARDEDITOR__


#include <dialog_edit_module_for_BoardEditor_base.h>
#include <wx/valnum.h>

class PANEL_PREV_3D;

class DIALOG_MODULE_BOARD_EDITOR: public DIALOG_MODULE_BOARD_EDITOR_BASE
{
private:
    PCB_EDIT_FRAME *            m_Parent;
    wxDC *                      m_DC;
    MODULE*                     m_CurrentModule;
    TEXTE_MODULE*               m_ReferenceCopy;
    TEXTE_MODULE*               m_ValueCopy;
    std::vector <S3D_INFO>      m_shapes3D_list;
    int                         m_LastSelected3DShapeIndex;
    static size_t               m_page; // remember the last open page during session
    PANEL_PREV_3D*              m_PreviewPane;
    MODULE*                     m_currentModuleCopy;

    wxFloatingPointValidator<double>    m_OrientValidator;
    double  m_OrientValue;

public:
    // The dialog can be closed for several reasons.
    // they are listed here:
    enum FP_PRM_EDITOR_RETVALUE
    {
        PRM_EDITOR_ABORT,
        PRM_EDITOR_WANT_EXCHANGE_FP,
        PRM_EDITOR_EDIT_OK,
        PRM_EDITOR_WANT_MODEDIT
    };

public:
    // Constructor and destructor
    DIALOG_MODULE_BOARD_EDITOR( PCB_EDIT_FRAME* aParent, MODULE* aModule, wxDC* aDC );
    ~DIALOG_MODULE_BOARD_EDITOR();

private:
    void BrowseAndAdd3DShapeFile();
    void InitBoardProperties();
    void InitModeditProperties();
    void Edit3DShapeFileName();

    // virtual event functions
    void OnEditValue( wxCommandEvent& event ) override;
    void OnEditReference( wxCommandEvent& event ) override;
    void On3DShapeSelection( wxCommandEvent& event );
    void On3DShapeNameSelected( wxCommandEvent& event ) override;
    void Edit3DShapeFilename( wxCommandEvent& event ) override
    {
        Edit3DShapeFileName();
    }
    void Remove3DShape( wxCommandEvent& event ) override;
    void Add3DShape( wxCommandEvent& event ) override
    {
        BrowseAndAdd3DShapeFile();
    }
    void GotoModuleEditor( wxCommandEvent& event ) override;
    void ExchangeModule( wxCommandEvent& event ) override;
    void ModuleOrientEvent( wxCommandEvent& event ) override;
    void Cfg3DPath( wxCommandEvent& event ) override;

    void OnInitDlg( wxInitDialogEvent& event ) override
    {
        // Call the default wxDialog handler of a wxInitDialogEvent
        TransferDataToWindow();

        // Now all widgets have the size fixed, call FinishDialogSettings
        FinishDialogSettings();
    }

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;
};


#endif      // __DIALOG_EDIT_MODULE_FOR_BOARDEDITOR__

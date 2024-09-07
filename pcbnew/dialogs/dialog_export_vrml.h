/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2013  Lorenzo Mercantonio
 * Copyright (C) 2013 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 2004-2023 KiCad Developers, see change_log.txt for contributors.
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

#pragma once

#include <dialog_export_vrml_base.h> // the wxFormBuilder header file

class PCB_EDIT_FRAME;

class DIALOG_EXPORT_VRML : public DIALOG_EXPORT_VRML_BASE
{
public:
    DIALOG_EXPORT_VRML( PCB_EDIT_FRAME* aEditFrame );

    ~DIALOG_EXPORT_VRML();

    void SetSubdir( const wxString& aDir ) { m_SubdirNameCtrl->SetValue( aDir ); }

    wxString GetSubdir3Dshapes() { return m_SubdirNameCtrl->GetValue(); }

    wxFilePickerCtrl* FilePicker() { return m_filePicker; }

    int GetRefUnitsChoice() { return m_VRML_RefUnitChoice->GetSelection(); }

    int GetOriginChoice() { return m_rbCoordOrigin->GetSelection(); }

    double GetXRef();

    double GetYRef();

    int GetUnits() { return m_unitsOpt = m_rbSelectUnits->GetSelection(); }

    bool GetNoUnspecifiedOption() { return m_cbRemoveUnspecified->GetValue(); }

    bool GetNoDNPOption() { return m_cbRemoveDNP->GetValue(); }

    bool GetCopyFilesOption() { return m_copy3DFilesOpt = m_cbCopyFiles->GetValue(); }

    bool GetUseRelativePathsOption()
    {
        return m_useRelativePathsOpt = m_cbUseRelativePaths->GetValue();
    }

    void OnUpdateUseRelativePath( wxUpdateUIEvent& event ) override
    {
        // Making path relative or absolute has no meaning when VRML files are not copied.
        event.Enable( m_cbCopyFiles->GetValue() );
    }

    bool TransferDataFromWindow() override;

private:
    PCB_EDIT_FRAME* m_editFrame;
    int             m_unitsOpt;            // Remember last units option
    bool            m_noUnspecified;       // Remember last No Unspecified Component option
    bool            m_noDNP;               // Remember last No DNP Component option
    bool            m_copy3DFilesOpt;      // Remember last copy model files option
    bool            m_useRelativePathsOpt; // Remember last use absolute paths option
    int             m_RefUnits;            // Remember last units for Reference Point
    double          m_XRef;                // Remember last X Reference Point
    double          m_YRef;                // Remember last Y Reference Point
    int             m_originMode;          // Origin selection option
                                           // (0 = user, 1 = board center)
};
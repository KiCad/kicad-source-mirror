/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2013  Lorenzo Mercantonio
 * Copyright (C) 2013 Jean-Pierre Charras jp.charras at wanadoo.fr
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

#pragma once

#include <dialog_export_vrml_base.h> // the wxFormBuilder header file
#include <widgets/unit_binder.h>

class PCB_EDIT_FRAME;

class DIALOG_EXPORT_VRML : public DIALOG_EXPORT_VRML_BASE
{
public:
    DIALOG_EXPORT_VRML( PCB_EDIT_FRAME* aEditFrame );
    ~DIALOG_EXPORT_VRML() = default;

    wxFilePickerCtrl* FilePicker() { return m_filePicker; }

    wxString GetSubdir3Dshapes() { return m_SubdirNameCtrl->GetValue(); }
    int GetSetUserDefinedOrigin() { return m_cbUserDefinedOrigin->GetValue(); }

    double GetXRefMM() { return pcbIUScale.IUTomm( m_xOrigin.GetIntValue() ); }
    double GetYRefMM() { return pcbIUScale.IUTomm( m_yOrigin.GetIntValue() ); }

    double GetScale()
    {
        // Assuming the VRML default unit is the mm
        // this is the mm to VRML scaling factor for mm, 0.1 inch, and inch
        double scaleList[4] = { 1.0, 0.001, 10.0/25.4, 1.0/25.4 };
        return scaleList[ m_unitsChoice->GetSelection() ];
    }

    bool GetNoUnspecifiedOption() { return m_cbRemoveUnspecified->GetValue(); }
    bool GetNoDNPOption() { return m_cbRemoveDNP->GetValue(); }
    bool GetCopyFilesOption() { return m_cbCopyFiles->GetValue(); }
    bool GetUseRelativePathsOption() { return m_cbUseRelativePaths->GetValue(); }

    void OnUpdateUseRelativePath( wxUpdateUIEvent& event ) override
    {
        // Making path relative or absolute has no meaning when VRML files are not copied.
        event.Enable( m_cbCopyFiles->GetValue() );
    }

    bool TransferDataFromWindow() override;

private:
    UNIT_BINDER     m_xOrigin;
    UNIT_BINDER     m_yOrigin;
};
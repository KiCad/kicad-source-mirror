/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ODBPP_EXPORT_DIALOG_H
#define ODBPP_EXPORT_DIALOG_H
#include "dialog_export_odbpp_base.h"

class PCB_EDIT_FRAME;

class DIALOG_EXPORT_ODBPP : public DIALOG_EXPORT_ODBPP_BASE
{
public:
    DIALOG_EXPORT_ODBPP( PCB_EDIT_FRAME* aParent );

    wxString GetOutputPath() const { return m_outputFileName->GetValue(); }

    wxString GetUnitsString() const
    {
        if( m_choiceUnits->GetSelection() == 0 )
            return wxT( "mm" );
        else
            return wxT( "inch" );
    }

    wxString GetPrecision() const { return wxString::Format( "%d", m_precision->GetValue() ); }


    bool GetCompress() const { return m_cbCompress->GetValue(); }

private:
    void onBrowseClicked( wxCommandEvent& event ) override;
    void onOKClick( wxCommandEvent& event ) override;

    bool Init();
    bool TransferDataFromWindow() override;

    PCB_EDIT_FRAME* m_parent;
};

#endif // ODBPP_EXPORT_DIALOG_H
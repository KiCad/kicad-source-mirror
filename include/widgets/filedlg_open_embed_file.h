/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KICAD_FILEDLG_OPEN_EMBED_FILE_H
#define KICAD_FILEDLG_OPEN_EMBED_FILE_H

#include <wx/wx.h>
#include <wx/filedlgcustomize.h>


class FILEDLG_OPEN_EMBED_FILE : public wxFileDialogCustomizeHook
{
public:
    FILEDLG_OPEN_EMBED_FILE( bool aDefaultEmbed = true ) :
            m_embed( aDefaultEmbed )
    {};

    virtual void AddCustomControls( wxFileDialogCustomize& customizer ) override
    {
        m_cb = customizer.AddCheckBox( _( "Embed File" ) );
        m_cb->SetValue( m_embed );
    }

    virtual void TransferDataFromCustomControls() override
    {
        m_embed = m_cb->GetValue();
    }

    bool GetEmbed() const { return m_embed; }

private:
    bool m_embed;

    wxFileDialogCheckBox* m_cb = nullptr;

    wxDECLARE_NO_COPY_CLASS( FILEDLG_OPEN_EMBED_FILE );
};

#endif //KICAD_FILEDLG_OPEN_EMBED_FILE_H

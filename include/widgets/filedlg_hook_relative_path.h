/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <wx/filedlgcustomize.h>

#include <pgm_base.h>
#include <settings/common_settings.h>


// Context the "Use relative path" choice is remembered under.
enum class RELATIVE_PATH_CONTEXT
{
    DRAWING_SHEET,
    JOBSET_DESTINATION
};


/**
 * Add a sticky "Use relative path" checkbox to a file dialog.
 *
 * wxFileDialog accepts a single customize hook, so another hook such as FILEDLG_HOOK_EMBED_FILE
 * can be chained in to share the dialog.
 */
class FILEDLG_HOOK_RELATIVE_PATH : public wxFileDialogCustomizeHook
{
public:
    FILEDLG_HOOK_RELATIVE_PATH( RELATIVE_PATH_CONTEXT aContext, wxFileDialogCustomizeHook* aChained = nullptr ) :
            m_context( aContext ),
            m_chained( aChained )
    {
        if( COMMON_SETTINGS* cfg = Pgm().GetCommonSettings() )
            m_relative = choiceRef( cfg, m_context );
    }

    virtual void AddCustomControls( wxFileDialogCustomize& customizer ) override
    {
        if( m_chained )
        {
            m_chained->AddCustomControls( customizer );
        }
        else
        {
#ifdef __WXMAC__
            customizer.AddStaticText( wxT( "\n\n" ) );  // Increase height of static box
#endif
        }

        m_cb = customizer.AddCheckBox( _( "Use relative path" ) );
        m_cb->SetValue( m_relative );
    }

    virtual void UpdateCustomControls() override
    {
        if( m_chained )
            m_chained->UpdateCustomControls();
    }

    virtual void TransferDataFromCustomControls() override
    {
        if( m_chained )
            m_chained->TransferDataFromCustomControls();

        m_relative = m_cb->GetValue();

        if( COMMON_SETTINGS* cfg = Pgm().GetCommonSettings() )
            choiceRef( cfg, m_context ) = m_relative;
    }

    bool GetUseRelativePath() const { return m_relative; }

private:
    static bool& choiceRef( COMMON_SETTINGS* aCfg, RELATIVE_PATH_CONTEXT aContext )
    {
        switch( aContext )
        {
        case RELATIVE_PATH_CONTEXT::JOBSET_DESTINATION: return aCfg->m_RelativePathDefaults.jobset_destination;
        case RELATIVE_PATH_CONTEXT::DRAWING_SHEET:
        default: return aCfg->m_RelativePathDefaults.drawing_sheet;
        }
    }

    RELATIVE_PATH_CONTEXT m_context;

    wxFileDialogCustomizeHook* m_chained;

    bool m_relative = false;

    wxFileDialogCheckBox* m_cb = nullptr;

    wxDECLARE_NO_COPY_CLASS( FILEDLG_HOOK_RELATIVE_PATH );
};

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


// Context the "Embed file" choice is remembered under. NONE disables persistence.
enum class EMBED_FILE_CONTEXT
{
    NONE,
    DATASHEET,
    DRAWING_SHEET,
    MODEL_3D,
    SIM_MODEL
};


class FILEDLG_HOOK_EMBED_FILE : public wxFileDialogCustomizeHook
{
public:
    FILEDLG_HOOK_EMBED_FILE( bool aDefaultEmbed = true, EMBED_FILE_CONTEXT aContext = EMBED_FILE_CONTEXT::NONE ) :
            m_context( aContext ),
            m_embed( aDefaultEmbed )
    {
        if( m_context != EMBED_FILE_CONTEXT::NONE )
        {
            if( COMMON_SETTINGS* cfg = Pgm().GetCommonSettings() )
                m_embed = choiceRef( cfg, m_context );
        }
    }

    virtual void AddCustomControls( wxFileDialogCustomize& customizer ) override
    {
#ifdef __WXMAC__
        customizer.AddStaticText( wxT( "\n\n" ) );  // Increase height of static box
#endif

        m_cb = customizer.AddCheckBox( _( "Embed file" ) );
        m_cb->SetValue( m_embed );
    }

    virtual void TransferDataFromCustomControls() override
    {
        m_embed = m_cb->GetValue();

        if( m_context != EMBED_FILE_CONTEXT::NONE )
        {
            if( COMMON_SETTINGS* cfg = Pgm().GetCommonSettings() )
                choiceRef( cfg, m_context ) = m_embed;
        }
    }

    bool GetEmbed() const { return m_embed; }

private:
    static bool& choiceRef( COMMON_SETTINGS* aCfg, EMBED_FILE_CONTEXT aContext )
    {
        switch( aContext )
        {
        case EMBED_FILE_CONTEXT::DRAWING_SHEET: return aCfg->m_EmbedFileDefaults.drawing_sheet;
        case EMBED_FILE_CONTEXT::MODEL_3D: return aCfg->m_EmbedFileDefaults.model_3d;
        case EMBED_FILE_CONTEXT::SIM_MODEL: return aCfg->m_EmbedFileDefaults.sim_model;
        case EMBED_FILE_CONTEXT::DATASHEET:
        default: return aCfg->m_EmbedFileDefaults.datasheet;
        }
    }

    EMBED_FILE_CONTEXT m_context;

    bool m_embed;

    wxFileDialogCheckBox* m_cb = nullptr;

    wxDECLARE_NO_COPY_CLASS( FILEDLG_HOOK_EMBED_FILE );
};

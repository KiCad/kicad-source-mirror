/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013 KiCad Developers, see change_log.txt for contributors.
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


#include <invoke_pcb_dialog.h>
#include <dialog_fp_plugin_options_base.h>
#include <fp_lib_table.h>


class DIALOG_FP_PLUGIN_OPTIONS : public DIALOG_FP_PLUGIN_OPTIONS_BASE
{

public:
    DIALOG_FP_PLUGIN_OPTIONS( wxTopLevelWindow* aParent,
            const wxString& aNickname, const wxString& aOptions, wxString* aResult ) :
        DIALOG_FP_PLUGIN_OPTIONS_BASE( aParent ),
        m_callers_options( aOptions ),
        m_options( aOptions ),
        m_result( aResult )
    {
        wxString title = wxString::Format(
                _( "Options for Library '%s'" ), GetChars( aNickname ) );

        SetTitle( title );
    }


private:
    const wxString& m_callers_options;
    wxString        m_options;
    wxString*       m_result;

    //-----<event handlers>------------------------------------------------------
    void onAddRow( wxCommandEvent& event )
    {
    }

    void onDeleteRow( wxCommandEvent& event )
    {
    }

    void onMoveUp( wxCommandEvent& event )
    {
    }

    void onMoveDown( wxCommandEvent& event )
    {
    }

    void onCancelButtonClick( wxCommandEvent& event )
    {
        *m_result = m_callers_options;  // no change
        EndModal( 0 );
    }

    void onCancelButtonClick( wxCloseEvent& event )
    {
        *m_result = m_callers_options;  // no change
        EndModal( 0 );
    }

    void onOKButtonClick( wxCommandEvent& event )
    {
        *m_result = m_options;  // change from edits

        EndModal( 1 );
    }

    //-----</event handlers>-----------------------------------------------------

};


void InvokePluginOptionsEditor( wxTopLevelWindow* aCaller,
        const wxString& aNickname, const wxString& aOptions, wxString* aResult )
{
    DIALOG_FP_PLUGIN_OPTIONS    dlg( aCaller, aNickname, aOptions, aResult );

    dlg.ShowModal();
}

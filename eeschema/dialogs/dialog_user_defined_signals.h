/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <dialog_user_defined_signals_base.h>


class SIMULATOR_FRAME;
class SCINTILLA_TRICKS;
class HTML_MESSAGE_BOX;


class DIALOG_USER_DEFINED_SIGNALS : public DIALOG_USER_DEFINED_SIGNALS_BASE
{
public:
    DIALOG_USER_DEFINED_SIGNALS( SIMULATOR_FRAME* parent, std::map<int, wxString>* aSignals );
    ~DIALOG_USER_DEFINED_SIGNALS();

private:
    void addGridRow( const wxString& aValue, int aId );

    void onAddSignal( wxCommandEvent& event ) override;
    void onDeleteSignal( wxCommandEvent& event ) override;
    void onScintillaCharAdded( wxStyledTextEvent &aEvent, SCINTILLA_TRICKS* aTricks );
    void OnFormattingHelp( wxHyperlinkEvent& aEvent ) override;

    bool TransferDataFromWindow() override;

private:
    SIMULATOR_FRAME*         m_frame;
    std::map<int, wxString>* m_signals;

    HTML_MESSAGE_BOX*        m_helpWindow;
};

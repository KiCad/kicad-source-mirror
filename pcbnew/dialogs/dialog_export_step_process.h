/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2020 New Pagodi(https://stackoverflow.com/users/6846682/new-pagodi)
 *                    from https://stackoverflow.com/a/63289812/1522001
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

#include "dialog_export_step_process_base.h"
#include <wx/process.h>
#include <wx/msgqueue.h>

class wxProcess;
class wxThread;

class DIALOG_EXPORT_STEP_LOG : public DIALOG_EXPORT_STEP_PROCESS_BASE
{
public:
    enum class STATE_MESSAGE : int
    {
        PROCESS_COMPLETE,   ///< Informs the thread the process terminate event was received from wx
        REQUEST_EXIT,       ///< Asks the thread to exit and kill the process
        SENTINEL            ///< Just a dummy entry for end of list
    };

    DIALOG_EXPORT_STEP_LOG( wxWindow* aParent, const wxString& aStepCmd );
    ~DIALOG_EXPORT_STEP_LOG() override;

private:
    void appendMessage( const wxString& aMessage );
    void onProcessTerminate( wxProcessEvent& aEvent );
    void onThreadInput( wxThreadEvent& );
    void onClose( wxCloseEvent& event );
    bool TransferDataToWindow() override;

    wxProcess*                    m_process;
    wxThread*                     m_stdioThread;
    wxMessageQueue<STATE_MESSAGE> m_msgQueue;
    wxString                      m_startMessage;
};
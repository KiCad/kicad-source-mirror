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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <wx/log.h>


namespace KI_TEST
{

/**
 * A stderr log target that drops informational messages.
 *
 * Importers and loaders narrate their progress through wxLogMessage and wxLogInfo, which buries
 * the errors and warnings a test run needs to surface.  Lowering the global log level instead
 * would also silence wxLogTrace, so traces enabled through WXTRACE still pass here.
 */
class QUIET_LOG : public wxLogStderr
{
protected:
    void DoLogRecord( wxLogLevel aLevel, const wxString& aMsg, const wxLogRecordInfo& aInfo ) override
    {
        if( aLevel == wxLOG_Message || aLevel == wxLOG_Status || aLevel == wxLOG_Info )
            return;

        wxLogStderr::DoLogRecord( aLevel, aMsg, aInfo );
    }
};

} // namespace KI_TEST

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

#ifndef KICAD_API_TASK_GUI_REPORTER_H
#define KICAD_API_TASK_GUI_REPORTER_H

#include <widgets/wx_progress_reporters.h>

/**
 * A WX_PROGRESS_REPORTER for GUI-session async API tasks.  Drains API events during UI updates,
 * which lets the API server answer status and result queries from API clients while a task runs.
 */
class API_TASK_GUI_PROGRESS_REPORTER : public WX_PROGRESS_REPORTER
{
public:
    API_TASK_GUI_PROGRESS_REPORTER( wxWindow* aParent, const wxString& aTitle, int aNumPhases,
                                    int aCanAbort ) :
            WX_PROGRESS_REPORTER( aParent, aTitle, aNumPhases, aCanAbort )
    {}

    bool updateUI() override;
};

#endif //KICAD_API_TASK_GUI_REPORTER_H

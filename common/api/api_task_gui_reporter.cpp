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

#include <api/api_task_gui_reporter.h>
#include <widgets/wx_event_utils.h>


bool API_TASK_GUI_PROGRESS_REPORTER::updateUI()
{
    const bool result = WX_PROGRESS_REPORTER::updateUI();

    // Serve pending API requests while the progress dialog updates
    DrainPendingEvents( wxEVT_CATEGORY_THREAD );

    return result;
}

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
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

#include <wx/string.h>
#include <atomic>
#include <memory>
#include <future>

class PROGRESS_REPORTER;
struct BACKGROUND_JOB;

class wxWindow;

class UPDATE_MANAGER
{
public:
    UPDATE_MANAGER();
    ~UPDATE_MANAGER();

    void CheckForUpdate( wxWindow* aNoticeParent );
    int PostRequest( const wxString& aUrl, std::string aRequestBody, std::ostream* aOutput,
                                           PROGRESS_REPORTER* aReporter, const size_t aSizeLimit );

private:
    std::atomic<bool>               m_working;
    std::shared_ptr<BACKGROUND_JOB> m_updateBackgroundJob;
    std::future<void>               m_updateTask;
};
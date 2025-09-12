/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef GIT_REVERT_HANDLER_H_
#define GIT_REVERT_HANDLER_H_

#include <git2.h>
#include <import_export.h>
#include <vector>
#include <wx/string.h>
// TEMPORARY HACKFIX INCLUDE FOR STD::VECTOR EXPORT OUT OF KICOMMON ON WINDOWS
#include <settings/parameters.h>

class LIBGIT_BACKEND;

class APIEXPORT GIT_REVERT_HANDLER
{
public:
    GIT_REVERT_HANDLER( git_repository* aRepository );
    virtual ~GIT_REVERT_HANDLER();

    bool Revert( const wxString& aFilePath );

    void PerformRevert();

    void PushFailedFile( const wxString& aFilePath )
    {
        m_filesFailedToRevert.push_back( aFilePath );
    }

private:
    friend class LIBGIT_BACKEND;
    git_repository* m_repository;

    std::vector<wxString> m_filesToRevert;
    std::vector<wxString> m_filesFailedToRevert;
};

#endif /* GIT_REVERT_HANDLER_H_ */
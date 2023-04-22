/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Chetan Subhash Shinde<chetanshinde2001@gmail.com>
 * Copyright (C) 2023 CERN
 * Copyright (C) 2022-2023 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SCH_LTSPICE_PLUGIN_H_
#define SCH_LTSPICE_PLUGIN_H_

#include <sch_io_mgr.h>
#include <reporter.h>
#include <wx/log.h>


class SCH_SHEET;
class SCH_SCREEN;


class SCH_LTSPICE_PLUGIN : public SCH_PLUGIN
{
public:
    SCH_LTSPICE_PLUGIN()
    {
        m_reporter = &WXLOG_REPORTER::GetInstance();
        m_progressReporter = nullptr;
    }

    ~SCH_LTSPICE_PLUGIN()
    {
    }

    const wxString GetName() const override;

    void SetReporter( REPORTER* aReporter ) override { m_reporter = aReporter; }

    void SetProgressReporter( PROGRESS_REPORTER* aReporter ) override
    {
        m_progressReporter = aReporter;
    }

    const wxString GetFileExtension() const override;

    const wxString GetLibraryFileExtension() const override;

    int GetModifyHash() const override;

    SCH_SHEET* Load( const wxString& aFileName, SCHEMATIC* aSchematic,
                     SCH_SHEET* aAppendToMe = nullptr,
                     const STRING_UTF8_MAP* aProperties = nullptr ) override;

    bool CheckHeader( const wxString& aFileName ) override;

private:
    REPORTER*          m_reporter;          // current reporter for warnings/errors
    PROGRESS_REPORTER* m_progressReporter;  // optional; may be nullptr
};
#endif // SCH_LTSPICE_PLUGIN_H_

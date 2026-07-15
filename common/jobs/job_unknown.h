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

#include <kicommon.h>
#include <jobs/job.h>

/**
 * Placeholder for a job type this version of KiCad does not know, for example a jobset
 * saved by a newer version. Keeps the original settings so the file round-trips intact.
 */
class KICOMMON_API JOB_UNKNOWN : public JOB
{
public:
    JOB_UNKNOWN( const std::string& aType, const nlohmann::json& aSettings );

    void FromJson( const nlohmann::json& j ) override;
    void ToJson( nlohmann::json& j ) const override;

    wxString GetDefaultDescription() const override;

private:
    nlohmann::json m_settings;
};

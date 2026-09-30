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

#ifndef JOB_EXPORT_PCB_FAB_H
#define JOB_EXPORT_PCB_FAB_H

#include <kicommon.h>
#include "job.h"

#include <optional>
#include <vector>


class KICOMMON_API JOB_EXPORT_PCB_FAB : public JOB
{
public:
    enum class UNITS
    {
        MM,
        INCH    // Not IN because a Windows header defines it as a macro
    };

    /**
     * Columns of the IPC-2581C function mode table, Table 4 p 80
     *
     * The order indexes that table.  COUNT also marks a token that no data set matches
     */
    enum class DATA_SET
    {
        USERDEF,
        BOM,
        STACKUP,
        FABRICATION,
        ASSEMBLY,
        TEST,
        STENCIL,
        DFX,

        COUNT
    };

    /// An anonymous name keeps the connections
    enum class NET_NAMES
    {
        INCLUDE,
        ANONYMIZE
    };

    /// Read a data set token in upper case or in lower case  An empty token is USERDEF
    static DATA_SET DataSetFromToken( const wxString& aToken );

    /// Lower case token that the job file and the CLI use for @a aDataSet  Empty when it has none
    static std::string DataSetToken( DATA_SET aDataSet );

    virtual bool SupportsDataSet( DATA_SET aDataSet ) const = 0;

    void FromJson( const nlohmann::json& aJson ) override;
    void ToJson( nlohmann::json& aJson ) const override;

    wxString              m_filename;
    wxString              m_drawingSheet;
    std::vector<wxString> m_variantNames;
    UNITS                 m_units = UNITS::MM;
    int                   m_precision = 6;
    bool                  m_checkZonesBeforeExport = false;
    wxString              m_colMfgPn;
    DATA_SET              m_dataSet = DATA_SET::USERDEF;

    /// Section key that replaces the optional sections of the data set
    /// Unset lets the data set select them and an empty key selects none
    std::optional<wxString> m_sections;
    NET_NAMES             m_netNames = NET_NAMES::INCLUDE;

protected:
    explicit JOB_EXPORT_PCB_FAB( const std::string& aType );
};

#endif

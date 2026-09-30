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

#ifndef JOB_EXPORT_PCB_ODB_H
#define JOB_EXPORT_PCB_ODB_H

#include <kicommon.h>
#include <layer_ids.h>
#include "job_export_pcb_fab.h"

struct ODB_LAYER_OVERRIDE
{
    PCB_LAYER_ID m_layer = UNDEFINED_LAYER;
    bool         m_include = true;
    wxString     m_odbName;
    wxString     m_odbType;
};

namespace nlohmann
{
template <>
struct adl_serializer<ODB_LAYER_OVERRIDE>
{
    static KICOMMON_API void from_json( const json& aJson, ODB_LAYER_OVERRIDE& aOverride );
    static KICOMMON_API void to_json( json& aJson, const ODB_LAYER_OVERRIDE& aOverride );
};
} // namespace nlohmann

class KICOMMON_API JOB_EXPORT_PCB_ODB : public JOB_EXPORT_PCB_FAB
{
public:
    JOB_EXPORT_PCB_ODB();
    wxString GetDefaultDescription() const override;
    wxString GetSettingsDialogTitle() const override;

    void SetDefaultOutputPath( const wxString& aReferenceName );

    bool SupportsDataSet( DATA_SET aDataSet ) const override;

    enum class ODB_COMPRESSION
    {
        NONE,
        ZIP,
        TGZ,
    };

    enum class VARIANT_PACKAGING
    {
        SEPARATE,
        COMBINED
    };

    enum class ORIGIN
    {
        ABSOLUTE_COORDS,
        AUX,
        GRID
    };

public:
    ODB_COMPRESSION                 m_compressionMode;
    VARIANT_PACKAGING               m_variantPackaging = VARIANT_PACKAGING::SEPARATE;
    ORIGIN                          m_origin = ORIGIN::ABSOLUTE_COORDS;
    wxString                        m_productName;
    bool                            m_boardMetadata = true;
    std::vector<ODB_LAYER_OVERRIDE> m_layerOverrides;
};

#endif

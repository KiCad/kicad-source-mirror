/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <jobs/job_export_sch_bom.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>


JOB_EXPORT_SCH_BOM::JOB_EXPORT_SCH_BOM() :
    JOB( "bom", false),
    m_filename(),

    m_fieldDelimiter(),
    m_stringDelimiter(),
    m_refDelimiter(),
    m_refRangeDelimiter(),
    m_keepTabs( false ),
    m_keepLineBreaks( false ),

    m_fieldsOrdered(),
    m_fieldsLabels(),
    m_fieldsGroupBy(),
    m_sortField(),
    m_sortAsc( true ),
    m_filterString(),
    m_excludeDNP( false ),
    m_variantNames()
{
    m_params.emplace_back( new JOB_PARAM<wxString>( "field_delimiter",
                                                    &m_fieldDelimiter,
                                                    m_fieldDelimiter ) );
    m_params.emplace_back( new JOB_PARAM<wxString>( "string_delimiter",
                                                    &m_stringDelimiter,
                                                    m_stringDelimiter ) );
    m_params.emplace_back( new JOB_PARAM<wxString>( "ref_delimiter",
                                                    &m_refDelimiter,
                                                    m_refDelimiter ) );
    m_params.emplace_back( new JOB_PARAM<wxString>( "ref_range_delimiter",
                                                    &m_refRangeDelimiter,
                                                    m_refRangeDelimiter ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "keep_tabs", &m_keepTabs, m_keepTabs ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "keep_line_breaks",
                                                &m_keepLineBreaks,
                                                m_keepLineBreaks ) );
    m_params.emplace_back( new JOB_PARAM_LIST<wxString>( "fields_ordered",
                                                                 &m_fieldsOrdered,
                                                                 m_fieldsOrdered ) );
    m_params.emplace_back( new JOB_PARAM_LIST<wxString>( "fields_labels",
                                                                 &m_fieldsLabels,
                                                                 m_fieldsLabels ) );
    m_params.emplace_back( new JOB_PARAM_LIST<wxString>( "fields_group_by",
                                                                 &m_fieldsGroupBy,
                                                                 m_fieldsGroupBy ) );
    m_params.emplace_back( new JOB_PARAM<wxString>( "sort_field", &m_sortField, m_sortField ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "sort_asc", &m_sortAsc, m_sortAsc ) );
    m_params.emplace_back( new JOB_PARAM<wxString>( "filter_string", &m_filterString, m_filterString ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "exclude_dnp", &m_excludeDNP, m_excludeDNP ) );

    m_params.emplace_back( new JOB_PARAM<wxString>( "bom_preset_name",
                                                    &m_bomPresetName,
                                                    m_bomPresetName ) );
    m_params.emplace_back( new JOB_PARAM<wxString>( "bom_format_preset_name",
                                                    &m_bomFmtPresetName,
                                                    m_bomFmtPresetName ) );

    m_params.emplace_back( new JOB_PARAM_LIST<wxString>( "variant_names",
                                                         &m_variantNames,
                                                         m_variantNames ) );
}


wxString JOB_EXPORT_SCH_BOM::GetDefaultDescription() const
{
    return wxString::Format( _( "Generate bill of materials" ) );
}


wxString JOB_EXPORT_SCH_BOM::GetSettingsDialogTitle() const
{
    return wxString::Format( _( "Generate Bill of Materials Job Settings" ) );
}


REGISTER_JOB( sch_export_bom, _HKI( "Schematic: Generate Bill of Materials" ), KIWAY::FACE_SCH,
              JOB_EXPORT_SCH_BOM );
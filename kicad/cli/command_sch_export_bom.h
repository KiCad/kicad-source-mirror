/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mike Williams <mike@mikebwilliams.com>
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

#ifndef COMMAND_EXPORT_SCH_BOM_H
#define COMMAND_EXPORT_SCH_BOM_H

#include "command_pcb_export_base.h"

namespace CLI
{
// Options for selecting presets of the export, e.g. GroupedByValue and CSV
#define ARG_PRESET "--preset"
#define ARG_PRESET_DESC "Use a named BOM preset setting from the schematic, e.g. \"Grouped By Value\"."

#define ARG_FMT_PRESET "--format-preset"
#define ARG_FMT_PRESET_DESC "Use a named BOM format preset setting from the schematic, e.g. CSV."

// Options for setting the format of the export, e.g. CSV
#define ARG_FIELD_DELIMITER "--field-delimiter"
#define ARG_FIELD_DELIMITER_DESC "Separator between output fields/columns."

#define ARG_STRING_DELIMITER "--string-delimiter"
#define ARG_STRING_DELIMITER_DESC "Character to surround fields with."

#define ARG_REF_DELIMITER "--ref-delimiter"
#define ARG_REF_DELIMITER_DESC "Character to place between individual references."

#define ARG_REF_RANGE_DELIMITER "--ref-range-delimiter"
#define ARG_REF_RANGE_DELIMITER_DESC "Character to place in ranges of references. Leave blank for no ranges."

#define ARG_KEEP_TABS "--keep-tabs"
#define ARG_KEEP_TABS_DESC "Keep tab characters from input fields. Stripped by default."

#define ARG_KEEP_LINE_BREAKS "--keep-line-breaks"
#define ARG_KEEP_LINE_BREAKS_DESC "Keep line break characters from input fields. Stripped by default."

//Options for controlling the fields and the grouping
#define ARG_FIELDS "--fields"
#define ARG_FIELDS_DESC "An ordered list of fields to export. See documentation for special substitutions."

#define ARG_LABELS "--labels"
#define ARG_LABELS_DESC "An ordered list of labels to apply the exported fields."

#define ARG_GROUP_BY "--group-by"
#define ARG_GROUP_BY_DESC "Fields to group references by when field values match."

#define ARG_SORT_FIELD "--sort-field"
#define ARG_SORT_FIELD_DESC "Field name to sort by."

#define ARG_SORT_ASC "--sort-asc"
#define ARG_SORT_ASC_DESC "Sort ascending (true) or descending (false)."

#define ARG_FILTER "--filter"
#define ARG_FILTER_DESC "Filter string to remove output lines."

#define ARG_EXCLUDE_DNP "--exclude-dnp"
#define ARG_EXCLUDE_DNP_DESC "Exclude symbols marked Do-Not-Populate."

#define DEPRECATED_ARG_INCLUDE_EXCLUDED_FROM_BOM "--include-excluded-from-bom"
#define DEPRECATED_ARG_INCLUDE_EXCLUDED_FROM_BOM_DESC "Deprecated.  Has no effect."
#define DEPRECATED_ARG_INCLUDE_EXCLUDED_FROM_BOM_WARNING "--include-excluded-from-bom has been deprecated as of " \
                                                         "KiCad 10.0.0.  It will have no effect."

class SCH_EXPORT_BOM_COMMAND : public COMMAND
{
public:
    SCH_EXPORT_BOM_COMMAND();

protected:
    int doPerform( KIWAY& aKiway ) override;

private:
    std::vector<wxString> convertStringList( const wxString& aList );
};
} // namespace CLI

#endif

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

#ifndef PANEL_DRILL_CHART_OPTIONS_H
#define PANEL_DRILL_CHART_OPTIONS_H

#include <panel_drill_chart_options_base.h>


/**
 * The drill chart's own settings in the generated table dialog: which holes it reports, the
 * totals row, the board's hole grouping and template import and export.
 */
class PANEL_DRILL_CHART_OPTIONS : public PANEL_DRILL_CHART_OPTIONS_BASE
{
public:
    explicit PANEL_DRILL_CHART_OPTIONS( wxWindow* aParent );

    void TransferToWindow( const PCB_GENERATED_TABLE& aTable ) override;
    bool TransferFromWindow( PCB_GENERATED_TABLE& aTable, GENERATED_TABLE_REFRESH& aRefresh ) override;

private:
    void onImportTemplate( wxCommandEvent& aEvent ) override;
    void onExportTemplate( wxCommandEvent& aEvent ) override;
};

#endif // PANEL_DRILL_CHART_OPTIONS_H

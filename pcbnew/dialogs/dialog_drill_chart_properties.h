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

#ifndef DIALOG_DRILL_CHART_PROPERTIES_H
#define DIALOG_DRILL_CHART_PROPERTIES_H

#include <vector>

#include <dialog_drill_chart_properties_base.h>
#include <drill/drill_chart_template.h>
#include <drill/drill_span.h>
#include <drill/drill_symbol_profile.h>
#include <widgets/unit_binder.h>


class PCB_BASE_EDIT_FRAME;
class PCB_DRILL_CHART;


/**
 * Chart-specific replacement for the generic table dialog.
 *
 * The generic one offers copper layers and direct cell editing, both of which a chart has to
 * refuse. Its cells are regenerated from the board, so an edit would be silently discarded,
 * and a fabrication chart on a copper layer would be plotted into the artwork.
 */
class DIALOG_DRILL_CHART_PROPERTIES : public DIALOG_DRILL_CHART_PROPERTIES_BASE
{
public:
    DIALOG_DRILL_CHART_PROPERTIES( PCB_BASE_EDIT_FRAME* aFrame, PCB_DRILL_CHART* aChart );
    ~DIALOG_DRILL_CHART_PROPERTIES() override;

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void onBorderChecked( wxCommandEvent& aEvent ) override;
    void onMoveUp( wxCommandEvent& aEvent ) override;
    void onMoveDown( wxCommandEvent& aEvent ) override;
    void onResetColumns( wxCommandEvent& aEvent ) override;
    void onImportTemplate( wxCommandEvent& aEvent ) override;
    void onExportTemplate( wxCommandEvent& aEvent ) override;

    /**
     * Rebuild m_columns/m_shown from a template, keeping unused columns available.
     */
    void applyTemplate( const DRILL_CHART_TEMPLATE& aTemplate );


    /**
     * Fill the column grid from m_columns, preserving the selected row.
     */
    void fillColumnGrid();

    /**
     * Read the grid back into m_columns.
     */
    bool harvestColumnGrid();

    void moveColumn( int aDelta );

    /**
     * Share the grid's width out across its columns so none of it goes unused.
     */
    void fitColumnGrid();

    void onColumnGridSize( wxSizeEvent& aEvent );

    PCB_BASE_EDIT_FRAME* m_frame;
    PCB_DRILL_CHART*     m_chart;

    /**
     * The board's grouping, edited through the Group By column. Applied on OK, so cancelling
     * leaves every other chart and map on the board as it was.
     */
    DRILL_SYMBOL_PROFILE m_profile;

    /**
     * Every known column, checked ones first in chart order. Held here rather than in the
     * grid so a heading survives being unchecked and rechecked.
     */
    std::vector<DRILL_CHART_COLUMN> m_columns;
    std::vector<bool>               m_shown;


    UNIT_BINDER m_borderWidth;
    UNIT_BINDER m_separatorsWidth;
};

#endif // DIALOG_DRILL_CHART_PROPERTIES_H

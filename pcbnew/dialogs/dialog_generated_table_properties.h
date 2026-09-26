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

#ifndef DIALOG_GENERATED_TABLE_PROPERTIES_H
#define DIALOG_GENERATED_TABLE_PROPERTIES_H

#include <functional>
#include <optional>
#include <vector>

#include <board_tables/generated_table_schema.h>
#include <dialog_generated_table_properties_base.h>
#include <widgets/unit_binder.h>


class GENERATED_TABLE_REFRESH;
class PCB_BASE_EDIT_FRAME;
class PCB_GENERATED_TABLE;


/**
 * What the shared dialog edits that a template also carries.
 */
struct GENERATED_TABLE_FORMAT
{
    std::vector<GENERATED_TABLE_COLUMN> m_Columns; ///< Shown columns only, in table order
    GENERATED_TABLE_UNITS               m_Units = GENERATED_TABLE_UNITS::MM;
    int                                 m_Precision = 0;
};


/**
 * Settings only one kind of generated table has, shown below the shared column grid.
 */
class GENERATED_TABLE_OPTIONS_PANEL : public wxPanel
{
public:
    using wxPanel::wxPanel;

    virtual void TransferToWindow( const PCB_GENERATED_TABLE& aTable ) = 0;

    /// Applies to the table. Board state goes into aRefresh so Cancel leaves the board alone
    virtual bool TransferFromWindow( PCB_GENERATED_TABLE& aTable, GENERATED_TABLE_REFRESH& aRefresh ) = 0;

    /// The dialog's columns, units and precision as edited so far, or nothing when a grid edit is invalid
    std::function<std::optional<GENERATED_TABLE_FORMAT>()> m_GetFormat;

    /// Hands the dialog a new column set, units and precision, e.g. from an imported template
    std::function<void( const GENERATED_TABLE_FORMAT& )> m_SetFormat;
};


/// The options panel must be created with its real parent, since a wxPanel cannot be reparented reliably
using GENERATED_TABLE_OPTIONS_FACTORY = std::function<GENERATED_TABLE_OPTIONS_PANEL*( wxWindow* aParent )>;


/**
 * Properties of any generated table, replacing the generic table dialog.
 *
 * The generic one offers copper layers and direct cell editing, both of which a generated table
 * has to refuse. Its cells are regenerated from the board, so an edit would be silently discarded,
 * and a fabrication table on a copper layer would be plotted into the artwork.
 */
class DIALOG_GENERATED_TABLE_PROPERTIES : public DIALOG_GENERATED_TABLE_PROPERTIES_BASE
{
public:
    /// aOptions may be empty. When given, the panel is created on the dialog and put below the column grid
    DIALOG_GENERATED_TABLE_PROPERTIES( PCB_BASE_EDIT_FRAME* aFrame, PCB_GENERATED_TABLE* aTable,
                                       GENERATED_TABLE_OPTIONS_FACTORY aOptions );

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void onBorderChecked( wxCommandEvent& aEvent ) override;
    void onMoveUp( wxCommandEvent& aEvent ) override;
    void onMoveDown( wxCommandEvent& aEvent ) override;
    void onResetColumns( wxCommandEvent& aEvent ) override;

    /**
     * Show aColumns checked and in order, then every other schema column unchecked so turning
     * one back on does not mean retyping its heading.
     */
    void setColumns( const std::vector<GENERATED_TABLE_COLUMN>& aColumns );

    std::vector<GENERATED_TABLE_COLUMN> shownColumns() const;

    std::optional<GENERATED_TABLE_FORMAT> getFormat();
    void                                  setFormat( const GENERATED_TABLE_FORMAT& aFormat );

    /**
     * Fill the column grid from m_rows.
     */
    void fillColumnGrid();

    /**
     * Read the grid back into m_rows.
     */
    bool harvestColumnGrid();

    void moveColumn( int aDelta );

    /**
     * Share the grid's width out across its columns so none of it goes unused.
     */
    void fitColumnGrid();

    void onColumnGridSize( wxSizeEvent& aEvent );

    PCB_BASE_EDIT_FRAME*           m_frame;
    PCB_GENERATED_TABLE*           m_table;
    GENERATED_TABLE_OPTIONS_PANEL* m_options;

    struct COLUMN_ROW
    {
        GENERATED_TABLE_COLUMN m_Column;
        bool                   m_Shown;
    };

    /**
     * Every known column, checked ones first in table order. Held here rather than in the
     * grid so a heading survives being unchecked and rechecked.
     */
    std::vector<COLUMN_ROW> m_rows;

    UNIT_BINDER m_borderWidth;
    UNIT_BINDER m_separatorsWidth;
};

#endif // DIALOG_GENERATED_TABLE_PROPERTIES_H

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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <widgets/panel_notebook_base.h>
#include <lib_table_grid_data_model.h>


class wxAuiNotebook;


class LIB_TABLE_NOTEBOOK_PANEL : public PANEL_NOTEBOOK_BASE
{
public:
    LIB_TABLE_NOTEBOOK_PANEL( wxWindow* parent, wxWindowID id = wxID_ANY ) :
            PANEL_NOTEBOOK_BASE( parent, id )
    { }

    ~LIB_TABLE_NOTEBOOK_PANEL() override;

    bool GetCanClose() override;

    WX_GRID* GetGrid()
    {
        std::function<WX_GRID*( wxWindow* )> findGrid =
                [&]( wxWindow* win ) -> WX_GRID*
                {
                    if( WX_GRID* grid = dynamic_cast<WX_GRID*>( win ) )
                        return grid;

                    for( wxWindow* child : win->GetChildren() )
                    {
                        if( WX_GRID* grid = findGrid( child ) )
                            return grid;
                    }

                    return nullptr;
                };

        return findGrid( this );
    }

    LIB_TABLE_GRID_DATA_MODEL* GetModel()
    {
        return static_cast<LIB_TABLE_GRID_DATA_MODEL*>( GetGrid()->GetTable() );
    }

    bool TableModified();
    bool SaveTable();

    static void AddTable( wxAuiNotebook* aNotebook, const wxString& aTitle, bool aClosable );
};

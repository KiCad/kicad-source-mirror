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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef GRID_COLOR_SWATCH_HELPERS
#define GRID_COLOR_SWATCH_HELPERS

#include <wx/generic/gridctrl.h>
#include <wx/generic/grideditors.h>
#include <gal/color4d.h>
#include <widgets/color_swatch.h>


class wxGrid;

//-------- Custom wxGridCellRenderers --------------------------------------------------

class GRID_CELL_COLOR_RENDERER : public wxGridCellRenderer
{
public:
    GRID_CELL_COLOR_RENDERER( wxWindow* aParent = nullptr, SWATCH_SIZE aSize = SWATCH_EXPAND,
                              const KIGFX::COLOR4D& aBackground = KIGFX::COLOR4D::UNSPECIFIED );
    GRID_CELL_COLOR_RENDERER( const GRID_CELL_COLOR_RENDERER& aOther );
    ~GRID_CELL_COLOR_RENDERER() override;

    wxGridCellRenderer* Clone() const override;
    wxSize GetBestSize( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, int row, int col ) override;

    void Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC, const wxRect& aRect, int aRow,
               int aCol, bool isSelected ) override;

    void OnDarkModeToggle();

private:
    wxWindow*       m_parent;

    KIGFX::COLOR4D  m_background;

    wxSize          m_size;
    wxSize          m_checkerboardSize;
    KIGFX::COLOR4D  m_checkerboardBg;
};


//-------- Custom wxGridCellEditors ----------------------------------------------------
//
// Note: this implementation is an adaptation of wxGridCellChoiceEditor

class GRID_CELL_COLOR_SELECTOR : public wxGridCellEditor
{
public:
    GRID_CELL_COLOR_SELECTOR( wxWindow* aParent, wxGrid* aGrid );

    wxGridCellEditor* Clone() const override;
    void Create( wxWindow* aParent, wxWindowID aId, wxEvtHandler* aEventHandler ) override;

    wxString GetValue() const override;

    void BeginEdit( int aRow, int aCol, wxGrid* aGrid ) override;
    bool EndEdit( int , int , const wxGrid* , const wxString& , wxString *newval ) override;
    void ApplyEdit( int aRow, int aCol, wxGrid* aGrid ) override;
    void Reset() override;

protected:
    wxWindow*      m_parent;
    wxGrid*        m_grid;
    KIGFX::COLOR4D m_value;

    wxDECLARE_NO_COPY_CLASS( GRID_CELL_COLOR_SELECTOR );
};

#endif  // GRID_COLOR_SWATCH_HELPERS

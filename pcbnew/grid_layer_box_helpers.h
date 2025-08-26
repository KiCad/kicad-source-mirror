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

#ifndef GRID_LAYER_BOX_HELPERS_H
#define GRID_LAYER_BOX_HELPERS_H

#include <wx/generic/gridctrl.h>
#include <wx/generic/grideditors.h>
#include "pcb_layer_box_selector.h"
#include <lset.h>
#include <wx/bitmap.h>

class wxGrid;
class PCB_LAYER_BOX_SELECTOR;


//-------- Custom wxGridCellRenderers --------------------------------------------------

class GRID_CELL_LAYER_RENDERER : public wxGridCellStringRenderer
{
public:
    GRID_CELL_LAYER_RENDERER( PCB_BASE_FRAME* aFrame );
    ~GRID_CELL_LAYER_RENDERER() override;

    void Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC,
               const wxRect& aRect, int aRow, int aCol, bool isSelected ) override;

private:
    PCB_BASE_FRAME* m_frame;
    wxBitmap        m_bitmap;
};


//-------- Custom wxGridCellEditors ----------------------------------------------------
//
// Note: this implementation is an adaptation of wxGridCellChoiceEditor

class GRID_CELL_LAYER_SELECTOR : public wxGridCellEditor
{
public:
    GRID_CELL_LAYER_SELECTOR( PCB_BASE_FRAME* aFrame, const LSET& forbiddenLayers,
                              bool aShowNonActivated = false );

    wxGridCellEditor* Clone() const override;
    void Create( wxWindow* aParent, wxWindowID aId, wxEvtHandler* aEventHandler ) override;

    wxString GetValue() const override;

    void SetSize( const wxRect& aRect ) override;

    void BeginEdit( int aRow, int aCol, wxGrid* aGrid ) override;
    bool EndEdit( int , int , const wxGrid* , const wxString& , wxString *newval ) override;
    void ApplyEdit( int aRow, int aCol, wxGrid* aGrid ) override;
    void Reset() override;

protected:
    // Event handlers to properly dismiss the layer selector when it loses focus
    void onComboDropDown( wxCommandEvent& aEvent );
    void onComboCloseUp( wxCommandEvent& aEvent );

    PCB_LAYER_BOX_SELECTOR* LayerBox() const
    {
        return static_cast<PCB_LAYER_BOX_SELECTOR*>( m_control );
    }

    PCB_BASE_FRAME* m_frame;
    LSET            m_mask;
    bool            m_showNonActivated;
    int             m_value;

    wxDECLARE_NO_COPY_CLASS( GRID_CELL_LAYER_SELECTOR );
};

#endif  // GRID_LAYER_BOX_HELPERS_H

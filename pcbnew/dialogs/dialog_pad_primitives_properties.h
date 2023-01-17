/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef _DIALOG_PAD_PRIMITIVES_PROPERTIES_
#define _DIALOG_PAD_PRIMITIVES_PROPERTIES_

#include <pcb_base_frame.h>
#include <wx/valnum.h>
#include <board.h>
#include <footprint.h>
#include <pad_shapes.h>
#include <pcb_shape.h>
#include <origin_viewitem.h>
#include <dialog_pad_primitives_properties_base.h>
#include <widgets/text_ctrl_eval.h>
#include <pcb_draw_panel_gal.h>
#include <widgets/unit_binder.h>


/**
 * A dialog to edit basic shape parameters.
 *
 * Polygonal shape is not handled by this dialog.
 */
class DIALOG_PAD_PRIMITIVES_PROPERTIES: public DIALOG_PAD_PRIMITIVES_PROPERTIES_BASE
{
public:
    DIALOG_PAD_PRIMITIVES_PROPERTIES( wxWindow* aParent, PCB_BASE_FRAME* aFrame,
                                      PCB_SHAPE* aShape );

    /**
     * Transfer data out of the GUI.
     */
    bool TransferDataFromWindow() override;

private:
    /**
     * Function TransferDataToWindow
     * Transfer data into the GUI.
     */
    bool TransferDataToWindow() override;

    // The basic shape currently edited
    PCB_SHAPE*   m_shape;

    UNIT_BINDER  m_startX;
    UNIT_BINDER  m_startY;
    UNIT_BINDER  m_ctrl1X;
    UNIT_BINDER  m_ctrl1Y;
    UNIT_BINDER  m_ctrl2X;
    UNIT_BINDER  m_ctrl2Y;
    UNIT_BINDER  m_endX;
    UNIT_BINDER  m_endY;
    UNIT_BINDER  m_radius;
    UNIT_BINDER  m_thickness;
};


/**
 * A dialog to edit basic polygonal shape parameters.
 */
class DIALOG_PAD_PRIMITIVE_POLY_PROPS: public DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE
{
public:
    DIALOG_PAD_PRIMITIVE_POLY_PROPS( wxWindow* aParent, PCB_BASE_FRAME* aFrame,
                                     PCB_SHAPE* aShape );
    ~DIALOG_PAD_PRIMITIVE_POLY_PROPS();

    /**
     * Transfer data out of the GUI.
     */
    bool TransferDataFromWindow() override;

private:
    /**
     * Transfer data into the GUI.
     */
    bool TransferDataToWindow() override;

    /**
     * Test for a valid polygon (a not self intersectiong polygon).
     */
    bool Validate() override;

    // Events handlers:
    void OnButtonAdd( wxCommandEvent& event ) override;
    void OnButtonDelete( wxCommandEvent& event ) override;
    void onPaintPolyPanel( wxPaintEvent& event ) override;
    void onPolyPanelResize( wxSizeEvent& event ) override;
    void onGridSelect( wxGridRangeSelectEvent& event ) override;
    void onCellChanging( wxGridEvent& event );
    void onCellSelect( wxGridEvent& event ) override
    {
        event.Skip();
    }

    bool doValidate( bool aRemoveRedundantCorners );

private:
    PCB_SHAPE*            m_shape;

    std::vector<VECTOR2I> m_currPoints;    // The working copy of the data being edited
    UNIT_BINDER           m_thickness;
};


/**
 * A dialog to apply geometry transforms to a shape or set of shapes (move, rotate around
 * origin, scaling factor, duplication).
 *
 * Shapes are scaled then moved then rotated.
 */

class DIALOG_PAD_PRIMITIVES_TRANSFORM : public DIALOG_PAD_PRIMITIVES_TRANSFORM_BASE
{
public:
    DIALOG_PAD_PRIMITIVES_TRANSFORM( wxWindow* aParent, PCB_BASE_FRAME* aFrame,
                                     std::vector<std::shared_ptr<PCB_SHAPE>>& aList,
                                     bool aShowDuplicate );

    /**
     * Apply geometric transform (rotation, move, scale) defined in dialog
     * aDuplicate = 1 .. n to duplicate the list of shapes
     * aDuplicate = 0 to transform the list of shapes
     * The duplicated items are transformed, but the initial shpes are not modified.
     * The duplicated items are added to aList
     */
    void Transform( std::vector<std::shared_ptr<PCB_SHAPE>>* aList = nullptr,
                    int aDuplicateCount = 0 );

    /**
     * @return the number of duplicate, chosen by user.
     */
    int GetDuplicateCount() { return m_spinCtrlDuplicateCount->GetValue(); }

private:
    std::vector<std::shared_ptr<PCB_SHAPE>>& m_list;

    UNIT_BINDER  m_vectorX;
    UNIT_BINDER  m_vectorY;
    UNIT_BINDER  m_rotation;
};

#endif      // #ifndef _DIALOG_PAD_PRIMITIVES_PROPERTIES_

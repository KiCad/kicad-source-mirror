/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2010 Kicad Developers, see change_log.txt for contributors.
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

#ifndef LAYERWIDGET_H_
#define LAYERWIDGET_H_

#include <wx/wx.h>
#include <wx/statbmp.h>
#include <wx/aui/aui.h>

#include "macros.h"
#include "common.h"

#include "layer_panel_base.h"
#include "colors.h"

/* no external data knowledge needed or wanted
#include "pcbnew.h"
#include "wxPcbStruct.h"
*/


/**
 * Class LAYER_WIDGET
 * is abstract and is derived from a wxFormBuilder maintained class called
 * LAYER_PANEL_BASE.  It is used to manage a list of layers, with the notion of
 * a "current" layer, and layer specific visibility control.  You must derive from
 * it to use it so you can implement the abstract functions which recieve the
 * events.  Each layer is given its own color, and that color can be changed
 * within the UI provided here.  This widget knows nothing of the client code, meaning
 * it has no knowledge of a BOARD or anything.  To use it you must derive from
 * this class and implement the abstract functions:
 * <p> void OnLayerColorChange( int aLayer, int aColor );
 * <p> bool OnLayerSelect( int aLayer );
 * <p> void OnLayerVisible( int aLayer, bool isVisible );
 * <p> void OnRenderColorChange( int id, int aColor );
 * <p> void OnRenderEnable( int id, bool isEnabled );
 */
class LAYER_WIDGET : public LAYER_PANEL_BASE
{

public:
    /**
     * Struct ROW
     * provides all the data needed to add a row to a LAYER_WIDGET.  This is
     * part of the public API for a LAYER_WIDGET.
     */
    struct ROW
    {
        wxString    rowName;    ///< the prompt or layername
        int         id;         ///< either a layer or "visible element" id
        int         color;      ///< -1 if none.
        bool        state;      ///< initial wxCheckBox state
        wxString    tooltip;    ///< if not empty, use this tooltip on row

        ROW( const wxString& aRowName, int aId, int aColor = -1,
            const wxString& aTooltip = wxEmptyString, bool aState = true )
        {
            rowName = aRowName;
            id      = aId;
            color   = aColor;
            state   = aState;
            tooltip = aTooltip;
        }
    };


protected:

#define MAX_LAYER_ROWS      64
#define BUTT_SIZE_X         32
#define BUTT_SIZE_Y         22
#define BUTT_VOID           6

    wxBitmap*       m_BlankBitmap;
    wxBitmap*       m_RightArrowBitmap;
    wxSize          m_BitmapSize;
    int             m_CurrentRow;           ///< selected row of layer list

    static wxBitmap makeBitmap( int aColor );


    /**
     * Function makeColorButton
     * creates a wxBitmapButton and assigns it a solid color and a control ID
     */
    wxBitmapButton* makeColorButton( wxWindow* aParent, int aColor, int aID );

    void OnLeftDownLayers( wxMouseEvent& event );

    /**
     * Function OnMiddleDownLayerColor
     * is called only from a color button when user right clicks.
     */
    void OnMiddleDownLayerColor( wxMouseEvent& event );

    /**
     * Function OnRightDownLayers
     * puts up a popup menu for the layer panel.
     */
    void OnRightDownLayers( wxMouseEvent& event );

    void OnPopupSelection( wxCommandEvent& event );

    /**
     * Function OnLayerCheckBox
     * handles the "is layer visible" checkbox and propogates the
     * event to the client's notification function.
     */
    void OnLayerCheckBox( wxCommandEvent& event );

    void OnMiddleDownRenderColor( wxMouseEvent& event );

    void OnRenderCheckBox( wxCommandEvent& event );

    /**
     * Function getLayerComp
     * returns the component within the m_LayersFlexGridSizer at aSizerNdx or
     * NULL if \a aSizerNdx is out of range.
     *
     * @param aSizerNdx is the 0 based index into all the wxWindows which have
     *   been added to the m_LayersFlexGridSizer.
     */
    wxWindow* getLayerComp( int aSizerNdx );

    /**
     * Function findLayerRow
     * returns the row index that \a aLayer resides in, or -1 if not found.
     */
    int findLayerRow( int aLayer );

    /**
     * Function insertLayerRow
     * appends or inserts a new row in the layer portion of the widget.
     */
    void insertLayerRow( int aRow, const ROW& aSpec );

    void insertRenderRow( int aRow, const ROW& aSpec );

public:

    /** Constructor */
    LAYER_WIDGET( wxWindow* parent );

    /**
     * Function GetBestSize
     * returns the preferred minimum size, taking into consideration the
     * dynamic content.  Nothing in wxWidgets was reliable enough so this
     * overrides one of their functions.
     */
    wxSize GetBestSize() const;

    /**
     * Function GetLayerRowCount
     * returns the number of rows in the layer tab.
     */
    int GetLayerRowCount() const;

    /**
     * Function GetRenderRowCount
     * returns the number of rows in the render tab.
     */
    int GetRenderRowCount() const;

    /**
     * Function AppendLayerRow
     * appends a new row in the layer portion of the widget.  The user must
     * ensure that ROW::id is unique for all existing rows on Windows.
     */
    void AppendLayerRow( const ROW& aRow );

    /**
     * Function ClearLayerRows
     * empties out the layer rows.
     */
    void ClearLayerRows();

    /**
     * Function AppendRenderRow
     * appends a new row in the render portion of the widget.  The user must
     * ensure that ROW::id is unique for all existing rows on Windows.
     */
    void AppendRenderRow( const ROW& aRow );

    /**
     * Function ClearRenderRows
     * empties out the render rows.
     */
    void ClearRenderRows();

    /**
     * Function SelectLayerRow
     * changes the row selection in the layer list to the given row.
     */
    void SelectLayerRow( int aRow );

    /**
     * Function SelectLayer
     * changes the row selection in the layer list to \a aLayer provided.
     */
    void SelectLayer( int aLayer );

    /**
     * Function GetSelectedLayer
     * returns the selected layer or -1 if none.
     */
    int GetSelectedLayer();

    /**
     * Function SetLayerVisible
     * sets \a aLayer visible or not.  This does not invoke OnLayerVisible().
     */
    void SetLayerVisible( int aLayer, bool isVisible );


    //-----<abstract functions>-------------------------------------------

    /**
     * Function OnLayerColorChange
     * is called to notify client code about a layer color change.  Derived
     * classes will handle this accordingly.
     */
    virtual void OnLayerColorChange( int aLayer, int aColor ) = 0;

    /**
     * Function OnLayerSelect
     * is called to notify client code whenever the user selects a different
     * layer.  Derived classes will handle this accordingly, and can deny
     * the change by returning false.
     */
    virtual bool OnLayerSelect( int aLayer ) = 0;

    /**
     * Function OnLayerVisible
     * is called to notify client code about a layer visibility change.
     */
    virtual void OnLayerVisible( int aLayer, bool isVisible ) = 0;

    /**
     * Function OnRenderColorChange
     * is called to notify client code whenever the user changes a rendering
     * color.
     * @param aId is the same id that was established in a Rendering row
     * via the AddRenderRow() function.
     */
    virtual void OnRenderColorChange( int aId, int aColor ) = 0;

    /**
     * Function OnRenderEnable
     * is called to notify client code whenever the user changes an rendering
     * enable in one of the rendering checkboxes.
     * @param aId is the same id that was established in a Rendering row
     * via the AddRenderRow() function.
     * @param isEnabled is the state of the checkbox, true if checked.
     */
    virtual void OnRenderEnable( int aId, bool isEnabled ) = 0;

    //-----</abstract functions>------------------------------------------
};


#endif // LAYERWIDGET_H_

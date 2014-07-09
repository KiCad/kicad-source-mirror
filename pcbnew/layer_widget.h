/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2010 KiCad Developers, see change_log.txt for contributors.
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

#include <wx/intl.h>
#include <wx/wx.h>
#include <wx/statbmp.h>
#include <wx/string.h>
#include <wx/aui/auibook.h>
#include <wx/notebook.h>
#include <wx/sizer.h>
#include <wx/gdicmn.h>
#include <wx/scrolwin.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <layers_id_colors_and_visibility.h>
#include <colors.h>

#define LYR_COLUMN_COUNT        4           ///< Layer tab column count
#define RND_COLUMN_COUNT        2           ///< Rendering tab column count

#define COLUMN_COLORBM 1
#define COLUMN_COLOR_LYR_CB 2
#define COLUMN_COLOR_LYRNAME 3


/**
 * Class LAYER_WIDGET
 * is abstract and is used to manage a list of layers, with the notion of
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
 *
 * Please note that even if designed toward layers, it is used to
 * contain other stuff, too (the second page in pcbnew contains render
 * items, for example)
 */
class LAYER_WIDGET : public wxPanel
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
        EDA_COLOR_T color;      ///< -1 if none.
        bool        state;      ///< initial wxCheckBox state
        wxString    tooltip;    ///< if not empty, use this tooltip on row

        ROW( const wxString& aRowName, int aId, EDA_COLOR_T aColor = UNSPECIFIED_COLOR,
            const wxString& aTooltip = wxEmptyString, bool aState = true )
        {
            rowName = aRowName;
            id      = aId;
            color   = aColor;
            state   = aState;
            tooltip = aTooltip;
        }
    };

    static const wxEventType EVT_LAYER_COLOR_CHANGE;

protected:

    wxAuiNotebook*      m_notebook;
    wxPanel*            m_LayerPanel;
    wxScrolledWindow*   m_LayerScrolledWindow;
    wxFlexGridSizer*    m_LayersFlexGridSizer;
    wxPanel*            m_RenderingPanel;
    wxScrolledWindow*   m_RenderScrolledWindow;
    wxFlexGridSizer*    m_RenderFlexGridSizer;

    wxWindow*           m_FocusOwner;
    wxBitmap*           m_BlankBitmap;
    wxBitmap*           m_BlankAlternateBitmap;
    wxBitmap*           m_RightArrowBitmap;
    wxBitmap*           m_RightArrowAlternateBitmap;
    wxSize              m_BitmapSize;
    int                 m_CurrentRow;           ///< selected row of layer list
    int                 m_PointSize;

    static wxBitmap makeBitmap( EDA_COLOR_T aColor );

    /**
     * Virtual Function useAlternateBitmap
     * @return true if bitmaps shown in Render layer list
     * are alternate bitmaps, or false if they are "normal" bitmaps
     * This is a virtual function because Pcbnew uses normal bitmaps
     * but GerbView uses both bitmaps
     * (alternate bitmaps to show layers in use, normal fo others)
     */
    virtual bool useAlternateBitmap(int aRow) { return false; }

    /**
     * Function encodeId
     * is here to allow saving a layer index within a control as its wxControl id,
     * but to do so in a way that all child wxControl ids within a wxWindow are unique,
     * since this is required by Windows.
     * @see getDecodedId()
     */
    static int encodeId( int aColumn, int aId );

    /**
     * Function getDecodedId
     * decodes \a aControlId to original un-encoded value. This of
     * course holds iff encodedId was called with a LAYER_NUM (this box
     * is used for other things than layers, too)
     */
    static LAYER_NUM getDecodedId( int aControlId );

    /**
     * Function makeColorButton
     * creates a wxBitmapButton and assigns it a solid color and a control ID
     */
    wxBitmapButton* makeColorButton( wxWindow* aParent, EDA_COLOR_T aColor, int aID );

    void OnLeftDownLayers( wxMouseEvent& event );

    /**
     * Function OnMiddleDownLayerColor
     * is called only from a color button when user right clicks.
     */
    void OnMiddleDownLayerColor( wxMouseEvent& event );

    /**
     * Function OnLayerCheckBox
     * handles the "is layer visible" checkbox and propogates the
     * event to the client's notification function.
     */
    void OnLayerCheckBox( wxCommandEvent& event );

    void OnMiddleDownRenderColor( wxMouseEvent& event );

    void OnRenderCheckBox( wxCommandEvent& event );

    void OnTabChange( wxNotebookEvent& event );


    /**
     * Function getLayerComp
     * returns the component within the m_LayersFlexGridSizer at @a aRow and @a aCol
     * or NULL if these parameters are out of range.
     *
     * @param aRow is the row index
     * @param aColumn is the column
     * @return wxWindow - the component installed within the sizer at given grid coordinate.
     */
    wxWindow* getLayerComp( int aRow, int aColumn ) const;
    wxWindow* getRenderComp( int aRow, int aColumn ) const;

    /**
     * Function findLayerRow
     * returns the row index that \a aLayer resides in, or -1 if not found.
     */
    int findLayerRow( LAYER_NUM aLayer ) const;
    int findRenderRow( int aId ) const;

    /**
     * Function insertLayerRow
     * appends or inserts a new row in the layer portion of the widget.
     */
    void insertLayerRow( int aRow, const ROW& aSpec );

    void insertRenderRow( int aRow, const ROW& aSpec );

    /**
     * Function passOnFocus
     * gives away the keyboard focus up to the main parent window.
     */
    void passOnFocus();

public:

    /** Constructor
     * @param aParent is the parent window
     * @param aFocusOwner is the window that should be sent the focus after
     * @param aPointSize is the font point size to use within the widget.  This
     *  effectively sets the overal size of the widget via the row height and bitmap
     *  button sizes.
     * @param id is the wxWindow id ( default = wxID_ANY)
     * @param pos is the window position
     * @param size is the window size
     * @param style is the window style
     * every operation.
     */
    LAYER_WIDGET( wxWindow* aParent, wxWindow* aFocusOwner, int aPointSize = -1,
        wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL );

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
     * Function AppendLayerRows
     * appends new rows in the layer portion of the widget.  The user must
     * ensure that ROW::id is unique for all existing rows on Windows.
     */
    void AppendLayerRows( const ROW* aRowsArray, int aRowCount )
    {
        for( int row=0;  row<aRowCount;  ++row )
            AppendLayerRow( aRowsArray[row] );
    }

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
     * Function AppendRenderRows
     * appends new rows in the render portion of the widget.  The user must
     * ensure that ROW::id is unique for all existing rows on Windows.
     */
    void AppendRenderRows( const ROW* aRowsArray, int aRowCount )
    {
        for( int row=0;  row<aRowCount;  ++row )
            AppendRenderRow( aRowsArray[row] );
    }

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
    void SelectLayer( LAYER_NUM aLayer );

    /**
     * Function GetSelectedLayer
     * returns the selected layer or -1 if none.
     */
    LAYER_NUM GetSelectedLayer();

    /**
     * Function SetLayerVisible
     * sets \a aLayer visible or not.  This does not invoke OnLayerVisible().
     */
    void SetLayerVisible( LAYER_NUM aLayer, bool isVisible );

    /**
     * Function IsLayerVisible
     * returns the visible state of the layer ROW associated with \a aLayer id.
     */
    bool IsLayerVisible( LAYER_NUM aLayer );

    /**
     * Function SetLayerColor
     * changes the color of \a aLayer
     */
    void SetLayerColor( LAYER_NUM aLayer, EDA_COLOR_T aColor );

    /**
     * Function GetLayerColor
     * returns the color of the layer ROW associated with \a aLayer id.
     */
    EDA_COLOR_T GetLayerColor( LAYER_NUM aLayer ) const;

    /**
     * Function SetRenderState
     * sets the state of the checkbox associated with \a aId within the
     * Render tab group of the widget.  Does not fire an event, i.e. does
     * not invoke OnRenderEnable().
     * @param aId is the same unique id used when adding a ROW to the
     *  Render tab.
     * @param isSet = the new checkbox state
     */
    void SetRenderState( int aId, bool isSet );

    /**
     * Function GetRenderState
     * returns the state of the checkbox associated with \a aId.
     * @return bool - true if checked, else false.
     */
    bool GetRenderState( int aId );

    void UpdateLayouts();

/*  did not help:
    void Freeze()
    {
        LAYER_PANEL_BASE::Freeze();
        m_LayerScrolledWindow->Freeze();
        m_RenderScrolledWindow->Freeze();
    }

    void Thaw()
    {
        m_RenderScrolledWindow->Thaw();
        m_LayerScrolledWindow->Thaw();
        LAYER_PANEL_BASE::Thaw();
    }
*/

    //-----<abstract functions>-------------------------------------------

    /**
     * Function OnLayerColorChange
     * is called to notify client code about a layer color change.  Derived
     * classes will handle this accordingly.
     * @param aLayer is the board layer to change
     * @param aColor is the new color
     */
    virtual void OnLayerColorChange( int aLayer, EDA_COLOR_T aColor ) = 0;

    /**
     * Function OnLayerSelect
     * is called to notify client code whenever the user selects a different
     * layer.  Derived classes will handle this accordingly, and can deny
     * the change by returning false.
     * @param aLayer is the board layer to select
     */
    virtual bool OnLayerSelect( int aLayer ) = 0;

    /**
     * Function OnLayerVisible
     * is called to notify client code about a layer visibility change.
     *
     * @param aLayer is the board layer to select
     * @param isVisible is the new vosible state
     * @param isFinal is true when this is the last of potentially several
     *  such calls, and can be used to decide when to update the screen only
     *  one time instead of several times in the midst of a multiple layer change.
     */
    virtual void OnLayerVisible( LAYER_NUM aLayer, bool isVisible, bool isFinal = true ) = 0;

    /**
     * Function OnRenderColorChange
     * is called to notify client code whenever the user changes a rendering
     * color.
     * @param aId is the same id that was established in a Rendering row
     * via the AddRenderRow() function.
     * @param aColor is the new color
     */
    virtual void OnRenderColorChange( int aId, EDA_COLOR_T aColor ) = 0;

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

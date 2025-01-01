/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#ifndef LAYERWIDGET_H_
#define LAYERWIDGET_H_

#include <wx/intl.h>
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
#include <layer_ids.h>
#include <gal/color4d.h>
#include <widgets/color_swatch.h>
#include <widgets/indicator_icon.h>

#define LYR_COLUMN_COUNT        5           ///< Layer tab column count
#define RND_COLUMN_COUNT        2           ///< Rendering tab column count

#define COLUMN_ICON_ACTIVE 0
#define COLUMN_COLORBM 1
#define COLUMN_COLOR_LYR_CB 2
#define COLUMN_COLOR_LYRNAME 3
#define COLUMN_ALPHA_INDICATOR 4

using KIGFX::COLOR4D;

/**
 * Manage a list of layers with the notion of a "current" layer, and layer specific visibility
 * control.
 *
 * You must derive from it to use it so you can implement the abstract functions which receive
 * the events.  Each layer is given its own color, and that color can be changed within the UI
 * provided here.  This widget knows nothing of the client code, meaning it has no knowledge of
 * a BOARD or anything.  To use it you must derive from this class and implement the abstract
 * functions:
 * <p> void OnLayerColorChange( int aLayer, int aColor );
 * <p> bool OnLayerSelect( int aLayer );
 * <p> void OnLayerVisible( int aLayer, bool isVisible );
 * <p> void OnRenderColorChange( int id, int aColor );
 * <p> void OnRenderEnable( int id, bool isEnabled );
 *
 * @note Even if designed toward layers, it is used to contain other stuff, too (the second page
 * in pcbnew contains render items, for example).
 */
class LAYER_WIDGET : public wxPanel
{
public:
    /**
     * Provide all the data needed to add a row to a LAYER_WIDGET.  This is
     * part of the public API for a LAYER_WIDGET.
     */
    struct ROW
    {
        wxString    rowName;      ///< the prompt or layername
        int         id;           ///< either a layer or "visible element" id
        COLOR4D     color;        ///< COLOR4D::UNSPECIFIED if none.
        bool        state;        ///< initial wxCheckBox state
        wxString    tooltip;      ///< if not empty, use this tooltip on row
        bool        changeable;   ///< if true, the state can be changed
        bool        spacer;       ///< if true, this row is a spacer
        COLOR4D     defaultColor; ///< The default color for the row

        ROW( const wxString& aRowName, int aId, const COLOR4D& aColor = COLOR4D::UNSPECIFIED,
             const wxString& aTooltip = wxEmptyString, bool aState = true,
             bool aChangeable = true, const COLOR4D& aDefaultColor = COLOR4D::UNSPECIFIED )
        {
            rowName = aRowName;
            id      = aId;
            color   = aColor;
            state   = aState;
            tooltip = aTooltip;
            changeable = aChangeable;
            spacer = false;
            defaultColor = aDefaultColor;
        }

        ROW()
        {
            id = 0;
            color = COLOR4D::UNSPECIFIED;
            state = true;
            changeable = true;
            spacer = true;
            defaultColor = COLOR4D::UNSPECIFIED;
        }
    };

    static const wxEventType EVT_LAYER_COLOR_CHANGE;

public:

    /**
     * @param aParent is the parent window.
     * @param aFocusOwner is the window that should be sent the focus after.
     * @param id is the wxWindow id ( default = wxID_ANY).
     * @param pos is the window position.
     * @param size is the window size.
     * @param style is the window style.
     */
    LAYER_WIDGET( wxWindow* aParent, wxWindow* aFocusOwner, wxWindowID id = wxID_ANY,
                  const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                  long style = wxTAB_TRAVERSAL );

    virtual ~LAYER_WIDGET();

    /**
     * Set the string that is used for determining the smallest string displayed in the layer's tab.
     */
    void SetSmallestLayerString( const wxString& aString )
    {
        m_smallestLayerString = aString;
    }

    /**
     * Return the preferred minimum size, taking into consideration the dynamic content.
     *
     * Nothing in wxWidgets was reliable enough so this overrides one of their functions.
     */
    wxSize GetBestSize() const;

    /**
     * Return the number of rows in the layer tab.
     */
    int GetLayerRowCount() const;

    /**
     * Return the number of rows in the render tab.
     */
    int GetRenderRowCount() const;

    /**
     * Append a new row in the layer portion of the widget.
     *
     * The user must ensure that ROW::id is unique for all existing rows on Windows.
     */
    void AppendLayerRow( const ROW& aRow );

    /**
     * Append new rows in the layer portion of the widget.
     *
     * The user must ensure that ROW::id is unique for all existing rows on Windows.
     */
    void AppendLayerRows( const ROW* aRowsArray, int aRowCount )
    {
        for( int row=0;  row<aRowCount;  ++row )
            AppendLayerRow( aRowsArray[row] );

        UpdateLayouts();
    }

    /**
     * Empty out the layer rows.
     */
    void ClearLayerRows();

    /**
     * Append a new row in the render portion of the widget.
     *
     * The user must ensure that ROW::id is unique for all existing rows on Windows.
     */
    void AppendRenderRow( const ROW& aRow );

    /**
     * Append new rows in the render portion of the widget.
     *
     * The user must ensure that ROW::id is unique for all existing rows on Windows.
     */
    void AppendRenderRows( const ROW* aRowsArray, int aRowCount )
    {
        for( int row=0;  row<aRowCount;  ++row )
            AppendRenderRow( aRowsArray[row] );

        UpdateLayouts();
    }

    /**
     * Empty out the render rows.
     */
    void ClearRenderRows();

    /**
     * Change the row selection in the layer list to the given row.
     */
    void SelectLayerRow( int aRow );

    /**
     * Change the row selection in the layer list to \a aLayer provided.
     */
    void SelectLayer( int aLayer );

    /**
     * Return the selected layer or -1 if none.
     */
    int GetSelectedLayer();

    /**
     * Set \a aLayer visible or not.  This does not invoke OnLayerVisible().
     */
    void SetLayerVisible( int aLayer, bool isVisible );

    /**
     * Return the visible state of the layer ROW associated with \a aLayer id.
     */
    bool IsLayerVisible( int aLayer );

    /**
     * Change the color of \a aLayer
     */
    void SetLayerColor( int aLayer, const COLOR4D& aColor );

    /**
     * Return the color of the layer ROW associated with \a aLayer id.
     */
    COLOR4D GetLayerColor( int aLayer ) const;

    /**
     * Return the color of the Render ROW in position \a aRow.
     */
    COLOR4D GetRenderColor( int aRow ) const;

    /**
     * Set the state of the checkbox associated with \a aId within the Render tab group of the
     * widget.
     *
     * This does not fire an event, i.e. does not invoke OnRenderEnable().
     *
     * @param aId is the same unique id used when adding a ROW to the Render tab.
     * @param isSet is the new checkbox state.
     */
    void SetRenderState( int aId, bool isSet );

    /**
     * Return the state of the checkbox associated with \a aId.
     *
     * @return true if checked, else false.
     */
    bool GetRenderState( int aId );

    void UpdateLayouts();

    /**
     * Update all layer manager icons (layers only).
     *
     * Useful when loading a file or clearing a layer because they change,
     * and the indicator arrow icon needs to be updated
     */
    void UpdateLayerIcons();

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
     * Notify client code about a layer color change.
     *
     * Derived objects will handle this accordingly.
     *
     * @param aLayer is the board layer to change.
     * @param aColor is the new color.
     */
    virtual void OnLayerColorChange( int aLayer, const COLOR4D& aColor ) = 0;

    /**
     * Notify client code whenever the user selects a different layer.
     *
     * Derived classes will handle this accordingly, and can deny the change by returning false.
     *
     * @param aLayer is the board layer to select.
     */
    virtual bool OnLayerSelect( int aLayer ) = 0;

    /**
     * Notify client code about a layer visibility change.
     *
     * @param aLayer is the board layer to select.
     * @param isVisible is the new visible state.
     * @param isFinal is true when this is the last of potentially several such calls, and can
     *                be used to decide when to update the screen only one time instead of
     *                several times in the midst of a multiple layer change.
     */
    virtual void OnLayerVisible( int aLayer, bool isVisible, bool isFinal = true ) = 0;

    /**
     * Notify client code about a layer being right-clicked.
     *
     * @param aMenu is the right-click menu containing layer-scoped options.
     */
    virtual void OnLayerRightClick( wxMenu& aMenu ) = 0;

    /**
     * Notify client code whenever the user changes a rendering color.
     *
     * @param aId is the same id that was established in a Rendering row via the AddRenderRow()
     *            function.
     * @param aColor is the new color.
     */
    virtual void OnRenderColorChange( int aId, const COLOR4D& aColor ) = 0;

    /**
     * Notify client code whenever the user changes an rendering enable in one of the rendering
     * checkboxes.
     *
     * @param aId is the same id that was established in a Rendering row via the AddRenderRow()
     *            function.
     * @param isEnabled is the state of the checkbox, true if checked.
     */
    virtual void OnRenderEnable( int aId, bool isEnabled ) = 0;

protected:
    /**
     * @return true if bitmaps shown in Render layer list
     * are alternate bitmaps, or false if they are "normal" bitmaps
     * This is a virtual function because Pcbnew uses normal bitmaps
     * but GerbView uses both bitmaps
     * (alternate bitmaps to show layers in use, normal for others)
     */
    virtual bool useAlternateBitmap(int aRow) { return false; }

    /**
     * Subclasses can override this to provide accurate representation
     * of transparent color swatches.
     */
    virtual COLOR4D getBackgroundLayerColor() { return COLOR4D::BLACK; }

    /**
     * Allow saving a layer index within a control as its wxControl id.
     *
     * To do so in a way that all child wxControl ids within a wxWindow are unique, since this
     * is required by Windows.
     *
     * @see getDecodedId()
     */
    static int encodeId( int aColumn, int aId );

    /**
     * Decode \a aControlId to original un-encoded value.
     *
     * This holds if encodedId was called with a layer (this box is used for other things
     * than layers, too).
     */
    static int getDecodedId( int aControlId );

    void OnLeftDownLayers( wxMouseEvent& event );

    /**
     * Called when user right-clicks a layer.
     */
    void OnRightDownLayer( wxMouseEvent& event, COLOR_SWATCH* aColorSwatch,
                           const wxString& aLayerName );

    /**
     * Called when a user changes a swatch color.
     */
    void OnLayerSwatchChanged( wxCommandEvent& aEvent );

    /**
     * Handle the "is layer visible" checkbox and propagates the event to the client's
     * notification function.
     */
    void OnLayerCheckBox( wxCommandEvent& event );

    /**
     * Notify when user right-clicks a render option.
     */
    void OnRightDownRender( wxMouseEvent& aEvent, COLOR_SWATCH* aColorSwatch,
                            const wxString& aRenderName );

    /**
     * Called when user has changed the swatch color of a render entry.
     */
    void OnRenderSwatchChanged( wxCommandEvent& aEvent );

    void OnRenderCheckBox( wxCommandEvent& event );

    void OnTabChange( wxNotebookEvent& event );


    /**
     * Return the component within the m_LayersFlexGridSizer at @a aRow and @a aCol
     * or NULL if these parameters are out of range.
     *
     * @param aRow is the row index
     * @param aColumn is the column
     * @return the component installed within the sizer at given grid coordinate.
     */
    wxWindow* getLayerComp( int aRow, int aColumn ) const;
    wxWindow* getRenderComp( int aRow, int aColumn ) const;

    /**
     * Return the row index that \a aLayer resides in, or -1 if not found.
     */
    int findLayerRow( int aLayer ) const;
    int findRenderRow( int aId ) const;

    /**
     * Append or insert a new row in the layer portion of the widget.
     */
    void insertLayerRow( int aRow, const ROW& aSpec );

    void insertRenderRow( int aRow, const ROW& aSpec );

    void setLayerCheckbox( int aLayer, bool isVisible );

    void updateLayerRow( int aRow, const wxString& aName );

    /**
     * Give away the keyboard focus up to the main parent window.
     */
    void passOnFocus();

    // popup menu ids.
    enum POPUP_ID
    {
        ID_CHANGE_LAYER_COLOR = wxID_HIGHEST,
        ID_CHANGE_RENDER_COLOR,
        ID_LAST_VALUE
    };

    wxNotebook*         m_notebook;
    wxPanel*            m_LayerPanel;
    wxScrolledWindow*   m_LayerScrolledWindow;
    wxFlexGridSizer*    m_LayersFlexGridSizer;
    wxPanel*            m_RenderingPanel;
    wxScrolledWindow*   m_RenderScrolledWindow;
    wxFlexGridSizer*    m_RenderFlexGridSizer;

    wxWindow*           m_FocusOwner;
    int                 m_CurrentRow;           ///< selected row of layer list
    int                 m_PointSize;

    ROW_ICON_PROVIDER*  m_IconProvider;

    wxString            m_smallestLayerString;
};

#endif // LAYERWIDGET_H_


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



/*  This source module implements the layer visibility and selection widget
*/


#define STAND_ALONE     1   // define to enable test program for LAYER_WIDGET
// also enable KICAD_AUIMANAGER and KICAD_AUITOOLBAR in ccmake to
// build this test program

#include <wx/wx.h>
#include <wx/statbmp.h>
#include <wx/aui/aui.h>

//#include "fctsys.h"
#include "common.h"

#include "layer_panel_base.h"
#include "colors.h"

/* no external data knowledge needed or wanted
#include "pcbnew.h"
#include "wxPcbStruct.h"
*/


/* XPM */
static const char * clear_xpm[] = {
"28 14 1 1",
" 	c None",
"                            ",
"                            ",
"                            ",
"                            ",
"                            ",
"                            ",
"                            ",
"                            ",
"                            ",
"                            ",
"                            ",
"                            ",
"                            ",
"                            "};

/* XPM */
static const char * rightarrow_xpm[] = {
"28 14 5 1",
" 	c None",
".	c white",
"X	c #8080ff",
"o	c BLUE",
"O	c gray56",
"                   .X       ",
"                   .XX      ",
"                   .XXX     ",
"                   .XXXX    ",
"    ................XXXXX   ",
"    XXXXXXXXXXXXXXXXXXXXXX  ",
"    XXXXXXXXXXXXXXXXXXXXXXX ",
"    oooooooooooooooooooooooO",
"    ooooooooooooooooooooooO ",
"     OOOOOOOOOOOOOOOoooooO  ",
"                    ooooO   ",
"                    oooO    ",
"                    ooO     ",
"                    oO      "};



/**
 * Struct LAYER_SPEC
 * provides all the data needed to add a layer row to a LAYER_WIDGET
 */
struct LAYER_SPEC
{
    wxString    layerName;
    int         layer;
    int         colorIndex;

    LAYER_SPEC( const wxString& aLayerName, int aLayer, int aColorIndex = 0 )
    {
        layerName  = aLayerName;
        layer      = aLayer;
        colorIndex = aColorIndex;
    }
};


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
 * <p>
 * void ColorChange( int aLayer, int aColor );
 * <p>
 * bool LayerChange( int aLayer );
 */
class LAYER_WIDGET : public LAYER_PANEL_BASE
{

#define MAX_LAYER_ROWS      64
#define LAYER_COLUMN_COUNT  4

protected:
    wxBitmap*       m_BlankBitmap;
    wxBitmap*       m_RightArrowBitmap;
    wxSize          m_BitmapSize;
    wxStaticBitmap* m_Bitmaps[MAX_LAYER_ROWS];
    int             m_CurrentRow;           ///< selected row of layer list


    /**
     * Function makeColorButton
     * creates a wxBitmapButton and assigns it a solid color and a control ID
     */
    wxBitmapButton* makeColorButton( int aColorIndex, int aID )
    {
        const int BUTT_SIZE_X = 32;
        const int BUTT_SIZE_Y = 22;

        // dynamically make a wxBitMap and brush it with the appropriate color,
        // then create a wxBitmapButton from it.

        wxBitmap    bitmap( BUTT_SIZE_X, BUTT_SIZE_Y );
        wxBrush     brush;
        wxMemoryDC  iconDC;

        iconDC.SelectObject( bitmap );

        brush.SetColour( MakeColour( aColorIndex ) );
        brush.SetStyle( wxSOLID );
        iconDC.SetBrush( brush );

        iconDC.DrawRectangle( 0, 0, BUTT_SIZE_X, BUTT_SIZE_Y );

        wxBitmapButton* ret = new wxBitmapButton( m_LayerScrolledWindow, aID, bitmap,
            wxDefaultPosition, wxSize(BUTT_SIZE_X, BUTT_SIZE_Y), wxBORDER_RAISED );

        ret->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnLeftDownLayers ), NULL, this );
        ret->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnRightDownLayers ), NULL, this );

        /* cannot get this event without also the wxEVT_LEFT_DOWN firing first
        ret->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( LAYER_WIDGET::OnLeftDClickLayers ), NULL, this );
        */

        return ret;
    }


    void OnLeftDownLayers( wxMouseEvent& event )
    {
        int row;

        wxObject* eventSource = event.GetEventObject();

        // if mouse event is coming from the m_LayerScrolledWindow and not one
        // of its children, we have to find the row manually based on y coord.
        if( eventSource == (wxObject*) m_LayerScrolledWindow )
        {
            int y = event.GetY();

            wxArrayInt heights = m_LayersFlexGridSizer->GetRowHeights();

            int height = 0;

            int rowCount = GetLayerRowCount();
            for( row = 0;  row<rowCount;  ++row )
            {
                if( y < height + heights[row] )
                    break;

                height += heights[row];
            }

            if( row >= rowCount )
                row = rowCount - 1;
        }

        // all nested controls on a given row will have the layer index as their ID
        else
        {
            int layer = ((wxWindow*)eventSource)->GetId();
            row   = findLayerRow( layer );
        }

        if( LayerChange( row ) )    // if owner allows this change.
            SelectLayerRow( row );
    }

    void OnRightDownLayers( wxMouseEvent& event )
    {
        printf("OnRightDownLayers\n");
    }

    /**
     * Function getLayerComp
     * returns the component within the m_LayersFlexGridSizer at aSizerNdx.
     *
     * @param aSizerNdx is the 0 based index into all the wxWindows which have
     *   been added to the m_LayersFlexGridSizer.
     */
    wxWindow* getLayerComp( int aSizerNdx )
    {
        return m_LayersFlexGridSizer->GetChildren()[aSizerNdx]->GetWindow();
    }

    /**
     * Function findLayerRow
     * returns the row index that \a aLayer resides in, or -1 if not found.
     */
    int findLayerRow( int aLayer )
    {
        int count = GetLayerRowCount();
        for( int row=0;  row<count; ++row )
        {
            // column 0 in the layer scroll window has a wxStaticBitmap, get its ID.
            wxStaticBitmap* bm = (wxStaticBitmap*) getLayerComp( row * LAYER_COLUMN_COUNT + 0 );

            if( aLayer == bm->GetId() )
                return row;
        }

        return -1;
    }

    /**
     * Function insertLayerRow
     * appends or inserts a new row in the layer portion of the widget.
     */
    void insertLayerRow( int aRow, const LAYER_SPEC& aSpec )
    {
        wxASSERT( aRow >= 0 && aRow < MAX_LAYER_ROWS );

        size_t index = aRow * LAYER_COLUMN_COUNT;

        wxSizerFlags    flags;

        flags.Align(wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);

        // column 0
        m_Bitmaps[aRow] = new wxStaticBitmap( m_LayerScrolledWindow, aSpec.layer, *m_BlankBitmap,
                            wxDefaultPosition, m_BitmapSize );
        m_Bitmaps[aRow]->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnLeftDownLayers ), NULL, this );
        m_LayersFlexGridSizer->Insert( index+0,
            new wxSizerItem( m_Bitmaps[aRow], wxSizerFlags().Align( wxALIGN_CENTER_VERTICAL ) ) );

        // column 1
        wxBitmapButton* bmb = makeColorButton( aSpec.colorIndex, aSpec.layer );
        bmb->SetToolTip( _("Right click to change layer color, left click to select layer" ) );
        m_LayersFlexGridSizer->Insert( index+1,
            new wxSizerItem( bmb, flags ) );

        // column 2
        wxStaticText* st = new wxStaticText( m_LayerScrolledWindow, aSpec.layer, aSpec.layerName );
        st->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnLeftDownLayers ), NULL, this );
        st->SetToolTip( _( "Click here to select this layer" ) );
        m_LayersFlexGridSizer->Insert( index+2,
            new wxSizerItem( st, wxSizerFlags().Align( wxALIGN_CENTER_VERTICAL )) );

        // column 3
        wxCheckBox* cb = new wxCheckBox( m_LayerScrolledWindow, aSpec.layer, wxEmptyString );
        cb->SetToolTip( _( "Enable this for visibility" ) );
        m_LayersFlexGridSizer->Insert( index+3, new wxSizerItem( cb, flags ) );
    }

public:

    /** Constructor */
    LAYER_WIDGET( wxWindow* parent ) :
        LAYER_PANEL_BASE( parent )
    {
        m_CurrentRow = 0;

        memset( m_Bitmaps, 0, sizeof(m_Bitmaps) );

        m_RightArrowBitmap = new wxBitmap( rightarrow_xpm );
        m_BlankBitmap = new wxBitmap( clear_xpm );     // translucent

        m_BitmapSize = wxSize(m_BlankBitmap->GetWidth(), m_BlankBitmap->GetHeight());

        AppendLayerRow(  LAYER_SPEC( wxT("layer 1"), 0, RED ) );
        AppendLayerRow(  LAYER_SPEC( wxT("layer 2"), 1, GREEN ) );
        AppendLayerRow(  LAYER_SPEC( wxT("brown_layer"), 2, BROWN ) );
        AppendLayerRow(  LAYER_SPEC( wxT("layer_4_you"), 3, BLUE ) );

        SelectLayerRow( 1 );

        m_LayerScrolledWindow->FitInside();
        m_LayerScrolledWindow->SetMinSize( m_LayerScrolledWindow->GetSize() );

        Fit();

        SetMinSize( GetSize() );
    }


    /**
     * Function GetLayerRowCount
     * returns the number of rows in the layer tab.
     */
    int GetLayerRowCount()
    {
        int controlCount = m_LayersFlexGridSizer->GetChildren().GetCount();
        return controlCount / LAYER_COLUMN_COUNT;
    }


    /**
     * Function AppendLayerRow
     * appends a new row in the layer portion of the widget.
     */
    void AppendLayerRow( const LAYER_SPEC& aSpec )
    {
        int nextRow = GetLayerRowCount();
        insertLayerRow( nextRow, aSpec );
    }


    /**
     * Function SelectLayerRow
     * changes the row selection in the layer list to the given row.
     */
    bool SelectLayerRow( int aRow )
    {
        if( (unsigned) aRow < (unsigned) GetLayerRowCount() )
        {
            int newNdx = LAYER_COLUMN_COUNT * aRow;
            int oldNdx = LAYER_COLUMN_COUNT * m_CurrentRow;

            wxStaticBitmap* oldbm = (wxStaticBitmap*) getLayerComp( oldNdx );
            wxStaticBitmap* newbm = (wxStaticBitmap*) getLayerComp( newNdx );

            oldbm->SetBitmap( *m_BlankBitmap );
            newbm->SetBitmap( *m_RightArrowBitmap );

            m_CurrentRow = aRow;

            return true;
        }
        return false;
    }

    /**
     * Function SelectLayer
     * changes the row selection in the layer list to the given layer.
     */
    bool SelectLayer( int aLayer )
    {
        int row = findLayerRow( aLayer );
        return SelectLayerRow( row );
    }

    //-----<abstract functions>-------------------------------------------

    /**
     * Function ColorChange
     * is called whenever the user changes the color of a layer.  Derived
     * classes will handle this accordingly.
     */
    virtual void ColorChange( int aLayer, int aColor ) = 0;

    /**
     * Function LayerChange
     * is called whenever the user selects a different layer.  Derived classes
     * will handle this accordingly, and can deny the change by returning false.
     */
    virtual bool LayerChange( int aLayer ) = 0;

    //-----</abstract functions>------------------------------------------

};


#if defined(STAND_ALONE)

/**
 * Class MYFRAME
 * is a test class here to exercise the LAYER_WIDGET and explore use cases.
 * @see http://www.kirix.com/labs/wxaui/screenshots.html
 * for ideas.
 */
class MYFRAME : public wxFrame
{

    // example of how to derive from LAYER_WIDGET in order to provide the
    // abstract methods.
    class MYLAYERS : public LAYER_WIDGET
    {
        MYFRAME*    frame;

    public:
        // your constructor could take a BOARD argument.  here I leave it
        // out because this source module wants to know nothing of BOARDs
        // to maximize re-use.
        MYLAYERS( wxWindow* aParent, MYFRAME* aFrame ) :
            LAYER_WIDGET( aParent ),
            frame( aFrame )
        {
        }

        void ColorChange( int aLayer, int aColor )
        {
            printf("ColorChange( aLayer:%d, aColor:%d )\n", aLayer, aColor );
        }

        bool LayerChange( int aLayer )
        {
            printf( "LayerChange( aLayer:%d )\n", aLayer );
            return true;
        }
    };


public:
    MYFRAME( wxWindow * parent ) :
        wxFrame( parent, -1, _( "wxAUI Test" ), wxDefaultPosition,
            wxSize( 800, 600 ), wxDEFAULT_FRAME_STYLE )
    {
        // notify wxAUI which frame to use
        m_mgr.SetManagedWindow( this );

        wxPanel* layerWidget = new MYLAYERS( this, this );

        wxTextCtrl* text2 = new wxTextCtrl( this, -1, _( "Pane 2 - sample text" ),
                                            wxDefaultPosition, wxSize( 200, 150 ),
                                            wxNO_BORDER | wxTE_MULTILINE );

        wxTextCtrl* text3 = new wxTextCtrl( this, -1, _( "Main content window" ),
                                            wxDefaultPosition, wxSize( 200, 150 ),
                                            wxNO_BORDER | wxTE_MULTILINE );

        // add the panes to the manager
        wxAuiPaneInfo li;
        li.MinSize( ayerWidget->GetSize() );    // ignored on linux
        li.BestSize( layerWidget->GetSize() );
        li.Left();
        li.MaximizeButton( false );
        li.MinimizeButton( false );
        li.CloseButton( false );
        li.Caption( wxT( "Layers" ) );
        m_mgr.AddPane( layerWidget, li );

        m_mgr.AddPane( text2, wxBOTTOM, wxT( "Pane Number Two" ) );
        m_mgr.AddPane( text3, wxCENTER );

        // tell the manager to "commit" all the changes just made
        m_mgr.Update();
    }

    ~MYFRAME()
    {
        // deinitialize the frame manager
        m_mgr.UnInit();
    }

private:
    wxAuiManager m_mgr;
};


// our normal wxApp-derived class, as usual
class MyApp : public wxApp {
public:

    bool OnInit()
    {
        wxFrame* frame = new MYFRAME( NULL );

        SetTopWindow( frame );
        frame->Show();
        return true;
    }
};

DECLARE_APP( MyApp );
IMPLEMENT_APP( MyApp );

#endif


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
    for PCBNEW.
*/


#define STAND_ALONE     1   // define to enable test program for LAYER_WIDGET
// also enable KICAD_AUIMANAGER and KICAD_AUITOOLBAR in ccmake to
// build this test program


#include "layer_panel_base.h"
#include "colors.h"
#include <wx/wx.h>
#include <wx/aui/aui.h>


/**
 * Struct LAYER_SPEC
 * provides all the data needed to add a layer row to a LAYER_PANEL
 */
struct LAYER_SPEC
{
    int         colorIndex;
    int         checkBoxId;
    int         layer;
    wxString    layerName;


    LAYER_SPEC( const wxString& aLayerName, int aColorIndex = 0 )
    {
        layerName  = aLayerName;
        colorIndex = aColorIndex;
    }
};


class BOARD;

/**
 * Class LAYER_PANEL
 * is derived from a wxFormBuilder maintained class called LAYER_PANEL_BASE
 * and is used to populate the wxListCtrl within.
 */
class LAYER_PANEL : public LAYER_PANEL_BASE
{
    BOARD*  m_Board;

#define LAYER_COLUMN_COUNT  3

    class MYSTATICTEXT : public wxStaticText
    {
    public:
        MYSTATICTEXT( wxWindow *parent, wxWindowID id, const wxString &label,
                const wxPoint &pos=wxDefaultPosition, const wxSize &size=wxDefaultSize,
                long style=0, const wxString &name=wxStaticTextNameStr) :
            wxStaticText( parent, id, label, pos, size, style, name )
        {
            //Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( LAYER_PANEL::OnLeftDownLayers ), NULL, parent );
            //Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( LAYER_PANEL::OnRightDownLayers ), NULL, parent );
        }

        ~MYSTATICTEXT()
        {
            //Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( LAYER_PANEL::OnLeftDownLayers ), NULL, GetParent() );
            //Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( LAYER_PANEL::OnRightDownLayers ), NULL, GetParent() );
        }
    };


    /**
     * Function getLayerSpec
     * returns a LAYER_SPEC from \a aLayer
    LAYER_SPEC  getLayerSpec( int aLayer )
    {
        if( m_Board )
        {
            return LAYER_SPEC( wxT("temporary") );
        }

        else    // test scaffolding
        {
            switch( aLayer )
            {
            }
        }
    }
     */

    static wxBitmapButton* makeColorButton( int aColorIndex, int aID, wxWindow* aParent )
    {
        const int BUTT_SIZE_X = 25;
        const int BUTT_SIZE_Y = 20;

        // dynamically make a wxBitMap and brush it with the appropriate color,
        // then create a wxBitmapButton from it.

        wxBitmap    bitmap( BUTT_SIZE_X, BUTT_SIZE_Y );
        wxBrush     brush;
        wxMemoryDC  iconDC;

        iconDC.SelectObject( bitmap );
        iconDC.SetPen( *wxBLACK_PEN );

        brush.SetColour( MakeColour( aColorIndex ) );
        brush.SetStyle( wxSOLID );
        iconDC.SetBrush( brush );

        iconDC.DrawRectangle( 0, 0, BUTT_SIZE_X, BUTT_SIZE_Y );

        return new wxBitmapButton( aParent, aID, bitmap,
            wxDefaultPosition, wxSize(BUTT_SIZE_X, BUTT_SIZE_Y) );
    }

    void insertLayerRow( int aRow, const LAYER_SPEC& aSpec )
    {
        wxASSERT( aRow >= 0 );

        size_t index = aRow * LAYER_COLUMN_COUNT;

        wxSizerFlags    flags;

        flags.Align(wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);

        // column 0
        m_LayersFlexGridSizer->Insert( index+0,
                makeColorButton( aSpec.colorIndex, 0, m_LayerScrolledWindow ),
                flags );

        // column 1
        m_LayersFlexGridSizer->Insert( index+1,
                new wxCheckBox( m_LayerScrolledWindow, aSpec.checkBoxId, wxEmptyString ),
                flags );

        // column 2
        m_LayersFlexGridSizer->Insert( index+2,
                new MYSTATICTEXT( m_LayerScrolledWindow, -1, aSpec.layerName ),
                wxSizerFlags() );
    }


    void OnLeftDownLayers( wxMouseEvent& event )
    {
        printf("OnLeftDownLayers\n");
    }
    void OnRightDownLayers( wxMouseEvent& event )
    {
        printf("OnRightDownLayers\n");
    }


public:

    /** Constructor */
    LAYER_PANEL( wxWindow* parent, BOARD* aBoard ) :
        LAYER_PANEL_BASE( parent )
    {
        m_Board = aBoard;

        insertLayerRow( 0, LAYER_SPEC( wxT("layer 1"), RED ) );

        insertLayerRow( 1, LAYER_SPEC( wxT("layer 2"), GREEN ) );
    }

};



/**
 *  class LAYER_WIDGET : public wxPanel
 *  {
 *  };
 */


#if defined(STAND_ALONE)

/**
 * Class MYFRAME
 * is a test class here to exercise the LAYER_WIDGET and explore use cases.
 * @see http://www.kirix.com/labs/wxaui/screenshots.html
 * for ideas.
 */
class MYFRAME : public wxFrame {
public:
    MYFRAME( wxWindow * parent ) : wxFrame( parent, -1, _( "wxAUI Test" ),
                                            wxDefaultPosition, wxSize( 800, 600 ),
                                            wxDEFAULT_FRAME_STYLE )
    {
        // notify wxAUI which frame to use
        m_mgr.SetManagedWindow( this );

        // create several text controls
        wxPanel* layerWidget = new LAYER_PANEL( this, NULL );

        wxTextCtrl* text2 = new wxTextCtrl( this, -1, _( "Pane 2 - sample text" ),
                                            wxDefaultPosition, wxSize( 200, 150 ),
                                            wxNO_BORDER | wxTE_MULTILINE );

        wxTextCtrl* text3 = new wxTextCtrl( this, -1, _( "Main content window" ),
                                            wxDefaultPosition, wxSize( 200, 150 ),
                                            wxNO_BORDER | wxTE_MULTILINE );

        // add the panes to the manager
        m_mgr.AddPane( layerWidget, wxLEFT, wxT( "Layer Visibility" ) );
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


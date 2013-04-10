#include <common.h>
#include <colors_selection.h>
#include <layers_id_colors_and_visibility.h>
#include <bitmaps.h>
#include <colors.h>

#include <wx/wx.h>
#include <wx/ownerdrw.h>
#include <wx/menuitem.h>
#include <wx/aui/aui.h>

#include <class_layer_box_selector.h>

/* class to display a layer list.
 *
 */

LAYER_BOX_SELECTOR::LAYER_BOX_SELECTOR( wxAuiToolBar* parent, wxWindowID id,
                                        const wxPoint& pos, const wxSize& size,
                                        int n, const wxString choices[] ) :
    wxBitmapComboBox( parent, id, wxEmptyString, pos, size, n, choices, wxCB_READONLY )
{
    m_layerorder   = true;
    m_layerhotkeys = true;
    m_hotkeys      = NULL;

    if( choices != NULL )
        ResyncBitmapOnly();
}


LAYER_BOX_SELECTOR::LAYER_BOX_SELECTOR( wxAuiToolBar* parent, wxWindowID id,
                                        const wxPoint& pos, const wxSize& size,
                                        const wxArrayString& choices ) :
    wxBitmapComboBox( parent, id, wxEmptyString, pos, size, choices, wxCB_READONLY )
{
    m_layerorder   = true;
    m_layerhotkeys = true;
    m_hotkeys      = NULL;

    if( !choices.IsEmpty() )
        ResyncBitmapOnly();
}


bool LAYER_BOX_SELECTOR::SetLayersOrdered( bool value )
{
    m_layerorder = value;
    return m_layerorder;
}


bool LAYER_BOX_SELECTOR::SetLayersHotkeys( bool value )
{
    m_layerhotkeys = value;
    return m_layerhotkeys;
}


// Get Current Item #
int LAYER_BOX_SELECTOR::GetChoice()
{
    return GetSelection();
}


// Get Current Layer
LAYER_NUM LAYER_BOX_SELECTOR::GetLayerSelection() const
{
    return (LAYER_NUM)((unsigned long) GetClientData( GetSelection() ) );
}


// Set Layer #
int LAYER_BOX_SELECTOR::SetLayerSelection( LAYER_NUM layer )
{
    int elements = GetCount();

    for( int i = 0; i < elements; i++ )
    {
        if( GetClientData( i ) == (void*)(intptr_t) layer )
        {
            if( GetSelection() != i )   // Element (i) is not selected
            {
                SetSelection( i );
                return i;
            }
            else
                return i;               //If element already selected; do nothing
        }
    }

    // Not Found
    SetSelection( -1 );
    return -1;
}

void LAYER_BOX_SELECTOR::ResyncBitmapOnly()
{
    LAYER_NUM elements = GetCount();
    for( LAYER_NUM i = FIRST_LAYER; i < elements; ++i )
    {
        wxBitmap layerbmp( 14, 14 );
        SetBitmapLayer( layerbmp, i );
    }
}


void LAYER_BOX_SELECTOR::SetBitmapLayer( wxBitmap& aLayerbmp, LAYER_NUM aLayer )
{
    wxMemoryDC bmpDC;
    wxBrush    brush;

    // Prepare Bitmap
    bmpDC.SelectObject( aLayerbmp );
    brush.SetColour( MakeColour( GetLayerColor( aLayer ) ) );
    brush.SetStyle( wxSOLID );

    bmpDC.SetBrush( brush );
    bmpDC.DrawRectangle( 0, 0, aLayerbmp.GetWidth(), aLayerbmp.GetHeight() );
    bmpDC.SetBrush( *wxTRANSPARENT_BRUSH );
    bmpDC.SetPen( *wxBLACK_PEN );
    bmpDC.DrawRectangle( 0, 0, aLayerbmp.GetWidth(), aLayerbmp.GetHeight() );
}

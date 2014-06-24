#include <common.h>
#include <colors_selection.h>
#include <layers_id_colors_and_visibility.h>
#include <bitmaps.h>
#include <colors.h>

#include <wx/wx.h>
#include <wx/ownerdrw.h>
#include <wx/menuitem.h>

#include <class_layer_box_selector.h>


LAYER_SELECTOR::LAYER_SELECTOR()
{
    m_layerhotkeys = true;
    m_hotkeys      = NULL;
}


bool LAYER_SELECTOR::SetLayersHotkeys( bool value )
{
    m_layerhotkeys = value;
    return m_layerhotkeys;
}


void LAYER_SELECTOR::SetBitmapLayer( wxBitmap& aLayerbmp, LAYER_ID aLayer )
{
    wxMemoryDC bmpDC;
    wxBrush    brush;

    // Prepare Bitmap
    bmpDC.SelectObject( aLayerbmp );
    brush.SetColour( MakeColour( GetLayerColor( aLayer ) ) );

#if wxCHECK_VERSION( 3, 0, 0 )
    brush.SetStyle( wxBRUSHSTYLE_SOLID );
#else
    brush.SetStyle( wxSOLID );
#endif

    bmpDC.SetBrush( brush );
    bmpDC.DrawRectangle( 0, 0, aLayerbmp.GetWidth(), aLayerbmp.GetHeight() );
    bmpDC.SetBrush( *wxTRANSPARENT_BRUSH );
    bmpDC.SetPen( *wxBLACK_PEN );
    bmpDC.DrawRectangle( 0, 0, aLayerbmp.GetWidth(), aLayerbmp.GetHeight() );
}

/* class to display a layer list in a wxBitmapComboBox.
 */
LAYER_BOX_SELECTOR::LAYER_BOX_SELECTOR( wxWindow* parent, wxWindowID id,
                                        const wxPoint& pos, const wxSize& size,
                                        int n, const wxString choices[] ) :
    wxBitmapComboBox( parent, id, wxEmptyString, pos, size, n, choices, wxCB_READONLY ),
    LAYER_SELECTOR()
{
    if( choices != NULL )
        ResyncBitmapOnly();
}


LAYER_BOX_SELECTOR::LAYER_BOX_SELECTOR( wxWindow* parent, wxWindowID id,
                                        const wxPoint& pos, const wxSize& size,
                                        const wxArrayString& choices ) :
    wxBitmapComboBox( parent, id, wxEmptyString, pos, size, choices, wxCB_READONLY ),
    LAYER_SELECTOR()
{
    if( !choices.IsEmpty() )
        ResyncBitmapOnly();
}


// Get Current Item #
int LAYER_BOX_SELECTOR::GetChoice()
{
    return GetSelection();
}


// Get Current Layer
LAYER_NUM LAYER_BOX_SELECTOR::GetLayerSelection() const
{
    if( GetSelection() < 0 )
        return UNDEFINED_LAYER;

    return (LAYER_NUM)(intptr_t) GetClientData( GetSelection() );
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
    int elements = GetCount();

    for( LAYER_NUM i = 0; i < elements; ++i )
    {
        wxBitmap layerbmp( 14, 14 );
        SetBitmapLayer( layerbmp, LAYER_ID( i ) );
    }
}


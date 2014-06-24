#ifndef CLASS_GBR_LAYER_BOX_SELECTOR_H
#define CLASS_GBR_LAYER_BOX_SELECTOR_H 1

#include <class_layer_box_selector.h>


// class to display a layer list in GerbView.
class GBR_LAYER_BOX_SELECTOR : public LAYER_BOX_SELECTOR
{
public:
    GBR_LAYER_BOX_SELECTOR( wxWindow* parent, wxWindowID id,
            const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
            int n = 0, const wxString choices[] = NULL ) :
        LAYER_BOX_SELECTOR( parent, id, pos, size, n, choices )
    {
        m_layerhotkeys = false;
    }

    // Reload the Layers names and bitmaps
    // Virtual function
    void Resync();

    // Returns a color index from the layer id
    // Virtual function
    EDA_COLOR_T GetLayerColor( LAYER_NUM aLayer ) const;

    // Returns true if the layer id is enabled (i.e. is it should be displayed)
    // Virtual function
    bool IsLayerEnabled( LAYER_NUM aLayer ) const { return true; };

    // Returns the name of the layer id
    // Virtual function
    wxString GetLayerName( LAYER_NUM aLayer ) const;
};

#endif //CLASS_GBR_LAYER_BOX_SELECTOR_H

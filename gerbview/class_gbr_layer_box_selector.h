#ifndef CLASS_GBR_LAYER_BOX_SELECTOR_H
#define CLASS_GBR_LAYER_BOX_SELECTOR_H 1

#include <class_layer_box_selector.h>


/* class to display a layer list in GerbView.
 *
 */

class GBR_LAYER_BOX_SELECTOR : public LAYER_BOX_SELECTOR
{
public:
    GBR_LAYER_BOX_SELECTOR( wxAuiToolBar* parent, wxWindowID id,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize,
                        int n = 0, const wxString choices[] = NULL )
        :LAYER_BOX_SELECTOR( parent, id, pos, size, n, choices )
        {
            m_layerhotkeys = false;
            m_layerorder = false;
        }

    GBR_LAYER_BOX_SELECTOR( wxAuiToolBar* parent, wxWindowID id,
                        const wxPoint& pos, const wxSize& size,
                        const wxArrayString& choices )
        :LAYER_BOX_SELECTOR( parent, id, pos, size, choices )
        {
            m_layerhotkeys = false;
            m_layerorder = false;
        }


    // Reload the Layers names and bitmaps
    // Virtual function
    void Resync();

    // Returns a color index from the layer id
    // Virtual function
    int GetLayerColor( int aLayerIndex );

    // Returns true if the layer id is enabled (i.e. is it should be displayed)
    // Virtual function
    bool IsLayerEnabled( int aLayerIndex ) { return true; };

    // Returns the name of the layer id
    // Virtual function
    const wxString GetLayerName( int aLayerIndex );
};

#endif //CLASS_GBR_LAYER_BOX_SELECTOR_H

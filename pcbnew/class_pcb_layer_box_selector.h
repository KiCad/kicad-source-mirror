#ifndef CLASS_PCB_PCB_LAYER_BOX_SELECTOR_H
#define CLASS_PCB_PCB_LAYER_BOX_SELECTOR_H 1

#include <class_layer_box_selector.h>


/* class to display a layer list in Pcbnew.
 *
 */

/* class to display a layer list.
 *
 */

class PCB_LAYER_BOX_SELECTOR : public LAYER_BOX_SELECTOR
{
public:
    // Hotkey Info
    struct EDA_HOTKEY_CONFIG* m_hotkeys;

public:
    PCB_LAYER_BOX_SELECTOR( wxAuiToolBar* parent, wxWindowID id,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize,
                        int n = 0, const wxString choices[] = NULL )
        :LAYER_BOX_SELECTOR( parent, id, pos, size, n, choices )
        {
        }

    PCB_LAYER_BOX_SELECTOR( wxAuiToolBar* parent, wxWindowID id,
                        const wxPoint& pos, const wxSize& size,
                        const wxArrayString& choices )
        :LAYER_BOX_SELECTOR( parent, id, pos, size, choices )
        {
        }

    // Reload the Layers names and bitmaps
    // Virtual function
    void Resync();

    // Returns a color index from the layer id
    // Virtual function
    EDA_COLOR_T GetLayerColor( LAYER_NUM aLayerIndex ) const;

    // Returns true if the layer id is enabled (i.e. is it should be displayed)
    // Virtual function
    bool IsLayerEnabled( LAYER_NUM aLayerIndex ) const;

    // Returns the name of the layer id
    // Virtual function
    wxString GetLayerName( LAYER_NUM aLayerIndex ) const;
};

#endif //CLASS_PCB_PCB_LAYER_BOX_SELECTOR_H

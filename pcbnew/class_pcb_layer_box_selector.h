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
    int GetLayerColor( int aLayerIndex );

    // Returns true if the layer id is enabled (i.e. is it should be displayed)
    // Virtual function
    bool IsLayerEnabled( int aLayerIndex );

    // Returns the name of the layer id
    // Virtual function
    const wxString GetLayerName( int aLayerIndex );
};

#define DECLARE_LAYERS_HOTKEY(list) int list[LAYER_COUNT] = \
        { \
            HK_SWITCH_LAYER_TO_COPPER,   \
            HK_SWITCH_LAYER_TO_INNER1,   \
            HK_SWITCH_LAYER_TO_INNER2,   \
            HK_SWITCH_LAYER_TO_INNER3,   \
            HK_SWITCH_LAYER_TO_INNER4,   \
            HK_SWITCH_LAYER_TO_INNER5,   \
            HK_SWITCH_LAYER_TO_INNER6,   \
            HK_SWITCH_LAYER_TO_INNER7,   \
            HK_SWITCH_LAYER_TO_INNER8,   \
            HK_SWITCH_LAYER_TO_INNER9,   \
            HK_SWITCH_LAYER_TO_INNER10,  \
            HK_SWITCH_LAYER_TO_INNER11,  \
            HK_SWITCH_LAYER_TO_INNER12,  \
            HK_SWITCH_LAYER_TO_INNER13,  \
            HK_SWITCH_LAYER_TO_INNER14,  \
            HK_SWITCH_LAYER_TO_COMPONENT \
        };
#endif //CLASS_PCB_PCB_LAYER_BOX_SELECTOR_H

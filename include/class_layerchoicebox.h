#ifndef CLASS_LAYERCHOICEBOX_H
#define CLASS_LAYERCHOICEBOX_H 1

#include <wx/bmpcbox.h>

/* class to display a layer list.
 *
 */

class WinEDALayerChoiceBox : public wxBitmapComboBox
{
private:
    bool m_layerhotkeys;
    bool m_layerorder;
public:
    WinEDALayerChoiceBox( WinEDA_Toolbar* parent, wxWindowID id,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     int n = 0, const wxString choices[] = NULL );

    WinEDALayerChoiceBox( WinEDA_Toolbar* parent, wxWindowID id,
                     const wxPoint& pos, const wxSize& size,
                     const wxArrayString& choices );

    // Get Current Item #
    int GetChoice();

    // Get Current Layer
    int GetLayerSelection();

    // Set Layer #
    int SetLayerSelection(int layer);

    // Reload the Layers
    void Resync();

    bool SetLayersOrdered(bool value);
    bool SetLayersHotkeys(bool value);
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

#endif //CLASS_LAYERCHOICEBOX_H

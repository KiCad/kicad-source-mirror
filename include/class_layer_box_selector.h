#ifndef CLASS_LAYER_BOX_SELECTOR_H
#define CLASS_LAYER_BOX_SELECTOR_H 1

#include <hotkeys_basic.h>
#include <wx/bmpcbox.h>


class wxAuiToolBar;


/* class to display a layer list.
 *
 */

class LAYER_BOX_SELECTOR : public wxBitmapComboBox
{
protected:
    bool m_layerhotkeys;
    bool m_layerorder;

public:
    // Hotkey Info
    struct EDA_HOTKEY_CONFIG* m_hotkeys;

public:
    LAYER_BOX_SELECTOR( wxAuiToolBar* parent, wxWindowID id,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize,
                        int n = 0, const wxString choices[] = NULL );

    LAYER_BOX_SELECTOR( wxAuiToolBar* parent, wxWindowID id,
                        const wxPoint& pos, const wxSize& size,
                        const wxArrayString& choices );

    // Returns a color index from the layer id
    // Virtual function because GerbView uses its own functions in a derived class
    virtual EDA_COLOR_T GetLayerColor( int aLayerIndex ) const = 0;

    // Returns the name of the layer id
    // Virtual pure function because GerbView uses its own functions in a derived class
    virtual wxString GetLayerName( int aLayerIndex ) const = 0;

    // Returns true if the layer id is enabled (i.e. is it should be displayed)
    // Virtual function pure because GerbView uses its own functions in a derived class
    virtual bool IsLayerEnabled( int aLayerIndex ) const = 0;

   // Get Current Item #
    int GetChoice();

    // Get Current Layer
    int GetLayerSelection();

    // Set Layer #
    int SetLayerSelection(int layer);

    // Reload the Layers
    // Virtual pure function because GerbView uses its own functions in a derived class
    virtual void Resync() = 0;

    // Reload the Layers bitmaps colors
    void ResyncBitmapOnly();

    bool SetLayersOrdered(bool value);
    bool SetLayersHotkeys(bool value);

protected:
   // Fills the layer bitmap aLayerbmp with the layer color
    void SetBitmapLayer( wxBitmap& aLayerbmp, int aLayerIndex );
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
#endif //CLASS_LAYER_BOX_SELECTOR_H

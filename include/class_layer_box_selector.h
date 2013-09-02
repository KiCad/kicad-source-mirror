#ifndef CLASS_LAYER_BOX_SELECTOR_H
#define CLASS_LAYER_BOX_SELECTOR_H 1

#include <wx/bmpcbox.h>
#include <colors.h>     // EDA_COLOR_T definition
#include <layers_id_colors_and_visibility.h>

class EDA_HOTKEY_CONFIG;

/* Basic class to build a layer list.
 * this is an basic abstract class to build a layer list selector.
 * To display this list, you should therefore derive this class
 */
class LAYER_SELECTOR
{
protected:
    bool m_layerhotkeys;
    bool m_layerorder;

public:
    // Hotkey Info
    struct EDA_HOTKEY_CONFIG* m_hotkeys;

public:
    LAYER_SELECTOR();

    // Returns a color index from the layer id
    // Virtual function because GerbView uses its own functions in a derived class
    virtual EDA_COLOR_T GetLayerColor( LAYER_NUM aLayer ) const = 0;

    // Returns the name of the layer id
    // Virtual pure function because GerbView uses its own functions in a derived class
    virtual wxString GetLayerName( LAYER_NUM aLayer ) const = 0;

    // Returns true if the layer id is enabled (i.e. is it should be displayed)
    // Virtual function pure because GerbView uses its own functions in a derived class
    virtual bool IsLayerEnabled( LAYER_NUM aLayer ) const = 0;

    bool SetLayersOrdered(bool value);
    bool SetLayersHotkeys(bool value);

protected:
   // Fills the layer bitmap aLayerbmp with the layer color
    void SetBitmapLayer( wxBitmap& aLayerbmp, LAYER_NUM aLayer );
};

/* class to display a layer list in a wxBitmapComboBox.
 */
class LAYER_BOX_SELECTOR :public wxBitmapComboBox, public LAYER_SELECTOR
{
public:
    // Hotkey Info
    struct EDA_HOTKEY_CONFIG* m_hotkeys;

public:
    LAYER_BOX_SELECTOR( wxWindow* parent, wxWindowID id,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize,
                        int n = 0, const wxString choices[] = NULL );

    LAYER_BOX_SELECTOR( wxWindow* parent, wxWindowID id,
                        const wxPoint& pos, const wxSize& size,
                        const wxArrayString& choices );

    // Get Current Item #
    int GetChoice();

    // Get Current Layer
    LAYER_NUM GetLayerSelection() const;

    // Set Layer #
    int SetLayerSelection(LAYER_NUM layer);

    // Reload the Layers
    // Virtual pure function because GerbView uses its own functions in a derived class
    virtual void Resync() = 0;

    // Reload the Layers bitmaps colors
    void ResyncBitmapOnly();
};

#define DECLARE_LAYERS_HOTKEY(list) int list[NB_LAYERS] = \
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

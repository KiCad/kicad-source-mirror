#ifndef CLASS_LAYER_BOX_SELECTOR_H
#define CLASS_LAYER_BOX_SELECTOR_H 1

#include <wx/bmpcbox.h>
#include <colors.h>     // EDA_COLOR_T definition
#include <layers_id_colors_and_visibility.h>

struct EDA_HOTKEY_CONFIG;

/* Basic class to build a layer list.
 * this is an basic abstract class to build a layer list selector.
 * To display this list, you should therefore derive this class
 */
class LAYER_SELECTOR
{
protected:
    bool m_layerhotkeys;

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
    void SetBitmapLayer( wxBitmap& aLayerbmp, LAYER_ID aLayer );
};


/* class to display a layer list in a wxBitmapComboBox.
 */
class LAYER_BOX_SELECTOR : public wxBitmapComboBox, public LAYER_SELECTOR
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
    int SetLayerSelection( LAYER_NUM layer );

    // Reload the Layers
    // Virtual pure function because GerbView uses its own functions in a derived class
    virtual void Resync() = 0;

    // Reload the Layers bitmaps colors
    void ResyncBitmapOnly();
};

#endif // CLASS_LAYER_BOX_SELECTOR_H

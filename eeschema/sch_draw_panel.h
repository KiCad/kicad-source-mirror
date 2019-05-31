#ifndef __SCH_DRAW_PANEL_H
#define __SCH_DRAW_PANEL_H

#include <class_draw_panel_gal.h>
#include <base_struct.h>
#include <gr_basic.h>
#include <eda_rect.h>


namespace KIGFX
{
    class SCH_VIEW;
    namespace PREVIEW
    {
        class SELECTION_AREA;
    };
};

class SCH_SHEET;
class LIB_PART;
class BASE_SCREEN;
class SCH_SCREEN;
class COLORS_DESIGN_SETTINGS;
class SCH_EDIT_FRAME;


class SCH_DRAW_PANEL : public EDA_DRAW_PANEL_GAL
{
public:
    SCH_DRAW_PANEL( wxWindow* aParentWindow, wxWindowID aWindowId, const wxPoint& aPosition,
                    const wxSize& aSize, KIGFX::GAL_DISPLAY_OPTIONS& aOptions,
                    GAL_TYPE aGalType = GAL_TYPE_OPENGL );

    ~SCH_DRAW_PANEL();

    void DisplayComponent( const LIB_PART *aComponent );
    void DisplaySheet( const SCH_SCREEN *aScreen );

    bool SwitchBackend( GAL_TYPE aGalType ) override;

    KIGFX::SCH_VIEW* GetView() const { return view(); }

protected:
    virtual void onPaint( wxPaintEvent& WXUNUSED( aEvent ) ) override;

    KIGFX::SCH_VIEW* view() const;
    wxWindow* m_parent;

    void setDefaultLayerOrder();    ///> Reassigns layer order to the initial settings.
    void setDefaultLayerDeps();     ///> Sets rendering targets & dependencies for layers.
};

#endif

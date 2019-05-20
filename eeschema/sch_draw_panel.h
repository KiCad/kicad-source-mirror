#ifndef __SCH_DRAW_PANEL_H
#define __SCH_DRAW_PANEL_H

#include <class_draw_panel_gal.h>
#include <base_struct.h>
#include <gr_basic.h>
#include <eda_rect.h>

#include <legacy_gal/class_drawpanel.h>


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


class SCH_DRAW_PANEL : public EDA_DRAW_PANEL, public EDA_DRAW_PANEL_GAL
{
public:
    SCH_DRAW_PANEL( wxWindow* aParentWindow, wxWindowID aWindowId, const wxPoint& aPosition,
                    const wxSize& aSize, KIGFX::GAL_DISPLAY_OPTIONS& aOptions,
                    GAL_TYPE aGalType = GAL_TYPE_OPENGL );

    ~SCH_DRAW_PANEL();

    virtual wxWindow* GetWindow() override { return this; }

    void DisplayComponent( const LIB_PART *aComponent );
    void DisplaySheet( const SCH_SCREEN *aScreen );

    bool SwitchBackend( GAL_TYPE aGalType ) override;
    void OnKeyEvent( wxKeyEvent& event );
    void OnCharHook( wxKeyEvent& event );

    void SetEnableMousewheelPan( bool aEnable ) override;
    void SetEnableZoomNoCenter( bool aEnable ) override;
    void SetEnableAutoPan( bool aEnable ) override;
    void SetAutoPanRequest( bool aEnable ) override;

    BASE_SCREEN* GetScreen() override;
    virtual EDA_DRAW_FRAME* GetParent() const override;

    virtual void MoveCursorToCrossHair() override;

    KIGFX::SCH_VIEW* GetView() const { return view(); }

    /// @copydoc wxWindow::Refresh()
    void Refresh( bool aEraseBackground = true, const wxRect* aRect = NULL ) override;


protected:
    virtual void onPaint( wxPaintEvent& WXUNUSED( aEvent ) ) override;

    KIGFX::SCH_VIEW* view() const;
    wxWindow* m_parent;

    void setDefaultLayerOrder();    ///> Reassigns layer order to the initial settings.
    void setDefaultLayerDeps();     ///> Sets rendering targets & dependencies for layers.

    DECLARE_EVENT_TABLE()
};

#endif

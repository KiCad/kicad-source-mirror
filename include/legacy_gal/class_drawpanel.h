#ifndef __EDA_DRAW_PANEL_H
#define __EDA_DRAW_PANEL_H

#include <wx/wx.h>
#include <base_struct.h>
#include <gr_basic.h>
#include <eda_rect.h>

class BASE_SCREEN;


class EDA_DRAW_PANEL
{
public:
    EDA_DRAW_PANEL()
    {};

    virtual ~EDA_DRAW_PANEL(){};

    virtual BASE_SCREEN* GetScreen() = 0;

    virtual EDA_DRAW_FRAME* GetParent() const = 0;

    // Only used for printing, so no clipping
    virtual EDA_RECT* GetClipBox() { return nullptr; }

    virtual void Refresh( bool eraseBackground = true, const wxRect* rect = NULL ) {}

    virtual wxWindow* GetWindow() = 0;
};

#endif

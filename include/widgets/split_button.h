#ifndef __SPLIT_BUTTON_H__
#define __SPLIT_BUTTON_H__

#include <wx/wx.h>
#include <wx/menu.h>

class SPLIT_BUTTON : public wxPanel
{
public:
    SPLIT_BUTTON(wxWindow* parent,
        wxWindowID id,
        const wxString& label,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize);

    ~SPLIT_BUTTON();

    wxMenu* GetSplitButtonMenu();

protected:
    void OnKillFocus(wxFocusEvent& event);
    void OnMouseLeave(wxMouseEvent& event);
    void OnMouseEnter(wxMouseEvent& event);
    void OnLeftButtonUp(wxMouseEvent& event);
    void OnLeftButtonDown(wxMouseEvent& event);
    void OnPaint(wxPaintEvent& WXUNUSED(event));

    bool Enable(bool enable = true) override;

private:
    int m_stateButton = 0;
    int m_stateMenu = 0;
    bool m_bIsEnable = true;
    wxColor m_colorNormal;
    wxColor m_colorDisabled;
    const int m_arrowButtonWidth = 20;
    bool m_bLButtonDown = false;
    wxString m_label;
    wxMenu* m_pMenu = nullptr;
};

#endif /*__SPLIT_BUTTON_H__*/
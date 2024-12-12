#pragma once

#include <wx/panel.h>
#include <functional>

class BOARD;
class NET_SELECTOR;
class NETCLASS_SELECTOR;
class AREA_SELECTOR;

class wxStaticText;
class wxChoice;
class wxBoxSizer;
class wxStyledTextCtrl;

/**
 * Panel used in the rule editor conditions to select object filters.
 */
class DRC_RE_OBJECT_SELECTOR_PANEL : public wxPanel
{
public:
    DRC_RE_OBJECT_SELECTOR_PANEL( wxWindow* parent, BOARD* board, const wxString& label );

    void SetLabelText( const wxString& text );
    void SetCustomQueryCtrl( wxStyledTextCtrl* ctrl );

    /**
     * Populate the panel controls from a rule condition expression.
     *
     * @param aExpr   the rule expression without the surrounding (condition "...") wrapper
     * @param aPrefix optional object designator ("A" or "B") that will be stripped if present
     */
    void ParseCondition( const wxString& aExpr, const wxString& aPrefix = wxEmptyString );

    /**
     * Build a rule condition expression based on the panel state.
     *
     * @param aPrefix object designator ("A" or "B") used when the expression refers to a
     *                single object.  Custom queries are returned verbatim without the prefix.
     */
    wxString BuildCondition( const wxString& aPrefix ) const;

    /**
     * @return true if the "Custom Query" option is selected.
     */
    bool HasCustomQuerySelected() const;

    /**
     * Set a callback to be invoked when the choice selection changes.
     */
    void SetChoiceChangeCallback( std::function<void()> aCallback )
    {
        m_choiceChangeCallback = aCallback;
    }

private:
    void onChoice( const wxCommandEvent& aEvent );

    wxStaticText*      m_label;
    wxChoice*          m_choice;
    NET_SELECTOR*      m_netSelector;
    NETCLASS_SELECTOR* m_netclassSelector;
    AREA_SELECTOR*     m_areaSelector;
    wxStyledTextCtrl*  m_customQueryCtrl;
    wxBoxSizer*        m_rowSizer;

    std::function<void()> m_choiceChangeCallback;
};


#ifndef __dialog_hotkeys_editor__
#define __dialog_hotkeys_editor__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/choice.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/listctrl.h>
#include <wx/dialog.h>
#include <wx/grid.h>

#include <hotkeys_basic.h>
#include <draw_frame.h>
#include <../common/dialogs/dialog_hotkeys_editor_base.h>

class HOTKEYS_EDITOR_DIALOG;

class HOTKEY_LIST_CTRL : public wxListCtrl
{
public:
    HOTKEY_LIST_CTRL( wxWindow* aParent, struct EDA_HOTKEY_CONFIG* aSection );
    ~HOTKEY_LIST_CTRL() {};

    void DeselectRow( int aRow );
    std::vector< EDA_HOTKEY* >& GetHotkeys() { return m_hotkeys; }
    void RestoreFrom( struct EDA_HOTKEY_CONFIG* aSection );

private:
    int m_curEditingRow;
    wxString* m_sectionTag;
    std::vector< EDA_HOTKEY* > m_hotkeys;

protected:
    wxString OnGetItemText( long aRow, long aColumn ) const;
    void OnChar( wxKeyEvent& aEvent );
    void OnListItemSelected( wxListEvent& aEvent );
    void OnSize( wxSizeEvent& aEvent );
};

class HOTKEY_SECTION_PAGE : public wxPanel
{
public:
private:
    EDA_HOTKEY_CONFIG*  m_hotkeySection;
    HOTKEY_LIST_CTRL *m_hotkeyList;
    HOTKEYS_EDITOR_DIALOG* m_dialog;

public:
    /** Constructor to create a setup page for one netlist format.
     * Used in Netlist format Dialog box creation
     * @param parent = wxNotebook * parent
     * @param title = title (name) of the notebook page
     * @param id_NetType = netlist type id
     */
    HOTKEY_SECTION_PAGE( HOTKEYS_EDITOR_DIALOG* aDialog, wxNotebook* aParent, 
                         const wxString& aTitle,
                         EDA_HOTKEY_CONFIG* aSection );
    ~HOTKEY_SECTION_PAGE() {};
    void Restore();

    std::vector< EDA_HOTKEY* >& GetHotkeys() { return m_hotkeyList->GetHotkeys(); }
    EDA_HOTKEY_CONFIG* GetHotkeySection() { return m_hotkeySection; }

    HOTKEYS_EDITOR_DIALOG* GetDialog() { return m_dialog; }
};


class HOTKEYS_EDITOR_DIALOG : public HOTKEYS_EDITOR_DIALOG_BASE
{
protected:
    EDA_DRAW_FRAME* m_parent;
    struct EDA_HOTKEY_CONFIG* m_hotkeys;
    
    std::vector<HOTKEY_SECTION_PAGE*> m_hotkeySectionPages;

public:
    HOTKEYS_EDITOR_DIALOG( EDA_DRAW_FRAME* aParent, EDA_HOTKEY_CONFIG* aHotkeys );

    ~HOTKEYS_EDITOR_DIALOG() {};

    bool CanSetKey( long aKey, const wxString* aSectionTag );

private:
    void OnOKClicked( wxCommandEvent& aEvent );
    void CancelClicked( wxCommandEvent& aEvent );
    void UndoClicked( wxCommandEvent& aEvent );
};

void InstallHotkeyFrame( EDA_DRAW_FRAME* aParent, EDA_HOTKEY_CONFIG* aHotkeys );

#endif

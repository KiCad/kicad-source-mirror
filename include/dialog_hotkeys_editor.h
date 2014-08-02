
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
#include <hotkey_grid_table.h>
#include <draw_frame.h>
#include <../common/dialogs/dialog_hotkeys_editor_base.h>

class HOTKEYS_EDITOR_DIALOG : public HOTKEYS_EDITOR_DIALOG_BASE
{
protected:
    EDA_DRAW_FRAME* m_parent;
    struct EDA_HOTKEY_CONFIG* m_hotkeys;
    HOTKEY_EDITOR_GRID_TABLE* m_table;

    int m_curEditingRow;

public:
    HOTKEYS_EDITOR_DIALOG( EDA_DRAW_FRAME* parent, EDA_HOTKEY_CONFIG* hotkeys );

    ~HOTKEYS_EDITOR_DIALOG() {};

private:
    void OnOKClicked( wxCommandEvent& event );
    void CancelClicked( wxCommandEvent& event );
    void UndoClicked( wxCommandEvent& event );
    void OnClickOnCell( wxGridEvent& event );
    void OnRightClickOnCell( wxGridEvent& event );
    void OnKeyPressed( wxKeyEvent& event );
    void SetHotkeyCellState( int aRow, bool aHightlight );
};

void InstallHotkeyFrame( EDA_DRAW_FRAME* parent, EDA_HOTKEY_CONFIG* hotkeys );

#endif

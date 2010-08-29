
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

#include "hotkeys_basic.h"
#include "hotkey_grid_table.h"
#include "wxstruct.h"
#include "dialog_hotkeys_editor_base.h"

class HOTKEYS_EDITOR_DIALOG : public HOTKEYS_EDITOR_DIALOG_BASE
{
protected:
    WinEDA_DrawFrame* m_parent;
    struct Ki_HotkeyInfoSectionDescriptor* m_hotkeys;
    HotkeyGridTable* m_table;

    int m_curEditingRow;

public:
    HOTKEYS_EDITOR_DIALOG( WinEDA_DrawFrame* parent,
                         Ki_HotkeyInfoSectionDescriptor* hotkeys );

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

void InstallHotkeyFrame( WinEDA_DrawFrame* parent,
                         Ki_HotkeyInfoSectionDescriptor* hotkeys );

#endif

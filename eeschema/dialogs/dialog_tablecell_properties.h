/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef DIALOG_TABLECELL_PROPERTIES_H
#define DIALOG_TABLECELL_PROPERTIES_H

#include <widgets/unit_binder.h>
#include <dialog_tablecell_properties_base.h>


class SCH_EDIT_FRAME;
class SCH_TABLE;
class SCH_TABLECELL;
class SCINTILLA_TRICKS;
class HTML_MESSAGE_BOX;

class DIALOG_TABLECELL_PROPERTIES : public DIALOG_TABLECELL_PROPERTIES_BASE
{
public:
    // The dialog can be closed for several reasons.
    enum TABLECELL_PROPS_RETVALUE
    {
        TABLECELL_PROPS_CANCEL,
        TABLECELL_PROPS_OK,
        TABLECELL_PROPS_EDIT_TABLE
    };

    DIALOG_TABLECELL_PROPERTIES( SCH_EDIT_FRAME* aParentFrame, std::vector<SCH_TABLECELL*> aCells );
    ~DIALOG_TABLECELL_PROPERTIES();

    ///< @return the value depending on the way the dialog was closed.
    enum TABLECELL_PROPS_RETVALUE GetReturnValue() { return m_returnValue; }

private:
    void getContextualTextVars( const wxString& aCrossRef, wxArrayString* aTokens );
    void onMultiLineTCLostFocus( wxFocusEvent& event ) override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void onHAlignButton( wxCommandEvent& aEvent );
    void onVAlignButton( wxCommandEvent& aEvent );
    void onTextColorPopup( wxCommandEvent& aEvent ) override;
    void onFillColorPopup( wxCommandEvent& aEvent ) override;

    void onEditTable( wxCommandEvent& aEvent ) override;
    void OnFormattingHelp( wxHyperlinkEvent& aEvent ) override;

    void OnSystemColourChanged( wxSysColourChangedEvent& event );

    void UpdateTheme( void );

private:
    SCH_EDIT_FRAME*             m_frame;
    SCH_TABLE*                  m_table;
    std::vector<SCH_TABLECELL*> m_cells;
    SCINTILLA_TRICKS*           m_scintillaTricks;
    HTML_MESSAGE_BOX*           m_helpWindow;

    UNIT_BINDER       m_textSize;
    UNIT_BINDER       m_marginLeft;
    UNIT_BINDER       m_marginTop;
    UNIT_BINDER       m_marginRight;
    UNIT_BINDER       m_marginBottom;
    wxStyledTextCtrl* m_cellText;

    enum TABLECELL_PROPS_RETVALUE m_returnValue; // the option that closed the dialog
};


#endif // DIALOG_TABLECELL_PROPERTIES_H

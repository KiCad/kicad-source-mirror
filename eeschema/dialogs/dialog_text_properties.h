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

#ifndef DIALOG_TEXT_PROPERTIES_H
#define DIALOG_TEXT_PROPERTIES_H

#include <widgets/unit_binder.h>
#include <sch_text.h>
#include <dialog_text_properties_base.h>


class SCH_EDIT_FRAME;
class SCH_TEXT;
class SCINTILLA_TRICKS;
class HTML_MESSAGE_BOX;


class DIALOG_TEXT_PROPERTIES : public DIALOG_TEXT_PROPERTIES_BASE
{
public:
    DIALOG_TEXT_PROPERTIES( SCH_BASE_FRAME* parent, SCH_ITEM* aTextItem );
    ~DIALOG_TEXT_PROPERTIES() override;

private:
    void getContextualTextVars( const wxString& aCrossRef, wxArrayString* aTokens );

    void onHAlignButton( wxCommandEvent &aEvent );
    void onVAlignButton( wxCommandEvent &aEvent );
    void onTextAngleButton( wxCommandEvent &aEvent );
    void onBorderChecked( wxCommandEvent& aEvent ) override;
    void onFillChecked( wxCommandEvent& aEvent ) override;
    void onHyperlinkChecked( wxCommandEvent& aEvent ) override;
    void onHyperlinkText( wxCommandEvent& aEvent ) override;
    void onHyperlinkCombo( wxCommandEvent& aEvent ) override;

    void OnFormattingHelp( wxHyperlinkEvent& aEvent ) override;
    void onMultiLineTCLostFocus( wxFocusEvent& event ) override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    SCH_BASE_FRAME*       m_frame;
    bool                  m_isSymbolEditor;
    SCH_ITEM*             m_currentItem;
    EDA_TEXT*             m_currentText;
    UNIT_BINDER           m_textSize;
    UNIT_BINDER           m_borderWidth;
    SCINTILLA_TRICKS*     m_scintillaTricks;
    std::vector<wxString> m_pageNumbers;

    HTML_MESSAGE_BOX*     m_helpWindow;

    wxString              m_lastLink;
};



#endif // DIALOG_TEXT_PROPERTIES_H

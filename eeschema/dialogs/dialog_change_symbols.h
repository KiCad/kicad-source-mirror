#ifndef _DIALOG_CHANGE_SYMBOLS_H_
#define _DIALOG_CHANGE_SYMBOLS_H_

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 *
 * @author Wayne Stambaugh <stambaughw@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <dialog_change_symbols_base.h>

class LIB_ID;
class SCH_COMPONENT;
class SCH_EDIT_FRAME;
class SCH_SCREEN;
class SCH_SHEET_PATH;

/**
 * Dialog to update or change schematic library symbols.
 */
class DIALOG_CHANGE_SYMBOLS : public DIALOG_CHANGE_SYMBOLS_BASE
{
public:
    enum class MODE { CHANGE, UPDATE };

    DIALOG_CHANGE_SYMBOLS( SCH_EDIT_FRAME* aParent, SCH_COMPONENT* aSymbol,
            MODE aMode = MODE::UPDATE );
    ~DIALOG_CHANGE_SYMBOLS() override;

protected:
    void launchMatchIdSymbolBrowser( wxCommandEvent& aEvent ) override;
    void launchNewIdSymbolBrowser( wxCommandEvent& aEvent ) override;
    void onOkButtonClicked( wxCommandEvent& aEvent ) override;
    void onMatchByReference( wxCommandEvent& aEvent ) override;
    void onMatchByValue( wxCommandEvent& aEvent ) override;
    void onMatchById( wxCommandEvent& aEvent ) override;

private:
    bool isMatch( SCH_COMPONENT* aSymbol, SCH_SHEET_PATH* aInstance );
    bool processMatchingSymbols();
    bool processSymbol( SCH_COMPONENT* aSymbol, SCH_SCREEN* aScreen, const LIB_ID& aNewId,
            bool aAppendToUndo );

    SCH_COMPONENT* m_symbol;
    MODE           m_mode;
};

#endif // _DIALOG_CHANGE_SYMBOLS_H_

/*
 * Copyright (C) 2018 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef DIALOG_PRINT_GENERIC_H
#define DIALOG_PRINT_GENERIC_H

#include <dialogs/dialog_print_generic_base.h>
#include <wx/valnum.h>
#include <widgets/unit_binder.h>

class EDA_DRAW_FRAME;
class APP_SETTINGS_BASE;
struct PRINTOUT_SETTINGS;
class wxPageSetupDialogData;
class wxPrintout;
class wxPrintData;

class DIALOG_PRINT_GENERIC : public DIALOG_PRINT_GENERIC_BASE
{
public:
    DIALOG_PRINT_GENERIC( EDA_DRAW_FRAME* aParent, PRINTOUT_SETTINGS* aSettings );
    virtual ~DIALOG_PRINT_GENERIC() = default;

    /**
     * Set 'print border and title block' to a requested value and hides the
     * corresponding checkbox.
     */
    void ForcePrintBorder( bool aValue );

protected:
    /**
     * Create a printout with a requested title.
     */
    virtual wxPrintout* createPrintout( const wxString& aTitle ) = 0;

    virtual void saveSettings();

    wxSizer* getMainSizer()
    {
        return m_bUpperSizer;
    }

    wxGridBagSizer* getOptionsSizer()
    {
        return m_gbOptionsSizer;
    }

    wxStaticBox* getOptionsBox()
    {
        return m_sbOptionsSizer->GetStaticBox();
    }

    /**
     * Return scale value selected in the dialog.
     *
     * If this value is outside limits, it will be clamped.
     */
    double getScaleValue();

    /**
    * Select a corresponding scale radio button and update custom scale value if needed.
    *
    * @param aValue is the scale value to be selected (0 stands for fit-to-page).
    */
    void setScaleValue( double aValue );

    // There is no TransferDataFromWindow() so options are saved
    // even if the dialog is closed without printing
    bool TransferDataToWindow() override;

private:
    void onPageSetup( wxCommandEvent& event ) override;
    void onPrintPreview( wxCommandEvent& event ) override;
    void onPrintButtonClick( wxCommandEvent& event ) override;
    void onCancelButtonClick( wxCommandEvent& aEvent ) override;

    // Needed to save the dialogs settings as TransferDataFromWindow()
    // is not called for 'Cancel' button that closes the window.
    void onClose( wxCloseEvent& event ) override;

    void onSetCustomScale( wxCommandEvent& event ) override;

    void initPrintData();

protected:
    PRINTOUT_SETTINGS* m_settings;

private:
    static wxPrintData*           s_printData;
    static wxPageSetupDialogData* s_pageSetupData;
};

#endif // DIALOG_PRINT_GENERIC_H

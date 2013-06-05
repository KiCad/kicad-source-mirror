
#ifndef _DIALOG_PRINT_USING_PRINTER_H_
#define _DIALOG_PRINT_USING_PRINTER_H_


#include <dialog_print_using_printer_base.h>


/**
 * Print schematic dialog.
 *
 * Class derived from DIALOG_PRINT_USING_PRINTER_base created by wxFormBuilder
 */
class DIALOG_PRINT_USING_PRINTER : public DIALOG_PRINT_USING_PRINTER_BASE
{
public:
    DIALOG_PRINT_USING_PRINTER( SCH_EDIT_FRAME* aParent );
    ~DIALOG_PRINT_USING_PRINTER() {};

    SCH_EDIT_FRAME* GetParent() const;

private:
    void OnCloseWindow( wxCloseEvent& event );
    void OnInitDialog( wxInitDialogEvent& event );
    void OnPageSetup( wxCommandEvent& event );
    void OnPrintPreview( wxCommandEvent& event );
    void OnPrintButtonClick( wxCommandEvent& event );
    void OnButtonCancelClick( wxCommandEvent& event ){ Close(); }

    void GetPrintOptions();
};


#endif    // _DIALOG_PRINT_USING_PRINTER_H_

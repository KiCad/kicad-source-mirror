// file class_DCodeSelectionbox.h

#ifndef CLASS_DCODESELECTIONBOX_H
#define CLASS_DCODESELECTIONBOX_H

/* helper class to display a DCode list and select a DCode id.
 */

// Define event type for DCODE_SELECTION_BOX
#define EVT_SELECT_DCODE EVT_COMBOBOX

class DCODE_SELECTION_BOX : public wxComboBox
{
private:
    const wxArrayString* m_dcodeList;

public: DCODE_SELECTION_BOX( WinEDA_Toolbar* aParent, wxWindowID aId,
                             const wxPoint& aLocation, const wxSize& aSize,
                             const wxArrayString& aChoices);
        ~DCODE_SELECTION_BOX();

    /**
     * Function GetSelectedDCodeId
     * @return the current selected DCode Id or -1 if no dcode
     */
    int GetSelectedDCodeId();
    /**
     * Function SetDCodeSelection
     * @param aDCodeId = the DCode Id to select or -1 to select "no dcode"
     */
    void SetDCodeSelection( int aDCodeId );
};

#endif //CLASS_DCODESELECTIONBOX_H

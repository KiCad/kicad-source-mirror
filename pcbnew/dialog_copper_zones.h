/* dialog_copper_zones.h */

#ifndef DIALOG_COPPER_ZONES
#define DIALOG_COPPER_ZONES

#include "dialog_copper_zones_base.h"

/* here is the derivated class from dialog_copper_zone_frame created by wxFormBuilder
*/
class dialog_copper_zone: public dialog_copper_zone_base
{
public:
    WinEDA_PcbFrame* m_Parent;
    ZONE_SETTING * m_Zone_Setting;
    long m_NetSorting;
	int m_LayerId[LAYER_COUNT];		// Handle the real layer number from layer name position in m_LayerSelectionCtrl

public:
    dialog_copper_zone( WinEDA_PcbFrame* parent, ZONE_SETTING * zone_setting);
    void OnInitDialog( wxInitDialogEvent& event );
	void OnButtonOkClick( wxCommandEvent& event );
	void OnButtonCancelClick( wxCommandEvent& event );
    bool AcceptOptions(bool aPromptForErrors, bool aUseExportableSetupOnly = false);
    void OnNetSortingOptionSelected( wxCommandEvent& event );
	void ExportSetupToOtherCopperZones( wxCommandEvent& event );
	void OnPadsInZoneClick( wxCommandEvent& event );
};

#endif      // #ifndef DIALOG_COPPER_ZONES

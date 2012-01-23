/* dialog_copper_zones.h */

#ifndef DIALOG_COPPER_ZONES_
#define DIALOG_COPPER_ZONES_

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <dialog_copper_zones_base.h>

/**
 * Class DIALOG_COPPER_ZONE
 * is the derivated class from dialog_copper_zone_frame created by wxFormBuilder
 */
class DIALOG_COPPER_ZONE : public DIALOG_COPPER_ZONE_BASE
{
private:
    PCB_EDIT_FRAME* m_Parent;
    wxConfig*       m_Config;               ///< Current config

    int             m_OnExitCode;           ///< exit code: ZONE_ABORT if no change,
                                            ///< ZONE_OK if new values accepted
                                            ///< ZONE_EXPORT_VALUES if values are exported to others zones

    ZONE_SETTING*   m_Zone_Setting;

    bool            m_NetSortingByPadCount; ///< false = alphabetic sort.
                                            ///< true = pad count sort.

    long            m_NetFiltering;
    std::vector<int> m_LayerId;             ///< Handle the real layer number from layer
                                            ///< name position in m_LayerSelectionCtrl

    static wxString m_netNameShowFilter;    ///< the filter to show nets (default * "*").
                                            ///< static to keep this pattern for an entire pcbnew session

    wxListView*     m_LayerSelectionCtrl;

    static wxPoint  prevPosition;           ///< Dialog position & size
    static wxSize   prevSize;

public:
    DIALOG_COPPER_ZONE( PCB_EDIT_FRAME* parent, ZONE_SETTING* zone_setting );
private:

    /**
     * Function initDialog
     * fills in the dialog controls using the current settings.
     */
    void initDialog();

    void OnButtonOkClick( wxCommandEvent& event );
    void OnButtonCancelClick( wxCommandEvent& event );
    void OnClose( wxCloseEvent& event );
    void OnSize( wxSizeEvent& event );
    void OnCornerSmoothingModeChoice( wxCommandEvent& event );

    /**
     * Function AcceptOptions
     * @param aPromptForErrors is true to prompt user on incorrect params.
     * @param aUseExportableSetupOnly is true to use exportable parametres only (used to export this setup to other zones).
     * @return bool - false if incorrect options, true if ok.
     */
    bool AcceptOptions( bool aPromptForErrors, bool aUseExportableSetupOnly = false );

    void OnNetSortingOptionSelected( wxCommandEvent& event );
    void ExportSetupToOtherCopperZones( wxCommandEvent& event );
    void OnPadsInZoneClick( wxCommandEvent& event );
    void OnRunFiltersButtonClick( wxCommandEvent& event );


    void buildAvailableListOfNets();

    /**
     * Function initListNetsParams
     * initializes m_NetSortingByPadCount and m_NetFiltering values
     * according to m_NetDisplayOption selection.
     */
    void initListNetsParams();

    /**
     * Function makeLayerBitmap
     * creates the colored rectangle bitmaps used in the layer selection widget.
     * @param aColor is the color to fill the rectangle with.
     */
    wxBitmap makeLayerBitmap( int aColor );
};

#endif  // DIALOG_COPPER_ZONES_

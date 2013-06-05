/**
 * @file transline_ident.h
 */

#ifndef TRANSLINE_IDENT_H
#define TRANSLINE_IDENT_H

#include <vector>
#include <wx/config.h>
#include <wx/bitmap.h>

#include <transline.h>

// An enum to handle muwave shapes:
enum transline_type_id {
    start_of_list_type = 0,
    default_type = start_of_list_type,
    microstrip_type = default_type,
    cpw_type,
    grounded_cpw_type,
    rectwaveguide_type,
    coax_type,
    c_microstrip_type,
    stripline_type,
    twistedpair_type,
    end_of_list_type
};

// A Class to handle parameters
enum PRM_TYPE {
    PRM_TYPE_SUBS,
    PRM_TYPE_PHYS,
    PRM_TYPE_ELEC,
    PRM_TYPE_FREQUENCY
};

class TRANSLINE_PRM
{
public:
    PRM_TYPE m_Type;            // Type of parameter: substr, physical, elect
    PRMS_ID  m_Id;              // Id of parameter ( link to transline functions )
    wxString m_Label;           // name for this parameter in dialog
    wxString m_ToolTip;         // Tool tip for this parameter in dialog
    double   m_Value;           // Value for this parameter in dialog
    double   m_NormalizedValue; // actual value for this parameter
    bool     m_ConvUnit;        // true if an unit selector must be used
    void*    m_ValueCtrl;       // The text ctrl containing the value in dialog
    void*    m_UnitCtrl;        // The UNIT_SELECTOR containing the unit in dialog
    int      m_UnitSelection;   // last selection for units

public: TRANSLINE_PRM( PRM_TYPE aType, PRMS_ID aId,
                       const wxString& aLabel = wxEmptyString,
                       const wxString& aToolTip = wxEmptyString,
                       double aValue = 0.0,
                       bool aConvUnit = false );

    void   ReadConfig( wxConfig* aConfig );
    void   WriteConfig( wxConfig* aConfig );
    double ToUserUnit();
    double FromUserUnit();
};


// A class to handle the list of availlable transm. lines
// with messages, tooptips ...
class TRANSLINE_IDENT
{
public:
    enum transline_type_id m_Type;              // The type of transline handled
    wxBitmap *             m_Icon;              // An icon to display in dialogs
    TRANSLINE*             m_TLine;             // The TRANSLINE itself
    wxArrayString          m_Messages;          // messages for results
    bool m_HasPrmSelection;                     // true if selection of parameters must be enabled in dialog menu

private:
    std::vector <TRANSLINE_PRM*> m_prms_List;

public: TRANSLINE_IDENT( enum transline_type_id aType );
    ~TRANSLINE_IDENT();

    // Add a new param in list
    void AddPrm( TRANSLINE_PRM* aParam )
    {
        m_prms_List.push_back( aParam );
    }


    TRANSLINE_PRM* GetPrm( unsigned aIdx )
    {
        if( aIdx <  m_prms_List.size() )
            return m_prms_List[aIdx];
        else
            return NULL;
    }


    unsigned GetPrmsCount()
    {
        return m_prms_List.size();
    }


    void ReadConfig( wxConfig* aConfig );
    void WriteConfig( wxConfig* aConfig );
};

#endif      //  TRANSLINE_IDENT_H

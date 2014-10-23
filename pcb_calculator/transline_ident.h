/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011-2014 Jean-Pierre Charras
 * Copyright (C) 2004-2014 KiCad Developers, see change_log.txt for contributors.
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
enum TRANSLINE_TYPE_ID {
    START_OF_LIST_TYPE = 0,
    DEFAULT_TYPE = START_OF_LIST_TYPE,
    MICROSTRIP_TYPE = DEFAULT_TYPE,
    CPW_TYPE,
    GROUNDED_CPW_TYPE,
    RECTWAVEGUIDE_TYPE,
    COAX_TYPE,
    C_MICROSTRIP_TYPE,
    STRIPLINE_TYPE,
    TWISTEDPAIR_TYPE,
    END_OF_LIST_TYPE
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

    void   ReadConfig( wxConfigBase* aConfig );
    void   WriteConfig( wxConfigBase* aConfig );
    double ToUserUnit();
    double FromUserUnit();
};


// A class to handle the list of availlable transm. lines
// with messages, tooptips ...
class TRANSLINE_IDENT
{
public:
    enum TRANSLINE_TYPE_ID m_Type;              // The type of transline handled
    wxBitmap *             m_Icon;              // An icon to display in dialogs
    TRANSLINE*             m_TLine;             // The TRANSLINE itself
    wxArrayString          m_Messages;          // messages for results
    bool m_HasPrmSelection;                     // true if selection of parameters must be enabled in dialog menu

private:
    std::vector <TRANSLINE_PRM*> m_prms_List;

public:
    TRANSLINE_IDENT( enum TRANSLINE_TYPE_ID aType );
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


    void ReadConfig( wxConfigBase* aConfig );
    void WriteConfig( wxConfigBase* aConfig );
};

#endif      //  TRANSLINE_IDENT_H

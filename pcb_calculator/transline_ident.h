/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011-2014 Jean-Pierre Charras
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file transline_ident.h
 */

#ifndef TRANSLINE_IDENT_H
#define TRANSLINE_IDENT_H

#include <vector>

#include "transline/transline.h"

// Forward declare the bitmaps enum from bitmaps/bitmap_list.h
enum class BITMAPS : unsigned int;

// An enum to handle muwave shapes:
enum TRANSLINE_TYPE_ID
{
    START_OF_LIST_TYPE = 0,
    DEFAULT_TYPE = START_OF_LIST_TYPE,
    MICROSTRIP_TYPE = DEFAULT_TYPE,
    C_MICROSTRIP_TYPE,
    STRIPLINE_TYPE,
    C_STRIPLINE_TYPE,
    CPW_TYPE,
    GROUNDED_CPW_TYPE,
    RECTWAVEGUIDE_TYPE,
    COAX_TYPE,
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


/**
 * A class to handle one parameter of transline.
 */
class TRANSLINE_PRM
{
public:
    /**
     * @param aKeywordCfg is the keyword used in config to identify the parameter
     *                    only ASCII7 keyword is valid.
     * @param aDlgLabel is a I18n string used to identify the parameter in dialog.
     *                  usually aDlgLabel is same as aKeywordCfg, but translatable.
     */
    TRANSLINE_PRM( PRM_TYPE aType, PRMS_ID aId, const char* aKeywordCfg = "", const wxString& aDlgLabel = wxEmptyString,
                   const wxString& aToolTip = wxEmptyString, double aValue = 0.0, bool aConvUnit = false,
                   int aDefaultUnit = 0 );

    double ToUserUnit();
    double FromUserUnit();

    PRM_TYPE    m_Type;            // Type of parameter: substr, physical, elect
    PRMS_ID     m_Id;              // Id of parameter ( link to transline functions )
    std::string m_KeyWord;         // keyword for this parameter in json config file in ASCII7 only
    wxString    m_DlgLabel;        // name for this parameter in dialog (usually translated
    wxString    m_ToolTip;         // Tool tip for this parameter in dialog
    double      m_Value;           // Value for this parameter in dialog
    double      m_DefaultValue;    // Default value for this parameter from CTOR build
    int         m_DefaultUnit;     // Default unit selection for this parameter
    double      m_NormalizedValue; // actual value for this parameter
    bool        m_ConvUnit;        // true if an unit selector must be used
    void*       m_ValueCtrl;       // The text ctrl containing the value in dialog
    void*       m_UnitCtrl;        // The UNIT_SELECTOR containing the unit in dialog
    int         m_UnitSelection;   // last selection for units
};


/**
 * A class to handle a list of parameters of a given transline.
 *
 * @note The first string of TRANSLINE_PRM (m_KeyWord) is a keyword in config file.
 *       It can contain only ASCII7 chars.  The second string of TRANSLINE_PRM is a
 *       string translated for dialog so mark it for translation.  Do not mark translatable
 *       m_DlgLabel that obviously cannot be translated, like "H" or "H_t".
 */
class TRANSLINE_IDENT
{
public:
    TRANSLINE_IDENT( enum TRANSLINE_TYPE_ID aType );
    ~TRANSLINE_IDENT();

    // Add a new param in list
    void AddPrm( TRANSLINE_PRM* aParam )
    {
        m_prms_List.push_back( aParam );
    }

    TRANSLINE_PRM* GetPrm( unsigned aIdx ) const
    {
        if( aIdx < m_prms_List.size() )
            return m_prms_List[aIdx];
        else
            return nullptr;
    }

    unsigned GetPrmsCount() const
    {
        return m_prms_List.size();
    }

    void ReadConfig();
    void WriteConfig();

public:
    enum TRANSLINE_TYPE_ID m_Type;              // The type of transline handled
    BITMAPS                m_BitmapName;        // The name of the bitmap to display in dialogs
    TRANSLINE*             m_TLine;             // The TRANSLINE itself
    wxArrayString          m_Messages;          // messages for results

    // true if selection of parameters must be enabled in dialog menu.
    bool m_HasPrmSelection;

private:
    std::vector<TRANSLINE_PRM*> m_prms_List;
};

#endif      //  TRANSLINE_IDENT_H

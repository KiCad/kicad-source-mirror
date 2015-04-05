/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 John Beard, john.j.beard@gmail.com
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef __DIALOG_CREATE_ARRAY__
#define __DIALOG_CREATE_ARRAY__

// Include the wxFormBuider header base:
#include <dialog_create_array_base.h>

#include <boost/bimap.hpp>

class CONFIG_SAVE_RESTORE_WINDOW
{
private:

    enum CONFIG_CTRL_TYPE_T
    {
        CFG_CTRL_TEXT,
        CFG_CTRL_CHECKBOX,
        CFG_CTRL_RADIOBOX,
        CFG_CTRL_CHOICE,
        CFG_CTRL_TAB
    };

    struct CONFIG_CTRL_T
    {
        wxControl* control;
        CONFIG_CTRL_TYPE_T type;
        void* dest;
    };

    std::vector<CONFIG_CTRL_T> ctrls;
    bool& valid;

protected:
    CONFIG_SAVE_RESTORE_WINDOW( bool& validFlag ) :
        valid( validFlag )
    {}

    void Add( wxRadioBox* ctrl, int& dest )
    {
        CONFIG_CTRL_T ctrlInfo = { ctrl, CFG_CTRL_RADIOBOX, (void*) &dest };

        ctrls.push_back( ctrlInfo );
    }

    void Add( wxCheckBox* ctrl, bool& dest )
    {
        CONFIG_CTRL_T ctrlInfo = { ctrl, CFG_CTRL_CHECKBOX, (void*) &dest };

        ctrls.push_back( ctrlInfo );
    }

    void Add( wxTextCtrl* ctrl, std::string& dest )
    {
        CONFIG_CTRL_T ctrlInfo = { ctrl, CFG_CTRL_TEXT, (void*) &dest };

        ctrls.push_back( ctrlInfo );
    }

    void Add( wxChoice* ctrl, int& dest )
    {
        CONFIG_CTRL_T ctrlInfo = { ctrl, CFG_CTRL_CHOICE, (void*) &dest };

        ctrls.push_back( ctrlInfo );
    }

    void Add( wxNotebook* ctrl, int& dest )
    {
        CONFIG_CTRL_T ctrlInfo = { ctrl, CFG_CTRL_TAB, (void*) &dest };

        ctrls.push_back( ctrlInfo );
    }

    void ReadConfigFromControls()
    {
        for( std::vector<CONFIG_CTRL_T>::const_iterator iter = ctrls.begin(), iend = ctrls.end();
             iter != iend; ++iter )
        {
            switch( iter->type )
            {
            case CFG_CTRL_CHECKBOX:
                *(bool*) iter->dest = static_cast<wxCheckBox*>( iter->control )->GetValue();
                break;

            case CFG_CTRL_TEXT:
                *(std::string*) iter->dest = static_cast<wxTextCtrl*>( iter->control )->GetValue();
                break;

            case CFG_CTRL_CHOICE:
                *(int*) iter->dest = static_cast<wxChoice*>( iter->control )->GetSelection();
                break;

            case CFG_CTRL_RADIOBOX:
                *(int*) iter->dest = static_cast<wxRadioBox*>( iter->control )->GetSelection();
                break;

            case CFG_CTRL_TAB:
                *(int*) iter->dest = static_cast<wxNotebook*>( iter->control )->GetSelection();
                break;

            default:
                wxASSERT_MSG( false, wxString(
                                "Unhandled control type for config store: " ) << iter->type );
            }
        }

        valid = true;
    }

    void RestoreConfigToControls()
    {
        if( !valid )
            return;

        for( std::vector<CONFIG_CTRL_T>::const_iterator iter = ctrls.begin(), iend = ctrls.end();
             iter != iend; ++iter )
        {
            switch( iter->type )
            {
            case CFG_CTRL_CHECKBOX:
                static_cast<wxCheckBox*>( iter->control )->SetValue( *(bool*) iter->dest );
                break;

            case CFG_CTRL_TEXT:
                static_cast<wxTextCtrl*>( iter->control )->SetValue( *(std::string*) iter->dest );
                break;

            case CFG_CTRL_CHOICE:
                static_cast<wxChoice*>( iter->control )->SetSelection( *(int*) iter->dest );
                break;

            case CFG_CTRL_RADIOBOX:
                static_cast<wxRadioBox*>( iter->control )->SetSelection( *(int*) iter->dest );
                break;

            case CFG_CTRL_TAB:
                static_cast<wxNotebook*>( iter->control )->SetSelection( *(int*) iter->dest );
                break;

            default:
                wxASSERT_MSG( false, wxString(
                                "Unhandled control type for config restore: " ) << iter->type );
            }
        }
    }
};

class DIALOG_CREATE_ARRAY : public DIALOG_CREATE_ARRAY_BASE,
    public CONFIG_SAVE_RESTORE_WINDOW
{
public:

    enum CREATE_ARRAY_EDIT_T
    {
        CREATE_ARRAY_ABORT,     ///< if not changed or error
        CREATE_ARRAY_OK,        ///< if successfully changed
    };

    enum ARRAY_TYPE_T
    {
        ARRAY_GRID,         ///< A grid (x*y) array
        ARRAY_CIRCULAR,     ///< A circular array
    };

    // NOTE: do not change order relative to charSetDescriptions
    enum ARRAY_NUMBERING_TYPE_T
    {
        NUMBERING_NUMERIC = 0,      ///< Arabic numerals: 0,1,2,3,4,5,6,7,8,9,10,11...
        NUMBERING_HEX,
        NUMBERING_ALPHA_NO_IOSQXZ,  /*!< Alphabet, excluding IOSQXZ
                                     *
                                     * Per ASME Y14.35M-1997 sec. 5.2 (previously MIL-STD-100 sec. 406.5)
                                     * as these can be confused with numerals and are often not used
                                     * for pin numbering on BGAs, etc
                                     */
        NUMBERING_ALPHA_FULL,       ///< Full 26-character alphabet
        NUMBERING_TYPE_Max          ///< Invalid maximum value, insert above here
    };

    /**
     * Persistent dialog options
     */
    struct ARRAY_OPTIONS
    {
        ARRAY_OPTIONS( ARRAY_TYPE_T aType ) :
            m_type( aType ),
            m_shouldRenumber( false )
        {}

        virtual ~ARRAY_OPTIONS() {};

        ARRAY_TYPE_T m_type;
        bool m_shouldRenumber;

        /*!
         * Function GetArrayPositions
         * Returns the set of points that represent the array
         * in order, if that is important
         *
         * TODO: Can/should this be done with some sort of iterator?
         */
        virtual void TransformItem( int n, BOARD_ITEM* item,
                const wxPoint& rotPoint ) const = 0;
        virtual int         GetArraySize() const = 0;
        virtual wxString GetItemNumber( int n ) const = 0;
        virtual wxString InterpolateNumberIntoString( int n, const wxString& pattern ) const;

        bool        ShouldRenumberItems() const
        {
            return m_shouldRenumber;
        }

protected:
        static std::string getCoordinateNumber( int n, ARRAY_NUMBERING_TYPE_T type );
    };

    struct ARRAY_GRID_OPTIONS : public ARRAY_OPTIONS
    {
        ARRAY_GRID_OPTIONS() :
            ARRAY_OPTIONS( ARRAY_GRID ),
            m_nx( 0 ), m_ny( 0 ),
            m_horizontalThenVertical( true ),
            m_reverseNumberingAlternate( false ),
            m_stagger( 0 ),
            m_stagger_rows( true ),
            m_2dArrayNumbering( false ),
            m_numberingOffsetX( 0 ),
            m_numberingOffsetY( 0 ),
            m_priAxisNumType( NUMBERING_NUMERIC ),
            m_secAxisNumType( NUMBERING_NUMERIC )
        {}

        long    m_nx, m_ny;
        bool    m_horizontalThenVertical, m_reverseNumberingAlternate;
        wxPoint m_delta;
        wxPoint m_offset;
        long    m_stagger;
        bool    m_stagger_rows;
        bool    m_2dArrayNumbering;
        int     m_numberingOffsetX, m_numberingOffsetY;
        ARRAY_NUMBERING_TYPE_T m_priAxisNumType, m_secAxisNumType;

        void        TransformItem( int n, BOARD_ITEM* item, const wxPoint& rotPoint ) const;    // override virtual
        int         GetArraySize() const;                                                       // override virtual
        wxString    GetItemNumber( int n ) const;                                               // override virtual

private:
        wxPoint getGridCoords( int n ) const;
    };

    struct ARRAY_CIRCULAR_OPTIONS : public ARRAY_OPTIONS
    {
        ARRAY_CIRCULAR_OPTIONS() :
            ARRAY_OPTIONS( ARRAY_CIRCULAR ),
            m_nPts( 0 ),
            m_angle( 0.0f ),
            m_rotateItems( false ),
            m_numberingType( NUMBERING_NUMERIC ),
            m_numberingOffset( 0 )
        {}

        long m_nPts;
        double m_angle;
        wxPoint m_centre;
        bool m_rotateItems;
        ARRAY_NUMBERING_TYPE_T m_numberingType;
        long m_numberingOffset;

        void        TransformItem( int n, BOARD_ITEM* item, const wxPoint& rotPoint ) const;    // override virtual
        int         GetArraySize() const;                                                       // override virtual
        wxString    GetItemNumber( int n ) const;                                               // override virtual
    };

    // Constructor and destructor
    DIALOG_CREATE_ARRAY( PCB_BASE_FRAME* aParent, wxPoint aOrigPos, ARRAY_OPTIONS** settings );
    virtual ~DIALOG_CREATE_ARRAY() {};

private:

    /**
     * The settings object returned to the caller.
     * We update the caller's object and never have ownership
     */
    ARRAY_OPTIONS** m_settings;

    /*
     * The position of the original item(s), used for finding radius, etc
     */
    const wxPoint m_originalItemPosition;

    // Event callbacks
    void    OnParameterChanged( wxCommandEvent& event );
    void    OnCancelClick( wxCommandEvent& event );
    void    OnOkClick( wxCommandEvent& event );

    // Internal callback handlers
    void setControlEnablement();
    void calculateCircularArrayProperties();

    struct CREATE_ARRAY_DIALOG_ENTRIES
    {
        CREATE_ARRAY_DIALOG_ENTRIES() :
            m_optionsSet( false ),
            m_gridStaggerType( 0 ),
            m_gridNumberingAxis( 0 ),
            m_gridNumberingReverseAlternate( false ),
            m_grid2dArrayNumbering( 0 ),
            m_gridPriAxisNumScheme( 0 ),
            m_gridSecAxisNumScheme( 0 ),
            m_circRotate( false ),
            m_arrayTypeTab( 0 )
        {}

        bool m_optionsSet;

        std::string m_gridNx, m_gridNy,
                    m_gridDx, m_gridDy,
                    m_gridOffsetX, m_gridOffsetY,
                    m_gridStagger;

        int m_gridStaggerType, m_gridNumberingAxis;
        bool    m_gridNumberingReverseAlternate;
        int     m_grid2dArrayNumbering;
        int     m_gridPriAxisNumScheme, m_gridSecAxisNumScheme;
        std::string m_gridPriNumberingOffset, m_gridSecNumberingOffset;

        std::string m_circCentreX, m_circCentreY,
                    m_circAngle, m_circCount, m_circNumberingOffset;
        bool m_circRotate;

        int m_arrayTypeTab;
    };

    static CREATE_ARRAY_DIALOG_ENTRIES m_options;

};

#endif      // __DIALOG_CREATE_ARRAY__

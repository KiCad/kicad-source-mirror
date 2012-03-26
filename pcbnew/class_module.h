/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file class_module.h
 * @brief Module description (excepted pads)
 */


#ifndef MODULE_H_
#define MODULE_H_


#include <dlist.h>
#include <layers_id_colors_and_visibility.h>       // ALL_LAYERS definition.
#include <class_board_item.h>

#include <class_text_mod.h>
#include "zones.h"

class LINE_READER;
class EDA_3D_CANVAS;
class S3D_MASTER;
class EDA_DRAW_PANEL;
class D_PAD;
class BOARD;


/**
 * Enum MODULE_ATTR_T
 * is the set of attributes allowed within a MODULE, using MODULE::SetAttributes()
 * and MODULE::GetAttributes().  These are to be ORed together when calling
 * MODULE::SetAttrbute()
 */
enum MODULE_ATTR_T
{
    MOD_DEFAULT = 0,    ///< default
    MOD_CMS = 1,        ///< Set for modules listed in the automatic insertion list
                        ///< (usually SMD footprints)
    MOD_VIRTUAL = 2     ///< Virtual component: when created by copper shapes on
                        ///<  board (Like edge card connectors, mounting hole...)
};


class MODULE : public BOARD_ITEM
{

public:
    double            m_Orient;        // orientation in 0.1 degrees
    wxPoint           m_Pos;           // Real coord on board
    DLIST<D_PAD>      m_Pads;          /* Pad list (linked list) */
    DLIST<BOARD_ITEM> m_Drawings;      /* Graphic items list (linked list) */
    DLIST<S3D_MASTER> m_3D_Drawings;   /* First item of the 3D shapes (linked list)*/
    TEXTE_MODULE*     m_Reference;     // Component reference (U34, R18..)
    TEXTE_MODULE*     m_Value;         // Component value (74LS00, 22K..)
    wxString          m_LibRef;        /* Name of the module in library (and
                                        * the default value when loading a
                                        * module from the library) */
    int           m_Attributs;          ///< Flag bits ( see Mod_Attribut )
    int           flag;                 /* Use to trace ratsnest and auto routing. */

    int           m_ModuleStatus;       ///< For autoplace: flags (LOCKED, AUTOPLACED)

// m_ModuleStatus bits:
#define MODULE_is_LOCKED    0x01        ///< module LOCKED: no autoplace allowed
#define MODULE_is_PLACED    0x02        ///< In autoplace: module automatically placed
#define MODULE_to_PLACE     0x04        ///< In autoplace: module waiting for autoplace


    EDA_RECT      m_BoundaryBox;        // Bounding box : coordinates on board, real orientation.
    int           m_PadNum;             // Pad count
    int           m_AltPadNum;          /* Pad with netcode > 0 (active pads) count */

    int           m_CntRot90;           ///< Automatic placement : cost ( 0..10 )
                                        ///< for 90 degrees rotation (Horiz<->Vertical)

    int           m_CntRot180;          ///< Automatic placement : cost ( 0..10 )
                                        ///< for 180 degrees rotation (UP <->Down)

    wxSize        m_Ext;                /* Automatic placement margin around the module */
    double        m_Surface;            // Bounding box area

    unsigned long m_Link;               /* Temporary variable ( used in editions, ...) */
    long          m_LastEdit_Time;
    wxString      m_Path;

    wxString      m_Doc;                // Module Description (info for users)
    wxString      m_KeyWord;            // Keywords to select the module in lib

    // Local tolerances. When zero, this means the corresponding netclass value
    // is used. Usually theses local tolerances zero, in deference to the
    // corresponding netclass values.
    int           m_LocalClearance;
    int           m_LocalSolderMaskMargin;         ///< Solder mask margin
    int           m_LocalSolderPasteMargin;        ///< Solder paste margin
                                                   ///< absolute value

    double        m_LocalSolderPasteMarginRatio;   ///< Solder mask margin ratio
                                                   ///< value of pad size
    // The final margin is the sum of these 2 values

    ZoneConnection m_ZoneConnection;
    int m_ThermalWidth, m_ThermalGap;

public:
    MODULE( BOARD* parent );

    MODULE( const MODULE& aModule );

    ~MODULE();

    MODULE* Next() const { return (MODULE*) Pnext; }
    MODULE* Back() const { return (MODULE*) Pback; }

    void Copy( MODULE* Module );        // Copy structure

    /*
     * Function Add
     * adds the given item to this MODULE and takes ownership of its memory.
     * @param aBoardItem The item to add to this board.
     * @param doInsert If true, then insert, else append
     *  void    Add( BOARD_ITEM* aBoardItem, bool doInsert = true );
     */

    /**
     * Function CalculateBoundingBox
     * calculates the bounding box in board coordinates.
     */
    void CalculateBoundingBox();

    /**
     * Function GetFootPrintRect()
     * Returns the area of the module footprint excluding any text.
     * @return EDA_RECT - The rectangle containing the footprint.
     */
    EDA_RECT GetFootPrintRect() const;

    EDA_RECT GetBoundingBox() const;

    void SetPosition( const wxPoint& aPos );                        // was overload
    const wxPoint& GetPosition() const          { return m_Pos; }   // was overload

    void SetOrientation( double newangle );
    double GetOrientation() const { return m_Orient; }

    const wxString& GetLibRef() const { return m_LibRef; }
    void SetLibRef( const wxString& aLibRef ) { m_LibRef = aLibRef; }

    const wxString& GetDescription() const { return m_Doc; }
    void SetDescription( const wxString& aDoc ) { m_Doc = aDoc; }

    const wxString& GetKeywords() const { return m_KeyWord; }
    void SetKeywords( const wxString& aKeywords ) { m_KeyWord = aKeywords; }

    const wxString& GetPath() const { return m_Path; }
    void SetPath( const wxString& aPath ) { m_Path = aPath; }

    int GetLocalSolderMaskMargin() const { return m_LocalSolderMaskMargin; }
    void SetLocalSolderMaskMargin( int aMargin ) { m_LocalSolderMaskMargin = aMargin; }

    int GetLocalClearance() const { return m_LocalClearance; }
    void SetLocalClearance( int aClearance ) { m_LocalClearance = aClearance; }

    int GetLocalSolderPasteMargin() const { return m_LocalSolderPasteMargin; }
    void SetLocalSolderPasteMargin( int aMargin ) { m_LocalSolderPasteMargin = aMargin; }

    double GetLocalSolderPasteMarginRatio() const { return m_LocalSolderPasteMarginRatio; }
    void SetLocalSolderPasteMarginRatio( double aRatio ) { m_LocalSolderPasteMarginRatio = aRatio; }

    void SetZoneConnection( ZoneConnection aType ) { m_ZoneConnection = aType; }
    ZoneConnection GetZoneConnection() const { return m_ZoneConnection; }

    void SetThermalWidth( int aWidth ) { m_ThermalWidth = aWidth; }
    int GetThermalWidth() const { return m_ThermalWidth; }

    void SetThermalGap( int aGap ) { m_ThermalGap = aGap; }
    int GetThermalGap() const { return m_ThermalGap; }

    int GetAttributes() const { return m_Attributs; }
    void SetAttributes( int aAttributes ) { m_Attributs = aAttributes; }

    void Move( const wxPoint& aMoveVector );

    void Rotate( const wxPoint& aRotCentre, double aAngle );

    void Flip( const wxPoint& aCentre );

    bool IsLocked() const
    {
        return (m_ModuleStatus & MODULE_is_LOCKED) != 0;
    }

    /**
     * Function SetLocked
     * sets the MODULE_is_LOCKED bit in the m_ModuleStatus
     * @param isLocked When true means turn on locked status, else unlock
     */
    void SetLocked( bool isLocked )
    {
        if( isLocked )
            m_ModuleStatus |= MODULE_is_LOCKED;
        else
            m_ModuleStatus &= ~MODULE_is_LOCKED;
    }

    bool IsPlaced() const   { return (m_ModuleStatus & MODULE_is_PLACED); }
    void SetIsPlaced( bool isPlaced )
    {
        if( isPlaced )
            m_ModuleStatus |= MODULE_is_PLACED;
        else
            m_ModuleStatus &= ~MODULE_is_PLACED;
    }

    void SetLastEditTime( long aTime ) { m_LastEdit_Time = aTime; }
    long GetLastEditTime() const { return m_LastEdit_Time; }

    /* Reading and writing data on files */

    bool Save( FILE* aFile ) const;

    int Write_3D_Descr( FILE* File ) const;

    int ReadDescr( LINE_READER* aReader );

    /**
     * Function Read_GPCB_Descr
     * reads a footprint description in GPCB format (newlib version)
     * @param CmpFullFileName = Full file name (there is one footprint per file.
     * this is also the footprint name
     * @return bool - true if success reading else false.
     */
    bool Read_GPCB_Descr( const wxString& CmpFullFileName );

    int Read_3D_Descr( LINE_READER* aReader );

    /* drawing functions */

    void Draw( EDA_DRAW_PANEL* aPanel,
               wxDC*           aDC,
               int             aDrawMode,
               const wxPoint&  aOffset = ZeroOffset );

    void Draw3D( EDA_3D_CANVAS* glcanvas );

    void DrawEdgesOnly( EDA_DRAW_PANEL* panel, wxDC* DC, const wxPoint& offset, int draw_mode );

    void DrawAncre( EDA_DRAW_PANEL* panel, wxDC* DC,
                    const wxPoint& offset, int dim_ancre, int draw_mode );

    void DisplayInfo( EDA_DRAW_FRAME* frame );

    bool HitTest( const wxPoint& aPosition );

    bool HitTest( const EDA_RECT& aRect ) const;

    /**
     * Function GetReference
     * @return const wxString& - the reference designator text.
     */
    const wxString& GetReference() const
    {
        return m_Reference->m_Text;
    }

    /**
     * Function SetReference
     * @param aReference A reference to a wxString object containing the reference designator
     *                   text.
     */
    void SetReference( const wxString& aReference )
    {
        m_Reference->m_Text = aReference;
    }

    /**
     * Function GetValue
     * @return const wxString& - the value text.
     */
    const wxString& GetValue()
    {
        return m_Value->m_Text;
    }

    /**
     * Function SetValue
     * @param aValue A reference to a wxString object containing the value text.
     */
    void SetValue( const wxString& aValue )
    {
        m_Value->m_Text = aValue;
    }

    /**
     * Function FindPadByName
     * returns a D_PAD* with a matching name.  Note that names may not be
     * unique, depending on how the foot print was created.
     * @param aPadName the pad name to find
     * @return D_PAD* - The first matching name is returned, or NULL if not
     *                  found.
     */
    D_PAD* FindPadByName( const wxString& aPadName ) const;

    /**
     * Function GetPad
     * get a pad at \a aPosition on \a aLayer in the footprint.
     *
     * @param aPosition A wxPoint object containing the position to hit test.
     * @param aLayerMask A layer or layers to mask the hit test.
     * @return A pointer to a D_PAD object if found otherwise NULL.
     */
    D_PAD* GetPad( const wxPoint& aPosition, int aLayerMask = ALL_LAYERS );

    SEARCH_RESULT Visit( INSPECTOR* inspector, const void* testData,
                         const KICAD_T scanTypes[] );

    wxString GetClass() const
    {
        return wxT( "MODULE" );
    }

    wxString GetSelectMenuText() const;

    BITMAP_DEF GetMenuImage() const { return  module_xpm; }

    EDA_ITEM* Clone() const;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const;     // overload
#endif
};


#endif     // MODULE_H_

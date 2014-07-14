/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2013 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <fpid.h>

#include <class_text_mod.h>
#include <PolyLine.h>
#include "zones.h"

#include <boost/function.hpp>

class LINE_READER;
class EDA_3D_CANVAS;
class S3D_MASTER;
class EDA_DRAW_PANEL;
class D_PAD;
class BOARD;
class MSG_PANEL_ITEM;


/**
 * Enum MODULE_ATTR_T
 * is the set of attributes allowed within a MODULE, using MODULE::SetAttributes()
 * and MODULE::GetAttributes().  These are to be ORed together when calling
 * MODULE::SetAttributes()
 */
enum MODULE_ATTR_T
{
    MOD_DEFAULT = 0,    ///< default
    MOD_CMS     = 1,    ///< Set for modules listed in the automatic insertion list
                        ///< (usually SMD footprints)
    MOD_VIRTUAL = 2     ///< Virtual component: when created by copper shapes on
                        ///<  board (Like edge card connectors, mounting hole...)
};


class MODULE : public BOARD_ITEM
{
public:
    MODULE( BOARD* parent );

    MODULE( const MODULE& aModule );

    ~MODULE();

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return PCB_MODULE_T == aItem->Type();
    }

    MODULE* Next() const { return static_cast<MODULE*>( Pnext ); }
    MODULE* Back() const { return static_cast<MODULE*>( Pback ); }

    void Copy( MODULE* Module );        // Copy structure

    /*
     * Function Add
     * adds the given item to this MODULE and takes ownership of its memory.
     * @param aBoardItem The item to add to this board.
     * @param doAppend If true, then append, else insert.
     */
    void Add( BOARD_ITEM* aBoardItem, bool doAppend = true );

    /**
     * Function Delete
     * removes the given single item from this MODULE and deletes its memory.
     * @param aBoardItem The item to remove from this module and delete
     */
    void Delete( BOARD_ITEM* aBoardItem )
    {
        // developers should run DEBUG versions and fix such calls with NULL
        wxASSERT( aBoardItem );

        if( aBoardItem )
            delete Remove( aBoardItem );
    }

    /**
     * Function Remove
     * removes \a aBoardItem from this MODULE and returns it to caller without deleting it.
     * @param aBoardItem The item to remove from this module.
     * @return BOARD_ITEM* \a aBoardItem which was passed in.
     */
    BOARD_ITEM* Remove( BOARD_ITEM* aBoardItem );

    /**
     * Function CalculateBoundingBox
     * calculates the bounding box in board coordinates.
     */
    void CalculateBoundingBox();

    /**
     * Function GetFootprintRect()
     * Returns the area of the module footprint excluding any text.
     * @return EDA_RECT - The rectangle containing the footprint.
     */
    EDA_RECT GetFootprintRect() const;

    // Virtual function
    const EDA_RECT GetBoundingBox() const;

    DLIST<D_PAD>& Pads()                        { return m_Pads; }
    const DLIST<D_PAD>& Pads() const            { return m_Pads; }

    DLIST<BOARD_ITEM>& GraphicalItems()         { return m_Drawings; }
    const DLIST<BOARD_ITEM>& GraphicalItems() const { return m_Drawings; }

    DLIST<S3D_MASTER>& Models()                 { return m_3D_Drawings; }
    const DLIST<S3D_MASTER>& Models() const     { return m_3D_Drawings; }

    void SetPosition( const wxPoint& aPos );                        // was overload
    const wxPoint& GetPosition() const          { return m_Pos; }   // was overload

    void SetOrientation( double newangle );
    double GetOrientation() const { return m_Orient; }

    const FPID& GetFPID() const { return m_fpid; }
    void SetFPID( const FPID& aFPID ) { m_fpid = aFPID; }

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

    void SetFlag( int aFlag ) { flag = aFlag; }
    void IncrementFlag() { flag += 1; }
    int GetFlag() const { return flag; }

    void Move( const wxPoint& aMoveVector );

    void Rotate( const wxPoint& aRotCentre, double aAngle );

    void Flip( const wxPoint& aCentre );

    /**
     * Function MoveAnchorPosition
     * Move the reference point of the footprint
     * It looks like a move footprint:
     * the footprints elements (pads, outlines, edges .. ) are moved
     * However:
     * - the footprint position is not modified.
     * - the relative (local) coordinates of these items are modified
     * (a move footprint does not change these local coordinates,
     * but changes the footprint position)
     */
    void MoveAnchorPosition( const wxPoint& aMoveVector );

    /**
     * function IsFlipped
     * @return true if the module is flipped, i.e. on the back side of the board
     */
    bool IsFlipped() const {return GetLayer() == B_Cu; }

// m_ModuleStatus bits:
#define MODULE_is_LOCKED    0x01        ///< module LOCKED: no autoplace allowed
#define MODULE_is_PLACED    0x02        ///< In autoplace: module automatically placed
#define MODULE_to_PLACE     0x04        ///< In autoplace: module waiting for autoplace


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

    bool NeedsPlaced() const  { return (m_ModuleStatus & MODULE_to_PLACE); }
    void SetNeedsPlaced( bool needsPlaced )
    {
        if( needsPlaced )
            m_ModuleStatus |= MODULE_to_PLACE;
        else
            m_ModuleStatus &= ~MODULE_to_PLACE;
    }

    void SetLastEditTime( time_t aTime ) { m_LastEditTime = aTime; }
    void SetLastEditTime( ) { m_LastEditTime = time( NULL ); }
    time_t GetLastEditTime() const { return m_LastEditTime; }

    /* drawing functions */

    /**
     * Function Draw
     * draws the footprint to the \a aDC.
     * @param aPanel = draw panel, Used to know the clip box
     * @param aDC = Current Device Context
     * @param aDrawMode = GR_OR, GR_XOR..
     * @param aOffset = draw offset (usually wxPoint(0,0)
     */
    void Draw( EDA_DRAW_PANEL* aPanel,
               wxDC*           aDC,
               GR_DRAWMODE     aDrawMode,
               const wxPoint&  aOffset = ZeroOffset );

    /**
     * function ReadandInsert3DComponentShape
     * read the 3D component shape(s) of the footprint (physical shape)
     * and insert mesh in gl list
     * @param glcanvas = the openGL canvas
     * @param  aAllowNonTransparentObjects = true to load non transparent objects
     * @param  aAllowTransparentObjects = true to load non transparent objects
     * in openGL, transparent objects should be drawn *after* non transparent objects
     */
    void ReadAndInsert3DComponentShape( EDA_3D_CANVAS* glcanvas,
                                        bool aAllowNonTransparentObjects,
                                        bool aAllowTransparentObjects );

    /**
     * function TransformPadsShapesWithClearanceToPolygon
     * generate pads shapes on layer aLayer as polygons,
     * and adds these polygons to aCornerBuffer
     * Useful to generate a polygonal representation of a footprint
     * in 3D view and plot functions, when a full polygonal approach is needed
     * @param aLayer = the current layer: pads on this layer are considered
     * @param aCornerBuffer = the buffer to store polygons
     * @param aInflateValue = an additionnal size to add to pad shapes
     *          aInflateValue = 0 to have the exact pad size
     * @param aCircleToSegmentsCount = number of segments to generate a circle
     * @param aCorrectionFactor = the correction to apply to a circle radius
     *  to approximate a circle by the polygon.
     *  if aCorrectionFactor = 1.0, the polygon is inside the circle
     *  the radius of circle approximated by segments is
     *  initial radius * aCorrectionFactor
     */
    void TransformPadsShapesWithClearanceToPolygon( LAYER_ID aLayer,
                            CPOLYGONS_LIST& aCornerBuffer,
                            int             aInflateValue,
                            int             aCircleToSegmentsCount,
                            double          aCorrectionFactor );

    /**
     * function TransformGraphicShapesWithClearanceToPolygonSet
     * generate shapes of graphic items (outlines) on layer aLayer as polygons,
     * and adds these polygons to aCornerBuffer
     * Useful to generate a polygonal representation of a footprint
     * in 3D view and plot functions, when a full polygonal approach is needed
     * @param aLayer = the current layer: items on this layer are considered
     * @param aCornerBuffer = the buffer to store polygons
     * @param aInflateValue = a value to inflate shapes
     *          aInflateValue = 0 to have the exact shape size
     * @param aCircleToSegmentsCount = number of segments to generate a circle
     * @param aCorrectionFactor = the correction to apply to a circle radius
     *  to approximate a circle by the polygon.
     *  if aCorrectionFactor = 1.0, the polygon is inside the circle
     *  the radius of circle approximated by segments is
     *  initial radius * aCorrectionFactor
     */
    void TransformGraphicShapesWithClearanceToPolygonSet(
                            LAYER_ID aLayer,
                            CPOLYGONS_LIST& aCornerBuffer,
                            int             aInflateValue,
                            int             aCircleToSegmentsCount,
                            double          aCorrectionFactor );

    /**
     * Function DrawEdgesOnly
     *  Draws the footprint edges only to the current Device Context
     *  @param panel = The active Draw Panel (used to know the clip box)
     *  @param DC = current Device Context
     *  @param offset = draw offset (usually wxPoint(0,0)
     *  @param draw_mode =  GR_OR, GR_XOR, GR_AND
     */
    void DrawEdgesOnly( EDA_DRAW_PANEL* panel, wxDC* DC, const wxPoint& offset,
                        GR_DRAWMODE draw_mode );

    void DrawAncre( EDA_DRAW_PANEL* panel, wxDC* DC,
                    const wxPoint& offset, int dim_ancre, GR_DRAWMODE draw_mode );

    void GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList );

    bool HitTest( const wxPoint& aPosition ) const;

    /** @copydoc BOARD_ITEM::HitTest(const EDA_RECT& aRect,
     *                               bool aContained = true, int aAccuracy ) const
     */
    bool HitTest( const EDA_RECT& aRect, bool aContained = true, int aAccuracy = 0 ) const;

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

    /// read/write accessors:
    TEXTE_MODULE& Value()       { return *m_Value; }
    TEXTE_MODULE& Reference()   { return *m_Reference; }

    /// The const versions to keep the compiler happy.
    TEXTE_MODULE& Value() const       { return *m_Value; }
    TEXTE_MODULE& Reference() const   { return *m_Reference; }


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
     * get a pad at \a aPosition on \a aLayerMask in the footprint.
     *
     * @param aPosition A wxPoint object containing the position to hit test.
     * @param aLayerMask A layer or layers to mask the hit test.
     * @return A pointer to a D_PAD object if found otherwise NULL.
     */
    D_PAD* GetPad( const wxPoint& aPosition, LSET aLayerMask = LSET::AllLayersMask() );

    enum INCLUDE_NPTH_T
    {
        DO_NOT_INCLUDE_NPTH = false,
        INCLUDE_NPTH = true
    };

    /**
     * GetPadCount
     * returns the number of pads.
     *
     * @param aIncludeNPTH includes non-plated through holes when true.  Does not include
     *                     non-plated through holes when false.
     * @return the number of pads according to \a aIncludeNPTH.
     */
    unsigned GetPadCount( INCLUDE_NPTH_T aIncludeNPTH = INCLUDE_NPTH ) const;

    double GetArea() const                  { return m_Surface; }

    time_t GetLink() const                  { return m_Link; }
    void SetLink( time_t aLink )            { m_Link = aLink; }

    int GetPlacementCost180() const         { return m_CntRot180; }
    void SetPlacementCost180( int aCost )   { m_CntRot180 = aCost; }

    int GetPlacementCost90() const          { return m_CntRot90; }
    void SetPlacementCost90( int aCost )    { m_CntRot90 = aCost; }

    /**
     * Function Add3DModel
     * adds \a a3DModel definition to the end of the 3D model list.
     *
     * @param a3DModel A pointer to a #S3D_MASTER to add to the list.
     */
    void Add3DModel( S3D_MASTER* a3DModel );

    SEARCH_RESULT Visit( INSPECTOR* inspector, const void* testData,
                         const KICAD_T scanTypes[] );

    wxString GetClass() const
    {
        return wxT( "MODULE" );
    }

    wxString GetSelectMenuText() const;

    BITMAP_DEF GetMenuImage() const { return  module_xpm; }

    EDA_ITEM* Clone() const;

    /**
     * Function RunOnChildren
     *
     * Invokes a function on all BOARD_ITEMs that belong to the module (pads, drawings, texts).
     * @param aFunction is the function to be invoked.
     */
    void RunOnChildren( boost::function<void (BOARD_ITEM*)> aFunction );

    /// @copydoc VIEW_ITEM::ViewUpdate()
    void ViewUpdate( int aUpdateFlags = KIGFX::VIEW_ITEM::ALL );

    /// @copydoc VIEW_ITEM::ViewGetLayers()
    virtual void ViewGetLayers( int aLayers[], int& aCount ) const;

    /// @copydoc VIEW_ITEM::ViewGetLOD()
    virtual unsigned int ViewGetLOD( int aLayer ) const;

    /**
     * Function CopyNetlistSettings
     * copies the netlist settings to \a aModule.
     *
     * The netlist settings are all of the #MODULE settings not define by a #MODULE in
     * a netlist.  These setting include position, orientation, local clearances, ets.
     * The reference designator, value, path, and physical geometry settings are not
     * copied.
     *
     * @param aModule is the #MODULE to copy the settings to.
     */
    void CopyNetlistSettings( MODULE* aModule );

    /**
     * static function IsLibNameValid
     * Test for validity of a name of a footprint to be used in a footprint library
     * ( no spaces, dir separators ... )
     * @param aName = the name in library to validate
     * @return true if the given name is valid
     */
    static bool IsLibNameValid( const wxString & aName );

    /**
     * static function StringLibNameInvalidChars
     * Test for validity of the name in a library of the footprint
     * ( no spaces, dir separators ... )
     * @param aUserReadable = false to get the list of invalid chars
     *        true to get a readable form (i.e ' ' = 'space' '\\t'= 'tab')
     * @return a constant std::string giving the list of invalid chars in lib name
     */
    static const wxChar* StringLibNameInvalidChars( bool aUserReadable );

    /**
     * Function SetInitialComments
     * takes ownership of caller's heap allocated aInitialComments block.  The comments
     * are single line strings already containing the s-expression comments with
     * optional leading whitespace and then a '#' character followed by optional
     * single line text (text with no line endings, not even one).
     * This block of single line comments will be output upfront of any generated
     * s-expression text in the PCBIO::Format() function.
     * <p>
     * Note that a block of single line comments constitutes a multiline block of
     * single line comments.  That is, the block is made of consecutive single line
     * comments.
     * @param aInitialComments is a heap allocated wxArrayString or NULL, which the caller
     *  gives up ownership of over to this MODULE.
     */
    void SetInitialComments( wxArrayString* aInitialComments )
    {
        delete m_initial_comments;
        m_initial_comments = aInitialComments;
    }

    /// Return the initial comments block or NULL if none, without transfer of ownership.
    const wxArrayString* GetInitialComments() const { return m_initial_comments; }

#if defined(DEBUG)
    virtual void Show( int nestLevel, std::ostream& os ) const { ShowDummy( os ); }    // override
#endif

private:
    DLIST<D_PAD>      m_Pads;           ///< Linked list of pads.
    DLIST<BOARD_ITEM> m_Drawings;       ///< Linked list of graphical items.
    DLIST<S3D_MASTER> m_3D_Drawings;    ///< Linked list of 3D models.
    double            m_Orient;         ///< Orientation in tenths of a degree, 900=90.0 degrees.
    wxPoint           m_Pos;            ///< Position of module on the board in internal units.
    TEXTE_MODULE*     m_Reference;      ///< Component reference designator value (U34, R18..)
    TEXTE_MODULE*     m_Value;          ///< Component value (74LS00, 22K..)
    FPID              m_fpid;           ///< The #FPID of the MODULE.
    int               m_Attributs;      ///< Flag bits ( see Mod_Attribut )
    int               m_ModuleStatus;   ///< For autoplace: flags (LOCKED, AUTOPLACED)
    EDA_RECT          m_BoundaryBox;    ///< Bounding box : coordinates on board, real orientation.

    // The final margin is the sum of these 2 values
    int               m_ThermalWidth;
    int               m_ThermalGap;
    wxString          m_Doc;            ///< File name and path for documentation file.
    wxString          m_KeyWord;        ///< Search keywords to find module in library.
    wxString          m_Path;
    ZoneConnection    m_ZoneConnection;
    time_t            m_LastEditTime;
    int               flag;             ///< Use to trace ratsnest and auto routing.
    double            m_Surface;        ///< Bounding box area
    time_t            m_Link;           ///< Temporary logical link used in edition
    int               m_CntRot90;       ///< Horizontal automatic placement cost ( 0..10 ).
    int               m_CntRot180;      ///< Vertical automatic placement cost ( 0..10 ).

    // Local tolerances. When zero, this means the corresponding netclass value
    // is used. Usually theses local tolerances zero, in deference to the
    // corresponding netclass values.
    int               m_LocalClearance;
    int               m_LocalSolderMaskMargin;    ///< Solder mask margin
    int               m_LocalSolderPasteMargin;   ///< Solder paste margin absolute value
    double            m_LocalSolderPasteMarginRatio;   ///< Solder mask margin ratio
                                                       ///< value of pad size

    wxArrayString*    m_initial_comments;   ///< leading s-expression comments in the module,
                                            ///< lazily allocated only if needed for speed
};

#endif     // MODULE_H_

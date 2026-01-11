/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Seth Hillbrand <seth@kipro-pcb.com>
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


#ifndef PCBNEW_IMPORTERS_IMPORT_FABMASTER_H_
#define PCBNEW_IMPORTERS_IMPORT_FABMASTER_H_

#include <eda_text.h>
#include <geometry/shape_arc.h>
#include <padstack.h>

#include <deque>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <string>
#include <vector>

#include <wx/filename.h>

enum PCB_LAYER_ID : int;
class BOARD;
class BOARD_ITEM;
class PCB_TEXT;
class PROGRESS_REPORTER;

class FABMASTER
{
public:

    using single_row = std::vector<std::string>;
    FABMASTER() :
        has_pads( false ),
        has_comps( false ),
        has_graphic( false ),
        has_nets( false ),
        has_pins( false ),
        m_progressReporter( nullptr ),
        m_doneCount( 0 ),
        m_lastProgressCount( 0 ),
        m_totalCount( 0 )

    {}

    bool Read( const std::string& aFile );

    bool Process();

    bool LoadBoard( BOARD* aBoard, PROGRESS_REPORTER* aProgressReporter );

private:

    wxFileName m_filename;

    enum section_type : int
    {
        UNKNOWN_EXTRACT,
        EXTRACT_PADSTACKS,
        EXTRACT_PAD_SHAPES,
        EXTRACT_FULL_LAYERS,
        EXTRACT_VIAS,
        FABMASTER_EXTRACT_PINS,
        EXTRACT_PINS,
        EXTRACT_TRACES,
        EXTRACT_GRAPHICS,
        EXTRACT_BASIC_LAYERS,
        EXTRACT_NETS,
        EXTRACT_REFDES
    };

    std::deque<single_row> rows;

    bool has_pads;
    bool has_comps;
    bool has_graphic;
    bool has_nets;
    bool has_pins;


    struct FM_PAD
    {
        std::string name;
        bool fixed;
        bool via;
        PAD_SHAPE shape;
        std::string custom_name;
        bool top;
        bool bottom;
        bool paste;
        bool mask;
        bool drill;
        bool plated;
        bool is_octogon;
        int drill_size_x;
        int drill_size_y;
        int width;
        int height;
        int mask_width;
        int mask_height;
        int paste_width;
        int paste_height;
        int x_offset;
        int y_offset;
        int antipad_size;

        struct HASH
        {
            std::size_t operator()(  const FM_PAD& aPad ) const
            {
                return std::hash<std::string>{}( aPad.name );
            }
        };
    };

    std::unordered_map<std::string, FM_PAD> pads;

    enum COMPCLASS
    {
        COMPCLASS_NONE,
        COMPCLASS_IO,
        COMPCLASS_IC,
        COMPCLASS_DISCRETE
    };

    enum SYMTYPE
    {
        SYMTYPE_NONE,
        SYMTYPE_PACKAGE,
        SYMTYPE_MECH,
        SYMTYPE_FORMAT,
        SYMTYPE_DRAFTING
    };

    // A!NET_NAME!REFDES!PIN_NUMBER!PIN_NAME!PIN_GROUND!PIN_POWER!
    struct NETNAME
    {
        std::string name;           ///!< NET_NAME
        std::string refdes;         ///!< REFDES
        std::string pin_num;        ///!< PIN_NUMBER
        std::string pin_name;       ///!< PIN_NAME
        bool pin_gnd;               ///!< PIN_GND
        bool pin_pwr;               ///!< PIN_PWR

        struct LESS
        {
            bool operator()(const NETNAME& lhs, const NETNAME& rhs) const
            {
                if( lhs.refdes == rhs.refdes )
                    return lhs.pin_num < rhs.pin_num;

                return lhs.refdes < rhs.refdes;
            }
        };
    };

    std::map<std::pair<std::string, std::string>, NETNAME> pin_nets;
    std::set<std::string> netnames;

    struct CLASS
    {
        std::string name;           ///!< CLASS
        std::string subclass;       ///!< SUBCLASS
    };


    enum GRAPHIC_SHAPE
    {
        GR_SHAPE_LINE,
        GR_SHAPE_TEXT,
        GR_SHAPE_RECTANGLE,
        GR_SHAPE_ARC,
        GR_SHAPE_CIRCLE, ///!< Actually 360° arcs (for both arcs where start==end and real circles)
        GR_SHAPE_OBLONG, ///!< X/Y oblongs
        GR_SHAPE_CROSS,
        GR_SHAPE_POLYGON, ///!< Hexagon, triangle
    };

    enum GRAPHIC_TYPE
    {
        GR_TYPE_NONE,
        GR_TYPE_CONNECT,
        GR_TYPE_NOTCONNECT,
        GR_TYPE_SHAPE,
        GR_TYPE_POLYGON,
        GR_TYPE_VOID
    };

    struct GRAPHIC_ITEM
    {
        virtual ~GRAPHIC_ITEM() = default;

        int           start_x;  ///<! GRAPHIC_DATA_1
        int           start_y;  ///<! GRAPHIC_DATA_2
        int           width;    ///<! Various sections depending on type
        std::string   layer;    ///<! SUBCLASS
        std::string   symbol;   ///<! SYMBOL
        std::string   refdes;   ///<! REFDES
        int           seq;      ///<! RECORD_TAG[0]
        int           subseq;   ///<! RECORD_TAG[1]
        GRAPHIC_TYPE  type;     ///<! Type of graphic item
        GRAPHIC_SHAPE shape;    ///<! Shape of the graphic_item


        struct SEQ_CMP
        {
            bool operator()(const std::unique_ptr<GRAPHIC_ITEM>& lhs,
                    const std::unique_ptr<GRAPHIC_ITEM>& rhs) const
            {
                if( lhs->refdes != rhs->refdes )
                    return lhs->refdes < rhs->refdes;

                if( lhs->layer != rhs->layer )
                    return lhs->layer < rhs->layer;

                return lhs->seq < rhs->seq;
            }
        };
    };

    struct GRAPHIC_LINE : public GRAPHIC_ITEM
    {
        int end_x;           ///<! GRAPHIC_DATA_3
        int end_y;           ///<! GRAPHIC_DATA_4
                             ///<! width is GRAPHIC_DATA_5
    };

    struct GRAPHIC_ARC : public GRAPHIC_ITEM
    {
        int end_x;           ///<! GRAPHIC_DATA_3
        int end_y;           ///<! GRAPHIC_DATA_4
        int center_x;        ///<! GRAPHIC_DATA_5
        int center_y;        ///<! GRAPHIC_DATA_6
        int radius;          ///<! GRAPHIC_DATA_7
                             ///<! width is GRAPHIC_DATA_8
        bool clockwise;      ///<! GRAPHIC_DATA_9

        SHAPE_ARC result;    ///<! KiCad-style arc representation
    };

    struct GRAPHIC_RECTANGLE : public GRAPHIC_ITEM
    {
        int end_x;           ///<! GRAPHIC_DATA_3
        int end_y;           ///<! GRAPHIC_DATA_4
        bool fill;           ///<! GRAPHIC_DATA_5
    };

    struct GRAPHIC_OBLONG : public GRAPHIC_ITEM
    {
        bool oblong_x; ///<! OBLONG_X (as opposed to OBLONG_Y)
        int  size_x;   ///<! GRAPHIC_DATA_3
        int  size_y;   ///<! GRAPHIC_DATA_4
        // width is GRAPHIC_DATA5 (or is it fill?)
    };

    struct GRAPHIC_CROSS : public GRAPHIC_ITEM
    {
        bool oblong_x; ///<! OBLONG_X (as opposed to OBLONG_Y)
        int  size_x;   ///<! GRAPHIC_DATA_3
        int  size_y;   ///<! GRAPHIC_DATA_4
        // width is GRAPHIC_DATA5
    };

    // Handles hexagons, triangles, octagons
    struct GRAPHIC_POLYGON : public GRAPHIC_ITEM
    {
        std::vector<VECTOR2I> m_pts;
    };

    struct GRAPHIC_TEXT : public GRAPHIC_ITEM
    {
        double rotation;    ///<! GRAPHIC_DATA_3
        bool mirror;        ///<! GRAPHIC_DATA_4
        GR_TEXT_H_ALIGN_T orient; ///<! GRAPHIC_DATA_5
        // GRAPHIC_DATA_6 is
        // SIZE FONT HEIGHT WIDTH ITAL LINESPACE THICKNESS
        int height;         ///<! GRAPHIC_DATA_6[2]
        int thickness;      ///<! GRAPHIC_DATA_6[6]
        bool ital;          ///<! GRAPHIC_DATA_6[4] != 0.0

        std::string text;   ///<! GRAPHIC_DATA_7
    };

    using graphic_element = std::set<std::unique_ptr<GRAPHIC_ITEM>, GRAPHIC_ITEM::SEQ_CMP>;

    /// A!LAYER_SORT!LAYER_SUBCLASS!LAYER_ARTWORK!LAYER_USE!LAYER_CONDUCTOR!LAYER_DIELECTRIC_CONSTANT
    ///     !LAYER_ELECTRICAL_CONDUCTIVITY!LAYER_MATERIAL!LAYER_SHIELD_LAYER!LAYER_THERMAL_CONDUCTIVITY!LAYER_THICKNESS!
    struct FABMASTER_LAYER
    {
        int id;                 ///<! LAYER_SORT
        std::string name;       ///<! LAYER_SUBCLASS
        bool positive;          ///<! LAYER_ARTWORK (either POSITIVE or NEGATIVE)
        std::string use;        ///<! LAYER_USE
        bool conductive;        ///<! LAYER_CONDUCTOR
        double er;              ///<! LAYER_DIELECTRIC_CONSTANT
        double conductivity;    ///<! LAYER_ELECTRICAL_CONDUCTIVITY
        std::string material;   ///<! LAYER_MATERIAL
        bool shield;            ///<! LAYER_SHIELD_LAYER
        double thermal_cond;    ///<! LAYER_THERMAL_CONDUCTIVITY
        double thickness;       ///<! LAYER_THICKNESS
        int layerid;            ///<! pcbnew layer (assigned)
        bool disable;           ///<! if true, prevent the layer elements from being used

        struct BY_ID
        {
            bool operator()(const FABMASTER_LAYER* lhs, const FABMASTER_LAYER* rhs) const
            {
                return lhs->id < rhs->id;
            }
        };
    };

    std::map<std::string, FABMASTER_LAYER> layers;

    /**
     * A!SUBCLASS!PAD_SHAPE_NAME!GRAPHIC_DATA_NAME!GRAPHIC_DATA_NUMBER!RECORD_TAG!GRAPHIC_DATA_1!
     * GRAPHIC_DATA_2!GRAPHIC_DATA_3!GRAPHIC_DATA_4!GRAPHIC_DATA_5!GRAPHIC_DATA_6!GRAPHIC_DATA_7!
     * GRAPHIC_DATA_8!GRAPHIC_DATA_9!PAD_STACK_NAME!REFDES!PIN_NUMBER!
     */
    struct FABMASTER_PAD_SHAPE
    {
        std::string name;       ///<! PAD_SHAPE_NAME
        std::string padstack;   ///<! PAD_STACK_NAME
        std::string refdes;     ///<! REFDES
        std::string pinnum;     ///<! PIN_NUMBER
        std::map<int, graphic_element> elements;

        struct HASH
        {
            std::size_t operator()( const std::unique_ptr<FABMASTER_PAD_SHAPE>& aPad ) const
            {
                return std::hash<std::string>{}( aPad->name );
            }
        };
    };

    std::unordered_map<std::string, FABMASTER_PAD_SHAPE> pad_shapes;

    // * A!SYM_TYPE!SYM_NAME!REFDES!SYM_X!SYM_Y!SYM_ROTATE!SYM_MIRROR!NET_NAME!CLASS!SUBCLASS!RECORD_TAG!
    // * GRAPHIC_DATA_NAME!GRAPHIC_DATA_NUMBER!GRAPHIC_DATA_1!GRAPHIC_DATA_2!GRAPHIC_DATA_3!GRAPHIC_DATA_4!
    // * GRAPHIC_DATA_5!GRAPHIC_DATA_6!GRAPHIC_DATA_7!GRAPHIC_DATA_8!GRAPHIC_DATA_9!GRAPHIC_DATA_10!COMP_DEVICE_TYPE!
    // * COMP_PACKAGE!COMP_PART_NUMBER!COMP_VALUE!CLIP_DRAWING!
    struct SYMBOL
    {
        int sym_id;             ///<! RECORD_TAG[0]
        int seq_id;             ///<! RECORD_TAG[1] (RECORD_TAG is of the form "x y z")
        int subseq_id;          ///<! RECORD_TAG[2] (RECORD_TAG is of the form "x y z")
        std::string name;       ///<! SYM_NAME
        std::string refdes;     ///<! REFDES
        int x;                  ///<! SYM_X
        int y;                  ///<! SYM_Y
        std::map<int, graphic_element>  elements;

        struct HASH
        {
            std::size_t operator()( const FABMASTER::SYMBOL& aSym ) const
            {
                return std::hash<std::string>{}( aSym.name );
            }
        };
    };

    // Temporary data structure to pass graphic data from file for processing
    struct GRAPHIC_DATA
    {
        std::string graphic_dataname;
        std::string graphic_datanum;
        std::string graphic_data1;
        std::string graphic_data2;
        std::string graphic_data3;
        std::string graphic_data4;
        std::string graphic_data5;
        std::string graphic_data6;
        std::string graphic_data7;
        std::string graphic_data8;
        std::string graphic_data9;
        std::string graphic_data10;
    };

    std::unordered_map<std::string, SYMBOL> symbols;


    // * A!GRAPHIC_DATA_NAME!GRAPHIC_DATA_NUMBER!RECORD_TAG!GRAPHIC_DATA_1!GRAPHIC_DATA_2!GRAPHIC_DATA_3!
    // * GRAPHIC_DATA_4!GRAPHIC_DATA_5!GRAPHIC_DATA_6!GRAPHIC_DATA_7!GRAPHIC_DATA_8!GRAPHIC_DATA_9!
    // * SUBCLASS!SYM_NAME!REFDES!

    struct GEOM_GRAPHIC
    {
        std::string     subclass;   ///<! SUBCLASS
        std::string     name;       ///<! SYM_NAME
        std::string     refdes;     ///<! REFDES
        int             id;         ///<! RECORD_TAG[0]

        std::unique_ptr<graphic_element> elements;

        struct BY_ID
        {
            bool operator()(const GEOM_GRAPHIC& lhs, const GEOM_GRAPHIC& rhs) const
            {
                return lhs.id < rhs.id;
            }
        };
    };

    std::vector<GEOM_GRAPHIC> board_graphics;
    std::map<std::string, std::map<int, GEOM_GRAPHIC>> comp_graphics;


    // A!VIA_X!VIA_Y!PAD_STACK_NAME!NET_NAME!TEST_POINT!
    struct FM_VIA
    {
        int x;                  ///<! VIA_X
        int y;                  ///<! VIA_Y
        std::string padstack;   ///<! PAD_STACK_NAME
        std::string net;        ///<! NET_NAME
        bool test_point;        ///<! TEST_POINT
        bool mirror;            ///<! VIA_MIRROR (VIA_MIRROR is an optional component)
    };

    std::vector<std::unique_ptr<FM_VIA>> vias;

    // A!CLASS!SUBCLASS!GRAPHIC_DATA_NAME!GRAPHIC_DATA_NUMBER!RECORD_TAG!GRAPHIC_DATA_1!
    // GRAPHIC_DATA_2!GRAPHIC_DATA_3!GRAPHIC_DATA_4!GRAPHIC_DATA_5!GRAPHIC_DATA_6!GRAPHIC_DATA_7!
    // GRAPHIC_DATA_8!GRAPHIC_DATA_9!NET_NAME!
    struct TRACE
    {
        std::string     lclass;     ///<! CLASS
        std::string     layer;      ///<! SUBCLASS
        std::string     netname;    ///<! NET_NAME
        int             id;         ///<! RECORD_TAG[0]

        graphic_element segment;    ///<! GRAPHIC_DATA (can be either LINE or ARC)

        struct BY_ID
        {
            bool operator()(const std::unique_ptr<TRACE>& lhs, const std::unique_ptr<TRACE>& rhs) const
            {
                return lhs->id < rhs->id;
            }
        };
    };

    std::set<std::unique_ptr<TRACE>, TRACE::BY_ID> traces;

    std::set<std::unique_ptr<TRACE>, TRACE::BY_ID> zones;

    std::set<std::unique_ptr<TRACE>, TRACE::BY_ID> polygons;

    std::set<std::unique_ptr<TRACE>, TRACE::BY_ID> refdes;


    // A!REFDES!COMP_CLASS!COMP_PART_NUMBER!COMP_HEIGHT!COMP_DEVICE_LABEL!COMP_INSERTION_CODE!SYM_TYPE!
    // SYM_NAME!SYM_MIRROR!SYM_ROTATE!SYM_X!SYM_Y!COMP_VALUE!COMP_TOL!COMP_VOLTAGE!
    struct COMPONENT
    {
        std::string refdes;         ///<! REFDES
        COMPCLASS cclass;           ///<! COMP_CLASS
        std::string pn;             ///<! COMP_PART_NUMBER
        std::string height;         ///<! COMP_HEIGHT
        std::string dev_label;      ///<! COMP_DEVICE_LABEL
        std::string insert_code;    ///<! COMP_INSERTION_CODE
        SYMTYPE type;               ///<! SYM_TYPE
        std::string name;           ///<! SYM_NAME
        bool mirror;                ///<! SYM_MIRROR
        double rotate;              ///<! SYM_ROTATE (degrees)
        int x;                      ///<! SYM_X
        int y;                      ///<! SYM_Y
        std::string value;          ///<! COMP_VALUE
        std::string tol;            ///<! COMP_TOL
        std::string voltage;        ///<! COMP_VOLTAGE

        struct HASH
        {
            std::size_t operator()( const FABMASTER::COMPONENT& aCmp ) const
            {
                return std::hash<std::string>{}( aCmp.refdes );
            }
        };

    };

    std::map<std::string, std::vector<std::unique_ptr<COMPONENT>>> components;


    // A!SYM_NAME!SYM_MIRROR!PIN_NAME!PIN_NUMBER!PIN_X!PIN_Y!PAD_STACK_NAME!REFDES!PIN_ROTATION!TEST_POINT!

    struct PIN
    {
        std::string name;       ///<! SYM_NAME
        bool mirror;            ///<! SYM_MIRROR
        std::string pin_name;   ///<! PIN_NAME
        std::string pin_number; ///<! PIN_NUMBER
        int pin_x;              ///<! PIN_X - Absolute board units
        int pin_y;              ///<! PIN_Y - Absolute board units
        std::string padstack;   ///<! PAD_STACK
        std::string refdes;     ///<! REFDES
        double rotation;        ///<! PIN_ROTATION

        struct BY_NUM
        {
            bool operator()(const std::unique_ptr<PIN>& lhs,
                    const std::unique_ptr<PIN>& rhs) const
            {
                return lhs->pin_number < rhs->pin_number;
            }
        };
    };

    std::map<std::string, std::set<std::unique_ptr<PIN>, PIN::BY_NUM>> pins;

    std::map<std::string, PCB_LAYER_ID> layer_map;

    section_type detectType( size_t aOffset );

    void checkpoint();

    int execute_recordbuffer( int filetype );
    int getColFromName( size_t aRow, const std::string& aStr );
    SYMTYPE parseSymType( const std::string& aSymType );
    COMPCLASS parseCompClass( const std::string& aCompClass );

    /**
     * Processes data from text vectors into internal database
     * for further ordering
     * @param aRow vector offset being processed
     * @return Count of the number of rows processed, return -1 on error
     */
    double processScaleFactor( size_t aRow );
    size_t processPadStacks( size_t aRow );
    size_t processCustomPads( size_t aRow );
    size_t processGeometry( size_t aRow );
    size_t processVias( size_t aRow );
    size_t processTraces( size_t aRow );
    size_t processFootprints( size_t aRow );
    size_t processNets( size_t aRow );
    size_t processLayers( size_t aRow );
    size_t processSimpleLayers( size_t aRow );
    size_t processPadStackLayers( size_t aRow );
    size_t processSymbols( size_t aRow );
    size_t processPins( size_t aRow );

    /**
     * Specialty functions for processing graphical data rows into the internal
     * database
     * @param aData Loaded data vector
     * @param aScale Prior loaded scale factor
     * @return Pointer to newly allocated graphical item or nullptr on failure
     */
    GRAPHIC_ITEM* processGraphic( const GRAPHIC_DATA& aData, double aScale );

    GRAPHIC_ARC*       processArc( const GRAPHIC_DATA& aData, double aScale );
    GRAPHIC_ARC*       processCircle( const GRAPHIC_DATA& aData, double aScale );
    GRAPHIC_LINE*      processLine( const GRAPHIC_DATA& aData, double aScale );
    GRAPHIC_TEXT*      processText( const GRAPHIC_DATA& aData, double aScale );
    GRAPHIC_RECTANGLE* processRectangle( const GRAPHIC_DATA& aData, double aScale );
    GRAPHIC_RECTANGLE* processFigRectangle( const GRAPHIC_DATA& aData, double aScale );
    GRAPHIC_RECTANGLE* processSquare( const GRAPHIC_DATA& aData, double aScale );
    GRAPHIC_OBLONG*    processOblong( const GRAPHIC_DATA& aData, double aScale );
    GRAPHIC_CROSS*     processCross( const GRAPHIC_DATA& aData, double aScale );
    GRAPHIC_POLYGON*   processPolygon( const GRAPHIC_DATA& aData, double aScale );

    PCB_LAYER_ID getLayer( const std::string& aLayerName );
    bool assignLayers();

    /**
     * Reads the double/integer value from a std string independent of the user locale
     * @param aStr string to generate value from
     * @return 0 if value cannot be created
     */
    double readDouble( const std::string& aStr ) const;
    int readInt( const std::string& aStr ) const;

    /**
     * Sets zone priorities based on zone BB size.  Larger bounding boxes get smaller priorities
     * so smaller zones can knock out areas where they overlap.
     * @param aBoard
     * @return True if successful
     */
    bool orderZones( BOARD* aBoard );

    /**
     * Loads sections of the database into the board
     * @param aBoard
     * @return True if successful
     */
    bool loadZones( BOARD* aBoard );
    bool loadOutline( BOARD* aBoard, const std::unique_ptr<TRACE>& aLine);
    bool loadNets( BOARD* aBoard );
    bool loadLayers( BOARD* aBoard );
    bool loadGraphics( BOARD* aBoard );
    bool loadVias( BOARD* aBoard );
    bool loadEtch( BOARD* aBoard, const std::unique_ptr<TRACE>& aLine);
    bool loadZone( BOARD* aBoard, const std::unique_ptr<FABMASTER::TRACE>& aLine);
    bool loadPolygon( BOARD* aBoard, const std::unique_ptr<FABMASTER::TRACE>& aLine);
    bool loadFootprints( BOARD* aBoard );

    SHAPE_POLY_SET loadShapePolySet( const graphic_element& aLine);

    static bool traceIsOpen( const FABMASTER::TRACE& aLine );

    /**
     * Set parameters for graphic text
     */
    static void setupText( const FABMASTER::GRAPHIC_TEXT& aGraphicText, PCB_LAYER_ID aLayer,
                           PCB_TEXT& aText, const BOARD& aBoard, const OPT_VECTOR2I& aMirrorPoint );

    /**
     * Convert one Fabmaster graphic item to one or more PCB items
     */
    static std::vector<std::unique_ptr<BOARD_ITEM>>
    createBoardItems( BOARD& aBoard, PCB_LAYER_ID aLayer, FABMASTER::GRAPHIC_ITEM& aGraphic );

    PROGRESS_REPORTER*  m_progressReporter;  ///< optional; may be nullptr
    unsigned            m_doneCount;
    unsigned            m_lastProgressCount;
    unsigned            m_totalCount;         ///< for progress reporting
};




#endif /* PCBNEW_IMPORTERS_IMPORT_FABMASTER_H_ */

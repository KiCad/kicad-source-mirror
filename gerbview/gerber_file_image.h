/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2019 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef GERBER_FILE_IMAGE_H
#define GERBER_FILE_IMAGE_H

#include <vector>
#include <set>

#include <dcode.h>
#include <gerber_draw_item.h>
#include <am_primitive.h>
#include <aperture_macro.h>
#include <gbr_netlist_metadata.h>

typedef std::vector<GERBER_DRAW_ITEM*> GERBER_DRAW_ITEMS;

class GERBVIEW_FRAME;
class D_CODE;

/* Gerber files have different parameters to define units and how items must be plotted.
 * some are for the entire file, and other can change along a file.
 * In Gerber world:
 * an image is the entire gerber file and its "global" parameters
 * a layer (that is very different from a board layer) is just a sub set of a file that
 * have specific parameters
 * if a Image parameter is set more than once, only the last value is used
 * Some parameters can change along a file and are not layer specific: they are stored
 * in GERBER_ITEM items, when instanced.
 *
 * In GerbView, to handle these parameters, there are 2 classes:
 * GERBER_FILE_IMAGE : the main class containing most of parameters and data to plot a
 * graphic layer
 * Some of them can change along the file
 * There is one GERBER_FILE_IMAGE per file and one graphic layer per file or GERBER_FILE_IMAGE
 * GerbView does not read and merge 2 gerber file in one graphic layer:
 * I believe this is not possible due to the constraints in Image parameters.
 * GERBER_LAYER : containing the subset of parameters that is layer specific
 * A GERBER_FILE_IMAGE must include one GERBER_LAYER to define all parameters to plot a file.
 * But a GERBER_FILE_IMAGE can use more than one GERBER_LAYER.
 */

class GERBER_FILE_IMAGE;
class X2_ATTRIBUTE;
class X2_ATTRIBUTE_FILEFUNCTION;

// For arcs, coordinates need 3 info: start point, end point and center or radius
// In Excellon files it can be a A## value (radius) or I#J# center coordinates (like in gerber)
// We need to know the last read type when reading a list of routing coordinates
enum LAST_EXTRA_ARC_DATA_TYPE
{
    ARC_INFO_TYPE_NONE,
    ARC_INFO_TYPE_CENTER,   // last info is a IJ command: arc center is given
    ARC_INFO_TYPE_RADIUS,   // last info is a A command: arc radius is given
};

class GERBER_LAYER
{
public:
    GERBER_LAYER();
    ~GERBER_LAYER();

private:
    void ResetDefaultValues();
    friend class GERBER_FILE_IMAGE;

public:
   // These parameters are layer specific:
    wxString    m_LayerName;            // Layer name, from LN <name>* command
    bool        m_LayerNegative;        // true = Negative Layer: command LP
    wxRealPoint m_StepForRepeat;        // X and Y offsets for Step and Repeat command
    int         m_XRepeatCount;         // The repeat count on X axis
    int         m_YRepeatCount;         // The repeat count on Y axis
    bool        m_StepForRepeatMetric;  // false = Inches, true = metric
                                        // needed here because repeated
                                        // gerber items can have coordinates
                                        // in different units than step parameters
                                        // and the actual coordinates calculation must handle this
};

// Currently, the Gerber file is parsed line by line.
// This is most of time OK
// GERBER_BUFZ is the max size of a single line of text from a gerber file.
// But warning: in rare cases some files can have *very long* lines,
// so the buffer must be very large.
// I saw a file using only one line of 1,400,000 chars
#define GERBER_BUFZ 5000000

/**
 * Hold the image data and parameters for one gerber file and layer parameters.
 *
 * @todo: Move them in #GERBER_LAYER class.
 */
class GERBER_FILE_IMAGE : public EDA_ITEM
{
public:
    GERBER_FILE_IMAGE( int layer );
    virtual ~GERBER_FILE_IMAGE();

    wxString GetClass() const override
    {
        return wxT( "GERBER_FILE_IMAGE" );
    }

    /**
     * @brief Performs a heuristics-based check of whether the file is an RS274 gerber file.
     *
     * Does not invoke the full parser.
     *
     * @param aFullFileName aFullFileName is the full filename of the gerber file.
     * @return True if RS274 file, false otherwise
     */
    static bool TestFileIsRS274( const wxString& aFullFileName );

    /**
     * Read and load a gerber file.
     *
     * If the file cannot be loaded, warning and information messages are stored in m_messagesList.
     *
     * @param aFullFileName The full filename of the Gerber file.
     * @return true if file loaded successfully, false if the gerber file was not loaded.
     */
    bool LoadGerberFile( const wxString& aFullFileName );

    const wxArrayString& GetMessages() const { return m_messagesList; }

    /**
     * @return the count of Dcode tools in use in the image
     */
    int GetDcodesCount();

    /**
     * Set all parameters to a default value, before reading a file
     */
    virtual void ResetDefaultValues();

    COLOR4D GetPositiveDrawColor() const { return m_PositiveDrawColor; }

    /**
     * @return a reference to the GERBER_DRAW_ITEMS deque list
     */
    GERBER_DRAW_ITEMS& GetItems() { return m_drawings; }

    /**
     * @return the count of GERBER_DRAW_ITEMS in the image
     */
    int GetItemsCount() { return m_drawings.size(); }

    /**
     * Add a new GERBER_DRAW_ITEM item to the drawings list
     *
     * @param aItem is the GERBER_DRAW_ITEM to add to list
     */
    void AddItemToList( GERBER_DRAW_ITEM* aItem )
    {
        m_drawings.push_back( aItem );
    }

    /**
     * @return the last GERBER_DRAW_ITEM* item of the items list
     */
    GERBER_DRAW_ITEM* GetLastItemInList() const
    {
        return m_drawings.back();
    }

    /**
     * @return the current layers params.
     */
    GERBER_LAYER& GetLayerParams()
    {
        return m_GBRLayerParams;
    }

    /**
     * @return true if at least one item must be drawn in background color used to optimize
     *         screen refresh (when no items are in background color refresh can be faster).
     */
    bool HasNegativeItems();

    /**
     * Clear the message list.
     *
     * Call it before reading a Gerber file
     */
    void ClearMessageList()
    {
        m_messagesList.Clear();
    }

    /**
     * Add a message to the message list
     */
    void AddMessageToList( const wxString& aMessage );

    /**
     * Return the current coordinate type pointed to by XnnYnn Text (XnnnnYmmmm).
     *
     * @param aText is a pointer to the text to parse.
     * @param aExcellonMode = true to parse a Excellon drill file.
     * it forces truncation of a digit string to a maximum length because the exact coordinate
     * format is not always known.
     */
    VECTOR2I ReadXYCoord( char*& aText, bool aExcellonMode = false );

    /**
     * Return the current coordinate type pointed to by InnJnn Text (InnnnJmmmm)
     *
     * These coordinates are relative, so if coordinate is absent, its value
     * defaults to 0
     */
    VECTOR2I ReadIJCoord( char*& Text );

    /**
     * Reads the next number and returns the value
     * @param aText Pointer to the input string vector
     * @return
     */
    int CodeNumber( char*& aText );

    /**
     * Return a pointer to the D_CODE within this GERBER for the given \a aDCODE.
     *
     * @param aDCODE The numeric value of the D_CODE to look up.
     * @param aCreateIfNoExist If true, then create the D_CODE if it does not
     *                         exist in list.
     * @return The one implied by the given \a aDCODE or NULL if the requested \a aDCODE is
     *         out of range.
     */
    D_CODE* GetDCODEOrCreate( int aDCODE, bool aCreateIfNoExist = true );

    /**
     * Return a pointer to the D_CODE within this GERBER for the given \a aDCODE.
     *
     * @param aDCODE The numeric value of the D_CODE to look up.
     * @return The D code implied by the given \a aDCODE or NULL if the requested \a aDCODE is
     *         out of range.
     */
    D_CODE* GetDCODE( int aDCODE ) const;

    /**
     * Look up a previously read in aperture macro.
     *
     * @param aLookup A dummy APERTURE_MACRO with [only] the name field set.
     * @return the aperture macro with a matching name or NULL if not found.
     */
    APERTURE_MACRO* FindApertureMacro( const APERTURE_MACRO& aLookup );

    /**
     * Gerber format has a command Step an Repeat.
     *
     * This function must be called when reading a gerber file and
     * after creating a new gerber item that must be repeated
     * (i.e when m_XRepeatCount or m_YRepeatCount are > 1)
     *
     * @param aItem = the item to repeat
     */
    void StepAndRepeatItem( const GERBER_DRAW_ITEM& aItem );

    /**
     * Display information about image parameters in the status bar.
     *
     * @param aMainFrame The #GERBVIEW_FRAME to display messages.
     */
    void DisplayImageInfo( GERBVIEW_FRAME* aMainFrame );

    /**
     * Set the offset and rotation to draw a file image
     * Does not change any coordinate od draw items
     * @param aOffsetMM is the draw offset in millimeters
     * @param aRotation is the draw roation
     * draw transform order is rotation and after offset
     */
    void SetDrawOffetAndRotation( VECTOR2D aOffsetMM, EDA_ANGLE aRotation );

    /**
     * Called when a %TD command is found the Gerber file
     *
     * Remove the attribute specified by the %TD command.
     * is no attribute, all current attributes specified by the %TO and the %TA
     * commands are cleared.
     * if a attribute name is specified (for instance %TD.CN*%) is specified,
     * only this attribute is cleared
     *
     * @param aAttribute is the X2_ATTRIBUTE which stores the full command
     */
    void RemoveAttribute( X2_ATTRIBUTE& aAttribute );

    ///< @copydoc EDA_ITEM::Visit()
    INSPECT_RESULT Visit( INSPECTOR inspector, void* testData,
                          const std::vector<KICAD_T>& aScanTypes ) override;

#if defined(DEBUG)

    void    Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }

#endif

private:
    /**
     * Test for an end of line.
     *
     * If a end of line is found, read a new line.
     *
     * @param aBuff = buffer (size = GERBER_BUFZ) to fill with a new line
     * @param aText = pointer to the last useful char in aBuff
     *          on return: points the beginning of the next line.
     * @param aBuffSize = the size in bytes of aBuff
     * @param aFile = the opened GERBER file to read
     * @return a pointer to the beginning of the next line or NULL if end of file
    */
    char* GetNextLine( char *aBuff, unsigned int aBuffSize, char* aText, FILE* aFile );

    bool GetEndOfBlock( char* aBuff, unsigned int aBuffSize, char*& aText, FILE* aGerberFile );

    /**
     * Read a single RS274X command terminated with a %
     */
    bool ReadRS274XCommand( char *aBuff, unsigned int aBuffSize, char*& aText );

    /**
     * Execute a RS274X command
     */
    bool ExecuteRS274XCommand( int aCommand, char* aBuff, unsigned int aBuffSize, char*& aText );

    /**
     * Read two bytes of data and assembles them into an int with the first
     * byte in the sequence put into the most significant part of a 16 bit value
     * to build a RS274X command identifier.
     *
     * @param text A reference to a pointer to read bytes from and to advance as
     *             they are read.
     * @return a RS274X command identifier.
     */
    int ReadXCommandID( char*& text );

    /**
     * Read in an aperture macro and saves it in m_aperture_macros.
     *
     * @param aBuff a character buffer at least GERBER_BUFZ long that can be
     *              used to read successive lines from the gerber file.
     * @param text A reference to a character pointer which gives the initial
     *             text to read from.
     * @param aBuffSize is the size of aBuff
     * @param gerber_file Which file to read from for continuation.
     * @return true if a macro was read in successfully, else false.
     */
    bool ReadApertureMacro( char *aBuff, unsigned int aBuffSize, char*& text, FILE* gerber_file );

    // functions to execute G commands or D basic commands:
    bool Execute_G_Command( char*& text, int G_command );
    bool Execute_DCODE_Command( char*& text, int D_command );

public:
    bool               m_InUse;                ///< true if this image is currently in use (a file
                                               ///<   is loaded in it)
                                               ///< false if it must be not drawn
    COLOR4D            m_PositiveDrawColor;    ///< The color used to draw positive items
    wxString           m_FileName;             ///< Full File Name for this layer
    wxString           m_ImageName;            ///< Image name, from IN <name>* command

    bool               m_IsX2_file;            ///< True if a X2 gerber attribute was found in file
    X2_ATTRIBUTE_FILEFUNCTION* m_FileFunction; ///< file function parameters, found in a %TF
                                               ///<   command or a G04
    wxString           m_MD5_value;            ///< MD5 value found in a %TF.MD5 command
    wxString           m_PartString;           ///< string found in a %TF.Part command
    int                m_GraphicLayer;         ///< Graphic layer Number
    bool               m_ImageNegative;        ///< true = Negative image

    bool               m_ImageJustifyXCenter;  ///< Image Justify Center on X axis (default = false)
    bool               m_ImageJustifyYCenter;  ///< Image Justify Center on Y axis (default = false)
    VECTOR2I           m_ImageJustifyOffset;   ///< Image Justify Offset on XY axis (default = 0,0)

    bool               m_GerbMetric;           ///< false = Inches, true = metric
    bool               m_Relative;             ///< false = absolute Coord, true = relative Coord.
    bool               m_NoTrailingZeros;      ///< true: remove tailing zeros.
    VECTOR2I           m_ImageOffset;          ///< Coord Offset, from IO command
    wxSize             m_FmtScale;             ///< Fmt 2.3: m_FmtScale = 3, fmt 3.4: m_FmtScale = 4
    wxSize             m_FmtLen;               ///< Nb chars per coord. ex fmt 2.3, m_FmtLen = 5

    int                m_ImageRotation;        ///< Image rotation (0, 90, 180, 270 only) in degrees
    double             m_LocalRotation;        ///< Local rotation added to m_ImageRotation
                                               ///< @note This value is stored in 0.1 degrees

    VECTOR2I           m_Offset;               ///< Coord Offset, from OF command
    VECTOR2I           m_Scale;                ///< scale (X and Y) of layer.
    bool               m_SwapAxis;             ///< false if A = X and B = Y (default); true if
                                               ///<   A = Y, B = X
    bool               m_MirrorA;              ///< true: mirror / axis A (X)
    bool               m_MirrorB;              ///< true: mirror / axis B (Y)
    int                m_Iterpolation;         ///< Linear, 90 arc, Circ.
    int                m_Current_Tool;         ///< Current Tool (Dcode) number selected
    int                m_Last_Pen_Command;     ///< Current or last pen state (0..9, set by Dn
                                               ///<   option with n < 10
    int                m_CommandState;         ///< state of gerber analysis command
    int                m_LineNum;              ///< Line number of the gerber file while reading.
    VECTOR2I           m_CurrentPos;           ///< current specified coord for plot
    VECTOR2I           m_PreviousPos;          ///< old current specified coord for plot
    VECTOR2I           m_IJPos;                ///< IJ coord (for arcs & circles )

    ///< True if a IJ coord was read (for arcs & circles ).
    bool               m_LastCoordIsIJPos;

    ///< A value ( = radius in circular routing in Excellon files ).
    int                m_ArcRadius;

    ///< Identifier for arc data type (IJ (center) or A## (radius)).
    LAST_EXTRA_ARC_DATA_TYPE m_LastArcDataType;
    FILE*              m_Current_File;                   // Current file to read

    int                m_Selected_Tool;                  // For highlight: current selected Dcode

    ///< True if has DCodes in file or false if no DCodes found.  Perhaps deprecated RS274D file.
    bool               m_Has_DCode;

    // true = some DCodes in file are not defined (broken file or deprecated RS274D file).
    bool               m_Has_MissingDCode;
    bool               m_360Arc_enbl;                    // Enable 360 deg circular interpolation

    // Set to true when a circular interpolation command type is found. Mandatory before
    // drawing an arc.
    bool               m_AsArcG74G75Cmd;

    // Enable polygon mode (read coord as a polygon descr)
    bool               m_PolygonFillMode;

    // In polygon mode: 0 = first segm, 1 = next segm
    int                m_PolygonFillModeState;

    ///< a collection of APERTURE_MACROS, sorted by name
    APERTURE_MACRO_SET m_aperture_macros;

    // the net attributes set by a %TO.CN, %TO.C and/or %TO.N add object attribute command.
    GBR_NETLIST_METADATA m_NetAttributeDict;

    // the aperture function set by a %TA.AperFunction, xxx (stores the xxx value).
    wxString            m_AperFunction;

    std::map<wxString, int> m_ComponentsList;            // list of components
    std::map<wxString, int> m_NetnamesList;              // list of net names

    /// Dcode (Aperture) List for this layer (see dcode.h)
    std::map<int, D_CODE*> m_ApertureList;

    ///< Whether an aperture macro tool is flashed on or off.
    bool               m_Exposure;

    GERBER_LAYER       m_GBRLayerParams;                 // hold params for the current gerber layer
    GERBER_DRAW_ITEMS  m_drawings;                       // linked list of Gerber Items to draw

    ///< Parameters used only to draw (display) items on this layer.
    ///< Do not change actual coordinates/orientation
    VECTOR2I           m_DisplayOffset;
    EDA_ANGLE          m_DisplayRotation;

    // A large buffer to store one line
    static char m_LineBuffer[GERBER_BUFZ+1];

private:
    wxArrayString      m_messagesList;         // A list of messages created when reading a file

    /**
     * True if the image is negative or has some negative items.
     *
     * Used to optimize drawing because when there are no negative items screen refresh does
     * not need to build an intermediate bitmap specific to this image.
     *
     *  - -1 negative items are.
     *  - 0 no negative items found.
     *  - 1 have negative items found.
     */
    int                m_hasNegativeItems;
};

#endif  // ifndef GERBER_FILE_IMAGE_H

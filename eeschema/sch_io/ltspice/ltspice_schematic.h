/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Chetan Subhash Shinde<chetanshinde2001@gmail.com>
 * Copyright (C) 2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LTSPICE_SCHEMATIC_LOADER_H
#define LTSPICE_SCHEMATIC_LOADER_H

#include <sch_io/sch_io_mgr.h>
#include <sch_sheet_path.h>
#include <pin_type.h>
#include <layer_ids.h>
#include <math/box2.h>
#include <wx/filename.h>
#include <wx/dir.h>


struct LTSPICE_FILE
{
    wxString       ElementName;
    VECTOR2I       Offset;
    int            ParentIndex;
    SCH_SHEET*     Sheet;
    SCH_SCREEN*    Screen;
    SCH_SHEET_PATH SheetPath;

    LTSPICE_FILE( const wxString& aElementName, const VECTOR2I& aOffset ) :
            ElementName( aElementName.Lower() ),
            Offset( aOffset ),
            ParentIndex( 0 ),
            Sheet( nullptr ),
            Screen( nullptr )
    { }

    bool operator<( const LTSPICE_FILE& t ) const
    {
        return ElementName < t.ElementName;
    }
};


class LTSPICE_SCHEMATIC
{
public:
    enum class LINESTYLE
    {
        SOLID = 0,
        DASH = 1,
        DOT = 2,
        DASHDOT = 3,
        DASHDOTDOT = 4
    };

    enum class LINEWIDTH
    {
        Normal = 5,
        Wide = 10
    };

    /**
     * Polarity enum represents polarity of pin
     */
    enum class POLARITY
    {
        PIN_INPUT,  // IN POLARITY
        OUTPUT, // OUT POLARITY
        BIDIR   // BI-DIRECTIONAL POLARITY
    };

    /**
     * Type of Symbol loaded from asc and asy file
     *
     * NOTE: Currently we have only found CELL and BLOCK type of symbol components others, we
     * are not able to reproduce till now.
     */
    enum class SYMBOLTYPE
    {
        CELL,
        BLOCK
    };

    /**
     * Defines in what ways the PIN or TEXT can be justified
     */
    enum class JUSTIFICATION
    {
        NONE,
        BOTTOM,
        LEFT,
        RIGHT,
        CENTER,
        TOP,
        VBOTTOM,  // Vertical Bottom Justification
        VLEFT,    // Vertical Left Justification
        VRIGHT,   // Vertical Right Justification
        VCENTER,  // Vertical Center Justification
        VTOP,      // Vertical Top Justification.
        INVISIBLE
    };

    /**
     * Defines different types or rotation and mirror operation can be applied to a LT_SYMBOL.
     */
    enum class ORIENTATION
    {
        R0,     // 0 degree rotation
        R90,    // 90 degree rotatioin.
        R180,   // 180 degree rotation.
        R270,   // 270 degree rotation.
        M0,     // 0 degree mirror
        M90,    // 90 degree mirror
        M180,   // 180 degree mirror
        M270    // 270 degree mirror
    };

    struct LINE
    {
        VECTOR2I  Start;
        VECTOR2I  End;
        LINEWIDTH LineWidth;
        LINESTYLE LineStyle;
    };

    struct LT_PIN
    {
        VECTOR2I                     PinLocation;
        JUSTIFICATION                PinJustification;
        int                          NameOffSet;
        std::map<wxString, wxString> PinAttribute;
    };

    /**
     * The CIRCLE is represented in Ltpsice inside a rectangle whose two opposite points and
     * line style are given.
     */
    struct CIRCLE
    {
        VECTOR2I  TopLeft;
        VECTOR2I  BotRight;
        LINEWIDTH LineWidth;
        LINESTYLE LineStyle;
    };

    struct LT_WINDOW
    {
        int           WindowNumber;
        int           FontSize;
        VECTOR2I      Position;
        JUSTIFICATION Justification;
    };

    /**
     * The ARC is represented inside a rectangle whose opposite site are given.
     * To mark the starting and ending of the ARC two points are given on the rectangle
     */
    struct ARC
    {
        VECTOR2I  TopLeft;
        VECTOR2I  BotRight;
        VECTOR2I  ArcStart;
        VECTOR2I  ArcEnd;
        LINEWIDTH LineWidth;
        LINESTYLE LineStyle;
    };

    /**
     * A 4-sided polygon with opposite equal sides, used in representing shapes
     */
    struct RECTANGLE
    {
        VECTOR2I  TopLeft;
        VECTOR2I  BotRight;
        LINEWIDTH LineWidth;
        LINESTYLE LineStyle;
    };

    /**
     * A metallic connection, used for transfer, between two points or pin.
     */
    struct WIRE
    {
        VECTOR2I Start;
        VECTOR2I End;
    };

    struct FLAG
    {
        VECTOR2I Offset;
        int      FontSize;
        wxString Value;
    };

    struct DATAFLAG
    {
        VECTOR2I Offset;
        int      FontSize;
        wxString Expression;
    };

    struct TEXT
    {
        VECTOR2I      Offset;
        JUSTIFICATION Justification;
        int           FontSize;
        wxString      Value;
    };

    /**
     * IOPIN is special contact on symbol used for IO operations.
     */
    struct IOPIN
    {
        VECTOR2I Location;
        POLARITY Polarity;
    };

    struct BUSTAP
    {
        VECTOR2I Start;
        VECTOR2I End;
    };

    /**
     * A struct to hold SYMBOL definition
     */
    struct LT_SYMBOL
    {
        wxString                     Name;
        SYMBOLTYPE                   SymbolType;
        VECTOR2I                     Offset;
        std::map<wxString, wxString> SymAttributes;
        std::vector<LT_PIN>          Pins;
        std::vector<LINE>            Lines;
        std::vector<CIRCLE>          Circles;
        std::vector<LT_WINDOW>       Windows;
        std::vector<ARC>             Arcs;
        std::vector<RECTANGLE>       Rectangles;
        std::vector<WIRE>            Wires;
        ORIENTATION                  SymbolOrientation;
    };

    /**
     * A struct to hold .asc file definition
     */
    struct LT_ASC
    {
        VECTOR2I               SheetSize;
        int                    Version;
        int                    VersionMinor;
        int                    SheetNumber;
        std::vector<LT_SYMBOL> Symbols;
        std::vector<LINE>      Lines;
        std::vector<CIRCLE>    Circles;
        std::vector<LT_WINDOW> Windows;
        std::vector<ARC>       Arcs;
        std::vector<RECTANGLE> Rectangles;
        std::vector<WIRE>      Wires;
        std::vector<FLAG>      Flags;
        std::vector<DATAFLAG>  DataFlags;
        std::vector<IOPIN>     Iopins;
        std::vector<BUSTAP>    Bustap;
        std::vector<TEXT>      Texts;
        BOX2I                  BoundingBox;
    };

    explicit LTSPICE_SCHEMATIC( const wxString& aFilename, const wxFileName& aLTspiceDataDir,
                                REPORTER* aReporter, PROGRESS_REPORTER* aProgressReporter ) :
            m_reporter( aReporter ),
            m_schematic( nullptr ),
            m_ltspiceDataDir( aLTspiceDataDir )
    {}

    ~LTSPICE_SCHEMATIC() {}

    /**
     * The main function responsible for loading the .asc and .asy files.
     *
     * @param aSchematic is the schematic object.
     * @param aRootSheet is the root sheet referencing variable.
     * @param aLibraryFileName is the name of the library which gets created when the plugin runs.
     */
    void Load( SCHEMATIC* aSchematic, SCH_SHEET* aRootSheet, const wxFileName& aLibraryFileName,
               REPORTER* aReporter );

    /**
     * Used to get file path for Asc and Asy files.
     *
     * @param aMapOfAscFiles map of string containing content from asc files.
     * @param aMapOfAsyFiles map of string containing content from asy files.
     */
    void GetAscAndAsyFilePaths( const wxDir& aDir, bool aRecursive,
                                std::map<wxString, wxString>& aMapOfAscFiles,
                                std::map<wxString, wxString>& aMapOfAsyFiles,
                                const std::vector<wxString>&  aBaseDirs = {} );

    /**
     * Used to get symbols list present in asc file.
     *
     * @param aFilePath path where file to be read is kept.
     * @param aReadType specifies in which type the file is to be read eg.
     *         r specifies file is use open for reading.
     */
    std::vector<LTSPICE_FILE> GetSchematicElements( const wxString& aAscFile );

    /**
     * Check if the asy file content indicates that we need to load subsheet.
     *
     * @param aAsyFile contents of .asy file.
     */
    bool IsAsySubsheet( const wxString& aAsyFile );

    /**
     * The function returns a map. This map has all the asy files in form of string. For asy files
     * the key will be symbol name.
     *
     * @param aFilenames contains all the symbols for which we
     * we have to load the .asy files
     * @return a map of String having all .asy files
     */
    std::map<wxString, wxString> ReadAsyFiles( const std::vector<LTSPICE_FILE>& aSourceFiles,
                                               const std::map<wxString, wxString>& aAsyFileMap );

    std::map<wxString, wxString> ReadAsyFile( const LTSPICE_FILE& aSourceFile,
                                              const std::map<wxString, wxString>& aAsyFileMap );

    /**
     * Build Intermediate data structure.
     * @return LT_ASC struct filled with all info from asc and asy files.
     */
    std::vector<LT_ASC> StructureBuilder();

    LT_SYMBOL SymbolBuilder( const wxString& aAscFileName, LT_ASC& aAscFile );

    LT_SYMBOL SymbolBuilder( const wxString& aAscFileName, const wxString& aAsyFileContent,
                             LT_ASC& aAscFile );

    LT_SYMBOL MakeDummySymbol( const wxString& aAscFileName );

    const wxFileName& GetLTspiceDataDir() { return m_ltspiceDataDir; }

private:
    /**
     * Join value present across multiple tokens into one
     *
     * @param aTokens an array of tokenised data.
     * @param aIndex from where the tokens should be concatenated.
     */
    static void aggregateAttributeValue( wxArrayString& aTokens, int aIndex );

    /**
     * Used to check if the given token in integer or not.
     *
     * @param aToken token to be verified.
     * @param aLineNumber gives on which line number the check is called.
     * @param aFileName gives in which file the check is been called.
     * @return the integer value.
     * @throw IO_ERROR if the token is not an integer.
     */
    static int integerCheck( const wxString& aToken, int aLineNumber, const wxString& aFileName );

    static VECTOR2I pointCheck( const wxString& aTokenX, const wxString& aTokenY, int aLineNumber,
                                const wxString& aFileName );
    /**
     * Used to check size of the token generated from split function.
     *
     * @param aActualSize actual size of array of token.
     * @param aExpectedMin expected lower range(i.e size) of array of token.
     * @param aExpectedMax expected higher range(i.e size) of array of token.
     * @param aLineNumber gives on which line number the check is called.
     * @param aFileName gives in which file the check is been called.
     * @throw IO_ERROR if the actualSize is not within expected lower range..higher range.
     */
    static void tokensSizeRangeCheck( size_t aActualSize, int aExpectedMin, int aExpectedMax,
                                      int aLineNumber, const wxString& aFileName );

    static void removeCarriageReturn( wxString& elementFromLine );

    /**
     * @param aValue integer value between 0-3
     * @return LINESTYLE enum
     */
    static LINESTYLE getLineStyle( int aValue );

    /**
     * @param aValue string value
     * @return LINEWIDTH enum
     */
    static LINEWIDTH getLineWidth( const wxString& aValue );

    /**
     * @param aValue string value
     * @return POLARITY enum
     */
    static POLARITY getPolarity( const wxString& aValue );

    /**
     * @param aValue string value
     * @return ORIENTATION enum
     */
    static ORIENTATION getSymbolRotationOrMirror( const wxString& aValue );

    /**
     * @param aValue string value
     * @return JUSTIFICATION enum
     */
    static JUSTIFICATION getTextJustification( const wxString& aValue );

    /**
     * @param aValue string value
     * @return JUSTIFICATION enum
     */
    static JUSTIFICATION getPinJustification( const wxString& aValue );

    /**
     * @param aValue string value
     * @return SYMBOLTYPE enum
     */
    static SYMBOLTYPE getSymbolType( const wxString& aValue );

private:
    REPORTER*            m_reporter;
    SCHEMATIC*           m_schematic;
    wxFileName           m_ltspiceDataDir;

    std::map<wxString, std::map<wxString, wxString>> m_fileCache;
};

#endif // LTSPICE_SCHEMATIC_LOADER_H

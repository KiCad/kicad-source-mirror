/********************/
/* class_excellon.h */
/********************/

#ifndef CLASS_EXCELLON_H
#define CLASS_EXCELLON_H


enum drill_M_code_t {
    DRILL_M_UNKNOWN,
    DRILL_M_END,
    DRILL_M_ENDREWIND,
    DRILL_M_MESSAGE,
    DRILL_M_LONGMESSAGE,
    DRILL_M_HEADER,
    DRILL_M_ENDHEADER,
    DRILL_M_BEGINPATTERN,
    DRILL_M_ENDPATTERN,
    DRILL_M_CANNEDTEXT,
    DRILL_M_TIPCHECK,
    DRILL_M_METRIC,
    DRILL_M_IMPERIAL,
    DRILL_METRICHEADER,
    DRILL_IMPERIALHEADER,
    DRILL_DETECT_BROKEN,
    DRILL_INCREMENTALHEADER,
    DRILL_REWIND_STOP,
    DRILL_TOOL_CHANGE_STOP,
    DRILL_AUTOMATIC_SPEED,
    DRILL_AXIS_VERSION,
    DRILL_RESET_CMD,
    DRILL_AUTOMATIC_TOOL_CHANGE,
    DRILL_FMT,
    DRILL_SKIP,
    DRILL_TOOL_INFORMATION
};


enum drill_G_code_t {
    DRILL_G_UNKNOWN,
    DRILL_G_ABSOLUTE,
    DRILL_G_INCREMENTAL,
    DRILL_G_ZEROSET,
    DRILL_G_ROUT,
    DRILL_G_DRILL,
    DRILL_G_SLOT,
    DRILL_G_ZERO_SET,
    DRILL_G_LINEARMOVE,
    DRILL_G_CWMOVE,
    DRILL_G_CCWMOVE
};

// Helper struct to analyse Excellon commands
struct EXCELLON_CMD
{
    std::string m_Name;      // key string
    int    m_Code;      // internal code, used as id in functions
    int    m_asParams;  // 0 = no param, -1 = skip params, 1 = read params
};


/* EXCELLON_IMAGE handle a drill image
 *  It is derived from GERBER_IMAGE because there is a lot of likeness
 *  between EXCELLON files and GERBER files
 *  DCode aperture are also similat to T Codes.
 *  So we can reuse GERBER_IMAGE to handle EXCELLON_IMAGE with very few new functions
 */

class EXCELLON_IMAGE : public GERBER_IMAGE
{
private:
    enum excellon_state {
        READ_HEADER_STATE,          // When we are in this state, we are reading header
        READ_PROGRAM_STATE          // When we are in this state, we are reading drill data
    };
    excellon_state m_State;         // state of excellon file analysis
    bool           m_SlotOn;        // true during an oval driil definition

public: EXCELLON_IMAGE( GERBVIEW_FRAME* aParent, int layer ) :
        GERBER_IMAGE( aParent, layer )
    {
        m_State  = READ_HEADER_STATE;
        m_SlotOn = false;
    }


    ~EXCELLON_IMAGE() {};

    virtual void ResetDefaultValues()
    {
        GERBER_IMAGE::ResetDefaultValues();
        SelectUnits( false );
    }


    bool Read_EXCELLON_File( FILE* aFile, const wxString& aFullFileName );

private:
    bool Execute_HEADER_Command( char*& text );
    bool Select_Tool( char*& text );
    bool Execute_EXCELLON_G_Command( char*& text );
    bool Execute_Drill_Command( char*& text );

    int TCodeNumber( char*& Text )
    {
        return DCodeNumber( Text );
    }


    void SelectUnits( bool aMetric );
};


/*
 *  EXCELLON commands are given here.
 *  Pcbnew uses only few excellon commands
 */

/*
 *  see http://www.excellon.com/manuals/program.htm
 */

/* coordintes units:
 *  Coordinates are measured either in inch or metric (millimeters).
 *  Inch coordinates are in six digits (00.0000) with increments as small as 0.0001 (1/10,000).
 *  Metric coordinates can be measured in microns (thousandths of a millimeter)
 *  in one of the following three ways:
 *   Five digit 10 micron resolution (000.00)
 *   Six digit 10 micron resolution (0000.00)
 *   Six digit micron resolution (000.000)
 *
 *  Leading and trailing zeros:
 *  Excellon (CNC-7) uses inches in six digits and metric in five or six digits.
 *  The zeros to the left of the coordinate are called leading zeros (LZ).
 *  The zeros to right of the coordinate are called trailing zeros (TZ).
 *  The CNC-7 uses leading zeros unless you specify otherwise through a part program.
 *  You can do so with the INCH/METRIC command.
 *  With leading zeros, the leading zeros must always be included.
 *  Trailing zeros are unneeded and may be left off.
 *  For trailing zeros, the reverse of the above is true.
 */

/*
 *  EXCELLON Commands Used in a Header
 *  The following table provides you with a list of commands which
 *  are the most used in a part program header.
 *  COMMAND         DESCRIPTION
 *  AFS	            Automatic Feeds and Speeds
 *  ATC             Automatic Tool Change
 *  BLKD            Delete all Blocks starting with a slash (/)
 *  CCW             Clockwise or Counter-clockwise Routing
 *  CP              Cutter Compensation
 *  DETECT          Broken Tool Detection
 *  DN              Down Limit Set
 *  DTMDIST         Maximum Rout Distance Before Toolchange
 *  EXDA            Extended Drill Area
 *  FMAT            Format 1 or 2
 *  FSB             Turns the Feed/Speed Buttons off
 *  HPCK            Home Pulse Check
 *  ICI             Incremental Input of Part Program Coordinates
 *  INCH            Measure Everything in Inches
 *  METRIC          Measure Everything in Metric
 *  M48             Beginning of Part Program Header
 *  M95             End of Header
 *  NCSL            NC Slope Enable/Disable
 *  OM48            Override Part Program Header
 *  OSTOP           Optional Stop Switch
 *  OTCLMP          Override Table Clamp
 *  PCKPARAM        Set up pecking tool,depth,infeed and retract parameters
 *  PF              Floating Pressure Foot Switch
 *  PPR             Programmable Plunge Rate Enable
 *  PVS             Pre-vacuum Shut-off Switch
 *  R,C             Reset Clocks
 *  R,CP            Reset Program Clocks
 *  R,CR            Reset Run Clocks
 *  R,D             Reset All Cutter Distances
 *  R,H             Reset All Hit Counters
 *  R,T             Reset Tool Data
 *  SBK             Single Block Mode Switch
 *  SG              Spindle Group Mode
 *  SIXM            Input From External Source
 *  T               Tool Information
 *  TCST            Tool Change Stop
 *  UP              Upper Limit Set
 *  VER             Selection of X and Y Axis Version
 *  Z               Zero Set
 *  ZA              Auxiliary Zero
 *  ZC              Zero Correction
 *  ZS              Zero Preset
 *  Z+# or Z-#      Set Depth Offset
 *  %               Rewind Stop
 *  #/#/#           Link Tool for Automatic Tool Change
 *  /               Clear Tool Linking
 */

/*
 *  Beyond The Header: The Part Program Body
 *  COMMAND         DESCRIPTION
 *  A#              Arc Radius
 *  B#              Retract Rate
 *  C#              Tool Diameter
 *  F#              Table Feed Rate;Z Axis Infeed Rate
 *  G00X#Y#         Route Mode
 *  G01             Linear (Straight Line) Mode
 *  G02             Circular CW Mode
 *  G03             Circular CCW Mode
 *  G04	X#          Variable Dwell
 *  G05             Drill Mode
 *  G07             Override current tool feed or speed
 *  G32X#Y#A#       Routed Circle Canned Cycle
 *  CW G33X#Y#A#    Routed Circle Canned Cycle
 *  CCW G34,#(,#)   Select Vision Tool
 *  G35(X#Y#)       Single Point Vision Offset (Relative to Work Zero)
 *  G36(X#Y#)       Multipoint Vision Translation (Relative to Work Zero)
 *  G37             Cancel Vision Translation or Offset (From G35 or G36)
 *  G38(X#Y#)       Vision Corrected Single Hole Drilling (Relative to Work Zero)
 *  G39(X#Y#)       Vision System Autocalibration
 *  G40             Cutter Compensation Off
 *  G41             Cutter Compensation Left
 *  G42             Cutter Compensation Right
 *  G45(X#Y#)       Single Point Vision Offset (Relative to G35 or G36)
 *  G46(X#Y#)       Multipoint Vision Translation (Relative to G35 or G36)
 *  G47             Cancel Vision Translation or Offset (From G45 or G46)
 *  G48(X#Y#)       Vision Corrected Single Hole Drilling (Relative to G35 or G36)
 *  G82(G81)        Dual In Line Package
 *  G83             Eight Pin L Pack
 *  G84             Circle
 *  G85             Slot
 *  G87             Routed Step Slot Canned Cycle
 *  G90             Absolute Mode
 *  G91             Incremental Input Mode
 *  G93X#Y#         Zero Set
 *  H#              Maximum hit count
 *  I#J#            Arc Center Offset
 *  M00(X#Y#)       End of Program - No Rewind
 *  M01             End of Pattern
 *  M02X#Y#         Repeat Pattern Offset
 *  M06(X#Y#)       Optional Stop
 *  M08             End of Step and Repeat
 *  M09(X#Y#)       Stop for Inspection
 *  M14             Z Axis Route Position With Depth Controlled Contouring
 *  M15             Z Axis Route Position
 *  M16             Retract With Clamping
 *  M17             Retract Without Clamping
 *  M18             Command tool tip check
 *  M25             Beginning of Pattern
 *  M30(X#Y#)       End of Program Rewind
 *  M45,long message\   Long Operator message on multiple\ part program lines
 *  M47,text        Operator Message
 *  M50,#           Vision Step and Repeat Pattern Start
 *  M51,#           Vision Step and Repeat Rewind
 *  M52(#)          Vision Step and Repeat Offset Counter Control
 *  M02XYM70        Swap Axes
 *  M60             Reference Scaling enable
 *  M61             Reference Scaling disable
 *  M62             Turn on peck drilling
 *  M63             Turn off peck drilling
 *  M71             Metric Measuring Mode
 *  M72             Inch Measuring Mode
 *  M02XYM80        Mirror Image X Axis
 *  M02XYM90        Mirror Image Y Axis
 *  M97,text        Canned Text
 *  M98,text        Canned Text
 *  M99,subprogram  User Defined Stored Pattern
 *  P#X#(Y#)        Repeat Stored Pattern
 *  R#M02X#Y#       Repeat Pattern (S&R)
 *  R#(X#Y#)        Repeat Hole
 *  S#              Spindle RPM
 *  T#              Tool Selection; Cutter Index
 *  Z+# or Z-#      Depth Offset
 *  %               Beginning of Pattern (see M25 command)
 *  /               Block Delete
 */

/*
 *  Example of a Header
 *  COMMAND         PURPOSE
 *  M48             The beginning of a header
 *  INCH,LZ         Use the inch measuring system with leading zeros
 *  VER,1           Use Version 1 X and Y axis layout
 *  FMAT,2          Use Format 2 commands
 *  1/2/3           Link tools 1, 2, and 3
 *  T1C.04F200S65   Set Tool 1 for 0.040" with infeed rate of 200 inch/min Speed of 65,000 RPM
 *  DETECT,ON       Detect broken tools
 *  M95             End of the header
 */

#endif  // CLASS_EXCELLON_H

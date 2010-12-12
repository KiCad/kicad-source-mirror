/**
 * Common plot library \n
 * Plot settings, postscript plotting, gerber plotting.
 *
 * @file plot_common.h
 */

#ifndef __INCLUDE__PLOT_COMMON_H__
#define __INCLUDE__PLOT_COMMON_H__ 1

#include <vector>
#include "drawtxt.h"


class Ki_PageDescr;


/**
 * Enum PlotFormat
 * is the set of supported output plot formats.  They should be kept in order
 * of the radio buttons in the plot panel/windows.
 */
enum PlotFormat {
    PLOT_FORMAT_HPGL,
    PLOT_FORMAT_GERBER,
    PLOT_FORMAT_POST,
    PLOT_FORMAT_DXF
};

const int PLOT_MIROIR = 1;

class PLOTTER
{
public:
    PlotFormat m_PlotType;          // type of plot
public:
    PLOTTER( PlotFormat aPlotType );

    virtual ~PLOTTER()
    {
        // Emergency cleanup
        if( output_file )
        {
            fclose( output_file );
        }
    }


    /**
     * Function GetPlotterType
     * @return the format of the plot file
     */
    PlotFormat GetPlotterType()
    { return m_PlotType; }

    virtual bool start_plot( FILE* fout ) = 0;
    virtual bool end_plot() = 0;

    virtual void set_negative( bool _negative )
    {
        negative_mode = _negative;
    }


    virtual void set_color_mode( bool _color_mode )
    {
        color_mode = _color_mode;
    }


    bool get_color_mode() const
    {
        return color_mode;
    }


    virtual void set_paper_size( Ki_PageDescr* sheet );
    virtual void set_current_line_width( int width ) = 0;
    virtual void set_default_line_width( int width ) = 0;
    virtual void set_color( int color )  = 0;
    virtual void set_dash( bool dashed ) = 0;

    virtual void set_creator( const wxString& _creator )
    {
        creator = _creator;
    }


    virtual void set_filename( const wxString& _filename )
    {
        filename = _filename;
    }


    virtual void set_viewport( wxPoint offset,
                               double scale, int orient ) = 0;

    // Standard primitives
    virtual void rect( wxPoint p1, wxPoint p2, FILL_T fill,
                       int width = -1 ) = 0;
    virtual void circle( wxPoint pos, int diametre, FILL_T fill,
                         int width = -1 ) = 0;
    virtual void arc( wxPoint centre, int StAngle, int EndAngle, int rayon,
                      FILL_T fill, int width = -1 );
    virtual void poly( int nb_segm, int* coord, FILL_T fill,
                       int width = -1 ) = 0;
    virtual void thick_segment( wxPoint start, wxPoint end, int width,
                                GRTraceMode tracemode );
    virtual void thick_arc( wxPoint centre, int StAngle, int EndAngle, int rayon,
                            int width, GRTraceMode tracemode );
    virtual void thick_rect( wxPoint p1, wxPoint p2, int width,
                             GRTraceMode tracemode );
    virtual void thick_circle( wxPoint pos, int diametre, int width,
                               GRTraceMode tracemode );
    virtual void pen_to( wxPoint pos, char plume ) = 0;

    // Flash primitives
    virtual void flash_pad_circle( wxPoint pos, int diametre,
                                   GRTraceMode trace_mode ) = 0;
    virtual void flash_pad_oval( wxPoint pos, wxSize size, int orient,
                                 GRTraceMode trace_mode ) = 0;
    virtual void flash_pad_rect( wxPoint pos, wxSize size,
                                 int orient, GRTraceMode trace_mode ) = 0;
    /** virtual function flash_pad_trapez
     * flash a trapezoidal pad
     * @param aPadPos = the position of the shape
     * @param aCorners = the list of 4 corners positions, relative to the shape position, pad orientation 0
     * @param aPadOrient = the rotation of the shape
     * @param aTrace_Mode = FILLED or SKETCH
     */
    virtual void flash_pad_trapez( wxPoint aPadPos, wxPoint aCorners[4],
                                   int aPadOrient, GRTraceMode aTrace_Mode ) = 0;

    // Convenience functions
    void move_to( wxPoint pos )
    {
        pen_to( pos, 'U' );
    }


    void line_to( wxPoint pos )
    {
        pen_to( pos, 'D' );
    }


    void finish_to( wxPoint pos )
    {
        pen_to( pos, 'D' );
        pen_to( pos, 'Z' );
    }


    void pen_finish()
    {
        // Shortcut
        pen_to( wxPoint( 0, 0 ), 'Z' );
    }


    void text( const wxPoint&              aPos,
               enum EDA_Colors             aColor,
               const wxString&             aText,
               int                         aOrient,
               const wxSize&               aSize,
               enum GRTextHorizJustifyType aH_justify,
               enum GRTextVertJustifyType  aV_justify,
               int                         aWidth,
               bool                        aItalic,
               bool                        aBold );
    void           marker( const wxPoint& position, int diametre, int aShapeId );

    /**
     * Function SetLayerPolarity
     * sets current Gerber layer polarity to positive or negative
     * by writing \%LPD*\% or \%LPC*\% to the Gerber file, respectively.
     * @param aPositive is the layer polarity and true for positive.
     */
    virtual void SetLayerPolarity( bool aPositive ) = 0;

protected:
    // These are marker subcomponents
    void           center_square( const wxPoint& position, int diametre, FILL_T fill );
    void           center_lozenge( const wxPoint& position, int diametre, FILL_T fill );

    // Helper function for sketched filler segment
    void           segment_as_oval( wxPoint start, wxPoint end, int width,
                                    GRTraceMode tracemode );
    void           sketch_oval( wxPoint pos, wxSize size, int orient, int width );

    virtual void   user_to_device_coordinates( wxPoint& pos );
    virtual void   user_to_device_size( wxSize& size );
    virtual double user_to_device_size( double size );

    // Plot scale
    double        plot_scale;
    // Device scale (from decimils to device units)
    double        device_scale;
    // Plot offset (in decimils)
    wxPoint       plot_offset;
    // Output file
    FILE*         output_file;
    // Pen handling
    bool          color_mode, negative_mode;
    int           default_pen_width;
    int           current_pen_width;
    char          pen_state;
    wxPoint       pen_lastpos;
    // Other stuff
    int           plot_orient_options; // For now, mirror plot
    wxString      creator;
    wxString      filename;
    Ki_PageDescr* sheet;
    wxSize        paper_size;
};

class HPGL_PLOTTER : public PLOTTER
{
public:
    HPGL_PLOTTER() : PLOTTER(PLOT_FORMAT_HPGL)
    {
    }

    virtual bool start_plot( FILE* fout );
    virtual bool end_plot();

    // HPGL doesn't handle line thickness or color
    virtual void set_current_line_width( int width )
    {
        // Handy override
        current_pen_width = wxRound( pen_diameter );
    };
    virtual void set_default_line_width( int width ) {};
    virtual void set_dash( bool dashed );

    virtual void set_color( int color ) {};
    virtual void set_pen_speed( int speed )
    {
        wxASSERT( output_file == 0 );
        pen_speed = speed;
    }


    virtual void set_pen_number( int number )
    {
        wxASSERT( output_file == 0 );
        pen_number = number;
    }


    virtual void set_pen_diameter( double diameter )
    {
        wxASSERT( output_file == 0 );
        pen_diameter = diameter;
    }


    virtual void set_pen_overlap( double overlap )
    {
        wxASSERT( output_file == 0 );
        pen_overlap = overlap;
    }


    virtual void set_viewport( wxPoint offset,
                               double scale, int orient );
    virtual void rect( wxPoint p1, wxPoint p2, FILL_T fill, int width = -1 );
    virtual void circle( wxPoint pos, int diametre, FILL_T fill, int width = -1 );
    virtual void poly( int nb_segm, int* coord, FILL_T fill, int width = -1 );
    virtual void thick_segment( wxPoint start, wxPoint end, int width,
                                GRTraceMode tracemode );
    virtual void arc( wxPoint centre, int StAngle, int EndAngle, int rayon,
                      FILL_T fill, int width = -1 );
    virtual void pen_to( wxPoint pos, char plume );
    virtual void flash_pad_circle( wxPoint pos, int diametre,
                                   GRTraceMode trace_mode );
    virtual void flash_pad_oval( wxPoint pos, wxSize size, int orient,
                                 GRTraceMode trace_mode );
    virtual void flash_pad_rect( wxPoint pos, wxSize size,
                                 int orient, GRTraceMode trace_mode );
    virtual void flash_pad_trapez( wxPoint aPadPos, wxPoint aCorners[4],
                                   int aPadOrient, GRTraceMode aTrace_Mode );

    virtual void SetLayerPolarity( bool aPositive ) {}

protected:
    void         pen_control( int plume );

    int    pen_speed;
    int    pen_number;
    double pen_diameter;
    double pen_overlap;
};


class PS_PLOTTER : public PLOTTER
{
public:
    PS_PLOTTER() : PLOTTER(PLOT_FORMAT_POST)
    {
        plot_scale_adjX = 1;
        plot_scale_adjY = 1;
    }

    virtual bool start_plot( FILE* fout );
    virtual bool end_plot();
    virtual void set_current_line_width( int width );
    virtual void set_default_line_width( int width );
    virtual void set_dash( bool dashed );
    virtual void set_color( int color );

    void set_scale_adjust( double scaleX, double scaleY )
    {
        plot_scale_adjX = scaleX;
        plot_scale_adjY = scaleY;
    }


    virtual void set_viewport( wxPoint offset,
                               double scale, int orient );
    virtual void rect( wxPoint p1, wxPoint p2, FILL_T fill, int width = -1 );
    virtual void circle( wxPoint pos, int diametre, FILL_T fill, int width = -1 );
    virtual void arc( wxPoint centre, int StAngle, int EndAngle, int rayon,
                      FILL_T fill, int width = -1 );
    virtual void poly( int nb_segm, int* coord, FILL_T fill, int width = -1 );
    virtual void pen_to( wxPoint pos, char plume );
    virtual void flash_pad_circle( wxPoint pos, int diametre,
                                   GRTraceMode trace_mode );
    virtual void flash_pad_oval( wxPoint pos, wxSize size, int orient,
                                 GRTraceMode trace_mode );
    virtual void flash_pad_rect( wxPoint pos, wxSize size,
                                 int orient, GRTraceMode trace_mode );
    virtual void flash_pad_trapez( wxPoint aPadPos, wxPoint aCorners[4],
                                   int aPadOrient, GRTraceMode aTrace_Mode );

    virtual void SetLayerPolarity( bool aPositive ) {}

protected:
    double plot_scale_adjX, plot_scale_adjY;
};

/* Class to handle a D_CODE when plotting a board : */
#define FIRST_DCODE_VALUE 10    // D_CODE < 10 is a command, D_CODE >= 10 is a tool

struct APERTURE
{
    enum Aperture_Type {
        Circle   = 1,
        Rect     = 2,
        Plotting = 3,
        Oval     = 4
    };

    wxSize        size;     // horiz and Vert size
    Aperture_Type type;     // Type ( Line, rect , circulaire , ovale .. )
    int           D_code;   // code number ( >= 10 );

    /* Trivia question: WHY Gerber decided to use D instead of the usual T for
     *  tool change? */
};


class GERBER_PLOTTER : public PLOTTER
{
public:
    GERBER_PLOTTER() : PLOTTER(PLOT_FORMAT_GERBER)
    {
        work_file  = 0;
        final_file = 0;
        current_aperture = apertures.end();
    }


    virtual bool        start_plot( FILE* fout );
    virtual bool        end_plot();
    virtual void        set_current_line_width( int width );
    virtual void        set_default_line_width( int width );

    // RS274X has no dashing, nor colours
    virtual void        set_dash( bool dashed ) {}
    virtual void        set_color( int color ) {}
    virtual void        set_viewport( wxPoint offset,
                                      double scale, int orient );
    virtual void        rect( wxPoint p1, wxPoint p2, FILL_T fill, int width = -1 );
    virtual void        circle( wxPoint pos, int diametre, FILL_T fill, int width = -1 );
    virtual void        poly( int nb_segm, int* coord, FILL_T fill, int width = -1 );
    virtual void        pen_to( wxPoint pos, char plume );
    virtual void        flash_pad_circle( wxPoint pos, int diametre,
                                          GRTraceMode trace_mode );
    virtual void        flash_pad_oval( wxPoint pos, wxSize size, int orient,
                                        GRTraceMode trace_mode );
    virtual void        flash_pad_rect( wxPoint pos, wxSize size,
                                        int orient, GRTraceMode trace_mode );
    virtual void        flash_pad_trapez( wxPoint aPadPos, wxPoint aCorners[4],
                                          int aPadOrient, GRTraceMode aTrace_Mode );

    virtual void        SetLayerPolarity( bool aPositive );

protected:
    void                select_aperture( const wxSize&           size,
                                                     APERTURE::Aperture_Type type );

    std::vector<APERTURE>::iterator
                        get_aperture( const wxSize& size, APERTURE::Aperture_Type type );

    FILE* work_file, * final_file;
    wxString    m_workFilename;

    void                            write_aperture_list();

    std::vector<APERTURE>           apertures;
    std::vector<APERTURE>::iterator current_aperture;
};

class DXF_PLOTTER : public PLOTTER
{
public:
    DXF_PLOTTER() : PLOTTER(PLOT_FORMAT_DXF)
    {
    }

    virtual bool start_plot( FILE* fout );
    virtual bool end_plot();

    // For now we don't use 'thick' primitives, so no line width
    virtual void set_current_line_width( int width )
    {
        // Handy override
        current_pen_width = 0;
    }

    virtual void set_default_line_width( int width )
    {
        // DXF lines are infinitesimal
        default_pen_width = 0;
    }

    virtual void set_dash( bool dashed );

    virtual void set_color( int color );

    virtual void set_viewport( wxPoint offset,
                               double scale, int orient );
    virtual void rect( wxPoint p1, wxPoint p2, FILL_T fill, int width = -1 );
    virtual void circle( wxPoint pos, int diametre, FILL_T fill, int width = -1 );
    virtual void poly( int nb_segm, int* coord, FILL_T fill, int width = -1 );
    virtual void thick_segment( wxPoint start, wxPoint end, int width,
                                GRTraceMode tracemode );
    virtual void arc( wxPoint centre, int StAngle, int EndAngle, int rayon,
                      FILL_T fill, int width = -1 );
    virtual void pen_to( wxPoint pos, char plume );
    virtual void flash_pad_circle( wxPoint pos, int diametre,
                                   GRTraceMode trace_mode );
    virtual void flash_pad_oval( wxPoint pos, wxSize size, int orient,
                                 GRTraceMode trace_mode );
    virtual void flash_pad_rect( wxPoint pos, wxSize size,
                                 int orient, GRTraceMode trace_mode );
    virtual void flash_pad_trapez( wxPoint aPadPos, wxPoint aCorners[4],
                                   int aPadOrient, GRTraceMode aTrace_Mode );

    virtual void SetLayerPolarity( bool aPositive ) {}

protected:
    int current_color;
};

#endif  // __INCLUDE__PLOT_COMMON_H__

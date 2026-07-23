
                            This is gd 2.0.1 BETA.
                                       
   _If you have problems, report them in detail, and consider using gd
   1.8.4 until gd 2.0 final is out._
   
   The gd 2.0 documentation update is not complete, but most new features
   are documented to some degree and the what's new section is reasonably
   complete. Enjoy!
   
gd 2.0.1

  A graphics library for fast image creation
  
  Follow this link to the latest version of this document.
  
     _HEY! READ THIS!_ gd 2.0.1 creates PNG, JPEG and WBMP images, not
     GIF images. This is a good thing. PNG is a more compact format, and
     full compression is available. JPEG works well with photographic
     images, and is still more compatible with the major Web browsers
     than even PNG is. WBMP is intended for wireless devices (not
     regular web browsers). Existing code will need modification to call
     gdImagePng or gdImageJpeg instead of gdImageGif. _Please do not ask
     us to send you the old GIF version of GD._ Unisys holds a patent on
     the LZW compression algorithm, which is used in fully compressed
     GIF images. The best solution is to move to legally unencumbered,
     well-compressed, modern image formats such as PNG and JPEG as soon
     as possible.
     
     gd 2.0.1 _requires_ that the following libraries also be installed:
     
     libpng (see the libpng home page)
     
     zlib (see the info-zip home page) zlib
     
     jpeg-6b or later, if desired (see the Independent JPEG Group home
     page)
     
     If you want to use the TrueType font support, you must also install
     the _FreeType 2.x library_, including the header files. See the
     Freetype Home Page, or SourceForge. No, I cannot explain why that
     site is down on a particular day, and no, I can't send you a copy.
     
     If you want to use the Xpm color bitmap loading support, you must
     also have the X Window System and the Xpm library installed (Xpm is
     often included in modern X distributions).
     
     Please read the documentation and install the required libraries.
     Do not send email asking why png.h is not found. Do not send email
     asking why libgd.so is not found, either. See the requirements
     section for more information. Thank you!
     
  Table of Contents
  
     * Credits and license terms
     * What's new in version "XYZ" of GD?
     * What is gd?
     * What if I want to use another programming language?
     * What else do I need to use gd?
     * How do I get gd?
     * How do I build gd?
     * gd basics: using gd in your program
     * webpng: a useful example
     * Function and type reference by category
     * About the additional .gd image file format
     * Please tell us you're using gd!
     * If you have problems
     * Alphabetical quick index
       
   Up to the Boutell.Com, Inc. Home Page
   
  Credits and license terms
  
   In order to resolve any possible confusion regarding the authorship of
   gd, the following copyright statement covers all of the authors who
   have required such a statement. _If you are aware of any oversights in
   this copyright notice, please contact Thomas Boutell who will be
   pleased to correct them._

COPYRIGHT STATEMENT FOLLOWS THIS LINE

     Portions copyright 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001
     by Cold Spring Harbor Laboratory. Funded under Grant P41-RR02188 by
     the National Institutes of Health.
     
     Portions copyright 1996, 1997, 1998, 1999, 2000, 2001 by
     Boutell.Com, Inc.
     
     Portions relating to GD2 format copyright 1999, 2000 Philip Warner.
     
     Portions relating to PNG copyright 1999, 2000 Greg Roelofs.
     
     Portions relating to libttf copyright 1999, 2000 John Ellson
     (ellson@lucent.com).
     
     Portions relating to JPEG and to color quantization copyright 2000,
     Doug Becker and copyright (C) 1994-1998, Thomas G. Lane. This
     software is based in part on the work of the Independent JPEG
     Group. See the file README-JPEG.TXT for more information.
     
     Portions relating to WBMP copyright 2000 Maurice Szmurlo and Johan
     Van den Brande.
     
     _Permission has been granted to copy, distribute and modify gd in
     any context without fee, including a commercial application,
     provided that this notice is present in user-accessible supporting
     documentation._
     
     This does not affect your ownership of the derived work itself, and
     the intent is to assure proper credit for the authors of gd, not to
     interfere with your productive use of gd. If you have questions,
     ask. "Derived works" includes all programs that utilize the
     library. Credit must be given in user-accessible documentation.
     
     _This software is provided "AS IS."_ The copyright holders disclaim
     all warranties, either express or implied, including but not
     limited to implied warranties of merchantability and fitness for a
     particular purpose, with respect to this code and accompanying
     documentation.
     
     Although their code does not appear in gd 2.0.1, the authors wish
     to thank David Koblas, David Rowley, and Hutchison Avenue Software
     Corporation for their prior contributions.
     
END OF COPYRIGHT STATEMENT

  What is gd?
  
   gd is a graphics library. It allows your code to quickly draw images
   complete with lines, arcs, text, multiple colors, cut and paste from
   other images, and flood fills, and write out the result as a PNG or
   JPEG file. This is particularly useful in World Wide Web applications,
   where PNG and JPEG are two of the formats accepted for inline images
   by most browsers.
   
   gd is not a paint program. If you are looking for a paint program, you
   are looking in the wrong place. If you are not a programmer, you are
   looking in the wrong place, unless you are installing a required
   library in order to run an application.
   
   gd does not provide for every possible desirable graphics operation.
   It is not necessary or desirable for gd to become a kitchen-sink
   graphics package, but version 2.0 does include most frequently
   requested features, including both truecolor and palette images,
   resampling (smooth resizing of truecolor images) and so forth.
   
  What if I want to use another programming language?
  
   Not all of these tools are necessarily up to date and fully compatible
   with 2.0.1.
   
    Perl
    
   gd can also be used from Perl, courtesy of Lincoln Stein's GD.pm
   library, which uses gd as the basis for a set of Perl 5.x classes.
   Highly recommended.
   
    Tcl
    
   gd can be used from Tcl with John Ellson's Gdtclft dynamically loaded
   extension package. (Gdtclft2.0 or later is needed for gd-1.6 and up
   with PNG output.)
   
    Pascal
    
   Pascal enthusiasts should look into Michael Bradbury's gdfp package.
   
    Haskell
    
   A new gd interface is now available for Haskell programmers.
   
    REXX
    
   A gd interface for the REXX language is available.
   
    Any Language
    
   There are, at the moment, at least three simple interpreters that
   perform gd operations. You can output the desired commands to a simple
   text file from whatever scripting language you prefer to use, then
   invoke the interpreter.
   
     * tgd, by Bradley K. Sherman
     * fly, by Martin Gleeson
       
  What's new in version 2.0.1?
  
     * Workaround for a bug in gcc, apparently found in gcc 2.7.2 and up.
       I reproduced and fixed it while using gcc 2.9.5.2. The bug
       occurred only when the -g option was in use. This problem caused
       gcc to spew internal error messages unrelated to the correctness
       of the code in gd_gd2.c. Howard Jones was first to report it.
     * gdImageFilledEllipse documented and altered; no longer requires a
       superfluous style argument. Thanks to Francis James Franklin.
     * The Makefile now offers the correct syntax for optionally creating
       a static library. Thanks to Jean-Lous Regez, among others.
     * A nested comment, an attempt to return the value of a void
       function, and a potentially significant error in
       gdImageCopyResampled were fixed thanks to Joseph Shirley.
     * A bug preventing proper truecolor text rendering was fixed, thanks
       to Jason Gallagher.
     * gdImageStringFT (FreeType) should now work better against a
       transparent or semitransparent background, and should act in a
       manner consistent with the most recent gdImageAlphaBlending
       setting. Antialiasing is now done via the alpha channel mechanism
       if the image is a truecolor image.
     * Bugs in the output of gdImageArc and gdImageFilledArc were
       reported by Bruce Verderaime. A simple and correct but inefficient
       implementation has been substituted until fixes are contributed
       for the faster code, which is in gd_arc_f_buggy.c along with the
       test program that reproduces the bug(s).
     * gdImageFilledArc now offers additional style options, which can be
       combined to produce various effects.
     * Masahito Yamaga (ma@yama-ga.com) sent a patch to improve support
       for Japanese output via gdImageStringFT. He also added a new
       readme.jpn file.
     * Zillions of documentation fixes.
       
  What's new in version 2.0?
  
     * _Support for truecolor images!_ Version 2.0 can load truecolor
       PNGs with no loss of color information, and almost no loss of
       alpha channel information. Version 2.0 can also load truecolor
       JPEGs with as little loss as possible; however, bear in mind that
       JPEG is a lossy format, so repeated load/save cycles always reduce
       image quality. This is not a bug. To create a truecolor image from
       scratch, call the new gdImageCreateTrueColor function. The
       gdImageCreate function is still available to create palette
       images, and may also be referred to as gdImageCreatePalette.
     * _Support for alpha channels!_ In addition to 24 bits of color
       information for each pixel (eight bits of red, green, and blue
       respectively), version 2.0 supports 7 bits of "alpha channel"
       information. This is used to determine exactly how transparent the
       pixel should be. There is also support for a full 7 bits of
       transparency for each individual palette index in a palette-based
       image. Please note that, as of this writing, only Macintosh
       Internet Explorer 5.x and Mozilla/Netscape 6.x display partial
       transparency properly.
     * The new gdImageAlphaBlending function allows for two different
       modes of drawing. In blending mode, the alpha channel component of
       the color supplied to all drawing functions, such as
       gdImageSetPixel, determines how much of the underlying color
       should be allowed to shine through. The resulting image is not
       transparent. In non-blending mode, drawing color is copied
       literally with the alpha channel information, resulting in a
       transparent image. Blending mode is not available when drawing on
       palette images.
     * The gdImageCopyResampled function provides "smooth" copying from a
       large image to a smaller one, using a weighted average of the
       pixels of the source area rather than selecting one representative
       pixel. This function is identical to gdImageCopyResized when the
       destination image is a palette image.
     * The gdImageTrueColorToPalette function converts a truecolor image
       to a palette image. The code for this function was originally
       drawn from the Independent JPEG Group library code, which is
       excellent. The code has been modified to preserve as much alpha
       channel information as possible in the resulting palette, in
       addition to preserving colors as well as possible. This does not
       work as well as might be hoped. It is usually best to simply
       produce a truecolor output image instead, which guarantees the
       highest output quality.
     * A very high degree of backwards compatibility with existing gd 1.x
       code has been maintained, at both the source code and binary
       level. _Code which directly accesses the pixels array will fail
       only if it encounters an existing truecolor image_, which may
       happen if the code attempts to open and modify an existing JPEG or
       truecolor PNG. Such code should be modified to check the trueColor
       flag of the gdImage structure, and refer to the tpixels array
       instead when it is set.
     * gd is now compiled and installed as a shared library. However, gd
       still does not use autoconf, because I (TBB) have very limited
       patience with autoconf. These days, most Unix systems provide a
       fairly POSIX-standard environment, and the provided Makefile is
       likely to work well if users read it and follow the instructions
       at the top.
     * Support for line thickness was added by Michael Schwartz. My
       apologies to him for sitting on his patches for so long. See the
       new gdImageSetThickness function, which affects all standard gd
       functions that draw lines and curves. In addition, Michael added a
       convenient gdImageEllipse function.
     * The new gdImageFilledArc function provides a straightforward way
       to draw filled arcs. Also, gdImageFilledEllipse is a convenient
       way to fill an ellipse without specifying starting and ending
       angles. Thanks go out to F J Franklin.
     * To put an end to the confusion, TrueType 1.x support has been
       removed in favor of TrueType 2.x support. The old gdImageStringTTF
       function simply invokes gdImageStringFT.
     * The specialized .gd and .gd2 file formats have been upgraded to
       support truecolor. New images written by the versions of these
       functions found in 2.0 will be rejected, with varying degrees of
       grace, by older versions of gd. THIS AFFECTS THE .GD and .GD2
       FORMATS ONLY. IF YOU ARE CONFUSED BY THIS PARAGRAPH, IT PROBABLY
       DOESN'T APPLY TO ANYTHING YOU WILL EVER ENCOUNTER. Since these
       file formats are absolutely, positively *not* designed for
       distributing images, just for preprocessing them, this should not
       be a big problem. gd 2.0 should read old .gd and .gd2 files
       correctly.
       
  What's new in version 1.8.4?
  
     * Add support for FreeType2 (John Ellson ellson@lucent.com)
     * Add support for finding in fonts in a builtin DEFAULT_FONTPATH, or
       in a path from the GDFONTPATH environment variable.
     * remove some unused symbols to reduce compiler warnings
     * bugfix in size comparisons in gdImageCompare
     * REXX now mentioned
     * All memory allocation functions are now wrapped within the
       library; gdFree is exported and recommended for freeing memory
       returned by the gdImage(Something)Ptr family of functions.
       
  What's new in version 1.8.3?
  
     * WBMP output memory leak fixed
     * #include <gd.h> corrected to #include "gd.h" in gd_wbmp.c
     * Documented the fact that the source and output images shouldn't
       match in the WBMP test except for black and white source images
       
  What's new in version 1.8.2?
  
     * WBMP support debugged and improved by Johann Van den Brande
     * WBMP tests added to gdtest.c by Thomas Boutell
     * Use of platform-dependent 'install' command removed by Thomas
       Boutell
     * Comments added to Makefile warning users to juggle the order of
       the libraries if the linker complains; is there any portable way
       to do this automatically, short of using autoconf?
     * Documentation of gdImageCreateFromXpm corrected
     * Updated links to fast-moving, always dodging libpng and zlib web
       sites
       
  What's new in version 1.8.1?
  
     * Optional components no longer built by default (following the
       documentation)
     * JPEG code no longer requires inappropriate header files
     * Win32 patches from Joe Gregorio
     * 16-bit font support for bdftogd, from Honza Pazdziora
       
  What's new in version 1.8?
  
     * Support for JPEG output, courtesy of Doug Becker
     * A link to Michael Bradbery's Pascal wrapper
     * Support for WBMP output, courtesy of Maurice Szmurlo
     * gdImageColorClosestHWB function based on hue, whiteness,
       blackness, superior to the regular gdImageColorClosest function,
       courtesy of Philip Warner
     * License clarification: yes, you can modify gd
       
    Additional JPEG Information
    
   Support for reading and writing JPEG-format images is courtesy of Doug
   Becker and the Independent JPEG Group / Thomas G. Lane. You can get
   the latest version of the IJG JPEG software from
   ftp://ftp.uu.net/graphics/jpeg/ (e.g., the jpegsrc.v6b.tar.gz file).
   You _must_ use version 6b or later of the IJG JPEG software. You might
   also consult the JPEG FAQ at http://www.faqs.org/faqs/jpeg-faq/.
   
  What's new in version 1.7.3?
  
   Another attempt at Makefile fixes to permit linking with all libraries
   required on platforms with order- dependent linkers. Perhaps it will
   work this time.
   
  What's new in version 1.7.2?
  
   An uninitialized-pointer bug in gdtestttf.c was corrected. This bug
   caused crashes at the end of each call to gdImageStringTTF on some
   platforms. Thanks to Wolfgang Haefelinger.
   
   Documentation fixes. Thanks to Dohn Arms.
   
   Makefile fixes to permit linking with all libraries required on
   platforms with order- dependent linkers.
   
  What's new in version 1.7.1?
  
   A minor buglet in the Makefile was corrected, as well as an inaccurate
   error message in gdtestttf.c. Thanks to Masahito Yamaga.
   
  What's new in version 1.7?
  
   Version 1.7 contains the following changes:
     * Japanese language support for the TrueType functions. Thanks to
       Masahito Yamaga.
     * autoconf and configure have been removed, in favor of a carefully
       designed Makefile which produces and properly installs the library
       and the binaries. System-dependent variables are at the top of the
       Makefile for easy modification. I'm sorry, folks, but autoconf
       generated _many, many confused email messages_ from people who
       didn't have things where autoconf expected to find them. I am not
       an autoconf/automake wizard, and gd is a simple, very compact
       library which does not need to be a shared library. I _did_ make
       many improvements over the old gd 1.3 Makefile, which were
       directly inspired by the autoconf version found in the 1.6 series
       (thanks to John Ellson).
     * Completely ANSI C compliant, according to the -pedantic-errors
       flag of gcc. Several pieces of not-quite-ANSI-C code were causing
       problems for those with non-gcc compilers.
     * gdttf.c patched to allow the use of Windows symbol fonts, when
       present (thanks to Joseph Peppin).
     * extern "C" wrappers added to gd.h and the font header files for
       the convenience of C++ programmers. bdftogd was also modified to
       automatically insert these wrappers into future font header files.
       Thanks to John Lindal.
     * Compiles correctly on platforms that don't define SEEK_SET. Thanks
       to Robert Bonomi.
     * Loads Xpm images via the gdImageCreateFromXpm function, if the Xpm
       library is available. Thanks to Caolan McNamara.
       
  What's new in version 1.6.3?
  
   Version 1.6.3 corrects a memory leak in gd_png.c. This leak caused a
   significant amount of memory to be allocated and not freed when
   writing a PNG image.
   
  What's new in version 1.6.2?
  
   Version 1.6.2 from John Ellson adds two new functions:
     * gdImageStringTTF - scalable, rotatable, anti-aliased, TrueType
       strings using the FreeType library, but only if libttf is found by
       configure. _We do not provide TrueType fonts. Obtaining them is
       entirely up to you._
     * gdImageColorResolve - an efficient alternative for the common code
       fragment:


      if ((color=gdImageColorExact(im,R,G,B)) < 0)
          if ((color=gdImageColorAllocate(im,R,G,B)) < 0)
              color=gdImageColorClosest(im,R,G,B);

   Also in this release the build process has been converted to GNU
   autoconf/automake/libtool conventions so that both (or either) static
   and shared libraries can be built.
   
  What's new in version 1.6.1?
  
   Version 1.6.1 incorporates superior PNG reading and writing code from
   Greg Roelofs, with minor modifications by Tom Boutell. Specifically, I
   altered his code to read non-palette images (converting them to
   palette images badly, by dithering them), and to tolerate palette
   images with types of transparency that gd doesn't actually support (it
   just ignores the advanced transparency features). Any bugs in this
   area are therefore my fault, not Greg's.
   
   Unlike gd 1.6, users should have no trouble linking with gd 1.6.1 if
   they follow the instructions and install all of the pieces. However,
   _If you get undefined symbol errors, be sure to check for older
   versions of libpng in your library directories!_
   
  What's new in version 1.6?
  
   Version 1.6 features the following changes:
   
   _Support for 8-bit palette PNG images has been added. Support for GIF
   has been removed._ This step was taken to completely avoid the legal
   controversy regarding the LZW compression algorithm used in GIF.
   Unisys holds a patent which is relevant to LZW compression. PNG is a
   superior image format in any case. Now that PNG is supported by both
   Microsoft Internet Explorer and Netscape (in their recent releases),
   we highly recommend that GD users upgrade in order to get
   well-compressed images in a format which is legally unemcumbered.
   
  What's new in version 1.5?
  
   Version 1.5 featured the following changes:
   
   _New GD2 format_
          An improvement over the GD format, the GD2 format uses the zlib
          compression library to compress the image in chunks. This
          results in file sizes comparable to GIFs, with the ability to
          access parts of large images without having to read the entire
          image into memory.
          
          This format also supports version numbers and rudimentary
          validity checks, so it should be more 'supportable' than the
          previous GD format.
          
   _Re-arranged source files_
          gd.c has been broken into constituant parts: io, gif, gd, gd2
          and graphics functions are now in separate files.
          
   _Extended I/O capabilities._
          The source/sink feature has been extended to support GD2 file
          formats (which require seek/tell functions), and to allow more
          general non-file I/O.
          
   _Better support for Lincoln Stein's Perl Module_
          The new gdImage*Ptr function returns the chosen format stored
          in a block of memory. This can be directly used by the GD perl
          module.
          
   _Added functions_
          gdImageCreateFromGd2Part - allows retrieval of part of an image
          (good for huge images, like maps),
          gdImagePaletteCopy - Copies a palette from one image to
          another, doing it's best to match the colors in the target
          image to the colors in the source palette.
          gdImageGd2, gdImageCreateFromGd2 - Support for new format
          gdImageCopyMerge - Merges two images (useful to highlight part
          of an image)
          gdImageCopyMergeGray - Similar to gdImageCopyMerge, but tries
          to preserve source image hue.
          gdImagePngPtr, gdImageJpegPtr, gdImageWBMPPtr, gdImageGdPtr,
          gdImageGd2Ptr - return memory blocks for each type of image.
          gdImageCreateFromPngCtx, gdImageCreateFromGdCtx,
          gdImageCreateFromGd2Ctx, gdImageCreateFromGd2PartCtx - Support
          for new I/O context.
          
   _NOTE:_ In fairness to Thomas Boutell, any bug/problems with any of
   the above features should probably be reported to Philip Warner.
   
  What's new in version 1.4?
  
   Version 1.4 features the following changes:
   
   Fixed polygon fill routine (again)
          Thanks to Kirsten Schulz, version 1.4 is able to fill numerous
          types of polygons that caused problems with previous releases,
          including version 1.3.
          
   Support for alternate data sources
          Programmers who wish to load a GIF from something other than a
          stdio FILE * stream can use the new gdImageCreateFromPngSource
          function.
          
   Support for alternate data destinations
          Programmers who wish to write a GIF to something other than a
          stdio FILE * stream can use the new gdImagePngToSink function.
          
   More tolerant when reading GIFs
          Version 1.4 does not crash when reading certain animated GIFs,
          although it still only reads the first frame. Version 1.4 also
          has overflow testing code to prevent crashes when reading
          damaged GIFs.
          
  What's new in version 1.3?
  
   Version 1.3 features the following changes:
   
   Non-LZW-based GIF compression code
          Version 1.3 contained GIF compression code that uses simple Run
          Length Encoding instead of LZW compression, while still
          retaining compatibility with normal LZW-based GIF decoders
          (your browser will still like your GIFs). _LZW compression is
          patented by Unisys. We are currently reevaluating the approach
          taken by gd 1.3. The current release of gd does not support
          this approach. We recommend that you use the current release,
          and generate PNG images._ Thanks to Hutchison Avenue Software
          Corporation for contributing the RLE GIF code.
          
   8-bit fonts, and 8-bit font support
          This improves support for European languages. Thanks are due to
          Honza Pazdziora and also to Jan Pazdziora . Also see the
          provided bdftogd Perl script if you wish to convert fixed-width
          X11 fonts to gd fonts.
          
   16-bit font support (no fonts provided)
          Although no such fonts are provided in the distribution, fonts
          containing more than 256 characters should work if the
          gdImageString16 and gdImageStringUp16 routines are used.
          
   Improvements to the "webpng" example/utility
          The "webpng" utility is now a slightly more useful application.
          Thanks to Brian Dowling for this code.
          
   Corrections to the color resolution field of GIF output
          Thanks to Bruno Aureli.
          
   Fixed polygon fills
          A one-line patch for the infamous polygon fill bug, courtesy of
          Jim Mason. I believe this fix is sufficient. However, if you
          find a situation where polygon fills still fail to behave
          properly, please send code that demonstrates the problem, _and_
          a fix if you have one. Verifying the fix is important.
          
   Row-major, not column-major
          Internally, gd now represents the array of pixels as an array
          of rows of pixels, rather than an array of columns of pixels.
          This improves the performance of compression and decompression
          routines slightly, because horizontally adjacent pixels are now
          next to each other in memory. _This should not affect properly
          written gd applications, but applications that directly
          manipulate the pixels array will require changes._
          
  What else do I need to use gd?
  
   To use gd, you will need an ANSI C compiler. _All popular Windows 95
   and NT C compilers are ANSI C compliant._ Any full-ANSI-standard C
   compiler should be adequate. _The cc compiler released with SunOS
   4.1.3 is not an ANSI C compiler. Most Unix users who do not already
   have gcc should get it. gcc is free, ANSI compliant and a de facto
   industry standard. Ask your ISP why it is missing._
   
   As of version 1.6, you also need the zlib compression library, and the
   libpng library. As of version 1.6.2, you can draw text using
   antialiased TrueType fonts if you also have the libttf library
   installed, but this is not mandatory. zlib is available for a variety
   of platforms from the zlib web site. libpng is available for a variety
   of platforms from the PNG web site.
   
   You will also want a PNG viewer, if you do not already have one for
   your system, since you will need a good way to check the results of
   your work. Netscape 4.04 and higher, and Microsoft Internet Explorer
   4.0 or higher, both support PNG. For some purposes you might be
   happier with a package like Lview Pro for Windows or xv for X. There
   are PNG viewers available for every graphics-capable modern operating
   system, so consult newsgroups relevant to your particular system.
   
  How do I get gd?
  
    By HTTP
    
     * Gzipped Tar File (Unix)
     * .ZIP File (Windows)
       
    By FTP
    
     * Gzipped Tar File (Unix)
     * .ZIP File (Windows)
       
  How do I build gd?
  
   In order to build gd, you must first unpack the archive you have
   downloaded. If you are not familiar with tar and gunzip (Unix) or ZIP
   (Windows), please consult with an experienced user of your system.
   Sorry, we cannot answer questions about basic Internet skills.
   
   Unpacking the archive will produce a directory called "gd-2.0.1".
   
    For Unix
    
   cd to the 2.0.1 directory. Edit the Makefile with your preferred text
   editor and make any necessary changes to the settings at the top,
   especially if you want Xpm or TrueType support. Next, type "make
   install". Because gd 2.0 and above installs as a shared library, it is
   necessary to install the library properly before running gd-based
   programs.
   
   If you get errors, edit the Makefile again, paying special attention
   to the INCLUDEDIRS and LIBDIRS settings.
   
   IF YOU GET LINKER ERRORS, TRY JUGGLING THE ORDER OF THE -l DIRECTIVES
   IN THE MAKEFILE. Some platforms may prefer that the libraries be
   listed in the opposite order.
   
    For Windows, Mac, Et Cetera
    
   Create a project using your favorite programming environment. Copy all
   of the gd files to the project directory. Add gd.c to your project.
   Add other source files as appropriate. Learning the basic skills of
   creating projects with your chosen C environment is up to you.
   
   If you wish to test the library, type "make test" AFTER you have
   successfully executed "make install". This will build several test
   programs, including "gddemo". Run gddemo to see some of the
   capabilities of gd.
   
   gddemo should execute without incident, creating the file demoout.png.
   (Note there is also a file named demoin.png, which is provided in the
   package as part of the demonstration.)
   
   Display demoout.png in your PNG viewer. The image should be 128x128
   pixels and should contain an image of the space shuttle with quite a
   lot of graphical elements drawn on top of it.
   
   (If you are missing the demoin.png file, the other items should appear
   anyway.)
   
   Look at demoin.png to see the original space shuttle image which was
   scaled and copied into the output image.
   
  gd basics: using gd in your program
  
   gd lets you create PNG or JPEG images on the fly. To use gd in your
   program, include the file gd.h, and link with the libgd.a library
   produced by "make libgd.a", under Unix. Under other operating systems
   you will add gd.c to your own project.
   
   If you want to use the provided fonts, include gdfontt.h, gdfonts.h,
   gdfontmb.h, gdfontl.h and/or gdfontg.h. For more impressive results,
   install FreeType 2.x and use the new gdImageStringFT function. If you
   are not using the provided Makefile and/or a library-based approach,
   be sure to include the source modules as well in your project. (They
   may be too large for 16-bit memory models, that is, 16-bit DOS and
   Windows.)
   
   Here is a short example program. _(For a more advanced example, see
   gddemo.c, included in the distribution. gddemo.c is NOT the same
   program; it demonstrates additional features!)_
   
/* Bring in gd library functions */
#include "gd.h"

/* Bring in standard I/O so we can output the PNG to a file */
#include <stdio.h>

int main() {
        /* Declare the image */
        gdImagePtr im;
        /* Declare output files */
        FILE *pngout, *jpegout;
        /* Declare color indexes */
        int black;
        int white;

        /* Allocate the image: 64 pixels across by 64 pixels tall */
        im = gdImageCreate(64, 64);

        /* Allocate the color black (red, green and blue all minimum).
                Since this is the first color in a new image, it will
                be the background color. */
        black = gdImageColorAllocate(im, 0, 0, 0);

        /* Allocate the color white (red, green and blue all maximum). */
        white = gdImageColorAllocate(im, 255, 255, 255);
        
        /* Draw a line from the upper left to the lower right,
                using white color index. */
        gdImageLine(im, 0, 0, 63, 63, white);

        /* Open a file for writing. "wb" means "write binary", important
                under MSDOS, harmless under Unix. */
        pngout = fopen("test.png", "wb");

        /* Do the same for a JPEG-format file. */
        jpegout = fopen("test.jpg", "wb");

        /* Output the image to the disk file in PNG format. */
        gdImagePng(im, pngout);

        /* Output the same image in JPEG format, using the default
                JPEG quality setting. */
        gdImageJpeg(im, jpegout, -1);

        /* Close the files. */
        fclose(pngout);
        fclose(jpegout);

        /* Destroy the image in memory. */
        gdImageDestroy(im);
}

   When executed, this program creates an image, allocates two colors
   (the first color allocated becomes the background color), draws a
   diagonal line (note that 0, 0 is the upper left corner), writes the
   image to PNG and JPEG files, and destroys the image.
   
   The above example program should give you an idea of how the package
   works. gd provides many additional functions, which are listed in the
   following reference chapters, complete with code snippets
   demonstrating each. There is also an alphabetical index.
   
  Webpng: a more powerful gd example
  
   Webpng is a simple utility program to manipulate PNGs from the command
   line. It is written for Unix and similar command-line systems, but
   should be easily adapted for other environments. Webpng allows you to
   set transparency and interlacing and output interesting information
   about the PNG in question.
   
   webpng.c is provided in the distribution. Unix users can simply type
   "make webpng" to compile the program. Type "webpng" with no arguments
   to see the available options.
   
Function and type reference

     * Types
     * Image creation, destruction, loading and saving
     * Drawing, styling, brushing, tiling and filling functions
     * Query functions (not color-related)
     * Font and text-handling functions
     * Color handling functions
     * Copying and resizing functions
     * Miscellaneous Functions
     * Constants
       
  Types
  
   gdImage_(TYPE)_
          The data structure in which gd stores images. gdImageCreate,
          gdImageCreateTrueColor and the various image file-loading
          functions return a pointer to this type, and the other
          functions expect to receive a pointer to this type as their
          first argument. It is reasonably safe to examine any of the
          members of this structure. It is also reasonably safe to modify
          individual pixels within the pixels or tpixels arrays. If the
          trueColor flag is set, the tpixels array is valid; otherwise
          the pixels array is valid.
          
          The colorsTotal, red, green, blue, alpha and open arrays manage
          the palette. They are valid only when the trueColor flag is not
          set. The transparent value contains the palette index of the
          first transparent color as read-only information for backwards
          compatibility; gd 2.0 stores this information in the alpha
          array so that variable transparency can be supported for each
          palette entry. However, for truecolor images, transparent
          represents a single RGB color which is _always 100%
          transparent_, and this feature is generally supported by
          browsers which do not support full alpha channels.
          

typedef struct {
        /* Palette-based image pixels */
        unsigned char ** pixels;
        int sx;
        int sy;
        /* These are valid in palette images only. See also
        /* 'alpha', which appears later in the structure to
                preserve binary backwards compatibility */
        int colorsTotal;
        int red[gdMaxColors];
        int green[gdMaxColors];
        int blue[gdMaxColors];
        int open[gdMaxColors];
        /* For backwards compatibility, this is set to the
                first palette entry with 100% transparency,
                and is also set and reset by the
                gdImageColorTransparent function. Newer
                applications can allocate palette entries
                with any desired level of transparency; however,
                bear in mind that many viewers, notably
                many web browsers, fail to implement
                full alpha channel for PNG and provide
                support for full opacity or transparency only. */
        int transparent;
        int *polyInts;
        int polyAllocated;
        struct gdImageStruct *brush;
        struct gdImageStruct *tile;
        int brushColorMap[gdMaxColors];
        int tileColorMap[gdMaxColors];
        int styleLength;
        int stylePos;
        int *style;
        int interlace;
        /* New in 2.0: alpha channel for palettes. Note that only
                Macintosh Internet Explorer and (possibly) Netscape 6
                really support multiple levels of transparency in
                palettes, to my knowledge, as of 2/15/01. Most
                common browsers will display 100% opaque and
                100% transparent correctly, and do something
                unpredictable and/or undesirable for levels
                in between. TBB */
        int alpha[gdMaxColors];
        /* Truecolor flag and pixels. New 2.0 fields appear here at the
                end to minimize breakage of existing object code. */
        int trueColor;
        int ** tpixels;
        /* Should alpha channel be copied, or applied, each time a
                pixel is drawn? This applies to truecolor images only.
                No attempt is made to alpha-blend in palette images,
                even if semitransparent palette entries exist.
                To do that, build your image as a truecolor image,
                then quantize down to 8 bits. */
        int alphaBlendingFlag;
        /* Should the alpha channel of the image be saved? This affects
                PNG at the moment; other future formats may also
                have that capability. JPEG doesn't. */
        int saveAlphaFlag;
} gdImage;

          The order of the structure members may appear confusing, but
          was chosen deliberately to increase backwards compatibility
          with existing gd 1.x-based binary code that references
          particular structure members.
          
   gdImagePtr _(TYPE)_
          A pointer to an image structure. gdImageCreate returns this
          type, and the other functions expect it as the first argument.
          
   gdFont _(TYPE)_
          A font structure. Used to declare the characteristics of a
          font. Plese see the files gdfontl.c and gdfontl.h for an
          example of the proper declaration of this structure. You can
          provide your own font data by providing such a structure and
          the associated pixel array. You can determine the width and
          height of a single character in a font by examining the w and h
          members of the structure. If you will not be creating your own
          fonts, you will not need to concern yourself with the rest of
          the components of this structure.
          

typedef struct {
        /* # of characters in font */
        int nchars;
        /* First character is numbered... (usually 32 = space) */
        int offset;
        /* Character width and height */
        int w;
        int h;
        /* Font data; array of characters, one row after another.
                Easily included in code, also easily loaded from
                data files. */
        char *data;
} gdFont;

   gdFontPtr _(TYPE)_
          A pointer to a font structure. Text-output functions expect
          these as their second argument, following the gdImagePtr
          argument. Two such pointers are declared in the provided
          include files gdfonts.h and gdfontl.h.
          
   gdPoint _(TYPE)_
          Represents a point in the coordinate space of the image; used
          by gdImagePolygon and gdImageFilledPolygon.
          

typedef struct {
        int x, y;
} gdPoint, *gdPointPtr;

   gdPointPtr _(TYPE)_
          A pointer to a gdPoint structure; passed as an argument to
          gdImagePolygon and gdImageFilledPolygon.
          
   gdSource _(TYPE)_

typedef struct {
        int (*source) (void *context, char *buffer, int len);
        void *context;
} gdSource, *gdSourcePtr;

   Represents a source from which a PNG can be read. Programmers who do
   not wish to read PNGs from a file can provide their own alternate
   input mechanism, using the gdImageCreateFromPngSource function. See
   the documentation of that function for an example of the proper use of
   this type.
   
   gdSink _(TYPE)_

typedef struct {
        int (*sink) (void *context, char *buffer, int len);
        void *context;
} gdSink, *gdSinkPtr;

   Represents a "sink" (destination) to which a PNG can be written.
   Programmers who do not wish to write PNGs to a file can provide their
   own alternate output mechanism, using the gdImagePngToSink function.
   See the documentation of that function for an example of the proper
   use of this type.
   
  Image creation, destruction, loading and saving
  
   gdImageCreate(sx, sy) _(FUNCTION)_
          gdImageCreate is called to create palette-based images, with no
          more than 256 colors. Invoke gdImageCreate with the x and y
          dimensions of the desired image. gdImageCreate returns a
          gdImagePtr to the new image, or NULL if unable to allocate the
          image. The image must eventually be destroyed using
          gdImageDestroy().
          

... inside a function ...
gdImagePtr im;
im = gdImageCreate(64, 64);
/* ... Use the image ... */
gdImageDestroy(im);

   gdImageCreateTrueColor(sx, sy) _(FUNCTION)_
          gdImageCreateTrueColor is called to create truecolor images,
          with an essentially unlimited number of colors. Invoke
          gdImageCreateTrueColor with the x and y dimensions of the
          desired image. gdImageCreateTrueColor returns a gdImagePtr to
          the new image, or NULL if unable to allocate the image. The
          image must eventually be destroyed using gdImageDestroy().
          
          Truecolor images are always filled with black at creation time.
          

... inside a function ...
gdImagePtr im;
im = gdImageCreateTrueColor(64, 64);
/* ... Use the image ... */
gdImageDestroy(im);

   gdImageCreateFromJpeg(FILE *in) _(FUNCTION)_
          gdImageCreateFromJpegCtx(FILE *in) _(FUNCTION)_
          
   gdImageCreateFromJpeg(FILE *in) _(FUNCTION)_
          gdImageCreateFromJpegCtx(FILE *in) _(FUNCTION)_
          
          
          gdImageCreateFromJpeg is called to load images from JPEG format
          files. Invoke gdImageCreateFromJpeg with an already opened
          pointer to a file containing the desired image.
          gdImageCreateFromJpeg returns a gdImagePtr to the new truecolor
          image, or NULL if unable to load the image (most often because
          the file is corrupt or does not contain a JPEG image).
          gdImageCreateFromJpeg does _not_ close the file. You can
          inspect the sx and sy members of the image to determine its
          size. The image must eventually be destroyed using
          gdImageDestroy(). _The returned image is always a truecolor
          image._
          

gdImagePtr im;
... inside a function ...
FILE *in;
in = fopen("myjpeg.jpg", "rb");
im = gdImageCreateFromJpeg(in);
fclose(in);
/* ... Use the image ... */
gdImageDestroy(im);

   gdImageCreateFromPng(FILE *in) _(FUNCTION)_
          gdImageCreateFromPngCtx(gdIOCtx *in) _(FUNCTION)_
          
          
          gdImageCreateFromPng is called to load images from PNG format
          files. Invoke gdImageCreateFromPng with an already opened
          pointer to a file containing the desired image.
          gdImageCreateFromPng returns a gdImagePtr to the new image, or
          NULL if unable to load the image (most often because the file
          is corrupt or does not contain a PNG image).
          gdImageCreateFromPng does _not_ close the file. You can inspect
          the sx and sy members of the image to determine its size. The
          image must eventually be destroyed using gdImageDestroy().
          
          If the PNG image being loaded is a truecolor image, the
          resulting gdImagePtr will refer to a truecolor image. If the
          PNG image being loaded is a palette or grayscale image, the
          resulting gdImagePtr will refer to a palette image. gd retains
          only 8 bits of resolution for each of the red, green and blue
          channels, and only 7 bits of resolution for the alpha channel.
          The former restriction affects only a handful of very rare
          48-bit color and 16-bit grayscale PNG images. The second
          restriction affects all semitransparent PNG images, but the
          difference is essentially invisible to the eye. 7 bits of alpha
          channel resolution is, in practice, quite a lot.
          

gdImagePtr im;
... inside a function ...
FILE *in;
in = fopen("mypng.png", "rb");
im = gdImageCreateFromPng(in);
fclose(in);
/* ... Use the image ... */
gdImageDestroy(im);

   gdImageCreateFromPngSource(gdSourcePtr in) _(FUNCTION)_
          gdImageCreateFromPngSource is called to load a PNG from a data
          source other than a file. Usage is very similar to the
          gdImageCreateFromPng function, except that the programmer
          provides a custom data source.
          
          The programmer must write an input function which accepts a
          context pointer, a buffer, and a number of bytes to be read as
          arguments. This function must read the number of bytes
          requested, unless the end of the file has been reached, in
          which case the function should return zero, or an error has
          occurred, in which case the function should return -1. The
          programmer then creates a gdSource structure and sets the
          source pointer to the input function and the context pointer to
          any value which is useful to the programmer.
          
          The example below implements gdImageCreateFromPng by creating a
          custom data source and invoking gdImageCreateFromPngSource.
          

static int freadWrapper(void *context, char *buf, int len);

gdImagePtr gdImageCreateFromPng(FILE *in)
{
        gdSource s;
        s.source = freadWrapper;
        s.context = in;
        return gdImageCreateFromPngSource(&s);
}

static int freadWrapper(void *context, char *buf, int len)
{
        int got = fread(buf, 1, len, (FILE *) context);
        return got;
}

   gdImageCreateFromGd(FILE *in) _(FUNCTION)_
          gdImageCreateFromGdCtx(gdIOCtx *in) _(FUNCTION)_
          
          
          gdImageCreateFromGd is called to load images from gd format
          files. Invoke gdImageCreateFromGd with an already opened
          pointer to a file containing the desired image in the gd file
          format, which is specific to gd and intended for very fast
          loading. (It is _not_ intended for compression; for
          compression, use PNG or JPEG.) gdImageCreateFromGd returns a
          gdImagePtr to the new image, or NULL if unable to load the
          image (most often because the file is corrupt or does not
          contain a gd format image). gdImageCreateFromGd does _not_
          close the file. You can inspect the sx and sy members of the
          image to determine its size. The image must eventually be
          destroyed using gdImageDestroy().
          

... inside a function ...
gdImagePtr im;
FILE *in;
in = fopen("mygd.gd", "rb");
im = gdImageCreateFromGd(in);
fclose(in);
/* ... Use the image ... */
gdImageDestroy(im);

   gdImageCreateFromGd2(FILE *in) _(FUNCTION)_
          gdImageCreateFromGd2Ctx(gdIOCtx *in) _(FUNCTION)_
          
          
          gdImageCreateFromGd2 is called to load images from gd2 format
          files. Invoke gdImageCreateFromGd2 with an already opened
          pointer to a file containing the desired image in the gd2 file
          format, which is specific to gd2 and intended for fast loading
          of parts of large images. (It is a compressed format, but
          generally not as good as maximum compression of the entire
          image would be.) gdImageCreateFromGd returns a gdImagePtr to
          the new image, or NULL if unable to load the image (most often
          because the file is corrupt or does not contain a gd format
          image). gdImageCreateFromGd2 does _not_ close the file. You can
          inspect the sx and sy members of the image to determine its
          size. The image must eventually be destroyed using
          gdImageDestroy().
          

... inside a function ...
gdImagePtr im;
FILE *in;
in = fopen("mygd.gd2", "rb");
im = gdImageCreateFromGd2(in);
fclose(in);
/* ... Use the image ... */
gdImageDestroy(im);

   gdImageCreateFromGd2Part(FILE *in, int srcX, int srcY, int w, int h)
          _(FUNCTION)_
          gdImageCreateFromGd2PartCtx(gdIOCtx *in) _(FUNCTION)_
          
          
          gdImageCreateFromGd2Part is called to load parts of images from
          gd2 format files. Invoked in the same way as
          gdImageCreateFromGd2, but with extra parameters indicating the
          source (x, y) and width/height of the desired image.
          gdImageCreateFromGd2Part returns a gdImagePtr to the new image,
          or NULL if unable to load the image. The image must eventually
          be destroyed using gdImageDestroy().
          
   gdImageCreateFromXbm(FILE *in) _(FUNCTION)_
          gdImageCreateFromXbm is called to load images from X bitmap
          format files. Invoke gdImageCreateFromXbm with an already
          opened pointer to a file containing the desired image.
          gdImageCreateFromXbm returns a gdImagePtr to the new image, or
          NULL if unable to load the image (most often because the file
          is corrupt or does not contain an X bitmap format image).
          gdImageCreateFromXbm does _not_ close the file. You can inspect
          the sx and sy members of the image to determine its size. The
          image must eventually be destroyed using gdImageDestroy().
          

... inside a function ...
gdImagePtr im;
FILE *in;
in = fopen("myxbm.xbm", "rb");
im = gdImageCreateFromXbm(in);
fclose(in);
/* ... Use the image ... */
gdImageDestroy(im);

   gdImageCreateFromXpm(char *filename) _(FUNCTION)_
          gdImageCreateFromXbm is called to load images from XPM X Window
          System color bitmap format files. This function is available
          only if HAVE_XPM is selected in the Makefile and the Xpm
          library is linked with the application. Unlike most gd file
          functions, the Xpm functions require filenames, not file
          pointers. gdImageCreateFromXpm returns a gdImagePtr to the new
          image, or NULL if unable to load the image (most often because
          the file is corrupt or does not contain an XPM bitmap format
          image). You can inspect the sx and sy members of the image to
          determine its size. The image must eventually be destroyed
          using gdImageDestroy().
          

... inside a function ...
gdImagePtr im;
FILE *in;
in = fopen("myxpm.xpm", "rb");
im = gdImageCreateFromXpm(in);
fclose(in);
/* ... Use the image ... */
gdImageDestroy(im);

   gdImageDestroy(gdImagePtr im) _(FUNCTION)_
          gdImageDestroy is used to free the memory associated with an
          image. It is important to invoke gdImageDestroy before exiting
          your program or assigning a new image to a gdImagePtr variable.
          

... inside a function ...
gdImagePtr im;
im = gdImageCreate(10, 10);
/* ... Use the image ... */
/* Now destroy it */
gdImageDestroy(im);

   void gdImageJpeg(gdImagePtr im, FILE *out, int quality) _(FUNCTION)_
          void gdImageJpegCtx(gdImagePtr im, gdIOCtx *out, int quality)
          
   _(FUNCTION)_
   
   gdImageJpeg outputs the specified image to the specified file in JPEG
   format. The file must be open for writing. Under MSDOS and all
   versions of Windows, it is important to use "wb" as opposed to simply
   "w" as the mode when opening the file, and under Unix there is no
   penalty for doing so. gdImageJpeg does _not_ close the file; your code
   must do so.
   
   If quality is negative, the default IJG JPEG quality value (which
   should yield a good general quality / size tradeoff for most
   situations) is used. Otherwise, for practical purposes, quality should
   be a value in the range 0-95, higher quality values usually implying
   both higher quality and larger image sizes.
   
   If you have set image interlacing using gdImageInterlace, this
   function will interpret that to mean you wish to output a progressive
   JPEG. Some programs (e.g., Web browsers) can display progressive JPEGs
   incrementally; this can be useful when browsing over a relatively slow
   communications link, for example. Progressive JPEGs can also be
   slightly smaller than sequential (non-progressive) JPEGs.

... inside a function ...
gdImagePtr im;
int black, white;
FILE *out;
/* Create the image */
im = gdImageCreate(100, 100);
/* Allocate background */
white = gdImageColorAllocate(im, 255, 255, 255);
/* Allocate drawing color */
black = gdImageColorAllocate(im, 0, 0, 0);
/* Draw rectangle */
gdImageRectangle(im, 0, 0, 99, 99, black);
/* Open output file in binary mode */
out = fopen("rect.jpg", "wb");
/* Write JPEG using default quality */
gdImageJpeg(im, out, -1);
/* Close file */
fclose(out);
/* Destroy image */
gdImageDestroy(im);

   void* gdImageJpegPtr(gdImagePtr im, int *size) _(FUNCTION)_
   Identical to gdImageJpeg except that it returns a pointer to a memory
   area with the JPEG data. This memory must be freed by the caller when
   it is no longer needed. _The caller must invoke gdFree(), not free(),
   unless the caller is absolutely certain that the same implementations
   of malloc, free, etc. are used both at library build time and at
   application build time._ The 'size' parameter receives the total size
   of the block of memory.
   
   void gdImagePng(gdImagePtr im, FILE *out) _(FUNCTION)_
   gdImagePng outputs the specified image to the specified file in PNG
   format. The file must be open for writing. Under MSDOS and all
   versions of Windows, it is important to use "wb" as opposed to simply
   "w" as the mode when opening the file, and under Unix there is no
   penalty for doing so. gdImagePng does _not_ close the file; your code
   must do so.

... inside a function ...
gdImagePtr im;
int black, white;
FILE *out;
/* Create the image */
im = gdImageCreate(100, 100);
/* Allocate background */
white = gdImageColorAllocate(im, 255, 255, 255);
/* Allocate drawing color */
black = gdImageColorAllocate(im, 0, 0, 0);
/* Draw rectangle */
gdImageRectangle(im, 0, 0, 99, 99, black);
/* Open output file in binary mode */
out = fopen("rect.png", "wb");
/* Write PNG */
gdImagePng(im, out);
/* Close file */
fclose(out);
/* Destroy image */
gdImageDestroy(im);

   void* gdImagePngPtr(gdImagePtr im, int *size) _(FUNCTION)_
   Identical to gdImagePng except that it returns a pointer to a memory
   area with the PNG data. This memory must be freed by the caller when
   it is no longer needed. _The caller must invoke gdFree(), not free(),
   unless the caller is absolutely certain that the same implementations
   of malloc, free, etc. are used both at library build time and at
   application build time._ The 'size' parameter receives the total size
   of the block of memory.
   
   gdImagePngToSink(gdImagePtr im, gdSinkPtr out) _(FUNCTION)_
   gdImagePngToSink is called to write a PNG to a data "sink"
   (destination) other than a file. Usage is very similar to the
   gdImagePng function, except that the programmer provides a custom data
   sink.
   
   The programmer must write an output function which accepts a context
   pointer, a buffer, and a number of bytes to be written as arguments.
   This function must write the number of bytes requested and return that
   number, unless an error has occurred, in which case the function
   should return -1. The programmer then creates a gdSink structure and
   sets the sink pointer to the output function and the context pointer
   to any value which is useful to the programmer.
   
   The example below implements gdImagePng by creating a custom data
   source and invoking gdImagePngFromSink.

static int stdioSink(void *context, char *buffer, int len)
{
        return fwrite(buffer, 1, len, (FILE *) context);
}

void gdImagePng(gdImagePtr im, FILE *out)
{
        gdSink mySink;
        mySink.context = (void *) out;
        mySink.sink = stdioSink;
        gdImagePngToSink(im, &mySink);
}

   void gdImageWBMP(gdImagePtr im, int fg, FILE *out)
   gdImageWBMPCtx(gdIOCtx *out) _(FUNCTION)__(FUNCTION)_
   gdImageWBMP outputs the specified image to the specified file in WBMP
   format. The file must be open for writing. Under MSDOS and all
   versions of Windows, it is important to use "wb" as opposed to simply
   "w" as the mode when opening the file, and under Unix there is no
   penalty for doing so. gdImageWBMP does _not_ close the file; your code
   must do so.
   
   _WBMP file support is black and white only. The color index specified
   by the fg argument is the "foreground," and only pixels of this color
   will be set in the WBMP file._ All other pixels will be considered
   "background."

... inside a function ...
gdImagePtr im;
int black, white;
FILE *out;
/* Create the image */
im = gdImageCreate(100, 100);
/* Allocate background */
white = gdImageColorAllocate(im, 255, 255, 255);
/* Allocate drawing color */
black = gdImageColorAllocate(im, 0, 0, 0);
/* Draw rectangle */
gdImageRectangle(im, 0, 0, 99, 99, black);
/* Open output file in binary mode */
out = fopen("rect.wbmp", "wb");
/* Write WBMP, with black as foreground */
gdImageWBMP(im, black, out);
/* Close file */
fclose(out);
/* Destroy image */
gdImageDestroy(im);

   void* gdImageWBMPPtr(gdImagePtr im, int *size) _(FUNCTION)_
   Identical to gdImageWBMP except that it returns a pointer to a memory
   area with the WBMP data. This memory must be freed by the caller when
   it is no longer needed. _The caller must invoke gdFree(), not free(),
   unless the caller is absolutely certain that the same implementations
   of malloc, free, etc. are used both at library build time and at
   application build time._ The 'size' parameter receives the total size
   of the block of memory.
   
   void gdImageGd(gdImagePtr im, FILE *out) _(FUNCTION)_
   gdImageGd outputs the specified image to the specified file in the gd
   image format. The file must be open for writing. Under MSDOS and all
   versions of Windows, it is important to use "wb" as opposed to simply
   "w" as the mode when opening the file, and under Unix there is no
   penalty for doing so. gdImagePng does _not_ close the file; your code
   must do so.
   
   The gd image format is intended for fast reads and writes of images
   your program will need frequently to build other images. It is _not_ a
   compressed format, and is not intended for general use.

... inside a function ...
gdImagePtr im;
int black, white;
FILE *out;
/* Create the image */
im = gdImageCreate(100, 100);
/* Allocate background */
white = gdImageColorAllocate(im, 255, 255, 255);
/* Allocate drawing color */
black = gdImageColorAllocate(im, 0, 0, 0);
/* Draw rectangle */
gdImageRectangle(im, 0, 0, 99, 99, black);
/* Open output file in binary mode */
out = fopen("rect.gd", "wb");
/* Write gd format file */
gdImageGd(im, out);
/* Close file */
fclose(out);
/* Destroy image */
gdImageDestroy(im);

   void* gdImageGdPtr(gdImagePtr im, int *size) _(FUNCTION)_
   Identical to gdImageGd except that it returns a pointer to a memory
   area with the GD data. This memory must be freed by the caller when it
   is no longer needed. _The caller must invoke gdFree(), not free(),
   unless the caller is absolutely certain that the same implementations
   of malloc, free, etc. are used both at library build time and at
   application build time._ The 'size' parameter receives the total size
   of the block of memory.
   
   void gdImageGd2(gdImagePtr im, FILE *out, int chunkSize, int fmt)
   _(FUNCTION)_
   gdImageGd2 outputs the specified image to the specified file in the
   gd2 image format. The file must be open for writing. Under MSDOS and
   all versions of Windows, it is important to use "wb" as opposed to
   simply "w" as the mode when opening the file, and under Unix there is
   no penalty for doing so. gdImageGd2 does _not_ close the file; your
   code must do so.
   
   The gd2 image format is intended for fast reads and writes of parts of
   images. It is a compressed format, and well suited to retrieving smll
   sections of much larger images. The third and fourth parameters are
   the 'chunk size' and format resposectively.
   
   The file is stored as a series of compressed subimages, and the _Chunk
   Size_ determines the sub-image size - a value of zero causes the GD
   library to use the default.
   
   It is also possible to store GD2 files in an uncompressed format, in
   which case the fourth parameter should be GD2_FMT_RAW.

... inside a function ...
gdImagePtr im;
int black, white;
FILE *out;
/* Create the image */
im = gdImageCreate(100, 100);
/* Allocate background */
white = gdImageColorAllocate(im, 255, 255, 255);
/* Allocate drawing color */
black = gdImageColorAllocate(im, 0, 0, 0);
/* Draw rectangle */
gdImageRectangle(im, 0, 0, 99, 99, black);
/* Open output file in binary mode */
out = fopen("rect.gd", "wb");
/* Write gd2 format file */
gdImageGd2(im, out, 0, GD2_FMT_COMPRESSED);
/* Close file */
fclose(out);
/* Destroy image */
gdImageDestroy(im);

   void* gdImageGd2Ptr(gdImagePtr im, int chunkSize, int fmt, int *size)
   _(FUNCTION)_
   Identical to gdImageGd2 except that it returns a pointer to a memory
   area with the GD2 data. This memory must be freed by the caller when
   it is no longer needed. _The caller must invoke gdFree(), not free(),
   unless the caller is absolutely certain that the same implementations
   of malloc, free, etc. are used both at library build time and at
   application build time._ The 'size' parameter receives the total size
   of the block of memory.
   
  Drawing Functions
  
   void gdImageSetPixel(gdImagePtr im, int x, int y, int color)
          _(FUNCTION)_
          gdImageSetPixel sets a pixel to a particular color index.
          Always use this function or one of the other drawing functions
          to access pixels; do not access the pixels of the gdImage
          structure directly.
          

... inside a function ...
gdImagePtr im;
int black;
int white;
im = gdImageCreate(100, 100);
/* Background color (first allocated) */
black = gdImageColorAllocate(im, 0, 0, 0);
/* Allocate the color white (red, green and blue all maximum). */
white = gdImageColorAllocate(im, 255, 255, 255);
/* Set a pixel near the center. */
gdImageSetPixel(im, 50, 50, white);
/* ... Do something with the image, such as saving it to a file... */
/* Destroy it */
gdImageDestroy(im);

   void gdImageLine(gdImagePtr im, int x1, int y1, int x2, int y2, int
          color) _(FUNCTION)_
          gdImageLine is used to draw a line between two endpoints (x1,y1
          and x2, y2). The line is drawn using the color index specified.
          Note that the color index can be an actual color returned by
          gdImageColorAllocate or one of gdStyled, gdBrushed or
          gdStyledBrushed.
          

... inside a function ...
gdImagePtr im;
int black;
int white;
im = gdImageCreate(100, 100);
/* Background color (first allocated) */
black = gdImageColorAllocate(im, 0, 0, 0);
/* Allocate the color white (red, green and blue all maximum). */
white = gdImageColorAllocate(im, 255, 255, 255);
/* Draw a line from the upper left corner to the lower right corner. */
gdImageLine(im, 0, 0, 99, 99, white);
/* ... Do something with the image, such as saving it to a file... */
/* Destroy it */
gdImageDestroy(im);

   void gdImageDashedLine(gdImagePtr im, int x1, int y1, int x2, int y2,
          int color) _(FUNCTION)_
          gdImageDashedLine is provided _solely for backwards
          compatibility _with gd 1.0. New programs should draw dashed
          lines using the normal gdImageLine function and the new
          gdImageSetStyle function.
          
          gdImageDashedLine is used to draw a dashed line between two
          endpoints (x1,y1 and x2, y2). The line is drawn using the color
          index specified. The portions of the line that are not drawn
          are left transparent so the background is visible.
          

... inside a function ...
gdImagePtr im;
int black;
int white;
im = gdImageCreate(100, 100);
/* Background color (first allocated) */
black = gdImageColorAllocate(im, 0, 0, 0);
/* Allocate the color white (red, green and blue all maximum). */
white = gdImageColorAllocate(im, 255, 255, 255);
/* Draw a dashed line from the upper left corner to the lower right corner. */
gdImageDashedLine(im, 0, 0, 99, 99);
/* ... Do something with the image, such as saving it to a file... */
/* Destroy it */
gdImageDestroy(im);

   void gdImagePolygon(gdImagePtr im, gdPointPtr points, int pointsTotal,
          int color) _(FUNCTION)_
          gdImagePolygon is used to draw a polygon with the verticies (at
          least 3) specified, using the color index specified. See also
          gdImageFilledPolygon.
          

... inside a function ...
gdImagePtr im;
int black;
int white;
/* Points of polygon */
gdPoint points[3];
im = gdImageCreate(100, 100);
/* Background color (first allocated) */
black = gdImageColorAllocate(im, 0, 0, 0);
/* Allocate the color white (red, green and blue all maximum). */
white = gdImageColorAllocate(im, 255, 255, 255);
/* Draw a triangle. */
points[0].x = 50;
points[0].y = 0;
points[1].x = 99;
points[1].y = 99;
points[2].x = 0;
points[2].y = 99;
gdImagePolygon(im, points, 3, white);
/* ... Do something with the image, such as saving it to a file... */
/* Destroy it */
gdImageDestroy(im);

   void gdImageRectangle(gdImagePtr im, int x1, int y1, int x2, int y2,
          int color) _(FUNCTION)_
          gdImageRectangle is used to draw a rectangle with the two
          corners (upper left first, then lower right) specified, using
          the color index specified.
          

... inside a function ...
gdImagePtr im;
int black;
int white;
im = gdImageCreate(100, 100);
/* Background color (first allocated) */
black = gdImageColorAllocate(im, 0, 0, 0);
/* Allocate the color white (red, green and blue all maximum). */
white = gdImageColorAllocate(im, 255, 255, 255);
/* Draw a rectangle occupying the central area. */
gdImageRectangle(im, 25, 25, 74, 74, white);
/* ... Do something with the image, such as saving it to a file... */
/* Destroy it */
gdImageDestroy(im);

   void gdImageFilledPolygon(gdImagePtr im, gdPointPtr points, int
          pointsTotal, int color) _(FUNCTION)_
          gdImageFilledPolygon is used to fill a polygon with the
          verticies (at least 3) specified, using the color index
          specified. See also gdImagePolygon.
          

... inside a function ...
gdImagePtr im;
int black;
int white;
int red;
/* Points of polygon */
gdPoint points[3];
im = gdImageCreate(100, 100);
/* Background color (first allocated) */
black = gdImageColorAllocate(im, 0, 0, 0);
/* Allocate the color white (red, green and blue all maximum). */
white = gdImageColorAllocate(im, 255, 255, 255);
/* Allocate the color red. */
red = gdImageColorAllocate(im, 255, 0, 0);
/* Draw a triangle. */
points[0].x = 50;
points[0].y = 0;
points[1].x = 99;
points[1].y = 99;
points[2].x = 0;
points[2].y = 99;
/* Paint it in white */
gdImageFilledPolygon(im, points, 3, white);
/* Outline it in red; must be done second */
gdImagePolygon(im, points, 3, red);
/* ... Do something with the image, such as saving it to a file... */
/* Destroy it */
gdImageDestroy(im);

   void gdImageFilledRectangle(gdImagePtr im, int x1, int y1, int x2, int
          y2, int color) _(FUNCTION)_
          gdImageFilledRectangle is used to draw a solid rectangle with
          the two corners (upper left first, then lower right) specified,
          using the color index specified.
          

... inside a function ...
gdImagePtr im;
int black;
int white;
im = gdImageCreate(100, 100);
/* Background color (first allocated) */
black = gdImageColorAllocate(im, 0, 0, 0);
/* Allocate the color white (red, green and blue all maximum). */
white = int gdImageColorAllocate(im, 255, 255, 255);
/* Draw a filled rectangle occupying the central area. */
gdImageFilledRectangle(im, 25, 25, 74, 74, white);
/* ... Do something with the image, such as saving it to a file... */
/* Destroy it */
gdImageDestroy(im);

   void gdImageArc(gdImagePtr im, int cx, int cy, int w, int h, int s,
          int e, int color) _(FUNCTION)_
          gdImageArc is used to draw a partial ellipse centered at the
          given point, with the specified width and height in pixels. The
          arc begins at the position in degrees specified by s and ends
          at the position specified by e. The arc is drawn in the color
          specified by the last argument. A circle can be drawn by
          beginning from 0 degrees and ending at 360 degrees, with width
          and height being equal. e must be greater than s. Values
          greater than 360 are interpreted modulo 360.
          

... inside a function ...
gdImagePtr im;
int black;
int white;
im = gdImageCreate(100, 50);
/* Background color (first allocated) */
black = gdImageColorAllocate(im, 0, 0, 0);
/* Allocate the color white (red, green and blue all maximum). */
white = gdImageColorAllocate(im, 255, 255, 255);
/* Inscribe an ellipse in the image. */
gdImageArc(im, 50, 25, 98, 48, 0, 360, white);
/* ... Do something with the image, such as saving it to a file... */
/* Destroy it */
gdImageDestroy(im);

   void gdImageFilledArc(gdImagePtr im, int cx, int cy, int w, int h, int
          s, int e, int color, int style) _(FUNCTION)_
          gdImageFilledArc is used to draw a partial ellipse centered at
          the given point, with the specified width and height in pixels.
          The arc begins at the position in degrees specified by s and
          ends at the position specified by e. The arc is filled in the
          color specified by the second to last argument. A circle can be
          drawn by beginning from 0 degrees and ending at 360 degrees,
          with width and height being equal. e must be greater than s.
          Values greater than 360 are interpreted modulo 360. The last
          argument is a bitwise OR of the following possibilities:
          
          + gdArc
          + gdChord
          + gdPie (synonym for gdChord)
          + gdNoFill
          + gdEdged
            
          gdArc and gdChord are mutually exclusive; gdChord just connects
          the starting and ending angles with a straight line, while
          gdArc produces a rounded edge. gdPie is a synonym for gdArc.
          gdNoFill indicates that the arc or chord should be outlined,
          not filled. gdEdged, used together with gdNoFill, indicates
          that the beginning and ending angles should be connected to the
          center; this is a good way to outline (rather than fill) a 'pie
          slice'.
          

... inside a function ...
gdImagePtr im;
int black;
int white;
im = gdImageCreate(100, 50);
/* Background color (first allocated) */
black = gdImageColorAllocate(im, 0, 0, 0);
/* Allocate the color white (red, green and blue all maximum). */
white = gdImageColorAllocate(im, 255, 255, 255);
/* Inscribe a filled pie slice in the image. */
gdImageFilledArc(im, 50, 25, 98, 48, 0, 45, white, gdArc);
/* ... Do something with the image, such as saving it to a file... */
/* Destroy it */
gdImageDestroy(im);

   void gdImageFilledEllipse(gdImagePtr im, int cx, int cy, int w, int h,
          int s, int e, int color) _(FUNCTION)_
          gdImageFilledEllipse is used to draw an ellipse centered at the
          given point, with the specified width and height in pixels. The
          ellipse is filled in the color specified by the last argument.
          A circle can be drawn by beginning from 0 degrees and ending at
          360 degrees, with width and height being equal. e must be
          greater than s. Values greater than 360 are interpreted modulo
          360.
          

... inside a function ...
gdImagePtr im;
int black;
int white;
im = gdImageCreate(100, 50);
/* Background color (first allocated) */
black = gdImageColorAllocate(im, 0, 0, 0);
/* Allocate the color white (red, green and blue all maximum). */
white = gdImageColorAllocate(im, 255, 255, 255);
/* Inscribe a filled ellipse in the image. */
gdImageFilledEllipse(im, 50, 25, 98, 48, white);
/* ... Do something with the image, such as saving it to a file... */
/* Destroy it */
gdImageDestroy(im);

   void gdImageFillToBorder(gdImagePtr im, int x, int y, int border, int
          color) _(FUNCTION)_
          gdImageFillToBorder floods a portion of the image with the
          specified color, beginning at the specified point and stopping
          at the specified border color. For a way of flooding an area
          defined by the color of the starting point, see gdImageFill.
          
          The border color _cannot_ be a special color such as gdTiled;
          it must be a proper solid color. The fill color can be,
          however.
          
          Note that gdImageFillToBorder is recursive. It is not the most
          naive implementation possible, and the implementation is
          expected to improve, but there will always be degenerate cases
          in which the stack can become very deep. This can be a problem
          in MSDOS and MS Windows 3.1 environments. (Of course, in a Unix
          or Windows 95/98/NT environment with a proper stack, this is
          not a problem at all.)
          

... inside a function ...
gdImagePtr im;
int black;
int white;
int red;
im = gdImageCreate(100, 50);
/* Background color (first allocated) */
black = gdImageColorAllocate(im, 0, 0, 0);
/* Allocate the color white (red, green and blue all maximum). */
white = gdImageColorAllocate(im, 255, 255, 255);
/* Allocate the color red. */
red = gdImageColorAllocate(im, 255, 0, 0);
/* Inscribe an ellipse in the image. */
gdImageArc(im, 50, 25, 98, 48, 0, 360, white);
/* Flood-fill the ellipse. Fill color is red, border color is
        white (ellipse). */
gdImageFillToBorder(im, 50, 50, white, red);
/* ... Do something with the image, such as saving it to a file... */
/* Destroy it */
gdImageDestroy(im);

   void gdImageFill(gdImagePtr im, int x, int y, int color) _(FUNCTION)_
          gdImageFill floods a portion of the image with the specified
          color, beginning at the specified point and flooding the
          surrounding region of the same color as the starting point. For
          a way of flooding a region defined by a specific border color
          rather than by its interior color, see gdImageFillToBorder.
          
          The fill color can be gdTiled, resulting in a tile fill using
          another image as the tile. However, the tile image cannot be
          transparent. If the image you wish to fill with has a
          transparent color index, call gdImageTransparent on the tile
          image and set the transparent color index to -1 to turn off its
          transparency.
          
          Note that gdImageFill is recursive. It is not the most naive
          implementation possible, and the implementation is expected to
          improve, but there will always be degenerate cases in which the
          stack can become very deep. This can be a problem in MSDOS and
          MS Windows environments. (Of course, in a Unix or Windows
          95/98/NT environment with a proper stack, this is not a problem
          at all.)
          

... inside a function ...
gdImagePtr im;
int black;
int white;
int red;
im = gdImageCreate(100, 50);
/* Background color (first allocated) */
black = gdImageColorAllocate(im, 0, 0, 0);
/* Allocate the color white (red, green and blue all maximum). */
white = gdImageColorAllocate(im, 255, 255, 255);
/* Allocate the color red. */
red = gdImageColorAllocate(im, 255, 0, 0);
/* Inscribe an ellipse in the image. */
gdImageArc(im, 50, 25, 98, 48, 0, 360, white);
/* Flood-fill the ellipse. Fill color is red, and will replace the
        black interior of the ellipse. */
gdImageFill(im, 50, 50, red);
/* ... Do something with the image, such as saving it to a file... */
/* Destroy it */
gdImageDestroy(im);

   void gdImageSetBrush(gdImagePtr im, gdImagePtr brush) _(FUNCTION)_
          A "brush" is an image used to draw wide, shaped strokes in
          another image. Just as a paintbrush is not a single point, a
          brush image need not be a single pixel. _Any_ gd image can be
          used as a brush, and by setting the transparent color index of
          the brush image with gdImageColorTransparent, a brush of any
          shape can be created. All line-drawing functions, such as
          gdImageLine and gdImagePolygon, will use the current brush if
          the special "color" gdBrushed or gdStyledBrushed is used when
          calling them.
          
          gdImageSetBrush is used to specify the brush to be used in a
          particular image. You can set any image to be the brush. If the
          brush image does not have the same color map as the first
          image, any colors missing from the first image will be
          allocated. If not enough colors can be allocated, the closest
          colors already available will be used. This allows arbitrary
          PNGs to be used as brush images. It also means, however, that
          you should not set a brush unless you will actually use it; if
          you set a rapid succession of different brush images, you can
          quickly fill your color map, and the results will not be
          optimal.
          
          You need not take any special action when you are finished with
          a brush. As for any other image, if you will not be using the
          brush image for any further purpose, you should call
          gdImageDestroy. You must not use the color gdBrushed if the
          current brush has been destroyed; you can of course set a new
          brush to replace it.
          

... inside a function ...
gdImagePtr im, brush;
FILE *in;
int black;
im = gdImageCreate(100, 100);
/* Open the brush PNG. For best results, portions of the
        brush that should be transparent (ie, not part of the
        brush shape) should have the transparent color index. */
in = fopen("star.png", "rb");
brush = gdImageCreateFromPng(in);
/* Background color (first allocated) */
black = gdImageColorAllocate(im, 0, 0, 0);
gdImageSetBrush(im, brush);
/* Draw a line from the upper left corner to the lower right corner
        using the brush. */
gdImageLine(im, 0, 0, 99, 99, gdBrushed);
/* ... Do something with the image, such as saving it to a file... */
/* Destroy it */
gdImageDestroy(im);
/* Destroy the brush image */
gdImageDestroy(brush);

   void gdImageSetTile(gdImagePtr im, gdImagePtr tile) _(FUNCTION)_
          A "tile" is an image used to fill an area with a repeated
          pattern. _Any_ gd image can be used as a tile, and by setting
          the transparent color index of the tile image with
          gdImageColorTransparent, a tile that allows certain parts of
          the underlying area to shine through can be created. All
          region-filling functions, such as gdImageFill and
          gdImageFilledPolygon, will use the current tile if the special
          "color" gdTiled is used when calling them.
          
          gdImageSetTile is used to specify the tile to be used in a
          particular image. You can set any image to be the tile. If the
          tile image does not have the same color map as the first image,
          any colors missing from the first image will be allocated. If
          not enough colors can be allocated, the closest colors already
          available will be used. This allows arbitrary PNGs to be used
          as tile images. It also means, however, that you should not set
          a tile unless you will actually use it; if you set a rapid
          succession of different tile images, you can quickly fill your
          color map, and the results will not be optimal.
          
          You need not take any special action when you are finished with
          a tile. As for any other image, if you will not be using the
          tile image for any further purpose, you should call
          gdImageDestroy. You must not use the color gdTiled if the
          current tile has been destroyed; you can of course set a new
          tile to replace it.
          

... inside a function ...
gdImagePtr im, tile;
FILE *in;
int black;
im = gdImageCreate(100, 100);
/* Open the tile PNG. For best results, portions of the
        tile that should be transparent (ie, allowing the
        background to shine through) should have the transparent
        color index. */
in = fopen("star.png", "rb");
tile = gdImageCreateFromPng(in);
/* Background color (first allocated) */
black = gdImageColorAllocate(im, 0, 0, 0);
gdImageSetTile(im, tile);
/* Fill an area using the tile. */
gdImageFilledRectangle(im, 25, 25, 75, 75, gdTiled);
/* ... Do something with the image, such as saving it to a file... */
/* Destroy it */
gdImageDestroy(im);
/* Destroy the tile image */
gdImageDestroy(tile);

   void gdImageSetStyle(gdImagePtr im, int *style, int styleLength)
          _(FUNCTION)_
          It is often desirable to draw dashed lines, dotted lines, and
          other variations on a broken line. gdImageSetStyle can be used
          to set any desired series of colors, including a special color
          that leaves the background intact, to be repeated during the
          drawing of a line.
          
          To use gdImageSetStyle, create an array of integers and assign
          them the desired series of color values to be repeated. You can
          assign the special color value gdTransparent to indicate that
          the existing color should be left unchanged for that particular
          pixel (allowing a dashed line to be attractively drawn over an
          existing image).
          
          Then, to draw a line using the style, use the normal
          gdImageLine function with the special color value gdStyled.
          
          As of version 1.1.1, the style array is copied when you set the
          style, so you need not be concerned with keeping the array
          around indefinitely. This should not break existing code that
          assumes styles are not copied.
          
          You can also combine styles and brushes to draw the brush image
          at intervals instead of in a continuous stroke. When creating a
          style for use with a brush, the style values are interpreted
          differently: zero (0) indicates pixels at which the brush
          should not be drawn, while one (1) indicates pixels at which
          the brush should be drawn. To draw a styled, brushed line, you
          must use the special color value gdStyledBrushed. For an
          example of this feature in use, see gddemo.c (provided in the
          distribution).
          

gdImagePtr im;
int styleDotted[2], styleDashed[6];
FILE *in;
int black;
int red;
im = gdImageCreate(100, 100);
/* Background color (first allocated) */
black = gdImageColorAllocate(im, 0, 0, 0);
red = gdImageColorAllocate(im, 255, 0, 0);
/* Set up dotted style. Leave every other pixel alone. */
styleDotted[0] = red;
styleDotted[1] = gdTransparent;
/* Set up dashed style. Three on, three off. */
styleDashed[0] = red;
styleDashed[1] = red;
styleDashed[2] = red;
styleDashed[3] = gdTransparent;
styleDashed[4] = gdTransparent;
styleDashed[5] = gdTransparent;
/* Set dotted style. Note that we have to specify how many pixels are
        in the style! */
gdImageSetStyle(im, styleDotted, 2);
/* Draw a line from the upper left corner to the lower right corner. */
gdImageLine(im, 0, 0, 99, 99, gdStyled);
/* Now the dashed line. */
gdImageSetStyle(im, styleDashed, 6);
gdImageLine(im, 0, 99, 0, 99, gdStyled);

/* ... Do something with the image, such as saving it to a file ... */

/* Destroy it */
gdImageDestroy(im);

   void gdImageAlphaBlending(gdImagePtr im, int blending) _(FUNCTION)_
          The gdImageAlphaBlending function allows for two different
          modes of drawing on truecolor images. In blending mode, the
          alpha channel component of the color supplied to all drawing
          functions, such as gdImageSetPixel, determines how much of the
          underlying color should be allowed to shine through. As a
          result, gd automatically blends the existing color at that
          point with the drawing color, and stores the result in the
          image. The resulting pixel is opaque. In non-blending mode, the
          drawing color is copied literally with its alpha channel
          information, replacing the destination pixel. Blending mode is
          not available when drawing on palette images.
          

gdImagePtr im;
int red, blue;
im = gdImageCreateTrueColor(100, 100);
/* Background color */
red = gdTrueColor(255, 0, 0);
gdImageFilledRectangle(im, 0, 0, 100, 100, red);
/* Drawing color. Full transparency would be an alpha channel value
        of 127 (gd has a 7 bit alpha chnanel). 0 is opaque,
        127 is transparent. So cut gdAlphaTransparent in half to get
        50% blending. */
blue = gdTrueColorAlpha(0, 0, 255, gdAlphaTransparent / 2);
/* Draw with blending. Result will be 50% red, 50% blue: yellow
        (emitted light, remember, not reflected light. What you learned
        in Kindergarten is wrong here). */
gdImageAlphaBlending(im, 1);
gdImageFilledRectangle(im, 0, 0, 25, 25, blue);
/* Draw without blending. Result will be 50% blue, 50%
        the background color of the image viewer or web browser
        used; results in browsers that don't support
        semi-transparent pixels are unpredictable! */
gdImageAlphaBlending(im, 0);
gdImageFilledRectangle(im, 75, 75, 25, 25, blue);
/* Write the image to disk, etc. */

  Query Functions
  
        int gdImageAlpha(gdImagePtr im, int color) _(MACRO)_
                gdImageAlpha is a macro which returns the alpha channel
                component of the specified color index. Alpha channel
                values vary between 0 (gdAlphaOpaque), which does not
                blend at all with the background, through 127
                (gdAlphaTransparent), which allows the background to
                shine through 100%. Use this macro rather than accessing
                the structure members directly. int
                gdImageBlue(gdImagePtr im, int color)
                
          _(MACRO)_
          gdImageBlue is a macro which returns the blue component of the
          specified color index. Use this macro rather than accessing the
          structure members directly.
          
   int gdImageGetPixel(gdImagePtr im, int x, int y) _(FUNCTION)_
          gdImageGetPixel() retrieves the color index of a particular
          pixel. Always use this function to query pixels; do not access
          the pixels of the gdImage structure directly.
          

... inside a function ...
FILE *in;
gdImagePtr im;
int c;
in = fopen("mypng.png", "rb");
im = gdImageCreateFromPng(in);
fclose(in);
c = gdImageGetPixel(im, gdImageSX(im) / 2, gdImageSY(im) / 2);
printf("The value of the center pixel is %d; RGB values are %d,%d,%d\n",
        c, im->red[c], im->green[c], im->blue[c]);
gdImageDestroy(im);

   int gdImageBoundsSafe(gdImagePtr im, int x, int y) _(FUNCTION)_
          gdImageBoundsSafe returns true (1) if the specified point is
          within the bounds of the image, false (0) if not. This function
          is intended primarily for use by those who wish to add
          functions to gd. All of the gd drawing functions already clip
          safely to the edges of the image.
          

... inside a function ...
gdImagePtr im;
int black;
int white;
im = gdImageCreate(100, 100);
if (gdImageBoundsSafe(im, 50, 50)) {
        printf("50, 50 is within the image bounds\n");
} else {
        printf("50, 50 is outside the image bounds\n");
}
gdImageDestroy(im);

   int gdImageGreen(gdImagePtr im, int color) _(MACRO)_
          gdImageGreen is a macro which returns the green component of
          the specified color index. Use this macro rather than accessing
          the structure members directly.
          
   int gdImageRed(gdImagePtr im, int color) _(MACRO)_
          gdImageRed is a macro which returns the red component of the
          specified color index. Use this macro rather than accessing the
          structure members directly.
          
   int gdImageSX(gdImagePtr im) _(MACRO)_
          gdImageSX is a macro which returns the width of the image in
          pixels. Use this macro rather than accessing the structure
          members directly.
          
   int gdImageSY(gdImagePtr im) _(MACRO)_
          gdImageSY is a macro which returns the height of the image in
          pixels. Use this macro rather than accessing the structure
          members directly.
          
  Fonts and text-handling functions
  
        void gdImageChar(gdImagePtr im, gdFontPtr font, int x, int y, int
                c, int color) _(FUNCTION)_
                gdImageChar is used to draw single characters on the
                image. (To draw multiple characters, use gdImageString or
                gdImageString16. See also gdImageStringFT for a high
                quality solution.) The second argument is a pointer to a
                font definition structure; five fonts are provided with
                gd, gdFontTiny, gdFontSmall, gdFontMediumBold,
                gdFontLarge, and gdFontGiant. You must include the files
                "gdfontt.h", "gdfonts.h", "gdfontmb.h", "gdfontl.h" and
                "gdfontg.h" respectively and (if you are not using a
                library-based approach) link with the corresponding .c
                files to use the provided fonts. The character specified
                by the fifth argument is drawn from left to right in the
                specified color. (See gdImageCharUp for a way of drawing
                vertical text.) Pixels not set by a particular character
                retain their previous color.
                

#include "gd.h"
#include "gdfontl.h"
... inside a function ...
gdImagePtr im;
int black;
int white;
im = gdImageCreate(100, 100);
/* Background color (first allocated) */
black = gdImageColorAllocate(im, 0, 0, 0);
/* Allocate the color white (red, green and blue all maximum). */
white = gdImageColorAllocate(im, 255, 255, 255);
/* Draw a character. */
gdImageChar(im, gdFontLarge, 0, 0, 'Q', white);
/* ... Do something with the image, such as saving it to a file... */
/* Destroy it */
gdImageDestroy(im);

        void gdImageCharUp(gdImagePtr im, gdFontPtr font, int x, int y,
                int c, int color) _(FUNCTION)_
                gdImageCharUp is used to draw single characters on the
                image, rotated 90 degrees. (To draw multiple characters,
                use gdImageStringUp or gdImageStringUp16.) The second
                argument is a pointer to a font definition structure;
                five fonts are provided with gd, gdFontTiny, gdFontSmall,
                gdFontMediumBold, gdFontLarge, and gdFontGiant. You must
                include the files "gdfontt.h", "gdfonts.h", "gdfontmb.h",
                "gdfontl.h" and "gdfontg.h" respectively and (if you are
                not using a library-based approach) link with the
                corresponding .c files to use the provided fonts. The
                character specified by the fifth argument is drawn from
                bottom to top, rotated at a 90-degree angle, in the
                specified color. (See gdImageChar for a way of drawing
                horizontal text.) Pixels not set by a particular
                character retain their previous color.
                

#include "gd.h"
#include "gdfontl.h"
... inside a function ...
gdImagePtr im;
int black;
int white;
im = gdImageCreate(100, 100);
/* Background color (first allocated) */
black = gdImageColorAllocate(im, 0, 0, 0);
/* Allocate the color white (red, green and blue all maximum). */
white = gdImageColorAllocate(im, 255, 255, 255);
/* Draw a character upwards so it rests against the top of the image. */
gdImageCharUp(im, gdFontLarge,
        0, gdFontLarge->h, 'Q', white);
/* ... Do something with the image, such as saving it to a file... */
/* Destroy it */
gdImageDestroy(im);

        void gdImageString(gdImagePtr im, gdFontPtr font, int x, int y,
                unsigned char *s, int color) _(FUNCTION)_
                gdImageString is used to draw multiple characters on the
                image. (To draw single characters, use gdImageChar.) The
                second argument is a pointer to a font definition
                structure; five fonts are provided with gd, gdFontTiny,
                gdFontSmall, gdFontMediumBold, gdFontLarge, and
                gdFontGiant. You must include the files "gdfontt.h",
                "gdfonts.h", "gdfontmb.h", "gdfontl.h" and "gdfontg.h"
                respectively and (if you are not using a library-based
                approach) link with the corresponding .c files to use the
                provided fonts. The null-terminated C string specified by
                the fifth argument is drawn from left to right in the
                specified color. (See gdImageStringUp for a way of
                drawing vertical text. See also gdImageStringFT for a
                high quality solution.) Pixels not set by a particular
                character retain their previous color.
                

#include "gd.h"
#include "gdfontl.h"
#include <string.h>
... inside a function ...
gdImagePtr im;
int black;
int white;
/* String to draw. */
char *s = "Hello.";
im = gdImageCreate(100, 100);
/* Background color (first allocated) */
black = gdImageColorAllocate(im, 0, 0, 0);
/* Allocate the color white (red, green and blue all maximum). */
white = gdImageColorAllocate(im, 255, 255, 255);
/* Draw a centered string. */
gdImageString(im, gdFontLarge,
        im->w / 2 - (strlen(s) * gdFontLarge->w / 2),
        im->h / 2 - gdFontLarge->h / 2,
        s, white);
/* ... Do something with the image, such as saving it to a file... */
/* Destroy it */
gdImageDestroy(im);

        void gdImageString16(gdImagePtr im, gdFontPtr font, int x, int y,
                unsigned short *s, int color) _(FUNCTION)_
                gdImageString is used to draw multiple 16-bit characters
                on the image. (To draw single characters, use
                gdImageChar.) The second argument is a pointer to a font
                definition structure; five fonts are provided with gd,
                gdFontTiny, gdFontSmall, gdFontMediumBold, gdFontLarge,
                and gdFontGiant. You must include the files "gdfontt.h",
                "gdfonts.h", "gdfontmb.h", "gdfontl.h" and "gdfontg.h"
                respectively and (if you are not using a library-based
                approach) link with the corresponding .c files to use the
                provided fonts. The null-terminated string of characters
                represented as 16-bit unsigned short integers specified
                by the fifth argument is drawn from left to right in the
                specified color. (See gdImageStringUp16 for a way of
                drawing vertical text.) Pixels not set by a particular
                character retain their previous color.
                
                This function was added in gd1.3 to provide a means of
                rendering fonts with more than 256 characters for those
                who have them. A more frequently used routine is
                gdImageString.
                
        void gdImageStringUp(gdImagePtr im, gdFontPtr font, int x, int y,
                unsigned char *s, int color) _(FUNCTION)_
                gdImageStringUp is used to draw multiple characters on
                the image, rotated 90 degrees. (To draw single
                characters, use gdImageCharUp.) The second argument is a
                pointer to a font definition structure; five fonts are
                provided with gd, gdFontTiny, gdFontSmall,
                gdFontMediumBold, gdFontLarge, and gdFontGiant. You must
                include the files "gdfontt.h", "gdfonts.h", "gdfontmb.h",
                "gdfontl.h" and "gdfontg.h" respectively and (if you are
                not using a library-based approach) link with the
                corresponding .c files to use the provided fonts.The
                null-terminated C string specified by the fifth argument
                is drawn from bottom to top (rotated 90 degrees) in the
                specified color. (See gdImageString for a way of drawing
                horizontal text.) Pixels not set by a particular
                character retain their previous color.
                

#include "gd.h"
#include "gdfontl.h"
#include <string.h>
... inside a function ...
gdImagePtr im;
int black;
int white;
/* String to draw. */
char *s = "Hello.";
im = gdImageCreate(100, 100);
/* Background color (first allocated) */
black = gdImageColorAllocate(im, 0, 0, 0);
/* Allocate the color white (red, green and blue all maximum). */
white = gdImageColorAllocate(im, 255, 255, 255);
/* Draw a centered string going upwards. Axes are reversed,
        and Y axis is decreasing as the string is drawn. */
gdImageStringUp(im, gdFontLarge,
        im->w / 2 - gdFontLarge->h / 2,
        im->h / 2 + (strlen(s) * gdFontLarge->w / 2),
        s, white);
/* ... Do something with the image, such as saving it to a file... */
/* Destroy it */
gdImageDestroy(im);

        void gdImageStringUp16(gdImagePtr im, gdFontPtr font, int x, int
                y, unsigned short *s, int color) _(FUNCTION)_
                gdImageString is used to draw multiple 16-bit characters
                vertically on the image. (To draw single characters, use
                gdImageChar.) The second argument is a pointer to a font
                definition structure; five fonts are provided with gd,
                gdFontTiny, gdFontSmall, gdFontMediumBold, gdFontLarge,
                and gdFontGiant. You must include the files "gdfontt.h",
                "gdfonts.h", "gdfontmb.h", "gdfontl.h" and "gdfontg.h"
                respectively and (if you are not using a library-based
                approach) link with the corresponding .c files to use the
                provided fonts. The null-terminated string of characters
                represented as 16-bit unsigned short integers specified
                by the fifth argument is drawn from bottom to top in the
                specified color. (See gdImageStringUp16 for a way of
                drawing horizontal text.) Pixels not set by a particular
                character retain their previous color.
                
                This function was added in gd1.3 to provide a means of
                rendering fonts with more than 256 characters for those
                who have them. A more frequently used routine is
                gdImageStringUp.
                
        char *gdImageStringFT(gdImagePtr im, int *brect, int fg, char
                *fontname, double ptsize, double angle, int x, int y,
                char *string) _(FUNCTION)_
                _RECOMMENDED. New in 1.8.4._ gdImageStringFT draws text
                using the FreeType 2.x library.
                
                gdImageStringFT draws a string of anti-aliased characters
                on the image using the FreeType library to render
                user-supplied TrueType fonts. _We do not provide TrueType
                fonts (.ttf and .ttc files). Obtaining them is entirely
                up to you._ The string is anti-aliased, meaning that
                there should be fewer "jaggies" visible. The fontname is
                the full pathname to a TrueType font file, or a font face
                name if the GDFONTPATH environment variable or FreeType's
                DEFAULT_FONTPATH variable have been set intelligently.
                The string may be arbitrarily scaled (ptsize) and rotated
                (angle in radians).
                
                The user-supplied int brect[8] array is filled on return
                from gdImageStringFT with the 8 elements representing the
                4 corner coordinates of the bounding rectangle.
                0 lower left corner, X position
                lower left corner, Y position
                lower right corner, X position
                3 lower right corner, Y position
                4 upper right corner, X position
                5 upper right corner, Y position
                6 upper left corner, X position
                7 upper left corner, Y position
                
                The points are relative to the text regardless of the
                angle, so "upper left" means in the top left-hand corner
                seeing the text horizontally.
                
                Use a NULL gdImagePtr to get the bounding rectangle
                without rendering. This is a relatively cheap operation
                if followed by a rendering of the same string, because of
                the caching of the partial rendering during bounding
                rectangle calculation.
                
                The string is rendered in the color indicated by the gf
                color index. _Use the negative of the desired color index
                to disable anti-aliasing._
                
                The string may contain UTF-8 sequences like: "&#192;"
                
                gdImageStringFT will return a null char* on success, or
                an error string on failure.
                

#include "gd.h"
#include <string.h>
... inside a function ...
gdImagePtr im;
int black;
int white;
int brect[8];
int x, y;
char *err;

char *s = "Hello."; /* String to draw. */
double sz = 40.;
char *f = "/usr/local/share/ttf/Times.ttf";  /* User supplied font */

/* obtain brect so that we can size the image */
err = gdImageStringFT(NULL,&brect[0],0,f,sz,0.,0,0,s);
if (err) {fprintf(stderr,err); return 1;}

/* create an image big enough for the string plus a little whitespace */
x = brect[2]-brect[6] + 6;
y = brect[3]-brect[7] + 6;
im = gdImageCreate(x,y);

/* Background color (first allocated) */
white = gdImageColorResolve(im, 255, 255, 255);
black = gdImageColorResolve(im, 0, 0, 0);

/* render the string, offset origin to center string*/
/* note that we use top-left coordinate for adjustment
 * since gd origin is in top-left with y increasing downwards. */
x = 3 - brect[6];
y = 3 - brect[7];
err = gdImageStringFT(im,&brect[0],black,f,sz,0.0,x,y,s);
if (err) {fprintf(stderr,err); return 1;}

/* Write img to stdout */
gdImagePng(im, stdout);

/* Destroy it */
gdImageDestroy(im);

        char *gdImageStringTTF(gdImagePtr im, int *brect, int fg, char
                *fontname, double ptsize, double angle, int x, int y,
                char *string) _(FUNCTION)_
                _DEPRECATED._ THis function simply invokes
                gdImageStringFT for backwards compatibility with old code
                that was written with FreeType 1.x. /DL>
                
  Color-handling functions
  
              int gdImageColorAllocate(gdImagePtr im, int r, int g, int
                      b) _(FUNCTION)_
                      gdImageColorAllocate finds the first available
                      color index in the image specified, sets its RGB
                      values to those requested (255 is the maximum for
                      each), and returns the index of the new color table
                      entry, or an RGBA value in the case of a truecolor
                      image; in either case you can then use the returned
                      value as a parameter to drawing functions. When
                      creating a new palette-based image, the first time
                      you invoke this function, you are setting the
                      background color for that image.
                      
                      In the event that all gdMaxColors colors (256) have
                      already been allocated, gdImageColorAllocate will
                      return -1 to indicate failure. (This is not
                      uncommon when working with existing PNG files that
                      already use 256 colors.) Note that
                      gdImageColorAllocate does not check for existing
                      colors that match your request; see
                      gdImageColorExact, gdImageColorClosest and
                      gdImageColorClosestHWB for ways to locate existing
                      colors that approximate the color desired in
                      situations where a new color is not available. Also
                      see gdImageColorResolve, new in gd-1.6.2.
                      

... inside a function ...
gdImagePtr im;
int black;
int red;
im = gdImageCreate(100, 100);
/* Background color (first allocated) */
black = gdImageColorAllocate(im, 0, 0, 0);
/* Allocate the color red. */
red = gdImageColorAllocate(im, 255, 0, 0);
/* Draw a dashed line from the upper left corner to the lower right corner. */
gdImageDashedLine(im, 0, 0, 99, 99, red);
/* ... Do something with the image, such as saving it to a file... */
/* Destroy it */
gdImageDestroy(im);

              int gdImageColorAllocateAlpha(gdImagePtr im, int r, int g,
                      int b, int a) _(FUNCTION)_
                      gdImageColorAllocateAlpha finds the first available
                      color index in the image specified, sets its RGBA
                      values to those requested (255 is the maximum for
                      red, green and blue, and 127 represents full
                      transparency for alpha), and returns the index of
                      the new color table entry, or an RGBA value in the
                      case of a truecolor image; in either case you can
                      then use the returned value as a parameter to
                      drawing functions. When creating a new
                      palette-based image, the first time you invoke this
                      function, you are setting the background color for
                      that image.
                      
                      In the event that all gdMaxColors colors (256) have
                      already been allocated, gdImageColorAllocate will
                      return -1 to indicate failure. (This is not
                      uncommon when working with existing palette-based
                      PNG files that already use 256 colors.) Note that
                      gdImageColorAllocateAlpha does not check for
                      existing colors that match your request; see
                      gdImageColorExactAlpha and gdImageColorClosestAlpha
                      for ways to locate existing colors that approximate
                      the color desired in situations where a new color
                      is not available. Also see
                      gdImageColorResolveAlpha.
                      

... inside a function ...
gdImagePtr im;
int black;
int red;
im = gdImageCreate(100, 100);
/* Background color (first allocated) */
black = gdImageColorAllocate(im, 0, 0, 0);
/* Allocate the color red, 50% transparent. */
red = gdImageColorAllocateAlpha(im, 255, 0, 0, 64);
/* Draw a dashed line from the upper left corner to the lower right corner. */
gdImageDashedLine(im, 0, 0, 99, 99, red);
/* ... Do something with the image, such as saving it to a file... */
/* Destroy it */
gdImageDestroy(im);

              int gdImageColorClosest(gdImagePtr im, int r, int g, int b)
                      _(FUNCTION)_
                      gdImageColorClosest searches the colors which have
                      been defined thus far in the image specified and
                      returns the index of the color with RGB values
                      closest to those of the request. (Closeness is
                      determined by Euclidian distance, which is used to
                      determine the distance in three-dimensional color
                      space between colors.)
                      
                      If no colors have yet been allocated in the image,
                      gdImageColorClosest returns -1.
                      
                      When applied to a truecolor image, this function
                      always succeeds in returning the desired color.
                      
                      This function is most useful as a backup method for
                      choosing a drawing color when an image already
                      contains gdMaxColors (256) colors and no more can
                      be allocated. (This is not uncommon when working
                      with existing PNG files that already use many
                      colors.) See gdImageColorExact for a method of
                      locating exact matches only.
                      

... inside a function ...
gdImagePtr im;
FILE *in;
int red;
/* Let's suppose that photo.png is a scanned photograph with
        many colors. */
in = fopen("photo.png", "rb");
im = gdImageCreateFromPng(in);
fclose(in);
/* Try to allocate red directly */
red = gdImageColorAllocate(im, 255, 0, 0);
/* If we fail to allocate red... */
if (red == (-1)) {
        /* Find the _closest_ color instead. */
        red = gdImageColorClosest(im, 255, 0, 0);
}
/* Draw a dashed line from the upper left corner to the lower right corner */
gdImageDashedLine(im, 0, 0, 99, 99, red);
/* ... Do something with the image, such as saving it to a file... */
/* Destroy it */
gdImageDestroy(im);

              int gdImageColorClosestAlpha(gdImagePtr im, int r, int g,
                      int b, int a) _(FUNCTION)_
                      gdImageColorClosest searches the colors which have
                      been defined thus far in the image specified and
                      returns the index of the color with RGBA values
                      closest to those of the request. (Closeness is
                      determined by Euclidian distance, which is used to
                      determine the distance in four-dimensional
                      color/alpha space between colors.)
                      
                      If no colors have yet been allocated in the image,
                      gdImageColorClosestAlpha returns -1.
                      
                      When applied to a truecolor image, this function
                      always succeeds in returning the desired color.
                      
                      This function is most useful as a backup method for
                      choosing a drawing color when a palette-based image
                      already contains gdMaxColors (256) colors and no
                      more can be allocated. (This is not uncommon when
                      working with existing palette-based PNG files that
                      already use many colors.) See
                      gdImageColorExactAlpha for a method of locating
                      exact matches only.
                      

... inside a function ...
gdImagePtr im;
FILE *in;
int red;
/* Let's suppose that photo.png is a scanned photograph with
        many colors. */
in = fopen("photo.png", "rb");
im = gdImageCreateFromPng(in);
fclose(in);
/* Try to allocate red, 50% transparent, directly */
red = gdImageColorAllocateAlpha(im, 255, 0, 0, 64);
/* If we fail to allocate red... */
if (red == (-1)) {
        /* Find the _closest_ color instead. */
        red = gdImageColorClosestAlpha(im, 255, 0, 0, 64);
}
/* Draw a dashed line from the upper left corner to the lower right corner */
gdImageDashedLine(im, 0, 0, 99, 99, red);
/* ... Do something with the image, such as saving it to a file... */
/* Destroy it */
gdImageDestroy(im);

              int gdImageColorClosestHWB(gdImagePtr im, int r, int g, int
                      b) _(FUNCTION)_
                      gdImageColorClosestHWB searches the colors which
                      have been defined thus far in the image specified
                      and returns the index of the color with hue,
                      whiteness and blackness closest to the requested
                      color. This scheme is typically superior to the
                      Euclidian distance scheme used by
                      gdImageColorClosest.
                      
                      If no colors have yet been allocated in the image,
                      gdImageColorClosestHWB returns -1.
                      
                      When applied to a truecolor image, this function
                      always succeeds in returning the desired color.
                      
                      This function is most useful as a backup method for
                      choosing a drawing color when an image already
                      contains gdMaxColors (256) colors and no more can
                      be allocated. (This is not uncommon when working
                      with existing PNG files that already use many
                      colors.) See gdImageColorExact for a method of
                      locating exact matches only.
                      

... inside a function ...
gdImagePtr im;
FILE *in;
int red;
/* Let's suppose that photo.png is a scanned photograph with
        many colors. */
in = fopen("photo.png", "rb");
im = gdImageCreateFromPng(in);
fclose(in);
/* Try to allocate red directly */
red = gdImageColorAllocate(im, 255, 0, 0);
/* If we fail to allocate red... */
if (red == (-1)) {
        /* Find the _closest_ color instead. */
        red = gdImageColorClosestHWB(im, 255, 0, 0);
}
/* Draw a dashed line from the upper left corner to the lower right corner */
gdImageDashedLine(im, 0, 0, 99, 99, red);
/* ... Do something with the image, such as saving it to a file... */
/* Destroy it */
gdImageDestroy(im);

              int gdImageColorExact(gdImagePtr im, int r, int g, int b)
                      _(FUNCTION)_
                      gdImageColorExact searches the colors which have
                      been defined thus far in the image specified and
                      returns the index of the first color with RGB
                      values which exactly match those of the request. If
                      no allocated color matches the request precisely,
                      gdImageColorExact returns -1. See
                      gdImageColorClosest for a way to find the color
                      closest to the color requested.
                      
                      When applied to a truecolor image, this function
                      always succeeds in returning the desired color.
                      

... inside a function ...
gdImagePtr im;
int red;
in = fopen("photo.png", "rb");
im = gdImageCreateFromPng(in);
fclose(in);
/* The image may already contain red; if it does, we'll save a slot
        in the color table by using that color. */
/* Try to allocate red directly */
red = gdImageColorExact(im, 255, 0, 0);
/* If red isn't already present... */
if (red == (-1)) {
        /* Second best: try to allocate it directly. */
        red = gdImageColorAllocate(im, 255, 0, 0);
        /* Out of colors, so find the _closest_ color instead. */
        red = gdImageColorClosest(im, 255, 0, 0);
}
/* Draw a dashed line from the upper left corner to the lower right corner */
gdImageDashedLine(im, 0, 0, 99, 99, red);
/* ... Do something with the image, such as saving it to a file... */
/* Destroy it */
gdImageDestroy(im);

              int gdImageColorResolve(gdImagePtr im, int r, int g, int b)
                      _(FUNCTION)_
                      gdImageColorResolve searches the colors which have
                      been defined thus far in the image specified and
                      returns the index of the first color with RGB
                      values which exactly match those of the request. If
                      no allocated color matches the request precisely,
                      then gdImageColorResolve tries to allocate the
                      exact color. If there is no space left in the color
                      table then gdImageColorResolve returns the closest
                      color (as in gdImageColorClosest). This function
                      always returns an index of a color.
                      
                      When applied to a truecolor image, this function
                      always succeeds in returning the desired color.
                      

... inside a function ...
gdImagePtr im;
int red;
in = fopen("photo.png", "rb");
im = gdImageCreateFromPng(in);
fclose(in);
/* The image may already contain red; if it does, we'll save a slot
        in the color table by using that color. */
/* Get index of red, or color closest to red */
red = gdImageColorResolve(im, 255, 0, 0);
/* Draw a dashed line from the upper left corner to the lower right corner */
gdImageDashedLine(im, 0, 0, 99, 99, red);
/* ... Do something with the image, such as saving it to a file... */
/* Destroy it */
gdImageDestroy(im);

              int gdImageColorResolveAlpha(gdImagePtr im, int r, int g,
                      int b, int a) _(FUNCTION)_
                      gdImageColorResolveAlpha searches the colors which
                      have been defined thus far in the image specified
                      and returns the index of the first color with RGBA
                      values which exactly match those of the request. If
                      no allocated color matches the request precisely,
                      then gdImageColorResolveAlpha tries to allocate the
                      exact color. If there is no space left in the color
                      table then gdImageColorResolveAlpha returns the
                      closest color (as in gdImageColorClosestAlpha).
                      This function always returns an index of a color.
                      
                      When applied to a truecolor image, this function
                      always succeeds in returning the desired color.
                      

... inside a function ...
gdImagePtr im;
int red;
in = fopen("photo.png", "rb");
im = gdImageCreateFromPng(in);
fclose(in);
/* The image may already contain red; if it does, we'll save a slot
        in the color table by using that color. */
/* Get index of red, 50% transparent, or the next best thing */
red = gdImageColorResolveAlpha(im, 255, 0, 0, 64);
/* Draw a dashed line from the upper left corner to the lower right corner */
gdImageDashedLine(im, 0, 0, 99, 99, red);
/* ... Do something with the image, such as saving it to a file... */
/* Destroy it */
gdImageDestroy(im);

              int gdImageColorsTotal(gdImagePtr im) _(MACRO)_
                      gdImageColorsTotal is a macro which returns the
                      number of colors currently allocated in a palette
                      image. For truecolor images, the result of this
                      call is undefined and should not be used.
                      
              int gdImageColorRed(gdImagePtr im, int c) _(MACRO)_
                      gdImageColorRed is a macro which returns the red
                      portion of the specified color in the image. This
                      macro works for both palette and truecolor images.
                      
              int gdImageColorGreen(gdImagePtr im, int c) _(MACRO)_
                      gdImageColorGreen is a macro which returns the
                      green portion of the specified color in the image.
                      This macro works for both palette and truecolor
                      images.
                      
              int gdImageColorBlue(gdImagePtr im, int c) _(MACRO)_
                      gdImageColorBlue is a macro which returns the green
                      portion of the specified color in the image. This
                      macro works for both palette and truecolor images.
                      
              int gdImageGetInterlaced(gdImagePtr im) _(MACRO)_
                      gdImageGetInterlaced is a macro which returns true
                      (1) if the image is interlaced, false (0) if not.
                      Use this macro to obtain this information; do not
                      access the structure directly. See gdImageInterlace
                      for a means of interlacing images.
                      
              int gdImageGetTransparent(gdImagePtr im) _(MACRO)_
                      gdImageGetTransparent is a macro which returns the
                      current transparent color index in the image. If
                      there is no transparent color,
                      gdImageGetTransparent returns -1. Use this macro to
                      obtain this information; do not access the
                      structure directly.
                      
              void gdImageColorDeallocate(gdImagePtr im, int color)
                      _(FUNCTION)_
                      gdImageColorDeallocate marks the specified color as
                      being available for reuse. It does not attempt to
                      determine whether the color index is still in use
                      in the image. After a call to this function, the
                      next call to gdImageColorAllocate for the same
                      image will set new RGB values for that color index,
                      changing the color of any pixels which have that
                      index as a result. If multiple calls to
                      gdImageColorDeallocate are made consecutively, the
                      lowest-numbered index among them will be reused by
                      the next gdImageColorAllocate call.
                      

... inside a function ...
gdImagePtr im;
int red, blue;
in = fopen("photo.png", "rb");
im = gdImageCreateFromPng(in);
fclose(in);
/* Look for red in the color table. */
red = gdImageColorExact(im, 255, 0, 0);
/* If red is present... */
if (red != (-1)) {
        /* Deallocate it. */
        gdImageColorDeallocate(im, red);
        /* Allocate blue, reusing slot in table.
                Existing red pixels will change color. */
        blue = gdImageColorAllocate(im, 0, 0, 255);
}
/* ... Do something with the image, such as saving it to a file... */
/* Destroy it */
gdImageDestroy(im);

              void gdImageColorTransparent(gdImagePtr im, int color)
                      _(FUNCTION)_
                      gdImageColorTransparent sets the transparent color
                      index for the specified image to the specified
                      index. To indicate that there should be _no_
                      transparent color, invoke gdImageColorTransparent
                      with a color index of -1. Note that JPEG images do
                      not support transparency, so this setting has no
                      effect when writing JPEG images.
                      
                      The color index used should be an index allocated
                      by gdImageColorAllocate, whether explicitly invoked
                      by your code or implicitly invoked by loading an
                      image. In order to ensure that your image has a
                      reasonable appearance when viewed by users who do
                      not have transparent background capabilities (or
                      when you are writing a JPEG-format file, which does
                      not support transparency), be sure to give
                      reasonable RGB values to the color you allocate for
                      use as a transparent color, _even though it will be
                      transparent on systems that support PNG
                      transparency_.
                      

... inside a function ...
gdImagePtr im;
int black;
FILE *in, *out;
in = fopen("photo.png", "rb");
im = gdImageCreateFromPng(in);
fclose(in);
/* Look for black in the color table and make it transparent. */
black = gdImageColorExact(im, 0, 0, 0);
/* If black is present... */
if (black != (-1)) {
        /* Make it transparent */
        gdImageColorTransparent(im, black);
}
/* Save the newly-transparent image back to the file */
out = fopen("photo.png", "wb");
gdImagePng(im, out);
fclose(out);
/* Destroy it */
gdImageDestroy(im);

              void gdImageTrueColor(int red, int green, int blue)
                      _(MACRO)_
                      gdImageTrueColor returns an RGBA color value for
                      use when drawing on a truecolor image. Red, green,
                      and blue are all in the range between 0 (off) and
                      255 (maximum). This macro should not be used with
                      palette-based images. If you need to write code
                      which is compatible with both palette-based and
                      truecolor images, use gdImageColorResolve.
                      
              void gdImageTrueColorAlpha(int red, int green, int blue,
                      int alpha) _(MACRO)_
                      gdImageTrueColorAlpha returns an RGBA color value
                      for use when drawing on a truecolor image with
                      alpha channel transparency. Red, green, and blue
                      are all in the range between 0 (off) and 255
                      (maximum). Alpha is in the range between 0 (opaque)
                      and 127 (fully transparent). This macro should not
                      be used with palette-based images. If you need to
                      write code which is compatible with both
                      palette-based and truecolor images, use
                      gdImageColorResolveAlpha.
                      
  Copying and resizing functions
  
              void gdImageCopy(gdImagePtr dst, gdImagePtr src, int dstX,
                      int dstY, int srcX, int srcY, int w, int h)
                      _(FUNCTION)_
                      gdImageCopy is used to copy a rectangular portion
                      of one image to another image. (For a way of
                      stretching or shrinking the image in the process,
                      see gdImageCopyResized.)
                      
                      The dst argument is the destination image to which
                      the region will be copied. The src argument is the
                      source image from which the region is copied. The
                      dstX and dstY arguments specify the point in the
                      destination image to which the region will be
                      copied. The srcX and srcY arguments specify the
                      upper left corner of the region in the source
                      image. The w and h arguments specify the width and
                      height of the region.
                      
                      When you copy a region from one location in an
                      image to another location in the same image,
                      gdImageCopy will perform as expected unless the
                      regions overlap, in which case the result is
                      unpredictable.
                      
                      _Important note on copying between images:_ since
                      different images do not necessarily have the same
                      color tables, pixels are not simply set to the same
                      color index values to copy them. gdImageCopy will
                      attempt to find an identical RGB value in the
                      destination image for each pixel in the copied
                      portion of the source image by invoking
                      gdImageColorExact. If such a value is not found,
                      gdImageCopy will attempt to allocate colors as
                      needed using gdImageColorAllocate. If both of these
                      methods fail, gdImageCopy will invoke
                      gdImageColorClosest to find the color in the
                      destination image which most closely approximates
                      the color of the pixel being copied.
                      

... Inside a function ...
gdImagePtr im_in;
gdImagePtr im_out;
int x, y;
FILE *in;
FILE *out;
/* Load a small png to tile the larger one with */
in = fopen("small.png", "rb");
im_in = gdImageCreateFromPng(in);
fclose(in);
/* Make the output image four times as large on both axes */
im_out = gdImageCreate(im_in->sx * 4, im_in->sy * 4);
/* Now tile the larger image using the smaller one */
for (y = 0; (y < 4); y++) {
        for (x = 0; (x < 4); x++) {
                gdImageCopy(im_out, im_in,
                        x * im_in->sx, y * im_in->sy,
                        0, 0,
                        im_in->sx, im_in->sy);
        }
}
out = fopen("tiled.png", "wb");
gdImagePng(im_out, out);
fclose(out);
gdImageDestroy(im_in);
gdImageDestroy(im_out);

              void gdImageCopyResized(gdImagePtr dst, gdImagePtr src, int
                      dstX, int dstY, int srcX, int srcY, int destW, int
                      destH, int srcW, int srcH) _(FUNCTION)_
                      gdImageCopyResized is used to copy a rectangular
                      portion of one image to another image. The X and Y
                      dimensions of the original region and the
                      destination region can vary, resulting in
                      stretching or shrinking of the region as
                      appropriate. (For a simpler version of this
                      function which does not deal with resizing, see
                      gdImageCopy.)
                      
                      The dst argument is the destination image to which
                      the region will be copied. The src argument is the
                      source image from which the region is copied. The
                      dstX and dstY arguments specify the point in the
                      destination image to which the region will be
                      copied. The srcX and srcY arguments specify the
                      upper left corner of the region in the source
                      image. The dstW and dstH arguments specify the
                      width and height of the destination region. The
                      srcW and srcH arguments specify the width and
                      height of the source region and can differ from the
                      destination size, allowing a region to be scaled
                      during the copying process.
                      
                      When you copy a region from one location in an
                      image to another location in the same image,
                      gdImageCopy will perform as expected unless the
                      regions overlap, in which case the result is
                      unpredictable. If this presents a problem, create a
                      scratch image in which to keep intermediate
                      results.
                      
                      _Important note on copying between images:_ since
                      images do not necessarily have the same color
                      tables, pixels are not simply set to the same color
                      index values to copy them. gdImageCopy will attempt
                      to find an identical RGB value in the destination
                      image for each pixel in the copied portion of the
                      source image by invoking gdImageColorExact. If such
                      a value is not found, gdImageCopy will attempt to
                      allocate colors as needed using
                      gdImageColorAllocate. If both of these methods
                      fail, gdImageCopy will invoke gdImageColorClosest
                      to find the color in the destination image which
                      most closely approximates the color of the pixel
                      being copied.
                      

... Inside a function ...
gdImagePtr im_in;
gdImagePtr im_out;
int x, y;
FILE *in;
FILE *out;
/* Load a small png to expand in the larger one */
in = fopen("small.png", "rb");
im_in = gdImageCreateFromPng(in);
fclose(in);
/* Make the output image four times as large on both axes */
im_out = gdImageCreate(im_in->sx * 4, im_in->sy * 4);
/* Now copy the smaller image, but four times larger */
gdImageCopyResized(im_out, im_in, 0, 0, 0, 0,
        im_out->sx, im_out->sy,
        im_in->sx, im_in->sy);
out = fopen("large.png", "wb");
gdImagePng(im_out, out);
fclose(out);
gdImageDestroy(im_in);
gdImageDestroy(im_out);

              void gdImageCopyResampled(gdImagePtr dst, gdImagePtr src,
                      int dstX, int dstY, int srcX, int srcY, int destW,
                      int destH, int srcW, int srcH) _(FUNCTION)_
                      gdImageCopyResampled is used to copy a rectangular
                      portion of one image to another image, smoothly
                      interpolating pixel values so that, in particular,
                      reducing the size of an image still retains a great
                      deal of clarity. The X and Y dimensions of the
                      original region and the destination region can
                      vary, resulting in stretching or shrinking of the
                      region as appropriate. (For a simpler version of
                      this function which does not deal with resizing,
                      see gdImageCopy. For a version which does not
                      interpolate pixel values, see gdImageCopyResized.
                      
                      Pixel values are only interpolated if the
                      destination image is a truecolor image. Otherwise,
                      gdImageCopyResized is automatically invoked.
                      
                      The dst argument is the destination image to which
                      the region will be copied. The src argument is the
                      source image from which the region is copied. The
                      dstX and dstY arguments specify the point in the
                      destination image to which the region will be
                      copied. The srcX and srcY arguments specify the
                      upper left corner of the region in the source
                      image. The dstW and dstH arguments specify the
                      width and height of the destination region. The
                      srcW and srcH arguments specify the width and
                      height of the source region and can differ from the
                      destination size, allowing a region to be scaled
                      during the copying process.
                      
                      When you copy a region from one location in an
                      image to another location in the same image,
                      gdImageCopy will perform as expected unless the
                      regions overlap, in which case the result is
                      unpredictable. If this presents a problem, create a
                      scratch image in which to keep intermediate
                      results.
                      
                      _Important note on copying between images:_ since
                      images do not necessarily have the same color
                      tables, pixels are not simply set to the same color
                      index values to copy them. If the destination image
                      is a palette image, gd will use the
                      gdImageColorResolve function to determine the best
                      color available.
                      

... Inside a function ...
gdImagePtr im_in;
gdImagePtr im_out;
int x, y;
FILE *in;
FILE *out;
/* Load a large png to shrink in the smaller one */
in = fopen("large.png", "rb");
im_in = gdImageCreateFromPng(in);
fclose(in);
/* Make the output image four times as small on both axes. Use
        a true color image so that we can interpolate colors. */
im_out = gdImageCreateTrueColor(im_in->sx / 4, im_in->sy / 4);
/* Now copy the large image, but four times smaller */
gdImageCopyResampled(im_out, im_in, 0, 0, 0, 0,
        im_out->sx, im_out->sy,
        im_in->sx, im_in->sy);
out = fopen("large.png", "wb");
gdImagePng(im_out, out);
fclose(out);
gdImageDestroy(im_in);
gdImageDestroy(im_out);

              void gdImageCopyMerge(gdImagePtr dst, gdImagePtr src, int
                      dstX, int dstY, int srcX, int srcY, int w, int h,
                      int pct) _(FUNCTION)_
                      gdImageCopyMerge is almost identical to
                      gdImageCopy, except that it 'merges' the two images
                      by an amount specified in the last parameter. If
                      the last parameter is 100, then it will function
                      identically to gdImageCopy - the source image
                      replaces the pixels in the destination.
                      
                      If, however, the _pct_ parameter is less than 100,
                      then the two images are merged. With pct = 0, no
                      action is taken.
                      
                      This feature is most useful to 'highlight' sections
                      of an image by merging a solid color with pct = 50:
                      

... Inside a function ...
gdImageCopyMerge(im_out, im_in, 100, 200, 0, 0, 30, 50, 50);

              void gdImageCopyMergeGray(gdImagePtr dst, gdImagePtr src,
                      int dstX, int dstY, int srcX, int srcY, int w, int
                      h, int pct) _(FUNCTION)_
                      gdImageCopyMergeGray is almost identical to
                      gdImageCopyMerge, except that when merging images
                      it preserves the hue of the source by converting
                      the destination pixels to grey scale before the
                      copy operation.
                      

... Inside a function ...
gdImageCopyMergeGray(im_out, im_in, 100, 200, 0, 0, 30, 50, 50);

              void gdImagePaletteCopy(gdImagePtr dst, gdImagePtr src)
                      _(FUNCTION)_
                      Copies a palette from one image to another,
                      attempting to match the colors in the target image
                      to the colors in the source palette.
                      
  Miscellaneous Functions
  
                    int gdImageCompare(gdImagePtr im1, gdImagePtr im2)
                            _(FUNCTION)_
                            gdImageCompare returns a bitmap indicating if
                            the two images are different. The members of
                            the bitmap are defined in gd.h, but the most
                            important is GD_CMP_IMAGE, which indicated
                            that the images will actually appear
                            different when displayed. Other, less
                            important, differences relate to pallette
                            entries. Any difference in the transparent
                            colour is assumed to make images display
                            differently, even if the transparent colour
                            is not used.
                            

... Inside a function ...
cmpMask = gdImageCompare(im1, im2);

                    gdImageInterlace(gdImagePtr im, int interlace)
                            _(FUNCTION)_
                            gdImageInterlace is used to determine whether
                            an image should be stored in a linear
                            fashion, in which lines will appear on the
                            display from first to last, or in an
                            interlaced fashion, in which the image will
                            "fade in" over several passes. By default,
                            images are not interlaced. (When writing JPEG
                            images, interlacing implies generating
                            progressive JPEG files, which are represented
                            as a series of scans of increasing quality.
                            Noninterlaced gd images result in regular
                            [sequential] JPEG data streams.)
                            
                            A nonzero value for the interlace argument
                            turns on interlace; a zero value turns it
                            off. Note that interlace has no effect on
                            other functions, and has no meaning unless
                            you save the image in PNG or JPEG format; the
                            gd and xbm formats do not support interlace.
                            
                            When a PNG is loaded with
                            gdImageCreateFromPng or a JPEG is loaded with
                            gdImageCreateFromJpeg, interlace will be set
                            according to the setting in the PNG or JPEG
                            file.
                            
                            Note that many PNG and JPEG viewers and web
                            browsers do _not_ support interlace or the
                            incremental display of progressive JPEGs.
                            However, the interlaced PNG or progressive
                            JPEG should still display; it will simply
                            appear all at once, just as other images do.
                            

gdImagePtr im;
FILE *out;
/* ... Create or load the image... */

/* Now turn on interlace */
gdImageInterlace(im, 1);
/* And open an output file */
out = fopen("test.png", "wb");
/* And save the image  -- could also use gdImageJpeg */
gdImagePng(im, out);
fclose(out);
gdImageDestroy(im);

                    gdFree(void *ptr) _(FUNCTION)_
                            gdFree provides a reliable way to free memory
                            allocated by functions such as gdImagePngPtr
                            which return blocks of memory. Use of this
                            function guarantees that the version of
                            free() that is ultimately called will be
                            intended for use with the version of malloc()
                            that originally allocated the block.
                            
  Constants
  
                          gdBrushed _(CONSTANT)_
                                  Used in place of a color when invoking
                                  a line-drawing function such as
                                  gdImageLine or gdImageRectangle. When
                                  gdBrushed is used as the color, the
                                  brush image set with gdImageSetBrush is
                                  drawn in place of each pixel of the
                                  line (the brush is usually larger than
                                  one pixel, creating the effect of a
                                  wide paintbrush). See also
                                  gdStyledBrushed for a way to draw
                                  broken lines with a series of distinct
                                  copies of an image.
                                  
                          gdMaxColors_(CONSTANT)_
                                  The constant 256. This is the maximum
                                  number of colors in a PNG file
                                  according to the PNG standard, and is
                                  also the maximum number of colors in a
                                  gd image.
                                  
                          gdStyled _(CONSTANT)_
                                  Used in place of a color when invoking
                                  a line-drawing function such as
                                  gdImageLine or gdImageRectangle. When
                                  gdStyled is used as the color, the
                                  colors of the pixels are drawn
                                  successively from the style that has
                                  been set with gdImageSetStyle. If the
                                  color of a pixel is equal to
                                  gdTransparent, that pixel is not
                                  altered. (This mechanism is completely
                                  unrelated to the "transparent color" of
                                  the image itself; see
                                  gdImageColorTransparent
                                  gdImageColorTransparent for that
                                  mechanism.) See also gdStyledBrushed.
                                  
                          gdStyledBrushed _(CONSTANT)_
                                  Used in place of a color when invoking
                                  a line-drawing function such as
                                  gdImageLine or gdImageRectangle. When
                                  gdStyledBrushed is used as the color,
                                  the brush image set with
                                  gdImageSetBrush is drawn at each pixel
                                  of the line, providing that the style
                                  set with gdImageSetStyle contains a
                                  nonzero value (OR gdTransparent, which
                                  does not equal zero but is supported
                                  for consistency) for the current pixel.
                                  (Pixels are drawn successively from the
                                  style as the line is drawn, returning
                                  to the beginning when the available
                                  pixels in the style are exhausted.)
                                  Note that this differs from the
                                  behavior of gdStyled, in which the
                                  values in the style are used as actual
                                  pixel colors, except for gdTransparent.
                                  
                          gdDashSize _(CONSTANT)_
                                  The length of a dash in a dashed line.
                                  Defined to be 4 for backwards
                                  compatibility with programs that use
                                  gdImageDashedLine. New programs should
                                  use gdImageSetStyle and call the
                                  standard gdImageLine function with the
                                  special "color" gdStyled or
                                  gdStyledBrushed.
                                  
                          gdTiled _(CONSTANT)_
                                  Used in place of a normal color in
                                  gdImageFilledRectangle,
                                  gdImageFilledPolygon, gdImageFill, and
                                  gdImageFillToBorder. gdTiled selects a
                                  pixel from the tile image set with
                                  gdImageSetTile in such a way as to
                                  ensure that the filled area will be
                                  tiled with copies of the tile image.
                                  See the discussions of gdImageFill and
                                  gdImageFillToBorder for special
                                  restrictions regarding those functions.
                                  
                          gdTransparent _(CONSTANT)_
                                  Used in place of a normal color in a
                                  style to be set with gdImageSetStyle.
                                  gdTransparent is _not_ the transparent
                                  color index of the image; for that
                                  functionality please see
                                  gdImageColorTransparent.
                                  
  About the additional .gd image file format
  
                                  In addition to reading and writing the
                                  PNG and JPEG formats and reading the X
                                  Bitmap format, gd has the capability to
                                  read and write its own ".gd" format.
                                  This format is _not_ intended for
                                  general purpose use and should never be
                                  used to distribute images. It is not a
                                  compressed format. Its purpose is
                                  solely to allow very fast loading of
                                  images your program needs often in
                                  order to build other images for output.
                                  If you are experiencing performance
                                  problems when loading large, fixed PNG
                                  images your program needs to produce
                                  its output images, you may wish to
                                  examine the functions
                                  gdImageCreateFromGd and gdImageGd,
                                  which read and write .gd format images.
                                  
                                  The program "pngtogd.c" is provided as
                                  a simple way of converting .png files
                                  to .gd format. I emphasize again that
                                  you will not need to use this format
                                  unless you have a need for high-speed
                                  loading of a few frequently-used images
                                  in your program.
                                  
  About the .gd2 image file format
  
                                  In addition to reading and writing the
                                  PNG format and reading the X Bitmap
                                  format, gd has the capability to read
                                  and write its own ".gd2" format. This
                                  format is _not_ intended for general
                                  purpose use and should never be used to
                                  distribute images. It is a compressed
                                  format allowing pseudo-random access to
                                  large image files. Its purpose is
                                  solely to allow very fast loading of
                                  _parts_ of images If you are
                                  experiencing performance problems when
                                  loading large, fixed PNG or JPEG images
                                  your program needs to produce its
                                  output images, you may wish to examine
                                  the functions gdImageCreateFromGd2,
                                  gdImageCreateFromGd2Part and
                                  gdImageGd2, which read and write .gd2
                                  format images.
                                  
                                  The program "pngtogd2.c" is provided as
                                  a simple way of converting .png files
                                  to .gd2 format.
                                  
  About the gdIOCtx structure
  
                                  Version 1.5 of GD added a new style of
                                  I/O based on an IOCtx structure (the
                                  most up-to-date version can be found in
                                  gd_io.h):
                                  

typedef struct gdIOCtx {
        int     (*getC)(struct gdIOCtx*);
        int     (*getBuf)(struct gdIOCtx*, void*, int);

        void     (*putC)(struct gdIOCtx*, int);
        int     (*putBuf)(struct gdIOCtx*, const void*, int);

        int     (*seek)(struct gdIOCtx*, const int);
        long    (*tell)(struct gdIOCtx*);

        void    (*free)(struct gdIOCtx*);

} gdIOCtx;

                          Most functions that accepted files in previous
                                  versions now also have a counterpart
                                  that accepts an I/O context. These
                                  functions have a 'Ctx' suffix.
                                  
                                  The Ctx routines use the function
                                  pointers in the I/O context pointed to
                                  by gdIOCtx to perform all I/O. Examples
                                  of how to implement an I/O context can
                                  be found in io_file.c (which provides a
                                  wrapper for file routines), and io_dp.c
                                  (which implements in-memory storage).
                                  
                                  It is not necessary to implement all
                                  functions in an I/O context if you know
                                  that it will only be used in limited
                                  cirsumstances. At the time of writing
                                  (Version 1.6.1, July 1999), the known
                                  requirements are:
                                  
                                  All   Must have 'free',
                                  Anything that reads from the context
                                  Must have 'getC' and 'getBuf',
                                  Anything that writes to the context
                                  Must have 'putC' and 'putBuf'.
                                  If gdCreateFromGd2Part is called Must
                                  also have 'seek' and 'tell'.
                                  If gdImageGd2 is called Must also have
                                  'seek' and 'tell'.
                                  
  Please tell us you're using gd!
  
                                  When you contact us and let us know you
                                  are using gd, you help us justify the
                                  time spent in maintaining and improving
                                  it. So please let us know. If the
                                  results are publicly visible on the
                                  web, a URL is a wonderful thing to
                                  receive, but if it's not a publicly
                                  visible project, a simple note is just
                                  as welcome.
                                  
  If you have problems
  
                                  If you have any difficulties with gd,
                                  feel free to contact the author, Thomas
                                  Boutell. Problems relating to the gd2
                                  format should be addressed to Philip
                                  Warner.
                                  
                                  _Be sure to read this manual carefully
                                  first. _
                                  
  Alphabetical quick index
  
                                  gdBrushed | gdDashSize | gdFont |
                                  gdFontPtr | gdFree | gdImage |
                                  gdImageAlphaBlending | gdImageArc |
                                  gdImageBlue | gdImageBoundsSafe |
                                  gdImageChar | gdImageCharUp |
                                  gdImageColorAllocate |
                                  gdImageColorAllocateAlpha |
                                  gdImageColorClosest |
                                  gdImageColorClosestAlpha |
                                  gdImageColorDeallocate |
                                  gdImageColorExact |
                                  gdImageColorExactAlpha |
                                  gdImageColorResolve |
                                  gdImageColorResolveAlpha |
                                  gdImageColorTransparent | gdImageCopy |
                                  gdImageCopyMerge | gdImageMergeGray |
                                  gdImageCopyResized |
                                  gdImageCopyResampled | gdImageCreate |
                                  gdImageCreatePalette |
                                  gdImageCreateTrueColor |
                                  gdImageCreateFromGd |
                                  gdImageCreateFromGd2 |
                                  gdImageCreateFromGd2Part |
                                  gdImageCreateFromJpeg |
                                  gdImageCreateFromPng |
                                  gdImageCreateFromPngSource |
                                  gdImageCreateFromXbm |
                                  gdImageCreateFromXpm |
                                  gdImageDashedLine | gdImageDestroy |
                                  gdImageFill | gdImageFilledArc |
                                  gdImageFilledEllipse |
                                  gdImageFillToBorder |
                                  gdImageFilledRectangle | gdImageGd |
                                  gdImageGd2 | gdImageGetInterlaced |
                                  gdImageGetPixel | gdImageGetTransparent
                                  | gdImageGreen | gdImageInterlace |
                                  gdImageJpeg | gdImageLine |
                                  gdImageFilledPolygon |
                                  gdImagePaletteCopy | gdImagePng |
                                  gdImagePngToSink | gdImagePolygon |
                                  gdImagePtr | gdImageWBMP |
                                  gdImageRectangle | gdImageRed |
                                  gdImageSetBrush | gdImageSetPixel |
                                  gdImageSetStyle | gdImageSetTile |
                                  gdImageString | gdImageString16 |
                                  gdImageStringFT | gdImageStringTTF |
                                  gdImageStringUp | gdImageStringUp16 |
                                  gdImageWBMP | gdMaxColors | gdPoint |
                                  gdStyled | gdStyledBrushed | gdTiled |
                                  gdTransparent
                                  
                                  _Boutell.Com, Inc._

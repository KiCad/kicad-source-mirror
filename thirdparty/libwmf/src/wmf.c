/* libwmf (wmf.c): library for wmf conversion
   Copyright (C) 2001 Francis James Franklin

   The libwmf Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The libwmf Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the libwmf Library; see the file COPYING.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */


#ifdef HAVE_CONFIG_H
#include "wmfconfig.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>

#include "wmfdefs.h"

#if defined (_WIN32) && !defined (LIBWMF_STATIC)
#include <windows.h>

static char installation_prefix[1000] = "";

BOOL WINAPI DllMain (HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	char* p;

	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		if (!GetModuleFileName ((HMODULE) hinstDLL,
					installation_prefix,
					sizeof (installation_prefix)))
			break;
		
		/* If the libwmf DLL is in a "bin" or "lib" subfolder, assume
		 * it's a Unix-style installation tree.
		 */
		p = strrchr (installation_prefix, '\\');
		if (p)
		{	*p = '\0';
			p = strrchr (installation_prefix, '\\');
			if (p && (stricmp (p + 1, "bin") == 0 ||
				  stricmp (p + 1, "lib") == 0))
				*p = '\0';
		}
		else
			installation_prefix[0] = '\0';
      
		break;
	}

  return TRUE;
}

char* _libwmf_get_fontdir (void)
{
	static char retval[1000] = "";

	if (retval[0] == '\0')
	{	strcpy (retval, installation_prefix);
		strcat (retval, "\\share\\libwmf\\fonts");
	}

	return retval;
}

#undef WMF_FONTDIR
#define WMF_FONTDIR _libwmf_get_fontdir ()

#endif

static void wmf_arg (unsigned long*,wmfAPI_Options*);
static void wmf_arg_fontdirs (wmfAPI*,wmfAPI_Options*);

/**
 * Creates and initializes an instance of the \b libwmf library for a specified device layer.
 * 
 * @param API_return pointer to a wmfAPI* (the API handle use henceforth)
 * @param flags      bitwise OR of WMF_OPT_ options
 * @param options    pointer to wmfAPI_Options structure
 * 
 * This is the first and necessary step when using \b libwmf. Options are passed via the wmfAPI_Options
 * structure and \p flags. wmf_api_create allocates the wmfAPI structure and initializes the color and
 * font tables, the metafile player, and the device layer. If successful then the pointer to the wmfAPI
 * structure is returned via \p API_return, otherwise all allocated memory is released and the library
 * exits with an appropriate error.
 * 
 * @return The error state of the library: \b wmf_E_None indicates successful creation and initialization
 *         of the library, and \p *API_return will be non-zero. For any other error value \p *API_return
 *         will be zero.
 */
wmf_error_t wmf_api_create (wmfAPI** API_return,unsigned long flags,wmfAPI_Options* options)
{	wmfAPI* API = 0;

	wmf_error_t err = wmf_E_None;

	(*API_return) = 0;

	if (flags & WMF_OPT_ARGS) wmf_arg (&flags,options);

	flags |= API_STANDARD_INTERFACE;

	err = wmf_lite_create (&API,flags,options);
	if (err != wmf_E_None) return err;

/* Create font data
 */
	API->font_data = 0;
	API->fonts = 0;

	wmf_ipa_font_init (API,options);
	wmf_arg_fontdirs (API,options);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		err = wmf_api_destroy (API);
		return (err);
	}

	if ((flags & WMF_OPT_WRITE) && (options->write_file))
	{	wmf_write_begin (API, options->write_file);

		if (ERR (API))
		{	WMF_DEBUG (API,"bailing...");
			err = wmf_api_destroy (API);
			return (err);
		}
	}

/* Have successfully created the API...
 */
	(*API_return) = API;

	return (wmf_E_None);
}

/**
 * Close the device layer, if open, and release all allocated memory attached to the memory manager.
 * 
 * @param API the API handle
 * 
 * @return The final error state of the library.
 */
wmf_error_t wmf_api_destroy (wmfAPI* API) /* Basically free all alloced memory */
{	wmf_error_t err;                  /* associated with the API */

	wmfFontmapData* FD = 0;

	FT_Library ft_lib = 0;

	if ((API->flags & API_FTLIBRARY_OPEN) && API->font_data)
	{	FD = (wmfFontmapData*) ((wmfFontData*) API->font_data)->user_data;
		if (FD) ft_lib = FD->Library;
	}

	if (API->write_data) wmf_write_end (API);

	err = wmf_lite_destroy (API);

	if (ft_lib) FT_Done_FreeType (ft_lib);

	return (err);
}

/* Check command line for arg.
 * ===========================
 */
static void wmf_arg (unsigned long* flags,wmfAPI_Options* options)
{	char** argv = options->argv;
	int    argc = options->argc;
	int    arg = 0;

	while ((++arg) < argc)
	{	if (strncmp (argv[arg],"--wmf-",6)) continue;

		if (strcmp (argv[arg],"--wmf-help") == 0)
		{	/* This is ignored; the controlling program can check for it... */
			continue;
		}

		if ((strcmp (argv[arg],"--wmf-error"    ) == 0)
		 || (strcmp (argv[arg],"--wmf-error=yes") == 0))
		{	(*flags) &= ~WMF_OPT_NO_ERROR;
			continue;
		}
		if (strcmp (argv[arg],"--wmf-error=no") == 0)
		{	(*flags) |= WMF_OPT_NO_ERROR;
			continue;
		}

		if ((strcmp (argv[arg],"--wmf-debug"    ) == 0)
		 || (strcmp (argv[arg],"--wmf-debug=yes") == 0))
		{	(*flags) &= ~WMF_OPT_NO_DEBUG;
			continue;
		}
		if (strcmp (argv[arg],"--wmf-debug=no") == 0)
		{	(*flags) |= WMF_OPT_NO_DEBUG;
			continue;
		}

		if (strcmp (argv[arg],"--wmf-sys-fonts") == 0)
		{	(*flags) |= WMF_OPT_SYS_FONTS;
			continue;
		}
		if (strncmp (argv[arg],"--wmf-sys-fontmap=",18) == 0)
		{	(*flags) |= WMF_OPT_SYS_FONTS;
			(*flags) |= WMF_OPT_SYS_FONTMAP;
			options->sys_fontmap_file = argv[arg] + 18;
			continue;
		}

		if (strcmp (argv[arg],"--wmf-xtra-fonts") == 0)
		{	(*flags) |= WMF_OPT_XTRA_FONTS;
			continue;
		}
		if (strncmp (argv[arg],"--wmf-xtra-fontmap=",19) == 0)
		{	(*flags) |= WMF_OPT_XTRA_FONTS;
			(*flags) |= WMF_OPT_XTRA_FONTMAP;
			options->xtra_fontmap_file = argv[arg] + 19;
			continue;
		}

		if (strncmp (argv[arg],"--wmf-gs-fontmap=",17) == 0)
		{	(*flags) |= WMF_OPT_GS_FONTMAP;
			options->gs_fontmap_file = argv[arg] + 17;
			continue;
		}

		if (strncmp (argv[arg],"--wmf-write=",12) == 0)
		{	(*flags) |= WMF_OPT_WRITE;
			options->write_file = argv[arg] + 12;
			continue;
		}

		if ((strcmp (argv[arg],"--wmf-ignore-nonfatal"    ) == 0)
		 || (strcmp (argv[arg],"--wmf-ignore-nonfatal=yes") == 0))
		{	(*flags) |= WMF_OPT_IGNORE_NONFATAL;
			continue;
		}
		if (strcmp (argv[arg],"--wmf-ignore-nonfatal=no") == 0)
		{	(*flags) &= ~WMF_OPT_IGNORE_NONFATAL;
			continue;
		}

		if (strcmp (argv[arg],"--wmf-diagnostics") == 0)
		{	(*flags) |= WMF_OPT_DIAGNOSTICS;
			continue;
		}

		if (strncmp (argv[arg],"--wmf-fontdir=",14) == 0) continue; /* ignore for now */
	}

	(*flags) &= 0x000fffff;
}

static void wmf_arg_fontdirs (wmfAPI* API,wmfAPI_Options* options)
{	char** argv = options->argv;
	int    argc = options->argc;
	int    arg = 0;

	if (API->flags & WMF_OPT_ARGS)
	{	while ((++arg) < argc)
		{	if (strncmp (argv[arg],"--wmf-fontdir=",14) == 0)
			{	wmf_ipa_font_dir (API,argv[arg] + 14);
			}
		}
	}

	if (API->flags & WMF_OPT_FONTDIRS)
	{	argv = options->fontdirs;
		while (*argv)
		{	wmf_ipa_font_dir (API,(*argv));
			argv++;
		}
	}

#ifndef _WIN32
 	wmf_ipa_font_dir (API,WMF_GS_FONTDIR);
#endif
	wmf_ipa_font_dir (API,WMF_FONTDIR);
}

/**
 * @verbatim 
Additional wmf-related options:

  --wmf-error[=yes|no]            switch for error reports.
  --wmf-debug[=yes|no]            switch for debug reports, if any.
  --wmf-ignore-nonfatal[=yes|no]  switch to ignore (some) non-fatal errors.
  --wmf-diagnostics               emit diagnostic information.
  --wmf-fontdir=<path>            add <path> to list of font directories.
  --wmf-sys-fonts                 use system fonts, if any found.
  --wmf-sys-fontmap=<file>        use system xml-fontmap file <file>.
  --wmf-xtra-fonts                use non-system fonts, if any found.
  --wmf-xtra-fontmap=<file>       use non-system xml-fontmap file <file>.
  --wmf-gs-fontmap=<file>         use ghostscript file <file>.
  --wmf-write=<file>              write metafile to <file>.

Report bugs to <http://www.wvware.com/>.
@endverbatim 
 * 
 * @return Returns the above as a string.
 */
char* wmf_help ()
{	static char* help = "\
Additional wmf-related options:\n\
\n\
  --wmf-error[=yes|no]            switch for error reports.\n\
  --wmf-debug[=yes|no]            switch for debug reports, if any.\n\
  --wmf-ignore-nonfatal[=yes|no]  switch to ignore (some) non-fatal errors.\n\
  --wmf-diagnostics               emit diagnostic information.\n\
  --wmf-fontdir=<path>            add <path> to list of font directories.\n\
  --wmf-sys-fonts                 use system fonts, if any found.\n\
  --wmf-sys-fontmap=<file>        use system xml-fontmap file <file>.\n\
  --wmf-xtra-fonts                use non-system fonts, if any found.\n\
  --wmf-xtra-fontmap=<file>       use non-system xml-fontmap file <file>.\n\
  --wmf-gs-fontmap=<file>         use ghostscript file <file>.\n\
  --wmf-write=<file>              write metafile to <file>.\n\
\n\
Report bugs to <http://www.wvware.com/>.\n";

	return (help);
}

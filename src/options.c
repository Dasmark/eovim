/*
 * Copyright (c) 2017 Jean Guyomarc'h
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "eovim/options.h"
#include "eovim/version.h"
#include "eovim/log.h"
#include <getopt.h>


static void
_show_help(void)
{
   /* Just show a pre-formatted help message */
   const char *const help =
      "Usage:\n"
      "  eovim [options] [files...]\n"
      "\n"
      "Options:\n"
      "  -N, --no-plugin         Do not load any plugin\n"
      "      --noplugin\n"
      "\n"
      "  --nvim <nvim>           Set the path to the Neovim program\n"
      "\n"
      "  -g, --geometry <WxH>    Set the initial dimensions of the window\n"
      "                          (e.g. 80x24 for a 80x24 cells window)\n"
      "  --config <path>         Provide an alternate GUI configuration\n"
      "  -F, --fullscreen        Run Eovim in fullscreen\n"
      "  -t, --theme <path>      Provide an alternate theme to Eovim\n"
      "  -h, --help              Display this message\n"
      "  -V, --version           Show Eovim's version\n"
      "\n"
      "Additionnaly, eovim forwards options it has no knowledge of to Neovim.\n"
      "Run nvim --help to learn more about Neovim's options.\n"
      ;
   puts(help);
}

/* Visual alias to tell that two strings are equal */
#define STREQ(StrA, StrB) (!strcmp(StrA, StrB))

/*
 * Okay listen up. Getopts suck. Especially the EFL one! I want Eovim to catch
 * some options, and let any other option alone. No reordering. So I describe
 * in an array of s_arg structures the options that Eovim accepts as a pair of
 * short/long options. Short options are used to identify matching options on
 * the command-line, pretty much like what GNU getopt_long does.
 *
 * When the short option has a letter, that's the short option, otherwise it
 * is used as a simple identifier.
 *
 * When we go through the command-line arguments, we always exclude the first
 * one, which is Eovim itself. If an argument starts with '-' then it may be
 * an Eovim option. If found, it will be processed by Eovim, otherwise it will
 * be directly forwarded to neovim. Positional arguments are automatically
 * forwarded to neovim.
 *
 * Forwarding is done by mutating argv, so it will contain uniquely forwarded
 * arguments until NULL is reached.
 */

typedef enum
{
   OPT_FORBIDDEN        = 0,
   OPT_CONFIG           = 1,
   OPT_NVIM             = 3,

   OPT_NO_PLUGIN        = 'N',
   OPT_GEOMETRY         = 'g',
   OPT_FULLSCREEN       = 'F',
   OPT_THEME            = 't',
   OPT_HELP             = 'h',
   OPT_VERSION          = 'V',
} e_opt;

typedef struct
{
   const char *const long_opt; /**< Long option name */
   const e_opt short_opt; /**< Option identifier */
} s_arg;

#define ARG(Long, Short) { .long_opt = Long, .short_opt = Short }

static const s_arg _args[] =
{
   ARG("noplugin",      OPT_NO_PLUGIN),
   ARG("no-plugin",     OPT_NO_PLUGIN),
   ARG("nvim",          OPT_NVIM),
   ARG("geometry",      OPT_GEOMETRY),
   ARG("config",        OPT_CONFIG),
   ARG("fullscreen",    OPT_FULLSCREEN),
   ARG("theme",         OPT_THEME),
   ARG("help",          OPT_HELP),
   ARG("version",       OPT_VERSION),
   ARG("embed",         OPT_FORBIDDEN),
   ARG("headless",      OPT_FORBIDDEN),
   ARG("api-info",      OPT_FORBIDDEN),
};
static const size_t _args_size = EINA_C_ARRAY_LENGTH(_args);


static int
_find_long_option(const char *arg)
{
   /* Match the string after the leading "--".  E.g. "--option" shall match
    * "option" */
   const char *const elem = &(arg[2]);
   for (size_t i = 0; i < _args_size; i++)
     if (STREQ(elem, _args[i].long_opt))
       return _args[i].short_opt;
   return -1;
}

static int
_find_short_option(const char *arg)
{
   /* Match the character after the leading '-'. E.g. "-o" shall match 'o' */
   for (size_t i = 0; i < _args_size; i++)
     if ((e_opt)(arg[1]) == _args[i].short_opt)
       return _args[i].short_opt;
   return -1;
}

static Eina_Bool
_parse_geometry(s_geometry *geo,
                const char *str)
{
   const int nb = sscanf(str, "%ux%u", &geo->w, &geo->h);
   if (nb != 2)
     {
        ERR("Failed to parse geometry. <UINT>x<UINT> is expected (e.g. 80x24)");
        return EINA_FALSE;
     }
   else if ((geo->w == 0) || (geo->h == 0))
     {
        ERR("Geometry cannot have a dimension of 0");
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

e_options_result
options_parse(int argc,
              const char *argv[],
              s_options *opts)
{
   e_options_result res = OPTIONS_RESULT_CONTINUE;

   /* This function will mutate argv, it will store in it the arguments to be
    * forwarded, in the order they have been encountered. NULL we be inserted
    * at the end of these. */
   size_t fwd = 0;
   for (int i = 1; i < argc; i++) /* Exclude argv[0] */
     {
        const char *const it = argv[i];
        if (it[0] == '-') /* We have an option */
          {
             int opt;
             if (it[1] == '-') /* Long option */
               opt = _find_long_option(it);
             else /* Short option */
               opt = _find_short_option(it);

             /* At this point we either have a >= 0 number, which is an eovim
              * option, or -1, which is an option to be forwarded to neovim */
             switch (opt)
               {
                  /* We make eovim catch some neovim options to make sure they
                   * are not used! */
                case OPT_FORBIDDEN:
                   fprintf(stderr,
                           "eovim: Forbidden option argument: \"%s\"\n", it);
                   return OPTIONS_RESULT_ERROR;

                   /* Theme, grab the next argument */
                case OPT_THEME:
                   opts->theme = argv[i + 1];
                   break;

                   /* Nvim program, grab the next argument */
                case OPT_NVIM:
                   opts->nvim_prog = argv[i + 1];
                   break;

                   /* GUI config, grab the next argument */
                case OPT_CONFIG:
                   opts->theme = argv[i + 1];
                   break;

                   /* Geomtry, parse the next argument */
                case OPT_GEOMETRY:
                   if (! _parse_geometry(&opts->geometry, argv[i + 1]))
                     return OPTIONS_RESULT_QUIT;
                   break;

                   /* No plugin, store true */
                case OPT_NO_PLUGIN:
                   opts->no_plugins = EINA_TRUE;
                   break;

                   /* Fullscreen, store true */
                case OPT_FULLSCREEN:
                   opts->fullscreen = EINA_TRUE;
                   break;

                   /* Help, print the help and stop */
                case OPT_HELP:
                   _show_help();
                   return OPTIONS_RESULT_QUIT;

                   /* Version, print the version and stop */
                case OPT_VERSION:
                   printf("eovim %s\n", EOVIM_VERSION);
                   return OPTIONS_RESULT_QUIT;

                   /* Unhandled: forward to neovim */
                default:
                   argv[fwd++] = argv[i];
                   break;
               }
          }
        else
          {
             /* That's a positional, forward to neovim */
             argv[fwd++] = argv[i];
          }
     }
   /* Done. Set to NULL the last element of argv to be forwarded. */
   argv[fwd] = NULL;

   return res;
}

void
options_defaults_set(s_options *opts)
{
   EINA_SAFETY_ON_NULL_RETURN(opts);

   /* Set everything to EINA_FALSE/NULL */
   memset(opts, 0, sizeof(s_options));

   /* Default geometry */
   opts->geometry.w = 120;
   opts->geometry.h = 40;

   opts->theme = "default";
   opts->nvim_prog = "nvim";
}

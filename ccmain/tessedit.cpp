/**********************************************************************
 * File:        tessedit.cpp  (Formerly tessedit.c)
 * Description: Main program for merge of tess and editor.
 * Author:					Ray Smith
 * Created:					Tue Jan 07 15:21:46 GMT 1992
 *
 * (C) Copyright 1992, Hewlett-Packard Ltd.
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 ** http://www.apache.org/licenses/LICENSE-2.0
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 *
 **********************************************************************/

#include "mfcpch.h"
//#include                                                      <osfcn.h>
//#include                                                      <signal.h>
//#include                                                      <time.h>
//#include                                                      <unistd.h>
#include          "tfacep.h"     //must be before main.h
//#include                                                      "fileerr.h"
#include          "stderr.h"
#include          "basedir.h"
#include          "tessvars.h"
//#include                                                      "debgwin.h"
//#include                                      "epapdest.h"
#include          "control.h"
#include          "imgs.h"
#include          "reject.h"
#include          "pageres.h"
//#include                                                      "gpapdest.h"
#include          "mainblk.h"
#include          "nwmain.h"
#include          "pgedit.h"
#include          "ocrshell.h"
#include          "tprintf.h"
//#include                                      "ipeerr.h"
//#include                                                      "restart.h"
#include          "tessedit.h"
//#include                                                      "fontfind.h"
#include "permute.h"
#include "permdawg.h"
#include "permnum.h"
#include "stopper.h"
#include "adaptmatch.h"
#include "intmatcher.h"
#include "chop.h"
#include "globals.h"

//extern "C" {
#include          "callnet.h"    //phils nn stuff
//}
#include          "notdll.h"     //phils nn stuff

#define VARDIR        "configs/" /*variables files */
                                 //config under api
#define API_CONFIG      "configs/api_config"
#define EXTERN

EXTERN BOOL_EVAR (tessedit_write_vars, FALSE, "Write all vars to file");
EXTERN BOOL_VAR (tessedit_tweaking_tess_vars, FALSE,
"Fiddle tess config values");

EXTERN INT_VAR (tweak_ReliableConfigThreshold, 2, "Tess VAR");

EXTERN double_VAR (tweak_garbage, 1.5, "Tess VAR");
EXTERN double_VAR (tweak_ok_word, 1.25, "Tess VAR");
EXTERN double_VAR (tweak_good_word, 1.1, "Tess VAR");
EXTERN double_VAR (tweak_freq_word, 1.0, "Tess VAR");
EXTERN double_VAR (tweak_ok_number, 1.4, "Tess VAR");
EXTERN double_VAR (tweak_good_number, 1.1, "Tess VAR");
EXTERN double_VAR (tweak_non_word, 1.25, "Tess VAR");
EXTERN double_VAR (tweak_CertaintyPerChar, -0.5, "Tess VAR");
EXTERN double_VAR (tweak_NonDictCertainty, -2.5, "Tess VAR");
EXTERN double_VAR (tweak_RejectCertaintyOffset, 1.0, "Tess VAR");
EXTERN double_VAR (tweak_GoodAdaptiveMatch, 0.125, "Tess VAR");
EXTERN double_VAR (tweak_GreatAdaptiveMatch, 0.10, "Tess VAR");
EXTERN INT_VAR (tweak_AdaptProtoThresh, 230, "Tess VAR");
EXTERN INT_VAR (tweak_AdaptFeatureThresh, 230, "Tess VAR");
EXTERN INT_VAR (tweak_min_outline_points, 6, "Tess VAR");
EXTERN INT_VAR (tweak_min_outline_area, 2000, "Tess VAR");
EXTERN double_VAR (tweak_good_split, 50.0, "Tess VAR");
EXTERN double_VAR (tweak_ok_split, 100.0, "Tess VAR");

extern INT16 XOFFSET;
extern INT16 YOFFSET;
extern int NO_BLOCK;

                                 //progress monitor
ETEXT_DESC *global_monitor = NULL;

int init_tesseract(const char *arg0,
                   const char *textbase,
                   const char *configfile,
                   int configc,
                   const char *const *configv) {
  FILE *var_file;
  static char c_path[MAX_PATH];  //path for c code

  // Set the basename, compute the data directory and read C++ configs.
  main_setup(arg0, textbase, configc, configv);
  debug_window_on.set_value (FALSE);

  if (tessedit_write_vars) {
    var_file = fopen ("edited.cfg", "w");
    if (var_file != NULL) {
      print_variables(var_file);
      fclose(var_file);
    }
  }
  strcpy (c_path, datadir.string ());
  c_path[strlen (c_path) - strlen (m_data_sub_dir.string ())] = '\0';
  demodir = c_path;
  start_recog(configfile, textbase);

  ReliableConfigThreshold = tweak_ReliableConfigThreshold;

  set_tess_tweak_vars();

  if (tessedit_use_nn)           //phils nn stuff
    init_net();
  return 0;                      //Normal exit
}

void end_tesseract() {
  end_recog();
}

#ifdef _TIFFIO_
void read_tiff_image(TIFF* tif, IMAGE* image) {
  tdata_t buf;
  uint32 image_width, image_height;
  uint16 photometric;
  short bpp;
  TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &image_width);
  TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &image_height);
  TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bpp);
  TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photometric);
  // Tesseract's internal representation is 0-is-black,
  // so if the photometric is 1 (min is black) then high-valued pixels
  // are 1 (white), otherwise they are 0 (black).
  UINT8 high_value = photometric == 1;
  image->create(image_width, image_height, bpp);
  IMAGELINE line;
  line.init(image_width);

  buf = _TIFFmalloc(TIFFScanlineSize(tif));
  int bytes_per_line = (image_width*bpp + 7)/8;
  UINT8* dest_buf = image->get_buffer() + bytes_per_line*image_height;
  // This will go badly wrong with one of the more exotic tiff formats,
  // but the majority will work OK.
  for (int y = 0; y < image_height; ++y) {
    TIFFReadScanline(tif, buf, y);
    dest_buf -= bytes_per_line;
    memcpy(dest_buf, buf, bytes_per_line);
  }
  if (high_value == 0)
    invert_image(image);
  _TIFFfree(buf);
}
#endif

/* Define command type identifiers */

enum CMD_EVENTS
{
  ACTION_1_CMD_EVENT,
  RECOG_WERDS,
  RECOG_PSEUDO,
  ACTION_2_CMD_EVENT
};

/**********************************************************************
 *  extend_menu()
 *
 *  Function called by pgeditor to let you extend the command menu.
 *  Items can be added to the "MODES" and "OTHER" menus.  The modes_id_base
 *  and other_id_base parameters are required to offset your command event ids
 *  from those of pgeditor, and to let the pgeditor which commands are mode
 *  changes and which are unmoded commands.  (Sorry if you think these offsets
 *  are a bit kludgy, the alternative would be to duplicate all the menu
 *  constructor modes within pgeditor so that the offsets could be hidden.)
 *
 *  Items for the "MODES" menu may only be simple menu items (just a name and
 *  id).  Items for the "OTHER" menu can be editable parameters or boolean
 *  toggles.  Refer to menu.h to see how to build different types.
 **********************************************************************/

void extend_menu(                             //handle for "MODES"
                 RADIO_MENU *modes_menu,
                 INT16 modes_id_base,         //mode cmd ids offset
                 NON_RADIO_MENU *other_menu,  //handle for "OTHER"
                 INT16 other_id_base          //mode cmd ids offset
                ) {
  /* Example new mode */

  modes_menu->add_child (new RADIO_MENU_LEAF ("Recog Words",
    modes_id_base + RECOG_WERDS));
  modes_menu->add_child (new RADIO_MENU_LEAF ("Recog Blobs",
    modes_id_base + RECOG_PSEUDO));

  /* Example toggle

  other_menu->add_child(
    new TOGGLE_MENU_LEAF( "Action 2",					//Display string
            other_id_base + ACTION_2_CMD_EVENT,	//offset command id
            FALSE ) );							//Initial value

   Example text parm  (commented out)

    other_menu->add_child(
    new VARIABLE_MENU_LEAF( "Parm change",				//Display string
            other_id_base + ACTION_3_CMD_EVENT,	//offset command id
            "default value" ) );				//default value string
  */
}


/**********************************************************************
 *  extend_moded_commands()
 *
 * Function called by pgeditor when the user is in one of the extended modes
 * defined by extend_menu() and the user has selected an area in the image
 * window.
 **********************************************************************/

void extend_moded_commands(                   //current mode
                           INT32 mode,
                           BOX selection_box  //area selected
                          ) {
  char msg[MAX_CHARS + 1];

  switch (mode) {
    case RECOG_WERDS:
      command_window->msg ("Recogging selected words");

      /* This is how to apply a "word processor" function to each selected word */

      process_selected_words(current_block_list,
                             selection_box,
                             &recog_interactive);
      break;
    case RECOG_PSEUDO:
      command_window->msg ("Recogging selected blobs");

      /* This is how to apply a "word processor" function to each selected word */

      recog_pseudo_word(current_block_list, selection_box);
      break;
    default:
      sprintf (msg, "Unexpected extended mode " INT32FORMAT, mode);
      command_window->msg (msg);
  }
}


/**********************************************************************
 *  extend_unmoded_commands()
 *
 * Function called by pgeditor when the user has selected one of the unmoded
 * extended menu options.
 **********************************************************************/

void extend_unmoded_commands(                 //current mode
                             INT32 cmd_event,
                             char *new_value  //changed value if any
                            ) {
  char msg[MAX_CHARS + 1];

  switch (cmd_event) {
    case ACTION_2_CMD_EVENT:     //a toggle event
      if (new_value[0] == 'T')
                                 //Display message
        command_window->msg ("Extended Action 2 ON!!");
      else
        command_window->msg ("Extended Action 2 OFF!!");
      break;
    default:
      sprintf (msg, "Unrecognised extended command " INT32FORMAT " (%s)",
        cmd_event, new_value);
      command_window->msg (msg);
      break;
  }
}


/*************************************************************************
 * set_tess_tweak_vars()
 * Set TESS vars from the tweek value - This is only really of use during search
 * of the space of tess configs - othertimes the default values are set
 *
 *************************************************************************/
void set_tess_tweak_vars() {
  if (tessedit_tweaking_tess_vars) {
    garbage = tweak_garbage;
    ok_word = tweak_ok_word;
    good_word = tweak_good_word;
    freq_word = tweak_freq_word;
    ok_number = tweak_ok_number;
    good_number = tweak_good_number;
    non_word = tweak_non_word;
    CertaintyPerChar = tweak_CertaintyPerChar;
    NonDictCertainty = tweak_NonDictCertainty;
    RejectCertaintyOffset = tweak_RejectCertaintyOffset;
    GoodAdaptiveMatch = tweak_GoodAdaptiveMatch;
    GreatAdaptiveMatch = tweak_GreatAdaptiveMatch;
    AdaptProtoThresh = tweak_AdaptProtoThresh;
    AdaptFeatureThresh = tweak_AdaptFeatureThresh;
    min_outline_points = tweak_min_outline_points;
    min_outline_area = tweak_min_outline_area;
    good_split = tweak_good_split;
    ok_split = tweak_ok_split;
  }
  //   if (expiry_day * 24 * 60 * 60 < time(NULL))
  //         err_exit();
}

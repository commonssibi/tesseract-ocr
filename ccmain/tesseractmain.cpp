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
#include "applybox.h"
#include "control.h"
#include "tessvars.h"
#include "tessedit.h"
#include "pageres.h"
#include "imgs.h"
#include "varabled.h"
#include "tprintf.h"
#include "tesseractmain.h"
#include "stderr.h"
#include "notdll.h"

#define VARDIR        "configs/" /*variables files */
                                 //config under api
#define API_CONFIG      "configs/api_config"
#define EXTERN

EXTERN BOOL_VAR (tessedit_read_image, TRUE, "Ensure the image is read");
EXTERN BOOL_VAR (tessedit_write_images, FALSE,
"Capture the image from the IPE");
EXTERN BOOL_VAR (tessedit_debug_to_screen, FALSE, "Dont use debug file");

extern INT16 XOFFSET;
extern INT16 YOFFSET;
extern int NO_BLOCK;

const ERRCODE USAGE = "Usage";
char szAppName[] = "Tessedit";   //app name

/**********************************************************************
 *  main()
 *
 **********************************************************************/

int main(int argc, char **argv) { 
  UINT16 lang;                   //language
  STRING pagefile;               //input file

  if (argc < 4) {
    USAGE.error (argv[0], EXIT,
      "%s imagename outputbase configfile [[+|-]varfile]...\n", argv[0]);
  }
  if (argc == 7
    && ocr_open_shm (argv[1], argv[2], argv[3], argv[4], argv[5], argv[6],
    &lang) == OKAY)
                                 //run api interface
    return api_main (argv[0], lang);

  init_tesseract (argv[0], argv[2], argv[3], argc - 4, argv + 4);

  tprintf ("Tesseract Open Source OCR Engine\n");

  if (tessedit_read_image) {
#ifdef _TIFFIO_
    TIFF* tif = TIFFOpen(argv[1], "r");
    if (tif) {
      read_tiff_image(tif);
      TIFFClose(tif); 
    } else
    READFAILED.error (argv[0], EXIT, argv[1]);

#else
    if (page_image.read_header (argv[1]) < 0)
      READFAILED.error (argv[0], EXIT, argv[1]);
    if (page_image.read (page_image.get_ysize ()) < 0) {
      MEMORY_OUT.error (argv[0], EXIT, "Read of image %s",
        argv[1]);
    }
#endif
  }

  pagefile = argv[1];
  recognize_page(pagefile);

  end_tesseract(); 

  return 0;                      //Normal exit
}


/**********************************************************************
 * api_main()
 *
 * Run the ocr engine via the ocr api.
 **********************************************************************/
//extern "C" {
//};

INT32 api_main(                   //run from api
               const char *arg0,  //program name
               UINT16 lang        //language
              ) {
  INT32 block_count;
  STRING var_path;               //output variables
  STRING debugfile;
  STRING pagefile;               //input file
  char imfile[MAX_PATH];         //output file
  INT16 result;                  //from open
  static const char *app_name = "HP OCR RESEARCH PROTOTYPE";
  static const char *app_version = "Version 7.0";
  static const char *fake_name = "input.tif";
  ETEXT_DESC *monitor;           //progress monitor
  const char *fake_argv[] = { "api_config" };

  init_tesseract (arg0, "tessapi", NULL, 1, fake_argv);
  if (interactive_mode) {
    debug_window_on.set_value (TRUE);
  }

  //ASSERT(FALSE);                                                                                                        //uncomment to use debugger
  result = setup_info (lang, app_name, app_version);
  if (result != OKAY) {
    tprintf ("Info setup failed\n");
    return 1;
  }
  block_count = 0;
  do {
                                 //read image from shm
    result = read_image (&page_image);
    if (result == HPERR) {
      tprintf ("Image read failed\n");
      return 1;
    }
    if (page_image.get_bpp () != 0) {
      //initialize the progress monitor
      monitor = ocr_setup_monitor ();
      if (monitor == NULL)
        return HPERR;            //release failed
      global_monitor = monitor;
      monitor->ocr_alive = TRUE;

      if (tessedit_write_images) {
        sprintf (imfile, "image" INT32FORMAT ".tif", block_count);
        page_image.write (imfile);
      }
      block_count++;

      pagefile = fake_name;
      pgeditor_read_file(pagefile, current_block_list); 

      monitor->ocr_alive = TRUE;

      if (edit_variables)
        start_variables_editor(); 
      if (interactive_mode) {
        pgeditor_main();  //pgeditor user I/F
      }
      else {
        PAGE_RES page_res(current_block_list); 

        //                              tprintf("Recognizing words\n");
        recog_all_words(&page_res, monitor); 

      }
                                 //no longer needed
      current_block_list->clear ();
      //                      if (text_lines<0)
      //                      {
      //                              logf("Operation cancelled!");
      //                              break;                                                                                                  //cancelled
      //                      }
    }
  }
  while (page_image.get_bpp () != 0);
  tprintf ("Closing down");
  end_tesseract(); 
  if (ocr_shutdown () != OKAY) {
    tprintf ("Closedown failed\n");
    return 1;
  }

  return 0;                      //Normal exit
}


/**********************************************************************
 * setup_info
 *
 * Setup the info on the engine and fonts.
 **********************************************************************/

INT16 setup_info(                     //setup dummy engine info
                 UINT16 lang,         //user language
                 const char *name,    //of engine
                 const char *version  //of engine
                ) {
  INT16 result;                  //from open

  //setup OCR engine here and get version info

  //If engine can really recognize fonts, do this
  //If it just recognizes families, make the names NULL
  //If it can't even recognize families, call it once:
  //result=ocr_append_fontinfo(lang,FFAM_NONE,
  //      CHSET_ANSI,PITCH_DEF,NULL);
                                 //index 0
  result = ocr_append_fontinfo (lang, FFAM_MODERN, CHSET_ANSI, PITCH_FIXED, "Courier New");
  if (result != OKAY)
    return result;
                                 //index 1
  result = ocr_append_fontinfo (lang, FFAM_ROMAN, CHSET_ANSI, PITCH_VAR, "Times New Roman");
  if (result != OKAY)
    return result;
                                 //index 2
  result = ocr_append_fontinfo (lang, FFAM_SWISS, CHSET_ANSI, PITCH_VAR, "Arial");
  if (result != OKAY)
    return result;
                                 //index 3
  result = ocr_append_fontinfo (lang, FFAM_SWISS, CHSET_ANSI, PITCH_FIXED, "Letter Gothic");
  if (result != OKAY)
    return result;
                                 //index 4
  result = ocr_append_fontinfo (lang, FFAM_ROMAN, CHSET_ANSI, PITCH_VAR, "Century Schoolbook");
  if (result != OKAY)
    return result;
                                 //index 5
  result = ocr_append_fontinfo (lang, FFAM_SWISS, CHSET_ANSI, PITCH_VAR, "Gill Sans");
  if (result != OKAY)
    return result;

  result = ocr_setup_startinfo_ansi (1, lang, name, version);
  //if your name is in unicode, do this...
  //result=ocr_setup_startinfo(1,lang,name,version);
  if (result != OKAY)
    return result;

  return OKAY;
}


/**********************************************************************
 * read_image
 *
 * Read the image from the shm and setup the ocr engine on the page.
 **********************************************************************/

INT16 read_image(               //read dummy image info
                 IMAGE *im_out  //output image
                ) {
  INT16 xsize;                   //image size
  INT16 ysize;                   //image size
  INT16 lines_read;              //lines of image
  ESTRIP_DESC *strip;            //strip info
  INT32 roundbytes;              //bytes on a rounded line

                                 //strip info
  strip = ocr_get_first_image_strip ();
  if (strip == NULL) {
    im_out->destroy ();          //no image
    tprintf ("Read termination command");
    return OKAY;                 //but OK
  }
  xsize = strip->x_size;
  ysize = strip->y_size;         //save size
  roundbytes = xsize / 8;
  im_out->create (xsize, ysize, 1);

  //setup OCR engine with page info here
  lines_read = 0;
  do {
    //copy image strip to buffer of whole image
    memcpy (im_out->get_buffer () + lines_read * roundbytes,
    //desination
      strip->data,               //source strip
                                 //size of strip
      strip->strip_size * roundbytes);
                                 //count up lines
    lines_read += strip->strip_size;
    if (lines_read < ysize) {
                                 //next strip from shm
      strip = ocr_get_next_image_strip ();
      if (strip == NULL) {
        tprintf ("Read of next strip failed\n");
        return HPERR;            //read failed
      }
    }
  }
  while (lines_read < ysize);    //read all image
  invert_image(im_out);  //white is 1
  tprintf ("Read image of size (%d,%d)", im_out->get_xsize (),
    im_out->get_ysize ());

  return OKAY;
}


int initialized = 0;

#ifdef __MSW32__
/**********************************************************************
 * WinMain
 *
 * Main function for a windows program.
 **********************************************************************/

int WINAPI WinMain(  //main for windows //command line
                   HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpszCmdLine,
                   int nCmdShow) {
  WNDCLASS wc;
  HWND hwnd;
  MSG msg;

  char **argv;
  char *argsin[2];
  int argc;
  int exit_code;

  wc.style = CS_NOCLOSE | CS_OWNDC;
  wc.lpfnWndProc = (WNDPROC) WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = NULL;               //LoadIcon (NULL, IDI_APPLICATION);
  wc.hCursor = NULL;             //LoadCursor (NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
  wc.lpszMenuName = NULL;
  wc.lpszClassName = szAppName;

  RegisterClass(&wc); 

  hwnd = CreateWindow (szAppName, szAppName,
    WS_OVERLAPPEDWINDOW | WS_DISABLED,
    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
    CW_USEDEFAULT, HWND_DESKTOP, NULL, hInstance, NULL);

  argsin[0] = strdup (szAppName);
  argsin[1] = strdup (lpszCmdLine);
  /*allocate memory for the args. There can never be more than half*/
  /*the total number of characters in the arguments.*/
  argv =
    (char **) malloc (((strlen (argsin[0]) + strlen (argsin[1])) / 2 + 1) *
    sizeof (char *));

  /*now construct argv as it should be for C.*/
  argc = parse_args (2, argsin, argv);

  //  ShowWindow (hwnd, nCmdShow);
  //  UpdateWindow (hwnd);

  if (initialized) {
    exit_code = main (argc, argv);
    return exit_code;
  }
  while (GetMessage (&msg, NULL, 0, 0)) {
    TranslateMessage(&msg); 
    DispatchMessage(&msg); 
    if (initialized) {
      exit_code = main (argc, argv);
      break;
    }
    else
      exit_code = msg.wParam;
  }
  free (argsin[0]);
  free (argsin[1]);
  free(argv); 
  return exit_code;
}


/**********************************************************************
 * WndProc
 *
 * Function to respond to messages.
 **********************************************************************/

LONG WINAPI WndProc(            //message handler
                    HWND hwnd,  //window with message
                    UINT msg,   //message typ
                    WPARAM wParam,
                    LPARAM lParam) {
  HDC hdc;

  if (msg == WM_CREATE) {
    //
    // Create a rendering context.
    //
    hdc = GetDC (hwnd);
    ReleaseDC(hwnd, hdc); 
    initialized = 1;
    return 0;
  }
  return DefWindowProc (hwnd, msg, wParam, lParam);
}


/**********************************************************************
 * parse_args
 *
 * Turn a list of args into a new list of args with each separate
 * whitespace spaced string being an arg.
 **********************************************************************/

int
parse_args (                     /*refine arg list */
int argc,                        /*no of input args */
char *argv[],                    /*input args */
char *arglist[]                  /*output args */
) {
  int argcount;                  /*converted argc */
  char *testchar;                /*char in option string */
  int arg;                       /*current argument */

  argcount = 0;                  /*no of options */
  for (arg = 0; arg < argc; arg++) {
    testchar = argv[arg];        /*start of arg */
    do {
      while (*testchar
        && (*testchar == ' ' || *testchar == '\n'
        || *testchar == '\t'))
        testchar++;              /*skip white space */
      if (*testchar) {
                                 /*new arg */
        arglist[argcount++] = testchar;
                                 /*skip to white space */
        for (testchar++; *testchar && *testchar != ' ' && *testchar != '\n' && *testchar != '\t'; testchar++);
        if (*testchar)
          *testchar++ = '\0';    /*turn to separate args */
      }
    }
    while (*testchar);
  }
  return argcount;               /*new number of args */
}
#endif

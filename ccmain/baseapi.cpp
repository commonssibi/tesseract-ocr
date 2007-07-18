/**********************************************************************
 * File:        baseapi.cpp
 * Description: Simple API for calling tesseract.
 * Author:      Ray Smith
 * Created:     Fri Oct 06 15:35:01 PDT 2006
 *
 * (C) Copyright 2006, Google Inc.
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

#include "baseapi.h"

#include "tessedit.h"
#include "ocrclass.h"
#include "pageres.h"
#include "tessvars.h"
#include "control.h"
#include "applybox.h"
#include "pgedit.h"
#include "varabled.h"
#include "output.h"
#include "adaptmatch.h"

BOOL_VAR(tessedit_resegment_from_boxes, FALSE,
         "Take segmentation and labeling from box file");
BOOL_VAR(tessedit_train_from_boxes, FALSE,
         "Generate training data from boxed chars");

// Minimum sensible image size to be worth running tesseract.
const int kMinRectSize = 10;

static STRING input_file = "noname.tif";

// Start tesseract.
// The datapath must be the name of the data directory or some other file
// in which the data directory resides (for instance argv[0].)
// The configfile is the name of a file in the tessconfigs directory
// (eg batch) or NULL to run on defaults.
// Outputbase may also be NULL, and is the basename of various output files.
// If the output of any of these files is enabled, then a name nmust be given.
// If numeric_mode is true, only possible digits and roman numbers are
// returned. Returns 0 if successful. Crashes if not.
// The argc and argv may be 0 and NULL respectively. They are used for
// providing config files for debug/display purposes.
// TODO(rays) get the facts straight. Is it OK to call
// it more than once? Make it properly check for errors and return them.
int TessBaseAPI::Init(const char* datapath, const char* outputbase,
                      const char* configfile, bool numeric_mode,
                      int argc, char* argv[]) {
  return InitWithLanguage(datapath, outputbase, NULL, configfile,
                          numeric_mode, argc, argv);
}

// Start tesseract.
// Similar to Init() except that it is possible to specify the language.
// Language is the code of the language for which the data will be loaded.
// (Codes follow ISO 639-2.) If it is NULL, english (eng) will be loaded.
int TessBaseAPI::InitWithLanguage(const char* datapath, const char* outputbase,
                                  const char* language, const char* configfile,
                                  bool numeric_mode, int argc, char* argv[]) {
  int result = init_tesseract(datapath, outputbase, language,
                              configfile, argc, argv);
  bln_numericmode.set_value(numeric_mode);
  return result;
}

// Set the name of the input file. Needed only for training and
// loading a UNLV zone file.
void TessBaseAPI::SetInputName(const char* name) {
  input_file = name;
}

// Recognize a rectangle from an image and return the result as a string.
// May be called many times for a single Init.
// Currently has no error checking.
// Greyscale of 8 and color of 24 or 32 bits per pixel may be given.
// Palette color images will not work properly and must be converted to
// 24 bit.
// Binary images of 1 bit per pixel may also be given but they must be
// byte packed with the MSB of the first byte being the first pixel, and a
// one pixel is WHITE. For binary images set bytes_per_pixel=0.
// The recognized text is returned as a char* which (in future will be coded
// as UTF8 and) must be freed with the delete [] operator.
char* TessBaseAPI::TesseractRect(const unsigned char* imagedata,
                                 int bytes_per_pixel,
                                 int bytes_per_line,
                                 int left, int top,
                                 int width, int height) {
  if (width < kMinRectSize || height < kMinRectSize)
    return NULL;  // Nothing worth doing.

  // Copy/Threshold the image to the tesseract global page_image.
  CopyImageToTesseract(imagedata, bytes_per_pixel, bytes_per_line,
                       left, top, width, height);

  return RecognizeToString();
}

// As TesseractRect but produces a box file as output.
char* TessBaseAPI::TesseractRectBoxes(const unsigned char* imagedata,
                                      int bytes_per_pixel,
                                      int bytes_per_line,
                                      int left, int top,
                                      int width, int height,
                                      int imageheight) {
  if (width < kMinRectSize || height < kMinRectSize)
  return NULL;  // Nothing worth doing.

  // Copy/Threshold the image to the tesseract global page_image.
  CopyImageToTesseract(imagedata, bytes_per_pixel, bytes_per_line,
                       left, top, width, height);

  BLOCK_LIST    block_list;

  FindLines(&block_list);

  // Now run the main recognition.
  PAGE_RES* page_res = Recognize(&block_list, NULL);

  return TesseractToBoxText(page_res, left, imageheight - (top + height));
}

char* TessBaseAPI::TesseractRectUNLV(const unsigned char* imagedata,
                                     int bytes_per_pixel,
                                     int bytes_per_line,
                                     int left, int top,
                                     int width, int height) {
  if (width < kMinRectSize || height < kMinRectSize)
    return NULL;  // Nothing worth doing.

  // Copy/Threshold the image to the tesseract global page_image.
  CopyImageToTesseract(imagedata, bytes_per_pixel, bytes_per_line,
                       left, top, width, height);

  BLOCK_LIST    block_list;

  FindLines(&block_list);

  // Now run the main recognition.
  PAGE_RES* page_res = Recognize(&block_list, NULL);

  return TesseractToUNLV(page_res);
}

// Call between pages or documents etc to free up memory and forget
// adaptive data.
void TessBaseAPI::ClearAdaptiveClassifier() {
  ResetAdaptiveClassifier();
}

// Close down tesseract and free up memory.
void TessBaseAPI::End() {
  ResetAdaptiveClassifier();
  end_tesseract();
}

// Dump the internal binary image to a PGM file.
void TessBaseAPI::DumpPGM(const char* filename) {
  IMAGELINE line;
  line.init(page_image.get_xsize());
  FILE *fp = fopen(filename, "w");
  fprintf(fp, "P5 " INT32FORMAT " " INT32FORMAT " 255\n", page_image.get_xsize(),
          page_image.get_ysize());
  for (int j = page_image.get_ysize()-1; j >= 0 ; --j) {
    page_image.get_line(0, j, page_image.get_xsize(), &line, 0);
    for (int i = 0; i < page_image.get_xsize(); ++i) {
      UINT8 b = line.pixels[i] ? 255 : 0;
      fwrite(&b, 1, 1, fp);
    }
  }
  fclose(fp);
}

// Copy the given image rectangle to Tesseract, with adaptive thresholding
// if the image is not already binary.
void TessBaseAPI::CopyImageToTesseract(const unsigned char* imagedata,
                                       int bytes_per_pixel,
                                       int bytes_per_line,
                                       int left, int top,
                                       int width, int height) {
  if (bytes_per_pixel > 0) {
    // Threshold grey or color.
    int* thresholds = new int[bytes_per_pixel];
    int* hi_values = new int[bytes_per_pixel];

    // Compute the thresholds.
    OtsuThreshold(imagedata, bytes_per_pixel, bytes_per_line,
                  left, top, left + width, top + height,
                  thresholds, hi_values);

    // Threshold the image to the tesseract global page_image.
    ThresholdRect(imagedata, bytes_per_pixel, bytes_per_line,
                  left, top, width, height,
                  thresholds, hi_values);
    delete [] thresholds;
    delete [] hi_values;
  } else {
    CopyBinaryRect(imagedata, bytes_per_line, left, top, width, height);
  }
}

// Compute the Otsu threshold(s) for the given image rectangle, making one
// for each channel. Each channel is always one byte per pixel.
// Returns an array of threshold values and an array of hi_values, such
// that a pixel value >threshold[channel] is considered foreground if
// hi_values[channel] is 0 or background if 1. A hi_value of -1 indicates
// that there is no apparent foreground. At least one hi_value will not be -1.
// thresholds and hi_values are assumed to be of bytes_per_pixel size.
void TessBaseAPI::OtsuThreshold(const unsigned char* imagedata,
                                int bytes_per_pixel,
                                int bytes_per_line,
                                int left, int top, int right, int bottom,
                                int* thresholds,
                                int* hi_values) {
  // Of all channels with no good hi_value, keep the best so we can always
  // produce at least one answer.
  int best_hi_value = 0;
  int best_hi_index = 0;
  bool any_good_hivalue = false;
  double best_hi_dist = 0.0;

  for (int ch = 0; ch < bytes_per_pixel; ++ch) {
    thresholds[ch] = 0;
    hi_values[ch] = -1;
    // Compute the histogram of the image rectangle.
    int histogram[256];
    HistogramRect(imagedata + ch, bytes_per_pixel, bytes_per_line,
                  left, top, right, bottom, histogram);
    int H;
    int best_omega_0;
    int best_t = OtsuStats(histogram, &H, &best_omega_0);
    // To be a convincing foreground we must have a small fraction of H
    // or to be a convincing background we must have a large fraction of H.
    // In between we assume this channel contains no thresholding information.
    int hi_value = best_omega_0 < H * 0.5;
    thresholds[ch] = best_t;
    if (best_omega_0 > H * 0.75) {
      any_good_hivalue = true;
      hi_values[ch] = 0;
    }
    else if (best_omega_0 < H * 0.25) {
      any_good_hivalue = true;
      hi_values[ch] = 1;
    }
    else {
      // In case all channels are like this, keep the best of the bad lot.
      double hi_dist = hi_value ? (H - best_omega_0) : best_omega_0;
      if (hi_dist > best_hi_dist) {
        best_hi_dist = hi_dist;
        best_hi_value = hi_value;
        best_hi_index = ch;
      }
    }
  }
  if (!any_good_hivalue) {
    // Use the best of the ones that were not good enough.
    hi_values[best_hi_index] = best_hi_value;
  }
}

// Compute the histogram for the given image rectangle, and the given
// channel. (Channel pointed to by imagedata.) Each channel is always
// one byte per pixel.
// Bytes per pixel is used to skip channels not being
// counted with this call in a multi-channel (pixel-major) image.
// Histogram is always a 256 element array to count occurrences of
// each pixel value.
void TessBaseAPI::HistogramRect(const unsigned char* imagedata,
                                int bytes_per_pixel,
                                int bytes_per_line,
                                int left, int top, int right, int bottom,
                                int* histogram) {
  int width = right - left;
  memset(histogram, 0, sizeof(*histogram) * 256);
  const unsigned char* pix = imagedata +
                             top*bytes_per_line +
                             left*bytes_per_pixel;
  for (int y = top; y < bottom; ++y) {
    for (int x = 0; x < width; ++x) {
      ++histogram[pix[x * bytes_per_pixel]];
    }
    pix += bytes_per_line;
  }
}

// Compute the Otsu threshold(s) for the given histogram.
// Also returns H = total count in histogram, and
// omega0 = count of histogram below threshold.
int TessBaseAPI::OtsuStats(const int* histogram,
                           int* H_out,
                           int* omega0_out) {
  int H = 0;
  double mu_T = 0.0;
  for (int i = 0; i < 256; ++i) {
    H += histogram[i];
    mu_T += i * histogram[i];
  }

  // Now maximize sig_sq_B over t.
  // http://www.ctie.monash.edu.au/hargreave/Cornall_Terry_328.pdf
  int best_t = -1;
  int omega_0, omega_1;
  int best_omega_0 = 0;
  double best_sig_sq_B = 0.0;
  double mu_0, mu_1, mu_t;
  omega_0 = 0;
  mu_t = 0.0;
  for (int t = 0; t < 255; ++t) {
    omega_0 += histogram[t];
    mu_t += t * static_cast<double>(histogram[t]);
    if (omega_0 == 0)
      continue;
    omega_1 = H - omega_0;
    mu_0 = mu_t / omega_0;
    mu_1 = (mu_T - mu_t) / omega_1;
    double sig_sq_B = mu_1 - mu_0;
    sig_sq_B *= sig_sq_B * omega_0 * omega_1;
    if (best_t < 0 || sig_sq_B > best_sig_sq_B) {
      best_sig_sq_B = sig_sq_B;
      best_t = t;
      best_omega_0 = omega_0;
    }
  }
  if (H_out != NULL) *H_out = H;
  if (omega0_out != NULL) *omega0_out = best_omega_0;
  return best_t;
}

// Threshold the given grey or color image into the tesseract global
// image ready for recognition. Requires thresholds and hi_value
// produced by OtsuThreshold above.
void TessBaseAPI::ThresholdRect(const unsigned char* imagedata,
                                int bytes_per_pixel,
                                int bytes_per_line,
                                int left, int top,
                                int width, int height,
                                const int* thresholds,
                                const int* hi_values) {
  IMAGELINE line;
  page_image.create(width, height, 1);
  line.init(width);
  // For each line in the image, fill the IMAGELINE class and put it into the
  // Tesseract global page_image. Note that Tesseract stores images with the
  // bottom at y=0 and 0 is black, so we need 2 kinds of inversion.
  const unsigned char* data = imagedata + top*bytes_per_line +
                              left*bytes_per_pixel;
  for (int y = height - 1 ; y >= 0; --y) {
    const unsigned char* pix = data;
    for (int x = 0; x < width; ++x, pix += bytes_per_pixel) {
      line.pixels[x] = 1;
      for (int ch = 0; ch < bytes_per_pixel; ++ch) {
        if (hi_values[ch] >= 0 &&
            (pix[ch] > thresholds[ch]) == (hi_values[ch] == 0)) {
          line.pixels[x] = 0;
          break;
        }
      }
    }
    page_image.put_line(0, y, width, &line, 0);
    data += bytes_per_line;
  }
}

// Cut out the requested rectangle of the binary image to the
// tesseract global image ready for recognition.
void TessBaseAPI::CopyBinaryRect(const unsigned char* imagedata,
                                 int bytes_per_line,
                                 int left, int top,
                                 int width, int height) {
  // Copy binary image, cutting out the required rectangle.
  IMAGE image;
  image.capture(const_cast<unsigned char*>(imagedata),
                bytes_per_line*8, top + height, 1);
  page_image.create(width, height, 1);
  copy_sub_image(&image, left, 0, width, height, &page_image, 0, 0, false);
}

// Low-level function to recognize the current global image to a string.
char* TessBaseAPI::RecognizeToString() {
  BLOCK_LIST    block_list;

  FindLines(&block_list);

  // Now run the main recognition.
  PAGE_RES* page_res = Recognize(&block_list, NULL);

  return TesseractToText(page_res);
}

// Find lines from the image making the BLOCK_LIST.
void TessBaseAPI::FindLines(BLOCK_LIST* block_list) {
  // The following call creates a full-page block and then runs connected
  // component analysis and text line creation.
  pgeditor_read_file(input_file, block_list);
}

// Recognize the tesseract global image and return the result as Tesseract
// internal structures.
PAGE_RES* TessBaseAPI::Recognize(BLOCK_LIST* block_list, ETEXT_DESC* monitor) {
  if (tessedit_resegment_from_boxes)
    apply_boxes(block_list);
  if (edit_variables)
    start_variables_editor();

  PAGE_RES* page_res = new PAGE_RES(block_list);
  if (interactive_mode) {
    pgeditor_main(block_list);                  //pgeditor user I/F
  } else if (tessedit_train_from_boxes) {
    apply_box_training(block_list);
  } else {
    // Now run the main recognition.
    recog_all_words(page_res, monitor);
  }
  return page_res;
}

// Return the maximum length that the output text string might occupy.
int TessBaseAPI::TextLength(PAGE_RES* page_res) {
  PAGE_RES_IT   page_res_it(page_res);
  int total_length = 2;
  // Iterate over the data structures to extract the recognition result.
  for (page_res_it.restart_page(); page_res_it.word () != NULL;
       page_res_it.forward()) {
    WERD_RES *word = page_res_it.word();
    WERD_CHOICE* choice = word->best_choice;
    if (choice != NULL) {
      total_length += choice->string().length() + 1;
      for (int i = 0; i < word->reject_map.length(); ++i) {
        if (word->reject_map[i].rejected())
          ++total_length;
      }
    }
  }
  return total_length;
}

// Make a text string from the internal data structures.
// The input page_res is deleted.
char* TessBaseAPI::TesseractToText(PAGE_RES* page_res) {
  if (page_res != NULL) {
    int total_length = TextLength(page_res);
    PAGE_RES_IT   page_res_it(page_res);
    char* result = new char[total_length];
    char* ptr = result;
    for (page_res_it.restart_page(); page_res_it.word () != NULL;
         page_res_it.forward()) {
      WERD_RES *word = page_res_it.word();
      WERD_CHOICE* choice = word->best_choice;
      if (choice != NULL) {
        strcpy(ptr, choice->string().string());
        ptr += strlen(ptr);
        if (word->word->flag(W_EOL))
          *ptr++ = '\n';
        else
          *ptr++ = ' ';
      }
    }
    *ptr++ = '\n';
    *ptr = '\0';
    delete page_res;
    return result;
  }
  return NULL;
}

static int ConvertWordToBoxText(WERD_RES *word,
                                ROW_RES* row,
                                int left,
                                int bottom,
                                char* word_str) {
  // Copy the output word and denormalize it back to image coords.
  WERD copy_outword;
  copy_outword = *(word->outword);
  copy_outword.baseline_denormalise(&word->denorm);
  PBLOB_IT blob_it;
  blob_it.set_to_list(copy_outword.blob_list());
  int length = copy_outword.blob_list()->length();
  int output_size = 0;

  if (length > 0) {
    for (int index = 0, offset = 0; index < length;
         offset += word->best_choice->lengths()[index++], blob_it.forward()) {
      PBLOB* blob = blob_it.data();
      BOX blob_box = blob->bounding_box();
      if (word->tess_failed ||
          blob_box.left() < 0 ||
          blob_box.right() > page_image.get_xsize() ||
          blob_box.bottom() < 0 ||
          blob_box.top() > page_image.get_ysize()) {
        // Bounding boxes can be illegal when tess fails on a word.
        blob_box = word->word->bounding_box();  // Use original word as backup.
        tprintf("Using substitute bounding box at (%d,%d)->(%d,%d)\n",
                blob_box.left(), blob_box.bottom(),
                blob_box.right(), blob_box.top());
      }

      // A single classification unit can be composed of several UTF-8
      // characters. Append each of them to the result.
      for (int sub = 0; sub < word->best_choice->lengths()[index]; ++sub) {
        char ch = word->best_choice->string()[offset + sub];
        // Tesseract uses space for recognition failure. Fix to a reject
        // character, '~' so we don't create illegal box files.
        if (ch == ' ')
          ch = '~';
        word_str[output_size++] = ch;
      }
      sprintf(word_str + output_size, " %d %d %d %d\n",
              blob_box.left() + left, blob_box.bottom() + bottom,
              blob_box.right() + left, blob_box.top() + bottom);
      output_size += strlen(word_str + output_size);
    }
  }
  return output_size;
}

// Multiplier for textlength assumes 4 numbers @ 5 digits and a space
// plus the newline and the orginial character = 4*(5+1)+2
const int kMaxCharsPerChar = 26;

// Make a text string from the internal data structures.
// The input page_res is deleted.
// The text string takes the form of a box file as needed for training.
char* TessBaseAPI::TesseractToBoxText(PAGE_RES* page_res,
                                      int left, int bottom) {
  if (page_res != NULL) {
    int total_length = TextLength(page_res) * kMaxCharsPerChar;
    PAGE_RES_IT   page_res_it(page_res);
    char* result = new char[total_length];
    char* ptr = result;
    for (page_res_it.restart_page(); page_res_it.word () != NULL;
         page_res_it.forward()) {
      WERD_RES *word = page_res_it.word();
      ptr += ConvertWordToBoxText(word,page_res_it.row(),left, bottom, ptr);
    }
    *ptr = '\0';
    delete page_res;
    return result;
  }
  return NULL;
}

// Make a text string from the internal data structures.
// The input page_res is deleted. The text string is converted
// to UNLV-format: Latin-1 with specific reject and suspect codes.
const char kUnrecognized = '~';
// Conversion table for non-latin characters.
// Maps characters out of the latin set into the latin set.
// TODO(rays) incorporate this translation into unicharset.
const int kUniChs[] = {
  0x20ac, 0x201c, 0x201d, 0x2018, 0x2019, 0x2022, 0x2014, 0
};
// Latin chars corresponding to the unicode chars above.
const int kLatinChs[] = {
  0x00a2, 0x0022, 0x0022, 0x0027, 0x0027, 0x00b7, 0x002d, 0
};

char* TessBaseAPI::TesseractToUNLV(PAGE_RES* page_res) {
  bool tilde_crunch_written = false;
  bool last_char_was_newline = true;
  bool last_char_was_tilde = false;

  if (page_res != NULL) {
    int total_length = TextLength(page_res);
    PAGE_RES_IT   page_res_it(page_res);
    char* result = new char[total_length];
    char* ptr = result;
    for (page_res_it.restart_page(); page_res_it.word () != NULL;
         page_res_it.forward()) {
      WERD_RES *word = page_res_it.word();
      // Process the current word.
      if (word->unlv_crunch_mode != CR_NONE) {
        if (word->unlv_crunch_mode != CR_DELETE &&
            (!tilde_crunch_written ||
             (word->unlv_crunch_mode == CR_KEEP_SPACE &&
              word->word->space () > 0 &&
              !word->word->flag (W_FUZZY_NON) &&
              !word->word->flag (W_FUZZY_SP)))) {
          if (!word->word->flag (W_BOL) &&
              word->word->space () > 0 &&
              !word->word->flag (W_FUZZY_NON) &&
              !word->word->flag (W_FUZZY_SP)) {
            /* Write a space to separate from preceeding good text */
            *ptr++ = ' ';
            last_char_was_tilde = false;
          }
          if (!last_char_was_tilde) {
            // Write a reject char.
            last_char_was_tilde = true;
            *ptr++ = kUnrecognized;
            tilde_crunch_written = true;
            last_char_was_newline = false;
          }
        }
      } else {
        // NORMAL PROCESSING of non tilde crunched words.
        tilde_crunch_written = false;

        if (last_char_was_tilde &&
            word->word->space () == 0 &&
            (word->best_choice->string ()[0] == ' ')) {
          /* Prevent adjacent tilde across words - we know that adjacent tildes within
             words have been removed */
          char* p = (char *) word->best_choice->string().string ();
          strcpy (p, p + 1);       //shuffle up
          p = (char *) word->best_choice->lengths().string ();
          strcpy (p, p + 1);       //shuffle up
          word->reject_map.remove_pos (0);
          PBLOB_IT blob_it = word->outword->blob_list ();
          delete blob_it.extract ();   //get rid of reject blob
        }

        if (word->word->flag(W_REP_CHAR) && tessedit_consistent_reps)
          ensure_rep_chars_are_consistent(word);

        set_unlv_suspects(word);
        const char* wordstr = word->best_choice->string().string();
        if (wordstr[0] != 0) {
          if (!last_char_was_newline)
            *ptr++ = ' ';
          else
            last_char_was_newline = false;
          int offset = 0;
          const STRING& lengths = word->best_choice->lengths();
          int length = lengths.length();
          for (int i = 0; i < length; offset += lengths[i++]) {
            if (wordstr[offset] == ' ' ||
                wordstr[offset] == '~' ||
                wordstr[offset] == '|') {
              *ptr++ = kUnrecognized;
              last_char_was_tilde = true;
            } else {
              if (word->reject_map[i].rejected())
                *ptr++ = '^';
              UNICHAR ch(wordstr + offset, lengths[i]);
              int uni_ch = ch.first_uni();
              for (int j = 0; kUniChs[j] != 0; ++j) {
                if (kUniChs[j] == uni_ch) {
                  uni_ch = kLatinChs[j];
                  break;
                }
              }
              if (uni_ch <= 0xff) {
                *ptr++ = static_cast<char>(uni_ch);
                last_char_was_tilde = false;
              } else {
                *ptr++ = kUnrecognized;
                last_char_was_tilde = true;
              }
            }
          }
        }
      }
      if (word->word->flag(W_EOL) && !last_char_was_newline) {
        /* Add a new line output */
        *ptr++ = '\n';
        tilde_crunch_written = false;
        last_char_was_newline = true;
        last_char_was_tilde = false;
      }
    }
    *ptr++ = '\n';
    *ptr = '\0';
    delete page_res;
    return result;
  }
  return NULL;
}


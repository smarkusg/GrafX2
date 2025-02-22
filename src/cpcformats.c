/* vim:expandtab:ts=2 sw=2:
*/
/*  Grafx2 - The Ultimate 256-color bitmap paint program

	Copyright owned by various GrafX2 authors, see COPYRIGHT.txt for details.

    Grafx2 is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; version 2
    of the License.

    Grafx2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Grafx2; if not, see <http://www.gnu.org/licenses/>
*/

///@file cpcformats.c
/// Formats for the Amstrad CPC / CPC Plus computers

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "engine.h"
#include "global.h"
#include "fileformats.h"
#include "input.h"
#include "io.h"
#include "loadsavefuncs.h"
#include "libraw2crtc.h"
#include "oldies.h"
#include "readline.h"
#include "windows.h"
#include "misc.h"
#include "gfx2mem.h"
#include "gfx2log.h"
#include "screen.h"
#include "windows.h"

#ifndef MIN
#define MIN(a,b) (((a)<(b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) (((a)>(b)) ? (a) : (b))
#endif

/**
 * @defgroup cpcformats Amstrad CPC/CPC+ picture formats
 * @ingroup loadsaveformats
 *
 * Support for Amstrad CPC/CPC+ picture formats. The Amstrad CPC has
 * 3 video modes :
 * - mode 0 : 160x200 16 colors
 * - mode 1 : 320x200 4 colors
 * - mode 2 : 640x200 2 colors
 *
 * Supported formats :
 * - GO1/GO2 : GraphOS
 * - SCR : OCP Art Studio / iMPdraw v2 / etc.
 * - CM5 : Mode 5 Viewer
 * - PPH : Perfect Pix
 * - SGX : SymbOS graphic files
 *
 * @{
 */

/**
 * Test for SGX file (SymbOS)
 *
 * http://www.cpcwiki.eu/index.php/Format:SGX_(SymbOS_graphic_files)
 */
void Test_SGX(T_IO_Context * context, FILE * file)
{
  byte header[8];
  (void)context;

  if (Read_bytes(file, header, 8))
  {
    unsigned long file_size = File_length_file(file);
    if (header[0] > 0 && header[0] < 64
        && ((header[1] + 3) >> 2) == header[0]
        && file_size >= (3 + (unsigned long)header[0] * (unsigned long)header[2]))
    {
      // Simple 4 colour graphic
      File_error = 0;
    }
    else if (header[0] == 64 && (header[1] == 0 || header[1] == 5))
    {
      // extended
      word byte_width = header[2] | ((word)header[3] << 8);
      word pixel_width = header[4] | ((word)header[5] << 8);
      word height = header[6] | ((word)header[7] << 8);
      if (height == 0 || byte_width == 0 || byte_width > 255) return;
      if (header[1] == 0 && byte_width != ((pixel_width + 3) >> 2)) return;
      if (header[1] == 5 && byte_width != ((pixel_width + 1) >> 1)) return;
      File_error = 0;
    }
  }
}

static void Set_SGX_Palette(T_IO_Context * context)
{
  static const byte sgx_palette[] = {
    0xff, 0xff, 0x80, // 0
    0x00, 0x00, 0x00,
    0xff, 0x80, 0x00,
    0x80, 0x00, 0x00,
    0x00, 0xff, 0xff, // 4
    0x00, 0x00, 0x80,
    0x80, 0x80, 0xff,
    0x80, 0x00, 0xff,
    0xff, 0xff, 0xff, // 8
    0x00, 0x80, 0x00,
    0x00, 0xff, 0x00,
    0xff, 0x00, 0xff,
    0xff, 0xff, 0x00, // 12
    0x80, 0x80, 0x80,
    0xff, 0x80, 0x80,
    0xff, 0x00, 0x00
  };
  if (Config.Clear_palette)
    memset(context->Palette,0,sizeof(T_Palette));
  memcpy(context->Palette, sgx_palette, sizeof(sgx_palette));
}

/**
 * Structure for current information about loading SGX file
 */
struct sgx_data {
  T_IO_Context * context;
  FILE * file;
  unsigned long file_size;
  word width, height;
  byte ncolors;
};

/**
 * Callback function for Read_SGX()
 */
typedef int (*Read_SGX_Callback)(struct sgx_data *, word, word, word, word, word, byte);

/**
 * Read a SGX file structure.
 *
 * @return 0 for error
 */
static int Read_SGX(struct sgx_data * data, Read_SGX_Callback cb)
{
  word posx = 0;
  word posy = 0;
  byte b;

  data->width = 0;
  data->height = 0;
  while (Read_byte(data->file, &b))
  {
    if (b == 0)
    {
      // EOF
      GFX2_Log(GFX2_DEBUG, "SGX EOF\n");
      return 1;
    }
    else if (b == 255)
    {
      GFX2_Log(GFX2_DEBUG, "SGX LF\n");
      // skip 2 bytes
      if (!(Read_byte(data->file, &b) && Read_byte(data->file, &b)))
        return 0;
      posx = 0;
      posy = data->height;
    }
    else if (b < 64)
    {
      byte byte_width, width, height;
      byte_width = b;
      if (!(Read_byte(data->file, &width) && Read_byte(data->file, &height)))
        return 0;
      GFX2_Log(GFX2_DEBUG, "SGX Simple 4c : %dx%d\n", (int)width, (int)height);
      if (!cb(data, posx, posy, width, height, byte_width, 4))
        return 0;
      posx += width;
      data->width = MAX(data->width, posx);
      data->height = MAX(data->height, posy + height);
      if (data->ncolors == 0)
        data->ncolors = 4;
    }
    else if (b == 64)
    {
      byte type;
      word byte_width, width, height;
      if (!(Read_byte(data->file, &type) && Read_word_le(data->file, &byte_width)
            && Read_word_le(data->file, &width) && Read_word_le(data->file, &height)))
        return 0;
      GFX2_Log(GFX2_DEBUG, "SGX Extended %dc : %dx%d\n", type == 0 ? 4 : 16, (int)width, (int)height);
      if (!cb(data, posx, posy, width, height, byte_width, type == 0 ? 4 : 16))
        return 0;
      posx += width;
      data->width = MAX(data->width, posx);
      data->height = MAX(data->height, posy + height);
      data->ncolors = MAX(data->ncolors, type == 0 ? 4 : 16);
    }
    else
    {
      GFX2_Log(GFX2_WARNING, "SGX : unrecognized chunk. Byte 0x%02x at offset 0x%06x\n", b, ftell(data->file) - 1);
    }
  }
  return 1; // it is OK if the file does finish with a EOF mark
}

/**
 * Only skip bytes
 */
int SGX_Get_dimensions(struct sgx_data * data, word posx, word posy, word width, word height, word byte_width, byte ncolors)
{
  (void)posx;
  (void)posy;
  (void)width;
  (void)ncolors;
  if (fseek(data->file, (long)byte_width * height, SEEK_CUR) < 0)
    return 0;
  return 1;
}

/**
 * Set the pixels
 * @return 0 in case of error
 */
int SGX_Load_Picture(struct sgx_data * data, word posx, word posy, word width, word height, word byte_width, byte ncolors)
{
  word y;

  for (y = posy; y < (posy + height); y++)
  {
    word i, x;
    for (i = 0, x = posx; i < byte_width; i++)
    {
      byte b;
      if (!Read_byte(data->file, &b))
        return 0;
      if (ncolors == 4)
      {
        do {
          // upper nibble is 4 lower color bits, lower nibble is 4 upper color bits
          Set_pixel(data->context, x++, y, (b & 0x80) >> 7 | (b & 0x08) >> 2);
          b <<= 1;
        }
        while (((x ^ posx) & 3) && (x < (posx + width)));
      }
      else
      {
        Set_pixel(data->context, x++, y, b >> 4);
        if (x < (posx + width))
          Set_pixel(data->context, x++, y, b & 0x0f);
      }
    }
  }
  return 1;
}

void Load_SGX(T_IO_Context * context)
{
  struct sgx_data data = { context, NULL, 0, 0, 0, 0 };

  File_error = 1;
  data.file = Open_file_read(context);
  if (data.file == NULL) return;
  data.file_size = File_length_file(data.file);
  if (Read_SGX(&data, SGX_Get_dimensions))
  {
    GFX2_Log(GFX2_DEBUG, "SGX total dimensions : %ux%u, %d colors\n",
             (unsigned)data.width, (unsigned)data.height, (int)data.ncolors);
    File_error = 0;
    Pre_load(context, data.width, data.height, data.file_size, FORMAT_SGX, PIXEL_SIMPLE, data.ncolors == 16 ? 4 : 2);
    if (File_error == 0)
    {
      if (fseek(data.file, 0, SEEK_SET) < 0 || !Read_SGX(&data, SGX_Load_Picture))
        File_error = 1;
      else
        Set_SGX_Palette(context);
    }
  }
  fclose(data.file);
}

/**
 * Write SGX file format as specified here :
 * http://www.cpcwiki.eu/index.php/Format:SGX_(SymbOS_graphic_files)
 *
 * We are using only simple chunks for 4 colors pictures.
 * Chunk dimentions are limited to the ones of simple chunks (252x255),
 * we could go up to 508x65535 with extended chunks.
 *
 * @return 0 for error
 */
static int Save_SGX_Sub(T_IO_Context * context, FILE * file, word n_colors)
{
  word posy = 0;

  while (posy < context->Height)
  {
    word posx = 0;
    word height = MIN(context->Height - posy, 255);
    if (posy != 0)
    {
      // Write Line Feed
      if (!Write_byte(file, 255)
          || !Write_byte(file, 255) || !Write_byte(file, 255))
        return 0;
    }
    while (posx < context->Width)
    {
      word width = MIN(context->Width - posx, 252);
      GFX2_Log(GFX2_DEBUG, "Save_SGX : (%hu,%hu) %hux%hu %huc\n",
               posx, posy, width, height, n_colors);
      if (n_colors == 4)
      {
        word y;
        byte byte_width = (width + 3) >> 2;
        if (!Write_byte(file, byte_width)
            || !Write_byte(file, (byte)width) || !Write_byte(file, (byte)height))
          return 0;
        for (y = 0; y < height; y++)
        {
          byte i;
          word x;
          for (i = 0, x = 0; i < byte_width; i++)
          {
            byte b = 0;
            do
            {
              byte c = Get_pixel(context, posx + x++, posy + y);
              b <<= 1;
              b |= (c & 1) << 4;
              b |= (c & 2) >> 1;
            } while (x & 3);
            if (!Write_byte(file, b))
              return 0;
          }
        }
      }
      else
      {
        word y;
        word byte_width = (width + 1) >> 1;
        if (!Write_byte(file, 64) || !Write_byte(file, 5)
            || !Write_word_le(file, byte_width) || !Write_word_le(file, width) || !Write_word_le(file, height))
          return 0;
        for (y = 0; y < height; y++)
        {
          word i, x;
          for (i = 0, x = 0; i < byte_width; i++)
          {
            byte b = Get_pixel(context, posx + x++, posy + y) << 4;
            b |= Get_pixel(context, posx + x++, posy + y) & 0x0f;
            if (!Write_byte(file, b))
              return 0;
          }
        }
      }
      posx += width;
    }
    posy += height;
  }
  // Write EOF marker
  for (posy = 0; posy < 3; posy++)
    Write_byte(file, 0);
  return 1;
}

void Save_SGX(T_IO_Context * context)
{
  FILE * file;
  dword color_usage[256];
  word n_colors;
  int i;

  File_error =  1;
  n_colors = Current_layer_count_used_colors(context, color_usage);
  for (i = 16; i < 256; i++)
  {
    if (color_usage[i] != 0)
    {
      Warning_message("SGX format is limited to 16 colors");
      return;
    }
  }
  // 4 colors mode if sufficient, otherwise 16 colors
  n_colors = 4;
  for (i = 4; i < 16; i++)
  {
    if (color_usage[i] != 0)
    {
      n_colors = 16;
      break;
    }
  }
  file = Open_file_write(context);
  if (file == NULL)
    return;
  if (Save_SGX_Sub(context, file, n_colors))
    File_error = 0;
  fclose(file);
}

/**
 * Test for SCR file (Amstrad CPC)
 *
 * SCR file format is originally from "Advanced OCP Art Studio" :
 * http://www.cpcwiki.eu/index.php/Format:Advanced_OCP_Art_Studio_File_Formats
 *
 * .WIN "window" format is also supported.
 *
 * SCR files are normally just a dump of the 16K of video memory. So they are
 * essentially 16 kilobytes of pixel data without any header. To make things
 * more fun, there is an optional compression. This all makes detection a bit
 * fuzzy. However there are various things we can still check:
 *
 * - Presence of a valid PAL file. If the PAL file is not there the pixel data
 *   may still be valid. The PAL file size depends on the screen mode (number
 *   of colors).
 * - An AMSDOS header is a good indication but in some cases it may not
 *   be there.
 * - Some tools embed the palette and mode (and usually some kind of loader
 *   code) in the SCR file, we can also detect these.
 */
void Test_SCR(T_IO_Context * context, FILE * file)
{
  // http://orgams.wikidot.com/le-format-impdraw-v2
  // http://orgams.wikidot.com/les-fichiers-win-compatibles-ocp-art-studio
  FILE * pal_file;
  unsigned long pal_size, file_size;
  byte mode, color_anim_flag;
  word loading_address = 0;
  word exec_address = 0;

  File_error = 1;

  if (CPC_check_AMSDOS(file, &loading_address, &exec_address, &file_size))
  {
    if (loading_address == 0x170) // iMPdraw v2
    {
      byte buffer[0x90];
      fseek(file, 128, SEEK_SET); // right after AMSDOS header
      Read_bytes(file, buffer, 0x90);
      GFX2_LogHexDump(GFX2_DEBUG, "", buffer, 0, 0x90);
      File_error = 0;
      return;
    }
    else if (loading_address == 0x200 && (exec_address == 0x811 || exec_address == 0x821) && file_size > 16000)
    {
      // convimgcpc
      File_error = 0;
      return;
    }
    else if (loading_address == 0x6000 && exec_address == 0x6000)
    {
      File_error = 0;
      return;
    }
    else if (loading_address == 0xc000 && file_size > 16000)
    {
      File_error = 0;
      return;
    }
    else if (loading_address == 0x0040 && exec_address == 0x8000)
    {
      File_error = 0;
      return;
    }
    else if (loading_address == 0xc000 || loading_address == 0x0200)
    {
      byte buffer[4];
      fseek(file, 128, SEEK_SET); // right after AMSDOS header
      Read_bytes(file, buffer, 4);
      // ConvImgCPC "LZW" packed pictures. Signatures :
      // PKSL -> 320x200 STD
      // PKS3 -> 320x200 Mode 3
      // PKSP -> 320x200 PLUS
      // PKVL -> Overscan STD
      // PKVP -> Overscan PLUS
      if (buffer[0] == 'P' && buffer[1] == 'K'
          && (buffer[2] == 'S' || buffer[2] == 'V')
          && (buffer[3] == 'L' || buffer[3] == 'P'))
      {
        File_error = 0;
        return;
      }
    }
  }
  else
    file_size = File_length_file(file);

  if (file_size > 16384*2)
    return;

  // requires the PAL file
  pal_file = Open_file_read_with_alternate_ext(context, "pal");
  if (pal_file == NULL)
    return;
  /** @todo the palette data can be hidden in the 48 "empty" bytes
   * every 2048 bytes of a standard resolution SCR file.
   * So we should detect the hidden Z80 code and load them.
   * Load address of file is C000. Z80 code :<br>
   * <tt>C7D0: 3a d0 d7 cd 1c bd 21 d1 d7 46 48 cd 38 bc af 21 | :.....!..FH.8..!</tt><br>
   * <tt>C7E0: d1 d7 46 48 f5 e5 cd 32 bc e1 f1 23 3c fe 10 20 | ..FH...2...#<.. </tt><br>
   * <tt>C7F0: f1 c3 18 bb 00 00 00 00 00 00 00 00 00 00 00 00 | ................</tt><br>
   * mode and palette :<br>
   * <tt>D7D0: 00 1a 00 0c 03 0b 01 0d 17 10 02 0f 09 19 06 00 | ................</tt><br>
   * https://gitlab.com/GrafX2/grafX2/merge_requests/121#note_119964168
   */


  if (CPC_check_AMSDOS(pal_file, NULL, NULL, &pal_size))
    fseek(pal_file, 128, SEEK_SET); // right after AMSDOS header
  else
  {
    pal_size = File_length_file(pal_file);
    fseek(pal_file, 0, SEEK_SET);
  }

  if (pal_size != 239)
  {
    fclose(pal_file);
    return;
  }

  if (!Read_byte(pal_file, &mode) || !Read_byte(pal_file, &color_anim_flag))
  {
    fclose(pal_file);
    return;
  }
  GFX2_Log(GFX2_DEBUG, "Test_SCR() mode=%d color animation flag %02X\n", mode, color_anim_flag);
  if (mode <= 2 && (color_anim_flag == 0 || color_anim_flag == 0xff))
    File_error = 0;
  fclose(pal_file);
}

/**
 * Unpack LZ streams from CPCconvImg v0.x (Demoniak/iMPACT!)
 * @param dst destination buffer
 * @param file input
 * @return unpacked length or -1 for error
 */
static int Depack_CPC_LZW(byte * dst, FILE * file)
{
  int bitcount = 0;
  byte control_byte = 0;
  int count = 0;

  for (;;)
  {
    if (bitcount == 0)
    {
      if (!Read_byte(file, &control_byte))
        return -1;
      bitcount = 8;
    }

    if (!(control_byte & 1))
    {
      if (!Read_byte(file, &dst[count++]))
        return -1;
    }
    else
    {
      byte code, code2;
      word delta;
      word len;
      if (!Read_byte(file, &code))
        return -1;
      if (code == 0)
      {
        return count; // EOF
      }
      if (code & 0x80)
      {
        // 1LLL DDDD DDDD DDDD
        if (!Read_byte(file, &code2))
          return -1;
        len = 3 + ((code >> 4) & 7);
        delta = ((word)(code & 15) << 8) + (word)code2 + 1;
      }
      else if (code & 0x40)
      {
        // 01DD DDDD
        len = 2;
        delta = (code & 0x3f) + 1;
      }
      else if (code & 0x20)
      {
        // 001L LLLL DDDD DDDD
        len = 2 + (code & 31);
        if (!Read_byte(file, &code2))
          return -1;
        delta = (word)code2 + 1;
      }
      else if (code & 0x10)
      {
        // 0001 DDDD DDDD DDDD LLLL LLLL
        if (!Read_byte(file, &code2))
          return -1;
        delta = ((word)(code & 15) << 8) + (word)code2 + 1;
        if (!Read_byte(file, &code2))
          return -1;
        len = (word)code2 + 1;
      }
      else if (code == 0x0f)
      {
        // 0000 1111 LDLD LDLD
        if (!Read_byte(file, &code2))
          return -1;
        delta = len = (word)code2 + 1;
      }
      else if (code > 1)
      {
        // 0000 LDLD   1 < len = delta < 15
        delta = len = (word)code;
      }
      else
      {
        // 0000 0001
        delta = len = 256;
      }
      while (len--)
      {
        dst[count] = dst[count - delta];
        count++;
      }
    }
    bitcount--;
    control_byte >>= 1;
  }
}


/**
 * Load Advanced OCP Art Studio files (Amstrad CPC)
 *
 * Standard resolution files (Mode 0 160x200, mode 1 320x200 and
 * mode 2 640x200) are supported. The .PAL file is loaded if available.
 * "MJH" RLE packing is supported.
 * Embedded CRTC registers and palette data from various tools is also
 * supported.
 *
 * .WIN "window" format is also loaded here and allows different picture sizes.
 */
void Load_SCR(T_IO_Context * context)
{
    // The Amstrad CPC screen memory is mapped in a weird mode, somewhere
    // between bitmap and textmode. Basically the only way to decode this is to
    // emulate the video chip and read the bytes as needed...
    // Moreover, the hardware allows the screen to have any size from 8x1 to
    // 800x273 pixels, and there is no indication of that in the file besides
    // its size. It can also use any of the 3 screen modes. Fortunately this
    // last bit of information is stored in the palette file.
    // Oh, and BTW, the picture can be offset, and it's even usual to do it,
    // because letting 128 pixels unused at the beginning of the file make it a
    // lot easier to handle screens using more than 16K of VRam.
    // The pixel encoding change with the video mode so we have to know that
    // before attempting to load anything...
    // As if this wasn't enough, Advanced OCP Art Studio, the reference tool on
    // Amstrad, can use RLE packing when saving files, meaning we also have to
    // handle that.

    // All this mess enforces us to load (and unpack if needed) the file to a
    // temporary 32k buffer before actually decoding it.
  FILE * pal_file, * file;
  unsigned long real_file_size, file_size, amsdos_file_size = 0;
  word addr;
  word load_address = 0x4000; // default for OCP Art studio
  word exec_address = 0;
  word display_start = 0x4000;
  byte mode = -1, color_anim_flag, color_anim_delay;
  byte pal_data[236]; // 12 palettes of 16+1 colors + 16 excluded inks + 16 protected inks
  word width, height = 200;
  byte bpp;
  enum PIXEL_RATIO ratio;
  byte * cpc_ram;
  word x, y;
  int i;
  byte sig[3];
  word block_length;
  word win_width, win_height;
  int linear = 0;
  int is_win = 0;
  int columns = 80;
  int cpc_plus = 0;
  const byte * cpc_plus_pal = NULL;

  File_error = 1;
  // requires the PAL file for OCP Art studio files
  pal_file = Open_file_read_with_alternate_ext(context, "pal");
  if (pal_file != NULL)
  {
    file_size = File_length_file(pal_file);
    if (CPC_check_AMSDOS(pal_file, NULL, NULL, &file_size))
      fseek(pal_file, 128, SEEK_SET); // right after AMSDOS header
    else
      fseek(pal_file, 0, SEEK_SET);
    if (!Read_byte(pal_file, &mode) || !Read_byte(pal_file, &color_anim_flag)
          || !Read_byte(pal_file, &color_anim_delay) || !Read_bytes(pal_file, pal_data, 236))
    {
      GFX2_Log(GFX2_WARNING, "Load_SCR() failed to load .PAL file\n");
      fclose(pal_file);
      return;
    }
    fclose(pal_file);
    GFX2_Log(GFX2_DEBUG, "Load_SCR() mode=%d color animation flag=%02X delay=%u\n",
             mode, color_anim_flag, color_anim_delay);
  }
  else
  {
    memset(pal_data, 0, sizeof(pal_data));
  }

  file = Open_file_read(context);
  if (file == NULL)
    return;
  file_size = File_length_file(file);
  real_file_size = file_size;
  if (CPC_check_AMSDOS(file, &load_address, &exec_address, &amsdos_file_size))
  {
    display_start = load_address;
    if (file_size < (amsdos_file_size + 128))
    {
      GFX2_Log(GFX2_ERROR, "Load_SCR() mismatch in file size. AMSDOS file size %lu, should be %lu\n", amsdos_file_size, file_size - 128);
      fclose(file);
      return;
    }
    else if (file_size > (amsdos_file_size + 128))
      GFX2_Log(GFX2_INFO, "Load_SCR() %lu extra bytes at end of file\n", file_size - 128 - amsdos_file_size);
    fseek(file, 128, SEEK_SET); // right after AMSDOS header
    snprintf(context->Comment, COMMENT_SIZE, "AMSDOS load@ &%04X exec@ &%04X",
             load_address, exec_address);
    file_size = amsdos_file_size;
  }
  else
    fseek(file, 0, SEEK_SET);

  if (!Read_bytes(file, sig, 3) || !Read_word_le(file, &block_length))
  {
    fclose(file);
    return;
  }
  fseek(file, -5, SEEK_CUR);

  cpc_ram = GFX2_malloc(64*1024);
  memset(cpc_ram, 0, 64*1024);

  if (0 == memcmp(sig, "PKV", 3))
  {
    // PKVL / PKVP => Overscan
    fseek(file, 4, SEEK_CUR);
    i = Depack_CPC_LZW(cpc_ram + load_address, file);
    if (i < 0)
    {
      File_error = 1;
      fclose(file);
      free(cpc_ram);
      return;
    }
    GFX2_Log(GFX2_DEBUG, "%c%c%c%c count=%d\n", sig[0], sig[1], sig[2], block_length & 255, i);
    cpc_plus = (block_length & 255) == 'P';
    if (cpc_plus)
      cpc_plus_pal = cpc_ram + 0x801;
    else
    {
      int j;
      for (j = 0; j < 16; j++)
        pal_data[12*j] = CPC_Firmware_to_Hardware_color(cpc_ram[0x801 + j]);
    }
    context->Comment[COMMENT_SIZE-2] = ' ';
    context->Comment[COMMENT_SIZE-1] = 'P';
    context->Comment[COMMENT_SIZE] = '\0';
  }
  else if (0 == memcmp(sig, "PKS", 3))
  {
    // PKSL = CPC "old"
    // PKSP = CPC +
    fseek(file, 4 + 1, SEEK_CUR);
    cpc_plus = (block_length & 255) == 'P';
    if (!Read_bytes(file, cpc_ram, cpc_plus ? 32 : 16))
    {
      File_error = 1;
      fclose(file);
      free(cpc_ram);
      return;
    }
    if (!cpc_plus)
    {
      for (i = 0; i < 16; i++)
        pal_data[12*i] = CPC_Firmware_to_Hardware_color(cpc_ram[i]);
    }
    else
      cpc_plus_pal = cpc_ram;
    i = Depack_CPC_LZW(cpc_ram + load_address, file);
    if (i < 0)
    {
      File_error = 1;
      fclose(file);
      free(cpc_ram);
      return;
    }
    mode = block_length >> 8;
    GFX2_Log(GFX2_DEBUG, "%c%c%c%c mode %d count=%d\n", sig[0], sig[1], sig[2], block_length & 255, mode, i);
    linear = 1;
    display_start = load_address;
    i = 0;
    height = 200;
    columns = 80;
    context->Comment[COMMENT_SIZE-2] = ' ';
    context->Comment[COMMENT_SIZE-1] = 'P';
    context->Comment[COMMENT_SIZE] = '\0';
  }
  else if (0 != memcmp(sig, "MJH", 3) || block_length > 16384)
  {
    // raw data
    Read_bytes(file, cpc_ram + load_address, file_size);
    i = file_size;
  }
  else
  {
    // MJH packed format
    i = 0;
    do
    {
      if (!Read_bytes(file, sig, 3) || !Read_word_le(file, &block_length))
        break;
      if (0 != memcmp(sig, "MJH", 3))
        break;
      GFX2_Log(GFX2_DEBUG, "  %.3s block %u\n", sig, block_length);
      file_size -= 5;
      while (block_length > 0)
      {
        byte code;
        if (!Read_byte(file, &code))
          break;
        file_size--;
        if (code == 1)
        {
          byte repeat, value;
          if (!Read_byte(file, &repeat) || !Read_byte(file, &value))
            break;
          file_size -= 2;
          do
          {
            cpc_ram[load_address + i++] = value;
            block_length--;
          }
          while(--repeat != 0);
        }
        else
        {
          cpc_ram[load_address + i++] = code;
          block_length--;
        }
      }
      GFX2_Log(GFX2_DEBUG, "  unpacked %d bytes. remaining bytes in file=%lu\n",
               i, file_size);
    }
    while(file_size > 0 && i < 16384);
  }
  fclose(file);

  if (i > 5)
  {
    win_width = cpc_ram[load_address + i - 4] + (cpc_ram[load_address + i - 3] << 8);  // in bits
    win_height = cpc_ram[load_address + i - 2];
    if (((win_width + 7) >> 3) * win_height + 5 == i) // that's a WIN file !
    {
      width = win_width >> (2 - mode);
      height = win_height;
      is_win = 1;
      columns = (win_width + 7) >> 3;
      GFX2_Log(GFX2_DEBUG, ".WIN file detected len=%d (%d,%d) %dcols %02X %02X %02X %02X %02X\n",
          i, width, height, columns,
          cpc_ram[load_address + i - 5], cpc_ram[load_address + i - 4], cpc_ram[load_address + i - 3],
          cpc_ram[load_address + i - 2], cpc_ram[load_address + i - 1]);
    }
    else
    {
      word crtc_regs_data = 0;
      GFX2_Log(GFX2_DEBUG, ".SCR file. Data length %d, load at %04X, run at %04X\n", i, load_address, exec_address);
      if (load_address == 0x170)
      {
        // fichier iMPdraw v2
        // http://orgams.wikidot.com/le-format-impdraw-v2
        GFX2_Log(GFX2_DEBUG, "Detected \"%s\"\n", cpc_ram + load_address + 6);
        mode = cpc_ram[load_address + 0x14] - 0x0e;
        cpc_plus = cpc_ram[load_address + 0x3c];
        GFX2_Log(GFX2_DEBUG, "Mode %d CPC %d\n", (int)mode, (int)cpc_plus);
        crtc_regs_data = load_address + 0x1d;
        snprintf(context->Comment, COMMENT_SIZE, "%s mode %d %s",
                 cpc_ram + load_address + 7, mode, cpc_plus ? "CPC+" : "");
        if (cpc_plus)
        {
          // palette at 0x801 (mode at 0x800 ?)
          GFX2_LogHexDump(GFX2_DEBUG, "", cpc_ram, 0x800, 0x21);
          cpc_plus_pal = cpc_ram + 0x801;
        }
        else
        {
          int j;
          // palette at 0x7f00
          GFX2_LogHexDump(GFX2_DEBUG, "", cpc_ram, 0x7f00, 16);
          for (j = 0; j < 16; j++)
            pal_data[12*j] = cpc_ram[0x7f00 + j];
        }
      }
      else if (load_address == 0x6000 && exec_address == 0x6000)
      {
        int j;
        /* CPCFILL.SCR
        6000 3e 00        LD A,0
        6002 cd 0e bc     CALL BC0E   ; SCR SET MODE
        6005 cd 19 bd     CALL BD19   ; MC WAIT FLYBACK
        6008 3a 55 a0     LD A,(A055)
        600b 47           LD B,A
        600c 4f           LD C,A
        600d cd 38 bc     CALL BC38   ; SCR SET BORDER
        6010 21 56 a0     LD HL,A056
        6013 cd 31 60     CALL 6031   ; LOADINKS
        6016 cd 19 bd     CALL BD19   ; MC WAIT FLYBACK
        6019 21 45 60     LD HL,6045  ; screen data
        601c 11 00 c0     LD DE,C000
        601f 01 00 40     LD BC,4000
        6022 ed b0        LDIR        ; copy bc bytes from (hl) to (de)
        6024 cd 19 bd     CALL BD19   ; MC WAIT FLYBACK
        6027 21 45 a0     LD HL,A045
        602a cd 31 60     CALL 6031   ; LOADINKS
        602d cd 18 bb     CALL BB18   ; KM WAIT KEY
        6030 c9           RET
          LOADINKS:
        6031 06 10        LD B,10   ; useless ?
        6033 3e 00        LD A,0
          LOOPINKS:
        6035 4e           LD C,(HL)
        6036 23           INC HL
        6037 e5           PUSH HL
        6038 c5           PUSH BC
        6039 f5           PUSH AF
        603a 41           LD B,C
        603b cd 32 bc     CALL BC32   ; SCR SET INK
        603e f1           POP AF
        603f c1           POP BC
        6040 e1           POP HL
        6041 3c           INC A
        6042 10 f1        DJNZ 6035 ; LOOPINKS
        6044 c9           RET
          PIXELDATA:
        */
        mode = cpc_ram[exec_address + 1];
        for (j = 0; j < 16; j++)
          pal_data[12*j] = CPC_Firmware_to_Hardware_color(cpc_ram[load_address + 0x4045 + j]);
        display_start = 0xc000;
        memcpy(cpc_ram + 0xc000, cpc_ram + load_address + 0x45, 0x4000);
      }
      else if (load_address == 0x200 && exec_address == 0x821)
      {
        /* BND.SCR
        0821 f3           DI
        0822 01 11 bc     LD BC,BC11
        0825 21 86 08     LD HL,0886 ; UNLOCK_SEQ
        0828 04           INC B     <--+
        0829 ed a3        OUTI         |
        082b 0d           DEC C        |
        082c 20 fa        JR NZ,0828 --+
        082e 21 97 08     LD HL,0897 ; OVERSCAN_REG_VALUES
        0831 cd 75 08     CALL 0875  ; SEND_CRTC
        0834 01 b8 7f     LD BC,7FB8
        0837 3a 00 08     LD A,(0800) ; 8C
        083a ed 49        OUT (C),C  ; CPC+ unleashed ;)
        083c ed 79        OUT (C),A  ; Set mode. 8C => Mode 0
        083e 21 01 08     LD HL,0801
        0841 11 00 64     LD DE,6400 ; CPC+ Palette
        0844 01 20 00     LD BC,0020
        0847 ed b0        LDIR       ; copy bc bytes from (hl) to (de)
          LOOP:
        0849 af           XOR A
        084a 01 0e f4     LD BC,F40E
        084d ed 49        OUT (C),C
        084f 01 c0 f6     LD BC,F6C0
        0852 ed 49        OUT (C),C
        0854 ed 79        OUT (C),A
        0856 01 92 f7     LD BC,F792
        0859 ed 49        OUT (C),C
        085b 01 45 f6     LD BC,F645
        085e ed 49        OUT (C),C
        0860 06 f4        LD B,F4    ; BC=F445
        0862 ed 78        IN A,(C)
        0864 01 82 f7     LD BC,F782
        0867 ed 49        OUT (C),C
        0869 17           RLA
        086a 38 dd        JR C,0849 ; LOOP
        086c 01 a0 7f     LD BC,7FA0
        086f ed 49        OUT (C),C  ; CPC+ disabled ?
        0871 fb           EI
        0872 21 a5 08     LD HL,08A5 ; STANDARD_REG_VALUES
          SEND_CRTC:
        0875 01 00 bc     LD BC,BC00
          CRTC_LOOP:
        0878 7e           LD A,(HL)
        0879 a7           AND A
        087a c8           RET Z
        087b ed 79        OUT (C),A
        087d 04           INC B
        087e 23           INC HL
        087f 7e           LD A,(HL)
        0880 ed 79        OUT (C),A
        0882 23           INT HL
        0883 05           DEC B
        0884 18 f2        JR 0878    ; CRTC_LOOP
          UNLOCK_SEQ:
        0886 ff 00 ff 77 b3 51 a8 d4 ; CPC Plus ASIC unlocking sequence
        088e 62 39 9c 46 2b 15 8a cd
        0896 ee
            OVERSCAN_REG_VALUES:
        0897 01 30 02 32 06 22 07 23 0c 0d 0d 00 00 00
            STANDARD_REG_VALUES:
        08a5 01 28 02 2e 06 19 07 1e 0c 30 00 00
        */
        mode = cpc_ram[0x0800] & 3;
        cpc_plus_pal = cpc_ram + 0x0801;
        crtc_regs_data = 0x897;
      }
      else if (load_address == 0x200 && exec_address == 0x811)
      {
        /* from HARLEY.SCR :
        0800  00 = mode
        0801-0810 palette (Firmware colors)
        0811  21 47 08      LD HL,0847  ; OVERSCAN_REG_VALUES
        0814  cd 36 08      CALL 0836 ; LOAD_CRTC_REGS
        0817  3a 00 08      LD A,(0800) ; MODE
        081a  cd 1c bd      CALL BD1C ; Set screen mode
        081d  21 01 08      LD HL,0801  ; PALETTE
        0820  af            XOR A
            LOOP:
        0821  4e            LD C,(HL)
        0822  41            LD B,C
        0823  f5            PUSH AF
        0824  e5            PUSH HL
        0825  cd 32 bc      CALL BC32   ; SET ink A to color B,C
        0828  e1            POP HL
        0829  f1            POP AF
        082a  23            INC HL
        082b  3c            INC A
        082c  fe 10         CMP 10
        082e  20 f1         JR NZ,0821  ; LOOP
        0830  cd 18 bb      CALL BB18 ; Wait key press
        0833  21 55 08      LD HL,0855  ; STANDARD_REG_VALUES
            LOAD_CRTC_REGS:
        0836  01 00 bc      LD BC,BC00
            LOOP_CRTC:
        0839  7e            LD A,(HL)
        083a  a7            AND A
        083b  c8            RET Z
        083c  ed 79         OUT (C),A
        083e  04            INC B
        083f  23            INC HL
        0840  7e            LD A,(HL)
        0841  ed 79         OUT (C),A
        0843  23            INC HL
        0844  05            DEC B
        0845  18 f2         JR 0839 ; LOOP_CRTC
            OVERSCAN_REG_VALUES:
        0847  01 30  02 32  06 22  07 23  0c 0d  0d 00  00 00
            STANDARD_REG_VALUES:
        0855  01 28  02 2e  06 19  07 1e  0c 30  00 00
        */
        int j;
        mode = cpc_ram[0x800];
        for (j = 0; j < 16; j++)
          pal_data[12*j] = CPC_Firmware_to_Hardware_color(cpc_ram[0x801 + j]);
        crtc_regs_data = 0x847;
        if (cpc_ram[0x80bb] == 1)
          crtc_regs_data = 0x80bb;
        snprintf(context->Comment, COMMENT_SIZE, "ConvImgCPC mode %d", mode);
      }
      else if (load_address == 0xC000 && cpc_ram[0xc7d0] != 0)
      {
        mode = cpc_ram[0xD7D0];
        if ((mode & 0xc0) == 0x80)
        {
          // value sent to gate array :
          // bits 7 & 6 = function :
          //              10 => select mode, ROM conf and interrupt control
          //              bit 4 : interrupt control
          //              bit 3 : disable upper ROM (C000 = Basic)
          //              bit 2 : disable lower ROM (0000 = Firmware)
          //              bits 1&0 : screen mode
          mode &= 3;
          cpc_plus_pal = cpc_ram + 0xD7D1;
        }
        else
        {
          int j;
          for (j = 0; j < 16; j++)
            pal_data[12*j] = CPC_Firmware_to_Hardware_color(cpc_ram[0xD7D1 + j]);
        }
        snprintf(context->Comment, COMMENT_SIZE, "loader@ &%04X mode %d %s", exec_address, mode, cpc_plus_pal ? "CPC+" : "");
      }
      else if (load_address == 0x0040 && exec_address == 0x8000)
      {
        mode = cpc_ram[0x8008] & 3;
        memset(cpc_ram, cpc_ram[0x40], 0x40);
        crtc_regs_data = 0x8094;
        cpc_plus_pal = cpc_ram + 0x80a0;
      }
      if (crtc_regs_data != 0)
      {
        display_start = 0xC000; /* default set by firmware */
        GFX2_Log(GFX2_DEBUG, "Loading CRTC registers from &H%04X\n", crtc_regs_data);
        for (addr = crtc_regs_data; cpc_ram[addr] > 0 && cpc_ram[addr] < 16; addr += 2)
        {
          GFX2_Log(GFX2_DEBUG, " R%d = &H%02x = %d\n", cpc_ram[addr], cpc_ram[addr+1], cpc_ram[addr+1]);
          // see http://www.cpcwiki.eu/index.php/CRTC#The_6845_Registers
          switch(cpc_ram[addr])
          {
            case 1:
              columns = cpc_ram[addr+1] * 2;
              break;
            case 6:
              height = cpc_ram[addr+1] * 8;
              break;
            case 12:
              display_start = (display_start & 0x01ff) | ((cpc_ram[addr+1] & 0x30) << 10) | ((cpc_ram[addr+1] & 0x03) << 9);
              break;
            case 13:
              display_start = (display_start & 0xfe00) | (cpc_ram[addr+1] << 1);
          }
        }
        GFX2_Log(GFX2_DEBUG, "  display_start &H%04X height %d columns %d\n", display_start, height, columns);
      }
      else if (i >= 30000)
      {
        height = 272; columns = 96;
      }
    }
  }

check_mode:
  switch (mode)
  {
    case 0:
      width = columns * 2;
      bpp = 4;
      ratio = PIXEL_WIDE;
      break;
    case 1:
      width = columns * 4;
      bpp = 2;
      ratio = PIXEL_SIMPLE;
      break;
    case 2:
      width = columns * 8;
      bpp = 1;
      ratio = PIXEL_TALL;
      break;
    default:
    {
      static const char* mode_label[] = {"0", "1", "2"};
      T_Dropdown_button* btn_mode;
      T_Special_button* btn_width;
      T_Special_button* btn_height;
      int button;
      unsigned int i;
      char  str[8];

      // We didn't find any recongizable header.
      // In preview, use some reasonable default values
      // In real loading, ask the user
      display_start = load_address;
      width = 320;
      height = 200;
      columns = 80;
      mode = 0;

      if (context->Type == CONTEXT_MAIN_IMAGE) {
        Open_window(200, 120, "CPC loading settings");
        Window_set_normal_button(110,100,80,15,"Load",1,1,KEY_RETURN); // 1
        Window_set_normal_button(10,100,80,15,"Cancel",1,1,KEY_ESCAPE); // 2
                                                                        // Variables we need to set: width (columns), height

        Print_in_window(13,20,"Mode:",MC_Dark,MC_Light);
        btn_mode = Window_set_dropdown_button(68,16,48,15,32,mode_label[0],1, 0, 1, LEFT_SIDE,0); // 3
        Window_dropdown_clear_items(btn_mode);
        for (i=0; i<sizeof(mode_label)/sizeof(mode_label[0]); i++)
          Window_dropdown_add_item(btn_mode,i,mode_label[i]);

        Print_in_window(13,40,"Width:",MC_Dark,MC_Light);
        btn_width = Window_set_input_button(68, 38, 4); // 4
        Num2str(width, str, 4);
        Window_input_content(btn_width, str);

        Print_in_window(13,60,"Height:",MC_Dark,MC_Light);
        btn_height = Window_set_input_button(68, 58, 4); // 5
        Num2str(height, str, 4);
        Window_input_content(btn_height, str);

        Update_window_area(0,0,Window_width,Window_height);
        Display_cursor();

        do
        {
          button = Window_clicked_button();

          if (button == 3) {
              mode = Window_attribute2;
          }

          if (button == 4) {
            Num2str(width, str, 4);
            Readline(70, 40, str, 4, INPUT_TYPE_INTEGER);
            width = atoi(str);
            Display_cursor();
          }

          if (button == 5) {
              Num2str(height, str, 4);
              Readline(70, 60, str, 4, INPUT_TYPE_INTEGER);
              height = atoi(str);
              Display_cursor();
          }
        } while(button!=1 && button!=2);

        Close_window();
        Display_cursor();

        if (button == 2)
            return;
      }

      switch (mode) {
        case 0: columns = width / 2; break;
        case 1: columns = width / 4; break;
        case 2: columns = width / 8; break;
      }
    }

    // All values set, try again
    goto check_mode;
  }

  if (Config.Clear_palette)
    memset(context->Palette,0,sizeof(T_Palette));
  // Setup the palette (amstrad hardware palette)
  CPC_set_HW_palette(context->Palette + 0x40);

  // Set the palette for this picture
  if (cpc_plus_pal)
  {
    for (i = 0; i < 16; i++)
    {
      context->Palette[i].G = cpc_plus_pal[i*2 + 1] * 0x11;
      context->Palette[i].R = (cpc_plus_pal[i*2] >> 4) * 0x11;
      context->Palette[i].B = (cpc_plus_pal[i*2] & 15) * 0x11;
    }
  }
  else
  {
    int ocp_plus_palette = 1;
    for (i = 0; i < 16 && ocp_plus_palette; i++)
    {
      byte inkr, inkb, inkg;
      int j;
      // OCP+ palettes use the 3 first slots and have the 9 remaining set to 0
      for (j = 3; j < 12; j++)
      {
        if (pal_data[12*i+j] != 0)
        {
          ocp_plus_palette = 0;
          break;
        }
      }
      inkr = (context->Palette[pal_data[12*i]].G / 86) * 9 + (context->Palette[pal_data[12*i]].R / 86) * 3 + (context->Palette[pal_data[12*i]].B / 86);
      inkb = (context->Palette[pal_data[12*i+1]].G / 86) * 9 + (context->Palette[pal_data[12*i+1]].R / 86) * 3 + (context->Palette[pal_data[12*i+1]].B / 86);
      inkg = (context->Palette[pal_data[12*i+2]].G / 86) * 9 + (context->Palette[pal_data[12*i+2]].R / 86) * 3 + (context->Palette[pal_data[12*i+2]].B / 86);
      if (inkr < 11 || inkb < 11 || inkg < 11)
          ocp_plus_palette = 0;
    }
    if (ocp_plus_palette)
    {
      GFX2_Log(GFX2_DEBUG, "OCP+ Palette detected\n");
      for (i = 0; i < 16; i++)
      {
        byte inkr, inkb, inkg;
        inkr = (context->Palette[pal_data[12*i]].G / 86) * 9 + (context->Palette[pal_data[12*i]].R / 86) * 3 + (context->Palette[pal_data[12*i]].B / 86);
        inkb = (context->Palette[pal_data[12*i+1]].G / 86) * 9 + (context->Palette[pal_data[12*i+1]].R / 86) * 3 + (context->Palette[pal_data[12*i+1]].B / 86);
        inkg = (context->Palette[pal_data[12*i+2]].G / 86) * 9 + (context->Palette[pal_data[12*i+2]].R / 86) * 3 + (context->Palette[pal_data[12*i+2]].B / 86);
        context->Palette[i].R = (26 - inkr) * 0x11;
        context->Palette[i].G = (26 - inkg) * 0x11;
        context->Palette[i].B = (26 - inkb) * 0x11;
      }
    }
    else
    {
      // Standard OCP palette
      for (i = 0; i < 16; i++)
        context->Palette[i] = context->Palette[pal_data[12*i]];
    }
  }

  File_error = 0;
  Pre_load(context, width, height, real_file_size, FORMAT_SCR, ratio, bpp);

  if (!is_win && !linear)
  {
    // Standard resolution files have the 200 lines stored in block
    // of 25 lines of 80 bytes = 2000 bytes every 2048 bytes.
    // so there are 48 bytes unused every 2048 bytes...
    for (y = 0; y < 8; y++)
    {
      addr = display_start + 0x800 * y;
      if (y > 0 && (display_start & 0x7ff))
      {
        if (!GFX2_is_mem_filled_with(cpc_ram + (addr & 0xf800), 0, display_start & 0x7ff))
          GFX2_LogHexDump(GFX2_DEBUG, "SCR1 ", cpc_ram,
                          addr & 0xf800, display_start & 0x7ff);
      }
      addr += (height >> 3) * columns;
      block_length = (height >> 3) * columns + (display_start & 0x7ff);
      if (block_length <= 0x800)
      {
        block_length = 0x800 - block_length;
        if (!GFX2_is_mem_filled_with(cpc_ram + addr, 0, block_length))
          GFX2_LogHexDump(GFX2_DEBUG, "SCR2 ", cpc_ram,
                          addr, block_length);
      }
      else
      {
        block_length = 0x1000 - block_length;
        if (!GFX2_is_mem_filled_with(cpc_ram + addr + 0x4000, 0, block_length))
          GFX2_LogHexDump(GFX2_DEBUG, "SCR2 ", cpc_ram,
                          addr + 0x4000, block_length);
      }
    }
    //for (j = 0; j < i; j += 2048)
    //  GFX2_LogHexDump(GFX2_DEBUG, "SCR ", cpc_ram, load_address + j + 2000, 48);
  }

  GFX2_Log(GFX2_DEBUG, "  display_start &H%04X  columns=%u\n", display_start, columns);
  for (y = 0; y < height; y++)
  {
    x = 0;
    for (i = 0; i < columns; i++)
    {
      byte pixels;
      if (is_win)
        addr = display_start + y * columns + i;
      else if (linear)
        addr = display_start + y + i * height;
      else
      {
        addr = display_start + ((y >> 3) * columns) + i;
        addr = (addr & 0xC7FF) + ((addr & 0x800) << 3);
        addr += (y & 7) << 11;
      }
      pixels = cpc_ram[addr];
      switch (mode)
      {
        case 0:
          Set_pixel(context, x++, y, (pixels & 0x80) >> 7 | (pixels & 0x08) >> 2 | (pixels & 0x20) >> 3 | (pixels & 0x02) << 2);
          Set_pixel(context, x++, y, (pixels & 0x40) >> 6 | (pixels & 0x04) >> 1 | (pixels & 0x10) >> 2 | (pixels & 0x01) << 3);
          break;
        case 1:
          do {
            // upper nibble is 4 lower color bits, lower nibble is 4 upper color bits
            Set_pixel(context, x++, y, (pixels & 0x80) >> 7 | (pixels & 0x08) >> 2);
            pixels <<= 1;
          }
          while ((x & 3) != 0);
          break;
        case 2:
          do {
            Set_pixel(context, x++, y, (pixels & 0x80) >> 7);
            pixels <<= 1;
          }
          while ((x & 7) != 0);
      }
    }
  }

  free(cpc_ram);
}

#include "impdraw_loader.h"
#include "cpc_scr_simple_loader.h"

/**
 * Save Amstrad SCR file
 *
 * guess mode from aspect ratio :
 * - normal pixels are mode 1
 * - wide pixels are mode 0
 * - tall pixels are mode 2
 *
 * Mode and palette are stored in a .PAL file for compatibility
 * with OCP Art studio.
 *
 * The picture color index should be 0-15,
 * The CPC Hardware palette is expected to be set (indexes 64 to 95)
 *
 * If the picture is using overscan (more than 16384 bytes)
 * we produce a iMPdraw v2 format autoloading file.
 * see http://orgams.wikidot.com/le-format-impdraw-v2
 *
 * If the picture is not using overscan (standard resolutions,
 * < 16384 screen buffer) a BASIC loader is saved.
 *
 * @todo Add possibility to set R9 value
 * @todo Add OCP packing support
 */
void Save_SCR(T_IO_Context * context)
{
  int i, j;
  unsigned char* output;
  unsigned long outsize = 0;
  unsigned char r1 = 0; // Horizontal Displayed. Standard value is 40 (=80 bytes)
  int cpc_mode;
  FILE* file;
  int cpc_plus_pal = 0;
  unsigned short load_address = 0xC000; // Standard CPC screen address
  unsigned short exec_address = 0xC7D0;
  int overscan;
  byte cpc_hw_pal[16];
  byte r12 = 0x0C | 0x30; // set Display Start Address at C000
  byte r13 = 0;

  switch(context->Ratio)
  {
    case PIXEL_WIDE:
    case PIXEL_WIDE2:
      cpc_mode = 0; // 16 colors, 2 pixels per byte
      overscan = (context->Width * context->Height) > (16384 * 2);
      break;
    case PIXEL_TALL:
    case PIXEL_TALL2:
    case PIXEL_TALL3:
      cpc_mode = 2; // 2 colors, 8 pixels per byte
      overscan = (context->Width * context->Height) > (16384 * 8);
      break;
    default:
      cpc_mode = 1; // 4 colors, 4 pixels per byte
      overscan = (context->Width * context->Height) > (16384 * 4);
      break;
  }
  if (overscan)
  {
    // format iMP v2
    load_address = 0x170; // BASIC program at 0x170
    // picture at 0x200
    r12 = 0x0C | (0x200 >> 9);
    r13 = (0x200 >> 1) & 0xff;
    exec_address = 0; // BASIC program
  }

  CPC_set_HW_palette(context->Palette + 0x40);
  for (i = 0; i < 16; i++)
  {
    // search for the color in the HW palette (0x40-0x5F)
    byte index = 0x40;
    while ((index < 0x60) &&
        !CPC_compare_colors(context->Palette + i, context->Palette + index))
      index++;
    if (index >= 0x60)
    {
      GFX2_Log(GFX2_WARNING, "Save_SCR() color #%i not found in CPC HW palette.\n", i);
      cpc_plus_pal = 1;
      index = 0x54 - i; // default
    }
    cpc_hw_pal[i] = index;
    if (!CPC_is_CPC_old_color(context->Palette + i))
      cpc_plus_pal = 1;
  }

  file = Open_file_write_with_alternate_ext(context, "pal");
  if (file == NULL)
    return;
  CPC_write_AMSDOS_header(file, context->File_name, "pal", 2, 0x8809, 0x8809, 239);
  if (!Write_byte(file, cpc_mode) || !Write_byte(file, 0) || !Write_byte(file, 0))
  {
    fclose(file);
    return;
  }
  for (i = 0; i < 16; i++)
  {
    for (j = 0; j < 12; j++)  // write the same color for the 12 frames
    {
      Write_byte(file, cpc_hw_pal[i]);
    }
  }
  // border
  for (j = 0; j < 12; j++)
  {
    Write_byte(file, 0x54); // black
  }
  // excluded inks
  for (i = 0; i < 16; i++)
  {
    Write_byte(file, 0);
  }
  // protected inks
  for (i = 0; i < 16; i++)
  {
    Write_byte(file, 0);
  }
  fclose(file);

  output = raw2crtc(context, cpc_mode, 7, &outsize, &r1, r12, r13);
  GFX2_Log(GFX2_DEBUG, "Save_SCR() output=%p outsize=%lu r1=$%02X\n", output, outsize, r1);

  if (output == NULL)
    return;

  file = Open_file_write(context);
  if (file == NULL)
    File_error = 1;
  else
  {
    File_error = 0;
    CPC_write_AMSDOS_header(file, context->File_name, "SCR", overscan ? 0 : 2, load_address, exec_address, overscan ? 32400 : outsize);
    if (overscan)
    {
      // write iMPdraw loader
      byte buffer[0x90];
      memcpy(buffer, impdraw_loader, 0x90);
      buffer[0x184 - 0x170] = 0x0e + cpc_mode;
      buffer[0x18e - 0x170] = r1;
      //buffer[0x190 - 0x170] = // R2 ?
      buffer[0x192 - 0x170] = context->Height / 8;  // r6
      //buffer[0x194 - 0x170] = // r7 ?
      buffer[0x196 - 0x170] = r12;
      buffer[0x198 - 0x170] = r13;
      buffer[0x1ac - 0x170] = cpc_plus_pal;
      if (!Write_bytes(file, buffer, 0x90))
        File_error = 1;
      output = realloc(output, 32400 - 0x90);
      memset(output + outsize, 0, 32400 - 0x90 - outsize);
      outsize = 32400 - 0x90;
      memcpy(output + 0x7F00 - 0x200, cpc_hw_pal, 16);
      for (i = 0; i < 16; i++)
      {
        output[0x601 + i*2] = (context->Palette[i].R & 0xf0) | (context->Palette[i].B >> 4);
        output[0x602 + i*2] = context->Palette[i].G >> 4;
      }
    }
    else
    {
      memcpy(output + exec_address - load_address, cpc_scr_simple_loader, sizeof(cpc_scr_simple_loader));
      output[0xd7d0 - load_address] = cpc_mode;
      //memcpy(output + 0xd7d1 - load_address, cpc_hw_pal, 16);
      for (i = 0; i < 16; i++)
      {
        output[0xd7d1 - load_address + i] = (context->Palette[i].G / 86) * 9 + (context->Palette[i].R / 86) * 3 + (context->Palette[i].B / 86);
      }
    }
    if (!Write_bytes(file, output, outsize))
      File_error = 1;
    fclose(file);
    if (!overscan)
    {
      file = Open_file_write_with_alternate_ext(context, "BAS");
      if (file != NULL)
      {
        byte buffer[128];
        memset(buffer, 0, sizeof(buffer));
        buffer[2] = 10;  // basic line number
        buffer[4] = 0xad; // MODE
        buffer[5] = ' ';
        buffer[6] = 0x0e + cpc_mode;
        buffer[7] = 0x01; // :
        buffer[8] = 0xa8; // LOAD
        buffer[9] = '"';
        for (i = 0; i < 8; i++)
        {
          if (context->File_name[i] == '\0' || context->File_name[i] == '.')
            break;
          buffer[10+i] = (byte)toupper(context->File_name[i]);
        }
        memcpy(buffer + 10 + i, ".SCR\"", 5);
        i += 15;
        buffer[i++] = 0x01; // :
        buffer[i++] = 0x83; // CALL
        buffer[i++] = 0x1c; // &
        buffer[i++] = 0xd0;
        buffer[i++] = 0xc7;
        buffer[i++] = 0x00;
        buffer[0] = (byte)i; // length of line data
        CPC_write_AMSDOS_header(file, context->File_name, "BAS", 0, 0x170, 0, i + 2);
        if (!Write_bytes(file, buffer, 128))
          File_error = 1;
        fclose(file);
      }
    }
  }
  free (output);
}

/**
 * Test for GO1/GO2/KIT - Amstrad Plus Graphos
 *
 * This format is made of 3 files
 * .KIT hold the palette in "Kit4096" format. There are 16 colors each stored
 * as 12 bit RGB in RB0G order.
 * .GO1 and GO2 hold each half of the picture (top and bottom)
 * The file always cover the whole display of the Plus (192*272 mode 0 pixels)
 * Only mode 0 is possible.
 */
void Test_GOS(T_IO_Context * context, FILE * file)
{
  FILE *file_oddeve;
  unsigned long file_size = 0;

  if (!CPC_check_AMSDOS(file, NULL, NULL, &file_size))
    file_size = File_length_file(file);
  if (file_size < 16383 || file_size > 16384) {
    File_error = 1;
    return;
  }

  file_oddeve = Open_file_read_with_alternate_ext(context, "GO2");
  if (file_oddeve == NULL) {
    File_error = 2;
    return;
  }
  if (!CPC_check_AMSDOS(file_oddeve, NULL, NULL, &file_size))
    file_size = File_length_file(file_oddeve);
  fclose(file_oddeve);
  if (file_size < 16383 || file_size > 16384) {
    File_error = 3;
    return;
  }

  File_error = 0;
}


/**
 * Load GO1/GO2/KIT - Amstrad CPC Plus Graphos
 */
void Load_GOS(T_IO_Context* context)
{
  FILE *file;
  unsigned long file_size;
  int i;
  int x, y;
  byte * pixel_data;

  if (!(file = Open_file_read(context)))
  {
      File_error = 1;
      return;
  }

  if (CPC_check_AMSDOS(file, NULL, NULL, &file_size))
    fseek(file, 128, SEEK_SET); // right after AMSDOS header
  else
    file_size = File_length_file(file);

  Pre_load(context, 192, 272, file_size, FORMAT_GOS, PIXEL_WIDE, 4);

  // load pixels
  pixel_data = GFX2_malloc(16384);
  memset(pixel_data, 0, 16384);
  Read_bytes(file, pixel_data, file_size);

  i = 0;
  for (y = 0; y < 168; y++) {
    x = 0;
    while (x < 192) {
      byte pixels = pixel_data[i];
      Set_pixel(context, x++, y, (pixels & 0x80) >> 7 | (pixels & 0x08) >> 2 | (pixels & 0x20) >> 3 | (pixels & 0x02) << 2);
      Set_pixel(context, x++, y, (pixels & 0x40) >> 6 | (pixels & 0x04) >> 1 | (pixels & 0x10) >> 2 | (pixels & 0x01) << 3);
      i++;
    }

    i += 0x800;
    if (i > 0x3FFF) {
      i -= 0x4000;
    } else {
      i -= 192 / 2;
    }
  }

  fclose(file);

  // load pixels from GO2
  file = Open_file_read_with_alternate_ext(context, "GO2");
  if (CPC_check_AMSDOS(file, NULL, NULL, &file_size))
    fseek(file, 128, SEEK_SET); // right after AMSDOS header

  Read_bytes(file, pixel_data, file_size);
  i = 0;
  for (y = 168; y < 272; y++) {
    x = 0;
    while (x < 192) {
      byte pixels = pixel_data[i];
      Set_pixel(context, x++, y, (pixels & 0x80) >> 7 | (pixels & 0x08) >> 2 | (pixels & 0x20) >> 3 | (pixels & 0x02) << 2);
      Set_pixel(context, x++, y, (pixels & 0x40) >> 6 | (pixels & 0x04) >> 1 | (pixels & 0x10) >> 2 | (pixels & 0x01) << 3);
      i++;
    }

    i += 0x800;
    if (i > 0x3FFF) {
      i -= 0x4000;
    } else {
      i -= 192 / 2;
    }
  }

  fclose(file);
  free(pixel_data);

  file = Open_file_read_with_alternate_ext(context, "KIT");
  if (file == NULL) {
    // There is no palette, but that's fine, we can still load the pixels
    // Setup a default grayscale palette
    for (i = 0; i < 16; i++)
    {
      context->Palette[i].R = i * 0x11;
      context->Palette[i].G = i * 0x11;
      context->Palette[i].B = i * 0x11;
    }
    return;
  }

  if (CPC_check_AMSDOS(file, NULL, NULL, &file_size)) {
    fseek(file, 128, SEEK_SET); // right after AMSDOS header
  } else {
    file_size = File_length_file(file);
  }

  if (Config.Clear_palette)
    memset(context->Palette,0,sizeof(T_Palette));

  File_error = 0;

  if (file_size == 32)
  {
    for (i = 0; i < 16; i++)
    {
      uint16_t word;
      if (!Read_word_le(file, &word))
      {
        File_error = 2;
        break;
      }

      context->Palette[i].R = ((word >>  4) & 0xF) * 0x11;
      context->Palette[i].G = ((word >>  8) & 0xF) * 0x11;
      context->Palette[i].B = ((word >>  0) & 0xF) * 0x11;
    }
  }
  else
  {
    // Setup the palette (amstrad hardware palette)
    CPC_set_HW_palette(context->Palette + 0x40);
    for (i = 0; i < 16; i++)
    {
      byte ink;
      if (!Read_byte(file, &ink))
      {
        File_error = 2;
        break;
      }
      context->Palette[i] = context->Palette[ink];
    }
  }

  fclose(file);
}


void Save_GOS(T_IO_Context* context)
{
  FILE* file;
  int i;
  unsigned char* output;
  unsigned long outsize = 0;
  unsigned char r1 = 0;

  // TODO check picture dimensions (GOS is a fixed resolution format)
  // For now, force the size
  context->Width = 192;
  context->Height = 168; // Convert the first half

  // convert and save page 1
  file = Open_file_write(context);
  if (file == NULL)
    return;
  output = raw2crtc(context, 0, 7, &outsize, &r1, 0, 0);
  File_error = 0;
  if (!Write_bytes(file, output, outsize))
    File_error = 1;
  // Pad to expected size
  GFX2_Log(GFX2_DEBUG, "written %lu bytes, padding to 16384\n", outsize);
  if (outsize >= 8192)
  {
    memset(output, 0, 16384 - outsize);
    if (!Write_bytes(file, output, 16384 - outsize))
      File_error = 1;
  }
  free(output);
  fclose(file);
  if (File_error)
    return;

  // convert and save page 2
  // Advance context to second half of picture
  context->Target_address += context->Pitch * 168;
  context->Height = 104;
  file = Open_file_write_with_alternate_ext(context, "GO2");
  if (file == NULL)
  {
    File_error = 1;
    return;
  }
  output = raw2crtc(context, 0, 7, &outsize, &r1, 0, 0);
  File_error = 0;
  if (!Write_bytes(file, output, outsize))
    File_error = 1;
  // Pad to expected size
  GFX2_Log(GFX2_DEBUG, "written %lu bytes, padding to 16384\n", outsize);
  if (outsize >= 8192)
  {
    memset(output, 0, 16384 - outsize);
    if (!Write_bytes(file, output, 16384 - outsize))
      File_error = 1;
  }
  free(output);
  fclose(file);
  if (File_error)
    return;

  file = Open_file_write_with_alternate_ext(context, "KIT");
  for (i = 0; i < 16 && File_error == 0; i++)
  {
    uint16_t word;
    word = (context->Palette[i].R & 0xF0)
           | ((context->Palette[i].G & 0xF0) << 4)
           | (context->Palette[i].B >> 4);
    if (!Write_word_le(file, word))
      File_error = 2;
  }
  fclose(file);
}


/**
 * Test for CM5 - Amstrad CPC "Mode 5" picture
 *
 * This is a format designed by SyX.
 * There is one .GFX file in the usual amstrad format
 * and a .CM5 file with the palette, which varies over time.
 *
 * CM5 file is 2049 bytes, GFX is 18432 bytes.
 *
 * @todo check CM5 contains only valid values [0x40-0x5f]
 */
void Test_CM5(T_IO_Context * context, FILE * file)
{
  // check cm5 file size == 2049 bytes
  FILE *file_gfx;
  unsigned long file_size;

  File_error = 1;

  if (!CPC_check_AMSDOS(file, NULL, NULL, &file_size))
    file_size = File_length_file(file);
  if (file_size != 2049)
    return;

  // check existence of a .GFX file with the same name
  file_gfx = Open_file_read_with_alternate_ext(context, "gfx");
  if (file_gfx == NULL)
    return;
  if (!CPC_check_AMSDOS(file_gfx, NULL, NULL, &file_size))
    file_size = File_length_file(file_gfx);
  fclose(file_gfx);
  if (file_size != 18432)
    return;

  File_error = 0;
}


/**
 * Load Amstrad CPC "Mode 5" picture
 *
 * Only support 288x256 resolution as the Mode 5 Viewer app only handles this
 * single resolution.
 * @see https://www.cpc-power.com/index.php?page=detail&num=12905
 * @see https://github.com/cpcsdk/cpctools/blob/master/resources/mode5_viewer/fx_mode5.s
 */
void Load_CM5(T_IO_Context* context)
{
  // Ensure "8bit" constraint mode is switched on
  // Set palette to the CPC hardware colors
  // Load the palette data to the 4 colorlayers
  FILE *file;
  byte value = 0;
  int mod=0;
  short line = 0;
  int tx, ty;
  // for preview :
  byte ink0;
  byte ink1[256];
  byte ink2[256];
  byte ink3[256*6];

  if (!(file = Open_file_read(context)))
  {
      File_error = 1;
      return;
  }

  Pre_load(context, 48*6, 256, 2049, FORMAT_CM5, PIXEL_SIMPLE, 5);

  if (Config.Clear_palette)
  {
    memset(context->Palette,0,sizeof(T_Palette));
    // setup colors 0,1,2,3 to see something in the thumbnail preview of layer 5
    context->Palette[1].R = 60;
    context->Palette[2].B = 60;
    context->Palette[3].G = 60;
  }

  // Setup the palette (amstrad hardware palette)
  CPC_set_HW_palette(context->Palette + 0x40);

  First_color_in_palette = 64;

  if (CPC_check_AMSDOS(file, NULL, NULL, NULL))
    fseek(file, 128, SEEK_SET); // seek after AMSDOS header
  if (!Read_byte(file, &ink0))
    File_error = 2;

  // This forces the creation of 5 layers total :
  // Needed because the "pixel" functions will seek layer 4
  Set_loading_layer(context, 4);
  // Now select layer 1 again
  Set_loading_layer(context, 0);

  if (context->Type == CONTEXT_MAIN_IMAGE)
  {
    Set_image_mode(context, IMAGE_MODE_MODE5);

    // Fill layer with color we just read (Layer 1 - INK 0)
    for(ty=0; ty<context->Height; ty++)
      for(tx=0; tx<context->Width; tx++)
        Set_pixel(context, tx, ty, ink0);
  }

  for (line = 0; line < 256; )
  {
    if (!Read_byte(file, &value))
    {
      File_error = 1;
      break;
    }
    switch(mod)
    {
      case 0:
        // This is color for layer 2 - INK 1
        Set_loading_layer(context, 1);
        for(tx=0; tx<context->Width; tx++)
          Set_pixel(context, tx, line, value);
        ink1[line] = value;
        break;
      case 1:
        // This is color for layer 3 - INK 2
        Set_loading_layer(context, 2);
        for(tx=0; tx<context->Width; tx++)
          Set_pixel(context, tx, line, value);
        ink2[line] = value;
        break;
      default:
        // This is color for a block in layer 4 - INK 3
        Set_loading_layer(context, 3);
        for(tx=(mod-2)*48; tx<(mod-1)*48; tx++)
          Set_pixel(context, tx, line, value);
        ink3[line*6+(mod-2)] = value;
        break;
    }
    mod++;
    if (mod > 7)
    {
      mod = 0;
      line++;
    }
  }

  fclose(file);

  // Load the pixeldata to the 5th layer
  file = Open_file_read_with_alternate_ext(context, "gfx");
  if (file == NULL)
  {
    File_error = 1;
    return;
  }
  if (CPC_check_AMSDOS(file, NULL, NULL, NULL))
    fseek(file, 128, SEEK_SET); // seek after AMSDOS header
  Set_loading_layer(context, 4);

  if (context->Type == CONTEXT_PREVIEW)
    for (ty = 0; ty < 256; ty++)
      for (tx = 0; tx < 48*6; )
      {
        Read_byte(file, &value);
        for (mod = 0; mod < 4; mod++, tx++, value <<= 1)
        {
          switch(3 ^ (((value&0x80) >> 7) | ((value&0x8)>>2)))  // INK
          {
            case 0:
              Set_pixel(context, tx, ty, ink0);
              break;
            case 1:
              Set_pixel(context, tx, ty, ink1[ty]);
              break;
            case 2:
              Set_pixel(context, tx, ty, ink2[ty]);
              break;
            default:
              Set_pixel(context, tx, ty, ink3[ty*6+(tx/48)]);
          }
        }
      }
  else
    for (ty = 0; ty < 256; ty++)
      for (tx = 0; tx < 48*6; )
      {
        Read_byte(file, &value);
        Set_pixel(context, tx++, ty, 3 ^ (((value&0x80) >> 7) | ((value&0x8)>>2)));
        Set_pixel(context, tx++, ty, 3 ^ (((value&0x40) >> 6) | ((value&0x4)>>1)));
        Set_pixel(context, tx++, ty, 3 ^ (((value&0x20) >> 5) | ((value&0x2)>>0)));
        Set_pixel(context, tx++, ty, 3 ^ (((value&0x10) >> 4) | ((value&0x1)<<1)));
      }

  fclose(file);

}

/**
 * Save a CPC Mode 5 picture.
 * Resolution is fixed to 288x256.
 * The pictures uses 5 layers. 4 for defining the "inks"
 * the 5th to select one of the 4 inks.
 *
 * - Layer 1 : 1 color Only
 * - Layer 2 and 3 : 1 color/line
 * - Layer 4 : 1 color / 48x1 block
 * - Layer 5 : CPC mode 2 288x256 picture.
 *
 * The .CM5 file contains the inks from layers 1-4,
 * the .GFX file contains the pixel data in linear fashion
 * @todo Check picture has 5 layers
 * @todo Check the constraints on the layers
 * @see https://www.cpc-power.com/index.php?page=detail&num=12905
 */
void Save_CM5(T_IO_Context* context)
{
  FILE* file;
  int tx, ty;

  if (!(file = Open_file_write(context)))
  {
    File_error = 1;
    return;
  }

  // Write layer 0
  Set_saving_layer(context, 0);
  Write_byte(file, Get_pixel(context, 0, 0));
  for(ty = 0; ty < 256; ty++)
  {
    Set_saving_layer(context, 1);
    Write_byte(file, Get_pixel(context, 0, ty));
    Set_saving_layer(context, 2);
    Write_byte(file, Get_pixel(context, 0, ty));
    Set_saving_layer(context, 3);
    for(tx = 0; tx < 6; tx++)
    {
      Write_byte(file, Get_pixel(context, tx*48, ty));
    }
  }

  fclose(file);

  // Now the pixeldata
  if (!(file = Open_file_write_with_alternate_ext(context, "gfx")))
  {
    File_error = 2;
    return;
  }

  Set_saving_layer(context, 4);

  for (ty = 0; ty < 256; ty++)
  {
    for (tx = 0; tx < 48*6; tx+=4)
    {
      byte code = 0;
      byte pixel;

      pixel = 3-Get_pixel(context, tx+3, ty);
      code |= (pixel&2)>>1 | ((pixel & 1)<<4);
      pixel = 3-Get_pixel(context, tx+2, ty);
      code |= ((pixel&2)<<0) | ((pixel & 1)<<5);
      pixel = 3-Get_pixel(context, tx+1, ty);
      code |= ((pixel&2)<<1) | ((pixel & 1)<<6);
      pixel = 3-Get_pixel(context, tx, ty);
      code |= ((pixel&2)<<2) | ((pixel & 1)<<7);
      Write_byte(file, code);
    }
  }

  fclose(file);
  File_error = 0;

}


/**
 * Amstrad CPC 'PPH' for Perfect Pix.
 * This is a format designed by Rhino.
 * There are 3 modes:
 * - Mode 'R': 1:1 pixels, 16 colors from the CPC 27 color palette.
 *   (this is implemented on CPC as two pictures with wide pixels, the "odd" one
 *   being shifted half a pixel to the right), and flipping)
 * - Mode 'B0': wide pixels, up to 126 out of 378 colors.
 *   (this is implemented as two pictures with wide pixels, sharing the same 16
 *   color palette, and flipping)
 * - Mode 'B1': 1:1 pixels, 1 fixed color, up to 34 palettes of 9 colors
 *   (actually 4 colors + flipping)
 *
 * - The standard CPC formats can also be encapsulated into a PPH file.
 *
 * @see http://www.pouet.net/prod.php?which=67770#c766959
 */
void Test_PPH(T_IO_Context * context, FILE * file)
{
  FILE *file_oddeve;
  byte buffer[6];
  unsigned long file_size;
  unsigned int w, h;
  unsigned int expected;

  File_error = 1;

  // First check file size is large enough to hold the header
  file_size = File_length_file(file);
  if (file_size < 11) {
    File_error = 1;
    return;
  }

  // File is large enough for the header, now check if the data makes some sense
  if (!Read_bytes(file, buffer, 6))
    return;
  if (buffer[0] > 5) {
    // Unknown mode
    File_error = 2;
    return;
  }

  w = buffer[1] | (buffer[2] << 8);
  if (w < 2 || w > 384) {
    // Invalid width
    File_error = 3;
    return;
  }

  h = buffer[3] | (buffer[4] << 8);
  if (h < 1 || h > 272) {
    // Invalid height
    File_error = 4;
    return;
  }

  if (buffer[5] < 1 || buffer[5] > 28)
  {
    // Invalid palettes count
    File_error = 5;
    return;
  }
  expected = 6; // Size of header
  switch(buffer[0])
  {
    case 0:
    case 3:
    case 4:
      // Palette size should be 16 bytes, only 1 palette.
      if (buffer[5] != 1) {
        File_error = 7;
        return;
      }
      expected += 16;
      break;

    case 1:
    case 5:
      expected += buffer[5] * 5 - 1;
      break;

    case 2:
      // Palette size should be 2 bytes
      if (buffer[5] != 1) {
        File_error = 7;
        return;
      }
      expected += 2;
      break;
  }

  if (file_size != expected)
  {
    File_error = 6;
    return;
  }

  // check existence of .ODD/.EVE files with the same name
  // and the right size
  expected = w * h / 4;
  file_oddeve = Open_file_read_with_alternate_ext(context, "odd");
  if (file_oddeve == NULL)
    return;
  file_size = File_length_file(file_oddeve);
  fclose (file_oddeve);
  if (file_size != expected)
  {
    File_error = 8;
    return;
  }
  file_oddeve = Open_file_read_with_alternate_ext(context, "eve");
  if (file_oddeve == NULL)
    return;
  file_size = File_length_file(file_oddeve);
  fclose(file_oddeve);
  if (file_size != expected)
  {
    File_error = 8;
    return;
  }
  File_error = 0;
}


static uint8_t pph_blend(uint8_t a, uint8_t b)
{
	uint32_t h,l;
	if (a > b) { h = a; l = b; }
	else       { h = b; l = a; }

	return (23 * h + 9 * l) / 32;
}


void Load_PPH(T_IO_Context* context)
{
  FILE *file;
  FILE *feven;

  // Read in the header
  uint8_t mode;
  uint16_t width;
  uint16_t height;
  uint8_t npal;
  int i,j;
  uint8_t a,b,c,d;
  int file_size;
  uint8_t pl[16];

  static const T_Components CPCPAL[27] =
  {
      { 0x00, 0x02, 0x01 }, { 0x00, 0x02, 0x6B }, { 0x0C, 0x02, 0xF4 },
      { 0x6C, 0x02, 0x01 }, { 0x69, 0x02, 0x68 }, { 0x6C, 0x02, 0xF2 },
      { 0xF3, 0x05, 0x06 }, { 0xF0, 0x02, 0x68 }, { 0xF3, 0x02, 0xF4 },
      { 0x02, 0x78, 0x01 }, { 0x00, 0x78, 0x68 }, { 0x0C, 0x7B, 0xF4 },
      { 0x6E, 0x7B, 0x01 }, { 0x6E, 0x7D, 0x6B }, { 0x6E, 0x7B, 0xF6 },
      { 0xF3, 0x7D, 0x0D }, { 0xF3, 0x7D, 0x6B }, { 0xFA, 0x80, 0xF9 },
      { 0x02, 0xF0, 0x01 }, { 0x00, 0xF3, 0x6B }, { 0x0F, 0xF3, 0xF2 },
      { 0x71, 0xF5, 0x04 }, { 0x71, 0xF3, 0x6B }, { 0x71, 0xF3, 0xF4 },
      { 0xF3, 0xF3, 0x0D }, { 0xF3, 0xF3, 0x6D }, { 0xFF, 0xF3, 0xF9 }
  };

  if (!(file = Open_file_read(context)))
  {
      File_error = 1;
      return;
  }

  file_size=File_length_file(file);

  Read_byte(file, &mode);
  Read_word_le(file, &width);
  Read_word_le(file, &height);
  Read_byte(file, &npal);

  if (npal > 16)
      npal = 16;

  // Switch to the proper aspect ratio
  switch (mode)
  {
      case 0:
      case 4:
        context->Ratio = PIXEL_WIDE;
        width /= 2;
        break;

      case 2:
        context->Ratio = PIXEL_TALL;
        break;

      case 1:
      case 5:
      case 3:
        context->Ratio = PIXEL_SIMPLE;
        break;
  }

  Pre_load(context, width, height, file_size, FORMAT_PPH, context->Ratio, 0);

  // First of all, detect the mode
  // 0, 1, 2 > Load as with SCR files?
  // R(3)    > Load as single layer, square pixels, 16 colors
  // B0(4)   > Load as single layer, wide pixels, expand palette with colorcycling
  // B1(5)   > Load as ???
  //           Maybe special mode similar to mode5, with 2 layers + auto-flicker?

  switch (mode)
  {
      case 0:
      case 3: // R
          // 16-color palette
          for (i = 0; i < 16; i++)
          {
              uint8_t color;
              Read_byte(file, &color);
              context->Palette[i] = CPCPAL[color];
          }
          break;

      case 1:
      case 5: // B1
      {
          // Single or multiple 4-color palettes
          uint8_t base[4];
          for (j = 0; j < npal; j++)
          {
            for (i = 0; i < 4; i++)
            {
              Read_byte(file,&base[i]);
            }
            for (i = 0; i < 16; i++)
            {
              context->Palette[i + 16*j].R = pph_blend(
                  CPCPAL[base[i & 3]].R, CPCPAL[base[i >> 2]].R);
              context->Palette[i + 16*j].G = pph_blend(
                  CPCPAL[base[i & 3]].G, CPCPAL[base[i >> 2]].G);
              context->Palette[i + 16*j].B = pph_blend(
                  CPCPAL[base[i & 3]].B, CPCPAL[base[i >> 2]].B);
            }
            // TODO this byte marks where this palette stops being used and the
            // next starts. We must handle this!
            Read_byte(file,&pl[j]);
          }
          pl[npal - 1] = 255;
          break;
      }

      case 2:
          // Single 2-color palette
          break;

      case 4: // B0
      {
          // Single 16-color palette + flipping, need to expand palette and
          // setup colorcycling ranges.
          uint8_t base[16];
          for (i = 0; i < 16; i++)
          {
              Read_byte(file,&base[i]);
          }

          for (i = 0; i < 256; i++)
          {
              context->Palette[i].R = pph_blend(
                  CPCPAL[base[i & 15]].R, CPCPAL[base[i >> 4]].R);
              context->Palette[i].G = pph_blend(
                  CPCPAL[base[i & 15]].G, CPCPAL[base[i >> 4]].G);
              context->Palette[i].B = pph_blend(
                  CPCPAL[base[i & 15]].B, CPCPAL[base[i >> 4]].B);
          }
      }
      break;
  }

  fclose(file);

  // Load the picture data
  // There are two pages, each storing bytes in the CPC vram format but lines in
  // linear order.
  file = Open_file_read_with_alternate_ext(context, "odd");
  if (file == NULL)
  {
    File_error = 3;
    return;
  }
  feven = Open_file_read_with_alternate_ext(context, "eve");
  if (feven == NULL)
  {
    File_error = 4;
    fclose(file);
    return;
  }

  c = 0;
  d = 0;

  for (j = 0; j < height; j++)
  {
      for (i = 0; i < width;)
      {
          uint8_t even, odd;
          Read_byte(feven, &even);
          Read_byte(file, &odd);

          switch (mode)
          {
              case 4:
                a = ((even & 0x02) << 2) | ((even & 0x08) >> 2)
                  | ((even & 0x20) >> 3) | ((even & 0x80) >> 7);
                a <<= 4;
                a |= ((odd & 0x02) << 2) | (( odd & 0x08) >> 2)
                  | (( odd & 0x20) >> 3) | (( odd & 0x80) >> 7);

                b = ((even & 0x01) << 3) | ((even & 0x04) >> 1)
                  | ((even & 0x10) >> 2) | ((even & 0x40) >> 6);
                b <<= 4;
                b |= ((odd & 0x01) << 3) | (( odd & 0x04) >> 1)
                  | (( odd & 0x10) >> 2) | (( odd & 0x40) >> 6);

                Set_pixel(context, i++, j, a);
                Set_pixel(context, i++, j, b);
                break;

              case 3:
                a = ((even & 0x02) << 2) | ((even & 0x08) >> 2)
                  | ((even & 0x20) >> 3) | ((even & 0x80) >> 7);
                b = (( odd & 0x02) << 2) | (( odd & 0x08) >> 2)
                  | (( odd & 0x20) >> 3) | (( odd & 0x80) >> 7);
                c = ((even & 0x01) << 3) | ((even & 0x04) >> 1)
                  | ((even & 0x10) >> 2) | ((even & 0x40) >> 6);
                d = (( odd & 0x01) << 3) | (( odd & 0x04) >> 1)
                  | (( odd & 0x10) >> 2) | (( odd & 0x40) >> 6);
                Set_pixel(context, i++, j, j & 1 ? b : a);
                Set_pixel(context, i++, j, j & 1 ? a : b);
                Set_pixel(context, i++, j, j & 1 ? d : c);
                Set_pixel(context, i++, j, j & 1 ? c : d);
                break;

              case 5:
                if (d >= pl[c])
                {
                    d = 0;
                    c++;
                }
                a = ((even & 0x80) >> 6) | ((even & 0x08) >> 3);
                b = (( odd & 0x80) >> 6) | (( odd & 0x08) >> 3);
                Set_pixel(context, i++, j, a + (b << 2) + c * 16);
                a = ((even & 0x40) >> 5) | ((even & 0x04) >> 2);
                b = (( odd & 0x40) >> 5) | (( odd & 0x04) >> 2);
                Set_pixel(context, i++, j, a + (b << 2) + c * 16);
                a = ((even & 0x20) >> 4) | ((even & 0x02) >> 1);
                b = (( odd & 0x20) >> 4) | (( odd & 0x02) >> 1);
                Set_pixel(context, i++, j, a + (b << 2) + c * 16);
                a = ((even & 0x10) >> 3) | ((even & 0x01) >> 0);
                b = (( odd & 0x10) >> 3) | (( odd & 0x01) >> 0);
                Set_pixel(context, i++, j, a + (b << 2) + c * 16);

                break;

              default:
                File_error = 2;
                return;
          }

      }
      d++;
  }
  fclose(file);
  fclose(feven);

  File_error = 0;
}

/**
 * Not yet implemented
 */
void Save_PPH(T_IO_Context* context)
{
  (void)context; // unused
    // TODO

    // Detect mode
    // Wide pixels => B0 (4)
    // Square pixels:
    // - 16 colors used => R
    // - more colors used => B1 (if <16 colors per line)

    // Check palette
    // B0: use diagonal: 0, 17, 34, ... (assume the other are mixes)
    // R: use 16 used colors (or 16 first?)
    // B1: find the 16 colors used in a line? Or assume they are in-order already?
}

/** @} */

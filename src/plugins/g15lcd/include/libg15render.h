#ifndef LIBG15RENDER_H_
#define LIBG15RENDER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <string.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef TTF_SUPPORT
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_BITMAP_H
#endif

#define G15R_FONT_SUPPORT 1
#define G15R_FONT_API_V2 1

#define BYTE_SIZE 		8
#define G15_BUFFER_LEN  	1048
#define G15_LCD_OFFSET  	32
#define G15_LCD_HEIGHT  	43
#define G15_LCD_WIDTH   	160
#define G15_COLOR_WHITE 	0
#define G15_COLOR_BLACK 	1
#define G15_TEXT_SMALL  	0
#define G15_TEXT_MED    	1
#define G15_TEXT_LARGE  	2
#define G15_TEXT_HUGE 		3
#define G15_PIXEL_NOFILL 	0
#define G15_PIXEL_FILL  	1
#define G15_MAX_FACE		5
#define G15_FONT_HEADER_SIZE 	15
#define G15_CHAR_HEADER_SIZE 	4
#define G15_MAX_GLYPH		256

#define G15_JUSTIFY_LEFT	0
#define G15_JUSTIFY_CENTER	1
#define G15_JUSTIFY_RIGHT	2
/** \brief This structure holds the data need to render objects to the LCD screen.*/
  typedef struct g15canvas
  {
/** g15canvas::buffer[] is a buffer holding the pixel data to be sent to the LCD.*/
    unsigned char buffer[G15_BUFFER_LEN];
/** g15canvas::mode_xor determines whether xor processing is used in g15r_setPixel.*/
    int mode_xor;
/** g15canvas::mode_cache can be used to determine whether caching should be used in an application.*/
    int mode_cache;
/** g15canvas::mode_reverse determines whether color values passed to g15r_setPixel are reversed.*/
    int mode_reverse;
#ifdef TTF_SUPPORT
    FT_Library ftLib;
    FT_Face ttf_face[G15_MAX_FACE][sizeof (FT_Face)];
    int ttf_fontsize[G15_MAX_FACE];
#endif
  } g15canvas;

/** \brief Structure holding glyph data for g15render font types */
  typedef struct g15glyph {
      /** g15glyph::buffer holds glyph data */
    unsigned char *buffer;
    /** g15glyph::width - width of the glyph, without padding */
    unsigned char width;
    /** g15glyph::gap - recommended gap between this character and the next */
    unsigned char gap;
}g15glyph;

/** \brief Structure holding a single font.  One g15font struct is needed per size. */
typedef struct g15font {
    /** g15font::font_height - total max height of font in pixels */
    unsigned int font_height;
    /** g15font::ascender_height - height in pixels from baseline to the top pixel of an ascender character */
    unsigned int ascender_height;
    /** g15font::lineheight - height in pixels from decender to ascender */
    unsigned int lineheight;
    /** g15font::numchars - number of glyphs available in this font */
    unsigned int numchars;
    /** g15font::glyph - contains all glyphs available in this font */
    g15glyph glyph[G15_MAX_GLYPH]; // allow 256 chars.. ought to be enough for our purposes
    /** g15font::default_gap - default gap between glyphs (in pixels). */
    unsigned int default_gap;
    /** g15font::active - each active glyph is set to 1 else 0 */
    unsigned char active[G15_MAX_GLYPH];
    /** g15font::glyph_buffer memory pool for glyphs */
    char *glyph_buffer;
}g15font;

/** \brief Fills an area bounded by (x1, y1) and (x2, y2)*/
  void g15r_pixelReverseFill (g15canvas * canvas, int x1, int y1, int x2,
			      int y2, int fill, int color);
/** \brief Overlays a bitmap of size width x height starting at (x1, y1)*/
  void g15r_pixelOverlay (g15canvas * canvas, int x1, int y1, int width,
			  int height, short colormap[]);
/** \brief Draws a line from (px1, py1) to (px2, py2)*/
  void g15r_drawLine (g15canvas * canvas, int px1, int py1, int px2, int py2,
		      const int color);
/** \brief Draws a box bounded by (x1, y1) and (x2, y2)*/
  void g15r_pixelBox (g15canvas * canvas, int x1, int y1, int x2, int y2,
		      int color, int thick, int fill);
/** \brief Draws a circle centered at (x, y) with a radius of r*/
  void g15r_drawCircle (g15canvas * canvas, int x, int y, int r, int fill,
			int color);
/** \brief Draws a box with rounded corners bounded by (x1, y1) and (x2, y2)*/
  void g15r_drawRoundBox (g15canvas * canvas, int x1, int y1, int x2, int y2,
			  int fill, int color);
/** \brief Draws a completion bar*/
  void g15r_drawBar (g15canvas * canvas, int x1, int y1, int x2, int y2,
		     int color, int num, int max, int type);
/** \brief Draw a splash screen from 160x43 wbmp file*/
int g15r_loadWbmpSplash(g15canvas *canvas, char *filename);
/** \brief Draw an icon to the screen from a wbmp buffer*/
void g15r_drawIcon(g15canvas *canvas, char *buf, int my_x, int my_y, int width, int height);
/** \brief Draw a sprite to the screen from a wbmp buffer*/
void g15r_drawSprite(g15canvas *canvas, char *buf, int my_x, int my_y, int width, int height, int start_x, int start_y, int total_width);
/** \brief Load a wbmp file into a buffer*/
char *g15r_loadWbmpToBuf(char *filename, int *img_width, int *img_height);
/** \brief Draw a large number*/
void g15r_drawBigNum (g15canvas * canvas, unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, int color, int num);
/** \brief Draw an XBM image*/
void
g15r_drawXBM (g15canvas *canvas, unsigned char* data, int width, int height, int pos_x, int pos_y);

/** \brief Gets the value of the pixel at (x, y)*/
  int g15r_getPixel (g15canvas * canvas, unsigned int x, unsigned int y);
/** \brief Sets the value of the pixel at (x, y)*/
  void g15r_setPixel (g15canvas * canvas, unsigned int x, unsigned int y,
		      int val);
/** \brief Fills the screen with pixels of color*/
  void g15r_clearScreen (g15canvas * canvas, int color);
/** \brief Clears the canvas and resets the mode switches*/
  void g15r_initCanvas (g15canvas * canvas);

/** \brief Renders a character in the large font at (x, y)*/
  void g15r_renderCharacterLarge (g15canvas * canvas, int x, int y,
				  unsigned char character, unsigned int sx,
				  unsigned int sy);
/** \brief Renders a character in the meduim font at (x, y)*/
  void g15r_renderCharacterMedium (g15canvas * canvas, int x, int y,
				   unsigned char character, unsigned int sx,
				   unsigned int sy);
/** \brief Renders a character in the small font at (x, y)*/
  void g15r_renderCharacterSmall (g15canvas * canvas, int x, int y,
				  unsigned char character, unsigned int sx,
				  unsigned int sy);
/** \brief Renders a string with font size in row*/
  void g15r_renderString (g15canvas * canvas, unsigned char stringOut[],
			  int row, int size, unsigned int sx,
			  unsigned int sy);

/** \brief Load G15 font and return it in g15font struct */
g15font * g15r_loadG15Font(char *filename);
/** \brief Save font in font struct to given file, return 0 on success */
int g15r_saveG15Font(char *oFilename, g15font *font);
/** \brief De-allocate memory associated with font */
void g15r_deleteG15Font(g15font*font);
/** \brief Returns length (in pixels) of string if rendered in font 'font'  */
int g15r_testG15FontWidth(g15font *font,char *string);
/** \brief Returns g15font structure containing the default font at requested size if available */
g15font * g15r_requestG15DefaultFont (int size);
/** \brief render glyph 'character' from loaded font struct 'font'.  Returns width (in pixels) of rendered glyph */
int g15r_renderG15Glyph(g15canvas *canvas, g15font *font,
                        unsigned char character,
                        int top_left_pixel_x, int top_left_pixel_y,
                        int colour, int paint_bg);
/** \brief Render a string in font 'font' to canvas */
void g15r_G15FontRenderString (g15canvas * canvas, g15font *font,
                               char *string,
                               int row, unsigned int sx, unsigned int sy,
                               int colour, int paint_bg);
/** \brief Print a string using the G15 default font at size 'size' */
void g15r_G15FPrint (g15canvas *canvas, char *string, int x, int y,
                int size, int center, int colour, int row);

#ifdef TTF_SUPPORT
/** \brief Loads a font through the FreeType2 library*/
  int g15r_ttfLoad (g15canvas * canvas, char *fontname, int fontsize,
		     int face_num);
/** \brief Prints a string in a given font*/
  void g15r_ttfPrint (g15canvas * canvas, int x, int y, int fontsize,
		      int face_num, int color, int center,
		      char *print_string);
#endif

#ifdef __cplusplus
}
#endif

#endif				/*LIBG15RENDER_H_ */

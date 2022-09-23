#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>
#include <hb-ft.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "gl_errors.hpp"

#include <math.h>

#include <iostream>

#define FONT_SIZE 36

//This file exists to check that programs that use freetype / harfbuzz link properly in this base code.
//You probably shouldn't be looking here to learn to use either library.

// referenced https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c
// and https://freetype.org/freetype2/docs/tutorial/step1.html#section-6
// and https://learnopengl.com/In-Practice/Text-Rendering


#define WIDTH   240
#define HEIGHT  240

/* origin is the upper left corner */
unsigned char image[HEIGHT][WIDTH];

void draw_bitmap( FT_Bitmap* bitmap, FT_Int x, FT_Int y)
{
  FT_Int  i, j, p, q;
  FT_Int  x_max = x + bitmap->width;
  FT_Int  y_max = y + bitmap->rows;


  /* for simplicity, we assume that `bitmap->pixel_mode' */
  /* is `FT_PIXEL_MODE_GRAY' (i.e., not a bitmap font)   */

  for ( i = x, p = 0; i < x_max; i++, p++ )
  {
    for ( j = y, q = 0; j < y_max; j++, q++ )
    {
      if ( i < 0      || j < 0       ||
           i >= WIDTH || j >= HEIGHT )
        continue;

      image[j][i] |= bitmap->buffer[q * bitmap->width + p];
    }
  }
}


void show_image( void )
{
  int  i, j;

  for ( i = 0; i < HEIGHT; i++ )
  {
    for ( j = 0; j < WIDTH; j++ )
      putchar( image[i][j] == 0 ? ' '
                                : image[i][j] < 128 ? '+'
                                                    : '*' );
    putchar( '\n' );
  }
}


int main(int argc, char **argv) {
	// 0. Init freetype
	FT_Library library;
	FT_Init_FreeType( &library );

	FT_Error error;

	// 1. get font face 
	FT_Face	face;
	if((error = FT_New_Face( library,
				"/Users/ashk/Library/Fonts/Imbue-VariableFont_opsz,wght.ttf",
				0,
				&face ))) {std::cout<<"new face error"<<std::endl; abort();}

	// 2. set char size and screen resolution
	if((error = FT_Set_Char_Size(
          face,             /* handle to face object           */
          0,     /* char_width in 1/64th of points  */
          FONT_SIZE*64,     /* char_height in 1/64th of points */
          0,                /* horizontal device resolution    */
          0                 /* vertical device resolution      */
		))) {std::cout<<"set char error"<<std::endl; abort();}  

	// 3. make hb font and buffer so that we can fill in text u want to render
	hb_font_t *font = hb_ft_font_create(face,NULL);
	hb_buffer_t *buf = hb_buffer_create();
	hb_buffer_add_utf8(buf,"ha",-1, 0, -1);
	hb_buffer_guess_segment_properties (buf);

	// 4. shape using hb
	hb_shape (font, buf, NULL, 0);

	// 5. get glyph info & pos from the buffer
	unsigned int len = hb_buffer_get_length (buf);
	hb_glyph_info_t *info = hb_buffer_get_glyph_infos (buf, NULL);
	hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (buf, NULL);

	// debug print info & pos

	printf ("Raw buffer contents:\n");
	for (unsigned int i = 0; i < len; i++)
	{
	hb_codepoint_t gid   = info[i].codepoint;
	unsigned int cluster = info[i].cluster;
	double x_advance = pos[i].x_advance / 64.;
	double y_advance = pos[i].y_advance / 64.;
	double x_offset  = pos[i].x_offset / 64.;
	double y_offset  = pos[i].y_offset / 64.;

	char glyphname[32];
	hb_font_get_glyph_name (font, gid, glyphname, sizeof (glyphname));

	printf ("glyph='%s'	cluster=%d	advance=(%g,%g)	offset=(%g,%g)\n",
			glyphname, cluster, x_advance, y_advance, x_offset, y_offset);
	}

	printf ("Converted to absolute positions:\n");
	/* And converted to absolute positions. */
	{
	double current_x = 0;
	double current_y = 0;
	for (unsigned int i = 0; i < len; i++)
	{
		hb_codepoint_t gid   = info[i].codepoint;
		unsigned int cluster = info[i].cluster;
		double x_position = current_x + pos[i].x_offset / 64.;
		double y_position = current_y + pos[i].y_offset / 64.;


		char glyphname[32];
		hb_font_get_glyph_name (font, gid, glyphname, sizeof (glyphname));

		printf ("glyph='%s'	cluster=%d	position=(%g,%g)\n",
			glyphname, cluster, x_position, y_position);

		current_x += pos[i].x_advance / 64.;
		current_y += pos[i].y_advance / 64.;
	}
	}

	// 6. get to-be-rendered sentence's total length w. glyph
	uint32_t width = FONT_SIZE;
	for (uint8_t i = 0; i < len; ++i) {
		width += pos[i].x_advance >> 6;
	}
	std::cout<<"sentence's total length: "<<width<< std::endl;

	// 7. set pixel size & get each char's glyph
	FT_UInt pix_size = 16;
	if ((error = FT_Set_Pixel_Sizes(face,0,pix_size))){
		std::cout<<"set pixel size error"<<std::endl; abort();
	}

	FT_GlyphSlot slot = face->glyph;
	FT_Matrix matrix;
	double angle = 0;
	/* set up matrix */
	matrix.xx = (FT_Fixed)( cos( angle ) * 0x10000L );
	matrix.xy = (FT_Fixed)(-sin( angle ) * 0x10000L );
	matrix.yx = (FT_Fixed)( sin( angle ) * 0x10000L );
	matrix.yy = (FT_Fixed)( cos( angle ) * 0x10000L );

	FT_Vector pen;
	pen.x = 0;
	pen.y = 0;

	for (uint16_t n = 0; n < len; ++n) {
		/* set transformation */
    	FT_Set_Transform( face, &matrix, &pen );
		// load and render glyph img into slot (replace prev in slot)
		if ((error = FT_Load_Glyph(face, info[n].codepoint, FT_LOAD_RENDER))){
			std::cout<<"load char error at "<<n<<std::endl; abort();
		}

		draw_bitmap( &slot->bitmap,
                 slot->bitmap_left,
                 HEIGHT - slot->bitmap_top );

		pen.x += pos[n].x_advance;
		pen.y += pos[n].y_advance;
	}

	// show bit map in terminal
	show_image();

	/* Now use OpenGL to render this on screen! */
	// 8. generate texture
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RED,
		face->glyph->bitmap.width,
		face->glyph->bitmap.rows,
		0,
		GL_RED,
		GL_UNSIGNED_BYTE,
		face->glyph->bitmap.buffer
	);

	hb_buffer_destroy(buf);
	hb_font_destroy(font);

	std::cout << "It worked?" << std::endl;
}

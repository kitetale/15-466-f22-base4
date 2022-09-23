#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>
#include <hb-ft.h>

#include <iostream>

#define FONT_SIZE 36

//This file exists to check that programs that use freetype / harfbuzz link properly in this base code.
//You probably shouldn't be looking here to learn to use either library.

// referenced https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c
// and https://freetype.org/freetype2/docs/tutorial/step1.html#section-6

int main(int argc, char **argv) {
	// 0. Init freetype
	FT_Library library;
	FT_Init_FreeType( &library );

	FT_Error error;

	// 1. get font face 
	FT_Face	face;
	if(error = FT_New_Face( library,
				"/Users/ashk/Library/Fonts/Imbue-VariableFont_opsz,wght.ttf",
				0,
				&face )) {std::cout<<"new face error"<<std::endl; abort();}

	// 2. set char size and screen resolution
	if(error = FT_Set_Char_Size(
          face,             /* handle to face object           */
          0,     /* char_width in 1/64th of points  */
          FONT_SIZE*64,     /* char_height in 1/64th of points */
          0,                /* horizontal device resolution    */
          0                 /* vertical device resolution      */
		)) {std::cout<<"set char error"<<std::endl; abort();}  

	// 3. make hb font and buffer so that we can fill in text u want to render
	hb_font_t *font;
	hb_buffer_t *buf = hb_buffer_create();
	hb_buffer_add_utf8(buf,"Print this sentence for test!\n",-1, 0, -1);
	hb_buffer_guess_segment_properties (buf);

	// 4. shape using hb
	hb_shape (font, buf, NULL, 0);

	// 5. get glyph info & pos from the buffer
	unsigned int len = hb_buffer_get_length (buf);
	hb_glyph_info_t *info = hb_buffer_get_glyph_infos (buf, NULL);
	hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (buf, NULL);

	// debug print info & pos

	// printf ("Raw buffer contents:\n");
	// for (unsigned int i = 0; i < len; i++)
	// {
	// hb_codepoint_t gid   = info[i].codepoint;
	// unsigned int cluster = info[i].cluster;
	// double x_advance = pos[i].x_advance / 64.;
	// double y_advance = pos[i].y_advance / 64.;
	// double x_offset  = pos[i].x_offset / 64.;
	// double y_offset  = pos[i].y_offset / 64.;

	// char glyphname[32];
	// hb_font_get_glyph_name (hb_font, gid, glyphname, sizeof (glyphname));

	// printf ("glyph='%s'	cluster=%d	advance=(%g,%g)	offset=(%g,%g)\n",
	// 		glyphname, cluster, x_advance, y_advance, x_offset, y_offset);
	// }

	// printf ("Converted to absolute positions:\n");
	// /* And converted to absolute positions. */
	// {
	// double current_x = 0;
	// double current_y = 0;
	// for (unsigned int i = 0; i < len; i++)
	// {
	// 	hb_codepoint_t gid   = info[i].codepoint;
	// 	unsigned int cluster = info[i].cluster;
	// 	double x_position = current_x + pos[i].x_offset / 64.;
	// 	double y_position = current_y + pos[i].y_offset / 64.;


	// 	char glyphname[32];
	// 	hb_font_get_glyph_name (hb_font, gid, glyphname, sizeof (glyphname));

	// 	printf ("glyph='%s'	cluster=%d	position=(%g,%g)\n",
	// 		glyphname, cluster, x_position, y_position);

	// 	current_x += pos[i].x_advance / 64.;
	// 	current_y += pos[i].y_advance / 64.;
	// }
	// }

	// 6. get to-be-rendered sentence's total length w. glyph
	uint32_t width = FONT_SIZE;
	for (uint8_t i = 0; i < len; ++i) {
		width += pos[i].x_advance >> 6;
	}
	std::cout<<"sentence's total length: "<<width<< std::endl;

	// 7. set pixel size & get each char's glyph
	FT_UInt pix_size = 16;
	if (error = FT_Set_Pixel_Sizes(face,0,pix_size)){
		std::cout<<"set pixel size error"<<std::endl; abort();
	}

	int pen_x = 300;
	int pen_y = 200;

	for (uint16_t n = 0; n < len; ++n) {
		// load and render glyph img into slot (replace prev in slot)
		if (error = FT_Load_Glyph(face, info[n].codepoint, FT_LOAD_RENDER)){
			std::cout<<"load char error at "<<n<<std::endl; abort();
		}

		pen_x += pos[n].x_advance >> 6;
		pen_y += pos[n].y_advance >> 6;
	}

	hb_buffer_destroy(buf);
	hb_font_destroy(font);

	std::cout << "It worked?" << std::endl;
}

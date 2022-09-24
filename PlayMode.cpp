#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <random>

#include <SDL.h>


PlayMode::PlayMode() {
	
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {


	return false;
}

void PlayMode::update(float elapsed) {

}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
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
          0,                /* char_width in 1/64th of points  */
          FONT_SIZE*64,     /* char_height in 1/64th of points */
          0,                /* horizontal device resolution    */
          0                 /* vertical device resolution      */
		))) {std::cout<<"set char error"<<std::endl; abort();}  

	/* Now use OpenGL to render this on screen! */
	// 8. setup shader

	GLuint shader =  color_program->program;
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(WIDTH), 0.0f, static_cast<float>(HEIGHT));
    glUseProgram(shader);
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	// 9. setup VAO/VBO for texture quad
	glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	//background color
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// render text on screen
	float aspect = float(drawable_size.x) / float(drawable_size.y);
	constexpr float H = 0.09f;
	RenderText(shader, "This is example test", -aspect + 0.1f * H, -1.0 + 0.1f * H, 1.0f, glm::vec3(0.0f, 0.0f, 0.0f),face);


	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

}


void PlayMode::RenderText(GLuint shader, std::string text, float x, float y, float scale, glm::vec3 color, FT_Face face) {
	FT_Error error;

	// 3. make hb font and buffer
	hb_font_t *font = hb_ft_font_create(face,NULL);
	hb_buffer_t *buf = hb_buffer_create();
	hb_buffer_add_utf8(buf,text.c_str(),-1, 0, -1);
	hb_buffer_guess_segment_properties (buf);

	// 4. shape using hb
	hb_shape (font, buf, NULL, 0);

	// 5. get glyph info & pos from the buffer
	unsigned int len = hb_buffer_get_length (buf);
	hb_glyph_info_t *info = hb_buffer_get_glyph_infos (buf, NULL);
	hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (buf, NULL);

	// 6. set pixel size & get each char's glyph
	FT_UInt pix_size = 16;
	if ((error = FT_Set_Pixel_Sizes(face,0,pix_size))){
		std::cout<<"set pixel size error"<<std::endl; abort();
	}

	// set matrix for FT transform
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

	// 8. Loop through each char in text string
    for (unsigned int i = 0; i < len; i++) 
    {
		// set transform 
		FT_Set_Transform( face, &matrix, &pen );
		// load and render glyph img into slot (replace prev in slot)
		if ((error = FT_Load_Glyph(face, info[i].codepoint, FT_LOAD_RENDER))){
			std::cout<<"load char error at "<<i<<std::endl; abort();
		}

		// progress with x y 
		float xpos = x + pos[i].x_offset * scale;
		float ypos = y - (pix_size - pos[i].y_offset) * scale;
		// width height of char
        float w = pix_size * scale;
        float h = pix_size * scale;

		// texture info
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

        // update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },            
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }             
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, texture);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (pos[i].x_advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    
		pen.x += pos[i].x_advance;
		pen.y += pos[i].y_advance;
	}
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

	std::cout<<"drawing text ' "<<text<<"' done!"<<std::endl;
}
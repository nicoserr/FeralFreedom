#pragma once

#include <string>
#include <glm/glm.hpp>
#include <map>
#include <vector>
#include <array>
#include <utility>

#include <common.hpp>
#include "core/components.hpp"
#include "core/ecs.hpp"

class TextRenderer {
public:
    // Initialize the text renderer
    void init(const std::string& font_filename, unsigned int font_default_size);

    // Render the specified text
    void RenderText(std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color);

	void RenderCenteredText(const std::string& text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color);

	// Render specified text in a box
	void RenderBoxedText(const std::string& text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color, GLfloat box_width, GLfloat box_height);

private:

    struct Character {
	unsigned int TextureID;  // ID handle of the glyph texture
	glm::ivec2   Size;       // Size of glyph
	glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
	unsigned int Advance;    // Offset to advance to next glyph
	char character;
    };

    std::map<char, Character> m_ftCharacters;
    GLuint textShader;
    GLuint VAO, VBO; 
    GLuint program;
};

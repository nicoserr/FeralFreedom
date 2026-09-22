#include "text_renderer.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <gl3w.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include <fstream>
#include <sstream>

#include "render_system.hpp"


// Initialize FreeType and text rendering components
void TextRenderer::init(const std::string& font_filename, unsigned int font_default_size) {
    if (!loadEffectFromFile(shader_path("font.vs.glsl"), shader_path("font.fs.glsl"), textShader)) {
        fprintf(stderr, "Failed to load font shader program\n");
        assert(false);
    }

    glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl_has_errors();

    glUseProgram(textShader);
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(window_width_px), 0.0f, static_cast<float>(window_height_px));
    GLint projection_location = glGetUniformLocation(textShader, "projection");
    // std::cout << "projection_location: " << projection_location << std::endl;
    assert(projection_location > -1);
    glUniformMatrix4fv(projection_location, 1, GL_FALSE, glm::value_ptr(projection));
    gl_has_errors();

    // glDeleteShader(font_vertexShader);
    // glDeleteShader(font_fragmentShader);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    gl_has_errors();

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    gl_has_errors();

    // Initialize FreeType library
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        // Handle error
        std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return;
    }

    // Load font face
    FT_Face face;
    if (FT_New_Face(ft, font_filename.c_str(), 0, &face)) {
        // Handle error
        std::cerr << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return;
    }

    // Set font size
    FT_Set_Pixel_Sizes(face, 0, font_default_size);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    gl_has_errors();

    // Generate texture for each character
    for (unsigned char c = 0; c < 128; c++) {
        // Load character glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "ERROR::FREETYPE: Failed to load Glyph" << std::endl;
            continue;
        }

        // Create texture
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width, face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x),
            (char)c
        };

		m_ftCharacters.insert(std::pair<char, Character>(c, character));
    }
    glBindTexture(GL_TEXTURE_2D, 0); // Unbind any textures bound by text rendering

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    glBindVertexArray(0);
}

void TextRenderer::RenderCenteredText(const std::string& text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color) {
    // Step 1: Calculate the total width of the text at the given scale
    GLfloat textWidth = 0.0f;
    for (const char& c : text) {
        Character ch = m_ftCharacters[c];
        textWidth += (ch.Advance >> 6) * scale; // Advance is in 1/64 pixels
    }

    // Step 2: Calculate the starting x position to center the text on the screen
    GLfloat new_x = (x - textWidth) / 2.0f;

    // Step 3: Render the text starting from the calculated x position
    RenderText(text, new_x, y, scale, color);
}

// Render the specified text
void TextRenderer::RenderText(std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color) {
    // Activate corresponding render state
    // std::cout << "Rendering text: " << text << " at (" << x << ", " << y << ") with scale " << scale << std::endl;
    glUseProgram(textShader);
    glEnable(GL_BLEND);
    glBindVertexArray(VAO);
    GLint textColorLocation = glGetUniformLocation(textShader, "textColor");
    assert(textColorLocation > -1);
    glUniform3f(textColorLocation, color.r, color.g, color.b);

    GLint transformLoc = glGetUniformLocation(textShader, "transform");
    assert(transformLoc > -1);
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

    glActiveTexture(GL_TEXTURE0);


    GLfloat originalX = x;


    for (const char& c : text)
    {
        if(c == '\n') {
            x = originalX;
            y -= 50 * scale;
            continue;
        }
        Character ch = m_ftCharacters[c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
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
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // std::cout << "binding texture: " << ch.character << " = " << ch.TextureID << std::endl;

        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // std::cout << "Successfully rendered" << std::endl;
}

// void TextRenderer::RenderBoxedText(const std::string& text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color, GLfloat box_width, GLfloat box_height) {
//     glUseProgram(textShader);
//     glEnable(GL_BLEND);
//     glBindVertexArray(VAO);

//     GLint textColorLocation = glGetUniformLocation(textShader, "textColor");
//     assert(textColorLocation > -1);
//     glUniform3f(textColorLocation, color.r, color.g, color.b);

//     GLint transformLoc = glGetUniformLocation(textShader, "transform");
//     assert(transformLoc > -1);
//     glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

//     glActiveTexture(GL_TEXTURE0);

//     GLfloat original_x = x;  // Save the original X position for wrapping
//     GLfloat line_height = 0; // Keep track of line height
//     const GLfloat line_spacing_multiplier = 1.5f; // To adjust line spacing


//     std::string::size_type start = 0;
//     while (start < text.length()) {
//         // Find the next word
//         std::string::size_type end = text.find(' ', start);
//         if (end == std::string::npos) {
//             end = text.length();
//         }

//         std::string word = text.substr(start, end - start);
//         GLfloat word_width = 0.0f;

//         // Calculate the width of the word
//         for (char c : word) {
//             Character ch = m_ftCharacters[c];
//             word_width += (ch.Advance >> 6) * scale;
//         }

//         // Check if the word fits in the remaining space of the current line
//         if (x + word_width > original_x + box_width) {
//             // Move to the next line
//             x = original_x;
//             y -= line_height * line_spacing_multiplier; // Move down by the line height
//             line_height = 0.0f; // Reset line height
//         }

//         // Render the word
//         for (char c : word) {
//             Character ch = m_ftCharacters[c];

//             GLfloat xpos = x + ch.Bearing.x * scale;
//             GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

//             GLfloat w = ch.Size.x * scale;
//             GLfloat h = ch.Size.y * scale;

//             // Calculate the maximum height of the line
//             line_height = std::max(line_height, h);

//             // Update VBO for each character
//             GLfloat vertices[6][4] = {
//                 { xpos,     ypos + h,   0.0f, 0.0f },
//                 { xpos,     ypos,       0.0f, 1.0f },
//                 { xpos + w, ypos,       1.0f, 1.0f },

//                 { xpos,     ypos + h,   0.0f, 0.0f },
//                 { xpos + w, ypos,       1.0f, 1.0f },
//                 { xpos + w, ypos + h,   1.0f, 0.0f }
//             };

//             // Render glyph texture over quad
//             glBindTexture(GL_TEXTURE_2D, ch.TextureID);

//             // Update content of VBO memory
//             glBindBuffer(GL_ARRAY_BUFFER, VBO);
//             glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
//             glBindBuffer(GL_ARRAY_BUFFER, 0);

//             // Render quad
//             glDrawArrays(GL_TRIANGLES, 0, 6);

//             // Advance cursor for next glyph
//             x += (ch.Advance >> 6) * scale; // Advance in pixels
//         }

//         // Add a space after the word (except for the last word)
//         if (end < text.length()) {
//             Character spaceChar = m_ftCharacters[' '];
//             x += (spaceChar.Advance >> 6) * scale; // Add space width
//         }

//         start = end + 1; // Move to the next word
//     }

//     glBindVertexArray(0);
//     glBindTexture(GL_TEXTURE_2D, 0);
// }

void TextRenderer::RenderBoxedText(const std::string& text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color, GLfloat box_width, GLfloat box_height) {
    glUseProgram(textShader);
    glEnable(GL_BLEND);
    glBindVertexArray(VAO);

    GLint textColorLocation = glGetUniformLocation(textShader, "textColor");
    assert(textColorLocation > -1);
    glUniform3f(textColorLocation, color.r, color.g, color.b);

    GLint transformLoc = glGetUniformLocation(textShader, "transform");
    assert(transformLoc > -1);
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

    glActiveTexture(GL_TEXTURE0);

    GLfloat original_x = x;  // Save the original X position for wrapping
    GLfloat line_height = 0; // Keep track of line height
    const GLfloat line_spacing_multiplier = 1.5f; // To adjust line spacing

    std::string::size_type start = 0;
    while (start < text.length()) {
        // Check if we encounter a newline character
        if (text[start] == '\n') {
            // Move to the next line
            x = original_x;
            y -= line_height * line_spacing_multiplier; // Move down by the line height
            line_height = 0.0f; // Reset line height
            start++; // Skip the newline character
            continue;
        }

        // Find the next word
        std::string::size_type end = text.find(' ', start);
        if (end == std::string::npos) {
            end = text.length();
        }

        std::string word = text.substr(start, end - start);
        GLfloat word_width = 0.0f;

        // Calculate the width of the word
        for (char c : word) {
            Character ch = m_ftCharacters[c];
            word_width += (ch.Advance >> 6) * scale;
        }

        // Check if the word fits in the remaining space of the current line
        if (x + word_width > original_x + box_width) {
            // Move to the next line
            x = original_x;
            y -= line_height * line_spacing_multiplier; // Move down by the line height
            line_height = 0.0f; // Reset line height
        }

        // Render the word
        for (char c : word) {
            Character ch = m_ftCharacters[c];

            GLfloat xpos = x + ch.Bearing.x * scale;
            GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

            GLfloat w = ch.Size.x * scale;
            GLfloat h = ch.Size.y * scale;

            // Calculate the maximum height of the line
            line_height = std::max(line_height, h);

            // Update VBO for each character
            GLfloat vertices[6][4] = {
                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos,     ypos,       0.0f, 1.0f },
                { xpos + w, ypos,       1.0f, 1.0f },

                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos + w, ypos,       1.0f, 1.0f },
                { xpos + w, ypos + h,   1.0f, 0.0f }
            };

            // Render glyph texture over quad
            glBindTexture(GL_TEXTURE_2D, ch.TextureID);

            // Update content of VBO memory
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            // Render quad
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Advance cursor for next glyph
            x += (ch.Advance >> 6) * scale; // Advance in pixels
        }

        // Add a space after the word (except for the last word)
        if (end < text.length()) {
            Character spaceChar = m_ftCharacters[' '];
            x += (spaceChar.Advance >> 6) * scale; // Add space width
        }

        start = end + 1; // Move to the next word
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
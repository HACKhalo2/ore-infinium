/******************************************************************************
 *   Copyright (C) 2013 by Shaun Reich <sreich@kde.org>                       *
 *                                                                            *
 *   This program is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU General Public License as           *
 *   published by the Free Software Foundation; either version 2 of           *
 *   the License, or (at your option) any later version.                      *
 *                                                                            *
 *   This program is distributed in the hope that it will be useful,          *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU General Public License for more details.                             *
 *                                                                            *
 *   You should have received a copy of the GNU General Public License        *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *****************************************************************************/

#include "tilerenderer.h"

#include "debug.h"
#include "game.h"
#include "camera.h"
#include "shader.h"
#include "image.h"

#include "src/world.h"
#include "src/player.h"

#include "settings/settings.h"

#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

TileRenderer::TileRenderer(World* world, Camera* camera, Player* mainPlayer)
    :   m_world(world),
        m_mainPlayer(mainPlayer)
{
    m_shader = new Shader("tilerenderer.vert", "tilerenderer.frag");
    setCamera(camera);

    initGL();

    float scale = 1.0f;
    m_modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scale));
    m_projectionMatrix = glm::ortho(0.0f, float(Settings::instance()->screenResolutionWidth), float(Settings::instance()->screenResolutionHeight), 0.0f, -1.0f, 1.0f);
    loadTileSheets();
}

TileRenderer::~TileRenderer()
{
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ebo);

    glDeleteVertexArrays(1, &m_vao);
}

void TileRenderer::setCamera(Camera* camera)
{
    m_camera = camera;
    m_camera->setShader(m_shader);
}

void TileRenderer::loadTileSheets()
{
    glGenTextures(1, &m_tileMapTexture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_tileMapTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    const GLint level = 0;
    glTexImage3D(GL_TEXTURE_2D_ARRAY, level, GL_RGBA, TILESHEET_WIDTH, TILESHEET_HEIGHT, Block::blockTypeMap.size(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0 /* if it's null it tells GL we will send in 2D images as elements one by one, later */);

for (auto & tile : Block::blockTypeMap) {
        loadTileSheet(tile.second.texture, tile.first);
    }
}

void TileRenderer::loadTileSheet(const std::string& fileName, Block::BlockType type)
{
    Image* image = new Image(Block::blockTypeMap.at(type).texture);

    const GLint level = 0;
    const GLint xoffset = 0;
    const GLint yoffset = 0;
    const GLint zoffset = m_tileSheetCount;
    const GLsizei depth = 1;

    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, level, xoffset, yoffset, zoffset, TILESHEET_WIDTH, TILESHEET_HEIGHT, depth, GL_BGRA, GL_UNSIGNED_BYTE, image->bytes());

    m_tileSheets[type] = image;

    ++m_tileSheetCount;
}

GLuint TileRenderer::fboTexture()
{
    return m_fboTexture;
}

void TileRenderer::render()
{
//    m_shader->bindProgram();

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, m_rb);
    glClearColor(0.f, 0.f, 0.f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

//    Debug::log() << "OFFSET: " << offset.x << " Y : " << offset.y;
    Debug::checkGLError();
    glm::vec2 playerPosition = m_mainPlayer->position();
    int tilesBeforeX = playerPosition.x / Block::BLOCK_SIZE;
    //row
    int tilesBeforeY = playerPosition.y / Block::BLOCK_SIZE;

    // -1 so that we render an additional row and column..to smoothly scroll
    const int startRow = tilesBeforeY - ((Settings::instance()->screenResolutionHeight * 0.5) / Block::BLOCK_SIZE) - 1;
    const int endRow = tilesBeforeY + ((Settings::instance()->screenResolutionHeight * 0.5) / Block::BLOCK_SIZE);

    //columns are our X value, rows the Y
    const int startColumn = tilesBeforeX - ((Settings::instance()->screenResolutionWidth * 0.5) / Block::BLOCK_SIZE) - 1;
    const int endColumn = tilesBeforeX + ((Settings::instance()->screenResolutionWidth * 0.5) / Block::BLOCK_SIZE);
//    Debug:: log() << "starRow: " << startRow << "endrow: " << endRow << "startcol: " << startColumn << " endcol: " << endColumn;

    if (std::abs(startColumn) != startColumn) {
        std::cout << "FIXME, WENT INTO NEGATIVE COLUMN!!";
        assert(0);
    } else if (std::abs(startRow) != startRow) {
        std::cout << "FIXME, WENT INTO NEGATIVE ROW!!";
        assert(0);
    }

    int drawingRow = 0;

    int index = 0;

    Debug::checkGLError();
    // [y*rowlength + x]
    for (int currentRow = startRow; currentRow < endRow; ++currentRow) {
        int drawingColumn = 0;
        for (int currentColumn = startColumn; currentColumn < endColumn; ++currentColumn) {

            // vertices that will be uploaded.
            Vertex vertices[4];

            // vertices[n][0] -> X, and [1] -> Y
            // vertices[0] -> top left
            // vertices[1] -> bottom left
            // vertices[2] -> bottom right
            // vertices[3] -> top right

            float positionX = Block::BLOCK_SIZE * drawingColumn;
            float positionY = Block::BLOCK_SIZE * drawingRow;

            float x = positionX;
            float width = x +  Block::BLOCK_SIZE;

            float y = positionY;
            float height = y  +  Block::BLOCK_SIZE;

            vertices[0].x = x; // top left X
            vertices[0].y = y; //top left Y

            vertices[1].x = x; // bottom left X
            vertices[1].y = height; // bottom left Y

            vertices[2].x = width; // bottom right X
            vertices[2].y = height; //bottom right Y

            vertices[3].x = width; // top right X
            vertices[3].y = y; // top right Y

            Debug::checkGLError();

            // copy color to the buffer
            for (size_t i = 0; i < sizeof(vertices) / sizeof(*vertices); i++) {
                //        *colorp = color.bgra;
                uint8_t red = 255;
                uint8_t blue = 255;
                uint8_t green = 255;
                uint8_t alpha = 255;
                int32_t color = red | (green << 8) | (blue << 16) | (alpha << 24);
                vertices[i].color = color;
            }

            //tilesheet index/row, column
            int row = 1;
            int column = 5;

            int blockIndex = currentColumn * WORLD_ROWCOUNT + currentRow;
            Block& block = m_world->m_blocks[blockIndex];

            const float tileWidth = 1.0f / TILESHEET_WIDTH * 16.0f;
            const float tileHeight = 1.0f / TILESHEET_HEIGHT * 16.0f;

            float xPadding = 1.0f / TILESHEET_WIDTH * 1.0f * (column + 1);
            float yPadding = 1.0f / TILESHEET_HEIGHT * 1.0f * (row + 1);

            const float tileLeft = (column *  tileWidth) + xPadding;
            const float tileRight = tileLeft + tileWidth;
            const float tileTop = 1.0f - ((row * tileHeight)) - yPadding;
            const float tileBottom = tileTop - tileHeight;

            // copy texcoords to the buffer
            vertices[0].u = vertices[1].u = tileLeft;
            vertices[0].v = vertices[3].v = tileTop;
            vertices[1].v = vertices[2].v = tileBottom;
            vertices[2].u = vertices[3].u = tileRight;

            //FIXME: use tile type index
            vertices[0].w = vertices[1].w = vertices[2].w = vertices[3].w = block.primitiveType;

            Debug::checkGLError();
            // finally upload everything to the actual vbo
            glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
            glBufferSubData(
                GL_ARRAY_BUFFER,
                sizeof(vertices) * index,
                sizeof(vertices),
                vertices);
            Debug::checkGLError();

            ++index;
            ++drawingColumn;
        }
        ++drawingRow;
    }

    Debug::checkGLError();
    ////////////////////////////////FINALLY RENDER IT ALL //////////////////////////////////////////
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBindVertexArray(m_vao);

    Debug::checkGLError();

    m_shader->bindProgram();

    // for smooth per-pixel scrolling, a value from 0-15 and when it's 16 we snap to the next tile
    glm::ivec2 offset = m_world->tileOffset(m_mainPlayer);
    GLint offsetLoc = glGetUniformLocation(m_shader->shaderProgram(), "offset");
    glUniform2f(offsetLoc, GLfloat(offset.x), GLfloat(offset.y));

    Debug::checkGLError();
//    Debug::log() << "RENDERING TILECOUNT: " << m_tileCount;

    glDrawElements(
        GL_TRIANGLES,
        6 * (index), // 6 indices per 2 triangles
        GL_UNSIGNED_INT,
        (const GLvoid*)0);

    m_shader->unbindProgram();
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDisable(GL_BLEND);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    Debug::checkGLError();
}

void TileRenderer::initGL()
{
    Debug::checkGLError();

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glGenRenderbuffers(1, &m_rb);
    glBindRenderbuffer(GL_RENDERBUFFER, m_rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, 1600, 900);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rb);

    GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, buffers);

    glGenTextures(1, &m_fboTexture);
    glBindTexture(GL_TEXTURE_2D, m_fboTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1600, 900, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // Attach the texture to the FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fboTexture, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    assert(status == GL_FRAMEBUFFER_COMPLETE);

    //////////////////////

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        m_maxTileCount * 4 * sizeof(Vertex),
        NULL,
        GL_DYNAMIC_DRAW);

    Debug::checkGLError();

    std::vector<u32> indicesv;

    // prepare and upload indices as a one time deal
    const u32 indices[] = { 0, 1, 2, 0, 2, 3 }; // pattern for a triangle array
    // for each possible sprite, add the 6 index pattern
    for (size_t j = 0; j < m_maxTileCount; j++) {
        for (size_t i = 0; i < sizeof(indices) / sizeof(*indices); i++) {
            indicesv.push_back(4 * j + indices[i]);
        }
    }

    glGenBuffers(1, &m_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indicesv.size()*sizeof(u32),
        indicesv.data(),
        GL_STATIC_DRAW);

    Debug::checkGLError();

    size_t buffer_offset = 0;

    GLint pos_attrib = glGetAttribLocation(m_shader->shaderProgram(), "position");
    glEnableVertexAttribArray(pos_attrib);
    glVertexAttribPointer(
        pos_attrib,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (const GLvoid*)buffer_offset);
    buffer_offset += sizeof(f32) * 2;

    GLint color_attrib = glGetAttribLocation(m_shader->shaderProgram(), "color");

    Debug::checkGLError();

    glEnableVertexAttribArray(color_attrib);
    glVertexAttribPointer(
        color_attrib,
        4,
        GL_UNSIGNED_BYTE,
        GL_TRUE,
        sizeof(Vertex),
        (const GLvoid*)buffer_offset);
    buffer_offset += sizeof(u32);

    Debug::checkGLError();

    GLint texcoord_attrib = glGetAttribLocation(m_shader->shaderProgram(), "texcoord");
    glEnableVertexAttribArray(texcoord_attrib);
    glVertexAttribPointer(
        texcoord_attrib,
        //2 + 1 because we need a depth the array of 2d textures
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (const GLvoid*)buffer_offset);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    Debug::checkGLError();
}

/******************************************************************************
 *   Copyright (C) 2013 by Shaun Reich <sreich@kde.org>                       *
 *   Based on original code from Ben English <benjamin.english@oit.edu>       *
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

#ifndef SPRITESHEETMANAGER_H
#define SPRITESHEETMANAGER_H

#include <FreeImage.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>

#include <map>
#include <string>
#include <vector>

class Sprite;

class SpriteSheetManager
{
public:
    static SpriteSheetManager* instance();
    virtual ~SpriteSheetManager();

    /**
     * Indicative of which spritesheet this sprite is a type of.
     * aka what it is considered and which category it will fall under.
     */
    enum class SpriteSheetType {
        Character,
        Entity
    };

    /**
     * Registers the sprite, taking over rendering control.
     * SpriteSheetManager will then fondle the privates of Sprite, by asking which frameName it is,
     * which position, etc.
     * @p spriteSheetType which sprite sheet it is considered to be from. A character, entity, etc.
     */
    void registerSprite(SpriteSheetType spriteSheetType, Sprite* sprite);

    /**
     * Renders PC's and NPC's
     */
    void renderCharacters();

    /**
     * Renders all entities, from the respective spritesheets
     */
    void renderEntitites();

private:
    struct SpriteSheet {
        SpriteSheetType type;
        GLuint textureID;
        unsigned int width;
        unsigned int height;
    };

    /**
     * In-mem representation of each sprite frame from the spritesheets
     * So we know the position within the frame (x,y) and size of it.
     */
    struct SpriteFrameIdentifier {
        unsigned int x;
        unsigned int y;
        unsigned int width;
        unsigned int height;
    };

    void parseAllSpriteSheets();
    std::map<std::string, SpriteFrameIdentifier> parseSpriteSheet(const std::string& filename);
    void loadAllSpriteSheets();

    /**
     * free the memory for a texture
     */
    void unloadSpriteSheet(SpriteSheetType type);

    /**
     * free all texture memory
     */
    void unloadAllSpriteSheets();

    void bindSpriteSheet(SpriteSheetType type);

    /**
     * Load an image as an OpenGL texture and make it the current texture
     * If @p texID is already in use, it will be unloaded and replaced with this texture
     *
     * @p filename path relative or absolute including extension to the image to load
     * @p texID the GLint handle to the resulting texture
     * @p image_format which format the image on disk is in. Default is GL_BGRA
     * @p internal_format the format to tell OpenGL how it should store it. Default GL_RGBA
     * @p level what mipmapping level to utilize. Default 0.
     * @p border border size. Default 0
     */
    void loadSpriteSheet(const std::string& filename, SpriteSheetType type, GLenum image_format = GL_BGRA, GLint internal_format = GL_RGBA, GLint level = 0, GLint border = 0);

    glm::vec2 spriteSheetSize(SpriteSheetType type);

    /**
     * It is a container of the GL textures, one for each spritesheet..
     * there will only be a handful. 1 texture per sprite sheet type
     * aka one for characters, 1 for entity, etc.
     */
    std::map<SpriteSheetType, SpriteSheet> m_spriteSheetTextures;

    /**
     * Map containing all the sprite frame names and their properties for this
     * particular spritesheet. e.g. x, y, width, height.
     */
    std::map<std::string, SpriteFrameIdentifier> m_spriteSheetCharactersDescription;

    std::vector<Sprite*> m_characterSprites;

    SpriteSheetManager();
    SpriteSheetManager(const SpriteSheetManager& tm) {};
    SpriteSheetManager& operator=(const SpriteSheetManager& tm);

    static SpriteSheetManager* s_instance;
    friend class Sprite;
};

#endif

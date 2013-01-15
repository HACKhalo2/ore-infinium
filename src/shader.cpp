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

#include "shader.h"

#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <vector>

#include "debug.h"

Shader::Shader()
{

}

Shader::~Shader()
{

}

// loadFile - loads text file into char* fname
// allocates memory - so need to delete after use
// size of file returned in fSize
char* Shader::loadFile(const char* fname, GLint* fSize)
{
    std::ifstream::pos_type size;
    char * memblock = 0;
    std::string text;

    // file read based on example in cplusplus.com tutorial
    std::ifstream file(fname, std::ios::in | std::ios::binary | std::ios::ate);
    if (file.is_open()) {
        size = file.tellg();
        *fSize = (GLuint) size;
        memblock = new char [size];
        file.seekg(0, std::ios::beg);
        file.read(memblock, size);
        file.close();
        Debug::log(Debug::Area::Graphics) << "shader : " << fname << " loaded successfully";
        text.assign(memblock);
    } else {
        Debug::fatal(false,  Debug::Area::Graphics, "failed to load shader: " + std::string(fname));
    }
    return memblock;
}

GLuint Shader::loadShaders(const char* vertexShader, const char* fragmentShader)
{
    GLuint program, vertex_shader, fragment_shader;

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);

    GLint vertLength;
    GLint fragLength;

    char* vertSource;
    char* fragSource;

    vertSource = loadFile(vertexShader, &vertLength);
    fragSource = loadFile(fragmentShader, &fragLength);

    const char* vertSourceConst = vertSource;
    const char* fragSourceConst = fragSource;

    glShaderSource(vertex_shader, 1, &vertSourceConst, &vertLength);
    glCompileShader(vertex_shader);

    if (!checkShaderCompileStatus(vertex_shader)) {
        assert(0);
    } else {
        Debug::log(Debug::Area::Graphics) << "vertex shader compiled!";
    }

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragSourceConst, &fragLength);
    glCompileShader(fragment_shader);

    if (!checkShaderCompileStatus(fragment_shader)) {
        assert(0);
    } else {
        Debug::log(Debug::Area::Graphics) << "fragment shader compiled!";
    }

    program = glCreateProgram();

    // attach shaders
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);

    // link the program and check for errors
    glLinkProgram(program);

    if (checkProgramLinkStatus(program)) {
        Debug::log(Debug::Area::Graphics) << "shader program linked!";
    } else {
        Debug::fatal(false, Debug::Area::Graphics, "shader program link FAILURE");
    }

    delete [] vertSource;
    delete [] fragSource;

    return program;
}

bool Shader::checkShaderCompileStatus(GLuint obj)
{
    GLint status;
    glGetShaderiv(obj, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint length;
        glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(length);
        glGetShaderInfoLog(obj, length, &length, &log[0]);
        std::cerr << &log[0];
        return false;
    }
    return true;
}

bool Shader::checkProgramLinkStatus(GLuint obj)
{
    GLint status;
    glGetProgramiv(obj, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        GLint length;
        glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(length);
        glGetProgramInfoLog(obj, length, &length, &log[0]);
        std::cerr << &log[0];
        return false;
    }
    return true;
}

void Shader::printShaderInfoLog(GLuint shader)
{
    int infoLogLen = 0;
    int charsWritten = 0;
    GLchar *infoLog;

    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);

    // should additionally check for OpenGL errors here

    if (infoLogLen > 0) {
        infoLog = new GLchar[infoLogLen];
        // error check for fail to allocate memory omitted
        glGetShaderInfoLog(shader, infoLogLen, &charsWritten, infoLog);
        //        std::cout << "InfoLog:" << std::endl << infoLog << std::endl;
        std::cout << infoLog;
        delete [] infoLog;
    }

    // should additionally check for OpenGL errors here
}
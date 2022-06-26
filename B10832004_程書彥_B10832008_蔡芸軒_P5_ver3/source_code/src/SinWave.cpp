#include <iostream>
#include <FileSystem>
#include <fstream>
#include <Fl/fl.h>
#include <windows.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glu.h>
#include "SinWave.H"

SinWave::SinWave(){
    for (int i = 0;i < 1800;i++)vertices[i] = 0;
    for (int i = -50;i < 50;i++) {
        int idx = (i + 50) * 18;
        vertices[idx + 0] = i / 100.0;   vertices[idx + 2] = -1;
        vertices[idx + 3] = i / 100.0;   vertices[idx + 5] = 1;
        vertices[idx + 6] = (1 + i) / 100.0;  vertices[idx + 8] = -1;
        vertices[idx + 9] = i / 100.0;   vertices[idx + 11] = 1;
        vertices[idx + 12] = (1 + i) / 100.0;   vertices[idx + 14] = -1;
        vertices[idx + 15] = (1 + i) / 100.0;  vertices[idx + 17] = 1;
    }
    reflectSkybox = new TextureCubeMap();
    shader = new Shader("../src/shaders/sinWave.vert", nullptr, nullptr, nullptr, "../src/shaders/sinWave.frag");
    vao = new VAO();
    vbo = new VBO();
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    vao->unbind();
}

void SinWave::draw() {
    t += 0.1;
    shader->Use();
    GLfloat view[16]; GLfloat projection[16];
    glGetFloatv(GL_PROJECTION_MATRIX, projection);
    glGetFloatv(GL_MODELVIEW_MATRIX, view);
    glUniform1f(glGetUniformLocation(shader->ID, "amplitude"), Global::amplitude);
    glUniform1f(glGetUniformLocation(shader->ID, "height"), Global::height);
    glUniform1f(glGetUniformLocation(shader->ID, "wavelength"), Global::waveLen);
    glUniform1f(glGetUniformLocation(shader->ID, "speed"), Global::speed);
    glUniform1f(glGetUniformLocation(shader->ID, "time"), t);

    glUniformMatrix4fv(glGetUniformLocation(shader->ID, "view"), 1, GL_FALSE, view);
    glUniformMatrix4fv(glGetUniformLocation(shader->ID, "projection"), 1, GL_FALSE, projection);
    glUniformMatrix4fv(glGetUniformLocation(shader->ID, "model"), 1, GL_FALSE, glm::value_ptr(model_matrix));
    glUniform3f(glGetUniformLocation(shader->ID, "cameraPos"), Global::position.x, Global::position.y, Global::position.z);
    glUniform1i(glGetUniformLocation(shader->ID, "reflectSkybox"), 0);

    vao->bind();
    //reflectSkybox->bind(0);
    glDrawArrays(GL_TRIANGLES, 0, 600);
    vao->unbind();
    
}
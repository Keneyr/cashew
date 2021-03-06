// Shipeng Xu
// billhsu.x@gmail.com
// Thanks to https://github.com/Kopakc/Visu

#include "DepthPeeling.h"
#include <stdio.h>
#include "OpenGL/Util/Utility.h"

DepthPeeling::~DepthPeeling() {
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteTextures(1, &colorTexture1);
    glDeleteTextures(1, &colorTexture2);
    glDeleteTextures(1, &depthTexture1);
    glDeleteTextures(1, &depthTexture2);
    glDeleteTextures(1, &compoTexture1);
    glDeleteTextures(1, &compoDepth1);
}
void DepthPeeling::init() {
    compoProgram.loadFromFile(GL_VERTEX_SHADER, "Shader/compo.vs");
    compoProgram.loadFromFile(GL_FRAGMENT_SHADER, "Shader/compo.fs");
    compoProgram.createProgram();

    glGenFramebuffers(1, &framebuffer);
    colorTexture1 = createTexture();
    colorTexture2 = createTexture();
    depthTexture1 = createTexture();
    depthTexture2 = createTexture();
    compoTexture1 = createTexture();
    compoDepth1 = createTexture();
    recreateForResolution(windowWidth, windowHeight);

    uvArray[0] = 0.0f;
    uvArray[1] = 0.0f;
    uvArray[2] = 1.0f;
    uvArray[3] = 0.0f;
    uvArray[4] = 1.0f;
    uvArray[5] = 1.0f;
    uvArray[6] = 0.0f;
    uvArray[7] = 1.0f;

    verticesArray[0] = -1;
    verticesArray[1] = -1;
    verticesArray[2] = 0;
    verticesArray[3] = 1;
    verticesArray[4] = -1;
    verticesArray[5] = 0;
    verticesArray[6] = 1;
    verticesArray[7] = 1;
    verticesArray[8] = 0;
    verticesArray[9] = -1;
    verticesArray[10] = 1;
    verticesArray[11] = 0;

    VBOInfo.vertexBufferSize = sizeof(verticesArray) / sizeof(float);
    VBOInfo.vertexBufferData = verticesArray;
    VBOInfo.uvBufferSize = sizeof(uvArray) / sizeof(float);
    VBOInfo.uvBufferData = uvArray;
    VBOInfo.indexBufferSize = sizeof(indices) / sizeof(int);
    VBOInfo.indexBufferData = indices;

    unsigned int flags = HardwareBuffer::FLAG_VERTEX_BUFFER |
                         HardwareBuffer::FLAG_UV_BUFFER |
                         HardwareBuffer::FLAG_INDEX_BUFFER;

    buffer.setVBOLocation(HardwareBuffer::FLAG_VERTEX_BUFFER, 0);
    buffer.setVBOLocation(HardwareBuffer::FLAG_UV_BUFFER, 1);

    buffer.setVBOUnitSize(HardwareBuffer::FLAG_VERTEX_BUFFER, 3);
    buffer.setVBOUnitSize(HardwareBuffer::FLAG_UV_BUFFER, 2);

    buffer.initVBO(VBOInfo, flags);
}
void DepthPeeling::setColorTextureSize(GLuint texture, int width, int height) {
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, 0);
}

void DepthPeeling::setDepthTextureSize(GLuint texture, int width, int height) {
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, 0);
}
GLuint DepthPeeling::createTexture() {
    GLuint texture;
    glGenTextures(1, &texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    return texture;
}

void DepthPeeling::recreateForResolution(int width, int height) {
    printf("recreateForResolution %d, %d\n", width, height);
    setColorTextureSize(colorTexture1, width, height);
    setColorTextureSize(colorTexture2, width, height);
    setDepthTextureSize(depthTexture1, width, height);
    setDepthTextureSize(depthTexture2, width, height);
    setColorTextureSize(compoTexture1, width, height);
    setDepthTextureSize(compoDepth1, width, height);
}

void DepthPeeling::clearTextures(GLuint depthTexture, GLuint colorTexture) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           depthTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           colorTexture, 0);
    glClearColor(backgroundColor[0], backgroundColor[1], backgroundColor[2],
                 backgroundColor[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void DepthPeeling::peelingPass(GLuint depthTexture, GLuint colorTexture,
                               GLuint peelDepthTexture) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           depthTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           colorTexture, 0);
    glClearColor(backgroundColor[0], backgroundColor[1], backgroundColor[2],
                 backgroundColor[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, peelDepthTexture);
}

void DepthPeeling::compoPass(GLuint depthTexture, GLuint colorTexture,
                             GLuint compoTexture, bool firstPass) {
    compoProgram.bind();
    glUniform1i(glGetUniformLocation(compoProgram.getProgram(), "image0"), 0);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           depthTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           colorTexture, 0);
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

    if (!firstPass) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
    }
    glBindTexture(GL_TEXTURE_2D, compoTexture);
    buffer.render();
    compoProgram.unbind();
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void DepthPeeling::render() {
    glBlendEquation(GL_MAX);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    clearTextures(compoDepth1, compoTexture1);
    clearTextures(depthTexture2, colorTexture2);
    clearTextures(depthTexture1, colorTexture1);
    runRenderCallbackList();
    compoPass(compoDepth1, compoTexture1, colorTexture1, true);
    for (int i = 0; i < passCount; i++) {
        GLuint peelDepthTexture = (i % 2) ? depthTexture2 : depthTexture1;
        GLuint outDepthTexture = (i % 2) ? depthTexture1 : depthTexture2;
        GLuint outColorTexture = (i % 2) ? colorTexture1 : colorTexture2;
        peelingPass(outDepthTexture, outColorTexture, peelDepthTexture);
        runRenderCallbackList();
        compoPass(compoDepth1, compoTexture1, outColorTexture, false);
    }
    clearRenderCallbackList();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    compoProgram.bind();
    glBindTexture(GL_TEXTURE_2D, compoTexture1);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    buffer.render();
    compoProgram.unbind();
    glBlendEquation(GL_FUNC_ADD);
    glDisable(GL_BLEND);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
}
// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Platform/PlatformFile.h"
#include "Renderer/RendererGL.h"
#include "RGLInternal.h"

BE_NAMESPACE_BEGIN

const GLenum toGLPrim[] = {
    GL_TRIANGLES,
    GL_TRIANGLE_FAN,
    GL_TRIANGLE_STRIP,
    GL_LINES,
    GL_LINE_STRIP,
    GL_LINE_LOOP,
    GL_POINTS,
};

RendererGL      glr;

Str             GLShader::programCacheDir;

CVar            gl_sRGB(L"gl_sRGB", L"1", CVar::Bool | CVar::Archive, L"enable sRGB color calibration");

RendererGL::RendererGL() {
    initialized = false;
    currentContext = nullptr;
    mainContext = nullptr;
}

void RendererGL::Init(const Settings *settings) {
    BE_LOG(L"Initializing OpenGL Renderer...\n");

    InitHandles();

    InitMainContext(settings);

    currentContext = mainContext;

    InitGL();

    SetDefaultState();

    if (OpenGL::SupportsProgramBinary()) {
        GLShader::programCacheDir = "Cache/ProgramBinaryCache";

        if (!PlatformFile::DirectoryExists(GLShader::programCacheDir)) {
            PlatformFile::CreateDirectoryTree(GLShader::programCacheDir);
        }
    }

    initialized = true;
}

void RendererGL::Shutdown() {
    BE_LOG(L"Shutting down OpenGL Renderer...\n");

    initialized = false;

    gglFinish();

    FreeMainContext();

    currentContext = nullptr;

    FreeHandles();
}

void RendererGL::InitHandles() {
    contextList.SetGranularity(16);
    GLContext *zeroContext = new GLContext;
    memset(zeroContext, 0, sizeof(*zeroContext));
    contextList.Append(zeroContext);

    stencilStateList.SetGranularity(32);
    GLStencilState *zeroStencilState = new GLStencilState;
    memset(zeroStencilState, 0, sizeof(*zeroStencilState));
    stencilStateList.Append(zeroStencilState);

    bufferList.SetGranularity(1024);
    GLBuffer *zeroBuffer = new GLBuffer;
    memset(zeroBuffer, 0, sizeof(*zeroBuffer));
    bufferList.Append(zeroBuffer);

    syncList.SetGranularity(8);
    GLSync *zeroSync = new GLSync;
    memset(zeroSync, 0, sizeof(*zeroSync));
    syncList.Append(zeroSync);

    textureList.SetGranularity(1024);
    GLTexture *zeroTexture = new GLTexture;
    memset(zeroTexture, 0, sizeof(*zeroTexture));
    textureList.Append(zeroTexture);

    shaderList.SetGranularity(1024);
    GLShader *zeroShader = new GLShader;
    memset(zeroShader, 0, sizeof(*zeroShader));
    shaderList.Append(zeroShader);

    vertexFormatList.SetGranularity(64);
    GLVertexFormat *zeroVertexFormat = new GLVertexFormat;
    memset(zeroVertexFormat, 0, sizeof(*zeroVertexFormat));
    vertexFormatList.Append(zeroVertexFormat);

    renderTargetList.SetGranularity(64);
    GLRenderTarget *zeroRenderTarget = new GLRenderTarget;
    memset(zeroRenderTarget, 0, sizeof(*zeroRenderTarget));
    renderTargetList.Append(zeroRenderTarget);

    queryList.SetGranularity(32);
    GLQuery *zeroQuery = new GLQuery;
    memset(zeroQuery, 0, sizeof(*zeroQuery));
    queryList.Append(zeroQuery);
}

void RendererGL::FreeHandles() {
    contextList.DeleteContents(true);
    stencilStateList.DeleteContents(true);
    bufferList.DeleteContents(true);
    syncList.DeleteContents(true);
    textureList.DeleteContents(true);
    shaderList.DeleteContents(true);
    vertexFormatList.DeleteContents(true);
    renderTargetList.DeleteContents(true);
    queryList.DeleteContents(true);
}

void RendererGL::InitGL() {
    OpenGL::Init();

    vendorString = (const char *)gglGetString(GL_VENDOR);
    BE_LOG(L"GL vendor: %hs\n", vendorString);

    rendererString = (const char *)gglGetString(GL_RENDERER);
    BE_LOG(L"GL renderer: %hs\n", rendererString);

    versionString = (const char *)gglGetString(GL_VERSION);
    version = atof(versionString);
    BE_LOG(L"GL version: %hs\n", versionString);	

    BE_LOG(L"GL extensions:");
    GLint numExtensions = 0;
    gglGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
    for (int i = 0; i < numExtensions; i++) {
        const char *extension = (const char *)gglGetStringi(GL_EXTENSIONS, i);
        BE_LOG(L" %hs", extension);
    }
    BE_LOG(L"\n");

#ifdef __WIN32__
    const char *winExtensions = (const char *)gwglGetExtensionsStringARB(mainContext->hdc);
    BE_LOG(L"WGL extensions: %hs\n", winExtensions);
#endif	

    glslVersionString = (const char *)gglGetString(GL_SHADING_LANGUAGE_VERSION);
    if (gglGetError() == GL_INVALID_ENUM) {
        glslVersionString = "1.051";
    }
    glslVersion = atof(glslVersionString);

    BE_LOG(L"GLSL version: %hs\n", glslVersionString);	

    /*int red, green, blue, alpha;
    gglGetIntegerv(GL_RED_BITS, &red);
    gglGetIntegerv(GL_GREEN_BITS, &green);
    gglGetIntegerv(GL_BLUE_BITS, &blue);
    gglGetIntegerv(GL_ALPHA_BITS, &alpha);
    gglGetIntegerv(GL_DEPTH_BITS, &depthBits);
    gglGetIntegerv(GL_STENCIL_BITS, &stencilBits);
    colorBits = red + green + blue + alpha;
    BE_LOG(L"Pixel format: color(%i-bits) depth(%i-bits) stencil(%i-bits)\n", colorBits, depthBits, stencilBits);*/

    memset(&hwLimit, 0, sizeof(hwLimit));

    // texture limits
    gglGetIntegerv(GL_MAX_TEXTURE_SIZE, &hwLimit.maxTextureSize);
    BE_LOG(L"Maximum texture size: %i\n", hwLimit.maxTextureSize);

    gglGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &hwLimit.max3dTextureSize);
    BE_LOG(L"Maximum 3d texture size: %i\n", hwLimit.max3dTextureSize);

    gglGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &hwLimit.maxCubeMapTextureSize);
    BE_LOG(L"Maximum cube map texture size: %i\n", hwLimit.maxCubeMapTextureSize);

#ifdef GL_ARB_texture_rectangle
    gglGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE, &hwLimit.maxRectangleTextureSize);
    BE_LOG(L"Maximum rectangle texture size: %i\n", hwLimit.maxRectangleTextureSize);
#endif
    
    if (OpenGL::SupportsTextureBufferObject()) {
        gglGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE_EXT, &hwLimit.maxTextureBufferSize);
        BE_LOG(L"Maximum texture buffer size: %i\n", hwLimit.maxTextureBufferSize);
    }
    
    if (OpenGL::SupportsTextureFilterAnisotropic()) {
        gglGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &hwLimit.maxTextureAnisotropy);
        BE_LOG(L"Maximum texture anisotropy: %i\n", hwLimit.maxTextureAnisotropy);
    }
    
    gglGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &hwLimit.maxTextureImageUnits);
    BE_LOG(L"Maximum texture image units: %i\n", hwLimit.maxTextureImageUnits);

    // ARB vertex/fragment/geometry shader limits
    gglGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &hwLimit.maxVertexAttribs);
    BE_LOG(L"Maximum vertex attribs: %i\n", hwLimit.maxVertexAttribs);

    gglGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &hwLimit.maxVertexUniformComponents);	
    BE_LOG(L"Maximum vertex uniform components: %i\n", hwLimit.maxVertexUniformComponents);	

    gglGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &hwLimit.maxVertexTextureImageUnits);
    BE_LOG(L"Maximum vertex texture image units: %i\n", hwLimit.maxVertexTextureImageUnits);

    gglGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &hwLimit.maxFragmentUniformComponents);
    BE_LOG(L"Maximum fragment uniform components: %i\n", hwLimit.maxFragmentUniformComponents);

    gglGetIntegerv(GL_MAX_FRAGMENT_INPUT_COMPONENTS, &hwLimit.maxFragmentInputComponents);
    BE_LOG(L"Maximum fragment input components: %i\n", hwLimit.maxFragmentInputComponents);

    if (OpenGL::SupportsGeometryShader()) {
        gglGetIntegerv(GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT, &hwLimit.maxGeometryTextureImageUnits);
        BE_LOG(L"Maximum geometry texture image units: %i\n", hwLimit.maxGeometryTextureImageUnits);
        
        gglGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT, &hwLimit.maxGeometryOutputVertices);
        BE_LOG(L"Maximum geometry output vertices: %i\n", hwLimit.maxGeometryOutputVertices);
    }

    // GL_ARB_framebuffer_object (3.0)
    gglGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &hwLimit.maxRenderBufferSize);
    gglGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &hwLimit.maxColorAttachments);	
    BE_LOG(L"Maximum render buffer size: %i\n", hwLimit.maxRenderBufferSize);	

    // GL_ARB_draw_buffers (2.0)
    gglGetIntegerv(GL_MAX_DRAW_BUFFERS, &hwLimit.maxDrawBuffers);
    BE_LOG(L"Maximum MRT: %i\n", hwLimit.maxDrawBuffers);

    // Checking NVDIA vertex program version
#ifdef GL_NV_vertex_program4
    if (gglext._GL_NV_vertex_program4) {
        hwLimit.nvVertexProgramVersion = 4;
    } else if (gglext._GL_NV_vertex_program3) {
        hwLimit.nvVertexProgramVersion = 3;
    } else if (gglext._GL_NV_vertex_program2 && gglext._GL_NV_vertex_program2_option) {
        hwLimit.nvVertexProgramVersion = 2;
    } else if (gglext._GL_NV_vertex_program && gglext._GL_NV_vertex_program1_1) {
        hwLimit.nvVertexProgramVersion = 1;
    }
#endif

    // Checking NVDIA fragment program version
#ifdef GL_NV_fragment_program4
    if (gglext._GL_NV_fragment_program4) {
        hwLimit.nvFragmentProgramVersion = 4;
    } else if (gglext._GL_NV_fragment_program2 && gglext._GL_NV_fragment_program_option) {
        hwLimit.nvFragmentProgramVersion = 2;
    } else if (gglext._GL_NV_fragment_program) {
        hwLimit.nvFragmentProgramVersion = 1;
    }
#endif
    
    // GL_ARB_fragment_shader
    gglHint(GL_FRAGMENT_SHADER_DERIVATIVE_HINT, GL_NICEST);

#ifdef GL_ARB_texture_compression // 1.3
    gglHint(GL_TEXTURE_COMPRESSION_HINT, GL_NICEST);
#endif

#ifdef GL_SGIS_generate_mipmap // 1.4 - deprecated in OpenGL3
    //gglHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
#endif

#ifdef GL_NV_multisample_filter_hint
    if (gglext._GL_NV_multisample_filter_hint) {
        gglHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
    }
#endif

#if 0
    GLint encoding;
    gglGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, desktop ? GL_FRONT_LEFT : GL_BACK, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &encoding);
    BE_LOG(L"default frame buffer encoding : ");
    if (encoding == GL_SRGB) {
        BE_LOG(L"SRGB\n");
    } else if (encoding == GL_LINEAR) {
        BE_LOG(L"Linear\n");
    } else {
        assert(0);
    }
#endif

    if (gl_sRGB.GetBool()) {
        SetSRGBWrite(true);
    }
}

bool RendererGL::SupportsPackedFloat() const {
    return OpenGL::SupportsPackedFloat();
}

bool RendererGL::SupportsDepthBufferFloat() const {
    return OpenGL::SupportsDepthBufferFloat();
}

bool RendererGL::SupportsPixelBufferObject() const {
    return OpenGL::SupportsPixelBufferObject();
}

bool RendererGL::SupportsTextureRectangle() const {
    return OpenGL::SupportsTextureRectangle();
}

bool RendererGL::SupportsTextureArray() const {
    return OpenGL::SupportsTextureArray();
}

bool RendererGL::SupportsTextureBufferObject() const {
    return OpenGL::SupportsTextureBufferObject();
}

bool RendererGL::SupportsTextureCompressionS3TC() const {
    return OpenGL::SupportsTextureCompressionS3TC();
}

bool RendererGL::SupportsTextureCompressionLATC() const {
    return OpenGL::SupportsTextureCompressionLATC();
}

bool RendererGL::SupportsTextureCompressionETC2() const {
    return OpenGL::SupportsTextureCompressionETC2();
}

bool RendererGL::SupportsDebugLabel() const {
    return OpenGL::SupportsDebugLabel();
}

void RendererGL::Clear(int clearBits, const Color4 &color, float depth, unsigned int stencil) {
#if 1
    if (clearBits & ColorBit) {
        if (gl_sRGB.GetBool()) {
            gglClearBufferfv(GL_COLOR, 0, Color4(color.ToColor3().SRGBtoLinear(), color[3]));
        } else {
            gglClearBufferfv(GL_COLOR, 0, color);
        }
    }

    if ((clearBits & (DepthBit | StencilBit)) == (DepthBit | StencilBit)) {
        gglStencilMask(~0);
        gglClearBufferfi(GL_DEPTH_STENCIL, 0, depth, stencil);
        gglStencilMask(0);
    } else if (clearBits & DepthBit) {
        gglClearBufferfv(GL_DEPTH, 0, &depth);
    } else if (clearBits & StencilBit) {
        gglStencilMask(~0);
        gglClearBufferiv(GL_STENCIL, 0, (const GLint *)&stencil);
        gglStencilMask(0);
    }

#else
    GLbitfield bits = 0;

    if (clearBits & ColorBit) {
        bits |= GL_COLOR_BUFFER_BIT;
        if (gl_sRGB.GetBool()) {
            Vec3 linearColor = color.ToColor3().SRGBtoLinear();
            gglClearColor(linearColor[0], linearColor[1], linearColor[2], color[3]);
        } else {
            gglClearColor(color[0], color[1], color[2], color[3]);
        }
    }
    
    if (clearBits & DepthBit) {
        bits |= GL_DEPTH_BUFFER_BIT;
        OpenGL::ClearDepth(depth);
    }
    
    if (clearBits & StencilBit) {
        bits |= GL_STENCIL_BUFFER_BIT;
        gglClearStencil(stencil);
        gglStencilMask(~0);
    }
    
    if (bits) {
        gglClear(bits);
    }
    
    if (clearBits & StencilBit) {
        gglStencilMask(0);
    }
#endif
}

void RendererGL::ReadPixels(int x, int y, int width, int height, Image::Format imageFormat, byte *data) {
    GLenum	format;
    GLenum	type;
    
    if (!OpenGL::ImageFormatToGLFormat(imageFormat, false, &format, &type, nullptr)) {
        BE_WARNLOG(L"RendererGL::ReadPixels: Unsupported image format %hs\n", Image::FormatName(imageFormat));
        return;
    }
    
    GLint oldPackAlignment;
    gglGetIntegerv(GL_PACK_ALIGNMENT, &oldPackAlignment);
    gglPixelStorei(GL_PACK_ALIGNMENT, 1);
    
    gglReadPixels(x, y, width, height, format, type, data);
    
    gglPixelStorei(GL_PACK_ALIGNMENT, oldPackAlignment);
}

void RendererGL::DrawArrays(Primitive primitives, const int startVertex, const int numVerts) const {
    gglDrawArrays(toGLPrim[primitives], startVertex, numVerts);
}

void RendererGL::DrawArraysInstanced(Primitive primitives, const int startVertex, const int numVerts, const int primCount) const {
    gglDrawArraysInstanced(toGLPrim[primitives], startVertex, numVerts, primCount);
}

void RendererGL::DrawElements(Primitive primitives, const int startIndex, const int numIndices, const int indexSize, const void *ptr) const {
    GLenum indexType = indexSize == 1 ? GL_UNSIGNED_BYTE : (indexSize == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT);
    int indexBufferHandle = currentContext->state->bufferHandles[IndexBuffer];
    const GLvoid *indices = indexBufferHandle != 0 ? BUFFER_OFFSET(indexSize * startIndex) : (byte *)ptr + indexSize * startIndex;
    gglDrawElements(toGLPrim[primitives], numIndices, indexType, indices);
}

void RendererGL::DrawElementsInstanced(Primitive primitives, const int startIndex, const int numIndices, const int indexSize, const void *ptr, const int primCount) const {
    GLenum indexType = indexSize == 1 ? GL_UNSIGNED_BYTE : (indexSize == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT);
    int indexBufferHandle = currentContext->state->bufferHandles[IndexBuffer];
    const GLvoid *indices = indexBufferHandle != 0 ? BUFFER_OFFSET(indexSize * startIndex) : (byte *)ptr + indexSize * startIndex;
    gglDrawElementsInstanced(toGLPrim[primitives], numIndices, indexType, indices, primCount);
}

extern "C" void CheckGLError(const char *msg);

void RendererGL::CheckError(const char *fmt, ...) const {
    char buffer[16384];
    va_list args;

    va_start(args, fmt);
    Str::vsnPrintf(buffer, COUNT_OF(buffer), fmt, args);
    va_end(args);

    CheckGLError(buffer);
}

BE_NAMESPACE_END

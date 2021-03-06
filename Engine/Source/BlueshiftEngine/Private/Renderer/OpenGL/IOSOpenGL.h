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

#pragma once

/*
===============================================================================

    OpenGL for iOS

===============================================================================
*/

// OpenGL ES Programming Guide for iOS - Adopting OpenGL ES 3.0
// https://developer.apple.com/library/ios/documentation/3DDrawing/Conceptual/OpenGLES_ProgrammingGuide/AdoptingOpenGLES3/AdoptingOpenGLES3.html

#include "OpenGLES3.h"

BE_NAMESPACE_BEGIN

class IOSOpenGL : public OpenGLES3 {
public:
    static void             Init();
    
    static void             DrawBuffer(GLenum buffer);
    static void             BindDefaultFBO() { gglBindFramebuffer(GL_FRAMEBUFFER, 0); }
};

typedef IOSOpenGL           OpenGL;

BE_NAMESPACE_END

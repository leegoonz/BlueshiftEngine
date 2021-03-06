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
#include "Render/Render.h"
#include "RenderInternal.h"
#include "Render/Font.h"
#include "Core/Cmds.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

renderGlobal_t      renderGlobal;
RenderSystem        renderSystem;

void RenderSystem::Init(const Renderer::Settings *settings) {
    cmdSystem.AddCommand(L"screenshot", Cmd_ScreenShot);

    // Initialize OpenGL renderer
    glr.Init(settings);

    // Save current gamma ramp table
    glr.GetGammaRamp(savedGammaRamp);

    if ((r_fastSkinning.GetInteger() == 2 || r_fastSkinning.GetInteger() == 3) && glr.HWLimit().maxVertexTextureImageUnits > 0) {
        renderGlobal.skinningMethod = Mesh::VtfSkinning;
    } else if (r_fastSkinning.GetInteger() == 1) {
        renderGlobal.skinningMethod = Mesh::VertexShaderSkinning;
    } else {
        renderGlobal.skinningMethod = Mesh::CpuSkinning;
    }

    if (r_vertexTextureUpdate.GetInteger() == 2 && glr.SupportsTextureBufferObject()) {
        renderGlobal.vtUpdateMethod = Mesh::TboUpdate;
    } else if (r_vertexTextureUpdate.GetInteger() == 1 && glr.SupportsPixelBufferObject()) {
        renderGlobal.vtUpdateMethod = Mesh::PboUpdate;
    } else {
        renderGlobal.vtUpdateMethod = Mesh::DirectCopyUpdate;
    }

    textureManager.Init();

    shaderManager.Init();	

    materialManager.Init();

    //skinManager.Init();

    fontManager.Init();

    bufferCacheManager.Init();

    VertexFormat::Init();

    skeletonManager.Init();

    meshManager.Init();

    animManager.Init();

    RB_Init();
    
    frameData.Init();
    
    primaryWorld = nullptr;

    currentContext = nullptr;
    mainContext = nullptr;

    r_gamma.SetModified();
    r_swapInterval.SetModified();

    initialized = true;
}

void RenderSystem::Shutdown() {
    cmdSystem.RemoveCommand(L"screenshot");

    frameData.Shutdown();

    bufferCacheManager.Shutdown();

    RB_Shutdown();

    animManager.Shutdown();

    meshManager.Shutdown();

    skeletonManager.Shutdown();

    VertexFormat::Free();

    fontManager.Shutdown();

    //skinManager.Shutdown();

    materialManager.Shutdown();

    shaderManager.Shutdown();

    textureManager.Shutdown();

    glr.Shutdown();

    initialized = false;
}

bool RenderSystem::IsFullscreen() const {
    return glr.IsFullscreen();
}

void RenderSystem::SetGamma(double gamma) {
    unsigned short ramp[768];

    Clamp(gamma, 0.5, 3.0);	
    double one_gamma = 1.0 / gamma;

    double div = (double)(1.0 / 255.0);

    for (int i = 0; i < 256; i++) {
        unsigned int value = (int)(65535 * pow((double)(i + 0.5) * div, one_gamma));
        Clamp(value, (unsigned int)0, (unsigned int)65535);

        ramp[i] = ramp[i + 256] = ramp[i + 512] = (unsigned short)value;
    }

    glr.SetGammaRamp(ramp);
}

void RenderSystem::RestoreGamma() {
    glr.SetGammaRamp(savedGammaRamp);
}

RenderContext *RenderSystem::AllocRenderContext(bool isMainContext) {
    RenderContext *rc = new RenderContext;
    renderContexts.Append(rc);

    if (isMainContext) {
        mainContext = rc;
    }

    return rc;
}

void RenderSystem::FreeRenderContext(RenderContext *rc) {
    if (mainContext == rc) {
        mainContext = nullptr;
    }

    renderContexts.Remove(rc);
    delete rc;
}

RenderWorld *RenderSystem::AllocRenderWorld() {
    RenderWorld *renderWorld = new RenderWorld;

    this->primaryWorld = renderWorld;
    return renderWorld;
}

void RenderSystem::FreeRenderWorld(RenderWorld *renderWorld) {
    if (renderWorld) {
        delete renderWorld;
        renderWorld = nullptr;
    }
}

void *RenderSystem::GetCommandBuffer(int bytes) {
    RenderCommandBuffer *cmds = frameData.GetCommands();

    if (cmds->used + bytes + sizeof(RenderCommand) >= RenderCommandBufferSize) {
        BE_WARNLOG(L"RenderSystem::GetCommandBuffer: not enough command buffer space\n");
        return nullptr;
    }

    cmds->used += bytes;

    return cmds->data + cmds->used - bytes;
}

void RenderSystem::CmdDrawView(const view_t *view) {
    DrawViewRenderCommand *cmd = (DrawViewRenderCommand *)GetCommandBuffer(sizeof(DrawViewRenderCommand));
    if (!cmd) {
        return;
    }

    cmd->commandId		= DrawViewCommand;
    cmd->view			= *view;
}

void RenderSystem::CmdScreenshot(int x, int y, int width, int height, const char *filename) {
    ScreenShotRenderCommand *cmd = (ScreenShotRenderCommand *)GetCommandBuffer(sizeof(ScreenShotRenderCommand));
    if (!cmd) {
        return;
    }

    cmd->commandId		= ScreenShotCommand;
    cmd->x				= x;
    cmd->y				= y;
    cmd->width			= width;
    cmd->height			= height;
    Str::Copynz(cmd->filename, filename, COUNT_OF(cmd->filename));
}

void RenderSystem::IssueCommands() {
    RenderCommandBuffer *cmds = frameData.GetCommands();
    // add an end-of-list command
    *(int *)(cmds->data + cmds->used) = EndOfCommand;

    // clear it out, in case this is a sync and not a buffer flip
    cmds->used = 0;

    if (!r_skipBackEnd.GetBool()) {
        RB_Execute(cmds->data);
    }
}

void RenderSystem::RecreateScreenMapRT() {
    for (int i = 0; i < renderContexts.Count(); i++) {
        RenderContext *rc = renderContexts[i];
        rc->FreeScreenMapRT();
        rc->InitScreenMapRT();
    }
}

void RenderSystem::RecreateHDRMapRT() {
    for (int i = 0; i < renderContexts.Count(); i++) {
        RenderContext *rc = renderContexts[i];
        rc->FreeHdrMapRT();
        rc->InitHdrMapRT();
    }
}

void RenderSystem::RecreateShadowMapRT() {
    for (int i = 0; i < renderContexts.Count(); i++) {
        RenderContext *rc = renderContexts[i];
        rc->FreeShadowMapRT();
        rc->InitShadowMapRT();
    }
}

void RenderSystem::CheckModifiedCVars() {
    if (r_gamma.IsModified()) {
        r_gamma.ClearModified();

        SetGamma(r_gamma.GetFloat());
    }
    
    if (TextureManager::texture_filter.IsModified()) {
        TextureManager::texture_filter.ClearModified();

        textureManager.SetFilter(WStr::ToStr(TextureManager::texture_filter.GetString()));
    }

    if (TextureManager::texture_anisotropy.IsModified()) {
        TextureManager::texture_anisotropy.ClearModified();

        textureManager.SetAnisotropy(TextureManager::texture_anisotropy.GetFloat());		
    }

    if (TextureManager::texture_lodBias.IsModified()) {
        TextureManager::texture_lodBias.ClearModified();

        textureManager.SetLodBias(TextureManager::texture_lodBias.GetFloat());		
    }

    if (r_swapInterval.IsModified()) {
        r_swapInterval.ClearModified();

        glr.SwapInterval(r_swapInterval.GetInteger());
    }   

    if (r_useDeferredLighting.IsModified()) {
        r_useDeferredLighting.ClearModified();

        if (r_useDeferredLighting.GetBool()) {
            if (!shaderManager.FindGlobalHeader("#define USE_DEFERRED_LIGHTING\n")) {
                shaderManager.AddGlobalHeader("#define USE_DEFERRED_LIGHTING\n");
                shaderManager.ReloadShaders();

                RecreateScreenMapRT();
            }
        } else {
            if (shaderManager.FindGlobalHeader("#define USE_DEFERRED_LIGHTING\n")) {
                shaderManager.RemoveGlobalHeader("#define USE_DEFERRED_LIGHTING\n");
                shaderManager.ReloadShaders();

                RecreateScreenMapRT();
            }
        }		
    }

    if (r_usePostProcessing.IsModified()) {
        r_usePostProcessing.ClearModified();

        r_HDR.SetModified();
        r_DOF.SetModified();
        r_SSAO.SetModified();
        r_sunShafts.SetModified();
        r_motionBlur.SetModified();
    }

    if (r_HDR.IsModified()) {
        r_HDR.ClearModified();

        /*if (r_usePostProcessing.GetBool() && r_HDR.GetInteger() == 1) {
            if (!shaderManager.FindGlobalHeader("#define LOGLUV_HDR\n")) {
                shaderManager.AddGlobalHeader("#define LOGLUV_HDR\n");
                shaderManager.ReloadShaders();
            }
        } else {
            if (shaderManager.FindGlobalHeader("#define LOGLUV_HDR\n")) {
                shaderManager.RemoveGlobalHeader("#define LOGLUV_HDR\n");
                shaderManager.ReloadShaders();
            }
        }*/

        RecreateScreenMapRT();
        RecreateHDRMapRT();
    }

    if (r_motionBlur.IsModified()) {
        r_motionBlur.ClearModified();
        
        if (r_usePostProcessing.GetBool() && (r_motionBlur.GetInteger() & 2)) {
            if (!shaderManager.FindGlobalHeader("#define OBJECT_MOTION_BLUR\n")) {
                shaderManager.AddGlobalHeader("#define OBJECT_MOTION_BLUR\n");
                shaderManager.ReloadShaders();

                meshManager.ReinstantiateSkinnedMeshes();

                RecreateScreenMapRT();
            }
        } else {
            if (shaderManager.FindGlobalHeader("#define OBJECT_MOTION_BLUR\n")) {
                shaderManager.RemoveGlobalHeader("#define OBJECT_MOTION_BLUR\n");
                shaderManager.ReloadShaders();

                meshManager.ReinstantiateSkinnedMeshes();

                RecreateScreenMapRT();
            }
        }
    }

    if (r_SSAO_quality.IsModified()) {
        r_SSAO_quality.ClearModified();
        
        if (r_usePostProcessing.GetBool() && r_SSAO_quality.GetInteger() > 0) {
            if (!shaderManager.FindGlobalHeader("#define HIGH_QUALITY_SSAO\n")) {
                shaderManager.AddGlobalHeader("#define HIGH_QUALITY_SSAO\n");
                shaderManager.ReloadShaders();
            }
        } else {
            if (shaderManager.FindGlobalHeader("#define HIGH_QUALITY_SSAO\n")) {
                shaderManager.RemoveGlobalHeader("#define HIGH_QUALITY_SSAO\n");
                shaderManager.ReloadShaders();
            }
        }
    }

    if (r_shadowMapSize.IsModified()) {
        r_shadowMapSize.ClearModified();
        RecreateShadowMapRT();
    }

    if (r_shadowMapFloat.IsModified()) {
        r_shadowMapFloat.ClearModified();
        RecreateShadowMapRT();
    }

    if (r_shadowCubeMapSize.IsModified()) {
        r_shadowCubeMapSize.ClearModified();
        RecreateShadowMapRT();
    }

    if (r_shadows.IsModified()) {
        r_shadows.ClearModified();

        if (r_shadows.GetInteger() == 1) {
            if (!shaderManager.FindGlobalHeader("#define USE_SHADOW_MAP\n")) {
                shaderManager.AddGlobalHeader("#define USE_SHADOW_MAP\n");
                shaderManager.ReloadLightingShaders();

                RecreateShadowMapRT();
            }
        } else {
            if (shaderManager.FindGlobalHeader("#define USE_SHADOW_MAP\n")) {
                shaderManager.RemoveGlobalHeader("#define USE_SHADOW_MAP\n");
                shaderManager.ReloadLightingShaders();
            }
        }		
    }

    if (r_showShadows.IsModified()) {
        r_showShadows.ClearModified();

        if (r_showShadows.GetInteger() == 1) {
            if (!shaderManager.FindGlobalHeader("#define DEBUG_CASCADE_SHADOW_MAP\n")) {
                shaderManager.AddGlobalHeader("#define DEBUG_CASCADE_SHADOW_MAP\n");

                shaderManager.ReloadLightingShaders();
            }
        } else {
            if (shaderManager.FindGlobalHeader("#define DEBUG_CASCADE_SHADOW_MAP\n")) {
                shaderManager.RemoveGlobalHeader("#define DEBUG_CASCADE_SHADOW_MAP\n");

                shaderManager.ReloadLightingShaders();
            }
        }
    }

    if (r_CSM_count.IsModified()) {
        r_CSM_count.ClearModified();

        shaderManager.RemoveGlobalHeader(va("#define CSM_COUNT "));
        
        shaderManager.AddGlobalHeader(va("#define CSM_COUNT %i\n", r_CSM_count.GetInteger()));
        shaderManager.ReloadLightingShaders();

        RecreateShadowMapRT();
    }

    if (r_CSM_selectionMethod.IsModified()) {
        r_CSM_selectionMethod.ClearModified();

        shaderManager.RemoveGlobalHeader(va("#define CASCADE_SELECTION_METHOD "));
        
        shaderManager.AddGlobalHeader(va("#define CASCADE_SELECTION_METHOD %i\n", r_CSM_selectionMethod.GetInteger()));
        shaderManager.ReloadLightingShaders();

        RecreateShadowMapRT();
    }

    if (r_CSM_blend.IsModified()) {
        r_CSM_blend.ClearModified();

        if (r_CSM_blend.GetBool()) {
            if (!shaderManager.FindGlobalHeader("#define BLEND_CASCADE\n")) {
                shaderManager.AddGlobalHeader("#define BLEND_CASCADE\n");
                shaderManager.ReloadLightingShaders();
            }
        } else {
            if (shaderManager.FindGlobalHeader("#define BLEND_CASCADE\n")) {
                shaderManager.RemoveGlobalHeader("#define BLEND_CASCADE\n");
                shaderManager.ReloadLightingShaders();
            }
        }
    }

    if (r_shadowMapQuality.IsModified()) {
        r_shadowMapQuality.ClearModified();
        
        shaderManager.RemoveGlobalHeader(va("#define SHADOW_MAP_QUALITY "));
        
        if (r_shadowMapQuality.GetInteger() >= 0) {
            shaderManager.AddGlobalHeader(va("#define SHADOW_MAP_QUALITY %i\n", r_shadowMapQuality.GetInteger()));
            shaderManager.ReloadLightingShaders();
        }
    }
}

//--------------------------------------------------------------------------------------------------

void RenderSystem::Cmd_ScreenShot(const CmdArgs &args) {	
    char	path[1024];
    char	picname[16];
    int		i;

    const char *homePath = PlatformFile::HomePath();
    
    if (args.Argc() > 1) {
        Str::snPrintf(path, sizeof(path), "%s/Screenshots/%ls", homePath, args.Argv(1));
    } else {
        strcpy(picname, "shot000.png");
        for (i = 0; i <= 999; i++) {
            picname[4] = '0' + i/100;
            picname[5] = '0' + i%100/10;
            picname[6] = '0' + i%10;
            Str::snPrintf(path, sizeof(path), "%s/Screenshots/%s", homePath, picname);
            
            if (!fileSystem.FileExists(path)) {
                break;
            }
        }

        if (i == 1000) {
            BE_WARNLOG(L"too many screenshot exist\n");
            return;
        }
    }

    renderSystem.CmdScreenshot(0, 0, renderSystem.currentContext->GetDeviceWidth(), renderSystem.currentContext->GetDeviceHeight(), path);
}

BE_NAMESPACE_END

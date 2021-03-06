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

BE_NAMESPACE_BEGIN

//--------------------------------------------------------------------------------------------------
// various pack/unpack function type for each color format
//--------------------------------------------------------------------------------------------------
using UserFormatToRGBA8888Func = void (*)(const byte *src, byte *dst, int numPixels);
using RGBA8888ToUserFormatFunc = void (*)(const byte *src, byte *dst, int numPixels);

struct ImageFormatInfo {
    const char *name;
    int size; // bytes per pixel or bytes per block
    int numComponents;
    int redBits;
    int greenBits;
    int blueBits;
    int alphaBits;
    int type;
    UserFormatToRGBA8888Func unpackFunc;
    RGBA8888ToUserFormatFunc packFunc;
};

void DecompressPVRTC(const Image &srcImage, Image &dstImage, int do2BitMode);

void DecompressETC(const Image &srcImage, Image &dstImage, int nMode);

bool CompressedFormatBlockDimensions(Image::Format imageFormat, int &blockWidth, int &blockHeight);
bool CompressedFormatMinDimensions(Image::Format imageFormat, int &minWidth, int &minHeight);

const ImageFormatInfo *GetImageFormatInfo(Image::Format imageFormat);

void RGBToYCoCg(short *YCoCg, const byte *rgb, int stride);
void RGBAToYCoCgA(short *YCoCgA, const byte *rgba, int stride);
void YCoCgToRGB(byte *rgb, int stride, const short *YCoCg);
void YCoCgAToRGBA(byte *rgba, int stride, const short *YCoCgA);

BE_NAMESPACE_END

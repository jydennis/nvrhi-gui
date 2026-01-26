#pragma once
#ifndef _IMGLOADER_H_
#define _IMGLOADER_H_

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <nvrhi/nvrhi.h>
#include <nvrhi/d3d11.h>
#include <nvrhi/utils.h>
#include <GLFW/glfw3.h>
//#include <d3d11.h>
#include <d3dcompiler.h>
#include <C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Include/D3DX11.h>
#include <iostream>
#define GLFW_EXPOSE_NATIVE_WIN32 1
#include <GLFW/glfw3native.h>

#include "imgui.h"
//#include "../bindings/imgui_impl_win32.h"
#include "../bindings/imgui_impl_glfw.h"
#include "../bindings/imgui_impl_dx11.h"


class ImgLoader_Streamer {
public:
    ImgLoader_Streamer(){}

    void bindCmdList(nvrhi::CommandListHandle in_commandList) {
        this->commandList = in_commandList;
    }

    void bindDevice(nvrhi::DeviceHandle in_nvrhiDevice) {
        this->nvrhiDevice = in_nvrhiDevice;
    }

    void loadImage(const char* filename){
        tmp_imgfilename = filename;
        this->pixelsdata = stbi_load(filename, &this->loadwidth, &this->loadheight, &this->loadchannels,  4);

        if (!pixelsdata) {
            printf("Fail to load image.\n");
            this->loadheight = 32;
            this->loadwidth = 32;
            pixelsdata = new unsigned char[UINT64(this->loadheight) * UINT64(this->loadwidth) * 4]();
        } else {
            printf("load image w:%d, h:%d\n",this->loadwidth,this->loadheight);
        }

        if (this->loadchannels != 4)
        {
            printf("image channel mismatce.\n");
        }

         // texture to show
        auto showImgtextureDesc = nvrhi::TextureDesc()
        .setDimension(nvrhi::TextureDimension::Texture2D)
        .setFormat(nvrhi::Format::RGBA8_UNORM)
        .setWidth(loadwidth)
        .setHeight(loadheight)
        .setSampleCount(1)
        .setIsRenderTarget(false)
        .setDebugName("show texture Image")
        .setIsUAV(true);
        showImgtextureDesc.initialState = nvrhi::ResourceStates::UnorderedAccess;

        myTexture = nvrhiDevice->createTexture(showImgtextureDesc);

        this->imageRowPitch = UINT64(this->loadwidth) * 4;

        commandList->open();
        commandList->writeTexture(myTexture, 0, 0, pixelsdata, imageRowPitch,imageRowPitch*loadheight);
        commandList->close();
        printf("write texture cmd recorded");
    }

    nvrhi::TextureHandle getTexture(){
        return myTexture;
    }

    float getMSAAFactor() {
        return std::pow(2, 3 - current);
    }

    bool showMenuWindow(std::string &used_imgfilename,int idx) {
        bool show_window = true;
        bool imagechanged = false;
        std::string texname = "Input Image Window";
        texname += std::to_string(idx);
        ImGui::Begin(texname.c_str() ,&show_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        if (!show_window)
        {
            printf("error showing window");
        }
        ImGui::InputText("File Name", tmp_imgfilename.data(), 100);
        if (ImGui::Button("Update Image"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
        {    
            used_imgfilename = tmp_imgfilename;
            imagechanged = true;
        }
        
        const char* items[] = {
            "1xMSAA",
            "2xMSAA",
            "4xMSAA",
            "8xMSAA",
        };

        ImGui::Combo("Mode", &current, items, IM_ARRAYSIZE(items));
        ImGui::End();
        return imagechanged;
    }

private:
    const unsigned char* pixelsdata;
    int loadwidth, loadheight, loadchannels;
    UINT64 imageRowPitch;

    nvrhi::TextureHandle myTexture;

    nvrhi::CommandListHandle commandList;
    nvrhi::DeviceHandle nvrhiDevice;

    std::string tmp_imgfilename;

    int idx;
    int current = 0;
};
    
    
#endif
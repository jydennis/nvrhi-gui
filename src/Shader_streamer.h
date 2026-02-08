#pragma once
#ifndef _CSLOADER_H_
#define _CSLOADER_H_

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

auto callback_resize_txt(ImGuiInputTextCallbackData* data) -> int
{
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
    {
        std::vector<char>* buf = (std::vector<char>*)data->UserData;
        buf->resize(data->BufTextLen + 1);  // +1 留给 '\0'
        data->Buf = buf->data();
    }
    return 0;
}

class ComputeShader_Streamer {
public:
    ComputeShader_Streamer(int out_w=640, int out_h=480, int bindpoint_tex_size = 2, int bindpoint_buffer_size = 0, int thread_b_w=32, int thread_b_h=32, int thread_cnt=1) : 
        out_w(out_w),
        out_h(out_h),
        thread_w(thread_b_w), 
        thread_h(thread_b_h), 
        thread_z(thread_cnt) {

        used_ComputeShader = std::string("\
        RWTexture2D<unorm float4> BufferOut : register(u0);\n\
        Texture2D<unorm float4> Buffer0 : register(t1);\n\
        \n\
        [numthreads(32, 32, 1)]\n\
        void CSMain( uint3 DTid : SV_DispatchThreadID )\n\
        {\n\
        BufferOut[DTid.xy] = Buffer0[DTid.xy];\n\
        }\0                                                                                        ");
        tmp_ComputeShader = used_ComputeShader;

        textureList.resize(bindpoint_tex_size);
        bufferList.resize(bindpoint_buffer_size);
        textureValidList.resize(bindpoint_tex_size, false);
        bufferValidList.resize(bindpoint_buffer_size, false);
        
        CSchanged = true;
    }

    void bindCmdList(nvrhi::CommandListHandle in_commandList) {
        this->commandList = in_commandList;
    }

    void bindDevice(nvrhi::DeviceHandle in_nvrhiDevice) {
        this->nvrhiDevice = in_nvrhiDevice;
        nvrhi::TextureDesc texDesc;
        texDesc.width = this->out_w;
        texDesc.height = this->out_h;
        texDesc.format = nvrhi::Format::RGBA8_UNORM;
        texDesc.isUAV = true;
        this->outTexUAV = this->nvrhiDevice->createTexture(texDesc);
        textureList[0] = this->outTexUAV;
    }

    void bindTexture(nvrhi::TextureHandle in_Texture, int bindPoint){
        if (bindPoint == 0) {
            printf("Tex bind point 0 in compute shader is left for output buffer.");
            return;
        }
        textureList[bindPoint] = in_Texture;
        textureValidList[bindPoint] = true;
    }

    void bindBuffer(nvrhi::BufferHandle  in_Buffer, int bindPoint) {
        bufferList[bindPoint] = in_Buffer;
        bufferValidList[bindPoint] = true;
    }

    void resetBindPoint() {
        for (int i = 1; i <= textureList.size(); i++ ) {
            textureValidList[i] = false;
        }

        for (int i =0; i < bufferList.size(); i++) {
           bufferValidList[i] = false;
        }
    }

    void runComputeShader() {
        if (CSchanged) 
        {
            CSchanged = false;
            HRESULT h = this->createShaderFromStrint(ptrComputeShader, used_ComputeShader);
        }
       

        nvrhi::BindingLayoutDesc layoutDesc;
        layoutDesc.visibility = nvrhi::ShaderType::Compute;
        layoutDesc.bindings.push_back(nvrhi::BindingLayoutItem::Texture_UAV(0));
        //printf("qhere1\n");

        // 例如：slot 0：UAV
        for (int i = 1; i <= textureList.size(); i++ ) {
            if(textureValidList[i]) {
            //printf("tex %d \n",i);
            layoutDesc.bindings.push_back(nvrhi::BindingLayoutItem::Texture_SRV(i));
            }
        }

        for (int i =0; i < bufferList.size(); i++) {
            if(bufferValidList[i]) {
            //printf("buffer %d \n",i);
            layoutDesc.bindings.push_back(nvrhi::BindingLayoutItem::RawBuffer_UAV(i));
            }
        }

        nvrhi::BindingLayoutHandle bindingLayout = nvrhiDevice->createBindingLayout(layoutDesc);
        nvrhi::BindingSetDesc bindingDesc;
        bindingDesc.bindings.push_back(nvrhi::BindingSetItem::Texture_UAV(0, this->outTexUAV));
        for (int i = 1; i <= textureList.size(); i++ ) {
            if (textureValidList[i]){
            bindingDesc.bindings.push_back(nvrhi::BindingSetItem::Texture_SRV(i, textureList[i]));
            }
        }

        for (int i = 0; i < bufferList.size(); i++ ) {
            if(bufferValidList[i]){
            bindingDesc.bindings.push_back(nvrhi::BindingSetItem::RawBuffer_UAV(i, bufferList[i]));
            }
        }
        nvrhi::BindingSetHandle bindingSet = nvrhiDevice->createBindingSet(bindingDesc, bindingLayout);

        computePipeline = nvrhiDevice->createComputePipeline(nvrhi::ComputePipelineDesc()
        .setComputeShader(this->ptrComputeShader)
        .addBindingLayout(bindingLayout));

        auto ComputeState = nvrhi::ComputeState()
        .setPipeline(computePipeline)
        .addBindingSet(bindingSet);

        commandList->open();

        commandList->setComputeState(ComputeState);
        commandList->setTextureState(this->outTexUAV, nvrhi::AllSubresources, nvrhi::ResourceStates::UnorderedAccess); //barrier
        commandList->setTextureState(this->textureList[1], nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource); //barrier
        // 例如 16x16 的线程组
        commandList->dispatch(thread_w, thread_h, thread_z);

        commandList->setTextureState(this->outTexUAV, nvrhi::AllSubresources , nvrhi::ResourceStates::ShaderResource); //barrier

        commandList->close();
        //nvrhiDevice->executeCommandList(commandList);
         //printf("compute shader added");
         resetBindPoint();
    }

    nvrhi::TextureHandle getOutTexture(){
        return this->outTexUAV;
    }
    



    void showMenuWindow(int idx, bool show_window) {
        if (!show_window)
        {
            printf("error showing CS window");
        }

        std::string inputtext = "CS Shader "+std::to_string(idx);
        std::string buttext = "Update Shader "+std::to_string(idx);
        ImGui::InputTextMultiline(inputtext.c_str(), tmp_ComputeShader.data(), 2000, ImVec2(600, 300), ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CallbackResize,
        callback_resize_txt,
        &tmp_ComputeShader);
        if (ImGui::Button(buttext.c_str()))                            // Buttons return true when clicked (most widgets return true when edited/activated)
        {    
            used_ComputeShader = tmp_ComputeShader;
            CSchanged = true;
        }
        
    }
private:
    HRESULT createShaderFromStrint(nvrhi::ShaderHandle& ptrComputeShader, std::string used_ComputeShader) {
        HRESULT hr = S_OK;
       
        const D3D_SHADER_MACRO defines[] = 
        {
            "EXAMPLE_DEFINE", "1",
            NULL, NULL
        };

        UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
        ID3DBlob* cshaderBlob = nullptr;
        ID3DBlob* errorBlob = nullptr;
        hr = D3DCompile(this->used_ComputeShader.c_str(), this->used_ComputeShader.length(), nullptr, defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                        "CSMain", "cs_5_0",
                                        flags, 0, &cshaderBlob, &errorBlob );
        if ( FAILED(hr) )
        {
            if ( errorBlob )
            {
                std::cerr << " shader compile error 1";
                OutputDebugStringA( (char*)errorBlob->GetBufferPointer() );
                errorBlob->Release();
            }

            if ( cshaderBlob )
            cshaderBlob->Release();

            return hr;
        }    

        nvrhi::ShaderDesc csshaderDesc;
            csshaderDesc.shaderType = nvrhi::ShaderType::Compute;
            csshaderDesc.debugName = "compute";
            csshaderDesc.entryName = "Main";
        ptrComputeShader = nvrhiDevice->createShader(
        csshaderDesc,
        cshaderBlob->GetBufferPointer(), cshaderBlob->GetBufferSize());

        return hr;
    }

private:
    nvrhi::TextureHandle outTexUAV;

    nvrhi::CommandListHandle commandList;
    nvrhi::DeviceHandle nvrhiDevice;

    nvrhi::ShaderHandle ptrComputeShader;
    bool CSchanged  = true;
    std::string tmp_ComputeShader;
    std::string used_ComputeShader;

    std::vector<nvrhi::BufferHandle> bufferList;
    std::vector<nvrhi::TextureHandle> textureList;
    std::vector<bool> bufferValidList;
    std::vector<bool> textureValidList;

    nvrhi::ComputePipelineHandle computePipeline;

    int thread_w, thread_h, thread_z;
    int out_w, out_h;

};
    
    
#endif
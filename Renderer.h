//
//  Renderer.h
//  Snakebite
//
//  Created by Stephen Harrison-Daly on 26/11/2015.
//  Copyright © 2015 Stephen Harrison-Daly. All rights reserved.
//

#pragma once

#include "Common.h"
#include "VertexBank.h"

#ifdef __APPLE__
#import <MetalKit/MetalKit.h>
#include "Synchronization.h"

#elif _WIN32
#include <windows.h>
#include <directxmath.h>

struct ID3D11Device;
struct ID3D11Device1;
struct ID3D11DeviceContext;
struct ID3D11DeviceContext1;
struct IDXGISwapChain;
struct IDXGISwapChain1;
struct ID3D11RenderTargetView;
struct ID3D11Texture2D;
struct ID3D11DepthStencilView;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11InputLayout;
struct ID3D11Buffer;
struct ID3D11ShaderResourceView;
struct ID3D11SamplerState;
struct ID3D11BlendState;

#endif

namespace shd
{
    // Forward declaration
    struct DrawableObject;
    
    struct GpuSharedBuffer
    {
#ifdef __APPLE__
		id <MTLBuffer> buffer;
#elif _WIN32
		ID3D11Buffer * buffer;
		GpuSharedBuffer() : buffer(nullptr) {}
#endif
    };
    
    struct Texture
    {
        bool isFlipped;

#ifdef __APPLE__
		id <MTLTexture> textureHandle;
		Texture() : isFlipped(false) {}
#elif _WIN32
		ID3D11ShaderResourceView * textureHandle;
		Texture() : isFlipped(false), textureHandle(nullptr) {}
#endif
    };
    
    class Renderer
    {
    public:

		struct InitParams
		{
#ifdef _WIN32
			HINSTANCE instanceHandle;
			int cmdShow;
#endif
			uint32_t screenWidth;
			uint32_t screenHeight;
			bool fullscreen;

			InitParams() : screenWidth(1920), screenHeight(1080), fullscreen(false)
			{}
		};

		enum ScreenResolution
		{
			SCREEN_RES_DEFAULT,
			SCREEN_RES_3840_X_2160,
			SCREEN_RES_2560_X_1440,
			SCREEN_RES_1920_X_1080,
			SCREEN_RES_1280_X_720,
			SCREEN_RES_800_X_600,
			SCREEN_RES_NUM_TYPES
		};
        
        bool init(InitParams & rendererInitParams);
        void term();
        void resize(float width, float height, bool appliedFromSettings = false);
        void draw(double delta);
		inline VertexBank * getVertexBank() { return &m_vertexBank; }
		bool createGpuBoundBuffers(DrawableObject * obj, bool isDynamic);
		bool createGpuBoundBuffer(GpuSharedBuffer * buffer, size_t size);
		bool updateGpuBoundBuffer(GpuSharedBuffer * buffer, void * newData, size_t dataSize);
		inline float getScreenWidth() { return m_screenWidth; }
		inline float getScreenHeight() { return m_screenHeight; }
		inline float getScreenWidthFromOS() { return m_screenWidthFromOS; }
		inline float getScreenHeightFromOS() { return m_screenHeightFromOS; }
		inline float getScreenAspectRatio() { return m_aspectRatio; }
		void setFullscreen(bool fullscreen);
		void updateProjectionMatrix();

#ifdef __APPLE__
		void setDrawableAndRenderPassDesc(id <CAMetalDrawable> currentDrawable, MTLRenderPassDescriptor * currentRenderPassDescriptor);
		id <MTLDevice> getDevice() { return m_device; }
#elif _WIN32
		ID3D11Device * getDevice() { return m_d3dDevice; }
		ID3D11DeviceContext * getDeviceContext() { return m_immediateContext; }
#endif
        
    private:
        
        bool setupRenderPipelines();

#ifdef __APPLE__

#elif _WIN32
		bool initWindow(InitParams & rendererInitParams);
		bool initDevice();
		void setPipelineState(RenderPipelineState state, DrawableObject * obj);
#endif

#ifdef __APPLE__
		// Semaphore used to make sure we wait until the GPU is ready before modifying shared GPU buffers
		Semaphore m_semaphore;
#endif

		// Buffers that are shared with the GPU
		GpuSharedBuffer m_gpuSharedBuffers[SHD_MAX_GPU_SHARED_BUFFERS];

		// Index of the m_gpuSharedBuffers
		uint32_t m_gpuBuffIndex;

		// Index that we use to know if what frame we are writing date into. Up to kMaxInflightBuffers
		uint8_t m_frameIndex;

		// Frame counter
		uint64_t m_frameCounter;

		// A big thing that contains all vertices in the app
		VertexBank m_vertexBank;

		// GPU bound buffer for our uniforms
		GpuSharedBuffer m_uniformsGpuBuffer;
		GpuSharedBuffer m_psUniformsGpuBuffer;

		// Screen width, height and aspect ratio
		float m_screenWidth;
		float m_screenHeight;
		float m_aspectRatio;

		// True screen width and height given from the OS
		float m_screenWidthFromOS;
		float m_screenHeightFromOS;

#ifdef __APPLE__

		// The graphics device, a.k.a. the GPU
		id <MTLDevice> m_device;

		// The device reads commands from the queue. The queue holds the command buffers
		id <MTLCommandQueue> m_commandQueue;

		// The library contains the compiled metal shaders
		id <MTLLibrary> m_shaderLibrary;

		// The pipeline state is like a blueprint for encoding things into a command buffer
		id <MTLRenderPipelineState> m_pipelineStates[PIPELINE_STATES_MAX];

		// Some information on how a graphics rendering pass should perform depth and stencil operations
		id <MTLDepthStencilState> m_depthState;

		// CAMetalDrawable represents a displayable buffer that vends an object that may be used to create a render target for Metal.
		id <CAMetalDrawable> m_currentDrawable;

		// MTLRenderPassDescriptor represents a collection of attachments to be used to create a concrete render command encoder
		MTLRenderPassDescriptor * m_currentRenderPassDescriptor;

		// The projection matrix
		matrix_float4x4 m_projectionMatrix;

		// The projection matrix for UI elements
		matrix_float4x4 m_projectionMatrixUI;

		// And view matrix
		matrix_float4x4 m_viewMatrix; 

#elif _WIN32

		// Enums for the different types of vertex and pixel shaders
		enum VertexShaderType
		{
			VS_TYPE_NOT_SET = -1,
			VS_TYPE_BASIC = 0,
			VS_TYPE_BASIC_UI,
			VS_TYPE_LIT,
			VS_TYPE_MAX
		};

		enum PixelShaderType
		{
			PS_TYPE_NOT_SET = -1,
			PS_TYPE_FLAT_COLOUR = 0,
			PS_TYPE_TEXTURED,
			PS_TYPE_GUY,
			PS_TYPE_JERSEY,
			PS_TYPE_SCOREBOARD,
			PS_TYPE_FONT,
			PS_TYPE_FLAT_LIT,
			PS_TYPE_TEXTURED_LIT,
			PS_TYPE_TEXTURED_LIT_BUMP,
			PS_TYPE_MAX
		};

		enum VertexLayoutType
		{
			VERTEX_LAYOUT_NOT_SET = -1,
			VERTEX_LAYOUT_TYPE_BASIC = 0,
			VERTEX_LAYOUT_TYPE_MAX
		};

		enum TextureSamplerType
		{
			TEXTURE_SAMPLER_NOT_SET = -1,
			TEXTURE_SAMPLER_TYPE_POINT = 0,
			TEXTURE_SAMPLER_TYPE_LINEAR,
			TEXTURE_SAMPLER_TYPE_MAX
		};

		// Represents that last state, so we don't make unnessary state changes in Directx during draw calls
		struct RenderingState
		{
			VertexShaderType vertexShader;
			PixelShaderType  pixelShader;
			VertexLayoutType vertexLayout;
			TextureSamplerType textureSampler;
			ID3D11ShaderResourceView * textureHandle;
			ID3D11ShaderResourceView * normalMaptextureHandle;

			RenderingState() :	vertexShader(VS_TYPE_NOT_SET),
								pixelShader(PS_TYPE_NOT_SET),
								vertexLayout(VERTEX_LAYOUT_NOT_SET),
								textureSampler(TEXTURE_SAMPLER_NOT_SET),
								textureHandle(nullptr),
								normalMaptextureHandle(nullptr) {}
		};

		// Handle to the instance of this application
		HINSTANCE m_instanceHandle;

		// Handle to the window
		HWND m_windowHandle;

		// The device interface represents a virtual adapter; it is used to create resources
		ID3D11Device * m_d3dDevice;
		ID3D11Device1 * m_d3dDevice1;

		// The ID3D11DeviceContext interface represents a device context which generates rendering commands
		ID3D11DeviceContext * m_immediateContext;
		ID3D11DeviceContext1 * m_immediateContext1;

		// An IDXGISwapChain interface implements one or more surfaces for storing rendered data before presenting it to an output
		IDXGISwapChain * m_swapChain;
		IDXGISwapChain1 * m_swapChain1;

		// A render - target - view interface identifies the render - target subresources that can be accessed during rendering
		ID3D11RenderTargetView * m_renderTargetView;

		// The back buffer
		ID3D11Texture2D * m_backBuffer;

		// These are used for depth stencil testing
		ID3D11Texture2D * m_depthStencil;
		ID3D11DepthStencilView * m_depthStencilView;

		// The projection matrix
		DirectX::XMMATRIX m_projectionMatrix;

		// The projection matrix for UI elements
		DirectX::XMMATRIX m_projectionMatrixUI;

		// And view matrix (camera)
		DirectX::XMMATRIX m_viewMatrix;

		// The blending state - for alpha blending
		ID3D11BlendState * m_alphaBlendingState;

		// The texture sampler
		ID3D11SamplerState * m_textureSamplers[TEXTURE_SAMPLER_TYPE_MAX];

		// Vertex shaders
		ID3D11VertexShader * m_vertexShaders[VS_TYPE_MAX];

		// Vertex layouts
		ID3D11InputLayout * m_vertexLayouts[VERTEX_LAYOUT_TYPE_MAX];

		// Pixel shaders
		ID3D11PixelShader * m_pixelShaders[PS_TYPE_MAX];

		// Last state of shaders, etc. so we don't make unnecessary state changes during draw calls
		RenderingState m_lastState;
#endif
    };
}

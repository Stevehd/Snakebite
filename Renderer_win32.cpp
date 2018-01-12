
#include "Renderer.h"
#include "resource.h"
#include "Application.h"
#include "DrawableObject.h"
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>
#include <stdio.h>

// Precompiled shader byte code
#include "CompiledShaders\shader_vsBasic_byte_code.h"
#include "CompiledShaders\shader_vsBasicUI_byte_code.h"
#include "CompiledShaders\shader_vsLit_byte_code.h"
#include "CompiledShaders\shader_psFlatColour_byte_code.h"
#include "CompiledShaders\shader_psTexture_byte_code.h"
#include "CompiledShaders\shader_psGuy_byte_code.h"
#include "CompiledShaders\shader_psJersey_byte_code.h"
#include "CompiledShaders\shader_psScoreboard_byte_code.h"
#include "CompiledShaders\shader_psFont_byte_code.h"
#include "CompiledShaders\shader_psFlatLit_byte_code.h"
#include "CompiledShaders\shader_psTexturedLit_byte_code.h"
#include "CompiledShaders\shader_psTexturedLitBump_byte_code.h"

using namespace DirectX;
using namespace shd;

#define SHD_LERP_LIMIT 8.0f

//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DCompile
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;

	// Disable optimizations to further improve shader debugging
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* pErrorBlob = nullptr;
	hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			pErrorBlob->Release();
		}
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
}

bool Renderer::init(InitParams & rendererInitParams)
{
	m_gpuBuffIndex = 0;
	m_frameIndex = 0;
	m_frameCounter = 0;
	m_instanceHandle = 0;
	m_screenWidth = 0;
	m_screenHeight = 0;
	m_aspectRatio = 0;
	m_windowHandle = 0;
	m_d3dDevice = 0;
	m_d3dDevice1 = 0;
	m_immediateContext = 0;
	m_immediateContext1 = 0;
	m_swapChain = 0;
	m_swapChain1 = 0;
	m_renderTargetView = 0;
	m_depthStencil = 0;
	m_depthStencilView = 0;
	m_projectionMatrix = XMMatrixIdentity();
	m_projectionMatrixUI = XMMatrixIdentity();
	m_viewMatrix = XMMatrixIdentity();
	m_alphaBlendingState = 0;
	memset(m_textureSamplers, 0, sizeof(ID3D11SamplerState *) * TEXTURE_SAMPLER_TYPE_MAX);
	memset(m_vertexShaders, 0, sizeof(ID3D11VertexShader *) * VS_TYPE_MAX);
	memset(m_vertexLayouts, 0, sizeof(ID3D11InputLayout *) * VERTEX_LAYOUT_TYPE_MAX);
	memset(m_pixelShaders, 0, sizeof(ID3D11PixelShader *) * PS_TYPE_MAX);

	bool ret = initWindow(rendererInitParams);
	if (ret == false)
	{
		return false;
	}

	ret = initDevice();
	if (ret == false)
	{
		return false;
	}

	ShowCursor(false);

	ret = m_vertexBank.init(SHD_MAX_VERTICES);
	if (ret == false)
	{
		return false;
	}

	ret = createGpuBoundBuffer(&m_uniformsGpuBuffer, sizeof(Uniforms_t));
	if (ret == false)
	{
		return false;
	}

	ret = createGpuBoundBuffer(&m_psUniformsGpuBuffer, sizeof(PixelShaderUniforms_t));
	if (ret == false)
	{
		return false;
	}

	return true;
}

void Renderer::term()
{

}

void Renderer::resize(float width, float height, bool appliedFromSettings)
{
	if (appliedFromSettings == false)
	{
		m_screenWidthFromOS = width;
		m_screenHeightFromOS = height;
	}

	if (Application::getInstance().globalSettings.screenResolution == SCREEN_RES_DEFAULT)
	{
		m_aspectRatio = fabs(m_screenWidthFromOS / m_screenHeightFromOS);
	}
	else
	{
		if (Application::getInstance().globalSettings.maintainAspectRatio)
		{
			m_aspectRatio = fabs(m_screenWidthFromOS / m_screenHeightFromOS);
			
			if (appliedFromSettings)
				width *= m_aspectRatio;
		}
		else
		{
			if(appliedFromSettings)
				m_aspectRatio = fabs(width / height);
			else
				m_aspectRatio = fabs(m_screenWidth / m_screenHeight);
		}
	}

	switch (Application::getInstance().globalSettings.screenResolution)
	{
	case SCREEN_RES_DEFAULT:
		width = m_screenWidth = m_screenWidthFromOS;
		height = m_screenHeight = m_screenHeightFromOS;
		break;
	case SCREEN_RES_3840_X_2160:
	case SCREEN_RES_2560_X_1440:
	case SCREEN_RES_1920_X_1080:
	case SCREEN_RES_1280_X_720:
	case SCREEN_RES_800_X_600:
		if (appliedFromSettings)
		{
			m_screenWidth = width;
			m_screenHeight = height;
		}
		else
		{
			width = m_screenWidth;
			height = m_screenHeight;
		}

		break;
	default:
		SHD_ASSERT(false);
		break;
	}

#ifdef USE_ORTHO_PROJECTION_MATRIX
	m_projectionMatrix = XMMatrixOrthographicLH((float)kWorldHeight * m_aspectRatio * Application::getInstance().camera.getZoomScaler(), kWorldHeight * Application::getInstance().camera.getZoomScaler(), 0.001f, 1000.0f);
	m_projectionMatrixUI = XMMatrixOrthographicLH((float)kWorldHeight * m_aspectRatio, kWorldHeight, 0.001f, 1000.0f);
#else
	m_projectionMatrix = XMMatrixPerspectiveFovLH((65.0f / 180.0f) * XM_PI, aspect, 0.01f, 100.0f);
#endif

	if (m_swapChain)
	{
		m_immediateContext->OMSetRenderTargets(0, 0, 0);

		// Release all outstanding references to the swap chain's buffers
		m_renderTargetView->Release();

		HRESULT hr;
		// Preserve the existing buffer count and format
		// Automatically choose the width and height to match the client rect for HWNDs
		hr = m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
		if (FAILED(hr))
		{
			SHD_ASSERT(false);
		}

		// Get buffer and create a render-target-view
		ID3D11Texture2D* pBuffer;
		hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBuffer);
		if (FAILED(hr))
		{
			SHD_ASSERT(false);
		}

		hr = m_d3dDevice->CreateRenderTargetView(pBuffer, NULL, &m_renderTargetView);
		pBuffer->Release();
		if (FAILED(hr))
		{
			SHD_ASSERT(false);
		}

		m_immediateContext->OMSetRenderTargets(1, &m_renderTargetView, NULL);

		// Set up the viewport
		D3D11_VIEWPORT vp;
		vp.Width = width;
		vp.Height = height;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		m_immediateContext->RSSetViewports(1, &vp);
	}
}

void Renderer::draw(double delta)
{
	UINT vertexOffset = 0;
	UINT vertexStride = sizeof(Vertex_t);
	Uniforms_t uniforms;
	PixelShaderUniforms_t psUniforms;
	Application * app = &shd::Application::getInstance();
	Vec3f eyePos = app->camera.getPreviousPos();
	Vec3f drawPosition;
	float drawRotation = 0;
	float lerpedZoom = shd::lerp(	Application::getInstance().camera.getPreviousZoom(),
									Application::getInstance().camera.getZoomScaler(),
									delta);

	m_projectionMatrix = XMMatrixOrthographicLH((float)kWorldHeight * m_aspectRatio * lerpedZoom, kWorldHeight * lerpedZoom, 0.001f, 1000.0f);
	app->onscreenDebugText.numDrawCalls = 0;
	eyePos.lerp(app->camera.getPosition(), delta);

	// Set vertex shader uniforms
	memset(&uniforms, 0, sizeof(Uniforms_t));

	uniforms.eyePosition = { eyePos.x, eyePos.y, eyePos.z - 100, 0 };
	uniforms.projectionMatrix = DirectX::XMMatrixTranspose(m_projectionMatrix);
	uniforms.projectionMatrixUI = DirectX::XMMatrixTranspose(m_projectionMatrixUI);
	uniforms.viewMatrix = DirectX::XMMatrixTranspose(DirectX::XMMatrixMultiply(m_viewMatrix, 
							DirectX::XMMatrixTranslation(-eyePos.x, -eyePos.y, -eyePos.z - 10)));

	m_immediateContext->UpdateSubresource(m_uniformsGpuBuffer.buffer, 0, nullptr, &uniforms, 0, 0);
	m_immediateContext->VSSetConstantBuffers( 0, 1, &m_uniformsGpuBuffer.buffer );

	// Set pixel shader uniforms
	memset(&psUniforms, 0, sizeof(PixelShaderUniforms_t));

	for (int i = 0; i < SHD_MAX_LIGHTS; i++)
	{
		psUniforms.lights[i] = app->lights.getLight(i);
	}

	psUniforms.isLightOn = app->lights.getLightOnState();
	psUniforms.ambientLightColour = {	app->lights.getAmbientLight().x,
										app->lights.getAmbientLight().y,
										app->lights.getAmbientLight().z,
										app->lights.getAmbientLight().w };

	m_immediateContext->UpdateSubresource(m_psUniformsGpuBuffer.buffer, 0, nullptr, &psUniforms, 0, 0);
	m_immediateContext->PSSetConstantBuffers(1, 1, &m_psUniformsGpuBuffer.buffer);

	// Clear the back buffer
	m_immediateContext->ClearRenderTargetView( m_renderTargetView, Colors::Black);
	
	// Clear the depth buffer to 1.0 (max depth)
	m_immediateContext->ClearDepthStencilView( m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 );

	for (int i = 0; i < kNumGameObjects; i++)
	{
		if (app->gameObjects[i] == nullptr || app->gameObjects[i]->shouldDraw == false)
		{
			continue;
		}

		XMVECTOR rotationAxis = { app->gameObjects[i]->rotation.x, app->gameObjects[i]->rotation.y, app->gameObjects[i]->rotation.z };

		PerObjectConstantBuffer_t objConstantBuff;
		memset(&objConstantBuff, 0, sizeof(PerObjectConstantBuffer_t));
		objConstantBuff.fadeAlpha = app->gameObjects[i]->drawableObj.fadeAlpha;

		// Linear interpolate the actual position to the drawing position
		// If the position is too far away from the previous position, don't lerp! It looks wrong on screen
		if (app->gameObjects[i]->drawableObj.isUiElement)
		{
			drawPosition = app->gameObjects[i]->position + eyePos;
		}
		else if (app->gameObjects[i]->drawableObj.shouldLerp && (app->gameObjects[i]->position - app->gameObjects[i]->previousPosition).length() < SHD_LERP_LIMIT)
		{
			drawPosition = app->gameObjects[i]->previousPosition;
			drawPosition.lerp(app->gameObjects[i]->position, delta);
		}
		else 
		{
			drawPosition = app->gameObjects[i]->position;
		}

		drawRotation = shd::lerpAngle(app->gameObjects[i]->previousRotation.w, app->gameObjects[i]->rotation.w, delta);

		objConstantBuff.worldMatrix = DirectX::XMMatrixMultiply(
			DirectX::XMMatrixScaling(app->gameObjects[i]->drawableObj.scale.x, app->gameObjects[i]->drawableObj.scale.y, app->gameObjects[i]->drawableObj.scale.z),
			DirectX::XMMatrixMultiply(DirectX::XMMatrixRotationAxis(rotationAxis, drawRotation * XM_PI / 180.0f),
			DirectX::XMMatrixTranslation(drawPosition.x, drawPosition.y, drawPosition.z)));

		// DirectX requires that the matrix is transposed for some reason
		objConstantBuff.worldMatrix = DirectX::XMMatrixTranspose(objConstantBuff.worldMatrix);

		// If there are frames to animate, do that here
		if (app->gameObjects[i]->drawableObj.numAnimFrames > 0)
		{
			objConstantBuff.animationOffset = app->gameObjects[i]->drawableObj.animationCellWidth * (float)(app->gameObjects[i]->drawableObj.currentAnimFrameIndex);
		}

		// Update the constant buffer for the world matrix, etc.
		m_immediateContext->UpdateSubresource(app->gameObjects[i]->drawableObj.gpuPerObjectConstantBuffer->buffer, 0, nullptr, &objConstantBuff, 0, 0);

		// Set vertex buffer that we want to use
		m_immediateContext->IASetVertexBuffers(0, 1, &app->gameObjects[i]->drawableObj.gpuVertexBuffer->buffer, &vertexStride, &vertexOffset);

		// Set relevant shaders, textures, etc. before drawing
		setPipelineState(app->gameObjects[i]->drawableObj.renderPipelineState, &app->gameObjects[i]->drawableObj);

		// Set our constant buffers (world matrix, etc)
		m_immediateContext->VSSetConstantBuffers(2, 1, &app->gameObjects[i]->drawableObj.gpuPerObjectConstantBuffer->buffer);

		// Render the thing
		m_immediateContext->Draw(app->gameObjects[i]->drawableObj.numVertices, 0);
		app->onscreenDebugText.numDrawCalls++;
	}
	
	// Present our back buffer to our front buffer
	if (app->globalSettings.vsyncEnabled == true)
	{
		m_swapChain->Present(1, 0);
	}
	else
	{
		m_swapChain->Present(0, 0);
	}

	m_frameCounter++;
}

bool Renderer::initWindow(InitParams & rendererInitParams)
{
	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = shd::Application::WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = rendererInitParams.instanceHandle;
	wcex.hIcon = LoadIcon(rendererInitParams.instanceHandle, (LPCTSTR)IDI_SNAKEBITE);
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = L"SnakebiteWindowClass";
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SNAKEBITE);
	if (!RegisterClassEx(&wcex))
	{
		return false;
	}

	// Create window
	m_instanceHandle = rendererInitParams.instanceHandle;
	RECT rc = { 0, 0, rendererInitParams.screenWidth, rendererInitParams.screenHeight };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	m_windowHandle = CreateWindow(L"SnakebiteWindowClass", L"Deathmatch Soccer",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, rendererInitParams.instanceHandle,
		nullptr);
	if (!m_windowHandle)
	{
		return false;
	}

	ShowWindow(m_windowHandle, rendererInitParams.cmdShow);

	return true;
}

bool Renderer::createGpuBoundBuffers(DrawableObject * obj, bool isDynamic)
{
	SHD_ASSERT(m_gpuBuffIndex + 1 < SHD_MAX_GPU_SHARED_BUFFERS);

	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.ByteWidth = sizeof( Vertex_t ) * obj->numVerticesAvailable;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	if (isDynamic == true)
	{
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	else
	{
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.CPUAccessFlags = 0;
	}

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory( &InitData, sizeof(InitData) );
	InitData.pSysMem = obj->vertices;

	HRESULT hr = m_d3dDevice->CreateBuffer( &bd, &InitData, &m_gpuSharedBuffers[m_gpuBuffIndex].buffer);
	if (FAILED(hr) || m_gpuSharedBuffers[m_gpuBuffIndex].buffer == nullptr)
	{
		return false;
	}

	obj->gpuVertexBuffer = &m_gpuSharedBuffers[m_gpuBuffIndex];
	m_gpuBuffIndex++;

	// Create the constant buffer for the world matrix and other things
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(PerObjectConstantBuffer_t);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;

	hr = m_d3dDevice->CreateBuffer( &bd, nullptr, &m_gpuSharedBuffers[m_gpuBuffIndex].buffer );
	if (FAILED(hr) || m_gpuSharedBuffers[m_gpuBuffIndex].buffer == nullptr)
	{
		return false;
	}

	obj->gpuPerObjectConstantBuffer = &m_gpuSharedBuffers[m_gpuBuffIndex];
	m_gpuBuffIndex++;

	return true;
}

bool Renderer::createGpuBoundBuffer(GpuSharedBuffer * buffer, size_t size)
{
	SHD_ASSERT(m_gpuBuffIndex + 1 < SHD_MAX_GPU_SHARED_BUFFERS);

	if (buffer == nullptr)
	{
		return false;
	}

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = size;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;

	HRESULT hr = m_d3dDevice->CreateBuffer(&bd, nullptr, &m_gpuSharedBuffers[m_gpuBuffIndex].buffer);
	if (FAILED(hr) || m_gpuSharedBuffers[m_gpuBuffIndex].buffer == nullptr)
	{
		return false;
	}

	*buffer = m_gpuSharedBuffers[m_gpuBuffIndex];
	m_gpuBuffIndex++;

	return true;
}

bool Renderer::updateGpuBoundBuffer(GpuSharedBuffer * buffer, void * newData, size_t dataSize)
{
	D3D11_MAPPED_SUBRESOURCE resource;

	m_immediateContext->Map(buffer->buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
	memcpy(resource.pData, newData, dataSize);
	m_immediateContext->Unmap(buffer->buffer, 0);

	return true;
}

void Renderer::setFullscreen(bool fullscreen)
{
	HRESULT hr = m_swapChain->SetFullscreenState(fullscreen, nullptr);
	if (FAILED(hr))
	{
		SHD_ASSERT(false);
	}
}

void Renderer::updateProjectionMatrix()
{
#ifdef USE_ORTHO_PROJECTION_MATRIX
	m_projectionMatrix = XMMatrixOrthographicLH((float)kWorldHeight * m_aspectRatio * Application::getInstance().camera.getZoomScaler(), kWorldHeight * Application::getInstance().camera.getZoomScaler(), 0.001f, 1000.0f);
#endif
}

#if 0

// Keeping this just in case - shaders compiled at runtime instead of loading the bytecode from a header
bool Renderer::setupRenderPipelines()
{
	/////////////////////////////////
	// Create the vertex shaders
	/////////////////////////////////

	// Vertex shader - basic vertex
	ID3DBlob* pVSBlob = nullptr;

	// TODO - load pre-compiled shader from header/cso file instead compiling at runtime
	HRESULT hr = CompileShaderFromFile(L"shader.fx", "vsBasic", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return false;
	}

	hr = m_d3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_vertexShaders[VS_TYPE_BASIC]);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return false;
	}

	hr = CompileShaderFromFile(L"shader.fx", "vsBasicUI", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return false;
	}

	hr = m_d3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_vertexShaders[VS_TYPE_BASIC_UI]);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return false;
	}

	hr = CompileShaderFromFile(L"shader.fx", "vsLit", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		pVSBlob->Release();
		return false;
	}

	hr = m_d3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_vertexShaders[VS_TYPE_LIT]);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return false;
	}

	/////////////////////////////////
	// Define vertex layouts
	/////////////////////////////////

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOUR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout
	hr = m_d3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &m_vertexLayouts[VERTEX_LAYOUT_TYPE_BASIC]);
	pVSBlob->Release();
	if (FAILED(hr))
		return false;

	/////////////////////////////////
	// Create the pixel shaders
	/////////////////////////////////

	// Pixel shader - Flat colour
	ID3DBlob* pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"shader.fx", "psFlatColour", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return false;
	}

	hr = m_d3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pixelShaders[PS_TYPE_FLAT_COLOUR]);
	pPSBlob->Release();
	if (FAILED(hr))
		return false;

	// Pixel shader - Textured
	pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"shader.fx", "psTexture", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return false;
	}

	hr = m_d3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pixelShaders[PS_TYPE_TEXTURED]);
	pPSBlob->Release();
	if (FAILED(hr))
		return false;

	// Pixel shader - guy
	pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"shader.fx", "psGuy", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return false;
	}

	hr = m_d3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pixelShaders[PS_TYPE_GUY]);
	pPSBlob->Release();
	if (FAILED(hr))
		return false;

	// Pixel shader - jersey
	pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"shader.fx", "psJersey", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return false;
	}

	hr = m_d3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pixelShaders[PS_TYPE_JERSEY]);
	pPSBlob->Release();
	if (FAILED(hr))
		return false;

	// Pixel shader - scoreboard
	pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"shader.fx", "psScoreboard", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return false;
	}

	hr = m_d3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pixelShaders[PS_TYPE_SCOREBOARD]);
	pPSBlob->Release();
	if (FAILED(hr))
		return false;

	// Pixel shader - Font
	pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"shader.fx", "psFont", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return false;
	}

	hr = m_d3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pixelShaders[PS_TYPE_FONT]);
	pPSBlob->Release();
	if (FAILED(hr))
		return false;

	// Pixel shader - Flat lit
	pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"shader.fx", "psFlatLit", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return false;
	}

	hr = m_d3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pixelShaders[PS_TYPE_FLAT_LIT]);
	pPSBlob->Release();
	if (FAILED(hr))
		return false;

	// Pixel shader - Textured lit
	pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"shader.fx", "psTexturedLit", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return false;
	}

	hr = m_d3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pixelShaders[PS_TYPE_TEXTURED_LIT]);
	pPSBlob->Release();
	if (FAILED(hr))
		return false;

	// Pixel shader - Textured lit bump
	pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"shader.fx", "psTexturedLitBump", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return false;
	}

	hr = m_d3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pixelShaders[PS_TYPE_TEXTURED_LIT_BUMP]);
	pPSBlob->Release();
	if (FAILED(hr))
		return false;

	return true;
}

#else

bool Renderer::setupRenderPipelines()
{
	/////////////////////////////////
	// Create the vertex shaders
	/////////////////////////////////

	// Vertex shader - basic vertex
	HRESULT hr = m_d3dDevice->CreateVertexShader(g_shader_vsBasic_byte_code, sizeof(g_shader_vsBasic_byte_code), nullptr, &m_vertexShaders[VS_TYPE_BASIC]);
	if (FAILED(hr))
	{
		return false;
	}

	// Vertex shader - basic vertex for UI
	hr = m_d3dDevice->CreateVertexShader(g_shader_vsBasicUI_byte_code, sizeof(g_shader_vsBasicUI_byte_code), nullptr, &m_vertexShaders[VS_TYPE_BASIC_UI]);
	if (FAILED(hr))
	{
		return false;
	}

	// Vertex shader - vertex shader for lit objects
	hr = m_d3dDevice->CreateVertexShader(g_shader_vsLit_byte_code, sizeof(g_shader_vsLit_byte_code), nullptr, &m_vertexShaders[VS_TYPE_LIT]);
	if (FAILED(hr))
	{
		return false;
	}

	/////////////////////////////////
	// Define vertex layouts
	/////////////////////////////////

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOUR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout
	hr = m_d3dDevice->CreateInputLayout(layout, 
										numElements, 
										g_shader_vsLit_byte_code,
										sizeof(g_shader_vsLit_byte_code),
										&m_vertexLayouts[VERTEX_LAYOUT_TYPE_BASIC]);
	if (FAILED(hr))
	{
		return false;
	}

	/////////////////////////////////
	// Create the pixel shaders
	/////////////////////////////////

	// Pixel shader - Flat colour
	hr = m_d3dDevice->CreatePixelShader(g_shader_psFlatColour_byte_code, sizeof(g_shader_psFlatColour_byte_code), nullptr, &m_pixelShaders[PS_TYPE_FLAT_COLOUR]);
	if (FAILED(hr))
	{
		return false;
	}

	// Pixel shader - Textured
	hr = m_d3dDevice->CreatePixelShader(g_shader_psTexture_byte_code, sizeof(g_shader_psTexture_byte_code), nullptr, &m_pixelShaders[PS_TYPE_TEXTURED]);
	if (FAILED(hr))
	{
		return false;
	}

	// Pixel shader - Guy
	hr = m_d3dDevice->CreatePixelShader(g_shader_psGuy_byte_code, sizeof(g_shader_psGuy_byte_code), nullptr, &m_pixelShaders[PS_TYPE_GUY]);
	if (FAILED(hr))
	{
		return false;
	}

	// Pixel shader - Jersey
	hr = m_d3dDevice->CreatePixelShader(g_shader_psJersey_byte_code, sizeof(g_shader_psJersey_byte_code), nullptr, &m_pixelShaders[PS_TYPE_JERSEY]);
	if (FAILED(hr))
	{
		return false;
	}

	// Pixel shader - Scoreboard
	hr = m_d3dDevice->CreatePixelShader(g_shader_psScoreboard_byte_code, sizeof(g_shader_psScoreboard_byte_code), nullptr, &m_pixelShaders[PS_TYPE_SCOREBOARD]);
	if (FAILED(hr))
	{
		return false;
	}

	// Pixel shader - Font
	hr = m_d3dDevice->CreatePixelShader(g_shader_psFont_byte_code, sizeof(g_shader_psFont_byte_code), nullptr, &m_pixelShaders[PS_TYPE_FONT]);
	if (FAILED(hr))
	{
		return false;
	}

	// Pixel shader - Flat lit
	hr = m_d3dDevice->CreatePixelShader(g_shader_psFlatLit_byte_code, sizeof(g_shader_psFlatLit_byte_code), nullptr, &m_pixelShaders[PS_TYPE_FLAT_LIT]);
	if (FAILED(hr))
	{
		return false;
	}

	// Pixel shader - Textured lit
	hr = m_d3dDevice->CreatePixelShader(g_shader_psTexturedLit_byte_code, sizeof(g_shader_psTexturedLit_byte_code), nullptr, &m_pixelShaders[PS_TYPE_TEXTURED_LIT]);
	if (FAILED(hr))
	{
		return false;
	}

	// Pixel shader - Textured lit bump
	hr = m_d3dDevice->CreatePixelShader(g_shader_psTexturedLitBump_byte_code, sizeof(g_shader_psTexturedLitBump_byte_code), nullptr, &m_pixelShaders[PS_TYPE_TEXTURED_LIT_BUMP]);
	if (FAILED(hr))
	{
		return false;
	}

	return true;
}

#endif

bool Renderer::initDevice()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(m_windowHandle, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		hr = D3D11CreateDevice(nullptr, driverTypes[driverTypeIndex], nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &m_d3dDevice, &featureLevel, &m_immediateContext);

		if (hr == E_INVALIDARG)
		{
			// DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
			hr = D3D11CreateDevice(nullptr, driverTypes[driverTypeIndex], nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
				D3D11_SDK_VERSION, &m_d3dDevice, &featureLevel, &m_immediateContext);
		}

		if (SUCCEEDED(hr))
			break;
	}
	if (FAILED(hr))
	{
		return false;
	}

	// Obtain DXGI factory from device (since we used nullptr for pAdapter above)
	IDXGIFactory1* dxgiFactory = nullptr;
	{
		IDXGIDevice* dxgiDevice = nullptr;
		hr = m_d3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
		if (SUCCEEDED(hr))
		{
			IDXGIAdapter* adapter = nullptr;
			hr = dxgiDevice->GetAdapter(&adapter);
			if (SUCCEEDED(hr))
			{
				hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
				adapter->Release();
			}
			dxgiDevice->Release();
		}
	}
	if (FAILED(hr))
	{
		return false;
	}

	// Create swap chain
	IDXGIFactory2* dxgiFactory2 = nullptr;
	hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));
	if (dxgiFactory2)
	{
		// DirectX 11.1 or later
		hr = m_d3dDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&m_d3dDevice1));
		if (SUCCEEDED(hr))
		{
			(void)m_immediateContext->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&m_immediateContext1));
		}

		DXGI_SWAP_CHAIN_DESC1 sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.Width = width;
		sd.Height = height;
		sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = 1;

		hr = dxgiFactory2->CreateSwapChainForHwnd(m_d3dDevice, m_windowHandle, &sd, nullptr, nullptr, &m_swapChain1);
		if (SUCCEEDED(hr))
		{
			hr = m_swapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&m_swapChain));
		}

		dxgiFactory2->Release();
	}
	else
	{
		// DirectX 11.0 systems
		DXGI_SWAP_CHAIN_DESC sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.BufferCount = 1;
		sd.BufferDesc.Width = width;
		sd.BufferDesc.Height = height;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = m_windowHandle;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;

		hr = dxgiFactory->CreateSwapChain(m_d3dDevice, &sd, &m_swapChain);
	}

	dxgiFactory->Release();

	if (FAILED(hr))
	{
		return false;
	}

	// Create a render target view
	ID3D11Texture2D* pBackBuffer = nullptr;
	hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
	if (FAILED(hr))
	{
		return false;
	}

	hr = m_d3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_renderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr))
	{
		return false;
	}

	// Create depth stencil texture
	D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = m_d3dDevice->CreateTexture2D(&descDepth, nullptr, &m_depthStencil);
	if (FAILED(hr))
	{
		return false;
	}

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
#ifdef USE_ORTHO_PROJECTION_MATRIX
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
#else
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS; // D3D11_COMPARISON_LESS
#endif
	depthStencilDesc.StencilEnable = FALSE;
	depthStencilDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	depthStencilDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;

	// Create depth stencil state
	ID3D11DepthStencilState * pDSState;
	hr = m_d3dDevice->CreateDepthStencilState(&depthStencilDesc, &pDSState);
	if (FAILED(hr))
	{
		return false;
	}

	// Bind depth stencil state
	m_immediateContext->OMSetDepthStencilState(pDSState, 1);

	// Create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = m_d3dDevice->CreateDepthStencilView(m_depthStencil, &descDSV, &m_depthStencilView);
	if (FAILED(hr))
	{
		return false;
	}

	m_immediateContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);

	// Setup blending state
	D3D11_BLEND_DESC blendStateDescription;
	ZeroMemory(&blendStateDescription, sizeof(D3D11_BLEND_DESC));

	blendStateDescription.RenderTarget[0].BlendEnable = TRUE;
	blendStateDescription.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendStateDescription.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendStateDescription.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendStateDescription.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendStateDescription.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDescription.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendStateDescription.RenderTarget[0].RenderTargetWriteMask = 0x0f;

	hr = m_d3dDevice->CreateBlendState(&blendStateDescription, &m_alphaBlendingState);
	if (FAILED(hr))
	{
		return false;
	}

	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	m_immediateContext->OMSetBlendState(m_alphaBlendingState, blendFactor, 0xffffffff);

	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	m_immediateContext->RSSetViewports(1, &vp);

	// Setup the shaders
	bool ret = setupRenderPipelines();
	if (ret == false)
	{
		return false;
	}

	// Set primitive topology
	m_immediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Create the sample state
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = m_d3dDevice->CreateSamplerState(&sampDesc, &m_textureSamplers[TEXTURE_SAMPLER_TYPE_POINT]);
	if (FAILED(hr))
		return false;

	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	hr = m_d3dDevice->CreateSamplerState(&sampDesc, &m_textureSamplers[TEXTURE_SAMPLER_TYPE_LINEAR]);
	if (FAILED(hr))
		return false;

	// Set up projection matrix
	resize(width, height);

	// Initialize the view matrix
	XMVECTOR Eye = XMVectorSet(0.0f, 0.0f, -100.0f, 0.0f);
	XMVECTOR At = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	m_viewMatrix = XMMatrixLookAtLH(Eye, At, Up);

	return true;
}

void Renderer::setPipelineState(RenderPipelineState state, DrawableObject * obj)
{
	if (obj == nullptr)
	{
		SHD_ASSERT(false);
		return;
	}

	RenderingState currentState = m_lastState;

	switch (state)
	{
	case PIPELINE_STATE_FLAT_COLOUR:
		currentState.vertexShader = VS_TYPE_BASIC;
		currentState.pixelShader = PS_TYPE_FLAT_COLOUR;
		currentState.vertexLayout = VERTEX_LAYOUT_TYPE_BASIC;
		break;

	case PIPELINE_STATE_FLAT_COLOUR_UI:
		currentState.vertexShader = VS_TYPE_BASIC_UI;
		currentState.pixelShader = PS_TYPE_FLAT_COLOUR;
		currentState.vertexLayout = VERTEX_LAYOUT_TYPE_BASIC;
		break;

	case PIPELINE_STATE_TEXTURED:
		currentState.vertexShader = VS_TYPE_BASIC;
		currentState.pixelShader = PS_TYPE_TEXTURED;
		currentState.vertexLayout = VERTEX_LAYOUT_TYPE_BASIC;
		currentState.textureSampler = TEXTURE_SAMPLER_TYPE_POINT;
		break;

	case PIPELINE_STATE_TEXTURED_UI:
		currentState.vertexShader = VS_TYPE_BASIC_UI;
		currentState.pixelShader = PS_TYPE_TEXTURED;
		currentState.vertexLayout = VERTEX_LAYOUT_TYPE_BASIC;
		currentState.textureSampler = TEXTURE_SAMPLER_TYPE_LINEAR;
		break;

	case PIPELINE_STATE_GUY:
		currentState.vertexShader = VS_TYPE_LIT;
		currentState.pixelShader = PS_TYPE_GUY;
		currentState.vertexLayout = VERTEX_LAYOUT_TYPE_BASIC;
		currentState.textureSampler = TEXTURE_SAMPLER_TYPE_POINT;
		break;

	case PIPELINE_STATE_JERSEY:
		currentState.vertexShader = VS_TYPE_BASIC;
		currentState.pixelShader = PS_TYPE_JERSEY;
		currentState.vertexLayout = VERTEX_LAYOUT_TYPE_BASIC;
		currentState.textureSampler = TEXTURE_SAMPLER_TYPE_POINT;
		break;

	case PIPELINE_STATE_JERSEY_UI:
		currentState.vertexShader = VS_TYPE_BASIC_UI;
		currentState.pixelShader = PS_TYPE_JERSEY;
		currentState.vertexLayout = VERTEX_LAYOUT_TYPE_BASIC;
		currentState.textureSampler = TEXTURE_SAMPLER_TYPE_POINT;
		break;

	case PIPELINE_STATE_SCOREBOARD:
		currentState.vertexShader = VS_TYPE_BASIC_UI;
		currentState.pixelShader = PS_TYPE_SCOREBOARD;
		currentState.vertexLayout = VERTEX_LAYOUT_TYPE_BASIC;
		currentState.textureSampler = TEXTURE_SAMPLER_TYPE_POINT;
		break;

	case PIPELINE_STATE_FONT:
		currentState.vertexShader = VS_TYPE_BASIC;
		currentState.pixelShader = PS_TYPE_FONT;
		currentState.vertexLayout = VERTEX_LAYOUT_TYPE_BASIC;
		currentState.textureSampler = TEXTURE_SAMPLER_TYPE_LINEAR;
		break;

	case PIPELINE_STATE_FONT_UI:
		currentState.vertexShader = VS_TYPE_BASIC_UI;
		currentState.pixelShader = PS_TYPE_FONT;
		currentState.vertexLayout = VERTEX_LAYOUT_TYPE_BASIC;
		currentState.textureSampler = TEXTURE_SAMPLER_TYPE_LINEAR;
		break;

	case PIPELINE_STATE_FLAT_LIT:
		currentState.vertexShader = VS_TYPE_LIT;
		currentState.pixelShader = PS_TYPE_FLAT_LIT;
		currentState.vertexLayout = VERTEX_LAYOUT_TYPE_BASIC;
		currentState.textureSampler = TEXTURE_SAMPLER_TYPE_POINT;
		break;

	case PIPELINE_STATE_TEXTURED_LIT:
		currentState.vertexShader = VS_TYPE_LIT;
		currentState.pixelShader = PS_TYPE_TEXTURED_LIT;
		currentState.vertexLayout = VERTEX_LAYOUT_TYPE_BASIC;
		currentState.textureSampler = TEXTURE_SAMPLER_TYPE_POINT;
		break;

	case PIPELINE_STATE_TEXTURED_LIT_BUMP:
		currentState.vertexShader = VS_TYPE_LIT;
		currentState.pixelShader = PS_TYPE_TEXTURED_LIT_BUMP;
		currentState.vertexLayout = VERTEX_LAYOUT_TYPE_BASIC;
		currentState.textureSampler = TEXTURE_SAMPLER_TYPE_POINT;
		break;

	default:
		SHD_ASSERT(false);
	}

	if (currentState.vertexShader != m_lastState.vertexShader)
	{
		m_immediateContext->VSSetShader(m_vertexShaders[currentState.vertexShader], nullptr, 0);
	}

	if (currentState.pixelShader != m_lastState.pixelShader)
	{
		m_immediateContext->PSSetShader(m_pixelShaders[currentState.pixelShader], nullptr, 0);
	}

	if (currentState.vertexLayout != m_lastState.vertexLayout)
	{
		m_immediateContext->IASetInputLayout(m_vertexLayouts[currentState.vertexLayout]);
	}

	if (currentState.textureSampler != m_lastState.textureSampler)
	{
		m_immediateContext->PSSetSamplers(0, 1, &m_textureSamplers[currentState.textureSampler]);
	}

	m_lastState = currentState;

	if(obj->texture != nullptr && m_lastState.textureHandle != obj->texture->textureHandle)
	{
		m_immediateContext->PSSetShaderResources(0, 1, &obj->texture->textureHandle);
		m_lastState.textureHandle = obj->texture->textureHandle;
	}

	ID3D11ShaderResourceView * normalMapTexture = Application::getInstance().textureAtlas.getNormalMapTexture()->textureHandle;

	if (normalMapTexture != nullptr && m_lastState.normalMaptextureHandle != normalMapTexture)
	{
		m_immediateContext->PSSetShaderResources(1, 1, &normalMapTexture);
		m_lastState.normalMaptextureHandle = normalMapTexture;
	}
}

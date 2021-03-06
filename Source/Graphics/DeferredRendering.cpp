#include "DeferredRendering.h"
#include "../Helper/GeneralHelper.h"

DeferredRendering::DeferredRendering()
{
	for (int i = 0; i < G_BUFFER_COUNT; ++i)
	{
		m_gBuffer[i] = 0;
		m_gBufferRTV[i] = 0;
		m_gBufferSRV[i] = 0;
	}

	m_postpBuffer = 0;
	m_postpRTV = 0;
	m_postpSRV = 0;

	m_depthStencilBuffer = 0;
	m_depthStencilView = 0;

	m_ppdepthStencilBuffer = 0;
	m_ppdepthStencilView = 0;
}

DeferredRendering::~DeferredRendering()
{
}

bool DeferredRendering::Initialize(ID3D11Device * device, int textureWidth, int textureHeight, float screenDepth, float screenNear, int spec)
{
	D3D11_TEXTURE2D_DESC texDesc;
	HRESULT result;

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	D3D11_TEXTURE2D_DESC depthBufferDesc;
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	int i;

	SpecDefiner(textureWidth, textureHeight, spec);

	ZeroMemory(&texDesc, sizeof(texDesc));
	
	texDesc.Width = m_textureWidth;
	texDesc.Height = m_textureHeight;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	for (int i = 0; i < G_BUFFER_COUNT; ++i)
	{
		result = device->CreateTexture2D(&texDesc, NULL, &m_gBuffer[i]);
		if (FAILED(result))
		{
			return false;
		}
	}

	texDesc.Width = m_realWidth;
	texDesc.Height = m_realHeight;

	result = device->CreateTexture2D(&texDesc, NULL, &m_postpBuffer);
	if (FAILED(result))
	{
		RadixLog("Failed to create post processing buffer");
		return false;
	}

	rtvDesc.Format = texDesc.Format;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;

	for (int i = 0; i < G_BUFFER_COUNT; ++i)
	{
		result = device->CreateRenderTargetView(m_gBuffer[i], &rtvDesc, &m_gBufferRTV[i]);
		if (FAILED(result))
		{
			return false;
		}
	}

	result = device->CreateRenderTargetView(m_postpBuffer, &rtvDesc, &m_postpRTV);
	if (FAILED(result))
	{
		RadixLog("Failed to create post processing RTV");
		return false;
	}

	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	for (int i = 0; i < G_BUFFER_COUNT; ++i)
	{
		result = device->CreateShaderResourceView(m_gBuffer[i], &srvDesc, &m_gBufferSRV[i]);
		if (FAILED(result))
		{
			return false;
		}
	}

	result = device->CreateShaderResourceView(m_postpBuffer, &srvDesc, &m_postpSRV);
	if (FAILED(result))
	{
		RadixLog("Failed to create post processing SRV");
		return false;
	}

	ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));

	depthBufferDesc.Width = m_textureWidth;
	depthBufferDesc.Height = m_textureHeight;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = 0;

	result = device->CreateTexture2D(&depthBufferDesc, NULL, &m_depthStencilBuffer);
	if (FAILED(result))
	{
		return false;
	}

	depthBufferDesc.Width = m_realWidth;
	depthBufferDesc.Height = m_realHeight;

	result = device->CreateTexture2D(&depthBufferDesc, NULL, &m_ppdepthStencilBuffer);
	if (FAILED(result))
	{
		return false;
	}

	ZeroMemory(&dsvDesc, sizeof(dsvDesc));

	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;

	result = device->CreateDepthStencilView(m_depthStencilBuffer, &dsvDesc, &m_depthStencilView);
	if (FAILED(result))
	{
		return false;
	}

	result = device->CreateDepthStencilView(m_ppdepthStencilBuffer, &dsvDesc, &m_ppdepthStencilView);
	if (FAILED(result))
	{
		return false;
	}

	return true;
}

void DeferredRendering::Shutdown()
{
	ReleaseCOM(m_ppdepthStencilView);
	ReleaseCOM(m_ppdepthStencilBuffer);

	ReleaseCOM(m_depthStencilView);
	ReleaseCOM(m_depthStencilBuffer);

	ReleaseCOM(m_postpBuffer);
	ReleaseCOM(m_postpRTV);
	ReleaseCOM(m_postpSRV);

	for (int i = 0; i < G_BUFFER_COUNT; ++i)
	{
		ReleaseCOM(m_gBufferSRV[i]);
		ReleaseCOM(m_gBufferRTV[i]);
		ReleaseCOM(m_gBuffer[i]);
	}
}

void DeferredRendering::SpecDefiner(int screenWidth, int screenHeight, int spec)
{
	m_realWidth = screenWidth;
	m_realHeight = screenHeight;

	if (spec == 0) //Ultra
	{
		//No resolution down grade
		m_textureWidth = screenWidth;
		m_textureHeight = screenHeight;
	}
	else if (spec == 1) //High
	{
		if ((screenWidth / screenHeight) == (16 / 9))
		{
			//One resolution down grade.
			if (screenWidth > 1920)
			{
				m_textureWidth = 1920;
				m_textureHeight = 1080;
			}
			else if (screenWidth == 1920)
			{
				m_textureWidth = 1280;
				m_textureHeight = 720;
			}
			else
			{
				m_textureWidth = 720;
				m_textureHeight = 480;
			}
		}
		else
		{
			m_textureWidth = screenWidth;
			m_textureHeight = screenHeight;
		}
	}
	else //low
	{
		if ((screenWidth / screenHeight) == (16 / 9))
		{
			//two resolution down grade.
			if (screenWidth > 1920)
			{
				m_textureWidth = 1280;
				m_textureHeight = 720;
			}
			else if (screenWidth == 1920)
			{
				m_textureWidth = 1280;
				m_textureHeight = 720;
			}
			else
			{
				m_textureWidth = 720;
				m_textureHeight = 480;
			}
		}
		else
		{
			m_textureWidth = screenWidth;
			m_textureHeight = screenHeight;
		}
	}
}

void DeferredRendering::SetRenderTargets(ID3D11DeviceContext * deviceContext)
{
	deviceContext->OMSetRenderTargets(G_BUFFER_COUNT, m_gBufferRTV, m_depthStencilView);
}

void DeferredRendering::ClearRenderTargets(ID3D11DeviceContext * deviceContext)
{
	float color[4] = {0.0f, 0.0f, 0.0f, 1.0f};

	for (int i = 0; i < G_BUFFER_COUNT; ++i)
	{
		deviceContext->ClearRenderTargetView(m_gBufferRTV[i], color);
	}
	
	deviceContext->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void DeferredRendering::SetPostpRenderTarget(ID3D11DeviceContext* deviceContext)
{
	deviceContext->OMSetRenderTargets(1, &m_postpRTV, m_depthStencilView);
}

void DeferredRendering::ClearPostpRenderTarget(ID3D11DeviceContext* deviceContext)
{
	float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

	deviceContext->ClearRenderTargetView(m_postpRTV, color);
	
	deviceContext->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

ID3D11ShaderResourceView * DeferredRendering::GetShaderResourceView(int index)
{
	return m_gBufferSRV[index];
}

ID3D11ShaderResourceView * DeferredRendering::GetPostpSRV()
{
	return m_postpSRV;
}
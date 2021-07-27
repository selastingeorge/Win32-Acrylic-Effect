#include "AcrylicCompositor.h"

AcrylicCompositor::AcrylicCompositor(HWND hwnd)
{
	InitLibs();
	CreateCompositionDevice();
	CreateEffectGraph(dcompDevice3);
}

bool AcrylicCompositor::SetAcrylicEffect(HWND hwnd, BackdropSource source, AcrylicEffectParameter params)
{
	fallbackColor = params.fallbackColor;
	tintColor = params.tintColor;
	if (source == BACKDROP_SOURCE_HOSTBACKDROP)
	{
		BOOL enable = TRUE;
		WINDOWCOMPOSITIONATTRIBDATA CompositionAttribute{};
		CompositionAttribute.Attrib = WCA_EXCLUDED_FROM_LIVEPREVIEW;
		CompositionAttribute.pvData = &enable;
		CompositionAttribute.cbData = sizeof(BOOL);
		DwmSetWindowCompositionAttribute(hwnd, &CompositionAttribute);
	}

	CreateBackdrop(hwnd,source);
	CreateCompositionVisual(hwnd);
	CreateFallbackVisual();
	fallbackVisual->SetContent(swapChain.Get());
	rootVisual->RemoveAllVisuals();
	switch (source)
	{
		case AcrylicCompositor::BACKDROP_SOURCE_DESKTOP:
			rootVisual->AddVisual(desktopWindowVisual.Get(), false, NULL);
			rootVisual->AddVisual(fallbackVisual.Get(), true, desktopWindowVisual.Get());
			break;
		case AcrylicCompositor::BACKDROP_SOURCE_HOSTBACKDROP:
			rootVisual->AddVisual(desktopWindowVisual.Get(), false, NULL);
			rootVisual->AddVisual(topLevelWindowVisual.Get(), true, desktopWindowVisual.Get());
			rootVisual->AddVisual(fallbackVisual.Get(), true, topLevelWindowVisual.Get());
			break;
		default:
			rootVisual->RemoveAllVisuals();
			break;
	}
	
	rootVisual->SetClip(clip.Get());
	rootVisual->SetTransform(translateTransform.Get());

	saturationEffect->SetSaturation(params.saturationAmount);

	blurEffect->SetBorderMode(D2D1_BORDER_MODE_HARD);
	blurEffect->SetInput(0, saturationEffect.Get(), 0);
	blurEffect->SetStandardDeviation(params.blurAmount);

	rootVisual->SetEffect(blurEffect.Get());
	Commit();

	SyncCoordinates(hwnd);

	return true;
}

long AcrylicCompositor::GetBuildVersion()
{
	if (GetVersionInfo != nullptr)
	{
		RTL_OSVERSIONINFOW versionInfo = { 0 };
		versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
		if (GetVersionInfo(&versionInfo) == 0x00000000)
		{
			return versionInfo.dwBuildNumber;
		}
	}
	return 0;
}

bool AcrylicCompositor::InitLibs()
{
	auto dwmapi = LoadLibrary(L"dwmapi.dll");
	auto user32 = LoadLibrary(L"user32.dll");
	auto ntdll = GetModuleHandleW(L"ntdll.dll");

	if (!dwmapi || !user32 || !ntdll)
	{
		return false;
	}

	GetVersionInfo = (RtlGetVersionPtr)GetProcAddress(ntdll, "RtlGetVersion");
	DwmSetWindowCompositionAttribute = (SetWindowCompositionAttribute)GetProcAddress(user32, "SetWindowCompositionAttribute");
	DwmCreateSharedThumbnailVisual = (DwmpCreateSharedThumbnailVisual)GetProcAddress(dwmapi, MAKEINTRESOURCEA(147));
	DwmCreateSharedMultiWindowVisual = (DwmpCreateSharedMultiWindowVisual)GetProcAddress(dwmapi, MAKEINTRESOURCEA(163));
	DwmUpdateSharedMultiWindowVisual = (DwmpUpdateSharedMultiWindowVisual)GetProcAddress(dwmapi, MAKEINTRESOURCEA(164));
	DwmCreateSharedVirtualDesktopVisual = (DwmpCreateSharedVirtualDesktopVisual)GetProcAddress(dwmapi, MAKEINTRESOURCEA(163));
	DwmUpdateSharedVirtualDesktopVisual = (DwmpUpdateSharedVirtualDesktopVisual)GetProcAddress(dwmapi, MAKEINTRESOURCEA(164));

	return true;
}

bool AcrylicCompositor::CreateCompositionDevice()
{
	if (D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, NULL,D3D11_CREATE_DEVICE_BGRA_SUPPORT, NULL, 0, D3D11_SDK_VERSION, d3d11Device.GetAddressOf(), nullptr, nullptr) != S_OK)
	{
		return false;
	}

	if (d3d11Device->QueryInterface(dxgiDevice.GetAddressOf()) != S_OK)
	{
		return false;
	}

	if (D2D1CreateFactory(D2D1_FACTORY_TYPE::D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory2), (void**)d2dFactory2.GetAddressOf()) != S_OK)
	{
		return false;
	}

	if (d2dFactory2->CreateDevice(dxgiDevice.Get(), d2Device.GetAddressOf()) != S_OK)
	{
		return false;
	}

	if(DCompositionCreateDevice3(dxgiDevice.Get(),__uuidof(dcompDevice),(void**)dcompDevice.GetAddressOf()) != S_OK)
	{
		return false;
	}

	if (dcompDevice->QueryInterface(__uuidof(IDCompositionDevice3), (LPVOID*)&dcompDevice3) != S_OK)
	{
		return false;
	}

	return true;
}

bool AcrylicCompositor::CreateFallbackVisual()
{
	description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	description.BufferCount = 2;
	description.SampleDesc.Count = 1;
	description.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

	description.Width = GetSystemMetrics(SM_CXSCREEN);
	description.Height = GetSystemMetrics(SM_CYSCREEN);

	d3d11Device.As(&dxgiDevice);

	if (CreateDXGIFactory2(0, __uuidof(dxgiFactory), reinterpret_cast<void**>(dxgiFactory.GetAddressOf())) != S_OK)
	{
		return false;
	}

	if (dxgiFactory->CreateSwapChainForComposition(dxgiDevice.Get(), &description, nullptr, swapChain.GetAddressOf()) != S_OK)
	{
		return false;
	}

	if (d2Device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, deviceContext.GetAddressOf()) != S_OK)
	{
		return false;
	}

	if (swapChain->GetBuffer(0, __uuidof(fallbackSurface), reinterpret_cast<void**>(fallbackSurface.GetAddressOf())) != S_OK)
	{
		return false;
	}

	properties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
	properties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
	properties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
	if (deviceContext->CreateBitmapFromDxgiSurface(fallbackSurface.Get(), properties, fallbackBitmap.GetAddressOf()) != S_OK)
	{
		return false;
	}

	deviceContext->SetTarget(fallbackBitmap.Get());
	deviceContext->BeginDraw();
	deviceContext->Clear();
	deviceContext->CreateSolidColorBrush(tintColor, fallbackBrush.GetAddressOf());
	deviceContext->FillRectangle(fallbackRect, fallbackBrush.Get());
	deviceContext->EndDraw();

	if (swapChain->Present(1, 0)!=S_OK)
	{
		return false;
	}

	return true;
}

bool AcrylicCompositor::CreateCompositionVisual(HWND hwnd)
{
	dcompDevice3->CreateVisual(&rootVisual);
	dcompDevice3->CreateVisual(&fallbackVisual);

	if (!CreateCompositionTarget(hwnd))
	{
		return false;
	}

	if (dcompTarget->SetRoot(rootVisual.Get()) != S_OK)
	{
		return false;
	}

	return true;
}

bool AcrylicCompositor::CreateCompositionTarget(HWND hwnd)
{
	if (dcompDevice->CreateTargetForHwnd(hwnd, FALSE, dcompTarget.GetAddressOf()) != S_OK)
	{
		return false;
	}

	return true;
}

bool AcrylicCompositor::CreateBackdrop(HWND hwnd,BackdropSource source)
{
	switch (source)
	{
		case BACKDROP_SOURCE_DESKTOP:
			desktopWindow = (HWND)FindWindow(L"Progman", NULL);

			GetWindowRect(desktopWindow, &desktopWindowRect);
			thumbnailSize.cx = (desktopWindowRect.right - desktopWindowRect.left);
			thumbnailSize.cy = (desktopWindowRect.bottom - desktopWindowRect.top);

			thumbnail.dwFlags = DWM_TNP_SOURCECLIENTAREAONLY | DWM_TNP_VISIBLE | DWM_TNP_RECTDESTINATION | DWM_TNP_RECTSOURCE | DWM_TNP_OPACITY | DWM_TNP_ENABLE3D;
			thumbnail.opacity = 255;
			thumbnail.fVisible = TRUE;
			thumbnail.fSourceClientAreaOnly = FALSE;
			thumbnail.rcDestination = RECT{ 0, 0, thumbnailSize.cx, thumbnailSize.cy };
			thumbnail.rcSource = RECT{ 0, 0, thumbnailSize.cx, thumbnailSize.cy };
			if (DwmCreateSharedThumbnailVisual(hwnd, desktopWindow, 2, &thumbnail, dcompDevice.Get(), (void**)desktopWindowVisual.GetAddressOf(), &desktopThumbnail) != S_OK)
			{
				return false;
			}
			break;
		case BACKDROP_SOURCE_HOSTBACKDROP:
			if (GetBuildVersion() >= 20000)
			{
				hr = DwmCreateSharedMultiWindowVisual(hwnd, dcompDevice.Get(), (void**)topLevelWindowVisual.GetAddressOf(), &topLevelWindowThumbnail);
			}
			else
			{
				hr = DwmCreateSharedVirtualDesktopVisual(hwnd, dcompDevice.Get(), (void**)topLevelWindowVisual.GetAddressOf(), &topLevelWindowThumbnail);
			}

			if (!CreateBackdrop(hwnd, BACKDROP_SOURCE_DESKTOP) || hr != S_OK)
			{
				return false;
			}
			hwndExclusionList = new HWND[1];
			hwndExclusionList[0] = (HWND)0x0;

			if (GetBuildVersion() >= 20000)
			{
				hr = DwmUpdateSharedMultiWindowVisual(topLevelWindowThumbnail, NULL, 0, hwndExclusionList, 1, &sourceRect, &destinationSize, 1);
			}
			else
			{
				hr = DwmUpdateSharedVirtualDesktopVisual(topLevelWindowThumbnail, NULL, 0, hwndExclusionList, 1, &sourceRect, &destinationSize);
			}

			if (hr != S_OK)
			{
				return false;
			}
			break;
	}
	return true;
}

bool AcrylicCompositor::CreateEffectGraph(ComPtr<IDCompositionDevice3> dcompDevice3)
{
	if (dcompDevice3->CreateGaussianBlurEffect(blurEffect.GetAddressOf()) != S_OK)
	{
		return false;
	}
	if (dcompDevice3->CreateSaturationEffect(saturationEffect.GetAddressOf()) != S_OK)
	{
		return false;
	}
	if (dcompDevice3->CreateTranslateTransform(&translateTransform) != S_OK)
	{
		return false;
	}
	if (dcompDevice3->CreateRectangleClip(&clip) != S_OK)
	{
		return false;
	}
	return true;
}

void AcrylicCompositor::SyncCoordinates(HWND hwnd)
{
	GetWindowRect(hwnd, &hostWindowRect);
	clip->SetLeft((float)hostWindowRect.left);
	clip->SetRight((float)hostWindowRect.right);
	clip->SetTop((float)hostWindowRect.top);
	clip->SetBottom((float)hostWindowRect.bottom);
	rootVisual->SetClip(clip.Get());
	translateTransform->SetOffsetX(-1 * (float)hostWindowRect.left - (GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER)));
	translateTransform->SetOffsetY(-1 * (float)hostWindowRect.top - (GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CXPADDEDBORDER)));
	rootVisual->SetTransform(translateTransform.Get());
	Commit();
	DwmFlush();
}

bool AcrylicCompositor::Sync(HWND hwnd, int msg, WPARAM wParam, LPARAM lParam,bool active)
{
	switch (msg)
	{
		case WM_ACTIVATE:
			SyncFallbackVisual(active);
			Flush();
			return true;
		case WM_WINDOWPOSCHANGED:
			SyncCoordinates(hwnd);
			return true;
		case WM_CLOSE:
			delete[] hwndExclusionList;
			return true;
	}
	return false;
}

bool AcrylicCompositor::Flush()
{
	if (topLevelWindowThumbnail !=NULL)
	{
		if (GetBuildVersion() >= 20000)
		{
			DwmUpdateSharedMultiWindowVisual(topLevelWindowThumbnail, NULL, 0, hwndExclusionList, 1, &sourceRect, &destinationSize, 1);
		}
		else
		{
			DwmUpdateSharedVirtualDesktopVisual(topLevelWindowThumbnail, NULL, 0, hwndExclusionList, 1, &sourceRect, &destinationSize);
		}
		DwmFlush();
	}
	return true;
}

bool AcrylicCompositor::Commit()
{
	if (dcompDevice->Commit() != S_OK)
	{
		return false;
	}
	return true;
}

void AcrylicCompositor::SyncFallbackVisual(bool active)
{
	if (!active)
	{
		fallbackBrush->SetColor(fallbackColor);
	}
	else
	{
		fallbackBrush->SetColor(tintColor);
	}

	deviceContext->BeginDraw();
	deviceContext->Clear();
	deviceContext->FillRectangle(fallbackRect, fallbackBrush.Get());
	deviceContext->EndDraw();
	swapChain->Present(1, 0);
}

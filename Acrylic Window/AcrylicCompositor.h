#pragma once

#ifndef UNICODE
#define UNICODE
#endif 

#include <Windows.h>
#include <dcomp.h>
#include <d2d1_2.h>
#include <dwmapi.h>
#include <dxgi1_3.h>
#include <d3d11_2.h>
#include <d2d1_2helper.h>
#include <comutil.h>
#include <wrl\implements.h>

#define DWM_TNP_FREEZE            0x100000
#define DWM_TNP_ENABLE3D          0x4000000
#define DWM_TNP_DISABLE3D         0x8000000
#define DWM_TNP_FORCECVI          0x40000000
#define DWM_TNP_DISABLEFORCECVI   0x80000000

#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d2d1")
#pragma comment(lib, "dcomp")
#pragma comment(lib, "dwmapi")

using namespace Microsoft::WRL;

class AcrylicCompositor
{
	public:
		enum BackdropSource
		{
			BACKDROP_SOURCE_DESKTOP = 0x0,
			BACKDROP_SOURCE_HOSTBACKDROP = 0x1
		};

		struct AcrylicEffectParameter
		{
			float blurAmount;
			float saturationAmount;
			D2D1_COLOR_F tintColor;
			D2D1_COLOR_F fallbackColor;
		};

		AcrylicCompositor(HWND hwnd);
		bool Sync(HWND hwnd,int msg,WPARAM wParam,LPARAM lParam,bool active);
		bool SetAcrylicEffect(HWND hwnd,BackdropSource source,AcrylicEffectParameter params);

	private:
		enum WINDOWCOMPOSITIONATTRIB
		{
			WCA_UNDEFINED = 0x0,
			WCA_NCRENDERING_ENABLED = 0x1,
			WCA_NCRENDERING_POLICY = 0x2,
			WCA_TRANSITIONS_FORCEDISABLED = 0x3,
			WCA_ALLOW_NCPAINT = 0x4,
			WCA_CAPTION_BUTTON_BOUNDS = 0x5,
			WCA_NONCLIENT_RTL_LAYOUT = 0x6,
			WCA_FORCE_ICONIC_REPRESENTATION = 0x7,
			WCA_EXTENDED_FRAME_BOUNDS = 0x8,
			WCA_HAS_ICONIC_BITMAP = 0x9,
			WCA_THEME_ATTRIBUTES = 0xA,
			WCA_NCRENDERING_EXILED = 0xB,
			WCA_NCADORNMENTINFO = 0xC,
			WCA_EXCLUDED_FROM_LIVEPREVIEW = 0xD,
			WCA_VIDEO_OVERLAY_ACTIVE = 0xE,
			WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 0xF,
			WCA_DISALLOW_PEEK = 0x10,
			WCA_CLOAK = 0x11,
			WCA_CLOAKED = 0x12,
			WCA_ACCENT_POLICY = 0x13,
			WCA_FREEZE_REPRESENTATION = 0x14,
			WCA_EVER_UNCLOAKED = 0x15,
			WCA_VISUAL_OWNER = 0x16,
			WCA_HOLOGRAPHIC = 0x17,
			WCA_EXCLUDED_FROM_DDA = 0x18,
			WCA_PASSIVEUPDATEMODE = 0x19,
			WCA_LAST = 0x1A,
		};

		struct WINDOWCOMPOSITIONATTRIBDATA
		{
			WINDOWCOMPOSITIONATTRIB Attrib;
			void* pvData;
			DWORD cbData;
		};

		ComPtr<ID2D1Device1> d2Device;
		ComPtr<ID3D11Device> d3d11Device;
		ComPtr<IDXGIDevice2> dxgiDevice;
		ComPtr<IDXGIFactory2> dxgiFactory;
		ComPtr<ID2D1Factory2> d2dFactory2;
		ComPtr<ID2D1DeviceContext> deviceContext;
		ComPtr<IDCompositionDesktopDevice> dcompDevice;
		ComPtr<IDCompositionDevice3> dcompDevice3;
		ComPtr<IDCompositionTarget> dcompTarget;

		ComPtr<IDCompositionVisual2> rootVisual;
		ComPtr<IDCompositionVisual2> fallbackVisual;
		ComPtr<IDCompositionVisual2> desktopWindowVisual;
		ComPtr<IDCompositionVisual2> topLevelWindowVisual;

		#pragma region Acrylic Essentials

		ComPtr<IDCompositionGaussianBlurEffect> blurEffect;
		ComPtr<IDCompositionSaturationEffect> saturationEffect;
		ComPtr<IDCompositionTranslateTransform> translateTransform;
		ComPtr<IDCompositionRectangleClip> clip;

		#pragma endregion

		#pragma region Fallback Visual

		DXGI_SWAP_CHAIN_DESC1 description = {};
		D2D1_BITMAP_PROPERTIES1 properties = {};
		ComPtr<IDXGISwapChain1> swapChain;
		ComPtr<IDXGISurface2> fallbackSurface;
		ComPtr<ID2D1Bitmap1> fallbackBitmap;
		ComPtr<ID2D1SolidColorBrush> fallbackBrush;
		D2D1_COLOR_F tintColor = D2D1::ColorF(0.0f, 0.0f, 0.0f, .70f);
		D2D1_COLOR_F fallbackColor = D2D1::ColorF(1.0f,1.0f,1.0f,1.0f);
		D2D1_RECT_F fallbackRect = D2D1::RectF(0, 0, (float)GetSystemMetrics(SM_CXSCREEN), (float)GetSystemMetrics(SM_CYSCREEN));

		#pragma endregion

		#pragma region Desktop Backdrop

		HWND desktopWindow;
		RECT desktopWindowRect;
		SIZE thumbnailSize{};
		DWM_THUMBNAIL_PROPERTIES thumbnail;
		HTHUMBNAIL desktopThumbnail = NULL;

		#pragma endregion

		#pragma region Top Level Window Backdrop

		RECT sourceRect{0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN)};
		SIZE destinationSize{ GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN) };
		HTHUMBNAIL topLevelWindowThumbnail = NULL;
		HWND* hwndExclusionList;

		#pragma endregion

		typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
		typedef BOOL(WINAPI* SetWindowCompositionAttribute)(IN HWND hwnd,IN WINDOWCOMPOSITIONATTRIBDATA* pwcad);
		typedef HRESULT(WINAPI* DwmpCreateSharedThumbnailVisual)(IN HWND hwndDestination,IN HWND hwndSource,IN DWORD dwThumbnailFlags,IN DWM_THUMBNAIL_PROPERTIES* pThumbnailProperties,IN VOID* pDCompDevice,OUT VOID** ppVisual,OUT PHTHUMBNAIL phThumbnailId);
		typedef HRESULT(WINAPI* DwmpCreateSharedMultiWindowVisual)(IN HWND hwndDestination,IN VOID* pDCompDevice,OUT VOID** ppVisual,OUT PHTHUMBNAIL phThumbnailId);
		typedef HRESULT(WINAPI* DwmpUpdateSharedMultiWindowVisual)(IN HTHUMBNAIL hThumbnailId,IN HWND* phwndsInclude,IN DWORD chwndsInclude,IN HWND* phwndsExclude,IN DWORD chwndsExclude,OUT RECT* prcSource,OUT SIZE* pDestinationSize,IN DWORD dwFlags);
		typedef HRESULT(WINAPI* DwmpCreateSharedVirtualDesktopVisual)(IN HWND hwndDestination,IN VOID* pDCompDevice,OUT VOID** ppVisual,OUT PHTHUMBNAIL phThumbnailId);
		typedef HRESULT(WINAPI* DwmpUpdateSharedVirtualDesktopVisual)(IN HTHUMBNAIL hThumbnailId,IN HWND* phwndsInclude,IN DWORD chwndsInclude,IN HWND* phwndsExclude,IN DWORD chwndsExclude,OUT RECT* prcSource,OUT SIZE* pDestinationSize);

		DwmpCreateSharedThumbnailVisual DwmCreateSharedThumbnailVisual;
		DwmpCreateSharedMultiWindowVisual DwmCreateSharedMultiWindowVisual;
		DwmpUpdateSharedMultiWindowVisual DwmUpdateSharedMultiWindowVisual;
		DwmpCreateSharedVirtualDesktopVisual DwmCreateSharedVirtualDesktopVisual;
		DwmpUpdateSharedVirtualDesktopVisual DwmUpdateSharedVirtualDesktopVisual;
		SetWindowCompositionAttribute DwmSetWindowCompositionAttribute;
		RtlGetVersionPtr GetVersionInfo;


		HRESULT hr;
		RECT hostWindowRect{};

		bool InitLibs();
		long GetBuildVersion();
		bool CreateCompositionDevice();
		bool CreateFallbackVisual();
		void SyncFallbackVisual(bool active);
		bool CreateCompositionVisual(HWND hwnd);
		bool CreateCompositionTarget(HWND hwnd);
		bool CreateBackdrop(HWND hwnd,BackdropSource source);
		bool CreateEffectGraph(ComPtr<IDCompositionDevice3> dcompDevice3);
		void SyncCoordinates(HWND hwnd);
		bool Flush();
		bool Commit();
};


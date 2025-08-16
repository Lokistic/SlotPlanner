#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <tchar.h>
#include <imgui.h>
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "Style.h"
#include "App.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#ifdef USE_IMPLOT
#include "implot.h"
#endif

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

extern IMGUI_IMPL_API LRESULT   ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Forward decl
static bool CreateDeviceD3D(HWND hWnd);
static void CleanupDeviceD3D();
static void CreateRenderTarget();
static void CleanupRenderTarget();

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("SlotPlannerClass"), NULL };
	RegisterClassEx(&wc);

	HWND hwnd = CreateWindow(wc.lpszClassName, _T("SlotPlanner"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

	if (!CreateDeviceD3D(hwnd)) { CleanupDeviceD3D(); UnregisterClass(wc.lpszClassName, wc.hInstance); return 1; }

	ShowWindow(hwnd, SW_SHOWDEFAULT); UpdateWindow(hwnd);

	IMGUI_CHECKVERSION(); ImGui::CreateContext();
#ifdef USE_IMPLOT
#include "implot.h"
	ImPlot::CreateContext();
#endif

	ApplyPrettyStyle();

	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	SlotPlannerApp app;

	// Main loop
	bool done = false;
	while (!done) {
		MSG msg; while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) { if (msg.message == WM_QUIT) done = true; TranslateMessage(&msg); DispatchMessage(&msg); } if (done) break;

		ImGui_ImplDX11_NewFrame(); ImGui_ImplWin32_NewFrame(); ImGui::NewFrame();

		app.Draw();

		ImGui::Render();
		const float clear_color_with_alpha[4] = { 0.06f,0.06f,0.07f,1.0f };
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		g_pSwapChain->Present(1, 0); // VSync
	}

	// Cleanup
#ifdef USE_IMPLOT
	ImPlot::DestroyContext();
#endif
	ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext();
	CleanupDeviceD3D(); DestroyWindow(hwnd); UnregisterClass(wc.lpszClassName, wc.hInstance); return 0;
}

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return 1;
	switch (msg) {
	case WM_SIZE:
		if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED) { CleanupRenderTarget(); g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0); CreateRenderTarget(); }
		return 0;
	case WM_DESTROY: PostQuitMessage(0); return 0;
	case WM_SYSCOMMAND: if ((wParam & 0xfff0) == SC_KEYMENU) return 0; break; // Disable ALT application menu
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

static bool CreateDeviceD3D(HWND hWnd) {
	DXGI_SWAP_CHAIN_DESC sd{}; sd.BufferCount = 2; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = hWnd; sd.SampleDesc.Count = 1; sd.Windowed = TRUE; sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	UINT createDeviceFlags = 0; D3D_FEATURE_LEVEL featureLevel; static const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
	if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK) return false;
	CreateRenderTarget(); return true;
}
static void CleanupDeviceD3D() { CleanupRenderTarget(); if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; } if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; } if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; } }
static void CreateRenderTarget() { ID3D11Texture2D* pBackBuffer = nullptr; g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer)); g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView); pBackBuffer->Release(); }
static void CleanupRenderTarget() { if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; } }
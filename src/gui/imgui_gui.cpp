// 透明 ImGui DirectX11 範例 - 工作列顯示修正版
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <windows.h>
#include <locale>
#include <codecvt>
#include <commdlg.h>
#include <filesystem>
#include <sstream>
#include <filesystem>

#include "../utils/PEChecker.h"
#include "../packer/pack.h"

// Data
static ID3D11Device *g_pd3dDevice = nullptr;
static ID3D11DeviceContext *g_pd3dDeviceContext = nullptr;
static IDXGISwapChain *g_pSwapChain = nullptr;
static bool g_SwapChainOccluded = false;
static UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView *g_mainRenderTargetView = nullptr;
static HWND g_hWnd = nullptr;
static std::string selected_path_utf8;

// Z-Order 管理相關
static bool g_EnableZOrderManagement = true;
static HWND g_LastForegroundWindow = nullptr;
static DWORD g_LastZOrderUpdateTime = 0;

// Forward declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
void UpdateWindowZOrder();
bool IsDesktopWindow(HWND hwnd);
bool IsValidTargetWindow(HWND hwnd);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void ShowPEFilePickerUI();
std::string WideToUtf8(const std::wstring &wstr);
std::string AddPackedToFilename(const std::string &path);

// Main code
int main(int, char **)
{
    // Create transparent application window
    WNDCLASSEXW wc = {
        sizeof(wc),
        CS_CLASSDC,
        WndProc,
        0L,
        0L,
        GetModuleHandle(nullptr),
        LoadIcon(nullptr, IDI_APPLICATION), // 添加圖示
        LoadCursor(nullptr, IDC_ARROW),     // 添加游標
        nullptr,
        nullptr,
        L"TransparentImGui",
        LoadIcon(nullptr, IDI_APPLICATION) // 小圖示
    };
    ::RegisterClassExW(&wc);

    // 創建透明視窗 - 保持原始的透明疊加效果，但確保在工作列顯示
    g_hWnd = ::CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_APPWINDOW, // 添加 WS_EX_APPWINDOW
        wc.lpszClassName,
        L"Transparent ImGui Example",
        WS_POPUP | WS_VISIBLE, // 保持無邊框，但確保可見
        0, 0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr);

    // 設定視窗透明度
    SetLayeredWindowAttributes(g_hWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

    // Initialize Direct3D
    if (!CreateDeviceD3D(g_hWnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(g_hWnd, SW_SHOWDEFAULT);
    ::UpdateWindow(g_hWnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // Setup Dear ImGui style for transparency
    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 0.7f); // 深灰 + 半透明背景
    style.WindowRounding = 8.0f;                                      // 圓角
    style.FrameRounding = 4.0f;
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f); // Child 視窗透明

    /*
    style.WindowRounding = 8.0f;
    style.WindowBorderSize = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 0.8f;
    style.Colors[ImGuiCol_ChildBg].w = 0.0f;
    */

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(g_hWnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Our state
    bool show_demo_window = false;
    bool show_control_window = true;
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    // Main loop
    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window being minimized or screen locked
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Handle window resize
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 更新 Z-Order 管理
        if (g_EnableZOrderManagement)
        {
            UpdateWindowZOrder();
        }

        // 緊急置頂熱鍵 (F1)
        if (GetAsyncKeyState(VK_F1) & 0x8000)
        {
            SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            SetForegroundWindow(g_hWnd);
        }

        // Main control panel
        if (show_control_window)
        {
            ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(350, 250), ImGuiCond_FirstUseEver);

            ImGui::Begin("Transparent Control Panel", &show_control_window);
            ImGui::Text("Press F1 to bring window to top");
            ImGui::Separator();
            ShowPEFilePickerUI();
            ImGui::Separator();
            std::string packed_filename = AddPackedToFilename(selected_path_utf8);
            ImGui::Text("Packed file will be saved as: %s", packed_filename.empty() ? "No file selected" : packed_filename.c_str());

            if (ImGui::Button("Pack File"))
            {
                pack(selected_path_utf8, packed_filename);
            }

            if (ImGui::Button("Bring to Front"))
            {
                SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                             SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                SetForegroundWindow(g_hWnd);
            }

            if (ImGui::Button("Exit Program"))
                done = true;

            ImGui::End();
        }
        else
        {
            done = true;
        }

        // Demo window
        if (show_demo_window)
        {
            ImGui::SetNextWindowPos(ImVec2(400, 100), ImGuiCond_FirstUseEver);
            ImGui::ShowDemoWindow(&show_demo_window);
        }

        // Rendering
        ImGui::Render();

        const float clear_color_with_alpha[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        HRESULT hr = g_pSwapChain->Present(1, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(g_hWnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// 檢查是否為桌面視窗
bool IsDesktopWindow(HWND hwnd)
{
    if (!hwnd)
        return true;

    char className[256];
    GetClassNameA(hwnd, className, sizeof(className));

    // 桌面和工作管理員相關的類名
    return (strcmp(className, "Progman") == 0 ||      // 桌面
            strcmp(className, "WorkerW") == 0 ||      // 桌面工作區
            strcmp(className, "Shell_TrayWnd") == 0); // 工作管理員
}

// 檢查是否為有效的目標視窗
bool IsValidTargetWindow(HWND hwnd)
{
    if (!hwnd || hwnd == g_hWnd)
        return false;
    if (IsDesktopWindow(hwnd))
        return false;

    // 檢查視窗是否可見
    if (!IsWindowVisible(hwnd))
        return false;

    // 檢查視窗是否有標題列（排除一些系統視窗）
    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    if (!(style & WS_CAPTION) && !(style & WS_POPUP))
        return false;

    // 檢查視窗大小（排除太小的視窗）
    RECT rect;
    if (GetWindowRect(hwnd, &rect))
    {
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        if (width < 100 || height < 50)
            return false;
    }

    return true;
}

// 更新視窗 Z-Order
void UpdateWindowZOrder()
{
    DWORD currentTime = GetTickCount();

    // 限制更新頻率
    if (currentTime - g_LastZOrderUpdateTime < 200) // 每200ms檢查一次
        return;

    g_LastZOrderUpdateTime = currentTime;

    HWND foregroundWindow = GetForegroundWindow();

    // 如果前景視窗沒有改變，不需要處理
    if (foregroundWindow == g_LastForegroundWindow)
        return;

    g_LastForegroundWindow = foregroundWindow;

    // 如果前景視窗是我們自己的視窗，不需要處理
    if (foregroundWindow == g_hWnd)
        return;

    // 如果前景視窗是桌面，保持在最上層
    if (IsDesktopWindow(foregroundWindow))
    {
        SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        return;
    }

    // 如果是有效的應用視窗，將我們的視窗放到它後面
    if (IsValidTargetWindow(foregroundWindow))
    {
        // 將我們的視窗放到前景視窗的後面
        SetWindowPos(g_hWnd, foregroundWindow, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

        // 確保前景視窗保持在前面
        SetWindowPos(foregroundWindow, HWND_TOP, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
}

// Helper functions
bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain)
    {
        g_pSwapChain->Release();
        g_pSwapChain = nullptr;
    }
    if (g_pd3dDeviceContext)
    {
        g_pd3dDeviceContext->Release();
        g_pd3dDeviceContext = nullptr;
    }
    if (g_pd3dDevice)
    {
        g_pd3dDevice->Release();
        g_pd3dDevice = nullptr;
    }
}

void CreateRenderTarget()
{
    ID3D11Texture2D *pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView)
    {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

std::string WideToUtf8(const std::wstring &wstr)
{
    if (wstr.empty())
        return "";

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, nullptr, nullptr);
    return strTo;
};

void ShowPEFilePickerUI()
{
    // 占滿整個寬度的按鈕
    ImVec2 full_width(ImGui::GetContentRegionAvail().x, 50.0f);
    if (ImGui::Button("Select PE file", full_width))
    {
        // 使用 Windows API 打開檔案選擇對話框

        // 使用 wchar_t 來處理 Unicode 字符串
        // 注意：這裡使用了 GetOpenFileNameW 來打開檔案選擇對話框
        wchar_t filename[MAX_PATH] = {0};

        OPENFILENAMEW ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = g_hWnd; // 你的主視窗 HWND
                                //  ofn.lpstrFilter = L"All file (*.*)\0*.*\0\0";
        ofn.lpstrFilter = L"Executable Files (*.exe)\0*.exe\0All Files (*.*)\0*.*\0\0";
        ofn.lpstrFile = filename;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

        if (GetOpenFileNameW(&ofn))
        {
            // 將 wchar_t* 轉成 std::string (UTF-8)
            std::wstring wpath(filename);
            selected_path_utf8 = WideToUtf8(wpath);
        }
        if (!IsValidPEFile(selected_path_utf8))
        {
            MessageBoxA(g_hWnd, "The selected file is not a valid PE file.", "Error", MB_OK | MB_ICONERROR);
        }
    }

    if (!selected_path_utf8.empty())
    {
        ImGui::Text("Already selected file:");
        ImGui::TextWrapped("%s", selected_path_utf8.c_str());
    }
}

std::string AddPackedToFilename(const std::string &path)
{
    std::filesystem::path p(path);
    std::string stem = p.stem().string() + "_packed";
    std::filesystem::path new_path = p.parent_path() / (stem + p.extension().string());
    return new_path.string();
}
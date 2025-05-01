#include <winrt/Microsoft.ReactNative.h>
#include <ReactPropertyBag.h>

winrt::Microsoft::ReactNative::ReactPropertyId<winrt::Microsoft::ReactNative::ReactNonAbiValue<HWND>> SystrayHwndPropertyId() noexcept;
void InitSysTray(HINSTANCE hInstance, const winrt::Microsoft::ReactNative::ReactNativeHost& host) noexcept;
void ShutdownSysTray(const winrt::Microsoft::ReactNative::ReactContext& reactContext) noexcept;

void ShowSysTrayWindow(const winrt::Microsoft::ReactNative::ReactContext& reactContext) noexcept;
void DismissSysTrayWindow() noexcept;
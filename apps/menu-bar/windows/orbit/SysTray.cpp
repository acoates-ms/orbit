#include "pch.h"
#include <winrt/Microsoft.UI.Composition.SystemBackdrops.h>
#include <winrt/Microsoft.UI.Input.h>
#include <winrt/Microsoft.UI.System.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.ViewManagement.h>
#include <nativeModules.h>
#include "shellapi.h"
#include <dwmapi.h>
#include "wchar.h"
#include "resource.h"

constexpr PCWSTR appName = L"Expo Orbit";
constexpr PCWSTR c_mainWindowClassName = L"EXPO_ORBIT_SYSTRAY_WINDOW_CLASS";
constexpr auto SysTrayWindowDataProperty = L"SysTrayWindowDataProperty";
UINT const WMAPP_NOTIFYCALLBACK = WM_APP + 1;

REACT_STRUCT(SystrayMenuProps)
struct SystrayMenuProps
{
	// noRootFlex is used in AppContainer to remove the flex property on the root view
	// to allow the container to size to content.
	REACT_FIELD(noRootFlex)
	bool noRootFlex { true };
};

struct SysTrayWindowData
{
	winrt::Microsoft::ReactNative::ReactNativeHost m_host { nullptr };
	HINSTANCE m_hInstance { nullptr };

	SysTrayWindowData() {}

	static SysTrayWindowData* GetFromWindow(HWND hwnd)
	{
		auto data = reinterpret_cast<SysTrayWindowData*>(GetProp(hwnd, SysTrayWindowDataProperty));
		return data;
	}
};


struct SysTrayMenu : winrt::implements<SysTrayMenu, winrt::Windows::Foundation::IInspectable>
{
	void ShowPopup(POINT pt, const winrt::Microsoft::ReactNative::ReactNativeHost& host)
	{
		m_pt = pt;

		if (!m_started)
		{
			m_started = true;
			m_properties = host.InstanceSettings().Properties();
			m_compositor = winrt::Microsoft::ReactNative::Composition::CompositionUIService::GetCompositor(m_properties);

			m_reactNativeIsland = winrt::Microsoft::ReactNative::ReactNativeIsland(m_compositor);
			m_reactNativeIsland.SizeChanged({ get_weak(), &SysTrayMenu::OnIslandSizeChanged });

			winrt::Microsoft::ReactNative::ReactViewOptions viewOptions;
			viewOptions.ComponentName(L"main");
			viewOptions.InitialProps(winrt::Microsoft::ReactNative::MakeJSValueWriter(m_props));
			m_reactNativeIsland.ReactViewHost(winrt::Microsoft::ReactNative::ReactCoreInjection::MakeViewHost(host, viewOptions));

			winrt::Microsoft::ReactNative::LayoutConstraints constraints;
			constraints.LayoutDirection = winrt::Microsoft::ReactNative::LayoutDirection::Undefined;
			constraints.MinimumSize = { 0, 0 };
			constraints.MaximumSize = { 1000, 1000 };
			m_reactNativeIsland.Arrange(constraints, { 0, 0 });
		}

		if (m_appWindow)
		{
			UpdateWindowPositionAfterCommit();
		}
	}

	void Shutdown()
	{
		if (m_bridge)
			m_bridge.Close();
		m_bridge = nullptr;

		m_compositor = nullptr;

		if (m_themeSettings)
		{
			m_themeSettings.Changed(m_themeSettingChangedEventToken);
			m_themeSettingChangedEventToken = winrt::event_token {};
			m_themeSettings = nullptr;
		}

		if (m_appWindow)
			m_appWindow.Destroy();
		m_appWindow = nullptr;
		m_reactNativeIsland = nullptr;
		if (m_backdropController)
			m_backdropController.Close();
		m_backdropController = nullptr;
		m_configuration = nullptr;
		m_uiSettings = nullptr;
	}

	void OnLightDismissDismissed(const winrt::Microsoft::UI::Input::InputLightDismissAction&, const winrt::Microsoft::UI::Input::InputLightDismissEventArgs&)
	{
		Dismiss();
	}

	void Dismiss() noexcept
	{
		m_appWindow.Hide();
	}

	void OnIslandSizeChanged(winrt::Windows::Foundation::IInspectable const& /*sender*/, const winrt::Microsoft::ReactNative::RootViewSizeChangedEventArgs& /*args*/)
	{
		if (!m_appWindow)
		{
			auto presenter = winrt::Microsoft::UI::Windowing::OverlappedPresenter::CreateForContextMenu();
			m_appWindow = winrt::Microsoft::UI::Windowing::AppWindow::Create(presenter);
			m_appWindow.IsShownInSwitchers(false);

            m_bridge = winrt::Microsoft::UI::Content::DesktopChildSiteBridge::Create(m_compositor, m_appWindow.Id());
			auto appContent = m_reactNativeIsland.Island();

			m_themeSettings = winrt::Microsoft::UI::System::ThemeSettings::CreateForWindowId(m_appWindow.Id());
			m_themeSettingChangedEventToken = m_themeSettings.Changed(
				[wkConfig = winrt::weak_ref(m_configuration)](
					const winrt::Microsoft::UI::System::ThemeSettings& themeSettings, const winrt::Windows::Foundation::IInspectable&)
			{
				if (auto config = wkConfig.get())
				{
					config.IsHighContrast(themeSettings.HighContrast());
				}
			});

			SetupSystemBackdropConfiguration();
			m_bridge.ResizePolicy(winrt::Microsoft::UI::Content::ContentSizePolicy::ResizeContentToParentWindow);
			m_bridge.Connect(appContent);
			m_bridge.Show();

			auto inputLightDismissAction = winrt::Microsoft::UI::Input::InputLightDismissAction::GetForWindowId(m_appWindow.Id());
			inputLightDismissAction.Dismissed({ get_weak(), &SysTrayMenu::OnLightDismissDismissed });
		}

		UpdateWindowPositionAfterCommit();
	}

	void UpdateWindowPositionAfterCommit()
	{
		auto compositor = winrt::Microsoft::ReactNative::Composition::CompositionUIService::GetCompositor(m_properties);
		auto async = compositor.RequestCommitAsync();
		async.Completed([wkThis = get_weak()](auto, winrt::Windows::Foundation::AsyncStatus /*asyncStatus*/)
		{
			if (auto pThis = wkThis.get())
			{
				if (!pThis->m_appWindow)
					return;
				pThis->UpdateWindowPosition();
				pThis->m_appWindow.Show(true);
			}
		});
	}

	void UpdateWindowPosition()
	{
		auto rect = GetDesiredWindowRectForDpi(m_reactNativeIsland.Size());
		if (rect.Width != 0 && rect.Height != 0)
		{
			auto hwnd = winrt::Microsoft::UI::GetWindowFromWindowId(m_appWindow.Id());
			RECT rc { rect.X, rect.Y, rect.X + rect.Width, rect.Y + rect.Height };
			AdjustWindowRectEx(&rc, GetWindowLong(hwnd, GWL_STYLE), false, GetWindowLong(hwnd, GWL_EXSTYLE));

			m_appWindow.MoveAndResize({ rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top });
			SetWindowPos(winrt::Microsoft::UI::GetWindowFromWindowId(m_appWindow.Id()), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}
	}

	static bool IsColorLight(const winrt::Windows::UI::Color& clr) noexcept
	{
		return (((5 * clr.G) + (2 * clr.R) + clr.B) > (8 * 128));
	}

	void SetupSystemBackdropConfiguration()
	{
		m_configuration = winrt::Microsoft::UI::Composition::SystemBackdrops::SystemBackdropConfiguration();

		auto island = m_reactNativeIsland.Island();

		// Initial state.
		m_configuration.IsInputActive(true);
		m_configuration.Theme(
			IsColorLight(m_uiSettings.GetColorValue(winrt::Windows::UI::ViewManagement::UIColorType::Foreground))
				? winrt::Microsoft::UI::Composition::SystemBackdrops::SystemBackdropTheme::Dark
				: winrt::Microsoft::UI::Composition::SystemBackdrops::SystemBackdropTheme::Light);

		m_configuration.IsHighContrast(m_themeSettings.HighContrast());

		m_backdropController = winrt::Microsoft::UI::Composition::SystemBackdrops::MicaController();
		m_backdropController.SetSystemBackdropConfiguration(m_configuration);
		m_backdropController.AddSystemBackdropTarget(island.as<winrt::Microsoft::UI::Composition::ICompositionSupportsSystemBackdrop>());

		m_uiSettingsColorChangedEventToken = m_uiSettings.ColorValuesChanged(
			[wkConfig = winrt::weak_ref(m_configuration)](
				const winrt::Windows::UI::ViewManagement::UISettings& uiSettings, const winrt::Windows::Foundation::IInspectable&)
		{
			if (auto config = wkConfig.get())
			{
				config.Theme(
					IsColorLight(uiSettings.GetColorValue(winrt::Windows::UI::ViewManagement::UIColorType::Foreground))
						? winrt::Microsoft::UI::Composition::SystemBackdrops::SystemBackdropTheme::Dark
						: winrt::Microsoft::UI::Composition::SystemBackdrops::SystemBackdropTheme::Light);
			}
		});
	}

	winrt::Windows::Graphics::RectInt32 GetDesiredWindowRectForDpi(winrt::Windows::Foundation::Size size)
	{
		UINT dpi = GetDpiForWindow(winrt::Microsoft::UI::GetWindowFromWindowId(m_appWindow.Id()));
		int desiredWidth = static_cast<int>(size.Width);
		int desiredHeight = static_cast<int>(size.Height);

		int scaledWidth = MulDiv(desiredWidth, dpi, 96);
		int scaledHeight = MulDiv(desiredHeight, dpi, 96);

		winrt::Windows::Graphics::RectInt32 desiredRect {};
		int left = m_pt.x;
		int top = m_pt.y - scaledHeight - 10;

		auto displayArea = winrt::Microsoft::UI::Windowing::DisplayArea::GetFromWindowId(
			m_appWindow.Id(), winrt::Microsoft::UI::Windowing::DisplayAreaFallback::Primary);
		if (displayArea)
		{
			auto workArea = displayArea.WorkArea();
			left = std::min(left, static_cast<int>(workArea.X + workArea.Width - scaledWidth - 10));
			top = std::min(top, static_cast<int>(workArea.Y + workArea.Height - scaledHeight - 10));
		}

		desiredRect.X = left;
		desiredRect.Y = top;
		desiredRect.Width = scaledWidth;
		desiredRect.Height = scaledHeight;

		return desiredRect;
	}

	static SysTrayMenu& Instance() noexcept
	{
		if (!s_instance)
		{
			s_instance = winrt::make_self<SysTrayMenu>();
		}
		return *s_instance.get();
	}

    SystrayMenuProps m_props;

	static winrt::com_ptr<SysTrayMenu> s_instance;
	bool m_started { false };
	winrt::Microsoft::UI::Composition::Compositor m_compositor { nullptr };
	winrt::Microsoft::UI::Windowing::AppWindow m_appWindow { nullptr };
	winrt::Microsoft::ReactNative::IReactPropertyBag m_properties { nullptr };
	winrt::Microsoft::ReactNative::ReactNativeIsland m_reactNativeIsland { nullptr };
	winrt::Microsoft::UI::Content::DesktopChildSiteBridge m_bridge { nullptr };
	winrt::Microsoft::UI::Composition::SystemBackdrops::SystemBackdropConfiguration m_configuration { nullptr };
	winrt::Microsoft::UI::Composition::SystemBackdrops::MicaController m_backdropController { nullptr };
	winrt::Windows::UI::ViewManagement::UISettings m_uiSettings;
	winrt::Microsoft::UI::System::ThemeSettings m_themeSettings { nullptr };
	winrt::event_token m_uiSettingsColorChangedEventToken;
	winrt::event_token m_themeSettingChangedEventToken;

	POINT m_pt;
};
winrt::com_ptr<SysTrayMenu> SysTrayMenu::s_instance { nullptr };

GUID NotificationGuid()
{
	static const GUID g = { 0x092AAEED, 0x45AB, 0x87DA, { 0xB6, 0xD7, 0x11, 0x4B, 0x44, 0x77, 0xA3, 0xD8 } };
	return g;
}

BOOL AddNotificationIcon(HWND hwnd, HINSTANCE hInstance)
{
	NOTIFYICONDATA nid;
	nid.cbSize = sizeof(nid);
	nid.hWnd = hwnd;
	nid.uCallbackMessage = WMAPP_NOTIFYCALLBACK;
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_GUID | NIF_SHOWTIP | NIF_MESSAGE | NIIF_USER;
	nid.guidItem = NotificationGuid();

	wcscpy_s(nid.szTip, ARRAYSIZE(nid.szTip), L"Expo Orbit");

	winrt::Windows::UI::ViewManagement::UISettings uiSettings;
	nid.hIcon = (HICON)LoadImage(
		hInstance,
        MAKEINTRESOURCEW(
			SysTrayMenu::IsColorLight(uiSettings.GetColorValue(winrt::Windows::UI::ViewManagement::UIColorType::Foreground))
				? IDI_SYSTRAY_DARK_ICON
				: IDI_SYSTRAY_ICON),
		IMAGE_ICON,
		32,
		32,
		LR_DEFAULTCOLOR);

	Shell_NotifyIcon(NIM_ADD, &nid);

	nid.uVersion = NOTIFYICON_VERSION_4;
	return Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

void UpdateNotifictionIcon(HWND hwnd, HINSTANCE hInstance)
{
	NOTIFYICONDATA nid;
	nid.cbSize = sizeof(nid);
	nid.hWnd = hwnd;
	nid.uFlags = NIF_ICON | NIF_GUID;
	nid.guidItem = NotificationGuid();
	nid.uVersion = NOTIFYICON_VERSION_4;

	winrt::Windows::UI::ViewManagement::UISettings uiSettings;
	nid.hIcon = (HICON)LoadImage(
		hInstance,
        MAKEINTRESOURCEW(
			SysTrayMenu::IsColorLight(uiSettings.GetColorValue(winrt::Windows::UI::ViewManagement::UIColorType::Foreground))
				? IDI_SYSTRAY_DARK_ICON
				: IDI_SYSTRAY_ICON),
		IMAGE_ICON,
		32,
		32,
		LR_DEFAULTCOLOR);

	Shell_NotifyIcon(NIM_MODIFY, &nid);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) noexcept
{
	switch (message)
	{
		case WM_NCCREATE:
		{
			auto cs = reinterpret_cast<CREATESTRUCT*>(lparam);
			auto windowData = static_cast<SysTrayWindowData*>(cs->lpCreateParams);
			WINRT_ASSERT(windowData);
			SetProp(hwnd, SysTrayWindowDataProperty, reinterpret_cast<HANDLE>(windowData));
			break;
		}
		case WM_CREATE:
		{
			auto data = SysTrayWindowData::GetFromWindow(hwnd);

			AddNotificationIcon(hwnd, data->m_hInstance);
			break;
		}
		case WM_SETTINGCHANGE:
		{
			auto data = SysTrayWindowData::GetFromWindow(hwnd);
			UpdateNotifictionIcon(hwnd, data->m_hInstance);
			break;
		}
		case WM_DESTROY:
		{
			// Before we shutdown the application - gracefully unload the ReactNativeHost instance
			auto data = SysTrayWindowData::GetFromWindow(hwnd);
			if (data->m_host)
			{
				auto async = data->m_host.UnloadInstance();
				async.Completed([host = data->m_host](auto /*asyncInfo*/, winrt::Windows::Foundation::AsyncStatus /*asyncStatus*/)
				{
					//Assert(asyncStatus == winrt::Windows::Foundation::AsyncStatus::Completed);
					host.InstanceSettings().UIDispatcher().Post([]()
					{
						PostQuitMessage(0);
					});
				});
			}
			return 0;
		}

		case WM_CLOSE:
		{
			ShowWindow(hwnd, SW_HIDE);
			return 0;
		}

		case WMAPP_NOTIFYCALLBACK:
			switch (LOWORD(lparam))
			{
				case NIN_SELECT:
				{
					// for NOTIFYICON_VERSION_4 clients, NIN_SELECT is preferable to listening to mouse clicks and key presses
					// directly.
					auto data = SysTrayWindowData::GetFromWindow(hwnd);
					SysTrayMenu::Instance().ShowPopup({ LOWORD(wparam), HIWORD(wparam) }, data->m_host);
				}
				break;
				case WM_CONTEXTMENU:
				{
					auto data = SysTrayWindowData::GetFromWindow(hwnd);
					SysTrayMenu::Instance().ShowPopup({ LOWORD(wparam), HIWORD(wparam) }, data->m_host);
				}
				break;
			}
			break;
	}
	return DefWindowProc(hwnd, message, wparam, lparam);
}

winrt::Microsoft::ReactNative::ReactPropertyId<winrt::Microsoft::ReactNative::ReactNonAbiValue<HWND>> SystrayHwndPropertyId() noexcept
{
	static const winrt::Microsoft::ReactNative::ReactPropertyId<winrt::Microsoft::ReactNative::ReactNonAbiValue<HWND>> prop {
		L"Expo", L"SystrayHwnd"
	};
	return prop;
}

void InitSysTray(HINSTANCE hInstance, const winrt::Microsoft::ReactNative::ReactNativeHost& host) noexcept
{
	WNDCLASSEXW wcex;
	ZeroMemory(&wcex, sizeof(WNDCLASSEXW));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = &WndProc;
	wcex.cbClsExtra = DLGWINDOWEXTRA;
	wcex.cbWndExtra = sizeof(SysTrayWindowData*);
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = c_mainWindowClassName;
	wcex.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_ICON1));
	ATOM classId = RegisterClassEx(&wcex);
	WINRT_VERIFY(classId);
	winrt::check_win32(!classId);

	auto windowData = std::make_unique<SysTrayWindowData>();
	windowData->m_host = host;
	windowData->m_hInstance = hInstance;
	auto hwnd = CreateWindowEx(
		WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
		c_mainWindowClassName,
		appName,
		WS_OVERLAPPEDWINDOW,
		0,
		0,
		0,
		0,
		nullptr,
		nullptr,
		hInstance,
		windowData.get());

	winrt::Microsoft::ReactNative::ReactPropertyBag(host.InstanceSettings().Properties()).Set(SystrayHwndPropertyId(), hwnd);

	winrt::Microsoft::ReactNative::ReactCoreInjection::SetTopLevelWindowId(
		host.InstanceSettings().Properties(), reinterpret_cast<UINT_PTR>(hwnd));

	windowData.release();
}

void ShutdownSysTray(const winrt::Microsoft::ReactNative::ReactContext& reactContext) noexcept
{
	SysTrayMenu::Instance().Shutdown();
	auto optHwnd = reactContext.Properties().Get(SystrayHwndPropertyId());
	if (optHwnd)
	{
		DestroyWindow(*optHwnd);
	}
}

void ShowSysTrayWindow(const winrt::Microsoft::ReactNative::ReactContext& reactContext) noexcept {
	auto host = winrt::Microsoft::ReactNative::ReactNativeHost::FromContext(reactContext.Handle());
	SysTrayMenu::Instance().ShowPopup({ 10000,10000 }, host);
}

void DismissSysTrayWindow() noexcept {
	SysTrayMenu::Instance().Dismiss();
}
// orbit.cpp : Defines the entry point for the application.
//

#include "pch.h"
#include "orbit.h"

#include <jsi/jsi.h>
#include <JSI/JsiApiContext.h>

#include "AutolinkedNativeModules.g.h"

#include "commctrl.h"
#include "shellapi.h"
#include "NativeModules.h"
#include "strsafe.h"
#include "windowsx.h"

#include <winrt/Microsoft.UI.interop.h>
#include <winrt/Windows.ApplicationModel.DataTransfer.h>
#include <winrt/Windows.Foundation.h>

REACT_MODULE(RNCClipboard)
struct RNCClipboard
{
	REACT_INIT(Initialize);
	void Initialize(const winrt::Microsoft::ReactNative::ReactContext& reactContext) noexcept
	{
		m_context = reactContext;
	}

	REACT_METHOD(getString);
	void getString(React::ReactPromise<std::string>&& result) noexcept
	{
		auto dataPackageView = winrt::Windows::ApplicationModel::DataTransfer::Clipboard::GetContent();
		if (dataPackageView.Contains(winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats::Text()))
		{
			dataPackageView.GetTextAsync().Completed([result, dataPackageView](winrt::Windows::Foundation::IAsyncOperation<winrt::hstring> info, winrt::Windows::Foundation::AsyncStatus status)
			{
				if (status == winrt::Windows::Foundation::AsyncStatus::Completed)
				{
					auto text = winrt::to_string(info.GetResults());
					result.Resolve(text);
				}
				else
				{
					result.Reject("Failure");
				}
			});
			return;
		}
		result.Resolve("");
	}

	REACT_METHOD(setString)
		void setString(std::string const& str) noexcept
	{
		m_context.UIDispatcher().Post([str]()
		{
			winrt::Windows::ApplicationModel::DataTransfer::DataPackage dataPackage {};
			dataPackage.SetText(winrt::to_hstring(str));
			winrt::Windows::ApplicationModel::DataTransfer::Clipboard::SetContent(dataPackage);
		});
	}

	REACT_METHOD(addListener)
		void addListener(std::string const&) noexcept
	{
	}

	REACT_METHOD(removeListeners)
		void removeListeners(int) noexcept
	{
	}

private:
	winrt::Microsoft::ReactNative::ReactContext m_context;
};

REACT_STRUCT(WindowsManagerConstants)
struct WindowsManagerConstants
{
	REACT_FIELD(STYLE_MASK_BORDERLESS)
		uint32_t STYLE_MASK_BORDERLESS { 0 };

	/*
	case WindowStyleMask.Titled:
		return WindowsManagerConstants.STYLE_MASK_TITLED;
	case WindowStyleMask.Closable:
		return WindowsManagerConstants.STYLE_MASK_CLOSABLE;
	case WindowStyleMask.Miniaturizable:
		return WindowsManagerConstants.STYLE_MASK_MINIATURIZABLE;
	case WindowStyleMask.Resizable:
		return WindowsManagerConstants.STYLE_MASK_RESIZABLE;
	case WindowStyleMask.UnifiedTitleAndToolbar:
		return WindowsManagerConstants.STYLE_MASK_UNIFIED_TITLE_AND_TOOLBAR;
	case WindowStyleMask.FullScreen:
		return WindowsManagerConstants.STYLE_MASK_FULL_SCREEN;
	case WindowStyleMask.FullSizeContentView:
		return WindowsManagerConstants.STYLE_MASK_FULL_SIZE_CONTENT_VIEW;
	case WindowStyleMask.UtilityWindow:
		return WindowsManagerConstants.STYLE_MASK_UTILITY_WINDOW;
	case WindowStyleMask.DocModalWindow:
		return WindowsManagerConstants.STYLE_MASK_DOC_MODAL_WINDOW;
	case WindowStyleMask.NonactivatingPanel:
		return WindowsManagerConstants.STYLE_MASK_NONACTIVATING_PANEL;
	default:
		return WindowsManagerConstants.STYLE_MASK_BORDERLESS;
		*/

};

constexpr PCWSTR appName = L"Expo Orbit";
constexpr auto WindowDataProperty = L"WindowData";
constexpr PCWSTR c_mainWindowClassName = L"EXPO_ORBIT_MAIN_WINDOW_CLASS";
constexpr PCWSTR c_popupClassName = L"EXPO_ORBIT_POPUP_CLASS";

struct WindowData
{
	static HINSTANCE s_instance;

	winrt::Microsoft::ReactNative::ReactNativeIsland m_compRootView { nullptr };
	winrt::Microsoft::UI::Content::DesktopChildSiteBridge m_bridge { nullptr };
	winrt::Microsoft::ReactNative::ReactInstanceSettings m_instanceSettings { nullptr };
	LONG m_height { 0 };
	LONG m_width { 0 };
	winrt::hstring m_componentName;
	winrt::event_token m_firstSizeToken;

	WindowData()
	{
	}

	static WindowData* GetFromWindow(HWND hwnd)
	{
		auto data = reinterpret_cast<WindowData*>(GetProp(hwnd, WindowDataProperty));
		return data;
	}
};

REACT_MODULE(WindowsManager)
struct WindowsManager
{
	REACT_INIT(Initialize);
	void Initialize(const winrt::Microsoft::ReactNative::ReactContext& reactContext) noexcept
	{
		m_context = reactContext;
	}

	REACT_GET_CONSTANTS(getConstants)
		WindowsManagerConstants getConstants() noexcept
	{
		return {};
	}

	REACT_METHOD(openWindow)
		void openWindow(const std::string& window, winrt::Microsoft::ReactNative::JSValueObject& args) noexcept
	{
		m_context.UIDispatcher().Post([window]()
		{
			auto windowData = std::make_unique<WindowData>();
			windowData->m_componentName = winrt::to_hstring(window);
			HWND hwnd = CreateWindow(
				c_popupClassName,
				appName,
				WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				nullptr,
				nullptr,
				WindowData::s_instance,
				windowData.get());

			WINRT_VERIFY(hwnd);

			// So not show the window until it has had a chance to get an intial size
			windowData->m_firstSizeToken = windowData->m_compRootView.SizeChanged(
				[hwnd](auto sender, const winrt::Microsoft::ReactNative::RootViewSizeChangedEventArgs& args)
			{
				auto data = WindowData::GetFromWindow(hwnd);
				ShowWindow(hwnd, SW_SHOW);
				UpdateWindow(hwnd);
				SetFocus(hwnd);
				data->m_compRootView.SizeChanged(data->m_firstSizeToken);
			});

			windowData.release();

		});
	}

	REACT_METHOD(closeWindow)
		void closeWindow(const std::string& window) noexcept
	{
		assert(false);
	}
	// TODO add more methods

private:
	winrt::Microsoft::ReactNative::ReactContext m_context;
};

struct CompReactPackageProvider
	: winrt::implements<CompReactPackageProvider, winrt::Microsoft::ReactNative::IReactPackageProvider>
{
public: // IReactPackageProvider
	void CreatePackage(winrt::Microsoft::ReactNative::IReactPackageBuilder const& packageBuilder) noexcept
	{
		AddAttributedModules(packageBuilder, true);
	}
};

// Global Variables:
constexpr PCWSTR windowTitle = L"Orbit";
constexpr PCWSTR mainComponentName = L"Settings";

float ScaleFactor(HWND hwnd) noexcept
{
	return GetDpiForWindow(hwnd) / static_cast<float>(USER_DEFAULT_SCREEN_DPI);
}

void UpdateRootViewSizeToAppWindow(
	winrt::Microsoft::ReactNative::ReactNativeIsland const& rootView,
	winrt::Microsoft::UI::Windowing::AppWindow const& window)
{
	auto hwnd = winrt::Microsoft::UI::GetWindowFromWindowId(window.Id());
	auto scaleFactor = ScaleFactor(hwnd);
	winrt::Windows::Foundation::Size size {
		window.ClientSize().Width / scaleFactor, window.ClientSize().Height / scaleFactor };
	// Do not relayout when minimized
	if (window.Presenter().as<winrt::Microsoft::UI::Windowing::OverlappedPresenter>().State() !=
		winrt::Microsoft::UI::Windowing::OverlappedPresenterState::Minimized)
	{
		winrt::Microsoft::ReactNative::LayoutConstraints constraints;
		constraints.MaximumSize = constraints.MinimumSize = size;
		rootView.Arrange(constraints, { 0,0 });
	}
}

class ExpoModulesHostObject : public facebook::jsi::HostObject
{
	facebook::jsi::Value get(facebook::jsi::Runtime&, const facebook::jsi::PropNameID& name) override
	{
		return facebook::jsi::Value::undefined();
	}

	std::vector<facebook::jsi::PropNameID> getPropertyNames(facebook::jsi::Runtime& rt) override
	{
		std::vector<facebook::jsi::PropNameID> result;
		return result;
	}
};

// Create and configure the ReactNativeHost
winrt::Microsoft::ReactNative::ReactNativeHost CreateReactNativeHost(
	const winrt::Microsoft::UI::Composition::Compositor& compositor)
{
	WCHAR appDirectory[MAX_PATH];
	GetModuleFileNameW(NULL, appDirectory, MAX_PATH);
	PathCchRemoveFileSpec(appDirectory, MAX_PATH);

	auto host = winrt::Microsoft::ReactNative::ReactNativeHost();

	// Include any autolinked modules
	RegisterAutolinkedNativeModulePackages(host.PackageProviders());

	host.PackageProviders().Append(winrt::make<CompReactPackageProvider>());

#if BUNDLE
	host.InstanceSettings().JavaScriptBundleFile(L"index.windows");
	host.InstanceSettings().BundleRootPath(std::wstring(L"file://").append(appDirectory).append(L"\\Bundle\\").c_str());
	host.InstanceSettings().UseFastRefresh(false);
#else
	host.InstanceSettings().JavaScriptBundleFile(L"index");
	host.InstanceSettings().UseFastRefresh(true);
#endif

#if _DEBUG
	host.InstanceSettings().UseDirectDebugger(true);
	host.InstanceSettings().UseDeveloperSupport(true);
#else
	host.InstanceSettings().UseDirectDebugger(false);
	host.InstanceSettings().UseDeveloperSupport(false);
#endif

	winrt::Microsoft::ReactNative::Composition::CompositionUIService::SetCompositor(
		host.InstanceSettings(), compositor);

	host.InstanceSettings().InstanceCreated([](const auto& sender, const winrt::Microsoft::ReactNative::InstanceCreatedEventArgs& args)
	{
		winrt::Microsoft::ReactNative::ExecuteJsi(
			args.Context(),
			[](facebook::jsi::Runtime& runtime)
		{
			// Install fake expo modules object so that expo doesn't crap out
			auto expoModules = std::make_shared<ExpoModulesHostObject>();
			auto expoModulesObject = facebook::jsi::Object::createFromHostObject(
				runtime,
				expoModules
			);
			auto mainObject = std::make_shared<facebook::jsi::Object>(runtime);

			auto global = runtime.global();
			global.setProperty(
				runtime,
				"expo",
				*mainObject
			);

			mainObject
				->setProperty(
					runtime,
					"modules",
					expoModulesObject
				);
		});
	});

	return host;
}

extern "C" IMAGE_DOS_HEADER __ImageBase;
HINSTANCE WindowData::s_instance = reinterpret_cast<HINSTANCE>(&__ImageBase);
UINT const WMAPP_NOTIFYCALLBACK = WM_APP + 1;
HWND g_hwnd;
winrt::Microsoft::ReactNative::ReactNativeHost g_host { nullptr };
winrt::Microsoft::UI::Composition::Compositor g_compositor { nullptr };

BOOL AddNotificationIcon(HWND hwnd)
{
	NOTIFYICONDATA nid;
	nid.cbSize = sizeof(nid);
	nid.hWnd = hwnd;
	nid.uCallbackMessage = WMAPP_NOTIFYCALLBACK;
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_GUID | NIF_SHOWTIP | NIF_MESSAGE;

	static const GUID myGUID =
	{ 0x092BFDBB, 0x56CB, 0x47AD, {0xA7, 0xA0, 0xB9, 0xE1, 0x38, 0xC9, 0x3A, 0xC8} };
	nid.guidItem = myGUID;

	StringCchCopy(nid.szTip, ARRAYSIZE(nid.szTip), L"Expo Orbit");

	// Not sure why this is causing an error
	//LoadIconMetric(instance, MAKEINTRESOURCEW(IDI_ICON1), LIM_LARGE, &(nid.hIcon));

	Shell_NotifyIcon(NIM_ADD, &nid);

	nid.uVersion = NOTIFYICON_VERSION_4;
	return Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

void ApplyConstraintsForContentSizedWindow(winrt::Microsoft::ReactNative::LayoutConstraints& constraints)
{
	constraints.MinimumSize = { 100, 200 };
	constraints.MaximumSize = { 1000, 1000 };
}

void HandleReactNativeWindowMessage(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) noexcept
{
	auto windowData = WindowData::GetFromWindow(hwnd);

	switch (message)
	{
	case WM_CREATE:
	{
		auto data = WindowData::GetFromWindow(hwnd);

		// Create a RootView which will present a react-native component
		winrt::Microsoft::ReactNative::ReactViewOptions viewOptions;
		viewOptions.ComponentName(data->m_componentName);
		data->m_compRootView = winrt::Microsoft::ReactNative::ReactNativeIsland(g_compositor);
		data->m_compRootView.ReactViewHost(winrt::Microsoft::ReactNative::ReactCoreInjection::MakeViewHost(g_host, viewOptions));

		// DesktopChildSiteBridge create a ContentSite that can host the RootView ContentIsland
		data->m_bridge = winrt::Microsoft::UI::Content::DesktopChildSiteBridge::Create(g_compositor, winrt::Microsoft::UI::GetWindowIdFromWindow(hwnd));
		data->m_bridge.Connect(data->m_compRootView.Island());
		data->m_bridge.ResizePolicy(winrt::Microsoft::UI::Content::ContentSizePolicy::ResizeContentToParentWindow);

		data->m_compRootView.ScaleFactor(ScaleFactor(hwnd));
		winrt::Microsoft::ReactNative::LayoutConstraints constraints;
		constraints.LayoutDirection = winrt::Microsoft::ReactNative::LayoutDirection::LeftToRight;
		ApplyConstraintsForContentSizedWindow(constraints);

		// Disable user sizing of the hwnd
		::SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_SIZEBOX);
		data->m_compRootView.SizeChanged(
			[hwnd](auto sender, const winrt::Microsoft::ReactNative::RootViewSizeChangedEventArgs& args)
		{
			RECT rcClient, rcWindow;
			GetClientRect(hwnd, &rcClient);
			GetWindowRect(hwnd, &rcWindow);

			SetWindowPos(
				hwnd,
				nullptr,
				0,
				0,
				static_cast<int>(args.Size().Width) + rcClient.left - rcClient.right + rcWindow.right -
				rcWindow.left,
				static_cast<int>(args.Size().Height) + rcClient.top - rcClient.bottom + rcWindow.bottom -
				rcWindow.top,
				SWP_DEFERERASE | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
		});

		data->m_compRootView.Arrange(constraints, { 0, 0 });
		data->m_bridge.Show();

		break;
	}
	case WM_DESTROY: {
		auto data = WindowData::GetFromWindow(hwnd);
		data->m_compRootView = nullptr;
		data->m_instanceSettings = nullptr;
		data->m_bridge.Close();
		data->m_bridge = nullptr;

		delete WindowData::GetFromWindow(hwnd);
		SetProp(hwnd, WindowDataProperty, 0);
		break;
	}
	case WM_NCCREATE: {
		auto cs = reinterpret_cast<CREATESTRUCT*>(lparam);
		auto windowData = static_cast<WindowData*>(cs->lpCreateParams);
		WINRT_ASSERT(windowData);
		SetProp(hwnd, WindowDataProperty, reinterpret_cast<HANDLE>(windowData));
		break;
	}
	case WM_WINDOWPOSCHANGED: {
		auto windowData = WindowData::GetFromWindow(hwnd);
		RECT rc;
		if (GetClientRect(hwnd, &rc))
		{
			if (windowData->m_height != (rc.bottom - rc.top) || windowData->m_width != (rc.right - rc.left))
			{
				windowData->m_height = rc.bottom - rc.top;
				windowData->m_width = rc.right - rc.left;

				if (windowData->m_compRootView)
				{
					winrt::Windows::Foundation::Size size { windowData->m_width / ScaleFactor(hwnd), windowData->m_height / ScaleFactor(hwnd) };
					if (!IsIconic(hwnd))
					{
						winrt::Microsoft::ReactNative::LayoutConstraints constraints;
						constraints.LayoutDirection = winrt::Microsoft::ReactNative::LayoutDirection::LeftToRight;
						ApplyConstraintsForContentSizedWindow(constraints);
						windowData->m_compRootView.Arrange(constraints, { 0, 0 });
					}
				}
			}
		}
		break;
	}
	}
}

LRESULT CALLBACK PopupWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) noexcept
{
	HandleReactNativeWindowMessage(hwnd, message, wparam, lparam);

	switch (message)
	{
	case WM_DESTROY:
	{
		return 0;
	}
	}

	return DefWindowProc(hwnd, message, wparam, lparam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) noexcept
{
	HandleReactNativeWindowMessage(hwnd, message, wparam, lparam);
	auto windowData = WindowData::GetFromWindow(hwnd);

	switch (message)
	{
	case WM_CREATE:
	{
		AddNotificationIcon(hwnd);
		break;
	}
	case WM_DESTROY: {
		auto data = WindowData::GetFromWindow(hwnd);
		// Before we shutdown the application - gracefully unload the ReactNativeHost instance
		bool shouldPostQuitMessage = true;
		if (g_host)
		{
			shouldPostQuitMessage = false;

			auto async = g_host.UnloadInstance();
			async.Completed([](auto asyncInfo, winrt::Windows::Foundation::AsyncStatus asyncStatus)
			{
				assert(asyncStatus == winrt::Windows::Foundation::AsyncStatus::Completed);
				g_host.InstanceSettings().UIDispatcher().Post([]()
				{
					PostQuitMessage(0);
				});
			});
		}
		return 0;
	}

	case WM_CLOSE: {
		ShowWindow(g_hwnd, SW_HIDE);
		return 0;
	}

	case WMAPP_NOTIFYCALLBACK:
		switch (LOWORD(lparam))
		{
		case NIN_SELECT:
			// for NOTIFYICON_VERSION_4 clients, NIN_SELECT is prerable to listening to mouse clicks and key presses
			// directly.
			if (IsWindowVisible(g_hwnd))
			{
				auto x = GET_X_LPARAM(wparam);
				auto y = GET_Y_LPARAM(wparam);

				ShowWindow(g_hwnd, SW_HIDE);
			}
			else
			{

				ShowWindow(hwnd, SW_SHOW);
				UpdateWindow(hwnd);
				SetFocus(hwnd);
				SetForegroundWindow(hwnd);
			}
			break;

			/*
		case NIN_BALLOONTIMEOUT:
			RestoreTooltip();
			break;
			*/
			/*
		case NIN_BALLOONUSERCLICK:
			RestoreTooltip();
			// placeholder for the user clicking on the balloon.
			MessageBox(hwnd, L"The user clicked on the balloon.", L"User click", MB_OK);
			break;
			*/

			/*
		case WM_CONTEXTMENU:
		{
			POINT const pt = { LOWORD(wParam), HIWORD(wParam) };
			ShowContextMenu(hwnd, pt);
		}
		*/
			break;
		}
		break;

	}
	return DefWindowProc(hwnd, message, wparam, lparam);
}

_Use_decl_annotations_ int CALLBACK WinMain(HINSTANCE instance, HINSTANCE, PSTR /* commandLine */, int showCmd)
{
	// Initialize WinRT.
	winrt::init_apartment(winrt::apartment_type::single_threaded);

	// Enable per monitor DPI scaling
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	// Create a DispatcherQueue for this thread.  This is needed for Composition, Content, and
	// Input APIs.
	auto dispatcherQueueController { winrt::Microsoft::UI::Dispatching::DispatcherQueueController::CreateOnCurrentThread() };

	// Create a Compositor for all Content on this thread.
	g_compositor = winrt::Microsoft::UI::Composition::Compositor();

	g_host = CreateReactNativeHost(g_compositor);

	// Start the react-native instance, which will create a JavaScript runtime and load the applications bundle
	g_host.ReloadInstance();

	WNDCLASSEXW wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = &WndProc;
	wcex.cbClsExtra = DLGWINDOWEXTRA;
	wcex.cbWndExtra = sizeof(WindowData*);
	wcex.hInstance = WindowData::s_instance;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
	wcex.lpszClassName = c_mainWindowClassName;
	wcex.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_ICON1));
	ATOM classId = RegisterClassEx(&wcex);
	WINRT_VERIFY(classId);
	winrt::check_win32(!classId);


	WNDCLASSEXW wcex_popup = { 0 };
	wcex_popup.cbSize = sizeof(WNDCLASSEX);
	wcex_popup.style = CS_HREDRAW | CS_VREDRAW;
	wcex_popup.lpfnWndProc = &PopupWndProc;
	wcex_popup.cbClsExtra = DLGWINDOWEXTRA;
	wcex_popup.cbWndExtra = sizeof(WindowData*);
	wcex_popup.hInstance = WindowData::s_instance;
	wcex_popup.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex_popup.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
	wcex_popup.lpszClassName = c_popupClassName;
	wcex_popup.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_ICON1));
	ATOM classIdPopup = RegisterClassEx(&wcex_popup);
	WINRT_VERIFY(classIdPopup);
	winrt::check_win32(!classIdPopup);


	auto windowData = std::make_unique<WindowData>();
	windowData->m_componentName = mainComponentName;
	HWND hwnd = g_hwnd = CreateWindow(
		c_mainWindowClassName,
		appName,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		nullptr,
		nullptr,
		WindowData::s_instance,
		windowData.get());

	winrt::Microsoft::ReactNative::ReactCoreInjection::SetTopLevelWindowId(
		g_host.InstanceSettings().Properties(), reinterpret_cast<uint64_t>(hwnd));

	WINRT_VERIFY(hwnd);

	windowData.release();

	// Run the main application event loop
	dispatcherQueueController.DispatcherQueue().RunEventLoop();

	// Rundown the DispatcherQueue. This drains the queue and raises events to let components
	// know the message loop has finished.
	dispatcherQueueController.ShutdownQueue();

	// Destroy all Composition objects
	g_compositor.Close();
	g_compositor = nullptr;
}

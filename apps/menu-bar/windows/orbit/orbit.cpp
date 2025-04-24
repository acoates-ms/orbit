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
#include "SysTray.h"

#include <winrt/Microsoft.UI.interop.h>
#include <winrt/Windows.ApplicationModel.DataTransfer.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.h>


// UI is currently using a couple of platformColor values which do not exist in RN-Windows
// This provides those additional PlatformColors.
struct PlatformColorProvider : public winrt::implements <PlatformColorProvider, winrt::Microsoft::ReactNative::Composition::ICustomResourceLoader>
{
  void GetResource(winrt::hstring resourceId, winrt::Microsoft::ReactNative::Composition::ResourceType resourceType, const winrt::Microsoft::ReactNative::Composition::CustomResourceResult& result)
{
	  if (resourceId == L"controlAccentColor")
	  {
		  result.AlternateResourceId(L"Accent");
		  return;
	  }
	  if (resourceId == L"labelColor")
	  {
		  result.AlternateResourceId(L"Foreground");
		  return;
	  }
	  else if (resourceId == L"placeholderTextColor")
	  {
		  result.Resource(winrt::box_value(winrt::Windows::UI::Colors::Gray()));
		  return;
	  }
 }

  winrt::event_token ResourcesChanged(
	  winrt::Windows::Foundation::EventHandler<winrt::Windows::Foundation::IInspectable> const& handler) noexcept
  {
	  return m_themeChangedEvent.add(handler);
  }
  void ResourcesChanged(winrt::event_token const& token) noexcept
  {
	  m_themeChangedEvent.remove(token);
  }

private:
  winrt::event<winrt::Windows::Foundation::EventHandler<winrt::Windows::Foundation::IInspectable>> m_themeChangedEvent;
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

REACT_STRUCT(WindowProps)
struct WindowProps
{
	// Used by root container to add/remove root view's flex:1 property
	REACT_FIELD(noRootFlex)
	bool noRootFlex { true };
};

REACT_MODULE(WindowsManager)
struct WindowsManager : std::enable_shared_from_this<WindowsManager>
{
	struct WindowData {
		winrt::Microsoft::ReactNative::ReactNativeIsland reactNativeIsland { nullptr };
		winrt::Microsoft::UI::Windowing::AppWindow appWindow { nullptr };
	};

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
		m_context.UIDispatcher().Post([window, wkThis = weak_from_this()]()
		{
			if (auto strongThis = wkThis.lock())
			{
				auto data = std::make_shared<WindowData>();
				strongThis->m_windows[window] = data;

				auto presenter = winrt::Microsoft::UI::Windowing::OverlappedPresenter::CreateForDialog();
				auto appWindow = winrt::Microsoft::UI::Windowing::AppWindow::Create(presenter);
				data->appWindow = appWindow;
				appWindow.IsShownInSwitchers(true);
				appWindow.Title(appName);

				auto compositor = winrt::Microsoft::ReactNative::Composition::CompositionUIService::GetCompositor(strongThis->m_context.Properties().Handle());
				auto reactNativeIsland = winrt::Microsoft::ReactNative::ReactNativeIsland(compositor);
				data->reactNativeIsland = reactNativeIsland;
				
				winrt::Microsoft::ReactNative::LayoutConstraints constraints;
				constraints.LayoutDirection = winrt::Microsoft::ReactNative::LayoutDirection::Undefined;
				constraints.MinimumSize = { 0, 0 };
				constraints.MaximumSize = { 1000, 1000 };
				reactNativeIsland.Arrange(constraints, { 0, 0 });
			
				//TODO remove sizeChanged on close
				reactNativeIsland.SizeChanged([window, wkThis](
					winrt::Windows::Foundation::IInspectable const& /*sender*/, const winrt::Microsoft::ReactNative::RootViewSizeChangedEventArgs& /* args*/)
				{
					if (auto innerStrong = wkThis.lock())
					{
						auto compositor = winrt::Microsoft::ReactNative::Composition::CompositionUIService::GetCompositor(innerStrong->m_context.Properties().Handle());
						auto async = compositor.RequestCommitAsync();
						const auto& data = innerStrong->m_windows[window];
						async.Completed([wkThis, size = data->reactNativeIsland.Size(), appWindow = data->appWindow](auto, winrt::Windows::Foundation::AsyncStatus /*asyncStatus*/)
						{
							auto scale = ScaleFactor(winrt::Microsoft::UI::GetWindowFromWindowId(appWindow.Id()));
							appWindow.ResizeClient({ static_cast<int32_t>(size.Width * scale), static_cast<int32_t>(size.Height * scale) });
							appWindow.Show();
						});
					}
				});
			
				winrt::Microsoft::ReactNative::ReactViewOptions viewOptions;
				viewOptions.ComponentName(winrt::to_hstring(window));
				viewOptions.InitialProps(winrt::Microsoft::ReactNative::MakeJSValueWriter(strongThis->m_props));
				auto bridge = winrt::Microsoft::UI::Content::DesktopChildSiteBridge::Create(compositor, appWindow.Id());
			
				auto appContent = reactNativeIsland.Island();
			
				bridge.ResizePolicy(winrt::Microsoft::UI::Content::ContentSizePolicy::ResizeContentToParentWindow);
				bridge.Connect(appContent);
				auto host = winrt::Microsoft::ReactNative::ReactNativeHost::FromContext(strongThis->m_context.Handle());
				reactNativeIsland.ReactViewHost(winrt::Microsoft::ReactNative::ReactCoreInjection::MakeViewHost(host, viewOptions));

				bridge.Show();
			}
		});
	}

	REACT_METHOD(closeWindow)
		void closeWindow(const std::string& window) noexcept
	{
		assert(false);
	}

private:

	static float ScaleFactor(HWND hwnd) noexcept
	{
		return GetDpiForWindow(hwnd) / static_cast<float>(USER_DEFAULT_SCREEN_DPI);
	}

	winrt::Microsoft::ReactNative::ReactContext m_context;
	WindowProps m_props;
	std::map<std::string, std::shared_ptr<WindowData>> m_windows;
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

	// Not sure why the metro config is treating the root of this url as from the repo root, rather than the project root...
	host.InstanceSettings().DebugBundlePath(L"apps/menu-bar/index");

#if BUNDLE
	host.InstanceSettings().JavaScriptBundleFile(L"index.windows");
	host.InstanceSettings().BundleRootPath(std::wstring(L"file://").append(appDirectory).append(L"\\Bundle\\").c_str());
	host.InstanceSettings().UseFastRefresh(false);
#else
	host.InstanceSettings().UseFastRefresh(true);
#endif

#if _DEBUG
	host.InstanceSettings().UseDirectDebugger(true);
	host.InstanceSettings().UseDeveloperSupport(true);
	host.InstanceSettings().NativeLogger([](winrt::Microsoft::ReactNative::LogLevel /*level*/, winrt::hstring message)
	{
		OutputDebugStringA(winrt::to_string(message).c_str());
	});
#else
	host.InstanceSettings().UseDirectDebugger(false);
	host.InstanceSettings().UseDeveloperSupport(false);
#endif

	winrt::Microsoft::ReactNative::Composition::Theme::SetDefaultResources(host.InstanceSettings(), winrt::make<PlatformColorProvider>());

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

winrt::Microsoft::ReactNative::ReactNativeHost g_host { nullptr };
winrt::Microsoft::UI::Composition::Compositor g_compositor { nullptr };

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

	InitSysTray(instance, g_host);

	// Run the main application event loop
	dispatcherQueueController.DispatcherQueue().RunEventLoop();

	// Rundown the DispatcherQueue. This drains the queue and raises events to let components
	// know the message loop has finished.
	dispatcherQueueController.ShutdownQueue();

	// Destroy all Composition objects
	g_compositor.Close();
	g_compositor = nullptr;
}

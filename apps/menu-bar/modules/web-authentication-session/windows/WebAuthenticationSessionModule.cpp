#include "pch.h"

#include "NativeModules.h"
#include <winrt/Microsoft.Security.Authentication.OAuth.h>
#include <winrt/Microsoft.UI.Interop.h>

namespace oAuth = winrt::Microsoft::Security::Authentication::OAuth;

namespace winrt {
}

REACT_STRUCT(WebAuthenticationSessionResultCancel)
struct WebAuthenticationSessionResultCancel
{
	REACT_FIELD(type)
	std::string type { "cancel" };
};

REACT_STRUCT(WebAuthenticationSessionResultSuccess)
struct WebAuthenticationSessionResultSuccess
{
	REACT_FIELD(type)
	std::string type { "success" };

	REACT_FIELD(url)
	winrt::hstring url;
};


REACT_MODULE(WebAuthenticationSession)
struct WebAuthenticationSession
{
	REACT_INIT(Initialize);
	void Initialize(const winrt::Microsoft::ReactNative::ReactContext& reactContext) noexcept
	{
		m_context = reactContext;
	}

	REACT_METHOD(openAuthSessionAsync)
		winrt::fire_and_forget openAuthSessionAsync(const std::wstring uri, winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValue> result) noexcept
	{
		try
		{
			oAuth::AuthRequestParams authRequestParams = oAuth::AuthRequestParams::CreateForAuthorizationCodeRequest(L"orbit",
				winrt::Windows::Foundation::Uri(L"expo-orbit:///auth"));

			auto windowId = winrt::Microsoft::UI::GetWindowIdFromWindow(reinterpret_cast<HWND>(
				winrt::Microsoft::ReactNative::ReactCoreInjection::GetTopLevelWindowId(m_context.Properties().Handle())));

			// Not sure what is wrong with the OAuth2Manager::RequestAuthWithParamsAsync work flow, but call to OAuth2Manager::CompleteAuthRequest in the app activation isn't getting back to the OAuth2Manager
			// So we're using this notification instead.
			auto subscription = m_context.Notifications().Handle().Subscribe(
				winrt::Microsoft::ReactNative::ReactPropertyBagHelper::GetName(winrt::Microsoft::ReactNative::ReactPropertyBagHelper::GetNamespace(L"Orbit"), L"AuthResponseUri"),
				m_context.JSDispatcher().Handle(),
				[result](auto sender, winrt::Microsoft::ReactNative::IReactNotificationArgs args)
				{
					args.Subscription().Unsubscribe();
					auto uri = winrt::unbox_value<winrt::Windows::Foundation::Uri>(args.Data());
					WebAuthenticationSessionResultSuccess successResult;
					successResult.url = uri.ToString();
					auto writer = winrt::Microsoft::ReactNative::MakeJSValueTreeWriter();
					winrt::Microsoft::ReactNative::WriteValue(writer, successResult);
					result.Resolve(winrt::Microsoft::ReactNative::TakeJSValue(writer));
				}
			);

			auto tokenRequestResult = co_await oAuth::OAuth2Manager::RequestAuthWithParamsAsync(windowId, winrt::Windows::Foundation::Uri(uri), authRequestParams);

			subscription.Unsubscribe();

			if (tokenRequestResult.Failure())
			{
				auto er = tokenRequestResult.Failure().Error();
				auto erd = tokenRequestResult.Failure().ErrorDescription();

				WebAuthenticationSessionResultCancel cancelResult;
				auto writer = winrt::Microsoft::ReactNative::MakeJSValueTreeWriter();
				winrt::Microsoft::ReactNative::WriteValue(writer, WebAuthenticationSessionResultCancel {});
				result.Resolve(winrt::Microsoft::ReactNative::TakeJSValue(writer));
				co_return;
			}

			auto response = tokenRequestResult.Response();

			WebAuthenticationSessionResultSuccess successResult;
			successResult.url = response.AccessToken();
			auto writer = winrt::Microsoft::ReactNative::MakeJSValueTreeWriter();
			winrt::Microsoft::ReactNative::WriteValue(writer, successResult);
			result.Resolve(winrt::Microsoft::ReactNative::TakeJSValue(writer));
		}
		catch (winrt::hresult_error e)
		{
			WebAuthenticationSessionResultCancel cancelResult;
			auto writer = winrt::Microsoft::ReactNative::MakeJSValueTreeWriter();
			winrt::Microsoft::ReactNative::WriteValue(writer, WebAuthenticationSessionResultCancel {});
			result.Resolve(winrt::Microsoft::ReactNative::TakeJSValue(writer));
			co_return;
		}
	}
private:
	winrt::Microsoft::ReactNative::ReactContext m_context;
};
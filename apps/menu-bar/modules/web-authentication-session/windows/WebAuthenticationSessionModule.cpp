#include "pch.h"

#include "NativeModules.h"

REACT_STRUCT(WebAuthenticationSessionResultCancel)
struct WebAuthenticationSessionResultCancel
{
	std::string type { "cancel" };
};

REACT_STRUCT(WebAuthenticationSessionResultSuccess)
struct WebAuthenticationSessionResultSuccess
{
	std::string type { "success" };
	std::string url;
};


REACT_MODULE(WebAuthenticationSession)
struct WebAuthenticationSession
{
	REACT_METHOD(openAuthSessionAsync)
		void openAuthSessionAsync(const std::string& uri, winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValue>&& result) noexcept
	{
		WebAuthenticationSessionResultCancel cancelResult;
		auto writer = winrt::Microsoft::ReactNative::MakeJSValueTreeWriter();
		winrt::Microsoft::ReactNative::WriteValue(writer, WebAuthenticationSessionResultCancel {});
		result.Resolve(winrt::Microsoft::ReactNative::TakeJSValue(writer));

		// Sadly winrt::Windows::Security::Authentication::Web::WebAuthenticationBroker::AuthenticateAsync doesn't work in win32 apps.
		// Will probably have to do something relatively complex with cpprestsdk to implement this until WinAppSDK comes out with a new version of AuthenticateAsync
	}
};
#include "pch.h"

#include "FileHandlerModule.h"
#include "NativeModules.h"


winrt::Microsoft::ReactNative::ReactNotificationId<winrt::hstring> FileHandlerOpenNotification() noexcept
{
	static winrt::Microsoft::ReactNative::ReactNotificationId<winrt::hstring> id { L"Expo", L"NotifyFileOpen" };
	return id;
}

REACT_STRUCT(FileHanderOnOpenFileArgs)
struct FileHanderOnOpenFileArgs
{
	REACT_FIELD(path)
	std::string path;
};

REACT_TURBO_MODULE(FileHandler)
struct FileHandler : public std::enable_shared_from_this< FileHandler>
{
	REACT_INIT(Initialize);
	void Initialize(const winrt::Microsoft::ReactNative::ReactContext& reactContext) noexcept
	{
		m_context = reactContext;

		m_subscriptionRevoker = m_context.Notifications().Subscribe(winrt::auto_revoke, FileHandlerOpenNotification(), [wkThis = weak_from_this()](winrt::Windows::Foundation::IInspectable const& sender, const winrt::Microsoft::ReactNative::ReactNotificationArgs<winrt::hstring>& args)
		{
			if (args.Data())	
			{
				if (auto strong = wkThis.lock())
				{
					FileHanderOnOpenFileArgs onOpenFileArgs;
					onOpenFileArgs.path = winrt::to_string(*args.Data());
					strong->onOpenFile(onOpenFileArgs);
				}
			}
		});
	}

	REACT_EVENT(onOpenFile)
	std::function<void(FileHanderOnOpenFileArgs)> onOpenFile;

private:
	winrt::Microsoft::ReactNative::ReactNotificationSubscriptionRevoker m_subscriptionRevoker;
	winrt::Microsoft::ReactNative::ReactContext m_context;

};
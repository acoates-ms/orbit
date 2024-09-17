#include "pch.h"

#include "NativeModules.h"

#include "Shlobj.h"
#include <winrt/Windows.System.h>

REACT_STRUCT(MenuBarConstants)
struct MenuBarConstants
{
    REACT_FIELD(homedir)
    std::string homedir;
};

REACT_MODULE(MenuBar)
struct MenuBar
{
    REACT_METHOD(exitApp)
    void exitApp() noexcept
    {
        assert(false);
    }

    REACT_GET_CONSTANTS(getConstants)
    static MenuBarConstants getConstants() noexcept
    {
        MenuBarConstants constants;
        PWSTR path = nullptr;
        auto hr = SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr , &path);
        std::wstring value(path);
        CoTaskMemFree(path);
        constants.homedir = winrt::to_string(value);
        return constants;
    }

    REACT_METHOD(openSystemSettingsLoginItems)
        winrt::fire_and_forget openSystemSettingsLoginItems() noexcept
    {
        try
        {
            winrt::Windows::Foundation::Uri uri(L"shell:startup");

            if (co_await winrt::Windows::System::Launcher::LaunchUriAsync(uri))
            {
                OutputDebugStringA("MenuBar.openSystemSettingsLoginItems success");
            }
            else
            {
                OutputDebugStringA("MenuBar.openSystemSettingsLoginItems fail");
            }
        }
        catch (winrt::hresult_error&)
        {
            OutputDebugStringA("MenuBar.openSystemSettingsLoginItems throw");
        }
    }

    REACT_METHOD(runCli)
        void runCli(const std::string& command, std::vector<std::string>& args, int listenerId, winrt::Microsoft::ReactNative::ReactPromise<std::string> result) noexcept
    {
        result.Reject("NYI MenuBar.runCli");
    }

    REACT_METHOD(runCommand)
        void runCommand(const std::string& command, std::vector<std::string>& args, winrt::Microsoft::ReactNative::ReactPromise<void> result) noexcept
    {
        result.Reject("NYI MenuBar.runCommand");
    }

    REACT_METHOD(setLoginItemEnabled)
        void setLoginItemEnabled(bool enabled, winrt::Microsoft::ReactNative::ReactPromise<void> result) noexcept
    {
        result.Reject("NYI MenuBar.setLoginItemEnabled");

    }

    REACT_METHOD(showMultiOptionAlert)
        void showMultiOptionAlert(const std::string& title, const std::string& message, const std::vector<std::string>& options, winrt::Microsoft::ReactNative::ReactPromise<int> result) noexcept
    {
        result.Reject("NYI MenuBar.showMultiOptionAlert");
    }

    REACT_METHOD(openPopover)
        void openPopover() noexcept
    {

    }

    REACT_METHOD(closePopover)
        void closePopover() noexcept
    {
    }

    REACT_METHOD(setEnvVars)
        void setEnvVars(const winrt::Microsoft::ReactNative::JSValueObject& vars) noexcept
    {

        //<homedir>/.expo/orbit/auth.json
        /*
        for (auto& p : vars)
        {

            p.first
        }
        */
    }

    REACT_METHOD(addListener)
        void addListener(std::string const&) noexcept
    {
    }

    REACT_METHOD(removeListeners)
        void removeListeners(int) noexcept
    {
    }
};
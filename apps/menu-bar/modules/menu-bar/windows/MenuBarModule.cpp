#include "pch.h"

#include "NativeModules.h"

#include "Shlobj.h"
#include <winrt/Windows.System.h>
#include <stdio.h>
#include <string>
#include <sstream>

REACT_STRUCT(Size)
struct Size
{
    REACT_FIELD(height)
    int32_t height;
    REACT_FIELD(width)
    int32_t width;
};

REACT_STRUCT(MenuBarConstants)
struct MenuBarConstants
{
    REACT_FIELD(homedir)
    std::string homedir;

    REACT_FIELD(initialScreenSize)
    Size initialScreenSize;
};

REACT_STRUCT(OnCLIOutputArgs)
struct OnCLIOutputArgs
{
    REACT_FIELD(listenerId)
    int listenerId;

    REACT_FIELD(output)
    std::string output;
};

REACT_MODULE(MenuBar)
struct MenuBar
{
    REACT_INIT(Initialize);
    void Initialize(const winrt::Microsoft::ReactNative::ReactContext& reactContext) noexcept
    {
        m_context = reactContext;
    }

    REACT_METHOD(exitApp)
    void exitApp() noexcept
    {
        auto host = winrt::Microsoft::ReactNative::ReactNativeHost::FromContext(m_context.Handle());
        auto async = host.UnloadInstance();
        async.Completed([host](auto /*asyncInfo*/, winrt::Windows::Foundation::AsyncStatus /*asyncStatus*/)
        {
            //Assert(asyncStatus == winrt::Windows::Foundation::AsyncStatus::Completed);
            host.InstanceSettings().UIDispatcher().Post([]()
            {
                PostQuitMessage(0);
            });
        });
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

        constants.initialScreenSize = { 1000, 1000 };
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

    REACT_EVENT(onCLIOutput)
    std::function<void(OnCLIOutputArgs)> onCLIOutput;

    REACT_METHOD(runCli)
        void runCli(const std::string& command, std::vector<std::string>& args, int listenerId, winrt::Microsoft::ReactNative::ReactPromise<std::string> result) noexcept
    {
        constexpr DWORD BUFSIZE = 4096;
        HANDLE hChildStd_OUT_Wr;
        HANDLE hChildStd_OUT_Rd;
        SECURITY_ATTRIBUTES saAttr;

        // Set the bInheritHandle flag so pipe handles are inherited. 
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        // Create a pipe for the child process's STDOUT. 
        if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0))
            result.Reject("StdoutRd CreatePipe");

        // Ensure the read handle to the pipe for STDOUT is not inherited.
        if (!SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
            result.Reject("Stdout SetHandleInformation");

        char appDirectory[MAX_PATH];
        GetModuleFileNameA(NULL, appDirectory, MAX_PATH);

        std::string cliPath(appDirectory);
        cliPath = cliPath.substr(0, cliPath.size() - std::string("orbit.exe").length());
        cliPath = cliPath + "..\\orbit-cli.exe ";

        auto cmdLine = cliPath + command;

        // Escape input for command line args
        for (const auto& arg : args)
        {
            std::string from { "\"" };
            std::string to { "\"\"" };
            std::string str = arg;
            size_t start_pos = 0;
            while ((start_pos = str.find(from, start_pos)) != std::string::npos)
            {
                str.replace(start_pos, from.length(), to);
                start_pos += to.length();
            }

            cmdLine = cmdLine + " \"" + str + "\"";
        }

        PROCESS_INFORMATION piProcInfo;
        STARTUPINFOA siStartInfo;
        BOOL bSuccess = FALSE;

        ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

        ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
        siStartInfo.cb = sizeof(STARTUPINFO);
        siStartInfo.hStdError = hChildStd_OUT_Wr;
        siStartInfo.hStdOutput = hChildStd_OUT_Wr;
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

        SetEnvironmentVariable(L"EXPO_MENU_BAR", L"1");

        bSuccess = CreateProcessA(NULL,
            const_cast<char*>(cmdLine.c_str()),     // command line 
            NULL,          // process security attributes 
            NULL,          // primary thread security attributes 
            TRUE,          // handles are inherited 
            CREATE_NO_WINDOW,             // creation flags 
            NULL,          // use parent's environment 
            NULL,          // use parent's current directory 
            &siStartInfo,  // STARTUPINFO pointer 
            &piProcInfo);  // receives PROCESS_INFORMATION 

        // If an error occurs, exit the application. 
        if (!bSuccess)
        {
            result.Reject("CreateProcess");
            return;
        }
        else
        {
            CloseHandle(piProcInfo.hProcess);
            CloseHandle(piProcInfo.hThread);
            CloseHandle(hChildStd_OUT_Wr);
        }

        bool hasReachedReturnOutput = false;
        bool hasReachedError = false;
        std::string returnOutput;

        // Read output from the child process's pipe for STDOUT
        // and write to the parent process's pipe for STDOUT. 
        // Stop when there is no more data. 
        {
            DWORD dwRead;
            CHAR chBuf[BUFSIZE];
            BOOL bSuccess = FALSE;

            for (;;)
            {
                bSuccess = ReadFile(hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
                if (!bSuccess || dwRead == 0) break;

                std::string output(chBuf, dwRead);
                std::stringstream ss(output);
                std::string t;

                while (std::getline(ss, t))
                {
                    if (hasReachedReturnOutput || hasReachedError)
                    {
                        returnOutput += t;
                    }

                    if (t == "---- return output ----")
                    {
                        hasReachedReturnOutput = true;
                    }
                    else if (t == "---- thrown error ----")
                    {
                        hasReachedError = true;
                    }
                    else if (!t.empty())
                    {
                        OnCLIOutputArgs onCliOutputArgs;
                        onCliOutputArgs.listenerId = listenerId;
                        onCliOutputArgs.output = t;
                        onCLIOutput(onCliOutputArgs);
                    }
                }
                if (!bSuccess) break;
            }
        }

        if (hasReachedError)
        {
            result.Reject(returnOutput.c_str());
        }
        else
        {
            // Post this to the JSDispatcher twice to ensure that the onCLIOutput events, which hit the native module queue, are processed before we return.
            // This is a requirement since the JS only registers for onCLIOutput until the promise is resolved.
            m_context.JSDispatcher().Post([context = m_context, r = std::move(result), output = std::move(returnOutput)]
            {
                context.JSDispatcher().Post([r, o = std::move(output)]
                {
                    r.Resolve(o.c_str());
                });
            });
        }
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
private:
    winrt::Microsoft::ReactNative::ReactContext m_context;
};
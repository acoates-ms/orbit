#include "pch.h"

#include "NativeModules.h"
#include <winrt/Windows.Storage.Pickers.h>
#include <Shobjidl.h>

REACT_MODULE(FilePicker)
struct FilePicker
{
    REACT_INIT(Initialize);
    void Initialize(const winrt::Microsoft::ReactNative::ReactContext& reactContext) noexcept
    {
        m_context = reactContext;
    }

    REACT_METHOD(pickFolder)
    winrt::fire_and_forget pickFolder(winrt::Microsoft::ReactNative::ReactPromise<std::string> result) noexcept
    {
        winrt::Windows::Storage::Pickers::FolderPicker picker;

        auto hwnd = reinterpret_cast<HWND>(
            winrt::Microsoft::ReactNative::ReactCoreInjection::GetTopLevelWindowId(m_context.Properties().Handle()));
        auto initializeWithWindow { picker.as<::IInitializeWithWindow>() };
        initializeWithWindow->Initialize(hwnd);

        auto storageFolder = co_await picker.PickSingleFolderAsync();
        if (!storageFolder)
        {
            result.Reject("No folder selected by user");
            co_return;
        }

        result.Resolve(winrt::to_string(storageFolder.Path()));
    }

    REACT_METHOD(pickFileWithFilenameExtension)
        winrt::fire_and_forget pickFileWithFilenameExtension(const std::vector<std::string> filenameExtensions, const std::string prompt, winrt::Microsoft::ReactNative::ReactPromise<std::string> result) noexcept
    {
        winrt::Windows::Storage::Pickers::FileOpenPicker picker;

        auto hwnd = reinterpret_cast<HWND>(
            winrt::Microsoft::ReactNative::ReactCoreInjection::GetTopLevelWindowId(m_context.Properties().Handle()));
        auto initializeWithWindow { picker.as<::IInitializeWithWindow>() };
        initializeWithWindow->Initialize(hwnd);

        for (const auto& ext : filenameExtensions)
        {
            picker.FileTypeFilter().Append(winrt::to_hstring("." + ext));
        }

        picker.CommitButtonText(winrt::to_hstring(prompt));

        auto storageFile = co_await picker.PickSingleFileAsync();
        if (!storageFile)
        {
            result.Reject("No file selected by user");
            co_return;
        }
        result.Resolve(winrt::to_string(storageFile.Path()));
    }

private:
    winrt::Microsoft::ReactNative::ReactContext m_context;
};

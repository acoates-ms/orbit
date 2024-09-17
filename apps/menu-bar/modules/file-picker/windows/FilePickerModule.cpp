#include "pch.h"

#include "NativeModules.h"

REACT_MODULE(FilePicker)
struct FilePicker
{
    REACT_METHOD(pickFolder)
    void pickFolder(winrt::Microsoft::ReactNative::ReactPromise<std::string> && result) noexcept
    {
        result.Reject("No impl");
    }

    REACT_METHOD(pickFileWithFilenameExtension)
    void pickFileWithFilenameExtension(const std::vector<std::string>& filenameExtensions, const std::string& prompt, winrt::Microsoft::ReactNative::ReactPromise<std::string>&& result) noexcept
    {
        result.Reject("No impl");
    }
};

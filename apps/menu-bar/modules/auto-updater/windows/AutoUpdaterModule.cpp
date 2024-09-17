#include "pch.h"

#include "NativeModules.h"

REACT_MODULE(AutoUpdater)
struct AutoUpdater
{
    REACT_METHOD(checkForUpdates)
    void checkForUpdates() noexcept { }

    REACT_METHOD(getAutomaticallyChecksForUpdates)
        void getAutomaticallyChecksForUpdates(winrt::Microsoft::ReactNative::ReactPromise<bool>&& result) noexcept
    {
        result.Resolve(false);
    }

    REACT_METHOD(setAutomaticallyChecksForUpdates)
        void setAutomaticallyChecksForUpdates(bool value) noexcept
    {
    }
};
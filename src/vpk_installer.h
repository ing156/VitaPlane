#pragma once
#include <functional>
#include <string>
// UI-thread progress callback draws/presents an app frame; -1 means indeterminate progress.
// The dialog callback confirms a question or acknowledges the result on the UI thread.
// True means the updater owns the staged VitaPlane package:
// the caller must exit without deleting staging or launching itself. Saved VPKs are preserved.
bool install_downloaded_vpk(const std::string& path,
                            const std::function<void(const std::string&, int)>& progress,
                            const std::function<bool(const std::string&, bool)>& dialog);

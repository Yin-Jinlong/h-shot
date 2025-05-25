#include "pre.h"

tstring GetErrorMessage(const DWORD code) {
    TCHAR buffer[4096];
    const auto len = FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, code, 0, buffer, sizeof(buffer), nullptr);
    return {buffer, len};
}

int ShowError(const DWORD code) {
    if (code == 0)
        return 0;
    return ShowError(GetErrorMessage(code));
}

int ShowErrorExit(const int exitCode, const DWORD code) {
    if (code == 0)
        return 0;
    ShowError(GetErrorMessage(code));
    exit(exitCode);
}

int ShowError(const tstring& msg) { return MessageBox(nullptr, msg.c_str(), TEXT("Error"), MB_ICONERROR); }

int ShowErrorExit(const tstring& msg, const DWORD code) {
    MessageBox(nullptr, msg.c_str(), TEXT("Error"), MB_ICONERROR);
    exit(code);
}

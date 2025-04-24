#include <Windows.h>
#include <vector>
#include <string>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    uint32_t numPathArrayElements = 0;
    uint32_t numModeInfoArrayElements = 0;
    std::vector<DISPLAYCONFIG_PATH_INFO> pathArray;
    std::vector<DISPLAYCONFIG_MODE_INFO> modeInfoArray;
    long status;
    std::string err_msg;

    // Take command line args to set HDR state across capable displays

    bool forceState = false;
    bool hdrRequired;

    int argNum;

    LPWSTR* argList = CommandLineToArgvW(GetCommandLineW(), &argNum);

    if (argNum == 2) {
        forceState = true;
        if (lstrcmpW(argList[1], L"0") != 0 && lstrcmpW(argList[1], L"1") != 0) {
            err_msg = "Provide HDR state as 0 or 1";
            OutputDebugStringW((LPCWSTR)err_msg.c_str());
            forceState = false;
        }
        hdrRequired = lstrcmpW(argList[1], L"0");
    }

    status = GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS | QDC_VIRTUAL_MODE_AWARE, &numPathArrayElements, &numModeInfoArrayElements);

    if (status != ERROR_SUCCESS) {
        err_msg = "GetDisplayConfigBufferSizes failed with code: " + std::to_string(status);
        OutputDebugStringW((LPCWSTR)err_msg.c_str());
        return status;
    }

    pathArray.resize(numPathArrayElements);
    modeInfoArray.resize(numModeInfoArrayElements);

    status = QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS | QDC_VIRTUAL_MODE_AWARE, &numPathArrayElements, pathArray.data(), &numModeInfoArrayElements, modeInfoArray.data(), nullptr);

    if (status != ERROR_SUCCESS) {
        err_msg = "QueryDisplayConfig failed with code: " + std::to_string(status);
        OutputDebugStringW((LPCWSTR)err_msg.c_str());
        return status;
    }

    DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO hdrCapability = {};
    DISPLAYCONFIG_SET_ADVANCED_COLOR_STATE hdrState = {};

    for (uint32_t i = 0; i < numPathArrayElements; ++i) {
        if (pathArray[i].flags & DISPLAYCONFIG_PATH_ACTIVE)
        {

            hdrCapability.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO;
            hdrCapability.header.size = sizeof(DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO);
            hdrCapability.header.adapterId = pathArray[i].sourceInfo.adapterId;
            hdrCapability.header.id = pathArray[i].targetInfo.id;

            status = DisplayConfigGetDeviceInfo(&hdrCapability.header);

            if (status != ERROR_SUCCESS) {
                err_msg = "DisplayConfigGetDeviceInfo failed with code: " + std::to_string(status) + " for id: " + std::to_string(pathArray[i].sourceInfo.id);
                OutputDebugStringW((LPCWSTR)err_msg.c_str());
                return status;
            }

            if (hdrCapability.advancedColorSupported == 1) {
                hdrState.header.type = DISPLAYCONFIG_DEVICE_INFO_SET_ADVANCED_COLOR_STATE;
                hdrState.header.size = sizeof(DISPLAYCONFIG_SET_ADVANCED_COLOR_STATE);
                hdrState.header.adapterId = pathArray[i].sourceInfo.adapterId;
                hdrState.header.id = pathArray[i].targetInfo.id;
                if (forceState == false) {
                    hdrState.enableAdvancedColor = (hdrCapability.advancedColorEnabled == 0) ? 1 : 0;
                }
                else {
                    hdrState.enableAdvancedColor = hdrRequired;
                }

                status = DisplayConfigSetDeviceInfo(&hdrState.header);

                if (status != ERROR_SUCCESS) {
                    err_msg = "DisplayConfigSetDeviceInfo failed with code: " + std::to_string(status) + " for id: " + std::to_string(pathArray[i].sourceInfo.id);
                    OutputDebugStringW((LPCWSTR)err_msg.c_str());
                    return status;
                }
            }
        }
    }
    return 0;
}
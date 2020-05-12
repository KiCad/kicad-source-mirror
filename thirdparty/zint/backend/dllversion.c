/* Sed: http://msdn.microsoft.com/library/en-us/shellcc/platform/shell/programmersguide/versions.asp */
#if defined (_WIN32) && (defined(_USRDLL) || defined(DLL_EXPORT) || defined(PIC))
#include <windows.h>
#include <shlwapi.h>

#ifdef __cplusplus
extern "C"
{
#endif

__declspec(dllexport) HRESULT DllGetVersion (DLLVERSIONINFO2* pdvi);

#ifdef __cplusplus
}
#endif

HRESULT DllGetVersion (DLLVERSIONINFO2* pdvi)
{
	if (!pdvi || (sizeof(*pdvi) != pdvi->info1.cbSize))
		return (E_INVALIDARG);

	pdvi->info1.dwMajorVersion = 2;
	pdvi->info1.dwMinorVersion = 2;
	pdvi->info1.dwBuildNumber = 1;
	pdvi->info1.dwPlatformID = DLLVER_PLATFORM_WINDOWS;
	if (sizeof(DLLVERSIONINFO2) == pdvi->info1.cbSize)
		pdvi->ullVersion = MAKEDLLVERULL(2, 2, 1, 0);

	return S_OK;
}
#endif /* _WIN32 */

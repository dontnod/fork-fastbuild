/*
 * Author:  David Robert Nadeau
 * Site:    http://NadeauSoftware.com/
 * License: Creative Commons Attribution 3.0 Unported License
 *          http://creativecommons.org/licenses/by/3.0/deed.en_US
 */

#if defined(_WIN32)
#include <Windows.h>
#else
#error "Unable to define GetSystemMemorySize( ) for an unknown OS."
#endif

// Returns the size of physical memory (RAM) in bytes.
//------------------------------------------------------------------------------
void GetSystemMemorySize( size_t * free, size_t * total )
{
#if defined(_WIN32) && (defined(__CYGWIN__) || defined(__CYGWIN32__))
	/* Cygwin under Windows. ------------------------------------ */
	/* New 64-bit MEMORYSTATUSEX isn't available.  Use old 32.bit */
	MEMORYSTATUS status;
	status.dwLength = sizeof(status);
	GlobalMemoryStatus( &status );

    *free = (size_t)status.dwAvailPhys;
	*total = (size_t)status.dwTotalPhys;

#elif defined(_WIN32)
	/* Windows. ------------------------------------------------- */
	/* Use new 64-bit MEMORYSTATUSEX, not old 32-bit MEMORYSTATUS */
	MEMORYSTATUSEX status;
	status.dwLength = sizeof(status);
	GlobalMemoryStatusEx( &status );

    *free = (size_t)status.ullAvailPhys;
	*total = (size_t)status.ullTotalPhys;

#else

    *free = *total = 0;
	return 0L;			/* Unknown OS. */
#endif
}

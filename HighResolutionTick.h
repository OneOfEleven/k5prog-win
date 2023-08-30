
/*
 * Use at your own risk
 *
 *
 * This program is licensed under the GNU GENERAL PUBLIC LICENSE v3
 * License text avaliable at: http://www.gnu.org/copyleft/gpl.html 
 */

/*
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef HighResolutionTickH
#define HighResolutionTickH

//#define NEW_HI_RES_TIMER

#ifdef _MSC_VER
	#include <afxwin.h>	// for CRITICAL_SECTION
#endif

#ifdef NEW_HI_RES_TIMER
	#include <ctime>
#endif

#define VC_EXTRALEAN
#define WIN32_EXTRA_LEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class CHighResolutionTick
{
private:
	#ifndef NEW_HI_RES_TIMER
		__int64 tick_frequency;
		__int64 tick_initial;
		__int64 tick_prev;
	#else
		std::clock_t clock_initial;
		std::clock_t clock_previous;
	#endif

	CRITICAL_SECTION criticalSection;

public:
	__fastcall CHighResolutionTick(void)
	{
		memset(&criticalSection, 0, sizeof(criticalSection));
		::InitializeCriticalSection(&criticalSection);

		::EnterCriticalSection(&criticalSection);

		#ifndef NEW_HI_RES_TIMER
			tick_frequency = 0;
			tick_initial = 0;
			tick_prev = 0;

			__int64 freq = 0;
			__int64 init = 0;

			if (::QueryPerformanceFrequency((LARGE_INTEGER *)&freq) == FALSE)
			{
				::LeaveCriticalSection(&criticalSection);
				return;
			}

			if (::QueryPerformanceCounter((LARGE_INTEGER *)&init) == FALSE)
			{
				::LeaveCriticalSection(&criticalSection);
				return;
			}

			if (freq <= 0 || init <= 0)
			{
				::LeaveCriticalSection(&criticalSection);
				return;
			}

			tick_frequency = freq;
			tick_initial = init;
			tick_prev = init;
		#else
			clock_initial = std::clock();
			clock_previous = clock_initial;
		#endif

		::LeaveCriticalSection(&criticalSection);
	}

	__fastcall ~CHighResolutionTick(void)
	{
		::DeleteCriticalSection(&criticalSection);
	}

	int __fastcall mark()
	{
		::EnterCriticalSection(&criticalSection);

		#ifndef NEW_HI_RES_TIMER
			if (tick_frequency <= 0)
			{
				::LeaveCriticalSection(&criticalSection);
				return -1;
			}

			if (::QueryPerformanceCounter((LARGE_INTEGER *)&tick_prev) == FALSE)
			{
				::LeaveCriticalSection(&criticalSection);
				return -2;
			}
		#else
			clock_previous = std::clock();
		#endif

		::LeaveCriticalSection(&criticalSection);

		return 0;
	}

	double __fastcall millisecs(bool update = false)
	{
		return (secs(update) * 1000);
	}

	double __fastcall secs(bool update = false)
	{
		::EnterCriticalSection(&criticalSection);

		#ifndef NEW_HI_RES_TIMER

			if (tick_frequency <= 0)
			{
				::LeaveCriticalSection(&criticalSection);
				return -1;
			}

			__int64 tick = 0;
			if (::QueryPerformanceCounter((LARGE_INTEGER *)&tick) == FALSE)
			{
				::LeaveCriticalSection(&criticalSection);
				return -2;
			}

			double diff_secs = static_cast <double> (tick - tick_prev) / tick_frequency;

			if (update)
				tick_prev = tick;

		#else

			double diff_secs;
			std::clock_t now = std::clock();

			if (now < clock_previous)
			{	// wrap around
				diff_secs = (double)(((uint64_t)0x100000000ull + now) - (uint64_t)clock_previous) / CLOCKS_PER_SEC;
			}
			else
			{
				diff_secs = (double)(now - clock_previous) / CLOCKS_PER_SEC;
			}

			if (update)
            	clock_previous = now;

		#endif

		::LeaveCriticalSection(&criticalSection);

		return diff_secs;
	}
};

#endif

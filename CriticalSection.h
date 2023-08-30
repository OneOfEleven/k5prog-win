/*
 * Copyright 2020 C Moss G6AMU
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

//#pragma once

#ifndef CriticalSectionH
#define CriticalSectionH

#ifdef _MSC_VER
	#include <afxwin.h>	// for CRITICAL_SECTION
#endif

class CCriticalSectionObj
{
private:
//	CRITICAL_SECTION m_cs;
public:
	CCriticalSectionObj()
	{
		::InitializeCriticalSection(&m_cs);
	}
	~CCriticalSectionObj()
	{
		::DeleteCriticalSection(&m_cs);
	}
	CRITICAL_SECTION m_cs;
//	__property CRITICAL_SECTION cs = {read = m_cs};
};

class CCriticalSection
{
private:
	volatile LONG m_count;
	CCriticalSectionObj *m_cso;

public:
	__fastcall CCriticalSection(CCriticalSectionObj &cso, bool enter = true)
	{
		m_count = 0;
		m_cso = &cso;
		if (m_cso && enter)
		{
			::EnterCriticalSection(&m_cso->m_cs);
			::InterlockedIncrement(&m_count);
		}
	}

	__fastcall ~CCriticalSection()
	{
		if (m_cso && m_count > 0)
		{
			::InterlockedDecrement(&m_count);
			::LeaveCriticalSection(&m_cso->m_cs);
		}
	}

	void __fastcall enter()
	{
		if (m_cso)
		{
			::EnterCriticalSection(&m_cso->m_cs);
			::InterlockedIncrement(&m_count);
		}
	}

	bool __fastcall tryEnter()
	{
		if (m_cso)
		{
			if (m_count > 0)
				return false;
			if (::TryEnterCriticalSection(&m_cso->m_cs) == FALSE)
				return false;
			::InterlockedIncrement(&m_count);
		}
		return true;
	}

	void __fastcall leave()
	{
		if (m_cso && m_count > 0)
		{
			::InterlockedDecrement(&m_count);
			::LeaveCriticalSection(&m_cso->m_cs);
		}
	}
};

#endif


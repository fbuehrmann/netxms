/*
** NetXMS - Network Management System
** Log Parsing Library
** Copyright (C) 2003-2020 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: file.cpp
**
**/

#include "libnxlp.h"
#include <nxstat.h>

#ifdef _WIN32
#include <share.h>
#include <comdef.h>
#endif

/**
 * Constants
 */
#define READ_BUFFER_SIZE      4096

/**
 * Find byte sequence in the stream
 */
static char *FindSequence(char *start, int length, const char *sequence, int seqLength)
{
	char *curr = start;
	int count = 0;
	while(length - count >= seqLength)
	{
		if (!memcmp(curr, sequence, seqLength))
			return curr;
		curr += seqLength;
		count += seqLength;
	}
	return NULL;
}

/**
 * Find end-of-line marker
 */
static char *FindEOL(char *start, int length, int encoding)
{
   char *eol = NULL;
	switch(encoding)
	{
		case LP_FCP_UCS2:
#if WORDS_BIGENDIAN
			eol = FindSequence(start, length, "\0\n", 2);
#else
			eol = FindSequence(start, length, "\n\0", 2);
#endif
			break;
		case LP_FCP_UCS2_LE:
			eol = FindSequence(start, length, "\n\0", 2);
			break;
		case LP_FCP_UCS2_BE:
			eol = FindSequence(start, length, "\0\n", 2);
			break;
		case LP_FCP_UCS4:
#if WORDS_BIGENDIAN
         eol = FindSequence(start, length, "\0\0\0\n", 4);
#else
         eol = FindSequence(start, length, "\n\0\0\0", 4);
#endif
		   break;
		case LP_FCP_UCS4_LE:
			eol = FindSequence(start, length, "\n\0\0\0", 4);
			break;
		case LP_FCP_UCS4_BE:
			eol = FindSequence(start, length, "\0\0\0\n", 4);
			break;
		default:
			eol = (char *)memchr(start, '\n', length);
			break;
	}

   if (eol == NULL)
   {
      // Try to find CR
	   switch(encoding)
	   {
		case LP_FCP_UCS2:
#if WORDS_BIGENDIAN
			eol = FindSequence(start, length, "\0\r", 2);
#else
			eol = FindSequence(start, length, "\r\0", 2);
#endif
			break;
		   case LP_FCP_UCS2_LE:
			   eol = FindSequence(start, length, "\r\0", 2);
				break;
		   case LP_FCP_UCS2_BE:
			   eol = FindSequence(start, length, "\0\r", 2);
				break;
		case LP_FCP_UCS4:
#if WORDS_BIGENDIAN
         eol = FindSequence(start, length, "\0\0\0\r", 4);
#else
         eol = FindSequence(start, length, "\r\0\0\0", 4);
#endif
		   break;
		   case LP_FCP_UCS4_LE:
			   eol = FindSequence(start, length, "\r\0\0\0", 4);
				break;
		   case LP_FCP_UCS4_BE:
			   eol = FindSequence(start, length, "\0\0\0\r", 4);
				break;
		   default:
			   eol = (char *)memchr(start, '\r', length);
				break;
	   }
   }

   return eol;
}

/**
 * Parse new log records
 */
static off_t ParseNewRecords(LogParser *parser, int fh)
{
   char *ptr, *eptr, buffer[READ_BUFFER_SIZE];
   int bytes, bufPos = 0;
   off_t resetPos;
	int encoding = parser->getFileEncoding();
	TCHAR text[READ_BUFFER_SIZE];

   do
   {
      resetPos = _lseek(fh, 0, SEEK_CUR);
      if ((bytes = _read(fh, &buffer[bufPos], READ_BUFFER_SIZE - bufPos)) > 0)
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Read %d bytes into buffer at offset %d"), bytes, bufPos);
         bytes += bufPos;
         for(ptr = buffer;; ptr = eptr + 1)
         {
            bufPos = (int)(ptr - buffer);
				eptr = FindEOL(ptr, bytes - bufPos, encoding);
            if (eptr == NULL)
            {
					int remaining = bytes - bufPos;
               resetPos = _lseek(fh, 0, SEEK_CUR) - remaining;
					if (remaining > 0)
					{
					   if (buffer != ptr)
					      memmove(buffer, ptr, remaining);
                  if (parser->isFilePreallocated() && !memcmp(buffer, "\x00\x00\x00\x00", std::min(remaining, 4)))
                  {
                     // Found zeroes in preallocated file, next read should be after last known EOL
                     return resetPos;
                  }
					}
					bufPos = remaining;
               break;
            }
				// remove possible CR character and put 0 to indicate end of line
				switch(encoding)
				{
					case LP_FCP_UCS2:
#if WORDS_BIGENDIAN
                  if ((eptr - ptr >= 2) && !memcmp(eptr - 2, "\0\r", 2))
							eptr -= 2;
						*eptr = 0;
						*(eptr + 1) = 0;
#else

						if ((eptr - ptr >= 2) && !memcmp(eptr - 2, "\r\0", 2))
							eptr -= 2;
						*eptr = 0;
						*(eptr + 1) = 0;
#endif
						break;
					case LP_FCP_UCS2_LE:
						if ((eptr - ptr >= 2) && !memcmp(eptr - 2, "\r\0", 2))
							eptr -= 2;
						*eptr = 0;
						*(eptr + 1) = 0;
						break;
					case LP_FCP_UCS2_BE:
						if ((eptr - ptr >= 2) && !memcmp(eptr - 2, "\0\r", 2))
							eptr -= 2;
						*eptr = 0;
						*(eptr + 1) = 0;
						break;
					case LP_FCP_UCS4:
#if WORDS_BIGENDIAN
                 if ((eptr - ptr >= 4) && !memcmp(eptr - 4, "\0\0\0\r", 4))
							eptr -= 4;
						memset(eptr, 0, 4);
#else

						if ((eptr - ptr >= 4) && !memcmp(eptr - 4, "\r\0\0\0", 4))
							eptr -= 4;
						memset(eptr, 0, 4);
#endif
						break;
					case LP_FCP_UCS4_LE:
						if ((eptr - ptr >= 4) && !memcmp(eptr - 4, "\r\0\0\0", 4))
							eptr -= 4;
						memset(eptr, 0, 4);
						break;
					case LP_FCP_UCS4_BE:
						if ((eptr - ptr >= 4) && !memcmp(eptr - 4, "\0\0\0\r", 4))
							eptr -= 4;
						memset(eptr, 0, 4);
						break;
					default:
						if ((eptr - ptr >= 1) && *(eptr - 1) == '\r')
							eptr--;
						*eptr = 0;
						break;
				}
				// Now ptr points to null-terminated string in original encoding
				// Do the conversion to platform encoding
#ifdef UNICODE
				switch(encoding)
				{
					case LP_FCP_ACP:
						MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, ptr, -1, text, READ_BUFFER_SIZE);
						break;
					case LP_FCP_UTF8:
						MultiByteToWideChar(CP_UTF8, 0, ptr, -1, text, READ_BUFFER_SIZE);
						break;
               case LP_FCP_UCS2_LE:
#if WORDS_BIGENDIAN
                  bswap_array_16((UINT16 *)ptr, -1);
#endif
#ifdef UNICODE_UCS2
						wcslcpy(text, (WCHAR *)ptr, READ_BUFFER_SIZE);
#else
                  ucs2_to_ucs4((UCS2CHAR *)ptr, -1, text, READ_BUFFER_SIZE);
#endif
                  break;
               case LP_FCP_UCS2_BE:
#if !WORDS_BIGENDIAN
                  bswap_array_16((UINT16 *)ptr, -1);
#endif
#ifdef UNICODE_UCS2
						wcslcpy(text, (WCHAR *)ptr, READ_BUFFER_SIZE);
#else
                  ucs2_to_ucs4((UCS2CHAR *)ptr, -1, text, READ_BUFFER_SIZE);
#endif
                  break;
               case LP_FCP_UCS2:
#ifdef UNICODE_UCS2
						wcslcpy(text, (WCHAR *)ptr, READ_BUFFER_SIZE);
#else
                  ucs2_to_ucs4((UCS2CHAR *)ptr, -1, text, READ_BUFFER_SIZE);
#endif
                  break;
               case LP_FCP_UCS4_LE:
#if WORDS_BIGENDIAN
                  bswap_array_32((UINT32 *)ptr, -1);
#endif
#ifdef UNICODE_UCS2
                  ucs4_to_ucs2((UCS4CHAR *)ptr, -1, text, READ_BUFFER_SIZE);
#else
                  wcslcpy(text, (WCHAR *)ptr, READ_BUFFER_SIZE);
#endif
                  break;
               case LP_FCP_UCS4_BE:
#if !WORDS_BIGENDIAN
                  bswap_array_32((UINT32 *)ptr, -1);
#endif
#ifdef UNICODE_UCS2
                  ucs4_to_ucs2((UCS4CHAR *)ptr, -1, text, READ_BUFFER_SIZE);
#else
                  wcslcpy(text, (WCHAR *)ptr, READ_BUFFER_SIZE);
#endif
                  break;
               case LP_FCP_UCS4:
#ifdef UNICODE_UCS2
                  ucs4_to_ucs2((UCS4CHAR *)ptr, -1, text, READ_BUFFER_SIZE);
#else
                  wcslcpy(text, (WCHAR *)ptr, READ_BUFFER_SIZE);
#endif
                  break;
					default:
						break;
				}
#else
				switch(encoding)
				{
					case LP_FCP_ACP:
						_tcslcpy(text, ptr, READ_BUFFER_SIZE);
						break;
               case LP_FCP_UTF8:
                  utf8_to_mb(ptr, -1, text, READ_BUFFER_SIZE);
                  break;
               case LP_FCP_UCS2_LE:
#if WORDS_BIGENDIAN
                  bswap_array_16((UINT16 *)ptr, -1);
#endif
                  ucs2_to_mb((UCS2CHAR *)ptr, -1, text, READ_BUFFER_SIZE);
                  break;
               case LP_FCP_UCS2_BE:
#if !WORDS_BIGENDIAN
                  bswap_array_16((UINT16 *)ptr, -1);
#endif
                  ucs2_to_mb((UCS2CHAR *)ptr, -1, text, READ_BUFFER_SIZE);
                  break;
               case LP_FCP_UCS2:
                  ucs2_to_mb((UCS2CHAR *)ptr, -1, text, READ_BUFFER_SIZE);
                  break;
               case LP_FCP_UCS4_LE:
#if WORDS_BIGENDIAN
                  bswap_array_32((UINT32 *)ptr, -1);
#endif
                  ucs4_to_mb((UCS4CHAR *)ptr, -1, text, READ_BUFFER_SIZE);
                  break;
               case LP_FCP_UCS4_BE:
#if !WORDS_BIGENDIAN
                  bswap_array_32((UINT32 *)ptr, -1);
#endif
                  ucs4_to_mb((UCS4CHAR *)ptr, -1, text, READ_BUFFER_SIZE);
                  break;
               case LP_FCP_UCS4:
                  ucs4_to_mb((UCS4CHAR *)ptr, -1, text, READ_BUFFER_SIZE);
                  break;
					default:
						break;
				}
#endif
				parser->matchLine(text);
         }
      }
      else
      {
         bytes = 0;
      }
   } while(bytes > 0);
   return resetPos;
}

/**
 * Scan first 10 bytes of a file to find its encoding
 */
static int ScanFileEncoding(int fh)
{
   char buffer[10];
   if (_read(fh, buffer, 4) > 3)
   {
      if (!memcmp(buffer, "\x00\x00\xFE\xFF", 4))
         return LP_FCP_UCS4_BE;
      if (!memcmp(buffer, "\xFF\xFE\x00\x00", 4))
         return LP_FCP_UCS4_LE;
      if (!memcmp(buffer, "\xEF\xBB\xBF", 3))
         return LP_FCP_UTF8;
      if (!memcmp(buffer, "\xFE\xFF", 2))
         return LP_FCP_UCS2_BE;
      if (!memcmp(buffer, "\xFF\xFE", 2))
         return LP_FCP_UCS2_LE;
   }

   return LP_FCP_ACP;
}

/**
 * Seek file to the end of zeroes block using given character size
 * Reset position to the beginning of zero block if zeroes are till end of file
 */
template<typename T> bool SkipZeroBlock(int fh)
{
   off_t startPos = _lseek(fh, 0, SEEK_CUR);

   char buffer[4096];
   while(true)
   {
      int bytes = _read(fh, buffer, 4096);
      if (bytes <= 0)
         break;
      T *p = reinterpret_cast<T*>(buffer);
      for(int i = 0; i < bytes - static_cast<int>(sizeof(T)) + 1; i += sizeof(T), p++)
      {
         if (*p != 0)
         {
            off_t pos = _lseek(fh, i - bytes, SEEK_CUR);
            nxlog_debug_tag(DEBUG_TAG, 6, _T("LogParser: end of zero block found at %ld"), (long)pos);
            return true;
         }
      }
   }

   _lseek(fh, startPos, SEEK_SET);
   return false;
}

/**
 * Seek file to the end of zeroes block
 * Reset position to the beginning of zero block if zeroes are till end of file
 */
static bool SkipZeroBlock(int fh, int chsize)
{
   switch(chsize)
   {
      case 1:
         return SkipZeroBlock<char>(fh);
      case 2:
         return SkipZeroBlock<INT16>(fh);
      case 4:
         return SkipZeroBlock<INT32>(fh);
   }
   return false;
}

/**
 * Seek file to the beginning of zeroes block using given character size
 */
template<typename T> bool SeekToZero(int fh)
{
   char buffer[4096];
   while(true)
   {
      int bytes = _read(fh, buffer, 4096);
      if (bytes <= 0)
         break;
      T *p = reinterpret_cast<T*>(buffer);
      for(int i = 0; i < bytes - static_cast<int>(sizeof(T)) + 1; i += sizeof(T), p++)
      {
         if (*p == 0)
         {
            off_t pos = _lseek(fh, i - bytes, SEEK_CUR);
            nxlog_debug_tag(DEBUG_TAG, 6, _T("Beginning of zero block found at %ld"), (long)pos);
            return true;
         }
      }
   }
   return false;
}

/**
 * Seek file to the beginning of zeroes block
 */
static void SeekToZero(int fh, int chsize, bool detectBrokenPrealloc)
{
   bool found;
   switch(chsize)
   {
      case 1:
         found = SeekToZero<char>(fh);
         break;
      case 2:
         found = SeekToZero<INT16>(fh);
         break;
      case 4:
         found = SeekToZero<INT32>(fh);
         break;
      default:
         found = false;
         break;
   }

   if (found && detectBrokenPrealloc)
   {
      if (SkipZeroBlock(fh, chsize))
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("LogParser: broken preallocated file detected"));
         _lseek(fh, 0, SEEK_END); // assume that file has broken preallocation, read from end of file
      }
   }
}

/**
 * File parser thread
 */
bool LogParser::monitorFile()
{
	TCHAR fname[MAX_PATH], temp[MAX_PATH];
	NX_STAT_STRUCT st, stn;
	int fh;
	bool readFromStart = m_rescan;

	if (m_fileName == NULL)
	{
		nxlog_debug_tag(DEBUG_TAG, 0, _T("Parser thread will not start, file name not set"));
		return false;
	}

#ifdef _WIN32
   if (m_useSnapshot)
   {
	   nxlog_debug_tag(DEBUG_TAG, 0, _T("Using VSS snapshots for file \"%s\""), m_fileName);
      return monitorFileWithSnapshot();
   }
#endif

   if (!m_keepFileOpen)
   {
	   nxlog_debug_tag(DEBUG_TAG, 0, _T("LogParser: \"keep open\" option disabled for file \"%s\""), m_fileName);
      return monitorFile2();
   }

	nxlog_debug_tag(DEBUG_TAG, 0, _T("Parser thread for file \"%s\" started"), m_fileName);
	bool exclusionPeriod = false;
	while(true)
	{
	   if (isExclusionPeriod())
	   {
	      if (!exclusionPeriod)
	      {
	         exclusionPeriod = true;
            nxlog_debug_tag(DEBUG_TAG, 6, _T("Will not open file \"%s\" because of exclusion period"), getFileName());
            setStatus(LPS_SUSPENDED);
	      }
         if (ConditionWait(m_stopCondition, 30000))
            break;
         continue;
	   }

	   if (exclusionPeriod)
	   {
	      exclusionPeriod = false;
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Exclusion period for file \"%s\" ended"), getFileName());
	   }

      ExpandFileName(getFileName(), fname, MAX_PATH, true);
		if (CALL_STAT(fname, &st) != 0)
      {
         if (errno == ENOENT)
            readFromStart = true;
         setStatus(LPS_NO_FILE);
         if (ConditionWait(m_stopCondition, 10000))
            break;
         continue;
      }

#ifdef _WIN32
      fh = _tsopen(fname, O_RDONLY, _SH_DENYNO);
#else
		fh = _topen(fname, O_RDONLY);
#endif
		if (fh == -1)
      {
         setStatus(LPS_OPEN_ERROR);
         if (ConditionWait(m_stopCondition, 10000))
            break;
         continue;
      }

		setStatus(LPS_RUNNING);
		nxlog_debug_tag(DEBUG_TAG, 3, _T("File \"%s\" (pattern \"%s\") successfully opened"), fname, m_fileName);

      if (m_fileEncoding == -1)
      {
         m_fileEncoding = ScanFileEncoding(fh);
         _lseek(fh, 0, SEEK_SET);
      }

		size_t size = (size_t)st.st_size;
		time_t mtime = st.st_mtime;
		if (readFromStart)
		{
			nxlog_debug_tag(DEBUG_TAG, 5, _T("Parsing existing records in file \"%s\""), fname);
			off_t resetPos = ParseNewRecords(this, fh);
         _lseek(fh, resetPos, SEEK_SET);
		}
		else if (m_preallocatedFile)
		{
			SeekToZero(fh, getCharSize(), m_detectBrokenPrealloc);
		}
		else
		{
			_lseek(fh, 0, SEEK_END);
		}

		while(true)
		{
			if (ConditionWait(m_stopCondition, 5000))
				goto stop_parser;

			// Check if file name was changed
			ExpandFileName(getFileName(), temp, MAX_PATH, true);
			if (_tcscmp(temp, fname))
			{
				nxlog_debug_tag(DEBUG_TAG, 5, _T("File name change for \"%s\" (\"%s\" -> \"%s\")"), m_fileName, fname, temp);
				readFromStart = true;
				break;
			}

			if (NX_FSTAT(fh, &st) < 0)
			{
				nxlog_debug_tag(DEBUG_TAG, 1, _T("fstat(%d) failed, errno=%d"), fh, errno);
				readFromStart = true;
				break;
			}

			if (CALL_STAT(fname, &stn) < 0)
			{
				nxlog_debug_tag(DEBUG_TAG, 1, _T("stat(%s) failed, errno=%d"), fname, errno);
				readFromStart = true;
				break;
			}

#ifdef _WIN32
			if (st.st_ctime != stn.st_ctime)
			{
				nxlog_debug_tag(DEBUG_TAG, 3, _T("Creation time for fstat(%d) is not equal to creation time for stat(%s), assume file rename"), fh, fname);
				readFromStart = true;
				break;
			}
#else
			if ((st.st_ino != stn.st_ino) || (st.st_dev != stn.st_dev))
			{
				nxlog_debug_tag(DEBUG_TAG, 3, _T("File device or inode differs for stat(%d) and fstat(%s), assume file rename"), fh, fname);
				readFromStart = true;
				break;
			}
#endif

			if (((size_t)st.st_size != size) ||
			    (!m_ignoreMTime && m_rescan && (mtime != st.st_mtime)))
			{
				if (((size_t)st.st_size < size) || m_rescan)
				{
					// File was cleared, start from the beginning
					_lseek(fh, 0, SEEK_SET);
					if (!m_rescan)
					   nxlog_debug_tag(DEBUG_TAG, 3, _T("File \"%s\" st_size < size, assume file rotation"), fname);
				}
				size = (size_t)st.st_size;
				mtime = st.st_mtime;
				nxlog_debug_tag(DEBUG_TAG, 6, _T("New data available in file \"%s\""), fname);
				off_t resetPos = ParseNewRecords(this, fh);
				_lseek(fh, resetPos, SEEK_SET);
			}
			else if (m_preallocatedFile)
			{
				char buffer[4];
				int bytes = _read(fh, buffer, 4);
				if ((bytes == 4) && memcmp(buffer, "\x00\x00\x00\x00", 4))
				{
               _lseek(fh, -4, SEEK_CUR);
	            nxlog_debug_tag(DEBUG_TAG, 6, _T("New data available in file \"%s\""), fname);
	            off_t resetPos = ParseNewRecords(this, fh);
	            _lseek(fh, resetPos, SEEK_SET);
				}
				else
				{
               off_t pos = _lseek(fh, -bytes, SEEK_CUR);
               if (pos > 0)
               {
                  int readSize = std::min(pos, (off_t)4);
                  _lseek(fh, -readSize, SEEK_CUR);
                  int bytes = _read(fh, buffer, readSize);
                  if ((bytes == readSize) && !memcmp(buffer, "\x00\x00\x00\x00", readSize))
                  {
                     nxlog_debug_tag(DEBUG_TAG, 6, _T("Detected reset of preallocated file \"%s\""), fname);
                     _lseek(fh, 0, SEEK_SET);
                     off_t resetPos = ParseNewRecords(this, fh);
                     _lseek(fh, resetPos, SEEK_SET);
                  }
               }
				}
			}

			if (isExclusionPeriod())
			{
            nxlog_debug_tag(DEBUG_TAG, 6, _T("Closing file \"%s\" because of exclusion period"), fname);
            exclusionPeriod = true;
            setStatus(LPS_SUSPENDED);
				break;
			}
		}
		_close(fh);
	}

stop_parser:
   nxlog_debug_tag(DEBUG_TAG, 0, _T("Parser thread for file \"%s\" stopped"), m_fileName);
	return true;
}

/**
 * File parser thread (do not keep it open)
 */
bool LogParser::monitorFile2()
{
   size_t size = 0;
   time_t mtime = 0;
#ifdef _WIN32
   time_t ctime = 0; // creation time on Windows
#endif
   off_t lastPos = 0;
   bool readFromStart = m_rescan;
   bool firstRead = true;

   nxlog_debug_tag(DEBUG_TAG, 0, _T("Parser thread for file \"%s\" started (\"keep open\" option disabled)"), m_fileName);
   bool exclusionPeriod = false;
   while(true)
   {
      if (isExclusionPeriod())
      {
         if (!exclusionPeriod)
         {
            exclusionPeriod = true;
            nxlog_debug_tag(DEBUG_TAG, 6, _T("Will not open file \"%s\" because of exclusion period"), getFileName());
            setStatus(LPS_SUSPENDED);
         }
         if (ConditionWait(m_stopCondition, 30000))
            break;
         continue;
      }

      if (exclusionPeriod)
      {
         exclusionPeriod = false;
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Exclusion period for file \"%s\" ended"), getFileName());
      }

      TCHAR fname[MAX_PATH];
      ExpandFileName(getFileName(), fname, MAX_PATH, true);
      
      NX_STAT_STRUCT st;
      if (CALL_STAT(fname, &st) != 0)
      {
         if (errno == ENOENT)
            readFromStart = true;
         setStatus(LPS_NO_FILE);
         if (ConditionWait(m_stopCondition, 10000))
            break;
         continue;
      }

#ifdef _WIN32
      if (firstRead)
         ctime = st.st_ctime; // prevent incorrect rotation detection on first read
#endif

      if (!readFromStart)
      {
#ifdef _WIN32
         if ((m_ignoreMTime && !m_preallocatedFile && (size == st.st_size) && (ctime == st.st_ctime)) ||
             (!m_ignoreMTime && (size == st.st_size) && (mtime == st.st_mtime) && (ctime == st.st_ctime)))
#else
         if ((m_ignoreMTime && !m_preallocatedFile && (size == st.st_size)) ||
             (!m_ignoreMTime && (size == st.st_size) && (mtime == st.st_mtime)))
#endif
         {
            if (ConditionWait(m_stopCondition, 10000))
               break;
            continue;
         }
      }

#ifdef _WIN32
      int fh = _tsopen(fname, O_RDONLY, _SH_DENYNO);
#else
      int fh = _topen(fname, O_RDONLY);
#endif
      if (fh == -1)
      {
         setStatus(LPS_OPEN_ERROR);
         if (ConditionWait(m_stopCondition, 10000))  // retry in 10 seconds
            break;
         continue;
      }

      setStatus(LPS_RUNNING);
      nxlog_debug_tag(DEBUG_TAG, 7, _T("File \"%s\" (pattern \"%s\") successfully opened"), fname, m_fileName);

#ifdef _WIN32
      if (!readFromStart && ((size > static_cast<size_t>(st.st_size)) || (ctime != st.st_ctime)))
#else
      if (!readFromStart && (size > static_cast<size_t>(st.st_size)))
#endif
      {
#ifdef _WIN32
         nxlog_debug_tag(DEBUG_TAG, 5, _T("File \"%s\" rotation detected (size=%llu/%llu, ctime=%llu/%llu)"),  fname,
            static_cast<UINT64>(size), static_cast<UINT64>(st.st_size), static_cast<UINT64>(ctime), static_cast<UINT64>(st.st_ctime));
         ctime = st.st_ctime;
#else
         nxlog_debug_tag(DEBUG_TAG, 5, _T("File \"%s\" rotation detected (size=%llu/%llu)"),  fname,
            static_cast<UINT64>(size), static_cast<UINT64>(st.st_size));
#endif
         readFromStart = true;   // Assume file rotation
      }

      if (m_fileEncoding == -1)
      {
         m_fileEncoding = ScanFileEncoding(fh);
         _lseek(fh, 0, SEEK_SET);
      }

      if (!readFromStart && !m_rescan)
      {
         if (firstRead)
         {
            if (m_preallocatedFile)
               SeekToZero(fh, getCharSize(), m_detectBrokenPrealloc);
            else
               _lseek(fh, 0, SEEK_END);
            firstRead = false;
         }
         else
         {
            _lseek(fh, lastPos, SEEK_SET);
            char buffer[4];
            int bytes = _read(fh, buffer, 4);
            if ((bytes == 4) && memcmp(buffer, "\x00\x00\x00\x00", 4))
            {
               _lseek(fh, -4, SEEK_CUR);
               nxlog_debug_tag(DEBUG_TAG, 6, _T("New data available in file \"%s\""), fname);
            }
            else
            {
               off_t pos = _lseek(fh, -bytes, SEEK_CUR);
               if (pos > 0)
               {
                  int readSize = std::min(pos, (off_t)4);
                  _lseek(fh, -readSize, SEEK_CUR);
                  int bytes = _read(fh, buffer, readSize);
                  if ((bytes == readSize) && !memcmp(buffer, "\x00\x00\x00\x00", readSize))
                  {
                     nxlog_debug_tag(DEBUG_TAG, 6, _T("Detected reset of preallocated file \"%s\""), fname);
                     _lseek(fh, 0, SEEK_SET);
                  }
               }
            }
         }
      }
      readFromStart = false;

      lastPos = ParseNewRecords(this, fh);
      _close(fh);
      size = static_cast<size_t>(st.st_size);
      mtime = st.st_mtime;

      if (ConditionWait(m_stopCondition, 10000))
         break;
   }

   nxlog_debug_tag(DEBUG_TAG, 0, _T("Parser thread for file \"%s\" stopped"), m_fileName);
   return true;
}

#ifdef _WIN32

/**
 * File parser thread (using VSS snapshots)
 */
bool LogParser::monitorFileWithSnapshot()
{
   HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
   if (FAILED(hr))
   {
      _com_error err(hr);
      nxlog_debug_tag(DEBUG_TAG, 0, _T("Parser thread will not start, COM initialization failed (%s)"), err.ErrorMessage());
      return false;
   }

   size_t size = 0;
   time_t mtime = 0;
   time_t ctime = 0;
   off_t lastPos = 0;
   bool readFromStart = m_rescan;
   bool firstRead = true;

   nxlog_debug_tag(DEBUG_TAG, 0, _T("Parser thread for file \"%s\" started (using VSS snapshots)"), m_fileName);
   bool exclusionPeriod = false;
   while(true)
   {
      if (isExclusionPeriod())
      {
         if (!exclusionPeriod)
         {
            exclusionPeriod = true;
            nxlog_debug_tag(DEBUG_TAG, 6, _T("Will not open file \"%s\" because of exclusion period"), getFileName());
            setStatus(LPS_SUSPENDED);
         }
         if (ConditionWait(m_stopCondition, 30000))
            break;
         continue;
      }

      if (exclusionPeriod)
      {
         exclusionPeriod = false;
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Exclusion period for file \"%s\" ended"), getFileName());
      }

      TCHAR fname[MAX_PATH];
      ExpandFileName(getFileName(), fname, MAX_PATH, true);
      
      NX_STAT_STRUCT st;
      if (CALL_STAT(fname, &st) != 0)
      {
         setStatus(LPS_NO_FILE);
         if (ConditionWait(m_stopCondition, 10000))
            break;
         continue;
      }

      if (firstRead)
         ctime = st.st_ctime; // prevent incorrect rotation detection on first read

      if ((size == st.st_size) && (mtime == st.st_mtime) && (ctime == st.st_ctime) && !readFromStart)
      {
         if (ConditionWait(m_stopCondition, 10000))
            break;
         continue;
      }

      FileSnapshot *snapshot = CreateFileSnapshot(fname);
      if (snapshot == NULL)
      {
         setStatus(LPS_VSS_FAILURE);
         if (ConditionWait(m_stopCondition, 30000))  // retry in 30 seconds
            break;
         continue;
      }

      int fh = _tsopen(snapshot->name, O_RDONLY, _SH_DENYNO);
      if (fh == -1)
      {
         DestroyFileSnapshot(snapshot);
         setStatus(LPS_OPEN_ERROR);
         if (ConditionWait(m_stopCondition, 10000))  // retry in 10 seconds
            break;
         continue;
      }

      setStatus(LPS_RUNNING);
      nxlog_debug_tag(DEBUG_TAG, 7, _T("File \"%s\" (pattern \"%s\", snapshot \"%s\") successfully opened"), fname, m_fileName, snapshot->name);

      if ((size > static_cast<size_t>(st.st_size)) || (ctime != st.st_ctime))
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("File \"%s\" rotation detected (size=%llu/%llu, ctime=%llu/%llu)"),  fname,
            static_cast<UINT64>(size), static_cast<UINT64>(st.st_size), static_cast<UINT64>(ctime), static_cast<UINT64>(st.st_ctime));
         readFromStart = true;   // Assume file rotation
         ctime = st.st_ctime;
      }

      if (m_fileEncoding == -1)
      {
         m_fileEncoding = ScanFileEncoding(fh);
         _lseek(fh, 0, SEEK_SET);
      }

      if (!readFromStart && !m_rescan)
      {
         if (firstRead)
         {
            if (m_preallocatedFile)
               SeekToZero(fh, getCharSize(), m_detectBrokenPrealloc);
            else
               _lseek(fh, 0, SEEK_END);
            firstRead = false;
         }
         else
         {
            _lseek(fh, lastPos, SEEK_SET);
         }
      }
      readFromStart = false;

      lastPos = ParseNewRecords(this, fh);
      _close(fh);
      size = static_cast<size_t>(st.st_size);
      mtime = st.st_mtime;

      DestroyFileSnapshot(snapshot);
      if (ConditionWait(m_stopCondition, 10000))
         break;
   }

   CoUninitialize();
   nxlog_debug_tag(DEBUG_TAG, 0, _T("Parser thread for file \"%s\" stopped"), m_fileName);
   return true;
}

#endif   /* _WIN32 */

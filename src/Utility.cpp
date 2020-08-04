#include "stdafx.h"
#include "scheme.h"
#include <gdiplus.h>
#include <string>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <functional>
#include <chrono>
#include <future>
 
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
extern "C" void appendTranscriptNL(char* s);
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid")

#define WinGetObject GetObjectW
#define WinSendMessage SendMessageW


#define CALL0(who) Scall0(Stop_level_value(Sstring_to_symbol(who)))
#define CALL1(who, arg) Scall1(Stop_level_value(Sstring_to_symbol(who)), arg)
#define CALL2(who, arg, arg2) Scall2(Stop_level_value(Sstring_to_symbol(who)), arg, arg2)
 

template <class callable, class... arguments>
void later(int after, bool async, callable f, arguments&&... args) {
	std::function<typename std::result_of < callable(arguments...)>::type() > task(std::bind(std::forward<callable>(f), std::forward<arguments>(args)...));

	if (async) {
		std::thread([after, task]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(after));
			task();
			}).detach();
	}
	else {
		std::this_thread::sleep_for(std::chrono::milliseconds(after));
		task();
	}
}

template <class T> void SafeRelease(T** ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

#define bank_size 512


namespace Text
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Text conversions
	// Text (UTF8) processing functions.


	typedef unsigned int UTF32;
	typedef unsigned short UTF16;
	typedef unsigned char UTF8;
	typedef unsigned char Boolean;

	static const unsigned char total_bytes[256] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6
	};

	static const char trailing_bytes_for_utf8[256] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5
	};

	static const UTF8 first_byte_mark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

	static const UTF32 offsets_from_utf8[6] = {
		0x00000000UL, 0x00003080UL, 0x000E2080UL,
		0x03C82080UL, 0xFA082080UL, 0x82082080UL
	};

	typedef enum
	{
		conversionOK,
		/* conversion successful */
		sourceExhausted,
		/* partial character in source, but hit end */
		targetExhausted,
		/* insuff. room in target for conversion */
		sourceIllegal /* source sequence is illegal/malformed */
	} ConversionResult;

	typedef enum
	{
		strictConversion = 0,
		lenientConversion
	} conversion_flags;

	/* Some fundamental constants */
#define UNI_REPLACEMENT_CHAR (UTF32)0x0000FFFD
#define UNI_MAX_BMP (UTF32)0x0000FFFF
#define UNI_MAX_UTF16 (UTF32)0x0010FFFF
#define UNI_MAX_UTF32 (UTF32)0x7FFFFFFF
#define UNI_MAX_LEGAL_UTF32 (UTF32)0x0010FFFF

	static const int halfShift = 10;
	static const UTF32 halfBase = 0x0010000UL;
	static const UTF32 halfMask = 0x3FFUL;

#define UNI_SUR_HIGH_START (UTF32)0xD800
#define UNI_SUR_HIGH_END (UTF32)0xDBFF
#define UNI_SUR_LOW_START (UTF32)0xDC00
#define UNI_SUR_LOW_END (UTF32)0xDFFF
#define false 0
#define true 1

	/* is c the start of a utf8 sequence? */
#define ISUTF8(c) (((c)&0xC0) != 0x80)
#define ISASCII(ch) ((unsigned char)ch < 0x80)

#define UTFmax 4
	typedef char32_t Rune;
#define RUNE_C(x) x##L
#define Runeself 0x80
#define Runemax RUNE_C(0x10FFFF)

	int runelen(Rune r)
	{
		if (r <= 0x7F)
			return 1;
		else if (r <= 0x07FF)
			return 2;
		else if (r <= 0xD7FF)
			return 3;
		else if (r <= 0xDFFF)
			return 0; /* surrogate character */
		else if (r <= 0xFFFD)
			return 3;
		else if (r <= 0xFFFF)
			return 0; /* illegal character */
		else if (r <= Runemax)
			return 4;
		else
			return 0; /* rune too large */
	}


	int strlen_utf8(const char* s)
	{
		auto i = 0, j = 0;
		while (s[i])
		{
			if ((s[i] & 0xc0) != 0x80) j++;
			i++;
		}
		return j;
	}


	int runetochar(char* s, const Rune* p)
	{
		const auto r = *p;

		switch (runelen(r)) {
		case 1: /* 0aaaaaaa */
			s[0] = r;
			return 1;
		case 2: /* 00000aaa aabbbbbb */
			s[0] = 0xC0 | ((r & 0x0007C0) >> 6); /* 110aaaaa */
			s[1] = 0x80 | (r & 0x00003F);        /* 10bbbbbb */
			return 2;
		case 3: /* aaaabbbb bbcccccc */
			s[0] = 0xE0 | ((r & 0x00F000) >> 12); /* 1110aaaa */
			s[1] = 0x80 | ((r & 0x000FC0) >> 6); /* 10bbbbbb */
			s[2] = 0x80 | (r & 0x00003F);        /* 10cccccc */
			return 3;
		case 4: /* 000aaabb bbbbcccc ccdddddd */
			s[0] = 0xF0 | ((r & 0x1C0000) >> 18); /* 11110aaa */
			s[1] = 0x80 | ((r & 0x03F000) >> 12); /* 10bbbbbb */
			s[2] = 0x80 | ((r & 0x000FC0) >> 6); /* 10cccccc */
			s[3] = 0x80 | (r & 0x00003F);        /* 10dddddd */
			return 4;
		default:
			return 0; /* error */
		}
	}
	// some functions from unicode.inc

	ConversionResult ConvertUTF16toUTF8(
		const UTF16** sourceStart, const UTF16* sourceEnd,
		UTF8** targetStart, UTF8* targetEnd, conversion_flags flags)
	{
		ConversionResult result = conversionOK;
		const UTF16* source = *sourceStart;
		UTF8* target = *targetStart;
		while (source < sourceEnd)
		{
			UTF32 ch;
			unsigned short bytesToWrite = 0;
			const UTF32 byteMask = 0xBF;
			const UTF32 byteMark = 0x80;
			const UTF16* oldSource = source; /* In case we have to back up because of target overflow. */
			ch = *source++;

			/* If we have a surrogate pair, convert to UTF32 first. */
			if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END)
			{
				/* If the 16 bits following the high surrogate are in the source buffer... */
				if (source < sourceEnd)
				{
					UTF32 ch2 = *source;
					/* If it's a low surrogate, convert to UTF32. */
					if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END)
					{
						ch = ((ch - UNI_SUR_HIGH_START) << halfShift) + (ch2 - UNI_SUR_LOW_START) + halfBase;
						++source;
					}
					else if (flags == strictConversion)
					{
						/* it's an unpaired high surrogate */
						--source; /* return to the illegal value itself */
						result = sourceIllegal;
						break;
					}
				}
				else
				{
					/* We don't have the 16 bits following the high surrogate. */
					--source; /* return to the high surrogate */
					result = sourceExhausted;
					break;
				}
			}
			else if (flags == strictConversion)
			{
				/* UTF-16 surrogate values are illegal in UTF-32 */
				if (ch >= UNI_SUR_LOW_START && ch <= UNI_SUR_LOW_END)
				{
					--source; /* return to the illegal value itself */
					result = sourceIllegal;
					break;
				}
			}
			/* Figure out how many bytes the result will require */
			if (ch < static_cast<UTF32>(0x80))
			{
				bytesToWrite = 1;
			}
			else if (ch < static_cast<UTF32>(0x800))
			{
				bytesToWrite = 2;
			}
			else if (ch < static_cast<UTF32>(0x10000))
			{
				bytesToWrite = 3;
			}
			else if (ch < static_cast<UTF32>(0x110000))
			{
				bytesToWrite = 4;
			}
			else
			{
				bytesToWrite = 3;
				ch = UNI_REPLACEMENT_CHAR;
			}

			target += bytesToWrite;
			if (target > targetEnd)
			{
				source = oldSource; /* Back up source pointer! */
				target -= bytesToWrite;
				result = targetExhausted;
				break;
			}
			switch (bytesToWrite)
			{
				/* note: everything falls through. */
			case 4:
				*--target = static_cast<UTF8>((ch | byteMark) & byteMask);
				ch >>= 6;
			case 3:
				*--target = static_cast<UTF8>((ch | byteMark) & byteMask);
				ch >>= 6;
			case 2:
				*--target = static_cast<UTF8>((ch | byteMark) & byteMask);
				ch >>= 6;
			case 1:
				*--target = static_cast<UTF8>(ch | first_byte_mark[bytesToWrite]);
			}
			target += bytesToWrite;
		}
		*sourceStart = source;
		*targetStart = target;
		return result;
	}

	static Boolean is_legal_utf8(const UTF8* source, int length)
	{
		UTF8 a;
		const UTF8* srcptr = source + length;
		switch (length)
		{
		default:
			return false;
			/* Everything else falls through when "true"... */
		case 4:
			if ((a = (*--srcptr)) < 0x80 || a > 0xBF)
				return false;
		case 3:
			if ((a = (*--srcptr)) < 0x80 || a > 0xBF)
				return false;
		case 2:
			if ((a = (*--srcptr)) > 0xBF)
				return false;

			switch (*source)
			{
				/* no fall-through in this inner switch */
			case 0xE0:
				if (a < 0xA0)
					return false;
				break;
			case 0xED:
				if (a > 0x9F)
					return false;
				break;
			case 0xF0:
				if (a < 0x90)
					return false;
				break;
			case 0xF4:
				if (a > 0x8F)
					return false;
				break;
			default:
				if (a < 0x80)
					return false;
			}
		case 1:
			if (*source >= 0x80 && *source < 0xC2)
				return false;
		}
		if (*source > 0xF4)
			return false;
		return true;
	}

	Boolean is_legal_utf8_sequence(const UTF8* source, const UTF8* sourceEnd)
	{
		const int length = trailing_bytes_for_utf8[*source] + 1;
		if (source + length > sourceEnd)
		{
			return false;
		}
		return is_legal_utf8(source, length);
	}

	ConversionResult ConvertUTF8toUTF16(
		const UTF8** sourceStart, const UTF8* sourceEnd,
		UTF16** targetStart, const UTF16* targetEnd, conversion_flags flags);

	ConversionResult ConvertUTF32toUTF8(
		const UTF32** sourceStart, const UTF32* sourceEnd,
		UTF8** targetStart, UTF8* targetEnd, conversion_flags flags)
	{
		ConversionResult result = conversionOK;
		const UTF32* source = *sourceStart;
		UTF8* target = *targetStart;
		while (source < sourceEnd)
		{
			UTF32 ch;
			unsigned short bytesToWrite = 0;
			const UTF32 byteMask = 0xBF;
			const UTF32 byteMark = 0x80;
			ch = *source++;
			if (flags == strictConversion)
			{
				/* UTF-16 surrogate values are illegal in UTF-32 */
				if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END)
				{
					--source; /* return to the illegal value itself */
					result = sourceIllegal;
					break;
				}
			}
			/*
			* Figure out how many bytes the result will require. Turn any
			* illegally large UTF32 things (> Plane 17) into replacement chars.
			*/
			if (ch < static_cast<UTF32>(0x80))
			{
				bytesToWrite = 1;
			}
			else if (ch < static_cast<UTF32>(0x800))
			{
				bytesToWrite = 2;
			}
			else if (ch < static_cast<UTF32>(0x10000))
			{
				bytesToWrite = 3;
			}
			else if (ch <= UNI_MAX_LEGAL_UTF32)
			{
				bytesToWrite = 4;
			}
			else
			{
				bytesToWrite = 3;
				ch = UNI_REPLACEMENT_CHAR;
				result = sourceIllegal;
			}
			target += bytesToWrite;
			if (target > targetEnd)
			{
				--source; /* Back up source pointer! */
				target -= bytesToWrite;
				result = targetExhausted;
				break;
			}
			switch (bytesToWrite)
			{
				/* note: everything falls through. */
			case 4:
				*--target = static_cast<UTF8>((ch | byteMark) & byteMask);
				ch >>= 6;
			case 3:
				*--target = static_cast<UTF8>((ch | byteMark) & byteMask);
				ch >>= 6;
			case 2:
				*--target = static_cast<UTF8>((ch | byteMark) & byteMask);
				ch >>= 6;
			case 1:
				*--target = static_cast<UTF8>(ch | first_byte_mark[bytesToWrite]);
			}
			target += bytesToWrite;
		}
		*sourceStart = source;
		*targetStart = target;
		return result;
	}

	ConversionResult ConvertUTF8toUTF32(
		const UTF8** source_start, const UTF8* source_end,
		UTF32** target_start, UTF32* target_end, conversion_flags flags)
	{
		ConversionResult result = conversionOK;
		const UTF8* source = *source_start;
		auto target = *target_start;
		while (source < source_end)
		{
			UTF32 ch = 0;
			const unsigned short extra_bytes_to_read = trailing_bytes_for_utf8[*source];
			if (source + extra_bytes_to_read >= source_end)
			{
				result = sourceExhausted;
				break;
			}
			/* Do this check whether lenient or strict */
			if (!is_legal_utf8(source, extra_bytes_to_read + 1))
			{
				result = sourceIllegal;
				break;
			}
			/*
			* The cases all fall through.
			*/
			switch (extra_bytes_to_read)
			{
			case 5:
				ch += *source++;
				ch <<= 6;
			case 4:
				ch += *source++;
				ch <<= 6;
			case 3:
				ch += *source++;
				ch <<= 6;
			case 2:
				ch += *source++;
				ch <<= 6;
			case 1:
				ch += *source++;
				ch <<= 6;
			case 0:
				ch += *source++;
			}
			ch -= offsets_from_utf8[extra_bytes_to_read];

			if (target >= target_end)
			{
				source -= (extra_bytes_to_read + 1); /* Back up the source pointer! */
				result = targetExhausted;
				break;
			}
			if (ch <= UNI_MAX_LEGAL_UTF32)
			{
				/*
				* UTF-16 surrogate values are illegal in UTF-32, and anything
				* over Plane 17 (> 0x10FFFF) is illegal.
				*/
				if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END)
				{
					if (flags == strictConversion)
					{
						source -= (extra_bytes_to_read + 1); /* return to the illegal value itself */
						result = sourceIllegal;
						break;
					}
					else
					{
						*target++ = UNI_REPLACEMENT_CHAR;
					}
				}
				else
				{
					*target++ = ch;
				}
			}
			else
			{
				/* i.e., ch > UNI_MAX_LEGAL_UTF32 */
				result = sourceIllegal;
				*target++ = UNI_REPLACEMENT_CHAR;
			}
		}
		*source_start = source;
		*target_start = target;
		return result;
	}

	// Runes UTF8

	int rune_length(const Rune r)
	{
		if (r <= 0x7F)
			return 1;
		else if (r <= 0x07FF)
			return 2;
		else if (r <= 0xD7FF)
			return 3;
		else if (r <= 0xDFFF)
			return 0; /* surrogate character */
		else if (r <= 0xFFFD)
			return 3;
		else if (r <= 0xFFFF)
			return 0; /* illegal character */
		else if (r <= Runemax)
			return 4;
		else
			return 0; /* rune too large */
	}

	int char_from_rune(char* s, const Rune* p)
	{
		auto r = *p;

		switch (rune_length(r))
		{
		case 1: /* 0aaaaaaa */
			s[0] = r;
			return 1;
		case 2: /* 00000aaa aabbbbbb */
			s[0] = 0xC0 | ((r & 0x0007C0) >> 6); /* 110aaaaa */
			s[1] = 0x80 | (r & 0x00003F); /* 10bbbbbb */
			return 2;
		case 3: /* aaaabbbb bbcccccc */
			s[0] = 0xE0 | ((r & 0x00F000) >> 12); /* 1110aaaa */
			s[1] = 0x80 | ((r & 0x000FC0) >> 6); /* 10bbbbbb */
			s[2] = 0x80 | (r & 0x00003F); /* 10cccccc */
			return 3;
		case 4: /* 000aaabb bbbbcccc ccdddddd */
			s[0] = 0xF0 | ((r & 0x1C0000) >> 18); /* 11110aaa */
			s[1] = 0x80 | ((r & 0x03F000) >> 12); /* 10bbbbbb */
			s[2] = 0x80 | ((r & 0x000FC0) >> 6); /* 10cccccc */
			s[3] = 0x80 | (r & 0x00003F); /* 10dddddd */
			return 4;
		default:
			return 0; /* error */
		}
	}

	int utf8_string_length(const char* s)
	{
		auto i = 0, j = 0;
		while (s[i])
		{
			if ((s[i] & 0xc0) != 0x80)
				j++;
			i++;
		}
		return j;
	}

	// returns a list of UTF decoded strings e.g. from a line of a CSV file.
	// generally input data is UTF8.
	//
	ptr utf8_string_separated_to_list(char* s, const char sep)
	{
		ptr lst = Snil;

		unsigned int byte2;

		if (s == nullptr)
		{
			return Sstring("");
		}
		const auto ll = utf8_string_length(s);
		if (ll == 0)
		{
			return Sstring("");
		}

		const auto cps = static_cast<int*>(calloc(ll + 20, sizeof(int)));
		auto cpsptr = cps;

		const char* cptr = s;
		unsigned int byte = static_cast<unsigned char>(*cptr++);

		bool enquoted = false;
		auto quotes = 0;
		bool eol = false;

		while (byte != 0)
		{
			// handle separated field.
			if (byte == '"')
			{
				enquoted = !enquoted;
			}

			if (!enquoted && byte == sep)
			{
				// we are looking backwards now into our code point array
				auto run = (cpsptr - cps);
				auto jlen = run - 1;

				// reduce run length for cr/lf as we skip these.
				for (auto j = 0; j <= jlen; j++)
				{
					if (cps[j] == '\n' || cps[j] == '\r')
					{
						run--;
						eol = true;
					}
				}

				auto j = 0;

				if (cps[j] == '"')
				{
					j++;
					run--;
					run--;
				}

				// detect comma; eat spaces until quote
				if (cps[j] == sep)
				{
					j++;
					run--;
					while (cps[j] == ' ' && j < jlen)
					{
						j++;
						run--;
					}
				}

				if (cps[j] == '"')
				{
					j++;
					run--;
					run--;
				}

				if (run > 0)
				{
					ptr ss = Sstring_of_length("", run);
					for (auto k = 0; k < run; k++)
					{
						Sstring_set(ss, k, cps[j++]);
					}
					lst = CALL2("cons", ss, lst);
				}
				else if (!eol)
				{
					// null string - avoids turning \r into ""
					lst = CALL2("cons", Sstring(""), lst);
				}

				// reset cpsptr
				cpsptr = cps;
				memset(cps, '*', sizeof(int));
			}

			// ascii
			if (byte < 0x80)
			{
				*cpsptr++ = byte;
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			if (byte < 0xC0)
			{
				*cpsptr++ = byte;
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			// skip
			while ((byte < 0xC0) && (byte >= 0x80))
			{
				byte = static_cast<unsigned char>(*cptr++);
			}

			if (byte < 0xE0)
			{
				byte2 = static_cast<unsigned char>(*cptr++);

				if ((byte2 & 0xC0) == 0x80)
				{
					byte = (((byte & 0x1F) << 6) | (byte2 & 0x3F));
					*cpsptr++ = byte;
					byte = static_cast<unsigned char>(*cptr++);
				}
				continue;
			}

			if (byte < 0xF0)
			{
				byte2 = static_cast<unsigned char>(*cptr++);
				if (byte2 == 0)
				{
					break;
				};
				const unsigned int byte3 = static_cast<unsigned char>(*cptr++);
				if (byte3 == 0)
				{
					break;
				};

				if (((byte2 & 0xC0) == 0x80) && ((byte3 & 0xC0) == 0x80))
				{
					byte = (((byte & 0x0F) << 12) | ((byte2 & 0x3F) << 6) | (byte3 & 0x3F));
					*cpsptr++ = byte;
					byte = static_cast<unsigned char>(*cptr++);
				}
				continue;
			}

			auto trail = total_bytes[byte] - 1; // expected number of trail bytes
			if (trail > 0)
			{
				int ch = byte & (0x3F >> trail);
				do
				{
					byte2 = static_cast<unsigned char>(*cptr++);
					if ((byte2 & 0xC0) != 0x80)
					{
						*cpsptr++ = byte;
						byte = static_cast<unsigned char>(*cptr++);
						continue;
					}
					ch <<= 6;
					ch |= (byte2 & 0x3F);
					trail--;
				} while (trail > 0);
				*cpsptr++ = byte;
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}
			byte = static_cast<unsigned char>(*cptr++);
		}

		// one last record to check for.
		auto run = (cpsptr - cps);
		const auto jlen = run - 1;

		// reduce length for cr/lf as we skip these.

		for (auto j = 0; j <= jlen; j++)
		{
			if (cps[j] == '\n' || cps[j] == '\r')
			{
				run--;
				eol = true;
			}
		}

		auto j = 0;

		if (cps[j] == '"')
		{
			j++;
			run--;
			run--;
		}

		// detect comma; and eat spaces until quote
		if (cps[j] == sep)
		{
			j++;
			run--;
			while (cps[j] == ' ' && j < jlen)
			{
				j++;
				run--;
			}
		}

		if (cps[j] == '"')
		{
			j++;
			run--;
			run--;
		}

		if (run > 0)
		{
			auto ss = Sstring_of_length("", run);

			for (auto k = 0; k < run; k++)
			{
				Sstring_set(ss, k, cps[j++]);
			}

			lst = CALL2("cons", ss, lst);
		}
		else if (!eol)
		{
			// null string
			lst = CALL2("cons", Sstring(""), lst);
		}

		free(cps);
		return lst;
	}

	ptr UTF8toSstring(char* s)
	{
		unsigned int byte2;

		if (s == nullptr)
		{
			return Sstring("");
		}
		const auto ll = utf8_string_length(s);
		if (ll == 0)
		{
			return Sstring("");
		}

		const char* cptr = s;
		unsigned int byte = static_cast<unsigned char>(*cptr++);

		ptr ss = Sstring_of_length("", ll);

		auto i = 0;

		while (byte != 0 && i < ll)
		{
			// ascii
			if (byte < 0x80)
			{
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			if (byte < 0xC0)
			{
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			// skip
			while ((byte < 0xC0) && (byte >= 0x80))
			{
				byte = static_cast<unsigned char>(*cptr++);
			}

			if (byte < 0xE0)
			{
				byte2 = static_cast<unsigned char>(*cptr++);

				if ((byte2 & 0xC0) == 0x80)
				{
					byte = (((byte & 0x1F) << 6) | (byte2 & 0x3F));
					Sstring_set(ss, i++, byte);
					byte = static_cast<unsigned char>(*cptr++);
				}
				continue;
			}

			if (byte < 0xF0)
			{
				byte2 = static_cast<unsigned char>(*cptr++);
				if (byte2 == 0)
				{
					break;
				};
				unsigned int byte3 = static_cast<unsigned char>(*cptr++);
				if (byte3 == 0)
				{
					break;
				};

				if (((byte2 & 0xC0) == 0x80) && ((byte3 & 0xC0) == 0x80))
				{
					byte = (((byte & 0x0F) << 12) | ((byte2 & 0x3F) << 6) | (byte3 & 0x3F));
					Sstring_set(ss, i++, byte);
					byte = static_cast<unsigned char>(*cptr++);
				}
				continue;
			}

			int trail = total_bytes[byte] - 1; // expected number of trail bytes
			if (trail > 0)
			{
				int ch = byte & (0x3F >> trail);
				do
				{
					byte2 = static_cast<unsigned char>(*cptr++);
					if ((byte2 & 0xC0) != 0x80)
					{
						Sstring_set(ss, i++, byte);
						byte = static_cast<unsigned char>(*cptr++);
						continue;
					}
					ch <<= 6;
					ch |= (byte2 & 0x3F);
					trail--;
				} while (trail > 0);
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			// no match..
			if (i == ll)
			{
				break;
			}
			byte = static_cast<unsigned char>(*cptr++);
		}
		return ss;
	}

	ptr constUTF8toSstring(const char* s)
	{
		// With UTF8 we just have sequences of bytes in a buffer.
		// in scheme we use a single longer integer for a code point.
		// see https://github.com/Komodo/KomodoEdit/blob/master/src/SciMoz/nsSciMoz.cxx
		// passes the greek test.

		unsigned int byte2;

		if (s == nullptr)
		{
			return Sstring("");
		}
		const auto ll = utf8_string_length(s);
		if (ll == 0)
		{
			return Sstring("");
		}

		const char* cptr = s;
		unsigned int byte = static_cast<unsigned char>(*cptr++);

		ptr ss = Sstring_of_length("", ll);

		auto i = 0;

		while (byte != 0 && i < ll)
		{
			// ascii
			if (byte < 0x80)
			{
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			if (byte < 0xC0)
			{
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			// skip
			while ((byte < 0xC0) && (byte >= 0x80))
			{
				byte = static_cast<unsigned char>(*cptr++);
			}

			if (byte < 0xE0)
			{
				byte2 = static_cast<unsigned char>(*cptr++);

				if ((byte2 & 0xC0) == 0x80)
				{
					byte = (((byte & 0x1F) << 6) | (byte2 & 0x3F));
					Sstring_set(ss, i++, byte);
					byte = static_cast<unsigned char>(*cptr++);
				}
				continue;
			}

			if (byte < 0xF0)
			{
				byte2 = static_cast<unsigned char>(*cptr++);
				const unsigned int byte3 = static_cast<unsigned char>(*cptr++);
				if (((byte2 & 0xC0) == 0x80) && ((byte3 & 0xC0) == 0x80))
				{
					byte = (((byte & 0x0F) << 12) | ((byte2 & 0x3F) << 6) | (byte3 & 0x3F));
					Sstring_set(ss, i++, byte);
					byte = static_cast<unsigned char>(*cptr++);
				}
				continue;
			}

			auto trail = total_bytes[byte] - 1; // expected number of trail bytes
			if (trail > 0)
			{
				int ch = byte & (0x3F >> trail);
				do
				{
					byte2 = static_cast<unsigned char>(*cptr++);
					if ((byte2 & 0xC0) != 0x80)
					{
						Sstring_set(ss, i++, byte);
						byte = static_cast<unsigned char>(*cptr++);
						continue;
					}
					ch <<= 6;
					ch |= (byte2 & 0x3F);
					trail--;
				} while (trail > 0);
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			// no match..
			if (i == ll)
			{
				break;
			}
			byte = static_cast<unsigned char>(*cptr++);
		}

		return ss;
	}

	ptr constUTF8toSstringOfLength(const char* s, const int length)
	{
		// With UTF8 we just have sequences of bytes in a buffer.
		// in scheme we use a single longer integer for a code point.
		// see https://github.com/Komodo/KomodoEdit/blob/master/src/SciMoz/nsSciMoz.cxx
		// passes the greek test.

		unsigned int byte2;

		if (s == nullptr)
		{
			return Sstring("");
		}
		const auto ll = utf8_string_length(s);
		if (ll == 0)
		{
			return Sstring("");
		}

		const char* cptr = s;
		unsigned int byte = static_cast<unsigned char>(*cptr++);

		ptr ss = Sstring_of_length("", length);

		auto i = 0;

		while (byte != 0 && i < length)
		{
			// ascii
			if (byte < 0x80)
			{
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			if (byte < 0xC0)
			{
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			// skip
			while ((byte < 0xC0) && (byte >= 0x80))
			{
				byte = static_cast<unsigned char>(*cptr++);
			}

			if (byte < 0xE0)
			{
				byte2 = static_cast<unsigned char>(*cptr++);

				if ((byte2 & 0xC0) == 0x80)
				{
					byte = (((byte & 0x1F) << 6) | (byte2 & 0x3F));
					Sstring_set(ss, i++, byte);
					byte = static_cast<unsigned char>(*cptr++);
				}
				continue;
			}

			if (byte < 0xF0)
			{
				byte2 = static_cast<unsigned char>(*cptr++);
				const unsigned int byte3 = static_cast<unsigned char>(*cptr++);
				if (((byte2 & 0xC0) == 0x80) && ((byte3 & 0xC0) == 0x80))
				{
					byte = (((byte & 0x0F) << 12) | ((byte2 & 0x3F) << 6) | (byte3 & 0x3F));
					Sstring_set(ss, i++, byte);
					byte = static_cast<unsigned char>(*cptr++);
				}
				continue;
			}

			auto trail = total_bytes[byte] - 1; // expected number of trail bytes
			if (trail > 0)
			{
				int ch = byte & (0x3F >> trail);
				do
				{
					byte2 = static_cast<unsigned char>(*cptr++);
					if ((byte2 & 0xC0) != 0x80)
					{
						Sstring_set(ss, i++, byte);
						byte = static_cast<unsigned char>(*cptr++);
						continue;
					}
					ch <<= 6;
					ch |= (byte2 & 0x3F);
					trail--;
				} while (trail > 0);
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			// no match..
			if (i == ll)
			{
				break;
			}
			byte = static_cast<unsigned char>(*cptr++);
		}
		return ss;
	}

	// use windows functions to widen string
	std::wstring Widen(const std::string& in)
	{
		// Calculate target buffer size (not including the zero terminator).
		const auto len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
			in.c_str(), in.size(), NULL, 0);
		if (len == 0)
		{
			throw std::runtime_error("Invalid character sequence.");
		}

		std::wstring out(len, 0);
		MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
			in.c_str(), in.size(), &out[0], out.size());
		return out;
	}

	// use windows functions to widen string
	std::wstring widen(const std::string& in)
	{
		// Calculate target buffer size (not including the zero terminator).
		const auto len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
			in.c_str(), in.size(), NULL, 0);
		if (len == 0)
		{
			throw std::runtime_error("Invalid character sequence.");
		}

		std::wstring out(len, 0);
		MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
			in.c_str(), in.size(), &out[0], out.size());
		return out;
	}
	// scheme string to c char*
	char* Sstring_to_charptr(ptr sparam)
	{
		ptr bytes = CALL1("string->utf8", sparam);
		const long len = Sbytevector_length(bytes);
		const auto data = Sbytevector_data(bytes);
		const auto text = static_cast<char*>(calloc(len + 1, sizeof(char)));
		memcpy(text, data, len);
		bytes = Snil;
		return text;
	}

	// scheme strings are wider UTF32 than windows wide UTF16 strings.
	// not sure if widestring code points span more than one
	// short int; if they do this is broken.

	ptr wideto_sstring(WCHAR* s)
	{
		if (s == nullptr)
		{
			return Sstring("");
		}
		const int len = wcslen(s);
		if (len == 0)
		{
			return Sstring("");
		}
		auto ss = Sstring_of_length("", len);
		for (auto i = 0; i < len; i++)
		{
			Sstring_set(ss, i, s[i]);
		}
		return ss;
	}

	ConversionResult ConvertUTF8toUTF16(const UTF8** sourceStart, const UTF8* sourceEnd, UTF16** targetStart,
		const UTF16* targetEnd, conversion_flags flags)
	{
		ConversionResult result = conversionOK;
		const UTF8* source = *sourceStart;
		UTF16* target = *targetStart;
		while (source < sourceEnd)
		{
			UTF32 ch = 0;
			unsigned short extra_bytes_to_read = trailing_bytes_for_utf8[*source];
			if (source + extra_bytes_to_read >= sourceEnd)
			{
				result = sourceExhausted;
				break;
			}
			/* Do this check whether lenient or strict */
			if (!is_legal_utf8(source, extra_bytes_to_read + 1))
			{
				result = sourceIllegal;
				break;
			}
			/*
			* The cases all fall through. See "Note A" below.
			*/
			switch (extra_bytes_to_read)
			{
			case 5:
				ch += *source++;
				ch <<= 6; /* remember, illegal UTF-8 */
			case 4:
				ch += *source++;
				ch <<= 6; /* remember, illegal UTF-8 */
			case 3:
				ch += *source++;
				ch <<= 6;
			case 2:
				ch += *source++;
				ch <<= 6;
			case 1:
				ch += *source++;
				ch <<= 6;
			case 0:
				ch += *source++;
			}
			ch -= offsets_from_utf8[extra_bytes_to_read];

			if (target >= targetEnd)
			{
				source -= (extra_bytes_to_read + 1); /* Back up source pointer! */
				result = targetExhausted;
				break;
			}
			if (ch <= UNI_MAX_BMP)
			{
				/* Target is a character <= 0xFFFF */
				/* UTF-16 surrogate values are illegal in UTF-32 */
				if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END)
				{
					if (flags == strictConversion)
					{
						source -= (extra_bytes_to_read + 1); /* return to the illegal value itself */
						result = sourceIllegal;
						break;
					}
					else
					{
						*target++ = UNI_REPLACEMENT_CHAR;
					}
				}
				else
				{
					*target++ = static_cast<UTF16>(ch); /* normal case */
				}
			}
			else if (ch > UNI_MAX_UTF16)
			{
				if (flags == strictConversion)
				{
					result = sourceIllegal;
					source -= (extra_bytes_to_read + 1); /* return to the start */
					break; /* Bail out; shouldn't continue */
				}
				else
				{
					*target++ = UNI_REPLACEMENT_CHAR;
				}
			}
			else
			{
				/* target is a character in range 0xFFFF - 0x10FFFF. */
				if (target + 1 >= targetEnd)
				{
					source -= (extra_bytes_to_read + 1); /* Back up source pointer! */
					result = targetExhausted;
					break;
				}
				ch -= halfBase;
				*target++ = static_cast<UTF16>((ch >> halfShift) + UNI_SUR_HIGH_START);
				*target++ = static_cast<UTF16>((ch & halfMask) + UNI_SUR_LOW_START);
			}
		}
		*sourceStart = source;
		*targetStart = target;
		return result;
	}


	// 
	// https://github.com/nlohmann/json/issues/1198
	//
	void trim_utf8(std::string& hairy) {
		std::vector<bool> results;
		std::string smooth;
		size_t len = hairy.size();
		results.reserve(len);
		smooth.reserve(len);
		const unsigned char* bytes = (const unsigned char*)hairy.c_str();

		auto read_utf8 = [](const unsigned char* bytes, size_t len, size_t* pos) -> unsigned {
			int code_unit1 = 0;
			int code_unit2, code_unit3, code_unit4;

			if (*pos >= len) goto ERROR1;
			code_unit1 = bytes[(*pos)++];

			if (code_unit1 < 0x80) return code_unit1;
			else if (code_unit1 < 0xC2) goto ERROR1; // continuation or overlong 2-byte sequence
			else if (code_unit1 < 0xE0) {
				if (*pos >= len) goto ERROR1;
				code_unit2 = bytes[(*pos)++]; //2-byte sequence
				if ((code_unit2 & 0xC0) != 0x80) goto ERROR2;
				return (code_unit1 << 6) + code_unit2 - 0x3080;
			}
			else if (code_unit1 < 0xF0) {
				if (*pos >= len) goto ERROR1;
				code_unit2 = bytes[(*pos)++]; // 3-byte sequence
				if ((code_unit2 & 0xC0) != 0x80) goto ERROR2;
				if (code_unit1 == 0xE0 && code_unit2 < 0xA0) goto ERROR2; // overlong
				if (*pos >= len) goto ERROR2;
				code_unit3 = bytes[(*pos)++];
				if ((code_unit3 & 0xC0) != 0x80) goto ERROR3;
				return (code_unit1 << 12) + (code_unit2 << 6) + code_unit3 - 0xE2080;
			}
			else if (code_unit1 < 0xF5) {
				if (*pos >= len) goto ERROR1;
				code_unit2 = bytes[(*pos)++]; // 4-byte sequence
				if ((code_unit2 & 0xC0) != 0x80) goto ERROR2;
				if (code_unit1 == 0xF0 && code_unit2 < 0x90) goto ERROR2; // overlong
				if (code_unit1 == 0xF4 && code_unit2 >= 0x90) goto ERROR2; // > U+10FFFF
				if (*pos >= len) goto ERROR2;
				code_unit3 = bytes[(*pos)++];
				if ((code_unit3 & 0xC0) != 0x80) goto ERROR3;
				if (*pos >= len) goto ERROR3;
				code_unit4 = bytes[(*pos)++];
				if ((code_unit4 & 0xC0) != 0x80) goto ERROR4;
				return (code_unit1 << 18) + (code_unit2 << 12) + (code_unit3 << 6) + code_unit4 - 0x3C82080;
			}
			else goto ERROR1; // > U+10FFFF

		ERROR4:
			(*pos)--;
		ERROR3:
			(*pos)--;
		ERROR2:
			(*pos)--;
		ERROR1:
			return code_unit1 + 0xDC00;
		};

		unsigned c;
		size_t pos = 0;
		size_t pos_before;
		size_t inc = 0;
		bool valid;

		for (;;) {
			pos_before = pos;
			c = read_utf8(bytes, len, &pos);
			inc = pos - pos_before;
			if (!inc) break; // End of string reached.

			valid = false;

			if ((c <= 0x00007F)
				|| (c >= 0x000080 && c <= 0x0007FF)
				|| (c >= 0x000800 && c <= 0x000FFF)
				|| (c >= 0x001000 && c <= 0x00CFFF)
				|| (c >= 0x00D000 && c <= 0x00D7FF)
				|| (c >= 0x00E000 && c <= 0x00FFFF)
				|| (c >= 0x010000 && c <= 0x03FFFF)
				|| (c >= 0x040000 && c <= 0x0FFFFF)
				|| (c >= 0x100000 && c <= 0x10FFFF)) valid = true;

			if (c >= 0xDC00 && c <= 0xDCFF) {
				valid = false;
			}

			do results.push_back(valid); while (--inc);
		}

		size_t sz = results.size();
		for (size_t i = 0; i < sz; ++i) {
			if (results[i]) smooth.append(1, hairy.at(i));
		}
		hairy.swap(smooth);
	}


	const char* correct_non_utf_8(const char* s)
	{
		std::string in = s;
		const auto str = &in;
		int i, f_size = str->size();
		unsigned char c, c2, c3, c4;
		std::string to;
		to.reserve(f_size);

		for (i = 0; i < f_size; i++) {
			c = (unsigned char)(*str)[i];
			if (c < 32) {//control char
				if (c == 9 || c == 10 || c == 13) {//allow only \t \n \r
					to.append(1, c);
				}
				continue;
			}
			else if (c < 127) {//normal ASCII
				to.append(1, c);
				continue;
			}
			else if (c < 160) {//control char (nothing should be defined here either ASCI, ISO_8859-1 or UTF8, so skipping)
				if (c2 == 128) {//fix microsoft mess, add euro
					to.append(1, 226);
					to.append(1, 130);
					to.append(1, 172);
				}
				if (c2 == 133) {//fix IBM mess, add NEL = \n\r
					to.append(1, 10);
					to.append(1, 13);
				}
				continue;
			}
			else if (c < 192) {//invalid for UTF8, converting ASCII
				to.append(1, (unsigned char)194);
				to.append(1, c);
				continue;
			}
			else if (c < 194) {//invalid for UTF8, converting ASCII
				to.append(1, (unsigned char)195);
				to.append(1, c - 64);
				continue;
			}
			else if (c < 224 && i + 1 < f_size) {//possibly 2byte UTF8
				c2 = (unsigned char)(*str)[i + 1];
				if (c2 > 127 && c2 < 192) {//valid 2byte UTF8
					if (c == 194 && c2 < 160) {//control char, skipping
						;
					}
					else {
						to.append(1, c);
						to.append(1, c2);
					}
					i++;
					continue;
				}
			}
			else if (c < 240 && i + 2 < f_size) {//possibly 3byte UTF8
				c2 = (unsigned char)(*str)[i + 1];
				c3 = (unsigned char)(*str)[i + 2];
				if (c2 > 127 && c2 < 192 && c3>127 && c3 < 192) {//valid 3byte UTF8
					to.append(1, c);
					to.append(1, c2);
					to.append(1, c3);
					i += 2;
					continue;
				}
			}
			else if (c < 245 && i + 3 < f_size) {//possibly 4byte UTF8
				c2 = (unsigned char)(*str)[i + 1];
				c3 = (unsigned char)(*str)[i + 2];
				c4 = (unsigned char)(*str)[i + 3];
				if (c2 > 127 && c2 < 192 && c3>127 && c3 < 192 && c4>127 && c4 < 192) {//valid 4byte UTF8
					to.append(1, c);
					to.append(1, c2);
					to.append(1, c3);
					to.append(1, c4);
					i += 3;
					continue;
				}
			}
			//invalid UTF8, converting ASCII (c>245 || string too short for multi-byte))
			to.append(1, (unsigned char)195);
			to.append(1, c - 64);
		}
		return _strdup(to.c_str());
	}



	ptr clean_utf8_to_sstring(const char* s) {

		unsigned int byte2;
		if (s == nullptr) {
			return Sstring("");
		}
		const auto ll = Text::strlen_utf8(s);
		if (ll == 0) {
			return Sstring("");
		}

		const char* cptr = s;

		unsigned int byte = static_cast<unsigned char>(*cptr++);
		auto ss = Sstring_of_length("", ll);
		auto i = 0;

		while (byte != 0 && i < ll) {


			if (byte < 0x80) {
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			if (byte < 0xC0) {
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			// skip
			while ((byte < 0xC0) && (byte >= 0x80)) {
				byte = static_cast<unsigned char>(*cptr++);
			}

			if (byte < 0xE0) {
				byte2 = static_cast<unsigned char>(*cptr++);
				if ((byte2 & 0xC0) == 0x80) {
					byte = (((byte & 0x1F) << 6) | (byte2 & 0x3F));
					Sstring_set(ss, i++, byte);
					byte = static_cast<unsigned char>(*cptr++);
				}
				continue;
			}

			if (byte < 0xF0) {

				byte2 = static_cast<unsigned char>(*cptr++);
				if (byte2 == 0) { break; };
				const unsigned int byte3 = static_cast<unsigned char>(*cptr++);
				if (byte3 == 0) { break; };

				if (((byte2 & 0xC0) == 0x80) && ((byte3 & 0xC0) == 0x80)) {
					byte = (((byte & 0x0F) << 12)
						| ((byte2 & 0x3F) << 6) | (byte3 & 0x3F));
					if (byte == 0x200B || byte == 0x200D) byte = '*';
					Sstring_set(ss, i++, byte);
					byte = static_cast<unsigned char>(*cptr++);
				}
				continue;
			}

			auto trail = total_bytes[byte] - 1; // expected number of trail bytes
			if (trail > 0) {
				int ch = byte & (0x3F >> trail);
				do {
					byte2 = static_cast<unsigned char>(*cptr++);
					if ((byte2 & 0xC0) != 0x80) {
						if (byte == 0x200B || byte == 0x200D) byte = '!';
						Sstring_set(ss, i++, byte);
						byte = static_cast<unsigned char>(*cptr++);
						continue;
					}
					ch <<= 6;
					ch |= (byte2 & 0x3F);
					trail--;
				} while (trail > 0);

				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			// no match..
			if (i == ll) {
				break;
			}
			byte = static_cast<unsigned char>(*cptr++);
		}
		return ss;
	}

	ptr utf8_to_sstring(char* s) {
		std::string trimmed = s;
		Text::trim_utf8(trimmed);
		return clean_utf8_to_sstring(trimmed.c_str());
	}




	ptr const_utf8_to_sstring(const char* s) {



		// With UTF8 we just have sequences of bytes in a buffer.
		// in scheme we use a single longer integer for a code point.
		// see https://github.com/Komodo/KomodoEdit/blob/master/src/SciMoz/nsSciMoz.cxx
		// passes the greek test.

		unsigned int byte2;

		if (s == nullptr) {
			return Sstring("");
		}
		const auto ll = Text::strlen_utf8(s);
		if (ll == 0) {
			return Sstring("");
		}

		auto cptr = s;
		unsigned int byte = static_cast<unsigned char>(*cptr++);

		auto ss = Sstring_of_length("", ll);

		auto i = 0;

		while (byte != 0 && i < ll) {

			// ascii
			if (byte < 0x80) {
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			if (byte < 0xC0) {
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			// skip
			while ((byte < 0xC0) && (byte >= 0x80)) {
				byte = static_cast<unsigned char>(*cptr++);
			}

			if (byte < 0xE0) {
				byte2 = static_cast<unsigned char>(*cptr++);
				if ((byte2 & 0xC0) == 0x80) {
					byte = (((byte & 0x1F) << 6) | (byte2 & 0x3F));
					Sstring_set(ss, i++, byte);
					byte = static_cast<unsigned char>(*cptr++);
				}
				continue;
			}

			if (byte < 0xF0) {
				byte2 = static_cast<unsigned char>(*cptr++);
				const unsigned int byte3 = static_cast<unsigned char>(*cptr++);
				if (((byte2 & 0xC0) == 0x80) && ((byte3 & 0xC0) == 0x80)) {
					byte = (((byte & 0x0F) << 12)
						| ((byte2 & 0x3F) << 6) | (byte3 & 0x3F));
					Sstring_set(ss, i++, byte);
					byte = static_cast<unsigned char>(*cptr++);
				}
				continue;
			}

			auto trail = total_bytes[byte] - 1; // expected number of trail bytes
			if (trail > 0) {
				int ch = byte & (0x3F >> trail);
				do {
					byte2 = static_cast<unsigned char>(*cptr++);
					if ((byte2 & 0xC0) != 0x80) {
						Sstring_set(ss, i++, byte);
						byte = static_cast<unsigned char>(*cptr++);
						continue;
					}
					ch <<= 6;
					ch |= (byte2 & 0x3F);
					trail--;
				} while (trail > 0);
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			// no match..
			if (i == ll) {
				break;
			}
			byte = static_cast<unsigned char>(*cptr++);
		}

		return ss;
	}

	ptr const_utf8_to_sstring_of_length(const char* s, const int length) {


		// With UTF8 we just have sequences of bytes in a buffer.
		// in scheme we use a single longer integer for a code point.
		// see https://github.com/Komodo/KomodoEdit/blob/master/src/SciMoz/nsSciMoz.cxx
		// passes the greek test.

		unsigned int byte2;
		if (s == nullptr) {
			return Sstring("");
		}
		const auto ll = Text::strlen_utf8(s);
		if (ll == 0) {
			return Sstring("");
		}

		auto cptr = s;
		unsigned int byte = static_cast<unsigned char>(*cptr++);
		auto ss = Sstring_of_length("", length);
		auto i = 0;

		while (byte != 0 && i < length) {

			// ascii
			if (byte < 0x80) {
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			if (byte < 0xC0) {
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			// skip
			while ((byte < 0xC0) && (byte >= 0x80)) {
				byte = static_cast<unsigned char>(*cptr++);
			}

			if (byte < 0xE0) {
				byte2 = static_cast<unsigned char>(*cptr++);
				if ((byte2 & 0xC0) == 0x80) {
					byte = (((byte & 0x1F) << 6) | (byte2 & 0x3F));
					Sstring_set(ss, i++, byte);
					byte = static_cast<unsigned char>(*cptr++);
				}
				continue;
			}

			if (byte < 0xF0) {
				byte2 = static_cast<unsigned char>(*cptr++);
				const unsigned int byte3 = static_cast<unsigned char>(*cptr++);
				if (((byte2 & 0xC0) == 0x80) && ((byte3 & 0xC0) == 0x80)) {
					byte = (((byte & 0x0F) << 12)
						| ((byte2 & 0x3F) << 6) | (byte3 & 0x3F));
					Sstring_set(ss, i++, byte);
					byte = static_cast<unsigned char>(*cptr++);
				}
				continue;
			}

			auto trail = total_bytes[byte] - 1; // expected number of trail bytes
			if (trail > 0) {
				int ch = byte & (0x3F >> trail);
				do {
					byte2 = static_cast<unsigned char>(*cptr++);
					if ((byte2 & 0xC0) != 0x80) {
						Sstring_set(ss, i++, byte);
						byte = static_cast<unsigned char>(*cptr++);
						continue;
					}
					ch <<= 6;
					ch |= (byte2 & 0x3F);
					trail--;
				} while (trail > 0);
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}
			// no match..
			if (i == ll) {
				break;
			}
			byte = static_cast<unsigned char>(*cptr++);
		}
		return ss;
	}


	// https://github.com/CovenantEyes/uri-parser
// Based (heavily) on an article from CodeGuru
// http://www.codeguru.com/cpp/cpp/string/conversions/article.php/c12759



	const char hex2_dec[256] =
	{
		/*       0  1  2  3   4  5  6  7   8  9  A  B   C  D  E  F */
		/* 0 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		/* 1 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		/* 2 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		/* 3 */ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1,

		/* 4 */ -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		/* 5 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		/* 6 */ -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		/* 7 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

		/* 8 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		/* 9 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		/* A */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		/* B */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

		/* C */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		/* D */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		/* E */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		/* F */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
	};

	std::string uri_decode(const std::string& s_src)
	{
		const auto* p_src = reinterpret_cast<const unsigned char*>(s_src.c_str());
		const int src_len = s_src.length();
		const auto src_end = p_src + src_len;
		const auto src_last_dec = src_end - 2; // last decodable '%' 
		const auto p_start = new char[src_len];
		auto p_end = p_start;

		while (p_src < src_last_dec)
		{
			if (*p_src == '%')
			{
				char dec1, dec2;
				if (-1 != (dec1 = hex2_dec[*(p_src + 1)])
					&& -1 != (dec2 = hex2_dec[*(p_src + 2)]))
				{
					*p_end++ = (dec1 << 4) + dec2;
					p_src += 3;
					continue;
				}
			}
			*p_end++ = *p_src++;
		}

		// the last 2- chars
		while (p_src < src_end)
			*p_end++ = *p_src++;

		std::string s_result(p_start, p_end);
		delete[] p_start;
		return s_result;
	}

	// Only alphanum is safe.
	const char safe[256] =
	{
		/*      0 1 2 3  4 5 6 7  8 9 A B  C D E F */
		/* 0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		/* 1 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		/* 2 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		/* 3 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,

		/* 4 */ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		/* 5 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
		/* 6 */ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		/* 7 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,

		/* 8 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		/* 9 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		/* A */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		/* B */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

		/* C */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		/* D */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		/* E */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		/* F */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	std::string uri_encode(const std::string& s_src)
	{
		const char dec2_hex[16 + 1] = "0123456789ABCDEF";
		auto p_src = reinterpret_cast<const unsigned char*>(s_src.c_str());
		const int src_len = s_src.length();
		const auto p_start = new unsigned char[src_len * 3];
		auto p_end = p_start;
		const auto src_end = p_src + src_len;

		for (; p_src < src_end; ++p_src)
		{
			if (safe[*p_src])
				*p_end++ = *p_src;
			else
			{
				// escape this char
				*p_end++ = '%';
				*p_end++ = dec2_hex[*p_src >> 4];
				*p_end++ = dec2_hex[*p_src & 0x0F];
			}
		}

		std::string s_result(reinterpret_cast<char*>(p_start), reinterpret_cast<char*>(p_end));
		delete[] p_start;
		return s_result;
	}


	static std::string tail_slice(std::string& subject, const std::string& delimiter, const bool keep_delim = false)
	{
		const auto delimiter_location = subject.find(delimiter);
		const auto delimiter_length = delimiter.length();
		std::string output;
		if (delimiter_location < std::string::npos)
		{
			const auto start = keep_delim ? delimiter_location : delimiter_location + delimiter_length;
			const auto end = subject.length() - start;
			output = subject.substr(start, end);
			subject = subject.substr(0, delimiter_location);
		}
		return output;
	}

	static std::string head_slice(std::string& subject, const std::string& delimiter)
	{
		const auto delimiter_location = subject.find(delimiter);
		const auto delimiter_length = delimiter.length();
		std::string output;
		if (delimiter_location < std::string::npos)
		{
			output = subject.substr(0, delimiter_location);
			subject = subject.substr(delimiter_location + delimiter_length,
				subject.length() - (delimiter_location + delimiter_length));
		}
		return output;
	}

	static inline int extract_port(std::string& hostport)
	{
		int port;
		const std::string delim = ":";
		auto portstring = tail_slice(hostport, delim);
		try { port = atoi(portstring.c_str()); }
		catch (std::exception& e) { port = -1; }
		return port;
	}

	std::string extract_path(std::string& in)
	{
		const std::string delim = "/";
		return head_slice(tail_slice(in, delim, false), "?");
	}

	static inline std::string extract_protocol(std::string& in)
	{
		const std::string delim = "://";
		return head_slice(in, delim);
	}

	std::string extract_search(std::string& in)
	{
		const std::string delim = "?";
		return tail_slice(in, delim);
	}

	static inline std::string extract_password(std::string& in)
	{
		const std::string delim = ":";
		return tail_slice(in, delim);
	}

	static inline std::string extract_user_pass(std::string& in)
	{
		const std::string delim = "@";
		return head_slice(in, delim);
	}



} // namespace Text



 

// we write onto the active_surface; we display from the display surface
Gdiplus::Bitmap* active_surface = nullptr;
Gdiplus::Bitmap* display_surface = nullptr;
Gdiplus::Bitmap* temp_surface = nullptr;
int _graphics_mode;
HANDLE g_image_rotation_mutex;

namespace Assoc
{
	ptr sstring(const char* symbol, const char* value)
	{
		ptr a = Snil;
		a = CALL2("cons", Sstring_to_symbol(symbol), Text::constUTF8toSstring(value));
		return a;
	}

	ptr sflonum(const char* symbol, const float value)
	{
		ptr a = Snil;
		a = CALL2("cons", Sstring_to_symbol(symbol), Sflonum(value));
		return a;
	}

	ptr sfixnum(const char* symbol, const int value)
	{
		ptr a = Snil;
		a = CALL2("cons", Sstring_to_symbol(symbol), Sfixnum(value));
		return a;
	}

	ptr sptr(const char* symbol, ptr value)
	{
		ptr a = Snil;
		a = CALL2("cons", Sstring_to_symbol(symbol), value);
		return a;
	}

	ptr cons_sstring(const char* symbol, const char* value, ptr l)
	{
		ptr a = Snil;
		a = CALL2("cons", Sstring_to_symbol(symbol), Text::constUTF8toSstring(value));
		l = CALL2("cons", a, l);
		return l;
	}

	ptr cons_sbool(const char* symbol, bool value, ptr l)
	{
		ptr a = Snil;
		if (value) {
			a = CALL2("cons", Sstring_to_symbol(symbol), Strue);
		}
		else
		{
			a = CALL2("cons", Sstring_to_symbol(symbol), Sfalse);
		}
		l = CALL2("cons", a, l);
		return l;
	}

	ptr cons_sptr(const char* symbol, ptr value, ptr l)
	{
		ptr a = Snil;
		a = CALL2("cons", Sstring_to_symbol(symbol), value);
		l = CALL2("cons", a, l);
		return l;
	}

	ptr cons_sfixnum(const char* symbol, const int value, ptr l)
	{
		ptr a = Snil;
		a = CALL2("cons", Sstring_to_symbol(symbol), Sfixnum(value));
		l = CALL2("cons", a, l);
		return l;
	}

	ptr cons_sflonum(const char* symbol, const float value, ptr l)
	{
		ptr a = Snil;
		a = CALL2("cons", Sstring_to_symbol(symbol), Sflonum(value));
		l = CALL2("cons", a, l);
		return l;
	}
} // namespace Assoc

namespace Graphics2D{

	#pragma comment(lib, "d2d1.lib")
	#pragma comment(lib, "Dwrite.lib")
	#pragma comment (lib, "Windowscodecs.lib")
	// set by init function

	HWND main_window = nullptr;
	HANDLE g_rotation_mutex=nullptr;

 

	float prefer_width = 800.0f;
	float prefer_height = 600.0f;
	// represents the visible surface on the window itself.

	ID2D1HwndRenderTarget* pRenderTarget;

	// stoke width 
	float d2d_stroke_width = 1.3;
	ID2D1StrokeStyle* d2d_stroke_style = nullptr;

	// colours and brushes used when drawing
	ID2D1SolidColorBrush* pColourBrush = nullptr;       // line-color
	ID2D1SolidColorBrush* pfillColourBrush = nullptr;   // fill-color
	ID2D1BitmapBrush* pPatternBrush = nullptr;          // brush-pattern
	ID2D1BitmapBrush* pTileBrush = nullptr;				// tile its U/S

	// we draw into this
	ID2D1Bitmap* bitmap = nullptr;
	ID2D1BitmapRenderTarget* BitmapRenderTarget = nullptr;
	ID2D1Bitmap* bitmap2 = nullptr;
	ID2D1BitmapRenderTarget* BitmapRenderTarget2 = nullptr;

	ID2D1Factory* pD2DFactory;

	// hiDPI
	float g_DPIScaleX = 1.0f;
	float g_DPIScaleY = 1.0f;
	float _pen_width = static_cast<float>(1.2);

#pragma warning(disable : 4996)

	void d2d_CreateOffscreenBitmap()
	{
		if (pRenderTarget == nullptr)
		{
			return;
		}

		if (BitmapRenderTarget == NULL) {
			pRenderTarget->CreateCompatibleRenderTarget(D2D1::SizeF(prefer_width, prefer_height), &BitmapRenderTarget);
			BitmapRenderTarget->GetBitmap(&bitmap);
		}
		if (BitmapRenderTarget2 == NULL) {
			pRenderTarget->CreateCompatibleRenderTarget(D2D1::SizeF(prefer_width, prefer_height), &BitmapRenderTarget2);
			BitmapRenderTarget2->GetBitmap(&bitmap2);
		}
	}

	void swap_buffers(int n) {

		if (pRenderTarget == nullptr) {
			return;
		}
		//WaitForSingleObject(g_image_rotation_mutex, INFINITE);
		ID2D1Bitmap* temp;
		d2d_CreateOffscreenBitmap();
		if (n == 1) {
			BitmapRenderTarget2->BeginDraw();
			BitmapRenderTarget2->DrawBitmap(bitmap, D2D1::RectF(0.0f, 0.0f, prefer_width, prefer_height));
			BitmapRenderTarget2->EndDraw();
		}
		temp = bitmap;
		bitmap = bitmap2;
		bitmap2 = temp;
		ID2D1BitmapRenderTarget* temptarget;
		temptarget = BitmapRenderTarget;
		BitmapRenderTarget = BitmapRenderTarget2;
		BitmapRenderTarget2 = temptarget;
	 
		if (main_window != nullptr) {
			InvalidateRect(main_window, NULL, FALSE);
		}

		//ReleaseMutex(g_image_rotation_mutex);
		Sleep(1);

	}

	ptr d2d_show(int n)
	{
		swap_buffers(n);
		return Strue;
	}

	ptr d2d_color(float r, float g, float b, float a) {

		if (BitmapRenderTarget == NULL)
		{
			return Snil;
		}
		SafeRelease(&pColourBrush);
		HRESULT hr = BitmapRenderTarget->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF(r, g, b, a)),
			&pColourBrush
		);
		return Strue;
	}


	ptr d2d_fill_color(float r, float g, float b, float a) {


		if (BitmapRenderTarget == nullptr)
		{
			return Snil;
		}
		SafeRelease(&pfillColourBrush);
		HRESULT hr = BitmapRenderTarget->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF(r, g, b, a)),
			&pfillColourBrush
		);
		return Strue;
	}

	ptr d2d_ellipse(float x, float y, float w, float h) {


		if (BitmapRenderTarget == nullptr)
		{
			return Snil;
		}

		if (pColourBrush == nullptr) {
			d2d_color(0.0, 0.0, 0.0, 1.0);
		}
		auto stroke_width = d2d_stroke_width;
		auto stroke_style = d2d_stroke_style;
		auto ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), w, h);

		BitmapRenderTarget->BeginDraw();
		BitmapRenderTarget->DrawEllipse(ellipse, pColourBrush, d2d_stroke_width);
		BitmapRenderTarget->EndDraw();

		return Strue;
	}

	ptr d2d_line(float x, float y, float x1, float y1) {


		if (BitmapRenderTarget == nullptr)
		{
			return Snil;
		}

		if (pColourBrush == nullptr) {
			d2d_color(0.0, 0.0, 0.0, 1.0);
		}
		auto stroke_width = d2d_stroke_width;
		auto stroke_style = d2d_stroke_style;
		auto p1 = D2D1::Point2F(x, y);
		auto p2 = D2D1::Point2F(x1, y1);


		BitmapRenderTarget->BeginDraw();
		BitmapRenderTarget->DrawLine(p1, p2, pfillColourBrush, stroke_width, stroke_style);
		BitmapRenderTarget->EndDraw();

		return Strue;
	}

	ptr d2d_fill_ellipse(float x, float y, float w, float h) {

		if (BitmapRenderTarget == nullptr)
		{
			return Snil;
		}

		if (pfillColourBrush == nullptr) {
			d2d_fill_color(0.0, 0.0, 0.0, 1.0);
		}
		auto ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), w, h);
		BitmapRenderTarget->BeginDraw();
		BitmapRenderTarget->FillEllipse(ellipse, pfillColourBrush);
		BitmapRenderTarget->EndDraw();
		return Strue;
	}


	ptr d2d_rectangle(float x, float y, float w, float h) {

		if (BitmapRenderTarget == nullptr)
		{
			return Snil;
		}
		if (pColourBrush == nullptr) {
			d2d_color(0.0, 0.0, 0.0, 1.0);
		}
		auto stroke_width = d2d_stroke_width;
		auto stroke_style = d2d_stroke_style;
		D2D1_RECT_F rectangle1 = D2D1::RectF(x, y, w, h);

		BitmapRenderTarget->BeginDraw();
		BitmapRenderTarget->DrawRectangle(&rectangle1, pColourBrush, stroke_width);
		BitmapRenderTarget->EndDraw();
		return Strue;
	}

	ptr d2d_rounded_rectangle(float x, float y, float w, float h, float rx, float ry) {

		if (BitmapRenderTarget == nullptr)
		{
			return Snil;
		}
		if (pColourBrush == nullptr) {
			d2d_color(0.0, 0.0, 0.0, 1.0);
		}
		auto stroke_width = d2d_stroke_width;
		auto stroke_style = d2d_stroke_style;
		D2D1_ROUNDED_RECT rectangle1 = { D2D1::RectF(x, y, w, h), rx, ry };
		BitmapRenderTarget->BeginDraw();
		BitmapRenderTarget->DrawRoundedRectangle(rectangle1, pfillColourBrush, stroke_width, stroke_style);
		BitmapRenderTarget->EndDraw();
		return Strue;
	}



	ptr d2d_fill_rectangle(float x, float y, float w, float h) {

		if (BitmapRenderTarget == nullptr)
		{
			return Snil;
		}
		if (pfillColourBrush == nullptr) {
			d2d_fill_color(0.0, 0.0, 0.0, 1.0);
		}
		D2D1_RECT_F rectangle1 = D2D1::RectF(x, y, w, h);
		BitmapRenderTarget->BeginDraw();
		BitmapRenderTarget->FillRectangle(&rectangle1, pfillColourBrush);
		BitmapRenderTarget->EndDraw();
		return Strue;
	}

	// reset matrix
	ptr d2d_matrix_identity() {
		if (BitmapRenderTarget == nullptr)
		{
			return Snil;
		}
		BitmapRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		return Strue;
	}

	ptr d2d_matrix_rotate(float a, float x, float y) {
		if (BitmapRenderTarget == nullptr)
		{
			return Snil;
		}
		BitmapRenderTarget->SetTransform(
			D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x, y)));
		return Strue;
	}

	ptr d2d_matrix_translate(float x, float y) {
		if (BitmapRenderTarget == nullptr)
		{
			return Snil;
		}
		BitmapRenderTarget->SetTransform(
			D2D1::Matrix3x2F::Translation(40, 10));
		return Strue;
	}

	ptr d2d_matrix_rotrans(float a, float x, float y, float x1, float y1) {
		if (BitmapRenderTarget == nullptr)
		{
			return Snil;
		}
		const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x, y));
		const D2D1::Matrix3x2F trans = D2D1::Matrix3x2F::Translation(x1, y1);
		BitmapRenderTarget->SetTransform(rot * trans);
		return Strue;
	}

	ptr d2d_matrix_transrot(float a, float x, float y, float x1, float y1) {
		if (BitmapRenderTarget == nullptr)
		{
			return Snil;
		}
		const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x, y));
		const D2D1::Matrix3x2F trans = D2D1::Matrix3x2F::Translation(x1, y1);
		BitmapRenderTarget->SetTransform(trans * rot);
		return Strue;
	}

	ptr d2d_matrix_scale(float x, float y) {
		if (BitmapRenderTarget == nullptr)
		{
			return Snil;
		}
		BitmapRenderTarget->SetTransform(
			D2D1::Matrix3x2F::Scale(
				D2D1::Size(x, y),
				D2D1::Point2F(prefer_width / 2, prefer_height / 2))
		);
		return Strue;
	}

	ptr d2d_matrix_rotscale(float a, float x, float y, float x1, float y1) {
		if (BitmapRenderTarget == nullptr)
		{
			return Snil;
		}
		const auto scale = D2D1::Matrix3x2F::Scale(
			D2D1::Size(x, y),
			D2D1::Point2F(prefer_width / 2, prefer_height / 2));
		const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x, y));
		BitmapRenderTarget->SetTransform(rot * scale);
		return Strue;
	}

	ptr d2d_matrix_scalerot(float a, float x, float y, float x1, float y1) {
		if (BitmapRenderTarget == nullptr)
		{
			return Snil;
		}
		const auto scale = D2D1::Matrix3x2F::Scale(
			D2D1::Size(x, y),
			D2D1::Point2F(prefer_width / 2, prefer_height / 2));
		const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x, y));
		BitmapRenderTarget->SetTransform(rot * scale);
		return Strue;
	}

	ptr d2d_matrix_scalerottrans(float a, float x, float y, float x1, float y1, float x2, float y2) {
		if (BitmapRenderTarget == nullptr)
		{
			return Snil;
		}
		const auto scale = D2D1::Matrix3x2F::Scale(
			D2D1::Size(x, y),
			D2D1::Point2F(prefer_width / 2, prefer_height / 2));
		const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x, y));
		const D2D1::Matrix3x2F trans = D2D1::Matrix3x2F::Translation(x1, y1);
		BitmapRenderTarget->SetTransform(scale * rot * trans);
		return Strue;
	}

	ptr d2d_matrix_skew(float x, float y) {
		if (BitmapRenderTarget == nullptr)
		{
			return Snil;
		}
		BitmapRenderTarget->SetTransform(
			D2D1::Matrix3x2F::Skew(
				x, y,
				D2D1::Point2F(prefer_width / 2, prefer_height / 2))
		);
		return Strue;
	}

	// display current display buffer.
	ptr d2d_render(float x, float y) {

		swap_buffers(1);
		if (pRenderTarget == nullptr)
		{
			return Snil;
		}

		pRenderTarget->BeginDraw();
		pRenderTarget->DrawBitmap(bitmap2, D2D1::RectF(x, y, prefer_width, prefer_height));
		pRenderTarget->EndDraw();
		return Strue;
	}


	void d2d_DPIScale(ID2D1Factory* pFactory)
	{
		FLOAT dpiX, dpiY;
		pFactory->GetDesktopDpi(&dpiX, &dpiY);
		g_DPIScaleX = dpiX / 96.0f;
		g_DPIScaleY = dpiY / 96.0f;
	}

	int d2d_CreateGridPatternBrush(
		ID2D1RenderTarget* pRenderTarget,
		ID2D1BitmapBrush** ppBitmapBrush
	)
	{

		if (pPatternBrush != nullptr) {
			return 0;
		}
		// Create a compatible render target.
		ID2D1BitmapRenderTarget* pCompatibleRenderTarget = nullptr;
		HRESULT hr = pRenderTarget->CreateCompatibleRenderTarget(
			D2D1::SizeF(10.0f, 10.0f),
			&pCompatibleRenderTarget
		);
		if (SUCCEEDED(hr))
		{
			// Draw a pattern.
			ID2D1SolidColorBrush* pGridBrush = nullptr;
			hr = pCompatibleRenderTarget->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF(0.93f, 0.94f, 0.96f, 1.0f)),
				&pGridBrush
			);

			// create offscreen bitmap
			d2d_CreateOffscreenBitmap();

			if (SUCCEEDED(hr))
			{
				pCompatibleRenderTarget->BeginDraw();
				pCompatibleRenderTarget->FillRectangle(D2D1::RectF(0.0f, 0.0f, 10.0f, 1.0f), pGridBrush);
				pCompatibleRenderTarget->FillRectangle(D2D1::RectF(0.0f, 0.1f, 1.0f, 10.0f), pGridBrush);
				pCompatibleRenderTarget->EndDraw();

				// Retrieve the bitmap from the render target.
				ID2D1Bitmap* pGridBitmap = nullptr;
				hr = pCompatibleRenderTarget->GetBitmap(&pGridBitmap);
				if (SUCCEEDED(hr))
				{
					// Choose the tiling mode for the bitmap brush.
					D2D1_BITMAP_BRUSH_PROPERTIES brushProperties =
						D2D1::BitmapBrushProperties(D2D1_EXTEND_MODE_WRAP, D2D1_EXTEND_MODE_WRAP);

					// Create the bitmap brush.
					hr = pRenderTarget->CreateBitmapBrush(pGridBitmap, brushProperties, ppBitmapBrush);

					SafeRelease(&pGridBitmap);
				}

				SafeRelease(&pGridBrush);

			}

			SafeRelease(&pCompatibleRenderTarget);

		}

		return hr;
	}

	void d2d_make_default_stroke() {
		HRESULT r = pD2DFactory->CreateStrokeStyle(
			D2D1::StrokeStyleProperties(
				D2D1_CAP_STYLE_FLAT,
				D2D1_CAP_STYLE_FLAT,
				D2D1_CAP_STYLE_ROUND,
				D2D1_LINE_JOIN_MITER,
				1.0f,
				D2D1_DASH_STYLE_SOLID,
				0.0f),
			nullptr, 0,
			&d2d_stroke_style
		);
	}


	// direct write
	IDWriteFactory* pDWriteFactory;
	IDWriteTextFormat* TextFont;
	ID2D1SolidColorBrush* pBlackBrush;

	ptr d2d_write_text(float x, float y, char* s) {

		if (BitmapRenderTarget == nullptr)
		{
			return Snil;
		}
		if (pfillColourBrush == nullptr) {
			d2d_fill_color(0.0, 0.0, 0.0, 1.0);
		}

		const auto text = Text::widen(s);
		const auto len = text.length();

		D2D1_RECT_F layoutRect = D2D1::RectF(x, y, prefer_width - x, prefer_height - y);

		BitmapRenderTarget->BeginDraw();
		BitmapRenderTarget->DrawTextW(text.c_str(), len, TextFont, layoutRect, pfillColourBrush);
		BitmapRenderTarget->EndDraw();
		return Strue;
	}


	ptr d2d_set_font(char* s, float size) {

		SafeRelease(&TextFont);
		auto face = Text::Widen(s);
		HRESULT
			hr = pDWriteFactory->CreateTextFormat(
				face.c_str(),
				NULL,
				DWRITE_FONT_WEIGHT_REGULAR,
				DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,
				size,
				L"en-us",
				&TextFont
			);
		if (SUCCEEDED(hr)) {
			return Strue;
		}
		return Snil;
	}


	// images bank
	ID2D1Bitmap* pSpriteSheet[bank_size];

	void d2d_sprite_loader(char* filename, int n)
	{
		if (n > bank_size - 1) {
			return;
		}
		HRESULT hr;
		CoInitialize(NULL);
		IWICImagingFactory* wicFactory = NULL;
		hr = CoCreateInstance(
			CLSID_WICImagingFactory,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IWICImagingFactory,
			(LPVOID*)&wicFactory);

		if (FAILED(hr)) {
			return;
		}
		//create a decoder
		IWICBitmapDecoder* wicDecoder = NULL;
		std::wstring fname = Text::Widen(filename);
		hr = wicFactory->CreateDecoderFromFilename(
			fname.c_str(),
			NULL,
			GENERIC_READ,
			WICDecodeMetadataCacheOnLoad,
			&wicDecoder);

		IWICBitmapFrameDecode* wicFrame = NULL;
		hr = wicDecoder->GetFrame(0, &wicFrame);

		// create a converter
		IWICFormatConverter* wicConverter = NULL;
		hr = wicFactory->CreateFormatConverter(&wicConverter);

		// setup the converter
		hr = wicConverter->Initialize(
			wicFrame,
			GUID_WICPixelFormat32bppPBGRA,
			WICBitmapDitherTypeNone,
			NULL,
			0.0,
			WICBitmapPaletteTypeCustom
		);
		if (SUCCEEDED(hr))
		{
			hr = pRenderTarget->CreateBitmapFromWicBitmap(
				wicConverter,
				NULL,
				&pSpriteSheet[n]
			);
		}
		SafeRelease(&wicFactory);
		SafeRelease(&wicDecoder);
		SafeRelease(&wicConverter);
		SafeRelease(&wicFrame);
	}

	ptr d2d_load_sprites(char* filename, int n) {
		if (n > bank_size - 1) {
			return Snil;
		}
		d2d_sprite_loader(filename, n);
		return Strue;
	}


	// from sheet n; at sx, sy to dx, dy, w,h
	ptr d2d_render_sprite(int n, float dx, float dy) {

		if (n > bank_size - 1) {
			return Snil;
		}
		if (BitmapRenderTarget == nullptr)
		{
			return Snil;
		}
		auto sprite_sheet = pSpriteSheet[n];
		if (sprite_sheet == NULL) {
			return Snil;
		}
		const auto size = sprite_sheet->GetPixelSize();
		const auto dest = D2D1::RectF(dx, dy, dx + size.width, dy + size.height);
		const auto opacity = 1.0f;
		BitmapRenderTarget->SetTransform(
			D2D1::Matrix3x2F::Scale(
				D2D1::Size(1.0, 1.0),
				D2D1::Point2F(size.width / 2, size.height / 2)));
		BitmapRenderTarget->BeginDraw();
		BitmapRenderTarget->DrawBitmap(sprite_sheet, dest);
		BitmapRenderTarget->EndDraw();
		return Strue;
	}


	ptr d2d_render_sprite_rotscale(int n, float dx, float dy, float a, float s) {

		if (n > bank_size - 1) {
			return Snil;
		}
		if (BitmapRenderTarget == nullptr)
		{
			return Snil;
		}
		auto sprite_sheet = pSpriteSheet[n];
		if (sprite_sheet == NULL) {
			return Snil;
		}
		const auto size = sprite_sheet->GetPixelSize();
		const auto dest = D2D1::RectF(dx, dy, dx + size.width, dy + size.height);
		const auto opacity = 1.0f;

		const auto scale = D2D1::Matrix3x2F::Scale(
			D2D1::Size(s, s),
			D2D1::Point2F(dx, dy));
		const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(dx + (size.width / 2), dy + (size.height / 2)));

		BitmapRenderTarget->BeginDraw();
		BitmapRenderTarget->SetTransform(rot * scale);
		BitmapRenderTarget->DrawBitmap(sprite_sheet, dest);
		BitmapRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		BitmapRenderTarget->EndDraw();
		return Strue;
	}

	// from sheet n; at sx, sy to dx, dy, w,h scale up
	ptr d2d_render_sprite_sheet(int n, float dx, float dy, float dw, float dh,
		float sx, float sy, float sw, float sh, float scale) {

		if (n > bank_size - 1) {
			return Snil;
		}

		if (BitmapRenderTarget == nullptr)
		{
			return Snil;
		}

		auto sprite_sheet = pSpriteSheet[n];
		if (sprite_sheet == NULL) {
			return Snil;
		}
		const auto size = sprite_sheet->GetPixelSize();
		const auto dest = D2D1::RectF(dx, dy, scale * (dx + dw), scale * (dy + dh));
		const auto source = D2D1::RectF(sx, sy, sx + sw, sy + sh);
		const auto opacity = 1.0f;
		BitmapRenderTarget->BeginDraw();
		BitmapRenderTarget->DrawBitmap(sprite_sheet, dest, 1.0f,
			D2D1_BITMAP_INTERPOLATION_MODE::D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, source);
		BitmapRenderTarget->EndDraw();
		return Strue;
	}

	// from sheet n; at sx, sy to dx, dy, w,h scale up
	ptr d2d_render_sprite_sheet_rot_scale(int n, float dx, float dy, float dw, float dh,
		float sx, float sy, float sw, float sh, float scale, float a, float x2, float y2) {

		if (n > bank_size - 1) {
			return Snil;
		}
		if (BitmapRenderTarget == nullptr)
		{
			return Snil;
		}

		auto sprite_sheet = pSpriteSheet[n];
		if (sprite_sheet == NULL) {
			return Snil;
		}
		const auto size = sprite_sheet->GetPixelSize();
		const auto dest = D2D1::RectF(dx, dy, scale * (dx + dw), scale * (dy + dh));
		const auto source = D2D1::RectF(sx, sy, sx + sw, sy + sh);
		const auto opacity = 1.0f;
		const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x2, y2));
		BitmapRenderTarget->BeginDraw();
		BitmapRenderTarget->SetTransform(rot);
		BitmapRenderTarget->DrawBitmap(sprite_sheet, dest, 1.0f,
			D2D1_BITMAP_INTERPOLATION_MODE::D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, source);
		BitmapRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		BitmapRenderTarget->EndDraw();
		return Strue;
	}



	void CreateFactory() {

		if (pD2DFactory == NULL) {
			HRESULT hr = D2D1CreateFactory(
				D2D1_FACTORY_TYPE_MULTI_THREADED, &pD2DFactory);
			d2d_DPIScale(pD2DFactory);
		}
		if (pDWriteFactory == NULL) {
			HRESULT hr = DWriteCreateFactory(
				DWRITE_FACTORY_TYPE_SHARED,
				__uuidof(IDWriteFactory),
				reinterpret_cast<IUnknown**>(&pDWriteFactory)
			);
		}
	}

	HRESULT Create_D2D_Device_Dep(HWND h)  
	{
		if (IsWindow(h)) {

			if (pRenderTarget == NULL)
			{
				appendTranscriptNL("New render target");
				CreateFactory();

				RECT rc;
				GetClientRect(h, &rc);

				D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

				HRESULT hr = pD2DFactory->CreateHwndRenderTarget(
					D2D1::RenderTargetProperties(),
					D2D1::HwndRenderTargetProperties(h,
						D2D1::SizeU((UINT32)rc.right, (UINT32)rc.bottom)),
					&pRenderTarget);

				if (FAILED(hr)) {
					appendTranscriptNL("New render target: Failed");
					return hr;
				}

				d2d_CreateGridPatternBrush(pRenderTarget, &pPatternBrush);

				hr = pDWriteFactory->CreateTextFormat(
					L"Consolas",
					NULL,
					DWRITE_FONT_WEIGHT_REGULAR,
					DWRITE_FONT_STYLE_NORMAL,
					DWRITE_FONT_STRETCH_NORMAL,
					32.0f,
					L"en-us",
					&TextFont
				);

				if (FAILED(hr)) {
					appendTranscriptNL("New render target: Text Format Failed");
					return hr;
				}

				hr = pRenderTarget->CreateSolidColorBrush(
					D2D1::ColorF(D2D1::ColorF::Black),
					&pBlackBrush
				);

				if (FAILED(hr)) {
					appendTranscriptNL("New render target: Black Brush Failed");
					return hr;
				}

	
				return hr;
			}
		}
		return 0;
	}


	void safe_release() {

		SafeRelease(&pRenderTarget);
		SafeRelease(&pPatternBrush);
		SafeRelease(&pBlackBrush);
		SafeRelease(&pD2DFactory);
		SafeRelease(&TextFont);
		SafeRelease(&BitmapRenderTarget);
		SafeRelease(&BitmapRenderTarget2);


	}


	ptr d2d_release() {
		
		safe_release();
 
		return Strue;
	}


	void onPaint(HWND hWnd) {

		WaitForSingleObject(g_image_rotation_mutex, INFINITE);

		if (main_window == nullptr) {
			main_window = hWnd;
		}

		if (main_window != hWnd) {
			main_window = hWnd;
			appendTranscriptNL("Window change detected");
			safe_release();
		}

		PAINTSTRUCT ps;
		HDC hdc = ::BeginPaint(hWnd, &ps);
		RECT rc;
		::GetClientRect(hWnd, &rc);
		D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);
		HRESULT

		hr = Create_D2D_Device_Dep(hWnd);
		if (SUCCEEDED(hr))
		{

			pRenderTarget->Resize(size);
			pRenderTarget->BeginDraw();
			D2D1_SIZE_F renderTargetSize = pRenderTarget->GetSize();
			pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::LightGray));

			pRenderTarget->FillRectangle(
				D2D1::RectF(0.0f, 0.0f, renderTargetSize.width, renderTargetSize.height), pPatternBrush);
	 
			if (bitmap2 != nullptr) {
				pRenderTarget->DrawBitmap(bitmap2, D2D1::RectF(0.0f, 0.0f, prefer_width, prefer_height));
			}
			hr = pRenderTarget->EndDraw();

			if (FAILED(hr)) {
				appendTranscriptNL("Draw failed!");
				safe_release();
			}

		}

		if (hr == D2DERR_RECREATE_TARGET)
		{
			safe_release();
			appendTranscriptNL("image was released by: D2DERR_RECREATE_TARGET");
		 
		}
		if (FAILED(hr)) {
			safe_release();
			appendTranscriptNL("Render target failed to be created.");
		}
		else
		{
			::ValidateRect(hWnd, NULL);
		}

		::EndPaint(hWnd, &ps);
		ReleaseMutex(g_image_rotation_mutex);
	}

	void step(ptr lpParam) {

		WaitForSingleObject(g_image_rotation_mutex, INFINITE);

		if (Sprocedurep(lpParam)) {
			Scall0(lpParam);
		}

		ReleaseMutex(g_image_rotation_mutex);
	}



	void add_commands() {
	 
		Sforeign_symbol("d2d_matrix_identity", static_cast<ptr>(d2d_matrix_identity));
		Sforeign_symbol("d2d_matrix_rotate", static_cast<ptr>(d2d_matrix_rotate));
		Sforeign_symbol("d2d_render_sprite", static_cast<ptr>(d2d_render_sprite));
		Sforeign_symbol("d2d_render_sprite_rotscale", static_cast<ptr>(d2d_render_sprite_rotscale));
		Sforeign_symbol("d2d_render_sprite_sheet", static_cast<ptr>(d2d_render_sprite_sheet));
		Sforeign_symbol("d2d_render_sprite_sheet_rot_scale", static_cast<ptr>(d2d_render_sprite_sheet_rot_scale));
		Sforeign_symbol("d2d_load_sprites", static_cast<ptr>(d2d_load_sprites));
		Sforeign_symbol("d2d_write_text", static_cast<ptr>(d2d_write_text));
		Sforeign_symbol("d2d_set_font", static_cast<ptr>(d2d_set_font));
		Sforeign_symbol("d2d_color", static_cast<ptr>(d2d_color));
		Sforeign_symbol("d2d_fill_color", static_cast<ptr>(d2d_fill_color));
		Sforeign_symbol("d2d_rectangle", static_cast<ptr>(d2d_rectangle));
		Sforeign_symbol("d2d_fill_rectangle", static_cast<ptr>(d2d_fill_rectangle));
		Sforeign_symbol("d2d_ellipse", static_cast<ptr>(d2d_ellipse));
		Sforeign_symbol("d2d_fill_ellipse", static_cast<ptr>(d2d_fill_ellipse));
		Sforeign_symbol("d2d_render", static_cast<ptr>(d2d_render));
		Sforeign_symbol("d2d_show", static_cast<ptr>(d2d_show));
		Sforeign_symbol("d2d_release", static_cast<ptr>(d2d_release));
	}

}
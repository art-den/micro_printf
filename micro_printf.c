#include "micro_printf.h"

#define PRINTF_BOOL unsigned char
#define PRINTF_FALSE 0
#define PRINTF_TRUE 1

#define FMT_FLAG_MINUS 1
#define FMT_FLAG_PLUS 2
#define FMT_FLAG_ZERO 4
#define FMT_FLAG_SPACE 8
#define FMT_FLAG_OCTOTHORP 16

typedef struct Format_
{
	unsigned char flags;
	int width;
	int precision;
	int length;
	char specifier;
} Format;

typedef struct Dest_
{
	PrintfCallBack callback;
	void* data;
	int chars_count;
} Dest;

typedef struct SNPrintfData_
{
	char* buffer_pos;
	char* buffer_end;
} SNPrintfData;

static PRINTF_BOOL put_char(Dest* dest, char character)
{
	PRINTF_BOOL done = dest->callback(dest->data, character);
	if (done) dest->chars_count++;
	return done;
}

static const char* get_format_specifier(Dest* dest, const char* format_str, Format* format_spec)
{
	format_spec->flags = 0;
	format_spec->width = -1;
	format_spec->precision = -1;
	format_spec->length = -1;
	format_spec->specifier = 0;

	if (*format_str == '%')
	{
		put_char(dest, *format_str);
		return format_str + 1;
	}

	const char* initial_format = format_str;
	int int_value = -1;
	PRINTF_BOOL prec_pt = PRINTF_FALSE;

	for (;;)
	{
		unsigned char c = (unsigned char)*format_str++;
		unsigned char flag =
			(c == '-') ? FMT_FLAG_MINUS :
			(c == '+') ? FMT_FLAG_PLUS :
			(c == ' ') ? FMT_FLAG_SPACE :
			(c == '0') ? FMT_FLAG_ZERO :
			(c == '#') ? FMT_FLAG_OCTOTHORP : 0;

		if ((flag != 0) && (int_value == -1))
		{
			if ((format_spec->flags & flag) ||
				(format_spec->width != -1) ||
				(format_spec->precision != -1) ||
				prec_pt
			)
				break;
			else
				format_spec->flags |= flag;
		}

		else if (c == '.')
		{
			if (prec_pt == PRINTF_FALSE)
			{
				format_spec->width = int_value;
				prec_pt = PRINTF_TRUE;
				int_value = -1;
			}
			else
				break;
		}

		else if ((c >= '0') && (c <= '9'))
		{
			if (int_value == -1) int_value = 0;
			int_value *= 10;
			int_value += (c - '0');
		}

		else if (
			(c == 'd') || (c == 'i') || (c == 'u') ||
			(c == 'o') || (c == 'x') || (c == 'X') ||
			(c == 'c') || (c == 's') ||
			(c == 'p'))
		{
			if (prec_pt == PRINTF_FALSE)
				format_spec->width = int_value;
			else
				format_spec->precision = int_value;

			format_spec->specifier = c;

			return format_str;
		}

		else
			break;
	}

	return initial_format;
}

static void print_sign(Dest* dest, PRINTF_BOOL is_negative, const Format* format)
{
	if (is_negative)
		put_char(dest, '-');
	else
	{
		if (format->flags & FMT_FLAG_PLUS)
			put_char(dest, '+');
		else if (format->flags & FMT_FLAG_SPACE)
			put_char(dest, ' ');
	}
}

static void print_format_specifier(Dest* dest, const Format* format)
{
	if (!(format->flags & FMT_FLAG_OCTOTHORP)) return;

	switch (format->specifier)
	{
	case 'X': case 'x':
		put_char(dest, '0');
		put_char(dest, format->specifier);
		break;

	case 'o':
		put_char(dest, '0');
		break;
	}
}

static void print_leading_spaces(Dest* dest, const Format* format, int len)
{
	if (!(format->flags & FMT_FLAG_MINUS) &&
		(format->width != -1) &&
		(format->width > len))
	{
		int chars_to_print = format->width - len;
		char char_to_print = (format->flags & FMT_FLAG_ZERO) ? '0' : ' ';
		for (int i = 0; i < chars_to_print; i++)
			put_char(dest, char_to_print);
	}
}

static void print_after_spaces(Dest* dest, const Format* format, int len)
{
	if ((format->flags & FMT_FLAG_MINUS) &&
		(format->width != -1) &&
		(format->width > len))
	{
		int chars_to_print = format->width - len;
		for (int i = 0; i < chars_to_print; i++)
			put_char(dest, ' ');
	}
}

static void print_uint_impl(Dest* dest, unsigned value, unsigned base, PRINTF_BOOL upper_case)
{
	if (value >= base)
		print_uint_impl(dest, value / base, base, upper_case);

	unsigned value_to_print = value % base;

	char char_to_print =
		(value_to_print < 10)
		? (value_to_print + '0')
		: (value_to_print - 10 + (upper_case ? 'A' : 'a'));

	put_char(dest, char_to_print);
}

static void print_uint(Dest* dest, unsigned value, unsigned base, PRINTF_BOOL upper_case)
{
	if (value == 0)
		put_char(dest, '0');
	else
		print_uint_impl(dest, value, base, upper_case);
}

static unsigned char find_integer_len(unsigned value, unsigned base)
{
	unsigned char len = 0;
	while (value != 0) { value /= base; len++; }
	if (len == 0) len = 1;
	return len;
}

static void print_i(Dest* dest, const Format* format, int value)
{
	PRINTF_BOOL is_negative = value < 0;
	if (is_negative) value = -value;

	// find length
	unsigned char len = find_integer_len(value, 10);
	if ((format->flags & (FMT_FLAG_PLUS | FMT_FLAG_SPACE)) || is_negative) len++;

	// sign
	if (format->flags & FMT_FLAG_ZERO)
		print_sign(dest, is_negative, format);

	// leading spaces or zeros
	print_leading_spaces(dest, format, len);

	// sign
	if (!(format->flags & FMT_FLAG_ZERO))
		print_sign(dest, is_negative, format);

	// integer
	print_uint(dest, value, 10, PRINTF_FALSE);

	// after spaces or zeros
	print_after_spaces(dest, format, len);
}

static void print_u(Dest* dest, const Format* format, unsigned value)
{
	unsigned base =
		(format->specifier == 'u') ? 10 :
		(format->specifier == 'x') ? 16 :
		(format->specifier == 'X') ? 16 :
		(format->specifier == 'p') ? 16 :
		(format->specifier == 'o') ? 8 : 10;

	// find length
	unsigned char real_len = find_integer_len(value, base);
	unsigned char len = (format->specifier != 'p') ? real_len : 2 * sizeof(void*);

	// format specifier
	if (format->flags & FMT_FLAG_ZERO)
		print_format_specifier(dest, format);

	// leading spaces or zeros
	print_leading_spaces(dest, format, len);

	// format specifier
	if (!(format->flags & FMT_FLAG_ZERO))
		print_format_specifier(dest, format);

	// integer
	PRINTF_BOOL upper_case = (format->specifier == 'X') ? PRINTF_TRUE : PRINTF_FALSE;
	if (format->specifier == 'p') // zeros before pointer
	{
		unsigned char zeros_count = len - real_len;
		for (unsigned char i = 0; i < zeros_count; i++)
			put_char(dest, '0');
	}
	print_uint(dest, value, base, upper_case);

	// after spaces or zeros
	print_after_spaces(dest, format, len);
}

static void print_s(Dest* dest, const Format* format, const char* str)
{
	// find string len
	int len = 0;
	for (; str[len]; len++) {}

	// leading spaces or zeros
	print_leading_spaces(dest, format, len);

	// string
	while (*str) put_char(dest, *str++);

	// after spaces or zeros
	print_after_spaces(dest, format, len);
}

static void print_c(Dest* dest, const Format* format, char value)
{
	int len = 1;

	// leading spaces or zeros
	print_leading_spaces(dest, format, len);

	// charaster
	put_char(dest, value);

	// after spaces or zeros
	print_after_spaces(dest, format, len);
}

static void print_by_format_specifier(Dest* dest, const Format* format, va_list* arg_ptr)
{
	switch (format->specifier)
	{
	case 'i': case 'd':
		print_i(dest, format, va_arg(*arg_ptr, int));
		break;

	case 'u': case 'x': case 'X': case 'o': case 'p':
		print_u(dest, format, va_arg(*arg_ptr, unsigned)); // "%p" works only if sizeof(unsigned) >= sizeof(void*)
		break;

	case 's':
		print_s(dest, format, va_arg(*arg_ptr, const char*));
		break;

	case 'c':
		print_c(dest, format, va_arg(*arg_ptr, unsigned) && 0xFF);
		break;
	}
}

int cb_vprintf(PrintfCallBack callback, void* data, const char* format_str, va_list arg_ptr)
{
	Format format_spec;
	Dest dest;

	dest.callback = callback;
	dest.data = data;
	dest.chars_count = 0;

	while (*format_str)
	{
		char chr = *format_str++;

		if (chr == '%')
		{
			format_str = get_format_specifier(&dest, format_str, &format_spec);
			if (format_spec.specifier != 0)
				print_by_format_specifier(&dest, &format_spec, &arg_ptr);
		}
		else
		{
			put_char(&dest, chr);
		}
	}

	return dest.chars_count;
}

int cb_printf(PrintfCallBack callback, void* data, const char* format_str, ...)
{
	va_list arg_list;
	va_start(arg_list, format_str);
	int result = cb_vprintf(callback, data, format_str, arg_list);
	va_end(arg_list);
	return result;
}

static int snprintf_callback(void* data, char character)
{
	SNPrintfData* sprintf_data = (SNPrintfData*)data;

	if (sprintf_data->buffer_pos != sprintf_data->buffer_end)
	{
		*sprintf_data->buffer_pos++ = character;
		return 1;
	}

	return 0;
}

int snprintf(char* buffer, unsigned buffer_len, const char* format_str, ...)
{
	va_list arg_list;
	va_start(arg_list, format_str);
	int result = vsnprintf(buffer, buffer_len, format_str, arg_list);
	va_end(arg_list);
	return result;
}

int vsnprintf(char* buffer, unsigned buffer_len, const char* format_str, va_list arg_ptr)
{
	SNPrintfData data;
	data.buffer_pos = buffer;
	data.buffer_end = buffer + buffer_len;

	int result = cb_vprintf(snprintf_callback, &data, format_str, arg_ptr);

	// add string null terminator
	if (snprintf_callback(&data, 0))
		result++;
	else
		buffer[buffer_len - 1] = 0;

	return result;
}

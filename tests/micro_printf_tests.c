#include <string.h>
#include <assert.h>

#include "../micro_printf.h"

static void check(const char* desire, const char* format, ...)
{
	char buffer[256];

	va_list arg_list;
	va_start(arg_list, format);
	int len = vsnprintf(buffer, sizeof(buffer), format, arg_list);
	va_end(arg_list);

	assert(strcmp(buffer, desire) == 0); // test result string
	assert((strlen(buffer)+1) == len);   // test return value
}

static void test_misk()
{
	check("", "");
	check("%", "%%");
	check("102030", "%i%i%i", 10, 20, 30);
	check("a2030", "%c%i%i", 'a', 20, 30);
}

static void test_integer()
{
	check("10",      "%i",      10);
	check("-123",    "%d",      -123);
	check("[10]",    "[%i]",    10);
	check("[-10]",   "[%i]",    -10);
	check("[  -10]", "[%5i]",   -10);
	check("[-10  ]", "[%-5i]",  -10);
	check("[00010]", "[%05i]",  10);
	check("[+0010]", "[%+05i]", 10);
	check("[-0010]", "[%+05i]", -10);
	check("[ 0010]", "[% 05i]", 10);
	check("[-0010]", "[% 05i]", -10);
	check("[ 10]",   "[% i]",   10);
	check("[-10]",   "[% i]",   -10);
	check("[-10  ]", "[% -5i]", -10);
	check("[ 10  ]", "[% -5i]",  10);
	check("[-10  ]", "[%-+5i]", -10);
}

static void test_unsigned()
{
	check("1234",       "%x",      0x1234);
	check(" 123b",      "%5x",     0x123b);
	check("0x123a",     "%#x",     0x123a);
	check("0X123A",     "%#X",     0x123a);
	check("[  0x123a]", "[%#8x]",  0x123a);
	check("[0x123a  ]", "[%-#8x]", 0x123a);
	check("[123a    ]", "[%-8x]",  0x123a);
	check("[    123a]", "[%8x]",   0x123a);

	check("12345",      "%o",      012345);
	check("012345",     "%#o",     012345);
	check("[  012345]", "[%#8o]",  012345);
	check("[   12345]", "[%8o]",   012345);
	check("[12345   ]", "[%-8o]",  012345);
	check("[012345  ]", "[%#-8o]", 012345);

	check("00123abc",   "%p", (void*)0x0123abc); // ok only at 32 bit platforms
}

static void test_string()
{
	check("str",   "%s",   "str");
	check("  str", "%5s",  "str");
	check("str  ", "%-5s", "str");
}

static void test_char()
{
	check("a",       "%c",     'a');
	check("[a]",     "[%c]",   'a');
	check("[    a]", "[%5c]",  'a');
	check("[a    ]", "[%-5c]", 'a');
}

int main()
{
	test_misk();
	test_integer();
	test_unsigned();
	test_string();
	test_char();

	return 0;
}
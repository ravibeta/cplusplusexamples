// StringReverse2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

void reverse(char* s, int start, int end)
{
	while (start < end)
	{
		char t = s[start];
		s[start] = s[end];
		s[end] = t;
		start++;
		end--;
	}
}

void string_reverse1(char *string) 
{ 
	char* s = string;	
	int start = 0;
	int end = start;

	while(s && *s)
	{
		end++;
		s++;
	}

	if (end > start)
	{
		end--;
		s = string;
		reverse(string, start, end);
	}

	//reverse each word
	start = 0;
	end = start;
	s = string;
	while(*s)
	{
		while(*s && (*s == ' ' || *s == '\t'))
		{
			start++;
			end++;
			s++;
		}

		while(*s && *s != ' ' && *s != '\t')
		{
			end++;
			s++;
		}
		if (end > start) 
			end--;
		reverse(string, start, end);
		start = end + 1;
		end = start;
	}
 }

int strlen(const char* s)
{
	int len = 0;
	while(s && *s)
	{
		len++;
		s++;
	}
	return len;
}

char *string_reverse2(const char *string)
 {
	int len = strlen(string);
	char* ptr = new char[len + 1];	
	//memset(ptr, 0, len*sizeof(char));
	memcpy(ptr, string, len);
	ptr[len] ='\0';

	string_reverse1(ptr);
	
	return ptr;
 }


int _tmain(int argc, _TCHAR* argv[])
{
	const char a[256] = {"this old fox"};
	char* ra = string_reverse2(a);
	printf("%s\n",ra);
	string_reverse1(ra);
	printf("%s\n",ra);

	const char b[256] = {" a "};
	char* rb = string_reverse2(b);
	printf("%s\n",rb);
	string_reverse1(rb);
	printf("%s\n",rb);


	char c[256] = {" \t"};
	char* rc = string_reverse2(c);
	printf("%s\n",rc);
	string_reverse1(rc);
	printf("%s\n",rc);

	char d[256] = {" ab  ba\t"};
	char* rd = string_reverse2(d);
	printf("%s\n",rd);
	string_reverse1(rd);
	printf("%s\n",rd);

	delete [] ra;
	delete [] rb;
	delete [] rc;
	delete [] rd;


	return 0;
}

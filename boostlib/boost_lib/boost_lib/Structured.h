#include "stdafx.h"
#include <stdlib.h>
#include <iostream>

using namespace std;

typedef int num;
enum SectionType
{
	HEADING = 0,
	ILLUSTRATION = 1
};

template <typename Tkey, typename Tval> 
class Structure
{
	int level;
	SectionType type;
	long begin;
	long end;
};
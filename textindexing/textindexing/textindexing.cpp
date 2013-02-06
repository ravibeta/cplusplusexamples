// textindexing.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "skiplist.h"
#include <string>
#include <list>

using namespace std;

class WordInfo
{
public:
	char word[32];
	int freq;
	list<int> offset;
};

struct Word
{
	TCHAR actual[64];
	TCHAR canonical[64];
	int offset;
	int freq;
	int pageno;
	TCHAR tags[32];
  };

	SkipList<string, WordInfo>* Parse(const string& text)
	{
	string sentinel = "zzzzzzz";
	SkipList<string, WordInfo>* skiplist = new SkipList<string, WordInfo>((float)0.5, 6, &sentinel);

	const char* start = text.c_str();
	const char* curr = text.c_str();
	const char* end = text.c_str();
	int offset = 0;
	string::const_iterator ps = text.begin();
	int count = 0;
	for (string::const_iterator p = text.begin(); p != text.end(); p++, end++)
	{
		if ( *p == ' ' )
		{
			if ( end > curr && end+1-curr < 32)
			{
				string w = text.substr(curr-start, end-curr);
				for (string::iterator r = w.begin(); r != w.end(); r++) *r = tolower(*r);
				string* word = &w;
				WordInfo* winfo = skiplist->retrieve(word);
				if (!winfo)
				{
					winfo = new WordInfo();
					winfo->word[0] = '\0';
					strncpy(winfo->word, w.c_str(), end-curr);
					winfo->word[end-curr] = '\0';	
					winfo->offset.push_back(curr-start);
					winfo->freq = 1;
					count++;
				}
				else
				{
					winfo->offset.push_back(curr-start);
					winfo->freq++;
				}
				curr = end;
			}
			else
			{
				// end++;
				curr++;
				ps = p;
			}
		}
		else
		{
			if ( curr > start && *curr == ' ') curr++;
			// end++;
			ps++;
		}
	}
	return skiplist;
	}

	int _tmain(int argc, _TCHAR* argv[])
	{
		const string stext = "Clustering and Segmentation. Clustering is a data mining technique that is directed towards the goals of identification and classification. Clustering tries to identify a finite set of categories or clusters to which each data object (tuple) can be mapped. The categories may be disjoint or overlapping and may sometimes be organized into trees. For example, one might form categories of customers into the form of a tree and then map each customer to one or more of the categories. A closely related problem is that of estimating multivariate probability density functions of all variables that could be attributes in a relation or from different relations.";
		SkipList<string, WordInfo>* skiplist  = Parse(stext);
		
		return 0;
	}


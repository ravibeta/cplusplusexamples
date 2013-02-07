// textindexing.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "skiplist.h"
#include <string>
#include <list>
#include "slist.h"

using namespace std;

struct WordInfo
{
	char word[32];
	int freq;
	int offset[32];
	int index;
	WordInfo()
	{
		for(int i = 0; i < 32; i++)
		{
			word[i] = '\0';
			offset[i] = '\0';
		}
		freq = 0;
		index = 0;
	}
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

	string clean(string w)
	{
		for (string::iterator r = w.begin(); r != w.end(); r++) *r = tolower(*r);
		while(w.find(",") == w.length() - 1)
			w.replace(w.find(","), 1, "");		
		while(w.find(";") == w.length() - 1)
			w.replace(w.find(";"), 1, "");
		while(w.find(".") == w.length() - 1)
			w.replace(w.find("."), 1, "");
		while(w.find(".") == w.length() - 1)
			w.replace(w.find(":"), 1, "");
		while(w.find(" ") == w.length() - 1)
			w.replace(w.find(" "), 1, "");
		return w;
	}


	skiplist<string, WordInfo>* Parse(const string& text)
	{
	string sentinel = "zzzzzzz";
	skiplist<string, WordInfo>* slist = new skiplist<string, WordInfo>(5);
	
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
				string w = clean(text.substr(curr-start, end-curr));
				string* word = &w;

				WordInfo* winfo = new WordInfo();
				winfo->word[0] = '\0';
				strncpy(winfo->word, w.c_str(), end-curr);
				winfo->word[end-curr] = '\0';	
				WordInfo ws = slist->search(w);
				if (ws.freq > 0 )
				{
					winfo->freq = ws.freq + 1;
					for (int i = 0; i < winfo->index; i++)
						winfo->offset[i] = ws.offset[i];
					winfo->index = ws.index;
					if (winfo->index < 31)
						winfo->index++;
				}
				else
				{
					winfo->freq = 1;
					winfo->index = 0;
					count++;
				}
				winfo->offset[winfo->index] = curr-start;
				slist->insert(w, *winfo);
				curr = end;
			}
			else
			{
				curr++;
				ps = p;
			}
		}
		else
		{
			if ( curr > start && *curr == ' ') curr++;
			ps++;
		}
	}
	return slist;
	}

	int _tmain(int argc, _TCHAR* argv[])
	{
		const string stext = "Clustering and Segmentation. Clustering is a data mining technique that is directed towards the goals of identification and classification. Clustering tries to identify a finite set of categories or clusters to which each data object (tuple) can be mapped. The categories may be disjoint or overlapping and may sometimes be organized into trees. For example, one might form categories of customers into the form of a tree and then map each customer to one or more of the categories. A closely related problem is that of estimating multivariate probability density functions of all variables that could be attributes in a relation or from different relations.";
		skiplist<string, WordInfo>* skiplist  = Parse(stext);
	    
		return 0;
	}


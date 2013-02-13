#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include <boost/progress.hpp>

#include "wordnet.h"
#include <load_wordnet.h>
#include <info_helper.h>
#include <nltk_similarity.h>
#include <std_ext.h>
#include "slist.h"

using namespace wnb;
using namespace std;
bool usage(int argc, char ** argv)
{
  std::string dir;
  if (argc >= 2)
    dir = std::string(argv[1]);
  if (argc != 3 || dir[dir.length()-1] != '/')
  {
    std::cout << argv[0] << " .../wordnet_dir/ word" << std::endl;
	std::cout << "Specify text in input.txt file" << std::endl;
    return true;
  }
  return false;
}

struct ws1
{
  std::string w;
  float       s;

  bool operator<(const ws1& a) const {return s > a.s;}
};
struct WordInfo
{
	char word[32];
	int freq;
	int offset[32];
	int index;
	WordInfo()
	{
		word[0] = '\0';
		offset[0] = 0;
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




/// Compute similarity of word with words in word list
std::vector<ws1>
compute_similarities(wordnet& wn,
                     const std::string& word,
                     const std::vector<std::string>& word_list)
{
  std::vector<ws1> wslist;
  std::vector<synset> synsets1 = wn.get_synsets(word);

  for (unsigned i = 0; i < synsets1.size(); i++)
    for (unsigned k = 0; k < synsets1[i].words.size(); k++)
      std::cout << " - " << synsets1[i].words[k] << std::endl;

  nltk_similarity path_similarity(wn);
  {
    boost::progress_timer t;
    boost::progress_display show_progress(word_list.size());

    for (unsigned k = 0; k < word_list.size(); k++)
    {
      const std::string& w = word_list[k];
      float max = 0;
      std::vector<synset> synsets2 = wn.get_synsets(w);
      for (unsigned i = 0; i < synsets1.size(); i++)
      {
        for (unsigned j = 0; j < synsets2.size(); j++)
        {
          float s = path_similarity(synsets1[i], synsets2[j], 6);
          if (s > max)
            max = s;
        }
      }
      ws1 e = {w, max};
      wslist.push_back(e);
      ++show_progress;
    }
  }

  return wslist;
}

void similarity_test(wordnet&     wn,
                     const std::string& word,
                     std::vector<std::string>& word_list)
{
  std::vector<ws1> wslist = compute_similarities(wn, word, word_list);

  std::stable_sort(wslist.begin(), wslist.end());
  for (unsigned i = 0; i < std::min(wslist.size(), size_t(10)); i++)
    std::cout << wslist[i].w << " " << wslist[i].s << std::endl;
}

void batch_test(wordnet& wn,
                std::vector<std::string>& word_list)
{
  for (std::size_t i = 0; i < word_list.size(); i++)
  {
    std::string& word = word_list[i];
    std::cout << "=== " << word << " === \n";

    std::vector<synset> synsets = wn.get_synsets(word);
    std::cout << synsets.size() << "\n";
    for (std::size_t j = 0; j < synsets.size(); j++)
    {
      std::cout << "  pos " << (int)synsets[j].pos << "| ";
      for (std::size_t k = 0; k < synsets[j].words.size(); k++)
        std::cout <<  synsets[j].words[k] << ", ";
    std::cout << std::endl;
    }
    std::cout << std::endl;
  }
}

int main(int argc, char ** argv)
{
  if (usage(argc, argv))
    return 1;

  // read command line
  std::string wordnet_dir = argv[1];
  std::string word        = argv[2];

  wordnet wn(wordnet_dir);

  // read input file
  std::string list = ext::read_file("input.txt");
  skiplist<string, WordInfo>* skiplist  = Parse(list);
  std::vector<std::string> wl        =  ext::split(list);
  std::vector<std::string> word_list =  ext::s_unique(wl);
  std::list<std::string>* selected = new std::list<std::string>();
  for (std::size_t k = 0; k < word_list.size(); k++)
  {
	  string s = clean(word_list[k]);
	  WordInfo wi = skiplist->search(s);
	  if( wi.freq > 2 && strlen(wi.word) >= 5)
		  selected->push_back(s);
  }

  cout << "Index" << endl;
  cout << "-----" << endl;
  for (std::list<string>::iterator i = selected->begin(); i != selected->end(); i++)
	cout << *i << endl;
  cout << "-----" << endl;
  
}


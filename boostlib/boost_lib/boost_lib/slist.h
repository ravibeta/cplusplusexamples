#include "stdafx.h"

#include <stdlib.h>
#include <iostream>
using namespace std;
  
typedef int num;// int, long, long long -- choose what you like

template <typename Tkey, typename Tval>
  class skiplist{
    
  private:
    
    struct node
    {
      Tkey key_;           // the key
      Tval val_;           // the val
      num size_;           // size of array   
      node **next_;        // array of pointers
	  bool visited;

      node(const Tkey key, const Tval val, num size) :
	key_(key), val_(val), size_(size), next_(new node*[size_]), visited(false)
      { for ( num i = 0 ; i < size_; ++i) next_[i] = 0; 
	//std::cerr << "\t Created node key="<< key 
	//	  << " val=" << val << " num_links=" << size 
	//	  << std::endl;
      }
      
    };

    typedef node* const link;

    Tkey default_key_;     // default item in skiplist
    Tval default_val_;     // default val in skiplist
    link head_;            // head of skiplist
    
    num lgN_;              // current number of link in skiplist
    num lgNmax_;           // max number of link in skiplist
	int count;             // number of nodes
	int index;

    // random skiplist expansion
    num rand_sl_gen__();     

    // remove a key and a val
    void remove__(link t, Tkey key, num k);

    // remove all keys
    bool remove_all__(link t, num k);

    // search a key, given a link and the current level
    Tval search__(link t, const Tkey key, num k) const;

    // search a key, given a link and the current level
    void* searchnode__(link t, const Tkey key, num k);

    // insert the new node pointed by link x with level k
    void insert__(link t, link x, num k);

	// print all nodes
	void dump__(link t, num k);

	void dumped__(link t);

	void dumpall__(link t, num k);

  public: 
    
    skiplist(num lgNmax = 5) : 
      head_(new node(default_key_, default_val_, lgNmax+1)), count(0), index(0),
      lgN_(0), lgNmax_(lgNmax) {}; 

    // TODO
    ~skiplist(){ 
		remove_all__(head_, lgN_);
	};

    // search a key and get a val
    //   start from head_ and current level reached lgN_
    inline Tval search(const Tkey key) const 
    { return search__(head_, key, lgN_); };

    // insert a key and a val
    //   start from head_ and 
    //              and the new randomized node with 
    //              j=rand_sl_gen() links built with 
    //              probability 1/t^j
    //   current level is lgN_
    //
    inline void insert(const Tkey key, const Tval val) 
    { 
		node* n = (node*)searchnode__(head_, key, lgN_);
		if(n)
			n->val_ = val;
		else
			insert__(head_, new node(key, val, rand_sl_gen__()), lgN_); 
	};

    
    inline void remove(Tkey key)
    { remove__(head_, key, lgN_-1); }



	inline bool removeall()
	{
		return remove_all__(head_, lgN_);
	}

	inline node** updateTable(Tkey key)
	{
		node* current = head_;
		node** ret = new node*[lgNmax_] ();
		if ( ret != NULL)
			for (int i = head_->size_ - 1; i >= 0; i--)
			{
				while(current->next_[i] && current->next_[i]->key_ < key && current->key_ < current->next_[i]->key_)
					current = current->next_[i];
				ret[i] = current;

			}
		return ret;
	}

	inline void insertIter(const Tkey key, const Tval val) 
    { 
		node** ret = updateTable(key);
		if (!ret) return;
		node* cur = ret[0];
		if (cur->next_[0] && cur->next_[0]->key_ == key && cur->key_ < cur->next_[0]->key_)
		{
			cur->next_[0]->val_ = val;
			// std::cerr << "insertIter update val" << n->val_  << std::endl;
			return;
		}

		node* n  = new node(key, val, rand_sl_gen__());
		if (!n) return;
		// for (int i = 0; i < n->size_; i++)
		for (int i = n->size_ - 1; i >=0 ; i--)
		{
			if ( i < lgNmax_)
			{
				n->next_[i] = ret[i]->next_[i];
#ifdef DEBUG
				std::cerr << "insertIter" << n->val_  << "level " << i << "next " << ret[i]->next_[i] << std::endl;
#endif
				ret[i]->next_[i] = n;
			}
		}
	}

	    
	inline Tval searchIter(const Tkey key) 
    {  
	    node** ret = updateTable(key);
		if (ret[0] && ret[0]->next_[0] && ret[0]->next_[0]->key_ == key)
			return ret[0]->next_[0]->val_;
		return default_val_; 
	};

	inline void dumpIter()
	{
		node* cur = head_;
		while(cur->next_[0])
		{
// #ifdef DEBUG
			std::cerr << "dump__ " << cur->val_ <<std::endl;
// #endif
			cur = cur->next_[0];
		}
	}

  };
/////////////////// PRIVATE //////////////////////

// generate 2^i, as side effect store max level currently reached
template <typename Tkey, typename Tval>
num skiplist<Tkey, Tval>::rand_sl_gen__()
{
  num i, j, t = rand();                    // t is in [0, RAND_MAX]

  for (i=1, j=2; i < lgNmax_; i++, j+=j)   // generate j=2, 4, ... i < lgNmax_
    if (t > RAND_MAX/j) break;             // t > RAND_MAX / 2^i 
                                           // t is a number < 1/2^i
  if (i > lgN_)                            // expand current level
    lgN_ = i;                              // this grows logaritimcally 

  return i;
}

// search skip list give the entry link, the key and the current level
template <typename Tkey, typename Tval>
Tval skiplist<Tkey, Tval>::search__(link t, const Tkey key, num k) const
{
#ifdef DEBUG
  std::cerr << "search__ " << t << " key=" << key << " level=" << k << std::endl;
#endif
  if (t == 0)                          // search failed
    return default_val_; 
#ifdef DEBUG
  std::cerr << "key =" << key << " val = " << t->val_<< std::endl;
#endif
  
  if (key == t->key_)                 // search success
    return t->val_; 

  link x = t->next_[k];                // link to the next level

  if ((x == 0) ||                      //   null?
      (key < x->key_))                 //   search key < next level link's key
    {
      if (k==0)                        // no more levels available
	return default_val_;
 
      return search__(t, key, k-1);    // try previous level
    }
  return search__(x, key, k);          //   keep searching on the same level
};

// search skip list given the entry link, the key and the current level
template <typename Tkey, typename Tval>
void* skiplist<Tkey, Tval>::searchnode__(link t, const Tkey key, num k) 
{
#ifdef DEBUG
  std::cerr << "searchnode__ " << t << " key=" << key << " level=" << k << std::endl;
#endif
  if (t == 0)                          // search failed
    return t; 
#ifdef DEBUG
  std::cerr << "key =" << key << " val = " << t->val_<< std::endl;
#endif
  
  if (key == t->key_)                 // search success
    return t; 

  link x = t->next_[k];                // link to the next level

  if ((x == 0) ||                      //   null?
      (key <= x->key_))                 //   search key < next level link's key
	{
		if ( x && key == x->key_)
			return x;
		if (k==0)                        // no more levels available
			return NULL;
 
		return searchnode__(t, key, k-1);    // try previous level
	}
  return searchnode__(x, key, k);          //   keep searching on the same level
};

//
// insert
//
template <typename Tkey, typename Tval>
void skiplist<Tkey, Tval>::insert__(link t, link x, num k) 
{
 #ifdef DEBUG
  std::cerr << "insert__ " << t << " " << x << " level=" << k 
  	    << " key=" << x->key_ << " value=" << x->val_ << std::endl;
 #endif
  /*if (head_->size_ < x->size_)
	    std::cerr << "mistake" << " value=" << t->val_ << std::endl;*/
  Tkey key = x->key_;      // current key

  link tk  = t->next_[k];  // link to next level current link  
  
  if ((tk == 0) ||         //   null?
      ( key < tk->key_))   //   search key < next level link's key
    {
      if (k < x->size_)    // is curr lev allowed for this node?
	{                  //   insert:
#ifdef DEBUG
		std::cerr << "insert__" << x->val_  << "level " << k << "next " << tk << std::endl;
#endif
		x->next_[k] = tk;//    new node's successor is tk
		t->next_[k] = x; //    t'successor is x  

#ifdef DEBUG
	    std::cerr << "\tdone inserted key=" << key 
		      << " value=" << x->val_ <<std::endl;
#endif
	}
      if (k==0)             // level 0 
	  {
		  count++;
		  return;             //   return
	  }                            // k >= x->size__
      insert__(t, x, k-1);  //  insert down a level
      return;               //  return
    }                       // k > tk->key_

  insert__(tk, x, k);       //   stay in the same level
};


template <typename Tkey, typename Tval>
void skiplist<Tkey, Tval>::remove__(link t, Tkey key, num k) 
{
#ifdef DEBUG
  std::cerr << "remove__ " << t << " key=" << key << " level=" 
	    << k <<std::endl;
#endif
  if (t==0) return;
  link x = t->next_[k];
  
  if ((x!=0) &&
      !(x->key_ < key))            // >=
    {
      if (key == x->key_){          // found it 
	t->next_[k] = x->next_[k]; //   remove
#ifdef DEBUG
	std::cerr  << " done" <<std::endl;
#endif
      }
      
      if (k==0)                    // can delete
	{                          //   no more links in level
	  delete x; 
	  return;
	}
      remove__(t, key, k-1);   // try to remove one level below
    }
  // x->key_ >= key

  remove__(t->next_[k], key, k);// try to remove in the same level
};


template <typename Tkey, typename Tval>
bool skiplist<Tkey, Tval>::remove_all__(link t, num k) 
{

		node* cur = head_;
		while(cur)
		{
			node* next = cur->next_[0];
			delete cur;
#ifdef DEBUG
			std::cerr << "remove_all" << cur->val_ <<std::endl;
#endif
			cur = next;
		}
		return true;
};



/////////////////// PUBLIC ///////////////////////

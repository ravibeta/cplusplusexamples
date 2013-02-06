#ifndef SKIP_LIST
#define SKIP_LIST

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <random>

#include "SkipNode.h"
#include "RandomHeight.h"

using namespace std;

template <class Key, class Obj>
  class SkipList
  {
  public:
    SkipList(float,int,Key*);
    ~SkipList();
 
    bool insert(Key*, Obj*);
    bool remove(Key*);
    Obj* retrieve(Key*);
    void dump(ofstream&);
 
  private:
    SkipNode<Key,Obj>* head;
    SkipNode<Key,Obj>* tail;
    float probability;
    int maxHeight;
    int curHeight;
    RandomHeight* randGen;
  };

#endif // SKIP_LIST

#ifndef SKIP_NODE
#define SKIP_NODE
 
#include "stdafx.h"
// Dr.Dobb's implementation
using namespace std;  
template <class Key, class Obj>
  class SkipList;
 
template <class Key, class Obj>
  class SkipNode
  {
  public:
    SkipNode(Key*, Obj*, int);
    SkipNode(int);
    ~SkipNode();
 
    Key* getKey(void);
    Obj* getObj(void);
    int   getHgt(void);
    SkipNode** fwdNodes;
 
  private:
    int nodeHeight;
    Key* key;
    Obj* obj;
  };
 
template <class Key, class Obj>
  SkipNode<Key,Obj>::~SkipNode()
  {
    delete key;
    delete obj;
    delete [] fwdNodes;
  }
 
template <class Key, class Obj>
  SkipNode<Key,Obj>::SkipNode(Key* k,
    Obj* o, int h)
  {
    nodeHeight = h;
    key = k;
    obj = o;
    fwdNodes =
  new SkipNode<Key,Obj>* [h+1];
    for ( int x = 1; x <= nodeHeight; x++ )
      fwdNodes[x] =
  (SkipNode<Key,Obj>*) NULL;
  }
 
template <class Key, class Obj>
  SkipNode<Key,Obj>::SkipNode(int h)
  {
    nodeHeight = h;
    key = (Key*) NULL;
    obj = (Obj*) NULL;
    fwdNodes =
  new SkipNode<Key,Obj>* [h+1];
    for ( int x = 1; x <= nodeHeight; x++ )
      fwdNodes[x] =
  (SkipNode<Key,Obj>*) NULL;
  }
 
 
template <class Key, class Obj>
  Key* SkipNode<Key,Obj>::getKey(void)
  {
    return key;
  }
 
template <class Key, class Obj>
  Obj* SkipNode<Key,Obj>::getObj(void)
  {
    return obj;
  }
 
template <class Key, class Obj>
  int SkipNode<Key,Obj>::getHgt(void)
  {
    return nodeHeight;
  }
 
/* End of File */
#endif // SKIP_NODE

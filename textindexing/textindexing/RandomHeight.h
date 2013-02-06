#include "stdafx.h"
#ifndef RANDOM_HEIGHT
#define RANDOM_HEIGHT

using namespace std;
// Dr.Dobb's implementation
class RandomHeight
{
  public:
    RandomHeight(int maxLvl, float prob);
    ~RandomHeight() {}
    int newLevel(void);
 
  private:
    int maxLevel;
    float probability;
};
#endif // RANDOM_HEIGHT
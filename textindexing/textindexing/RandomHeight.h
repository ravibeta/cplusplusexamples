#include "stdafx.h"
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

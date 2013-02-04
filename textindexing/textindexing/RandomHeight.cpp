#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <random>
#include "RandomHeight.h"

using namespace std;

// Dr.Dobb's implementation
RandomHeight::RandomHeight
    (int maxLvl, float prob)
{
  // randomize();
  maxLevel = maxLvl;
  probability = prob;
}
 
int RandomHeight::newLevel(void)
{
int tmpLvl = 1;
  // Develop a random number between 1 and
  // maxLvl (node height).
  while ((rand()%2 < probability) &&
         (tmpLvl < maxLevel))
    tmpLvl++;
 
  return tmpLvl;
}
 
//End of File
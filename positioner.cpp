#include "positioner.hpp"

bool Positioner::setFrom(double from)
{
   this->from=from; //todo: maybe we should set it only when its value is valid
   return from>=min() && from<=max();
}

bool Positioner::addMovement(double to, double duration, int trigger)
{
   if(to>=min() && to<=max()){
      movements.push_back(Movement(to,duration,trigger));
      return true;
   }
   else return false;
}

void Positioner::clearCmd()
{
   movements.clear();
}

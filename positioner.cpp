#include "positioner.hpp"

bool Positioner::addMovement(double from, double to, double duration, int trigger)
{
    if ((_isnan(from) && _isnan(to)) || (to >= minPos() && to <= maxPos() && from >= minPos() && from <= maxPos())) {
        movements.push_back(new Movement(from, to, duration, trigger));
        return true;
    }
    else {
        lastErrorMsg = "addMovment: out of range";
        return false;
    }
}

void Positioner::clearCmd()
{
    for (unsigned i = 0; i < movements.size(); ++i)delete movements[i];
    movements.clear();
}

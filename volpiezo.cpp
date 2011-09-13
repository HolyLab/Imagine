#include "volpiezo.hpp"

#include "ni_daq_g.hpp"

NiDaqAoWriteOne * aoOnce=NULL;

double VolPiezo::zpos2voltage(double um)
{
    return um/max()*10; // *10: 0-10vol range for piezo
}


bool VolPiezo::moveTo(double to)
{

}

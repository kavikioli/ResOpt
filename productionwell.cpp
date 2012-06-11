#include "productionwell.h"

#include <iostream>

#include "pipeconnection.h"
#include "binaryvariable.h"
#include "constraint.h"
#include "pipe.h"
#include "midpipe.h"
#include "stream.h"



using std::cout;
using std::endl;

namespace ResOpt
{


ProductionWell::ProductionWell()
    : p_bhp_contraint(new Constraint(0.5, 1.0, 0.0)),
      p_connection_constraint(new Constraint(1.0, 1.0, 1.0))

{

}

ProductionWell::~ProductionWell()
{
    for(int i = 0; i < numberOfPipeConnections(); i++)
    {
        delete pipeConnection(i);
    }
}


//-----------------------------------------------------------------------------------------------
// sets the name of the well (overloaded from Well)
//-----------------------------------------------------------------------------------------------
void ProductionWell::setName(const QString &n)
{
    p_bhp_contraint->setName("Bottomhole pressure constraint for well: " + n);
    p_connection_constraint->setName("Pipe routing constraint for well: " + n);

    Well::setName(n);

}

//-----------------------------------------------------------------------------------------------
// updates the value of the bhp constraint
//-----------------------------------------------------------------------------------------------
void ProductionWell::updateBhpConstraint()
{
    cout << "Updating the BHP constraint for well " << name().toAscii().data() << endl;

    // finding the pipe connection with the highes fraction
    Pipe *p = 0;
    double frac = -1.0;
    for(int i = 0; i < numberOfPipeConnections(); i++)
    {
        if(frac < pipeConnection(i)->variable()->value())
        {
            frac = pipeConnection(i)->variable()->value();
            p = pipeConnection(i)->pipe();
        }
    }

    // checking if the pipe and well has the same number of streams
    if(numberOfStreams() != p->numberOfStreams())
    {
        cout << endl << "### Runtime Error ###" << endl
             << "Well and pipe do not have the same number of time steps..." << endl
             << "WELL: " << name().toAscii().data() << ", N = " << numberOfStreams() << endl
             << "PIPE: " << p->number() << endl << ", N = " << p->numberOfStreams() << endl;

        exit(1);

    }

    // looping through the time steps to find the time steps that violates the constraint the most

    double c = 1.0;
    for(int i = 0; i < numberOfStreams(); i++)
    {
        // the constraint is calculated as:
        // c = (p_wf - p_pipe) / p_wf
        //
        // when the pipe pressure is higher than the bhp, the constraint is violated, and c < 0

        double c_ts = (stream(i)->pressure() - p->stream(i)->pressure()) / stream(i)->pressure();

        // if the current time step violates more, update
        if(c_ts < c) c = c_ts;
    }

    // updating the value of the constraint
    bhpConstraint()->setValue(c);

    // printing if violating
    if(c < 0)
    {
        cout << "BHP constraint for Well " << name().toAscii().data() << " is violated, c = " << c << endl;
    }


}

//-----------------------------------------------------------------------------------------------
// updates the value of the pipe connection constraint
//-----------------------------------------------------------------------------------------------
void ProductionWell::updatePipeConnectionConstraint()
{
    cout << "Updating the routing constraint for well " << name().toAscii().data() << endl;

    double c = 0;

    // looping through the pipe connections, adding the value of the variable
    for(int i = 0; i < numberOfPipeConnections(); i++)
    {
        c += pipeConnection(i)->variable()->value();
    }

    // updating the value of the constraint
    pipeConnectionConstraint()->setValue(c);

}

//-----------------------------------------------------------------------------------------------
// finds the fraction of the rates from this well that flows through a pipe
//-----------------------------------------------------------------------------------------------
double ProductionWell::flowFraction(Pipe *p, bool *ok)
{
    double frac = 0.0;


    for(int i = 0; i < numberOfPipeConnections(); i++)
    {
        // first checking if the well is directly connected to the pipe
        if(p->number() == pipeConnection(i)->pipe()->number())
        {
            frac += pipeConnection(i)->variable()->value();
        }

        // then checking if the well is connected indirectly to the pipe (this only appplies to MidPipes)
        MidPipe *p_mid = dynamic_cast<MidPipe*>(pipeConnection(i)->pipe());
        if(p_mid != 0)
        {
            frac += p_mid->flowFraction(p, ok) * pipeConnection(i)->variable()->value();
        }
    }


    return frac;
}


} // namespace ResOpt

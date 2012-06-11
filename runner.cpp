
#include "runner.h"

#include <iostream>
#include <QTextStream>

#include "modelreader.h"
#include "model.h"
#include "gprssimulator.h"
#include "stream.h"
#include "pressuredropcalculator.h"
#include "pipe.h"
#include "bonminoptimizer.h"
#include "runonceoptimizer.h"
#include "objective.h"
#include "binaryvariable.h"
#include "realvariable.h"
#include "constraint.h"

// needed for debug
#include "productionwell.h"



using std::cout;
using std::endl;

namespace ResOpt
{



Runner::Runner(const QString &driver_file)
    : p_reader(0),
      p_model(0),
      p_simulator(0),
      p_summary(0),
      m_number_of_runs(0),
      m_up_to_date(false)
{
    p_reader = new ModelReader(driver_file);
}

Runner::~Runner()
{
    if(p_model != 0) delete p_model;
    if(p_simulator != 0) delete p_simulator;
    if(p_reader != 0) delete p_reader;
    if(p_optimizer != 0) delete p_optimizer;
}

//-----------------------------------------------------------------------------------------------
// Reads the driver file, and makes the model ready for launch
//-----------------------------------------------------------------------------------------------
void Runner::initialize()
{
    // reading the driver file and initializing the model
    p_model = p_reader->readDriverFile();

    // reading the pipe pressure drop definition files
    p_model->readPipeFiles();

    // resolving separator connections
    p_model->resolveSeparatorConnections();

    // resolving the pipe routing (this must be done before each launch of the model)
    p_model->resolvePipeRouting();

    cout << "Initializing the reservoir simulator..." << endl;
    // initializing the reservoir simulator
    p_simulator = new GprsSimulator();
    p_simulator->setFolder("output");


    cout << "Initializing the optimizer..." << endl;
    // initializing the optimizer
    p_optimizer = new BonminOptimizer(this);
    //p_optimizer = new RunonceOptimizer(this);
    p_optimizer->initialize();

    // setting up the summary file
    setSummaryFile("run_summary.out");
    writeProblemDefToSummary();

    cout << "Done initializing the model..." << endl;


}


//-----------------------------------------------------------------------------------------------
// Main control loop
//-----------------------------------------------------------------------------------------------
void Runner::run()
{

    // checking if the model has been initialized
    if(p_model == 0) initialize();


    // starting the optimizer
    p_optimizer->start();


    ///// debug code  //////



    /*

    cout << "*****  DEBUG  *****" << endl;



    // running the model once

    evaluate();



    // testing arithmetic manipulation of streams
    Stream str;
    str.setGasRate(10000);
    str.setOilRate(1000);
    str.setWaterRate(1.0);
    str.setTime(10);
    str.setPressure(14.7);

    Stream result = str*0.23;

    cout << "--- Streams multiplication ---" << endl;
    result.printToCout();



    // testing the flow fractions code

    ProductionWell *prod1 = dynamic_cast<ProductionWell*>(p_model->well(0));
    if(prod1 != 0)
    {
        cout << "calculating the flow fraction from prod1 to Pipe 1..." << endl;

        double frac = prod1->flowFraction(p_model->pipe(0));

        cout <<"fraction = " << frac << endl;


        cout << "calculating the flow fraction from prod1 to Pipe 2..." << endl;

        frac = prod1->flowFraction(p_model->pipe(1));

        cout <<"fraction = " << frac << endl;

        cout << "calculating the flow fraction from prod1 to Pipe 3..." << endl;

        frac = prod1->flowFraction(p_model->pipe(2));

        cout <<"fraction = " << frac << endl;



    }



    // making a stream
    Stream *str = new Stream();
    str->setGasRate(10000);
    str->setOilRate(1000);
    str->setWaterRate(0.0);
    str->setTime(10);

    // getting the calculator for the first pipe
    PressureDropCalculator *calc = p_model->pipe(1)->calculator();

    // calculating the pressure drop for the stream
    double dp = calc->pressureDrop(str, 57.9);

    cout << "Calculated pressure drop = " << dp << endl;


    // testing the reservoir simulator input file generation
    p_simulator->setFolder("output");
    p_simulator->generateInputFiles(p_model);
    p_simulator->launchSimulator();
    p_simulator->readOutput(p_model);


    // calculating the pipe inlet pressures
    p_model->calculatePipePressures();


    // printing the calculated pressures and rates for PIPE 1

    Pipe *p = p_model->pipe(0);

    cout << "Calculated pressures for PIPE: " << p->number() << endl;
    for(int i = 0; i < p->numberOfStreams(); i++)
    {
        cout << p->stream(i)->time() << "    " << p->stream(i)->pressure() << endl;
    }


    // updating the separator constraints
    p_model->updateSeparatorConstraints();

    */
}


//-----------------------------------------------------------------------------------------------
// Running the model, calculating results
//-----------------------------------------------------------------------------------------------
bool Runner::evaluate()
{
    bool ok = true;
    m_number_of_runs += 1;

    cout << endl << "***** Starting new iteration *****" << endl << endl;

    // running the reservoir simulator
    p_simulator->generateInputFiles(p_model);   // generating input based on the current Model
    p_simulator->launchSimulator();             // running the simulator
    p_simulator->readOutput(p_model);           // reading output from the simulator run, and setting to Model


    // calculating pressures in the Pipe network
    p_model->calculatePipePressures();

    // updating the constraints (this must be done after the pressure calc)
    p_model->updateConstraints();

    // updating the objective
    p_model->updateObjectiveValue();

    // changing the status to up to date
    m_up_to_date = true;

    // writing to summary file
    writeIterationToSummary();

    return ok;
}


//-----------------------------------------------------------------------------------------------
// Initializes the summary file
//-----------------------------------------------------------------------------------------------
void Runner::setSummaryFile(const QString &f)
{
    p_summary = new QFile(p_simulator->folder() + "/" + f);

    if(!p_summary->open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qWarning("Could not connect to summary file: %s", p_summary->fileName().toAscii().data());

        delete p_summary;
        p_summary = 0;
    }
    else
    {
        // deleting content from previous launches
        p_summary->resize(0);
    }

}

//-----------------------------------------------------------------------------------------------
// Writes the problem definition to the summary file
//-----------------------------------------------------------------------------------------------
void Runner::writeProblemDefToSummary()
{
    if(p_summary != 0)
    {
        QTextStream out(p_summary);

        out << "----------------------------------------------------------------------\n";
        out << "------------------------ ResOpt Summary File -------------------------\n";
        out << "----------------------------------------------------------------------\n\n";

        out << "MODEL DESCRIPTION:" << "\n";
        out << "Number of wells      = " << p_model->numberOfWells() << "\n";
        out << "Number of pipes      = " << p_model->numberOfPipes() << "\n";
        out << "Number of separators = " << p_model->numberOfSeparators() << "\n\n";


        QVector<shared_ptr<BinaryVariable> >  binary_vars = p_model->binaryVariables();
        QVector<shared_ptr<RealVariable> > real_vars = p_model->realVariables();
        QVector<shared_ptr<Constraint> > cons = p_model->constraints();


        out << "OPTIMIZATION PROBLEM:" << "\n";
        out << "Number of contineous variables  = " << real_vars.size() << "\n";
        out << "Number of binary variables      = " << binary_vars.size() << "\n";
        out << "Number of constraints           = " << cons.size() << "\n\n";

        out << "CONTINEOUS VARIABLES:" << "\n";
        for(int i = 0; i < real_vars.size(); i++)
        {
            out << "VAR_C" << i +1 << ": " << real_vars.at(i)->name();
            out << ", bounds: (" << real_vars.at(i)->min() << " < " << real_vars.at(i)->value() << " < " << real_vars.at(i)->max() << ")\n";
        }
        out << "\n";

        out << "BINARY VARIABLES:" << "\n";
        for(int i = 0; i < binary_vars.size(); i++)
        {
            out << "VAR_B" << i +1 << ": " << binary_vars.at(i)->name();
            out << ", bounds: (" << binary_vars.at(i)->min() << " < " << binary_vars.at(i)->value() << " < " << binary_vars.at(i)->max() << ")\n";
        }

        out << "\n";

        out << "CONSTRAINTS:" << "\n";
        for(int i = 0; i < cons.size(); i++)
        {
            out << "CON" << i +1 << ": " << cons.at(i)->name();
            out << ", bounds: (" << cons.at(i)->min() << " < c < " << cons.at(i)->max() << ")\n";
        }


        out << "\nMODEL EVALUATIONS:" << "\n";
        out << "----------------------------------------------------------------------\n";


        // header

        out << "#\t" << "OBJ\t";


        for(int i = 0; i < real_vars.size(); i++)
        {
            out << "VAR_C" << i +1 << "\t";
        }

        for(int i = 0; i < binary_vars.size(); i++)
        {
            out << "VAR_B" << i + 1 << "\t";
        }

        for(int i = 0; i < cons.size(); i++)
        {
            out << "CON" << i +1 << "\t";
        }


        out << "\n";


        p_summary->flush();

    }
}

//-----------------------------------------------------------------------------------------------
// Writes the results from the current iteration to the summary file
//-----------------------------------------------------------------------------------------------
void Runner::writeIterationToSummary()
{
    if(p_summary != 0)
    {
        QTextStream out(p_summary);

        out << m_number_of_runs << "\t" << p_model->objective()->value() << "\t";

        QVector<shared_ptr<BinaryVariable> >  binary_vars = p_model->binaryVariables();
        QVector<shared_ptr<RealVariable> > real_vars = p_model->realVariables();
        QVector<shared_ptr<Constraint> > cons = p_model->constraints();

        for(int i = 0; i < real_vars.size(); i++)
        {
            out << real_vars.at(i)->value() << "\t";
        }

        for(int i = 0; i < binary_vars.size(); i++)
        {
            out << binary_vars.at(i)->value() << "\t";
        }

        for(int i = 0; i < cons.size(); i++)
        {
            out << cons.at(i)->value() << "\t";
        }


        out << "\n";

        p_summary->flush();

    }

}

} // namespace ResOpt

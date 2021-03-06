/*
 * This file is part of the ResOpt project.
 *
 * Copyright (C) 2011-2013 Aleksander O. Juell <aleksander.juell@ntnu.no>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */


#ifndef LSHOPTIMIZER_H
#define LSHOPTIMIZER_H

#include "optimizer.h"
#include <QVector>

#include "nomad.hpp"

namespace ResOpt
{

class Runner;
class Pipe;




/**
 * @brief Runs the project once with the starting point values for the variables.
 *
 */
class LshOptimizer : public Optimizer
{
private:

    QVector<Case*> m_solutions;
    Case* p_best_solution;
    Case* p_current_solution;
    Case* p_current_values;
    bool m_last_sol_best;
    bool m_use_nomad;

    void solveContineous();
    void solveContineousIpopt();
    void solveContineousNomad();
    NOMAD::Parameters* generateNomadParameters(NOMAD::Display *disp);

public:
    LshOptimizer(Runner *r);
    virtual ~LshOptimizer();


    virtual void initialize();

    virtual void start();

    virtual QString description() const;

    void sendCasesToOptimizer(CaseQueue *cases);

    void setCurrentSolution(Case *c);

};

} // namespace ResOpt


#endif // LSHOPTIMIZER_H

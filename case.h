/*
 * This file is part of the ResOpt project.
 *
 * Copyright (C) 2011-2012 Aleksander O. Juell <aleksander.juell@ntnu.no>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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


#ifndef CASE_H
#define CASE_H

#include <QVector>

namespace ResOpt
{

class Model;
class Derivative;


/**
 * @brief Container class for storing variable, constraint, and objective values.
 * @details This class is used by the Optimizer and Runner to set up cases to be run by a Launcher. The Optimizer should set the variable values
 *          that should be used when running the given Case. The number of binary and real variables in the Case must match what is in the model.
 *
 *          When the Launcher has finished the model evaluation, it will set the constraint and objective values to the Case, so that they may be
 *          accessed by the Optimizer.
 *
 */
class Case
{
private:
    QVector<double> m_real_var_values;
    QVector<double> m_binary_var_values;
    QVector<int> m_integer_var_values;

    QVector<double> m_constraint_values;
    double m_objective_value;

    QVector<Derivative*> m_constraint_derivatives;
    Derivative *p_objective_derivative;

    double m_infeasibility;

public:
    Case();
    Case(Model *m, bool cpy_output = false); // constructs a case based on the current variable values in the model
    Case(const Case &c, bool cpy_output = false);    // only copies obj and con if cpy_output = true
    ~Case();

    void clearConstraints() {m_constraint_values.resize(0);}

    void printToCout();

    // add functions
    void addRealVariableValue(double v) {m_real_var_values.push_back(v);}
    void addBinaryVariableValue(double v) {m_binary_var_values.push_back(v);}
    void addIntegerVariableValue(int v) {m_integer_var_values.push_back(v);}

    void addConstraintValue(double v) {m_constraint_values.push_back(v);}
    void addConstraintDerivative(Derivative *d) {m_constraint_derivatives.push_back(d);}


    // set functions
    void setObjectiveValue(double v) {m_objective_value = v;}
    void setObjectiveDerivative(Derivative *d) {p_objective_derivative = d;}

    void setRealVariableValue(int i, double v) {m_real_var_values.replace(i,v);}
    void setBinaryVariableValue(int i, double v) {m_binary_var_values.replace(i,v);}
    void setIntegerVariableValue(int i, int v) {m_integer_var_values.replace(i,v);}

    void setInfeasibility(double i) {m_infeasibility = i;}

    // get functions
    int numberOfRealVariables() const {return m_real_var_values.size();}
    int numberOfBinaryVariables() const {return m_binary_var_values.size();}
    int numberOfIntegerVariables() const {return m_integer_var_values.size();}

    int numberOfConstraints() const {return m_constraint_values.size();}
    int numberOfConstraintDerivatives() const {return m_constraint_derivatives.size();}

    double realVariableValue(int i) const {return m_real_var_values.at(i);}
    double binaryVariableValue(int i) const {return m_binary_var_values.at(i);}
    int integerVariableValue(int i) const {return m_integer_var_values.at(i);}

    double constraintValue(int i) const {return m_constraint_values.at(i);}
    double objectiveValue() const {return m_objective_value;}

    Derivative* constraintDerivative(int i) {return m_constraint_derivatives.at(i);}
    Derivative* objectiveDerivative() {return p_objective_derivative;}

    double infeasibility() {return m_infeasibility;}

    // overloaded operators
    Case& operator=(const Case &rhs);


};

} // namespace ResOpt
#endif // CASE_H

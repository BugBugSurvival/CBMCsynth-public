#include "mini_verify.h"
#include "../utils/util.h"

#include <solvers/smt2/smt2_dec.h>
#include <langapi/language_util.h>

#include <util/format_expr.h>
#include <util/replace_expr.h>
#include <util/replace_symbol.h>
#include <util/run.h>

#include "../utils/expr2sygus.h"
#include <iostream>



// adds the constraints to the solver
void mini_verifyt::add_problem(const sygus_problemt &problem, const solutiont &solution, const counterexamplet &cex, decision_proceduret &solver)
{ 
  // expand function applications, and add to solver.
  if(problem.assumptions.size() > 0)
    UNEXPECTEDCASE( "Assumptions are not supported in mini_verify")

    exprt constraints = conjunction(problem.constraints);
    expand_function_applications(constraints, solution.functions);
    expand_function_applications(constraints, problem.defined_functions);
    // expand again, incase the body of any defined functions contained the synth functions
    expand_function_applications(constraints, solution.functions);
    solver.set_to_false(constraints);
    // add counterexample to solver
    for(const auto & c: cex.assignment)
    {
      solver.set_to_true(equal_exprt(c.first, c.second));
    }

}

// verifies whether a candidate solution works for a set of counterexamples
// returns PASS if it works for all counterexamples, FAIL otherwise
mini_verifyt::resultt mini_verifyt::operator()(sygus_problemt &problem,
    const solutiont &solution,const std::vector<counterexamplet> &cex)
  {

    for (const auto &c : cex)
    {
      // get SMT solver
      // we get a new SMT solver for every counterexample because it was
      // easier than resetting the solver. It would be better to do this
      // using solving under assumptions.
      // TODO: implement this using solving under assumptions
      smt2_dect solver(
      ns, "cex_verify", "generated by counterexample verifier",
      "ALL", smt2_dect::solvert::Z3, log.get_message_handler());

      add_problem(problem, solution, c, solver);
      // get the result
      decision_proceduret::resultt result = solver();
      switch (result)
      {
      case decision_proceduret::resultt::D_SATISFIABLE:
      {
        // failed to satisfy a counterexample, return FAIL
        return mini_verifyt::resultt::FAIL;
      }
      case decision_proceduret::resultt::D_ERROR:
      {
        std::cout << "ERROR in verification\n";
        return mini_verifyt::resultt::FAIL;
      }
      case decision_proceduret::resultt::D_UNSATISFIABLE:
      default:
      {
        // do nothing, we satisfied this counterexample
      }
      }
    }
    // we only get here if we satisfied all counterexamples
    return mini_verifyt::resultt::PASS;
  }

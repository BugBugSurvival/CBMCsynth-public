/*******************************************************************\

Module: SMT2 frontend example

Author: Elizabeth Polgreen, epolgreen@gmail.com

\*******************************************************************/

#include "parser.h"
#include "printing_utils.h"
#include "sygus_problem.h"
#include "cvc5_synth.h"
#include "util.h"

#include <fstream>
#include <iostream>

#include <util/cmdline.h>
#include <util/cout_message.h>
#include <util/format_expr.h>
#include <util/namespace.h>
#include <util/replace_symbol.h>
#include <util/simplify_expr.h>
#include <util/symbol_table.h>
#include <util/std_expr.h>
#include <util/expr.h>

#include <solvers/smt2/smt2_dec.h>

#include "constants.h"

void expand_function_applications(exprt &expr, const smt_problemt &problem)
{
  for (exprt &op : expr.operands())
  {
    expand_function_applications(op, problem);
  }
  if (expr.id() == ID_function_application)
  {
    auto &app = to_function_application_expr(expr);

    if (app.function().id() == ID_symbol)
    {
      // look up the symbol
      auto func = to_symbol_expr(app.function());
      auto f_it = problem.defined_functions.find(func);

      if (f_it != problem.defined_functions.end())
      {
        // Does it have a definition? It's otherwise uninterpreted.
        if (!f_it->second.is_nil())
        {
          exprt body = f_it->second;

          if (body.id() == ID_lambda)
          {
            body = to_lambda_expr(body).application(app.arguments());
          }
          expand_function_applications(body, problem); // rec. call
          expr = body;
        }
      }
    }
  }
}

// send the problem to an SMT solver and get the result
decision_proceduret::resultt solve_problem(smt_problemt &problem, namespacet &ns, messaget &message)
{
  message.debug() << "Solving problem with smt2 solver" << messaget::eom;
  smt2_dect solver(
      ns, "fastsynth", "generated by fastsynth",
      problem.logic, smt2_dect::solvert::Z3, message.get_message_handler(), {"-T:" + std::to_string(SMT_SOLVER_TIMEOUT)});

  for (const auto &a : problem.assertions)
  {
    exprt copy_of_a = a;
    expand_function_applications(copy_of_a, problem);
    solver.set_to_true(copy_of_a);
  }

  decision_proceduret::resultt result = solver();

  switch (result)
  {
  case decision_proceduret::resultt::D_SATISFIABLE:
  {
    message.debug() << "RESULT is sat" << messaget::eom;
    for (const auto &e : problem.free_var)
    {
      exprt val = solver.get(e.first);
      problem.free_var[e.first] = val;
    }
    return result;
  }
  case decision_proceduret::resultt::D_UNSATISFIABLE:
  {
    message.debug() << "RESULT is unsat" << messaget::eom;
    return result;
  }

  case decision_proceduret::resultt::D_ERROR:
  default:
  {
    message.debug() << "ERROR from smt solver " << messaget::eom;
    return result;
  }
  }
  // UNREACHABLE
  return decision_proceduret::resultt::D_ERROR;
}

smt_problemt substitute_model_into_problem(const smt_problemt &problem)
{
  smt_problemt new_problem;
  for (const auto &a : problem.assertions)
  {
    replace_symbolt replace_symbol;
    for (const auto &var : problem.free_var)
    {
      if (var.second.is_not_nil())
      {
        replace_symbol.insert(var.first, var.second);
      }
    }

    exprt new_assertion = a;
    replace_symbol(new_assertion);
    new_problem.assertions.push_back(new_assertion);
  }

  sort(new_problem.assertions.begin(), new_problem.assertions.end());
  new_problem.assertions.erase(std::unique(new_problem.assertions.begin(), new_problem.assertions.end()),
                               new_problem.assertions.end());

  new_problem.logic = problem.logic;
  new_problem.comments = problem.comments;
  new_problem.filename = problem.filename;

  for (const auto &f : problem.defined_functions)
  {
    new_problem.defined_functions[f.first] = f.second;
  }

  return new_problem;
}

int smt2_frontend(const cmdlinet &cmdline)
{

  console_message_handlert message_handler;
  messaget message(message_handler);

  // this is our default verbosity
  unsigned int v = messaget::M_STATISTICS;

  if (cmdline.isset("verbosity"))
  {
    v = std::stol(
        cmdline.get_value("verbosity"));
    ;
    if (v > 10)
    {
      v = 10;
    }
  }

  // parse input file
  assert(cmdline.args.size() == 1);
  std::ifstream in(cmdline.args.front());

  if (!in)
  {
    std::cerr << "Failed to open input file" << std::endl;
    return 10;
  }

  symbol_tablet symbol_table;
  namespacet ns(symbol_table);

  message_handler.set_verbosity(v);
  parsert parser(in);
  smt_problemt problem;
  // parse the problem
  try
  {
    parser.parse();
    problem = parser.get_smt_problem();

  }
  catch (const parsert::smt2_errort &e)
  {
    message.error() << e.get_line_no() << ": "
                    << e.what() << messaget::eom;
    return 20;
  }

  if (cmdline.isset("dump-problem"))
  {
    print_smt_problem(problem, message.status());
  }
  if (cmdline.isset("solve-smt"))
  {
    // solve the problem with an smt solver
    decision_proceduret::resultt res = solve_problem(problem, ns, message);
    // print problem and model
    message.status() << "Solving with SMT solver:" << messaget::eom;

    if (res == decision_proceduret::resultt::D_SATISFIABLE)
    {
      // replace the free variables in the assertions with the values from the model
      message.status() << "Problem is SATISFIABLE\n";
      print_model(problem, message.status());
      message.status() << messaget::eom;
    }
    else if (res == decision_proceduret::resultt::D_UNSATISFIABLE)
    {
      message.status() << "Problem is UNSATISFIABLE\n";
      message.status() << messaget::eom;
    }
    else
    {
      message.status() << "SMT result error/unknown/timeout\n";
      message.status() << messaget::eom;
    }

    message.status() << messaget::eom; // flush
  }
  else
  {
    message.status() <<"No command line option given for SMT files  \n"<<messaget::eom;
  }

  return 1;
}
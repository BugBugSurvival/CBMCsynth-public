#include "synth_td.h"
#include "../utils/expr2sygus.h"
#include <util/arith_tools.h>
#include <iostream>
#include <util/mathematical_expr.h>

std::mt19937 rng2(rand());

top_down_syntht::enum_resultt top_down_syntht::replace_nts(exprt &expr, std::size_t &current_depth, std::vector<unsigned> &sequence)
{
  std::cout<<"depth "<<current_depth<<" and prog " << expr2sygus(expr) << "  and prog limit "<< program_size <<std::endl;
  enum_resultt result = enum_resultt::NO_CHANGE;
  if (expr.id() == ID_symbol)
  {
    auto &symbol = to_symbol_expr(expr);
    if (grammar.production_rules.find(symbol.get_identifier()) != grammar.production_rules.end())
    {
      auto &rules = grammar.production_rules.at(symbol.get_identifier());
      if (rules.size() > 0)
      {
        if (current_depth < program_size)
        {
          // randomly pick nonterminal
          while (result==enum_resultt::NO_CHANGE)
          {
            std::size_t random_index = distributions[symbol.get_identifier()](rng2);
            sequence.push_back(random_index);
            if (prev_solutions.find(sequence) == prev_solutions.end())
            {
              expr = rules[random_index];
              result = enum_resultt::CHANGED;
            }
            else
            {
              sequence.pop_back();
            }
          }
        }
        else
        {
          std::cout<<"replacing with terminal"<<std::endl;
          // replace with a terminal
          for (unsigned i = 0; i < rules.size(); i++)
          {
            if (!contains_nonterminal(rules[i], grammar))
            {
              sequence.push_back(i);
              if (prev_solutions.find(sequence) == prev_solutions.end())
              {
                expr = rules[i];
                result = enum_resultt::CHANGED;
                break;
              }
              else
              {
                // we've seen this before
                sequence.pop_back();
              }
            }
          }
          if(result == enum_resultt::NO_CHANGE)
          {
            // should have replaced this nonterminal
            return enum_resultt::ABORT;
          }
        }
      }
      else
      {
        UNEXPECTEDCASE("No production rules for nonterminal " + id2string(symbol.get_identifier()));
      }
    }
  }
  else
  {
    current_depth++;
    for(auto &op: expr.operands())
    {
      switch(replace_nts(op, current_depth, sequence))
      {
        case enum_resultt::ABORT:
          return enum_resultt::ABORT;
        case enum_resultt::CHANGED:
          result = enum_resultt::CHANGED;
          break;
        case enum_resultt::NO_CHANGE:
          break;
      }
    }
  }
  return result;
}


void top_down_syntht::top_down_enumerate()
{
  exprt current_program = symbol_exprt(grammar.start, grammar.start_type);

  // enumerate through the grammar until a complete program is found
  std::vector<unsigned> sequence;
  bool no_solution=true;
  std::size_t count=0;
  while (no_solution)
  {
    std::cout<<"iter of replaceNT "<<count++<<std::endl;
    std::size_t current_depth = 0;
    message.debug()<<"partial program: "<<expr2sygus(current_program)<<messaget::eom;

    // if we didn't replace anything, we had a complete program. Exit the loop
    switch(replace_nts(current_program, current_depth, sequence))
    {
      case enum_resultt::ABORT:
        current_program = symbol_exprt(grammar.start, grammar.start_type);
        sequence.clear();
        break;
      case enum_resultt::CHANGED:
        break;
      case enum_resultt::NO_CHANGE:
        prev_solutions.insert(sequence);
        no_solution=false;
        break;
    }
  }
  std::cout << "Complete prog: " << expr2sygus(current_program) << std::endl;

  last_solution.functions[symbol_exprt(problem.synthesis_functions[0].id, 
  problem.synthesis_functions[0].type)] = 
  lambda_exprt(problem.synthesis_functions[0].parameters, current_program);
}

solutiont top_down_syntht::get_solution() const
{
  return last_solution;
}

void top_down_syntht::set_program_size(std::size_t size)
{
  program_size = size;
}

void top_down_syntht::add_counterexample(const counterexamplet &cex)
{
  counterexamples.push_back(cex);
}

void top_down_syntht::create_distributions()
{
  if(grammar.production_rule_weights.size()==0)
  {
    std::cout<<"adding default weights"<<std::endl;
    add_grammar_weights(grammar);
  }
  for (auto &nt : grammar.production_rule_weights)
  {
    distributions[nt.first] = std::discrete_distribution<int>(nt.second.begin(), nt.second.end());
    std::cout<<"created distribution for "<<nt.first<<std::endl;
  }
}

top_down_syntht::resultt top_down_syntht::operator()(std::size_t iteration)
{
  std::cout<<"starting enumeration"<<std::endl;
  while(true)
  {
    std::cout<<"iter "<<iteration<<std::endl;
    // maybe put this in a timeout
    top_down_enumerate();
    // check against counterexamples
    if(counterexamples.size()==0)
    {
      std::cout<<"No counterexamples, returning candidate"<<std::endl;
      return CANDIDATE;
    }
    else if(cex_verifier(problem, last_solution, counterexamples)==counterexample_verifyt::resultt::PASS)
    {
      std::cout<<"counterexample verifier passed "<<std::endl;
      return CANDIDATE;
    }
    else
    {
      std::cout<<"counterexample verifier failed "<<std::endl;
    }
    iter++;
  }
  return NO_SOLUTION;
}
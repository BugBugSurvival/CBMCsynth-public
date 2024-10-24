#ifndef PARSER_CPP
#define PARSER_CPP

#include <solvers/smt2/smt2_parser.h>
#include "../sygus_problem.h"
#include "../smt_problem.h"

#include <util/mathematical_expr.h>
#include <util/mathematical_types.h>

class parsert : public smt2_parsert {
public:
    explicit parsert(std::istream &_in) : smt2_parsert(_in) {
        setup_commands();
        setup_expressions();
    }

    exprt::operandst assertions;
    std::string logic;
    exprt::operandst constraints;
    exprt::operandst assumptions;

    // store synth funcs
    std::map<irep_idt, synth_funt> synthesis_functions;
    // might contain information from where the problem was originally
    std::vector<std::string> set_info_cmds;

    void print_problem();
    void parse_model();
    exprt parse_expression();

    using smt2_errort = smt2_tokenizert::smt2_errort;

    void expand_function_applications(exprt &expr);

    smt_problemt get_smt_problem();
    sygus_problemt get_sygus_problem();
    void add_defined_functions(const std::map<symbol_exprt, exprt> &defined_functions);
    void add_symbols(const std::vector<symbol_exprt> &symbols);

protected:
    enum invariant_variablet { PRIMED, UNPRIMED };

    void setup_commands();
    void setup_expressions();
    void add_synth_fun_id(irep_idt id, 
    const smt2_parsert::signature_with_parameter_idst &sig, const syntactic_templatet& grammar);
    smt2_parsert::signature_with_parameter_idst inv_function_signature();
    void build_smt_problem();
    void build_sygus_problem();
    syntactic_templatet parse_grammar();
    symbol_exprt NTDef();
    std::vector<exprt> GTerm_seq(const symbol_exprt &nonterminal);
    smt_problemt smt_problem;
    sygus_problemt sygus_problem;
    lambda_exprt get_lambda(const irep_idt &id);
    void generate_inv_constraint();

    
};

#endif
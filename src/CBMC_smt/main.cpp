/*******************************************************************\
 Module: CBMC SMT stuff
 Author: Elizabeth Polgreen, epolgreen@gmail.com. 
\*******************************************************************/

#include <util/cmdline.h>
#include <util/suffix.h>

#include "smt2_frontend.h"

#include "uuid_v4.h"
UUIDv4::UUIDGenerator<std::mt19937_64> uuidGenerator;
UUIDv4::UUID uuid = uuidGenerator.getUUID();
std::string bytes = uuid.bytes();

#include <iostream>

#define OPTIONS                                                    \
  "(verbosity): "                                                   \




/// File ending of SMT2 files. Used to determine the language frontend that
/// shall be used.
#define SMT2_FILE_ENDING ".smt2"

/// File ending of Sygus files. Used to determine the language frontend that
/// shall be used. 
#define SYGUS_FILE_ENDING ".sl"

void help(std::ostream &out)
{
  out <<
     "\n"
     "* *                          CBMC SMT stuff                    * *\n";

  out  <<
     "* *                                                            * *\n"
     "\n"
     "Usage:                           Purpose:\n"
     "\n"
     " CBMC_smt [-?] [-h] [--help]       show help\n"
     " CBMC_smt file.smt2 ...              source file names\n"
     "\n"
     "\n"
     "Command line options\n"
     " --verbosity N                   increase verbosity (10 gives maximum verbosity)\n\n"
     "\n"
     "\n";
}

int main(int argc, const char *argv[])
{
  cmdlinet cmdline;

  if(cmdline.parse(argc, argv, OPTIONS))
  {
    std::cerr << "Usage error\n";
    help(std::cerr);
    return 1;
  }

  
  if(cmdline.isset("help") || cmdline.isset("h") || cmdline.isset("?") || cmdline.args.empty())
  {
    help(std::cout);
    return 1;
  }

  try
  {
    if(has_suffix(cmdline.args.back(), SMT2_FILE_ENDING))
    {
      return smt2_frontend(cmdline);
    }
    else
    {
      std::cerr<<"Unknown file type \n";
    } 

  }
  catch(const char *s)
  {
    std::cerr << "Error: " << s << '\n';
  }
  catch(const std::string &s)
  {
    std::cerr << "Error: " << s << '\n';
  }
}

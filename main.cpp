// $Id: cppstrtok.cpp,v i0.7 2016-09-30 11:45:04-07 - - $

// Use cpp to scan a file and print line numbers.
// Print out each input line read in, then strtok it for
// tokens.

#include <string>
using namespace std;

#include <unistd.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wait.h>

#include "string_set.h"
#include "auxlib.h"
#include "lyutils.h"
#include "astree.h"

astree *yyparse_astree;


//#include "yylex.cpp"

const string CPP = "/usr/bin/cpp -nostdinc";
constexpr size_t LINESIZE = 1024;
string cpp_command;
string dstring;

FILE* tokfile;

void cpp_popen (const char* filename) {
   cpp_command = CPP + dstring + " " + filename;
   yyin = popen (cpp_command.c_str(), "r");
   if (yyin == NULL) {
      syserrprintf (cpp_command.c_str());
      exit (exec::exit_status);
   }else {
      if (yy_flex_debug) {
         fprintf (stderr, "-- popen (%s), fileno(yyin) = %d\n",
                  cpp_command.c_str(), fileno (yyin));
      }
      lexer::newfilename (cpp_command);
   }
}

void cpp_pclose() {
   int pclose_rc = pclose (yyin);
   eprint_status (cpp_command.c_str(), pclose_rc);
   if (pclose_rc != 0) exec::exit_status = EXIT_FAILURE;
}

void scan_opts (int argc, char** argv) {
  int option;
   
  yy_flex_debug = 0;
  yydebug = 0;

  while((option = getopt(argc, argv, "lyD:@:"))!=-1){
   switch(option){
      case 'l':
        yy_flex_debug = 1;
        break;
      case 'y':
        yydebug = 1;
        break;
      case 'D':
        dstring = " -D" + string(optarg);
        break;
      case '@':
        set_debugflags(optarg);
        break;
      default:
        errprintf("%: Invalid option used \n", optopt);
        break;
    }
  }
}

int main (int argc, char** argv) {

   exec::execname = basename (argv[0]);
   exec::exit_status = EXIT_SUCCESS;

   scan_opts(argc, argv);

    if(optind >= argc){
        eprintf("oc: fatal error: no input filename\n");
        exit(EXIT_FAILURE);
    }

    string filename = basename(argv[optind]);

    if(filename.size() <= 3 || 
        filename.substr(filename.size()-3, 3) != ".oc"){
        eprintf("oc: fatal error: invalid filename\n");
        exit(EXIT_FAILURE);
    }

    string basefilename = filename.substr(0, filename.size()-3);

    string strfilename = basefilename + ".str";
    FILE *strfile = fopen(strfilename.c_str(), "w");
    if(strfile == NULL){
      syserrprintf(strfilename.c_str());
      exit(exec::exit_status);
    }

    string tokfilename = basefilename + ".tok";
    tokfile = fopen(tokfilename.c_str(), "w");
    if(tokfile == NULL){
      syserrprintf(tokfilename.c_str());
      exit(exec::exit_status);
    }

    string astfilename = basefilename + ".ast";
    FILE *astfile = fopen(astfilename.c_str(), "w");
    if(astfile == NULL){
      syserrprintf(astfilename.c_str());
      exit(exec::exit_status);
    }

    string symfilename = basefilename + ".sym";
    FILE *symfile = fopen(symfilename.c_str(), "w");
    if(symfile == NULL){
      syserrprintf(symfilename.c_str());
      exit(exec::exit_status);
    }

    cpp_popen(argv[optind]);

    int parse_val = yyparse();

    if(parse_val != 0){
      errprintf("parse value was invalid (%d)\n", parse_val);
      return exec::exit_status;
    } else {
      check_tree(symfile, yyparse_astree);

      //check_tree(stdout, yyparse_astree);
    }

    astree::print(astfile, yyparse_astree);

    cpp_pclose();

    string_set::dump(strfile);          

    fclose(astfile);
    fclose(strfile);
    fclose(tokfile);
    fclose(symfile);

    return exec::exit_status;
}

// $Id: astree.h,v 1.7 2016-10-06 16:13:39-07 - - $

#ifndef __ASTREE_H__
#define __ASTREE_H__

#include <string>
#include <vector>
using namespace std;

#include "auxlib.h"
#include "symtable.h"

struct location {
   size_t filenr;
   size_t linenr;
   size_t offset;
};

struct astree {

   // Fields.
   int symbol;               // token code
   location lloc;            // source location
   const string* lexinfo;    // pointer to lexical information symbel_name
   vector<astree*> children; // children of this n-way node
   //
   attr_bitset attributes;
   size_t block_num;
   //declloc is location of the declartion within the tree but regarding the 
   //symbol table. location was assigned in make_symbol
   //takes the declloc where things where declared in where the tree.
   //lloc where varable declared in the ast tree.
   location declloc;          //node location 4 storeing/dec
   const string *strukt_name;
   const string *field_name;


   // Functions.
   astree (int symbol, const location&, const char* lexinfo);
   ~astree();
   astree* adopt (astree* child1, astree* child2 = nullptr, 
      astree* child3 = nullptr );
   astree* adopt_sym (astree* child, int symbol);
   void dump_node (FILE*);
   void dump_tree (FILE*, int depth = 0);
   static void dump (FILE* outfile, astree* tree);
   static void print (FILE* outfile, astree* tree, int depth = 0);
};

void destroy (astree* tree1, astree* tree2 = nullptr, 
      astree* tree3 = nullptr);

void errllocprintf (const location&, const char* format, const char*);

#endif

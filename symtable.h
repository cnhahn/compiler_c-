#ifndef __SYMTABLE_H__
#define __SYMTABLE_H__

#include <bitset>
#include <vector>
#include <unordered_map>
#include <string>

using namespace std;

struct astree;
struct symbol;


using sym_table = unordered_map<const string*, symbol*>;
extern vector<sym_table *> symbol_stack;

enum{
  //ATTR_bool, ATTR_char,
  ATTR_void, ATTR_int, ATTR_null, ATTR_string, ATTR_proto,
  ATTR_struct, ATTR_array, ATTR_function, ATTR_variable, ATTR_field,
  ATTR_typeid, ATTR_param, ATTR_lval, ATTR_const, ATTR_vreg, ATTR_vaddr,
  ATTR_bitset_size,
};

using attr_bitset = bitset<ATTR_bitset_size>;

struct symbol{
  symbol();
  ~symbol();

  const string *symbel_name;   //the symbols name - enum value
  const string *strukt_name;   //the struct name 
  const string *field_name;  
  attr_bitset attributes;      //
  sym_table *fields;           //
                               //0 to large not -# to large
  size_t file_nr, line_nr, offset, block_nr;
  vector<symbol*> *parameters; //moveing where instance is
};

extern sym_table gobal_tab;
extern sym_table struct_tab;
extern bool fail;

string attr_str(astree*); //if attr add .append with strings
        //name->c_str()
string str_from_attr(auto node);
void check_tree(FILE* outfile, astree* root);

#endif
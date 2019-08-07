#include <bitset>
#include <vector>
#include <unordered_map>
#include <string>

using namespace std;

#include "symtable.h"
#include "astree.h"
#include "lyutils.h"

vector<sym_table *> symbol_stack;

FILE* sym_file;

astree *funkt;

symbol *temp_gobal_symbel = nullptr;

sym_table gobal_tab;// hash
sym_table struct_tab;
bool fail;

int block_num; //only increases based on the amount of things you went into scopes
int block_depth; //only decreases based on the amount of the things you went into scopes

void check_childs(astree *node);
void checknode(astree *node);

symbol::symbol(){
   symbel_name = nullptr;
   strukt_name = nullptr;
   field_name = nullptr; 
   attributes = attr_bitset(0);
   fields = nullptr;
   parameters = nullptr;
}

symbol::~symbol(){
   if(symbel_name != nullptr){
      delete symbel_name;
   }
   if(field_name != nullptr){
      delete field_name;
   }
   if(strukt_name != nullptr){
      delete strukt_name;
   }
   if(parameters != nullptr){
      delete parameters;
   }
   if(fields != nullptr){
      delete fields;
   }
}

//print functions
string str_from_attr(auto node){
   string string_attr = "";
   const string *name = node->strukt_name;
   const string *field = node->field_name;

   if(node->attributes[ATTR_void]){
      string_attr.append("void ");
   }
   if(node->attributes[ATTR_int]){
      string_attr.append("int ");
   }
   if(node->attributes[ATTR_null]){
      string_attr.append("null ");
   }
   if(node->attributes[ATTR_string]){
      string_attr.append("string ");
   }
   if(node->attributes[ATTR_struct])
   {
      string_attr.append("struct ");
      string_attr.append("\"");
      string_attr.append(name->c_str());
      string_attr.append("\" ");  
   }
   
   if(node->attributes[ATTR_array]){
      string_attr.append("array ");
   }
   if(node->attributes[ATTR_function]){
      string_attr.append("function ");
   }
   if(node->attributes[ATTR_proto]){
      string_attr.append("prototype ");
   }
   if(node->attributes[ATTR_variable]){
      string_attr.append("variable ");
   }
   if(node->attributes[ATTR_field])
   {
      string_attr.append("field ");
      string_attr.append("{");
      string_attr.append(field->c_str());
      string_attr.append("} ");  
   }

   if(node->attributes[ATTR_typeid]){
      string_attr.append("typeid ");
   }
   if(node->attributes[ATTR_param]){
      string_attr.append("parameters ");
   }
   if(node->attributes[ATTR_lval]){
      string_attr.append("lval ");
   }
   if(node->attributes[ATTR_const]){
      string_attr.append("const ");
   }
   if(node->attributes[ATTR_vreg]){
      string_attr.append("vreg ");
   }
   if(node->attributes[ATTR_vaddr]){
      string_attr.append("vaddr ");
   }
   return string_attr;
}

//print functions end

//use this function to get the curent block number.
int get_blocknum(){
   int temp_blocknum = 0;
   if(block_depth != 0){
      temp_blocknum = block_num;
   }
   return temp_blocknum;
}

void set_dekl_loc(astree *node, symbol *symbel){
   node->declloc.filenr = symbel->file_nr;
   node->declloc.linenr = symbel->line_nr;
   node->declloc.offset = symbel->offset;
}

symbol *make_symbol(astree *node){
   symbol *symbel = new symbol();
   symbel->file_nr = node->lloc.filenr;
   symbel->line_nr = node->lloc.linenr;
   symbel->offset = node->lloc.offset;
   symbel->block_nr = get_blocknum();
   symbel->attributes = node->attributes;
   symbel->symbel_name = node->lexinfo;
   symbel->strukt_name = node->strukt_name;
   return symbel;
}

string get_string_type(auto node){
   attr_bitset attr = node->attributes;
   string message = "";
   if(attr[ATTR_void] == 1){
      message.append("void");
   }
   if(attr[ATTR_int] == 1){
      message.append("int");
   }
   if(attr[ATTR_string] == 1){
      message.append("string");
   }
   if(attr[ATTR_struct] == 1){
      message.append(node->strukt_name->c_str());
   } 
   if(attr[ATTR_null] == 1){
      message.append("null");
   } 
   if(attr[ATTR_array] == 1){
      message.append("[]");
   } 
   return message; 
}

//used for  + = things like this that done have explictied attr
//make a funtion to set the attr from the node attr type
//requires the attr form the node to be passed in. (node)
int get_attr_type(auto node){
   attr_bitset attr = node->attributes;
   if(attr[ATTR_void] == 1){
      //attr_void = 0;
      return ATTR_void;
   }
   if(attr[ATTR_int] == 1){
      return ATTR_int;
   }
   if(attr[ATTR_string] == 1){
      return ATTR_string;
   }
   if(attr[ATTR_struct] == 1){
      return ATTR_struct;
   } 
   return -1; 
}


//will determine what attr the thing is based off its token
//requires the child of node to be passed in.(node->child[0])
int get_base_attr(astree *child){
   int base_attr = child->symbol; 
   if(base_attr == TOK_VOID){
      return ATTR_void;
   }
   if(base_attr == TOK_INT){
      return ATTR_int;
   }
   if(base_attr == TOK_STRING){
      return ATTR_string;
   }
   if(base_attr == TOK_TYPEID){
      return ATTR_struct;
   } 
   return -1; 
}

bool get_ref_type(astree *node){
   bool result = false;
   if(node->attributes[ATTR_array] or
      node->attributes[ATTR_string] or
      node->attributes[ATTR_struct]){
      result = true;
   }
   return result;
}

//if and whiles are ints nulls or reference
//non null references are true and null is false

symbol *make_ident(astree *name){

   int type = get_base_attr(name->children[0]); 
   name->attributes[type] = 1;

   if(name->children.size() == 2){
      name->children[1]->block_num = get_blocknum();
      name->attributes[ATTR_array] = 1;
   }

   if(name->attributes[ATTR_struct] == 1){
      name->attributes[ATTR_typeid] = 1;
      name->strukt_name = name->children[0]->lexinfo;
   }

   name->block_num = get_blocknum();

   return make_symbol(name);
}

symbol *make_vardecl(astree *node){
   node->attributes[ATTR_lval] = 1;
   node->attributes[ATTR_variable] = 1;
   return make_ident(node);
}

symbol *make_param(astree *node){
   node->attributes[ATTR_param] = 1;
   return make_vardecl(node);
}

symbol *make_proto(astree *node){
   node->attributes[ATTR_proto] = 1;
   symbol *symbel = make_ident(node);
   return symbel;
}

symbol *make_function(astree *node){
   node->attributes[ATTR_proto] = 0;
   node->attributes[ATTR_function] = 1;
   symbol *symbel = make_ident(node);
   return symbel;
}

symbol *make_field(astree *node){
   node->attributes[ATTR_field] = 1;
   symbol *symbel = make_ident(node);
   symbel->field_name = node->field_name;
   return symbel;
}


symbol *make_struct(astree *node){
   astree *name = node->children[0];
   astree *field= node->children[1];
   node->attributes[ATTR_struct] = 1;
   return make_symbol(name);
}


void create_sym_new(astree *name){
   int type = get_base_attr(name->children[0]); 
   name->attributes[type] = 1;
}

bool compare_sym(symbol *lhs, symbol *rhs){
   bool result = false;
   bool type_same 
      = get_attr_type(lhs) == get_attr_type(rhs);
   bool type_array_same
      = rhs->attributes[ATTR_array] == lhs->attributes[ATTR_array];
   if( type_same and get_attr_type(lhs) != -1 and
         type_array_same){
      result = true;
   }
   if( lhs->attributes[ATTR_struct] and
         (lhs->strukt_name != rhs->strukt_name)){
      result = false;
   }
   return result;
}

bool compare(astree *lhs, astree *rhs){
   bool result = false;
   bool type_same 
      = get_attr_type(lhs) == get_attr_type(rhs);
   bool type_array_same
      = rhs->attributes[ATTR_array] == lhs->attributes[ATTR_array];
   if( type_same and get_attr_type(lhs) != -1 and
         type_array_same){
      result = true;
   }
   if( lhs->attributes[ATTR_struct] and
         (lhs->strukt_name != rhs->strukt_name)){
      result = false;
   }
   if( (lhs->attributes[ATTR_null] == 1 and get_ref_type(rhs)) or
         (rhs->attributes[ATTR_null] == 1 and get_ref_type(lhs)) ){
      result = true;
   }
   return result;
}

//key = name of symbol
bool insert_symtable(const string *key, symbol *symbel){
   sym_table *curent_table = symbol_stack.back();
   if(curent_table == NULL){
      //symbol_stack[symbol_stack.size() - 1] = new sym_table();
      symbol_stack.back() = new sym_table();
   }
   auto inserted = symbol_stack.back()->emplace(key, symbel);
   //second = true if inserted, false if something was in the table.
   return inserted.second;
}

void print_symbol(symbol *symbel){
   fprintf(sym_file, "%*s", block_depth * 3, "");
   fprintf(sym_file, "%s (%zu.%zu.%zu) { %zu } ", 
      symbel->symbel_name->c_str(), symbel->file_nr,
      symbel->line_nr, symbel->offset, 
      symbel->block_nr);
   fprintf(sym_file, "%s\n", str_from_attr(symbel).c_str());
}

void print_symbol(astree *print){
   fprintf(sym_file, "%*s", block_depth * 3, "");
   fprintf(sym_file, "%s (%zu.%zu.%zu) { %zu } ", 
      print->lexinfo->c_str(), print->lloc.filenr,
      print->lloc.linenr, print->lloc.offset, 
      print->block_num);
   fprintf(sym_file, "%s\n", str_from_attr(print).c_str());
}

void errorprint(size_t file_nr,size_t line_nr,size_t offset){
   eprintf("%s:%zu:%zu: ERROR: ", lexer::filename(file_nr)->c_str(),
    line_nr, offset);       
}

symbol *search_symbol(const string *key){
   symbol *symbel = nullptr;
   for(int index = symbol_stack.size() -1; index >=0; index--){
      if(symbol_stack[index] != nullptr){
         auto found = symbol_stack[index]->find(key);
         if(found != symbol_stack[index]->end()){
            symbel = found->second;
            break;
         }
      }
   }
   return symbel;
}

//takes in a leaf node.
void variable_refence(astree *child){
   symbol *teliscope = search_symbol(child->lexinfo);
   child->block_num = get_blocknum();
   if(teliscope != nullptr and 
      teliscope->attributes[ATTR_function] == 0){
      child->attributes = teliscope->attributes;
      child->strukt_name = teliscope->strukt_name;
      set_dekl_loc(child, teliscope);
   }else if(teliscope != nullptr and 
      teliscope->attributes[ATTR_function] == 1){
      errorprint(teliscope->file_nr, teliscope->line_nr,
       teliscope->offset);
      eprintf("can not use function \"%s\" as a variable.\n",
         child->lexinfo->c_str());
   }else if(teliscope == nullptr){
      errorprint(child->lloc.filenr, child->lloc.linenr,
       child->lloc.offset);
      eprintf("made a reference to an undefined variable \"%s\".\n",
         child->lexinfo->c_str());
   }
}

//make new variable we use the make
void create_sym_vardecl(astree *target){
   astree *rhs = target->children[1];
   astree *lhs = target->children[0];

   symbol *verdekl = make_vardecl(lhs);
   //check strukt_name is defined
   //if(target->strukt_name)
   //set block number of vardecl
   target->block_num = get_blocknum();
   checknode(rhs);
   bool successfully_added;
   if(compare(lhs, rhs) && (verdekl->attributes[ATTR_void] == 0)){
      successfully_added = insert_symtable(lhs->lexinfo, verdekl);
      if(successfully_added){
         print_symbol(lhs);
      }else{
         errorprint(verdekl->file_nr, verdekl->line_nr, 
            verdekl->offset);
         eprintf("variable \"%s\" is already defined.\n",
         lhs->lexinfo->c_str());
      }
   }else if(verdekl->attributes[ATTR_void] == 1){
      errorprint(verdekl->file_nr, verdekl->line_nr,
       verdekl->offset);
      eprintf("variable \"%s\" can not be defined as type void.\n",
      lhs->lexinfo->c_str());
   }else if(!compare(lhs, rhs)){
      errorprint(verdekl->file_nr, verdekl->line_nr, verdekl->offset);
      eprintf("operands of type \"%s\" and \"%s\" "
         "are not compatible.\n", 
         get_string_type(lhs).c_str(), get_string_type(rhs).c_str());
   }
}

//takes the root node and called visit on children
void create_sym_intexpr(astree *root){
   astree *rhs = root->children[1];
   astree *lhs = root->children[0];
   root->block_num = get_blocknum();
   checknode(lhs);
   checknode(rhs);
   bool compatable = rhs->attributes[ATTR_int]   == 1 and 
                     rhs->attributes[ATTR_array] == 0 and
                     lhs->attributes[ATTR_array] == 0 and
                     lhs->attributes[ATTR_int]   == 1;
   if(compatable){
      root->attributes[ATTR_int]  = 1;
      root->attributes[ATTR_vreg] = 1;
   }else{
      errorprint(root->lloc.filenr, root->lloc.linenr,
       root->lloc.offset);
      eprintf("operands of type \"%s\" and \"%s\" "
         "are not compatible.\n", 
         get_string_type(lhs).c_str(), get_string_type(rhs).c_str());
   }
}

//-------------------------------------------------------------
//if while statmts check for type 
//can take ints and null and references and arrays of anything and strings  and structs
bool get_check_type(astree *node){
   bool result = node->attributes[ATTR_int] or
                  node->attributes[ATTR_null] or
                     get_ref_type(node);
   return result;
}


//


void push_stack(){
   block_depth++;
   symbol_stack.push_back(nullptr);
   block_num++;
}

void pop_stack(){
   block_depth--;
   symbol_stack.pop_back();
}

//-------------------------------------------------------------
//-------------------------------------------------------------
//needs to be checked
void create_sym_comparison_opperator(astree *root){
   astree *rhs = root->children[1];
   astree *lhs = root->children[0];
   root->block_num = get_blocknum();
   checknode(lhs);
   checknode(rhs);
   if(compare(lhs, rhs)){
      root->attributes[ATTR_int] = 1;
      root->attributes[ATTR_vreg] = 1;
   }else{
      errorprint(root->lloc.filenr, root->lloc.linenr,
       root->lloc.offset);
      eprintf("operands of type \"%s\" and \"%s\" "
         "are not compatible.\n", 
         get_string_type(lhs).c_str(), get_string_type(rhs).c_str());
   }
}

//needs to be checked
void create_sym_equals_opperator(astree *root){
   astree *rhs = root->children[1];
   astree *lhs = root->children[0];
   root->block_num = get_blocknum();
   checknode(lhs);
   checknode(rhs);
   bool check = lhs->attributes[ATTR_lval] == 1;
   if(compare(lhs, rhs) && check){
      root->attributes[get_attr_type(lhs)] = 1;
      root->strukt_name = lhs->strukt_name;
      root->attributes[ATTR_array] = lhs->attributes[ATTR_array];
      root->attributes[ATTR_vreg] = 1;
   }else{
      errorprint(root->lloc.filenr, root->lloc.linenr,
       root->lloc.offset);
      eprintf("operands of type \"%s\" and \"%s\" "
         "are not compatible.\n", 
         get_string_type(lhs).c_str(), get_string_type(rhs).c_str());
   }
}

//needs to be checked
void create_sym_post_opperator(astree *root){
   astree *lhs = root->children[0];
   root->block_num = get_blocknum();
   checknode(lhs);
   if(lhs->attributes[ATTR_int] and 
      lhs->attributes[ATTR_array] == 0){
      root->attributes[ATTR_int] = 1;
      root->attributes[ATTR_vreg] = 1;
   }else{
      errorprint(root->lloc.filenr, root->lloc.linenr,
       root->lloc.offset);
      eprintf("operand of type \"%s\" "
         "is not compatible.\n", 
         get_string_type(lhs).c_str());
   }
}

//needs check
void create_sym_period_opperator(astree *root){
   astree *rhs = root->children[1];
   astree *lhs = root->children[0];
   root->block_num = get_blocknum();
   checknode(lhs);
   checknode(rhs);
   bool compared = compare(lhs, rhs);
   if(compared){
      root->attributes[ATTR_int] = 1;
      root->attributes[ATTR_lval] = 1;
      root->attributes[ATTR_vaddr] = 1;
   }else{
      errorprint(root->lloc.filenr, root->lloc.linenr,
       root->lloc.offset);
      eprintf("operands of type \"%s\" and \"%s\" "
         "are not compatible.\n", 
         get_string_type(lhs).c_str(), get_string_type(rhs).c_str());
   }
}

bool compatible_param(vector<symbol*> *current,
                         vector<symbol*> *target){
   bool result = true;
   if(current->size() != target->size()){
      
      //not number of elements taken are not the same.
      errorprint(temp_gobal_symbel->file_nr,
      temp_gobal_symbel->line_nr, 
      temp_gobal_symbel->offset); 
      eprintf("number of elements taken do not match.\n");
      result = false;
   }else{
      for(size_t index = 0; index < current->size(); index++ ){
         if(!compare_sym(current->at(index), target->at(index))){
            result = false;
            symbol *location = current->at(index);
            //complain about each param not compatible.
            errorprint(location->file_nr,
            location->line_nr, 
            location->offset); 
            eprintf("parameter miss-match.\n ");
         }
      }
   }
   return result;
}

bool create_sym_protype(astree *root){
   bool result = true;
   //para has 0 or more childeren
   astree *params = root->children[1];
   //hase child of type and array
   astree *name = root->children[0];
   //this is the symbel table reference with new data in it
   symbol *symbel = make_proto(name);
   //temp vector to storeing the symbel tabs
   symbel->parameters = new vector<symbol *>;
   for(auto param : params->children){

      symbel->parameters->push_back(make_param(param));
   }
   symbol *find = search_symbol(name->lexinfo);
   if(find){
      temp_gobal_symbel = symbel;
      result = compatible_param(symbel->parameters, find->parameters);
      //printf("previously found result is: %d\n", result);
   }else{
      if(symbel->attributes[ATTR_void] and 
         symbel->attributes[ATTR_array]){
         errorprint(root->lloc.filenr, root->lloc.linenr,
          root->lloc.offset);
         eprintf("can not have return type as void []. \n");
         result = false;
      }else{
         
         insert_symtable(name->lexinfo, symbel);
         print_symbol(symbel);
         push_stack();
         for(auto param : *symbel->parameters){
            param->block_nr = get_blocknum();
            print_symbol(param);
         }
         pop_stack();
      }
   }
   return result;
}

void create_sym_function(astree *root){
   astree *name = root->children[0];
   //astree *param = root->children[1];
   astree *block = root->children[2];
   bool result = create_sym_protype(root);
   if(result){
      //printf("here");
      //new symbol tab
      symbol *symbel = make_function(name);
      // pints to old symble tab
      symbol *old_sym = search_symbol(name->lexinfo);
      symbel->parameters = old_sym->parameters;
      //breaks pointer of old_sym
      gobal_tab.erase(name->lexinfo);
      insert_symtable(name->lexinfo, symbel);
      print_symbol(symbel);
      push_stack();
      for(auto param : *symbel->parameters){
         param->block_nr = get_blocknum();
         print_symbol(param);

      }
      funkt = name;
      check_childs(block);
      funkt = nullptr;
      pop_stack();
   }
}


void create_sym_return(astree *leaf){
   if(funkt != nullptr){
      leaf->block_num = get_blocknum();
      if(funkt->attributes[ATTR_void] == 0){
         errorprint(leaf->lloc.filenr, leaf->lloc.linenr,
          leaf->lloc.offset);
         eprintf("can not have return in a void function. \n");
      }
      if(compare(funkt, leaf)){
         errorprint(leaf->lloc.filenr, leaf->lloc.linenr,
          leaf->lloc.offset);
         eprintf("the return type and function type are"
            "a miss match. \n");
      }
   }else{
      if(leaf->attributes[ATTR_void] == 0){
         errorprint(leaf->lloc.filenr, leaf->lloc.linenr,
          leaf->lloc.offset);
         eprintf("the return type cant not be a non-void type.\n");         
      }
   }
}
//check funkt if its defined //not prosseing function
//handeling processing a function
// pass in campatible funkt to node->child[0] of return statemt // error return of type function
//  check if tree symbol == tok_return
//  else if ! funkt->attr[attr void] // return 
//else if node->symbol == tok_return //return something when your suppose to return nothing
//processing a golbal return stmt//error cant have that

//-------------------------------------------------------------------------
//needs check if and else are the same
// if -> expr ;if-> stmt (consequent);
//ifelse -> expr ; ifelse-> stmt (consequent); ifelse-> stmt (alternate);
// consequent = true do this ; alternate = false do this
void create_sym_if_stmt(astree *root){
   astree *alternate = nullptr;
   astree *stmt = root->children[1];
   astree *expr = root->children[0];
   int token = root->symbol;
   bool compared = false;
   root->block_num = get_blocknum();
   checknode(expr);
   checknode(stmt);
   if(token == TOK_IFELSE){
      alternate = root->children[2];
      checknode(alternate);
   }
   compared = get_check_type(expr);
   if(!compared){
      errorprint(root->lloc.filenr, root->lloc.linenr,
       root->lloc.offset);
      eprintf("unexpected type was expecting an int expression.\n");
   }
}

void create_sym_while_stmt(astree *root){
   astree *stmt = root->children[1];
   astree *expr = root->children[0];
   bool compared = false;
   root->block_num = get_blocknum();
   checknode(expr);
   checknode(stmt);
   compared = get_check_type(expr);
   if(!compared){
      errorprint(root->lloc.filenr, root->lloc.linenr,
       root->lloc.offset);
      eprintf("unexpected type was expecting an int expression.\n");
   }
}

//use this function for in the other functions
void check_childs(astree *node){
   node->block_num = get_blocknum();
   for(astree *child : node->children){
      checknode(child);
   }
}

void set_attr_const(astree *node){
   int token = node->symbol;
   node->attributes[ATTR_const] = 1;
   node->block_num = get_blocknum();
   if(token == TOK_INTCON || token == TOK_CHARCON){
      node->attributes[ATTR_int] = 1;
   }else if(token == TOK_STRINGCON){
      node->attributes[ATTR_string] = 1;
   }else if(token == TOK_NULL){
      node->attributes[ATTR_null] = 1;
   }
}

void create_sym_array(astree *node){
   node->attributes[ATTR_array] = 1;
   node->children[1]->attributes[ATTR_int] = 1;
}

void create_sym_declid(astree *node){
   if(node->attributes[ATTR_function] == 0){
      node->attributes[ATTR_variable] = 1;
   }
}

void create_sym_typeid(astree *node){
   node->attributes[ATTR_typeid] = 1;
}

void create_sym_struct(astree *root){
   bool result = true;
   astree *fieldz = root->children[1];
   astree *name = root->children[0];

   symbol *symbel = make_struct(name);
   //temp vector to storeing the symbel tabs

   insert_symtable(name->lexinfo, symbel);
   print_symbol(symbel);
   push_stack();
   if(root->children.size() > 1){
      push_stack();
      symbel->fields = new sym_table();
      for(auto field : root->children){
         field->block_num = get_blocknum();
         //symbel->fields.back()->embplace(field->field_name, make_field(field));
      }
      //insert_symtable(symbel->field_name, symbel->fields);
      print_symbol(symbel);
      pop_stack();
   }
   pop_stack();
}


//lval attr set functions end   ---------------------------------

//assume check worked node is ??? 
void checknode(astree* node) {
   switch (node->symbol) {
      case TOK_ROOT:
         check_childs(node);
         break;
      case TOK_IF:
      case TOK_IFELSE:
         create_sym_if_stmt(node);
         break;
      case TOK_WHILE:
         create_sym_while_stmt(node);
         break;
      case TOK_RETURN:
         create_sym_return(node);
         break;
      case TOK_STRUCT:
         create_sym_struct(node); 
         break;
      case TOK_NEW:
         create_sym_new(node);
         break;
      case TOK_ARRAY:
         create_sym_array(node);
         break;
      case TOK_EQ:
      case TOK_NE:
      case TOK_LT:
      case TOK_LE:
      case TOK_GT:
      case TOK_GE:
         create_sym_comparison_opperator(node);
         break;
      case TOK_IDENT:
         variable_refence(node);
         break;
      case TOK_INTCON: 
      case TOK_CHARCON:
      case TOK_STRINGCON:
      case TOK_NULL:
         set_attr_const(node);
         break;
      case TOK_BLOCK:
         push_stack();
         check_childs(node);
         pop_stack();
         break;
      case TOK_CALL:
      //
         break;
      case TOK_NEWARRAY:
      //
         break;
      case TOK_TYPEID:
         create_sym_typeid(node);
         break;
      case TOK_PROTOTYPE:
         create_sym_protype(node);
         break;
      case TOK_FUNCTION:
         create_sym_function(node);
         break;
      case TOK_DECLID:
      case TOK_INDEX:
         create_sym_declid(node);
         break;
      case TOK_NEWSTRING:
         break;
      case TOK_VARDECL:
         create_sym_vardecl(node);
         break;
      case '=':
         create_sym_equals_opperator(node);
         break;
      case '+':
      case '-':  
      case '*': 
      case '/': 
      case '%':
         create_sym_intexpr(node);
      //int exprestion children need to have only have int not array if they do they give int and vreg
      //example is a + 2; root will be +; are both children just integers if not complain supose to have interger childern
         break;
      case TOK_POS:
      case TOK_NEG:
      case '!':
         create_sym_post_opperator(node);
         break;
      case '.':
         create_sym_period_opperator(node);
         break;
      default: 
         node->block_num = get_blocknum();
         break;
   }
}

//while ifelse compare expr int expr and functions and arrays to handel

void check_tree(FILE* outfile, astree* root){
   sym_file = outfile;
   symbol_stack.push_back(&gobal_tab);
   checknode(root);
}

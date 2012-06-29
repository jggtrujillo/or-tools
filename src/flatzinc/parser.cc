#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "base/stl_util.h"
#include "base/stringprintf.h"
#include "flatzinc/flatzinc.h"
#include "flatzinc/parser.h"

using namespace std;
extern int yyparse(void*);
extern int yylex(YYSTYPE*, void* scanner);
extern int yylex_init (void** scanner);
extern int yylex_destroy (void* scanner);
extern int yyget_lineno (void* scanner);
extern void yyset_extra (void* user_defined ,void* yyscanner );
extern void yyerror(void* parm, const char *str);

namespace operations_research {
ParserState::~ParserState() {
  STLDeleteElements(&int_variables_);
  STLDeleteElements(&bool_variables_);
  STLDeleteElements(&set_variables_);
  STLDeleteElements(&constraints_);
  for (int i = 0; i < int_domain_constraints_.size(); ++i) {
    delete int_domain_constraints_[i].second;
  }
  for (int i = 0; i < bool_domain_constraints_.size(); ++i) {
    delete bool_domain_constraints_[i].second;
  }
}

int ParserState::FillBuffer(char* lexBuf, unsigned int lexBufSize) {
  if (pos >= length)
    return 0;
  int num = std::min(length - pos, lexBufSize);
  memcpy(lexBuf, buf + pos, num);
  pos += num;
  return num;
}

void ParserState::output(std::string x, AST::Node* n) {
  output_.push_back(std::pair<std::string,AST::Node*>(x,n));
}

AST::Array* ParserState::Output(void) {
  OutputOrder oo;
  std::sort(output_.begin(),output_.end(),oo);
  AST::Array* const a = new AST::Array();
  for (unsigned int i = 0; i < output_.size(); i++) {
    a->a.push_back(new AST::String(output_[i].first+" = "));
    if (output_[i].second->isArray()) {
      AST::Array* const oa = output_[i].second->getArray();
      for (unsigned int j = 0; j < oa->a.size(); j++) {
        a->a.push_back(oa->a[j]);
        oa->a[j] = NULL;
      }
      delete output_[i].second;
    } else {
      a->a.push_back(output_[i].second);
    }
    a->a.push_back(new AST::String(";\n"));
  }
  return a;
}

int ParserState::FindTarget(AST::Node* const annotations) const {
  if (annotations != NULL) {
    if (annotations->isArray()) {
      AST::Array* const ann_array = annotations->getArray();
      if (ann_array->a[0]->isCall("defines_var")) {
        AST::Call* const call = ann_array->a[0]->getCall();
        AST::Node* const args = call->args;
        return VarIndex(args);
      }
    }
  }
  return CtSpec::kNoDefinition;
}

bool HasDomainAnnotation(AST::Node* const annotations) {
  if (annotations != NULL) {
    return annotations->hasAtom("domain");
  }
  return false;
}

void ParserState::CollectRequired(AST::Array* const args,
                                  const hash_set<int>& candidates,
                                  hash_set<int>* const require) const {
  for (int i = 0; i < args->a.size(); ++i) {
    AST::Node* const node = args->a[i];
    if (node->isArray()) {
      CollectRequired(node->getArray(), candidates, require);
    } else if (node->isIntVar() || node->isBoolVar()) {
      const int req = VarIndex(node);
      if (ContainsKey(candidates, req)) {
        require->insert(req);
      }
    }
  }
}

void ParserState::ComputeViableTarget(
    CtSpec* const spec,
    hash_set<int>* const candidates) const {
  const string& id = spec->Id();
  if (id == "bool2int" ||
      id == "int_plus" ||
      id == "int_minus" ||
      id == "int_times" ||
      (id == "array_var_int_element" && !IsBound(spec->Arg(2))) ||
      id == "array_int_element" ||
      id == "int_abs" ||
      (id == "int_lin_eq" && !HasDomainAnnotation(spec->annotations())) ||
      id == "int_max" ||
      id == "int_min" ||
      id == "int_eq") {
    // Defines an int var.
    const int define = FindTarget(spec->annotations());
    if (define != CtSpec::kNoDefinition) {
      CHECK(int_variables_[define]->introduced);
      candidates->insert(define);
      VLOG(2) << id << " -> insert " << define;
    }
  } else if (id == "array_bool_and" ||
             id == "array_bool_or" ||
             id == "array_bool_element" ||
             id == "int_lin_eq_reif" ||
             id == "int_eq_reif" ||
             id == "int_ne_reif" ||
             id == "bool_eq_reif" ||
             id == "bool_ne_reif") {
    // Defines a bool var.
    const int bool_define = FindTarget(spec->annotations());
    if (bool_define != CtSpec::kNoDefinition) {
      CHECK(bool_variables_[BoolFromVarIndex(bool_define)]->introduced);
      candidates->insert(bool_define);
      VLOG(2) << id << " -> insert " << bool_define;
    }
  } else if (id == "int2int") {
    const int int_define = VarIndex(spec->Arg(1));
    candidates->insert(int_define);
    VLOG(2) << id << " -> insert " << int_define;
  } else if (id == "bool2bool") {
    const int bool_define = VarIndex(spec->Arg(1));
    candidates->insert(bool_define);
    VLOG(2) << id << " -> insert " << bool_define;
  }
}

bool DoDefine(const CtSpec* const spec) {
  return spec->defines() != CtSpec::kNoDefinition;
}

void ParserState::ComputeDependencies(const hash_set<int>& candidates,
                                      CtSpec* const spec) const {
  const int define = spec->defines() == CtSpec::kNoDefinition ?
      FindTarget(spec->annotations()) :
      spec->defines();
  if (ContainsKey(candidates, define)) {
    spec->set_defines(define);
  }
  CollectRequired(spec->Args(), candidates, spec->require_map());
  if (define != CtSpec::kNoDefinition) {
    spec->require_map()->erase(define);
  }
}

void MarkAllVariables(AST::Node* const node, hash_set<int>* const computed) {
  if (node->isIntVar()) {
    computed->insert(node->getIntVar());
    VLOG(1) << "  - " << node->DebugString();
  }
  if (node->isArray()) {
    AST::Array* const array = node->getArray();
    for (int i = 0; i < array->a.size(); ++i) {
      if (array->a[i]->isIntVar()) {
        computed->insert(array->a[i]->getIntVar());
        VLOG(1) << "  - " << array->a[i]->DebugString();
      }
    }
  }
}

void MarkComputedVariables(CtSpec* const spec, hash_set<int>* const computed) {
  const string& id = spec->Id();
  if (id == "global_cardinality") {
    VLOG(1) << "Marking " << spec->DebugString();
    MarkAllVariables(spec->Arg(2), computed);
  }
}

void ParserState::Sanitize(CtSpec* const spec) {
  if (spec->Id() == "int_lin_eq" && HasDomainAnnotation(spec->annotations())) {
    VLOG(1) << "  - presolve: remove defines part on " << spec->DebugString();
    // Remove defines_var part.
    spec->RemoveDefines();
  }
}

void ParserState::CreateModel() {
  hash_set<int> candidates;
  hash_set<int> computed_variables;

  // Sanity.
  for (int i = 0; i < constraints_.size(); ++i) {
    Sanitize(constraints_[i]);
  }
  // Find orphans (is_defined && not a viable target).
  hash_set<int> targets;
  for (int i = 0; i < constraints_.size(); ++i) {
    const int target = FindTarget(constraints_[i]->annotations());
    if (target != CtSpec::kNoDefinition) {
      VLOG(1) << "  - presolve:  mark xi(" << target << ") as defined";
      targets.insert(target);
    }
  }
  for (int i = 0; i < int_variables_.size(); ++i) {
    IntVarSpec* const spec = int_variables_[i];
    if (spec->introduced && !ContainsKey(targets, i)) {
      orphans_.insert(i);
      VLOG(1) << "  - presolve:  mark xi(" << i << ") as orphan";
    }
  }

  // Presolve (propagate bounds).
  bool repeat = true;
  while (repeat) {
    repeat = false;
    for (int i = 0; i < constraints_.size(); ++i) {
      CtSpec* const spec = constraints_[i];
      if (spec->Nullified()) {
        continue;
      }
      if (Presolve(spec)) {
        repeat = true;
      }
    }
  }

  // Add aliasing constraints.
  for (int i = 0; i < int_variables_.size(); ++i) {
    IntVarSpec* const spec = int_variables_[i];
    if (spec->alias) {
      AST::Array* args = new AST::Array(2);
      args->a[0] = new AST::IntVar(spec->i);
      args->a[1] = new AST::IntVar(i);
      CtSpec* const alias_ct = new CtSpec(constraints_.size(),
                                          "int2int",
                                          args,
                                          NULL);
      alias_ct->set_defines(i);
      constraints_.push_back(alias_ct);
    }
  }

  for (int i = 0; i < bool_variables_.size(); ++i) {
    BoolVarSpec* const spec = bool_variables_[i];
    if (spec->alias) {
      AST::Array* args = new AST::Array(2);
      args->a[0] = new AST::BoolVar(spec->i);
      args->a[1] = new AST::BoolVar(i);
      CtSpec* const alias_ct = new CtSpec(constraints_.size(),
                                          "bool2bool",
                                          args,
                                          NULL);
      alias_ct->set_defines(VarIndex(args->a[1]));
      constraints_.push_back(alias_ct);
    }
  }

  // Setup mapping structures (constraints per id, and constraints per
  // variable).
  for (unsigned int i = 0; i < constraints_.size(); i++) {
    CtSpec* const spec = constraints_[i];
    const int index = spec->Index();
    if (!spec->Nullified()) {
      constraints_per_id_[spec->Id()].push_back(index);
      const int num_args = spec->NumArgs();
      for (int i = 0; i < num_args; ++i) {
        AST::Node* const arg = spec->Arg(i);
        if (arg->isIntVar()) {
          constraints_per_int_variables_[arg->getIntVar()].push_back(index);
        } else if (arg->isBoolVar()) {
          constraints_per_bool_variables_[arg->getBoolVar()].push_back(index);
        } else if (arg->isArray()) {
          const std::vector<AST::Node*>& array = arg->getArray()->a;
          for (int j = 0; j < array.size(); ++j) {
            if (array[j]->isIntVar()) {
              constraints_per_int_variables_[array[j]->getIntVar()].push_back(index);        } else if (array[j]->isBoolVar()) {
              constraints_per_bool_variables_[array[j]->getBoolVar()].push_back(index);
            }
          }
        }
      }
    }
  }

  VLOG(1) << "Model statistics";
  for (ConstIter<hash_map<string, std::vector<int> > > it(constraints_per_id_);
        !it.at_end();
       ++it) {
    VLOG(1) << "  - " << it->first << ": " << it->second.size();
  }

  if (ContainsKey(constraints_per_id_, "array_bool_or")) {
    const std::vector<int>& ors = constraints_per_id_["array_bool_or"];
    for (int i = 0; i < ors.size(); ++i) {
      Strongify(ors[i]);
    }
  }

  // Discover expressions, topological sort of constraints.

  for (unsigned int i = 0; i < constraints_.size(); i++) {
    CtSpec* const spec = constraints_[i];
    ComputeViableTarget(spec, &candidates);
  }

  for (unsigned int i = 0; i < constraints_.size(); i++) {
    CtSpec* const spec = constraints_[i];
    ComputeDependencies(candidates, spec);
    if (spec->defines() != CtSpec::kNoDefinition ||
        !spec->require_map()->empty()) {
      VLOG(2) << spec->DebugString();
    }
  }

  VLOG(1) << "Sort constraints";
  std::vector<CtSpec*> defines_only;
  std::vector<CtSpec*> no_defines;
  std::vector<CtSpec*> defines_and_require;
  for (unsigned int i = 0; i < constraints_.size(); i++) {
    CtSpec* const spec = constraints_[i];
    if (DoDefine(spec) && spec->require_map()->empty()) {
      defines_only.push_back(spec);
    } else if (!DoDefine(spec)) {
      no_defines.push_back(spec);
    } else {
      defines_and_require.push_back(spec);
    }
  }

  VLOG(1) << "  - defines only        : " << defines_only.size();
  VLOG(1) << "  - no defines          : " << no_defines.size();
  VLOG(1) << "  - defines and require : " << defines_and_require.size();

  const int size = constraints_.size();
  constraints_.clear();
  constraints_.resize(size);
  int index = 0;
  hash_set<int> defined;
  for (int i = 0; i < defines_only.size(); ++i) {
    constraints_[index++] = defines_only[i];
    defined.insert(defines_only[i]->defines());
    VLOG(2) << "defined.insert(" << defines_only[i]->defines() << ")";
  }

  // Topological sorting.
  hash_set<int> to_insert;
  for (int i = 0; i < defines_and_require.size(); ++i) {
    to_insert.insert(i);
    VLOG(2) << " to_insert " << defines_and_require[i]->DebugString();
  }

  while (!to_insert.empty()) {
    std::vector<int> inserted;
    for (ConstIter<hash_set<int> > it(to_insert); !it.at_end(); ++it) {
      CtSpec* const spec = defines_and_require[*it];
      VLOG(2) << "check " << spec->DebugString();
      bool ok = true;
      hash_set<int>* const required = spec->require_map();
      for (ConstIter<hash_set<int> > def(*required);
           !def.at_end();
           ++def) {
        if (!ContainsKey(defined, *def)) {
          ok = false;
          break;
        }
      }
      if (ok) {
        inserted.push_back(*it);
        defined.insert(spec->defines());
        VLOG(2) << "inserted.push_back " << *it;
        VLOG(2) << "defined.insert(" << spec->defines() << ")";
        constraints_[index++] = spec;
      }
    }
    CHECK(!inserted.empty());
    for (int i = 0; i < inserted.size(); ++i) {
      to_insert.erase(inserted[i]);
    }
  }

  // Push the rest.
  for (int i = 0; i < no_defines.size(); ++i) {
    constraints_[index++] = no_defines[i];
  }

  for (unsigned int i = 0; i < constraints_.size(); i++) {
    CtSpec* const spec = constraints_[i];
    VLOG(2) << i << " -> " << spec->DebugString();
    MarkComputedVariables(spec, &computed_variables);
  }

  VLOG(1) << "Creating variables";

  int array_index = 0;
  for (unsigned int i = 0; i < int_variables_.size(); i++) {
    VLOG(1) << "xi(" << i << ") -> " << int_variables_[i]->DebugString();
    if (!hadError) {
      const std::string& raw_name = int_variables_[i]->Name();
      std::string name;
      if (raw_name[0] == '[') {
        name = StringPrintf("%s[%d]", raw_name.c_str() + 1, ++array_index);
      } else {
        if (array_index == 0) {
          name = raw_name;
        } else {
          name = StringPrintf("%s[%d]", raw_name.c_str(), array_index + 1);
          array_index = 0;
        }
      }
      if (!ContainsKey(candidates, i)) {
        const bool active = (!int_variables_[i]->introduced) &&
            !ContainsKey(computed_variables, i);
        model_->NewIntVar(name, int_variables_[i], active);
      } else {
        model_->SkipIntVar();
        VLOG(1) << "  - skipped";
        if (!int_variables_[i]->alias &&
            !int_variables_[i]->assigned &&
            int_variables_[i]->HasDomain() &&
            int_variables_[i]->Domain() != NULL) {
          AddIntVarDomainConstraint(i, int_variables_[i]->Domain()->Copy());
        }
      }
    }
  }

  array_index = 0;
  for (unsigned int i = 0; i < bool_variables_.size(); i++) {
    VLOG(1) << "xb(" << i << ") -> " << bool_variables_[i]->DebugString();
    if (!hadError) {
      const std::string& raw_name = bool_variables_[i]->Name();
      std::string name;
      if (raw_name[0] == '[') {
        name = StringPrintf("%s[%d]", raw_name.c_str() + 1, ++array_index);
      } else {
        if (array_index == 0) {
          name = raw_name;
        } else {
          name = StringPrintf("%s[%d]", raw_name.c_str(), array_index + 1);
          array_index = 0;
        }
      }
      if (!ContainsKey(candidates, i + int_variables_.size())) {
        model_->NewBoolVar(name, bool_variables_[i]);
      } else {
        model_->SkipBoolVar();
        VLOG(1) << "  - skipped";
      }
    }
  }

  array_index = 0;
  for (unsigned int i = 0; i < set_variables_.size(); i++) {
    if (!hadError) {
      const std::string& raw_name = set_variables_[i]->Name();
      std::string name;
      if (raw_name[0] == '[') {
        name = StringPrintf("%s[%d]", raw_name.c_str() + 1, ++array_index);
      } else {
        if (array_index == 0) {
          name = raw_name;
        } else {
          name = StringPrintf("%s[%d]", raw_name.c_str(), array_index + 1);
          array_index = 0;
        }
      }
      model_->NewSetVar(name, set_variables_[i]);
    }
  }

  VLOG(1) << "Creating constraints";

  for (unsigned int i = 0; i < constraints_.size(); i++) {
    if (!hadError) {
      CtSpec* const spec = constraints_[i];
      VLOG(1) << "Constraint " << constraints_[i]->DebugString();
      model_->PostConstraint(constraints_[i]);
    }
  }

  VLOG(1) << "Adding domain constraints";

  for (unsigned int i = int_domain_constraints_.size(); i--;) {
    if (!hadError) {
      const int var_id = int_domain_constraints_[i].first;
      IntVar* const var = model_->IntegerVariable(var_id);
      AST::SetLit* const dom = int_domain_constraints_[i].second;
      if (dom->interval && (dom->min > var->Min() || dom->max < var->Max())) {
        VLOG(1) << "Reduce integer variable " << var_id
                << " to " << dom->DebugString();
        var->SetRange(dom->min, dom->max);
      } else if (!dom->interval) {
        VLOG(1) << "Reduce integer variable " << var_id
                << " to " << dom->DebugString();
        var->SetValues(dom->s);
      }
    }
  }

  for (unsigned int i = bool_domain_constraints_.size(); i--;) {
    if (!hadError) {
      const int var_id = bool_domain_constraints_[i].first;
      AST::SetLit* const dom = bool_domain_constraints_[i].second;
      VLOG(1) << "Reduce bool variable " << var_id
              << " to " << dom->DebugString();
      if (dom->interval) {
        model_->BooleanVariable(var_id)->SetRange(dom->min, dom->max);
      } else {
        model_->BooleanVariable(var_id)->SetValues(dom->s);
      }
    }
  }
}

AST::Node* ParserState::ArrayElement(string id, unsigned int offset) {
  if (offset > 0) {
    vector<int64> tmp;
    if (int_var_array_map_.get(id, tmp) && offset<=tmp.size())
      return new AST::IntVar(tmp[offset-1]);
    if (bool_var_array_map_.get(id, tmp) && offset<=tmp.size())
      return new AST::BoolVar(tmp[offset-1]);
    if (set_var_array_map_.get(id, tmp) && offset<=tmp.size())
      return new AST::SetVar(tmp[offset-1]);

    if (int_value_array_map_.get(id, tmp) && offset<=tmp.size())
      return new AST::IntLit(tmp[offset-1]);
    if (bool_value_array_map_.get(id, tmp) && offset<=tmp.size())
      return new AST::BoolLit(tmp[offset-1]);
    vector<AST::SetLit> tmpS;
    if (set_value_array_map_.get(id, tmpS) && offset<=tmpS.size())
      return new AST::SetLit(tmpS[offset-1]);
  }

  LOG(ERROR) << "Error: array access to " << id << " invalid"
             << " in line no. " << yyget_lineno(yyscanner);
  hadError = true;
  return new AST::IntVar(0); // keep things consistent
}

AST::Node* ParserState::VarRefArg(string id, bool annotation) {
  int64 tmp;
  if (int_var_map_.get(id, tmp))
    return new AST::IntVar(tmp);
  if (bool_var_map_.get(id, tmp))
    return new AST::BoolVar(tmp);
  if (set_var_map_.get(id, tmp))
    return new AST::SetVar(tmp);
  if (annotation)
    return new AST::Atom(id);
  LOG(ERROR) << "Error: undefined variable " << id
             << " in line no. " << yyget_lineno(yyscanner);
  hadError = true;
  return new AST::IntVar(0); // keep things consistent
}

void ParserState::AddIntVarDomainConstraint(int var_id,
                                            AST::SetLit* const dom) {
  if (dom != NULL) {
    VLOG(1) << "  - adding int var domain constraint (" << var_id
            << ") : " << dom->DebugString();
    int_domain_constraints_.push_back(std::make_pair(var_id, dom));
  }
}

void ParserState::AddBoolVarDomainConstraint(int var_id,
                                             AST::SetLit* const dom) {
  if (dom != NULL) {
    VLOG(1) << "  - adding bool var domain constraint (" << var_id
            << ") : " << dom->DebugString();
    bool_domain_constraints_.push_back(std::make_pair(var_id, dom));
  }
}

void ParserState::AddSetVarDomainConstraint(int var_id,
                                            AST::SetLit* const dom) {
  if (dom != NULL) {
    VLOG(1) << "  - adding set var domain constraint (" << var_id
            << ") : " << dom->DebugString();
    set_domain_constraints_.push_back(std::make_pair(var_id, dom));
  }
}

int ParserState::FindEndIntegerVariable(int index) {
  while (int_variables_[index]->alias) {
    index = int_variables_[index]->i;
  }
  return index;
}

bool ParserState::IsBound(AST::Node* const node) const {
  return node->isInt() ||
      (node->isIntVar() && int_variables_[node->getIntVar()]->IsBound()) ||
      node->isBool() ||
      (node->isBoolVar() && bool_variables_[node->getBoolVar()]->IsBound());
}

int ParserState::GetBound(AST::Node* const node) const {
  if (node->isInt()) {
    return node->getInt();
  }
  if (node->isIntVar()) {
    return int_variables_[node->getIntVar()]->GetBound();
  }
  if (node->isBool()) {
    return node->getBool();
  }
  if (node->isBoolVar()) {
    return bool_variables_[node->getBoolVar()]->GetBound();
  }
  return 0;
}

bool ParserState::IsAllDifferent(AST::Node* const node) const {
  AST::Array* const array_variables = node->getArray();
  const int size = array_variables->a.size();
  std::vector<int> variables(size);
  for (int i = 0; i < size; ++i) {
    if (array_variables->a[i]->isIntVar()) {
      const int var = array_variables->a[i]->getIntVar();
      variables[i] = var;
    } else {
      return false;
    }
  }
  std::sort(variables.begin(), variables.end());

  // Naive
  for (int i = 0; i < all_differents_.size(); ++i) {
    const std::vector<int>& v = all_differents_[i];
    if (v.size() != size) {
      continue;
    }
    bool ok = true;
    for (int i = 0; i < size; ++i) {
      if (v[i] != variables[i]) {
        ok = false;
        continue;
      }
    }
    if (ok) {
      return true;
    }
  }
  return false;
}

bool ParserState::Presolve(CtSpec* const spec) {
  const string& id = spec->Id();
  const int index = spec->Index();
  if (id == "int_le") {
    if (spec->Arg(0)->isIntVar() && IsBound(spec->Arg(1))) {
      IntVarSpec* const var_spec =
          int_variables_[FindEndIntegerVariable(spec->Arg(0)->getIntVar())];
      const int bound = GetBound(spec->Arg(1));
      VLOG(1) << "  - presolve:  merge " << var_spec->DebugString()
              << " with kint32min.." << bound;
      const bool ok = var_spec->MergeBounds(kint32min, bound);
      if (ok) {
        spec->Nullify();
      }
      return ok;
    } else if (IsBound(spec->Arg(0)) && spec->Arg(1)->isIntVar()) {
      IntVarSpec* const var_spec =
          int_variables_[FindEndIntegerVariable(spec->Arg(1)->getIntVar())];
      const int bound = GetBound(spec->Arg(0));
      VLOG(1) << "  - presolve:  merge " << var_spec->DebugString() << " with "
              << bound << "..kint32max";
      const bool ok = var_spec->MergeBounds(bound, kint32max);
      if (ok) {
        spec->Nullify();
      }
      return ok;
    }
  }
  if (id == "int_eq") {
    if (spec->Arg(0)->isIntVar() && IsBound(spec->Arg(1))) {
      IntVarSpec* const var_spec =
          int_variables_[FindEndIntegerVariable(spec->Arg(0)->getIntVar())];
      const int bound = GetBound(spec->Arg(1));
      VLOG(1) << "  - presolve:  assign " << var_spec->DebugString()
              << " to " << bound;
      const bool ok = var_spec->MergeBounds(bound, bound);
      if (ok) {
        spec->Nullify();
      }
      return ok;
    } else if (IsBound(spec->Arg(0)) && spec->Arg(1)->isIntVar()) {
      IntVarSpec* const var_spec =
          int_variables_[FindEndIntegerVariable(spec->Arg(1)->getIntVar())];
      const int bound = GetBound(spec->Arg(0));
      VLOG(1) << "  - presolve:  assign " <<  var_spec->DebugString()
              << " to " << bound;
      const bool ok = var_spec->MergeBounds(bound, bound);
      if (ok) {
        spec->Nullify();
      }
      return ok;
    } else if (spec->Arg(0)->isIntVar() &&
               spec->Arg(1)->isIntVar() &&
               spec->annotations() == NULL &&
               !ContainsKey(stored_constraints_, index) &&
               (ContainsKey(orphans_, spec->Arg(0)->getIntVar()) ||
                ContainsKey(orphans_, spec->Arg(1)->getIntVar()))) {
      stored_constraints_.insert(index);
      const int var0 = spec->Arg(0)->getIntVar();
      const int var1 = spec->Arg(1)->getIntVar();
      if (ContainsKey(orphans_, var0)) {
        IntVarSpec* const spec0 = int_variables_[FindEndIntegerVariable(var0)];
        AST::Call* const call =
            new AST::Call("defines_var", new AST::IntVar(var0));
        spec->AddAnnotation(call);
        VLOG(1) << "  - presolve:  aliasing xi(" << var0 << ") to xi("
                << var1 << ")";
        orphans_.erase(var0);
        return true;
      } else if (ContainsKey(orphans_, var1)) {
        IntVarSpec* const spec1 = int_variables_[FindEndIntegerVariable(var1)];
        AST::Call* const call =
            new AST::Call("defines_var", new AST::IntVar(var1));
        spec->AddAnnotation(call);
        VLOG(1) << "  - presolve:  aliasing xi(" << var1 << ") to xi("
                << var0 << ")";
        orphans_.erase(var1);
        return true;
      }
    }
  }
  if (id == "set_in") {
    if (spec->Arg(0)->isIntVar() && spec->Arg(1)->isSet()) {
      IntVarSpec* const var_spec =
          int_variables_[FindEndIntegerVariable(spec->Arg(0)->getIntVar())];
      AST::SetLit* const domain = spec->Arg(1)->getSet();
      VLOG(1) << "  - presolve:  merge " << var_spec->DebugString() << " with "
              << domain->DebugString();
      bool ok = false;
      if (domain->interval) {
        ok = var_spec->MergeBounds(domain->min, domain->max);
      } else {
        ok = var_spec->MergeDomain(domain->s);
      }
      if (ok) {
        spec->Nullify();
      }
      return ok;
    }
  }
  if (id == "array_bool_and" &&
      IsBound(spec->Arg(1)) &&
      GetBound(spec->Arg(1)) == 1) {
    VLOG(1) << "  - presolve:  forcing array_bool_and to 1 on "
            << spec->DebugString();
    AST::Array* const array_variables = spec->Arg(0)->getArray();
    const int size = array_variables->a.size();
    for (int i = 0; i < size; ++i) {
      if (array_variables->a[i]->isBoolVar()) {
        const int boolvar = array_variables->a[i]->getBoolVar();
        bool_variables_[boolvar]->Assign(true);
      }
    }
    spec->Nullify();
    return true;
  }
  if (id.find("_reif") != string::npos &&
      IsBound(spec->LastArg()) &&
      GetBound(spec->LastArg()) == 1) {
    VLOG(1) << "  - presolve:  unreify " << spec->DebugString();
    spec->Unreify();
    return true;
  }
  if (id == "all_different_int" && !ContainsKey(stored_constraints_, index)) {
    AST::Array* const array_variables = spec->Arg(0)->getArray();
    const int size = array_variables->a.size();
    std::vector<int> variables(size);
    for (int i = 0; i < size; ++i) {
      if (array_variables->a[i]->isIntVar()) {
        const int var = array_variables->a[i]->getIntVar();
        variables[i] = var;
      } else {
        return false;
      }
    }
    VLOG(1) << "  - presolve:  store all diff info " << spec->DebugString();
    std::sort(variables.begin(), variables.end());
    stored_constraints_.insert(index);
    all_differents_.push_back(variables);
    return true;
  }
  if (id == "array_var_int_element" &&
      IsBound(spec->Arg(2)) &&
      IsAllDifferent(spec->Arg(1))) {
    VLOG(1) << "  - presolve:  reinforce " << spec->DebugString()
            << " to array_var_int_position";
    spec->SetId("array_var_int_position");
    const int bound = GetBound(spec->Arg(2));
    spec->ReplaceArg(2, new AST::IntLit(bound));
    return true;
  }
  if (id == "int_abs" &&
      !ContainsKey(stored_constraints_, index) &&
      spec->Arg(0)->isIntVar() &&
      spec->Arg(1)->isIntVar()) {
    abs_map_[spec->Arg(1)->getIntVar()] = spec->Arg(0)->getIntVar();
    stored_constraints_.insert(index);
    return true;
  }
  if (id == "int_eq_reif") {
    if (spec->Arg(0)->isIntVar() &&
        ContainsKey(abs_map_, spec->Arg(0)->getIntVar()) &&
        spec->Arg(1)->isInt() &&
        spec->Arg(1)->getInt() == 0) {
      VLOG(1) << "  - presolve:  remove abs() in " << spec->DebugString();
      dynamic_cast<AST::IntVar*>(spec->Arg(0))->i =
          abs_map_[spec->Arg(0)->getIntVar()];
    }
  }
  if (id == "int_ne_reif") {
    if (spec->Arg(0)->isIntVar() &&
        ContainsKey(abs_map_, spec->Arg(0)->getIntVar()) &&
        spec->Arg(1)->isInt() &&
        spec->Arg(1)->getInt() == 0) {
      VLOG(1) << "  - presolve:  remove abs() in " << spec->DebugString();
      dynamic_cast<AST::IntVar*>(spec->Arg(0))->i =
          abs_map_[spec->Arg(0)->getIntVar()];
    }
  }
  if (id == "int_ne") {
    if (spec->Arg(0)->isIntVar() &&
        ContainsKey(abs_map_, spec->Arg(0)->getIntVar()) &&
        spec->Arg(1)->isInt() &&
        spec->Arg(1)->getInt() == 0) {
      VLOG(1) << "  - presolve:  remove abs() in " << spec->DebugString();
      dynamic_cast<AST::IntVar*>(spec->Arg(0))->i =
          abs_map_[spec->Arg(0)->getIntVar()];
    }
  }
  return false;
}

void ParserState::Strongify(int constraint_index) {

}

void ParserState::AddConstraint(const std::string& id,
                                AST::Array* const args,
                                AST::Node* const annotations) {
  constraints_.push_back(
      new CtSpec(constraints_.size(), id, args, annotations));
}

void ParserState::InitModel() {
  if (!hadError) {
    model_->Init(int_variables_.size(),
                 bool_variables_.size(),
                 set_variables_.size());
    constraints_per_int_variables_.resize(int_variables_.size());
    constraints_per_bool_variables_.resize(bool_variables_.size());
  }
}

void ParserState::FillOutput(operations_research::FlatZincModel& m) {
  m.InitOutput(Output());
}

void FlatZincModel::Parse(const std::string& filename) {
  filename_ = filename;
  filename_.resize(filename_.size() - 4);
  size_t found = filename_.find_last_of("/\\");
  if (found != string::npos) {
    filename_ = filename_.substr(found + 1);
  }
#ifdef HAVE_MMAP
  int fd;
  char* data;
  struct stat sbuf;
  fd = open(filename.c_str(), O_RDONLY);
  if (fd == -1) {
    LOG(ERROR) << "Cannot open file " << filename;
    return NULL;
  }
  if (stat(filename.c_str(), &sbuf) == -1) {
    LOG(ERROR) << "Cannot stat file " << filename;
    return NULL;
  }
  data = (char*)mmap((caddr_t)0, sbuf.st_size, PROT_READ, MAP_SHARED, fd,0);
  if (data == (caddr_t)(-1)) {
    LOG(ERROR) << "Cannot mmap file " << filename;
    return NULL;
  }

  ParserState pp(data, sbuf.st_size, this);
#else
  std::ifstream file;
  file.open(filename.c_str());
  if (!file.is_open()) {
    LOG(FATAL) << "Cannot open file " << filename;
  }
  std::string s = string(istreambuf_iterator<char>(file),
                         istreambuf_iterator<char>());
  ParserState pp(s, this);
#endif
  yylex_init(&pp.yyscanner);
  yyset_extra(&pp, pp.yyscanner);
  // yydebug = 1;
  yyparse(&pp);
  pp.FillOutput(*this);

  if (pp.yyscanner)
    yylex_destroy(pp.yyscanner);
  parsed_ok_ = !pp.hadError;
}

void FlatZincModel::Parse(std::istream& is) {
  filename_ = "stdin";
  std::string s = string(istreambuf_iterator<char>(is),
                         istreambuf_iterator<char>());

  ParserState pp(s, this);
  yylex_init(&pp.yyscanner);
  yyset_extra(&pp, pp.yyscanner);
  // yydebug = 1;
  yyparse(&pp);
  pp.FillOutput(*this);

  if (pp.yyscanner)
    yylex_destroy(pp.yyscanner);
  parsed_ok_ = !pp.hadError;
}

AST::Node* ArrayOutput(AST::Call* ann) {
  AST::Array* a = NULL;

  if (ann->args->isArray()) {
    a = ann->args->getArray();
  } else {
    a = new AST::Array(ann->args);
  }

  std::string out;

  out = StringPrintf("array%dd(", a->a.size());;
  for (unsigned int i = 0; i < a->a.size(); i++) {
    AST::SetLit* s = a->a[i]->getSet();
    if (s->empty()) {
      out += "{}, ";
    } else if (s->interval) {
      out += StringPrintf("%d..%d, ", s->min, s->max);
    } else {
      out += "{";
      for (unsigned int j = 0; j < s->s.size(); j++) {
        out += s->s[j];
        if (j < s->s.size() - 1) {
          out += ",";
        }
      }
      out += "}, ";
    }
  }

  if (!ann->args->isArray()) {
    a->a[0] = NULL;
    delete a;
  }
  return new AST::String(out);
}
}  // namespace operations_research

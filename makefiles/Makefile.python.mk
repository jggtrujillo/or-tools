# Python support using SWIG

# pywrapknapsack_solver

pyalgorithms: _pywrapknapsack_solver.$(SHAREDLIBEXT) gen/algorithms/pywrapknapsack_solver.py $(ALGORITHMS_LIBS) $(BASE_LIBS)

gen/algorithms/pywrapknapsack_solver.py: algorithms/knapsack_solver.swig algorithms/knapsack_solver.h base/base.swig
	$(SWIG_BINARY) -c++ -python -o gen/algorithms/knapsack_solver_wrap.cc -module pywrapknapsack_solver algorithms/knapsack_solver.swig

gen/algorithms/knapsack_solver_wrap.cc: gen/algorithms/pywrapknapsack_solver.py

objs/knapsack_solver_wrap.$O: gen/algorithms/knapsack_solver_wrap.cc
	$(CCC) $(CFLAGS) $(PYTHON_INC) -c gen/algorithms/knapsack_solver_wrap.cc $(OBJOUT)objs/knapsack_solver_wrap.$O

_pywrapknapsack_solver.$(SHAREDLIBEXT): objs/knapsack_solver_wrap.$O $(ALGORITHMS_LIBS) $(LP_LIBS) $(BASE_LIBS)
	$(LD) $(LDOUT)_pywrapknapsack_solver.$(SHAREDLIBEXT) objs/knapsack_solver_wrap.$O $(ALGORITHMS_LIBS) $(LP_LIBS) $(BASE_LIBS) $(LDLPDEPS) $(LDFLAGS) $(PYTHON_LNK)
ifeq "$(SYSTEM)" "win"
	copy _pywrapknapsack_solver.dll gen\\algorithms\\_pywrapknapsack_solver.pyd
endif
# pywrapflow

pygraph: _pywrapflow.$(SHAREDLIBEXT) gen/graph/pywrapflow.py $(GRAPH_LIBS) $(BASE_LIBS)

gen/graph/pywrapflow.py: graph/flow.swig graph/min_cost_flow.h graph/max_flow.h graph/ebert_graph.h base/base.swig
	$(SWIG_BINARY) -c++ -python -o gen/graph/pywrapflow_wrap.cc -module pywrapflow graph/flow.swig

gen/graph/pywrapflow_wrap.cc: gen/graph/pywrapflow.py

objs/pywrapflow_wrap.$O: gen/graph/pywrapflow_wrap.cc
	$(CCC) $(CFLAGS) $(PYTHON_INC) -c gen/graph/pywrapflow_wrap.cc $(OBJOUT)objs/pywrapflow_wrap.$O

_pywrapflow.$(SHAREDLIBEXT): objs/pywrapflow_wrap.$O $(GRAPH_LIBS) $(BASE_LIBS)
	$(LD) $(LDOUT)_pywrapflow.$(SHAREDLIBEXT) objs/pywrapflow_wrap.$O $(GRAPH_LIBS) $(BASE_LIBS) $(LDFLAGS) $(PYTHON_LNK)
ifeq "$(SYSTEM)" "win"
	copy _pywrapflow.dll gen\\graph\\_pywrapflow.pyd
endif

# pywrapcp

pycp: _pywrapcp.$(SHAREDLIBEXT) gen/constraint_solver/pywrapcp.py _pywraprouting.$(SHAREDLIBEXT) gen/constraint_solver/pywraprouting.py $(CP_LIBS) $(BASE_LIBS)

gen/constraint_solver/pywrapcp.py: constraint_solver/constraint_solver.swig constraint_solver/constraint_solver.h constraint_solver/constraint_solveri.h base/base.swig
	$(SWIG_BINARY) -c++ -python -o gen/constraint_solver/constraint_solver_wrap.cc -module pywrapcp constraint_solver/constraint_solver.swig

gen/constraint_solver/constraint_solver_wrap.cc: gen/constraint_solver/pywrapcp.py

objs/constraint_solver_wrap.$O: gen/constraint_solver/constraint_solver_wrap.cc
	$(CCC) $(CFLAGS) $(PYTHON_INC) -c gen/constraint_solver/constraint_solver_wrap.cc $(OBJOUT)objs/constraint_solver_wrap.$O

_pywrapcp.$(SHAREDLIBEXT): objs/constraint_solver_wrap.$O $(CP_LIBS) $(BASE_LIBS)
	$(LD) $(LDOUT)_pywrapcp.$(SHAREDLIBEXT) objs/constraint_solver_wrap.$O $(CP_LIBS) $(BASE_LIBS) $(LDFLAGS) $(PYTHON_LNK)
ifeq "$(SYSTEM)" "win"
	copy _pywrapcp.dll gen\\constraint_solver\\_pywrapcp.pyd
endif


# pywraprouting

gen/constraint_solver/pywraprouting.py: constraint_solver/routing.swig constraint_solver/constraint_solver.h constraint_solver/constraint_solveri.h constraint_solver/routing.h base/base.swig
	$(SWIG_BINARY) -c++ -python -o gen/constraint_solver/routing_wrap.cc -module pywraprouting constraint_solver/routing.swig

gen/constraint_solver/routing_wrap.cc: gen/constraint_solver/pywraprouting.py

objs/routing_wrap.$O: gen/constraint_solver/routing_wrap.cc
	$(CCC) $(CFLAGS) $(PYTHON_INC) -c gen/constraint_solver/routing_wrap.cc $(OBJOUT)objs/routing_wrap.$O

_pywraprouting.$(SHAREDLIBEXT): objs/routing_wrap.$O $(ROUTING_LIBS) $(BASE_LIBS)
	$(LD) $(LDOUT)_pywraprouting.$(SHAREDLIBEXT) objs/routing_wrap.$O $(ROUTING_LIBS) $(BASE_LIBS) $(LDFLAGS) $(PYTHON_LNK)
ifeq "$(SYSTEM)" "win"
	copy _pywraprouting.dll gen\\constraint_solver\\_pywraprouting.pyd
endif

# pywraplp

pylp: _pywraplp.$(SHAREDLIBEXT) gen/linear_solver/pywraplp.py $(LP_LIBS) $(BASE_LIBS)

gen/linear_solver/pywraplp.py: linear_solver/linear_solver.swig linear_solver/linear_solver.h base/base.swig gen/linear_solver/linear_solver.pb.h
	$(SWIG_BINARY)  $(SWIG_INC) -c++ -python -o gen/linear_solver/linear_solver_wrap.cc -module pywraplp linear_solver/linear_solver.swig

gen/linear_solver/linear_solver_wrap.cc: gen/linear_solver/pywraplp.py

objs/linear_solver_wrap.$O: gen/linear_solver/linear_solver_wrap.cc
	$(CCC) $(CFLAGS) $(PYTHON_INC) -c gen/linear_solver/linear_solver_wrap.cc $(OBJOUT)objs/linear_solver_wrap.$O

_pywraplp.$(SHAREDLIBEXT): objs/linear_solver_wrap.$O $(LP_LIBS) $(BASE_LIBS)
	$(LD) $(LDOUT)_pywraplp.$(SHAREDLIBEXT) objs/linear_solver_wrap.$O $(LP_LIBS) $(BASE_LIBS) $(LDLPDEPS) $(LDFLAGS) $(PYTHON_LNK)
ifeq "$(SYSTEM)" "win"
	copy _pywraplp.dll gen\\linear_solver\\_pywraplp.pyd
endif

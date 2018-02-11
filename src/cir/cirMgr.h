/****************************************************************************
  FileName     [ cirMgr.h ]
  PackageName  [ cir ]
  Synopsis     [ Define circuit manager ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_MGR_H
#define CIR_MGR_H

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <set>
#include <map>
#include "cirGate.h"

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

#include "cirDef.h"

extern CirMgr *cirMgr;

class CirMgr
{
typedef vector<unsigned> IdList;
public:
   CirMgr() : _gateList(0), _PIList(0), _POList(0), _AIGList(0), _size(0), _PIsize(0), _POsize(0), _AIGsize(0), _fecInit(false) {}
   ~CirMgr() { reset(); } 

   // Access functions
   // return '0' if "gid" corresponds to an undefined gate.
   CirGate* getGate(unsigned gid) const { if(gid > _size) return 0; return _gateList[gid]; }
   static CirGate* readAigGateV(const AigGateV& ptr) { return ptr.gate(); }
   const vector<unsigned>& getFEClist(const unsigned& id) const { return *(_fecGrps[id]); }
   unsigned getFECsize() const { return _fecGrps.size(); }

   // Member functions about circuit construction
   bool readCircuit(const string&);

   // Member functions about circuit optimization
   void sweep();
   void optimize();

   // Member functions about simulation
   void randomSim();
   void fileSim(ifstream&);
   void setSimLog(ofstream *logFile) { _simLog = logFile; }

   // Member functions about fraig
   void strash();
   void printFEC() const;
   void fraig();

   // Member functions about circuit reporting
   void printSummary() const;
   void printNetlist() const;
   void printPIs() const;
   void printPOs() const;
   void printFloatGates() const;
   void printFECPairs() const;
   void writeAag(ostream&) const;
   void writeGate(ostream&, CirGate*) const;

private:
   bool lexOptions(const string&, vector<string>&, size_t nOpts = 0) const;
   void printNetlist(AigGateV,int&) const;
   vector<int> getAigList() const;
   void traverseFanIn(AigGateV,vector<int>&) const;
   void updateDfsList();
   void dfs(CirGate*);
   void gateDfs(CirGate*,vector<CirGate*>&) const;
   void optSameFanIn(CirGate*,bool);
   void optConstFanIn(CirGate*,bool);
   void sim(unsigned);
   void resetPattern();
   void initFEC();
   void chkFEC();
   void resetFecGrps();
   void genProofModel(SatSolver*&);
   unsigned proveFEC(SatSolver*&,CirGate*&,vector<pair<CirGate*,unsigned>>&);
   void merge(vector<pair<CirGate*,unsigned>>&);
   static bool sortFanOut(AigGateV p1,AigGateV p2) { return (p1.gate() -> getGateID() < p2.gate() -> getGateID()); }
   void reset();

   CirGate** _gateList;
   size_t* _PIList;
   size_t* _POList;
   size_t* _AIGList;
   unsigned _size;
   size_t _PIsize;
   size_t _POsize;
   size_t _AIGsize;
   bool _fecInit;
   vector<int> headerInfo; //M I L O A
   mutable set<size_t> _floatList; //gates with floating fanin(s)
   mutable set<size_t> _unusedList; //gates defined but not used
   mutable vector<CirGate*> _dfsList;
   mutable vector<IdList*> _fecGrps;
   ofstream           *_simLog;

};

#endif // CIR_MGR_H

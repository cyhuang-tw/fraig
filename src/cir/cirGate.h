/****************************************************************************
  FileName     [ cirGate.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic gate data structures ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_GATE_H
#define CIR_GATE_H

#include <string>
#include <vector>
#include <iostream>
#include "cirDef.h"
#include "sat.h"

using namespace std;

class CirGate;
class AigGateV;
class AigGate;
class PIGate;
class POGate;
class Const0Gate;
class UnDefGate;

//------------------------------------------------------------------------
//   Define classes
//------------------------------------------------------------------------
// TODO: Define your own data members and member functions, or classes

//------------------------------------------------------------------------
//   class AigGateV
//------------------------------------------------------------------------

class AigGateV{
  friend class CirGate;
  friend class AigGate;
  friend class PIGate;
  friend class POGate;
  friend class Const0Gate;
  friend class UnDefGate;
  friend class CirMgr;
  #define NEG 0x1
public:
  bool operator == (const AigGateV& ptr) const { return (_gateV == ptr._gateV); }
  size_t operator & (const AigGateV& g) const { return (_gateV & g._gateV); }

  AigGateV(CirGate* g, size_t phase): _gateV(size_t(g) + phase) {}
  CirGate* gate() const {return (CirGate*)(_gateV & ~size_t(NEG));}
  bool isInv() const {return (_gateV & NEG);}
private:
  size_t _gateV;
};

//------------------------------------------------------------------------
//   class CirGate
//------------------------------------------------------------------------

class CirGate
{
public:
   CirGate() {}
   virtual ~CirGate() {}

   // Basic access methods
   virtual string getTypeStr() const = 0;
   unsigned getLineNo() const { return _line; }
   unsigned getGateID() const { return _id; }
   size_t& getRef() const { return _ref; }
   string getName() const { return _name; }
   vector<AigGateV>& getFanInList() { return _fanInList; }
   vector<AigGateV>& getFanOutList() { return _fanOutList; }
   size_t& getPattern() { return _pattern; }
   virtual int getFec() { return -1; }
   virtual Var& getVar() { return _var; }
   static size_t& getGlobalRef() { return _globalRef; }
   virtual bool isAig() const { return false; }

   // Printing functions
   virtual void printGate() const = 0;
   void reportGate() const;
   void reportFanin(int level) const;
   void reportFanout(int level) const;

   //setting functions
   void push_fanInList(AigGateV ptr) { _fanInList.push_back(ptr); }
   void push_fanOutList(AigGateV ptr) { _fanOutList.push_back(ptr); }
   //void setFanIn(size_t& pos, AigGateV ptr) { _fanInList[pos] = ptr; }
   void setName(string& str) { _name = str; }
   void setRef(size_t num) const { _ref = num; }
   void merge(CirGate*);
   void fraigMerge(CirGate*,bool);
   void setPattern(size_t& p) { _pattern = p; }
   void resetPattern() { _pattern = 0; }
   virtual void setFecId(int&) {}
   virtual void setFecId(int) {}
   virtual void setVar(Var& v) { _var = v; }
   virtual void simulation() {}
   virtual void updateInfo() {}
   virtual void updateFanIn(size_t&) {}
   static void IncGlobalRef() { ++_globalRef; }

private:
   void reportFanin(int,int,AigGateV) const;
   void reportFanout(int,int,AigGateV) const;
   virtual string getFECpair(const CirMgr*) const { return ""; };
   string getSimValue() const;

protected:
   string _name;
   unsigned _id;
   unsigned _line;
   vector<AigGateV> _fanInList;
   vector<AigGateV> _fanOutList;
   size_t _pattern;
   Var _var;
   mutable size_t _ref;
   static size_t _globalRef;
};


//------------------------------------------------------------------------
//   class AigGate
//------------------------------------------------------------------------

class AigGate : public CirGate {
public:
   AigGate(unsigned i,unsigned l) { _name = ""; _id = i; _line = l; _ref = 0; _pattern = 0; _fecId = 0; }
   ~AigGate() {}
   string getTypeStr() const { return "AIG"; }
   bool isAig() const { return true; }
   void setFEC(int& n) { _fecId = n; }
   int getFec() { return _fecId; }
   void setFecId(int& n) { _fecId = n; }
   void setFecId(int n) { _fecId = n; }
   Var& getVar() { return _var; }
   void setVar(Var& v) { _var = v; }
   void updateFanIn(size_t& pos) { if(pos == 0) swap(_fanInList[0],_fanInList[1]); }
   void printGate() const {
    cout << "AIG" << " " << _id << " ";
    for(short i = 0; i < 2; ++i){
      if(_fanInList[i].gate() -> getTypeStr() == "UNDEF") cout << "*";
      if(_fanInList[i].isInv()) cout << "!";
      cout << _fanInList[i].gate() -> getGateID();
      if(i < 1) cout << " ";
    }
    if(_name != "") cout << " " << "(" << _name << ")";
    cout << endl;  
   }
   void simulation(){
    size_t p1 = (_fanInList[0].gate()) -> getPattern();
    size_t p2 = (_fanInList[1].gate()) -> getPattern();
    if(_fanInList[0].isInv()) p1 = ~p1;
    if(_fanInList[1].isInv()) p2 = ~p2;
    _pattern = p1 & p2;
   }
 private:
  string getFECpair(const CirMgr*) const;
  int _fecId;
};

//------------------------------------------------------------------------
//   class PIGate
//------------------------------------------------------------------------

class PIGate : public CirGate {
public:
   PIGate(unsigned i,unsigned l) { _name = ""; _id = i; _line = l; _ref = 0; _pattern = 0; }
   ~PIGate() {}
   string getTypeStr() const { return "PI"; }
   Var& getVar() { return _var; }
   void setVar(Var& v) { _var = v; }
   void printGate() const {
    cout << "PI" << "  " << _id;
    if(_name != "") cout << " " << "(" << _name << ")";
    cout << endl;
   }
private:
};

//------------------------------------------------------------------------
//   class POGate
//------------------------------------------------------------------------

class POGate : public CirGate {
public:
   POGate(unsigned i,unsigned l,size_t p) : _out(p) { _name = ""; _id = i; _line = l; _ref = 0; _pattern = 0; }
   ~POGate() {}
   string getTypeStr() const { return "PO"; }
   bool isInv() const { return (_out%2 != 0); }
   size_t outGateIdx() const { return _out/2; }
   size_t getInfo() const { return _out; }
   void setInfo(size_t n) { _out = n; }
   void updateInfo() { _out = 2*((_fanInList[0]).gate() -> getGateID()); if((_fanInList[0]).isInv()) ++_out; }
   void printGate() const {
    cout << "PO" << "  " << _id << " ";
    if(_fanInList[0].gate() -> getTypeStr() == "UNDEF") cout << "*";
    if(isInv()) cout << "!";
    cout << _out/2;
    if(_name != "") cout << " " << "(" << _name << ")";
    cout << endl;
   }
   void simulation(){ _pattern = _fanInList[0].gate() -> getPattern(); if(isInv()) _pattern = ~_pattern; }
private:
   size_t _out;  
};

//------------------------------------------------------------------------
//   class Const0Gate
//------------------------------------------------------------------------

class Const0Gate : public CirGate {
public:
   Const0Gate(unsigned i,unsigned l) { _name = ""; _id = i; _line = l; _ref = 0; _pattern = 0; _fecId = 0; }
   ~Const0Gate() {}
   string getTypeStr() const { return "CONST"; }
   int getFec() { return _fecId; }
   void setFecId(int& n) { _fecId = n; }
   void setFecId(int n) { _fecId = n; }
   Var& getVar() { return _var; }
   void setVar(Var& v) { _var = v; }
   void printGate() const {
    cout << "CONST0" << endl;
   }
private:
  string getFECpair(const CirMgr*) const;
  int _fecId;
};

//------------------------------------------------------------------------
//   class UnDefGate
//------------------------------------------------------------------------

class UnDefGate : public CirGate {
public:
   UnDefGate(unsigned i) { _name = ""; _id = i; _line = 0; _ref = 0; _pattern = 0; }  
   ~UnDefGate() {}
   string getTypeStr() const { return "UNDEF"; }
   void printGate() const {}
   Var& getVar() { return _var; }
   void setVar(Var& v) { _var = v; }
private:
};


#endif // CIR_GATE_H

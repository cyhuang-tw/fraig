/****************************************************************************
  FileName     [ cirFraig.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir FRAIG functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include <algorithm>
#include <ctime>
#include "cirMgr.h"
#include "cirGate.h"
#include "sat.h"
#include "myHashMap.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::strash()" and "CirMgr::fraig()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
class CirKey
{
public:
	//CirKey() { cerr << "You should not use default constructor, idiot!" << endl; }
	CirKey(AigGateV ptr1,AigGateV ptr2): g1(ptr1), g2(ptr2) {}

	size_t operator() () const { return (g1 & g2); }

	bool operator == (const CirKey& k) const { return (((g1 == k.g1) && (g2 == k.g2)) || ((g1 == k.g2) && (g2 == k.g1))); }

private:
	AigGateV g1,g2;
};


/*******************************************/
/*   Public member functions about fraig   */
/*******************************************/
// _floatList may be changed.
// _unusedList and _undefList won't be changed
void
CirMgr::strash()
{
	HashMap<CirKey,CirGate*> cirHash(_size/2);
	updateDfsList();
	vector<size_t> mergedList;
	for(unsigned i = 0; i < _dfsList.size(); ++i){
		if(_dfsList[i] -> getTypeStr() != "AIG") continue;
		//cout << "checking "; _dfsList[i] -> printGate();  cout << endl;
		AigGateV& g1 = (_dfsList[i] -> getFanInList())[0];
		AigGateV& g2 = (_dfsList[i] -> getFanInList())[1];
		CirKey k(g1,g2);
		CirGate* ptr = 0;
		if(cirHash.query(k,ptr)){
			size_t gid = _dfsList[i] -> getGateID();
			cout << "Strashing: " << ptr -> getGateID() << " merging " << gid << endl;
			mergedList.push_back(_dfsList[i] -> getGateID());
			ptr -> merge(_dfsList[i]);
			_dfsList[i] = 0;
			_gateList[gid] = 0;
			--_AIGsize;
		}
		else
			cirHash.insert(k,_dfsList[i]);
	}
}

void
CirMgr::fraig()
{
	unsigned count = 0; //indicate the proofs SAT engine has proved.
	unsigned unsatNum = 0; //indicate the number of UNSAT (that should be merged) SAT engine has made.
	clock_t start,stop;
	bool _do = true;
	vector<pair<CirGate*,unsigned>> mergeList;
	vector<CirGate*> undefList;
	updateDfsList();
	SatSolver* s = new SatSolver;
	s -> initialize();
	genProofModel(s);
	vector<CirGate*> _netList = _dfsList;
	for(unsigned i = 0; i < _dfsList.size(); ++i){
		if(_dfsList[i] -> getTypeStr() == "UNDEF")
			undefList.push_back(_dfsList[i]);
	}

	if(_fecGrps.empty()) { return; }

	start = clock();

	if(_gateList[0] -> getFec() != -1)
		proveFEC(s,_gateList[0],mergeList);

	stop = clock();
	if((double)(stop-start)/CLOCKS_PER_SEC > 120) _do = false;

	if(_do){
		for(unsigned i = 0; i < _netList.size(); ++i){
			if(_netList[i] -> getTypeStr() != "AIG") continue;
			if(_netList[i] -> getFec() == -1) continue;
			proveFEC(s,_netList[i],mergeList);
			stop = clock();
			//if(abs((double)(stop-start)/CLOCKS_PER_SEC) < 2) break;
			if((double)(stop-start)/CLOCKS_PER_SEC > 150) break;
		}
	}

	merge(mergeList);
	resetFecGrps();
	for(unsigned i = 0; i <= _size; ++i){
		if(!_gateList[i]) continue;
		_gateList[i] -> setFecId(0);
	}
	_fecInit = false;
/*
	vector<IdList*> _newFecGrps;

	for(unsigned i = 0; i < _fecGrps.size(); ++i){
		if(!_fecGrps[i]) continue;
		if(_fecGrps[i] -> empty()) delete _fecGrps[i];
		else
			_newFecGrps.push_back(_fecGrps[i]);
	}

	_fecGrps.swap(_newFecGrps);

	for(unsigned i = 0; i < _fecGrps.size(); ++i){
		for(unsigned j = 0; j < _fecGrps[i] -> size(); ++j){
			_gateList[(*_fecGrps[i])[j]] -> setFecId(i);
		}
	}
*/
	updateDfsList();

	for(unsigned i = 0; i < _POsize; ++i){
		dynamic_cast<POGate*>(_gateList[_POList[i]]) -> updateInfo();
	}

	_AIGsize = 0;

	for(unsigned i = 0; i <= _size; ++i){
		if(!_gateList[i]) continue;
		if(_gateList[i] -> getTypeStr() == "AIG")
			++_AIGsize;
	}

	strash();

	delete s;
	
}

/********************************************/
/*   Private member functions about fraig   */
/********************************************/
void
CirMgr::genProofModel(SatSolver*& s)
{
	updateDfsList();
	Var vc = s -> newVar();
	(_gateList[0]) -> setVar(vc);
	for(unsigned i = 0; i < _dfsList.size(); ++i){
		if(_dfsList[i] -> getTypeStr() == "AIG"){
			Var v = s -> newVar();
			(_dfsList[i]) -> setVar(v);
		}
		if(_dfsList[i] -> getTypeStr() == "PI"){
			Var v = s -> newVar();
			(_dfsList[i]) -> setVar(v);
		}
		if(_dfsList[i] -> getTypeStr() == "UNDEF"){
			Var v = s -> newVar();
			(_dfsList[i]) -> setVar(v);
			s -> assumeProperty(_dfsList[i] -> getVar(),false);
		}
	}

	for(unsigned i = 0; i < _dfsList.size(); ++i){
		if(_dfsList[i] -> getTypeStr() != "AIG") continue;
		AigGateV& p1 = (_dfsList[i] -> getFanInList())[0];
		AigGateV& p2 = (_dfsList[i] -> getFanInList())[1];
		s -> addAigCNF((_dfsList[i]) -> getVar(),p1.gate() -> getVar(),p1.isInv(),p2.gate() -> getVar(),p2.isInv());
	}
	s -> assumeProperty((_gateList[0]) -> getVar(),false);
}

unsigned
CirMgr::proveFEC(SatSolver*& s,CirGate*& g,vector<pair<CirGate*,unsigned>>& p) //return the # of gates that should be merge
{
	IdList*& list = (_fecGrps[g -> getFec()]);
	unsigned id = g -> getGateID();
	unsigned oldSize = list -> size();

	vector<int> tmpList;
	for(unsigned i = 0; i < list -> size(); ++i)
		tmpList.push_back((*list)[i]);

	if(tmpList.size() == 1){ _gateList[tmpList[0]] -> setFecId(-1); tmpList[0] = -1; list -> clear(); return 0; }

	for(unsigned i = 0; i < tmpList.size(); ++i){
		if(tmpList[i] == id) { tmpList[i] = -1; continue; }
		if(tmpList[i] == -1) continue;
		bool isInv = false;
		bool result;
		if(g -> getPattern() == ~(_gateList[tmpList[i]]) -> getPattern()) isInv = true;
		Var newVar = s -> newVar();
		s -> addXorCNF(newVar,g -> getVar(),false,_gateList[tmpList[i]] -> getVar(),isInv);
		s -> assumeRelease();
		s -> assumeProperty(newVar,true);
		s -> assumeProperty(_gateList[0] -> getVar(),false);
		/*for(unsigned i = 0; i < undefList.size() ++i){
			s -> assumeProperty(undefList[i] -> getVar(),false);
		}
		*/
		cout << "Proving (" << id << ",";
		if(isInv) cout << "!";
		cout << tmpList[i] << ")..." << flush;
		cout << char(13) << setw(30) << ' ' << char(13);
		result = s -> assumpSolve();

		cout << "(" << id << ",";
		if(isInv) cout << "!";
		cout << tmpList[i] << ")" << " ";

		if(result){
			cout << "SAT" << flush;
			cout << char(13) << setw(30) << ' ' << char(13);
		}
		if(!result){
			cout << "UNSAT" << flush;
			cout << char(13) << setw(30) << ' ' << char(13);
			p.push_back(pair<CirGate*,unsigned>(g,2*tmpList[i]+isInv));
			_gateList[tmpList[i]] -> setFecId(-1);
			tmpList[i] = -1;
		}
	}
	g -> setFecId(-1);

	list -> clear();

	for(unsigned i = 0; i < tmpList.size(); ++i){
		if(tmpList[i] == -1) continue;
		list -> push_back(tmpList[i]);
	}
	return (oldSize - list -> size());
}

void
CirMgr::merge(vector<pair<CirGate*,unsigned>>& mergeList)
{
	for(unsigned i = 0; i < mergeList.size(); ++i){
		cout << "fraig: " << (mergeList[i]).first -> getGateID() << " merging ";
		if((mergeList[i]).second % 2 != 0) cout << "!";
		cout << ((mergeList[i]).second / 2) << endl;
		if((mergeList[i]).second % 2 != 0)
			(mergeList[i]).first -> fraigMerge(_gateList[((mergeList[i]).second)/2],true);
		else
			(mergeList[i]).first -> fraigMerge(_gateList[((mergeList[i]).second)/2],false);
		//--_AIGsize;
		delete _gateList[((mergeList[i]).second)/2];
		_gateList[((mergeList[i]).second)/2] = 0;
	}
}
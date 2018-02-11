/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir optimization functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include <set>
#include <vector>
#include <algorithm>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::sweep()" and "CirMgr::optimize()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/
enum GateCase
{
	CONST0_FANIN = 0,
	CONST1_FANIN = 1,
	SAME_FANIN = 2,
	INV_FANIN = 3,
	NORMAL = 4
};

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
static int
binary_search (const vector<AigGateV>& v,const size_t& n)
{
	size_t mid = 0, left = 0;
	size_t right = v.size();
	while(left < right){
		mid = left + (right -left)/2;
		if(n > CirMgr::readAigGateV(v[mid]) -> getGateID())
			left = mid + 1;
		else
			if(n < CirMgr::readAigGateV(v[mid]) -> getGateID())
				right = mid;
			else
				return mid;
	}
	return -1;
}

static bool
removeInput(CirGate* g,const size_t& n)
{
	/*
	vector<AigGateV>::iterator it = (g -> getFanInList()).begin();
	if(CirMgr::readAigGateV(*it) -> getGateID() != n) ++it;
	(g -> getFanInList()).erase(it);
	return true;
	*/
	vector<AigGateV>& inList = g -> getFanInList();
	for(unsigned i = 0; i < inList.size(); ++i){
		if(inList[i].gate() -> getGateID() == n){
			swap(inList[i],inList[inList.size()-1]);
			inList.pop_back();
		}
	}
		for(unsigned i = 0; i < inList.size(); ++i){
		if(inList[i].gate() -> getGateID() == n){
			swap(inList[i],inList[inList.size()-1]);
			inList.pop_back();
		}
	}
}

static unsigned
findInput(CirGate* g,const size_t& n)
{
	vector<AigGateV>::iterator it = (g -> getFanInList()).begin();
	if(CirMgr::readAigGateV(*it) -> getGateID() != n) return 1;
	return 0;
}


static bool
removeOutput(CirGate* g, const size_t& n)
{
	// int idx = binary_search(g -> getFanOutList(),n);
	// if(idx == -1) cerr << "something goes wrong..." << endl;
	vector<AigGateV>::iterator it = (g -> getFanOutList()).begin();
	while(CirMgr::readAigGateV(*it) -> getGateID() != n) ++it;
	if(it == (g -> getFanOutList()).end()) cerr << "something goes wrong..." << endl;
	swap(*it,*(--(g -> getFanOutList()).end()));
	(g -> getFanOutList()).pop_back();
	// (g -> getFanOutList()).erase((g -> getFanOutList()).begin() + idx);
	return true;
}

static GateCase
chkFanIn(CirGate* g)
{
	vector<AigGateV>& list = g -> getFanInList();
	if(list.size() != 2) cerr << "error: FanInList has incorrect size!!" << endl;
	if((list[0]).gate() == (list[1]).gate()){
		if((list[0]).isInv() == (list[1]).isInv())
			return SAME_FANIN;
		else
			return INV_FANIN;
	}
	if(((list[0]).gate()) -> getTypeStr() == "CONST"){
		if((list[0]).isInv())
			return CONST1_FANIN;
		else
			return CONST0_FANIN;
	}
	if(((list[1]).gate()) -> getTypeStr() == "CONST"){
		if((list[1]).isInv())
			return CONST1_FANIN;
		else
			return CONST0_FANIN;
	}
	return NORMAL;
}



/**************************************************/
/*   Public member functions about optimization   */
/**************************************************/
// Remove unused gates
// DFS list should NOT be changed
// UNDEF, float and unused list may be changed
void
CirMgr::sweep()
{
  updateDfsList();
  /*
  for(size_t count = 0; count < _dfsList.size(); ++count){
  	cout << "[" << count << "]" << " ";
  	_dfsList[count] -> printGate();
  }
  */
  for(int count = 0; count <= _size; ++count){
  	if(!_gateList[count]) continue;
  	if(_gateList[count] -> getTypeStr() != "AIG" && _gateList[count] -> getTypeStr() != "UNDEF") continue;
  	if(_gateList[count] -> getRef() == CirGate::getGlobalRef()) continue;
  	cout << "Sweeping: " << _gateList[count] -> getTypeStr() << "(" << count << ")" << " removed..." << endl;
  	for(vector<AigGateV>::iterator it = (_gateList[count] -> getFanOutList()).begin(); it != (_gateList[count] -> getFanOutList()).end(); ++it){
  		removeInput((*it).gate(),count);
  	}
  	for(vector<AigGateV>::iterator it = (_gateList[count] -> getFanInList()).begin(); it != (_gateList[count] -> getFanInList()).end(); ++it){
  		removeOutput((*it).gate(),count);
  	}
  	if(_gateList[count] -> getTypeStr() == "AIG")
  		--_AIGsize;
  	delete _gateList[count];
  	_gateList[count] = 0;
  }
}

// Recursively simplifying from POs;
// _dfsList needs to be reconstructed afterwards
// UNDEF gates may be delete if its fanout becomes empty...
void
CirMgr::optimize()
{
	updateDfsList();
	for(unsigned i = 0; i < _dfsList.size(); ++i){
		if(_dfsList[i] -> getTypeStr() != "AIG") continue;
		GateCase gc = chkFanIn(_dfsList[i]);
		if(gc == NORMAL) continue;
		if(gc == SAME_FANIN) optSameFanIn(_dfsList[i],false);
		if(gc == INV_FANIN) optSameFanIn(_dfsList[i],true);
		if(gc == CONST0_FANIN) optConstFanIn(_dfsList[i],true);
		if(gc == CONST1_FANIN) optConstFanIn(_dfsList[i],false);

		size_t gid = _dfsList[i] -> getGateID();
		delete _dfsList[i];
		_gateList[gid] = _dfsList[i] = 0;
		--_AIGsize;
	}
	for(unsigned i = 1; i <= _size; ++i){
		if(!_gateList[i]) continue;
		_gateList[i] -> updateInfo();
		if(_gateList[i] -> getTypeStr() != "UNDEF") continue;
		if((_gateList[i] -> getFanOutList()).empty()){
			delete _gateList[i];
			_gateList[i] = 0;
		}
	}
	updateDfsList();
}

/***************************************************/
/*   Private member functions about optimization   */
/***************************************************/
void
CirMgr::optSameFanIn(CirGate* g,bool inv)
{
	vector<AigGateV>& outList = g -> getFanOutList();
	vector<AigGateV>& inList = g -> getFanInList();
	AigGateV& ptr = inList[0];
	removeOutput(ptr.gate(),g -> getGateID());
	removeOutput(ptr.gate(),g -> getGateID());
	//same inputs
	if(!inv){
		cout << "Simplifying: " << ptr.gate() -> getGateID() << " merging " << g -> getGateID() << "..." << endl;
		for(unsigned i = 0; i < outList.size(); ++i){
			size_t pos = findInput(outList[i].gate(),g -> getGateID());
			removeInput(outList[i].gate(),g -> getGateID());
			if((ptr.isInv() && outList[i].isInv()) || (!(ptr.isInv()) && !(outList[i].isInv()))){
				AigGateV p1(ptr.gate(),0);
				AigGateV p2(outList[i].gate(),0);
				(outList[i]).gate() -> push_fanInList(p1);
				//swap((outList[i].gate() -> getFanInList())[0],(outList[i].gate() -> getFanInList())[1]);
				(outList[i]).gate() -> updateFanIn(pos);
				(ptr.gate()) -> push_fanOutList(p2);
			}
			else{
				AigGateV p1(ptr.gate(),1);
				AigGateV p2((outList[i]).gate(),1);
				(outList[i]).gate() -> push_fanInList(p1);
				//swap((outList[i].gate() -> getFanInList())[0],(outList[i].gate() -> getFanInList())[1]);
				(outList[i]).gate() -> updateFanIn(pos);
				(ptr.gate()) -> push_fanOutList(p2);
			}
		}
		return;
	}

	//inverted inputs
	cout << "Simplifying: " << _gateList[0] -> getGateID() << " merging " << g -> getGateID() << "..." << endl;
	for(unsigned i = 0; i < outList.size(); ++i){
		size_t pos = findInput(outList[i].gate(),g -> getGateID());
		removeInput(outList[i].gate(),g -> getGateID());
		if((outList[i]).isInv()){
			AigGateV p1(_gateList[0],1);
			AigGateV p2(outList[i].gate(),1);
			(outList[i]).gate() -> push_fanInList(p1);
			//swap((outList[i].gate() -> getFanInList())[0],(outList[i].gate() -> getFanInList())[1]);
			(outList[i]).gate() -> updateFanIn(pos);
			_gateList[0] -> push_fanOutList(p2);
		}
		else{
			AigGateV p1(_gateList[0],0);
			AigGateV p2(outList[i].gate(),0);
			(outList[i]).gate() -> push_fanInList(p1);
			//swap((outList[i].gate() -> getFanInList())[0],(outList[i].gate() -> getFanInList())[1]);
			(outList[i]).gate() -> updateFanIn(pos);
			_gateList[0] -> push_fanOutList(p2);
		}
	}
}

void
CirMgr::optConstFanIn(CirGate* g,bool zero)
{
	vector<AigGateV>& inList = g -> getFanInList();
	vector<AigGateV>& outList = g -> getFanOutList();
	size_t pos = 0;
	if((inList[0].gate()) -> getTypeStr() == "CONST") ++pos;
	for(unsigned i = 0; i < inList.size(); ++i){
		removeOutput(inList[i].gate(),g -> getGateID());
	}
	if(zero) cout << "Simplifying: " << "0" << " merging " << g -> getGateID() << "..." << endl;
	else
		cout << "Simplifying: " << _gateList[pos] -> getGateID() << " merging " << g -> getGateID() << "..." << endl;
	for(unsigned i = 0; i < outList.size(); ++i){
		size_t pivot = findInput(outList[i].gate(),g -> getGateID());
		removeInput(outList[i].gate(),g -> getGateID());
		if(zero){
			if(outList[i].isInv()){
				AigGateV p1(_gateList[0],1);
				AigGateV p2(outList[i].gate(),1);
				outList[i].gate() -> push_fanInList(p1);
				//swap((outList[i].gate() -> getFanInList())[0],(outList[i].gate() -> getFanInList())[1]);
				(outList[i]).gate() -> updateFanIn(pivot);
				_gateList[0] -> push_fanOutList(p2);
			}
			else{
				AigGateV p1(_gateList[0],0);
				AigGateV p2(outList[i].gate(),0);
				outList[i].gate() -> push_fanInList(p1);
				//swap((outList[i].gate() -> getFanInList())[0],(outList[i].gate() -> getFanInList())[1]);
				(outList[i]).gate() -> updateFanIn(pivot);
				_gateList[0] -> push_fanOutList(p2);
			}
		}
		else{
			cout << "Simplifying: " << "1" << " merging " << g -> getGateID() << "..." << endl;
			if((inList[pos].isInv() && outList[i].isInv()) || (!(inList[pos].isInv()) && !(outList[i].isInv()))){
				AigGateV p1(inList[pos].gate(),0);
				AigGateV p2(outList[i].gate(),0);
				outList[i].gate() -> push_fanInList(p1);
				//swap((outList[i].gate() -> getFanInList())[0],(outList[i].gate() -> getFanInList())[1]);
				(outList[i]).gate() -> updateFanIn(pivot);
				inList[pos].gate() -> push_fanOutList(p2);
			}
			else{
				AigGateV p1(inList[pos].gate(),1);
				AigGateV p2(outList[i].gate(),1);
				outList[i].gate() -> push_fanInList(p1);
				//swap((outList[i].gate() -> getFanInList())[0],(outList[i].gate() -> getFanInList())[1]);
				(outList[i]).gate() -> updateFanIn(pivot);
				inList[pos].gate() -> push_fanOutList(p2);
			}
		}
	}
}
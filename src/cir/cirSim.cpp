/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir simulation functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"
#include <climits>
#include <sstream>
#include <bitset>
#include <ctime>

using namespace std;

// TODO: Keep "CirMgr::randimSim()" and "CirMgr::fileSim()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
class SimValue
{
public:
	SimValue(size_t& val): _simValue(val) {}
	size_t operator() () const { if(_simValue < ~_simValue) return _simValue; else return ~_simValue; }
	bool operator== (const SimValue& k) const { return ((_simValue == k._simValue) || (_simValue == ~(k._simValue))); }
private:
	size_t _simValue;
};

/************************************************/
/*   Public member functions about Simulation   */
/************************************************/
void
CirMgr::randomSim()
{
	updateDfsList();
	if(!_fecInit) { initFEC(); _fecInit = true; }
	resetPattern();
	unsigned count = 0;
	unsigned fail = 0;
	unsigned wanted = 160;
	clock_t start,stop;
	start = clock();
	while(fail < wanted){
		++count;
		for(unsigned i = 0; i < _PIsize; ++i){
			size_t pattern = 0;
			pattern = rnGen(INT_MAX);
			pattern = pattern << (size_t)32;
			pattern = pattern | rnGen(INT_MAX);
			_gateList[_PIList[i]] -> setPattern(pattern);
		}
		sim(64);
		int old = _fecGrps.size();
		chkFEC();
		if(_fecGrps.size()){
			cout << "Total #FEC Group = " << _fecGrps.size() << flush;
			cout << char(13) << setw(30) << ' ' << char(13);
		}
		if(_fecGrps.size() == 0) break;
		stop = clock();
		if((double)((stop-start)/CLOCKS_PER_SEC) > 15) break;
		if(abs(old - _fecGrps.size()) < 5 && count > 320) fail++;
		else
			fail = 0;
	}
	cout << (count*64) << " " << "patterns simulated." << endl;

}

void
CirMgr::fileSim(ifstream& patternFile)
{
	string str;
	updateDfsList();

	if(!_fecInit){ initFEC(); }

	for(unsigned i = 0; i < _PIsize; ++i){
		_gateList[_PIList[i]] -> resetPattern();
	}

	unsigned count = 0;

	while(patternFile >> str){
		//patternFile >> str;
		if(str.empty()) continue;
		++count;
		if(count%64 == 1) resetPattern();
		if(str.length() != _PIsize){
			cerr << "Error: Pattern(" << str << ") length(" << str.length() << ") does not match the number of inputs(" << _PIsize << ") in a circuit!!" << endl;
			resetPattern();
			if(!_fecInit)
				resetFecGrps();
			return;
		}
		for(unsigned i = 0; i < _PIsize; ++i){
			if(str[i] != '0' && str[i] != '1'){
				cerr << "Error: Pattern(" << str << ") contains a non-0/1 character('" << str[i] << "')." << endl;
				resetPattern();
				if(!_fecInit)
					resetFecGrps();
				return;
			}
			size_t p = 0;
			if(str[i] == '1') p = (size_t)1;
			p = p << (size_t)((count-1)%64);
			_gateList[_PIList[i]] -> getPattern() = _gateList[_PIList[i]] -> getPattern() | p;
		}
		if(count%64 == 0){
			sim(64);
			chkFEC();
			if(_fecGrps.size()){
				cout << "Total #FEC Group = " << _fecGrps.size() << flush;
				cout << char(13) << setw(30) << ' ' << char(13);
			}
		}
	}
	if(count%64 != 0){
		sim(count%64);
		chkFEC();
		if(_fecGrps.size()){
			cout << "Total #FEC Group = " << _fecGrps.size() << flush;
			cout << char(13) << setw(30) << ' ' << char(13);
		}
	}
	if(count <= 0 && !_fecInit){
		resetFecGrps();
	}
	else
		_fecInit = true;
	cout << count << " " << "patterns simulated." << endl;
}

/*************************************************/
/*   Private member functions about Simulation   */
/*************************************************/
void
CirMgr::sim(unsigned size){
	for(unsigned i = 0; i < _dfsList.size(); ++i){
		_dfsList[i] -> simulation();
	}
	if(_simLog){
		bitset<64>* _inList = new bitset<64>[_PIsize];
		bitset<64>* _outList = new bitset<64>[_POsize];
		for(unsigned i = 0; i < _PIsize; ++i){
			_inList[i] = bitset<64>(_gateList[_PIList[i]] -> getPattern());
		}
		for(unsigned i = 0; i < _POsize; ++i){
			_outList[i] = bitset<64>(_gateList[_POList[i]] -> getPattern());
		}
		for(unsigned i = 0; i < size; ++i){
			for(unsigned j = 0; j < _PIsize; ++j){
				(*_simLog) << _inList[j][i];
			}
			(*_simLog) << " ";
			for(unsigned j = 0; j < _POsize; ++j){
				(*_simLog) << _outList[j][i];
			}
			(*_simLog) << endl;
		}
		delete[] _inList;
		delete[] _outList;
	}
}

void
CirMgr::initFEC()
{
	updateDfsList();
	IdList* _initList = new IdList;
	_initList -> push_back(0);
	for(unsigned i = 0; i < _dfsList.size(); ++i){
		if(_dfsList[i] -> getTypeStr() != "AIG") continue;
		_initList -> push_back(_dfsList[i] -> getGateID());
		_dfsList[i] -> setFecId(1);
	}
	_fecGrps.push_back(_initList);
	for(unsigned i = 0; i <= _size; ++i){
		if(!_gateList[i]) continue;
		if(_gateList[i] -> getTypeStr() != "AIG") continue;
		if(_gateList[i] -> getRef() != CirGate::getGlobalRef())
			_gateList[i] -> setFecId(-1);
	}

}

void
CirMgr::chkFEC()
{
	vector<IdList*> newGrps;
	if(_fecGrps.empty()) return;
	for(unsigned i = 0; i < _fecGrps.size(); ++i){ //for each fecGrp in fecGrps
		HashMap<SimValue,IdList*> fecHash((_fecGrps[i] -> size()));
		for(unsigned j = 0; j < _fecGrps[i] -> size(); ++j){ //for each gate in fecGrp
			IdList* _list = 0;
			if(!_gateList[(*(_fecGrps[i]))[j]]) continue;
			size_t& _p = _gateList[(*(_fecGrps[i]))[j]] -> getPattern();
			SimValue val(_p);
			fecHash.query(val,_list);
			if(_list){
				_list -> push_back((*(_fecGrps[i]))[j]);
			}
			else{
				_list = new IdList;
				_list -> push_back((*(_fecGrps[i]))[j]);
				fecHash.insert(val,_list);
			}
		}
		//check groups in hash
		size_t count = 0;
		for(HashMap<SimValue,IdList*>::iterator it = fecHash.begin(); it != fecHash.end(); ++it){
			count++;
			if((*it).second -> size() > 1){
				newGrps.push_back((*it).second);
			}
			else{
				_gateList[*((*it).second -> begin())] -> setFecId(-1);
				delete (*it).second;
			}
		}
	}
	_fecGrps.swap(newGrps);
	for(unsigned i = 0; i < _fecGrps.size(); ++i){
		for(unsigned j = 0; j < _fecGrps[i] -> size(); ++j){
			_gateList[(*_fecGrps[i])[j]] -> setFecId(i);
		}
	}
	for(unsigned i = 0; i < newGrps.size(); ++i){
		if(newGrps[i]) delete newGrps[i];
	}
}

void
CirMgr::resetFecGrps()
{
	for(unsigned i = 0; i < _fecGrps.size(); ++i){
		if(_fecGrps[i]) delete _fecGrps[i];
	}
	_fecGrps.clear();
}

void
CirMgr::resetPattern()
{
	for(unsigned i = 0; i <= _size; ++i){
		if(_gateList[i]) _gateList[i] -> resetPattern();
	}
}
/****************************************************************************
  FileName     [ cirGate.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define class CirAigGate member functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdarg.h>
#include <cassert>
#include <bitset>
#include <algorithm>
#include "cirGate.h"
#include "cirMgr.h"
#include "util.h"

using namespace std;

// TODO: Keep "CirGate::reportGate()", "CirGate::reportFanin()" and
//       "CirGate::reportFanout()" for cir cmds. Feel free to define
//       your own variables and functions.

extern CirMgr *cirMgr;

/**************************************/
/*   class CirGate member functions   */
/**************************************/
size_t CirGate::_globalRef = 0;

void
CirGate::reportGate() const
{
	stringstream ss;
	string gateInfo = "";
	ss << getTypeStr() << "(" << _id << ")";
	if(_name.length())
		ss << "\"" << _name << "\"";
	ss << ", line " << _line;
	gateInfo = ss.str();
	string value = getSimValue();
	cout << "================================================================================" << endl;
	cout << "="; cout << " ";
	cout << left << setw(47) << gateInfo;
	cout << endl;
	cout << "="; cout << " ";
	cout << left << setw(47) << "FECs:" + getFECpair(cirMgr);
	cout << endl;
	cout << "="; cout << " ";
	cout << left << setw(47) << "Value: " + value;
	cout << endl;
	cout << "================================================================================" << endl; 
}

void
CirGate::reportFanin(int level) const
{
   assert (level >= 0);
   CirGate::IncGlobalRef();
   cout << getTypeStr() << " " << _id << endl;
   if(level == 0) return;
   _ref = _globalRef;
   for(vector<AigGateV>::const_iterator it = _fanInList.begin(); it != _fanInList.end(); ++it){
   	reportFanin(1,level - 1,*it);
   }
}

void
CirGate::reportFanout(int level) const
{
   assert (level >= 0);
   CirGate::IncGlobalRef();
   cout << getTypeStr() << " " << _id << endl;
   if(level == 0) return;
   _ref = _globalRef;
   for(vector<AigGateV>::const_iterator it = _fanOutList.begin(); it != _fanOutList.end(); ++it){
   	reportFanout(1,level - 1,*it);
   }
}

void
CirGate::reportFanin(int space,int level,AigGateV ptr) const
{
	for(size_t count = 0; count < space; ++count) cout << "  ";
	if(ptr.isInv()) cout << "!";
	cout << ptr.gate() -> getTypeStr() << " " << ptr.gate() -> getGateID();
	if(ptr.gate() -> getTypeStr() == "PI" || ptr.gate() -> getTypeStr() == "CONST" || ptr.gate() -> getTypeStr() == "UNDEF") { cout << endl; return; }
	if(level == 0) { cout << endl; return; }
	if(ptr.gate() -> _ref == _globalRef) { cout << " " << "(*)" << endl; return; }
	if(((ptr.gate()) -> _fanInList).empty()) { cout << endl; return; }
	ptr.gate() -> _ref = _globalRef;
	cout << endl;
	for(vector<AigGateV>::const_iterator it = (ptr.gate()) -> _fanInList.begin(); it != (ptr.gate()) -> _fanInList.end(); ++it){
		reportFanin(space + 1,level - 1, *it);
	}
}

void
CirGate::reportFanout(int space,int level,AigGateV ptr) const
{
	for(size_t count = 0; count < space; ++count) cout << "  ";
	if(ptr.isInv()) cout << "!";
	cout << ptr.gate() -> getTypeStr() << " " << ptr.gate() -> getGateID();
	if(ptr.gate() -> getTypeStr() == "PO") { cout << endl; return; }
	if(level == 0) { cout << endl; return; }
	if(ptr.gate() -> _ref == _globalRef) { cout << " " << "(*)" << endl; return; }
	if(((ptr.gate()) -> _fanOutList).empty()) { cout << endl; return; }
	ptr.gate() -> _ref = _globalRef;
	cout << endl;
	for(vector<AigGateV>::const_iterator it = (ptr.gate()) -> _fanOutList.begin(); it != (ptr.gate()) -> _fanOutList.end(); ++it){
		reportFanout(space + 1,level - 1, *it);
	}
}

void
CirGate::merge(CirGate* g)
{

	CirGate& p1 = *((_fanInList[0]).gate());
	for(vector<AigGateV>::iterator it = (p1._fanOutList).begin(); it != (p1._fanOutList).end(); ++it){
		if((*it).gate() == g){
			swap(*it,*(--(p1._fanOutList).end()));
			(p1._fanOutList).pop_back();
			break;
		}
	}
	for(vector<AigGateV>::iterator it = (p1._fanOutList).begin(); it != (p1._fanOutList).end(); ++it){
		if((*it).gate() == g){
			swap(*it,*(--(p1._fanOutList).end()));
			(p1._fanOutList).pop_back();
			break;
		}
	}
	CirGate& p2 = *((_fanInList[1]).gate());
	for(vector<AigGateV>::iterator it = (p2._fanOutList).begin(); it != (p2._fanOutList).end(); ++it){
		if((*it).gate() == g){
			swap(*it,*(--(p2._fanOutList).end()));
			(p2._fanOutList).pop_back();
			break;
		}
	}
	for(vector<AigGateV>::iterator it = (p2._fanOutList).begin(); it != (p2._fanOutList).end(); ++it){
		if((*it).gate() == g){
			swap(*it,*(--(p2._fanOutList).end()));
			(p2._fanOutList).pop_back();
			break;
		}
	}

	for(vector<AigGateV>::iterator it = (g -> _fanOutList).begin(); it != (g -> _fanOutList).end(); ++it){
		_fanOutList.push_back(*it);
		if(((*it).gate()) -> getTypeStr() == "PO"){
			POGate* ptr = (POGate*)((*it).gate());
			if((*it).isInv())
				ptr -> setInfo(2*_id + 1);
			else
				ptr -> setInfo(2*_id);
		}
		vector<AigGateV>::iterator v = ((*it).gate() -> _fanInList).begin();
		if((*v).gate() != g) ++v;
		(*v) = AigGateV(this,(*v).isInv());

	}
	(g -> _fanInList).clear();
	(g -> _fanOutList).clear();
	delete g;
}

string
CirGate::getSimValue() const
{
	bitset<64> simInfo(_pattern);
	string tmp = simInfo.to_string();
	string str;
	for(unsigned i = 0; i < 64;){
		str += tmp[i];
		if((++i)%8 == 0 && i != 64)
			str += "_";
	}
	return str;

}

string
AigGate::getFECpair(const CirMgr* mgr) const
{
	if(_fecId == -1) return "";
	if(mgr -> getFECsize() == 0) return "";
	vector<unsigned> list = mgr -> getFEClist(_fecId);
	sort(list.begin(),list.end());
	string str;
	stringstream ss;
	for(unsigned i = 0; i < list.size(); ++i){
		if(list[i] == _id) continue;
		ss << " ";
		if((mgr -> getGate(list[i]) -> getPattern()) == ~(_pattern))
			ss << "!";
		ss << list[i];
	}
	str = ss.str();
	return str;
}

string
Const0Gate::getFECpair(const CirMgr* mgr) const
{
	if(_fecId == -1) return "";
	if(mgr -> getFECsize() == 0) return "";
	vector<unsigned> list = mgr -> getFEClist(_fecId);
	sort(list.begin(),list.end());
	string str;
	stringstream ss;
	for(unsigned i = 0; i < list.size(); ++i){
		if(list[i] == _id) continue;
		ss << " ";
		if((mgr -> getGate(list[i]) -> getPattern()) == ~(_pattern))
			ss << "!";
		ss << list[i];
	}
	str = ss.str();
	return str;
}

void
CirGate::fraigMerge(CirGate* g,bool inv)
{
	CirGate* p1 = (g -> _fanInList)[0].gate();
	CirGate* p2 = (g -> _fanInList)[1].gate();
	
	vector<AigGateV>& p1List = p1 -> _fanOutList;
	vector<AigGateV>& p2List = p2 -> _fanOutList;
	for(unsigned i = 0; i < p1List.size(); ++i){
		if(p1List[i].gate() == g){
			swap(p1List[i],p1List[p1List.size()-1]);
			p1List.pop_back();
		}
	}
	for(unsigned i = 0; i < p1List.size(); ++i){
		if(p1List[i].gate() == g){
			swap(p1List[i],p1List[p1List.size()-1]);
			p1List.pop_back();
		}
	}
	for(unsigned i = 0; i < p2List.size(); ++i){
		if(p2List[i].gate() == g){
			swap(p2List[i],p2List[p2List.size()-1]);
			p2List.pop_back();
		}
	}
	for(unsigned i = 0; i < p2List.size(); ++i){
		if(p2List[i].gate() == g){
			swap(p2List[i],p2List[p2List.size()-1]);
			p2List.pop_back();
		}
	}
	

	vector<AigGateV>& outList = g -> _fanOutList;
	for(unsigned i = 0; i < outList.size(); ++i){
		vector<AigGateV>& ls = outList[i].gate() -> _fanInList;
		for(unsigned j = 0; j < ls.size(); ++j){
			if(ls[j].gate() == g){
				if((inv && !ls[j].isInv()) || (!inv && ls[j].isInv())){
					ls[j] = AigGateV(this,1);
					_fanOutList.push_back(AigGateV(outList[i].gate(),1));
				}
				else{
					ls[j] = AigGateV(this,0);
					_fanOutList.push_back(AigGateV(outList[i].gate(),0));
				}
			}
		}
	}
	outList.clear();

}
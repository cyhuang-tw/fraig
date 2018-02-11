/****************************************************************************
  FileName     [ myHashSet.h ]
  PackageName  [ util ]
  Synopsis     [ Define HashSet ADT ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2014-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef MY_HASH_SET_H
#define MY_HASH_SET_H

#include <vector>

using namespace std;

//---------------------
// Define HashSet class
//---------------------
// To use HashSet ADT,
// the class "Data" should at least overload the "()" and "==" operators.
//
// "operator ()" is to generate the hash key (size_t)
// that will be % by _numBuckets to get the bucket number.
// ==> See "bucketNum()"
//
// "operator ==" is to check whether there has already been
// an equivalent "Data" object in the HashSet.
// Note that HashSet does not allow equivalent nodes to be inserted
//
template <class Data>
class HashSet
{
public:
   HashSet(size_t b = 0) : _numBuckets(0), _buckets(0) { if (b != 0) init(b); }
   ~HashSet() { reset(); }

   // TODO: implement the HashSet<Data>::iterator
   // o An iterator should be able to go through all the valid Data
   //   in the Hash
   // o Functions to be implemented:
   //   - constructor(s), destructor
   //   - operator '*': return the HashNode
   //   - ++/--iterator, iterator++/--
   //   - operators '=', '==', !="
   //
   class iterator
   {
      friend class HashSet<Data>;

   public:
      iterator() {};
      const Data& operator * () const { return _buckets[_pos][_idx]; }
      iterator& operator ++ () {
         if(++_idx != _buckets[_pos].size()) return *this;
         if(_pos == _numBuckets - 1) return *this;
         for(size_t count = _pos + 1; count < _numBuckets; ++count){
            if(!_buckets[count].empty()){
               _pos = count;
               _idx = 0;
               return *this;
            }
         }
         _pos = _numBuckets - 1; _idx = _buckets[_numBuckets - 1].size();
         return *this;
      }
      iterator& operator -- () {
         if(_idx != 0) { --_idx; return *this; }
         for(size_t count = _pos - 1; count >= 0; --count){
            if(!_buckets[count].empty()){
               _pos = count;
               _idx = _buckets[count].size() - 1;
               return *this;
            }
         }
         _pos = 0; _idx = 0;
         return *this;
      }
      iterator operator ++ (int) { iterator i = *this; ++(*this); return i; }
      iterator operator -- (int) { iterator i = *this; --(*this); return i; }

      bool operator == (const iterator& i) const { return ((_buckets == i._buckets) && (_numBuckets == i._numBuckets) && (_pos == i._pos) && (_idx == i._idx)); }
      bool operator != (const iterator& i) const { return !(*this == i); }
      iterator& operator = (const iterator& i) { _buckets = i._buckets; _numBuckets = i._numBuckets; _pos = i._pos; _idx = i._idx; return *(this); }

   private:
      iterator(vector<Data>* b, size_t n, size_t p, size_t i) : _buckets(b), _numBuckets(n), _pos(p), _idx(i) {}
      vector<Data>* _buckets;
      size_t _numBuckets;
      size_t _pos;
      size_t _idx;
   };

   void init(size_t b) { _numBuckets = b; _buckets = new vector<Data>[b]; }
   void reset() {
      _numBuckets = 0;
      if (_buckets) { delete [] _buckets; _buckets = 0; }
   }
   void clear() {
      for (size_t i = 0; i < _numBuckets; ++i) _buckets[i].clear();
   }
   size_t numBuckets() const { return _numBuckets; }

   vector<Data>& operator [] (size_t i) { return _buckets[i]; }
   const vector<Data>& operator [](size_t i) const { return _buckets[i]; }

   // TODO: implement these functions
   //
   // Point to the first valid data
   iterator begin() const {
      if(empty()) return end();
      for(size_t count = 0; count < _numBuckets; ++count){
         if(!_buckets[count].empty()){
            iterator i(_buckets, _numBuckets, count, 0);
            return i;
         }
      }
   }
   // Pass the end
   iterator end() const { iterator i(_buckets, _numBuckets, _numBuckets-1, _buckets[_numBuckets-1].size()); return i; }
   // return true if no valid data
   bool empty() const {
      for(size_t count = 0; count < _numBuckets; ++count){
         if(!_buckets[count].empty()) return false;
      }
      return true;
   }
   // number of valid data
   size_t size() const {
      size_t s = 0;
      for(size_t count = 0; count < _numBuckets; ++count){
         s += _buckets[count].size();
      }
      return s;
   }

   // check if d is in the hash...
   // if yes, return true;
   // else return false;
   bool check(const Data& d) const {
      for(size_t count = 0; count < _buckets[bucketNum(d)].size(); ++count){
         if(_buckets[bucketNum(d)][count] == d) return true;
      }
      return false;
   }

   // query if d is in the hash...
   // if yes, replace d with the data in the hash and return true;
   // else return false;
   bool query(Data& d) const {
      for(size_t count = 0; count < _buckets[bucketNum(d)].size(); ++count){
         if(_buckets[bucketNum(d)][count] == d) { d = _buckets[bucketNum(d)][count]; return true; }
      }
      return false;
   }

   // update the entry in hash that is equal to d (i.e. == return true)
   // if found, update that entry with d and return true;
   // else insert d into hash as a new entry and return false;
   bool update(const Data& d) {
      Data _d = d;
      for(size_t count = 0; count < _buckets[bucketNum(d)].size(); ++count){
         if(_buckets[bucketNum(d)][count] == d) { _buckets[bucketNum(d)][count] = d; return true; }
      }
      _buckets[bucketNum(d)].push_back(d);
      return false;
   }

   // return true if inserted successfully (i.e. d is not in the hash)
   // return false is d is already in the hash ==> will not insert
   bool insert(const Data& d) {
      Data _d = d;
      if(query(_d)) return false;
      _buckets[bucketNum(d)].push_back(d);
      return true;
   }

   // return true if removed successfully (i.e. d is in the hash)
   // return fasle otherwise (i.e. nothing is removed)
   bool remove(const Data& d) {
      size_t idx = -1;
      for(size_t count = 0; count < _buckets[bucketNum(d)].size(); ++count){
         if(_buckets[bucketNum(d)][count] == d) { idx = count; break; }
      }
      if(idx == -1) return false;
      swap(_buckets[bucketNum(d)][idx],_buckets[bucketNum(d)][_buckets[bucketNum(d)].size()-1]);
      _buckets[bucketNum(d)].pop_back();
      return true;
   }

private:
   // Do not add any extra data member
   size_t            _numBuckets;
   vector<Data>*     _buckets;

   size_t bucketNum(const Data& d) const {
      return (d() % _numBuckets); }
};

#endif // MY_HASH_SET_H

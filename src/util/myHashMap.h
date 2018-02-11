/****************************************************************************
  FileName     [ myHashMap.h ]
  PackageName  [ util ]
  Synopsis     [ Define HashMap and Cache ADT ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2009-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef MY_HASH_MAP_H
#define MY_HASH_MAP_H

#include <vector>

using namespace std;

// TODO: (Optionally) Implement your own HashMap and Cache classes.

//-----------------------
// Define HashMap classes
//-----------------------
// To use HashMap ADT, you should define your own HashKey class.
// It should at least overload the "()" and "==" operators.
//
// class HashKey
// {
// public:
//    HashKey() {}
// 
//    size_t operator() () const { return 0; }
// 
//    bool operator == (const HashKey& k) const { return true; }
// 
// private:
// };
//
template <class HashKey, class HashData>
class HashMap
{
typedef pair<HashKey, HashData> HashNode;

public:
   HashMap(size_t b=0) : _numBuckets(0), _buckets(0) { if (b != 0) init(b); }
   ~HashMap() { reset(); }

   // [Optional] TODO: implement the HashMap<HashKey, HashData>::iterator
   // o An iterator should be able to go through all the valid HashNodes
   //   in the HashMap
   // o Functions to be implemented:
   //   - constructor(s), destructor
   //   - operator '*': return the HashNode
   //   - ++/--iterator, iterator++/--
   //   - operators '=', '==', !="
   //
   class iterator
   {
      friend class HashMap<HashKey, HashData>;

   public:
      iterator() {};
      const HashNode& operator * () const { return (_buckets[_pos][_idx]); }
      iterator& operator ++ () {
         if(++_idx != _buckets[_pos].size()) return *this;
         if(_pos == _numBuckets - 1) return *this;
         for(unsigned count = _pos + 1; count < _numBuckets; ++count){
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
      iterator(vector<HashNode>* b, size_t n, size_t p, size_t i) : _buckets(b), _numBuckets(n), _pos(p), _idx(i) {}
      vector<HashNode>* _buckets;
      size_t _numBuckets;
      size_t _pos;
      size_t _idx;
   };

   void init(size_t b) {
      reset(); _numBuckets = b; _buckets = new vector<HashNode>[b]; }
   void reset() {
      _numBuckets = 0;
      if (_buckets) { delete [] _buckets; _buckets = 0; }
   }
   void clear() {
      for (size_t i = 0; i < _numBuckets; ++i) _buckets[i].clear();
   }
   size_t numBuckets() const { return _numBuckets; }

   vector<HashNode>& operator [] (size_t i) { return _buckets[i]; }
   const vector<HashNode>& operator [](size_t i) const { return _buckets[i]; }

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
      for(size_t count = 0; count < _numBuckets; ++count)
         s += _buckets[count].size();
      return s;
   }

   // check if k is in the hash...
   // if yes, return true;
   // else return false;
   bool check(const HashKey& k) const {
      for(size_t count = 0; count < _buckets[bucketNum(k)].size(); ++count){
         if((_buckets[bucketNum(k)][count]).first == k)
            return true;
      }
      return false;
   }

   // query if k is in the hash...
   // if yes, replace d with the data in the hash and return true;
   // else return false;
   bool query(const HashKey& k, HashData& d) const {
      for(size_t count = 0; count < _buckets[bucketNum(k)].size(); ++count){
         if((_buckets[bucketNum(k)][count]).first == k){
            d = _buckets[bucketNum(k)][count].second;
            return true;
         }
      }
      return false;
   }

   // update the entry in hash that is equal to k (i.e. == return true)
   // if found, update that entry with d and return true;
   // else insert d into hash as a new entry and return false;
   bool update(const HashKey& k, HashData& d) {
      HashData _d = d;
      for(size_t count = 0; count < _buckets[bucketNum(k)].size(); ++count){
         if((_buckets[bucketNum(k)][count]).first == k){
            _buckets[bucketNum(k)][count].second = d;
            return true;
         }
      }
      _buckets[bucketNum(d)].push_back(HashNode(k,d));
      return false;
   }

   // return true if inserted d successfully (i.e. k is not in the hash)
   // return false is k is already in the hash ==> will not insert
   bool insert(const HashKey& k, const HashData& d) {
      for(size_t count = 0; count < _buckets[bucketNum(k)].size(); ++count){
         if((_buckets[bucketNum(k)][count]).first == k)
            return false;
      }
      _buckets[bucketNum(k)].push_back(HashNode(k,d));
      return true;
   }

   // return true if removed successfully (i.e. k is in the hash)
   // return fasle otherwise (i.e. nothing is removed)
   bool remove(const HashKey& k) {
      for(size_t count = 0; count < _buckets[bucketNum(k)].size(); ++count){
         if((_buckets[bucketNum(k)][count]).first == k){
            swap(_buckets[bucketNum(k)][count],_buckets[bucketNum(k)][_buckets[bucketNum(k)].size()-1]);
            _buckets[bucketNum(k)].pop_back();
            return true;
         }
      }
      return false;
   }

private:
   // Do not add any extra data member
   size_t                   _numBuckets;
   vector<HashNode>*        _buckets;

   size_t bucketNum(const HashKey& k) const {
      return (k() % _numBuckets); }

};


//---------------------
// Define Cache classes
//---------------------
// To use Cache ADT, you should define your own HashKey class.
// It should at least overload the "()" and "==" operators.
//
// class CacheKey
// {
// public:
//    CacheKey() {}
//    
//    size_t operator() () const { return 0; }
//   
//    bool operator == (const CacheKey&) const { return true; }
//       
// private:
// }; 
// 
template <class CacheKey, class CacheData>
class Cache
{
typedef pair<CacheKey, CacheData> CacheNode;

public:
   Cache() : _size(0), _cache(0) {}
   Cache(size_t s) : _size(0), _cache(0) { init(s); }
   ~Cache() { reset(); }

   // NO NEED to implement Cache::iterator class

   // TODO: implement these functions
   //
   // Initialize _cache with size s
   void init(size_t s) { reset(); _size = s; _cache = new CacheNode[s]; }
   void reset() {  _size = 0; if (_cache) { delete [] _cache; _cache = 0; } }

   size_t size() const { return _size; }

   CacheNode& operator [] (size_t i) { return _cache[i]; }
   const CacheNode& operator [](size_t i) const { return _cache[i]; }

   // return false if cache miss
   bool read(const CacheKey& k, CacheData& d) const {
      size_t i = k() % _size;
      if (k == _cache[i].first) {
         d = _cache[i].second;
         return true;
      }
      return false;
   }
   // If k is already in the Cache, overwrite the CacheData
   void write(const CacheKey& k, const CacheData& d) {
      size_t i = k() % _size;
      _cache[i].first = k;
      _cache[i].second = d;
   }

private:
   // Do not add any extra data member
   size_t         _size;
   CacheNode*     _cache;
};


#endif // MY_HASH_H

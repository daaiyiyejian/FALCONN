#ifndef __DATA_STORAGE_H__
#define __DATA_STORAGE_H__

#include <cstdint>
#include <vector>
#include <utility>

#include "../falconn_global.h"
#include "prefetchers.h"

// TODO: add tests

namespace falconn {
namespace core {

class DataStorageError : public FalconnError {
 public:
  DataStorageError(const char* msg) : FalconnError(msg) {}
};

// A class for providing access to points stored in a std::vector.
// Using a DataStorage class in NearestNeighborQuery (as opposed to a
// std::vector) allows us to use the same implementation of
// NearestNeighborQuery for points stored in std::vectors, arbitrary memory
// locations (keys are pointers), and contiguous data in an Eigen matrix.
// TODO: implement EigenMatrixDataStorage and PointerDataStorage.
template<
typename PointType,
typename KeyType = int32_t>
class ArrayDataStorage {
 public:
  class FullSequenceIterator {
   public:
    FullSequenceIterator(const ArrayDataStorage& parent) : parent_(&parent) {
      if (parent_->data_.size() == 0) {
        parent_ = nullptr;
        index_ = -1;
      } else {
        index_ = 0;
        // TODO: try different prefetching steps
        prefetcher_.prefetch(parent_->data_, 0);
        if (parent_->data_.size() >= 2) {
          prefetcher_.prefetch(parent_->data_, 1);

          if (parent_->data_.size() >= 3) {
            prefetcher_.prefetch(parent_->data_, 2);
          }
        }
      }
    }

    FullSequenceIterator() {}

    const PointType& get_point() const {
      return parent_->data_[index_];
    }

    const KeyType& get_key() const {
      return index_;
    }

    bool is_valid() const {
      return parent_ != nullptr;
    }
    
    void operator ++ () {
      if (index_ >= 0
          && index_ + 1 < static_cast<int_fast64_t>(parent_->data_.size())) {
        index_ += 1;
        if (index_ + 2 < static_cast<int_fast64_t>(parent_->data_.size())) {
          // TODO: try different prefetching steps
          prefetcher_.prefetch(parent_->data_, index_ + 2);
        }
      } else {
        if (index_ == -1) {
          throw DataStorageError("Advancing invalid FullSequenceIterator.");
        } else {
          parent_ = nullptr;
          index_ = -1;
        }
      }
    }

   private:
    int_fast64_t index_ = -1;
    const ArrayDataStorage* parent_ = nullptr;
    StdVectorPrefetcher<PointType> prefetcher_;
  };


  class SubsequenceIterator {
   public:
    SubsequenceIterator(const std::vector<KeyType>& keys,
                        const ArrayDataStorage& parent)
        : keys_(&keys), parent_(&parent) {
      if (keys_->size() == 0) {
        keys_ = nullptr;
        parent_ = nullptr;
        index_ = -1;
      } else {
        index_ = 0;
        // TODO: try different prefetching steps
        prefetcher_.prefetch(parent_->data_, (*keys_)[0]);
        if (keys_->size() >= 2) {
          prefetcher_.prefetch(parent_->data_, (*keys_)[1]);

          if (keys_->size() >= 3) {
            prefetcher_.prefetch(parent_->data_, (*keys_)[2]);
          }
        }
      }
    }

    SubsequenceIterator() {}

    // Not using STL-style iterators for now because the custom format below
    // makes more sense in NearestNeighborQuery.
    /*const PointType& operator * () const {
      return parent->data[(*keys)[index_]];
    }*/

    const PointType& get_point() const {
      return parent_->data_[(*keys_)[index_]];
    }

    const KeyType& get_key() const {
      return (*keys_)[index_];
    }

    bool is_valid() const {
      return parent_ != nullptr;
    }

    /*bool operator != (const Iterator& rhs) const {
      if (parent_ != rhs.parent_) {
          return true;
      } else if (keys_ != rhs.keys_) {
          return true;
      } else if (index_ != rhs.index_) {
        return true;
      }
      return false;
    }*/

    void operator ++ () {
      if (index_ >= 0
          && index_ + 1 < static_cast<int_fast64_t>(keys_->size())) {
        index_ += 1;
        if (index_ + 2 < static_cast<int_fast64_t>(keys_->size())) {
          // TODO: try different prefetching steps
          prefetcher_.prefetch(parent_->data_, (*keys_)[index_ + 2]);
        }
      } else {
        if (index_ == -1) {
          throw DataStorageError("Advancing invalid SubsequenceIterator.");
        }
        keys_ = nullptr;
        parent_ = nullptr;
        index_ = -1;
      }
    }

   private:
    int_fast64_t index_ = -1;
    const std::vector<KeyType>* keys_ = nullptr;
    const ArrayDataStorage* parent_ = nullptr;
    StdVectorPrefetcher<PointType> prefetcher_;
  };

  ArrayDataStorage(const std::vector<PointType>& data) : data_(data) {}

  /*std::pair<Iterator, Iterator> get_sequence(const std::vector<KeyType>& keys)
      const {
    return std::make_pair(Iterator(keys, *this), Iterator());
  }*/
  /*const PointType& operator [] (int_fast64_t index) const {
    return data_[index];
  }*/

  int_fast64_t size() const {
    return data_.size();
  }

  SubsequenceIterator get_subsequence(const std::vector<KeyType>& keys) const {
    return SubsequenceIterator(keys, *this);
  }

  FullSequenceIterator get_full_sequence() const {
    return FullSequenceIterator(*this);
  }
 
 private:
  const std::vector<PointType>& data_;
};

}  // namespace core
}  // namespace falconn

#endif
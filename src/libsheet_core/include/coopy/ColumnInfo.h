#ifndef COOPY_COLUMNINFO
#define COOPY_COLUMNINFO

#include <string>

namespace coopy {
  namespace store {
    class ColumnInfo;
  }
}

class coopy::store::ColumnInfo {
private:
  int index;
public:
  ColumnInfo(int index=-1) : index(index) {}

  virtual bool hasName() const { return false; }

  virtual std::string getName() const;
};

#endif

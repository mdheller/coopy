#ifndef COOPY_INTSHEET
#define COOPY_INTSHEET

#include <coopy/TypedSheet.h>

#include <stdio.h>

namespace coopy {
  namespace store {
    class IntSheet;
  }
}

class coopy::store::IntSheet : public TypedSheet<int> {
public:
  virtual std::string cellString(int x, int y) const {
    char buf[256];
    snprintf(buf,sizeof(buf),"%d",cell(x,y));
    return buf;
  }
};


#endif

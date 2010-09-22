#ifndef COOPY_TEXTBOOK
#define COOPY_TEXTBOOK

#include <coopy/DataBook.h>
#include <coopy/TextSheet.h>

namespace coopy {
  namespace store {
    class TextBook;
  }
}

class coopy::store::TextBook : public DataBook {
public:
  virtual std::vector<std::string> getNames() = 0;

  virtual bool readSheet(const std::string& name, TextSheet& sheet) = 0;
};

#endif

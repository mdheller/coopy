#ifndef COOPY_MERGEOUTPUTCSVDIFF
#define COOPY_MERGEOUTPUTCSVDIFF

#include <coopy/MergeOutput.h>
#include <coopy/CsvSheet.h>

#include <map>

namespace coopy {
  namespace cmp {
    class MergeOutputCsvDiff;
    class MergeOutputCsvDiffV0p2;
    typedef MergeOutputCsvDiffV0p2 MergeOutputCsvDiffStable;
    typedef MergeOutputCsvDiff MergeOutputCsvDiffUnstable;
  }
}

class coopy::cmp::MergeOutputCsvDiff : public MergeOutput {
public:
  std::string currentSheetName;
  std::string pendingSheetName;
  coopy::store::CsvSheet result;
  std::vector<std::string> ops;
  std::vector<std::string> nops;
  std::map<std::string,bool> activeColumn;
  std::map<std::string,bool> showForSelect;
  std::map<std::string,bool> showForDescribe;
  std::map<std::string,bool> prevSelect;
  std::map<std::string,bool> prevDescribe;
  bool constantColumns;
  std::vector<std::string> columns;
  bool showedColumns;

  MergeOutputCsvDiff();

  virtual bool wantDiff() { return true; }

  virtual bool changeColumn(const OrderChange& change);
  virtual bool changeRow(const RowChange& change);

  bool operateRow(const RowChange& change, const char *tag);
  bool updateRow(const RowChange& change, const char *tag, bool select, 
		 bool update, bool practice);
  bool describeRow(const RowChange& change, const char *tag);

  virtual bool mergeStart();
  virtual bool mergeClear();
  virtual bool mergeDone();

  virtual bool mergeAllDone();

  virtual bool changeName(const NameChange& change);

  const coopy::store::CsvSheet& get() { return result; }

  virtual bool setSheet(const char *name);

  bool clearThroat();
};

class coopy::cmp::MergeOutputCsvDiffV0p2 : public MergeOutput {
public:
  coopy::store::CsvSheet result;

  MergeOutputCsvDiffV0p2();

  virtual bool wantDiff() { return true; }

  virtual bool changeColumn(const OrderChange& change);
  virtual bool changeRow(const RowChange& change);

  bool selectRow(const RowChange& change, const char *tag);
  bool describeRow(const RowChange& change, const char *tag);

  virtual bool mergeAllDone();

  virtual bool declareNames(const std::vector<std::string>& names, bool final);

  const coopy::store::CsvSheet& get() { return result; }

};

#endif

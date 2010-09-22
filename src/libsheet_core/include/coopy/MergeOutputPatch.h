#ifndef COOPY_MERGEOUTPUTPATCH
#define COOPY_MERGEOUTPUTPATCH

#include <coopy/MergeOutput.h>
#include <coopy/CsvSheet.h>

class MergeOutputPatch : public MergeOutput {
public:
  coopy::store::CsvSheet result;

  virtual bool wantDiff() { return true; }

  virtual bool addRow(const char *tag,
		      const std::vector<std::string>& row,
		      const std::string& blank);

  const coopy::store::CsvSheet& get() { return result; }

};

#endif

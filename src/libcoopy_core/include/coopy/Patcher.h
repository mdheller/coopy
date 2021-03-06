#ifndef COOPY_PATCHER
#define COOPY_PATCHER

#include <string>
#include <vector>
#include <map>

#include <coopy/SheetCell.h>
#include <coopy/CompareFlags.h>
#include <coopy/SheetCell.h>
#include <coopy/PolySheet.h>
#include <coopy/TextBook.h>
#include <coopy/NameSniffer.h>

namespace coopy {
  namespace cmp {
    class ConfigChange;
    class OrderChange;
    class RowChangeContext;
    class RowChange;
    class NameChange;
    class TableField;
    class PoolChange;
    class LinkDeclare;
    class Patcher;

    enum {
      ORDER_CHANGE_NONE,
      ORDER_CHANGE_DELETE,
      ORDER_CHANGE_INSERT,
      ORDER_CHANGE_MOVE,
      ORDER_CHANGE_RENAME,
    };
    
    enum {
      ROW_CHANGE_NONE,
      ROW_CHANGE_DELETE,
      ROW_CHANGE_INSERT,
      ROW_CHANGE_UPDATE,
      ROW_CHANGE_CONTEXT,
      ROW_CHANGE_MOVE,
    };

    enum {
      NAME_CHANGE_NONE,
      NAME_CHANGE_DECLARE,
      NAME_CHANGE_SELECT,
    };

    enum {
      LINK_DECLARE_NONE,
      LINK_DECLARE_LOCAL,
      LINK_DECLARE_REMOTE,
      LINK_DECLARE_MERGE,
    };
  }
}

/**
 * Description of a general difference style and options.
 */
class coopy::cmp::ConfigChange {
public:
  bool ordered;
  bool complete;
  bool trustNames;

  ConfigChange() {
    ordered = true;
    complete = true;
    trustNames = false;
  }
};

/**
 * Name of a specific field within a specific table.
 */
class coopy::cmp::TableField {
public:
  std::string tableName;
  std::string fieldName;
  bool invented;

  TableField() {
    invented = false;
  }

  TableField(const std::string& tableName,
	     const std::string& fieldName,
	     bool invented = false) : tableName(tableName),
    fieldName(fieldName),
    invented(invented) {
    }

  std::string toString() const {
    return tableName + ":" + fieldName + (invented?"(create)":"");
  }
};

/**
 * Description of a needed pool for cross-referenced columns.
 */
class coopy::cmp::PoolChange {
public:
  std::string poolName;
  std::string tableName;
  std::vector<coopy::cmp::TableField> pool;
};

/**
 * Description of a column order change.
 */
class coopy::cmp::OrderChange {
public:
  // local-to-global mapping before and after change
  std::vector<int> indicesBefore;
  std::vector<std::string> namesBefore;
  std::vector<int> indicesAfter;
  std::vector<std::string> namesAfter;

  int subject; // subject in local (prior-to-action) coords
  int object; // object in local (prior-to-action) coords

  int mode;

  int identityToIndex(int id) const;
  int identityToIndexAfter(int id) const;

  OrderChange() {
    subject = object = -1;
    mode = -1;
  }

  std::string modeString() const {
    switch (mode) {
    case ORDER_CHANGE_NONE:
      return "none";
    case ORDER_CHANGE_DELETE:
      return "delete";
    case ORDER_CHANGE_INSERT:
      return "insert";
    case ORDER_CHANGE_MOVE:
      return "move";
    case ORDER_CHANGE_RENAME:
      return "rename";
    }
    return "unknown";
  }
};

/**
 * (Unused) description of row context.
 */
class coopy::cmp::RowChangeContext {
public:
  typedef std::map<std::string,coopy::store::SheetCell> row;
  typedef std::vector<row> rows;
  rows before;
  rows after;

  std::map<std::string,coopy::store::SheetCell> getRow(int index) const {
    if (index>=0) {
      return after[index];
    }
    return before[-index-1];
  }

  int afterCount() const {
    return (int)after.size();
  }

  int beforeCount() const {
    return (int)before.size();
  }
};


/**
 * Description of a difference between rows.
 */
class coopy::cmp::RowChange {
private:
  void copy(const RowChange& alt);
public:
  typedef std::map<std::string,coopy::store::SheetCell> txt2cell;
  typedef std::map<std::string,bool> txt2bool;

  int mode;         // One of ROW_CHANGE_*, see below
  txt2cell cond;    // conditions for a match
  txt2cell val;     // values to be assigned
  txt2cell conflictingVal;
  txt2cell conflictingParentVal;
  std::vector<std::string> names;
  std::vector<std::string> allNames;
  txt2bool indexes; // conditions which are indexical, rather than confirming
  bool sequential;
  bool conflicted;
  //RowChangeContext context;
  int pRow, lRow, rRow;

  RowChange() {
    mode = ROW_CHANGE_NONE;
    sequential = true;
    conflicted = false;
    pRow = lRow = rRow = -2;
  }

  RowChange(const RowChange& alt) {
    copy(alt);
  }
  
  const RowChange& operator= (const RowChange& alt) {
    copy(alt);
    return *this;
  }

  bool rowsKnown() const {
    return pRow>=-1;
  }

  std::string modeString() const {
    switch (mode) {
    case ROW_CHANGE_NONE:
      return "none";
    case ROW_CHANGE_DELETE:
      return "delete";
    case ROW_CHANGE_INSERT:
      return "insert";
    case ROW_CHANGE_UPDATE:
      return "update";
    case ROW_CHANGE_MOVE:
      return "move";
    case ROW_CHANGE_CONTEXT:
      return "context";
    }
    return "unknown";
  }

  void show();
};


/**
 * Declaration of all column names and column order.
 */
class coopy::cmp::NameChange {
public:
  int mode;
  bool final;
  bool constant;
  bool loud;
  bool strong;
  std::vector<std::string> names;

  NameChange() {
    mode = -1;
    final = false;
    constant = false;
    loud = false;
    strong = false;
  }

  std::string modeString() const {
    switch (mode) {
    case NAME_CHANGE_NONE:
      return "none";
    case NAME_CHANGE_DECLARE:
      return "declare";
    case NAME_CHANGE_SELECT:
      return "select";
    }
    return "unknown";
  }
};

/**
 * Declaration of the relationship between rows in a set of tables.
 */
class coopy::cmp::LinkDeclare {
public:
  int mode;
  bool column;

  int rc_id_pivot;
  int rc_id_local;
  int rc_id_remote;
  bool rc_deleted;
  std::string rc_str_pivot;
  std::string rc_str_local;
  std::string rc_str_remote;

  // should also give all necessary info to show indexes
  coopy::store::PolySheet pivot;
  coopy::store::PolySheet local;
  coopy::store::PolySheet remote;
};

/**
 * Base class for all sinks of table differences.
 */
class coopy::cmp::Patcher {
private:
  int ct;
  coopy::store::PolySheet getSheetBase();

protected:
  CompareFlags flags;
  FILE *out;
  coopy::store::PolySheet patch_sheet;
  coopy::store::PolySheet active_sheet;
  coopy::store::TextBook *patch_book;
  coopy::store::TextBook *output_book;
  bool conflicted;
  bool pending;
public:
  Patcher() {
    ct = 0;
    out = stdout;
    patch_book = 0;
    output_book = 0;
    conflicted = false;
    pending = true;
  }

  virtual ~Patcher() {}

  virtual bool wantLinks() { return false; }

  virtual bool changeConfig(const ConfigChange& change) { return false; }

  virtual bool changeColumn(const OrderChange& change) { return false; }

  virtual bool changeRow(const RowChange& change) { return false; }

  virtual bool changePool(const PoolChange& change) { return false; }

  virtual bool declareLink(const LinkDeclare& decl) { return false; }

  /**
   *
   * This method is headed toward deprecation in favor of changeName.
   *
   * Called with a sequence of column names.  It is called twice.
   * First, with final=false, giving an initial sequence.
   * Then, a series of changeColumn() calls may happen, specifying
   * manipulations of the columns.  After all changeColumn() calls
   * have happened, this method is called again, with final=true,
   * giving a final sequence of column names.
   *
   */
  virtual bool declareNames(const std::vector<std::string>& names, bool isFinal) {
    return false;
  }

  virtual bool changeName(const NameChange& change) { 
    if (change.mode==NAME_CHANGE_DECLARE) {
      return declareNames(change.names,change.final);
    }
    return false; 
  }

  virtual bool mergeStart() {
    return true;
  }

  virtual bool mergeDone() {
    return true;
  }

  virtual bool mergeAllDone() {
    return true;
  }

  virtual bool setSheet(const char *name) {
    ct++;
    return (ct<=1); 
  }

  virtual bool addSheet(const char *name, 
			const coopy::store::PolySheet& sheet) {
    return false;
  }

  virtual bool removeSheet(const char *name) {
    return false;
  }

  virtual bool renameSheet(const char *name0, const char *name1) {
    return false;
  }

  virtual bool setFlags(const CompareFlags& flags);

  const CompareFlags& getFlags() const {
    return flags;
  }

  CompareFlags& getMutableFlags() {
    return flags;
  }

  // Old stuff, still used but not very important

  virtual bool wantDiff() { return true; }

  virtual bool addRow(const char *tag,
		      const std::vector<coopy::store::SheetCell>& row,
		      const std::string& blank) { return false; }

  virtual bool addRow(const char *tag,
		      const std::vector<std::string>& row,
		      const std::string& blank) { 
    std::vector<coopy::store::SheetCell> row2;
    for (int i=0; i<(int)row.size(); i++) {
      row2.push_back(coopy::store::SheetCell(row[i],false));
    }
    return addRow(tag,row2,blank);
  }

  virtual bool addHeader(const char *tag,
			 const std::vector<coopy::store::SheetCell>& row,
			 const std::string& blank) {
    return addRow(tag,row,blank);
  }

  virtual bool addHeader(const char *tag,
		         const std::vector<std::string>& row,
		         const std::string& blank) { 
    std::vector<coopy::store::SheetCell> row2;
    for (int i=0; i<(int)row.size(); i++) {
      row2.push_back(coopy::store::SheetCell(row[i],false));
    }
    return addHeader(tag,row2,blank);
  }

  virtual bool stripMarkup() { return false; }

  //virtual bool metaHint(const coopy::store::SheetSchema *schema) { 
  //return false; 
  //}

  virtual void attachSheet(coopy::store::PolySheet sheet) {
    patch_sheet = sheet;
  }

  void attachBook(coopy::store::TextBook& book);
  
  coopy::store::TextBook *getBook() {
    return patch_book;
  }

  static Patcher *createByName(const char *name, const char *version = NULL);

  virtual bool startOutput(const std::string& output, CompareFlags& flags);

  virtual bool stopOutput(const std::string& output, CompareFlags& flags);

  virtual bool needOutputBook() {
    return false;
  }

  virtual bool outputStartsFromInput() {
    return false;
  }

  void attachOutputBook(coopy::store::TextBook& book) {
    output_book = &book;
  }
  
  coopy::store::TextBook *getOutputBook() {
    return output_book;
  }

  virtual int getChangeCount() {
    return 0;
  }

  static bool copyFile(const char *src, const char *dest);

  virtual void setConflicted() {
    conflicted = true;
  }

  virtual bool isConflicted() const {
    return conflicted;
  }

  bool applyPool(const PoolChange& change);

  bool addPoolsFromFlags(const coopy::store::DataSheet& sheet, bool msg=true);

  bool addPoolsFromSchema(const coopy::store::DataSheet& sheet, 
			  const coopy::store::NameSniffer& sniffer,
			  const std::string& sheetName,
			  bool msg=true);


  virtual coopy::store::PolySheet getSheet();

  virtual bool metaHint(const coopy::store::DataSheet& sheet);

  virtual bool setNames(bool forceSheetChange = false) { return true; }

};



#endif

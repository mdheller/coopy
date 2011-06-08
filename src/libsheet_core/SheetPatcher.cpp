#include <coopy/SheetPatcher.h>
#include <coopy/NameSniffer.h>

#include <map>
#include <algorithm>

#include <stdio.h>

using namespace std;
using namespace coopy::cmp;
using namespace coopy::store;

#define FULL_COLOR (65535)
#define HALF_COLOR (65535/2)

int SheetPatcher::matchRow(const vector<int>& active_cond,
			   const vector<SheetCell>& cond,
			   int width) {
  PolySheet sheet = getSheet();
  if (!sheet.isValid()) return false;
  int r = -1;
  for (r=0; r<sheet.height(); r++) {
    if (activeRow.cellString(0,r)!="---") {
      bool match = true;
      for (int c=0; c<width; c++) {
	if (active_cond[c]) {
	  if (sheet.cellSummary(c,r)!=cond[c]) {
	    match = false;
	    break;
	  }
	}
      }
      if (match) return r;
    }
  }
  return -1;
}

int SheetPatcher::matchCol(const std::string& mover) {
  string imover = mover;
  if (syn2name.find(mover)!=syn2name.end()) {
    imover = syn2name[mover];
  }
  for (int i=0; i<activeCol.width(); i++) {
    //    printf("Checking %s against %s\n", activeCol.cellString(i,0).c_str(),
    //	   mover.c_str());
    if (activeCol.cellString(i,0)==imover) {
      if (statusCol.cellString(i,0)!="---") {
	return i;
      }
    }
  }
  fprintf(stderr,"column not found: %s\n", mover.c_str());
  exit(1);
  return -1;
}

int SheetPatcher::updateCols() {
  PolySheet sheet = getSheet();
  if (activeCol.width()!=sheet.width()) {
    printf("activeCol drift\n");
    exit(1);
  }
  name2col.clear();
  for (int i=0; i<activeCol.width(); i++) {
    string name = activeCol.cellString(i,0);
    //printf("NAME %d %s\n", i, name.c_str());
    if (name!="---") {
      name2col[name] = i;
      if (name2syn.find(name)!=name2syn.end()) {
	name2col[name2syn[name]] = i;
      }
    }
  }
  return 0;
}

bool SheetPatcher::moveColumn(int idx, int idx2) {
  PolySheet sheet = getSheet();
  ColumnRef from(idx);
  ColumnRef to(idx2);
  bool ok = sheet.moveColumn(from,to).isValid();
  activeCol.moveColumn(from,to);
  ColumnRef at = statusCol.moveColumn(from,to);
  if (descriptive) {
    int first = idx;
    int final = at.getIndex();
    int sgn = 1;
    string name = "";
    string ch = ">";
    if (final<first) {
      first = final;
      final = idx;
      sgn = -1;
      ch = "<";
    }
    for (int i=first; i!=final; i+=sgn) {
      if (statusCol.cellString(i,0)!="---") {
	name += ch;
      }
    }
    statusCol.cellString(final,0,name);
  }
  updateCols();
  return ok;
}

bool SheetPatcher::changeColumn(const OrderChange& change) {
  changeCount++;
  if (chain) chain->changeColumn(change);

  PolySheet sheet = getSheet();
  if (!sheet.isValid()) {
    fprintf(stderr,"No sheet available to patch\n");
    return false;
  }
  switch (change.mode) {
  case ORDER_CHANGE_DELETE:
    //return sheet->deleteColumn(ColumnRef(change.subject));
    {
      string mover = change.namesBefore[change.identityToIndex(change.subject)];
      //printf("Deleting %s\n", mover.c_str());
      int idx = matchCol(mover);
      bool ok = true;
      if (!descriptive) {
	ColumnRef col(idx); //change.identityToIndex(change.subject));
	ok = sheet.deleteColumn(col);
	activeCol.deleteColumn(col);
	statusCol.deleteColumn(col);
	if (sheet.width()==0) {
	  sheet.deleteData();
	  rowCursor = -1;
	}
      } else {
	statusCol.cellString(idx,0,"---");
	bool gone = true;
	for (int i=0; i<(int)statusCol.width(); i++) {
	  if (statusCol.cellString(i,0)=="") {
	    gone = false;
	    break;
	  }
	}
	if (gone) {
	  killNeutral = true;
	}
      }
      updateCols();
      return ok;
    }
    break;
  case ORDER_CHANGE_INSERT:
    //return sheet->insertColumn(ColumnRef(change.subject)).isValid();
    {
      int toSheet = change.identityToIndexAfter(change.subject);
      string mover = change.namesAfter[toSheet];
      //printf("Adding %s\n", mover.c_str());
      /*
      if (toSheet==change.indicesAfter.size()-1) {
	toSheet = -1;
      } else {
	int toId = change.indicesAfter[toSheet+1];
	toSheet = change.identityToIndex(toId);
      }
      */
      string before = "";
      int idx = -1;
      if (toSheet!=change.indicesAfter.size()-1) {
	//if (toSheet==-1) {
	//printf("At end\n");
	//} else {
	before = change.namesAfter[toSheet+1];
	//printf("Before %s\n", before.c_str());
	idx = matchCol(before);
      }
      //bool ok = sheet.insertColumn(ColumnRef(toSheet)).isValid();
      bool ok = sheet.insertColumn(ColumnRef(idx)).isValid();
      ColumnRef at = activeCol.insertColumn(ColumnRef(idx));
      statusCol.insertColumn(ColumnRef(idx));
      activeCol.cellString(at.getIndex(),0,mover);
      statusCol.cellString(at.getIndex(),0,"+++");
      if (descriptive) {
	Poly<Appearance> appear = sheet.getColAppearance(at.getIndex());
	if (appear.isValid()) {
	  appear->begin();
	  appear->setBackgroundRgb16(HALF_COLOR,
				     FULL_COLOR,
				     HALF_COLOR,
				     AppearanceRange::full());
	  appear->end();
	}
      }
      updateCols();
      return ok;
    }
    break;
  case ORDER_CHANGE_MOVE:
    //return sheet->moveColumn(ColumnRef(change.subject),
    //ColumnRef(change.object)
    //).isValid();
    {
      int toSheet = change.identityToIndexAfter(change.subject);
      string mover = change.namesAfter[toSheet];
      int idx = matchCol(mover);
      //printf("Moving %s\n", mover.c_str());
      string before = "";
      int idx2 = -1;
      if (toSheet!=change.indicesAfter.size()-1) {
	before = change.namesAfter[change.indicesAfter[toSheet+1]];
	//printf("Before %s\n", before.c_str());
	idx2 = matchCol(before);
	//int toId = change.indicesAfter[toSheet+1];
	//toSheet = change.identityToIndex(toId);
      }
      /*
      if (toSheet==change.indicesAfter.size()-1) {
	toSheet = -1;
      } else {
	int toId = change.indicesAfter[toSheet+1];
	toSheet = change.identityToIndex(toId);
      }
      ColumnRef from(change.identityToIndex(change.subject));
      ColumnRef to(toSheet);
      bool ok = sheet.moveColumn(from,to).isValid();
      */
      return moveColumn(idx,idx2);
    }
    break;
  default:
    fprintf(stderr,"* ERROR: Unknown column operation\n");
    break;
  }
  return false;
}

bool SheetPatcher::markChanges(int r,int width,
			       vector<int>& active_val,
			       vector<SheetCell>& val) {
  PolySheet sheet = getSheet();

  string separator = "";
  for (int c=0; c<width; c++) {
    if (active_val[c]) {
      if (separator=="") {
	separator = "->";
	bool more = true;
	while (more) {
	  more = false;
	  for (int i=0; i<width; i++) {
	    SheetCell prev = sheet.cellSummary(i,r);
	    if (prev.text.find(separator)!=string::npos) {
	      separator = string("-") + separator;
	      more = true;
	      break;
	    }
	  }
	}
      }
      activeRow.cellString(0,r,separator);
      if (descriptive) {
	SheetCell prev = sheet.cellSummary(c,r);
	string from = prev.toString();
	if (prev.escaped) from = "";
	string to = val[c].toString();
	sheet.cellString(c,r,from + separator + to);
	Poly<Appearance> appear = sheet.getCellAppearance(c,r);
	if (appear.isValid()) {
	  appear->begin();
	  appear->setBackgroundRgb16(HALF_COLOR,
				     HALF_COLOR,
				     FULL_COLOR,
				     AppearanceRange::full());
	  appear->setWeightBold(true,AppearanceRange::full());
	  appear->end();
	}
      } else {
	sheet.cellSummary(c,r,val[c]);
      }
    }
  }
  return true;
}

bool SheetPatcher::changeRow(const RowChange& change) {
  changeCount++;
  if (chain) chain->changeRow(change);

  PolySheet sheet = getSheet();
  if (!sheet.isValid()) {
    fprintf(stderr,"No sheet available to patch\n");
    return false;
  }

  if (activeRow.height()!=sheet.height() || activeRow.width()!=1) {
    activeRow.resize(1,sheet.height());
  }

  dbg_printf("\n======================\nRow cursor in: %d\n", rowCursor);
  if (!change.sequential) rowCursor = -1;
  //map<string,int> dir;
  vector<int> active_cond;
  int active_conds = 0;
  vector<SheetCell> cond;
  vector<int> active_val;
  vector<SheetCell> val;
  vector<string> allNames = change.allNames;
  int width = sheet.width(); //(int)change.allNames.size();
  /*
  if (width==0) {
    if (column_names.size()==0) {
      NameSniffer sniffer(sheet);
      column_names = sniffer.suggestNames();
    }
    allNames = column_names;
    width = (int)allNames.size();
  }
  */
  for (int i=0; i<width; i++) {
    //dir[allNames[i]] = i;
    active_cond.push_back(0);
    cond.push_back(SheetCell());
    active_val.push_back(0);
    val.push_back(SheetCell());
  }
  for (RowChange::txt2cell::const_iterator it = change.cond.begin();
       it!=change.cond.end(); it++) {
    if (name2col.find(it->first)!=name2col.end()) {
      int idx = name2col[it->first]; //dir[it->first];
      //printf("  [cond] %d %s -> %s\n", idx, it->first.c_str(), it->second.toString().c_str());
      active_cond[idx] = 1;
      active_conds++;
      cond[idx] = it->second;
    }
  }
  for (RowChange::txt2cell::const_iterator it = change.val.begin();
       it!=change.val.end(); it++) {
    if (name2col.find(it->first)!=name2col.end()) {
      int idx = name2col[it->first]; //dir[it->first];
      //printf("  [val] %d %s -> %s\n", idx, it->first.c_str(), it->second.toString().c_str());
      active_val[idx] = 1;
      val[idx] = it->second;
    }
  }
  
  switch (change.mode) {
  case ROW_CHANGE_INSERT:
    {
      if (sheet.isSequential()) {
	RowRef tail(rowCursor);
	int r = sheet.insertRow(tail).getIndex();
	activeRow.insertRow(tail);
	/*
	if (rowCursor!=-1) {
	  rowCursor++;
	  if (rowCursor==sheet->height()) {
	    rowCursor = -1;
	  }
	  }*/
	if (r>=0) {
	  activeRow.cellString(0,r,"+++");
	  for (int c=0; c<width; c++) {
	    if (active_val[c]) {
	      sheet.cellSummary(c,r,val[c]);
	    }
	  }
	  if (descriptive) {
	    Poly<Appearance> appear = sheet.getRowAppearance(r);
	    if (appear.isValid()) {
	      appear->begin();
	      appear->setBackgroundRgb16(HALF_COLOR,
					 FULL_COLOR,
					 HALF_COLOR,
					 AppearanceRange::full());
	      appear->setWeightBold(true,AppearanceRange::full());
	      appear->end();
	    }
	  }
	}
	r++;
	if (r>=sheet.height()) {
	  r = -1;
	}
	rowCursor = r;
      } else {
	Poly<SheetRow> inserter = sheet.insertRow();
	for (int c=0; c<width; c++) {
	  if (active_val[c]) {
	    inserter->setCell(c,val[c]);
	  }
	}
	inserter->flush();
      }
    }
    break;
  case ROW_CHANGE_DELETE:
    {
      int r = matchRow(active_cond,cond,width);
      if (r<0) return false;
      RowRef row(r);
      rowCursor = r;
      if (!descriptive) {
	sheet.deleteRow(row);
	activeRow.deleteRow(row);
      } else {
	Poly<Appearance> appear = sheet.getRowAppearance(r);
	if (appear.isValid()) {
	  appear->begin();
	  appear->setBackgroundRgb16(FULL_COLOR,
				     HALF_COLOR,
				     HALF_COLOR,
				     AppearanceRange::full());
	  //appear->setWeightBold(true,AppearanceRange::full());
	  appear->setStrikethrough(true,AppearanceRange::full());
	  appear->end();
	}
	activeRow.cellString(0,r,"---");
      }
      if (rowCursor>=sheet.height()) {
	rowCursor = -1;
      }
      return true;
    }
    break;
  case ROW_CHANGE_CONTEXT:
    {
      if (active_conds>0) {
	int r = matchRow(active_cond,cond,width);
	if (r<0) return false;
	r++;
	if (r>=sheet.height()) {
	  r = -1;
	}
	RowRef row(r);
	rowCursor = r;
      } else {
	rowCursor = 0;
      }
      return true;
    }
    break;
  case ROW_CHANGE_MOVE:
    {
      bool success = false;
      int r = matchRow(active_cond,cond,width);
      if (r<0) return false;
      RowRef from(r);
      RowRef to(rowCursor);
      dbg_printf("Moving %d to %d in sheet of length %d\n", from.getIndex(), to.getIndex(), sheet.height());
      RowRef result = sheet.moveRow(from,to);
      if (result.getIndex() == -1) {
	fprintf(stderr,"Row move failed in sheet of type %s\n",
		sheet.desc().c_str());
      } else {
	activeRow.moveRow(from,to);
      }
      r = result.getIndex();
      dbg_printf("Move result was %d\n", r);
      for (int y=0; y<sheet.height(); y++) {
	dbg_printf("%d %s / ", y, sheet.cellString(0,y).c_str());
      }
      dbg_printf("\n");
      markChanges(r,width,active_val,val);
      r++;
      if (r>=sheet.height()) {
	r = -1;
      }
      rowCursor = r;
      return true;
    }
    break;
  case ROW_CHANGE_UPDATE:
    {
      bool success = false;
      int r = matchRow(active_cond,cond,width);
      if (r<0) {
	dbg_printf("No match for update\n");
	rowCursor = -1;
	return false;
      }
      dbg_printf("Match for assignment\n");
      markChanges(r,width,active_val,val);
      r++;
      if (r>=sheet.height()) {
	r = -1;
      }
      rowCursor = r;
      dbg_printf("Cursor moved to %d\n", r);
      return true;
    }
    break;
  default:
    fprintf(stderr,"* ERROR: unsupported row operation\n");
    return false;
    break;
  }
  return true;
}

bool SheetPatcher::declareNames(const std::vector<std::string>& names, 
				bool final) {
  PolySheet sheet = getSheet();
  if (!sheet.isValid()) return false;
  if (chain) chain->declareNames(names,final);
  if (config.trustNames==false) {
    if (!descriptive) {
      if ((int)names.size()!=sheet.width()) {
	fprintf(stderr,"* ERROR: name mismatch\n");
	return false;
      }
    }
    if (!final) {
      for (int i=0; i<(int)names.size(); i++) {
	if (names[i]!=activeCol.cellString(i,0)) {
	  if (names[i][0]=='[') {
	    syn2name[names[i]] = activeCol.cellString(i,0);
	    name2syn[activeCol.cellString(i,0)] = names[i];
	  }
	}
      }
      updateCols();
    } else {
      if (!descriptive) {
	for (int i=0; i<(int)names.size(); i++) {
	  if (names[i]!=activeCol.cellString(i,0)) {
	    if (names[i][0]!='[') {
	      int idx = matchCol(names[i]);
	      int idx1 = matchCol(activeCol.cellString(i,0));
	      moveColumn(idx,idx1);
	    }
	  }
	}
	updateCols();
      }
    }
  } else {
    for (int i=0; i<(int)names.size(); i++) {
      printf("Checking %s\n", names[i].c_str());
    }
    fprintf(stderr,"Named columns not implemented yet\n");
    exit(1);
  }
  return true;
}

bool SheetPatcher::setSheet(const char *name) {
  if (chain) chain->setSheet(name);

  TextBook *book = getBook();
  if (book==NULL) return false;

  //reset
  config = ConfigChange();
  columns.clear();
  column_names.clear();
  rowCursor = -1;

  // load
  PolySheet psheet;
  attachSheet(psheet);
  psheet = book->readSheet(name);
  if (!psheet.isValid()) {
    if (!book->namedSheets()) {
      psheet = book->readSheetByIndex(0);
    }
  }
  if (!psheet.isValid()) {
    fprintf(stderr,"Cannot find sheet %s\n", name);
    return false;
  }
  dbg_printf("Moved to sheet %s\n", name);
  attachSheet(psheet);
  //sheet = &psheet;
  setNames();

  return true;
}


bool SheetPatcher::mergeStart() {
  killNeutral = false;
  activeRow.resize(1,0);
  setNames();
  if (chain) chain->mergeStart();
  return true;
}

static string clean(const string& s) {
  return (s!="NULL")?s:"";
}

bool SheetPatcher::mergeAllDone() {
  if (chain) chain->mergeAllDone();
  if (descriptive) {
    PolySheet sheet = getSheet();
    if (!sheet.isValid()) return false;
    sheet.insertColumn(ColumnRef(0)); 
    //sheet.insertRow(RowRef(0));
    for (int i=0; i<sheet.height()&&i<activeRow.height(); i++) {
      string txt = activeRow.cellString(0,i);
      if (txt==""&&killNeutral) {
	txt = "---";
      }
      if (txt!="") {
	sheet.cellString(0,i,txt);
	Poly<Appearance> appear = sheet.getCellAppearance(0,i);
	if (appear.isValid()) {
	  appear->begin();
	  appear->setWeightBold(true,AppearanceRange::full());
	  appear->setStrikethrough(false,AppearanceRange::full());
	  appear->end();
	}
      }
    }
    COOPY_ASSERT(sniffer);
    int r = -1;
    r = sniffer->getHeaderHeight()-1;
    if (r>=0 && r<sheet.height()) {
      string key = clean(sheet.cellString(0,r));
      if (key == "" || key == "->") {
	for (int i=0; i<=r; i++) {
	  sheet.cellString(0,i,string("@")+clean(sheet.cellString(0,i)));
	}
      } else {
	r = -1;
      }
    }
    if (r<0) {
      sheet.insertRow(RowRef(0)); 
      sheet.cellString(0,yoff,"@@");
      for (int i=1; i<sheet.width(); i++) {
	sheet.cellString(i,yoff,activeCol.cellString(i-1,0));
      }
    }
    int yoff = 0;
    bool colAction = false;
    for (int i=1; i<sheet.width(); i++) {
      if (statusCol.cellString(i-1,0)!="") {
	colAction = true;
	break;
      }
    }
    if (colAction) {
      sheet.insertRow(RowRef(0)); 
      sheet.cellString(0,0,"!");
      for (int i=1; i<sheet.width(); i++) {
	sheet.cellString(i,0,statusCol.cellString(i-1,0));
      }
      yoff++;
    }
    Poly<Appearance> appear = sheet.getRowAppearance(r+yoff);
    if (appear.isValid()) {
      appear->begin();
      appear->setWeightBold(true,AppearanceRange::full());
      appear->end();
    }

    vector<int> show;
    show.resize(sheet.height(),0);
    for (int i=0; i<sheet.height(); i++) {
      bool present = clean(sheet.cellString(0,i))!="";
      if (present) {
	for (int j=-2; j<=2; j++) {
	  int k = i+j;
	  if (k>=0 && k<sheet.height()) {
	    show[k] = 1;
	  }
	}
      }
    }
    int offset = 0;
    bool addedBreak = false;
    for (int i=0; i<(int)show.size(); i++) {
      if (show[i]==0) {	
	int k = i+offset;
	if (addedBreak) {
	  sheet.deleteRow(RowRef(k));
	  offset--;
	} else {
	  sheet.cellString(0,k,"...");
	  for (int j=1; j<sheet.width(); j++) {
	    sheet.cellString(j,k,"...");
	  }
	  addedBreak = true;
	}
      } else {
	addedBreak = false;
      }
    }
    
  }
  return true;
}

bool SheetPatcher::mergeDone() {
  if (chain) chain->mergeDone();
  return true;
}


void SheetPatcher::setNames() {
  PolySheet sheet = getSheet();
  if (!sheet.isValid()) return;
  if (&sheet!=sniffedSheet) {
    clearNames();
    activeCol.resize(0,1);
    statusCol.resize(0,1);
    sniffer = new NameSniffer(sheet);
    COOPY_ASSERT(sniffer);
    sniffedSheet = &sheet;
    activeCol.resize(sheet.width(),1);
    statusCol.resize(sheet.width(),1);
    for (int i=0; i<sheet.width(); i++) {
      activeCol.cellString(i,0,sniffer->suggestColumnName(i));
    }
    updateCols();
  }
}



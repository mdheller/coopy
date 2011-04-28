#include <coopy/SheetPatcher.h>
#include <coopy/NameSniffer.h>

#include <map>
#include <algorithm>

#include <stdio.h>

using namespace std;
using namespace coopy::cmp;
using namespace coopy::store;

bool SheetPatcher::changeColumn(const OrderChange& change) {
  changeCount++;
  if (sheet==NULL) return false;
  if (chain) chain->changeColumn(change);
  switch (change.mode) {
  case ORDER_CHANGE_DELETE:
    //return sheet->deleteColumn(ColumnRef(change.subject));
    {
      bool ok = sheet->deleteColumn(ColumnRef(change.identityToIndex(change.subject)));
      //dbg_printf("Sheet width is %d\n", sheet->width());
      if (sheet->width()==0) {
	sheet->deleteData();
	rowCursor = -1;
      }
      return ok;
    }
    break;
  case ORDER_CHANGE_INSERT:
    //return sheet->insertColumn(ColumnRef(change.subject)).isValid();
    {
      int toSheet = change.identityToIndexAfter(change.subject);
      if (toSheet==change.indicesAfter.size()-1) {
	toSheet = -1;
      } else {
	int toId = change.indicesAfter[toSheet+1];
	toSheet = change.identityToIndex(toId);
      }
      return sheet->insertColumn(ColumnRef(toSheet)).isValid();
    }
    break;
  case ORDER_CHANGE_MOVE:
    //return sheet->moveColumn(ColumnRef(change.subject),
    //ColumnRef(change.object)
    //).isValid();
    {
      int toSheet = change.identityToIndexAfter(change.subject);
      if (toSheet==change.indicesAfter.size()-1) {
	toSheet = -1;
      } else {
	int toId = change.indicesAfter[toSheet+1];
	toSheet = change.identityToIndex(toId);
      }
      return sheet->moveColumn(ColumnRef(change.identityToIndex(change.subject)),
			       ColumnRef(toSheet)
			       ).isValid();
    }
    break;
  default:
    fprintf(stderr,"* ERROR: Unknown column operation\n");
    break;
  }
  return false;
}

bool SheetPatcher::changeRow(const RowChange& change) {
  dbg_printf("\n======================\nRow cursor in: %d\n", rowCursor);
  changeCount++;
  if (sheet==NULL) return false;
  if (chain) chain->changeRow(change);
  if (!change.sequential) rowCursor = -1;
  map<string,int> dir;
  vector<int> active_cond;
  vector<SheetCell> cond;
  vector<int> active_val;
  vector<SheetCell> val;
  vector<string> allNames = change.allNames;
  int width = (int)change.allNames.size();
  if (width==0) {
    if (column_names.size()==0) {
      NameSniffer sniffer(*sheet);
      column_names = sniffer.suggestNames();
    }
    allNames = column_names;
    width = (int)allNames.size();
  }
  for (int i=0; i<width; i++) {
    dir[allNames[i]] = i;
    active_cond.push_back(0);
    cond.push_back(SheetCell());
    active_val.push_back(0);
    val.push_back(SheetCell());
  }
  for (RowChange::txt2cell::const_iterator it = change.cond.begin();
       it!=change.cond.end(); it++) {
    int idx = dir[it->first];
    //printf("  [cond] %d %s -> %s\n", idx, it->first.c_str(), it->second.toString().c_str());
    active_cond[idx] = 1;
    cond[idx] = it->second;
  }
  for (RowChange::txt2cell::const_iterator it = change.val.begin();
       it!=change.val.end(); it++) {
    int idx = dir[it->first];
    //printf("  [val] %d %s -> %s\n", idx, it->first.c_str(), it->second.toString().c_str());
    active_val[idx] = 1;
    val[idx] = it->second;
  }
  
  switch (change.mode) {
  case ROW_CHANGE_INSERT:
    {
      if (sheet->isSequential()) {
	RowRef tail(rowCursor);
	int r = sheet->insertRow(tail).getIndex();
	/*
	if (rowCursor!=-1) {
	  rowCursor++;
	  if (rowCursor==sheet->height()) {
	    rowCursor = -1;
	  }
	  }*/
	if (r>=0) {
	  for (int c=0; c<width; c++) {
	    if (active_val[c]) {
	      sheet->cellSummary(c,r,val[c]);
	    }
	  }
	}
	r++;
	if (r>=sheet->height()) {
	  r = -1;
	}
	rowCursor = r;
      } else {
	Poly<SheetRow> inserter = sheet->insertRow();
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
      bool success = false;
      int r;
      for (r=0; r<sheet->height(); r++) {
	bool match = true;
	for (int c=0; c<width; c++) {
	  if (active_cond[c]) {
	    if (sheet->cellSummary(c,r)!=cond[c]) {
	      match = false;
	      break;
	    }
	  }
	}
	if (match) {
	  RowRef row(r);
	  rowCursor = r;
	  sheet->deleteRow(row);
	  if (rowCursor>=sheet->height()) {
	    rowCursor = -1;
	  }
	  success = true;
	  break;
	}
      }
      return success;
    }
    break;
  case ROW_CHANGE_CONTEXT:
    {
      bool success = false;
      int r;
      for (r=0; r<sheet->height(); r++) {
	bool match = true;
	for (int c=0; c<width; c++) {
	  if (active_cond[c]) {
	    if (sheet->cellSummary(c,r)!=cond[c]) {
	      match = false;
	      break;
	    }
	  }
	}
	if (match) {
	  r++;
	  if (r>=sheet->height()) {
	    r = -1;
	  }
	  RowRef row(r);
	  rowCursor = r;
	  success = true;
	  break;
	}
      }
      return success;
    }
    break;
  case ROW_CHANGE_MOVE:
    {
      bool success = false;
      int r;
      for (r=0; r<sheet->height(); r++) {
	bool match = true;
	for (int c=0; c<width; c++) {
	  if (active_cond[c]) {
	    if (sheet->cellSummary(c,r)!=cond[c]) {
	      match = false;
	      break;
	    }
	  }
	}
	if (match) {
	  RowRef from(r);
	  RowRef to(rowCursor);
	  dbg_printf("Moving %d to %d in sheet of length %d\n", from.getIndex(), to.getIndex(), sheet->height());
	  RowRef result = sheet->moveRow(from,to);
	  if (result.getIndex() == -1) {
	    fprintf(stderr,"Row move failed in sheet of type %s\n",
		    sheet->desc().c_str());
	  }
	  r = result.getIndex();
	  dbg_printf("Move result was %d\n", r);
	  for (int y=0; y<sheet->height(); y++) {
	    dbg_printf("%d %s / ", y, sheet->cellString(0,y).c_str());
	  }
	  dbg_printf("\n");
	  r++;
	  if (r>=sheet->height()) {
	    r = -1;
	  }
	  rowCursor = r;
	  success = true;
	  break;
	}
      }
      return success;
    }
    break;
  case ROW_CHANGE_UPDATE:
    {
      bool success = false;
      int r;
      for (r=0; r<sheet->height(); r++) {
	bool match = true;
	for (int c=0; c<width; c++) {
	  if (active_cond[c]) {
	    dbg_printf("compare %s and %s\n",
		       sheet->cellSummary(c,r).toString().c_str(),
		       cond[c].toString().c_str());
	    if (sheet->cellSummary(c,r)!=cond[c]) {
	      match = false;
	      break;
	    }
	  }
	}
	if (match) {
	  dbg_printf("Match for assignment\n");
	  for (int c=0; c<width; c++) {
	    if (active_val[c]) {
	      sheet->cellSummary(c,r,val[c]);
	    }
	  }
	  r++;
	  if (r>=sheet->height()) {
	    r = -1;
	  }
	  rowCursor = r;
	  dbg_printf("Cursor moved to %d\n", r);
	  success = true;
	  break;
	}
      }
      if (!success) {
	dbg_printf("No match for update\n");
	rowCursor = -1;
      }
      return success;
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
  if (sheet==NULL) return false;
  if (chain) chain->declareNames(names,final);
  if (config.trustNames==false) {
    if ((int)names.size()!=sheet->width()) {
      fprintf(stderr,"* WARNING: name mismatch\n");
      return false;
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
  if (book==NULL) return false;
  if (chain) chain->setSheet(name);

  //reset
  config = ConfigChange();
  columns.clear();
  column_names.clear();
  rowCursor = 0;
  sheet = NULL;

  // load
  psheet = book->readSheet(name);
  if (!psheet.isValid()) {
    fprintf(stderr,"Cannot find sheet %s\n", name);
    return false;
  }
  dbg_printf("Moved to sheet %s\n", name);
  sheet = &psheet;
  return true;
}


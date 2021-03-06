
#include <coopy/DataSheet.h>
#include <coopy/Sha1Generator.h>
#include <coopy/SheetSchema.h>

using namespace coopy::store;

DataSheet::~DataSheet() {
  setMeta(NULL);
}

SheetSchema *DataSheet::getMeta() const {
  //printf("Meta data? %ld %s\n", (long int) this, meta_hint?"yes":"no");
  return meta_hint;
}


void DataSheet::setMeta(SheetSchema *hint) {
  if (meta_hint) {
    delete meta_hint;
    meta_hint = NULL;
  }
  meta_hint = hint;
  /*
  if (hint) {
    printf("Set some meta data! %ld %s\n", 
	   (long int) this,
	   hint->toString().c_str());
  }
  */
}

std::string DataSheet::encode(const SheetStyle& style) const {
  std::string delim = style.getDelimiter();
  std::string eol = style.getEol();
  std::string result = "";
  int last = height()-1;
  for (int y=0;y<height();y++) {
    std::string line = "";
    for (int x=0;x<width();x++) {
      if (x>0) {
	line += delim;
      }
      line += encodeCell(cellSummary(x,y),style);
    }
    int len = line.length();
    if (style.shouldEolAtEof()||y!=last) {
      line += eol;
    }
    if (style.shouldMarkHeader()) {
      SheetSchema *schema = getSchema();
      if (schema!=NULL) {
	if (schema->headerHeight()>=0) {
	  if (schema->headerHeight()==y) {
	    if (len<3) len = 3;
	    if (len>79) len = 79;
	    for (int i=0; i<len; i++) {
	      line += '-';
	    }
	    line += eol;
	  }
	}
      }
    }
    result += line;
  }
  return result;
}

std::string DataSheet::encodeCell(const SheetCell& c, 
				  const SheetStyle& style) {
  std::string str = c.text;
  std::string delim = style.getDelimiter();
  bool need_quote = false;
  for (size_t i=0; i<str.length(); i++) {
    char ch = str[i];
    if (ch=='"'||ch=='\''||ch==delim[0]||ch=='\r'||ch=='\n'||ch=='\t'||ch==' ') {
      need_quote = true;
      break;
    }
  }
  std::string nil = style.getNullToken();
  /*
  if (str==nil) {
    need_quote = !c.escaped;
  }
  */
  //printf("encoding [%s] [%d]\n", str.c_str(), c.escaped);
  if (str=="" && c.escaped) {
    if (style.haveNullToken()) {
      return style.getNullToken();
    }
  }
  std::string result = "";
  if (!c.escaped) {
    if (style.quoteCollidingText()) {
      int score = 0;
      for (score=0; score<(int)str.length(); score++) {
	if (str[score]!='_') {
	  break;
	}
      }
      if (str.substr(score,str.length())==nil) {
	str = std::string("_") + str;
      }
    }
  }
  if (need_quote) { result += '"'; }
  std::string line_buf = "";
  for (size_t i=0; i<str.length(); i++) {
    char ch = str[i];
    if (ch=='"') {
      result += '"';
    }
    if (ch!='\r'&&ch!='\n') {
      if (line_buf.length()>0) {
	result += line_buf;
	line_buf = "";
      }
      result += ch;
    } else {
      if (style.shouldTrimEnd()) {
	line_buf+=ch;
      } else {
	result+=ch;
      }
    }
  }
  if (need_quote) { result += '"'; }
  return result;
}


bool DataSheet::copyData(const DataSheet& src) {
  if (!canWrite()) {
    fprintf(stderr,"Copy failed, cannot write to target\n");
    return false;
  }
  if (width()!=src.width()||height()!=src.height()) {
    if (canResize()) {
      resize(src.width(),src.height());
    }
  }
  if (width()!=src.width()||height()!=src.height()) {
    fprintf(stderr,"Copy failed, src and target are not the same size\n");
    return false;
  }
  for (int i=0; i<src.height(); i++) {
    for (int j=0; j<src.width(); j++) {
      setCell(j,i,src.getCell(j,i));
    }
  }
  return true;
}

bool DataSheet::applyRowCache(const RowCache& cache, int row, 
			      SheetCell *result) {
  //printf("Apply row %d\n", row);
  if (row==-1) {
    fprintf(stderr,"Sheet requires a row\n");
    return false;
  }
  while (row>=height()) {
    insertRow(RowRef(-1));
    //dbg_printf("adding row.. %d\n", height());
  }
  for (int i=0; i<(int)cache.cells.size(); i++) {
    if (cache.flags[i]) {
      //printf("SET CELL TO %s\n", cache.cells[i].toString().c_str());
      setCell(i,row,cache.cells[i]);
    }
  }
  if (result) {
    *result = SheetCell();
  }
  return true;
}


Poly<SheetRow> DataSheet::insertRow() {
  if (isSequential()) {
    Poly<SheetRow> row = insertRowOrdered(RowRef(-1));
    return row;
  }
  SheetRow *row = new CacheSheetRow(this);
  COOPY_ASSERT(row);
  return Poly<SheetRow>(row,true);
}

Poly<SheetRow> DataSheet::insertRowOrdered(const RowRef& base) {
  RowRef at = insertRow(base);
  OrderedSheetRow *row = new OrderedSheetRow(this,at.getIndex());
  COOPY_ASSERT(row);
  return Poly<SheetRow>(row,true);
}

std::string DataSheet::getHash(bool cache) const {
  DataSheet *mod = (DataSheet *)this;
  if (!cache) mod->hash_cache = "";
  if (hash_cache!="") {
    dbg_printf("(sha1 %ld %s %s)\n", (long int)this, hash_cache.c_str(), desc().c_str());
    return hash_cache;
  }
  mod->hash_cache = mod->getRawHash();
  if (hash_cache!="") {
    dbg_printf("(raw sha1 %ld %s %s)\n", (long int)this, hash_cache.c_str(), desc().c_str());
    return hash_cache;
  }
  dbg_printf("Computing sha1\n");
  Sha1Generator sha1;
  for (int y=0;y<height();y++) {
    std::string txt;
    for (int x=0;x<width();x++) {
      SheetCell cell = cellSummary(x,y);
      if (cell.escaped) {
	txt += "N";
      } else {
	txt += "X";
	txt += cell.text;
      }
    }
    sha1.add(txt);
  }
  std::string key = sha1.finish();
  dbg_printf("sha1 %ld %s %s\n", (long int)this, key.c_str(), desc().c_str());
  if (cache) {
    mod->hash_cache = key;
  }
  return key;
}



bool OrderedSheetRow::invent(int x) {
  dbg_printf("\n\n*** Maxes need to get cached! Hop to it! %s:%d\n\n\n", __FILE__, __LINE__);
  
  int top = -1;
  for (int i=0; i<sheet->height(); i++) {
    SheetCell v = sheet->cellSummary(x,i);
    int v2 = v.asInt();
    if (v2>top) top = v2;
  }

  return sheet->cellSummary(x,y,SheetCell(top+1));
}



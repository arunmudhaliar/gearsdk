//
//  Copyright 2024 homenet25
//  qtextfile.hpp
//  networkcommon
//
//  Created by Arun A on 28/12/23.
//

#ifndef qtextfile_hpp
#define qtextfile_hpp

#include "../../common/qstring.hpp"
#include "../../common/sdktypes.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "qtextfile"

class qtextfile {
   public:
	qtextfile();
	qtextfile(const fs::path& path, const qstring& mode = "r");
	~qtextfile();
	int open(const fs::path& path, const qstring& mode);
	void close();
	ssize_t read();
	const qstring& get_buffer() { return buffer; }
	static int get_content(const fs::path& path, qstring& out);
	int seek_to_begining();

   private:
	FILE* fp = nullptr;
	qstring buffer;
};
#endif /* qtextfile_hpp */

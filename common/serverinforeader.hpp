//
//  serverinforeader.hpp
//  common
//
//  Created by Arun A on 10/02/25.
//

#ifndef serverinforeader_hpp
#define serverinforeader_hpp

#include "qstring.hpp"
#include "sdktypes.hpp"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>

#undef __LOGTAG__
#define __LOGTAG__ "server_info_reader"

namespace gsdk {
namespace common {
class server_info_reader {
   private:
	std::map<qstring, qstring> config;
	std::mutex config_mutex;
	bool loaded = false;
	static server_info_reader* instance;
	static std::mutex instance_mutex;

	server_info_reader() = default;	 // private constructor
	~server_info_reader() { debug_print(LOG_LEVEL_0, __LOGTAG__, "destroyed"); }

	char* trim(char* str);

   public:
	static server_info_reader* get_instance();
	static void destroy_instance();

	// delete copy constructor and assignment operator to prevent multiple instances
	server_info_reader(const server_info_reader&) = delete;
	server_info_reader& operator=(const server_info_reader&) = delete;

	// method to load and parse the config file
	bool load_config(const qstring& file_path, bool force = false);
	// method to print loaded config
	void print_config();
	// method to get a value by key
	const qstring get_value(const qstring& key);
	// method to get a value with a default fallback
	const qstring get_value_else_default(const qstring& key, const qstring& default_value);
	// method to get a value as a number, with a default fallback
	int get_value_as_number(const qstring& key, int default_value);
};
}  // namespace common
}  // namespace gsdk
#endif /* serverinforeader_hpp */

/**
 * @file qstats-crawler.h
 * @brief This file contains the declaration of the qstats_crawler class, which is responsible for crawling and parsing files to extract statistics.
 *
 * @author Arun A
 * @copyright Copyright (c) 2024 homenet25
 */

#ifndef qstats_crawler_h
#define qstats_crawler_h

#include "../../networkcommon/source/qstatslogger.hpp"
#include "../qpgsql/qpgsql.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "qstats_crawler"

/**
 * @class qstats_crawler
 * @brief The qstats_crawler class is responsible for crawling and parsing files to extract statistics.
 */
class qstats_crawler {
   public:
	/**
	 * @brief Default constructor for qstats_crawler.
	 */
	qstats_crawler();

	/**
	 * @brief Destructor for qstats_crawler.
	 */
	~qstats_crawler();

	/**
	 * @enum CRAWL_EVENT
	 * @brief Enumeration for crawl events.
	 */
	enum CRAWL_EVENT {
		CRAWL_START, /**< Crawl start event. */
		CRAWL_STOP	 /**< Crawl stop event. */
	};

	/**
	 * @typedef type_qstats_crawler_crawl_event_cb
	 * @brief Type definition for the crawl event callback function.
	 * @param root_filename The root filename.
	 * @param files The list of files.
	 * @param event The crawl event.
	 */
	typedef std::function<void(const qstring& root_filename, const std::vector<fs::path>& files, CRAWL_EVENT event)> type_qstats_crawler_crawl_event_cb;

	/**
	 * @brief Tries to crawl and parse the specified file.
	 * @param root_filename The root filename.
	 * @param host The host.
	 * @param port The port.
	 * @param event_cb The crawl event callback function.
	 */
	void try_crawl(const qstring& root_filename, const qstring& host, const qstring& port, type_qstats_crawler_crawl_event_cb event_cb);

   private:
	/**
	 * @brief Parses a file and updates the parsed lines count.
	 * @param file The file to parse.
	 * @param parsed_lines The parsed lines count.
	 * @return The result of the parsing operation.
	 */
	int parse_file(fs::path file, int& parsed_lines);

	/**
	 * @brief Parses a line in the current file.
	 * @param current_file The current file being parsed.
	 * @param line The line to parse.
	 * @return The result of the parsing operation.
	 */
	int parse_line(fs::path current_file, const qstring& line);

	/**
	 * @brief Appends count statistics to the values string.
	 * @param list The list of count statistics.
	 * @return The result of the appending operation.
	 */
	int append_count_stats(std::vector<qstring>& list);

	/**
	 * @brief Sends count statistics to the database in batches.
	 * @param current_file The current file being parsed.
	 * @return successfull inserts.
	 */
	int batch_send_count_stats(fs::path current_file);

	/**
	 * @brief Parses open statistics in the current file.
	 * @param current_file The current file being parsed.
	 * @param list The list of open statistics.
	 * @return The result of the parsing operation.
	 */
	int parse_open_stats(fs::path current_file, std::vector<qstring>& list);

	/**
	 * @brief Appends open statistics to the values string.
	 * @param values The values string to append to.
	 * @param list The list of open statistics.
	 * @return The result of the appending operation.
	 */
	int append_open_stats(qstring& values, std::vector<qstring>& list);

	/**
	 * @brief Sends open statistics to the database in batches.
	 * @param current_file The current file being parsed.
	 * @return The result of the batch sending operation.
	 */
	int batch_send_open_stats(fs::path current_file);

	int get_number_of_count_stats();

	/**
	 * @brief replace the \n with \0  at string length
	 * @param str input string
	 * @return status
	 */
	static bool replace_newline_with_zero_at_the_end(char* str);

	struct batch_data_t {
		int count = 0;
		qstring value;
	};
	qpgsql pgsql_client;							   /**< The PostgreSQL client. */
													   //	int count_stats_counter = 0;					   /**< The count statistics counter. */
	int total_records_sent_to_db_through_batching = 0; /**< The total records sent to the database through batching. */
													   //	qstring batch_count_stats_values;				   /**< The batch count statistics values. */
	std::map<qstring, batch_data_t> batches;
	int open_stats_counter = 0;		 /**< The open statistics counter. */
	qstring batch_open_stats_values; /**< The batch open statistics values. */
};

#endif /* qstats_crawler_h */

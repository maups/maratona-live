  //---------------//
 // Author: maups //
//---------------//
#ifndef __BOCA__
#define __BOCA__

#include <vector>
#include <map>
#include <set>
#include <ctime>
#include <sys/time.h>
#include <thread>
#include <opencv2/opencv.hpp>

long long time_in_ms();

typedef enum {UNKNOWN, NO, YES} result_type;

typedef struct {
	std::string institution, name;
	std::vector<std::string> attributes;
} team_descriptor;

typedef struct {
	long long time_penalty, problem_id, timestamp_discovery;
	result_type result;
} submission_descriptor;

typedef struct {
	bool accepted;
	long long time_penalty, failed_submissions, unknown_submissions;
} problem_descriptor;

typedef struct {
	long long solved_problems, time_penalty;
	std::vector<problem_descriptor> problems;
} score_descriptor;

typedef struct {
	std::string attribute, title;
	long long attribute_index, gold, silver, bronze;
} scoreboard_descriptor;

class BOCA_Contest {
	private:
		void get_webcast();
		std::vector<std::string> split(std::string, char);
		std::string remove_accents(std::string);
		void blend_bgra2bgra(cv::Mat &, cv::Mat &);
		void copy_bgra2bgr(cv::Mat &, cv::Mat &, double);
		void copy_bgr2bgr(cv::Mat &, cv::Mat &, double);
		void mergesort(std::vector<long long> &, long long);

	public:
		// contest information
		std::string name;
		long long duration, blind, freeze, fail_penalty, num_problems, num_teams, num_attributes, start_time;
		std::map<std::string, long long> team_to_index;
		std::vector<std::string> index_to_team;
		std::vector<team_descriptor> teams;
		std::vector<std::set<std::string>> attributes;
		std::vector<std::string> attribute_names;

		// submissions and scoreboard
		std::map<long long, std::pair<long long, submission_descriptor>> all_submissions;
		pthread_mutex_t all_submissions_mutex;
		std::vector<std::map<long long, submission_descriptor>> team_submissions;
		std::vector<score_descriptor> scoreboard;
		std::vector<long long> sorted_scoreboard;
		std::vector<std::pair<long long, long long>> first_to_solve;

		// interface
		bool gui_fullHD;
		long long scoreboard_width, scoreboard_row_height, scoreboard_row_space, scoreboard_problem_shift, scoreboard_problem_width;
		cv::Mat gui_scoreboard_header, gui_star, gui_yes, gui_no, gui_unknown, gui_loading[8], gui_title;
		std::vector<cv::Mat> gui_team_name, gui_position[4], gui_problem, gui_scoreboard;
		std::vector<pthread_mutex_t> gui_scoreboard_mutex;
		std::vector<std::map<std::string, cv::Mat>> gui_attribute_images;
		std::vector<long long> gui_attribute_precedence;
		std::map<long long, std::pair<long long, submission_descriptor>>::iterator gui_runlist_init;
		std::vector<scoreboard_descriptor> gui_scoreboard_filters;
		std::vector<cv::Mat> gui_scoreboard_filter_header;

		BOCA_Contest(bool);
		bool update(long long);
		bool draw_main_scoreboard(cv::Mat &, long long, long long);
		void draw_runlist(cv::Mat &);
		void draw_status_bar(cv::Mat &, bool, long long);
		void blend_bgra2bgr(cv::Mat &, cv::Mat &);
};

#endif


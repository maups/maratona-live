  //---------------//
 // Author: maups //
//---------------//
#include <iostream>
#include <sstream>
#include <fstream>
#include <regex>
#include <climits>
#include <sys/stat.h>
#include "boca.hpp"

// I hate std:: and cv::
using namespace std;
using namespace cv;

// rainbow
#define fg_white cv::Scalar(255,255,255,255)
#define fg_black cv::Scalar(0,0,0,255)
#define fg_red cv::Scalar(0,0,64,255)
#define fg_dark_blue cv::Scalar(64,0,0,255)

#define bg_black cv::Scalar(0,0,0,192)
#define bg_gold cv::Scalar(32,165,218,192)
#define bg_silver cv::Scalar(160,160,160,192)
#define bg_bronze cv::Scalar(61,121,166,192)
#define bg_blue cv::Scalar(128,0,0,192)
#define bg_red cv::Scalar(0,0,64,192)
#define bg_green cv::Scalar(0,64,0,192)
#define bg_dark_green cv::Scalar(0,32,0,192)
#define bg_transparent cv::Scalar(0,0,0,0)

// get current time in miliseconds
long long time_in_ms() {
	struct timeval tmp_time;
	gettimeofday(&tmp_time, NULL);
	return tmp_time.tv_sec*1000ll + tmp_time.tv_usec/1000ll;
}

// check if file exists
bool file_exists(string filename) {
	struct stat buffer;   
	return stat(filename.c_str(), &buffer) == 0; 
}

// get BOCA webcast files
void BOCA_Contest::get_webcast() {
	if(system("./get_webcast.sh"))
		throw;
}

// split string into a vector of strings using a character as delimiter
vector<string> BOCA_Contest::split(string to_split, char delimeter) {
	stringstream ss(to_split);
	string s;
	vector<string> r;
	while(getline(ss, s, delimeter))
		r.push_back(s);
	return r;
}

// remove bad characters
string BOCA_Contest::remove_accents(string s) {
	s = regex_replace(s, regex("\\ã"), "a");
	s = regex_replace(s, regex("\\á"), "a");
	s = regex_replace(s, regex("\\â"), "a");
	s = regex_replace(s, regex("\\é"), "e");
	s = regex_replace(s, regex("\\ê"), "e");
	s = regex_replace(s, regex("\\í"), "i");
	s = regex_replace(s, regex("\\õ"), "o");
	s = regex_replace(s, regex("\\ó"), "o");
	s = regex_replace(s, regex("\\ô"), "o");
	s = regex_replace(s, regex("\\ö"), "o");
	s = regex_replace(s, regex("\\ú"), "u");
	s = regex_replace(s, regex("\\ç"), "c");
	s = regex_replace(s, regex("\\ñ"), "n");
	s = regex_replace(s, regex("\\Ã"), "A");
	s = regex_replace(s, regex("\\Á"), "A");
	s = regex_replace(s, regex("\\Â"), "A");
	s = regex_replace(s, regex("\\É"), "E");
	s = regex_replace(s, regex("\\Ê"), "E");
	s = regex_replace(s, regex("\\Í"), "I");
	s = regex_replace(s, regex("\\Õ"), "O");
	s = regex_replace(s, regex("\\Ó"), "O");
	s = regex_replace(s, regex("\\Ô"), "O");
	s = regex_replace(s, regex("\\Ö"), "O");
	s = regex_replace(s, regex("\\Ú"), "U");
	s = regex_replace(s, regex("\\Ç"), "C");
	s = regex_replace(s, regex("\\Ñ"), "N");
	s = regex_replace(s, regex("\\’"), "'");
	s = regex_replace(s, regex("\\”"), "\"");
	s = regex_replace(s, regex("\\´"), "'");
	s = regex_replace(s, regex("\\°"), "o");
	s = regex_replace(s, regex("\\ʖ"), "b");
	s = regex_replace(s, regex("\\¡"), "!");
	return s;	
}

// load file 'contest' from webcast and other configuration files
BOCA_Contest::BOCA_Contest(bool fullHD) {

	  //---------------------//
	 // CONTEST INFORMATION //
	//---------------------//

	string s;
	vector<string> v;
	ifstream fp;

	cerr << "Loading contest information... ";

	// get webcast files
	try {
		get_webcast();
	}
	catch(...) {
		cerr << "Failed to get contest description file.\n";
		throw;
	}

	// parse contest description file
	fp.open("/tmp/contest", ifstream::in);
	if(not fp.is_open()) {
		cerr << "Failed to open contest description file.\n";
		throw;
	}

	// file header
	getline(fp, this->name);
	getline(fp, s);
	v = split(s, '\x1c');
	this->duration = stoi(v[0]);
	this->blind = stoi(v[1]);
	this->freeze = stoi(v[2]);
	this->fail_penalty = stoi(v[3]);
	getline(fp, s);
	v = split(s, '\x1c');
	this->num_teams = stoi(v[0]);
	this->num_problems = stoi(v[1]);

	// team information
	this->teams.resize(this->num_teams);
	this->index_to_team.resize(this->num_teams);
	for(long long i=0; i < this->num_teams; i++) {
		getline(fp, s);
		s = remove_accents(s);
		v = split(s, '\x1c');

		this->team_to_index[v[0]] = i;
		this->index_to_team[i] = v[0];

		team_descriptor team;

		// institution name
		team.institution = v[1];

		// remove institution name from team name
		size_t pos = v[2].find("]");
		if(pos == string::npos || pos+2 >= v[2].size())
			team.name = v[2];
		else
			team.name = v[2].substr(pos+2, string::npos);

		this->teams[i] = team;
	}
	fp.close();

	// parse contest configuration file
	fp.open("../config/contest.txt", ifstream::in);
	if(not fp.is_open()) {
		cerr << "Failed to open contest configuration file.\n";
		throw;
	}

	do {
		getline(fp, s);
	} while(s.size() == 0 || s[0] == '#');
	this->start_time = stoi(s);
	fp.close();

	// parse team attributes
	fp.open("../config/attributes.txt", ifstream::in);
	if(not fp.is_open()) {
		cerr << "Failed to open file with team attributes.\n";
		throw;
	}

	do {
		getline(fp, s);
	} while(s.size() == 0 || s[0] == '#');
	v = split(s, ';');
	this->num_attributes = v.size()-1;
	this->attribute_names.insert(this->attribute_names.end(), v.begin()+1, v.end());
	this->attributes.resize(this->num_attributes);
	while(fp >> s) {
		v = split(s, ';');
		if(this->team_to_index.count(v[0]) != 0) {
			long long i = this->team_to_index[v[0]];
			this->teams[i].attributes.insert(this->teams[i].attributes.end(), v.begin()+1, v.end());
			for(long long j=0; j < this->num_attributes; j++)
				this->attributes[j].insert(v[j+1]);
		}
	}
	fp.close();

	// parse gui configuration file
	fp.open("../config/gui.txt", ifstream::in);
	if(not fp.is_open()) {
		cerr << "Failed to open file with gui configuration.\n";
		throw;
	}

	do {
		getline(fp, s);
	} while(s.size() == 0 || s[0] == '#');
	v = split(s, ';');
	for(auto attribute : v)
		for(long long i=0; i < this->attribute_names.size(); i++)
			if(attribute == this->attribute_names[i]) {
				this->gui_attribute_precedence.push_back(i);
				break;
			}
	while(getline(fp, s)) {
		v = split(s, ';');
		scoreboard_descriptor sd;
		sd.attribute_index = -1;
		for(long long i=0; i < this->attribute_names.size(); i++)
			if(v[0] == this->attribute_names[i]) {
				sd.attribute_index = i;
				break;
			}
		if(sd.attribute_index == -1)
			continue;
		sd.attribute = v[1];
		sd.gold = stoi(v[2]);
		sd.silver = stoi(v[3]);
		sd.bronze = stoi(v[4]);
		sd.title = v[5];

		this->gui_scoreboard_filters.push_back(sd);
	}

	fp.close();

	cerr << "done." << endl;

	  //----------------------------//
	 // SCOREBOARD AND SUBMISSIONS //
	//----------------------------//

	cerr << "Creating empty scoreboard... ";

	// create empty scoreboard
	problem_descriptor pd;
	pd.accepted = false;
	pd.time_penalty = 0;
	pd.failed_submissions = 0;
	pd.unknown_submissions = 0;
	score_descriptor sd;
	sd.solved_problems = 0;
	sd.time_penalty = 0;
	sd.problems.assign(this->num_problems, pd);
	this->scoreboard.assign(this->num_teams, sd);

	// initial scoreboard order
	for(long long i=0; i < this->num_teams; i++)
		this->sorted_scoreboard.push_back(i);

	// create empty lists of team submissions
	this->team_submissions.resize(this->num_teams);
	this->all_submissions_mutex = PTHREAD_MUTEX_INITIALIZER;
	this->gui_runlist_init = this->all_submissions.end();

	// create empty list of first solvers
	this->first_to_solve.assign(this->num_problems, make_pair(LLONG_MAX, -1));

	cerr << "done." << endl;

	  //-----------//
	 // INTERFACE //
	//-----------//

	Mat tmp, tmp2;
	long long width;
	double scale;

	// set resolution
	if(fullHD) {
		this->gui_fullHD = true;
		this->scoreboard_width = 1350;
		this->scoreboard_row_height = 60;
		this->scoreboard_row_space = 3;
	}
	else {
		this->gui_fullHD = false;
		this->scoreboard_width = 900;
		this->scoreboard_row_height = 40;
		this->scoreboard_row_space = 2;
	}
	// discover the width of a problem cell
	// flag pos  name          score ---space for problems---
	// _______________________________________________________
	// |   ||   ||            ||   ||                        |
	// |   ||   ||            ||   ||                        |
	// ‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
	// flag: scoreboard_row_height+scoreboard_row_space
	// pos: scoreboard_row_height+scoreboard_row_space
	// name: 4*scoreboard_row_height+scoreboard_row_space
	// score: scoreboard_row_height+scoreboard_row_space
	// total: 7*scoreboard_row_height+4*scoreboard_row_space
	this->scoreboard_problem_shift = 7*this->scoreboard_row_height+4*this->scoreboard_row_space;
	this->scoreboard_problem_width = min((this->scoreboard_width-this->scoreboard_problem_shift)/this->num_problems-this->scoreboard_row_space, 2*this->scoreboard_row_height);

	// load interface images
	this->gui_star = imread("../assets/star.png", IMREAD_UNCHANGED);
	this->gui_unknown = imread("../assets/question.png", IMREAD_COLOR);
	resize(this->gui_unknown, this->gui_unknown, Size(this->scoreboard_row_height,this->scoreboard_row_height), 0, 0, INTER_CUBIC);
	this->gui_yes = imread("../assets/yes.png", IMREAD_COLOR);
	resize(this->gui_yes, this->gui_yes, Size(this->scoreboard_row_height,this->scoreboard_row_height), 0, 0, INTER_CUBIC);
	this->gui_no = imread("../assets/no.png", IMREAD_COLOR);
	resize(this->gui_no, this->gui_no, Size(this->scoreboard_row_height,this->scoreboard_row_height), 0, 0, INTER_CUBIC);
	for(int i=0; i < 8; i++) {
		this->gui_loading[i] = imread("../assets/loading-"+to_string(i)+".png", IMREAD_COLOR);
		resize(this->gui_loading[i], this->gui_loading[i], Size(this->scoreboard_row_height,this->scoreboard_row_height), 0, 0, INTER_CUBIC);
	}

	// create scoreboard header
	cerr << "Creating scoreboard header... ";

	tmp.create(180, 26*180, CV_8UC4);
	tmp = bg_black;
	putText(tmp, this->name, Point(45,120), FONT_HERSHEY_TRIPLEX, 3.0, fg_white, 3);
	resize(tmp, tmp2, Size(26*90,90), 0, 0, INTER_CUBIC);
	resize(tmp2, this->gui_title, Size((fullHD ? 1920 : 1280)-6*this->scoreboard_row_height,this->scoreboard_row_height), 0, 0, INTER_CUBIC);

	this->gui_scoreboard_header.create(this->scoreboard_row_height, this->scoreboard_width, CV_8UC4);
	this->gui_scoreboard_header = bg_transparent;

	this->gui_scoreboard_filter_header.resize(this->gui_scoreboard_filters.size());
	for(long long i=0; i < this->gui_scoreboard_filters.size(); i++) {
		this->gui_scoreboard_header.copyTo(this->gui_scoreboard_filter_header[i]);

		tmp.create(180, 1098, CV_8UC4);
		tmp = fg_dark_blue;
		width = getTextSize(this->gui_scoreboard_filters[i].title, FONT_HERSHEY_TRIPLEX, 1.8, 3, NULL).width;
		putText(tmp, this->gui_scoreboard_filters[i].title, Point((1098-width)/2,110), FONT_HERSHEY_TRIPLEX, 1.8, fg_white, 3);
		resize(tmp, tmp2, Size(549,90), 0, 0, INTER_CUBIC);
		resize(tmp2, this->gui_scoreboard_filter_header[i].colRange(0,7*this->scoreboard_row_height+2*this->scoreboard_row_space), Size(7*this->scoreboard_row_height+2*this->scoreboard_row_space,this->scoreboard_row_height), 0, 0, INTER_CUBIC);
	}

	/*tmp.create(180, 180, CV_8UC4);
	tmp = fg_black;
	width = getTextSize("POS.", FONT_HERSHEY_TRIPLEX, 1.8, 3, NULL).width;
	putText(tmp, "POS.", Point((180-width)/2,110), FONT_HERSHEY_TRIPLEX, 1.8, fg_white, 3);
	resize(tmp, tmp2, Size(90,90), 0, 0, INTER_CUBIC);
	resize(tmp2, this->gui_scoreboard_header.colRange(this->scoreboard_row_height+this->scoreboard_row_space,2*this->scoreboard_row_height+this->scoreboard_row_space), Size(this->scoreboard_row_height,this->scoreboard_row_height), 0, 0, INTER_CUBIC);

	tmp.create(180, 720, CV_8UC4);
	tmp = fg_black;
	width = getTextSize("EQUIPE", FONT_HERSHEY_TRIPLEX, 1.8, 3, NULL).width;
	putText(tmp, "EQUIPE", Point((720-width)/2,110), FONT_HERSHEY_TRIPLEX, 1.8, fg_white, 3);
	resize(tmp, tmp2, Size(360,90), 0, 0, INTER_CUBIC);
	resize(tmp2, this->gui_scoreboard_header.colRange(2*this->scoreboard_row_height+2*this->scoreboard_row_space,6*this->scoreboard_row_height+2*this->scoreboard_row_space), Size(4*this->scoreboard_row_height,this->scoreboard_row_height), 0, 0, INTER_CUBIC);*/

	tmp.create(180, 180, CV_8UC4);
	tmp = fg_black;
	width = getTextSize("#", FONT_HERSHEY_TRIPLEX, 1.8, 3, NULL).width;
	putText(tmp, "#", Point((180-width)/2,90), FONT_HERSHEY_TRIPLEX, 1.8, fg_white, 3);
	width = getTextSize("TEMPO", FONT_HERSHEY_DUPLEX, 1.2, 2, NULL).width;
	putText(tmp, "TEMPO", Point((180-width)/2, 155), FONT_HERSHEY_DUPLEX, 1.2, fg_white, 2);
	resize(tmp, tmp2, Size(90,90), 0, 0, INTER_CUBIC);
	resize(tmp2, this->gui_scoreboard_header.colRange(6*this->scoreboard_row_height+3*this->scoreboard_row_space,7*this->scoreboard_row_height+3*this->scoreboard_row_space), Size(this->scoreboard_row_height,this->scoreboard_row_height), 0, 0, INTER_CUBIC);

	for(long long i=0; i < this->num_problems; i++) {
		tmp.create(180, 180*this->scoreboard_problem_width/this->scoreboard_row_height, CV_8UC4);
		tmp = fg_black;
		string s = "";
		s += (char)('A'+i);
		width = getTextSize(s, FONT_HERSHEY_TRIPLEX, 1.8, 3, NULL).width;
		putText(tmp, s, Point((180*this->scoreboard_problem_width/this->scoreboard_row_height-width)/2,110), FONT_HERSHEY_TRIPLEX, 1.8, fg_white, 3);
		resize(tmp, tmp2, Size(90*this->scoreboard_problem_width/this->scoreboard_row_height,90), 0, 0, INTER_CUBIC);
		resize(tmp2, this->gui_scoreboard_header.colRange(this->scoreboard_problem_shift+i*(this->scoreboard_problem_width+this->scoreboard_row_space),this->scoreboard_problem_shift+i*(this->scoreboard_problem_width+this->scoreboard_row_space)+this->scoreboard_problem_width), Size(this->scoreboard_problem_width,this->scoreboard_row_height), 0, 0, INTER_CUBIC);
	}
	this->gui_scoreboard_header.colRange(this->scoreboard_problem_shift+this->num_problems*(this->scoreboard_problem_width+this->scoreboard_row_space),this->scoreboard_width) = fg_black;

	cerr << "done." << endl;

	// precompute team names
	cerr << "Rendering team names... ";

	this->gui_team_name.resize(this->num_teams);
	tmp.create(180, 720, CV_8UC4);
	for(long long i=0; i < this->num_teams; i++) {
		tmp = bg_black;
		// write institution
		width = getTextSize(this->teams[i].institution, FONT_HERSHEY_TRIPLEX, 3.0, 3, NULL).width;
		//scale = 2100.0/max(700ll, width);
		scale = 2040.0/max(680ll, width);
		putText(tmp, this->teams[i].institution, Point(/*710-min(700ll, width)*/700-min(680ll, width), 90), FONT_HERSHEY_TRIPLEX, scale, fg_white, 3);
		// write team name
		width = getTextSize(this->teams[i].name, FONT_HERSHEY_DUPLEX, 1.5, 3, NULL).width;
		//scale = 1050.0/max(700ll, width);
		scale = 1020.0/max(680ll, width);
		putText(tmp, this->teams[i].name, Point(/*710-min(700ll, width)*/700-min(680ll, width), 155), FONT_HERSHEY_DUPLEX, scale, fg_white, 3);

		resize(tmp, tmp2, Size(360, 90), 0, 0, INTER_CUBIC);
		resize(tmp2, this->gui_team_name[i], Size(4*this->scoreboard_row_height, this->scoreboard_row_height), 0, 0, INTER_CUBIC);
	}

	cerr << "done." << endl;

	// load attribute images
	cerr << "Loading attribute images... ";

	this->gui_attribute_images.resize(this->num_attributes);
	for(int i=0; i < this->num_attributes; i++) {
		string path = "../assets/"+this->attribute_names[i]+"/";
		for(auto attribute : this->attributes[i]) {
			string filename = path+attribute+".png";

			if(!file_exists(filename))
				filename = "../assets/blank.png";

			tmp = imread(filename, IMREAD_UNCHANGED);
			resize(tmp, this->gui_attribute_images[i][attribute], Size(this->scoreboard_row_height, this->scoreboard_row_height), 0, 0, INTER_CUBIC);
		}
	}

	cerr << "done." << endl;

	// add position (max gold medals; max silver medals; max bronze medals, max teams)
	cerr << "Rendering position... ";

	for(long long i=0; i < 4; i++)
		this->gui_position[i].resize(this->num_teams);
	tmp.create(180, 180, CV_8UC4);
	for(long long i=0; i < this->num_teams; i++) {
		s = to_string(i+1);
		scale = 3.0;
		width = getTextSize(s, FONT_HERSHEY_TRIPLEX, scale, 3, NULL).width;
		while(width > 160) {
			scale *= 0.9;
			width = getTextSize(s, FONT_HERSHEY_TRIPLEX, scale, 3, NULL).width;
		}

		for(long long j=0; j < 4; j++) {
			tmp = (j==0 ? bg_gold : (j==1 ? bg_silver : (j==2 ? bg_bronze : bg_black)));
			putText(tmp, s, Point((180-width)/2, 120), FONT_HERSHEY_TRIPLEX, scale, fg_white, 3);
			resize(tmp, tmp2, Size(90, 90), 0, 0, INTER_CUBIC);
			resize(tmp2, this->gui_position[j][i], Size(this->scoreboard_row_height, this->scoreboard_row_height), 0, 0, INTER_CUBIC);
		}
	}

	cerr << "done." << endl;

	// add problem letter
	cerr << "Rendering problem letters... ";

	this->gui_problem.resize(this->num_problems);
	tmp.create(180, 180, CV_8UC4);
	for(long long i=0; i < this->num_problems; i++) {
		s = "A";
		s[0] += i;
		tmp = bg_black;
		width = getTextSize(s, FONT_HERSHEY_TRIPLEX, 3.0, 3, NULL).width;
		putText(tmp, s, Point((180-width)/2, 120), FONT_HERSHEY_TRIPLEX, scale, fg_white, 3);
		resize(tmp, tmp2, Size(90, 90), 0, 0, INTER_CUBIC);
		resize(tmp2, this->gui_problem[i], Size(this->scoreboard_row_height, this->scoreboard_row_height), 0, 0, INTER_CUBIC);
	}

	cerr << "done." << endl;

	// render empty scoreboard
	cerr << "Rendering empty scoreboard... ";

	Mat scoreline(this->scoreboard_row_height, this->scoreboard_width, CV_8UC4);
	scoreline = bg_transparent;

	this->gui_position[3][0].copyTo(scoreline.colRange(this->scoreboard_row_height+this->scoreboard_row_space,2*this->scoreboard_row_height+this->scoreboard_row_space));

	tmp.create(180, 180, CV_8UC4);
	tmp = bg_black;
	width = getTextSize("0", FONT_HERSHEY_TRIPLEX, 3.0, 3, NULL).width;
	putText(tmp, "0", Point((180-width)/2, 90), FONT_HERSHEY_TRIPLEX, 3.0, fg_white, 3);
	width = getTextSize("0", FONT_HERSHEY_DUPLEX, 1.5, 3, NULL).width;
	putText(tmp, "0", Point((180-width)/2, 155), FONT_HERSHEY_DUPLEX, 1.5, fg_white, 3);
	resize(tmp, tmp2, Size(90, 90), 0, 0, INTER_CUBIC);
	resize(tmp2, scoreline.colRange(6*this->scoreboard_row_height+3*this->scoreboard_row_space,7*this->scoreboard_row_height+3*this->scoreboard_row_space), Size(this->scoreboard_row_height, this->scoreboard_row_height), 0, 0, INTER_CUBIC);

	tmp.create(180, 180*this->scoreboard_problem_width/this->scoreboard_row_height, CV_8UC4);
	tmp = bg_black;
	width = getTextSize(".", FONT_HERSHEY_TRIPLEX, 3.0, 3, NULL).width;
	putText(tmp,".", Point((180*this->scoreboard_problem_width/this->scoreboard_row_height-width)/2,90), FONT_HERSHEY_TRIPLEX, 3.0, fg_white, 3);
	resize(tmp, tmp2, Size(90*this->scoreboard_problem_width/this->scoreboard_row_height,90), 0, 0, INTER_CUBIC);
	resize(tmp2, tmp, Size(this->scoreboard_problem_width,this->scoreboard_row_height), 0, 0, INTER_CUBIC);
	for(long long i=0; i < this->num_problems; i++)
		tmp.copyTo(scoreline.colRange(this->scoreboard_problem_shift+i*(this->scoreboard_problem_width+this->scoreboard_row_space),this->scoreboard_problem_shift+i*(this->scoreboard_problem_width+this->scoreboard_row_space)+this->scoreboard_problem_width));
	scoreline.colRange(this->scoreboard_problem_shift+this->num_problems*(this->scoreboard_problem_width+this->scoreboard_row_space),this->scoreboard_width) = bg_black;

	this->gui_scoreboard.resize(this->num_teams);
	for(long long i=0; i < this->num_teams; i++) {
		scoreline.copyTo(this->gui_scoreboard[i]);
		this->gui_team_name[i].copyTo(this->gui_scoreboard[i].colRange(2*this->scoreboard_row_height+2*this->scoreboard_row_space,6*this->scoreboard_row_height+2*this->scoreboard_row_space));

		if(this->teams[i].attributes.size() == this->num_attributes) {
			for(long long j : this->gui_attribute_precedence)
				if(this->teams[i].attributes[j] != "none") {
					this->gui_attribute_images[j][this->teams[i].attributes[j]].copyTo(this->gui_scoreboard[i].colRange(0,this->scoreboard_row_height));
					break;
				}
		}
	}

	this->gui_scoreboard_mutex.assign(this->num_teams, PTHREAD_MUTEX_INITIALIZER);

	cerr << "done." << endl;
}

void BOCA_Contest::blend_bgra2bgra(Mat &bgra, Mat &out) {
	for(int y=0; y < bgra.rows; y++)
		for(int x=0; x < bgra.cols; x++) {
			double w = bgra.at<Vec4b>(y,x)[3]/255.0;
			uchar tmp = max(bgra.at<Vec4b>(y,x)[3],out.at<Vec4b>(y,x)[3]);
			out.at<Vec4b>(y,x) = (1.0-w)*out.at<Vec4b>(y,x) + w*bgra.at<Vec4b>(y,x);
			out.at<Vec4b>(y,x)[3] = tmp;
	}
}

void BOCA_Contest::blend_bgra2bgr(Mat &bgra, Mat &out) {
	for(int y=0; y < bgra.rows; y++)
		for(int x=0; x < bgra.cols; x++) {
			double w = bgra.at<Vec4b>(y,x)[3]/255.0;
			Vec3b bgr = Vec3b(bgra.at<Vec4b>(y,x)[0], bgra.at<Vec4b>(y,x)[1], bgra.at<Vec4b>(y,x)[2]);
			out.at<Vec3b>(y,x) = (1.0-w)*out.at<Vec3b>(y,x) + w*bgr;
	}
}

void BOCA_Contest::mergesort(vector<long long> &v, long long filter_index) {
	if(v.size() == 1)
		return;

	long long half_size = v.size()/2;
	vector<long long> split_lo(v.begin(), v.begin()+half_size);
	vector<long long> split_hi(v.begin()+half_size, v.end());

	this->mergesort(split_lo, filter_index);
	this->mergesort(split_hi, filter_index);

	for(long long i=0, l=0, h=0; i < v.size(); i++) {
		if(l < split_lo.size()) {
			if(h < split_hi.size()) {
				long long j = this->sorted_scoreboard[l];
				if(this->teams[j].attributes.size() == this->num_attributes && this->teams[j].attributes[this->gui_scoreboard_filters[filter_index].attribute_index] == this->gui_scoreboard_filters[filter_index].attribute) {
					long long k = this->sorted_scoreboard[h];
					if(this->teams[k].attributes.size() == this->num_attributes && this->teams[k].attributes[this->gui_scoreboard_filters[filter_index].attribute_index] == this->gui_scoreboard_filters[filter_index].attribute) {
						if(this->scoreboard[j].solved_problems > this->scoreboard[k].solved_problems || this->scoreboard[j].solved_problems == this->scoreboard[k].solved_problems && this->scoreboard[j].time_penalty < this->scoreboard[k].time_penalty)
							v[i] = split_lo[l++];
						else
							v[i] = split_hi[h++];
					}
					else
						v[i] = split_lo[l++];
				}
				else
					v[i] = split_hi[h++];
			}
			else
				v[i] = split_lo[l++];
		}
		else
			v[i] = split_hi[h++];
	}
}

/*		for(long long i=0, f=0; i < this->num_teams; i++) {
			long long j = this->sorted_scoreboard[i];

			if(this->teams[j].attributes.size() == this->num_attributes && this->teams[j].attributes[gui_scoreboard_filters[filter_index].attribute_index] == gui_scoreboard_filters[filter_index].attribute) {
				swap(this->sorted_scoreboard[i], this->sorted_scoreboard[f]);
				long long k=f;
				while(k > 0) {
					long long jprev = this->sorted_scoreboard[k-1];
					if(this->teams[jprev].attributes.size() != this->num_attributes || this->teams[jprev].attributes[gui_scoreboard_filters[filter_index].attribute_index] != gui_scoreboard_filters[filter_index].attribute || this->scoreboard[j].solved_problems > this->scoreboard[jprev].solved_problems || this->scoreboard[j].solved_problems == this->scoreboard[jprev].solved_problems && this->scoreboard[j].time_penalty < this->scoreboard[jprev].time_penalty) {
						swap(this->sorted_scoreboard[k], this->sorted_scoreboard[k-1]);
					}
					else
						break;
					k--;
				}
				f++;
			}
		}*/

// update scoreboard and submissions
bool BOCA_Contest::update(long long filter_index) {
	static long long last_filter_index = 0;

	string s;
	vector<string> v;
	ifstream fp;

	// get webcast files
	try {
		get_webcast();
	}
	catch(...) {
		cerr << "Failed to get contest description file.\n";
		throw;
	}

	// open file with list of runs
	fp.open("/tmp/runs", ifstream::in);
	if(not fp.is_open()) {
		cerr << "Failed to open contest description file.\n";
		throw;
	}

	// parse submissions
	long long init = -1;
	vector<bool> changed(this->num_teams, false);
	long long t = time_in_ms();
	while(fp >> s) {
		v = split(s, '\x1c');

		submission_descriptor submission;
		submission.time_penalty = stoi(v[1]);

		// drop overtime submissions and submissions from unknown teams
		if(submission.time_penalty >= this->duration || this->team_to_index.count(v[2]) == 0)
			continue;

		long long submission_id = stoi(v[0]), team_id = this->team_to_index[v[2]];

		submission.result = (v[4][0] == '?' ? UNKNOWN : (v[4][0] == 'N' ? NO : YES));
		submission.problem_id = v[3][0]-'A';
		submission.timestamp_discovery = t;

		// new submission
		if(this->team_submissions[team_id].count(submission_id) == 0) {
			submission.result = UNKNOWN;
			this->team_submissions[team_id][submission_id] = submission;
			changed[team_id] = true;

			pthread_mutex_lock(&this->all_submissions_mutex);
			this->all_submissions[submission_id] = make_pair(team_id, this->team_submissions[team_id][submission_id]);
			pthread_mutex_unlock(&this->all_submissions_mutex);
		}
		// keep submission result unknown for at least 30 seconds
		else if(submission.result != this->team_submissions[team_id][submission_id].result && t-this->team_submissions[team_id][submission_id].timestamp_discovery >= 30000ll) {
			this->team_submissions[team_id][submission_id].result = submission.result;
			changed[team_id] = true;

			pthread_mutex_lock(&this->all_submissions_mutex);
			this->all_submissions[submission_id] = make_pair(team_id, this->team_submissions[team_id][submission_id]);
			pthread_mutex_unlock(&this->all_submissions_mutex);
		}

		// first submissions of the last 2 minutes
		if(t-this->all_submissions[submission_id].second.timestamp_discovery < 120000ll)
			init = submission_id;
	}
	fp.close();
	this->gui_runlist_init = this->all_submissions.find(init);

	// update scoreboard
	Mat tmp, tmp2;
	long long width;
	double scale;

	problem_descriptor pd;
	pd.accepted = false;
	pd.time_penalty = 0;
	pd.failed_submissions = 0;
	pd.unknown_submissions = 0;

	bool flag = false;
	if(filter_index != last_filter_index) {
		last_filter_index = filter_index;
		flag = true;
		for(long long i=0, f=0; i < this->num_teams; i++) {
			long long j = this->sorted_scoreboard[i];

			if(this->teams[j].attributes.size() == this->num_attributes && this->teams[j].attributes[gui_scoreboard_filters[filter_index].attribute_index] == gui_scoreboard_filters[filter_index].attribute) {
				swap(this->sorted_scoreboard[i], this->sorted_scoreboard[f]);
				long long k=f;
				while(k > 0) {
					long long jprev = this->sorted_scoreboard[k-1];
					if(this->teams[jprev].attributes.size() != this->num_attributes || this->teams[jprev].attributes[gui_scoreboard_filters[filter_index].attribute_index] != gui_scoreboard_filters[filter_index].attribute || this->scoreboard[j].solved_problems > this->scoreboard[jprev].solved_problems || this->scoreboard[j].solved_problems == this->scoreboard[jprev].solved_problems && this->scoreboard[j].time_penalty < this->scoreboard[jprev].time_penalty || this->scoreboard[j].solved_problems == this->scoreboard[jprev].solved_problems && this->scoreboard[j].time_penalty == this->scoreboard[jprev].time_penalty && this->index_to_team[j] < this->index_to_team[jprev]) {
						swap(this->sorted_scoreboard[k], this->sorted_scoreboard[k-1]);
					}
					else
						break;
					k--;
				}
				f++;
			}
		}
		//cout << "hu" << endl;
		//this->mergesort(this->sorted_scoreboard, filter_index);
		//cout << "hu" << endl;
	}

	for(long long i=0; i < this->num_teams; i++) {
		long long j = this->sorted_scoreboard[i];
		// if team has new submission or new result
		if(changed[j]) {
			// reset team scoreboard
			long long solved_problems=0, time_penalty=0;
			this->scoreboard[j].problems.assign(this->num_problems, pd);
			// look every team submission
			for(auto it = this->team_submissions[j].begin(); it != this->team_submissions[j].end(); it++) {
				// ignore if the problem was accepted before
				if(not this->scoreboard[j].problems[it->second.problem_id].accepted) {
					// if accepted
					if(it->second.result == YES) {
						// first to solve?
						this->first_to_solve[it->second.problem_id] = min(this->first_to_solve[it->second.problem_id], make_pair(it->first, j));

						this->scoreboard[j].problems[it->second.problem_id].accepted = true;
						this->scoreboard[j].problems[it->second.problem_id].time_penalty = it->second.time_penalty;
						solved_problems++;
						time_penalty += it->second.time_penalty+this->fail_penalty*this->scoreboard[j].problems[it->second.problem_id].failed_submissions;
					}
					// if not accepted
					else if(it->second.result == NO) {
						this->scoreboard[j].problems[it->second.problem_id].failed_submissions++;
					}
					// if in queue
					else {
						this->scoreboard[j].problems[it->second.problem_id].unknown_submissions++;
					}
				}
			}

			// if team score changed, update scoreboard order
			if(solved_problems != this->scoreboard[j].solved_problems || time_penalty != this->scoreboard[j].time_penalty) {
				this->scoreboard[j].solved_problems = solved_problems;
				this->scoreboard[j].time_penalty = time_penalty;
				// insertion sort (faster when only a few teams are updated)
				if(this->teams[j].attributes.size() == this->num_attributes && this->teams[j].attributes[gui_scoreboard_filters[filter_index].attribute_index] == gui_scoreboard_filters[filter_index].attribute) {
					long long k=i;
					while(k > 0) {
						long long jprev = this->sorted_scoreboard[k-1];
						if(this->teams[jprev].attributes.size() != this->num_attributes || this->teams[jprev].attributes[gui_scoreboard_filters[filter_index].attribute_index] != gui_scoreboard_filters[filter_index].attribute || this->scoreboard[j].solved_problems > this->scoreboard[jprev].solved_problems || this->scoreboard[j].solved_problems == this->scoreboard[jprev].solved_problems && this->scoreboard[j].time_penalty < this->scoreboard[jprev].time_penalty || this->scoreboard[j].solved_problems == this->scoreboard[jprev].solved_problems && this->scoreboard[j].time_penalty == this->scoreboard[jprev].time_penalty && this->index_to_team[j] < this->index_to_team[jprev] || this->scoreboard[j].solved_problems == this->scoreboard[jprev].solved_problems && this->scoreboard[j].time_penalty == this->scoreboard[jprev].time_penalty && this->index_to_team[j] < this->index_to_team[jprev]) {
							swap(this->sorted_scoreboard[k], this->sorted_scoreboard[k-1]);
							flag = true;
						}
						else
							break;
						k--;
					}
				}

				// render new score
				tmp.create(180, 180, CV_8UC4);
				tmp = bg_black;
				width = getTextSize(to_string(solved_problems), FONT_HERSHEY_TRIPLEX, 3.0, 3, NULL).width;
				putText(tmp, to_string(solved_problems), Point((180-width)/2, 90), FONT_HERSHEY_TRIPLEX, 3.0, fg_white, 3);
				width = getTextSize(to_string(time_penalty), FONT_HERSHEY_DUPLEX, 1.5, 3, NULL).width;
				putText(tmp, to_string(time_penalty), Point((180-width)/2, 155), FONT_HERSHEY_DUPLEX, 1.5, fg_white, 3);
				resize(tmp, tmp2, Size(90, 90), 0, 0, INTER_CUBIC);
				pthread_mutex_lock(&this->gui_scoreboard_mutex[j]);
				resize(tmp2, this->gui_scoreboard[j].colRange(6*this->scoreboard_row_height+3*this->scoreboard_row_space,7*this->scoreboard_row_height+3*this->scoreboard_row_space), Size(this->scoreboard_row_height, this->scoreboard_row_height), 0, 0, INTER_CUBIC);
				pthread_mutex_unlock(&this->gui_scoreboard_mutex[j]);
			}
		}
	}
	// second loop is required to solve first_to_solve and position issues
	for(long long i=0; i < this->num_teams; i++) {
		long long j = this->sorted_scoreboard[i];
		if(flag) {
			pthread_mutex_lock(&this->gui_scoreboard_mutex[j]);
			if(this->teams[j].attributes.size() == this->num_attributes && this->teams[j].attributes[gui_scoreboard_filters[filter_index].attribute_index] == gui_scoreboard_filters[filter_index].attribute) {
				if(i > 0 && this->scoreboard[j].solved_problems == this->scoreboard[this->sorted_scoreboard[i-1]].solved_problems && this->scoreboard[j].time_penalty == this->scoreboard[this->sorted_scoreboard[i-1]].time_penalty) {
					Mat roi = this->gui_scoreboard[this->sorted_scoreboard[i-1]].colRange(this->scoreboard_row_height+this->scoreboard_row_space,2*this->scoreboard_row_height+this->scoreboard_row_space);
					roi.copyTo(this->gui_scoreboard[j].colRange(this->scoreboard_row_height+this->scoreboard_row_space,2*this->scoreboard_row_height+this->scoreboard_row_space));
				}
				else {
					long long k=3;
					if(this->scoreboard[j].solved_problems > 0) {
						if(i < gui_scoreboard_filters[filter_index].gold)
							k = 0;
						else if(i < gui_scoreboard_filters[filter_index].gold+gui_scoreboard_filters[filter_index].silver)
							k = 1;
						else if(i < gui_scoreboard_filters[filter_index].gold+gui_scoreboard_filters[filter_index].silver+gui_scoreboard_filters[filter_index].bronze)
							k = 2;
					}
					this->gui_position[k][i].copyTo(this->gui_scoreboard[j].colRange(this->scoreboard_row_height+this->scoreboard_row_space,2*this->scoreboard_row_height+this->scoreboard_row_space));
				}
			}
			else {
				Mat roi = this->gui_scoreboard[j].colRange(this->scoreboard_row_height+this->scoreboard_row_space,2*this->scoreboard_row_height+this->scoreboard_row_space);
				roi = bg_black;
			}
				
			pthread_mutex_unlock(&this->gui_scoreboard_mutex[j]);
		}
		// if team has new submission or new result
		if(changed[j]) {
			// render new problem results
			tmp.create(180, 180*this->scoreboard_problem_width/this->scoreboard_row_height, CV_8UC4);
			for(long long k=0; k < this->num_problems; k++)
				if(this->scoreboard[j].problems[k].accepted || this->scoreboard[j].problems[k].failed_submissions || this->scoreboard[j].problems[k].unknown_submissions) {
					string txt1, txt2;
					if(this->scoreboard[j].problems[k].accepted) {
						tmp = (j == this->first_to_solve[k].second ? bg_dark_green : bg_green);
						txt1 = "+"+(this->scoreboard[j].problems[k].failed_submissions > 0 ? to_string(this->scoreboard[j].problems[k].failed_submissions) : "");
						txt2 = to_string(this->scoreboard[j].problems[k].time_penalty);
					} else if(this->scoreboard[j].problems[k].unknown_submissions > 0) {
						tmp = bg_blue;
						txt1 = "?";
						txt2 = "("+to_string(this->scoreboard[j].problems[k].unknown_submissions)+")";
					} else if(this->scoreboard[j].problems[k].failed_submissions > 0) {
						tmp = bg_red;
						txt1 = "x";
						txt2 = "("+to_string(this->scoreboard[j].problems[k].failed_submissions)+")";
					}

					scale = 3.0;
					width = getTextSize(txt1, FONT_HERSHEY_TRIPLEX, scale, 3, NULL).width;
					while(width > (180*this->scoreboard_problem_width/this->scoreboard_row_height)*0.8) {
						scale *= 0.9;
						width = getTextSize(txt1, FONT_HERSHEY_TRIPLEX, scale, 3, NULL).width;
					}
					putText(tmp, txt1, Point((180*this->scoreboard_problem_width/this->scoreboard_row_height-width)/2, 90), FONT_HERSHEY_TRIPLEX, scale, fg_white, 3);

					width = getTextSize(txt2, FONT_HERSHEY_DUPLEX, 1.5, 3, NULL).width;
					putText(tmp, txt2, Point((180*this->scoreboard_problem_width/this->scoreboard_row_height-width)/2, 155), FONT_HERSHEY_DUPLEX, 1.5, fg_white, 3);

					if(j == this->first_to_solve[k].second) {
						Mat roi = tmp.colRange(180*this->scoreboard_problem_width/this->scoreboard_row_height-this->gui_star.cols,180*this->scoreboard_problem_width/this->scoreboard_row_height).rowRange(0,this->gui_star.rows);
						this->blend_bgra2bgra(this->gui_star, roi);
					}

					resize(tmp, tmp2, Size(90*this->scoreboard_problem_width/this->scoreboard_row_height,90), 0, 0, INTER_CUBIC);
					pthread_mutex_lock(&this->gui_scoreboard_mutex[j]);
					resize(tmp2, this->gui_scoreboard[j].colRange(this->scoreboard_problem_shift+k*(this->scoreboard_problem_width+this->scoreboard_row_space),this->scoreboard_problem_shift+k*(this->scoreboard_problem_width+this->scoreboard_row_space)+this->scoreboard_problem_width), Size(this->scoreboard_problem_width,this->scoreboard_row_height), 0, 0, INTER_CUBIC);
					pthread_mutex_unlock(&this->gui_scoreboard_mutex[j]);
				}
		}
	}
}

// draw main scoreboard
bool BOCA_Contest::draw_main_scoreboard(Mat &frame, long long ty, long long filter_index) {
	if(this->gui_fullHD)
		assert(frame.rows == 1080 && frame.cols == 1920);
	else
		assert(frame.rows == 720 && frame.cols == 1280);

	Mat roi, roi2;
	long long dx = 9*this->scoreboard_row_height, dy = this->scoreboard_row_height/2+this->scoreboard_row_height+this->scoreboard_row_space;

	roi = frame.colRange(dx,dx+this->scoreboard_width).rowRange(this->scoreboard_row_height/2,this->scoreboard_row_height/2+this->scoreboard_row_height);
	this->blend_bgra2bgr(this->gui_scoreboard_header, roi);
	this->blend_bgra2bgr(this->gui_scoreboard_filter_header[filter_index], roi);

	bool flag = false;
	long long height = 14*this->scoreboard_row_height+13*this->scoreboard_row_space;
	long long i = ty/(this->scoreboard_row_height+this->scoreboard_row_space);
	long long ti = -(ty%(this->scoreboard_row_height+this->scoreboard_row_space));
	while(ti < height) {
		if(i >= this->num_teams)
			break;

		long long j=this->sorted_scoreboard[i];
		if(this->teams[j].attributes.size() == this->num_attributes && this->teams[j].attributes[gui_scoreboard_filters[filter_index].attribute_index] == gui_scoreboard_filters[filter_index].attribute) {
			flag = true;

			pthread_mutex_lock(&this->gui_scoreboard_mutex[j]);
			if(ti < 0) {
				if(abs(ti) < this->scoreboard_row_height) {
					roi = this->gui_scoreboard[j].rowRange(abs(ti),this->scoreboard_row_height);
					roi2 = frame.colRange(dx,dx+this->scoreboard_width).rowRange(dy,dy+(this->scoreboard_row_height+ti));
					this->blend_bgra2bgr(roi, roi2);
				}
			}
			else if(ti+this->scoreboard_row_height > height) {
				roi = this->gui_scoreboard[j].rowRange(0,height-ti);
				roi2 = frame.colRange(dx,dx+this->scoreboard_width).rowRange(dy+ti,dy+height);
				this->blend_bgra2bgr(roi, roi2);
			}
			else {
				roi = frame.colRange(dx,dx+this->scoreboard_width).rowRange(dy+ti,dy+ti+this->scoreboard_row_height);
				this->blend_bgra2bgr(this->gui_scoreboard[j], roi);
			}
			pthread_mutex_unlock(&this->gui_scoreboard_mutex[j]);

			ti += this->scoreboard_row_height+this->scoreboard_row_space;
		}
		i++;
	}

	return flag;
}

void BOCA_Contest::copy_bgra2bgr(Mat &bgra, Mat &out, double fade) {
	for(int y=0; y < bgra.rows; y++)
		for(int x=0; x < bgra.cols; x++)
			out.at<Vec3b>(y,x) = (1.0-fade)*out.at<Vec3b>(y,x) + fade*Vec3b(bgra.at<Vec4b>(y,x)[0], bgra.at<Vec4b>(y,x)[1], bgra.at<Vec4b>(y,x)[2]);
}

void BOCA_Contest::copy_bgr2bgr(Mat &bgr, Mat &out, double fade) {
	for(int y=0; y < bgr.rows; y++)
		for(int x=0; x < bgr.cols; x++)
			out.at<Vec3b>(y,x) = (1.0-fade)*out.at<Vec3b>(y,x) + fade*bgr.at<Vec3b>(y,x);
}

// draw runlist
void BOCA_Contest::draw_runlist(Mat &frame) {
	if(this->gui_fullHD)
		assert(frame.rows == 1080 && frame.cols == 1920);
	else
		assert(frame.rows == 720 && frame.cols == 1280);

	Mat roi, roi2;
	long long t = time_in_ms();
	long long dx = this->scoreboard_row_height/2, dy = frame.rows-17*this->scoreboard_row_height-this->scoreboard_row_height/2, count = 0;

	pthread_mutex_lock(&this->all_submissions_mutex);
	for(auto it = this->all_submissions.begin(); it != this->all_submissions.end() && count < 16; it++)
		if((t-it->second.second.timestamp_discovery)/1000ll < 60)
			count++;

	if(count) {
		long long i=0;
		for(auto it = this->gui_runlist_init; it != this->all_submissions.end() && i < 16; it++)
			if((t-it->second.second.timestamp_discovery)/1000ll < 60) {
				long long j = it->second.first;
				double alpha=max(0.0,min(1.0,((t-it->second.second.timestamp_discovery)/1000ll < 45ll ? 1.0 : 1.0-(1.0/15.0)*((t-it->second.second.timestamp_discovery)/1000.0-45.0))));
				pthread_mutex_lock(&this->gui_scoreboard_mutex[j]);
				roi = this->gui_scoreboard[j].colRange(this->scoreboard_row_height+this->scoreboard_row_space,2*this->scoreboard_row_height+this->scoreboard_row_space);
				roi2 = frame.colRange(dx,dx+this->scoreboard_row_height).rowRange(dy+(16-count+i)*this->scoreboard_row_height,dy+(16-count+i+1)*this->scoreboard_row_height);
				this->copy_bgra2bgr(roi, roi2, alpha);
				roi = this->gui_scoreboard[j].colRange(2*this->scoreboard_row_height+2*this->scoreboard_row_space,6*this->scoreboard_row_height+2*this->scoreboard_row_space);
				roi2 = frame.colRange(dx+this->scoreboard_row_height,dx+5*this->scoreboard_row_height).rowRange(dy+(16-count+i)*this->scoreboard_row_height,dy+(16-count+i+1)*this->scoreboard_row_height);
				this->copy_bgra2bgr(roi, roi2, alpha);
				roi = frame.colRange(dx+5*this->scoreboard_row_height,dx+6*this->scoreboard_row_height).rowRange(dy+(16-count+i)*this->scoreboard_row_height,dy+(16-count+i+1)*this->scoreboard_row_height);
				this->copy_bgra2bgr(this->gui_problem[it->second.second.problem_id], roi, alpha);
				roi = frame.colRange(dx+6*this->scoreboard_row_height,dx+7*this->scoreboard_row_height).rowRange(dy+(16-count+i)*this->scoreboard_row_height,dy+(16-count+i+1)*this->scoreboard_row_height);
				if(it->second.second.result == YES)
					this->copy_bgr2bgr(this->gui_yes, roi, alpha);
				else if(it->second.second.result == NO)
					this->copy_bgr2bgr(this->gui_no, roi, alpha);
				else if(it->second.second.time_penalty >= this->freeze)
					this->copy_bgr2bgr(this->gui_unknown, roi, alpha);
				else
					this->copy_bgr2bgr(this->gui_loading[((t-it->second.second.timestamp_discovery)/50ll)%8], roi, alpha);
				pthread_mutex_unlock(&this->gui_scoreboard_mutex[j]);
				i++;
			}
	}
	pthread_mutex_unlock(&this->all_submissions_mutex);
}

void BOCA_Contest::draw_status_bar(Mat &frame, bool show_scoreboard, long long filter_index) {
	static Mat bar(this->scoreboard_row_height, (this->gui_fullHD ? 1920 : 1280), CV_8UC4, bg_transparent);
	static long long tinit = -1, filter_index_inuse = -1;

	long long t = time(0);
	string str = (t < this->start_time ? "-" : "");
	long long h = llabs(t-this->start_time)/3600, m = llabs(t-this->start_time)%3600/60, s = llabs(t-this->start_time)%60;
	str += (h < 10 ? "0" : "");
	str += to_string(h);
	str += (m < 10 ? ":0" : ":");
	str += to_string(m);
	str += (s < 10 ? ":0" : ":");
	str += to_string(s);

	Mat tmp(180, 1080, CV_8UC4), tmp2;
	long long width = getTextSize(str, FONT_HERSHEY_TRIPLEX, 3.0, 3, NULL).width;
	tmp = fg_dark_blue;
	putText(tmp, str, Point((1080-width)/2, 120), FONT_HERSHEY_TRIPLEX, 3.0, fg_white, 3);
	resize(tmp, tmp2, Size(540, 90), 0, 0, INTER_CUBIC);
	resize(tmp2, bar.colRange(0,6*this->scoreboard_row_height), Size(6*this->scoreboard_row_height,this->scoreboard_row_height), 0, 0, INTER_CUBIC);

	if(!show_scoreboard) {
		this->gui_title.copyTo(bar.colRange(6*this->scoreboard_row_height,bar.cols));
		tinit = -1;
	} else {
		t = time_in_ms();
		if(tinit == -1 || filter_index_inuse != filter_index) {
			tinit = t;
			filter_index_inuse = filter_index;
		}

		Mat roi = bar.colRange(6*this->scoreboard_row_height,bar.cols), roi2;
		roi = bg_transparent;

		long long dx = 0;
		for(long long c=0; c < 2; c++) {
			long long i = (t-tinit)/10000;
			for(long long k=0; k < 5; k++) {
				long long j=this->sorted_scoreboard[i*5+k];
				if(this->teams[j].attributes.size() == this->num_attributes && this->teams[j].attributes[gui_scoreboard_filters[filter_index].attribute_index] == gui_scoreboard_filters[filter_index].attribute) {
					long long h=this->scoreboard_row_height;
					if((t-tinit)%10000 < 1000)
						h = this->scoreboard_row_height*((t-tinit)%10000)/1000;
					else if((t-tinit)%10000 > 9000)
						h = this->scoreboard_row_height*(10000-(t-tinit)%10000)/1000;

					pthread_mutex_lock(&this->gui_scoreboard_mutex[j]);
					roi2 = this->gui_scoreboard[j].colRange(2*this->scoreboard_row_height+2*this->scoreboard_row_space,6*this->scoreboard_row_height+2*this->scoreboard_row_space).rowRange(0,h);
					roi2.copyTo(roi.colRange(dx+3*this->scoreboard_row_space,dx+4*this->scoreboard_row_height+3*this->scoreboard_row_space).rowRange(this->scoreboard_row_height-h,this->scoreboard_row_height));
					roi2 = this->gui_scoreboard[j].colRange(this->scoreboard_row_height+this->scoreboard_row_space,2*this->scoreboard_row_height+this->scoreboard_row_space).rowRange(0,h);
					roi2.copyTo(roi.colRange(dx+4*this->scoreboard_row_height+4*this->scoreboard_row_space,dx+5*this->scoreboard_row_height+4*this->scoreboard_row_space).rowRange(this->scoreboard_row_height-h,this->scoreboard_row_height));
					pthread_mutex_unlock(&this->gui_scoreboard_mutex[j]);
					dx += 5*this->scoreboard_row_height+4*this->scoreboard_row_space;
				}
				else
					break;
			}

			if(dx == 0)
				tinit = t;
			else
				break;
		}

	}

	Mat roi = frame.rowRange(frame.rows-this->scoreboard_row_height,frame.rows);
	this->blend_bgra2bgr(bar, roi);
		
}


#include <bits/stdc++.h>
#include <sys/stat.h>
#include <opencv2/opencv.hpp>

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

// check if file exists
bool file_exists(string filename) {
	struct stat buffer;   
	return stat(filename.c_str(), &buffer) == 0; 
}

// split string into a vector of strings using a character as delimiter
vector<string> split(string to_split, char delimeter) {
	stringstream ss(to_split);
	string s;
	vector<string> r;
	while(getline(ss, s, delimeter))
		r.push_back(s);
	return r;
}

// remove bad characters
string remove_accents(string s) {
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

void blend_bgra2bgra(Mat &bgra, Mat &out) {
	for(int y=0; y < bgra.rows; y++)
		for(int x=0; x < bgra.cols; x++) {
			double w = bgra.at<Vec4b>(y,x)[3]/255.0;
			uchar tmp = max(bgra.at<Vec4b>(y,x)[3],out.at<Vec4b>(y,x)[3]);
			out.at<Vec4b>(y,x) = (1.0-w)*out.at<Vec4b>(y,x) + w*bgra.at<Vec4b>(y,x);
			out.at<Vec4b>(y,x)[3] = tmp;
	}
}

void write_large(Mat &img, string s) {
	Mat tmp(330, 330, CV_8UC4, bg_black), tmp2;
	long long width = getTextSize(s, FONT_HERSHEY_TRIPLEX, 5.0, 8, NULL).width;
	putText(tmp, s, Point((330-width)/2, 220), FONT_HERSHEY_TRIPLEX, 5.0, fg_white, 8);
	resize(tmp, tmp2, Size(220,220), 0, 0, INTER_CUBIC);
	resize(tmp2, img, Size(110,110), 0, 0, INTER_CUBIC);
}

void write(Mat &img, string s, long long size) {
	Mat tmp(90, size*3, CV_8UC4, bg_black), tmp2;
	double scale = 3.0;
	long long width = getTextSize(s, FONT_HERSHEY_TRIPLEX, scale, 4, NULL).width;
	while(width > 3*size) {
		scale *= 0.99;
		width = getTextSize(s, FONT_HERSHEY_TRIPLEX, scale, 4, NULL).width;
	}
	putText(tmp, s, Point(0, 66), FONT_HERSHEY_TRIPLEX, scale, fg_white, 4);
	resize(tmp, tmp2, Size(size*2,60), 0, 0, INTER_CUBIC);
	resize(tmp2, img, Size(size,30), 0, 0, INTER_CUBIC);
}

// get BOCA webcast files
pair<int,int> get_cf(string s) {
	string cmd = "./stats.sh "+s+" > stats.tmp";
	system(cmd.c_str());
	int u, v;
	ifstream fp("stats.tmp", ifstream::in);
	fp >> u >> v;
	fp.close();
	system("rm stats.tmp");
	return make_pair(u,v);
}

#define fg_3000 Scalar(0,0,170,255)
#define fg_2400 Scalar(0,0,255,255)
#define fg_2100 Scalar(0,140,255,255)
#define fg_1900 Scalar(170,2,170,255)
#define fg_1600 Scalar(255,64,64,255)
#define fg_1400 Scalar(158,168,3,255)
#define fg_1200 Scalar(0,128,0,255)
#define fg_0001 Scalar(128,128,128,255)
#define fg_0000 Scalar(255,255,255,255)

Scalar get_color(int score) {
	if(score >= 3000) return fg_3000;
	else if(score >= 3000) return fg_3000;
	else if(score >= 2400) return fg_2400;
	else if(score >= 2100) return fg_2100;
	else if(score >= 1900) return fg_1900;
	else if(score >= 1600) return fg_1600;
	else if(score >= 1400) return fg_1400;
	else if(score >= 1200) return fg_1200;
	else if(score >= 0001) return fg_0001;
	else return fg_0000;
}

void write_small(Mat &img, string s, long long size, string handle) {
	Mat tmp(60, size*3, CV_8UC4, bg_black), tmp2;
	long long width = getTextSize(s, FONT_HERSHEY_DUPLEX, 2.0, 2, NULL).width;
	putText(tmp, s, Point(0, 44), FONT_HERSHEY_DUPLEX, 2.0, fg_white, 2);
	if(handle != "") {
		long long dx = width;
		pair<int,int> p = get_cf(handle);
		width = getTextSize(" (", FONT_HERSHEY_DUPLEX, 2.0, 2, NULL).width;
		putText(tmp, " (", Point(dx, 44), FONT_HERSHEY_DUPLEX, 2.0, fg_white, 2);
		dx += width;
		width = getTextSize(handle, FONT_HERSHEY_DUPLEX, 2.0, 3, NULL).width;
		putText(tmp, handle, Point(dx, 44), FONT_HERSHEY_DUPLEX, 2.0, get_color(p.first), 3);
		dx += width;
		if(p.first > 0) {
			width = getTextSize(", ", FONT_HERSHEY_DUPLEX, 2.0, 2, NULL).width;
			putText(tmp, ", ", Point(dx, 44), FONT_HERSHEY_DUPLEX, 2.0, fg_white, 2);
			dx += width;
			width = getTextSize(to_string(p.first), FONT_HERSHEY_DUPLEX, 2.0, 3, NULL).width;
			putText(tmp, to_string(p.first), Point(dx, 44), FONT_HERSHEY_DUPLEX, 2.0, get_color(p.first), 3);
			dx += width;
			width = getTextSize(", max:", FONT_HERSHEY_DUPLEX, 2.0, 2, NULL).width;
			putText(tmp, ", max:", Point(dx, 44), FONT_HERSHEY_DUPLEX, 2.0, fg_white, 2);
			dx += width;
			width = getTextSize(to_string(p.second), FONT_HERSHEY_DUPLEX, 2.0, 3, NULL).width;
			putText(tmp, to_string(p.second), Point(dx, 44), FONT_HERSHEY_DUPLEX, 2.0, get_color(p.second), 3);
			dx += width;
		}
		width = getTextSize(")", FONT_HERSHEY_DUPLEX, 2.0, 2, NULL).width;
		putText(tmp, ")", Point(dx, 44), FONT_HERSHEY_DUPLEX, 2.0, fg_white, 2);
		dx += width;
	}
	resize(tmp, tmp2, Size(size*2,40), 0, 0, INTER_CUBIC);
	resize(tmp2, img, Size(size,20), 0, 0, INTER_CUBIC);
}

int main(int argc, char **argv) {
	Mat icons[7], tmp;
	for(int i=0; i < 7; i++) {
		tmp = imread("../assets/icon-"+to_string(i+1)+".png", IMREAD_UNCHANGED);
		resize(tmp, icons[i], Size(110,110), 0, 0, INTER_CUBIC);
	}

	string s;
	ifstream fp("team_info.txt", ifstream::in);
	while(getline(fp, s)) {
		s = remove_accents(s);

		vector<string> v = split(s, '\x1c');

		if(argc == 2 && v[0] != argv[1])
			continue;

		cout << v[0] << endl;

		Mat stats(860, 1350, CV_8UC4, bg_black), photo, logo;

		string photoname = "../assets/teams/"+v[0]+".png";
		if(file_exists(photoname)) {
			photo = imread(photoname, IMREAD_COLOR);
		}
		else {
			photo = imread("../assets/teams/blank.png", IMREAD_COLOR);
		}
		//cout << photo.channels() << endl;

		string logoname = "../assets/logos/"+v[1]+".png";
		if(file_exists(logoname)) {
			logo = imread(logoname, IMREAD_UNCHANGED);
		}
		else {
			cout << "fuck" << endl;
		}

		Mat roi = stats.colRange(40,240).rowRange(40,240);
		resize(logo, tmp, Size(200, 200), 0, 0, INTER_CUBIC);
		blend_bgra2bgra(tmp, roi);

		Mat text;
		write(text, v[4], 720);
		roi = stats.colRange(280,1000).rowRange(60,90);
		text.copyTo(roi);

		write_small(text, v[5], 720, v[6]);
		roi = stats.colRange(280,1000).rowRange(110,130);
		text.copyTo(roi);

		write_small(text, v[7], 720, v[8]);
		roi = stats.colRange(280,1000).rowRange(140,160);
		text.copyTo(roi);

		write_small(text, v[9], 720, v[10]);
		roi = stats.colRange(280,1000).rowRange(170,190);
		text.copyTo(roi);

		write_small(text, v[11]+" (coach)", 720, "");
		roi = stats.colRange(280,1000).rowRange(200,220);
		text.copyTo(roi);

		resize(photo, tmp, Size(960, 540), 0, 0, INTER_CUBIC);
		for(int i=0; i < 540; i++)
			for(int j=0; j < 960; j++)
				stats.at<Vec4b>(280+i,40+j) = Vec4b(tmp.at<Vec3b>(i,j)[0],tmp.at<Vec3b>(i,j)[1],tmp.at<Vec3b>(i,j)[2],255);

		for(int i=0; i < 7; i++) {
			roi = stats.colRange(1060,1140).rowRange(45+i*110,155+i*110);
			blend_bgra2bgra(icons[i], roi);

			write_large(text, v[12+i]);
			roi = stats.colRange(1180,1290).rowRange(45+i*110,155+i*110);
			text.copyTo(roi);
		}
			

		//imshow("stats", stats);
		//waitKey(0);

		imwrite("../assets/stats/"+v[0]+".png", stats);

		//break;
	}
}


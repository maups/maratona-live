  //---------------//
 // Author: maups //
//---------------//
#include <bits/stdc++.h>
#include "boca.hpp"
#include <atomic>
#include <thread>
#include <unistd.h>

using namespace std;

BOCA_Contest *contest;

bool fullhd = false;
atomic<bool> system_running(true);
int scoreboard_filter = 0, featured_team = 0;

void * constest_update(void *param) {
	while(system_running) {
		contest->update(scoreboard_filter);
		usleep(100000);
	}
}

#define black cv::Vec3b(0,0,0)
#define red cv::Vec3b(0,0,64)
#define orange cv::Vec3b(0,165,255)
#define green cv::Vec3b(0,64,0)
#define white cv::Vec3b(255,255,255)

/* CAMERA STUFF */
typedef enum {CAM_OFF, CAM_ON, CAM_LOADING} cam_status;
atomic<cam_status> camera_status[2];
atomic<bool> camera_turnoff[2];
pthread_mutex_t camera_inuse[2];
cv::Mat camera_frame[2], camera_off;
vector<pair<string, string> > cameras;

void initialize_cameras() {
	string camera_address, camera_name; 
	ifstream fp;
	fp.open("../config/cameras.txt", ifstream::in);
	if(not fp.is_open()) {
		cerr << "Failed to open list of cameras.\n";
		throw;
	}
	while(fp >> camera_address) {
		fp.ignore();
		getline(fp, camera_name);
		cameras.push_back(make_pair(camera_address, camera_name));
	}
	fp.close();

	if(fullhd)
		camera_off.create(1080, 1920, CV_8UC3);
	else
		camera_off.create(720, 1280, CV_8UC3);
	camera_off = white;

	camera_status[0] = camera_status[1] = CAM_OFF;
	camera_inuse[0] = camera_inuse[1] = PTHREAD_MUTEX_INITIALIZER;
	camera_turnoff[0] = camera_turnoff[1] = false;
	camera_off.copyTo(camera_frame[0]);
	camera_off.copyTo(camera_frame[1]);
}

void capture(int index, string camera_adress, bool crop = false) {
	camera_status[index] = CAM_LOADING;

	cv::VideoCapture cap;
	cap.open(camera_adress);
	if(!cap.isOpened()) {
		camera_status[index] = CAM_OFF;
		camera_turnoff[index] = false;
		return;
	}

	if(fullhd) {
		cap.set(CV_CAP_PROP_FRAME_WIDTH,1920);
		cap.set(CV_CAP_PROP_FRAME_HEIGHT,1080);
	} else {
		cap.set(CV_CAP_PROP_FRAME_WIDTH,1280);
		cap.set(CV_CAP_PROP_FRAME_HEIGHT,720);
	}

	cv::Mat image;
	while(cap.read(image)) {
		pthread_mutex_lock(&camera_inuse[index]);
		if(!crop)
			image.copyTo(camera_frame[index]);
		else {
			cv::Mat roi = image.colRange(image.cols*0.2,image.cols).rowRange(image.rows*0.1,image.rows*0.9);
			roi.copyTo(camera_frame[index]);
		}
		pthread_mutex_unlock(&camera_inuse[index]);
		camera_status[index] = CAM_ON;
		if(camera_turnoff[index])
			break;
	}

	cap.release();
	pthread_mutex_lock(&camera_inuse[index]);
	camera_off.copyTo(camera_frame[index]);
	pthread_mutex_unlock(&camera_inuse[index]);
	camera_turnoff[index] = false;
	camera_status[index] = CAM_OFF;
}
/* END OF CAMERA STUFF */

/* CONTROL PANEL STUFF */
typedef enum {MAIN_SCREEN, AUX_SCREEN, SPLIT_SCREEN} screen_mode;
screen_mode mode = MAIN_SCREEN;
cv::Mat panel, panel2, featured_team_image;
atomic<bool> show_scoreboard(false), run_scoreboard(false), show_runlist(false), show_featured(false);
int camera_index[2] = {-1, -1};
long long show_scoreboard_delay = LLONG_MAX;
static vector<string> featured_teams;

void draw_panel() {
	static bool flag = true;
	if(flag) {
		panel.create(1600,800,CV_8UC3);
		panel2.create(800,400,CV_8UC3);
		panel = black;

		long long feat_id = -1;
		for(long long i=0; i < contest->attribute_names.size(); i++)
			if(contest->attribute_names[i] == "featured") {
				feat_id = i;
				break;
			}
		if(feat_id != -1) {
			for(long long i=0; i < contest->teams.size(); i++)
				if(contest->teams[i].attributes.size() == contest->num_attributes && contest->teams[i].attributes[feat_id] == "yes")
					featured_teams.push_back(contest->index_to_team[i]);
			sort(featured_teams.begin(), featured_teams.end());
		}
	}

	if(flag) putText(panel, "MAIN CAMERA", cv::Point(10,35), cv::FONT_HERSHEY_DUPLEX, 1.0, white, 1);
	for(int i=0; i < cameras.size(); i++) {
		rectangle(panel, cv::Rect(10,60+i*50,30,30), (camera_index[0] == i ? (camera_status[0] == CAM_ON ? green : (camera_status[0] == CAM_OFF ? red : orange)) : white), -1);
		if(flag) putText(panel, cameras[i].second, cv::Point(50,85+i*50), cv::FONT_HERSHEY_DUPLEX, 1.0, white, 1);
	}

	if(flag) putText(panel, "AUX CAMERA", cv::Point(410,35), cv::FONT_HERSHEY_DUPLEX, 1.0, white, 1);
	for(int i=0; i < cameras.size(); i++) {
		rectangle(panel, cv::Rect(410,60+i*50,30,30), (camera_index[1] == i ? (camera_status[1] == CAM_ON ? green : (camera_status[1] == CAM_OFF ? red : orange)) : white), -1);
		if(flag) putText(panel, cameras[i].second, cv::Point(450,85+i*50), cv::FONT_HERSHEY_DUPLEX, 1.0, white, 1);
	}

	if(flag) panel.rowRange(70+cameras.size()*50,70+cameras.size()*50+10) = white;

	long long offset = 50*(cameras.size()+2);

	rectangle(panel, cv::Rect(10,offset+10,30,30), (mode == MAIN_SCREEN ? green : white), -1);
	if(flag) putText(panel, "MAIN", cv::Point(50,offset+35), cv::FONT_HERSHEY_DUPLEX, 1.0, white, 1);
	rectangle(panel, cv::Rect(260,offset+10,30,30), (mode == AUX_SCREEN ? green : white), -1);
	if(flag) putText(panel, "AUX", cv::Point(300,offset+35), cv::FONT_HERSHEY_DUPLEX, 1.0, white, 1);
	rectangle(panel, cv::Rect(510,offset+10,30,30), (mode == SPLIT_SCREEN ? green : white), -1);
	if(flag) putText(panel, "SPLIT", cv::Point(550,offset+35), cv::FONT_HERSHEY_DUPLEX, 1.0, white, 1);

	if(flag) panel.rowRange(offset+70,offset+80) = white;

	offset += 100;

	rectangle(panel, cv::Rect(10,offset+10,30,30), (show_scoreboard ? green : white), -1);
	if(flag) putText(panel, "Scoreboard", cv::Point(50,offset+35), cv::FONT_HERSHEY_DUPLEX, 0.6, white, 1);
	rectangle(panel, cv::Rect(210,offset+10,30,30), (run_scoreboard ? green : (show_scoreboard ? white : red)), -1);
	if(flag) putText(panel, "Roll", cv::Point(250,offset+35), cv::FONT_HERSHEY_DUPLEX, 0.6, white, 1);
	rectangle(panel, cv::Rect(410,offset+10,30,30), (show_runlist ? green : white), -1);
	if(flag) putText(panel, "Runlist", cv::Point(450,offset+35), cv::FONT_HERSHEY_DUPLEX, 0.6, white, 1);
	rectangle(panel, cv::Rect(610,offset+10,30,30), (show_featured ? green : white), -1);
	if(flag) putText(panel, "Featured", cv::Point(650,offset+35), cv::FONT_HERSHEY_DUPLEX, 0.6, white, 1);

	if(flag) panel.rowRange(offset+70,offset+80) = white;

	offset += 100;

	int i=0, j=0;
	for(auto sd : contest->gui_scoreboard_filters) {
		rectangle(panel, cv::Rect(10+j*400,offset+10+i*50,30,30), (scoreboard_filter == i*2+j ? green : (!show_scoreboard ? red : white)), -1);
		if(flag) putText(panel, sd.title, cv::Point(50+j*400,offset+35+i*50), cv::FONT_HERSHEY_DUPLEX, 1.0, white, 1);

		j++;
		if(j >= 2) {
			i++;
			j=0;
		}
	}
	if(j > 0) i++;

	if(flag) panel.rowRange(offset+i*50+20,offset+i*50+30) = white;

	offset += (i+1)*50;

	// featured team
	i=0; j=0;
	for(string t : featured_teams) {
		rectangle(panel, cv::Rect(10+j*200,offset+10+i*50,30,30), (featured_team == i*4+j ? green : white), -1);
		if(flag) putText(panel, t, cv::Point(50+j*200,offset+35+i*50), cv::FONT_HERSHEY_DUPLEX, 0.7, white, 1);

		j++;
		if(j >= 4) {
			i++;
			j=0;
		}
	}
	if(j > 0) i++;

	cv::resize(panel, panel2, cv::Size(400,800), 0, 0, cv::INTER_CUBIC);
	flag = false;
}

static void on_mouse(int event, int x, int y, int, void *) {
	if(event != cv::EVENT_LBUTTONDOWN)
		return;

	x*=2;
	y*=2;

	for(int i=0; i < cameras.size(); i++) {
		if(x >= 10 && x <= 40 && y >= 60+i*50 && y <= 90+i*50) {
			if(camera_index[0] != i) {
				if(camera_index[0] != -1 && camera_status[0] == CAM_ON) {
					camera_turnoff[0] = true;
					while(camera_status[0] != CAM_OFF) usleep(100);
				}
				if(camera_index[1] == i && camera_status[1] == CAM_ON) {
					camera_turnoff[1] = true;
					while(camera_status[1] != CAM_OFF) usleep(100);
					camera_index[1] = -1;
				}
				camera_index[0] = i;
				if(cameras[i].second == "IPCAM0")
					thread(capture, 0, cameras[i].first, true).detach();
				else
					thread(capture, 0, cameras[i].first, false).detach();
			}
			else {
				if(camera_status[0] == CAM_ON) {
					camera_turnoff[0] = true;
					while(camera_status[0] != CAM_OFF) usleep(100);
				}
				camera_index[0] = -1;
			}
		}
		else if(x >= 410 && x <= 440 && y >= 60+i*50 && y <= 90+i*50) {
			if(camera_index[1] != i) {
				if(camera_index[1] != -1 && camera_status[1] == CAM_ON) {
					camera_turnoff[1] = true;
					while(camera_status[1] != CAM_OFF) usleep(100);
				}
				if(camera_index[0] == i && camera_status[0] == CAM_ON) {
					camera_turnoff[0] = true;
					while(camera_status[0] != CAM_OFF) usleep(100);
					camera_index[0] = -1;
				}
				camera_index[1] = i;
				if(cameras[i].second == "IPCAM0")
					thread(capture, 1, cameras[i].first, true).detach();
				else
					thread(capture, 1, cameras[i].first, false).detach();
			}
			else {
				if(camera_status[1] == CAM_ON) {
					camera_turnoff[1] = true;
					while(camera_status[1] != CAM_OFF) usleep(100);
				}
				camera_index[1] = -1;
			}
		}
	}

	long long offset = 50*(cameras.size()+2);

	if(x >= 10 && x <= 40 && y >= offset+10 && y <= offset+40)
		mode = MAIN_SCREEN;
	else if(x >= 260 && x <= 290 && y >= offset+10 && y <= offset+40)
		mode = AUX_SCREEN;
	else if(x >= 510 && x <= 540 && y >= offset+10 && y <= offset+40)
		mode = SPLIT_SCREEN;

	offset += 100;

	if(x >= 10 && x <= 40 && y >= offset+10 && y <= offset+40) {
		if(!show_scoreboard)
			show_featured = false;
		show_scoreboard = !show_scoreboard;
		run_scoreboard = false;
		if(!show_scoreboard) {
			scoreboard_filter = 0;
			show_scoreboard_delay = LLONG_MAX;
		}
		else
			show_scoreboard_delay = time_in_ms();
	} else if(x >= 210 && x <= 240 && y >= offset+10 && y <= offset+40)
		run_scoreboard = (!run_scoreboard) & show_scoreboard; // & (!update_scoreboard);
	else if(x >= 410 && x <= 440 && y >= offset+10 && y <= offset+40)
		show_runlist = !show_runlist;
	else if(x >= 610 && x <= 640 && y >= offset+10 && y <= offset+40) {
		if(!show_featured) {
			show_scoreboard = false;
			show_scoreboard_delay = LLONG_MAX;
			run_scoreboard = false;
			scoreboard_filter = 0;
			featured_team_image = cv::imread("../assets/stats/"+featured_teams[featured_team]+".png", cv::IMREAD_UNCHANGED);
			if(!fullhd)
				cv::resize(featured_team_image, featured_team_image, cv::Size(900, 574), 0, 0, cv::INTER_CUBIC);
		}
		show_featured = !show_featured;
	}

	offset += 100;

	int i=0, j=0;
	for(int k=0; k < contest->gui_scoreboard_filters.size(); k++) {
		if(x >= 10+j*400 && x <= 40+j*400 && y >= offset+10+i*50 && y <= offset+40+i*50) {
			if(show_scoreboard) {
				scoreboard_filter = i*2+j;
				show_scoreboard_delay = time_in_ms();
				run_scoreboard = false;
			}
		}

		j++;
		if(j >= 2) {
			i++;
			j=0;
		}
	}
	if(j > 0) i++;

	offset += (i+1)*50;


	// featured team
	i=0; j=0;
	for(int k=0; k < featured_teams.size(); k++) {
		if(x >= 10+j*200 && x <= 40+j*200 && y >= offset+10+i*50 && y <= offset+40+i*50) {
			featured_team = i*4+j;
			if(show_featured) {
				show_featured = false;
				featured_team_image = cv::imread("../assets/stats/"+featured_teams[featured_team]+".png", cv::IMREAD_UNCHANGED);
				if(!fullhd)
					cv::resize(featured_team_image, featured_team_image, cv::Size(900, 574), 0, 0, cv::INTER_CUBIC);
				show_featured = true;
			}
		}
		j++;
		if(j >= 4) {
			i++;
			j=0;
		}
	}
	if(j > 0) i++;


	draw_panel();
	cv::imshow("control", panel2);
}
/* END OF CONTROL PANEL STUFF */

int main(int argc, char **argv) {
	if(argc > 1)
		fullhd = true;

	initialize_cameras();

	cv::namedWindow("control", CV_WINDOW_AUTOSIZE);
	cv::setMouseCallback("control", on_mouse, 0);

	contest = new BOCA_Contest(fullhd);

	draw_panel();
	cv::imshow("control", panel2);

	pthread_t constest_update_thread;
	pthread_create(&constest_update_thread, 0, constest_update, NULL);


	cv::Mat image, image2;
	long long t = time_in_ms(), trun = -1, tant = time_in_ms();
	for(long long i=0;;i++) {
		if(mode == MAIN_SCREEN) {
			pthread_mutex_lock(&camera_inuse[0]);
			camera_frame[0].copyTo(image);
			pthread_mutex_unlock(&camera_inuse[0]);
		}
		else {
			pthread_mutex_lock(&camera_inuse[1]);
			camera_frame[1].copyTo(image);
			pthread_mutex_unlock(&camera_inuse[1]);
		}
		if(mode == SPLIT_SCREEN) {
			pthread_mutex_lock(&camera_inuse[0]);
			camera_frame[0].copyTo(image2);
			pthread_mutex_unlock(&camera_inuse[0]);
		}
		if(fullhd) {
			if(image.rows != 1080 || image.cols != 1920)
				cv::resize(image, image, cv::Size(1920,1080), 0, 0, cv::INTER_CUBIC);
		}
		else if(image.rows != 720 || image.cols != 1280)
			cv::resize(image, image, cv::Size(1280,720), 0, 0, cv::INTER_CUBIC);
		if(mode == SPLIT_SCREEN) {
			if(fullhd)
				cv::resize(image2, image.colRange(1920-510,1920-30).rowRange(30,300), cv::Size(480, 270), 0, 0, cv::INTER_NEAREST);
			else
				cv::resize(image2, image.colRange(1280-340,1280-20).rowRange(20,200), cv::Size(320, 180), 0, 0, cv::INTER_NEAREST);
		}

		if(show_scoreboard && time_in_ms()-3000 > show_scoreboard_delay) {
			if(run_scoreboard && trun == -1)
				trun = time_in_ms();
			run_scoreboard = run_scoreboard & contest->draw_main_scoreboard(image, (run_scoreboard ? contest->scoreboard_row_height*(time_in_ms()-trun)/1000 : 0), scoreboard_filter);
			if(!run_scoreboard)
				trun = -1;
		}

		if(show_featured) {
			long long dx = 9*contest->scoreboard_row_height, dy = contest->scoreboard_row_height/2;

			long long j = contest->team_to_index[featured_teams[featured_team]];
			cv::Mat roi = image.colRange(dx,dx+contest->scoreboard_width).rowRange(dy,dy+contest->scoreboard_row_height);
			pthread_mutex_lock(&contest->gui_scoreboard_mutex[j]);
			contest->blend_bgra2bgr(contest->gui_scoreboard[j], roi);
			pthread_mutex_unlock(&contest->gui_scoreboard_mutex[j]);

			dy += contest->scoreboard_row_height/*featured_team_image.rows*/+contest->scoreboard_row_space;
			roi = image.colRange(dx,dx+featured_team_image.cols).rowRange(dy,dy+featured_team_image.rows);
			contest->blend_bgra2bgr(featured_team_image, roi);
		}

		if(show_runlist)
			contest->draw_runlist(image);

		contest->draw_status_bar(image, !show_scoreboard, scoreboard_filter);

		if(time_in_ms()-tant < 25) {
			long long u = time_in_ms();
			//cout << "usleep " << (25-(u-tant))*900 << endl;
			usleep((25-(u-tant))*900);
		}
		tant = time_in_ms();
		cv::imshow("scoreboard", image);
		cv::waitKey(1);
		if(i%100 == 99) {
			long long u = time_in_ms();
			cout << 100000.0/(u-t) << "fps" << endl;
			t = u;
		}
	}

	camera_turnoff[0] = camera_turnoff[1] = true;
	system_running = false;

	while(camera_status[0] != CAM_OFF) usleep(100);
	while(camera_status[1] != CAM_OFF) usleep(100);
	pthread_join(constest_update_thread, NULL);
}


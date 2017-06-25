#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>
#include <algorithm>
#include <utility>
#include <string>
#include "model.h"

void help_message(){
	cout<<"usage:./bin/main -[downlink|uplink] <input_file> <output_file>"<<endl;
}

int main (int argc , char* argv[])
{
	bool isRandom;
	char buffer[200];
	fstream file_out(argv[2], ios::out);

	cout<<"Random? (y/n : 1/0)"<<endl;
	cin>>isRandom;
	if (isRandom) {
		srand(time(NULL)); // set random
	}

	// --Aloha--
	if(!strcmp(argv[1],"-aloha")){

		model Model;
			int num=50;
		random_i r;
		r.generate(num);

	    vector<vector<double>> D=r.BS_dis();
            vector<double> bit_rate;
	    vector<double> SINR;

            for(unsigned i=0;i<D.size();++i){
	       bit_rate.push_back((Model.two_ray_ground(D[i])).bit_rate);
	       SINR.push_back((Model.two_ray_ground(D[i])).SINR);
	    }
	    aloha hello;
	    hello.add_mobiles(r.dis());
            hello.total_time=100;
	    vector<double> time_stat=hello.random_time();
	    //for(unsigned i=0;i<time_stat.size();++i)
		//cout<<time_stat[i]<<endl;
		//cout<<bit_rate[i]<<endl;
		//cout<<r.dis()[i]<<endl;
		//cout<<SINR[i]<<endl;
	    for(unsigned bits=100;bits<30*100;bits=bits+1){
	       hello.bits=bits;	
	       vector<bool> transmit=hello.collision(time_stat,bit_rate); 
	       double data=0;
	       for(unsigned i=0;i<transmit.size();++i)
		if(transmit[i])
		   data=data+bits;
	       double size=transmit.size();
	       
	       file_out<<bits<<"  "<<data<<endl;
	    
	    //for(unsigned i=0;i<transmit.size();++i)
	    //	cout<<transmit[i]<<"/";
	    //cout<<endl;
	    }
	}
	// --end--

	// --CSMA--
	else if (!strcmp(argv[1],"-CSMA")) {
	int runTime;
	vector<double> bit_rate;
	vector<double> SINR;
	vector<double> time_stat;
	vector<double> result[3];
	one_CSMA CSMA_model;

	cout<<"Run Time : ";
	cin>>runTime;
	cout<<"Stimulation end time (100) : ";
	cin>>CSMA_model.total_time;
	cout<<"Cpature threshold (10) : ";
	cin>>CSMA_model.captureThreshold;
	cout<<"P value (1) : ";
	cin>>CSMA_model.p;
	cout<<"Start bits (100) : ";
	cin>>CSMA_model.startBits;
	cout<<"End bits (15000) : ";
	cin>>CSMA_model.endBits;
	cout<<"Bits interval (20) : ";
	cin>>CSMA_model.bitsInterval;

	for (int n = 0; n < runTime; n++) {
		// initializing
		model Model;
		int count = 0;
		int num = 50;
		random_i r;
		r.generate(num);
		vector<vector<double>> D = r.BS_dis();
		CSMA_model.add_mobiles(r.dis());
		for (int i = 0; i < D.size(); i++) {
	       bit_rate.push_back((Model.two_ray_ground(D[i])).bit_rate);
	       SINR.push_back((Model.two_ray_ground(D[i])).SINR);
		}
		time_stat = CSMA_model.random_time();

		// round time_stat
		int res = round(1.0 / CSMA_model.time_interval);
		for (int i = 0 ; i < num; i++) {
			time_stat[i] *= res;
			time_stat[i] = round(time_stat[i]);
			time_stat[i] /= res;
		}
		
		//cout<<"send time"<<'\t'<<"bit rate"<<'\t'<<"SINR"<<endl; // log
		// log
		/*for (int i = 0; i < D.size(); i++) {
			cout<<time_stat[i]<<'\t'<<bit_rate[i]<<'\t'<<SINR[i]<<endl;
		}*/

		for (int bits = CSMA_model.startBits; bits < CSMA_model.endBits; bits += CSMA_model.bitsInterval) {
			// initializing

			CSMA_model.isBusy = false;
			CSMA_model.isCollision = false;
			CSMA_model.bits = bits;
			CSMA_model.count = 0;
			CSMA_model.data = 0;
			CSMA_model.sending = -1;
			CSMA_model.succeed = 0;
			for (int j = 0; j < num; j++) {
				CSMA_model.MSstatus[j] = 0;
				CSMA_model.idleTime[j] = -1;
			}

			//cout<<"start stimulation with total time "<<CSMA_model.total_time<<" (sec) and data "<<bits<<" (bits)"<<endl; // log

			// main loop
			for (double t = 0; t <= CSMA_model.total_time; t += CSMA_model.time_interval) {
				//cout<<t; // debug log
				CSMA_model.isCollision = false;
				// send data
				for (int j = 0; j < num; j++) {
					if (CSMA_model.MSstatus[j] == 2) {
						if (CSMA_model.sending == -1) {
							CSMA_model.MSstatus[j] = 3;
							CSMA_model.sending = j;
						}
						else {
							CSMA_model.MSstatus[j] = 4;
							CSMA_model.MSstatus[CSMA_model.sending] = 4;
							CSMA_model.isCollision = true;
						}
						CSMA_model.idleTime[j] = t + bits / bit_rate[j];
						CSMA_model.idleTime[j] *= res;
						CSMA_model.idleTime[j] = round(CSMA_model.idleTime[j]);
						CSMA_model.idleTime[j] /= res;
						CSMA_model.isBusy = true;
					}
				}
				// handle collision and capture
				if (CSMA_model.isCollision) {
					CSMA_model.sending = -1;
					vector<pair<double, int>> capture;
					//cout<<t<<" sec "<<"collision, "; // log
					for (int j = 0; j < num; j++) {
						if (CSMA_model.MSstatus[j] == 4) {
							capture.push_back(make_pair(SINR[j], j));
							//CSMA_model.MScollide[j] = false;
						}
					}
					//cout<<"failed"<<endl; // log
					sort(capture.begin(), capture.end());
					for (int k = 0; k < capture.size(); k++) {
						//cout<<capture[k].first<<" ";
					}
					if (capture.size() >= 2 && abs(capture[capture.size() - 1].first - capture[capture.size() - 2].first) >= CSMA_model.captureThreshold) {
						//cout<<capture[capture.size() - 1].second<<"captured others";
						CSMA_model.MSstatus[capture[capture.size() - 1].second] = 3;
						CSMA_model.sending = capture[capture.size() - 1].second;
						//cout<<CSMA_model.idleTime<< " sec "<<endl; // log
					}
				}
				// update status
				for (int j = 0; j < num; j++) {
					if (abs(t - CSMA_model.idleTime[j]) < CSMA_model.time_interval / 2) {
						if (CSMA_model.MSstatus[j] == 3) {
							CSMA_model.MSstatus[j] = 5;
							CSMA_model.sending = -1;
						}
						else if (CSMA_model.MSstatus[j] == 4) {
							CSMA_model.MSstatus[j] = 6;				
						}
						CSMA_model.idleTime[j] = -1;
					}
					//cout<<"channel clear"; // debug log
				}		
				for (int j = 0; j < num; j++) {
					CSMA_model.isBusy = false;
					if (abs(-1 - CSMA_model.idleTime[j]) < CSMA_model.time_interval / 2) {
					}
					else {
						//cout<<"set busy"; // debug log
						CSMA_model.isBusy = true;
						break;
					}
					//cout<<"channel clear"; // debug log
				}
				// time to send
				if (abs(t - time_stat[CSMA_model.count]) < CSMA_model.time_interval / 2) {
					//cout<<"time to send"; // debug log
					//cout<<t<<" sec "<<CSMA_model.count<<" time to send"<<endl; // log
					if (!CSMA_model.isBusy) {
						//cout<<"no busy"<<endl; // debug log
						CSMA_model.MSstatus[CSMA_model.count] = 2;
					}
					else {
						CSMA_model.MSstatus[CSMA_model.count] = 1;
					}
					CSMA_model.count++;
				}
				// one-persistent
				for (int j = 0; j < num; j++) {
					if ((double)rand() / RAND_MAX <= CSMA_model.p && CSMA_model.MSstatus[j] == 1 && !CSMA_model.isBusy) {
						CSMA_model.MSstatus[j] = 2;
					}
				}
				// calculate data
				if (CSMA_model.sending != -1) {
					CSMA_model.data = CSMA_model.data + CSMA_model.time_interval * bit_rate[CSMA_model.sending];
				}
			}
			// end time stimulation
			for (int i = 0; i < D.size(); i++) {
				if (CSMA_model.MSstatus[i] == 5) {
					CSMA_model.succeed++;
				}
				//cout<<CSMA_model.MSsucceed[i]; // debug log
			}
				//file_out<<bits<<"  "<<CSMA_model.data<<"  "<<CSMA_model.succeed / D.size()<<endl;
				if (n == 0) {
					result[0].push_back(bits);
					result[1].push_back(CSMA_model.data);
					result[2].push_back(CSMA_model.succeed / D.size());
				}
				else {
					result[0][count] += CSMA_model.bits;
					result[1][count] += CSMA_model.data;
					result[2][count] += CSMA_model.succeed / D.size();
				}
				count++;
		}
		// end bits stimulation
		//cout<<n<<" end";
	}
	/*vector<double>::iterator iter = result[0].begin();
	for (int ix = 0; iter != result[0].end(); iter++, ix++) {
		cout<<*iter<<endl;
	}*/
	for (int i = 0; i < result[0].size(); i++) {
		result[0][i] /= runTime;
		result[1][i] /= runTime;
		result[2][i] /= runTime;
		file_out<<result[0][i]<<"  "<<result[1][i]<<"  "<<result[2][i]<<endl;
	}
	}
	// --end--

	else if(!strcmp(argv[1],"-uplink")){
	}		
	else{
		help_message();
		return 0;
	}

	return 1;
}

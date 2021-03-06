#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>
#include <string>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <netdb.h>
#include <resolv.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>
#include <pcap.h>

#include <sstream>

#include "ipaccount.h"
#include "voipmonitor.h"
#include "calltable.h"
#include "sniff.h"
#include "format_slinear.h"
#include "codec_alaw.h"
#include "codec_ulaw.h"
#include "tools.h"
#include "calltable.h"
#include "format_ogg.h"

#define BUFSIZE 1024

extern Calltable *calltable;
extern int opt_manager_port;
extern char opt_manager_ip[32];
extern int calls;
extern char opt_clientmanager[1024];
extern int opt_clientmanagerport;
extern char mac[32];
extern int verbosity;
extern char opt_chdir[1024];
extern int terminating;
extern int manager_socket_server;
extern int terminating;
extern int opt_nocdr;
extern int global_livesniffer;
extern int global_livesniffer_all;
extern map<unsigned int, octects_live_t*> ipacc_live;

extern map<unsigned int, livesnifferfilter_t*> usersniffer;

using namespace std;

struct listening_worker_arg {
	Call *call;
};

static void updateLivesnifferfilters();
static bool cmpCallBy_destroy_call_at(Call* a, Call* b);

livesnifferfilter_use_siptypes_s livesnifferfilterUseSipTypes;

/* 
 * this function runs as thread. It reads RTP audio data from call
 * and write it to output buffer 
 *
 * input parameter is structure where call 
 *
*/
void *listening_worker(void *arguments) {
	struct listening_worker_arg *args = (struct listening_worker_arg*)arguments;

        int ret = 0;
        unsigned char read1[1024];
        unsigned char read2[1024];
        struct timeval tv;
        int diff;

	getUpdDifTime(&tv);
	alaw_init();
	ulaw_init();

        struct timeval tvwait;

	short int r1;
	short int r2;
	int len1,len2;

	// if call is hanged hup it will set listening_worker_run in its destructor to 0
	int listening_worker_run = 1;
	args->call->listening_worker_run = &listening_worker_run;
	pthread_mutex_lock(&args->call->listening_worker_run_lock);


//	FILE *out = fopen("/tmp/test.raw", "w");
//	FILE *outa = fopen("/tmp/test.alaw", "w");

//	vorbis_desc ogg;
//	ogg_header(out, &ogg);
//	fclose(out);
//	pthread_mutex_lock(&args->call->buflock);
//	ogg_header_live(&args->call->spybufferchar, &ogg);
//	pthread_mutex_unlock(&args->call->buflock);

	timespec tS;
	timespec tS2;

	tS.tv_sec = 0;
	tS.tv_nsec = 0;
	tS2.tv_sec = 0;
	tS2.tv_nsec = 0;

	long int udiff;

        while(listening_worker_run) {

		if(tS.tv_nsec > tS2.tv_nsec) {
			udiff = (1000 * 1000 * 1000 - (tS.tv_nsec - tS2.tv_nsec)) / 1000;
		} else {
			udiff = (tS2.tv_nsec - tS.tv_nsec) / 1000;
		}

		tvwait.tv_sec = 0;
		tvwait.tv_usec = 1000*20 - udiff; //20 ms
//		long int usec = tvwait.tv_usec;
		ret = select(NULL, NULL, NULL, NULL, &tvwait);

		clock_gettime(CLOCK_REALTIME, &tS);
		char *s16char;

		//usleep(tvwait.tv_usec);
		pthread_mutex_lock(&args->call->buflock);
		diff = getUpdDifTime(&tv) / 1000;
		len1 = circbuf_read(args->call->audiobuffer1, (char*)read1, 160);
		len2 = circbuf_read(args->call->audiobuffer2, (char*)read2, 160);
//		printf("codec_caller[%d] codec_called[%d] len1[%d] len2[%d] outbc[%d] outbchar[%d] wait[%u]\n", args->call->codec_caller, args->call->codec_called, len1, len2, (int)args->call->spybuffer.size(), (int)args->call->spybufferchar.size(), usec);
		if(len1 == 160 and len2 == 160) {
			for(int i = 0; i < len1; i++) {
				switch(args->call->codec_caller) {
				case 0:
					r1 = ULAW(read1[i]);
					break;
				case 8:
					r1 = ALAW(read1[i]);
					break;
				}
					
				switch(args->call->codec_caller) {
				case 0:
					r2 = ULAW(read2[i]);
					break;
				case 8:
					r2 = ALAW(read2[i]);
					break;
				}
				s16char = (char *)&r1;
				slinear_saturated_add((short int*)&r1, (short int*)&r2);
				//fwrite(&r1, 1, 2, out);
				args->call->spybufferchar.push(s16char[0]);
				args->call->spybufferchar.push(s16char[1]);
//				ogg_write_live(&ogg, &args->call->spybufferchar, (short int*)&r1);
			}
		} else if(len2 == 160) {
			for(int i = 0; i < len2; i++) {
				switch(args->call->codec_caller) {
				case 0:
					r2 = ULAW(read2[i]);
					break;
				case 8:
					r2 = ALAW(read2[i]);
					break;
				}
				//fwrite(&r2, 1, 2, out);
				s16char = (char *)&r2;
				args->call->spybufferchar.push(s16char[0]);
				args->call->spybufferchar.push(s16char[1]);
//				ogg_write_live(&ogg, &args->call->spybufferchar, (short int*)&r2);
			}
		} else if(len1 == 160) {
			for(int i = 0; i < len1; i++) {
				switch(args->call->codec_caller) {
				case 0:
					r1 = ULAW(read1[i]);
					break;
				case 8:
					r1 = ALAW(read1[i]);
					break;
				}
				//fwrite(&r1, 1, 2, out);
				s16char = (char *)&r1;
				args->call->spybufferchar.push(s16char[0]);
				args->call->spybufferchar.push(s16char[1]);
//				ogg_write_live(&ogg, &args->call->spybufferchar, (short int*)&r1);
			}
		} else {
                        //printf("diff [%d] timeout\n", diff);
			// write 20ms silence 
			int16_t s = 0;
			//unsigned char sa = 255;
			for(int i = 0; i < 160; i++) {
				//fwrite(&s, 1, 2, out);
				s16char = (char *)&s;
				args->call->spybufferchar.push(s16char[0]);
				args->call->spybufferchar.push(s16char[1]);
//				ogg_write_live(&ogg, &args->call->spybufferchar, (short int*)&s);
			}
		}
		pthread_mutex_unlock(&args->call->buflock);
		clock_gettime(CLOCK_REALTIME, &tS2);
        }

	// reset pointer to NULL as we are leaving the stack here
	args->call->listening_worker_run = NULL;
	pthread_mutex_unlock(&args->call->listening_worker_run_lock);

	//clean ogg
/*
        ogg_stream_clear(&ogg.os);
        vorbis_block_clear(&ogg.vb);
        vorbis_dsp_clear(&ogg.vd);
        vorbis_comment_clear(&ogg.vc);
        vorbis_info_clear(&ogg.vi);
*/

	free(args);
	return 0;
}

int parse_command(char *buf, int size, int client, int eof, const char *buf_long) {
	char sendbuf[BUFSIZE];
	u_int32_t uid = 0;

	if(strstr(buf, "getversion") != NULL) {
		if ((size = send(client, RTPSENSOR_VERSION, strlen(RTPSENSOR_VERSION), 0)) == -1){
			cerr << "Error sending data to client" << endl;
			return -1;
		}
	} else if(strstr(buf, "totalcalls") != NULL) {
		snprintf(sendbuf, BUFSIZE, "%d", calls);
		if ((size = send(client, sendbuf, strlen(sendbuf), 0)) == -1){
			cerr << "Error sending data to client" << endl;
			return -1;
		}
	} else if(strstr(buf, "disablecdr") != NULL) {
		opt_nocdr = 1;
		if ((size = send(client, "disabled", 8, 0)) == -1){
			cerr << "Error sending data to client" << endl;
			return -1;
		}
	} else if(strstr(buf, "enablecdr") != NULL) {
		opt_nocdr = 0;
		if ((size = send(client, "enabled", 7, 0)) == -1){
			cerr << "Error sending data to client" << endl;
			return -1;
		}
	} else if(strstr(buf, "listcalls") != NULL) {
		//list<Call*>::iterator call;
		map<string, Call*>::iterator callMAPIT;
		Call *call;
		char outbuf[2048];
		char *resbuf = (char*)realloc(NULL, 32 * 1024 * sizeof(char));;
		unsigned int resbufalloc = 32 * 1024, outbuflen = 0, resbuflen = 0;
		if(outbuf == NULL) {
			syslog(LOG_ERR, "Cannot allocate memory\n");
			return -1;
		}
		/* headers */
		outbuflen = sprintf(outbuf, "[[\"callreference\", \"callid\", \"callercodec\", \"calledcodec\", \"caller\", \"callername\", \"called\", \"calldate\", \"duration\", \"callerip\", \"calledip\", \"lastpackettime\"]");
		memcpy(resbuf + resbuflen, outbuf, outbuflen);
		resbuflen += outbuflen;
		calltable->lock_calls_listMAP();
		for (callMAPIT = calltable->calls_listMAP.begin(); callMAPIT != calltable->calls_listMAP.end(); ++callMAPIT) {
			call = (*callMAPIT).second;
			if(call->type == REGISTER or call->destroy_call_at > 0) {
				// skip register and calls which are scheduled to be closed
				continue;
			}
			/* 
			 * caller 
			 * callername
			 * called
			 * calldate
			 * duration
			 * callerip htonl(sipcallerip)
			 * sipcalledip htonl(sipcalledip)
			*/
			//XXX: escape " or replace it to '
			outbuflen = sprintf(outbuf, ",[\"%p\", \"%s\", \"%d\", \"%d\", \"%s\", \"%s\", \"%s\", \"%d\", \"%d\", \"%u\", \"%u\", \"%u\"]",
				call, call->call_id, call->last_callercodec, call->last_callercodec, call->caller, 
				call->callername, call->called, call->calltime(), call->duration(), htonl(call->sipcallerip), 
				htonl(call->sipcalledip), (unsigned int)call->get_last_packet_time());
			if((resbuflen + outbuflen) > resbufalloc) {
				resbuf = (char*)realloc(resbuf, resbufalloc + 32 * 1024 * sizeof(char));
				resbufalloc += 32 * 1024;
			}
			memcpy(resbuf + resbuflen, outbuf, outbuflen);
			resbuflen += outbuflen;
		}
		calltable->unlock_calls_listMAP();
		if((resbuflen + 1) > resbufalloc) {
			resbuf = (char*)realloc(resbuf, resbufalloc + 32 * 1024 * sizeof(char));
			resbufalloc += 32 * 1024;
		}
		resbuf[resbuflen] = ']';
		resbuflen++;
		if ((size = send(client, resbuf, resbuflen, 0)) == -1){
			cerr << "Error sending data to client" << endl;
			return -1;
		}
		free(resbuf);
		return 0;
	} else if(strstr(buf, "d_lc_for_destroy") != NULL) {
		ostringstream outStr;
		if(calltable->calls_queue.size()) {
			Call *call;
			vector<Call*> vectCall;
			calltable->lock_calls_queue();
			for(size_t i = 0; i < calltable->calls_queue.size(); ++i) {
				call = calltable->calls_queue[i];
				if(call->type != REGISTER && call->destroy_call_at) {
					vectCall.push_back(call);
				}
			}
			if(vectCall.size()) { 
				std::sort(vectCall.begin(), vectCall.end(), cmpCallBy_destroy_call_at);
				for(size_t i = 0; i < vectCall.size(); i++) {
					call = vectCall[i];
					outStr.width(15);
					outStr << call->caller << " -> ";
					outStr.width(15);
					outStr << call->called << "  "
					<< sqlDateTimeString(call->calltime()) << "  ";
					outStr.width(6);
					outStr << call->duration() << "s  "
					<< sqlDateTimeString(call->destroy_call_at) << "  "
					<< call->fbasename;
					outStr << endl;
				}
			}
			calltable->unlock_calls_queue();
		}
		outStr << "-----------" << endl;
		if ((size = send(client, outStr.str().c_str(), outStr.str().length(), 0)) == -1){
			cerr << "Error sending data to client" << endl;
			return -1;
		}
		return 0;
	} else if(strstr(buf, "d_lc_bye") != NULL) {
		ostringstream outStr;
		map<string, Call*>::iterator callMAPIT;
		Call *call;
		vector<Call*> vectCall;
		calltable->lock_calls_listMAP();
		for (callMAPIT = calltable->calls_listMAP.begin(); callMAPIT != calltable->calls_listMAP.end(); ++callMAPIT) {
			call = (*callMAPIT).second;
			if(call->type != REGISTER && call->seenbye) {
				vectCall.push_back(call);
			}
		}
		if(vectCall.size()) { 
			std::sort(vectCall.begin(), vectCall.end(), cmpCallBy_destroy_call_at);
			for(size_t i = 0; i < vectCall.size(); i++) {
				call = vectCall[i];
				outStr.width(15);
				outStr << call->caller << " -> ";
				outStr.width(15);
				outStr << call->called << "  "
				<< sqlDateTimeString(call->calltime()) << "  ";
				outStr.width(6);
				outStr << call->duration() << "s  "
				<< (call->destroy_call_at ? sqlDateTimeString(call->destroy_call_at) : "    -  -     :  :  ")  << "  "
				<< call->fbasename;
				outStr << endl;
			}
		}
		calltable->unlock_calls_listMAP();
		outStr << "-----------" << endl;
		if ((size = send(client, outStr.str().c_str(), outStr.str().length(), 0)) == -1){
			cerr << "Error sending data to client" << endl;
			return -1;
		}
		return 0;
	} else if(strstr(buf, "getipaccount") != NULL) {
		sscanf(buf, "getipaccount %u", &uid);
		map<unsigned int, octects_live_t*>::iterator it = ipacc_live.find(uid);
		if(it != ipacc_live.end()) {
			snprintf(sendbuf, BUFSIZE, "%d", 1);
		} else {
			snprintf(sendbuf, BUFSIZE, "%d", 0);
		}
		if ((size = send(client, sendbuf, strlen(sendbuf), 0)) == -1){
			cerr << "Error sending data to client" << endl;
			return -1;
		}
	} else if(strstr(buf, "ipaccountfilter set") != NULL) {
		
		string ipfilter;
		if(buf_long) {
			buf = (char*)buf_long;
		}
		u_int32_t id = atol(buf + strlen("ipaccountfilter set "));
		char *pointToSeparatorBefereIpfilter = strchr(buf + strlen("ipaccountfilter set "), ' ');
		if(pointToSeparatorBefereIpfilter) {
			ipfilter = pointToSeparatorBefereIpfilter + 1;
		}
		if(!ipfilter.length() || ipfilter.find("ALL") != string::npos) {
			map<unsigned int, octects_live_t*>::iterator it = ipacc_live.find(id);
			octects_live_t* filter;
			if(it != ipacc_live.end()) {
				filter = it->second;
			} else {
				filter = (octects_live_t*)calloc(1, sizeof(octects_live_t));
				filter->all = 1;
				filter->fetch_timestamp = time(NULL);
				ipacc_live[id] = filter;
				if(verbosity > 0) {
					cout << "START LIVE IPACC " << "id: " << id << " ipfilter: " << "ALL" << endl;
				}
			}
			return 0;
		} else {
			map<unsigned int, octects_live_t*>::iterator ipacc_liveIT = ipacc_live.find(id);
			octects_live_t* filter;
			filter = (octects_live_t*)calloc(1, sizeof(octects_live_t));
			filter->setFilter(ipfilter.c_str());
			filter->fetch_timestamp = time(NULL);
			ipacc_live[id] = filter;
			if(verbosity > 0) {
				cout << "START LIVE IPACC " << "id: " << id << " ipfilter: " << ipfilter << endl;
			}
		}
		return(0);
	} else if(strstr(buf, "stopipaccount")) {
		u_int32_t id = 0;
		sscanf(buf, "stopipaccount %u", &id);
		map<unsigned int, octects_live_t*>::iterator it = ipacc_live.find(id);
		if(it != ipacc_live.end()) {
			free(it->second);
			ipacc_live.erase(it);
			if(verbosity > 0) {
				cout << "STOP LIVE IPACC " << "id:" << id << endl;
			}
		}
		return 0;
	} else if(strstr(buf, "fetchipaccount")) {
		u_int32_t id = 0;
		sscanf(buf, "fetchipaccount %u", &id);
		map<unsigned int, octects_live_t*>::iterator it = ipacc_live.find(id);
		char sendbuf[1024];
		if(it == ipacc_live.end()) {
			strcpy(sendbuf, "stopped");
		} else {
			octects_live_t *data = it->second;
			snprintf(sendbuf, 1024, "%u;%llu;%u;%llu;%u;%llu;%u;%llu;%u;%llu;%u;%llu;%u", 
				(unsigned int)time(NULL),
				data->dst_octects, data->dst_numpackets, 
				data->src_octects, data->src_numpackets, 
				data->voipdst_octects, data->voipdst_numpackets, 
				data->voipsrc_octects, data->voipsrc_numpackets, 
				data->all_octects, data->all_numpackets,
				data->voipall_octects, data->voipall_numpackets);
			data->fetch_timestamp = time(NULL);
		}
		if((size = send(client, sendbuf, strlen(sendbuf), 0)) == -1) {
			cerr << "Error sending data to client" << endl;
			return -1;
		}
		return 0;
///////////////////////////////////////////////////////////////
        } else if(strstr(buf, "stoplivesniffer")) {
                sscanf(buf, "stoplivesniffer %u", &uid);
                map<unsigned int, livesnifferfilter_t*>::iterator usersnifferIT = usersniffer.find(uid);
                if(usersnifferIT != usersniffer.end()) {
                        free(usersnifferIT->second);
                        usersniffer.erase(usersnifferIT);
			if(!usersniffer.size()) {
				global_livesniffer = 0;
				//global_livesniffer_all = 0;
			}
			updateLivesnifferfilters();
                }
                return 0;
	} else if(strstr(buf, "getlivesniffer") != NULL) {
		sscanf(buf, "getlivesniffer %u", &uid);
		map<unsigned int, livesnifferfilter_t*>::iterator usersnifferIT = usersniffer.find(uid);
		if(usersnifferIT != usersniffer.end()) {
			snprintf(sendbuf, BUFSIZE, "%d", 1);
		} else {
			snprintf(sendbuf, BUFSIZE, "%d", 0);
		}
		if ((size = send(client, sendbuf, strlen(sendbuf), 0)) == -1){
			cerr << "Error sending data to client" << endl;
			return -1;
		}
	} else if(strstr(buf, "livefilter set") != NULL) {
		char search[1024] = "";
		char value[1024] = "";

		global_livesniffer_all = 0;
		sscanf(buf, "livefilter set %u %s %[^\n\r]", &uid, search, value);

		if(memmem(search, sizeof(search), "all", 3)) {
			global_livesniffer = 1;
			map<unsigned int, livesnifferfilter_t*>::iterator usersnifferIT = usersniffer.find(uid);
			livesnifferfilter_t* filter;
			if(usersnifferIT != usersniffer.end()) {
				filter = usersnifferIT->second;
			} else {
				filter = (livesnifferfilter_t*)calloc(1, sizeof(livesnifferfilter_t));
				usersniffer[uid] = filter;
			}
			updateLivesnifferfilters();
			return 0;
		}

		map<unsigned int, livesnifferfilter_t*>::iterator usersnifferIT = usersniffer.find(uid);
		livesnifferfilter_t* filter;
		if(usersnifferIT != usersniffer.end()) {
			filter = usersnifferIT->second;
		} else {
			filter = (livesnifferfilter_t*)calloc(1, sizeof(livesnifferfilter_t));
			usersniffer[uid] = filter;
		}

		if(strstr(search, "srcaddr")) {
			int i = 0;
			//reset filters 
			for(i = 0; i < MAXLIVEFILTERS; i++) {
				filter->lv_saddr[i] = 0;
			}
			stringstream  data(value);
			string val;
			// read all argumens lkivefilter set saddr 123 345 244
			i = 0;
			while(i < MAXLIVEFILTERS and getline(data, val,' ')){
				global_livesniffer = 1;
				//convert doted ip to unsigned int
				filter->lv_bothaddr[i] = ntohl((unsigned int)inet_addr(val.c_str()));
				i++;
			}
			updateLivesnifferfilters();
		} else if(strstr(search, "dstaddr")) {
			int i = 0;
			//reset filters 
			for(i = 0; i < MAXLIVEFILTERS; i++) {
				filter->lv_daddr[i] = 0;
			}
			stringstream  data(value);
			string val;
			i = 0;
			// read all argumens livefilter set daddr 123 345 244
			while(i < MAXLIVEFILTERS and getline(data, val,' ')){
				global_livesniffer = 1;
				//convert doted ip to unsigned int
				filter->lv_bothaddr[i] = ntohl((unsigned int)inet_addr(val.c_str()));
				i++;
			}
			updateLivesnifferfilters();
		} else if(strstr(search, "bothaddr")) {
			int i = 0;
			//reset filters 
			for(i = 0; i < MAXLIVEFILTERS; i++) {
				filter->lv_bothaddr[i] = 0;
			}
			stringstream  data(value);
			string val;
			i = 0;
			// read all argumens livefilter set bothaddr 123 345 244
			while(i < MAXLIVEFILTERS and getline(data, val,' ')){
				global_livesniffer = 1;
				//convert doted ip to unsigned int
				filter->lv_bothaddr[i] = ntohl((unsigned int)inet_addr(val.c_str()));
				i++;
			}
			updateLivesnifferfilters();
		} else if(strstr(search, "srcnum")) {
			int i = 0;
			//reset filters 
			for(i = 0; i < MAXLIVEFILTERS; i++) {
				filter->lv_srcnum[i][0] = '\0';
			}
			stringstream  data(value);
			string val;
			i = 0;
			// read all argumens livefilter set srcaddr 123 345 244
			while(i < MAXLIVEFILTERS and getline(data, val,' ')){
				global_livesniffer = 1;
				stringstream tmp;
				tmp << val;
				tmp >> filter->lv_srcnum[i];
				//cout << filter->lv_srcnum[i] << "\n";
				i++;
			}
			updateLivesnifferfilters();
		} else if(strstr(search, "dstnum")) {
			int i = 0;
			//reset filters 
			for(i = 0; i < MAXLIVEFILTERS; i++) {
				filter->lv_dstnum[i][0] = '\0';
			}
			stringstream  data(value);
			string val;
			i = 0;
			// read all argumens livefilter set dstaddr 123 345 244
			while(i < MAXLIVEFILTERS and getline(data, val,' ')){
				global_livesniffer = 1;
				stringstream tmp;
				tmp << val;
				tmp >> filter->lv_dstnum[i];
				//cout << filter->lv_dstnum[i] << "\n";
				i++;
			}
			updateLivesnifferfilters();
		} else if(strstr(search, "bothnum")) {
			int i = 0;
			//reset filters 
			for(i = 0; i < MAXLIVEFILTERS; i++) {
				filter->lv_bothnum[i][0] = '\0';
			}
			stringstream  data(value);
			string val;
			i = 0;
			// read all argumens livefilter set bothaddr 123 345 244
			while(i < MAXLIVEFILTERS and getline(data, val,' ')){
				global_livesniffer = 1;
				stringstream tmp;
				tmp << val;
				tmp >> filter->lv_bothnum[i];
				//cout << filter->lv_bothnum[i] << "\n";
				i++;
			}
			updateLivesnifferfilters();
		} else if(strstr(search, "siptypes")) {
			//cout << "siptypes: " << value << "\n";
			for(size_t i = 0; i < strlen(value) && i < MAXLIVEFILTERS; i++) {
				filter->lv_siptypes[i] = value[i] == 'I' ? INVITE :
							 value[i] == 'R' ? REGISTER :
							 value[i] == 'O' ? OPTIONS :
							 value[i] == 'S' ? SUBSCRIBE :
							 value[i] == 'M' ? MESSAGE :
									   0;
			}
			updateLivesnifferfilters();
		}
	} else if(strstr(buf, "listen") != NULL) {
		long long callreference;

		intptr_t tmp1,tmp2;

		sscanf(buf, "listen %llu", &callreference);

		tmp1 = callreference;
	
		map<string, Call*>::iterator callMAPIT;
		Call *call;
		calltable->lock_calls_listMAP();
		for (callMAPIT = calltable->calls_listMAP.begin(); callMAPIT != calltable->calls_listMAP.end(); ++callMAPIT) {
			call = (*callMAPIT).second;
			tmp2 = (intptr_t)call;

			//printf("call[%p] == [%li] [%d] [%li] [%li]\n", call, callreference, (long int)call == (long int)callreference, (long int)call, (long int)callreference);
				
			//if((long long)call == (long long)callreference) {
			if(tmp1 == tmp2) {
				if(call->listening_worker_run) {
					// the thread is already running. 
					if ((size = send(client, "call already listening", 22, 0)) == -1){
						cerr << "Error sending data to client" << endl;
						return -1;
					}
					calltable->unlock_calls_listMAP();
					return 0;
				} else {
					struct listening_worker_arg *args = (struct listening_worker_arg*)malloc(sizeof(listening_worker_arg));
					args->call = call;
					call->audiobuffer1 = new pvt_circbuf;
					call->audiobuffer2 = new pvt_circbuf;
					circbuf_init(call->audiobuffer1, 4092);
					circbuf_init(call->audiobuffer2, 4092);

					pthread_t call_thread;
					pthread_create(&call_thread, NULL, listening_worker, (void *)args);
					calltable->unlock_calls_listMAP();
					if ((size = send(client, "success", 7, 0)) == -1){
						cerr << "Error sending data to client" << endl;
						return -1;
					}
					return 0;
				}
			}
		}
		calltable->unlock_calls_listMAP();
		if ((size = send(client, "call not found", 14, 0)) == -1){
			cerr << "Error sending data to client" << endl;
			return -1;
		}
		return 0;
	} else if(strstr(buf, "readaudio") != NULL) {
		long long callreference;

		sscanf(buf, "readaudio %llu", &callreference);
	
		map<string, Call*>::iterator callMAPIT;
		Call *call;
		int i;
		calltable->lock_calls_listMAP();
		for (callMAPIT = calltable->calls_listMAP.begin(); callMAPIT != calltable->calls_listMAP.end(); ++callMAPIT) {
			call = (*callMAPIT).second;
			if((long int)call == (long int)callreference) {
				pthread_mutex_lock(&call->buflock);
				size_t bsize = call->spybufferchar.size();
				char *buff = (char*)malloc(sizeof(char) * bsize);
				for(i = 0; i < (int)bsize; i++) {
					buff[i] = call->spybufferchar.front();
					call->spybufferchar.pop();
				}
				pthread_mutex_unlock(&call->buflock);
				if ((size = send(client, buff, bsize, 0)) == -1){
					free(buff);
					calltable->unlock_calls_listMAP();
					cerr << "Error sending data to client" << endl;
					return -1;
				}
				free(buff);
			}
		}
		calltable->unlock_calls_listMAP();
		return 0;
	} else if(strstr(buf, "reload") != NULL) {
		reload_config();
		if ((size = send(client, "reload ok", 9, 0)) == -1){
			cerr << "Error sending data to client" << endl;
			return -1;
		}
		return 0;
	} else if(strstr(buf, "getfile") != NULL) {
		char filename[2048];
		char rbuf[4096];
		int fd;
		ssize_t nread;

		sscanf(buf, "getfile %s", filename);

		fd = open(filename, O_RDONLY);
		if(fd < 0) {
			sprintf(buf, "error: cannot open file [%s]", filename);
			if ((size = send(client, buf, strlen(buf), 0)) == -1){
				cerr << "Error sending data to client" << endl;
			}
			return -1;
		}
		while(nread = read(fd, rbuf, sizeof rbuf), nread > 0) {
			if ((size = send(client, rbuf, nread, 0)) == -1){
				close(fd);
				return -1;
			}
		}
		close(fd);
		return 0;
	} else if(strstr(buf, "fileexists") != NULL) {
		char filename[2048];
		unsigned int size;

		sscanf(buf, "fileexists %s", filename);
		size = file_exists(filename);
		sprintf(buf, "%d", size);
		send(client, buf, strlen(buf), 0);
		return 0;
	} else if(strstr(buf, "genwav") != NULL) {
		char filename[2048];
		unsigned int size;
		char wavfile[2048];
		char pcapfile[2048];
		char cmd[4092];
		int secondrun = 0;

		sscanf(buf, "genwav %s", filename);

		sprintf(pcapfile, "%s.pcap", filename);
		sprintf(wavfile, "%s.wav", filename);

getwav2:
		size = file_exists(wavfile);
		if(size) {
			sprintf(buf, "%d", size);
			send(client, buf, strlen(buf), 0);
			return 0;
		}
		if(secondrun > 0) {
			// wav does not exist 
			send(client, "0", 1, 0);
			return -1;
		}

		// wav does not exists, check if exists pcap and try to create wav
		size = file_exists(pcapfile);
		if(!size) {
			send(client, "0", 1, 0);
			return -1;
		}
		sprintf(cmd, "PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/sbin:/usr/local/bin voipmonitor --rtp-firstleg -k -WRc -r \"%s.pcap\" -y -d %s 2>/dev/null >/dev/null", filename, opt_chdir);
		system(cmd);
		secondrun = 1;
		goto getwav2;
	} else if(strstr(buf, "getwav") != NULL) {
		char filename[2048];
		int fd;
		unsigned int size;
		char wavfile[2048];
		char pcapfile[2048];
		char cmd[4092];
		char rbuf[4096];
		int res;
		ssize_t nread;
		int secondrun = 0;

		sscanf(buf, "getwav %s", filename);

		sprintf(pcapfile, "%s.pcap", filename);
		sprintf(wavfile, "%s.wav", filename);

getwav:
		size = file_exists(wavfile);
		if(size) {
			fd = open(wavfile, O_RDONLY);
			if(fd < 0) {
				sprintf(buf, "error: cannot open file [%s]", wavfile);
				if ((res = send(client, buf, strlen(buf), 0)) == -1){
					cerr << "Error sending data to client" << endl;
				}
				return -1;
			}
			while(nread = read(fd, rbuf, sizeof rbuf), nread > 0) {
				if ((res = send(client, rbuf, nread, 0)) == -1){
					close(fd);
					return -1;
				}
			}
			if(eof) {
				if ((res = send(client, "EOF", 3, 0)) == -1){
					close(fd);
					return -1;
				}
			}
			close(fd);
			return 0;
		}
		if(secondrun > 0) {
			// wav does not exist 
			send(client, "0", 1, 0);
			return -1;
		}

		// wav does not exists, check if exists pcap and try to create wav
		size = file_exists(pcapfile);
		if(!size) {
			send(client, "0", 1, 0);
			return -1;
		}
		sprintf(cmd, "PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/sbin:/usr/local/bin voipmonitor --rtp-firstleg -k -WRc -r \"%s.pcap\" -y 2>/dev/null >/dev/null", filename);
		system(cmd);
		secondrun = 1;
		goto getwav;
	} else if(strstr(buf, "getsiptshark") != NULL) {
		char filename[2048];
		int fd;
		unsigned int size;
		char tsharkfile[2048];
		char pcapfile[2048];
		char cmd[4092];
		char rbuf[4096];
		int res;
		ssize_t nread;

		sscanf(buf, "getsiptshark %s", filename);

		sprintf(tsharkfile, "%s.pcap2txt", filename);
		sprintf(pcapfile, "%s.pcap", filename);


		size = file_exists(tsharkfile);
		if(size) {
			fd = open(tsharkfile, O_RDONLY);
			if(fd < 0) {
				sprintf(buf, "error: cannot open file [%s]", tsharkfile);
				if ((res = send(client, buf, strlen(buf), 0)) == -1){
					cerr << "Error sending data to client" << endl;
				}
				return -1;
			}
			while(nread = read(fd, rbuf, sizeof rbuf), nread > 0) {
				if ((res = send(client, rbuf, nread, 0)) == -1){
					close(fd);
					return -1;
				}
			}
			if(eof) {
				if ((res = send(client, "EOF", 3, 0)) == -1){
					close(fd);
					return -1;
				}
			}
			close(fd);
			return 0;
		}

		size = file_exists(pcapfile);
		if(!size) {
			send(client, "0", 1, 0);
			return -1;
		}
	
		sprintf(cmd, "PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin tshark -r \"%s.pcap\" -R sip > \"%s.pcap2txt\" 2>/dev/null", filename, filename);
		system(cmd);
		sprintf(cmd, "echo ==== >> \"%s.pcap2txt\"", filename);
		system(cmd);
		sprintf(cmd, "PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin tshark -r \"%s.pcap\" -V -R sip >> \"%s.pcap2txt\" 2>/dev/null", filename, filename);
		system(cmd);

		size = file_exists(tsharkfile);
		if(size) {
			fd = open(tsharkfile, O_RDONLY);
			if(fd < 0) {
				sprintf(buf, "error: cannot open file [%s]", filename);
				return -1;
			}
			while(nread = read(fd, rbuf, sizeof rbuf), nread > 0) {
				if ((res = send(client, rbuf, nread, 0)) == -1){
					close(fd);
					return -1;
				}
			}
			if(eof) {
				if ((res = send(client, "EOF", 3, 0)) == -1){
					close(fd);
					return -1;
				}
			}
			close(fd);
			return 0;
		}
		return 0;
	} else if(strstr(buf, "quit") != NULL) {
		return 0;
	} else if(strstr(buf, "custipcache_get_cust_id") != NULL) {
		char ip[20];
		sscanf(buf, "custipcache_get_cust_id %s", ip);
		extern CustIpCache *custIpCache;
		unsigned int cust_id = custIpCache->getCustByIp(inet_addr(ip));
		snprintf(sendbuf, BUFSIZE, "cust_id: %u\n", cust_id);
		if((size = send(client, sendbuf, strlen(sendbuf), 0)) == -1) {
			cerr << "Error sending data to client" << endl;
			return -1;
		}
		return 0;
	} else if(strstr(buf, "custipcache_refresh") != NULL) {
		extern CustIpCache *custIpCache;
		custIpCache->clear();
		int rslt = custIpCache->fetchAllIpQueryFromDb();
		snprintf(sendbuf, BUFSIZE, "rslt: %i\n", rslt);
		if((size = send(client, sendbuf, strlen(sendbuf), 0)) == -1) {
			cerr << "Error sending data to client" << endl;
			return -1;
		}
		return 0;
	} else if(strstr(buf, "custipcache_vect_print") != NULL) {
		extern CustIpCache *custIpCache;
		string rslt = custIpCache->printVect();
		if((size = send(client, rslt.c_str(), rslt.length(), 0)) == -1) {
			cerr << "Error sending data to client" << endl;
			return -1;
		}
		return 0;
	} else {
		if ((size = send(client, "command not found\n", 18, 0)) == -1){
			cerr << "Error sending data to client" << endl;
			return -1;
		}
	}
	return 1;
}


void *manager_client(void *dummy) {
	struct hostent* host;
	struct sockaddr_in addr;
	int res;
	int client = 0;
	char buf[BUFSIZE];
	char sendbuf[BUFSIZE];
	int size;
	

	while(1) {
		host = gethostbyname(opt_clientmanager);
		if (!host) { //Report lookup failure  
			syslog(LOG_ERR, "Cannot resolv: %s: host [%s] trying again...\n",  hstrerror(h_errno),  opt_clientmanager);  
			sleep(1);
			continue;  
		} 
		break;
	}
connect:
	client = socket(PF_INET, SOCK_STREAM, 0); /* create socket */
	memset(&addr, 0, sizeof(addr));    /* create & zero struct */
	addr.sin_family = AF_INET;    /* select internet protocol */
	addr.sin_port = htons(opt_clientmanagerport);         /* set the port # */
	addr.sin_addr.s_addr = *(long*)host->h_addr_list[0]; /* set the addr */
	syslog(LOG_NOTICE, "Connecting to manager server [%s]\n", inet_ntoa( *(struct in_addr *) host->h_addr_list[0]));
	while(1) {
		res = connect(client, (struct sockaddr *)&addr, sizeof(addr));         /* connect! */
		if(res == -1) {
			syslog(LOG_NOTICE, "Failed to connect to server [%s] error:[%s] trying again...\n", inet_ntoa( *(struct in_addr *) host->h_addr_list[0]), strerror(errno));
			sleep(1);
			continue;
		}
		break;
	}

	// send login
	snprintf(sendbuf, BUFSIZE, "login %s", mac);
	if ((size = send(client, sendbuf, strlen(sendbuf), 0)) == -1){
		perror("send()");
		sleep(1);
		goto connect;
	}

	while(1) {

		string buf_long;
		
		//cout << "New manager connect from: " << inet_ntoa((in_addr)clientInfo.sin_addr) << endl;
		size = recv(client, buf, BUFSIZE - 1, 0);
		if (size == -1 or size == 0) {
			//cerr << "Error in receiving data" << endl;
			perror("recv()");
			close(client);
			sleep(1);
			goto connect;
		} else {
			buf[size] = '\0';
			buf_long = buf;
			char buf_next[BUFSIZE];
			while((size = recv(client, buf_next, BUFSIZE - 1, 0)) > 0) {
				buf_next[size] = '\0';
				buf_long += buf_next;
			}
		}
		buf[size] = '\0';
		if(verbosity > 0) syslog(LOG_NOTICE, "recv[%s]\n", buf);
		res = parse_command(buf, size, client, 1, buf_long.c_str());
	}

	return 0;
}

void *manager_read_thread(void * arg) {

	char buf[BUFSIZE];
	string buf_long;
	int size;
	unsigned int    client;
	client = *(unsigned int *)arg;

	//cout << "New manager connect from: " << inet_ntoa((in_addr)clientInfo.sin_addr) << endl;
	if ((size = recv(client, buf, BUFSIZE - 1, 0)) == -1) {
		cerr << "Error in receiving data" << endl;
		close(client);
		return 0;
	} else {
		buf[size] = '\0';
		buf_long = buf;
		////cout << "DATA: " << buf << endl;
		if(size == BUFSIZE - 1 && !strstr(buf, "\r\n\r\n")) {
			char buf_next[BUFSIZE];
			////cout << "NEXT_RECV start" << endl;
			while((size = recv(client, buf_next, BUFSIZE - 1, 0)) > 0) {
				buf_next[size] = '\0';
				buf_long += buf_next;
				////cout << "NEXT DATA: " << buf_next << endl;
				////cout << "NEXT_RECV read" << endl;
				if(buf_long.find("\r\n\r\n") != string::npos) {
					break;
				}
			}
			////cout << "NEXT_RECV stop" << endl;
			size_t posEnd;
			if((posEnd = buf_long.find("\r\n\r\n")) != string::npos) {
				buf_long.resize(posEnd);
			}
		}
	}
	parse_command(buf, size, client, 0, buf_long.c_str());
	close(client);
	return 0;
}

void *manager_server(void *dummy) {
	sockaddr_in sockName;
	sockaddr_in clientInfo;
	socklen_t addrlen;

	// Vytvorime soket - viz minuly dil
	if ((manager_socket_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		cerr << "Cannot create manager tcp socket" << endl;
		return 0;
	}
	sockName.sin_family = AF_INET;
	sockName.sin_port = htons(opt_manager_port);
	//sockName.sin_addr.s_addr = INADDR_ANY;
	sockName.sin_addr.s_addr = inet_addr(opt_manager_ip);
	int on = 1;
	setsockopt(manager_socket_server, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
tryagain:
	if (bind(manager_socket_server, (sockaddr *)&sockName, sizeof(sockName)) == -1) {
		syslog(LOG_ERR, "Cannot bind to port [%d] trying again after 5 seconds intervals\n", opt_manager_port);
		sleep(5);
		goto tryagain;
	}
	// create queue with 100 connections max 
	if (listen(manager_socket_server, 100) == -1) {
		cerr << "Cannot create manager queue" << endl;
		return 0;
	}
	unsigned int ids;
	pthread_t threads;
	pthread_attr_t        attr;
	pthread_attr_init(&attr);
	/* set the thread detach state */
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	while(1 && terminating == 0) {
		addrlen = sizeof(clientInfo);
		int client = accept(manager_socket_server, (sockaddr*)&clientInfo, &addrlen);
		if(terminating == 1) {
			close(client);
			close(manager_socket_server);
			return 0;
		}
		if (client == -1) {
			//cerr << "Problem with accept client" <<endl;
			close(client);
			continue;
		}

		ids = client;
		pthread_create (                    /* Create a child thread        */
			&threads,                /* Thread ID (system assigned)  */    
			&attr,                   /* Default thread attributes    */
			manager_read_thread,               /* Thread routine               */
			&ids);                   /* Arguments to be passed       */
	}
	close(manager_socket_server);
	return 0;
}

void livesnifferfilter_s::updateState() {
	state_s new_state; 
	new_state.all_saddr = true;
	new_state.all_daddr = true;
	new_state.all_bothaddr = true;
	new_state.all_srcnum = true;
	new_state.all_dstnum = true;
	new_state.all_bothnum = true;
	new_state.all_siptypes = true;
	for(int i = 0; i < MAXLIVEFILTERS; i++) {
		if(this->lv_saddr[i]) {
			new_state.all_saddr = false;
		}
		if(this->lv_daddr[i]) {
			new_state.all_daddr = false;
		}
		if(this->lv_bothaddr[i]) {
			new_state.all_bothaddr = false;
		}
		if(this->lv_srcnum[i][0]) {
			new_state.all_srcnum = false;
		}
		if(this->lv_dstnum[i][0]) {
			new_state.all_dstnum = false;
		}
		if(this->lv_bothnum[i][0]) {
			new_state.all_bothnum = false;
		}
		if(this->lv_siptypes[i]) {
			new_state.all_siptypes = false;
		}
	}
	new_state.all_addr = new_state.all_saddr && new_state.all_daddr && new_state.all_bothaddr;
	new_state.all_num = new_state.all_srcnum && new_state.all_dstnum && new_state.all_bothnum;
	new_state.all_all = new_state.all_addr && new_state.all_num && new_state.all_siptypes;
	this->state = new_state;
}

void updateLivesnifferfilters() {
	livesnifferfilter_use_siptypes_s new_livesnifferfilterUseSipTypes;
	memset(&new_livesnifferfilterUseSipTypes, 0, sizeof(new_livesnifferfilterUseSipTypes));
	if(usersniffer.size()) {
		global_livesniffer = 1;
		map<unsigned int, livesnifferfilter_t*>::iterator usersnifferIT;
		for(usersnifferIT = usersniffer.begin(); usersnifferIT != usersniffer.end(); ++usersnifferIT) {
			usersnifferIT->second->updateState();
			if(usersnifferIT->second->state.all_siptypes) {
				new_livesnifferfilterUseSipTypes.u_invite = true;
				new_livesnifferfilterUseSipTypes.u_register = true;
				new_livesnifferfilterUseSipTypes.u_options = true;
				new_livesnifferfilterUseSipTypes.u_subscribe = true;
				new_livesnifferfilterUseSipTypes.u_message = true;
			} else {
				for(int i = 0; i < MAXLIVEFILTERS; i++) {
					if(usersnifferIT->second->lv_siptypes[i]) {
						switch(usersnifferIT->second->lv_siptypes[i]) {
						case INVITE:
							new_livesnifferfilterUseSipTypes.u_invite = true;
							break;
						case REGISTER:
							new_livesnifferfilterUseSipTypes.u_register = true;
							break;
						case OPTIONS:
							new_livesnifferfilterUseSipTypes.u_options = true;
							break;
						case SUBSCRIBE:
							new_livesnifferfilterUseSipTypes.u_subscribe = true;
							break;
						case MESSAGE:
							new_livesnifferfilterUseSipTypes.u_message = true;
							break;
						}
					}
				}
			}
		}
	} else {
		global_livesniffer = 0;
	}
	livesnifferfilterUseSipTypes = new_livesnifferfilterUseSipTypes;
	/*
	cout << "livesnifferfilterUseSipTypes" << endl;
	if(livesnifferfilterUseSipTypes.u_invite) cout << "INVITE" << endl;
	if(livesnifferfilterUseSipTypes.u_register) cout << "REGISTER" << endl;
	if(livesnifferfilterUseSipTypes.u_options) cout << "OPTIONS" << endl;
	if(livesnifferfilterUseSipTypes.u_subscribe) cout << "SUBSCRIBE" << endl;
	if(livesnifferfilterUseSipTypes.u_message) cout << "MESSAGE" << endl;
	*/
}

bool cmpCallBy_destroy_call_at(Call* a, Call* b) {
	return(a->destroy_call_at < b->destroy_call_at);   
}
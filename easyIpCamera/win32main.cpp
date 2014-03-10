#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "RTSPCommon.hh"

#define MAX_PARAMETER_LEN 32
#define CONFIG_FILE_PATH "\\config.ini"
#define CONFIG_ROOT	"general"
#define CONFIG_SERVER "server"
#define	CONFIG_PORT	"port"
#define	CONFIG_PUID	"device"
#define	CONFIG_SUFFIX "suffix"

UsageEnvironment* env;
char* server;
char* port;
char* puid;
char* suffix;
char timeStr[60];
FramedSource* audioSource;
FramedSource* videoSource;
RTPSink* audioSink;
RTPSink* videoSink;
DarwinInjector* injector;
Boolean isPlaying = false;
char loopMutex = ~0;

bool readLocalConfig();
bool RedirectStream(char const* ip, unsigned port);
char* printTime();

char eventLoopWatchVariable = 0;
bool play(); // forward
void pause(void* clientData);// forward
int main(int argc, char** argv) 
{
  // Begin by setting up our usage environment:
  TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  env = BasicUsageEnvironment::createNew(*scheduler);

  if(!readLocalConfig())
  {
	*env << printTime() << "load config error!\n";
	exit(1);
  }
  
  RedirectStream(server,::atoi(port));
  env->taskScheduler().doEventLoop(&eventLoopWatchVariable);

  return 0;
}

bool readLocalConfig()
{
	char *p = NULL;
	char exeFullPath[MAX_PARAMETER_LEN * 5];
	int len = GetModuleFileName(NULL,exeFullPath,  MAX_PARAMETER_LEN * 5);
	p=strrchr(exeFullPath, '\\'); 
	*p='\0';

	len = strlen(exeFullPath);
	char* path = CONFIG_FILE_PATH;
	::strcpy(exeFullPath+len,path);

	*env << printTime() << "load config:(" << exeFullPath << ")\n";

	server = new char[MAX_PARAMETER_LEN];
	len = GetPrivateProfileString(CONFIG_ROOT, CONFIG_SERVER, "127.0.0.1", server, MAX_PARAMETER_LEN,exeFullPath);
	if(len == 0) 
	{ 
		*env << printTime() << "read parameter 'server' fail...\n";
		return false;
	}

	port = new char[MAX_PARAMETER_LEN];
	len = GetPrivateProfileString(CONFIG_ROOT, CONFIG_PORT, "554", port, MAX_PARAMETER_LEN,exeFullPath);
	if(len == 0) 
	{ 
		*env << printTime() << "read parameter 'port' fail...\n"; 
		return false;
	}

	puid = new char[MAX_PARAMETER_LEN];
	len = GetPrivateProfileString(CONFIG_ROOT, CONFIG_PUID, "live", puid, MAX_PARAMETER_LEN,exeFullPath);
	if(len == 0) 
	{ 
		*env << printTime() << "read parameter 'puid' fail...\n"; 
		return false;
	}

	suffix = new char[MAX_PARAMETER_LEN];
		len = GetPrivateProfileString(CONFIG_ROOT, CONFIG_SUFFIX, "sdp", suffix, MAX_PARAMETER_LEN,exeFullPath);
	if(len == 0) 
	{ 
		*env << printTime() << "read parameter 'Suffix' fail...\n"; 
		return false;
	}
	return true;
}

bool RedirectStream(char const* ip, unsigned port)
{
  // Create a 'Darwin injector' object:
  injector = DarwinInjector::createNew(*env);

  ////////// AUDIO //////////
  struct in_addr dummyDestAddress;
  dummyDestAddress.s_addr = 0;
  Groupsock rtpGroupsockAudio(*env, dummyDestAddress, 0, 0);
  Groupsock rtcpGroupsockAudio(*env, dummyDestAddress, 0, 0);

  //AudioSink
  audioSink = MPEG4GenericRTPSink::createNew(*env, &rtpGroupsockAudio,
                                        97,
                                        44100,
                                        "audio", "AAC-hbr", "",
                                        2);
  const unsigned estimatedSessionBandwidthAudio = 160; // in kbps; for RTCP b/w share
  const unsigned maxCNAMElen = 100;
  unsigned char CNAME[maxCNAMElen+1];
  gethostname((char*)CNAME, maxCNAMElen);
  CNAME[maxCNAMElen] = '\0'; // just in case
  RTCPInstance* audioRTCP = NULL;//

  //RTCPInstance::createNew(*env, &rtcpGroupsockAudio,
		//	    estimatedSessionBandwidthAudio, CNAME,
		//	    audioSink, NULL /* we're a server */);
  injector->addStream(audioSink, audioRTCP);
  ////////// END AUDIO //////////

  ////////// VIDEO //////////
  Groupsock rtpGroupsockVideo(*env, dummyDestAddress, 0, 0);
  Groupsock rtcpGroupsockVideo(*env, dummyDestAddress, 0, 0);

  // VideoSink
  videoSink = H264VideoRTPSink::createNew(*env,&rtpGroupsockVideo,96);

  const unsigned estimatedSessionBandwidthVideo = 4500; // in kbps; for RTCP b/w share
  RTCPInstance* videoRTCP = NULL;//

    //RTCPInstance::createNew(*env, &rtcpGroupsockVideo,
			 //     estimatedSessionBandwidthVideo, CNAME,
			 //     videoSink, NULL /* we're a server */);

  injector->addStream(videoSink, videoRTCP);
  ////////// END VIDEO //////////

  char remoteFilename[MAX_PARAMETER_LEN];
  sprintf(remoteFilename,"%s.%s", puid, suffix); 
  // Next, specify the destination Darwin Streaming Server:
  if (!injector->setDestination(ip, remoteFilename,
				"liveUSBCamera", "LIVE555 Streaming Media", port)) {
    *env << printTime() << "injector->setDestination() failed: " << env->getResultMsg() << "\n";
    return false;
  }

  if(play())
  {
		return true;
  }
  else
  {
		*env << printTime() << "inject error\n";

		audioSink->stopPlaying();
		Medium::close(audioSource);

		videoSink->stopPlaying();
		Medium::close(videoSource);
		Medium::close(*env, injector->name());
		injector == NULL;
		return false;
  }
}


void afterPlaying(void* clientData) {

  if (audioSource->isCurrentlyAwaitingData() || videoSource->isCurrentlyAwaitingData()) return;

  if(audioSource != NULL)
  {
	audioSink->stopPlaying();
	Medium::close(audioSource);
  }

  videoSink->stopPlaying();
  Medium::close(videoSource);

  
  // Start playing once again:
  if(!play())
	  isPlaying = false;
  //isPlaying = false;
}

bool play() {
	bool audioValid = true;
	// ¨°??¦Ì2?¨ºy?¨¦¡Á??¡§¨°?
	FramedSource* audioES = AudioInputMicDevice::createNew(*env,16,2,44100);
	audioSource = audioES;

	FramedSource* videoES = CamH264VideoStreamFramer::createNew(*env, NULL);
	videoSource = videoES;
	
	//video invalid
	if(videoSource == NULL)
		return false;
	//audio invalid
	if(audioSource == NULL)
		audioValid = false;

  // Finally, start playing each sink.
  *env << printTime() << "Beginning to get camera video...\n";
  videoSink->startPlaying(*videoSource, afterPlaying, videoSink);
  videoSink->SetOnDarwinClosure(pause, (void*)videoSink);

  {
	  audioSink->startPlaying(*audioSource, afterPlaying, audioSink);
	  audioSink->SetOnDarwinClosure(pause, (void*)audioSink);
  }
  return true;
}

void pause(void* clientData)
{
	*env << printTime() << "error:stop to inject stream to server\n";

	audioSink->stopPlaying();
	Medium::close(audioSource);

	videoSink->stopPlaying();
	Medium::close(videoSource);
	Medium::close(*env, injector->name());
	injector == NULL;
	isPlaying = false;
}

char* printTime()
{
	SYSTEMTIME sys; 
	GetLocalTime( &sys ); 
	sprintf(timeStr,"%4d/%02d/%02d-%02d:%02d:%02d %03d: \0",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute,sys.wSecond,sys.wMilliseconds); 
	return timeStr;
}
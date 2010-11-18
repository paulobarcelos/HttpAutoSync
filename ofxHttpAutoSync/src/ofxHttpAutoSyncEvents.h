/*
 *  ofxHttpAutoSyncEvents.h
 *  ofxHttpAutoSync
 *
 *  Paulo Barcelos, October 2010.
 *	
 */

#ifndef _OFX_APP_UPDATER_EVENTS
#define _OFX_APP_UPDATER_EVENTS


#include "ofMain.h"
#include "ofxXmlSettings.h"

class ofxHttpAutoSyncInitEventArgs {
public:
	int				numFiles;
	ofxXmlSettings	syncFiles;
};
class ofxHttpAutoSyncAbortEventArgs {
public:
	string			message;
};

class ofxHttpAutoSyncCompleteEventArgs {
public:
	int				errors;
};

class ofxHttpAutoSyncStartEventArgs {
public:
	int				numFiles;
};

class ofxHttpAutoSyncEndEventArgs {
public:
	int				errors;
};

class ofxHttpAutoSyncUpdateEventArgs {
public:
	bool			sucess;
	string			path;
};

class ofxHttpAutoSyncEvents{
public:
	
	ofEvent<ofxHttpAutoSyncInitEventArgs>		initEvent;
	ofxHttpAutoSyncInitEventArgs				initEventArgs;
	
	ofEvent<ofxHttpAutoSyncAbortEventArgs>		abortEvent;
	ofxHttpAutoSyncAbortEventArgs				abortEventArgs;
	
	ofEvent<ofxHttpAutoSyncCompleteEventArgs>	completeEvent;
	ofxHttpAutoSyncCompleteEventArgs			completeEventArgs;
	

	
	ofEvent<ofxHttpAutoSyncStartEventArgs>	startDeleteEvent;
	ofxHttpAutoSyncStartEventArgs				startDeleteEventArgs;
	
	ofEvent<ofxHttpAutoSyncStartEventArgs>	startDirCreationEvent;
	ofxHttpAutoSyncStartEventArgs				startDirCreationEventArgs;
	
	ofEvent<ofxHttpAutoSyncStartEventArgs>	startDonwloadEvent;
	ofxHttpAutoSyncStartEventArgs				startDonwloadEventArgs;
	
	

	ofEvent<ofxHttpAutoSyncEndEventArgs>		endDeleteEvent;
	ofxHttpAutoSyncEndEventArgs				endDeleteEventArgs;
	
	ofEvent<ofxHttpAutoSyncEndEventArgs>		endDirCreationEvent;
	ofxHttpAutoSyncEndEventArgs				endDirCreationEventArgs;
	
	ofEvent<ofxHttpAutoSyncEndEventArgs>		endDonwloadEvent;
	ofxHttpAutoSyncEndEventArgs				endDonwloadEventArgs;
	
	
	
	ofEvent<ofxHttpAutoSyncUpdateEventArgs>	deletedEvent;
	ofxHttpAutoSyncUpdateEventArgs			deletedEventArgs;		
	
	ofEvent<ofxHttpAutoSyncUpdateEventArgs>	dirCreatedEvent;
	ofxHttpAutoSyncUpdateEventArgs			dirCreatedEventArgs;		
	
	ofEvent<ofxHttpAutoSyncUpdateEventArgs>	donwloadedEvent;
	ofxHttpAutoSyncUpdateEventArgs			donwloadedEventArgs;
};
#endif
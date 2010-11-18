/*
 *  ofxHttpAutoSync.h
 *  ofxHttpAutoSync
 *
 *  Paulo Barcelos, October 2010.
 *	
 */

#ifndef _OFX_APP_UPDATER
#define _OFX_APP_UPDATER

#include "ofMain.h"
#include "ofxHttpAutoSyncEvents.h"
#include "ofxHttpUtils.h"
#include "ofxFileHelper.h"
#include "ofxDirList.h"
#include "ofxXmlSettings.h"
#include <sstream>

////////////////////////////////////////////////////////////
// CONSTANTS -----------------------------------------------
////////////////////////////////////////////////////////////
// This is script MUST be inside the remote directory
#define REMOTE_FILE_LIST_GENERATOR					"HttpAutoSync.php"
// This is a hardcoded version of the serside script, to be used if the php file get lost
#define FILE_LIST_GENERATOR_EMERGENCY_CONTENT		"<?php\n$home_dir = realpath(getcwd());\n$objects = new RecursiveIteratorIterator(new RecursiveDirectoryIterator($home_dir), RecursiveIteratorIterator::SELF_FIRST);\necho '<?xml version=\"1.0\" encoding=\"UTF-8\" ?>'.\"\\n\";\necho \"<FILES>\\n\";\nforeach($objects as $name => $object){\n    $dir = $object->isDir();\n    if(!$dir){\n        $dir = 0;\n    }\n    echo \"<FILE>\\n\";\n    echo \"<PATH>\".str_replace($home_dir.\"/\", \"\", $name).\"</PATH>\\n\";\n    echo \"<LAST_MODIFIED>\".$object->getMTime().\"</LAST_MODIFIED>\\n\";\n    echo \"<IS_DIRECTORY>\".$dir.\"</IS_DIRECTORY>\\n\";\n    echo \"</FILE>\\n\";\n}\necho \"</FILES>\\n\";\n?>"

// This is the prefix name that will be added to any modified file that was busy on deletion time, this way we can be sure it will be deleted on the next update.
#define OLD_FILE_PREFIX					"____old____"

////////////////////////////////////////////////////////////
// STRUCTS -------------------------------------------------
////////////////////////////////////////////////////////////
struct FileInformation {
	string		path;
	int			lastModified;
};
////////////////////////////////////////////////////////////
// CLASS DEFINITION ----------------------------------------
////////////////////////////////////////////////////////////
class ofxHttpAutoSync{

	public:
		ofxHttpAutoSync();
		~ofxHttpAutoSync();
		void				sync(string remoteDirectoryURL);
		void				setVerbose(bool verbose);
	
		// IGNORES ----------------------------------------------------
		// All ignore commands are case sensitive!
		void				ignoreExtension(string extension);
							//ignoreExtension("jpg") -> will ignore only JPEG type of file (Don't use the dot ".")
							//ignoreExtension("*") -> will ignore all files (good if you want to clone just a directory structure)
		void				ignoreDirectoryName(string name);
							//ignoreDirectoryName("ignore") -> will ignore all directories named "ignore"
		void				ignoreFileName(string name, bool useExtension = true);
							//ignoreFileName("test", false) -> will ignore all "test.txt", "text.exe", etc...
							//ignoreFileName("test.txt", true) -> will ignore only "test.txt" files. 
		void				ignorePath(string path);
							//ignorePath("data/example.txt") -> will ignore only that specific file
							//ignorePath("data/example") -> will ignore that specific directory (No backslash at the end!)
	
		bool				willBeIgnored(string path);
							//use this to test if a file/directory will be ignore by any of the above rules
	
		//events
		ofxHttpAutoSyncEvents	events;
				
	private:

		string				remoteDirectoryURL;
		string				appName;
		string				remoteListURL;
	
		void				listLoaderResponse(ofxHttpResponse & response);
		void				onFileLoaderResponse(ofxHttpResponse & response);
	
		vector<string>		ignoredExtensions,	ignoredDirectoryNames,	ignoredFileNames, ignoredPaths;
		vector<bool>													ignoredFileNamesUseExtension;
	
		void				requestRemoteList();
		void				populateLocalList();
		void				recursiveDirList(ofxXmlSettings fileList, string dirPath);
		void				compareLists();	
		ofxXmlSettings		localFiles, remoteFiles, syncFiles, ignoredFiles;
		ofxHttpUtils		listLoader, fileLoader;
	
		void				init();
		void				startDelete();
		void				startDirCreation();
		void				startDownload();
		void				recursiveDowloadFiles(vector<FileInformation*> fileList);
		vector<FileInformation*>	filesToDonwload;
	
		bool				verbose;
	
		// EVENT HANDLERS ---------------------------------------------
		void				addListeners();
		void				removeListeners();
	
		void				onInit(ofxHttpAutoSyncInitEventArgs &args);
		void				onAbort(ofxHttpAutoSyncAbortEventArgs &args);
		void				onComplete(ofxHttpAutoSyncCompleteEventArgs &args);
	
		void				onStartDelete(ofxHttpAutoSyncStartEventArgs &args);
		void				onStartDirCreation(ofxHttpAutoSyncStartEventArgs &args);
		void				onStartDownload(ofxHttpAutoSyncStartEventArgs &args);
	
		void				onEndDelete(ofxHttpAutoSyncEndEventArgs &args);
		void				onEndDirCreation(ofxHttpAutoSyncEndEventArgs &args);
		void				onEndDownload(ofxHttpAutoSyncEndEventArgs &args);
	
		void				onDelete(ofxHttpAutoSyncUpdateEventArgs &args);
		void				onDirCreated(ofxHttpAutoSyncUpdateEventArgs &args);
		void				onDownloaded(ofxHttpAutoSyncUpdateEventArgs &args);
		
		// UTILS ------------------------------------------------------
		
		// Always when we mess with files, we wanna make sure the dataPathRoot is the folder where the app lies.
		// As the user could have changed the dataPathRoot previously, we first store the curent dataPathRoot,
		// then root it to where we want and do the changes, and finnaly restore it to the previous value.
		void				rootDataPath();
		void				restoreDataPath();
		bool				isDataPathRooted;
		string				currentDataPathRoot;
		string				rootedDataPathRoot;
	
		// The xml list of files can't have any anomaly, us this method to test if the xml structure is healthy
		bool				hasAnomalies(ofxXmlSettings xml);

};

#endif
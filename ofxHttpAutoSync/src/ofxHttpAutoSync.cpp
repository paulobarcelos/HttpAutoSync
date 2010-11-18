/*
 *  ofxHttpAutoSync.h
 *  ofxHttpAutoSync
 *
 *  Paulo Barcelos, October 2010.
 *	
 */

#include "ofxHttpAutoSync.h"

///////////////////////////////////////////////////////////////////////////////////
// Constructor --------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
ofxHttpAutoSync::ofxHttpAutoSync(){
	verbose = true;
	isDataPathRooted = false;
	ignorePath(REMOTE_FILE_LIST_GENERATOR);
}
///////////////////////////////////////////////////////////////////////////////////
// Destructor ---------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
ofxHttpAutoSync::~ofxHttpAutoSync(){
	
}

///////////////////////////////////////////////////////////////////////////////////
// update -------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::sync(string remoteDirectory){	
	remoteDirectoryURL = remoteDirectory;
	addListeners();	
	requestRemoteList();	
}
///////////////////////////////////////////////////////////////////////////////////
// setVerbose ---------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::setVerbose(bool verbose){	
	this->verbose = verbose;
}
///////////////////////////////////////////////////////////////////////////////////
// ignoreExtension ----------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::ignoreExtension(string extension){	
	ignoredExtensions.push_back(extension);
}
///////////////////////////////////////////////////////////////////////////////////
// ignoreDirectoryName ------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::ignoreDirectoryName(string name){	
	ignoredDirectoryNames.push_back(name);
}
///////////////////////////////////////////////////////////////////////////////////
// ignoreFileName -----------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::ignoreFileName(string name, bool useExtension){	
	ignoredFileNames.push_back(name);
	ignoredFileNamesUseExtension.push_back(useExtension);
}
///////////////////////////////////////////////////////////////////////////////////
// ignorePath ---------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::ignorePath(string path){	
	ignoredPaths.push_back((path));
}
///////////////////////////////////////////////////////////////////////////////////
// willBeIgnored ------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
bool ofxHttpAutoSync::willBeIgnored(string path){
	// Check if the file is inside of an ignored path
	if(ignoredPaths.size()>0)
	{
		for (vector<string>::iterator it = ignoredPaths.begin(); it!=ignoredPaths.end(); ++it) {
			size_t foundAt = path.find(*it);
			if(foundAt == 0)
			{
				string lastChar = path.substr (string(*it).length(),1);
				if ( lastChar == "/" || lastChar == "\\" || lastChar == "") return true;
			}
		}
		
	}
	
	// Check if the file is inside of an ignored directory name
	if(ignoredDirectoryNames.size()>0)
	{
		for (vector<string>::iterator it = ignoredDirectoryNames.begin(); it!=ignoredDirectoryNames.end(); ++it) {
			size_t foundAt = path.find(*it);
			if(foundAt != std::string::npos)
			{
				string lastChar = path.substr (foundAt+string(*it).length(),1);
				if ( lastChar == "/" || lastChar == "\\" || lastChar == "") return true;
			}
			
		}
		
	}
	
	// Check if the file has an ignore extension
	if(ignoredExtensions.size()>0)
	{
		for (vector<string>::iterator it = ignoredExtensions.begin(); it!=ignoredExtensions.end(); ++it) {
			if(ofxFileHelper::getFileExt(path) != "")
			{
				if(*it == "*")return true;
			
				if(ofxFileHelper::getFileExt(path) == *it) return true;
			}
		}
			
	}
	
	// Check if the file is inside of an ignored directory name
	if(ignoredFileNames.size()>0)
	{
		string filename = ofxFileHelper::getFilename(path);
		vector<bool>::iterator boolIt = ignoredFileNamesUseExtension.begin();
		for (vector<string>::iterator it = ignoredFileNames.begin(); it!=ignoredFileNames.end(); ++it) {
			
			if(*boolIt)
			{
				if(ofxFileHelper::getFilename(path) == *it) return true;
			}
			else 
			{
				if(ofxFileHelper::removeExt(ofxFileHelper::getFilename(path)) == *it) return true;
			}
		
			++boolIt;
		}
		
	}
	
	return false;
}
///////////////////////////////////////////////////////////////////////////////////
// requestRemoteList --------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::requestRemoteList(){
	ofAddListener(listLoader.newResponseEvent, this, &ofxHttpAutoSync::listLoaderResponse);
	
	remoteListURL = remoteDirectoryURL + REMOTE_FILE_LIST_GENERATOR;
	if(verbose) printf("ofxHttpAutoSync - Requesting remote list from %s\n", remoteListURL.c_str());
	
	// try to load, if not we can abort straight away.
	if(!listLoader.getUrl(remoteListURL))
	{
		// dispatch event
		events.abortEventArgs.message = "Host not found. (Please check host URL and internet connection)";
		ofNotifyEvent(events.abortEvent, events.abortEventArgs);
	}
}
///////////////////////////////////////////////////////////////////////////////////
// listLoaderResponse -------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::listLoaderResponse(ofxHttpResponse & response){
	
	if(verbose) printf("ofxHttpAutoSync - Remote list request response: %d\n", response.status);
	
	// If the http response is sucessfull
	if (response.status == 200){
		// Adjust the date path before messign with files
		rootDataPath();
		//create temp xml file
		ofstream myfile;
		string filePath = ofToDataPath("tempremotelist.xml", false);
		myfile.open ( filePath.c_str() );
		myfile << response.responseBody;
		myfile.close();
		
		//load the xml file into an xml settings
		remoteFiles.loadFile("tempremotelist.xml");
		
		//delete temp xml file
		ofxFileHelper::deleteFile( "tempremotelist.xml", true );
		// Restore the date path to the original
		restoreDataPath();
		
		// Check if the xml structure is healthy
		if(!hasAnomalies(remoteFiles))
		{
			if(verbose)
			{
				printf("\n--------------------------------------------\nofxHttpAutoSync - Remote files:\n\n");
				remoteFiles.pushTag("FILES",0);
				for(int i = 0; i < remoteFiles.getNumTags("FILE"); i++)
				{
					printf("%i ", remoteFiles.getValue("FILE:LAST_MODIFIED", 0, i));
					printf("%s\n", remoteFiles.getValue("FILE:PATH", "", i).c_str());
				}
				remoteFiles.popTag();
			}
			
			//if we could successfully donwload thre remote list, we go ahead and populate the local list
			populateLocalList();
			// and then compare the local and the remote list to generate the sync list
			compareLists();
			// With the sync list ready we can go on and start syncing the folders
			init();
		}
		else
		{
			// if we found anomalies on the loaded xml file, something was wrong on the server resposne
			// dispatch event
			events.abortEventArgs.message = "Anomalies found on the remote list. Please check the code at "+remoteDirectoryURL+REMOTE_FILE_LIST_GENERATOR+"\nIf you are not using a custom script, "+remoteDirectoryURL+REMOTE_FILE_LIST_GENERATOR+" should look like:\n"+FILE_LIST_GENERATOR_EMERGENCY_CONTENT;
			ofNotifyEvent(events.abortEvent, events.abortEventArgs);		
		}
	}
	else
	{
		// If we coudln't load the remote list, the update process is aborted
		if(verbose) printf("ofxHttpAutoSync - error loading remote list from  %s\n", remoteListURL.c_str());
		// dispatch event
		events.abortEventArgs.message = "Error loading remote list.";
		ofNotifyEvent(events.abortEvent, events.abortEventArgs);		
	}
}
///////////////////////////////////////////////////////////////////////////////////
// populateLocalList --------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::populateLocalList(){
	// Clear the list of items, and start fresh
	localFiles.clear();
	localFiles.addTag("FILES");
	localFiles.pushTag("FILES",0);
		// Fill the list of items using recursive search through the local directory
		rootDataPath();
			recursiveDirList(localFiles, rootedDataPathRoot);
		restoreDataPath();
	localFiles.popTag();

	
	if(verbose)
	{
		printf("\n--------------------------------------------\nofxHttpAutoSync - Local files:\n\n");
		localFiles.pushTag("FILES",0);
		for(int i = 0; i < localFiles.getNumTags("FILE"); i++)
		{
			printf("%i ", localFiles.getValue("FILE:LAST_MODIFIED", 0, i));
			printf("%s\n", localFiles.getValue("FILE:PATH", "", i).c_str());
		}
		localFiles.popTag();
	}
	
}
///////////////////////////////////////////////////////////////////////////////////
// recursiveDirList ---------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::recursiveDirList(ofxXmlSettings fileList, string dirPath){
	
	ofxDirList dir;
	int numFiles = dir.listDir(dirPath);
	for(int i = 0; i < numFiles; i++){
		string path =  dirPath+dir.getName(i);
		string rootedPath =  path.substr(rootedDataPathRoot.length(), path.length()-rootedDataPathRoot.length());

		time_t	lastModified = ofxFileHelper::getLastModifiedDate(path, true).epochTime();
		std::stringstream lastModifiedsstr;
		lastModifiedsstr << lastModified;
		bool isDirectory = ofxFileHelper::isDirectory(path, true);
		
		int totalFiles = fileList.getNumTags("FILE");
		fileList.setValue("FILE:PATH", rootedPath, totalFiles);
		fileList.setValue("FILE:LAST_MODIFIED", lastModifiedsstr.str(), totalFiles);
		fileList.setValue("FILE:IS_DIRECTORY", isDirectory, totalFiles);
			
		if(isDirectory) recursiveDirList (localFiles, path+"/");
		
	}
	
}

///////////////////////////////////////////////////////////////////////////////////
// compareLists -------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::compareLists(){
	// First create the structure for the sync list
	syncFiles.addTag("FILES");
	syncFiles.pushTag("FILES",0);
	
	// Then make sure the local and remote lists are in the "FILES" level
	localFiles.pushTag("FILES",0);
	remoteFiles.pushTag("FILES",0);
	
	
	
	
	// Now the actual work starts...
	// First loop throght the list of local files
	for(int i = 0; i < localFiles.getNumTags("FILE"); i++)
	{
		// Check if the file needs to be ignored
		if( willBeIgnored(localFiles.getValue("FILE:PATH", "", i)))
		{
		   // If so, we flag it as "I" - ignored
		   int totalFiles = syncFiles.getNumTags("FILE");
		   syncFiles.setValue("FILE:PATH", localFiles.getValue("FILE:PATH", "", i), totalFiles);
		   syncFiles.setValue("FILE:IS_DIRECTORY", localFiles.getValue("FILE:IS_DIRECTORY", 0, i), totalFiles);
		   syncFiles.setValue("FILE:LAST_MODIFIED", localFiles.getValue("FILE:LAST_MODIFIED", 0, i), totalFiles);
		   syncFiles.setValue("FILE:SYNC_ACTION", "I", totalFiles);
		   syncFiles.setValue("FILE:IS_IGNORE_LOCAL", 1, totalFiles); // to check later if the files were ignored from local or from remote
		}
		else
		{
		   // If the file is not ignored...
		   // Check if it needs to be deleted
		   bool deleteFile = true;
		   // For all local files, loop through the remote files
		   for(int j = 0; j < remoteFiles.getNumTags("FILE"); j++)
		   {
				// if the file path matches...
				if(localFiles.getValue("FILE:PATH", "", i) == remoteFiles.getValue("FILE:PATH", "", j))
				{
					//... then we know the file exists in the remote server
					deleteFile = false;
					break;
				}
		   }
		   if(deleteFile)
		   {
				// If the files don't exist in the remote server, we flag it as "D" - delete
				int totalFiles = syncFiles.getNumTags("FILE");
				syncFiles.setValue("FILE:PATH", localFiles.getValue("FILE:PATH", "", i), totalFiles);
				syncFiles.setValue("FILE:IS_DIRECTORY", localFiles.getValue("FILE:IS_DIRECTORY", 0, i), totalFiles);
				syncFiles.setValue("FILE:LAST_MODIFIED", localFiles.getValue("FILE:LAST_MODIFIED", 0, i), totalFiles);
				syncFiles.setValue("FILE:SYNC_ACTION", "D", totalFiles);
		   }
		}
	}
		   
		   
		   
		   
		   
		   
	// Now check for modified and new files
	for(int i = 0; i < remoteFiles.getNumTags("FILE"); i++)
	{
		// Check if the file needs to be ignored
		if( willBeIgnored(remoteFiles.getValue("FILE:PATH", "", i)))
		{
			// If so, we flag it as "I" - ignored
			int totalFiles = syncFiles.getNumTags("FILE");
			syncFiles.setValue("FILE:PATH", remoteFiles.getValue("FILE:PATH", "", i), totalFiles);
			syncFiles.setValue("FILE:IS_DIRECTORY", remoteFiles.getValue("FILE:IS_DIRECTORY", 0, i), totalFiles);
			syncFiles.setValue("FILE:LAST_MODIFIED", remoteFiles.getValue("FILE:LAST_MODIFIED", 0, i), totalFiles);
			syncFiles.setValue("FILE:SYNC_ACTION", "I", totalFiles);
			syncFiles.setValue("FILE:IS_IGNORE_LOCAL", 0, totalFiles); // to check later if the files were ignored from local or from remote
		}
		else
		{
			// So, for all remote files, loop through the local files
			bool fileIsNew = true;
			bool fileIsIgnored = false;
			for(int j = 0; j < localFiles.getNumTags("FILE"); j++)
			{
				// If the path matches...
				if(remoteFiles.getValue("FILE:PATH", "", i) == localFiles.getValue("FILE:PATH", "", j))
				{
					//... check if the local file has a diferent last modified timestamp from the remote file
					if(remoteFiles.getValue("FILE:LAST_MODIFIED", 0, i) != localFiles.getValue("FILE:LAST_MODIFIED", 0, j))
					{
						//If so, we flag it as "M" - modify
						int totalFiles = syncFiles.getNumTags("FILE");
						syncFiles.setValue("FILE:PATH", remoteFiles.getValue("FILE:PATH", "", i), totalFiles);
						syncFiles.setValue("FILE:IS_DIRECTORY", remoteFiles.getValue("FILE:IS_DIRECTORY", 0, i), totalFiles);
						syncFiles.setValue("FILE:LAST_MODIFIED", remoteFiles.getValue("FILE:LAST_MODIFIED", 0, i), totalFiles);
						syncFiles.setValue("FILE:SYNC_ACTION", "M", totalFiles);
					}
					// if we get here, we are shure this file is not new, and was only modified
					fileIsNew = false;
				}
			}
			if(fileIsNew)
			{
				// If the files is new, we flag it as "A" - add
				int totalFiles = syncFiles.getNumTags("FILE");
				syncFiles.setValue("FILE:PATH", remoteFiles.getValue("FILE:PATH", "", i), totalFiles);
				syncFiles.setValue("FILE:IS_DIRECTORY", remoteFiles.getValue("FILE:IS_DIRECTORY", 0, i), totalFiles);
				syncFiles.setValue("FILE:LAST_MODIFIED", remoteFiles.getValue("FILE:LAST_MODIFIED", 0, i), totalFiles);
				syncFiles.setValue("FILE:SYNC_ACTION", "A", totalFiles);
			}
		}
		
	}
		   
		   
		   
		   
		   
	// Now put pop all the tags back to root
	syncFiles.popTag();
	
	localFiles.popTag();
	
	remoteFiles.popTag();
}

///////////////////////////////////////////////////////////////////////////////////
// init -----------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::init(){
	syncFiles.pushTag("FILES",0);
	int numFiles = 0;
	for(int i = 0; i < syncFiles.getNumTags("FILE"); i++)
	{
		if(syncFiles.getValue("FILE:SYNC_ACTION", "", i) != "I") numFiles ++;
	}
	syncFiles.popTag();
	// dispatch event
	events.initEventArgs.numFiles = numFiles;
	events.initEventArgs.syncFiles = syncFiles;
	ofNotifyEvent(events.initEvent, events.initEventArgs);
	
	// Start by removing the unecessary files;
	startDelete();

	// Then create the folders that are missing
	startDirCreation();
	
	// To finaly start donwloding new files
	startDownload();
}

///////////////////////////////////////////////////////////////////////////////////
// startDelete ---------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::startDelete(){
	syncFiles.pushTag("FILES",0);
	
	int numFiles = 0;
	for(int i = 0; i < syncFiles.getNumTags("FILE"); i++)
	{
		if(syncFiles.getValue("FILE:SYNC_ACTION", "", i) == "D") numFiles ++;
	}
	// dispatch event
	events.startDeleteEventArgs.numFiles = numFiles;
	ofNotifyEvent(events.startDeleteEvent, events.startDeleteEventArgs);
	
	// As we just started, set the errors to 0
	events.endDeleteEventArgs.errors = 0;

	// Adjust the date path before messign with files
	rootDataPath();	
	for(int i = 0; i < syncFiles.getNumTags("FILE"); i++)
	{
		if(syncFiles.getValue("FILE:SYNC_ACTION", "", i) == "D")
		{
			string path = syncFiles.getValue("FILE:PATH", "", i);
			
			// As when we delete the whole content of a directory when we find one,
			// we need to always check if the current file exists. It could be already be deleted...
			// If so, we go ahead and dispach the delete event anyway, so we can keep track of everything.
			if(verbose) printf("Trying to delete %s\n",path.c_str());
			if(ofxFileHelper::doesFileExist(path , true))
			{
				// Check if it is a file
				if(!ofxFileHelper::isDirectory(path , true))
				{
					// Try to delete the file
					if(ofxFileHelper::deleteFile(path , true)) events.deletedEventArgs.sucess = true;
					else 
					{
						// if we couldn't delete, it means the file was busy, so we just rename it 
						string newname = OLD_FILE_PREFIX+ofxFileHelper::getFilename(path);
						if(ofxFileHelper::renameTo(path, newname, true))
						{
							if(verbose) printf("%s was renamed to %s\n", path.c_str(), newname.c_str());
						}
						else
						{
							// the deletedEvent sucess with false
							events.deletedEventArgs.sucess = false;
							// and increase the error list
							events.endDeleteEventArgs.errors++;
						}
					}
				}
				else
				{
					// Try to delete all the content from the directory
					if(ofxFileHelper::deleteFolder(path , true)) events.deletedEventArgs.sucess = true;
					else 
					{
						// if we couldn't delete, it means the file was busy, so we just rename it 
						string newname = OLD_FILE_PREFIX+ofxFileHelper::getFilename(path);
						if(ofxFileHelper::renameTo(path, newname, true))
						{
							if(verbose) printf("%s was renamed to %s\n", path.c_str(), newname.c_str());
						}
						else
						{
							// the deletedEvent sucess with false
							events.deletedEventArgs.sucess = false;
							// and increase the error list
							events.endDeleteEventArgs.errors++;
						}
					}
				}
				// finally dispatch the event
				events.deletedEventArgs.path = path;
				ofNotifyEvent(events.deletedEvent, events.deletedEventArgs);
			}
			else
			{
				events.deletedEventArgs.sucess = true;
				events.deletedEventArgs.path = path;
				ofNotifyEvent(events.deletedEvent, events.deletedEventArgs);
			}
		}   
	}
	
	// if we arrived here, it means all the delete operations are done, so dipatch the event
	ofNotifyEvent(events.endDeleteEvent, events.endDeleteEventArgs);
	// Restore the date path to the original
	restoreDataPath();
	
	// get the xml back to root
	syncFiles.popTag();
	
}

///////////////////////////////////////////////////////////////////////////////////
// startDirCreation ----------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::startDirCreation(){
	syncFiles.pushTag("FILES",0);
		
	int numFiles = 0;
	for(int i = 0; i < syncFiles.getNumTags("FILE"); i++)
	{
		// Check for files need to be added("A"), and if they are directories (we olny care about new directories now)
		if(syncFiles.getValue("FILE:SYNC_ACTION", "", i) == "A" && syncFiles.getValue("FILE:IS_DIRECTORY", 0, i)) numFiles ++;
	}
	// dispatch event
	events.startDirCreationEventArgs.numFiles = numFiles;
	ofNotifyEvent(events.startDirCreationEvent, events.startDirCreationEventArgs);
	
	// As we just started, set the errors to 0
	events.endDirCreationEventArgs.errors = 0;
	
	// Adjust the date path before messign with files
	rootDataPath();	
	for(int i = 0; i < syncFiles.getNumTags("FILE"); i++)
	{
		// Check for files need to be added("A"), and if they are directories (we olny care about new directories now)
		if(syncFiles.getValue("FILE:SYNC_ACTION", "", i) == "A" && syncFiles.getValue("FILE:IS_DIRECTORY", 0, i))
		{
			string path = syncFiles.getValue("FILE:PATH", "", i);
			// Try to create the directory
			if(ofxFileHelper::makeDirectory(path , true))
			{
				// if sucessfull flag the event with true
				events.dirCreatedEventArgs.sucess = true;
			}
			else 
			{
				// if fail flag the event with false
				events.dirCreatedEventArgs.sucess = true;
				// and increase the error list
				events.endDirCreationEventArgs.errors++;	
			}
			// finally dispatch the event
			events.dirCreatedEventArgs.path = path;
			ofNotifyEvent(events.dirCreatedEvent, events.dirCreatedEventArgs);
		}
	}

	// if we arrived here, it means all the directory creation operations are done, so dipatch the event
	ofNotifyEvent(events.endDirCreationEvent, events.endDirCreationEventArgs);
	
	// Restore the date path to the original
	restoreDataPath();
	
	// get the xml back to root
	syncFiles.popTag();
	
}
///////////////////////////////////////////////////////////////////////////////////
// startDownload -------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::startDownload(){
	syncFiles.pushTag("FILES",0);
	
	int numFiles = 0;
	for(int i = 0; i < syncFiles.getNumTags("FILE"); i++)
	{
		// Check for files need to be added("A") or modified ("M")...
		if(syncFiles.getValue("FILE:SYNC_ACTION", "", i) == "A" || syncFiles.getValue("FILE:SYNC_ACTION", "", i) == "M")
		{
			//... and if they are not directorie (we want to ignore the directories)
			if(!syncFiles.getValue("FILE:IS_DIRECTORY", 0, i))  numFiles ++;
		}
	}
	// dispatch event
	events.startDonwloadEventArgs.numFiles = numFiles;
	ofNotifyEvent(events.startDonwloadEvent, events.startDonwloadEventArgs);
	
	// As we just stared, set the errors to 0
	events.endDonwloadEventArgs.errors = 0;
	
		
	for(int i = 0; i < syncFiles.getNumTags("FILE"); i++)
	{
		// Check for files need to be added("A") or modified ("M")...
		if(syncFiles.getValue("FILE:SYNC_ACTION", "", i) == "A" || syncFiles.getValue("FILE:SYNC_ACTION", "", i) == "M")
		{
			//... and if they are not directorie (we want to ignore the directories)
			if(!syncFiles.getValue("FILE:IS_DIRECTORY", 0, i))
			{
				FileInformation * fileInfo = new FileInformation();
				fileInfo->path = syncFiles.getValue("FILE:PATH", "", i);
				fileInfo->lastModified = syncFiles.getValue("FILE:LAST_MODIFIED", 0, i);
				filesToDonwload.push_back(fileInfo);
			}
		}
	}
	
		
	// If there is files to download, start donwloading them, if not just call the download end straight away
	if(filesToDonwload.size()>0) recursiveDowloadFiles(filesToDonwload);
	else ofNotifyEvent(events.endDonwloadEvent, events.endDonwloadEventArgs);
		
	syncFiles.popTag();
	
}
///////////////////////////////////////////////////////////////////////////////////
// recursiveDowloadFiles ----------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::recursiveDowloadFiles(vector<FileInformation*> fileList){
	string fileUrl = remoteDirectoryURL + fileList[0]->path;
	if(verbose) printf("Trying to donwload %s\n",fileUrl.c_str());
	ofAddListener(fileLoader.newResponseEvent, this, &ofxHttpAutoSync::onFileLoaderResponse);
	// try to load, if not we can abort straight away.
	if(!fileLoader.getUrl(fileUrl))
	{
		if(verbose) printf("Could not connect to %s\n",fileUrl.c_str());
		// dispatch event with success as false
		events.donwloadedEventArgs.sucess = false;
		events.donwloadedEventArgs.path = fileList[0]->path;
		ofNotifyEvent(events.donwloadedEvent, events.donwloadedEventArgs);
		// increase the error list
		events.endDonwloadEventArgs.errors++;

		// remove the file from the donwlaod list and move on.
		filesToDonwload.erase(filesToDonwload.begin());
		if(filesToDonwload.size()>0) recursiveDowloadFiles(filesToDonwload);
		else
		{
			// All donwlods done, dipatch the event
			ofNotifyEvent(events.endDonwloadEvent, events.endDonwloadEventArgs);
		}
	}
	
}
///////////////////////////////////////////////////////////////////////////////////
// onFileLoaderResponse -----------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::onFileLoaderResponse(ofxHttpResponse & response){
	
	if(verbose) printf("ofxHttpAutoSync - File Loader response: %d\n", response.status);
	
	string path = filesToDonwload[0]->path;
	int lastModified = filesToDonwload[0]->lastModified;
	
	if (response.status == 200){
						
		rootDataPath();

		bool fileWriteAllowed = true;
		// first check if the file already exists
		if(ofxFileHelper::doesFileExist(path, true))
		{
			// then try to delete the file
			if(!ofxFileHelper::deleteFile(path , true))
			{
				// if we couldn't delete, it means the file was busy, so we just rename it 
				string newname = OLD_FILE_PREFIX+ofxFileHelper::getFilename(path);
				if(ofxFileHelper::renameTo(path, newname, true))
				{
					if(verbose) printf("%s was renamed to %s\n", path.c_str(), newname.c_str());
				}
				else
				{
					// If we could not even rename the file, than there is no choice but skip it
					fileWriteAllowed = false;
					// and increment the error list and flag the success as false
					events.donwloadedEventArgs.sucess = false;
					events.endDonwloadEventArgs.errors++;	
				}
			}
			
		}
	
		if(fileWriteAllowed)
		{
			// then we create fresh one
			string filePath = ofToDataPath(path, false);
			FILE * file = fopen( filePath.c_str(), "wb");
			fwrite (response.responseBody.c_str() , 1 , response.responseBody.length() , file );
			fclose( file);
			
			// set the timestamp of the new file to be exactly like the one form the server
			time_t epochTime = static_cast<time_t>((double)lastModified);
			Timestamp timestamp;
			ofxFileHelper::setLastModifiedDate(path, timestamp.fromEpochTime(epochTime));
			
			// now we can flag teh event success as true
			events.donwloadedEventArgs.sucess = true;
		}

		restoreDataPath();

		// finally dispatch the event
		events.donwloadedEventArgs.path = path;
		ofNotifyEvent(events.donwloadedEvent, events.donwloadedEventArgs);

		filesToDonwload.erase(filesToDonwload.begin());
		if(filesToDonwload.size()>0) recursiveDowloadFiles(filesToDonwload);
		else
		{
			// All donwlods done, dipatch the event
			ofNotifyEvent(events.endDonwloadEvent, events.endDonwloadEventArgs);
		}
	}
	else
	{
		// dispatch event
		events.donwloadedEventArgs.sucess = false;
		events.donwloadedEventArgs.path = path;
		ofNotifyEvent(events.donwloadedEvent, events.donwloadedEventArgs);
		// increase the error list
		events.endDonwloadEventArgs.errors++;	
	}

	
}
///////////////////////////////////////////////////////////////////////////////////
// addListeners -------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::addListeners(){
	ofAddListener(events.initEvent, this, &ofxHttpAutoSync::onInit);
	ofAddListener(events.abortEvent, this, &ofxHttpAutoSync::onAbort);
	ofAddListener(events.completeEvent, this, &ofxHttpAutoSync::onComplete);
	
	ofAddListener(events.startDeleteEvent, this, &ofxHttpAutoSync::onStartDelete);
	ofAddListener(events.startDirCreationEvent, this, &ofxHttpAutoSync::onStartDirCreation);
	ofAddListener(events.startDonwloadEvent, this, &ofxHttpAutoSync::onStartDownload);
	
	ofAddListener(events.endDeleteEvent, this, &ofxHttpAutoSync::onEndDelete);
	ofAddListener(events.endDirCreationEvent, this, &ofxHttpAutoSync::onEndDirCreation);
	ofAddListener(events.endDonwloadEvent, this, &ofxHttpAutoSync::onEndDownload);
	
	ofAddListener(events.deletedEvent, this, &ofxHttpAutoSync::onDelete);
	ofAddListener(events.dirCreatedEvent, this, &ofxHttpAutoSync::onDirCreated);
	ofAddListener(events.donwloadedEvent, this, &ofxHttpAutoSync::onDownloaded);
}
///////////////////////////////////////////////////////////////////////////////////
// removeListeners ----------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::removeListeners(){
	ofRemoveListener(events.initEvent, this, &ofxHttpAutoSync::onInit);
	ofRemoveListener(events.abortEvent, this, &ofxHttpAutoSync::onAbort);
	ofRemoveListener(events.completeEvent, this, &ofxHttpAutoSync::onComplete);
	
	ofRemoveListener(events.startDeleteEvent, this, &ofxHttpAutoSync::onStartDelete);
	ofRemoveListener(events.startDirCreationEvent, this, &ofxHttpAutoSync::onStartDirCreation);
	ofRemoveListener(events.startDonwloadEvent, this, &ofxHttpAutoSync::onStartDownload);
	
	ofRemoveListener(events.endDeleteEvent, this, &ofxHttpAutoSync::onEndDelete);
	ofRemoveListener(events.endDirCreationEvent, this, &ofxHttpAutoSync::onEndDirCreation);
	ofRemoveListener(events.endDonwloadEvent, this, &ofxHttpAutoSync::onEndDownload);
	
	ofRemoveListener(events.deletedEvent, this, &ofxHttpAutoSync::onDelete);
	ofRemoveListener(events.dirCreatedEvent, this, &ofxHttpAutoSync::onDirCreated);
	ofRemoveListener(events.donwloadedEvent, this, &ofxHttpAutoSync::onDownloaded);
}
///////////////////////////////////////////////////////////////////////////////////
// onInit -------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::onInit(ofxHttpAutoSyncInitEventArgs &args){
	if(verbose)
	{
		printf("\n--------------------------------------------\n");
		printf("ofxHttpAutoSync - INIT \n");
		printf("%i files to sync (Ignored files not included).\n", args.numFiles);
		if(args.numFiles>0) printf("\n");
		
		syncFiles.pushTag("FILES",0);
		for(int i = 0; i < syncFiles.getNumTags("FILE"); i++)
		{
			printf("%s ", syncFiles.getValue("FILE:SYNC_ACTION", "", i).c_str());
			
			if(syncFiles.getValue("FILE:SYNC_ACTION", "", i) == "I")
			{
				if(syncFiles.getValue("FILE:IS_IGNORE_LOCAL", 0, i)) printf("LOCAL  ");
				else printf("REMOTE ");
			}
			printf("%s\n", syncFiles.getValue("FILE:PATH", "", i).c_str());
		}
		syncFiles.popTag();
		printf("--------------------------------------------\n");
	}
}
///////////////////////////////////////////////////////////////////////////////////
// onAbort ------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::onAbort(ofxHttpAutoSyncAbortEventArgs &args){
	removeListeners();
	if(verbose)
	{
		printf("\n--------------------------------------------\n");
		printf("ofxHttpAutoSync - ABORT \n");
		printf("Abort message: %s\n", args.message.c_str());
		printf("--------------------------------------------\n");
	}
}
///////////////////////////////////////////////////////////////////////////////////
// onComplete ---------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::onComplete(ofxHttpAutoSyncCompleteEventArgs &args){
	removeListeners();
	if(verbose)
	{
		printf("\n--------------------------------------------\n");
		printf("ofxHttpAutoSync - COMPLETE \n");
		printf("%i errors.\n", args.errors);
		printf("--------------------------------------------\n");
	}
}

///////////////////////////////////////////////////////////////////////////////////
// onStartDelete -------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::onStartDelete(ofxHttpAutoSyncStartEventArgs &args){
	if(verbose)
	{
		printf("\n--------------------------------------------\n");
		printf("ofxHttpAutoSync - START DELETE\n");
		printf("%i files to delete.\n", args.numFiles);
		if(args.numFiles>0) printf("\n");
	}
}
///////////////////////////////////////////////////////////////////////////////////
// onStartDirCreation --------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::onStartDirCreation(ofxHttpAutoSyncStartEventArgs &args){
	if(verbose)
	{
		printf("\n--------------------------------------------\n");
		printf("ofxHttpAutoSync - START DIRECTORIES CREATION\n");
		printf("%i directories to create.\n", args.numFiles);
		if(args.numFiles>0) printf("\n");
	}
}
///////////////////////////////////////////////////////////////////////////////////
// onStartDownload -----------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::onStartDownload(ofxHttpAutoSyncStartEventArgs &args){
	if(verbose)
	{
		printf("\n--------------------------------------------\n");
		printf("ofxHttpAutoSync - START DONWLOAD\n");
		printf("%i files to donwload.\n", args.numFiles);
		if(args.numFiles>0) printf("\n");
	}
}
///////////////////////////////////////////////////////////////////////////////////
// onEndDelete ---------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::onEndDelete(ofxHttpAutoSyncEndEventArgs &args){
	if(verbose)
	{
		printf("\n");
		printf("ofxHttpAutoSync - END DELETE\n");
		printf("%i errors.\n", args.errors);
		printf("--------------------------------------------\n");
	}
}
///////////////////////////////////////////////////////////////////////////////////
// onEndDirCreation ----------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::onEndDirCreation(ofxHttpAutoSyncEndEventArgs &args){
	if(verbose)
	{
		printf("\n");
		printf("ofxHttpAutoSync - END DIRECTORIES CREATION\n");
		printf("%i errors.\n", args.errors);
		printf("--------------------------------------------\n");
	}
}
///////////////////////////////////////////////////////////////////////////////////
// onEndDownload -------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::onEndDownload(ofxHttpAutoSyncEndEventArgs &args){
	if(verbose)
	{
		printf("\n");
		printf("ofxHttpAutoSync - END DONWLOAD\n");
		printf("%i errors.\n", args.errors);
		printf("--------------------------------------------\n");
	}
	// As donwloading is the last operation we perform, if we got here we can go ahead and dispatch the final complete event
	events.completeEventArgs.errors = events.endDeleteEventArgs.errors + events.endDirCreationEventArgs.errors + events.endDonwloadEventArgs.errors;
	ofNotifyEvent(events.completeEvent, events.completeEventArgs);
	
}
///////////////////////////////////////////////////////////////////////////////////
// onDelete -----------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::onDelete(ofxHttpAutoSyncUpdateEventArgs &args){
	if(verbose)
	{
		if(args.sucess) printf("DELETED: %s\n\n", args.path.c_str());
		else printf("ERROR DELETING: %s\n\n", args.path.c_str());
	}
}
///////////////////////////////////////////////////////////////////////////////////
// onDirCreated -------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::onDirCreated(ofxHttpAutoSyncUpdateEventArgs &args){
	if(verbose) printf("CREATED: %s\n", args.path.c_str());
}
///////////////////////////////////////////////////////////////////////////////////
// onDownloaded -------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::onDownloaded(ofxHttpAutoSyncUpdateEventArgs &args){
	if(verbose)
	{
		if(args.sucess) printf("DOWNLOADED: %s\n\n", args.path.c_str());
		else printf("ERROR DONWLODING: %s\n\n", args.path.c_str());
	}
}
///////////////////////////////////////////////////////////////////////////////////
// rootDataPath -------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::rootDataPath(){
	if (!isDataPathRooted)
	{
	currentDataPathRoot = ofToDataPath("", true);
	#if defined TARGET_OSX
		ofSetDataPathRoot("../../../");
	#else
		ofSetDataPathRoot("");
	#endif
		isDataPathRooted = true;
		rootedDataPathRoot = ofToDataPath("", true);
	}
}
///////////////////////////////////////////////////////////////////////////////////
// restoreDataPath ----------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
void ofxHttpAutoSync::restoreDataPath(){
	if (isDataPathRooted)
	{
		ofSetDataPathRoot(currentDataPathRoot);
		isDataPathRooted = false;
	}
}
///////////////////////////////////////////////////////////////////////////////////
// hasAnomalies ----------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////
bool ofxHttpAutoSync::hasAnomalies(ofxXmlSettings xml){
	if(!xml.pushTag("FILES",0)) return true;
	else
	{
		int nFiles = xml.getNumTags("FILE");
		if(nFiles > 0)
		{
			for (int i = 0; i < nFiles; i++)
			{
				if(!xml.pushTag("FILE",0)) return true;
				else
				{
					if(!xml.pushTag("PATH",0)) return true;
					else xml.popTag();
					if(!xml.pushTag("IS_DIRECTORY",0)) return true;
					else xml.popTag();
					if(!xml.pushTag("LAST_MODIFIED",0)) return true;
					else xml.popTag();
					
					xml.popTag();
				}
			}		
		}
		xml.popTag();
	}
	return false;
}
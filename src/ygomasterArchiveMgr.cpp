#include "ygomasterArchiveMgr.h"
#include <cjson/cJSON.h>
#include <fstream>

namespace fs = std::filesystem;

YgoMasterArchiveMgr::YgoMasterArchiveMgr()
{
	m_YMDataPath = "";
	m_configPath = fs::current_path().string() + "\\config.json";
	m_YMListPath = fs::current_path().string() + "\\ArchiveList.json";
	m_archivesPath = fs::current_path().string() + "\\Archives";
	m_currentArchiveIndex = 0;
}

void YgoMasterArchiveMgr::Run()
{
	if (!ReadConfig()) {
		printf("Read config failed.\n");
		return;
	}
	printf("Read config done: %s\n", m_YMListPath.c_str());

	if (!ReadYMList()) {
		printf("Read ArchiveList failed.\n");
		return;
	}
	printf("Read ArchiveList done.\n");

	int input = 0;
	while (true)
	{
		printf("\n*========== YgoMaster Archive Manager ==========*\n");
		for (const auto& option : sc_InputOptions) {
			printf("%d. %s\n", static_cast<int>(option.first), option.second.c_str());
		}
		printf("================================================*\n");
		printf("Enter your choice: ");

		std::cin >> input;
		if (std::cin.fail()) {
			std::cin.clear(); // Clear the error flag
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Discard invalid input
			printf("Invalid input, please enter a number.\n");
			continue;
		}

		switch (input)
		{
		case static_cast<int>(EInputOption::EXIT):
			printf("Exiting...\n");
			return;
		case static_cast<int>(EInputOption::DISPLAY_ARCHIVE_LIST):
			QuerryArchiveList(DEFAULT_MAX_ARCHIVE_LIST_SIZE, true, false);
			break;
		case static_cast<int>(EInputOption::DISPLAY_ARCHIVE_DETAIL):
		{
			int archiveID;
			printf("Enter ArchiveID to display detail (-1 for current archive): ");
			std::cin >> archiveID;
			if (std::cin.fail()) {
				std::cin.clear(); // Clear the error flag
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Discard invalid input
				printf("Invalid input, please enter a number.\n");
				continue;
			}
			DisplayArchiveDetail(archiveID);
			break;
		}
		case static_cast<int>(EInputOption::BACKUP_ARCHIVE_COPY):
			if (BackupAndCreateNewArchive()) {
				printf("New archive created successfully.\n");
			}
			else {
				printf("Create new archive failed.\n");
			}
			break;
		case static_cast<int>(EInputOption::BACKUP_ARCHIVE_REPLACE):
			if (BackupArchive(m_currentArchiveIndex, false)) {
				printf("Current archive updated successfully.\n");
			}
			else {
				printf("Update current archive failed.\n");
			}
			break;
		case static_cast<int>(EInputOption::DELETE_ARCHIVE):
		{
			int archiveID;
			printf("Enter ArchiveID to delete: ");
			std::cin >> archiveID;
			if (std::cin.fail()) {
				std::cin.clear(); // Clear the error flag
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Discard invalid input
				printf("Invalid input, please enter a number.\n");
				continue;
			}
			if (DeleteArchive(archiveID)) {
				printf("ArchiveID %d deleted successfully.\n", archiveID);
			}
			else {
				printf("Delete ArchiveID %d failed.\n", archiveID);
			}
			break;
		}
		case static_cast<int>(EInputOption::RESTORE_ARCHIVE):
		{
			int archiveID;
			printf("Enter ArchiveID to restore: ");
			std::cin >> archiveID;
			if (std::cin.fail()) {
				std::cin.clear(); // Clear the error flag
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Discard invalid input
				printf("Invalid input, please enter a number.\n");
				continue;
			}
			if (RestoreArchive(archiveID, false)) {
				printf("ArchiveID %d restored successfully.\n", archiveID);
			}
			else {
				printf("Restore ArchiveID %d failed.\n", archiveID);
			}
			break;
		}
		case static_cast<int>(EInputOption::RESTORE_ARCHIVE_WITH_BACKUP):
		{
			int archiveID;
			printf("Enter ArchiveID to restore with backup: ");
			std::cin >> archiveID;
			if (std::cin.fail()) {
				std::cin.clear(); // Clear the error flag
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Discard invalid input
				printf("Invalid input, please enter a number.\n");
				continue;
			}
			if (RestoreArchive(archiveID, true)) {
				printf("ArchiveID %d restored successfully with backup.\n", archiveID);
			}
			else {
				printf("Restore ArchiveID %d with backup failed.\n", archiveID);
			}
			break;
		}
		default:
			break;
		}
	}
	return;
}

bool YgoMasterArchiveMgr::ReadConfig()
{
	/*
	* Read the config file, if not exist, create a default one.
	* If read success, return true and set result to YMListPath.
	*/

	//If config file not exist, create a default one and return
	if (!fs::exists(m_configPath)) {
		printf("Config file not exist, creating a default one.\n");
		cJSON* root = cJSON_CreateObject();
		if (!root) {
			printf("Create JSON object failed.\n");
			return false;
		}

		if (!CheckYMDataDir()) {
			cJSON_Delete(root);
			printf("YgoMaster Data directory not found in default search paths.\n");
			return false;
		}

		cJSON_AddStringToObject(root, "Desc", sc_configDescText.c_str());
		cJSON_AddStringToObject(root, "YMListPath", m_YMListPath.c_str());
		cJSON_AddStringToObject(root, "YMDataPath", m_YMDataPath.c_str());
		cJSON_AddStringToObject(root, "ArchivesPath", m_archivesPath.c_str());
		char* jsonFileString = cJSON_Print(root);

		std::ofstream outFile(m_configPath);
		if (!outFile) {
			printf("Create config file failed at %s\n", m_configPath.c_str());
			cJSON_free(jsonFileString);
			cJSON_Delete(root);
			return false;
		}
		outFile << jsonFileString;
		if (!outFile) {
			printf("Write to config file failed at %s\n", m_configPath.c_str());
			outFile.close();
			cJSON_free(jsonFileString);
			cJSON_Delete(root);
			return false;
		}
		outFile.close();
		cJSON_free(jsonFileString);
		cJSON_Delete(root);
		printf("Default config file created at %s\n", m_configPath.c_str());
		return true;
	}

	//Read config file
	printf("Reading config file at %s\n", m_configPath.c_str());
	std::ifstream file(m_configPath, std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		printf("Open config file failed.\n");
		return false;
	}
	std::streamsize size = file.tellg();
	if (size <= 0) {
		printf("Config file is empty.\n");
		return false;
	}

	file.seekg(0, std::ios::beg);
	std::vector<char> buffer(size + 1);
	if (!file.read(buffer.data(), size)) {
		printf("Read config file failed.\n");
		return false;
	}
	buffer[size] = '\0';
	file.close();

	//Parse JSON
	printf("Parsing config file.\n");
	cJSON* root = cJSON_Parse(buffer.data());
	if (!root) {
		printf("Parse config file failed.\n");
		return false;
	}
	cJSON* ymListPath = cJSON_GetObjectItem(root, "YMListPath");
	cJSON* ymDataPath = cJSON_GetObjectItem(root, "YMDataPath");
	cJSON* archivesPath = cJSON_GetObjectItem(root, "ArchivesPath");
	if (cJSON_IsString(ymListPath) && (ymListPath->valuestring != nullptr)
		&& cJSON_IsString(ymDataPath) && (ymDataPath->valuestring != nullptr)
		&& cJSON_IsString(archivesPath) && (archivesPath->valuestring != nullptr)
		) {
		m_YMListPath = ymListPath->valuestring;
		m_YMDataPath = ymDataPath->valuestring;
		m_archivesPath = archivesPath->valuestring;
		cJSON_Delete(root);
		return true;
	}
	cJSON_Delete(root);

	return false;
}

bool YgoMasterArchiveMgr::ReadYMList()
{
	/*
	* Read the YgoMaster save list, if not exist, create a default one.
	* Return false if read failed.
	*/

	if (!fs::exists(m_YMListPath)) {
		printf("ArchiveList file not exist, creating a default one.\n");
		cJSON* root = cJSON_CreateObject();
		cJSON_AddItemToObject(root, "Currently in use ArchiveID", cJSON_CreateNumber(m_currentArchiveIndex));
		cJSON_AddItemToObject(root, "ArchivesCount", cJSON_CreateNumber(0));
		cJSON_AddItemToObject(root, "Archives", cJSON_CreateArray());
		char* jsonFileString = cJSON_Print(root);

		std::ofstream outFile(m_YMListPath);
		if (!outFile) {
			printf("Create ArchiveList file failed at %s\n", m_YMListPath.c_str());
			cJSON_free(jsonFileString);
			cJSON_Delete(root);
			return false;
		}
		outFile << jsonFileString;
		if (!outFile) {
			printf("Write to ArchiveList file failed at %s\n", m_YMListPath.c_str());
			outFile.close();
			cJSON_free(jsonFileString);
			cJSON_Delete(root);
			return false;
		}
		outFile.close();
		cJSON_free(jsonFileString);
		cJSON_Delete(root);

		//Create the first archive
		if (!BackupArchive(0)) {
			printf("Create first archive failed.\n");
			return false;
		}
	}

	//Read ArchiveList file
	printf("Reading ArchiveList file at %s\n", m_YMListPath.c_str());
	if (!QuerryArchiveList(DEFAULT_MAX_ARCHIVE_LIST_SIZE, true, true))
	{
		printf("Read ArchiveList file failed.\n");
		return false;
	}

	return true;
}

bool YgoMasterArchiveMgr::CheckYMDataDir()
{
	for (const auto& dir : sc_YgoArchiveSearchPaths) {
		fs::path potentialPath = fs::current_path().string() + fs::path(dir).string();
		if (fs::exists(potentialPath) && fs::is_directory(potentialPath)) {
			m_YMDataPath = potentialPath.string();
			printf("YgoMaster Data directory found at %s\n", m_YMDataPath.c_str());
			return true;
		}
	}
	printf("YgoMaster Data directory not found in default search paths.\n");

	//Ask user to input the path
	std::string inputPath;
	printf("Please input the YgoMaster Data directory path:\n");
	while (true) {
		std::cin >> inputPath;
		fs::path userPath(inputPath);
		if (fs::exists(userPath) && fs::is_directory(userPath)) {
			m_YMDataPath = userPath.string();
			printf("YgoMaster Data directory set to %s\n", m_YMDataPath.c_str());
			return true;
		}
		else {
			printf("Path %s is not a valid directory, please input again:\n", inputPath.c_str());
		}
	}

	return false;
}

bool YgoMasterArchiveMgr::QuerryArchiveList(const int maxSize, const bool display, const bool updateArchives)
{
	//Display the archive list, if updateArchives is true, update m_archives
	std::ifstream inFile(m_YMListPath);
	if (!inFile.is_open()) {
		printf("Open ArchiveList file failed.\n");
		return false;
	}
	std::string jsonContent((std::istreambuf_iterator<char>(inFile)), 
		std::istreambuf_iterator<char>());
	inFile.close();
	if (jsonContent.empty()) {
		printf("ArchiveList file is empty.\n");
		return false;
	}

	cJSON* root = cJSON_Parse(jsonContent.c_str());
	if (!root) {
		printf("Parse ArchiveList file failed.\n");
		return false;
	}

	cJSON* archivesArray = cJSON_GetObjectItem(root, "Archives");
	if (!archivesArray || !cJSON_IsArray(archivesArray)) {
		printf("ArchiveList file format error: Archives is not an array.\n");
		cJSON_Delete(root);
		return false;
	}
	int size = cJSON_GetArraySize(archivesArray);
	int displaySize = (maxSize == -1 || size < maxSize) ? size : maxSize;
	printf("*------------------ Archive List ------------------*\n");
	printf("There are total %d archives, displaying %d archives:\n", size, displaySize);
	for (int i = 0; i < size; ++i) {
		cJSON* archiveItem = cJSON_GetArrayItem(archivesArray, i);
		if (!archiveItem) continue;
		YgoArchiveInfo info;
		cJSON* idItem = cJSON_GetObjectItem(archiveItem, "id");
		cJSON* nameItem = cJSON_GetObjectItem(archiveItem, "Name");
		cJSON* pathItem = cJSON_GetObjectItem(archiveItem, "Path");
		cJSON* timeItem = cJSON_GetObjectItem(archiveItem, "LastBackupTime");
		cJSON* descItem = cJSON_GetObjectItem(archiveItem, "Description");
		if (idItem && cJSON_IsNumber(idItem)) {
			info.m_id = static_cast<int>(cJSON_GetNumberValue(idItem));
		}
		if (nameItem && cJSON_IsString(nameItem)) {
			info.m_name = cJSON_GetStringValue(nameItem);
		}
		if (pathItem && cJSON_IsString(pathItem)) {
			info.m_path = cJSON_GetStringValue(pathItem);
		}
		if (timeItem && cJSON_IsString(timeItem)) {
			info.m_time = cJSON_GetStringValue(timeItem);
		}
		if (descItem && cJSON_IsString(descItem)) {
			info.m_desc = cJSON_GetStringValue(descItem);
		}
		if (updateArchives) {
			m_archives[info.m_id] = info;
			if (info.m_id == m_currentArchiveIndex) {
				m_currentArchiveIndex = i;
			}
		}
		if (display && i < displaySize) {
			printf("\tArchiveID: %d,\n \tName: %s,\n \tLast update time: %s\n \tDescription: %s\n",
				info.m_id,
				info.m_name.c_str(),
				info.m_time.c_str(),
				info.m_desc.c_str());
			printf("--------------------------------------------------\n");
		}
	}
	if (size > displaySize) {
		printf("...\n");
	}
	printf("*--------------------------------------------------*\n");
	cJSON_Delete(root);
	return true;
}

void YgoMasterArchiveMgr::DisplayArchiveDetail(const int archiveID)
{
	//Display detailed info for a specific archiveID, if archiveID is -1 display current archive
	if (m_archives.empty()) {
		printf("No archives available to display.\n");
		return;
	}

	const int displayID = (-1 == archiveID) ? m_archives[m_currentArchiveIndex].m_id : archiveID;
	const auto& archiveIt = m_archives.find(displayID);
	if (archiveIt == m_archives.end()) {
		printf("ArchiveID %d not found.\n", displayID);
		return;
	}

	const YgoArchiveInfo& archive = archiveIt->second;
	const std::string playerJsonPath = archive.m_path + sc_YgoPlayerJsonSearchPath;
	const std::string settingsJsonPath = archive.m_path + sc_YgoSettingsJsonSearchPath;
	if (!fs::exists(playerJsonPath) || !fs::exists(settingsJsonPath)) {
		printf("Warning: Some important files are missing in this archive.\n");
		printf("Missing files:\n");
		if (!fs::exists(playerJsonPath)) {
			printf("\t%s\n", playerJsonPath.c_str());
		}
		if (!fs::exists(settingsJsonPath)) {
			printf("\t%s\n", settingsJsonPath.c_str());
		}
		printf("This archive was not backed up properly or damaged !\n");
	}

	bool readPlayerSuccess = false;
	int code = 0;
	int gems = 0;
	//Read Player.json
	std::ifstream playerFile(playerJsonPath, std::ios::binary);
	if (playerFile.is_open()) {
		std::string jsonContent((std::istreambuf_iterator<char>(playerFile)),
			std::istreambuf_iterator<char>());
		playerFile.close();
		cJSON* root = cJSON_Parse(jsonContent.c_str());
		if (root) {
			cJSON* codeItem = cJSON_GetObjectItem(root, "Code");
			if (codeItem && cJSON_IsNumber(codeItem)) {
				code = static_cast<int>(cJSON_GetNumberValue(codeItem));
			}
			cJSON* gemsItem = cJSON_GetObjectItem(root, "Gems");
			if (gemsItem && cJSON_IsNumber(gemsItem)) {
				gems = static_cast<int>(cJSON_GetNumberValue(gemsItem));
			}
			cJSON_Delete(root);
			readPlayerSuccess = true;
		}
	}

	bool unlockAllCards = false;
	int defaultsGems = 0;

	//Found the archive, display details
	printf("*------------------ Archive Detail ------------------*\n");
	printf("\tArchiveID: %d\n", archive.m_id);
	if (readPlayerSuccess) {
		printf("\tPlayer Code: %d\n", code);
		printf("\tPlayer Gems: %d\n", gems);
	}
	else {
		printf("\tPlayer Code: N/A (Failed to read Player.json)\n");
		printf("\tPlayer Gems: N/A (Failed to read Player.json)\n");
	}
	printf("\tPlayer Name: %s\n", archive.m_name.c_str());
	printf("\tArchive Path: %s\n", archive.m_path.c_str());
	printf("\tLast update time: %s\n", archive.m_time.c_str());
	printf("\tDescription: %s\n", archive.m_desc.c_str());
	printf("*----------------------------------------------------*\n");
	return;
}

bool YgoMasterArchiveMgr::GetNewYgoArchiveInfo(YgoArchiveInfo& result, const bool needDesc)
{
	//Backup targets files or directories, and fill the result structure
	if (!CopyTargetFiles(result)) {
		printf("Update target files failed.\n");
		return false;
	}

	//Read Player.json to get player name
	const std::string playerJsonPath = result.m_path + sc_YgoPlayerJsonSearchPath;
	std::ifstream file(playerJsonPath, std::ios::binary);
	if (!fs::exists(playerJsonPath)) {
		printf("Cannot find Player.json in the backup archive, cannot get player name.\n");
		return false;
	}
	std::string jsonContent((std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>());
	file.close();

	cJSON* root = cJSON_Parse(jsonContent.c_str());
	if (!root) {
		printf("Parse Player.json failed in the backup archive, cannot get player name.\n");
	}
	else {
		cJSON* playerNameItem = cJSON_GetObjectItem(root, "Name");
		if (playerNameItem && cJSON_IsString(playerNameItem)) {
			result.m_name = playerNameItem->valuestring;
		}
		else {
			printf("PlayerName not found or invalid in Player.json, cannot get player name.\n");
		}
	}
	cJSON_Delete(root);
	printf("Backup newest archive done. \nEnter the description information of the archive and press Enter (default is empty): \n");
	
	std::string desc("");
	if (needDesc) {
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear the input buffer
		std::getline(std::cin, desc);
	}
	result.m_desc = desc;
	return true;
}

bool YgoMasterArchiveMgr::CopyTargetFiles(YgoArchiveInfo& result)
{
	const auto timestamp = std::chrono::system_clock::to_time_t(
		std::chrono::system_clock::now());
	char timeBuffer[20];

#if defined(_WIN32)
	struct tm timeInfo;
	localtime_s(&timeInfo, &timestamp);
	std::strftime(timeBuffer, sizeof(timeBuffer), "%Y_%m_%d_%H%M%S", &timeInfo);
#else
	struct tm timeInfo;
	localtime_r(&timestamp, &timeInfo);
	std::strftime(timeBuffer, sizeof(timeBuffer), "%Y_%m_%d_%H%M%S", &timeInfo);
#endif

	result.m_time = std::string(timeBuffer);
	//Create archive directory if not exist
	if (result.m_path.empty()) {
		result.m_path = m_archivesPath + "\\" + result.m_time;
	}

	if (!fs::exists(result.m_path)) {
		if (!fs::create_directories(result.m_path)) {
			printf("Create archive directory %s failed.\n", result.m_path.c_str());
			return false;
		}
	}

	//Copy target files or directories
	for (const auto& target : sc_BackupTargets) {
		fs::path sourcePath = fs::path(m_YMDataPath) / fs::path(target.second);
		fs::path destPath = fs::path(result.m_path) / fs::path(target.second);
		if (!fs::exists(sourcePath)) {
			printf("Source path %s not exist, skipping.\n", sourcePath.string().c_str());
			continue;
		}
		try {
			if (target.first == IS_FILE) {
				fs::copy_file(sourcePath, destPath, fs::copy_options::overwrite_existing);
			}
			else if (target.first == IS_DIRECTORY) {
				fs::copy(sourcePath, destPath, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
			}
			else {
				printf("Unknown target type for %s, skipping.\n", sourcePath.string().c_str());
			}
		}
		catch (const fs::filesystem_error& e) {
			printf("Error copying %s to %s: %s\n",
				sourcePath.string().c_str(),
				destPath.string().c_str(),
				e.what());
			return false;
		}
	}

	return true;
}

bool YgoMasterArchiveMgr::ResetData(const int archiveID, const YMArchiveData& YMdataID)
{
	const int dataID = static_cast<int>(YMdataID);
	if (dataID < static_cast<int>(YMArchiveData::MIN_DATA)
		|| dataID >= static_cast<int>(YMArchiveData::MAX_DATA))
	{
		printf("Invalid YMArchiveData ID: %d\n", dataID);
		return false;
	}

	auto it = m_archives.find(archiveID);
	if (it == m_archives.end()) {
		printf("ArchiveID %d not found.\n", archiveID);
		return false;
	}
	const YgoArchiveInfo& archive = it->second;

	switch (dataID)
	{
	case static_cast<int>(YMArchiveData::CONFIG_DATA_DESC):
	{
		printf("Resetting description for ArchiveID %d...\n", archiveID);
		std::string newDesc;
		printf("Enter new description: \n");
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear the input buffer
		std::getline(std::cin, newDesc);
		//Update ArchiveList file
		std::fstream file(m_YMListPath,
			std::ios::in | std::ios::out | std::ios::binary);
		if (!file.is_open()) {
			printf("Open ArchiveList file failed for reset.\n");
			return false;
		}
		std::string jsonContent((std::istreambuf_iterator<char>(file)),
			std::istreambuf_iterator<char>());
		cJSON* root = cJSON_Parse(jsonContent.c_str());
		if (!root) {
			printf("Parse ArchiveList file failed for reset.\n");
			file.close();
			return false;
		}
		cJSON* archivesArray = cJSON_GetObjectItem(root, "Archives");
		if (!archivesArray || !cJSON_IsArray(archivesArray)) {
			printf("ArchiveList file format error: Archives is not an array.\n");
			cJSON_Delete(root);
			file.close();
			return false;
		}
		int size = cJSON_GetArraySize(archivesArray);
		for (int i = 0; i < size; ++i) {
			cJSON* archiveItem = cJSON_GetArrayItem(archivesArray, i);
			if (!archiveItem) continue;
			cJSON* idItem = cJSON_GetObjectItem(archiveItem, "id");
			if (idItem && cJSON_IsNumber(idItem) && idItem->valueint == archiveID) {
				cJSON* descItem = cJSON_GetObjectItem(archiveItem, "Description");
				if (descItem && cJSON_IsString(descItem)) {
					cJSON_SetValuestring(descItem, newDesc.c_str());
					//Write back to file
					std::string jsonFileString = std::string(cJSON_Print(root));
					file.seekp(0, std::ios::beg);
					file.write(jsonFileString.c_str(), jsonFileString.size());
					cJSON_Delete(root);
					file.close();
					printf("Description for ArchiveID");
				}
			}
		}
		printf(" %d reset successfully.\n", archiveID);
		return true;
	}
	case static_cast<int>(YMArchiveData::ARCHIVE_DATA_GEMS):
	{
		printf("Resetting player gems for ArchiveID %d...\n", archiveID);
		int newGems;
		printf("Enter new gems amount: ");
		std::cin >> newGems;
		if (std::cin.fail() || newGems < 0) {
			std::cin.clear(); // Clear the error flag
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Discard invalid input
			printf("Invalid input, please enter a non-negative number.\n");
			return false;
		}
		//Update Player.json
		const std::string playerJsonPath = archive.m_path + sc_YgoPlayerJsonSearchPath;
		std::fstream file(playerJsonPath,
			std::ios::in | std::ios::out | std::ios::binary);
		if (!file.is_open()) {
			printf("Open Player.json failed for reset.\n");
			return false;
		}
		std::string jsonContent((std::istreambuf_iterator<char>(file)),
			std::istreambuf_iterator<char>());
		cJSON* root = cJSON_Parse(jsonContent.c_str());
		if (!root) {
			printf("Parse Player.json failed for reset.\n");
			file.close();
			return false;
		}
		cJSON* gemsItem = cJSON_GetObjectItem(root, "Gems");
		if (gemsItem && cJSON_IsNumber(gemsItem)) {
			cJSON_SetNumberValue(gemsItem, newGems);
			//Write back to file
			std::string jsonFileString = std::string(cJSON_Print(root));
			file.seekp(0, std::ios::beg);
			file.write(jsonFileString.c_str(), jsonFileString.size());
			cJSON_Delete(root);
			file.close();
			printf("Player gems for ArchiveID %d reset successfully.\n", archiveID);
			return true;
		}
		cJSON_Delete(root);
		file.close();
		printf("Gems item not found in Player.json, cannot reset.\n");
		return false;
	}
	default:
		break;
	}
	return false;
}


bool YgoMasterArchiveMgr::BackupArchive(const int targetID, const bool copy)
{
	/*
	* Backup the latest archive to ArchiveList file for targetID.
	* if copy is true, create a new archive entry.
	*/
	if (copy) {
		printf("Creating a new archive entry in ArchiveList file...\n");
	} else{
		printf("Updating ArchiveList file for ArchiveID %d...\n", targetID);
	}

	std::fstream file(m_YMListPath,
		std::ios::in | std::ios::out | std::ios::binary);
	if (!file.is_open()) {
		printf("Open ArchiveList file failed for backup.\n");
		return false;
	}
	std::string jsonContent((std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>());
	cJSON* root = cJSON_Parse(jsonContent.c_str());
	if (!root) {
		printf("Parse ArchiveList file failed for backup.\n");
		file.close();
		return false;
	}

	cJSON* currentID = cJSON_GetObjectItem(root, "Currently in use ArchiveID");
	cJSON* archivesArray = cJSON_GetObjectItem(root, "Archives");
	if (!currentID || !cJSON_IsNumber(currentID)
		|| !archivesArray || !cJSON_IsArray(archivesArray)) {
		printf("ArchiveList file format error: Archives is not an array or Currently in use ArchiveID is not a number..\n");
		cJSON_Delete(root);
		file.close();
		return false;
	}

	int size = cJSON_GetArraySize(archivesArray);
	bool found = false;

	if (size != 0 && !copy) {
		//Search for targetID in the archives array
		for (int i = 0; i < size; ++i) {
			cJSON* archiveItem = cJSON_GetArrayItem(archivesArray, i);
			if (!archiveItem) continue;
			cJSON* idItem = cJSON_GetObjectItem(archiveItem, "id");
			if (idItem && cJSON_IsNumber(idItem) && idItem->valueint == targetID) {
				//Found targetID, update Currently in use ArchiveID
				cJSON_SetNumberValue(currentID, targetID);
				found = true;

				//Get new archive info
				YgoArchiveInfo newInfo;
				cJSON* pathItem = cJSON_GetObjectItem(archiveItem, "Path");
				if (pathItem && cJSON_IsString(pathItem)) {
					newInfo.m_path = pathItem->valuestring;
				}
				if (GetNewYgoArchiveInfo(newInfo)) {
					//Update archive info
					cJSON* nameItem = cJSON_GetObjectItem(archiveItem, "Name");
					cJSON* pathItem = cJSON_GetObjectItem(archiveItem, "Path");
					cJSON* descItem = cJSON_GetObjectItem(archiveItem, "Description");
					cJSON* timeItem = cJSON_GetObjectItem(archiveItem, "LastBackupTime");
					if (nameItem && cJSON_IsString(nameItem)) {
						cJSON_SetValuestring(nameItem, newInfo.m_name.c_str());
					}
					if (pathItem && cJSON_IsString(pathItem)) {
						cJSON_SetValuestring(pathItem, newInfo.m_path.c_str());
					}
					if (descItem && cJSON_IsString(descItem)) {
						cJSON_SetValuestring(descItem, newInfo.m_desc.c_str());
					}
					if (timeItem && cJSON_IsString(timeItem)) {
						cJSON_SetValuestring(timeItem, newInfo.m_time.c_str());
					}
					break;
				}
				else {
					printf("Get new YgoArchiveInfo failed, cannot update archive info.\n");
					cJSON_Delete(root);
					file.close();
					return false;
				}
			}
		}
	} 
	
	//Create a new archive entry
	if (!found || 0 == size || copy) {
		// Get new ID
		int currentMaxID = -1;
		for (int i = 0; i < size; ++i) {
			cJSON* archiveItem = cJSON_GetArrayItem(archivesArray, i);
			if (!archiveItem) continue;
			cJSON* idItem = cJSON_GetObjectItem(archiveItem, "id");
			if (idItem && cJSON_IsNumber(idItem) && idItem->valueint > currentMaxID) {
				currentMaxID = idItem->valueint;
			}
		}

		// Get new archive info
		YgoArchiveInfo newInfo;
		if (GetNewYgoArchiveInfo(newInfo, (0!=size))) {
			cJSON* newArchive = cJSON_CreateObject();
			cJSON_AddItemToObject(newArchive, "id", cJSON_CreateNumber(currentMaxID + 1));
			cJSON_AddItemToObject(newArchive, "Name", cJSON_CreateString(newInfo.m_name.c_str()));
			cJSON_AddItemToObject(newArchive, "Path", cJSON_CreateString(newInfo.m_path.c_str()));
			cJSON_AddItemToObject(newArchive, "Description", cJSON_CreateString(newInfo.m_desc.c_str()));
			cJSON_AddItemToObject(newArchive, "LastBackupTime", cJSON_CreateString(newInfo.m_time.c_str()));
			cJSON_AddItemToArray(archivesArray, newArchive);
			cJSON_SetNumberValue(currentID, currentMaxID + 1);
			//Update ArchivesCount
			cJSON* archivesCount = cJSON_GetObjectItem(root, "ArchivesCount");
			if (archivesCount && cJSON_IsNumber(archivesCount)) {
				cJSON_SetNumberValue(archivesCount, cJSON_GetArraySize(archivesArray));
				printf("Now there are %d archives in total.\n", archivesCount->valueint);
			}
		}
		else {
			printf("Get new YgoArchiveInfo failed, cannot create new archive entry.\n");
			cJSON_Delete(root);
			file.close();
			return false;
		}
	}
	//Write back to file
	std::string jsonFileString = std::string(cJSON_Print(root));
	file.seekp(0, std::ios::beg);
	file.write(jsonFileString.c_str(), jsonFileString.size());

	cJSON_Delete(root);
	file.close();
	// Update m_archives
	m_archives.clear();
	QuerryArchiveList(DEFAULT_MAX_ARCHIVE_LIST_SIZE, false, true);
	printf("ArchiveList file updated successfully for ArchiveID %d.\n", targetID);
	return true;
}

bool YgoMasterArchiveMgr::BackupAndCreateNewArchive()
{
	//Create a new archive entry and backup the latest archive
	if (BackupArchive(0, true)) {
		return true;
	}
	return false;
}

bool YgoMasterArchiveMgr::DeleteArchive(const int archiveID)
{
	//Delete a specific archive by archiveID
	printf("Deleting ArchiveID %d...\n", archiveID);
	auto it = m_archives.find(archiveID);
	if (it == m_archives.end()) {
		printf("ArchiveID %d not found.\n", archiveID);
		return false;
	}
	const std::string archivePath = it->second.m_path;
	try {
		if (fs::exists(archivePath)) {
			fs::remove_all(archivePath);
			printf("Archive directory %s deleted successfully.\n", archivePath.c_str());
			//Update ArchiveList file
			std::fstream file(m_YMListPath,
				std::ios::in | std::ios::out | std::ios::binary);
			if (!file.is_open()) {
				printf("Open ArchiveList file failed for deletion.\n");
				return false;
			}
			std::string jsonContent((std::istreambuf_iterator<char>(file)),
				std::istreambuf_iterator<char>());
			cJSON* root = cJSON_Parse(jsonContent.c_str());
			if (!root) {
				printf("Parse ArchiveList file failed for deletion.\n");
				file.close();
				return false;
			}
			cJSON* archivesArray = cJSON_GetObjectItem(root, "Archives");
			if (!archivesArray || !cJSON_IsArray(archivesArray)) {
				printf("ArchiveList file format error: Archives is not an array.\n");
				cJSON_Delete(root);
				file.close();
				return false;
			}
			int size = cJSON_GetArraySize(archivesArray);
			for (int i = 0; i < size; ++i) {
				cJSON* archiveItem = cJSON_GetArrayItem(archivesArray, i);
				if (!archiveItem) continue;
				cJSON* idItem = cJSON_GetObjectItem(archiveItem, "id");
				if (idItem && cJSON_IsNumber(idItem) && idItem->valueint == archiveID) {
					//Found targetID, remove it from array
					cJSON_DeleteItemFromArray(archivesArray, i);
					//Update ArchivesCount
					cJSON* archivesCount = cJSON_GetObjectItem(root, "ArchivesCount");
					if (archivesCount && cJSON_IsNumber(archivesCount)) {
						cJSON_SetNumberValue(archivesCount, cJSON_GetArraySize(archivesArray));
						printf("Now there are %d archives in total.\n", archivesCount->valueint);
					}
					break;
				}
			}
			//Write back to file
			std::string jsonFileString = std::string(cJSON_Print(root));
			file.seekp(0, std::ios::beg);
			file.write(jsonFileString.c_str(), jsonFileString.size());
			cJSON_Delete(root);
			file.close();

			//Remove from m_archives
			m_archives.erase(it);
			return true;
		}
		else {
			printf("Archive directory %s does not exist, skipping deletion.\n", archivePath.c_str());
			m_archives.erase(it);
			return true;
		}
	}
	catch (const fs::filesystem_error& e) {
		printf("Error deleting archive directory %s: %s\n", archivePath.c_str(), e.what());
		return false;
	}

	return true;
}

bool YgoMasterArchiveMgr::RestoreArchive(const int archiveID, const bool backup)
{
	if (backup) {
		//Backup current data first
		printf("Backing up current data before restoring...\n");
		if (!BackupArchive(m_currentArchiveIndex, false)) {
			printf("Backup current data failed, cannot restore archive.\n");
			return false;
		}
	}
	printf("Restoring ArchiveID %d...\n", archiveID);
	//Find the archive
	auto it = m_archives.find(archiveID);
	if (it == m_archives.end()) {
		printf("ArchiveID %d not found.\n", archiveID);
		return false;
	}
	const YgoArchiveInfo& archive = it->second;
	//Copy target files or directories back to YMDataPath
	for (const auto& target : sc_BackupTargets) {
		fs::path sourcePath = fs::path(archive.m_path) / fs::path(target.second);
		fs::path destPath = fs::path(m_YMDataPath) / fs::path(target.second);
		if (!fs::exists(sourcePath)) {
			printf("Source path %s not exist in the archive, skipping.\n", sourcePath.string().c_str());
			continue;
		}
		try {
			if (target.first == IS_FILE) {
				fs::copy_file(sourcePath, destPath, fs::copy_options::overwrite_existing);
			}
			else if (target.first == IS_DIRECTORY) {
				fs::copy(sourcePath, destPath, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
			}
			else {
				printf("Unknown target type for %s, skipping.\n", sourcePath.string().c_str());
			}
		}
		catch (const fs::filesystem_error& e) {
			printf("Error restoring %s to %s: %s\n",
				sourcePath.string().c_str(),
				destPath.string().c_str(),
				e.what());
			return false;
		}
	}
	printf("ArchiveID %d restored successfully.\n", archiveID);
	//Update Currently in use ArchiveID in ArchiveList file
	std::fstream file(m_YMListPath,
		std::ios::in | std::ios::out | std::ios::binary);
	if (!file.is_open()) {
		printf("Open ArchiveList file failed for updating current archive index.\n");
		return false;
	}
	std::string jsonContent((std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>());
	cJSON* root = cJSON_Parse(jsonContent.c_str());
	if (!root) {
		printf("Parse ArchiveList file failed for updating current archive index.\n");
		file.close();
		return false;
	}
	cJSON* currentID = cJSON_GetObjectItem(root, "Currently in use ArchiveID");
	if (!currentID || !cJSON_IsNumber(currentID)) {
		printf("ArchiveList file format error: Currently in use ArchiveID is not a number..\n");
		cJSON_Delete(root);
		file.close();
		return false;
	}
	cJSON_SetNumberValue(currentID, archiveID);
	//Write back to file
	std::string jsonFileString = std::string(cJSON_Print(root));
	file.seekp(0, std::ios::beg);
	file.write(jsonFileString.c_str(), jsonFileString.size());
	cJSON_Delete(root);
	file.close();
	//Update current archive index
	m_currentArchiveIndex = archiveID;
	printf("Current archive index updated successfully to ArchiveID %d.\n", archiveID);
	return true;
}

void GetYgoMasterMgr(IYgoMasterMgr** imp)
{
	*imp = new YgoMasterArchiveMgr();
	return;
}

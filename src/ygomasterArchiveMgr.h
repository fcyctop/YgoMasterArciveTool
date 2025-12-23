#include<interface.h>
#include"public.h"

//Input options
enum class EInputOption: int
{
	EXIT = 0, // Exit the program
	BACKUP_ARCHIVE_REPLACE = 1, // Backup the current archive, replace the existing one
	BACKUP_ARCHIVE_COPY, // Backup the current archive, create a new one
	DISPLAY_ARCHIVE_LIST, // Display the list of archives
	DISPLAY_ARCHIVE_DETAIL, // Display detailed info for a specific archive
	DELETE_ARCHIVE, // Delete a specific archive
	RESTORE_ARCHIVE, // Restore a specific archive
	RESTORE_ARCHIVE_WITH_BACKUP, // Restore a specific archive with backup
	SIZE_OF_OPTIONS // Keep this as the last item
};
static const std::vector<std::pair<int, std::string>> sc_InputOptions = {
	{ (int)EInputOption::EXIT, "Exit the program" },
	{ (int)EInputOption::BACKUP_ARCHIVE_REPLACE, "Backup the current archive, replace the existing one" },
	{ (int)EInputOption::BACKUP_ARCHIVE_COPY, "Backup the current archive, copy a new one" },
	{ (int)EInputOption::DISPLAY_ARCHIVE_LIST, "Display the list of archives" },
	{ (int)EInputOption::DISPLAY_ARCHIVE_DETAIL, "Display a info of archive" },
	{ (int)EInputOption::DELETE_ARCHIVE, "Delete a specific archive" },
    { (int)EInputOption::RESTORE_ARCHIVE, "Restore a specific archive" },
    { (int)EInputOption::RESTORE_ARCHIVE_WITH_BACKUP, "Restore a specific archive after backup(replace)" }
};

// Search paths for YgoMaster Data directory
static const std::vector<std::string> sc_YgoArchiveSearchPaths = {
	"\\..\\Data",
	"\\Data",
	"\\YgoMaster\\Data",
};

static const std::string sc_YgoPlayerJsonSearchPath = "\\Players\\Local\\Player.json";
static const std::string sc_YgoSettingsJsonSearchPath = "\\Settings.json";

// Backup targets: files or directories under the YgoMaster Data directory
constexpr int IS_FILE = 0;
constexpr int IS_DIRECTORY = 1;
static const std::vector<std::pair<int, std::string>> sc_BackupTargets = {
	{ IS_FILE, "Settings.json"},
	{ IS_DIRECTORY, "Players"}
};

// Default maximum number of archives to display
constexpr int DEFAULT_MAX_ARCHIVE_LIST_SIZE = 5;

// Structure to hold YgoMaster Archive information
struct YgoArchiveInfo
{
	int m_id;
	std::string m_name;
	std::string m_path;
	std::string m_desc;
	std::string m_time;

	YgoArchiveInfo() :m_id(0), m_name(""), m_path(""), m_desc("") {}

} *YgoMasterInfoPtr;

static const std::string sc_configDescText = 
"This file must be placed in the same directory as test.exe."
"YMListPath points to the save path of \'YgoMasterArchiveList.json\' file."
"YMDataPath points to the \'Data\' directory of YgoMaster."
"YMArchivesPath points to the directory where backups are stored."
"If there is a change in the positions of the above files or folders, "
"the following paths need to be modified so that the program can accurately retrieve them!";


class YgoMasterArchiveMgr : public IYgoMasterMgr
{
public:
	YgoMasterArchiveMgr();
	virtual ~YgoMasterArchiveMgr() {};

	virtual void Run()override;

private:
	// Read config file and set m_YMListPath
	bool ReadConfig();
	// Read YgoMasterList file and populate m_archives
	bool ReadYMList();
	// Backup the newst archive to YgoMasterList file
	bool BackupArchive(const int targetID, const bool copy = false);
	// Backup the latest archive and save it as a new archive
	bool BackupAndCreateNewArchive();
	// Delete a specific archive by archiveID
	bool DeleteArchive(const int archiveID);
	// Restore a specific archive by archiveID
	bool RestoreArchive(const int archiveID, const bool backup = false);

	bool CheckYMDataDir();
	// Display the archive list, if updateArchives is true, update m_archives
	bool QuerryArchiveList(const int maxSize = DEFAULT_MAX_ARCHIVE_LIST_SIZE, const bool display = true, const bool updateArchives = false);
	// Display detailed info for a specific archiveID, if archiveID is -1 display current archive
	void DisplayArchiveDetail(const int archiveID);
	// Get new YgoMaster archive info from user input
	bool GetNewYgoArchiveInfo(YgoArchiveInfo& result, const bool needDesc = true);
	// Copy target files from YgoMaster Data directory to result path
	bool CopyTargetFiles(YgoArchiveInfo& result);

	// Reset archive data for a specific archiveID
	enum class YMArchiveData :int
	{
		MIN_DATA = -1,
		CONFIG_DATA_DESC = 0,
		CONFIG_DATA_SPLIT,
		ARCHIVE_DATA_GEMS,
		MAX_DATA
	};
	bool ResetData(const int archiveID, const YMArchiveData& YMdataID);

private:
	std::string m_YMDataPath;
	std::string m_YMListPath;
	std::string m_configPath;
	std::string m_archivesPath;

	int m_currentArchiveIndex;
	std::unordered_map<int, YgoArchiveInfo> m_archives;
};


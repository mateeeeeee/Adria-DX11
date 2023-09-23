#pragma once
#include <filesystem>
#include <chrono>
#include "HashMap.h"
#include "Core/CoreTypes.h"
#include "Events/Delegate.h"

namespace fs = std::filesystem;

namespace adria
{
	enum class EFileStatus : uint8
	{ 
		Created, 
		Modified, 
		Deleted 
	};

	DECLARE_EVENT(FileModifiedEvent, FileWatcher, std::string const&);

	class FileWatcher
	{
	public:
		FileWatcher() = default;
		~FileWatcher()
		{
			file_modified_event.RemoveAll();
		}

		void AddPathToWatch(std::string const& path, bool recursive = true)
		{
			if (recursive)
			{
				for (auto& path : fs::recursive_directory_iterator(path))
				{
					if(path.is_regular_file()) files_map[path.path().string()] = fs::last_write_time(path);
				}
			}
			else
			{
				for (auto& path : fs::directory_iterator(path))
				{
					if (path.is_regular_file()) files_map[path.path().string()] = fs::last_write_time(path);
				}
			}
		}
		void CheckWatchedFiles()
		{
			for (auto const& [file, ft] : files_map)
			{
				auto current_file_last_write_time = fs::last_write_time(file);
				if (files_map[file] != current_file_last_write_time)
				{
					files_map[file] = current_file_last_write_time;
					file_modified_event.Broadcast(file);
				}
			}
		}
		
		FileModifiedEvent& GetFileModifiedEvent() { return file_modified_event; }

	private:
		std::vector<std::string> paths_to_watch;
		HashMap<std::string, fs::file_time_type> files_map;
		FileModifiedEvent file_modified_event;
	};
}
#pragma once
#include <filesystem>
#include <chrono>
#include <thread>
#include "HashMap.h"
#include "../Core/Definitions.h"
#include "../Events/Delegate.h"

namespace adria
{
	enum class EFileStatus : uint8
	{ 
		Created, 
		Modified, 
		Deleted 
	};

	DECLARE_EVENT(FileModified, FileWatcher, std::string const&);

	class FileWatcher
	{
	public:
		FileWatcher() = default;
		~FileWatcher() = default;

		void AddPathToWatch(std::string const& path, bool recursive = true)
		{
			if (recursive)
			{
				for (auto& file : std::filesystem::recursive_directory_iterator(path))
				{
					paths_map[file.path().string()] = std::filesystem::last_write_time(file);
				}
			}
			else
			{
				for (auto& file : std::filesystem::directory_iterator(path))
				{
					paths_map[file.path().string()] = std::filesystem::last_write_time(file);
				}
			}
		}
		void CheckWatchedFiles()
		{
			for (auto const& [file, ft] : paths_map)
			{
				auto current_file_last_write_time = std::filesystem::last_write_time(file);
				if (paths_map[file] != current_file_last_write_time)
				{
					paths_map[file] = current_file_last_write_time;
					file_modified_event.Broadcast(file);
				}
			}
		}
		
		FileModified& GetFileModifiedEvent() { return file_modified_event; }

	private:
		std::vector<std::string> paths_to_watch;
		HashMap<std::string, std::filesystem::file_time_type> paths_map;
		FileModified file_modified_event;
	};
}
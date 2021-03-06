#pragma once

#include <iostream>
#include <vector>
#include <Windows.h>
#include <string>

struct FileNode
{
  enum Type
  {
    NONE,
    FOLDER,
    FILE
  };

  FileNode(Type A_Type, std::string A_Name)
    :FileType(A_Type)
    , Name(A_Name)
  {}

  Type FileType;
  std::string Name;
};

class FolderCrawler
{
public:
  void Crawl(std::string Path)
  {
    CrawlFolder(Path, 0);
  }

private:

  void CrawlFolder(std::string Path, int Level)
  {
    std::vector<FileNode> Files = ListCurrentFolder(Path + "\\*");
    for (int i = 0; i < Files.size(); i++)
    {
      switch (Files[i].FileType)
      {
      case FileNode::FILE:
        ProcessFile(Path + "\\" + Files[i].Name, Level + 1);
        break;
      case FileNode::FOLDER:
        ProcessFile(Path + "\\" + Files[i].Name, Level + 1); CrawlFolder(Path + "\\" + Files[i].Name, Level + 1); break;
      }

    }
  }

  std::vector<FileNode> ListCurrentFolder(std::string Path)
  {
    std::vector<FileNode> Files;

    WIN32_FIND_DATA FindFileData;
    HANDLE hFind;

    hFind = FindFirstFile(Path.c_str(), &FindFileData);
    while (hFind != INVALID_HANDLE_VALUE)
    {
      if (std::string(FindFileData.cFileName) != "." && std::string(FindFileData.cFileName) != "..")
      {
        FileNode Node = FileNode(
          FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? FileNode::FOLDER : FileNode::FILE,
          FindFileData.cFileName);
        Files.push_back(Node);
      }

      if (!FindNextFile(hFind, &FindFileData))
      {
        hFind = INVALID_HANDLE_VALUE;
      }
    }

    return Files;
  }

  virtual void ProcessFile(std::string Name, int Level) {};
  virtual void ProcessFolder(std::string Name, int Level) {};
};

class StringReplacer : public FolderCrawler
{
public:
  StringReplacer(std::string A_OldWord, std::string A_NewWord)
    :OldWord(A_OldWord)
    , NewWord(A_NewWord)
  {
  }
  std::string OldWord;
  std::string NewWord;
protected:
  virtual void ProcessFile(std::string Name, int Level)
  {
    std::ifstream InFile(Name);
    std::ofstream OutFile(Name + ".temp");
    std::string Line;

    int WordLength = OldWord.size();

    while (getline(InFile, Line))
    {
      int Start = std::string::npos;
      do
      {
        Start = Line.find(OldWord);
        if (Start != std::string::npos)
          Line.replace(Start, WordLength, NewWord);
      } while (Start != std::string::npos);

      OutFile << Line << std::endl;
    }

    InFile.close();
    OutFile.close();

    remove(Name.c_str());
    rename((Name + ".temp").c_str(), Name.c_str());
  };

  virtual void ProcessFolder(std::string Name, int Level)
  {

  };
};

class FolderPrinter : public FolderCrawler
{
protected:
  void PrintName(std::string Name, int Level, bool bIsFile)
  {
    std::cout << std::endl;
    for (int i = 0; i < Level; i++)
    {
      std::cout << " ";
    }

    if (bIsFile)
      std::cout << "-" << Name.c_str();
    else
      std::cout << Name.c_str();
  }

  virtual void ProcessFile(std::string Name, int Level)
  {
    PrintName(Name, Level, true);
  };

  virtual void ProcessFolder(std::string Name, int Level)
  {
    PrintName(Name, Level, false);
  };
};
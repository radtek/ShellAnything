/**********************************************************************************
 * MIT License
 * 
 * Copyright (c) 2018 Antoine Beauchamp
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *********************************************************************************/

#include "shellanything/ConfigManager.h"
#include "shellanything/Menu.h"

#include "rapidassist/filesystem_utf8.h"
#include "rapidassist/strings.h"

#pragma warning( push )
#pragma warning( disable: 4355 ) // glog\install_dir\include\glog/logging.h(1167): warning C4355: 'this' : used in base member initializer list
#include <glog/logging.h>
#pragma warning( pop )

namespace shellanything
{

  ConfigManager::ConfigManager()
  {
  }

  ConfigManager::~ConfigManager()
  {
  }

  ConfigManager & ConfigManager::GetInstance()
  {
    static ConfigManager _instance;
    return _instance;
  }

  void ConfigManager::Clear()
  {
    ClearSearchPath(); //remove all search path to make sure that a refresh won�t find any other configuration file
    mConfigurations.RemoveChildren();
    Refresh(); //forces all loaded configurations to be unloaded
  }

  void ConfigManager::Refresh()
  {
    LOG(INFO) << __FUNCTION__ << "()";
    
    //validate existing configurations
    Configuration::ConfigurationPtrList existing = GetConfigurations();
    for(size_t i=0; i<existing.size(); i++)
    {
      Configuration * config = existing[i];

      //compare the file's date at the load time and the current date
      const std::string & file_path = config->GetFilePath();
      const uint64_t & old_file_date = config->GetFileModifiedDate();
      const uint64_t new_file_date = ra::filesystem::GetFileModifiedDateUtf8(file_path);
      if (ra::filesystem::FileExistsUtf8(file_path.c_str()) && old_file_date == new_file_date)
      {
        //current configuration is up to date
        LOG(INFO) << "Configuration file '" << file_path << "' is up to date.";
      }
      else
      {
        //file is missing or current configuration is out of date
        //forget about existing config
        LOG(INFO) << "Configuration file '" << file_path << "' is missing or is not up to date. Deleting configuration.";
        mConfigurations.RemoveChild(config);
      }
    }
   
    //search every known path
    for (size_t i=0; i<mPaths.size(); i++)
    {
      const std::string & path = mPaths[i];
 
      LOG(INFO) << "Searching configuration files in directory '" << path << "'";

      //search files in each directory
      ra::strings::StringVector files;
      bool dir_found = ra::filesystem::FindFilesUtf8(files, path.c_str());
      if (dir_found)
      {
        //search through each files for *.xml files
        for(size_t j=0; j<files.size(); j++)
        {
          const std::string & file_path = files[j];
          if (Configuration::IsValidConfigFile(file_path))
          {
            //is this file already loaded ?
            if (!IsConfigFileLoaded(file_path))
            {
              LOG(INFO) << "Found new configuration file '" << file_path << "'";

              //parse the file
              std::string error;
              Configuration * config = Configuration::LoadFile(file_path, error);
              if (config == NULL)
              {
                //log an error message
                LOG(ERROR) << "Failed loading configuration file '" << file_path << "'. Error=" << error << ".";
              }
              else
              {
                //add to current list of configurations
                mConfigurations.AddChild(config);

                //apply default properties of the configuration
                config->ApplyDefaultSettings();
              }
            }
            else
            {
              LOG(INFO) << "Skipped configuration file '" << file_path << "'. File is already loaded.";
            }
          }
        }
      }
      else
      {
        //log an error message
        LOG(ERROR) << "Failed searching for configuration files in directory '" << path << "'.";
      }
    }
  }

  void ConfigManager::Update(const Context & c)
  {
    //for each child
    Configuration::ConfigurationPtrList configurations = ConfigManager::GetConfigurations();
    for(size_t i=0; i<configurations.size(); i++)
    {
      Configuration * config = configurations[i];
      config->Update(c);
    }
  }

  Menu * ConfigManager::FindMenuByCommandId(const uint32_t & iCommandId)
  {
    //for each child
    Configuration::ConfigurationPtrList configurations = ConfigManager::GetConfigurations();
    for(size_t i=0; i<configurations.size(); i++)
    {
      Configuration * config = configurations[i];
      Menu * match = config->FindMenuByCommandId(iCommandId);
      if (match)
        return match;
    }
 
    return NULL;
  }
 
  uint32_t ConfigManager::AssignCommandIds(const uint32_t & iFirstCommandId)
  {
    uint32_t nextCommandId = iFirstCommandId;

    //for each child
    Configuration::ConfigurationPtrList configurations = ConfigManager::GetConfigurations();
    for(size_t i=0; i<configurations.size(); i++)
    {
      Configuration * config = configurations[i];
      nextCommandId = config->AssignCommandIds(nextCommandId);
    }
 
    return nextCommandId;
  }
 
  Configuration::ConfigurationPtrList ConfigManager::GetConfigurations()
  {
    Configuration::ConfigurationPtrList configurations = FilterNodes<Configuration*>(mConfigurations.FindChildren("Configuration"));
    return configurations;
  }

  void ConfigManager::ClearSearchPath()
  {
    mPaths.clear();
  }

  void ConfigManager::AddSearchPath(const std::string & path)
  {
    mPaths.push_back(path);
  }

  bool ConfigManager::IsConfigFileLoaded(const std::string & path) const
  {
    for(size_t i=0; i<mConfigurations.Size(); i++)
    {
      const Node * node = mConfigurations.GetChild(i);
      const Configuration * config = dynamic_cast<const Configuration *>(node);
      if (config != NULL && config->GetFilePath() == path)
        return true;
    }
    return false;
  }

} //namespace shellanything

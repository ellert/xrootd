#ifndef __XRDPFC_CACHE_HH__
#define __XRDPFC_CACHE_HH__
//----------------------------------------------------------------------------------
// Copyright (c) 2014 by Board of Trustees of the Leland Stanford, Jr., University
// Author: Alja Mrak-Tadel, Matevz Tadel, Brian Bockelman
//----------------------------------------------------------------------------------
// XRootD is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// XRootD is distributed in the hope that it will be useful,
// but WITHOUT ANY emacs WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with XRootD.  If not, see <http://www.gnu.org/licenses/>.
//----------------------------------------------------------------------------------
#include <string>
#include <list>
#include <map>
#include <set>

#include "Xrd/XrdScheduler.hh"
#include "XrdVersion.hh"
#include "XrdSys/XrdSysPthread.hh"
#include "XrdOuc/XrdOucCache.hh"
#include "XrdOuc/XrdOucCallBack.hh"

#include "XrdPfcFile.hh"
#include "XrdPfcDecision.hh"

class XrdOss;
class XrdOucStream;
class XrdSysError;
class XrdSysTrace;
class XrdXrootdGStream;

namespace XrdPfc
{
class File;
class IO;
class PurgePin;
class ResourceMonitor;


template<class MOO>
struct MutexHolder {
   MOO &mutex;
   MutexHolder(MOO &m) : mutex(m) { mutex.Lock(); }
   ~MutexHolder() { mutex.UnLock(); }
};
}


namespace XrdPfc
{

//----------------------------------------------------------------------------
//! Contains parameters configurable from the xrootd config file.
//----------------------------------------------------------------------------
struct Configuration
{
   Configuration();

   bool are_file_usage_limits_set()    const { return m_fileUsageMax > 0; }
   bool is_age_based_purge_in_effect() const { return m_purgeColdFilesAge > 0 ; }
   bool is_uvkeep_purge_in_effect()    const { return m_cs_UVKeep >= 0; }
   bool is_dir_stat_reporting_on()     const { return m_dirStatsMaxDepth >= 0 || ! m_dirStatsDirs.empty() || ! m_dirStatsDirGlobs.empty(); }
   bool is_purge_plugin_set_up()       const { return false; }

   CkSumCheck_e get_cs_Chk() const { return (CkSumCheck_e) m_cs_Chk; }

   bool is_cschk_cache() const { return m_cs_Chk & CSChk_Cache; }
   bool is_cschk_net()   const { return m_cs_Chk & CSChk_Net;   }
   bool is_cschk_any()   const { return m_cs_Chk & CSChk_Both;  }
   bool is_cschk_both()  const { return (m_cs_Chk & CSChk_Both) == CSChk_Both; }

   bool does_cschk_have_missing_bits(CkSumCheck_e cks_on_file) const { return m_cs_Chk & ~cks_on_file; }

   bool should_uvkeep_purge(time_t delta) const { return m_cs_UVKeep >= 0 && delta > m_cs_UVKeep; }

   bool m_hdfsmode;                     //!< flag for enabling block-level operation
   bool m_allow_xrdpfc_command;         //!< flag for enabling access to /xrdpfc-command/ functionality.

   std::string m_username;              //!< username passed to oss plugin
   std::string m_data_space;            //!< oss space for data files
   std::string m_meta_space;            //!< oss space for metadata files (cinfo)

   long long m_diskTotalSpace;          //!< total disk space on configured partition or oss space
   long long m_diskUsageLWM;            //!< cache purge - disk usage low water mark
   long long m_diskUsageHWM;            //!< cache purge - disk usage high water mark
   long long m_fileUsageBaseline;       //!< cache purge - files usage baseline
   long long m_fileUsageNominal;        //!< cache purge - files usage nominal
   long long m_fileUsageMax;            //!< cache purge - files usage maximum
   int       m_purgeInterval;           //!< sleep interval between cache purges
   int       m_purgeColdFilesAge;       //!< purge files older than this age
   int       m_purgeAgeBasedPeriod;     //!< peform cold file / uvkeep purge every this many purge cycles
   int       m_accHistorySize;          //!< max number of entries in access history part of cinfo file

   std::set<std::string> m_dirStatsDirs;     //!< directories for which stat reporting was requested
   std::set<std::string> m_dirStatsDirGlobs; //!< directory globs for which stat reporting was requested
   int       m_dirStatsInterval;        //!< time between resource monitor statistics dump in seconds
   int       m_dirStatsMaxDepth;        //!< maximum depth for statistics write out
   int       m_dirStatsStoreDepth;      //!< depth to which statistics should be collected

   long long m_bufferSize;              //!< prefetch buffer size, default 1MB
   long long m_RamAbsAvailable;         //!< available from configuration
   int       m_RamKeepStdBlocks;        //!< number of standard-sized blocks kept after release
   int       m_wqueue_blocks;           //!< maximum number of blocks written per write-queue loop
   int       m_wqueue_threads;          //!< number of threads writing blocks to disk
   int       m_prefetch_max_blocks;     //!< maximum number of blocks to prefetch per file

   long long m_hdfsbsize;               //!< used with m_hdfsmode, default 128MB
   long long m_flushCnt;                //!< nuber of unsynced blcoks on disk before flush is called

   time_t    m_cs_UVKeep;               //!< unverified checksum cache keep
   int       m_cs_Chk;                  //!< Checksum check
   bool      m_cs_ChkTLS;               //!< Allow TLS

   long long m_onlyIfCachedMinSize;     //!< minumum size of downloaded file, used by only-if-cached CGI option
   double    m_onlyIfCachedMinFrac;     //!< minimum fraction of downloaded file, used by only-if-cached CGI option
};

//------------------------------------------------------------------------------

struct TmpConfiguration
{
   std::string m_diskUsageLWM;
   std::string m_diskUsageHWM;
   std::string m_fileUsageBaseline;
   std::string m_fileUsageNominal;
   std::string m_fileUsageMax;
   std::string m_flushRaw;

   TmpConfiguration() :
      m_diskUsageLWM("0.90"), m_diskUsageHWM("0.95"),
      m_flushRaw("")
   {}
};


//==============================================================================
// Cache
//==============================================================================

//----------------------------------------------------------------------------
//! Attaches/creates and detaches/deletes cache-io objects for disk based cache.
//----------------------------------------------------------------------------
class Cache : public XrdOucCache
{
public:
   //---------------------------------------------------------------------
   //! Constructor
   //---------------------------------------------------------------------
   Cache(XrdSysLogger *logger, XrdOucEnv *env);

   //---------------------------------------------------------------------
   //! Obtain a new IO object that fronts existing XrdOucCacheIO.
   //---------------------------------------------------------------------
   using XrdOucCache::Attach;

   virtual XrdOucCacheIO *Attach(XrdOucCacheIO *, int Options = 0);

   //---------------------------------------------------------------------
   // Virtual function of XrdOucCache. Used for redirection to a local
   // file on a distributed FS.
   virtual int LocalFilePath(const char *url, char *buff=0, int blen=0,
                             LFP_Reason why=ForAccess, bool forall=false);

   //---------------------------------------------------------------------
   // Virtual function of XrdOucCache. Used for deferred open.
   virtual int  Prepare(const char *url, int oflags, mode_t mode);

   // virtual function of XrdOucCache.
   virtual int  Stat(const char *url, struct stat &sbuff);

   // virtual function of XrdOucCache.
   virtual int  Unlink(const char *url);

   //---------------------------------------------------------------------
   //  Used by PfcFstcl::Fsctl function.
   //  Test if file is cached taking in onlyifcached configuration parameters.
   //---------------------------------------------------------------------
   virtual int ConsiderCached(const char *url);

   bool DecideIfConsideredCached(long long file_size, long long bytes_on_disk);
   void WriteFileSizeXAttr(int cinfo_fd, long long file_size);
   long long DetermineFullFileSize(const std::string &cinfo_fname);

   //--------------------------------------------------------------------
   //! \brief Makes decision if the original XrdOucCacheIO should be cached.
   //!
   //! @param & URL of file
   //!
   //! @return decision if IO object will be cached.
   //--------------------------------------------------------------------
   bool Decide(XrdOucCacheIO*);

   //------------------------------------------------------------------------
   //! Reference XrdPfc configuration
   //------------------------------------------------------------------------
   const Configuration& RefConfiguration() const { return m_configuration; }

   //---------------------------------------------------------------------
   //! \brief Parse configuration file
   //!
   //! @param config_filename    path to configuration file
   //! @param parameters         optional parameters to be passed
   //!
   //! @return parse status
   //---------------------------------------------------------------------
   bool Config(const char *config_filename, const char *parameters);

   //---------------------------------------------------------------------
   //! Singleton creation.
   //---------------------------------------------------------------------
   static Cache &CreateInstance(XrdSysLogger *logger, XrdOucEnv *env);

  //---------------------------------------------------------------------
   //! Singleton access.
   //---------------------------------------------------------------------
   static       Cache         &GetInstance();
   static const Cache         &TheOne();
   static const Configuration &Conf();

   static ResourceMonitor     &ResMon();

   //---------------------------------------------------------------------
   //! Version check.
   //---------------------------------------------------------------------
   static bool VCheck(XrdVersionInfo &urVersion) { return true; }

   //---------------------------------------------------------------------
   //! Remove cinfo and data files from cache.
   //---------------------------------------------------------------------
   int  UnlinkFile(const std::string& f_name, bool fail_if_open);

   //---------------------------------------------------------------------
   //! Add downloaded block in write queue.
   //---------------------------------------------------------------------
   void AddWriteTask(Block* b, bool from_read);

   //---------------------------------------------------------------------
   //!  \brief Remove blocks from write queue which belong to given prefetch.
   //! This method is used at the time of File destruction.
   //---------------------------------------------------------------------
   void RemoveWriteQEntriesFor(File *f);

   //---------------------------------------------------------------------
   //! Separate task which writes blocks from ram to disk.
   //---------------------------------------------------------------------
   void ProcessWriteTasks();

   long long WritesSinceLastCall();

   char* RequestRAM(long long size);
   void  ReleaseRAM(char* buf, long long size);

   void RegisterPrefetchFile(File*);
   void DeRegisterPrefetchFile(File*);

   File* GetNextFileToPrefetch();

   void Prefetch();

   XrdOss* GetOss() const { return m_oss; }

   bool IsFileActiveOrPurgeProtected(const std::string&) const;
   void ClearPurgeProtectedSet();
   PurgePin* GetPurgePin() const { return m_purge_pin; }

   File* GetFile(const std::string&, IO*, long long off = 0, long long filesize = 0);

   void  ReleaseFile(File*, IO*);

   void ScheduleFileSync(File* f) { schedule_file_sync(f, false, false); }

   void FileSyncDone(File*, bool high_debug);

   XrdSysError* GetLog()   { return &m_log;  }
   XrdSysTrace* GetTrace() { return m_trace; }

   ResourceMonitor& RefResMon() { return *m_res_mon; }
   XrdXrootdGStream* GetGStream() { return m_gstream; }

   void ExecuteCommandUrl(const std::string& command_url);

   static XrdScheduler *schedP;

private:
   bool ConfigParameters(std::string, XrdOucStream&, TmpConfiguration &tmpc);
   bool ConfigXeq(char *, XrdOucStream &);
   bool xcschk(XrdOucStream &);
   bool xdlib(XrdOucStream &);
   bool xplib(XrdOucStream &);
   bool xtrace(XrdOucStream &);
   bool test_oss_basics_and_features();

   bool cfg2bytes(const std::string &str, long long &store, long long totalSpace, const char *name);

   static Cache     *m_instance;        //!< this object

   XrdOucEnv        *m_env;             //!< environment passed in at creation
   XrdSysError       m_log;             //!< XrdPfc namespace logger
   XrdSysTrace      *m_trace;
   const char       *m_traceID;

   XrdOss           *m_oss;             //!< disk cache file system

   XrdXrootdGStream *m_gstream;

   ResourceMonitor  *m_res_mon;

   std::vector<Decision*> m_decisionpoints; //!< decision plugins
   PurgePin*              m_purge_pin;      //!< purge plugin

   Configuration m_configuration;           //!< configurable parameters

   XrdSysCondVar m_prefetch_condVar;        //!< lock for vector of prefetching files
   bool          m_prefetch_enabled;        //!< set to true when prefetching is enabled

   XrdSysMutex m_RAM_mutex;                 //!< lock for allcoation of RAM blocks
   long long   m_RAM_used;
   long long   m_RAM_write_queue;
   std::list<char*> m_RAM_std_blocks;       //!< A list of blocks of standard size, to be reused.
   int              m_RAM_std_size;

   bool        m_isClient;                  //!< True if running as client
   bool        m_dataXattr = false;         //!< True if xattrs are available on the data space
   bool        m_metaXattr = false;         //!< True if xattrs are available on the meta space

   struct WriteQ
   {
      WriteQ() : condVar(0), writes_between_purges(0), size(0) {}

      XrdSysCondVar     condVar;      //!< write list condVar
      std::list<Block*> queue;        //!< container
      long long         writes_between_purges; //!< upper bound on amount of bytes written between two purge passes
      int               size;         //!< current size of write queue
   };

   WriteQ m_writeQ;

   // active map, purge delay set
   typedef std::map<std::string, File*>               ActiveMap_t;
   typedef ActiveMap_t::iterator                      ActiveMap_i;
   typedef std::set<std::string>                      FNameSet_t;

   ActiveMap_t            m_active;          //!< Map of currently active / open files.
   FNameSet_t             m_purge_delay_set; //!< Set of files that should not be purged.
   mutable XrdSysCondVar  m_active_cond;     //!< Cond-var protecting active file data structures.

   void inc_ref_cnt(File*, bool lock, bool high_debug);
   void dec_ref_cnt(File*, bool high_debug);

   void schedule_file_sync(File*, bool ref_cnt_already_set, bool high_debug);

   // prefetching
   typedef std::vector<File*>  PrefetchList;
   PrefetchList m_prefetchList;
};

}

#endif

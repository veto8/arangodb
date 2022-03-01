////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2022 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Jan Steemann
/// @author Jan Christoph Uhde
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Aql/types.h"
#include "Basics/Common.h"
#include "Basics/Result.h"
#include "Cluster/ClusterTypes.h"
#include "Basics/ResultT.h"
#include "Replication/Syncer.h"
#include "Replication/common-defines.h"
#include "RestHandler/RestVocbaseBaseHandler.h"
#include "StorageEngine/ReplicationIterator.h"

#include <string>
#include <unordered_set>

namespace arangodb {
class ClusterInfo;
class CollectionNameResolver;
class LogicalCollection;
class ReplicationApplier;
class SingleCollectionTransaction;

namespace transaction {
class Methods;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief replication request handler
////////////////////////////////////////////////////////////////////////////////

class RestReplicationHandler : public RestVocbaseBaseHandler {
 public:
  RequestLane lane() const override final;

  RestStatus execute() override;

  // Never instantiate this.
  // Only specific implementations allowed
 protected:
  RestReplicationHandler(ArangodServer&, GeneralRequest*, GeneralResponse*);

 public:
  static std::string const Revisions;
  static std::string const Tree;
  static std::string const Ranges;
  static std::string const Documents;

 protected:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief list of available commands
  //////////////////////////////////////////////////////////////////////////////
  static std::string const LoggerState;
  static std::string const LoggerTickRanges;
  static std::string const LoggerFirstTick;
  static std::string const LoggerFollow;
  static std::string const Batch;
  static std::string const Barrier;
  static std::string const Inventory;
  static std::string const Keys;
  static std::string const Dump;
  static std::string const RestoreCollection;
  static std::string const RestoreIndexes;
  static std::string const RestoreData;
  static std::string const RestoreView;
  static std::string const Sync;
  static std::string const MakeFollower;
  static std::string const ServerId;
  static std::string const ApplierConfig;
  static std::string const ApplierStart;
  static std::string const ApplierStop;
  static std::string const ApplierState;
  static std::string const ApplierStateAll;
  static std::string const ClusterInventory;
  static std::string const AddFollower;
  static std::string const RemoveFollower;
  static std::string const SetTheLeader;
  static std::string const HoldReadLockCollection;

 protected:
  ResultT<std::pair<std::string, bool>> forwardingTarget() override final;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief creates an error if called on a coordinator server
  //////////////////////////////////////////////////////////////////////////////

  bool isCoordinatorError();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief turn the server into a follower of another
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandMakeFollower();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief forward a command in the coordinator case
  //////////////////////////////////////////////////////////////////////////////

  void handleUnforwardedTrampolineCoordinator();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief returns the cluster inventory, only on coordinator
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandClusterInventory();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief handle a restore command for a specific collection
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandRestoreCollection();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief handle a restore command for a specific collection
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandRestoreIndexes();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief handle a restore command for a specific collection
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandRestoreData();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief handle a restore of all views for this collection
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandRestoreView();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief handle a server-id command
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandServerId();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief handle a sync command
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandSync();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief return the configuration of the the replication applier
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandApplierGetConfig();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief configure the replication applier
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandApplierSetConfig();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief start the replication applier
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandApplierStart();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief stop the replication applier
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandApplierStop();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief return the state of the replication applier
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandApplierGetState();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief return the state of the all replication applier
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandApplierGetStateAll();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief delete the replication applier state
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandApplierDeleteState();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief add a follower of a shard to the list of followers
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandAddFollower();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief remove a follower of a shard from the list of followers
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandRemoveFollower();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief update the leader of a shard
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandSetTheLeader();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief hold a read lock on a collection to stop writes temporarily
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandHoldReadLockCollection();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief cancel holding a read lock on a collection
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandCancelHoldReadLockCollection();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief get an ID for a hold read lock job
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandGetIdForReadLockCollection();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief return the state of the replication logger
  /// @route GET logger-state
  /// @caller Syncer::getLeaderState
  /// @response VPackObject describing the ServerState in a certain point
  ///           * state (server state)
  ///           * server (version / id)
  ///           * clients (list of followers)
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandLoggerState();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief return the first tick available in a logfile
  /// @route GET logger-first-tick
  /// @caller js/client/modules/@arangodb/replication.js
  /// @response VPackObject with minTick of LogfileManager->ranges()
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandLoggerFirstTick();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief return the available logfile range
  /// @route GET logger-tick-ranges
  /// @caller js/client/modules/@arangodb/replication.js
  /// @response VPackArray, containing info about each datafile
  ///           * filename
  ///           * status
  ///           * tickMin - tickMax
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandLoggerTickRanges();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief rebuild the revision tree for a given collection, if allowed
  /// @response 204 No Content if all goes well
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandRebuildRevisionTree();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief return the requested revision ranges for a given collection, if
  ///        available
  /// @response VPackObject, containing
  ///           * ranges, VPackArray of VPackArray of revisions
  ///           * resume, optional, if response is chunked; revision resume
  ///                     point to specify on subsequent requests
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandRevisionRanges();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief return the requested documents from a given collection, if
  ///        available
  /// @response VPackArray, containing VPackObject documents or errors
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandRevisionDocuments();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief determine chunk size from request
  ///        Reads chunkSize attribute from request
  //////////////////////////////////////////////////////////////////////////////

  uint64_t determineChunkSize() const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief Get correct replication applier, based on global paramerter
  //////////////////////////////////////////////////////////////////////////////
  ReplicationApplier* getApplier(bool& global);

 protected:
  struct RevisionOperationContext {
    uint64_t batchId;
    RevisionId resume;
    std::string cname;
    std::shared_ptr<LogicalCollection> collection;
    std::unique_ptr<ReplicationIterator> iter;
  };

  bool prepareRevisionOperation(RevisionOperationContext&);

 private:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief restores the structure of a collection
  //////////////////////////////////////////////////////////////////////////////

  Result processRestoreCollection(VPackSlice const&, bool overwrite, bool force,
                                  bool ignoreDistributeShardsLikeErrors);

  /// @brief helper function for processRestoreCoordinatorAnalyzersBatch() and
  /// processRestoreUsersBatch().
  Result parseBatchForSystemCollection(
      std::string const& collectionName, VPackBuilder& documentsToInsert,
      std::unordered_set<std::string>& documentsToRemove,
      bool generateNewRevisionIds);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief restores the data of the _analyzers collection in cluster
  //////////////////////////////////////////////////////////////////////////////

  Result processRestoreCoordinatorAnalyzersBatch(bool generateNewRevisionIds);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief restores the data of the _users collection
  //////////////////////////////////////////////////////////////////////////////

  Result processRestoreUsersBatch(bool generateNewRevisionIds);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief restores the data of a collection
  //////////////////////////////////////////////////////////////////////////////

  Result processRestoreDataBatch(transaction::Methods& trx,
                                 std::string const& colName,
                                 bool generateNewRevisionIds);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief restores the indexes of a collection
  //////////////////////////////////////////////////////////////////////////////

  Result processRestoreIndexes(VPackSlice const&, bool);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief restores the indexes of a collection, coordinator case
  //////////////////////////////////////////////////////////////////////////////

  Result processRestoreIndexesCoordinator(VPackSlice const&, bool);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief restores the data of a collection
  //////////////////////////////////////////////////////////////////////////////

  Result processRestoreData(std::string const& colName);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief parse an input batch
  //////////////////////////////////////////////////////////////////////////////

  Result parseBatch(transaction::Methods& trx,
                    std::string const& collectionName,
                    VPackBuilder& documentToInsert,
                    std::unordered_set<std::string>& documentsToRemove,
                    bool generateNewRevisionIds);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief creates a collection, based on the VelocyPack provided
  //////////////////////////////////////////////////////////////////////////////

  ErrorCode createCollection(VPackSlice slice);

 private:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief minimum chunk size
  //////////////////////////////////////////////////////////////////////////////

  static uint64_t const _defaultChunkSize;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief maximum chunk size
  //////////////////////////////////////////////////////////////////////////////

  static uint64_t const _maxChunkSize;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief timeout for tombstones
  //////////////////////////////////////////////////////////////////////////////

  static std::chrono::hours const _tombstoneTimeout;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief lock for the tombstone list
  ///        I do not think that this will ever be a bottleneck,
  ///        if it is we can easily make one lock per vocbase by
  ///        modifying the tombstones map.
  //////////////////////////////////////////////////////////////////////////////

  static basics::ReadWriteLock _tombLock;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief tombstones, should only be used, if a lock is cancelled
  ///        before it was actually registered and therefor only seldomly
  //////////////////////////////////////////////////////////////////////////////

  static std::unordered_map<std::string,
                            std::chrono::time_point<std::chrono::steady_clock>>
      _tombstones;

 protected:
  //////////////////////////////////////////////////////////////////////////////
  /// SECTION:
  /// Functions to be implemented by specialization
  //////////////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////////////
  /// @brief handle a follow command for the replication log
  //////////////////////////////////////////////////////////////////////////////

  virtual void handleCommandLoggerFollow() = 0;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief handle the command to determine the transactions that were open
  /// at a certain point in time
  //////////////////////////////////////////////////////////////////////////////

  virtual void handleCommandDetermineOpenTransactions() = 0;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief handle a batch command
  //////////////////////////////////////////////////////////////////////////////

  virtual void handleCommandBatch() = 0;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief add or remove a WAL logfile barrier
  //////////////////////////////////////////////////////////////////////////////

  virtual void handleCommandBarrier() = 0;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief return the inventory (current replication and collection state)
  //////////////////////////////////////////////////////////////////////////////

  virtual void handleCommandInventory() = 0;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief produce list of keys for a specific collection
  //////////////////////////////////////////////////////////////////////////////

  virtual void handleCommandCreateKeys() = 0;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief returns a key range
  //////////////////////////////////////////////////////////////////////////////

  virtual void handleCommandGetKeys() = 0;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief returns date for a key range
  //////////////////////////////////////////////////////////////////////////////

  virtual void handleCommandFetchKeys() = 0;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief remove a list of keys for a specific collection
  //////////////////////////////////////////////////////////////////////////////

  virtual void handleCommandRemoveKeys() = 0;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief handle a dump command for a specific collection
  //////////////////////////////////////////////////////////////////////////////

  virtual void handleCommandDump() = 0;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief return the revision tree for a given collection, if available
  //////////////////////////////////////////////////////////////////////////////

  virtual void handleCommandRevisionTree() = 0;

 private:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief Cleanup tombstones that have expired
  //////////////////////////////////////////////////////////////////////////////
  void timeoutTombstones() const;

  bool isTombstoned(TransactionId id) const;

  void registerTombstone(TransactionId id) const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief Create a blocking transaction for the given collectionName,
  ///        It will be registered with the given id, and it will have
  ///        the given time to live.
  //////////////////////////////////////////////////////////////////////////////
  Result createBlockingTransaction(TransactionId tid, LogicalCollection& col,
                                   double ttl, AccessMode::Type access,
                                   RebootId const& rebootId,
                                   std::string const& serverId);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief Test if we already have the read-lock
  ///        Will return true, if we have it and can use it
  ///        Will return false, if we are still in the process of getting it.
  ///        Will return error, if the lock has expired.
  //////////////////////////////////////////////////////////////////////////////

  Result isLockHeld(TransactionId tid) const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief compute a local checksum for the given collection
  ///        Will return error if the lock has expired.
  //////////////////////////////////////////////////////////////////////////////

  ResultT<std::string> computeCollectionChecksum(TransactionId readLockId,
                                                 LogicalCollection* col) const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief Cacnel the lock with the given id
  ///        Will return true, if we did have the lock
  ///        Will return false, if we were still in the process of getting it.
  ///        Will return error if the lock has expired or is not found.
  //////////////////////////////////////////////////////////////////////////////

  ResultT<bool> cancelBlockingTransaction(TransactionId id) const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief Validate that the requesting user has access rights to this route
  ///        Will return TRI_ERROR_NO_ERROR if user has access
  ///        Will return error code otherwise.
  //////////////////////////////////////////////////////////////////////////////
  Result testPermissions();
};
}  // namespace arangodb

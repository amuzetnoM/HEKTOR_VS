#pragma once

#include <napi.h>
#include "common.h"
#include <memory>

// Forward declarations
namespace vdb {
    class ReplicationManager;
    class ShardingManager;
    class DistributedVectorDatabase;
}

namespace hektor_native {

// ReplicationManager wrapper
class ReplicationManagerWrap : public Napi::ObjectWrap<ReplicationManagerWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    ReplicationManagerWrap(const Napi::CallbackInfo& info);
    
private:
    // Lifecycle
    Napi::Value Start(const Napi::CallbackInfo& info);
    Napi::Value Stop(const Napi::CallbackInfo& info);
    
    // Replication
    Napi::Value ReplicateAdd(const Napi::CallbackInfo& info);
    Napi::Value ReplicateRemove(const Napi::CallbackInfo& info);
    Napi::Value ReplicateUpdate(const Napi::CallbackInfo& info);
    
    // Node management
    Napi::Value AddReplica(const Napi::CallbackInfo& info);
    Napi::Value RemoveReplica(const Napi::CallbackInfo& info);
    Napi::Value GetReplicas(const Napi::CallbackInfo& info);
    
    // Health
    Napi::Value IsHealthy(const Napi::CallbackInfo& info);
    Napi::Value GetPrimaryNode(const Napi::CallbackInfo& info);
    Napi::Value PromoteToPrimary(const Napi::CallbackInfo& info);
    Napi::Value DemoteFromPrimary(const Napi::CallbackInfo& info);
    
    // Failover
    Napi::Value TriggerFailover(const Napi::CallbackInfo& info);
    Napi::Value SetFailoverCallback(const Napi::CallbackInfo& info);
    
    std::unique_ptr<vdb::ReplicationManager> manager_;
    Napi::FunctionReference failover_callback_;
};

// ShardingManager wrapper
class ShardingManagerWrap : public Napi::ObjectWrap<ShardingManagerWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    ShardingManagerWrap(const Napi::CallbackInfo& info);
    
private:
    // Lifecycle
    Napi::Value Start(const Napi::CallbackInfo& info);
    Napi::Value Stop(const Napi::CallbackInfo& info);
    
    // Routing
    Napi::Value GetShardForId(const Napi::CallbackInfo& info);
    Napi::Value GetShardForKey(const Napi::CallbackInfo& info);
    Napi::Value GetAllShards(const Napi::CallbackInfo& info);
    
    // Management
    Napi::Value AddShard(const Napi::CallbackInfo& info);
    Napi::Value RemoveShard(const Napi::CallbackInfo& info);
    Napi::Value RebalanceShards(const Napi::CallbackInfo& info);
    
    // Statistics
    Napi::Value GetShardSize(const Napi::CallbackInfo& info);
    Napi::Value GetShardImbalance(const Napi::CallbackInfo& info);
    Napi::Value NeedsResharding(const Napi::CallbackInfo& info);
    
    // Resharding
    Napi::Value TriggerResharding(const Napi::CallbackInfo& info);
    Napi::Value SetReshardingCallback(const Napi::CallbackInfo& info);
    
    std::unique_ptr<vdb::ShardingManager> manager_;
    Napi::FunctionReference resharding_callback_;
};

// DistributedVectorDatabase wrapper
class DistributedVectorDatabaseWrap : public Napi::ObjectWrap<DistributedVectorDatabaseWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    DistributedVectorDatabaseWrap(const Napi::CallbackInfo& info);
    
private:
    // Initialization
    Napi::Value Init(const Napi::CallbackInfo& info);
    Napi::Value Close(const Napi::CallbackInfo& info);
    
    // Operations
    Napi::Value Add(const Napi::CallbackInfo& info);
    Napi::Value Remove(const Napi::CallbackInfo& info);
    Napi::Value Get(const Napi::CallbackInfo& info);
    Napi::Value UpdateMetadata(const Napi::CallbackInfo& info);
    Napi::Value Search(const Napi::CallbackInfo& info);
    
    // Cluster
    Napi::Value AddNode(const Napi::CallbackInfo& info);
    Napi::Value RemoveNode(const Napi::CallbackInfo& info);
    Napi::Value GetAllNodes(const Napi::CallbackInfo& info);
    Napi::Value IsClusterHealthy(const Napi::CallbackInfo& info);
    
    std::unique_ptr<vdb::DistributedVectorDatabase> db_;
};

} // namespace hektor_native

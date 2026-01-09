import { Component, inject, signal, OnInit } from '@angular/core';
import { CommonModule } from '@angular/common';
import { VectorDbService } from '../services/vector-db.service';
import { ClusterInfo, NodeInfo } from '../models/core';

@Component({
    selector: 'app-cluster-view',
    standalone: true,
    imports: [CommonModule],
    template: `
    <div class="h-full overflow-y-auto custom-scrollbar bg-[#09090b]">
      <div class="max-w-5xl mx-auto p-8 space-y-8">
        <!-- Header -->
        <div class="flex items-center justify-between">
          <div>
            <h2 class="text-2xl font-bold text-white mb-2">Cluster Overview</h2>
            <p class="text-zinc-500">Monitor replication and sharding status</p>
          </div>
          <button 
            (click)="refreshClusterInfo()"
            class="px-4 py-2 bg-white/5 hover:bg-white/10 text-zinc-300 text-sm font-medium rounded-lg transition-colors border border-white/5 flex items-center gap-2"
          >
            <svg xmlns="http://www.w3.org/2000/svg" class="h-4 w-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15" />
            </svg>
            Refresh
          </button>
        </div>

        @if (clusterInfo()) {
          <!-- Cluster Health Banner -->
          <div 
            class="p-4 rounded-xl border flex items-center gap-4"
            [class.bg-emerald-500/5]="clusterInfo()!.isHealthy"
            [class.border-emerald-500/20]="clusterInfo()!.isHealthy"
            [class.bg-rose-500/5]="!clusterInfo()!.isHealthy"
            [class.border-rose-500/20]="!clusterInfo()!.isHealthy"
          >
            <div 
              class="w-4 h-4 rounded-full"
              [class.bg-emerald-500]="clusterInfo()!.isHealthy"
              [class.bg-rose-500]="!clusterInfo()!.isHealthy"
            ></div>
            <div>
              <div 
                class="font-semibold"
                [class.text-emerald-400]="clusterInfo()!.isHealthy"
                [class.text-rose-400]="!clusterInfo()!.isHealthy"
              >
                Cluster {{ clusterInfo()!.isHealthy ? 'Healthy' : 'Unhealthy' }}
              </div>
              <div class="text-sm text-zinc-500">
                {{ clusterInfo()!.nodes.length }} nodes • 
                {{ clusterInfo()!.sharding.shardCount }} shards
              </div>
            </div>
          </div>

          <div class="grid grid-cols-1 lg:grid-cols-2 gap-8">
            <!-- Replication Status -->
            <section class="rounded-xl border border-white/5 bg-white/[0.02] overflow-hidden">
              <div class="px-6 py-4 border-b border-white/5">
                <h3 class="font-semibold text-white">Replication</h3>
              </div>
              <div class="p-6 space-y-4">
                <div class="grid grid-cols-2 gap-4">
                  <div>
                    <div class="text-xs text-zinc-500 uppercase tracking-wider mb-1">Mode</div>
                    <div class="text-lg font-mono text-indigo-400 uppercase">
                      {{ clusterInfo()!.replication.mode }}
                    </div>
                  </div>
                  @if (clusterInfo()!.replication.primaryNode) {
                    <div>
                      <div class="text-xs text-zinc-500 uppercase tracking-wider mb-1">Primary</div>
                      <div class="text-sm font-mono text-white">
                        {{ clusterInfo()!.replication.primaryNode }}
                      </div>
                    </div>
                  }
                  @if (clusterInfo()!.replication.lagMs !== undefined) {
                    <div>
                      <div class="text-xs text-zinc-500 uppercase tracking-wider mb-1">Replication Lag</div>
                      <div 
                        class="text-lg font-mono"
                        [class.text-emerald-400]="clusterInfo()!.replication.lagMs! < 100"
                        [class.text-amber-400]="clusterInfo()!.replication.lagMs! >= 100 && clusterInfo()!.replication.lagMs! < 1000"
                        [class.text-rose-400]="clusterInfo()!.replication.lagMs! >= 1000"
                      >
                        {{ clusterInfo()!.replication.lagMs }}ms
                      </div>
                    </div>
                  }
                  <div>
                    <div class="text-xs text-zinc-500 uppercase tracking-wider mb-1">Replicas</div>
                    <div class="text-lg font-mono text-white">
                      {{ clusterInfo()!.replication.replicas.length }}
                    </div>
                  </div>
                </div>

                @if (clusterInfo()!.replication.replicas.length > 0) {
                  <div class="pt-4 border-t border-white/5 space-y-2">
                    @for (replica of clusterInfo()!.replication.replicas; track replica.nodeId) {
                      <div class="flex items-center justify-between p-2 bg-white/[0.02] rounded-lg">
                        <div class="flex items-center gap-2">
                          <div 
                            class="w-2 h-2 rounded-full"
                            [class.bg-emerald-500]="replica.isHealthy"
                            [class.bg-rose-500]="!replica.isHealthy"
                          ></div>
                          <span class="text-sm font-mono text-zinc-300">{{ replica.nodeId }}</span>
                        </div>
                        @if (replica.replicationLag !== undefined) {
                          <span class="text-xs text-zinc-500">{{ replica.replicationLag }}ms lag</span>
                        }
                      </div>
                    }
                  </div>
                }
              </div>
            </section>

            <!-- Sharding Status -->
            <section class="rounded-xl border border-white/5 bg-white/[0.02] overflow-hidden">
              <div class="px-6 py-4 border-b border-white/5">
                <h3 class="font-semibold text-white">Sharding</h3>
              </div>
              <div class="p-6 space-y-4">
                <div class="grid grid-cols-2 gap-4">
                  <div>
                    <div class="text-xs text-zinc-500 uppercase tracking-wider mb-1">Strategy</div>
                    <div class="text-lg font-mono text-violet-400 uppercase">
                      {{ clusterInfo()!.sharding.strategy }}
                    </div>
                  </div>
                  <div>
                    <div class="text-xs text-zinc-500 uppercase tracking-wider mb-1">Shards</div>
                    <div class="text-lg font-mono text-white">
                      {{ clusterInfo()!.sharding.shardCount }}
                    </div>
                  </div>
                  <div>
                    <div class="text-xs text-zinc-500 uppercase tracking-wider mb-1">Imbalance</div>
                    <div 
                      class="text-lg font-mono"
                      [class.text-emerald-400]="clusterInfo()!.sharding.imbalance < 0.1"
                      [class.text-amber-400]="clusterInfo()!.sharding.imbalance >= 0.1 && clusterInfo()!.sharding.imbalance < 0.2"
                      [class.text-rose-400]="clusterInfo()!.sharding.imbalance >= 0.2"
                    >
                      {{ (clusterInfo()!.sharding.imbalance * 100).toFixed(1) }}%
                    </div>
                  </div>
                  <div>
                    <div class="text-xs text-zinc-500 uppercase tracking-wider mb-1">Resharding</div>
                    <div class="text-sm font-mono" [class.text-amber-400]="clusterInfo()!.sharding.needsResharding" [class.text-zinc-400]="!clusterInfo()!.sharding.needsResharding">
                      {{ clusterInfo()!.sharding.needsResharding ? 'Recommended' : 'Not needed' }}
                    </div>
                  </div>
                </div>

                @if (clusterInfo()!.sharding.shards.length > 0) {
                  <div class="pt-4 border-t border-white/5 space-y-2">
                    <div class="text-xs text-zinc-500 uppercase tracking-wider mb-2">Shard Distribution</div>
                    @for (shard of clusterInfo()!.sharding.shards; track shard.shardId) {
                      <div class="flex items-center justify-between p-2 bg-white/[0.02] rounded-lg">
                        <span class="text-sm font-mono text-zinc-300">{{ shard.shardId }}</span>
                        <div class="flex items-center gap-4">
                          <span class="text-xs text-zinc-500">{{ shard.vectorCount | number }} vectors</span>
                          <span class="text-xs text-zinc-600">{{ shard.nodeId }}</span>
                        </div>
                      </div>
                    }
                  </div>
                }
              </div>
            </section>
          </div>

          <!-- Nodes Table -->
          <section class="rounded-xl border border-white/5 bg-white/[0.02] overflow-hidden">
            <div class="px-6 py-4 border-b border-white/5">
              <h3 class="font-semibold text-white">Cluster Nodes</h3>
            </div>
            <div class="overflow-x-auto">
              <table class="w-full text-left text-sm">
                <thead class="bg-zinc-900/50 text-zinc-500 uppercase text-xs tracking-wider">
                  <tr>
                    <th class="px-6 py-3 font-medium">Node ID</th>
                    <th class="px-6 py-3 font-medium">Host</th>
                    <th class="px-6 py-3 font-medium">Port</th>
                    <th class="px-6 py-3 font-medium">Role</th>
                    <th class="px-6 py-3 font-medium">Status</th>
                  </tr>
                </thead>
                <tbody class="divide-y divide-white/5">
                  @for (node of clusterInfo()!.nodes; track node.nodeId) {
                    <tr class="hover:bg-white/[0.02] transition-colors">
                      <td class="px-6 py-4 font-mono text-zinc-300">{{ node.nodeId }}</td>
                      <td class="px-6 py-4 font-mono text-zinc-400">{{ node.host }}</td>
                      <td class="px-6 py-4 font-mono text-zinc-400">{{ node.port }}</td>
                      <td class="px-6 py-4">
                        @if (node.isPrimary) {
                          <span class="px-2 py-1 bg-indigo-500/10 text-indigo-400 text-xs rounded-full">Primary</span>
                        } @else {
                          <span class="px-2 py-1 bg-zinc-800 text-zinc-400 text-xs rounded-full">Replica</span>
                        }
                      </td>
                      <td class="px-6 py-4">
                        <div class="flex items-center gap-2">
                          <div 
                            class="w-2 h-2 rounded-full"
                            [class.bg-emerald-500]="node.isHealthy"
                            [class.bg-rose-500]="!node.isHealthy"
                          ></div>
                          <span [class.text-emerald-400]="node.isHealthy" [class.text-rose-400]="!node.isHealthy">
                            {{ node.isHealthy ? 'Healthy' : 'Unhealthy' }}
                          </span>
                        </div>
                      </td>
                    </tr>
                  }
                </tbody>
              </table>
            </div>
          </section>
        } @else {
          <!-- Not Configured -->
          <div class="rounded-xl border border-white/5 bg-white/[0.02] p-12 text-center">
            <svg xmlns="http://www.w3.org/2000/svg" class="h-16 w-16 mx-auto text-zinc-700 mb-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="1" d="M19 11H5m14 0a2 2 0 012 2v6a2 2 0 01-2 2H5a2 2 0 01-2-2v-6a2 2 0 012-2m14 0V9a2 2 0 00-2-2M5 11V9a2 2 0 012-2m0 0V5a2 2 0 012-2h6a2 2 0 012 2v2M7 7h10" />
            </svg>
            <h3 class="text-xl font-semibold text-zinc-400 mb-2">Single Node Mode</h3>
            <p class="text-zinc-600 max-w-md mx-auto">
              Distributed features are not configured. The database is running in single-node mode.
              Enable replication and sharding for high availability and horizontal scaling.
            </p>
          </div>
        }
      </div>
    </div>
  `
})
export class ClusterViewComponent implements OnInit {
    private db = inject(VectorDbService);

    clusterInfo = signal<ClusterInfo | null>(null);

    ngOnInit() {
        this.refreshClusterInfo();
    }

    async refreshClusterInfo() {
        try {
            const info = await this.db.getClusterInfo();
            this.clusterInfo.set(info);
        } catch (error) {
            console.error('Failed to fetch cluster info:', error);
            // Set null to show "not configured" state
            this.clusterInfo.set(null);
        }
    }
}

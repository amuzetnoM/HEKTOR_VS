import { Component, inject, signal, Input, OnInit } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { VectorDbService } from '../services/vector-db.service';
import { IndexType, IndexStats, BenchmarkResult } from '../models/core';

@Component({
    selector: 'app-index-manager',
    standalone: true,
    imports: [CommonModule, FormsModule],
    template: `
    <div class="h-full overflow-y-auto custom-scrollbar bg-[#09090b]">
      <div class="max-w-4xl mx-auto p-8 space-y-8">
        <!-- Header -->
        <div>
          <h2 class="text-2xl font-bold text-white mb-2">Index Management</h2>
          <p class="text-zinc-500">Configure and optimize your vector index for optimal search performance</p>
        </div>

        <!-- Current Index Stats -->
        <section class="rounded-xl border border-white/5 bg-white/[0.02] overflow-hidden">
          <div class="px-6 py-4 border-b border-white/5 flex items-center justify-between">
            <h3 class="font-semibold text-white">Index Statistics</h3>
            <button 
              (click)="refreshStats()"
              class="text-xs text-zinc-400 hover:text-white transition-colors flex items-center gap-1"
            >
              <svg xmlns="http://www.w3.org/2000/svg" class="h-4 w-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15" />
              </svg>
              Refresh
            </button>
          </div>
          
          @if (stats()) {
            <div class="p-6 grid grid-cols-2 md:grid-cols-4 gap-6">
              <div>
                <div class="text-xs text-zinc-500 uppercase tracking-wider mb-1">Type</div>
                <div class="text-lg font-mono text-white uppercase">{{ stats()!.type }}</div>
              </div>
              <div>
                <div class="text-xs text-zinc-500 uppercase tracking-wider mb-1">Vectors</div>
                <div class="text-lg font-mono text-white">{{ stats()!.totalVectors | number }}</div>
              </div>
              <div>
                <div class="text-xs text-zinc-500 uppercase tracking-wider mb-1">Index Size</div>
                <div class="text-lg font-mono text-white">{{ formatBytes(stats()!.indexSize) }}</div>
              </div>
              <div>
                <div class="text-xs text-zinc-500 uppercase tracking-wider mb-1">Memory</div>
                <div class="text-lg font-mono text-white">{{ formatBytes(stats()!.memoryUsage) }}</div>
              </div>
              @if (stats()!.hnswM) {
                <div>
                  <div class="text-xs text-zinc-500 uppercase tracking-wider mb-1">HNSW M</div>
                  <div class="text-lg font-mono text-white">{{ stats()!.hnswM }}</div>
                </div>
              }
              @if (stats()!.hnswEfConstruction) {
                <div>
                  <div class="text-xs text-zinc-500 uppercase tracking-wider mb-1">ef_construction</div>
                  <div class="text-lg font-mono text-white">{{ stats()!.hnswEfConstruction }}</div>
                </div>
              }
              <div>
                <div class="text-xs text-zinc-500 uppercase tracking-wider mb-1">Optimized</div>
                <div class="text-lg font-mono" [class.text-emerald-400]="stats()!.optimized" [class.text-amber-400]="!stats()!.optimized">
                  {{ stats()!.optimized ? 'Yes' : 'No' }}
                </div>
              </div>
              @if (stats()!.buildTimestamp) {
                <div>
                  <div class="text-xs text-zinc-500 uppercase tracking-wider mb-1">Built</div>
                  <div class="text-sm font-mono text-zinc-400">{{ stats()!.buildTimestamp }}</div>
                </div>
              }
            </div>
          } @else {
            <div class="p-6 text-center text-zinc-500">
              <p>No index statistics available</p>
            </div>
          }
        </section>

        <!-- Build/Rebuild Index -->
        <section class="rounded-xl border border-white/5 bg-white/[0.02] overflow-hidden">
          <div class="px-6 py-4 border-b border-white/5">
            <h3 class="font-semibold text-white">Build Index</h3>
          </div>
          <div class="p-6 space-y-6">
            <div class="grid grid-cols-1 md:grid-cols-2 gap-6">
              <!-- Index Type -->
              <div>
                <label class="block text-xs font-bold text-zinc-500 uppercase tracking-wider mb-2">Index Type</label>
                <select 
                  [(ngModel)]="buildConfig.type"
                  class="w-full bg-zinc-900/50 border border-white/10 rounded-lg px-4 py-2.5 text-white text-sm focus:outline-none focus:border-indigo-500/50"
                >
                  <option value="hnsw">HNSW (Recommended)</option>
                  <option value="flat">Flat (Exact Search)</option>
                </select>
              </div>

              <!-- HNSW M -->
              <div>
                <label class="block text-xs font-bold text-zinc-500 uppercase tracking-wider mb-2">
                  HNSW M Parameter
                  <span class="text-zinc-600 normal-case font-normal ml-1">(connections per node)</span>
                </label>
                <input 
                  type="number" 
                  [(ngModel)]="buildConfig.hnswM"
                  [disabled]="buildConfig.type !== 'hnsw'"
                  min="4" 
                  max="64"
                  class="w-full bg-zinc-900/50 border border-white/10 rounded-lg px-4 py-2.5 text-white text-sm focus:outline-none focus:border-indigo-500/50 disabled:opacity-50"
                />
              </div>

              <!-- HNSW ef_construction -->
              <div>
                <label class="block text-xs font-bold text-zinc-500 uppercase tracking-wider mb-2">
                  ef_construction
                  <span class="text-zinc-600 normal-case font-normal ml-1">(build quality)</span>
                </label>
                <input 
                  type="number" 
                  [(ngModel)]="buildConfig.hnswEfConstruction"
                  [disabled]="buildConfig.type !== 'hnsw'"
                  min="50" 
                  max="500"
                  class="w-full bg-zinc-900/50 border border-white/10 rounded-lg px-4 py-2.5 text-white text-sm focus:outline-none focus:border-indigo-500/50 disabled:opacity-50"
                />
              </div>

              <!-- HNSW ef_search -->
              <div>
                <label class="block text-xs font-bold text-zinc-500 uppercase tracking-wider mb-2">
                  ef_search
                  <span class="text-zinc-600 normal-case font-normal ml-1">(search quality)</span>
                </label>
                <input 
                  type="number" 
                  [(ngModel)]="buildConfig.hnswEfSearch"
                  [disabled]="buildConfig.type !== 'hnsw'"
                  min="10" 
                  max="500"
                  class="w-full bg-zinc-900/50 border border-white/10 rounded-lg px-4 py-2.5 text-white text-sm focus:outline-none focus:border-indigo-500/50 disabled:opacity-50"
                />
              </div>
            </div>

            <!-- Build Progress -->
            @if (buildProgress() !== null) {
              <div class="space-y-2">
                <div class="flex justify-between text-xs text-zinc-400">
                  <span>Building index...</span>
                  <span>{{ buildProgress() }}%</span>
                </div>
                <div class="h-2 bg-zinc-800 rounded-full overflow-hidden">
                  <div 
                    class="h-full bg-gradient-to-r from-indigo-500 to-violet-500 transition-all duration-300"
                    [style.width.%]="buildProgress()"
                  ></div>
                </div>
              </div>
            }

            <div class="flex gap-4">
              <button 
                (click)="buildIndex()"
                [disabled]="isBuilding()"
                class="px-5 py-2.5 bg-indigo-600 hover:bg-indigo-500 disabled:bg-zinc-800 disabled:text-zinc-500 text-white text-sm font-medium rounded-lg transition-colors"
              >
                {{ isBuilding() ? 'Building...' : 'Build Index' }}
              </button>
              <button 
                (click)="optimizeIndex()"
                [disabled]="isOptimizing()"
                class="px-5 py-2.5 bg-white/5 hover:bg-white/10 text-zinc-300 text-sm font-medium rounded-lg transition-colors border border-white/5"
              >
                {{ isOptimizing() ? 'Optimizing...' : 'Optimize' }}
              </button>
            </div>
          </div>
        </section>

        <!-- Benchmark -->
        <section class="rounded-xl border border-white/5 bg-white/[0.02] overflow-hidden">
          <div class="px-6 py-4 border-b border-white/5">
            <h3 class="font-semibold text-white">Performance Benchmark</h3>
          </div>
          <div class="p-6 space-y-6">
            <div class="flex items-end gap-4">
              <div class="flex-1">
                <label class="block text-xs font-bold text-zinc-500 uppercase tracking-wider mb-2">Number of Queries</label>
                <input 
                  type="number" 
                  [(ngModel)]="benchmarkQueries"
                  min="10" 
                  max="10000"
                  class="w-full bg-zinc-900/50 border border-white/10 rounded-lg px-4 py-2.5 text-white text-sm focus:outline-none focus:border-indigo-500/50"
                />
              </div>
              <button 
                (click)="runBenchmark()"
                [disabled]="isBenchmarking()"
                class="px-5 py-2.5 bg-emerald-600 hover:bg-emerald-500 disabled:bg-zinc-800 disabled:text-zinc-500 text-white text-sm font-medium rounded-lg transition-colors"
              >
                {{ isBenchmarking() ? 'Running...' : 'Run Benchmark' }}
              </button>
            </div>

            @if (benchmarkResult()) {
              <div class="grid grid-cols-2 md:grid-cols-3 gap-4 pt-4 border-t border-white/5">
                <div class="p-4 bg-white/[0.02] rounded-lg border border-white/5">
                  <div class="text-xs text-zinc-500 uppercase tracking-wider mb-1">Avg Latency</div>
                  <div class="text-xl font-mono text-indigo-400">{{ benchmarkResult()!.avgLatencyMs.toFixed(2) }}ms</div>
                </div>
                <div class="p-4 bg-white/[0.02] rounded-lg border border-white/5">
                  <div class="text-xs text-zinc-500 uppercase tracking-wider mb-1">P95 Latency</div>
                  <div class="text-xl font-mono text-amber-400">{{ benchmarkResult()!.p95LatencyMs.toFixed(2) }}ms</div>
                </div>
                <div class="p-4 bg-white/[0.02] rounded-lg border border-white/5">
                  <div class="text-xs text-zinc-500 uppercase tracking-wider mb-1">P99 Latency</div>
                  <div class="text-xl font-mono text-rose-400">{{ benchmarkResult()!.p99LatencyMs.toFixed(2) }}ms</div>
                </div>
                <div class="p-4 bg-white/[0.02] rounded-lg border border-white/5">
                  <div class="text-xs text-zinc-500 uppercase tracking-wider mb-1">Throughput</div>
                  <div class="text-xl font-mono text-emerald-400">{{ benchmarkResult()!.throughputQps.toFixed(0) }} QPS</div>
                </div>
                <div class="p-4 bg-white/[0.02] rounded-lg border border-white/5">
                  <div class="text-xs text-zinc-500 uppercase tracking-wider mb-1">Queries Run</div>
                  <div class="text-xl font-mono text-white">{{ benchmarkResult()!.queriesRun }}</div>
                </div>
                @if (benchmarkResult()!.recall !== undefined) {
                  <div class="p-4 bg-white/[0.02] rounded-lg border border-white/5">
                    <div class="text-xs text-zinc-500 uppercase tracking-wider mb-1">Recall@10</div>
                    <div class="text-xl font-mono text-violet-400">{{ (benchmarkResult()!.recall! * 100).toFixed(1) }}%</div>
                  </div>
                }
              </div>
            }
          </div>
        </section>
      </div>
    </div>
  `
})
export class IndexManagerComponent implements OnInit {
    @Input() collectionName = '';

    private db = inject(VectorDbService);

    stats = signal<IndexStats | null>(null);
    benchmarkResult = signal<BenchmarkResult | null>(null);

    isBuilding = signal(false);
    isOptimizing = signal(false);
    isBenchmarking = signal(false);
    buildProgress = signal<number | null>(null);

    buildConfig = {
        type: 'hnsw' as IndexType,
        hnswM: 16,
        hnswEfConstruction: 200,
        hnswEfSearch: 50
    };

    benchmarkQueries = 100;

    ngOnInit() {
        this.refreshStats();
    }

    async refreshStats() {
        try {
            const indexStats = await this.db.getIndexStats(this.collectionName);
            this.stats.set(indexStats);
        } catch (error) {
            console.error('Failed to fetch index stats:', error);
        }
    }

    async buildIndex() {
        this.isBuilding.set(true);
        this.buildProgress.set(0);

        try {
            // Simulate progress updates
            const progressInterval = setInterval(() => {
                const current = this.buildProgress() ?? 0;
                if (current < 90) {
                    this.buildProgress.set(current + Math.random() * 15);
                }
            }, 500);

            await this.db.buildIndex(this.collectionName, this.buildConfig);

            clearInterval(progressInterval);
            this.buildProgress.set(100);

            await this.refreshStats();

            setTimeout(() => this.buildProgress.set(null), 1000);
        } catch (error) {
            console.error('Failed to build index:', error);
            this.buildProgress.set(null);
        } finally {
            this.isBuilding.set(false);
        }
    }

    async optimizeIndex() {
        this.isOptimizing.set(true);
        try {
            await this.db.optimizeIndex(this.collectionName);
            await this.refreshStats();
        } catch (error) {
            console.error('Failed to optimize index:', error);
        } finally {
            this.isOptimizing.set(false);
        }
    }

    async runBenchmark() {
        this.isBenchmarking.set(true);
        this.benchmarkResult.set(null);

        try {
            const result = await this.db.benchmarkIndex(this.collectionName, this.benchmarkQueries);
            this.benchmarkResult.set(result);
        } catch (error) {
            console.error('Benchmark failed:', error);
        } finally {
            this.isBenchmarking.set(false);
        }
    }

    formatBytes(bytes: number): string {
        if (bytes === 0) return '0 B';
        const k = 1024;
        const sizes = ['B', 'KB', 'MB', 'GB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        return parseFloat((bytes / Math.pow(k, i)).toFixed(1)) + ' ' + sizes[i];
    }
}
